#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <thread>
#include <torch/script.h>
#include "cJSON.h"
#include "tensor.h"
#include "config.h"
#include "share.h"
#include "log.h"

#define VOCAB_MASK_INDEX    103

typedef struct {
  struct torch::jit::Module module;
} BertEngine;

typedef struct {
  int id;
} ModuleObj;

extern int TokenizersInit(const char* vocab_file);
extern std::vector<float> TokenizersEncode(const char* text);
extern const char* ConvertIdToToken(int idx);

static BertEngine* engine = NULL;
static ShareParams share_params = {0};
static std::unique_ptr<char[]> MakeJson(int id, at::Tensor result, char* ptr) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  cJSON* root = cJSON_CreateObject();
  cJSON* data_root =  cJSON_CreateObject();
  cJSON* object_root =  cJSON_CreateArray();
  cJSON_AddStringToObject(root, "msg_type", "common");
  cJSON_AddItemToObject(root, "data", data_root);
  cJSON_AddNumberToObject(data_root, "id", id);
  cJSON_AddNumberToObject(data_root, "timestamp", tv.tv_sec);
  cJSON_AddItemToObject(data_root, "object", object_root);
  for (int64_t idx = 0; idx < result.size(0); idx++) {
    int id = result[idx].item<int>();
    cJSON* obj =  cJSON_CreateObject();
    cJSON_AddItemToArray(object_root, obj);
    cJSON_AddNumberToObject(obj, "index", idx);
    const char* _val = ConvertIdToToken(id);
    cJSON_AddStringToObject(obj, "val", _val);
  }
  if (cJSON *user_data = cJSON_Parse(ptr)) {
    cJSON_DeleteItemFromObject(user_data, "text");
    cJSON_DeleteItemFromObject(user_data, "topk");
    cJSON_AddItemToObject(data_root, "userdata", user_data);
  }
  char *json = cJSON_Print(root);
  auto val = std::make_unique<char[]>(strlen(json) + 1);
  strcpy(val.get(), json);
  free(json);
  cJSON_Delete(root);
  return val;
}

extern "C" int BertInit(ElementData* data, char* params) {
  share_params = GlobalConfig();
  strncpy(data->input_name[0], "text_input", sizeof(data->input_name[0]));
  if (params == NULL) {
    AppWarn("params is null");
    return -1;
  }
  data->queue_len = GetIntValFromFile(share_params.config_file, "img", "queue_len");
  if (data->queue_len < 0) {
    data->queue_len = 50;
  }
  if (engine != NULL) {
    return 0;
  }
  auto model = GetStrValFromJson(params, "model");
  auto vocab = GetStrValFromJson(params, "vocab");
  if (model == nullptr || vocab == nullptr) {
    AppDebug("get model/vocab failed, %s", params);
    return -1;
  }
  TokenizersInit(vocab.get());
  auto module = torch::jit::load(model.get());
  module.eval();
  engine = new BertEngine();
  engine->module = module;
  return 0;
}

extern "C" IHandle BertStart(int channel, char* params) {
  ModuleObj* obj = new ModuleObj();
  obj->id = channel;
  return obj;
}

extern "C" int BertProcess(IHandle handle, TensorData* data) {
  ModuleObj* obj = (ModuleObj* )handle;
  auto pkt = data->tensor_buf.input[0];

  char* ptr = pkt->_params.ptr;
  if (ptr == NULL) {
    printf("warning, bert, id:%d, input ptr is null\n", obj->id);
    return 0;
  }
  auto text = GetStrValFromJson(ptr, "text");
  int k = GetIntValFromJson(ptr, "topk");
  if (text == nullptr || k < 0) {
    printf("warning, bert, id:%d, get text/topk failed\n", obj->id);
    return 0;
  }

  int masked_index = 1;
  std::vector<float> input_ids;
  std::vector<float> input_mask;
  // generate input id from tokenizer
  input_ids = TokenizersEncode(text.get());
  size_t len = input_ids.size();
  for (size_t i = 0; i < len; i ++) {
    input_mask.push_back(1);
    if ((int)input_ids[i] == VOCAB_MASK_INDEX) {
      masked_index = i;
    }
  }
  std::vector<torch::jit::IValue> inputs;
  inputs.push_back(torch::from_blob(input_ids.data(), {1, (int)len}).to(torch::kInt64));
  inputs.push_back(torch::from_blob(input_mask.data(), {1, (int)len}).to(torch::kInt64));
  // inference and get output
  auto elms = engine->module.forward(inputs);
  auto dict = elms.toGenericDict();
  auto output = dict.at("logits");
  auto tensor = output.toTensor();
  at::Tensor result = std::get<1>(tensor[0][masked_index].topk(k, -1, true, true));
  // Make json output
  auto json = MakeJson(obj->id, result, ptr);
  if (json != nullptr) {
    HeadParams params = {0};
    params.frame_id = pkt->_params.frame_id;
    auto _packet = new Packet(json.get(), strlen(json.get())+1, &params);
    data->tensor_buf.output = _packet;
  }

  return 0;
}

extern "C" int BertStop(IHandle handle) {
  ModuleObj* obj = (ModuleObj* )handle;
  if (obj == NULL) {
    AppWarn("obj is null");
    return -1;
  }
  delete obj;
  return 0;
}

extern "C" int BertRelease(void) {
  if (engine != NULL) {
    delete engine;
  }
  engine = NULL;
  return 0;
}

extern "C" int DylibRegister(DLRegister** r, int& size) {
  size = 1;
  DLRegister* p = (DLRegister*)calloc(size, sizeof(DLRegister));
  strncpy(p->name, "bert", sizeof(p->name));
  p->init = "BertInit";
  p->start = "BertStart";
  p->process = "BertProcess";
  p->stop = "BertStop";
  p->release = "BertRelease";
  *r = p;
  return 0;
}

