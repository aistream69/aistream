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

typedef struct {
    int init;
    AVCodec* codec;
    AVCodecContext* ctx;
    AVPacket *avpkt;
    AVFrame* frame;
    int y_size;
    int uv_size;
} EncodeParams;

typedef struct {
    int id;
    EncodeParams encoder;
} OSDParams;

typedef struct {
    int init;
    uint8_t yuv[3];
    int bitrate;
    int gop;
} OSDConfig;

static OSDConfig config = {0};
static void SetRectangle(auto pkt) {
    int img_w = pkt->_params.width;
    int img_h = pkt->_params.height;
    uint8_t* _y = (uint8_t* )pkt->_data;
    uint8_t* _uv = (uint8_t* )pkt->_data + img_w*img_h;
    DetectionResult* dets = (DetectionResult* )pkt->_params.ptr;
    int num = pkt->_params.ptr_size/sizeof(DetectionResult);
    uint8_t color_y = config.yuv[0], color_u = config.yuv[1], color_v = config.yuv[2];

    for(int n = 0; n < num; n ++) {
        DetectionResult* det = dets + n;
        int x = ((int)det->left)/2*2;
        int y = ((int)det->top)/2*2;
        int w = ((int)det->width)/2*2;
        int h = ((int)det->height)/2*2;
        // y
        for(int i = y; i < y + h; i ++) {
            int start_x = img_w*i + x;
            int end_x = start_x + w;
            // x axis line
            if(i == y || i == y+1 || i == y+h-1 || i == y+h-2) {
                memset(_y + start_x, color_y, w);
            }
            // y axis line
            _y[start_x] = color_y;
            _y[start_x + 1] = color_y;
            _y[end_x] = color_y;
            _y[end_x + 1] = color_y;
        }
        // uv
        int yy = y/2;
        int hh = (y + h)/2;
        for(int i = yy; i < hh; i ++) {
            int start_x = img_w*i + x;
            int end_x = start_x + w;
            // x axis line
            if(i == yy || i == hh - 1) {
                for(int j = 0; j < w; j+=2) {
                    _uv[start_x + j] = color_u;
                    _uv[start_x + j + 1] = color_v;
                }
            }
            // y axis line
            _uv[start_x] = color_u;
            _uv[start_x + 1] = color_v;
            _uv[end_x] = color_u;
            _uv[end_x + 1] = color_v;
        }
    }
}

static int YuvColorInit(char* params) {
    int size = 0;
    auto color = GetArrayBufFromJson(params, size, "color");
    if(color == nullptr || size != 3) {
        AppWarn("get color failed, size:%d, %s", size, params);
        return -1;
    }
    uint8_t rgb[3];
    for(int i = 0; i < size; i ++) {
        auto arrbuf = GetBufFromArray(color.get(), i);
        if(arrbuf == nullptr) {
            AppWarn("get color %d failed", i);
            break;
        }
        rgb[i] = atoi(arrbuf.get());
    }
    config.yuv[0] = 0.299*rgb[0] + 0.587*rgb[1] + 0.114*rgb[2]; // y
    config.yuv[1] = - 0.1687*rgb[0] - 0.3313*rgb[1] + 0.5*rgb[2] + 128; // u
    config.yuv[2] = 0.5*rgb[0] - 0.4187*rgb[1] - 0.0813*rgb[2] + 128; // v
    return 0;
}

static int EncoderInit(char* params) {
    config.bitrate = GetIntValFromJson(params, "bitrate");
    if(config.bitrate < 0) {
        AppWarn("get bitrate failed, use default 2000000");
        config.bitrate = 2000000;
    }
    config.gop = GetIntValFromJson(params, "gop");
    if(config.gop < 0) {
        AppWarn("get gop failed, use default 100");
        config.gop = 100;
    }
    return 0;
}

