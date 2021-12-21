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
    auto buf = ReadFile2Buf(cfg);
    if(buf == nullptr) {
        AppWarn("%s, ReadFile2Buf failed", cfg);
        return false;
    }
    char *ptr = buf.get();
    master_enable = GetIntValFromJson(ptr, "master", "enable");
    slave_enable = GetIntValFromJson(ptr, "slave", "enable");
    master_rest_port = GetIntValFromJson(ptr, "master", "rest_port");
    slave_rest_port = GetIntValFromJson(ptr, "slave", "rest_port");
    obj_max = GetIntValFromJson(ptr, "system", "obj_max");
    GetLocalIp(local_ip);
    return true;
}

