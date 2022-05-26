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
#include <dirent.h>
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
#include "share.h"
#include "common.h"
#include "log.h"

#define SEGMENT_TIME_SEC    "2"
#define HLS_LIST_SIZE       "2"
#define HLS_LIST_NUM        2

typedef struct {
    int id;
    char type[64]; // "hls"/"http-flv"/"ws"
    pthread_t pid;
    int frame_id;
    AVFormatContext *ifmt_ctx;
    AVFormatContext *ofmt_ctx_v;
    AVOutputFormat *ofmt_v;
    AVIOContext *avio;
    int find_idr;
    int stream_index;
    std::mutex mtx;
    std::queue<std::shared_ptr<Packet>> _queue;
    int running;
} PreviewParams;

typedef struct {
    int id;
    int last_ts_num;
    int last_ts_same_cnt;
} TsFile;

typedef struct {
    int queue_len;
    int framesize_max;
    NginxParams nginx;
    pthread_t pid;
    std::mutex obj_mtx;
    std::vector<TsFile> obj_vec;
    int running;
} PreviewConfig;

static PreviewConfig config = {0};
static int DelLastTsFile(char *path) {
    DIR *dirp;
    struct dirent *dp;
    char filename[1024];
    dirp = opendir(path);
    if(dirp != NULL) {
        while ((dp = readdir(dirp)) != NULL) {
            if(!strcmp(dp->d_name,".") || !strcmp(dp->d_name,".."))
                continue;
            snprintf(filename, sizeof(filename), "%s/%s", path, dp->d_name);
            if(!access(filename, F_OK)) {
                remove(filename);
            }
        }
        closedir(dirp);
    }
    return 0;
}

static int DelOldTsFile(auto _ts) { // ugly code
    FILE *fp;
    int j, k;
    char buf[256], m3u8[384], ts_file[384];

    snprintf(m3u8, sizeof(m3u8), "%s/m3u8/stream%d/play.m3u8", config.nginx.workdir, _ts->id);
    if(!access(m3u8, F_OK)) {
        fp = fopen(m3u8, "rb");
        if(fp == NULL) {
            AppWarn("fopen %s failed", m3u8);
            return -1;
        }
        char *p1, *p2, *p3;
        int try_cnt = 48;
        int len = fread(buf, 1, sizeof(buf), fp);
        if(len > 64) {
            p1 = buf + len - 2;
            while((*p1 != '\r') && (*p1 != '\n') && (try_cnt > 0)) {
                p1 --;
                try_cnt --;
            }
            if(try_cnt <= 0) {
                fclose(fp);
                return 0;
            }
            p1 ++;
            p2 = p1 + 4; // strlen("play") is 4
            p3 = strstr(p2, ".ts");
            if(p3 == NULL) {
                fclose(fp);
                return 0;
            }
            *p3 = '\0';
            int ts_num;
            int latest_num = atoi(p2);
            if(latest_num > 5) {
                for(j = 1; j <= 5; j ++) {
                    ts_num = latest_num - HLS_LIST_NUM - j;
                    if(ts_num < 0) {
                        break;
                    }
                    snprintf(ts_file, sizeof(ts_file), 
                            "%s/m3u8/stream%d/play%d.ts", config.nginx.workdir, _ts->id, ts_num);
                    if(!access(ts_file, F_OK)) {
                        remove(ts_file);
                    }
                }
            }
            if(latest_num != _ts->last_ts_num) {
                _ts->last_ts_num = latest_num;
                _ts->last_ts_same_cnt = 0;
            }
            else {
                _ts->last_ts_same_cnt ++;
                if(_ts->last_ts_same_cnt > 360) {
                    AppWarn("stream%d, last ts file is too long, rm m3u8", _ts->id);
                    remove(m3u8);
                    _ts->last_ts_same_cnt = 0;
                }
            }
            for(k = 0; k < 3; k ++) {
                snprintf(ts_file, sizeof(ts_file), 
                        "%s/m3u8/stream%d/play%d.ts", config.nginx.workdir, _ts->id, latest_num + k);
                if(access(ts_file, F_OK) != 0) {
                    continue;
                }
                int file_size = GetFileSize(ts_file);
                if(file_size > 30000000) {
                    printf("exception ts file, remove %s, size:%d\n", ts_file, file_size);
                    remove(ts_file);
                }
            }
        }
        fclose(fp);
    }
    else {
        for(k = 0; k < 3; k ++) {
            snprintf(ts_file, sizeof(ts_file), 
                    "%s/m3u8/stream%d/play%d.ts", config.nginx.workdir, _ts->id, k);
            if(access(ts_file, F_OK) != 0) {
                continue;
            }
            int file_size = GetFileSize(ts_file);
            if(file_size > 30000000) {
                printf("exception ts file, remove %s, size:%d\n", ts_file, file_size);
                remove(ts_file);
            }
        }
    }

    return 0;
}

