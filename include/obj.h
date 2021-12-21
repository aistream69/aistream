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

