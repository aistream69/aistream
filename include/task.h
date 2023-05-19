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

#ifndef __AISTREAM_TASK_H__
#define __AISTREAM_TASK_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include "pipeline.h"
#include "framework.h"

class Object;

class TaskElement : public Element {
 public:
  TaskElement(std::shared_ptr<TaskParams> _task);
  ~TaskElement(void);
  bool Start(bool sync_in);
  bool Stop(void);
  std::unique_ptr<Framework> framework;
  ElementData data;
  std::mutex data_mtx;
  int exception_cnt;
 private:
  void ConnectElement(void);
  std::shared_ptr<TaskParams> task;
};

class TaskThread : public std::enable_shared_from_this<TaskThread> {
 public:
  TaskThread(void);
  ~TaskThread(void);
  // one thread can have one or more element, decided by async,
  // or element is multi used
  std::vector<std::shared_ptr<TaskElement>> t_ele_vec;
  void Start(std::shared_ptr<TaskParams> _task);
  void Stop(bool sync = false);
 private:
  std::thread* t;
  void ThreadFunc(void);
  std::shared_ptr<TaskParams> task;
};

class TaskParams : public std::enable_shared_from_this<TaskParams> {
 public:
  TaskParams(std::shared_ptr<Object> _obj);
  ~TaskParams(void);
  void SetTaskName(char *_name) {
    strncpy(name, _name, sizeof(name));
  }
  char *GetTaskName(void) {
    return name;
  }
  void SetParams(char *str);
  auto GetParams(void) {
    return params;
  }
  int Start(void);
  int Stop(bool sync = false);
  bool KeepAlive(void);
  void BeatAlive(MediaServer* media);
  void ThreadSync(const char* name);
  std::shared_ptr<Object> GetTaskObj(void);
  std::vector<std::shared_ptr<TaskThread>> thread_vec;
  int running;
 private:
  char name[256];
  // obj thread should set this params in cycle correctly
  long int task_beat;
  size_t thread_sync;
  std::weak_ptr<Object> obj;
  std::shared_ptr<char> params;
  std::shared_ptr<AlgTask> alg;
};

#endif