static void *PreviewDaemonThread(void *arg) {
    while(config.running) {
        std::unique_lock<std::mutex> lock(config.obj_mtx);
        auto obj_vec = config.obj_vec;
        for(auto itr = obj_vec.begin(); itr != obj_vec.end(); ++itr) {
            DelOldTsFile(itr);
        }
        lock.unlock();
        sleep(5);
    }
    return NULL;
}

//if data is zero, waiting until valid, don't return, else avformat_open_input may be panic
static int ReadVideoPacket(void *opaque, uint8_t *buf, int size){
    int len = 0;
    PreviewParams *preview = (PreviewParams *)opaque;
    do {
        if(preview->_queue.size() == 0) {
            usleep(40000);
            continue;
        }
        std::unique_lock<std::mutex> lock(preview->mtx);
        auto pkt = preview->_queue.front();
        preview->_queue.pop();
        lock.unlock();
        if(!preview->find_idr){
            if((pkt->_data[4] & 0x1f) != 1) {
                preview->find_idr = 1;
                AppDebug("id:%d, find IDR ok", preview->id);
            }
        }
        if(preview->find_idr) {
            if(len + (int)pkt->_size < size) {
                memcpy(buf + len, pkt->_data, pkt->_size);
                len += pkt->_size;
            }
            else {
                printf("read video packet exception, id:%d,size:%ld,len:%d\n", 
                        preview->id, pkt->_size, len);
                break;
            }
            preview->frame_id = pkt->_params.frame_id;
        }
    } while(len == 0 && preview->running);
    return len;
}

