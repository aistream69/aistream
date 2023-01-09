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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <thread>
#include "tensor.h"
#include "share.h"
#include "log.h"

#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>
#include "infer.grpc.pb.h"
using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;
using grpc::StatusCode;
using google::protobuf::Empty;
using triton::ModelInfer;
using triton::InferResponse;
using triton::InferRequest;
using triton::Greeter;
using triton::ModelRepos;
using triton::LoadRequest;

typedef struct {
  uint32_t request_id;
  void* arg;            // ModelParams
} ProcessAddr;

typedef struct {
  // engine params
  int batch_size;
  int running;
} ModelParams;

typedef struct {
  int init;
  std::string model_repository;
  std::mutex model_mtx;
  std::map<std::string, ModelParams> models;
  int infer_port;
  int cmd_port;
} ModuleParams;

static ModuleParams module = {0};

// The step of processing that the state is in. 
// Every state must recognize START.
typedef enum {
  START,
  COMPLETE,
  FINISH,
} Steps;

class InferState {
 public:
  InferState(ServerCompletionQueue* cq) : cq_(cq), responder_(&ctx_) {
    step_ = Steps::START;
  }
  ~InferState() {}
  uint64_t unique_id_;
  Steps step_;

  ServerCompletionQueue* cq_;
  ServerContext ctx_;
  InferRequest request_;
  InferResponse reply_;
  ServerAsyncResponseWriter<InferResponse> responder_;
};

typedef std::function<void (const char*, ModelParams*)> ReadModelCB;
static void ReadModel(ReadModelCB cb = nullptr) {
  struct dirent* dp;
  struct stat st_dir = {0};
  struct stat st = {0};
  const char* dir = module.model_repository.data();
  stat(dir, &st_dir);
  DIR* dirp = opendir(dir);
  if (dirp == NULL) {
    AppWarn("opendir %s failed", dir);
    return;
  }
  std::vector<std::string> model_vec;
  while ((dp = readdir(dirp)) != NULL) {
    char cfg[PATH_MAX];
    char path[PATH_MAX];
    const char* name = dp->d_name;
    // scan model repository directory
    if (!strcmp(name, ".") || !strcmp(name, "..")) {
      continue;
    }
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    if (stat(path, &st) != 0 || !S_ISDIR(st.st_mode)) {
      continue;
    }
    snprintf(path, sizeof(path), "%s/%s/model.ngf", dir, name);
    snprintf(cfg, sizeof(cfg), "%s/%s/config.json", dir, name);
    if (access(path, F_OK) != 0 || access(cfg, F_OK) != 0) {
      continue;
    }
    model_vec.push_back(name);
    // update model map
    ModelParams* p = nullptr;
    std::unique_lock<std::mutex> lock(module.model_mtx);
    auto itr = module.models.find(name);
    if (itr == module.models.end()) {
      ModelParams model = {0};
      if (0) {
        AppWarn("create engine failed, %s", path);
      }
      else {
        model.running = 1;
      }
      model.batch_size = GetIntValFromFile(cfg, "batch_size");
      if (model.batch_size < 0) model.batch_size = 1;
      auto [it, success] = module.models.insert({name, model});
      if (!success) {
        AppWarn("insert model failed, %s", path);
      }
      else {
        p = &it->second;
        //StreamEvtCreate(p);
      }
    }
    else {
      p = &itr->second;
    }
    if (cb != nullptr && p != nullptr) {
      cb(name, p);
    }
    lock.unlock();
  }
  closedir(dirp);
}

class GreeterServiceImpl final : public Greeter::Service {
  Status GetModelRepository(ServerContext* context, const Empty* request,
                  ModelRepos* reply) override {
    // cudaSetDevice(dev);
    ReadModel([reply](const char* name, ModelParams* p) {
      auto model = reply->add_model();
      model->set_name(name);
      model->set_batch_size(p->batch_size);
      model->set_running(p->running);
      if (!p->running) {
        return;
      }
      int num_input = 1;
      int num_output = 1;
      for (int i = 0; i < num_input; i ++) {
        auto input = model->add_inputs();
        input->set_name("test");
        input->set_dtype(static_cast<triton::DataType>(1));
        for (uint32_t j = 0; j < 3; j ++) {
          input->add_shape(j + 1);
        }
      }
      for (int i = 0; i < num_output; i ++) {
        auto output = model->add_outputs();
        output->set_name("test");
        output->set_dtype(static_cast<triton::DataType>(1));
        for (uint32_t j = 0; j < 3; j ++) {
          output->add_shape(j + 1);
        }
      }
    });
    return Status::OK;
  }
};

class ServerImpl final {
 public:
  ~ServerImpl() {
    infer_server->Shutdown();
    cq_->Shutdown();
  }

  void Run() {
    ReadModel();
    RunInferServer();
    RunCommandServer();
  }

 private:
  void StartNewRequest() {
    InferState* state = new InferState(cq_.get());
    infer_service.RequestInfer(&state->ctx_, &state->request_, 
                             &state->responder_, state->cq_, state->cq_, state);
  }

