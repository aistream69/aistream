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

#ifndef __AISTREAM_COMMON_H__
#define __AISTREAM_COMMON_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void FFmpegInit(void);
void RGBInit(void);
void ConvertYUV2RGB(unsigned char *src0,
                    unsigned char *src1,
                    unsigned char *src2,
                    unsigned char *dst_ori, 
                    int width,int height, int format);
void ConvertRGBToYUV420p(int w, 
                         int h, 
                         unsigned char* rgb,
                         unsigned char* yuv, 
                         unsigned char* work_space);

#endif

