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

#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <sys/stat.h>

#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>

#include "infer.grpc.pb.h"

using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;
using google::protobuf::Empty;
using triton::ModelInfer;
using triton::InferResponse;
using triton::InferRequest;
using triton::Greeter;
using triton::ModelRepos;
using triton::LoadRequest;

static int ReadFilee(const char* filename, void* buf, int max) {
  struct stat f_stat;
  FILE *fp = fopen(filename, "rb");
  if (fp == NULL) {
    printf("fopen %s failed\n", filename);
    return -1;
  }
  if (stat(filename, &f_stat) == 0) {
    if (f_stat.st_size > max) {
      return -1;
    }
  }
  int n = fread(buf, 1, f_stat.st_size, fp);
  fclose(fp);
  return n;
}

class ModelInferClient {
 public:
  explicit ModelInferClient(std::shared_ptr<Channel> channel)
      : stub_(ModelInfer::NewStub(channel)) {}

  // Assembles the client's payload and sends it to the server.
  void Infer(const char* model_name,
             const char* buf,
             int size, int batch_size) {
    // Data we are sending to the server.
    InferRequest request;
    request.set_name(model_name);
    request.set_request_id(++frame_id);
    for (int i = 0; i < batch_size; i ++) {
      auto input = request.add_inputs();
      // TODO: model input num > 1
      auto tensor = input->add_tensor();
      tensor->set_data(buf, size);
    }
    // Call object to store rpc data
    AsyncClientCall* call = new AsyncClientCall;

    // stub_->PrepareAsyncInfer() creates an RPC object, returning
    // an instance to store in "call" but does not actually start the RPC
    // Because we are using the asynchronous API, we need to hold on to
    // the "call" instance in order to get updates on the ongoing RPC.
    call->response_reader =
        stub_->PrepareAsyncInfer(&call->context, request, &cq_);

    // StartCall initiates the RPC call
    call->response_reader->StartCall();

    // Request that, upon completion of the RPC, "reply" be updated with the
    // server's response; "status" with the indication of whether the operation
    // was successful. Tag the request with the memory address of the call
    // object.
    call->response_reader->Finish(&call->reply, &call->status, (void*)call);
  }

  // Loop while listening for completed responses.
  // Prints out the response from the server.
  void AsyncCompleteRpc() {
    void* got_tag;
    bool ok = false;

    // Block until the next result is available in the completion queue "cq".
    while (cq_.Next(&got_tag, &ok)) {
      // The tag in this example is the memory location of the call object
      AsyncClientCall* call = static_cast<AsyncClientCall*>(got_tag);

      // Verify that the request was completed successfully. Note that "ok"
      // corresponds solely to the request for updates introduced by Finish().
      GPR_ASSERT(ok);

      if (call->status.ok()) {
        uint32_t request_id = call->reply.request_id();
        int batch_size = call->reply.outputs_size();
        printf("RequestId %d, Inference Result:\n", request_id);
        for (int i = 0; i < batch_size; i ++) {
          auto& output = call->reply.outputs(i);
          int size = output.tensor_size();
          printf("batch[%d], output num: %d\n", i, size);
          for (int j = 0; j < size; j ++) {
            auto& tensor = output.tensor(j);
            const char* buf = tensor.data().data();
            size_t len = tensor.data().size();
            printf("output[%d], name:%s, size:%ld\n",
                   j, tensor.name().data(), len);
            std::string filename = "batch_" + std::to_string(i) +
                                   "_output_" + std::to_string(j);
            FILE* fp = fopen(filename.c_str(), "wb");
            fwrite(buf, 1, len, fp);
            fclose(fp);
          }
        }
        printf("\n");
      }
      else {
        std::cout << "rpc failed, " << call->status.error_code() << ": " 
                  << call->status.error_message() << std::endl;
      }

      // Once we're complete, deallocate the call object.
      delete call;
    }
  }

 private:
  // struct for keeping state and data information
  struct AsyncClientCall {
    // Container for the data we expect from the server.
    InferResponse reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // Storage for the status of the RPC upon completion.
    Status status;

    std::unique_ptr<ClientAsyncResponseReader<InferResponse>> response_reader;
  };

  // Out of the passed in Channel comes the stub, stored here, our view of the
  // server's exposed services.
  std::unique_ptr<ModelInfer::Stub> stub_;

  // The producer-consumer queue we use to communicate asynchronously with the
  // gRPC runtime.
  CompletionQueue cq_;
  uint32_t frame_id = 0;
};

class GreeterClient {
 public:
  GreeterClient(std::shared_ptr<Channel> channel)
      : stub_(Greeter::NewStub(channel)) {}

  void GetModelRepository() {
    Empty request;
    ModelRepos reply;
    ClientContext context;
    Status status = stub_->GetModelRepository(&context, request, &reply);
    if (status.ok()) {
      int size = reply.model_size();
      printf("Model Repository:\n");
      printf("------------------------------------"
             "------------------------------------\n");
      for (int i = 0; i < size; i ++) {
        auto& model = reply.model(i);
        printf("name:%s, batch_size:%d, "
               "input num:%d, output num:%d, running:%d\n",
               model.name().data(), model.batch_size(),
               model.inputs_size(), model.outputs_size(), model.running());
        for (int j = 0; j < model.inputs_size(); j ++) {
          auto& tensor = model.inputs(j);
          printf("input[%d], name:%s, dtype:%d\nshape: [",
                 j, tensor.name().data(), tensor.dtype());
          for (int n = 0; n < tensor.shape_size(); n ++) {
            printf("%d ", tensor.shape(n));
          }
          printf("]\n");
        }
        for (int j = 0; j < model.outputs_size(); j ++) {
          auto& tensor = model.outputs(j);
          printf("output[%d], name:%s, dtype:%d\nshape: [",
                 j, tensor.name().data(), tensor.dtype());
          for (int n = 0; n < tensor.shape_size(); n ++) {
            printf("%d ", tensor.shape(n));
          }
          printf("]\n");
        }
        printf("------------------------------------"
               "------------------------------------\n");
      }
    } else {
      std::cout << "rpc failed, " << status.error_code() << ": " 
                << status.error_message() << std::endl;
    }
  }

 private:
  std::unique_ptr<Greeter::Stub> stub_;
};

static void GetServerStatus() {
  std::string target_str = "localhost:50052";
  GreeterClient greeter(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
  greeter.GetModelRepository();
}

int main(int argc, char** argv) {

  // Get model repository
  GetServerStatus();

  if (argc < 5) {
    printf("warning, usage: ./test model_name input_data batch_size num");
    return 0;
  }

  const char* model_name = argv[1];
  const char* input_data = argv[2];
  int batch_size = atoi(argv[3]);
  int num = atoi(argv[4]);

  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint (in this case,
  // localhost at port 50051). We indicate that the channel isn't authenticated
  // (use of InsecureChannelCredentials()).
  ModelInferClient client(grpc::CreateChannel(
      "localhost:50051", grpc::InsecureChannelCredentials()));

  // Spawn reader thread that loops indefinitely
  std::thread thread_ = std::thread(&ModelInferClient::AsyncCompleteRpc, &client);
  int max = 1024*1024*100;
  char* buf = new char[max];
  int filesize = ReadFilee(input_data, buf, max);
  for (int i = 0; i < num; i++) {
    // The actual RPC call!
    client.Infer(model_name, buf, filesize, batch_size);
  }
  free(buf);

  thread_.join();  // blocks forever

  return 0;
}
