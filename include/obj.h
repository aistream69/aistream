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

class ObjManager {
public:
    ObjManager(MediaServer* _media);
    ~ObjManager(void);
    void start(void);
    MediaServer* media;
private:
};

class Object {
public:
    Object(MediaServer* _media);
    ~Object(void);
    MediaServer* media;
    void SetId(int _id) {id = _id;}
    int GetId(void) {return id;}
    bool Put2TaskQue(std::shared_ptr<TaskParams> task);
    std::shared_ptr<TaskParams> GetTask(char *name);
    bool DelFromTaskQue(char *name);
private:
    int id;
    char *user_data;
    std::mutex task_mtx;
    std::vector<std::shared_ptr<TaskParams>> task_vec;
};

class ObjParams {
public:
    ObjParams(MediaServer* _media);
    ~ObjParams(void);
    MediaServer* media;
    bool Put2ObjQue(std::shared_ptr<Object> obj);
    std::shared_ptr<Object> GetObj(int id);
    bool DelFromObjQue(int id);
private:
    std::mutex obj_mtx;
    std::vector<std::shared_ptr<Object>> obj_vec;
};

#endif

