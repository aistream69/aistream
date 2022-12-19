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

#ifndef __AISTREAM_FRAMEWORK_H__
#define __AISTREAM_FRAMEWORK_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class Framework {
 public:
  Framework(void) {}
  virtual ~Framework(void) {}
  virtual int Init(char* path, ElementData* data, char* _params = NULL) {
    return 0;
  }
  virtual int Start(int channel, char* _params = NULL) {
    return 0;
  }
  virtual int Process(TensorData* data) {
    return 0;
  }
  virtual int Stop(void) {
    return 0;
  }
  virtual int Notify(void) {
    return 0;
  }
  virtual int Release(void) {
    return 0;
  }
 private:
};

#endif

