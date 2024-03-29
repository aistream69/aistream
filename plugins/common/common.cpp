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
  crv = 104597;
  cbu = 132201; /* fra matrise i global.h */
  cgu = 25675;
  cgv = 53279;
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
  for (i=0; i<256; i++)
    clp[ind++]=i;
  ind=640;
  for (i=0; i<384; i++)
    clp[ind++]=255;
}

void ConvertRGBToYUV420p(int w, 
                         int h, 
                         unsigned char* rgb,
                         unsigned char* yuv, 
                         unsigned char* work_space) {
  int i,j;
  unsigned char *u,*v,*y,*uu,*vv;
  unsigned char *pu1,*pu2,*pu3,*pu4;
  unsigned char *pv1,*pv2,*pv3,*pv4;
  unsigned char *r,*g,*b;

  uu = work_space;
  vv = work_space + w*h;
  y=yuv;
  u=uu;
  v=vv;
  // Get r,g,b pointers from image data....
  r=rgb;
  g=rgb+1;
  b=rgb+2;
  //Get YUV values for rgb values...
  for (i=0; i<h; i++) {
    for (j=0; j<w; j++) {
      *y++=( RGB2YUV_YR[*r] +RGB2YUV_YG[*g]+RGB2YUV_YB[*b]+1048576)>>16;
      *u++=(-RGB2YUV_UR[*r] -RGB2YUV_UG[*g]+RGB2YUV_UBVR[*b]+8388608)>>16;
      *v++=( RGB2YUV_UBVR[*r]-RGB2YUV_VG[*g]-RGB2YUV_VB[*b]+8388608)>>16;
      r+=3;
      g+=3;
      b+=3;
    }
  }
  // Now sample the U & V to obtain YUV 4:2:0 format
  u=yuv+w*h;
  v=u+(w*h)/4;
  // For U
  pu1=uu;
  pu2=pu1+1;
  pu3=pu1+w;
  pu4=pu3+1;
  // For V
  pv1=vv;
  pv2=pv1+1;
  pv3=pv1+w;
  pv4=pv3+1;
  // Do sampling....
  for (i=0; i<h; i+=2) {
    for (j=0; j<w; j+=2) {
      *u++=(*pu1+*pu2+*pu3+*pu4)>>2;
      *v++=(*pv1+*pv2+*pv3+*pv4)>>2;
      pu1+=2;
      pu2+=2;
      pu3+=2;
      pu4+=2;
      pv1+=2;
      pv2+=2;
      pv3+=2;
      pv4+=2;
    }
    pu1+=w;
    pu2+=w;
    pu3+=w;
    pu4+=w;
    pv1+=w;
    pv2+=w;
    pv3+=w;
    pv4+=w;
  }
}

static void ConvertNv12ToRGB24(unsigned char *src0, 
                               unsigned char *src1, 
                               unsigned char *dst_ori, 
                               int width,int height) {
  int y1,y2,u,v;
  unsigned char *py1,*py2;
  int i,j, c1, c2, c3, c4;
  unsigned char *d1, *d2;

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
      *d1++ = clp[384+((y1 + c1)>>16)];
      *d1++ = clp[384+((y1 - c2 - c3)>>16)];
      *d1++ = clp[384+((y1 + c4)>>16)];
      //down-left
      y2 = tab_76309[*py2++];
      *d2++ = clp[384+((y2 + c1)>>16)];
      *d2++ = clp[384+((y2 - c2 - c3)>>16)];
      *d2++ = clp[384+((y2 + c4)>>16)];
      //up-right
      y1 = tab_76309[*py1++];
      *d1++ = clp[384+((y1 + c1)>>16)];
      *d1++ = clp[384+((y1 - c2 - c3)>>16)];
      *d1++ = clp[384+((y1 + c4)>>16)];
      //down-right
      y2 = tab_76309[*py2++];
      *d2++ = clp[384+((y2 + c1)>>16)];
      *d2++ = clp[384+((y2 - c2 - c3)>>16)];
      *d2++ = clp[384+((y2 + c4)>>16)];
    }
    d1 += 3*width;
    d2 += 3*width;
    py1+= width;
    py2+= width;
  }
}

static void ConvertYuv420pToRGB24(unsigned char *src0, 
                                  unsigned char *src1, 
                                  unsigned char *src2,
                                  unsigned char *dst_ori, 
                                  int width,int height) {
  int y1,y2,u,v;
  unsigned char *py1,*py2;
  int i,j, c1, c2, c3, c4;
  unsigned char *d1, *d2;

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
      *d1++ = clp[384+((y1 + c1)>>16)];
      *d1++ = clp[384+((y1 - c2 - c3)>>16)];
      *d1++ = clp[384+((y1 + c4)>>16)];
      //down-left
      y2 = tab_76309[*py2++];
      *d2++ = clp[384+((y2 + c1)>>16)];
      *d2++ = clp[384+((y2 - c2 - c3)>>16)];
      *d2++ = clp[384+((y2 + c4)>>16)];
      //up-right
      y1 = tab_76309[*py1++];
      *d1++ = clp[384+((y1 + c1)>>16)];
      *d1++ = clp[384+((y1 - c2 - c3)>>16)];
      *d1++ = clp[384+((y1 + c4)>>16)];
      //down-right
      y2 = tab_76309[*py2++];
      *d2++ = clp[384+((y2 + c1)>>16)];
      *d2++ = clp[384+((y2 - c2 - c3)>>16)];
      *d2++ = clp[384+((y2 + c4)>>16)];
    }
    d1 += 3*width;
    d2 += 3*width;
    py1+= width;
    py2+= width;
  }
}

// Convert from YUV420 to RGB24
void ConvertYUV2RGB(unsigned char *src0,
                    unsigned char *src1,
                    unsigned char *src2,
                    unsigned char *dst_ori, 
                    int width,int height, int format) {
  if (format == AV_PIX_FMT_NV12) {
    ConvertNv12ToRGB24(src0, src1, dst_ori, width, height);
  } else if (format == AV_PIX_FMT_YUVJ420P || format == AV_PIX_FMT_YUV420P) {
    ConvertYuv420pToRGB24(src0, src1, src2, dst_ori, width, height);
  } else {
    printf("not support yuv format : %d\n", format);
  }
}

void RGBInit(void) {
  static int init = 0;
  if (__sync_add_and_fetch(&init, 1) > 1) {
    return;
  }
  InitLookupTable();
  InitConvertTable();
}

void FFmpegInit(void) {
  static int init = 0;
  if (__sync_add_and_fetch(&init, 1) > 1) {
    return;
  }
  avcodec_register_all();
  av_register_all();
  av_log_set_level(AV_LOG_FATAL);
}

