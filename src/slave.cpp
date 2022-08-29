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
#include "rest.h"

SlaveParams::SlaveParams(MediaServer* _media)
  : media(_media) {
    pipe = new Pipeline(media);
}

SlaveParams::~SlaveParams(void) {
}

static int DelOldImage(SlaveParams* slave) {
    char dir[URL_LEN+16];
    MediaServer* media = slave->media;
    ConfigParams* config = media->GetConfig();
    uint32_t max_sec = config->img_save_days * (24 * 60 * 60);
    snprintf(dir, sizeof(dir), "%s/image", config->nginx.workdir);
    DelOldFile(dir, max_sec, 0, ".jpg");
    return 0;
}

void SlaveParams::SlaveManager(void) {
    int i = 0;
    while(media->running) {
        if(i++ % 360 == 0) {
            DelOldImage(this);
        }
        sleep(10);
    }
    AppDebug("run ok");
}

void SlaveParams::Start(void) {
    SlaveRestful* rest = new SlaveRestful(media);
    ObjParams* obj_params = media->GetObjParams();
    pipe->Start();
    rest->Start();
    obj_params->Start();
    std::thread t(&SlaveParams::SlaveManager, this);
    t.detach();
}

