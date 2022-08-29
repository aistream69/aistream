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
#include <ft2build.h>
#include <freetype/freetype.h>
#include FT_FREETYPE_H
#include "tensor.h"
#include "share.h"
#include "common.h"
#include "log.h"

typedef void (*OSDFunc)(Packet* output);

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
    OSDFunc set_rect_text;
} OSDParams;

typedef struct {
    uint8_t* buf;
    int w;
    int h;
    int horiBearingX;
    int horiBearingY;
} FontBmp;

#define CHAR_NUM    26
typedef struct {
    int init;
    int bitrate;
    int gop;
    uint8_t rect_yuv[3];
    uint8_t text_yuv[3];
    int pix_w;
    int pix_h;
    FontBmp bmp[CHAR_NUM]; // a-z
} OSDConfig;

static OSDConfig config = {0};
static void Nv12pSetRect(int x, int y, int w, int h, Packet* pkt) {
    int img_w = pkt->_params.width;
    int img_h = pkt->_params.height;
    uint8_t* _y = (uint8_t* )pkt->_data;
    uint8_t* _uv = (uint8_t* )pkt->_data + img_w*img_h;
    uint8_t color_y = config.rect_yuv[0], color_u = config.rect_yuv[1], color_v = config.rect_yuv[2];
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

static void Nv12SetText(char* name, int x, int y, Packet* pkt) {
    int img_w = pkt->_params.width;
    int img_h = pkt->_params.height;
    uint8_t* _y = (uint8_t* )pkt->_data;
    uint16_t* _uv = (uint16_t* )(_y + img_w*img_h);
    int img_w_d_2 = img_w/2;
    uint8_t color_y = config.text_yuv[0], color_u = config.text_yuv[1], color_v = config.text_yuv[2];
    int ft_x = x;
    for(int i = 0; name[i] != '\0'; i ++) {
        int j = name[i] - 'a';
        if(j < 0 || j >= CHAR_NUM) {
            printf("warning, not support font bmp : %c\n", name[i]);
            continue;
        }
        FontBmp& bmp = config.bmp[j];
        int ft_y = y - bmp.horiBearingY - 6;
        if(ft_y < 0) ft_y = 0;
        int ft_y_d_2 = ft_y/2;
        int ft_x_d_2 = ft_x/2;
        for(int _h = 0, k = 0; _h < bmp.h; _h ++) {
            int start_x_y = img_w * (ft_y + _h) + ft_x;
            int start_x_uv = img_w_d_2 * (ft_y_d_2 + _h/2) + ft_x_d_2;
            for(int _w = 0; _w < bmp.w; _w ++) {
                if(bmp.buf[k ++]) {
                    _y[start_x_y + _w] = color_y;
                    _uv[start_x_uv + _w/2] = color_u + (color_v<<8);
                }
            }
        }
        ft_x += bmp.w/2*2 + 2;
    }
}

static void Nv12SetRectText(Packet* pkt) {
    DetectionResult* dets = (DetectionResult* )pkt->_params.ptr;
    int num = pkt->_params.ptr_size/sizeof(DetectionResult);
    for(int n = 0; n < num; n ++) {
        DetectionResult* det = dets + n;
        int x = ((int)det->left)/2*2;
        int y = ((int)det->top)/2*2;
        int w = ((int)det->width)/2*2;
        int h = ((int)det->height)/2*2;
        // set rect
        Nv12pSetRect(x, y, w, h, pkt);
        // put text
        if(det->name[0]) {
            Nv12SetText(det->name, x, y, pkt);
        }
    }
}

static void Yuv420pSetRect(int x, int y, int w, int h, Packet* pkt) {
    int img_w = pkt->_params.width;
    int img_h = pkt->_params.height;
    uint8_t* _y = (uint8_t* )pkt->_data;
    uint8_t* _u = _y + img_w*img_h;
    uint8_t* _v = _u + img_w*img_h/4;
    uint8_t color_y = config.rect_yuv[0], color_u = config.rect_yuv[1], color_v = config.rect_yuv[2];
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
        int start_x = (img_w*i + x)/2;
        int end_x = start_x + w/2;
        // x axis line
        if(i == yy || i == hh - 1) {
            for(int j = 0; j < w/2; j++) {
                _u[start_x + j] = color_u;
                _v[start_x + j] = color_v;
            }
        }
        // y axis line
        _u[start_x] = color_u;
        _u[end_x] = color_u;
        _v[start_x] = color_v;
        _v[end_x] = color_v;
    }
}

