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
    FrameParam rgb;
} DecodeParams;

static int g_init = 0;

// Conversion from RGB to YUV420
static int RGB2YUV_YR[256], RGB2YUV_YG[256], RGB2YUV_YB[256];
static int RGB2YUV_UR[256], RGB2YUV_UG[256], RGB2YUV_UBVR[256];
static int RGB2YUV_VG[256], RGB2YUV_VB[256];
// Conversion from YUV420 to RGB24
static long int crv_tab[256];
static long int cbu_tab[256];
static long int cgu_tab[256];
static long int cgv_tab[256];
static long int tab_76309[256];
static unsigned char clp[1024]; //for clip in CCIR601

// Table used for RGB to YUV420 conversion
static void InitLookupTable(void) {
    int i;
    for (i = 0; i < 256; i++) RGB2YUV_YR[i] = (float)65.481 * (i<<8);
    for (i = 0; i < 256; i++) RGB2YUV_YG[i] = (float)128.553 * (i<<8);
    for (i = 0; i < 256; i++) RGB2YUV_YB[i] = (float)24.966 * (i<<8);
    for (i = 0; i < 256; i++) RGB2YUV_UR[i] = (float)37.797 * (i<<8);
    for (i = 0; i < 256; i++) RGB2YUV_UG[i] = (float)74.203 * (i<<8);
    for (i = 0; i < 256; i++) RGB2YUV_VG[i] = (float)93.786 * (i<<8);
    for (i = 0; i < 256; i++) RGB2YUV_VB[i] = (float)18.214 * (i<<8);
    for (i = 0; i < 256; i++) RGB2YUV_UBVR[i] = (float)112 * (i<<8);
}

//Initialize conversion table for YUV420 to RGB
static void InitConvertTable(void) {
    long int crv,cbu,cgu,cgv;
    int i,ind;
    crv = 104597; cbu = 132201; /* fra matrise i global.h */
    cgu = 25675; cgv = 53279;
    for (i = 0; i < 256; i++) {
        crv_tab[i] = (i-128) * crv;
        cbu_tab[i] = (i-128) * cbu;
        cgu_tab[i] = (i-128) * cgu;
        cgv_tab[i] = (i-128) * cgv;
        tab_76309[i] = 76309*(i-16);
    }
    for (i=0; i<384; i++)
        clp[i] =0;
    ind=384;
    for (i=0;i<256; i++)
        clp[ind++]=i;
    ind=640;
    for (i=0;i<384;i++)
        clp[ind++]=255;
}

static void ConvertNv12ToRGB24(unsigned char *src0, unsigned char *src1, unsigned char *dst_ori, int width,int height) {
    int y1,y2,u,v;
    unsigned char *py1,*py2;
    int i,j, c1, c2, c3, c4;
    unsigned char *d1, *d2;
    int cnt = 0;
    
    py1=src0;
    py2=py1+width;
    d1=dst_ori;
    d2=d1+3*width;
    for (j = 0; j < height; j += 2) {
        for (i = 0; i < width; i += 2) {
            u = *src1++;
            v = *src1++;
            c1 = crv_tab[v];
            c2 = cgu_tab[u];
            c3 = cgv_tab[v];
            c4 = cbu_tab[u];
            //up-left
            y1 = tab_76309[*py1++];
            *d1++ = clp[384+((y1 + c4)>>16)];
            *d1++ = clp[384+((y1 - c2 - c3)>>16)];
            *d1++ = clp[384+((y1 + c1)>>16)];
            //down-left
            y2 = tab_76309[*py2++];
            *d2++ = clp[384+((y2 + c4)>>16)];
            *d2++ = clp[384+((y2 - c2 - c3)>>16)];
            *d2++ = clp[384+((y2 + c1)>>16)];
            //up-right
            y1 = tab_76309[*py1++];
            *d1++ = clp[384+((y1 + c4)>>16)];
            *d1++ = clp[384+((y1 - c2 - c3)>>16)];
            *d1++ = clp[384+((y1 + c1)>>16)];
            //down-right
            y2 = tab_76309[*py2++];
            *d2++ = clp[384+((y2 + c4)>>16)];
            *d2++ = clp[384+((y2 - c2 - c3)>>16)];
            *d2++ = clp[384+((y2 + c1)>>16)];
            cnt += 12;
        }
        d1 += 3*width;
        d2 += 3*width;
        py1+= width;
        py2+= width;
    }
}

