#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <thread>
#include <opencv2/dnn.hpp>
#include "opencv2/core.hpp"
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include "cJSON.h"
#include "tensor.h"
#include "config.h"
#include "share.h"
#include "log.h"

using namespace cv;
using namespace dnn;

typedef struct {
    std::mutex mtx;
    Net net;
    float scale;
    bool rgb;
    bool crop;
    Scalar mean;
    int width;
    int height;
    float conf_threshold;
    float nms_threshold;
    std::vector<String> out_names;
    std::vector<std::string> labels;
} Yolov3Engine;

typedef struct {
    int id;
} ModuleObj;

static Yolov3Engine* engine = NULL;
static std::unique_ptr<char[]> MakeJson(int id, auto pkt, std::vector<int> classIds, 
                std::vector<float> confidences, std::vector<Rect> boxes, auto labels) {
    char str[384];
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
    for(size_t idx = 0; idx < boxes.size(); ++idx) {
        Rect box = boxes[idx];
        int class_id = classIds[idx];
        cJSON* obj =  cJSON_CreateObject();
        cJSON_AddItemToArray(object_root, obj);
        snprintf(str, sizeof(str), "name:%s, score:%.2f", labels[class_id].c_str(), confidences[idx]);
        cJSON_AddStringToObject(obj, "type", str);
        cJSON_AddNumberToObject(obj, "x", box.x);
        cJSON_AddNumberToObject(obj, "y", box.y);
        cJSON_AddNumberToObject(obj, "w", box.width);
        cJSON_AddNumberToObject(obj, "h", box.height);
        cJSON_AddStringToObject(obj, "url", "null");
    }
    if(pkt->_params.ptr != NULL) {
        char* url = pkt->_params.ptr + pkt->_params.ptr_size;
        if(cJSON *user_data = cJSON_Parse(pkt->_params.ptr)) {
            cJSON_AddItemToObject(data_root, "userdata", user_data);
        }
        cJSON_AddStringToObject(sceneimg_root, "url", url);
    }
    char *json = cJSON_Print(root);
    auto val = std::make_unique<char[]>(strlen(json) + 1);
    strcpy(val.get(), json);
    free(json);
    cJSON_Delete(root);
    return val;
}

static void LabelsInit(const char* filename) {
    char buf[256], name[256];
    FILE* fp = fopen(filename, "r");
    if(fp == NULL) {
        AppError("open %s failed", filename);
        return;
    }
    while(fgets(buf, sizeof(buf), fp) != NULL) {
        int n = sscanf(buf, "%[^\t\n]", name);
        if(n != 1) {
            break;
        }
        engine->labels.push_back(name);
    }
    fclose(fp);
}
    
static int Postprocess(std::vector<Mat> outs, std::vector<int>& classIds, 
        std::vector<float>& confidences, std::vector<Rect>& boxes, Mat& img) {
    for(size_t i = 0; i < outs.size(); ++i) {
        // Network produces output blob with a shape NxC where N is a number of
        // detected objects and C is a number of classes + 4 where the first 4
        // numbers are [center_x, center_y, width, height]
        float* data = (float*)outs[i].data;
        for(int j = 0; j < outs[i].rows; ++j, data += outs[i].cols) {
            Mat scores = outs[i].row(j).colRange(5, outs[i].cols);
            Point classIdPoint;
            double confidence;
            minMaxLoc(scores, 0, &confidence, 0, &classIdPoint);
            if(confidence > engine->conf_threshold) {
                int centerX = (int)(data[0] * img.cols);
                int centerY = (int)(data[1] * img.rows);
                int width = (int)(data[2] * img.cols);
                int height = (int)(data[3] * img.rows);
                int left = centerX - width / 2;
                int top = centerY - height / 2;
                classIds.push_back(classIdPoint.x);
                confidences.push_back((float)confidence);
                boxes.push_back(Rect(left, top, width, height));
            }
        }
    }
    // Nms
    std::map<int, std::vector<size_t> > class2indices;
    for(size_t i = 0; i < classIds.size(); i++) {
        if(confidences[i] >= engine->conf_threshold) {
            class2indices[classIds[i]].push_back(i);
        }
    }
    std::vector<Rect> nmsBoxes;
    std::vector<float> nmsConfidences;
    std::vector<int> nmsClassIds;
    for(std::map<int, std::vector<size_t> >::iterator it = class2indices.begin(); 
        it != class2indices.end(); ++it) {
        std::vector<Rect> localBoxes;
        std::vector<float> localConfidences;
        std::vector<size_t> classIndices = it->second;
        for(size_t i = 0; i < classIndices.size(); i++) {
            localBoxes.push_back(boxes[classIndices[i]]);
            localConfidences.push_back(confidences[classIndices[i]]);
        }
        std::vector<int> nmsIndices;
        NMSBoxes(localBoxes, localConfidences, engine->conf_threshold, engine->nms_threshold, nmsIndices);
        for(size_t i = 0; i < nmsIndices.size(); i++) {
            size_t idx = nmsIndices[i];
            nmsBoxes.push_back(localBoxes[idx]);
            nmsConfidences.push_back(localConfidences[idx]);
            nmsClassIds.push_back(it->first);
        }
    }
    boxes = nmsBoxes;
    classIds = nmsClassIds;
    confidences = nmsConfidences;
    return 0;
}

