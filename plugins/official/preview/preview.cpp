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
} PreviewParams;

extern "C" int PreviewInit(ElementData* data, char* params) {
    auto queue = std::make_shared<PacketQueue>();
    strncpy(queue->name, "preview_input1_frame", sizeof(queue->name));
    data->input.push_back(queue);
    queue = std::make_shared<PacketQueue>();
    strncpy(queue->name, "preview_input2_osd", sizeof(queue->name));
    data->input.push_back(queue);
    return 0;
}

extern "C" IHandle PreviewStart(int channel, char* params) {
    AppDebug("##test");
    if(params != NULL) {
        AppDebug("params:%s", params);
    }
    return (IHandle)1;
}

extern "C" int PreviewProcess(IHandle handle, TensorData* data) {
    for(size_t i = 0; i < data->_in.size(); i++) {
        auto pkt = data->_in[i];
        AppDebug("##test, input %ld, size:%ld, data:%d", i, pkt->_data.size(), pkt->_data[0]);
    }
    return 0;
}

extern "C" int PreviewStop(IHandle handle) {
    AppDebug("##test");
    return 0;
}

extern "C" int PreviewRelease(void) {
    AppDebug("##test");
    return 0;
}

extern "C" int DylibRegister(DLRegister** r, int& size) {
    size = 1; // reserved
    DLRegister* p = (DLRegister*)calloc(size, sizeof(DLRegister));
    strncpy(p->name, "preview", sizeof(p->name));
    p->init = "PreviewInit";
    p->start = "PreviewStart";
    p->process = "PreviewProcess";
    p->stop = "PreviewStop";
    p->release = "PreviewRelease";
    *r = p;
    return 0;
}

