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

#ifndef __AISTREAM_CONFIG_H__
#define __AISTREAM_CONFIG_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define URL_LEN 512
#define CONFIG_FILE     "cfg/config.json"

typedef struct {
    int http_port;
    char workdir[URL_LEN];
} NginxParams;

class MediaServer;
class ConfigParams {
public:
    ConfigParams(MediaServer* _media);
    ~ConfigParams(void);
    bool Read(const char *cfg);
    char* LocalIp(void) {return local_ip;}
    int MasterEnable(void) {return master_enable;}
    int SlaveEnable(void) {return slave_enable;}
    int GetMRestPort(void) {return master_rest_port;}
    int GetSRestPort(void) {return slave_rest_port;}
    int GetObjMax(void) {return obj_max;}
    void SetOutput(char *str);
    auto GetOutput(void) {return out_params;}
    NginxParams nginx;
    MediaServer* media;
private:
    char local_ip[128];
    int master_enable;
    int slave_enable;
    int master_rest_port;
    int slave_rest_port;
    int obj_max;
    std::shared_ptr<char> out_params;
};

#endif

