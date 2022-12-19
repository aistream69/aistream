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
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavutil/imgutils.h>
#include <libavutil/parseutils.h>
#include <libswscale/swscale.h>
#include <libavutil/mathematics.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>
}
#include "tensor.h"
#include "config.h"
#include "share.h"
#include "common.h"
#include "log.h"

typedef struct {
  int id;
  char url[512];
  int frame_id;
  pthread_t pid;
  std::mutex mtx;
  std::condition_variable condition;
  std::queue<Packet*> _queue;
  int queue_len_max;
  std::thread* t;
  long int rtmp_beat;
  int running;
  int _running;
} ModuleObj;

typedef struct {
  int init;
  uint64_t now_sec;
  ShareParams share_params;
  std::mutex obj_mtx;
  std::vector<ModuleObj*> objs;
  std::thread* t;
  int running;
} ModuleParams;

static ModuleParams module = {0};
static void CopyToPacket(uint8_t* buf, int size, ModuleObj* obj) {
  HeadParams params = {0};
  params.frame_id = ++obj->frame_id;
  std::unique_lock<std::mutex> lock(obj->mtx);
  if (obj->_queue.size() < (size_t)obj->queue_len_max) {
    auto _packet = new Packet(buf, size, &params);
    obj->_queue.push(_packet);
  } else {
    printf("warning,rtmp,id:%d, put to queue failed, quelen:%ld\n",
           obj->id, obj->_queue.size());
  }
  obj->condition.notify_one();
}

static void RtmpThread(ModuleObj* obj) {
  int ret;
  char errstr[256];
  AVBSFContext* bsf_ctx = NULL;
  AVFormatContext* ifmt_ctx = NULL;
  int video_index = -1, audio_index = -1;

  if ((ret = avformat_open_input(&ifmt_ctx, obj->url, NULL, NULL)) < 0) {
    av_strerror(ret, errstr, sizeof(errstr));
    AppWarn("id:%d, could not open input file, %s", obj->id, errstr);
    return;
  }
  if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
    AppWarn("id:%d, failed to retrieve input stream information", obj->id);
    return;
  }
  for (int i = 0; i < (int)ifmt_ctx->nb_streams; i++) {
    if (ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      video_index = i;
    } else if (ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      audio_index = i;
    } else {
      AppWarn("id:%d, streams:%d, unsupport type : %d", obj->id,
              ifmt_ctx->nb_streams, ifmt_ctx->streams[i]->codec->codec_type);
      return;
    }
  }
  const AVBitStreamFilter* pfilter = av_bsf_get_by_name("h264_mp4toannexb");
  if (pfilter == NULL) {
    AppWarn("id:%d,get bsf failed", obj->id);
    return;
  }
  if ((ret = av_bsf_alloc(pfilter, &bsf_ctx)) != 0) {
    AppWarn("id:%d,alloc bsf failed", obj->id);
    return;
  }
  ret = avcodec_parameters_from_context(bsf_ctx->par_in, ifmt_ctx->streams[video_index]->codec);
  if (ret < 0) {
    AppWarn("id:%d,set codec failed", obj->id);
    return;
  }
  bsf_ctx->time_base_in = ifmt_ctx->streams[video_index]->codec->time_base;
  ret = av_bsf_init(bsf_ctx);
  if (ret < 0) {
    AppWarn("id:%d,init bsf failed", obj->id);
    return;
  }
  AppDebug("id:%d, start ...", obj->id);

  AVPacket pkt;
  while (obj->running && obj->_running) {
    ret = av_read_frame(ifmt_ctx, &pkt);
    if (ret < 0) {
      printf("id:%d, av_read_frame failed, %d, continue\n", obj->id, ret);
      usleep(200000);
      continue;
    }
    if (pkt.stream_index != video_index) {
      printf("warning, id:%d, not support stream index %d, video:%d,audio:%d\n",
             obj->id, pkt.stream_index, video_index, audio_index);
      av_packet_unref(&pkt);
      continue;
    }
    ret = av_bsf_send_packet(bsf_ctx, &pkt);
    if (ret != 0) {
      printf("warning, send pkt failed, ret:%d\n", ret);
      break;
    }
    ret = av_bsf_receive_packet(bsf_ctx, &pkt);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      printf("warning, recv pkt EAGAIN/EOF, ret:%d\n", ret);
      break;
    } else if (ret < 0) {
      printf("warning, recv pkt failed, ret:%d\n", ret);
      break;
    }
    //printf("##test, frameid:%d, pkt size:%d, %02x:%02x:%02x:%02x:%02x\n", obj->frame_id,
    //        pkt.size, pkt.data[0], pkt.data[1], pkt.data[2], pkt.data[3], pkt.data[4]);
    CopyToPacket(pkt.data, pkt.size, obj);
    av_packet_unref(&pkt);
    obj->rtmp_beat = module.now_sec;
  }

  av_bsf_free(&bsf_ctx);
  avformat_close_input(&ifmt_ctx);
  AppDebug("id:%d, run ok", obj->id);
}

