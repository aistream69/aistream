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

#include "obj.h"
#include "stream.h"

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
    std::vector<std::shared_ptr<Object>>::iterator itr;
    obj_mtx.lock();
    for(itr = obj_vec.begin(); itr != obj_vec.end(); ++itr) {
        if((*itr)->GetId() == id) {
            obj = *itr;
            break;
        }
    }
    obj_mtx.unlock();
    return obj;
}

bool ObjParams::DelFromObjQue(int id) {
    std::vector<std::shared_ptr<Object>>::iterator itr;
    obj_mtx.lock();
    for(itr = obj_vec.begin(); itr != obj_vec.end(); ++itr) {
        if((*itr)->GetId() == id) {
            obj_vec.erase(itr);
            itr--;
        }
    }
    obj_mtx.unlock();
    return true;
}

bool Object::Put2TaskQue(std::shared_ptr<TaskParams> task) {
    task_mtx.lock();
    task_vec.push_back(task);
    task_mtx.unlock();
    return true;
}

std::shared_ptr<TaskParams> Object::GetTask(char *name) {
    std::shared_ptr<TaskParams> task = nullptr;
    std::vector<std::shared_ptr<TaskParams>>::iterator itr;
    task_mtx.lock();
    for(itr = task_vec.begin(); itr != task_vec.end(); ++itr) {
        if(!strcmp((*itr)->GetTaskName(), name)) {
            task = *itr;
            break;
        }
    }
    task_mtx.unlock();
    return task;
}

bool Object::DelFromTaskQue(char *name) {
    std::vector<std::shared_ptr<TaskParams>>::iterator itr;
    task_mtx.lock();
    for(itr = task_vec.begin(); itr != task_vec.end(); ++itr) {
        if(!strcmp((*itr)->GetTaskName(), name)) {
            task_vec.erase(itr);
            itr--;
        }
    }
    task_mtx.unlock();
    return true;
}

