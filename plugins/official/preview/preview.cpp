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
    int id;
    int frame_id;
} PreviewParams;

extern "C" int PreviewInit(ElementData* data, char* params) {
    strncpy(data->input_name[0], "preview_input1_frame", sizeof(data->input_name[0]));
    strncpy(data->input_name[1], "preview_input2_osd", sizeof(data->input_name[1]));
    return 0;
}

extern "C" IHandle PreviewStart(int channel, char* params) {
    if(params != NULL) {
        AppDebug("params:%s", params);
    }
    PreviewParams* preview_params = (PreviewParams* )calloc(1, sizeof(PreviewParams));
    preview_params->id = channel;
    return preview_params;
}

extern "C" int PreviewProcess(IHandle handle, TensorData* data) {
    PreviewParams* preview_params = (PreviewParams* )handle;
    TensorBuffer& tensor_buf = data->tensor_buf;
    if(preview_params->frame_id % 200 == 0) {
        for(size_t i = 0; i < tensor_buf.input_num; i++) {
            auto pkt = tensor_buf.input[i];
            printf("##test, preview, id:%d, frameid:%d, "
                    "input %ld, size:%ld, data:%02x:%02x:%02x:%02x:%02x\n", 
                preview_params->id, preview_params->frame_id, i, 
                pkt->_size, pkt->_data[0], pkt->_data[1], 
                pkt->_data[2], pkt->_data[3], pkt->_data[4]);
        }
    }
    preview_params->frame_id ++;
    return 0;
}

extern "C" int PreviewStop(IHandle handle) {
    PreviewParams* preview_params = (PreviewParams* )handle;
    free(preview_params);
    return 0;
}

extern "C" int PreviewRelease(void) {
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

