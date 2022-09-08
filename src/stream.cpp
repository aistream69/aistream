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

MediaServer::MediaServer(void) {
    running = 1;
    system_init = 0;
    config_file = "cfg/config.json";
    config = new ConfigParams(this);
    obj_params = new ObjParams(this);
    UpdateTime();
}

MediaServer::~MediaServer(void) {
}

void MediaServer::Run(const char* cfg) {
    bool ret;
    if(cfg != NULL) {
        config_file = cfg;
    }
    ret = config->Read(config_file.c_str());
    assert(ret == true);
    AppDebug("master_enable:%d, slave_enable:%d", config->MasterEnable(), config->SlaveEnable());
    db = new DbParams(this);
    if(config->SlaveEnable()) {
        slave = new SlaveParams(this);
        slave->Start();
    }
    if(config->MasterEnable()) {
        master = new MasterParams(this);
        master->Start();
    }
    while(running) {
        if(!access(DEBUG_STOP, F_OK)) {
            running = 0;
            break;
        }
        UpdateTime();
        sleep(2);
    }
}

void MediaServer::UpdateTime(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    now_sec = tv.tv_sec;
}

