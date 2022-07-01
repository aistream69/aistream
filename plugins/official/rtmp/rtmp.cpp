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
    std::thread* _t;
    long int rtmp_beat;
    int running;
    int _running;
} RtmpParams;

static long int _now_sec = 0;
static void CopyToPacket(uint8_t* buf, int size, RtmpParams* rtmp) {
    HeadParams params = {0};
    params.frame_id = ++rtmp->frame_id;
    std::unique_lock<std::mutex> lock(rtmp->mtx);
    if(rtmp->_queue.size() < (size_t)rtmp->queue_len_max) {
        auto _packet = new Packet(buf, size, &params);
        rtmp->_queue.push(_packet);
    }
    else {
        printf("warning,rtmp,id:%d, put to queue failed, quelen:%ld\n", 
                rtmp->id, rtmp->_queue.size());
    }
    rtmp->condition.notify_one();
}

static void RtmpThread(RtmpParams* rtmp) {
    int ret;
    char errstr[256];
    AVBSFContext* bsf_ctx = NULL; 
    AVFormatContext* ifmt_ctx = NULL;
    int video_index = -1, audio_index = -1;

    if((ret = avformat_open_input(&ifmt_ctx, rtmp->url, NULL, NULL)) < 0) {
        av_strerror(ret, errstr, sizeof(errstr));
        AppWarn("id:%d, could not open input file, %s", rtmp->id, errstr);
        return;
    }
    if((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
        AppWarn("id:%d, failed to retrieve input stream information", rtmp->id);
        return;
    }
    for(int i = 0; i < (int)ifmt_ctx->nb_streams; i++) {
        if(ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            video_index = i;
        }
        else if(ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
            audio_index = i;
        }
        else{
            AppWarn("id:%d, streams:%d, unsupport type : %d", rtmp->id,
                    ifmt_ctx->nb_streams, ifmt_ctx->streams[i]->codec->codec_type);
            return;
        }
    }
    const AVBitStreamFilter* pfilter = av_bsf_get_by_name("h264_mp4toannexb");
    if(pfilter == NULL) {
        AppWarn("id:%d,get bsf failed", rtmp->id);
        return;
    }
    if((ret = av_bsf_alloc(pfilter, &bsf_ctx)) != 0) {
        AppWarn("id:%d,alloc bsf failed", rtmp->id);
        return;
    }
    ret = avcodec_parameters_from_context(bsf_ctx->par_in, ifmt_ctx->streams[video_index]->codec);
    if(ret < 0) {
        AppWarn("id:%d,set codec failed", rtmp->id);
        return;
    }
    bsf_ctx->time_base_in = ifmt_ctx->streams[video_index]->codec->time_base;
    ret = av_bsf_init(bsf_ctx);
    if(ret < 0) {
        AppWarn("id:%d,init bsf failed", rtmp->id);
        return;
    }
    AppDebug("id:%d, start ...", rtmp->id);

    AVPacket pkt;
    while(rtmp->running && rtmp->_running) {
        ret = av_read_frame(ifmt_ctx, &pkt);
        if(ret < 0) {
            printf("id:%d, av_read_frame failed, %d, continue\n", rtmp->id, ret);
            usleep(200000);
            continue;
        }
        if(pkt.stream_index != video_index) {
            printf("warning, id:%d, not support stream index %d, video:%d,audio:%d\n", 
                    rtmp->id, pkt.stream_index, video_index, audio_index);
            av_packet_unref(&pkt);
            continue;
        }
        ret = av_bsf_send_packet(bsf_ctx, &pkt);
        if(ret != 0) {
            printf("warning, send pkt failed, ret:%d\n", ret);
            break;
        }
        ret = av_bsf_receive_packet(bsf_ctx, &pkt);
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            printf("warning, recv pkt EAGAIN/EOF, ret:%d\n", ret);
            break;
        }
        else if (ret < 0) {
            printf("warning, recv pkt failed, ret:%d\n", ret);
            break;
        }
        //printf("##test, frameid:%d, pkt size:%d, %02x:%02x:%02x:%02x:%02x\n", rtmp->frame_id, 
        //        pkt.size, pkt.data[0], pkt.data[1], pkt.data[2], pkt.data[3], pkt.data[4]);
        CopyToPacket(pkt.data, pkt.size, rtmp);
        av_packet_unref(&pkt);
        rtmp->rtmp_beat = _now_sec;
    }

    av_bsf_free(&bsf_ctx);
    avformat_close_input(&ifmt_ctx);
    AppDebug("id:%d, run ok", rtmp->id);
}

