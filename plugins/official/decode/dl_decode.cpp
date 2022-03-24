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
#include "tensor.h"
#include "log.h"

typedef struct {
} DecodeParams;

extern "C" int DecodeInit(ElementData* data, char* params) {
    AppDebug("##test");
    auto queue = std::make_shared<PacketQueue>();
    strncpy(queue->name, "decode_input", sizeof(queue->name));
    data->input.push_back(queue);
    return 0;
}

extern "C" IHandle DecodeStart(int channel, char* params) {
    AppDebug("##test");
    if(params != NULL) {
        AppDebug("params:%s", params);
    }
    return (IHandle)1;
}

extern "C" int DecodeProcess(IHandle handle, TensorData* data) {
    //AppDebug("##test");
    static int cnt = 0;
    char buf[16];
    int size = 16;
    memset(buf, ++cnt, size);
    HeadParams params = {0};
    for(size_t i = 0; i < data->_in.size(); i++) {
        auto pkt = data->_in[i];
        params.frame_id = pkt->_params.frame_id;
    }
    auto _packet = std::make_shared<Packet>(buf, 16, &params);
    data->_out = _packet;
    return 0;
}

extern "C" int DecodeStop(IHandle handle) {
    AppDebug("##test");
    return 0;
}

extern "C" int DecodeRelease(void) {
    AppDebug("##test");
    return 0;
}

extern "C" int DylibRegister(DLRegister** r, int& size) {
    size = 1; // reserved
    DLRegister* p = (DLRegister*)calloc(size, sizeof(DLRegister));
    strncpy(p->name, "decode", sizeof(p->name));
    p->init = "DecodeInit";
    p->start = "DecodeStart";
    p->process = "DecodeProcess";
    p->stop = "DecodeStop";
    p->release = "DecodeRelease";
    *r = p;
    return 0;
}

