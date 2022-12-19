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
#include "task.h"
#include "dylib.h"

TaskParams::TaskParams(std::shared_ptr<Object> _obj)
  : obj(_obj) {
  running = 0;
  task_beat = 0;
  params = nullptr;
  alg = nullptr;
}

TaskParams::~TaskParams(void) {
}

void TaskParams::SetParams(char *str) {
  std::shared_ptr<char> p(new char[strlen(str)+1]);
  strcpy(p.get(), str);
  params = p;
}

int TaskParams::Start(void) {
  auto _obj = GetTaskObj();
  assert(_obj != nullptr);

  MediaServer* media = _obj->media;
  SlaveParams* slave = media->GetSlave();
  Pipeline* pipe = slave->GetPipe();;
  alg = pipe->GetAlgTask(name);
  if (alg == nullptr) {
    AppWarn("id:%d, task:%s, get task failed", _obj->GetId(), name);
    return -1;
  }
  if (alg->AssignToThreads(shared_from_this()) != true) {
    AppWarn("assign ele to threads failed, id:%d,task:%s", _obj->GetId(), name);
    return -1;
  }

  running = 1;
  for (size_t i = 0; i < thread_vec.size(); i ++) {
    auto tt = thread_vec[i];
    tt->Start(shared_from_this());
  }
  AppDebug("start task ok, id:%d,task:%s", _obj->GetId(), name);

  return 0;
}

int TaskParams::Stop(bool sync) {
  running = 0;
  for (size_t i = 0; i < thread_vec.size(); i ++) {
    auto tt = thread_vec[i];
    tt->Stop(sync);
  }
  if (sync) {
    thread_vec.clear();
  }

  return 0;
}

bool TaskParams::KeepAlive(void) {
  auto _obj = GetTaskObj();
  assert(_obj != nullptr);
  MediaServer* media = _obj->media;
  ConfigParams* config = media->GetConfig();

  if (task_beat == 0) {
    task_beat = media->now_sec;
    return false;
  } else if (media->now_sec - task_beat > config->GetTaskTimeout()) {
    AppWarn("id:%d,task:%s,detected exception, restart it ...", 
            _obj->GetId(), name);
    task_beat = media->now_sec;
    return false;
  }

  return true;
}

void TaskParams::BeatAlive(MediaServer* media) {
  task_beat = media->now_sec;
}

std::shared_ptr<Object> TaskParams::GetTaskObj(void) {
  std::shared_ptr<Object> ret = nullptr;
  if (obj.expired()) {
    AppWarn("task:%s, obj is expired", name);
    return ret;
  }
  return obj.lock();
}

TaskElement::TaskElement(std::shared_ptr<TaskParams> _task)
  : task(_task) {
  framework = nullptr;
  exception_cnt = 0;
}

TaskElement::~TaskElement(void) {
}

void TaskElement::ConnectElement(void) {
  for (size_t i = 0; i < data.input.size(); i++) {
    auto _queue = data.input[i];
    auto _map = GetInputMap([](KeyValue* p, void* arg) {
      char* name = (char* )arg;
      if (!strncmp(p->key, name, sizeof(p->key))) {
        return true;
      } else {
        return false;
      }
    }, _queue->name);
    if (_map == nullptr) {
      AppWarn("%s, get input by %s failed", GetName(), _queue->name);
      continue;
    }
    bool connect = false;
    for (size_t j = 0; j < task->thread_vec.size() && !connect; j ++) {
      auto tt = task->thread_vec[j];
      for (size_t k = 0; k < tt->t_ele_vec.size(); k ++) {
        auto ele = tt->t_ele_vec[k];
        auto output = ele->GetOutputMap();
        if (output == nullptr) {
          AppWarn("get output map failed, %s", ele->GetName());
          continue;
        }
        if (!strncmp(_map->val, output->val, sizeof(output->val))) {
          ele->data.output.push_back(_queue);
          connect = true;
          break;
        }
      }
    }
    if (!connect) {
      AppWarn("connect %s %s:%s to previous failed", 
              GetName(), _map->key, _map->val);
    }
  }
}

bool TaskElement::Start(void) {
  char* path = GetPath();
  char* framework_name = GetFramework();
  auto obj = task->GetTaskObj();
  assert(obj != nullptr);
  MediaServer* media = obj->media;
  ConfigParams* config = media->GetConfig();

  auto ele_params = GetParams();
  auto task_params = task->GetParams();
  if (!strcmp(framework_name, "tvm")) {
    AppDebug("TODO:tvm");
  } else if (!strcmp(framework_name, "tensorrt")) {
    AppDebug("TODO:tensorrt");
  } else if (strlen(framework_name) == 0) {
    framework = std::make_unique<DynamicLib>();
    if (!strcmp(GetName(), "object")) {
      path = obj->GetPath(path);
      task_params = obj->GetParams();
    }
  } else {
    AppWarn("unsupport framework:%s", framework_name);
  }
  assert(framework != nullptr);

  //printf("task:%s, ele:%s,path:%s\n", task->GetTaskName(), GetName(), path);
  // params from json, for example: samples/face_detection.json
  char* params = ele_params != nullptr ? ele_params.get() : NULL;
  // ouput params from restful api
  auto out_params = config->GetOutput();
  if (out_params != nullptr && !strcmp(GetName(), "rabbitmq")) {
    params = out_params.get();
  }
  if (framework->Init(path, &data, params) != 0) {
    AppWarn("framework start failed, id:%d, %s", obj->GetId(), path);
    return false;
  }
  if (data.input.size() == 0 && strcmp(GetName(), "object") != 0) {
    AppWarn("%s input num is 0, please init correctly", GetName());
    return false;
  }
  for (size_t j = 0; j < data.input.size(); j++) {
    auto input = data.input[j];
    input->running = &task->running;
  }

  params = task_params != nullptr ? task_params.get() : NULL;
  if (framework->Start(obj->GetId(), params) != 0) {
    AppWarn("framework start failed, id:%d, %s", obj->GetId(), path);
    return false;
  }
  // connect ele input to it's previous output
  ConnectElement();

  return true;
}

