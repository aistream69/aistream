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

#ifndef __AISTREAM_CONFIG_H__
#define __AISTREAM_CONFIG_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONFIG_FILE     "cfg/config.json"

class MediaServer;
class ConfigParams {
public:
    ConfigParams(MediaServer* _media);
    ~ConfigParams(void);
    bool Read(const char *cfg);
    char local_ip[128];
    int master_enable;
    int slave_enable;
    int master_rest_port;
    int slave_rest_port;
    int obj_max;
private:
    MediaServer* media;
};

#endif