static void RtmpDaemon(void) {
  struct timeval tv;
  while (module.running) {
    gettimeofday(&tv, NULL);
    module.now_sec = tv.tv_sec;
    std::unique_lock<std::mutex> obj_lock(module.obj_mtx);
    for (size_t i = 0; i < module.objs.size(); i ++) {
      auto obj = module.objs[i];
      if (module.now_sec - obj->rtmp_beat > 15) {
        AppWarn("id:%d,detect exception,restart it ...", obj->id);
        obj->_running = 0;
        if (obj->t != nullptr) {
          if (obj->t->joinable()) {
            obj->t->join();
          }
          delete obj->t;
          obj->t = nullptr;
        }
        std::unique_lock<std::mutex> lock(obj->mtx);
        while (!obj->_queue.empty()) {
          Packet* pkt = obj->_queue.front();
          obj->_queue.pop();
          if (pkt != nullptr) {
            delete pkt;
          }
        }
        lock.unlock();
        obj->_running = 1;
        obj->t = new std::thread(&RtmpThread, obj);
        obj->rtmp_beat = module.now_sec;
      }
    }
    obj_lock.unlock();
    sleep(3);
  }
  AppDebug("run ok");
}

extern "C" int RtmpInit(ElementData* data, char* params) {
  if (__sync_add_and_fetch(&module.init, 1) <= 1) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    module.now_sec = tv.tv_sec;
    module.share_params = GlobalConfig();
    FFmpegInit();
    module.t = new std::thread(&RtmpDaemon);
    module.running = 1;
  }
  const char* cfg_file = module.share_params.config_file;
  data->queue_len = GetIntValFromFile(cfg_file, "video", "queue_len");
  if (data->queue_len < 0) {
    data->queue_len = 50;
  }
  return 0;
}

extern "C" IHandle RtmpStart(int channel, char* params) {
  if (params == NULL) {
    AppWarn("id:%d, params is null", channel);
    return NULL;
  }
  auto url = GetStrValFromJson(params, "data", "url");
  if (url == nullptr) {
    AppWarn("get url failed, %s", params);
    return NULL;
  }
  ModuleObj* obj = new ModuleObj();
  obj->id = channel;
  obj->rtmp_beat = module.now_sec;
  const char* cfg_file = module.share_params.config_file;
  obj->queue_len_max = GetIntValFromFile(cfg_file, "video", "queue_len");
  obj->queue_len_max = obj->queue_len_max > 0 ? obj->queue_len_max : 50;
  strncpy(obj->url, url.get(), sizeof(obj->url));
  obj->running = 1;
  obj->_running = 1;
  obj->t = new std::thread(&RtmpThread, obj);
  std::unique_lock<std::mutex> obj_lock(module.obj_mtx);
  module.objs.push_back(obj);
  obj_lock.unlock();
  return obj;
}

extern "C" int RtmpProcess(IHandle handle, TensorData* data) {
  ModuleObj* obj = (ModuleObj*)handle;
  std::unique_lock<std::mutex> lock(obj->mtx);
  if (obj->_queue.empty()) {
    obj->condition.wait(lock, [obj] {
      return !obj->_queue.empty() || !(obj->running);
    });
    if (!obj->running) {
      return -1;
    }
  }
  data->tensor_buf.output = obj->_queue.front();
  obj->_queue.pop();
  return 0;
}

extern "C" int RtmpStop(IHandle handle) {
  ModuleObj* obj = (ModuleObj*)handle;
  if (obj == NULL) {
    AppWarn("id:%d, params is null", obj->id);
    return -1;
  }
  obj->running = 0;
  if (obj->t != nullptr) {
    if (obj->t->joinable()) {
      obj->t->join();
    }
    delete obj->t;
    obj->t = nullptr;
  }
  std::unique_lock<std::mutex> lock(obj->mtx);
  while (!obj->_queue.empty()) {
    Packet* pkt = obj->_queue.front();
    obj->_queue.pop();
    if (pkt != nullptr) {
      delete pkt;
    }
  }
  lock.unlock();
  std::unique_lock<std::mutex> obj_lock(module.obj_mtx);
  for (auto itr = module.objs.begin(); itr != module.objs.end(); ++itr) {
    auto _obj = *itr;
    if (_obj->id == obj->id) {
      module.objs.erase(itr);
      break;
    }
  }
  obj_lock.unlock();
  delete obj;
  return 0;
}

extern "C" int RtmpNotify(IHandle handle) {
  ModuleObj* obj = (ModuleObj*)handle;
  if (obj == NULL) {
    AppWarn("id:%d, rtmp params is null", obj->id);
    return -1;
  }
  obj->running = 0;
  std::unique_lock<std::mutex> lock(obj->mtx);
  obj->condition.notify_all();
  return 0;
}

extern "C" int RtmpRelease(void) {
  module.running = 0;
  if (module.t != nullptr) {
    if (module.t->joinable()) {
      module.t->join();
    }
    delete module.t;
    module.t = nullptr;
  }
  return 0;
}

extern "C" int DylibRegister(DLRegister** r, int& size) {
  size = 1; // reserved
  DLRegister* p = (DLRegister*)calloc(size, sizeof(DLRegister));
  strncpy(p->name, "rtmp", sizeof(p->name));
  p->init = "RtmpInit";
  p->start = "RtmpStart";
  p->process = "RtmpProcess";
  p->stop = "RtmpStop";
  p->notify = "RtmpNotify";
  p->release = "RtmpRelease";
  *r = p;
  return 0;
}

