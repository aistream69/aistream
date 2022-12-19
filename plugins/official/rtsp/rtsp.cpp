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
#include "tensor.h"
#include "config.h"
#include "share.h"
#include "playerapi.h"
#include "log.h"

typedef struct {
  int id;
  int frame_id;
  std::mutex mtx;
  std::condition_variable condition;
  std::queue<Packet*> _queue;
  RtspPlayer player;
  int queue_len_max;
  long int rtsp_beat;
  int running;
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
static void RtspDaemon(void) {
  struct timeval tv;
  while (module.running) {
    gettimeofday(&tv, NULL);
    module.now_sec = tv.tv_sec;
    std::unique_lock<std::mutex> obj_lock(module.obj_mtx);
    for (size_t i = 0; i < module.objs.size(); i ++) {
      auto obj = module.objs[i];
      RtspPlayer* player = &obj->player;
      if (module.now_sec - obj->rtsp_beat > 15) {
        AppWarn("id:%d,detect exception,restart it ...", obj->id);
        if (player->playhandle != NULL) {
          RtspPlayerStop(player);
        }
        if (RtspPlayerStart(player)) {
          AppError("start play %s failed ", player->url);
        }
        obj->rtsp_beat = module.now_sec;
      }
    }
    obj_lock.unlock();
    sleep(3);
  }
  AppDebug("run ok");
}

static int RtspCallback(unsigned char *buf, int size, void *arg) {
  HeadParams params = {0};
  ModuleObj* obj = (ModuleObj* )arg;

  //printf("rtsp,id:%d,frameid:%d,size:%d,%02x:%02x:%02x:%02x:%02x\n",
  //      obj->id, obj->frame_id, size, buf[0], buf[1], buf[2], buf[3], buf[4]);
  params.type = (buf[4]&0x1f) != 0x1;
  params.frame_id = ++obj->frame_id;
  std::unique_lock<std::mutex> lock(obj->mtx);
  if (obj->_queue.size() < (size_t)obj->queue_len_max) {
    auto _packet = new Packet(buf, size, &params);
    obj->_queue.push(_packet);
  } else {
    printf("warning,rtsp,id:%d, put to queue failed, quelen:%ld\n",
           obj->id, obj->_queue.size());
  }
  obj->condition.notify_one();
  obj->rtsp_beat = module.now_sec;

  return 0;
}

extern "C" int RtspInit(ElementData* data, char* params) {
  if (__sync_add_and_fetch(&module.init, 1) <= 1) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    module.now_sec = tv.tv_sec;
    module.share_params = GlobalConfig();
    module.t = new std::thread(&RtspDaemon);
    module.running = 1;
  }
  const char* cfg_file = module.share_params.config_file;
  data->queue_len = GetIntValFromFile(cfg_file, "video", "queue_len");
  if (data->queue_len < 0) {
    data->queue_len = 50;
  }
  return 0;
}

extern "C" IHandle RtspStart(int channel, char* params) {
  if (params == NULL) {
    AppWarn("id:%d, params is null", channel);
    return NULL;
  }
  int tcp_enable = GetIntValFromJson(params, "data", "tcp_enable");
  tcp_enable = tcp_enable < 0 ? 0 : tcp_enable;
  auto url = GetStrValFromJson(params, "data", "url");
  if (url == nullptr) {
    AppWarn("get url failed, %s", params);
    return NULL;
  }
  ModuleObj* obj = new ModuleObj();
  obj->id = channel;
  obj->rtsp_beat = module.now_sec;
  RtspPlayer* player = &obj->player;
  player->cb = RtspCallback;
  player->streamUsingTCP = tcp_enable;
  const char* cfg_file = module.share_params.config_file;
  player->buffersize = GetIntValFromFile(cfg_file, "video", "framesize_max");
  player->buffersize = player->buffersize > 0 ? player->buffersize : 1024000;
  strncpy((char *)player->url, url.get(), sizeof(player->url));
  player->arg = obj;
  if (RtspPlayerStart(player)) {
    AppError("start play %s failed ", player->url);
    delete obj;
    return NULL;
  }
  obj->queue_len_max = GetIntValFromFile(cfg_file, "video", "queue_len");
  obj->queue_len_max = obj->queue_len_max > 0 ? obj->queue_len_max : 50;
  obj->running = 1;
  std::unique_lock<std::mutex> obj_lock(module.obj_mtx);
  module.objs.push_back(obj);
  obj_lock.unlock();

  return obj;
}

extern "C" int RtspProcess(IHandle handle, TensorData* data) {
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

extern "C" int RtspStop(IHandle handle) {
  ModuleObj* obj = (ModuleObj*)handle;
  if (obj == NULL) {
    AppWarn("id:%d, rtsp params is null", obj->id);
    return -1;
  }
  obj->running = 0;
  RtspPlayer *player = &obj->player;
  if (player->playhandle != NULL) {
    RtspPlayerStop(player);
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

extern "C" int RtspNotify(IHandle handle) {
  ModuleObj* obj = (ModuleObj*)handle;
  if (obj == NULL) {
    AppWarn("id:%d, player is null", obj->id);
    return -1;
  }
  obj->running = 0;
  std::unique_lock<std::mutex> lock(obj->mtx);
  obj->condition.notify_all();
  return 0;
}

extern "C" int RtspRelease(void) {
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
  strncpy(p->name, "rtsp", sizeof(p->name));
  p->init = "RtspInit";
  p->start = "RtspStart";
  p->process = "RtspProcess";
  p->stop = "RtspStop";
  p->notify = "RtspNotify";
  p->release = "RtspRelease";
  *r = p;
  return 0;
}