extern "C" int YoloInit(ElementData* data, char* params) {
    strncpy(data->input_name[0], "img_input", sizeof(data->input_name[0]));
    if(params == NULL) {
        AppWarn("params is null");
        return -1;
    }
    data->queue_len = GetIntValFromFile(CONFIG_FILE, "img", "queue_len");
    if(data->queue_len < 0) {
        data->queue_len = 50;
    }
    if(engine != NULL) {
        return 0;
    }
    // get params
    auto model = GetStrValFromJson(params, "model");
    auto cfg = GetStrValFromJson(params, "cfg");
    int backend_id = GetIntValFromJson(params, "backend");
    int target_id = GetIntValFromJson(params, "target");
    double scale = GetDoubleValFromJson(params, "scale");
    auto mean = GetStrValFromJson(params, "mean");
    int width = GetIntValFromJson(params, "width");
    int height = GetIntValFromJson(params, "height");
    double thr = GetDoubleValFromJson(params, "thr");
    double nms = GetDoubleValFromJson(params, "nms");
    if(model == nullptr || cfg == nullptr || backend_id < 0 || target_id < 0 || 
       scale < 0 || mean == nullptr || width < 0 || height < 0 || thr < 0 || nms < 0) {
        AppWarn("get model params failed");
        return -1;
    }
    float val[3];
    sscanf(mean.get(), "%f%f%f", val, val+1, val+2);
    Scalar _mean(val[0], val[1], val[2]);
    // init net engine
    Net net = readNet(model.get(), cfg.get());
    if(net.empty()) {
        AppWarn("create engine failed, model:%s", model.get());
        return -1;
    }
    net.setPreferableBackend(backend_id);
    net.setPreferableTarget(target_id);
    std::vector<int> out_layers = net.getUnconnectedOutLayers();
    std::string out_layer_type = net.getLayer(out_layers[0])->type;
    if(out_layer_type != "Region") {
        AppWarn("out layer type:%s, model:%s", out_layer_type.c_str(), model.get());
        return -1;
    }
    engine = new Yolov3Engine();
    engine->net = net;
    engine->scale = scale;
    engine->mean = _mean;
    engine->width = width;
    engine->height = height;
    engine->conf_threshold = thr;
    engine->nms_threshold = nms;
    engine->rgb = true;
    engine->crop = false;
    engine->out_names = net.getUnconnectedOutLayersNames();
    LabelsInit("./data/coco.names");
    return 0;
}

extern "C" IHandle YoloStart(int channel, char* params) {
    ModuleObj* obj = new ModuleObj();
    obj->id = channel;
    return obj;
}

extern "C" int YoloProcess(IHandle handle, TensorData* data) {
    ModuleObj* obj = (ModuleObj* )handle;
    auto pkt = data->tensor_buf.input[0];

    // PreProcess
    std::vector<char> img_data(pkt->_data, pkt->_data + pkt->_size);
    Mat img = imdecode(Mat(img_data), IMREAD_UNCHANGED);
    if(img.empty()) {
        printf("yolov3, id:%d, imdecode failed, size:%ld\n", obj->id, pkt->_size);
        return -1;
    }
    Mat blob;
    blobFromImage(img, blob, 1.0, Size(engine->width, engine->height), 
                  Scalar(), engine->rgb, engine->crop, CV_8U);
    // Forward
    engine->mtx.lock();
    engine->net.setInput(blob, "", engine->scale, engine->mean);
    std::vector<Mat> outs;
    engine->net.forward(outs, engine->out_names);
    engine->mtx.unlock();
    // Post process
    std::vector<int> classIds;
    std::vector<float> confidences;
    std::vector<Rect> boxes;
    Postprocess(outs, classIds, confidences, boxes, img);
    // Make json output
    auto json = MakeJson(obj->id, pkt, classIds, confidences, boxes, engine->labels);
    if(json != nullptr) {
        HeadParams params = {0};
        params.frame_id = pkt->_params.frame_id;
        auto _packet = new Packet(json.get(), strlen(json.get())+1, &params);
        data->tensor_buf.output = _packet;
    }
    return 0;
}

extern "C" int YoloStop(IHandle handle) {
    ModuleObj* obj = (ModuleObj* )handle;
    if(obj == NULL) {
        AppWarn("obj is null");
        return -1;
    }
    delete obj;
    return 0;
}

extern "C" int YoloRelease(void) {
    return 0;
}

extern "C" int DylibRegister(DLRegister** r, int& size) {
    size = 1; // reserved
    DLRegister* p = (DLRegister*)calloc(size, sizeof(DLRegister));
    strncpy(p->name, "yolov3", sizeof(p->name));
    p->init = "YoloInit";
    p->start = "YoloStart";
    p->process = "YoloProcess";
    p->stop = "YoloStop";
    p->release = "YoloRelease";
    *r = p;
    return 0;
}

