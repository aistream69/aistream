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
#include "share.h"
#include "log.h"

#define VIDEO_W 1280
#define VIDEO_H 720

typedef struct {
  FILE *fp;
  char* yuv_buf;
  int yuv_size;
} DebugParams;

static ShareParams share_params = {0};
static void CopyToPacket(DebugParams* debug, TensorData* data) {
  int num = 3;
  size_t size= sizeof(DetectionResult)*num;
  char* ptr = new char[size];
  DetectionResult* dets = (DetectionResult* )ptr;

  static int offset = 50;
  for (int i = 0; i < num; i ++) {
    DetectionResult* det = dets + i;
    det->left = i*10 + offset;
    det->top = i*50 + 10 + offset;
    if (det->top > VIDEO_H) {
      offset = 0;
    }
    det->width = 20 + i*20;
    det->height = 30 + i*30;
    det->score = 0.99;
    det->classid = 1;
  }
  //offset += 2;

  HeadParams params = {0};
  auto pkt = data->tensor_buf.input[0];
  params.ptr = ptr;
  params.ptr_size = size;
  params.frame_id = pkt->_params.frame_id;
  params.width = VIDEO_W;
  params.height = VIDEO_H;
  auto _packet = new Packet(debug->yuv_buf, debug->yuv_size, &params);
  data->tensor_buf.output = _packet;
}

extern "C" int DebugInit(ElementData* data, char* params) {
  share_params = GlobalConfig();
  strncpy(data->input_name[0], "debug_input", sizeof(data->input_name[0]));
  data->queue_len = GetIntValFromFile(share_params.config_file, "video", "rgb_queue_len");
  if (data->queue_len < 0) {
    data->queue_len = 10;
  }
  return 0;
}

extern "C" IHandle DebugStart(int channel, char* params) {
  if (params != NULL) {
    AppDebug("params:%s", params);
  }
  const char* filename = "./data/video/test.yuv";
  FILE *fp = fopen(filename, "rb");
  if (fp == NULL) {
    AppError("fopen %s failed", filename);
    return NULL;
  }
  DebugParams* debug = new DebugParams();
  debug->fp = fp;
  debug->yuv_size = VIDEO_W*VIDEO_H*3/2;
  debug->yuv_buf = (char* )malloc(debug->yuv_size);
  return debug;
}

extern "C" int DebugProcess(IHandle handle, TensorData* data) {
  DebugParams* debug = (DebugParams* )handle;
  int n = fread(debug->yuv_buf, 1, debug->yuv_size, debug->fp);
  if (n != debug->yuv_size) {
    AppDebug("read end, loop ...");
    fseek(debug->fp, 0L, SEEK_SET);
    int n = fread(debug->yuv_buf, 1, debug->yuv_size, debug->fp);
    if (n != debug->yuv_size) {
      AppDebug("loop read failed");
      sleep(3600);
    }
  }
  CopyToPacket(debug, data);

  return 0;
}

extern "C" int DebugStop(IHandle handle) {
  DebugParams* debug = (DebugParams* )handle;
  if (debug != NULL) {
    fclose(debug->fp);
    free(debug->yuv_buf);
    delete debug;
  }
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

