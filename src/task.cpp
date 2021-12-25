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

#include "task.h"
#include "stream.h"

TaskParams::TaskParams(std::shared_ptr<Object> _obj)
  : obj(_obj) {
    running = 0;
    task_beat = 0;
    params = nullptr;
}

TaskParams::~TaskParams(void) {
}

void TaskParams::SetParams(char *str) {
    params = std::make_unique<char[]>(strlen(str)+1);
    strcpy(params.get(), str);
}

void TaskParams::Start(void) {
    if(obj.expired()) {
        AppWarn("task:%s, obj is expired", name);
        return;
    }
    auto _obj = obj.lock();
    if(params != nullptr) {
        AppDebug("id:%d, task %s params:%s", _obj->GetId(), name, params.get());
    }
    running = 1;
}

void TaskParams::Stop(bool sync) {
    if(!running) {
        return;
    }
    running = 0;
    if(!sync) {
        return;
    }
}

bool TaskParams::KeepAlive(void) {
    if(obj.expired()) {
        AppWarn("task:%s, obj is expired", name);
        return false;
    }
    auto _obj = obj.lock();
    MediaServer* media = _obj->media;
    if(task_beat == 0) {
        task_beat = media->now_sec;
        return false;
    }
    else if(media->now_sec - task_beat > 20) { //TODO: support timeout in config.json
        AppWarn("id:%d,task:%s,detected exception, restart it ...", _obj->GetId(), name);
        task_beat = media->now_sec;
        return false;
    }
    return true;
}