static void Yuv420pSetText(char* name, int x, int y, Packet* pkt) {
    int img_w = pkt->_params.width;
    int img_h = pkt->_params.height;
    uint8_t* _y = (uint8_t* )pkt->_data;
    uint8_t* _u = _y + img_w*img_h;
    uint8_t* _v = _u + img_w*img_h/4;
    int img_w_d_2 = img_w/2;
    uint8_t color_y = config.text_yuv[0], color_u = config.text_yuv[1], color_v = config.text_yuv[2];
    int ft_x = x;
    for(int i = 0; name[i] != '\0'; i ++) {
        int j = name[i] - 'a';
        if(j < 0 || j >= CHAR_NUM) {
            printf("warning, not support font bmp : %c\n", name[i]);
            continue;
        }
        FontBmp& bmp = config.bmp[j];
        //printf("%c,%dx%d\n", name[i], bmp.w, bmp.h);
        int ft_y = y - bmp.horiBearingY - 6;
        if(ft_y < 0) ft_y = 0;
        int ft_y_d_2 = ft_y/2;
        int ft_x_d_2 = ft_x/2;
        for(int _h = 0, k = 0; _h < bmp.h; _h ++) {
            int start_x_y = img_w * (ft_y + _h) + ft_x;
            int start_x_uv = img_w_d_2 * (ft_y_d_2 + _h/2) + ft_x_d_2;
            for(int _w = 0; _w < bmp.w; _w ++) {
                int _w_d_2 = _w/2;
                if(bmp.buf[k ++]) {
                    _y[start_x_y + _w] = color_y;
                    _u[start_x_uv + _w_d_2] = color_u;
                    _v[start_x_uv + _w_d_2] = color_v;
                }
            }
        }
        ft_x += bmp.w/2*2 + 2;
        //for(int m=0, k=0; m<bmp.h; m++) {
        //    for(int n=0; n<bmp.w; n++) {
        //        printf((bmp.buf[k++] == 0 ) ? " " : "1");
        //        //printf((bmp.buf[m * bmp.w  + n] == 0 ) ? " " : "1");
        //        //printf("%02x", bmp.buf[m * bmp.w  + n]);
        //    }
        //    printf("\n");
        //}
    }
    //WriteFile("test.yuv", pkt->_data, pkt->_size, "wb");
    //AppDebug("write yuv ok, %dx%d", pkt->_params.width, pkt->_params.height);
    //exit(0);
}

static void Yuv420pSetRectText(Packet* pkt) {
    DetectionResult* dets = (DetectionResult* )pkt->_params.ptr;
    int num = pkt->_params.ptr_size/sizeof(DetectionResult);
    for(int n = 0; n < num; n ++) {
        DetectionResult* det = dets + n;
        int x = ((int)det->left)/2*2;
        int y = ((int)det->top)/2*2;
        int w = ((int)det->width)/2*2;
        int h = ((int)det->height)/2*2;
        // set rect
        Yuv420pSetRect(x, y, w, h, pkt);
        // put text
        if(det->name[0]) {
            Yuv420pSetText(det->name, x, y, pkt);
        }
    }
}

