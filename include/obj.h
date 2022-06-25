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

#ifndef __AISTREAM_OBJ_H__
#define __AISTREAM_OBJ_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <vector>
#include <mutex>
#include "task.h"

class MediaServer;

class Object : public std::enable_shared_from_this<Object> {
public:
    Object(MediaServer* _media);
    virtual ~Object(void);
    void SetId(int _id) {id = _id;}
    int GetId(void) {return id;}
    virtual char* GetPath(char* path) {return path;}
    bool Put2TaskQue(std::shared_ptr<TaskParams> task);
    std::shared_ptr<TaskParams> GetTask(char *name);
    bool DelFromTaskQue(char *name);
    void TraverseTaskQue(void);
    void SetParams(char *str);
    auto GetParams(void) {return params;}
    MediaServer* media;
private:
    int id;
    std::mutex task_mtx;
    std::vector<std::shared_ptr<TaskParams>> task_vec;
    std::shared_ptr<char> params;
    void CheckWorkDir(void);
};

class ObjParams {
public:
    ObjParams(MediaServer* _media);
    ~ObjParams(void);
    bool Put2ObjQue(std::shared_ptr<Object> obj);
    void TraverseObjQue(void* arg, int (*cb)(std::shared_ptr<Object> obj, void* arg));
    std::shared_ptr<Object> GetObj(int id);
    bool DelFromObjQue(int id);
    void Start(void);
    int GetObjNum(void);
    MediaServer* media;
private:
    void ObjManager(void);
    std::mutex obj_mtx;
    std::vector<std::shared_ptr<Object>> obj_vec;
};

#endif

