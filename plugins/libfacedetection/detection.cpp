/****************************************************************************************
 * Copyright (C) 2021 aistream <aistream@yeah.net>
 *
 * Licensed under the BSD 3-Clause License (the "License"); you may not use this 
 * file except in compliance with the License. You may obtain a copy of the License at
 *
 * https://opensource.org/licenses/BSD-3-Clause
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations under the License.
 *
 ***************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <opencv2/dnn.hpp>
#include "opencv2/core.hpp"
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/objdetect.hpp>
#include "share.h"
#include "tensor.h"
#include "log.h"

using namespace cv;

typedef struct {
    int id;
    bool init;
    int input_w;
    int input_h;
    std::vector<cv::Rect2f> priors;
} DetectionParams;

typedef struct {
    std::mutex mtx;
    cv::dnn::Net net;
    float score_threshold;
    float nms_threshold;
    int top_k;
} FaceEngine;

static FaceEngine* engine = NULL;
static cv::Mat postProcess(const std::vector<cv::Mat>& output_blobs, DetectionParams* detection) {
    auto priors = detection->priors;
    int inputW = detection->input_w;
    int inputH = detection->input_h;

    // Extract from output_blobs
    cv::Mat loc = output_blobs[0];
    cv::Mat conf = output_blobs[1];
    cv::Mat iou = output_blobs[2];

    // Decode from deltas and priors
    const std::vector<float> variance = {0.1f, 0.2f};
    float* loc_v = (float*)(loc.data);
    float* conf_v = (float*)(conf.data);
    float* iou_v = (float*)(iou.data);
    cv::Mat faces;
    // (tl_x, tl_y, w, h, re_x, re_y, le_x, le_y, nt_x, nt_y, rcm_x, rcm_y, lcm_x, lcm_y, score)
    // 'tl': top left point of the bounding box
    // 're': right eye, 'le': left eye
    // 'nt':  nose tip
    // 'rcm': right corner of mouth, 'lcm': left corner of mouth
    cv::Mat face(1, 15, CV_32FC1);
    for (size_t i = 0; i < priors.size(); ++i) {
        // Get score
        float clsScore = conf_v[i*2+1];
        float iouScore = iou_v[i];
        // Clamp
        if (iouScore < 0.f) {
            iouScore = 0.f;
        }
        else if (iouScore > 1.f) {
            iouScore = 1.f;
        }
        float score = std::sqrt(clsScore * iouScore);
        face.at<float>(0, 14) = score;

        // Get bounding box
        float cx = (priors[i].x + loc_v[i*14+0] * variance[0] * priors[i].width)  * inputW;
        float cy = (priors[i].y + loc_v[i*14+1] * variance[0] * priors[i].height) * inputH;
        float w  = priors[i].width  * exp(loc_v[i*14+2] * variance[0]) * inputW;
        float h  = priors[i].height * exp(loc_v[i*14+3] * variance[1]) * inputH;
        float x1 = cx - w / 2;
        float y1 = cy - h / 2;
        face.at<float>(0, 0) = x1;
        face.at<float>(0, 1) = y1;
        face.at<float>(0, 2) = w;
        face.at<float>(0, 3) = h;

        // Get landmarks
        face.at<float>(0, 4) = (priors[i].x + loc_v[i*14+ 4] * variance[0] * priors[i].width)  * inputW;  // right eye, x
        face.at<float>(0, 5) = (priors[i].y + loc_v[i*14+ 5] * variance[0] * priors[i].height) * inputH;  // right eye, y
        face.at<float>(0, 6) = (priors[i].x + loc_v[i*14+ 6] * variance[0] * priors[i].width)  * inputW;  // left eye, x
        face.at<float>(0, 7) = (priors[i].y + loc_v[i*14+ 7] * variance[0] * priors[i].height) * inputH;  // left eye, y
        face.at<float>(0, 8) = (priors[i].x + loc_v[i*14+ 8] * variance[0] * priors[i].width)  * inputW;  // nose tip, x
        face.at<float>(0, 9) = (priors[i].y + loc_v[i*14+ 9] * variance[0] * priors[i].height) * inputH;  // nose tip, y
        face.at<float>(0, 10) = (priors[i].x + loc_v[i*14+10] * variance[0] * priors[i].width)  * inputW; // right corner of mouth, x
        face.at<float>(0, 11) = (priors[i].y + loc_v[i*14+11] * variance[0] * priors[i].height) * inputH; // right corner of mouth, y
        face.at<float>(0, 12) = (priors[i].x + loc_v[i*14+12] * variance[0] * priors[i].width)  * inputW; // left corner of mouth, x
        face.at<float>(0, 13) = (priors[i].y + loc_v[i*14+13] * variance[0] * priors[i].height) * inputH; // left corner of mouth, y

        faces.push_back(face);
    }
    if (faces.rows > 1)
    {
        // Retrieve boxes and scores
        std::vector<Rect2i> faceBoxes;
        std::vector<float> faceScores;
        for (int rIdx = 0; rIdx < faces.rows; rIdx++)
        {
            faceBoxes.push_back(Rect2i(int(faces.at<float>(rIdx, 0)),
                                       int(faces.at<float>(rIdx, 1)),
                                       int(faces.at<float>(rIdx, 2)),
                                       int(faces.at<float>(rIdx, 3))));
            faceScores.push_back(faces.at<float>(rIdx, 14));
        }

        std::vector<int> keepIdx;
        dnn::NMSBoxes(faceBoxes, faceScores, engine->score_threshold, 
                engine->nms_threshold, keepIdx, 1.f, engine->top_k);

        // Get NMS results
        cv::Mat nms_faces;
        for (int idx: keepIdx)
        {
            nms_faces.push_back(faces.row(idx));
        }
        return nms_faces;
    }
    else
    {
        return faces;
    }
    return faces;
}

static void GeneratePriors(DetectionParams* detection) {
    int inputW = detection->input_w;
    int inputH = detection->input_h;
    // Calculate shapes of different scales according to the shape of input image
    Size feature_map_2nd = {
        int(int((inputW+1)/2)/2), int(int((inputH+1)/2)/2)
    };
    Size feature_map_3rd = {
        int(feature_map_2nd.width/2), int(feature_map_2nd.height/2)
    };
    Size feature_map_4th = {
        int(feature_map_3rd.width/2), int(feature_map_3rd.height/2)
    };
    Size feature_map_5th = {
        int(feature_map_4th.width/2), int(feature_map_4th.height/2)
    };
    Size feature_map_6th = {
        int(feature_map_5th.width/2), int(feature_map_5th.height/2)
    };

    std::vector<Size> feature_map_sizes;
    feature_map_sizes.push_back(feature_map_3rd);
    feature_map_sizes.push_back(feature_map_4th);
    feature_map_sizes.push_back(feature_map_5th);
    feature_map_sizes.push_back(feature_map_6th);

    // Fixed params for generating priors
    const std::vector<std::vector<float>> min_sizes = {
        {10.0f,  16.0f,  24.0f},
        {32.0f,  48.0f},
        {64.0f,  96.0f},
        {128.0f, 192.0f, 256.0f}
    };
    CV_Assert(min_sizes.size() == feature_map_sizes.size()); // just to keep vectors in sync
    const std::vector<int> steps = { 8, 16, 32, 64 };

    // Generate priors
    detection->priors.clear();
    for (size_t i = 0; i < feature_map_sizes.size(); ++i)
    {
        Size feature_map_size = feature_map_sizes[i];
        std::vector<float> min_size = min_sizes[i];

        for (int _h = 0; _h < feature_map_size.height; ++_h)
        {
            for (int _w = 0; _w < feature_map_size.width; ++_w)
            {
                for (size_t j = 0; j < min_size.size(); ++j)
                {
                    float s_kx = min_size[j] / inputW;
                    float s_ky = min_size[j] / inputH;

                    float cx = (_w + 0.5f) * steps[i] / inputW;
                    float cy = (_h + 0.5f) * steps[i] / inputH;

                    Rect2f prior = { cx, cy, s_kx, s_ky };
                    detection->priors.push_back(prior);
                }
            }
        }
    }
}

static int get_detections(cv::Mat faces, int w, int h, auto& output) {
    int n = 0;
    DetectionResult* result = (DetectionResult* )output.get();
    for (int i = 0; i < faces.rows; i++) {
        float left = faces.at<float>(i, 0);
        float top = faces.at<float>(i, 1);
        float width = faces.at<float>(i, 2);
        float height = faces.at<float>(i, 3);
        float score = faces.at<float>(i, 14);
        left = (int)(left - width*0.25);
        top = (int)(top - height*0.25);
        width *= 1.5;width = (int)width;
        height *= 1.5;height = (int)height;
        if(left < 0) left = 0;
        if(top < 0) top = 0;
        if(left + width > w-1) width = w-1-left;
        if(top + height > h-1) height = h-1-top;
        if(width > w*1/2 || height > h*1/2 || width*height < 20) {
            continue;
        }
        result[n].left = left;
        result[n].top = top;
        result[n].width = width;
        result[n].height = height;
        result[n].score = score;
        result[n].classid = 81; // coco:face
        n ++;
    }
    return n;
}

extern "C" int DetectionInit(ElementData* data, char* params) {
    strncpy(data->input_name[0], "detection_input", sizeof(data->input_name[0]));
    if(engine != NULL) {
        return 0;
    }
    if(params == NULL) {
        AppWarn("params is null");
        return -1;
    }
    auto model = GetStrValFromJson(params, "model");
    int backend_id = GetIntValFromJson(params, "backend_id");
    int target_id = GetIntValFromJson(params, "target_id");
    int top_k = GetIntValFromJson(params, "top_k");
    double score_threshold = GetDoubleValFromJson(params, "score_threshold");
    double nms_threshold = GetDoubleValFromJson(params, "nms_threshold");
    if(model == nullptr || backend_id < 0 || target_id < 0 || top_k < 0 ||
            score_threshold < 0 || nms_threshold < 0) {
        AppWarn("create engine failed, params exception");
        return -1;
    }

    cv::dnn::Net net = cv::dnn::readNet(model.get(), "");
    if(net.empty()) {
        AppWarn("create engine failed, model:%s", model.get());
        return -1;
    }
    net.setPreferableBackend(backend_id);
    net.setPreferableTarget(target_id);
    engine = new FaceEngine();
    engine->net = net;
    engine->score_threshold = score_threshold;
    engine->nms_threshold = nms_threshold;
    engine->top_k = top_k;

    return 0;
}

extern "C" IHandle DetectionStart(int channel, char* params) {
    DetectionParams* detection = (DetectionParams* )calloc(1, sizeof(DetectionParams));
    detection->id = channel;
    return detection;
}

extern "C" int DetectionProcess(IHandle handle, TensorData* data) {
    auto pkt = data->tensor_buf.input[0];
    int w = pkt->_params.width;
    int h = pkt->_params.height;
    char *buf = pkt->_data;
    std::vector<cv::Mat> output_blobs;
    std::vector<cv::String> output_names = { "loc", "conf", "iou" };
    DetectionParams* detection = (DetectionParams* )handle;

    //struct timeval tv1, tv2;
    //gettimeofday(&tv1, NULL);
    if(!detection->init) {
        detection->input_w = w;
        detection->input_h = h;
        detection->init = true;
        GeneratePriors(detection);
    }

    Mat img = Mat(h, w, CV_8UC3, buf);
    Mat input_blob = cv::dnn::blobFromImage(img);
    // Forward
    engine->mtx.lock();
    engine->net.setInput(input_blob);
    engine->net.forward(output_blobs, output_names);
    engine->mtx.unlock();
    // Post process
    cv::Mat faces;
    cv::Mat results = postProcess(output_blobs, detection);
    results.convertTo(faces, CV_32FC1);
    // Copy to out
    if(faces.rows == 0) {
        return 0;
    }
    auto output = std::make_unique<char[]>(sizeof(DetectionResult)*faces.rows);
    int n = get_detections(faces, w, h, output);
    if(n > 0) {
        HeadParams params = {0};
        params.frame_id = pkt->_params.frame_id;
        params.width = pkt->_params.width;
        params.height = pkt->_params.height;
        auto _packet = new Packet(output.get(), sizeof(DetectionResult)*n, &params);
        data->tensor_buf.output = _packet;
    }
    //gettimeofday(&tv2, NULL);
    //AppDebug("frameid:%d, cost:%fms", pkt->_params.frame_id, (tv2.tv_sec - tv1.tv_sec)*1000.0 + (tv2.tv_usec - tv1.tv_usec)/1000.0);
    return 0;
}

extern "C" int DetectionStop(IHandle handle) {
    DetectionParams* detection = (DetectionParams* )handle;
    if(detection == NULL) {
        AppWarn("detection is null");
        return -1;
    }
    detection->priors.clear();
    free(detection);
    return 0;
}

extern "C" int DetectionRelease(void) {
    return 0;
}

extern "C" int DylibRegister(DLRegister** r, int& size) {
    size = 1;
    DLRegister* p = (DLRegister*)calloc(size, sizeof(DLRegister));
    strncpy(p->name, "detection", sizeof(p->name));
    p->init = "DetectionInit";
    p->start = "DetectionStart";
    p->process = "DetectionProcess";
    p->stop = "DetectionStop";
    p->release = "DetectionRelease";
    *r = p;
    return 0;
}

