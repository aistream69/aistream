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
#include "obj.h"
#include "httpfile.h"
#include <thread>

ObjParams::ObjParams(MediaServer* _media)
  : media(_media) {
}

ObjParams::~ObjParams(void) {
}

bool ObjParams::Put2ObjQue(std::shared_ptr<Object> obj) {
  obj_mtx.lock();
  obj_vec.push_back(obj);
  obj_mtx.unlock();
  return true;
}

void ObjParams::TraverseObjQue(void* arg, int (*cb)(std::shared_ptr<Object> obj, void* arg)) {
  obj_mtx.lock();
  auto _end = obj_vec.end();
  for (auto itr = obj_vec.begin(); itr != _end; ++itr) {
    if (cb(*itr, arg) != 0) {
      break;
    }
  }
  obj_mtx.unlock();
}

std::shared_ptr<Object> ObjParams::GetObj(int id) {
  std::shared_ptr<Object> obj = nullptr;
  obj_mtx.lock();
  for (auto itr = obj_vec.begin(); itr != obj_vec.end(); ++itr) {
    if ((*itr)->GetId() == id) {
      obj = *itr;
      break;
    }
  }
  obj_mtx.unlock();
  return obj;
}

bool ObjParams::DelFromObjQue(int id) {
  obj_mtx.lock();
  for (auto itr = obj_vec.begin(); itr != obj_vec.end(); ++itr) {
    auto obj = *itr;
    if (obj->GetId() == id) {
      obj->DelFromTaskQue("all");
      obj_vec.erase(itr);
      break;
    }
  }
  obj_mtx.unlock();
  return true;
}

int ObjParams::GetObjNum(void) {
  return (int)obj_vec.size();
}

void ObjParams::ObjManager(void) {
  while (media->running) {
    obj_mtx.lock();
    for (auto itr = obj_vec.begin(); itr != obj_vec.end(); ++itr) {
      (*itr)->TraverseTaskQue();
    }
    obj_mtx.unlock();
    sleep(2);
  }
  AppDebug("run ok");
}

void ObjParams::StartHttpfileTask(void) {
  int size = 0;
  SlaveParams* slave = media->GetSlave();
  Pipeline* pipe = slave->GetPipe();;
  ConfigParams* config = media->GetConfig();
  const char* filename = media->config_file.c_str();

  // wait output init from master
  while (config->GetOutput() == nullptr) {
    sleep(1);
  }
  AppDebug("wait output config ok, start task ...");
  // cal obj base id by ip address
  int a,b,c,d;
  char* local_ip = config->LocalIp();
  sscanf(local_ip, "%d.%d.%d.%d", &a, &b, &c, &d);
  int id = (d<<24) + (c<<16) + (b<<8) + a;
  auto buf = GetArrayBufFromFile(filename, size, "system", "httpfile");
  if (buf == nullptr) {
    //AppWarn("read %s failed", filename);
    return;
  }
  for (int i = 0; i < size; i ++) {
    // parase task from json config
    auto arrbuf = GetBufFromArray(buf.get(), i);
    if (arrbuf == nullptr) {
      break;
    }
    auto task_name = GetStrValFromJson(arrbuf.get(), "task");
    if (task_name == nullptr) {
      AppWarn("get task name failed");
      break;
    }
    // wait alg support
    int try_sec = 10;
    do {
      auto alg = pipe->GetAlgTask(task_name.get());
      if (alg != nullptr) {
        break;
      }
      sleep(1);
    } while (try_sec--);
    // add obj id
    auto obj = std::make_shared<HttpFile>(media);
    obj->SetId(id + i);
    obj->SetParams(arrbuf.get());
    Put2ObjQue(obj);
    // start task
    auto task = std::make_shared<TaskParams>(obj);
    task->SetTaskName(task_name.get());
    obj->Put2TaskQue(task);
  }
}

void ObjParams::Start(void) {
  std::thread t(&ObjParams::ObjManager, this);
  t.detach();
  std::thread _t(&ObjParams::StartHttpfileTask, this);
  _t.detach();
}

Object::Object(MediaServer *_media)
  : media(_media) {
  params = nullptr;
}

Object::~Object(void) {
}

void Object::SetParams(char *str) {
  std::shared_ptr<char> p(new char[strlen(str)+1]);
  strcpy(p.get(), str);
  params = p;
}

bool Object::Put2TaskQue(std::shared_ptr<TaskParams> task) {
  task_mtx.lock();
  task_vec.push_back(task);
  task_mtx.unlock();
  return true;
}

std::shared_ptr<TaskParams> Object::GetTask(char *name) {
  std::shared_ptr<TaskParams> task = nullptr;
  task_mtx.lock();
  for (auto itr = task_vec.begin(); itr != task_vec.end(); ++itr) {
    if (!strcmp((*itr)->GetTaskName(), name)) {
      task = *itr;
      break;
    }
  }
  task_mtx.unlock();
  return task;
}

bool Object::DelFromTaskQue(const char *name) {
  task_mtx.lock();
  for (auto itr = task_vec.begin(); itr != task_vec.end(); ++itr) {
    if (!strcmp(name, "all") || !strcmp((*itr)->GetTaskName(), name)) {
      (*itr)->Stop();
      task_vec.erase(itr);
      break;
    }
  }
  task_mtx.unlock();
  return true;
}

void Object::TraverseTaskQue(void) {
  CheckWorkDir();
  task_mtx.lock();
  for (auto itr = task_vec.begin(); itr != task_vec.end(); ++itr) {
    auto task = (*itr);
    if (!task->KeepAlive()) {
      task->Stop(true);
      task->Start();
    }
  }
  task_mtx.unlock();
}

void Object::CheckWorkDir(void) {
  struct tm _time;
  char path[URL_LEN*2], date[32];
  ConfigParams* config = media->GetConfig();
  localtime_r(&media->now_sec, &_time);
  snprintf(date, sizeof(date), "%d%02d%02d", _time.tm_year + 1900, _time.tm_mon + 1, _time.tm_mday);
  snprintf(path, sizeof(path), "%s/image/%s/%d", config->nginx.workdir, date, id);
  DirCheck(path);
}