bool TaskElement::Stop(void) {
  if (framework != nullptr) {
    framework->Stop();
    framework->Release();
  }
  return 0;
}

TaskThread::TaskThread(void) {
  t = nullptr;
  task = nullptr;
}

TaskThread::~TaskThread(void) {
}

static int GetInput(TensorData& tensor, auto ele, auto obj) {
  if (ele->data.input.size() == 0) {
    return 0;
  }
  // pop highest priority input
  auto input = ele->data.input[0];
  std::unique_lock<std::mutex> lock(input->mtx);
  if (input->_queue.empty()) {
    input->condition.wait(lock, [input] {
      return !input->_queue.empty() || !(*input->running);
    });
    if (!(*input->running)) {
      return -1;
    }
  }
  auto pkt = input->_queue.front();
  input->_queue.pop();
  lock.unlock();
  tensor._in.push_back(pkt);

  // search and match other inputs
  int frame_id = pkt->_params.frame_id;
  for (size_t i = 1; i < ele->data.input.size(); i++) {
    auto input = ele->data.input[i];
    std::unique_lock<std::mutex> lock(input->mtx);
    while (!input->_queue.empty()) {
      auto pkt = input->_queue.front();
      input->_queue.pop();
      if (pkt->_params.frame_id >= frame_id) {
        tensor._in.push_back(pkt);
        break;
      }
    }
    if (tensor._in.size() != i + 1) {
      AppWarn("id:%d, %s, i:%ld, match frameid %d failed, "
              "please check if the slower input connect to input[0]",
              obj->GetId(), ele->GetName(), i, frame_id);
      return 1;
    }
  }
  return 0;
}

void TaskThread::ThreadFunc(void) {
  auto obj = task->GetTaskObj();
  assert(obj != nullptr);
  MediaServer* media = obj->media;

  for (size_t i = 0; i < t_ele_vec.size(); i ++) {
    auto ele = t_ele_vec[i];
    if (!ele->Start()) {
      task->running = 0;
      break;
    }
  }

  while (task->running) {
    for (size_t i = 0; i < t_ele_vec.size(); i ++) {
      TensorData tensor;
      auto ele = t_ele_vec[i];
      int ret = GetInput(tensor, ele, obj);
      if (ret != 0) {
        if (ret < 0) {
          break;
        }
        continue;
      }
      tensor.tensor_buf.output_num = ele->data.output.size();
      if (ele->framework->Process(&tensor) != 0) {
        break;
      }
      for (size_t j = 0; j < ele->data.output.size() && 
           tensor._out != nullptr; j ++) {
        auto output = ele->data.output[j];
        if (output == nullptr) {
          AppWarn("id:%d,%s,%ld,shared_ptr exception",
                  obj->GetId(), ele->GetName(), j);
          continue;
        }
        std::unique_lock<std::mutex> lock(output->mtx);
        if (output->_queue.size() >= (size_t)ele->data.queue_len) {
          output->_queue.pop();
          if (ele->exception_cnt++ % 200 == 0) {
            printf("warning,id:%d,%s,output[%ld],%d,%d,queue is full\n",
                   obj->GetId(), ele->GetName(), j,
                   ele->data.queue_len, ele->exception_cnt);
          }
        }
        output->_queue.push(tensor._out);
        output->condition.notify_one();
      }
      if (ele->data.sleep_usec) {
        // Note: if t_ele_vec.size() > 1, please set sleep_usec correctly
        usleep(ele->data.sleep_usec);
      }
    }
    task->BeatAlive(media);
  }

  for (size_t i = 0; i < t_ele_vec.size(); i ++) {
    auto ele = t_ele_vec[i];
    for (size_t j = 0; j < ele->data.output.size(); j ++) {
      auto output = ele->data.output[j];
      std::unique_lock<std::mutex> lock(output->mtx);
      while (!output->_queue.empty()) output->_queue.pop();
      /* when stoping task by restful api, in order to return quickly,
       * sync is false, and tt->stop will not be invoked,
       * so it should notify next module here */
      output->condition.notify_all();
    }
    ele->Stop();
  }
  AppDebug("id:%d, %s, run ok", obj->GetId(), t_ele_vec[0]->GetName());
}

void TaskThread::Start(std::shared_ptr<TaskParams> _task) {
  task = _task;
  t = new std::thread(&TaskThread::ThreadFunc, shared_from_this());
}

void TaskThread::Stop(bool sync) {
  for (size_t i = 0; i < t_ele_vec.size(); i ++) {
    auto ele = t_ele_vec[i];
    for (size_t j = 0; j < ele->data.output.size(); j ++) {
      auto output = ele->data.output[j];
      std::unique_lock<std::mutex> lock(output->mtx);
      output->condition.notify_all();
    }
    ele->framework->Notify();
  }
  if (sync && t != nullptr) {
    if (t->joinable()) {
      t->join();
    }
    delete t;
    t = nullptr;
  }
}

