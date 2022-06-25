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
#include <vector>
#include <queue>
#include <condition_variable>

#define MAX_DIMS    4

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
    //char name[256];
    IDataType data_type;
    ImgFormat format;
    LayoutFormat layout;
    IDims dims;
} TTensor;

typedef struct {
    int type;
    int width;
    int height;
    int frame_id;
    char* ptr;
    size_t ptr_size;
    //TTensor tensor;
} HeadParams;

typedef struct {
    char* buf;
    int size;
    int type;
    int width;
    int height;
    int frame_id;
} FrameParam;

typedef struct {
    float left;
    float top;
    float width;
    float height;
    float score;
    int classid;
} DetectionResult;

class Packet {
public:
    Packet(void* buf, size_t size, HeadParams* params = nullptr) {
        _size = size;
        if(size > 0) {
            _data = new char[size];
            memcpy(_data, buf, size);
        }
        else {
            _data = nullptr;
        }
        if(params != nullptr) {
            _params = *params;
        }
    }
    ~Packet() {
        if(_data != nullptr) {
            delete _data;
            _data = nullptr;
        }
        if(_params.ptr != nullptr) {
            delete _params.ptr;
            _params.ptr = nullptr;
        }
    }

    char* _data;
    size_t _size;
    HeadParams _params;
};

typedef struct {
    size_t input_num;
    Packet* input[MAX_DIMS];
    Packet* output;
} TensorBuffer;

class PacketQueue {
public:
    char name[256];
    std::mutex mtx;
    std::condition_variable condition;
    std::queue<std::shared_ptr<Packet>> _queue;
    int *running;
};

class ElementData {
public:
    ElementData(void) {
        queue_len = 100;
        sleep_usec = 0;
        memset(input_name, 0, sizeof(input_name));
    }
    ~ElementData(void) {}
    // input: allocated by itself
    // every element can have many inputs, connected to other elements
    char input_name[MAX_DIMS][256];
    std::vector<std::shared_ptr<PacketQueue>> input;
    // output: allocated by next users, mutex lock?
    // every element have only one output, but the same data can be send to many other elements
    std::vector<std::shared_ptr<PacketQueue>> output;
    int queue_len;
    int sleep_usec;
};

class TensorData {
public:
    TensorData(void) {
        _out = nullptr;
        memset(&tensor_buf, 0, sizeof(tensor_buf));
    }
    ~TensorData(void) {}
    // element support multi intput, but single output
    std::vector<std::shared_ptr<Packet>> _in;
    std::shared_ptr<Packet> _out;
    TensorBuffer tensor_buf;
};

typedef struct {
    char name[64];
    const char* init;
    const char* start;
    const char* process;
    const char* stop;
    const char* notify;
    const char* release;
} DLRegister;

#endif

