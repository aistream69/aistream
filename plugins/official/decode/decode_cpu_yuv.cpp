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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    int dec_init;
    AVFrame *decFrame;
    AVPacket decAvpkt;
    AVCodecContext *decContex;
    AVCodecParserContext *parser;
} FFmpegParam;

typedef struct {
    int id;
    int skip;
    FFmpegParam *ffmpeg;
    FrameParam yuv;
} DecodeParams;

static ShareParams share_params = {0};
static int InitFFmpeg(FFmpegParam *ffmpeg) {
    int ret;
    const AVCodec *codec;
    av_init_packet(&(ffmpeg->decAvpkt));
    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if(!codec) {
        fprintf(stderr, "Codec not found\n");
        return -1;
    }
    ffmpeg->decContex = avcodec_alloc_context3(codec);
    if(!ffmpeg->decContex) {
        fprintf(stderr, "Could not allocate video codec context\n");
        return -1;
    }
    if(codec->capabilities & AV_CODEC_CAP_TRUNCATED)
        ffmpeg->decContex->flags |= AV_CODEC_FLAG_TRUNCATED; // we do not send complete frames
    //av_opt_set(ffmpeg->decContex->priv_data, "pix_fmt", AV_PIX_FMT_NV12, 0);
    //ffmpeg->decContex->sw_pix_fmt = AV_PIX_FMT_NV12;
    /* open it */
    ret = avcodec_open2(ffmpeg->decContex, codec, NULL);
    if(ret < 0) {
        //fprintf(stderr, "Could not open codec, ret:%x, %s\n", ret, av_err2str(ret));
        fprintf(stderr, "Could not open codec, ret:%x\n", ret);
        return -1;
    }
    ffmpeg->decFrame = av_frame_alloc();
    if(!ffmpeg->decFrame) {
        fprintf(stderr, "Could not allocate video frame\n");
        return -1;
    }
    ffmpeg->parser = av_parser_init(codec->id);
    if(!ffmpeg->parser) {
        fprintf(stderr, "parser not found\n");
        return -1;
    }
    return 0;
}

static int FreeFFmpeg(FFmpegParam *ffmpeg) {
    if(ffmpeg->decContex != NULL) {
        avcodec_close(ffmpeg->decContex);
        avcodec_free_context(&(ffmpeg->decContex));
        ffmpeg->decContex = NULL;
    }
    if(ffmpeg->decFrame != NULL) {
        av_frame_free(&(ffmpeg->decFrame));
        ffmpeg->decFrame = NULL;
    }
    if(ffmpeg->parser != NULL) {
        av_parser_close(ffmpeg->parser);
        ffmpeg->parser = NULL;
    }
    return 0;
}

static int InitWithFFmpeg(FrameParam* frame, FrameParam* yuv, FFmpegParam* ffmpeg, int id) {
    int ret;
    ffmpeg->decAvpkt.size = frame->size;
    ffmpeg->decAvpkt.data = (uint8_t*)frame->buf;
    ret = avcodec_send_packet(ffmpeg->decContex, &(ffmpeg->decAvpkt));
    if(ret != 0) {
        printf("avcodec_send_packet, id:%d, type:%02x, size:%d, ret:%x\n", 
                id, frame->buf[4], frame->size, ret);
        av_packet_unref(&(ffmpeg->decAvpkt));
        return -1;
    }
    ret = avcodec_receive_frame(ffmpeg->decContex, ffmpeg->decFrame);
    if(ret != 0) {
        printf("avcodec_receive_frame err, id:%d, type:%02x, size:%d, ret:%x\n", 
                id, frame->buf[4], frame->size, ret);
        av_packet_unref(&(ffmpeg->decAvpkt));
        return -1;
    }
    av_packet_unref(&(ffmpeg->decAvpkt));
    yuv->width = ffmpeg->decFrame->width;
    yuv->height = ffmpeg->decFrame->height;
    yuv->size = yuv->width*yuv->height*3/2;
    //yuv->buf = (char *)malloc(yuv->size);
    return 0;
    
}

