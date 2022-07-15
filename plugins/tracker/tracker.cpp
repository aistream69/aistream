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
#include "BYTETracker.h"
#include "cJSON.h"
#include "share.h"
#include "tensor.h"
#include "config.h"
#include "log.h"

using namespace cv;

typedef struct {
    int capture;
    int last_frameid;
    int dir; // 0:up->down, 1:down->up
} TParams;

typedef struct {
    int id;
    int w;
    int h;
    BYTETracker *btrack;
    int last_capture_frameid;
    std::map<int, TParams*> tracking;
} TrackerParams;

static NginxParams nginx;
static float capture_line = 0.66;
static char local_ip[128] = {0};
static BObject RectCorrect(STrack output_strack, int w, int h) {
    BObject det;
    vector<float> tlwh = output_strack.tlwh;
    det.rect.x = (int)tlwh[0];
    det.rect.y = (int)tlwh[1];
    det.rect.width = (int)tlwh[2];
    det.rect.height = (int)tlwh[3];
    if(det.rect.x < 0) det.rect.x = 0;
    if(det.rect.y < 0) det.rect.y = 0;
    if(det.rect.x + det.rect.width > w-1) det.rect.width = w-1-det.rect.x;
    if(det.rect.y + det.rect.height > h-1) det.rect.height = h-1-det.rect.y;
    det.label = output_strack.label;
    det.track_id = output_strack.track_id;
    return det;
}

static void MatToJpg(const cv::Mat &mat, std::vector<unsigned char> &buff) {
    if(mat.empty()) {
        return;
    }
    std::vector<int> compressing_factor;
    compressing_factor.push_back(IMWRITE_JPEG_QUALITY);
    compressing_factor.push_back(95); // default(95) 0-100
    cv::imencode(".jpg", mat, buff, compressing_factor);
}

static void Rgb2Jpg(int id, BObject& det, auto rgb, char scene_path[URL_LEN], char obj_path[URL_LEN]) {
    char date[32];
    struct tm _time;
    struct timeval tv;
    int w = rgb->_params.width;
    int h = rgb->_params.height;
    char* buf = rgb->_data;

    Mat _scene_img, scene_img;
    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &_time);
    snprintf(date, sizeof(date), "%d%02d%02d", _time.tm_year + 1900, _time.tm_mon + 1, _time.tm_mday);
    _scene_img = Mat(h, w, CV_8UC3, buf);
    cvtColor(_scene_img, scene_img, COLOR_RGB2BGR);
    Rect _rect(det.rect.x, det.rect.y, det.rect.width, det.rect.height);
    Mat frame = scene_img(_rect);
    Mat obj_img = frame.clone();
    std::vector<unsigned char> obj_buf;
    MatToJpg(obj_img, obj_buf);
    snprintf(obj_path, URL_LEN, "%s/image/%s/%d/%ld_0_obj.jpg", 
            nginx.workdir, date, id, tv.tv_sec);
    WriteFile(obj_path, &obj_buf[0], obj_buf.size(), "wb");
    snprintf(obj_path, URL_LEN, "http://%s:%d/image/%s/%d/%ld_0_obj.jpg", 
            local_ip, nginx.http_port, date, id, tv.tv_sec);

    std::vector<unsigned char> scene_buf;
    rectangle(scene_img, _rect, Scalar(0,255,0), 2);
    MatToJpg(scene_img, scene_buf);
    snprintf(scene_path, URL_LEN, "%s/image/%s/%d/%ld_0_scene.jpg", 
            nginx.workdir, date, id, tv.tv_sec);
    WriteFile(scene_path, &scene_buf[0], scene_buf.size(), "wb");
    snprintf(scene_path, URL_LEN, "http://%s:%d/image/%s/%d/%ld_0_scene.jpg", 
            local_ip, nginx.http_port, date, id, tv.tv_sec);
}

static std::unique_ptr<char[]> MakeJson(int id, BObject& det, char *scene_url, char *obj_url) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    cJSON* root = cJSON_CreateObject();
    cJSON* data_root =  cJSON_CreateObject();
    cJSON* sceneimg_root =  cJSON_CreateObject();
    cJSON* object_root =  cJSON_CreateArray();
    cJSON_AddStringToObject(root, "msg_type", "common");
    cJSON_AddItemToObject(root, "data", data_root);
    cJSON_AddNumberToObject(data_root, "id", id);
    cJSON_AddNumberToObject(data_root, "timestamp", tv.tv_sec);
    cJSON_AddItemToObject(data_root, "sceneimg", sceneimg_root);
    cJSON_AddItemToObject(data_root, "object", object_root);
    int track_id = (int)det.track_id;
    int x = (int)det.rect.x;
    int y = (int)det.rect.y;
    int w = (int)det.rect.width;
    int h = (int)det.rect.height;
    cJSON* obj =  cJSON_CreateObject();
    cJSON_AddItemToArray(object_root, obj);
    cJSON_AddStringToObject(obj, "type", "face");
    cJSON_AddNumberToObject(obj, "trackid", track_id);
    cJSON_AddNumberToObject(obj, "x", x);
    cJSON_AddNumberToObject(obj, "y", y);
    cJSON_AddNumberToObject(obj, "w", w);
    cJSON_AddNumberToObject(obj, "h", h);
    cJSON_AddStringToObject(obj, "url", obj_url);
    cJSON_AddStringToObject(sceneimg_root, "url", scene_url);
    char *json = cJSON_Print(root);
    auto val = std::make_unique<char[]>(strlen(json) + 1);
    strcpy(val.get(), json);
    free(json);
    cJSON_Delete(root);
    return val;
}

