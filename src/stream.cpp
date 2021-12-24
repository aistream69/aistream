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
    config = new ConfigParams(this);
    obj_params = new ObjParams(this);
    db = new DbParams(this);
}

MediaServer::~MediaServer(void) {
}

void MediaServer::run(void) {
    bool ret;
    ret = config->Read(CONFIG_FILE);
    assert(ret == true);
    AppDebug("master_enable:%d, slave_enable:%d", config->MasterEnable(), config->SlaveEnable());
    if(config->MasterEnable()) {
        master = new MasterParams(this);
        master->start();
    }
    if(config->SlaveEnable()) {
        slave = new SlaveParams(this);
        slave->start();
    }
    while(running) {
        if(!access("aistream.stop", F_OK)) {
            break;
        }
        sleep(2);
    }
}

