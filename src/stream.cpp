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

#include "stream.h"

MediaServer::MediaServer(void) {
    running = 1;
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

