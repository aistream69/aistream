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
    std::thread* t;
    long int rtsp_beat;
    int running;
} RtspParams;

static long int _now_sec = 0;
static void RtspDaemon(RtspParams* rtsp_params) {
    struct timeval tv;
    RtspPlayer* player = &rtsp_params->player;

    gettimeofday(&tv, NULL);
    _now_sec = tv.tv_sec;
    rtsp_params->rtsp_beat = _now_sec;
    while(rtsp_params->running) {
        gettimeofday(&tv, NULL);
        _now_sec = tv.tv_sec;
        if(_now_sec - rtsp_params->rtsp_beat > 5) {
            AppWarn("id:%d,detected exception,restart it ...", rtsp_params->id);
            if(player->playhandle != NULL) {
                RtspPlayerStop(player);
            }
            if(RtspPlayerStart(player)) {
                AppError("start play %s failed ", player->url);
            }
            rtsp_params->rtsp_beat = _now_sec;
        }
        sleep(3);
    }

    AppDebug("run ok");
}

static int RtspCallback(unsigned char *buf, int size, void *arg) {
    HeadParams params;
    RtspParams* rtsp_params = (RtspParams* )arg;

    //printf("rtsp,id:%d,frameid:%d,size:%d,%02x:%02x:%02x:%02x:%02x\n", 
    //        rtsp_params->id, rtsp_params->frame_id, size, buf[0], buf[1], buf[2], buf[3], buf[4]);
    params.frame_id = ++rtsp_params->frame_id;
    std::unique_lock<std::mutex> lock(rtsp_params->mtx);
    if(rtsp_params->_queue.size() < (size_t)rtsp_params->queue_len_max) {
        auto _packet = new Packet(buf, size, &params);
        rtsp_params->_queue.push(_packet);
    }
    else {
        printf("warning,rtsp,id:%d, put to queue failed, quelen:%ld\n", 
                rtsp_params->id, rtsp_params->_queue.size());
    }
    rtsp_params->condition.notify_one();
    rtsp_params->rtsp_beat = _now_sec;

    return 0;
}

extern "C" int RtspInit(ElementData* data, char* params) {
    data->queue_len = GetIntValFromFile(CONFIG_FILE, "video", "queue_len");
    if(data->queue_len < 0) {
        data->queue_len = 50;
    }
    /*
    data->sleep_usec = GetIntValFromFile(CONFIG_FILE, "obj", "rtsp", "sleep_usec");
    if(data->sleep_usec < 0) {
        data->sleep_usec = 20000;
    }
    */
    return 0;
}

extern "C" IHandle RtspStart(int channel, char* params) {
    if(params == NULL) {
        AppWarn("params is null");
        return NULL;
    }

    int tcp_enable = GetIntValFromJson(params, "data", "tcp_enable");
    tcp_enable = tcp_enable < 0 ? 0 : tcp_enable;
    auto url = GetStrValFromJson(params, "data", "url");
    if(url == nullptr) {
        AppWarn("get url failed, %s", params);
        return NULL;
    }
    RtspParams* rtsp_params = new RtspParams();
    rtsp_params->id = channel;
    RtspPlayer* player = &rtsp_params->player;
    player->cb = RtspCallback;
    player->streamUsingTCP = tcp_enable;
    player->buffersize = GetIntValFromFile(CONFIG_FILE, "video", "framesize_max");
    player->buffersize = player->buffersize > 0 ? player->buffersize : 1024000;
    strncpy((char *)player->url, url.get(), sizeof(player->url));
    player->arg = rtsp_params;
    if(RtspPlayerStart(player)) {
        AppError("start play %s failed ", player->url);
        delete rtsp_params;
        return NULL;
    }
    rtsp_params->queue_len_max = GetIntValFromFile(CONFIG_FILE, "video", "queue_len");
    rtsp_params->queue_len_max = rtsp_params->queue_len_max > 0 ? rtsp_params->queue_len_max : 50;
    rtsp_params->running = 1;
    rtsp_params->t = new std::thread(&RtspDaemon, rtsp_params);

    return rtsp_params;
}

extern "C" int RtspProcess(IHandle handle, TensorData* data) {
    RtspParams* rtsp_params = (RtspParams*)handle;
    std::unique_lock<std::mutex> lock(rtsp_params->mtx);
    rtsp_params->condition.wait(lock, [rtsp_params] {
            return !rtsp_params->_queue.empty() || !(rtsp_params->running);
        });
    if(!rtsp_params->running) {
        return -1;
    }
    data->tensor_buf.output = rtsp_params->_queue.front();
    rtsp_params->_queue.pop();
    return 0;
}

extern "C" int RtspStop(IHandle handle) {
    RtspParams* rtsp_params = (RtspParams*)handle;
    if(rtsp_params == NULL) {
        AppWarn("rtsp params is null");
        return -1;
    }
    rtsp_params->running = 0;
    if(rtsp_params->t != nullptr) {
        if(rtsp_params->t->joinable()) {
            rtsp_params->t->join();
        }
        delete rtsp_params->t;
        rtsp_params->t = nullptr;
    }
    RtspPlayer *player = &rtsp_params->player;
    if(player->playhandle != NULL) {
        RtspPlayerStop(player);
    }
    std::unique_lock<std::mutex> lock(rtsp_params->mtx);
    while(!rtsp_params->_queue.empty()) {
        Packet* pkt = rtsp_params->_queue.front();
        rtsp_params->_queue.pop();
        if(pkt != nullptr) {
            delete pkt;
        }
    }
    delete rtsp_params;
    return 0;
}

extern "C" int RtspNotify(IHandle handle) {
    RtspParams* rtsp_params = (RtspParams*)handle;
    if(rtsp_params == NULL) {
        AppWarn("id:%d, player is null", rtsp_params->id);
        return -1;
    }
    rtsp_params->running = 0;
    std::unique_lock<std::mutex> lock(rtsp_params->mtx);
    rtsp_params->condition.notify_all();
    return 0;
}

extern "C" int RtspRelease(void) {
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