  bool Process(InferState* state, bool rpc_ok) {
    bool finished = false;
    const bool shutdown = (!rpc_ok && (state->step_ == Steps::START));
    if (shutdown) {
      state->step_ = Steps::FINISH;
      finished = true;
    }
    if (state->step_ == Steps::START) {
      // Start a new request to replace this one...
      if (!shutdown) {
        StartNewRequest();
      }
      // The actual processing.
      auto& request = state->request_;
      auto itr = module.models.find(request.name());
      if (itr == module.models.end() || !itr->second.running) {
        printf("warning, infer %s, find model failed\n", request.name().data());
        return !finished;
      }
      auto& model = itr->second;
      int batch_size = model.batch_size;
      int num_input = 1;
      if (batch_size != request.inputs_size() ||
          num_input != request.inputs(0).tensor_size()) {
        printf("warning, infer %s, match batch_size/input_num failed\n",
               request.name().data());
        return !finished;
      }
      auto& input = request.inputs(0);
      for (int i = 0; i < num_input; i ++) {
        long unsigned int tensor_size = 1;
        auto& input_tensor = input.tensor(i);
        if (tensor_size != input_tensor.data().size()) {
          printf("warning, input[%d], %s, match data size failed, %ld:%ld\n",
                 i, request.name().data(), tensor_size,
                 input_tensor.data().size());
          return !finished;
        }
      }
      state->step_ = Steps::COMPLETE;
      state->responder_.Finish(state->reply_, Status::OK, state);
    } else if (state->step_ == Steps::COMPLETE) {
      state->step_ = Steps::FINISH;
      finished = true;
    }

    return !finished;
  }

  void RunInferServer() {
    std::string ip = "0.0.0.0:";
    std::string port = std::to_string(module.infer_port);
    std::string server_address = ip + port;

    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service_" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *asynchronous* service.
    builder.RegisterService(&infer_service);
    // Get hold of the completion queue used for the asynchronous communication
    // with the gRPC runtime.
    cq_ = builder.AddCompletionQueue();
    // Finally assemble the server.
    infer_server = builder.BuildAndStart();
    AppDebug("grpc inference listening on %s", server_address.c_str());
    
    // TODO: multi threads
    tid_infer = new std::thread([this] {
      bool ok;
      void* tag;
      StartNewRequest();
      while (cq_->Next(&tag, &ok)) {
        GPR_ASSERT(ok);
        InferState* state = static_cast<InferState* >(tag);
        if (!Process(state, ok)) {
          delete state; // TODO: reuse
        }
      }
    });
  }

  void RunCommandServer() {
    std::string ip = "0.0.0.0:";
    std::string port = std::to_string(module.cmd_port);
    std::string server_address = ip + port;

    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&cmd_service);
    // Finally assemble the server.
    cmd_server = builder.BuildAndStart();
    AppDebug("grpc infer cmd listening on %s", server_address.c_str());

    // Enter loop, wait for the server to shutdown.
    tid_cmd = new std::thread([this] {
      cmd_server->Wait();
    });
  }

  std::unique_ptr<ServerCompletionQueue> cq_;
  ModelInfer::AsyncService infer_service;
  std::unique_ptr<Server> infer_server;
  GreeterServiceImpl cmd_service;
  std::unique_ptr<Server> cmd_server;
  std::thread* tid_infer;
  std::thread* tid_cmd;
};

typedef struct {
  int id;
  ServerImpl server;
} ModuleObj;

extern "C" int ModuleInit(ElementData* data, char* params) {
  strncpy(data->input_name[0], "input0", sizeof(data->input_name[0]));
  if (__sync_add_and_fetch(&module.init, 1) > 1) {
    return 0;
  }
  if (params == nullptr) {
    AppWarn("params is null");
    return -1;
  }
  auto model = GetStrValFromJson(params, "model_repository");
  int infer_port = GetIntValFromJson(params, "infer_port");
  int cmd_port = GetIntValFromJson(params, "cmd_port");
  if (model == nullptr || infer_port < 0 || cmd_port < 0) {
    AppWarn("get params failed: %s", params);
    return -1;
  }
  module.model_repository = model.get();
  module.infer_port = infer_port;
  module.cmd_port = cmd_port;
  return 0;
}

extern "C" IHandle ModuleStart(int channel, char* params) {
  ModuleObj* obj = new ModuleObj();
  obj->id = channel;
  obj->server.Run();
  return obj;
}

extern "C" int ModuleProcess(IHandle handle, TensorData* data) {
  AppDebug("");
  return 0;
}

extern "C" int ModuleStop(IHandle handle) {
  ModuleObj* obj = (ModuleObj* )handle;
  if (obj == nullptr) {
    AppWarn("obj is null");
    return -1;
  }
  delete obj;
  return 0;
}

extern "C" int ModuleRelease(void) {
  return 0;
}

extern "C" int DylibRegister(DLRegister** r, int& size) {
  size = 1;
  DLRegister* p = (DLRegister*)calloc(size, sizeof(DLRegister));
  strncpy(p->name, "triton", sizeof(p->name));
  p->init = "ModuleInit";
  p->start = "ModuleStart";
  p->process = "ModuleProcess";
  p->stop = "ModuleStop";
  p->release = "ModuleRelease";
  *r = p;
  return 0;
}