static int YuvColorInit(char* params) {
    int size = 0;
    uint8_t rgb[3];

    // init rect yuv color
    auto color = GetArrayBufFromJson(params, size, "color");
    if(color == nullptr || size != 3) {
        AppWarn("get rect color failed, size:%d, %s", size, params);
        return -1;
    }
    for(int i = 0; i < size; i ++) {
        auto arrbuf = GetBufFromArray(color.get(), i);
        if(arrbuf == nullptr) {
            AppWarn("get rect color %d failed", i);
            break;
        }
        rgb[i] = atoi(arrbuf.get());
    }
    config.rect_yuv[0] = 0.299*rgb[0] + 0.587*rgb[1] + 0.114*rgb[2];            // y
    config.rect_yuv[1] = - 0.1687*rgb[0] - 0.3313*rgb[1] + 0.5*rgb[2] + 128;    // u
    config.rect_yuv[2] = 0.5*rgb[0] - 0.4187*rgb[1] - 0.0813*rgb[2] + 128;      // v
    // init text yuv color
    size = 0;
    color = GetArrayBufFromJson(params, size, "font", "color");
    if(color == nullptr || size != 3) {
        AppWarn("get text color failed, size:%d, %s", size, params);
        return -1;
    }
    for(int i = 0; i < size; i ++) {
        auto arrbuf = GetBufFromArray(color.get(), i);
        if(arrbuf == nullptr) {
            AppWarn("get text color %d failed", i);
            break;
        }
        rgb[i] = atoi(arrbuf.get());
    }
    config.text_yuv[0] = 0.299*rgb[0] + 0.587*rgb[1] + 0.114*rgb[2];            // y
    config.text_yuv[1] = - 0.1687*rgb[0] - 0.3313*rgb[1] + 0.5*rgb[2] + 128;    // u
    config.text_yuv[2] = 0.5*rgb[0] - 0.4187*rgb[1] - 0.0813*rgb[2] + 128;      // v

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
    if(pkt->_params.type == AV_PIX_FMT_YUVJ420P) {
        encoder.ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    }
    else {
        encoder.ctx->pix_fmt = (AVPixelFormat)pkt->_params.type;
    }
    //encoder.ctx->pix_fmt = AV_PIX_FMT_NV12;
    if(encoder.ctx->pix_fmt == AV_PIX_FMT_YUV420P) {
        osd->set_rect_text = Yuv420pSetRectText;
    }
    else if(encoder.ctx->pix_fmt == AV_PIX_FMT_NV12) {
        osd->set_rect_text = Nv12SetRectText;
    }
    else {
        AppWarn("unsupport pix format : %d", encoder.ctx->pix_fmt);
        osd->set_rect_text = Nv12SetRectText;
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

static int FontInit(char* params) {
    FT_Library  m_pFTLib;
    FT_Face     m_pFTFace;

    // init
    auto ttf_file = GetStrValFromJson(params, "font", "ttf");
    int pix_w = GetIntValFromJson(params, "font", "width");
    int pix_h = GetIntValFromJson(params, "font", "height");
    if(ttf_file == nullptr || pix_w < 0 || pix_h < 0) {
        AppError("get font params failed");
        return -1;
    }
    int ret = FT_Init_FreeType(&m_pFTLib);
    if(ret != 0) {
        AppError("ft init failed, ret:%d", ret);
        return -1;
    }
    ret = FT_New_Face(m_pFTLib, ttf_file.get(), 0, &m_pFTFace);
    if(ret != 0) {
        AppError("ft new face failed, %s, ret:%d", ttf_file.get(), ret);
        return -1;
    }
    ret = FT_Select_Charmap(m_pFTFace, FT_ENCODING_UNICODE);
    if(ret != 0) {
        AppError("ft select charmap failed, ret:%d", ret);
        return -1;
    }
    ret = FT_Set_Pixel_Sizes(m_pFTFace, pix_w, pix_h);
    if(ret != 0) {
        AppError("ft set pixel size %dx%d failed, ret:%d", pix_w, pix_h, ret);
        return -1;
    }
    // generate bitmap for a-z
    for(char c = 'a'; c <= 'z'; c ++) {
        int i = c - 'a';
        FontBmp& bmp = config.bmp[i];
        ret = FT_Load_Glyph(m_pFTFace, FT_Get_Char_Index(m_pFTFace, c), FT_LOAD_DEFAULT);
        if(ret != 0) {
            AppError("ft load glyph failed, %dx%d, ret:%d", pix_w, pix_h, ret);
            break;
        }
        ret = FT_Render_Glyph(m_pFTFace->glyph, FT_RENDER_MODE_NORMAL);  
        if(ret != 0) {
            AppError("ft render glyph failed, %dx%d, ret:%d", pix_w, pix_h, ret);
            break;
        }
        FT_Bitmap _bmp = m_pFTFace->glyph->bitmap;
        bmp.w = _bmp.width;
        bmp.h = _bmp.rows;
        bmp.horiBearingX = m_pFTFace->glyph->metrics.horiBearingX/64;
        bmp.horiBearingY = m_pFTFace->glyph->metrics.horiBearingY/64;
        bmp.buf = (uint8_t* )malloc(bmp.w*bmp.h);
        memcpy(bmp.buf, _bmp.buffer, bmp.w*bmp.h);
    }
    // free
    FT_Done_Face(m_pFTFace);
    FT_Done_FreeType(m_pFTLib);
    config.pix_w = pix_w;
    config.pix_h = pix_h;

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
    FontInit(params);
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
    Packet* pkt = data->tensor_buf.input[0];
    if(!osd->encoder.init) {
        CreateEncoder(pkt, osd);
        osd->encoder.init = 1;
    }
    osd->set_rect_text(pkt);
    Encoding(pkt, osd, data);
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
    for(int i = 0; i < CHAR_NUM; i ++) {
        FontBmp& bmp = config.bmp[i];
        if(bmp.buf != NULL) {
            free(bmp.buf);
            bmp.buf = NULL;
        }
    }
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

