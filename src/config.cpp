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

ConfigParams::ConfigParams(MediaServer* _media)
  : media(_media) {
      out_params = nullptr;
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
    auto localhost = GetStrValFromJson(ptr, "system", "localhost");
    if(localhost != nullptr) {
        strncpy(local_ip, localhost.get(), sizeof(local_ip));
    }
    else {
        GetLocalIp(local_ip);
    }
    NginxInit(nginx);
    AppDebug("nginx path:%s, http_port:%d", nginx.workdir, nginx.http_port);
    
    return true;
}

void ConfigParams::SetOutput(char *str) {
    std::shared_ptr<char> p(new char[strlen(str)+1]);
    strcpy(p.get(), str);
    out_params = p;
}