static int CreateEncoder(auto pkt, OSDParams* osd) {
    int dst_w = pkt->_params.width;
    int dst_h = pkt->_params.height;
    EncodeParams& encoder = osd->encoder;
    const char* codec_name = "libx264";

    /* find the mpeg1video encoder */
    encoder.codec = avcodec_find_encoder_by_name(codec_name);
    if(encoder.codec == NULL) {
        AppError("id:%d, codec %s not found", osd->id, codec_name);
        return -1;
    }
    encoder.ctx = avcodec_alloc_context3(encoder.codec);
    if(encoder.ctx == NULL) {
        AppError("id:%d, alloc video codec context failed", osd->id);
        return -1;
    }
    encoder.avpkt = av_packet_alloc();
    if(encoder.avpkt == NULL) {
        AppError("id:%d, alloc av packet failed", osd->id);
        return -1;
    }
    /* put sample parameters */
    encoder.ctx->bit_rate = config.bitrate;
    /* resolution must be a multiple of two */
    encoder.ctx->width = dst_w;
    encoder.ctx->height = dst_h;
    /* frames per second */
    encoder.ctx->time_base = (AVRational){1, 25};
    encoder.ctx->framerate = (AVRational){25, 1};

    /* emit one intra frame every ten frames
     * check frame pict_type before passing frame
     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
     * then gop_size is ignored and the output of encoder
     * will always be I frame irrespective to gop_size
     */
    encoder.ctx->gop_size = config.gop;
    encoder.ctx->max_b_frames = 0;
    //encoder.ctx->pix_fmt = AV_PIX_FMT_NV12;
    if(pkt->_params.type == AV_PIX_FMT_YUVJ420P) {
        encoder.ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    }
    else {
        encoder.ctx->pix_fmt = (AVPixelFormat)pkt->_params.type;
    }

    if(encoder.codec->id == AV_CODEC_ID_H264) {
        // ultrafast, superfast, veryfast, faster, fast, medium, slow, slower, veryslow
        av_opt_set(encoder.ctx->priv_data, "preset", "superfast", 0);
        av_opt_set(encoder.ctx->priv_data, "tune", "zerolatency", 0);
    }
    /* open it */
    if(avcodec_open2(encoder.ctx, encoder.codec, NULL) < 0) {
        AppError("id:%d, could not open codec %s", osd->id, codec_name);
        return -1;
    }

    encoder.frame = av_frame_alloc();
    if(!encoder.frame) {
        AppError("id:%d, could not allocate video frame, %s", osd->id, codec_name);
        return -1;
    }
    encoder.frame->format = encoder.ctx->pix_fmt;
    encoder.frame->width  = dst_w;
    encoder.frame->height = dst_h;

    int ret = av_frame_get_buffer(encoder.frame, 32);
    if(ret < 0) {
        AppError("id:%d, could not get frame buffer, %s", osd->id, codec_name);
        return -1;
    }
    /* make sure the frame data is writable */
    ret = av_frame_make_writable(encoder.frame);
    if(ret < 0) {
        AppError("id:%d, make writable failed, %s", osd->id, codec_name);
        return -1;
    }
    encoder.y_size = dst_w*dst_h;
    encoder.uv_size = encoder.y_size/4;
    AppDebug("create encoder success, id:%d,format:%d,codec:%s", osd->id, encoder.ctx->pix_fmt, codec_name);

    return 0;
}

static void DestroyEncoder(EncodeParams& encoder) {
    if(encoder.ctx) {
        avcodec_free_context(&encoder.ctx);
        encoder.ctx = NULL;
    }
    if(encoder.frame) {
        av_frame_free(&encoder.frame);
        encoder.frame = NULL;
    }
    if(encoder.avpkt) {
        av_packet_free(&encoder.avpkt);
        encoder.avpkt = NULL;
    }
    AppDebug("destroy encoder success");
}