static int FFmpegDecoding(FrameParam* frame, FrameParam* yuv, FFmpegParam* ffmpeg, int id, bool enable) {
    int num = 0;
    int ret, len;
    char *data = frame->buf;
    int size = frame->size;
    while(size > 0) {
        len = av_parser_parse2(ffmpeg->parser, ffmpeg->decContex, 
                &(ffmpeg->decAvpkt.data), &(ffmpeg->decAvpkt.size),
                (const uint8_t *)data, size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
        if(len < 0) {
            fprintf(stderr, "Error while parsing\n");
            break;
        }
        data += len;
        size -= len;
        if(ffmpeg->decAvpkt.size <= 0) {
            continue;
        }
        ffmpeg->decAvpkt.pts = frame->frame_id;
        ret = avcodec_send_packet(ffmpeg->decContex, &(ffmpeg->decAvpkt));
        if(ret != 0) {
            printf("avcodec_send_packet, id:%d, ret:%x\n", id, ret);
            av_packet_unref(&(ffmpeg->decAvpkt));
            continue;
        }
        while(ret >= 0) {
            ret = avcodec_receive_frame(ffmpeg->decContex, ffmpeg->decFrame);
            if(!ret) {
                if(enable) {
                    if(++num > 1) {
                        AppWarn("id:%d, recv frame %d>1", id, num);
                        continue;
                    }
                    int y_size = yuv->width*yuv->height;
                    int uv_size = yuv->width*yuv->height/4;
                    yuv->buf = new char[yuv->size];
                    memcpy(yuv->buf, ffmpeg->decFrame->data[0], y_size);
                    memcpy(yuv->buf + y_size, ffmpeg->decFrame->data[1], uv_size);
                    memcpy(yuv->buf + y_size + uv_size, ffmpeg->decFrame->data[2], uv_size);
                    yuv->type = ffmpeg->decFrame->format;
                    //ConvertYUV2RGB(ffmpeg->decFrame->data[0], ffmpeg->decFrame->data[1], 
                    //        ffmpeg->decFrame->data[2], (unsigned char *)yuv->buf, 
                    //        yuv->width, yuv->height, ffmpeg->decFrame->format);
                }
            }
            else if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }
            else {
                printf("avcodec_receive_frame error:%x, id:%d\n", ret, id);
                break;
            }
        }
        av_packet_unref(&(ffmpeg->decAvpkt));
    }
    return num;
}

static int FFmpegDecode(FrameParam* frame, FrameParam* yuv, FFmpegParam* ffmpeg, int id, bool enable) {
    if(ffmpeg->dec_init) {
        return FFmpegDecoding(frame, yuv, ffmpeg, id, enable);
    }
    else if((frame->buf[4]&0x1f) != 1 && !InitWithFFmpeg(frame, yuv, ffmpeg, id)) {
        ffmpeg->dec_init = 1;
        AppDebug("find IDR ok, id:%d, %dx%d", id, yuv->width, yuv->height);
        return FFmpegDecoding(frame, yuv, ffmpeg, id, enable);
    }
    return -1;
}

extern "C" int DecodeInit(ElementData* data, char* params) {
    share_params = GlobalConfig();
    strncpy(data->input_name[0], "decode_input", sizeof(data->input_name[0]));
    data->queue_len = GetIntValFromFile(share_params.config_file, "video", "rgb_queue_len");
    if(data->queue_len < 0) {
        data->queue_len = 10;
    }
    RGBInit();
    FFmpegInit();
    return 0;
}

extern "C" IHandle DecodeStart(int channel, char* params) {
    DecodeParams* dec_params = (DecodeParams* )calloc(1, sizeof(DecodeParams));
    dec_params->id = channel;
    dec_params->ffmpeg = (FFmpegParam* )calloc(1, sizeof(FFmpegParam));
    if(InitFFmpeg(dec_params->ffmpeg) != 0) {
        AppWarn("init ffmpeg failed, id:%d", channel);
        free(dec_params->ffmpeg);
        free(dec_params);
        return NULL;
    }
    dec_params->skip = GetIntValFromFile(share_params.config_file, "video", "rgb_skip");
    if(dec_params->skip < 0) {
        dec_params->skip = 1;
    }
    AppDebug("rgb skip : %d", dec_params->skip);
    return dec_params;
}

extern "C" int DecodeProcess(IHandle handle, TensorData* data) {
    DecodeParams* dec_params = (DecodeParams* )handle;
    FrameParam* yuv = &dec_params->yuv;

    FrameParam frame = {0};
    auto pkt = data->tensor_buf.input[0];
    frame.buf = pkt->_data;
    frame.size = (int)pkt->_size;
    frame.frame_id = pkt->_params.frame_id;
    bool enable = pkt->_params.frame_id % dec_params->skip == 0;
    if(FFmpegDecode(&frame, yuv, dec_params->ffmpeg, dec_params->id, enable) > 0) {
        HeadParams params = {0};
        params.ptr = yuv->buf;
        params.ptr_size = yuv->size;
        params.type = yuv->type;
        params.frame_id = pkt->_params.frame_id;
        params.width = yuv->width;
        params.height = yuv->height;
        auto _packet = new Packet(nullptr, 0, &params);
        data->tensor_buf.output = _packet;
    }
    return 0;
}

extern "C" int DecodeStop(IHandle handle) {
    DecodeParams* dec_params = (DecodeParams* )handle;
    if(dec_params == NULL) {
        AppWarn("id:%d, dec is null", dec_params->id);
        return -1;
    }
    //FrameParam* yuv = &dec_params->yuv;
    //if(yuv->buf != NULL) {
    //    free(yuv->buf);
    //}
    FreeFFmpeg(dec_params->ffmpeg);
    free(dec_params->ffmpeg);
    free(dec_params);
    return 0;
}

extern "C" int DecodeRelease(void) {
    return 0;
}

extern "C" int DylibRegister(DLRegister** r, int& size) {
    size = 1;
    DLRegister* p = (DLRegister*)calloc(size, sizeof(DLRegister));
    strncpy(p->name, "decode", sizeof(p->name));
    p->init = "DecodeInit";
    p->start = "DecodeStart";
    p->process = "DecodeProcess";
    p->stop = "DecodeStop";
    p->release = "DecodeRelease";
    *r = p;
    return 0;
}

