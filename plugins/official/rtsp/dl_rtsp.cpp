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
#include "config.h"
#include "share.h"
#include "log.h"

typedef struct {
} RtspParams;

extern "C" int RtspInit(ElementData* data) {
    data->queue_len = GetIntValFromFile(CONFIG_FILE, "obj", "rtsp", "queue_len");
    if(data->queue_len < 0) {
        data->queue_len = 50;
    }
    data->sleep_usec = GetIntValFromFile(CONFIG_FILE, "obj", "rtsp", "sleep_usec");
    if(data->sleep_usec < 0) {
        data->sleep_usec = 20000;
    }
    return 0;
}

extern "C" IHandle RtspStart(int channel, char* params) {
    AppDebug("##test");
    if(params != NULL) {
        AppDebug("params:%s", params);
    }
    return (IHandle)1;
}

extern "C" int RtspProcess(IHandle handle, TensorData* data) {
    static int cnt = 0;
    char buf[1024];
    int size = 1024;
    memset(buf, ++cnt, size);
    auto _packet = std::make_shared<Packet>(buf, 1024);
    data->_out = _packet;
    return 0;
}

extern "C" int RtspStop(IHandle handle) {
    AppDebug("##test");
    return 0;
}

extern "C" int RtspRelease(void) {
    AppDebug("##test");
    return 0;
}

extern "C" int DylibRegister(DLRegister** r, int& size) {
    size = 1; // reserved
    DLRegister* p = (DLRegister*)calloc(size, sizeof(DLRegister));
    strncpy(p->name, "rtsp", sizeof(p->name));
    p->init = "RtspInit";
    p->start = "RtspStart";
    p->process = "RtspProcess";
    p->stop = "RtspStop";
    p->release = "RtspRelease";
    *r = p;
    return 0;
}