static void RtmpDaemon(RtmpParams* rtmp) {
    struct timeval tv;

    gettimeofday(&tv, NULL);
    _now_sec = tv.tv_sec;
    rtmp->rtmp_beat = _now_sec;
    while(rtmp->running) {
        gettimeofday(&tv, NULL);
        _now_sec = tv.tv_sec;
        if(_now_sec - rtmp->rtmp_beat > 15) {
            AppWarn("id:%d,detected exception,restart it ...", rtmp->id);
            rtmp->_running = 0;
            if(rtmp->t != nullptr) {
                if(rtmp->t->joinable()) {
                    rtmp->t->join();
                }
                delete rtmp->t;
                rtmp->t = nullptr;
            }
            std::unique_lock<std::mutex> lock(rtmp->mtx);
            while(!rtmp->_queue.empty()) {
                Packet* pkt = rtmp->_queue.front();
                rtmp->_queue.pop();
                if(pkt != nullptr) {
                    delete pkt;
                }
            }
            lock.unlock();
            rtmp->_running = 1;
            rtmp->t = new std::thread(&RtmpThread, rtmp);
            rtmp->rtmp_beat = _now_sec;
        }
        sleep(3);
    }

    AppDebug("run ok");
}


extern "C" int RtmpInit(ElementData* data, char* params) {
    data->queue_len = GetIntValFromFile(CONFIG_FILE, "video", "queue_len");
    if(data->queue_len < 0) {
        data->queue_len = 50;
    }
    FFmpegInit();
    return 0;
}

extern "C" IHandle RtmpStart(int channel, char* params) {
    if(params == NULL) {
        AppWarn("params is null");
        return NULL;
    }
    auto url = GetStrValFromJson(params, "data", "url");
    if(url == nullptr) {
        AppWarn("get url failed, %s", params);
        return NULL;
    }
    RtmpParams* rtmp = new RtmpParams();
    rtmp->id = channel;
    rtmp->queue_len_max = GetIntValFromFile(CONFIG_FILE, "video", "queue_len");
    rtmp->queue_len_max = rtmp->queue_len_max > 0 ? rtmp->queue_len_max : 50;
    strncpy(rtmp->url, url.get(), sizeof(rtmp->url));
    rtmp->running = 1;
    rtmp->_running = 1;
    rtmp->t = new std::thread(&RtmpThread, rtmp);
    // TODO: only create one thread in Init func
    rtmp->_t = new std::thread(&RtmpDaemon, rtmp);
    return rtmp;
}

extern "C" int RtmpProcess(IHandle handle, TensorData* data) {
    RtmpParams* rtmp = (RtmpParams*)handle;
    std::unique_lock<std::mutex> lock(rtmp->mtx);
    rtmp->condition.wait(lock, [rtmp] {
            return !rtmp->_queue.empty() || !(rtmp->running);
        });
    if(!rtmp->running) {
        return -1;
    }
    data->tensor_buf.output = rtmp->_queue.front();
    rtmp->_queue.pop();
    return 0;
}

extern "C" int RtmpStop(IHandle handle) {
    RtmpParams* rtmp = (RtmpParams*)handle;
    if(rtmp == NULL) {
        AppWarn("rtmp params is null");
        return -1;
    }
    rtmp->running = 0;
    if(rtmp->_t != nullptr) {
        if(rtmp->_t->joinable()) {
            rtmp->_t->join();
        }
        delete rtmp->_t;
        rtmp->_t = nullptr;
    }
    if(rtmp->t != nullptr) {
        if(rtmp->t->joinable()) {
            rtmp->t->join();
        }
        delete rtmp->t;
        rtmp->t = nullptr;
    }
    std::unique_lock<std::mutex> lock(rtmp->mtx);
    while(!rtmp->_queue.empty()) {
        Packet* pkt = rtmp->_queue.front();
        rtmp->_queue.pop();
        if(pkt != nullptr) {
            delete pkt;
        }
    }
    lock.unlock();
    delete rtmp;
    return 0;
}

extern "C" int RtmpNotify(IHandle handle) {
    RtmpParams* rtmp = (RtmpParams*)handle;
    if(rtmp == NULL) {
        AppWarn("id:%d, rtmp params is null", rtmp->id);
        return -1;
    }
    rtmp->running = 0;
    std::unique_lock<std::mutex> lock(rtmp->mtx);
    rtmp->condition.notify_all();
    return 0;
}

extern "C" int RtmpRelease(void) {
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

