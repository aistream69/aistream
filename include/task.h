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

#ifndef __AISTREAM_TASK_H__
#define __AISTREAM_TASK_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include "pipeline.h"

class Object;
class TaskParams {
public:
    TaskParams(std::shared_ptr<Object> _obj);
    ~TaskParams(void);
    void SetTaskName(char *_name) {strncpy(name, _name, sizeof(name));}
    char *GetTaskName(void) {return name;}
    void SetParams(char *str);
    int Start(void);
    int Stop(bool sync = false);
    bool KeepAlive(void);
    std::weak_ptr<Object> obj;
    int running;
private:
    char name[256];
    long int task_beat;
    std::unique_ptr<char[]> params;
    std::shared_ptr<AlgTask> alg;
};

#endif

