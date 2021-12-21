/******************************************************************************
 * Copyright (C) 2021 aistream <aistream@yeah.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

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
    std::unique_ptr<char[]> params;
    std::vector<std::shared_ptr<TTensor>> tensor;
} KeyValue;

class Element {
public:
    Element(void);
    ~Element(void);
    void SetName(const char* _name) {strncpy(name, _name, sizeof(name));}
    void SetPath(const char* _path) {strncpy(path, _path, sizeof(path));}
    void Put2InputMap(auto _map) {input_map.push_back(_map);}
    void Put2OutputMap(auto _map) {output_map.push_back(_map);}
    void SetAsync(bool val) {async = val;}
private:
    char name[256];
    char path[256];
    bool async;
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
    int running;
    std::mutex alg_mtx;
    std::vector<std::shared_ptr<AlgTask>> alg_vec;
    void AlgThread(void);
    void UpdateTask(const char *filename);
};

#endif

