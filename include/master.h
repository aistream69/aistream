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

#ifndef __AISTREAM_MASTER_H__
#define __AISTREAM_MASTER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <vector>
#include <mutex>
#include <map>

#define SLAVE_LOAD_MAX  99

typedef struct {
    int obj_num;
    int cpu_load; // include cpu mem detection
    int gpu_load; // include gpu mem detection
    float total_load;
} SlaveLoad;

typedef struct {
    char name[64];
    char input[32];
    int port;
} TaskCfg;

class SlaveParam : public std::enable_shared_from_this<SlaveParam> {
public:
    SlaveParam(void) {
        alive = false;
        offline_cnt = 0;
        params = nullptr;
        internet_ip[0] = '\0';
        memset(&load, 0, sizeof(load));
    }
    ~SlaveParam(void) {
    }
    char ip[128];
    char internet_ip[128];
    int rest_port;
    SlaveLoad load;
    bool alive;
    int offline_cnt;
    std::vector<int> obj_id_vec; // if async, use mutex
    std::unique_ptr<char[]> params;
    std::vector<TaskCfg> httpf_task;
};

class MObjParam : public std::enable_shared_from_this<MObjParam> {
public:
    MObjParam(void) {
        status = 0;
        params = nullptr;
        slave = nullptr;
        memset(name, 0, sizeof(name));
    }
    ~MObjParam(void) {
    }
    void AddTask(char* params);
    bool DelTask(char *name);
    std::unique_ptr<char[]> GetTask(void);
    int id;
    int status;
    char name[256];
    std::mutex m_task_mtx;
    std::vector<std::string> m_task_vec;
    std::unique_ptr<char[]> params;
    std::shared_ptr<SlaveParam> slave;
};

class MediaServer;
class MasterParams {
public:
    MasterParams(MediaServer* _media);
    ~MasterParams(void);
    void Start(void);
    void SlaveThread(void);
    void ObjThread(void);
    std::mutex m_slave_mtx;
    std::vector<std::shared_ptr<SlaveParam>> m_slave_vec;
    std::mutex m_obj_mtx;
    std::vector<std::shared_ptr<MObjParam>> m_obj_vec;
    std::map<int, std::string> id_name;
    std::unique_ptr<char[]> output;
    std::vector<TaskCfg> cfg_task_vec;
    char user[256];
    char password[256];
    MediaServer* media;
};

#endif

