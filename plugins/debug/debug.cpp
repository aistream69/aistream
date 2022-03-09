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
} DebugParams;

extern "C" int DebugInit(ElementData* data) {
    auto queue = std::make_shared<PacketQueue>();
    strncpy(queue->name, "debug_input_frame", sizeof(queue->name));
    data->input.push_back(queue);
    return 0;
}

extern "C" IHandle DebugStart(int channel, char* params) {
    AppDebug("##test");
    if(params != NULL) {
        AppDebug("params:%s", params);
    }
    return (IHandle)1;
}

extern "C" int DebugProcess(IHandle handle, TensorData* data) {
    static int cnt = 0;
    char buf[16];
    int size = 16;
    memset(buf, ++cnt, size);
    auto _packet = std::make_shared<Packet>(buf, 16);
    data->_out = _packet;
    return 0;
}

extern "C" int DebugStop(IHandle handle) {
    AppDebug("##test");
    return 0;
}

extern "C" int DebugRelease(void) {
    AppDebug("##test");
    return 0;
}

extern "C" int DylibRegister(DLRegister** r, int& size) {
    size = 1; // reserved
    DLRegister* p = (DLRegister*)calloc(size, sizeof(DLRegister));
    strncpy(p->name, "debug", sizeof(p->name));
    p->init = "DebugInit";
    p->start = "DebugStart";
    p->process = "DebugProcess";
    p->stop = "DebugStop";
    p->release = "DebugRelease";
    *r = p;
    return 0;
}

