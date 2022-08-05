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
#include <thread>
#include "cJSON.h"
#include "tensor.h"
#include "config.h"
#include "share.h"
#include "log.h"

typedef struct {
    int id;
} Resnet50Params;

static std::unique_ptr<char[]> MakeJson(int id, auto pkt) {
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
    for(int i = 0; i < 3; i ++) {
        int x = 100 + 200*i;
        int y = 100;
        int w = 160;
        int h = 300;
        cJSON* obj =  cJSON_CreateObject();
        cJSON_AddItemToArray(object_root, obj);
        cJSON_AddStringToObject(obj, "type", "cat");
        cJSON_AddNumberToObject(obj, "x", x);
        cJSON_AddNumberToObject(obj, "y", y);
        cJSON_AddNumberToObject(obj, "w", w);
        cJSON_AddNumberToObject(obj, "h", h);
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
    return 0;
}

extern "C" IHandle ResnetStart(int channel, char* params) {
    Resnet50Params* resnet = new Resnet50Params();
    resnet->id = channel;
    return resnet;
}

extern "C" int ResnetProcess(IHandle handle, TensorData* data) {
    Resnet50Params* resnet = (Resnet50Params* )handle;
    auto pkt = data->tensor_buf.input[0];
    AppDebug("type:%d,w:%d,h:%d,frameid:%d,ptr_size:%ld,data_size:%ld",
            pkt->_params.type, pkt->_params.width, pkt->_params.height, 
            pkt->_params.frame_id, pkt->_params.ptr_size, pkt->_size);
    auto json = MakeJson(resnet->id, pkt);
    if(json != nullptr) {
        HeadParams params = {0};
        params.frame_id = pkt->_params.frame_id;
        auto _packet = new Packet(json.get(), strlen(json.get())+1, &params);
        data->tensor_buf.output = _packet;
    }
    //WriteFile("test.jpg", pkt->_data, pkt->_size, "wb");
    return 0;
}

extern "C" int ResnetStop(IHandle handle) {
    Resnet50Params* resnet = (Resnet50Params* )handle;
    if(resnet == NULL) {
        AppWarn("resnet is null");
        return -1;
    }
    delete resnet;
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

