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

#ifndef __AISTREAM_TENSOR_H__
#define __AISTREAM_TENSOR_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_DIMS    8

typedef void* IHandle;

typedef enum {
    IMG_UNKNOWN = 0,
    IMAGE_FORMAT_RGB,
    IMAGE_FORMAT_BGR,
    IMAGE_FORMAT_GRAY,
} ImgFormat;

typedef enum {
    C_UNKNOWN = 0,
    C_NCHW,
    C_NHWC,
} LayoutFormat;

typedef enum {
    DUNKNOWN = 0,
    IINT8,
    IINT16,
    IINT32,
    IINT64,
    FFP16,
    FFP32,
    FFP64,
    BBOOL,
} IDataType;

typedef struct {
    int n_dims;
    int d[MAX_DIMS];
} IDims;

typedef struct {
    char name[256];
    IDataType data_type;
    ImgFormat format;
    LayoutFormat layout;
    IDims dims;
} TTensor;

typedef struct {
    char *buf;
    int size;
    int width;
    int height;
    TTensor tensor;
} TBuffer;

typedef struct {
    int input_num;
    TBuffer *input;
    int output_num;
    TBuffer *output;
} TensorData;

typedef struct {
    char name[64];
    const char* init;
    const char* start;
    const char* process;
    const char* stop;
    const char* release;
} DLRegister;

#endif

