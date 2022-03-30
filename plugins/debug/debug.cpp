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

extern "C" int DebugInit(ElementData* data, char* params) {
    strncpy(data->input_name[0], "debug_input_frame", sizeof(data->input_name[0]));
    //data->sleep_usec = 40000;
    return 0;
}

extern "C" IHandle DebugStart(int channel, char* params) {
    if(params != NULL) {
        AppDebug("params:%s", params);
    }
    return (IHandle)1;
}

extern "C" int DebugProcess(IHandle handle, TensorData* data) {
    static int cnt = 0;
    int size = 20480000;
    char* buf = (char *)malloc(size);
    if(buf == NULL) {
        AppError("malloc failed");
        return -1;
    }
    memset(buf, ++cnt, size);
    HeadParams params = {0};
    // input
    TensorBuffer& tensor_buf = data->tensor_buf;
    for(size_t i = 0; i < tensor_buf.input_num; i++) {
        auto pkt = tensor_buf.input[i];
        params.frame_id = pkt->_params.frame_id;
    }
    // output
    auto _packet = new Packet(buf, size, &params);
    tensor_buf.output = _packet;
    free(buf);
    return 0;
}

extern "C" int DebugStop(IHandle handle) {
    return 0;
}

extern "C" int DebugRelease(void) {
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

