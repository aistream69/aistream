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

#include "stream.h"
#include "obj.h"
#include <thread>

Object::Object(MediaServer *_media)
  : media(_media) {
}

Object::~Object(void) {
}

ObjParams::ObjParams(MediaServer* _media)
  : media(_media) {
}

ObjParams::~ObjParams(void) {
}

bool ObjParams::Put2ObjQue(std::shared_ptr<Object> obj) {
    obj_mtx.lock();
    obj_vec.push_back(obj);
    obj_mtx.unlock();
    return true;
}

std::shared_ptr<Object> ObjParams::GetObj(int id) {
    std::shared_ptr<Object> obj = nullptr;
    obj_mtx.lock();
    for(auto itr = obj_vec.begin(); itr != obj_vec.end(); ++itr) {
        if((*itr)->GetId() == id) {
            obj = *itr;
            break;
        }
    }
    obj_mtx.unlock();
    return obj;
}

bool ObjParams::DelFromObjQue(int id) {
    obj_mtx.lock();
    for(auto itr = obj_vec.begin(); itr != obj_vec.end(); ++itr) {
        if((*itr)->GetId() == id) {
            obj_vec.erase(itr);
            break;
        }
    }
    obj_mtx.unlock();
    return true;
}

void ObjParams::ObjManager(void) {
    while(media->running) {
        obj_mtx.lock();
        for(auto itr = obj_vec.begin(); itr != obj_vec.end(); ++itr) {
            (*itr)->TraverseTaskQue();
        }
        obj_mtx.unlock();
        sleep(2);
    }
    AppDebug("run ok");
}

void ObjParams::Start(void) {
    std::thread t(&ObjParams::ObjManager, this);
    t.detach();
}

bool Object::Put2TaskQue(std::shared_ptr<TaskParams> task) {
    task_mtx.lock();
    task_vec.push_back(task);
    task_mtx.unlock();
    return true;
}

std::shared_ptr<TaskParams> Object::GetTask(char *name) {
    std::shared_ptr<TaskParams> task = nullptr;
    task_mtx.lock();
    for(auto itr = task_vec.begin(); itr != task_vec.end(); ++itr) {
        if(!strcmp((*itr)->GetTaskName(), name)) {
            task = *itr;
            break;
        }
    }
    task_mtx.unlock();
    return task;
}

bool Object::DelFromTaskQue(char *name) {
    task_mtx.lock();
    for(auto itr = task_vec.begin(); itr != task_vec.end(); ++itr) {
        if(!strcmp((*itr)->GetTaskName(), name)) {
            (*itr)->Stop();
            task_vec.erase(itr);
            break;
        }
    }
    task_mtx.unlock();
    return true;
}

void Object::TraverseTaskQue(void) {
    task_mtx.lock();
    for(auto itr = task_vec.begin(); itr != task_vec.end(); ++itr) {
        auto task = (*itr);
        if(!task->KeepAlive()) {
            task->Stop(true);
            task->Start();
        }
    }
    task_mtx.unlock();
}