static void ConvertYuv420pToRGB24(unsigned char *src0, unsigned char *src1, unsigned char *src2, 
        unsigned char *dst_ori, int width,int height) {
    int y1,y2,u,v;
    unsigned char *py1,*py2;
    int i,j, c1, c2, c3, c4;
    unsigned char *d1, *d2;
    int cnt = 0;
    
    py1=src0;
    py2=py1+width;
    d1=dst_ori;
    d2=d1+3*width;
    for (j = 0; j < height; j += 2) {
        for (i = 0; i < width; i += 2) {
            u = *src1++;
            v = *src2++;
            c1 = crv_tab[v];
            c2 = cgu_tab[u];
            c3 = cgv_tab[v];
            c4 = cbu_tab[u];
            //up-left
            y1 = tab_76309[*py1++];
            *d1++ = clp[384+((y1 + c4)>>16)];
            *d1++ = clp[384+((y1 - c2 - c3)>>16)];
            *d1++ = clp[384+((y1 + c1)>>16)];
            //down-left
            y2 = tab_76309[*py2++];
            *d2++ = clp[384+((y2 + c4)>>16)];
            *d2++ = clp[384+((y2 - c2 - c3)>>16)];
            *d2++ = clp[384+((y2 + c1)>>16)];
            //up-right
            y1 = tab_76309[*py1++];
            *d1++ = clp[384+((y1 + c4)>>16)];
            *d1++ = clp[384+((y1 - c2 - c3)>>16)];
            *d1++ = clp[384+((y1 + c1)>>16)];
            //down-right
            y2 = tab_76309[*py2++];
            *d2++ = clp[384+((y2 + c4)>>16)];
            *d2++ = clp[384+((y2 - c2 - c3)>>16)];
            *d2++ = clp[384+((y2 + c1)>>16)];
            cnt += 12;
        }
        d1 += 3*width;
        d2 += 3*width;
        py1+= width;
        py2+= width;
    }
}

// Convert from YUV420 to RGB24
static void ConvertYUV2RGB(unsigned char *src0,unsigned char *src1,unsigned char *src2,
        unsigned char *dst_ori, int width,int height, int format) {
    if(format == AV_PIX_FMT_NV12) {
        ConvertNv12ToRGB24(src0, src1, dst_ori, width, height);
    }
    else if(format == AV_PIX_FMT_YUVJ420P || format == AV_PIX_FMT_YUV420P) {
        ConvertYuv420pToRGB24(src0, src1, src2, dst_ori, width, height);
    }
    else {
        printf("not support yuv format : %d\n", format);
    }
}

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

static int InitWithFFmpeg(FrameParam* frame, FrameParam* rgb, FFmpegParam* ffmpeg, int id) {
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
    rgb->width = ffmpeg->decFrame->width;
    rgb->height = ffmpeg->decFrame->height;
    rgb->size = rgb->width*rgb->height*3;
    rgb->buf = (char *)malloc(rgb->size);
    return 0;
    
}

static int FFmpegDecoding(FrameParam* frame, FrameParam* rgb, FFmpegParam* ffmpeg, int id, bool enable) {
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
                    ConvertYUV2RGB(ffmpeg->decFrame->data[0], ffmpeg->decFrame->data[1], 
                            ffmpeg->decFrame->data[2], (unsigned char *)rgb->buf, 
                            rgb->width, rgb->height, ffmpeg->decFrame->format);
                    if(++num > 1) {
                        AppWarn("id:%d, recv frame %d>1", id, num);
                    }
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

static int FFmpegDecode(FrameParam* frame, FrameParam* rgb, FFmpegParam* ffmpeg, int id, bool enable) {
    if(ffmpeg->dec_init) {
        return FFmpegDecoding(frame, rgb, ffmpeg, id, enable);
    }
    else if((frame->buf[4]&0x1f) != 1 && !InitWithFFmpeg(frame, rgb, ffmpeg, id)) {
        ffmpeg->dec_init = 1;
        AppDebug("find IDR ok, id:%d, %dx%d", id, rgb->width, rgb->height);
        return FFmpegDecoding(frame, rgb, ffmpeg, id, enable);
    }
    return -1;
}

extern "C" int DecodeInit(ElementData* data, char* params) {
    strncpy(data->input_name[0], "decode_input", sizeof(data->input_name[0]));
    data->queue_len = GetIntValFromFile(CONFIG_FILE, "video", "rgb_queue_len");
    if(data->queue_len < 0) {
        data->queue_len = 10;
    }
    if(g_init) {
        return 0;
    }
    FFmpegInit();
    InitLookupTable();
    InitConvertTable();
    g_init = 1;
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
    dec_params->skip = GetIntValFromFile(CONFIG_FILE, "video", "rgb_skip");
    if(dec_params->skip < 0) {
        dec_params->skip = 1;
    }
    AppDebug("rgb skip : %d", dec_params->skip);
    return dec_params;
}

extern "C" int DecodeProcess(IHandle handle, TensorData* data) {
    DecodeParams* dec_params = (DecodeParams* )handle;
    FrameParam* rgb = &dec_params->rgb;

    FrameParam frame = {0};
    auto pkt = data->tensor_buf.input[0];
    frame.buf = pkt->_data;
    frame.size = (int)pkt->_size;
    frame.frame_id = pkt->_params.frame_id;
    bool enable = pkt->_params.frame_id % dec_params->skip == 0;
    if(FFmpegDecode(&frame, rgb, dec_params->ffmpeg, dec_params->id, enable) > 0) {
        HeadParams params = {0};
        params.frame_id = pkt->_params.frame_id;
        params.width = rgb->width;
        params.height = rgb->height;
        auto _packet = new Packet(rgb->buf, rgb->size, &params);
        data->tensor_buf.output = _packet;
    }
    return 0;
}

extern "C" int DecodeStop(IHandle handle) {
    DecodeParams* dec_params = (DecodeParams* )handle;
    if(dec_params == NULL) {
        AppWarn("dec is null");
        return -1;
    }
    FrameParam* rgb = &dec_params->rgb;
    if(rgb->buf != NULL) {
        free(rgb->buf);
    }
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