static int CreatePreview(PreviewParams* preview) {
    int ret;
    unsigned char *aviobuffer;
    const char *out_filename_v;
    char path[URL_LEN*2], errstr[256];
    int aviobuf_size = config.framesize_max*2;

    preview->frame_id = -1;
    preview->stream_index = -1;
    if(!strncmp(preview->type, "hls", sizeof(preview->type))) {
        snprintf(path, sizeof(path), "%s/m3u8/stream%d", config.nginx.workdir, preview->id);
        DirCheck(path);
        DelLastTsFile(path);
        strncat(path, "/play.m3u8", sizeof(path));
    }
    else if(!strncmp(preview->type, "http-flv", sizeof(preview->type))) {
        snprintf(path, sizeof(path), "rtmp://127.0.0.1:1935/myapp/stream%d", preview->id);
    }
    else {
        strcpy(path, "null");
        AppWarn("id:%d, unsupport mode:%s", preview->id, preview->type);
        return -1;
    }
    out_filename_v = path;

    printf("preview, %d, waiting stream ...\n", preview->id);
    while(1) {
        if(!preview->running){
            printf("preview thread exit, id:%d\n", preview->id);
            return -1;
        }
        std::unique_lock<std::mutex> lock(preview->mtx);
        if(preview->_queue.size() > 0) {
            break;
        }
        lock.unlock();
        usleep(40000);
    }
    printf("preview, %d, waiting stream ok\n", preview->id);

    aviobuffer = (unsigned char *)av_mallocz(aviobuf_size);
    if(aviobuffer == NULL) {
        AppError("id:%d, av_mallocz %d failed", preview->id, aviobuf_size);
        return -1;
    }
    preview->ifmt_ctx = avformat_alloc_context();
    if(preview->ifmt_ctx == NULL) {
        AppError("id:%d, Could not alloc input context", preview->id);
        return -1;
    }
    preview->avio = avio_alloc_context(aviobuffer, aviobuf_size, 0, preview, ReadVideoPacket, NULL, NULL);
    if(preview->avio == NULL) {
        AppError("id:%d, avio_alloc_context %d failed", preview->id, aviobuf_size);
        return -1;
    }
    preview->ifmt_ctx->pb = preview->avio;

    if ((ret = avformat_open_input(&(preview->ifmt_ctx), NULL, NULL, NULL)) < 0) {
        av_strerror(ret, errstr, sizeof(errstr));
        AppError("id:%d, Could not open input file, %s", preview->id, errstr);
        return -1;
    }
    if ((ret = avformat_find_stream_info(preview->ifmt_ctx, 0)) < 0) {
        AppError("id:%d, Failed to retrieve input stream information", preview->id);
        return -1;
    }

    if(!strncmp(preview->type, "hls", sizeof(preview->type))) {
        avformat_alloc_output_context2(&(preview->ofmt_ctx_v), NULL, "hls", out_filename_v);
        if (!(preview->ofmt_ctx_v)) {
            AppError("id:%d, Could not create output context", preview->id);
            return -1;
        }
        av_opt_set(preview->ofmt_ctx_v->priv_data, "segment_time", SEGMENT_TIME_SEC, 0);
        av_opt_set(preview->ofmt_ctx_v->priv_data, "hls_list_size", HLS_LIST_SIZE, 0);
    }
    else if(!strncmp(preview->type, "http-flv", sizeof(preview->type))) {
        avformat_alloc_output_context2(&(preview->ofmt_ctx_v), NULL, "flv", out_filename_v);
        if (!(preview->ofmt_ctx_v)) {
            AppError("id:%d, Could not create output context", preview->id);
            return -1;
        }
        av_opt_set(preview->ofmt_ctx_v->priv_data, "live", NULL, 0);
    }
    preview->ofmt_v = preview->ofmt_ctx_v->oformat;

    for(int i = 0; i < (int)preview->ifmt_ctx->nb_streams; i++) {
        //Create output AVStream according to input AVStream
        AVFormatContext *ofmt_ctx;
        AVStream *in_stream = preview->ifmt_ctx->streams[i];
        AVStream *out_stream = NULL;
        if(preview->ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
            preview->stream_index = i;
            out_stream = avformat_new_stream(preview->ofmt_ctx_v, in_stream->codec->codec);
            ofmt_ctx = preview->ofmt_ctx_v;
        }
        else{
            AppWarn("id:%d, streams:%d, unsupport type : %d", preview->id, 
                    preview->ifmt_ctx->nb_streams, preview->ifmt_ctx->streams[i]->codec->codec_type);
            return -1;
        }
        if (!out_stream) {
            AppError("id:%d, Failed allocating output stream", preview->id);
            return -1;
        }
        //Copy the settings of AVCodecContext
        if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
            AppError("id:%d, failed copy context", preview->id);
            return -1;
        }
        out_stream->codec->codec_tag = 0;
        if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
            out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
        }
    }
    av_dump_format(preview->ifmt_ctx, 0, NULL, 0);
    av_dump_format(preview->ofmt_ctx_v, 0, out_filename_v, 1);
    if (!(preview->ofmt_v->flags & AVFMT_NOFILE)) {
        if (avio_open(&(preview->ofmt_ctx_v->pb), out_filename_v, AVIO_FLAG_WRITE) < 0) {
            AppError("id:%d, Could not open output file '%s'", preview->id, out_filename_v);
            return -1;
        }
    }
    if (preview->stream_index != -1 && avformat_write_header(preview->ofmt_ctx_v, NULL) < 0) {
        AppError("id:%d, Error occurred when opening video output file", preview->id);
        return -1;
    }

    return 0;
}

