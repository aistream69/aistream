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

#ifndef __AISTREAM_PIPELINE_H__
#define __AISTREAM_PIPELINE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <vector>
#include <mutex>
#include "tensor.h"

class MediaServer;

typedef struct {
    char key[256];
    char val[256];
    std::vector<std::shared_ptr<TTensor>> tensor;
} KeyValue;

class Element {
public:
    Element(void);
    ~Element(void);
    void SetName(const char* _name) {strncpy(name, _name, sizeof(name));}
    void SetPath(const char* _path) {strncpy(path, _path, sizeof(path));}
    void SetFramework(const char* _framework) {strncpy(framework, _framework, sizeof(framework));}
    void Put2InputMap(auto _map) {input_map.push_back(_map);}
    void Put2OutputMap(auto _map) {output_map.push_back(_map);}
    void SetAsync(bool val) {async = val;}
    void SetParams(char *str);
private:
    char name[256];
    char path[256];
    char framework[256];
    bool async;
    std::unique_ptr<char[]> params;
    std::vector<std::shared_ptr<KeyValue>> input_map;
    std::vector<std::shared_ptr<KeyValue>> output_map;
};

class AlgTask {
public:
    AlgTask(MediaServer* _media);
    ~AlgTask(void);
    MediaServer* media;
    void SetName(const char* _name) {strncpy(name, _name, sizeof(name));}
    char* GetName(void) {return name;}
    void SetConfig(const char* cfg) {strncpy(config, cfg, sizeof(config));}
    void SetBatchSize(int val) {batch_size = val;}
    bool Put2ElementQue(auto ele);
private:
    char name[128];
    char config[256];
    int batch_size;
    std::mutex ele_mtx;
    std::vector<std::shared_ptr<Element>> ele_vec;
};

class Pipeline {
public:
    Pipeline(MediaServer* _media);
    ~Pipeline(void);
    MediaServer* media;
    void start(void);
    bool Put2AlgQue(auto alg);
    auto GetAlgTask(const char* name);
    void CheckIfDelAlg(auto config_map);
    size_t GetAlgNum(void);
private:
    std::mutex alg_mtx;
    std::vector<std::shared_ptr<AlgTask>> alg_vec;
};

#endif

