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

#ifndef __AISTREAM_RTSP_H__
#define __AISTREAM_RTSP_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "obj.h"
#include "share.h"
#include "log.h"

class Rtsp : public Object {
public:
    Rtsp(MediaServer* _media):Object(_media) {
        memset(lib, 0, sizeof(lib));
    }
    ~Rtsp(void) {}
    virtual char* GetPath(char* path) {
        auto name = GetStrValFromFile(path, "rtsp", "lib");
        if(name == nullptr) {
            AppWarn("get path failed, %s", path);
            return lib;
        }
        strncpy(lib, name.get(), sizeof(lib));
        return lib;
    }
private:
    char lib[256];
};

#endif