static int Encoding(auto pkt, OSDParams* osd, TensorData* data) {
    int ret;
    EncodeParams& encoder = osd->encoder;
    AVFrame* frame = encoder.frame;
    AVPacket* avpkt = encoder.avpkt;
    if(frame->format == AV_PIX_FMT_YUV420P) {
        uint8_t* y = (uint8_t* )pkt->_data;
        uint8_t* u = y + encoder.y_size;
        uint8_t* v = u + encoder.uv_size;
        memcpy(frame->data[0], y, encoder.y_size);
        memcpy(frame->data[1], u, encoder.uv_size);
        memcpy(frame->data[2], v, encoder.uv_size);
    }
    else if(frame->format == AV_PIX_FMT_NV12) {
        uint8_t* y = (uint8_t* )pkt->_data;
        uint8_t* uv = (uint8_t* )pkt->_data + encoder.y_size;
        memcpy(frame->data[0], y, encoder.y_size);
        memcpy(frame->data[1], uv, encoder.uv_size*2);
    }
    else {
        printf("id:%d, unsupport yuv format:%d\n", osd->id, frame->format);
        return -1;
    }
    frame->pts = pkt->_params.frame_id;
    ret = avcodec_send_frame(encoder.ctx, frame);
    if(ret < 0) {
        AppWarn("id:%d, send frame failed, ret:%d", osd->id, ret);
        return -1;
    }
    while(ret >= 0) {
        ret = avcodec_receive_packet(encoder.ctx, avpkt);
        if(!ret) {
            //printf("##debug, recv %d:%ld\n", pkt->_params.frame_id, avpkt->pts);
            HeadParams params = {0};
            params.frame_id = pkt->_params.frame_id;
            auto _packet = new Packet(avpkt->data, avpkt->size, &params);
            data->tensor_buf.output = _packet;
            //WriteFile("test.264", avpkt->data, avpkt->size, "a+");
            //printf("frameid:%d, h264 size:%d, %02x:%02x:%02x:%02x:%02x\n", pkt->_params.frame_id, 
            //  avpkt->size, avpkt->data[0], avpkt->data[1], avpkt->data[2], avpkt->data[3], avpkt->data[4]);
            av_packet_unref(encoder.avpkt);
        }
        else if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }
        else {
            AppWarn("id:%d, recv packet failed, ret:%d", osd->id, ret);
            break;
        }
    }

    return 0;
}

extern "C" int OSDInit(ElementData* data, char* params) {
    strncpy(data->input_name[0], "osd_input", sizeof(data->input_name[0]));
    data->queue_len = GetIntValFromFile(CONFIG_FILE, "video", "queue_len");
    if(data->queue_len < 0) {
        data->queue_len = 50;
    }
    if(config.init) {
        return 0;
    }
    if(params == NULL) {
        AppWarn("params is null");
        return -1;
    }
    FFmpegInit();
    YuvColorInit(params);
    EncoderInit(params);
    config.init = 1;
    return 0;
}

extern "C" IHandle OSDStart(int channel, char* params) {
    OSDParams* osd = new OSDParams();
    osd->id = channel;
    return osd;
}

extern "C" int OSDProcess(IHandle handle, TensorData* data) {
    OSDParams* osd = (OSDParams* )handle;
    auto pkt = data->tensor_buf.input[0];

    SetRectangle(pkt);
    if(!osd->encoder.init) {
        CreateEncoder(pkt, osd);
        osd->encoder.init = 1;
    }
    Encoding(pkt, osd, data);
    //struct timeval tv1, tv2;
    //gettimeofday(&tv1, NULL);
    //gettimeofday(&tv2, NULL);
    //float cost_ms = (tv2.tv_sec - tv1.tv_sec)*1000.0 + (tv2.tv_usec - tv1.tv_usec)/1000.0;
    //printf("id:%d, frameid:%d, w:%d,h:%d, size:%ld, cost:%fms\n", osd->id, 
    //    pkt->_params.frame_id, pkt->_params.width, pkt->_params.height, pkt->_size, cost_ms);

    return 0;
}

extern "C" int OSDStop(IHandle handle) {
    OSDParams* osd = (OSDParams* )handle;
    if(osd == NULL) {
        return 0;
    }
    DestroyEncoder(osd->encoder);
    delete osd;
    return 0;
}

extern "C" int OSDRelease(void) {
    return 0;
}

extern "C" int DylibRegister(DLRegister** r, int& size) {
    size = 1;
    DLRegister* p = (DLRegister*)calloc(size, sizeof(DLRegister));
    strncpy(p->name, "osd", sizeof(p->name));
    p->init = "OSDInit";
    p->start = "OSDStart";
    p->process = "OSDProcess";
    p->stop = "OSDStop";
    p->release = "OSDRelease";
    *r = p;
    return 0;
}

