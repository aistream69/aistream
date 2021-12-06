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

ConfigParams::ConfigParams(MediaServer* _media)
  : media(_media) {
}

ConfigParams::~ConfigParams(void) {
}

bool ConfigParams::Read(const char *cfg) {
    int ret = false;
    char *buf = ReadFile2Buf(cfg);
    if(buf == NULL) {
        AppWarning("%s, ReadFile2Buf failed", cfg);
        goto end;
    }
    master_enable = GetIntValFromJson(buf, "master", "enable");
    slave_enable = GetIntValFromJson(buf, "slave", "enable");
    master_rest_port = GetIntValFromJson(buf, "master", "rest_port");
    slave_rest_port = GetIntValFromJson(buf, "slave", "rest_port");
    obj_max = GetIntValFromJson(buf, "system", "obj_max");
    GetLocalIp(local_ip);
    ret = true;
end:
    if(buf != NULL) {
        free(buf);
    }
    return ret;
}