static int DestroyPreview(PreviewParams *preview) {
    //Write file trailer
    if(preview->stream_index != -1) {
        av_write_trailer(preview->ofmt_ctx_v);
    }
    if(preview->ifmt_ctx != NULL) {
        avformat_close_input(&(preview->ifmt_ctx));
        preview->ifmt_ctx = NULL;
    }
    /* close output */
    if(preview->ofmt_ctx_v && !(preview->ofmt_v->flags & AVFMT_NOFILE))
        avio_close(preview->ofmt_ctx_v->pb);
    if(preview->ofmt_ctx_v != NULL) {
        avformat_free_context(preview->ofmt_ctx_v);
        preview->ofmt_ctx_v = NULL;
    }
    if(preview->avio != NULL) {
        av_freep(&(preview->avio->buffer));
        av_freep(&(preview->avio));
        preview->avio = NULL;
    }
    preview->find_idr = 0;

    char m3u8[URL_LEN*2];
    snprintf(m3u8, sizeof(m3u8), "%s/m3u8/stream%d/play.m3u8", config.nginx.workdir, preview->id);
    if(!access(m3u8, F_OK)) {
        remove(m3u8);
    }

    return 0;
}

// TODO : preview daemon thread, use recursion function?
static void *PreviewThread(void *arg) {
    AVPacket pkt;
    int frame_index = 0, exception = 0;
    PreviewParams* preview = (PreviewParams* )arg;

    if(CreatePreview(preview) != 0) {
        AppWarn("init preview failed, id:%d,type:%s", preview->id, preview->type);
        preview->running = 0;
        return NULL;
    }
    while(preview->running) {
        if(av_read_frame(preview->ifmt_ctx, &pkt) < 0) {
            printf("id:%d, av_read_frame failed, continue\n", preview->id);
            usleep(200000);
            continue;
        }
        if(pkt.stream_index != preview->stream_index) {
            printf("id:%d, exception stream_index, %d:%d\n", 
                    preview->id, pkt.stream_index, preview->stream_index);
            if(exception++ > 500) {
                AppWarn("id:%d, exception stream_index, %d:%d,%d, restart thread", 
                        preview->id, pkt.stream_index, preview->stream_index, exception);
                break;
            }
            continue;
        }
        AVStream* out_stream = preview->ofmt_ctx_v->streams[0];
        AVStream* in_stream  = preview->ifmt_ctx->streams[pkt.stream_index];
        if(pkt.pts == AV_NOPTS_VALUE) {
            AVRational _time_base = in_stream->time_base;
            int64_t calc_duration=(double)AV_TIME_BASE/av_q2d(in_stream->r_frame_rate);
            pkt.pts=(double)(frame_index*calc_duration)/(double)(av_q2d(_time_base)*AV_TIME_BASE);
            pkt.dts=pkt.pts;
            pkt.duration=(double)calc_duration/(double)(av_q2d(_time_base)*AV_TIME_BASE);
        }
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, 
                out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, 
                out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.stream_index = 0;
        pkt.pos = -1;
        if(av_interleaved_write_frame(preview->ofmt_ctx_v, &pkt) < 0) {
            AppError("id:%d, err muxing packet\n", preview->id);
            break;
        }
        av_free_packet(&pkt);
        frame_index++;
        exception = 0;
    }
    preview->running = 0;
    DestroyPreview(preview);
    AppDebug("id:%d, run ok", preview->id);

    return NULL;
}