static std::unique_ptr<char[]> TrackCapture(vector<STrack> output_stracks, 
                                TrackerParams* tracker, auto pkt, auto rgb) {
    int w = pkt->_params.width;
    int h = pkt->_params.height;
    float line = h*capture_line;
    int frame_id = pkt->_params.frame_id;
    std::unique_ptr<char[]> json = nullptr;
    
    for(unsigned int i = 0; i < output_stracks.size(); i++) {
        vector<float> tlwh = output_stracks[i].tlwh;
        int track_id = (int)output_stracks[i].track_id;
        auto itr = tracker->tracking.find(track_id);
        float y_up = tlwh[1];
        float y_bottom = tlwh[1] + tlwh[3];
        if(itr != tracker->tracking.end()) {
            auto t = itr->second;
            t->last_frameid = frame_id;
            if(t->capture || tracker->last_capture_frameid + 1 >= frame_id) {
                continue;
            }
            if((y_bottom >= line && !t->dir) || (y_up <= line && t->dir)) {
                char scene_url[URL_LEN] = {0}, obj_url[URL_LEN] = {0};
                BObject det = RectCorrect(output_stracks[i], w, h);
                Rgb2Jpg(tracker->id, det, rgb, scene_url, obj_url); // 40ms-50ms
                json = MakeJson(tracker->id, det, scene_url, obj_url);
                tracker->last_capture_frameid = frame_id;
                t->capture = 1;
                printf("id:%d, trackid:%d, frameid:%d, capture ok\n", tracker->id, track_id, frame_id);
                break; // capture one object every frame
            }
        }
        else {
            TParams *t = new TParams;
            t->capture = 0;
            t->last_frameid = frame_id;
            if(y_bottom < line) {
                t->dir = 0;
            }
            else {
                t->dir = 1;
            }
            tracker->tracking[track_id] = t;
        }
    }
    if(frame_id % 100 == 0) {
        for(auto itr = tracker->tracking.begin(); itr != tracker->tracking.end();) {
            auto t = itr->second;
            if(t->last_frameid + 500 < frame_id) { // 25fps*20s
                tracker->tracking.erase(itr++);
                delete t;
            }
            else {
                itr ++;
            }
        }
    }

    return json;
}

extern "C" int TrackerInit(ElementData* data, char* params) {
    strncpy(data->input_name[0], "tracker_input1", sizeof(data->input_name[0]));
    strncpy(data->input_name[1], "tracker_input2", sizeof(data->input_name[1]));
    
    if(strlen(local_ip) > 0) {
        return 0;
    }
    if(params == NULL) {
        AppWarn("params is null");
        return -1;
    }
    capture_line = GetDoubleValFromJson(params, "capture_line");
    if(capture_line <= 0) {
        capture_line = 0.66;
    }
    auto localhost = GetStrValFromFile(CONFIG_FILE, "system", "localhost");
    if(localhost != nullptr) {
        strncpy(local_ip, localhost.get(), sizeof(local_ip));
    }
    else {
        GetLocalIp(local_ip);
    }
    NginxInit(nginx);

    return 0;
}

extern "C" IHandle TrackerStart(int channel, char* params) {
    int fps = 25;
    TrackerParams* tracker = new TrackerParams();
    tracker->id = channel;
    tracker->btrack = new BYTETracker(fps, 30);
    return tracker;
}

extern "C" int TrackerProcess(IHandle handle, TensorData* data) {
    TrackerParams* tracker = (TrackerParams* )handle;
    auto pkt = data->tensor_buf.input[0];
    auto rgb = data->tensor_buf.input[1];
    int num = (int)pkt->_size/sizeof(DetectionResult);
    DetectionResult* det = (DetectionResult* )pkt->_data;
    vector<BObject> objects;
    for(int i = 0; i < num; i ++) {
        BObject _det;
        _det.rect.x = det[i].left;
        _det.rect.y = det[i].top;
        _det.rect.width = det[i].width;
        _det.rect.height = det[i].height;
        _det.label = det[i].classid;
        _det.prob = det[i].score;
        objects.push_back(_det);
    }
    vector<STrack> output_stracks = tracker->btrack->update(objects);
    auto json = TrackCapture(output_stracks, tracker, pkt, rgb);
    if(json != nullptr) {
        HeadParams params = {0};
        params.frame_id = pkt->_params.frame_id;
        auto _packet = new Packet(json.get(), strlen(json.get())+1, &params);
        data->tensor_buf.output = _packet;
    }
    return 0;
}

extern "C" int TrackerStop(IHandle handle) {
    TrackerParams* tracker = (TrackerParams* )handle;
    if(tracker == NULL) {
        AppWarn("tracker is null");
        return -1;
    }
    if(tracker->btrack != NULL) {
        delete tracker->btrack;
    }
    delete tracker;
    return 0;
}

extern "C" int TrackerRelease(void) {
    return 0;
}

extern "C" int DylibRegister(DLRegister** r, int& size) {
    size = 1;
    DLRegister* p = (DLRegister*)calloc(size, sizeof(DLRegister));
    strncpy(p->name, "tracker", sizeof(p->name));
    p->init = "TrackerInit";
    p->start = "TrackerStart";
    p->process = "TrackerProcess";
    p->stop = "TrackerStop";
    p->release = "TrackerRelease";
    *r = p;
    return 0;
}

