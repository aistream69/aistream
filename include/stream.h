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

#ifndef __AISTREAM_H__
#define __AISTREAM_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "master.h"
#include "slave.h"
#include "config.h"
#include "db.h"
#include "obj.h"
#include "rest.h"
#include "pipeline.h"
#include "db.h"
#include "obj.h"
#include "task.h"
#include "share.h"
#include "log.h"

#define SW_VERSION      "V0.01.2021112201"

class MediaServer {
 public:
  MediaServer(void);
  ~MediaServer(void);
  void Run(const char* cfg);
  MasterParams* GetMaster(void) {
    return master;
  }
  SlaveParams* GetSlave(void) {
    return slave;
  }
  ConfigParams* GetConfig(void) {
    return config;
  }
  ObjParams* GetObjParams(void) {
    return obj_params;
  }
  DbParams* GetDB(void) {
    return db;
  }
  std::string config_file;
  int running;
  int system_init;
  long int now_sec;
 private:
  void UpdateTime(void);
  MasterParams*   master;
  SlaveParams*    slave;
  ConfigParams*   config;
  ObjParams*      obj_params;
  DbParams*       db;
};

#endif