extern "C" int PreviewInit(ElementData* data, char* params) {
    strncpy(data->input_name[0], "preview_input", sizeof(data->input_name[0]));
    if(config.queue_len != 0) {
        return 0;
    }
    FFmpegInit();
    NginxInit(config.nginx);
    config.framesize_max = GetIntValFromFile(CONFIG_FILE, "video", "framesize_max");
    if(config.framesize_max < 0) {
        AppWarn("get video framesize_max failed");
        config.framesize_max = 1024000;
    }
    config.queue_len = GetIntValFromFile(CONFIG_FILE, "video", "queue_len");
    if(config.queue_len < 0) {
        AppWarn("get video queue_len failed");
        config.queue_len = 50;
    }
    config.running = 1;
    if(pthread_create(&config.pid, NULL, PreviewDaemonThread, NULL) != 0) {
        AppError("create preview daemon thread failed");
    }

    return 0;
}

extern "C" IHandle PreviewStart(int channel, char* params) {
    PreviewParams* preview = new PreviewParams();
    preview->id = channel;
    if(params == NULL) {
        AppWarn("id:%d, params is null", channel);
        return preview;
    }
    auto type = GetStrValFromJson(params, "preview");
    if(type == nullptr) {
        AppWarn("id:%d, get type failed", channel);
        return preview;
    }
    strncpy(preview->type, type.get(), sizeof(preview->type));
    if(!strncmp(preview->type, "0", sizeof(preview->type))) {
        return preview;
    }
    if(!strncmp(preview->type, "hls", sizeof(preview->type)) || 
            !strncmp(preview->type, "http-flv", sizeof(preview->type))) {
        AppDebug("id:%d, preview type:%s", channel, preview->type);
    }
    else {
        AppWarn("unsupport type : %s", preview->type);
        return preview;
    }
    preview->running = 1;
    if(pthread_create(&preview->pid, NULL, PreviewThread, preview) != 0) {
        AppError("create preview thread failed, id:%d", channel);
        return preview;
    }

    TsFile ts;
    ts.id = channel;
    ts.last_ts_num = 0;
    ts.last_ts_same_cnt = 0;
    std::unique_lock<std::mutex> lock(config.obj_mtx);
    config.obj_vec.push_back(ts);

    return preview;
}

extern "C" int PreviewProcess(IHandle handle, TensorData* data) {
    PreviewParams* preview = (PreviewParams* )handle;
    TensorBuffer& tensor_buf = data->tensor_buf;

    if(!preview->running) {
        return 0;
    }
    auto pkt = tensor_buf.input[0];
    std::unique_lock<std::mutex> lock(preview->mtx);
    if(preview->_queue.size() < (size_t)config.queue_len) {
        HeadParams params = {0};
        params.frame_id = pkt->_params.frame_id;
        auto _packet = std::make_shared<Packet>(pkt->_data, pkt->_size, &params);
        preview->_queue.push(_packet);
    }
    else if(pkt->_params.frame_id % 200 == 0) {
        printf("warning,preview,id:%d,frameid:%d,put to queue failed,quelen:%ld\n", 
                preview->id, pkt->_params.frame_id, preview->_queue.size());
    }

    return 0;
}

extern "C" int PreviewStop(IHandle handle) {
    PreviewParams* preview = (PreviewParams* )handle;
    preview->running = 0;
    if(pthread_join(preview->pid, NULL) != 0) {
        AppError("pthread join failed, %s", strerror(errno));
    }
    std::unique_lock<std::mutex> lock(config.obj_mtx);
    auto obj_vec = config.obj_vec;
    for(auto itr = obj_vec.begin(); itr != obj_vec.end(); ++itr) {
        if(itr->id == preview->id) {
            obj_vec.erase(itr);
            break;
        }
    }
    delete preview;
    return 0;
}

extern "C" int PreviewRelease(void) {
    config.running = 0;
    if(pthread_join(config.pid, NULL) != 0) {
        AppError("pthread join failed, %s", strerror(errno));
    }
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

