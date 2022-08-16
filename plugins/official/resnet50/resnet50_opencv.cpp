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
    std::vector<std::string> labels;
} Resnet50Engine;

typedef struct {
    int id;
} ModuleObj;

static Resnet50Engine* engine = NULL;
static int net_w = 224, net_h = 224;
static std::unique_ptr<char[]> MakeJson(int id, auto pkt, const char* name, float confidence) {
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
    cJSON* obj =  cJSON_CreateObject();
    cJSON_AddItemToArray(object_root, obj);
    snprintf(str, sizeof(str), "name:%s, score:%.2f", name, confidence);
    cJSON_AddStringToObject(obj, "type", str);
    cJSON_AddNumberToObject(obj, "x", 20);
    cJSON_AddNumberToObject(obj, "y", 30);
    cJSON_AddNumberToObject(obj, "w", 0);
    cJSON_AddNumberToObject(obj, "h", 0);
    cJSON_AddStringToObject(obj, "url", "null");
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

extern "C" int ResnetInit(ElementData* data, char* params) {
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
    int backend_id = GetIntValFromJson(params, "backend");
    int target_id = GetIntValFromJson(params, "target");
    double scale = GetDoubleValFromJson(params, "scale");
    auto mean = GetStrValFromJson(params, "mean");
    if(model == nullptr || backend_id < 0 || target_id < 0 || scale < 0 || mean == nullptr) {
        AppWarn("get model params failed");
        return -1;
    }
    float val[3];
    sscanf(mean.get(), "%f%f%f", val, val+1, val+2);
    Scalar _mean(val[0], val[1], val[2]);
    // init net engine
    Net net = readNet(model.get());
    if(net.empty()) {
        AppWarn("create engine failed, model:%s", model.get());
        return -1;
    }
    net.setPreferableBackend(backend_id);
    net.setPreferableTarget(target_id);
    engine = new Resnet50Engine();
    engine->net = net;
    engine->scale = scale;
    engine->mean = _mean;
    engine->rgb = true;
    engine->crop = false;
    LabelsInit("./data/labels-1k.txt");
    return 0;
}

extern "C" IHandle ResnetStart(int channel, char* params) {
    ModuleObj* obj = new ModuleObj();
    obj->id = channel;
    return obj;
}

extern "C" int ResnetProcess(IHandle handle, TensorData* data) {
    ModuleObj* obj = (ModuleObj* )handle;
    auto pkt = data->tensor_buf.input[0];

    // PreProcess
    std::vector<char> img_data(pkt->_data, pkt->_data + pkt->_size);
    Mat img = imdecode(Mat(img_data), IMREAD_UNCHANGED);
    if(img.empty()) {
        printf("resnet50, id:%d, imdecode failed, size:%ld\n", obj->id, pkt->_size);
        return -1;
    }
    Mat blob;
    blobFromImage(img, blob, engine->scale, Size(net_w, net_h), 
                  engine->mean, engine->rgb, engine->crop);
    // Forward
    engine->mtx.lock();
    engine->net.setInput(blob);
    Mat prob = engine->net.forward();
    engine->mtx.unlock();
    // Post process
    Mat softmax_prob;
    double confidence;
    float max_prob = *std::max_element(prob.begin<float>(), prob.end<float>());
    cv::exp(prob-max_prob, softmax_prob);
    float sum = (float)cv::sum(softmax_prob)[0];
    softmax_prob /= sum;
    Point class_point;
    minMaxLoc(softmax_prob.reshape(1, 1), 0, &confidence, 0, &class_point);
    int classid = class_point.x;
    // Make json output
    auto json = MakeJson(obj->id, pkt, engine->labels[classid].c_str(), confidence);
    if(json != nullptr) {
        HeadParams params = {0};
        params.frame_id = pkt->_params.frame_id;
        auto _packet = new Packet(json.get(), strlen(json.get())+1, &params);
        data->tensor_buf.output = _packet;
    }
    return 0;
}

extern "C" int ResnetStop(IHandle handle) {
    ModuleObj* obj = (ModuleObj* )handle;
    if(obj == NULL) {
        AppWarn("obj is null");
        return -1;
    }
    delete obj;
    return 0;
}

extern "C" int ResnetRelease(void) {
    return 0;
}

extern "C" int DylibRegister(DLRegister** r, int& size) {
    size = 1; // reserved
    DLRegister* p = (DLRegister*)calloc(size, sizeof(DLRegister));
    strncpy(p->name, "resnet50", sizeof(p->name));
    p->init = "ResnetInit";
    p->start = "ResnetStart";
    p->process = "ResnetProcess";
    p->stop = "ResnetStop";
    p->release = "ResnetRelease";
    *r = p;
    return 0;
}

