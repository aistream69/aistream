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
#include "config.h"
#include "log.h"
#include "rtsp.h"
#include "rtmp.h"
#include "gat1400.h"
#include "cJSON.h"

static void request_login(struct evhttp_request* req, void* arg) {
    request_first_stage;
}

static void request_logout(struct evhttp_request* req, void* arg) {
    request_first_stage;
}

static void request_system_init(struct evhttp_request* req, void* arg) {
    request_first_stage;
    CommonParams* params = (CommonParams* )arg;
    Restful* rest = (Restful* )params->argc;
    MediaServer* media = rest->media;
    media->system_init = 1;
}

/**********************************************************
{
    "type": "mq",
    "data": {
        "host": "192.168.0.10",
        "port": 5672,
        "username": "guest",
        "password": "guest",
        "exchange": "amq.direct",
        "routingkey": ""
    }
}
**********************************************************/
static void request_set_output(struct evhttp_request* req, void* arg) {
    request_first_stage;
    CommonParams* params = (CommonParams* )arg;
    char* buf = (char* )params->arga;
    Restful* rest = (Restful* )params->argc;
    MediaServer* media = rest->media;
    ConfigParams* config = media->GetConfig();
    auto _params = GetObjBufFromJson(buf, "data");
    if(_params != nullptr) {
        config->SetOutput(_params.get());
    }
}

/**********************************************************
{
  "id":99,
  "data":{
    "tcp_enable":0,
    "url":"rtsp://127.0.0.1:8554/test.264"
  }
} 
**********************************************************/
static void request_add_rtsp(struct evhttp_request* req, void* arg) {
    request_first_stage;
    CommonParams* params = (CommonParams* )arg;
    Restful* rest = (Restful* )params->argc;
    char* buf = (char* )params->arga;
    char **ppbody = (char **)params->argb;
    MediaServer* media = rest->media;
    ObjParams* obj_params = media->GetObjParams();
    ConfigParams* config = media->GetConfig();
    int id = GetIntValFromJson(buf, "id");
    auto url = GetStrValFromJson(buf, "data", "url");
    if(id < 0 || url == nullptr) {
        AppWarn("get id or url failed, %s", buf);
        return;
    }
    auto _obj = obj_params->GetObj(id);
    if(_obj != nullptr) {
        AppWarn("obj %d already exist", id);
        return;
    }
    auto obj = std::make_shared<Rtsp>(media);
    obj->SetId(id);
    obj->SetParams(buf);
    obj_params->Put2ObjQue(obj);
    // calc load
    *ppbody = (char *)malloc(ACK_BODY_LEN);
    float load = (float)obj_params->GetObjNum()/config->GetObjMax()*100;
    snprintf(*ppbody, ACK_BODY_LEN, "{\"code\":0,\"msg\":\"success\",\"data\":{\"load\":%f}}", load);
}

/**********************************************************
{
  "id":99,
  "data":{
    "url":"rtmp://127.0.0.1:1935/myapp/stream99"
  }
} 
**********************************************************/
static void request_add_rtmp(struct evhttp_request* req, void* arg) {
    request_first_stage;
    CommonParams* params = (CommonParams* )arg;
    Restful* rest = (Restful* )params->argc;
    char* buf = (char* )params->arga;
    char **ppbody = (char **)params->argb;
    MediaServer* media = rest->media;
    ObjParams* obj_params = media->GetObjParams();
    ConfigParams* config = media->GetConfig();
    int id = GetIntValFromJson(buf, "id");
    auto url = GetStrValFromJson(buf, "data", "url");
    if(id < 0 || url == nullptr) {
        AppWarn("get id or url failed, %s", buf);
        return;
    }
    auto _obj = obj_params->GetObj(id);
    if(_obj != nullptr) {
        AppWarn("obj %d already exist", id);
        return;
    }
    auto obj = std::make_shared<Rtmp>(media);
    obj->SetId(id);
    obj->SetParams(buf);
    obj_params->Put2ObjQue(obj);
    // calc load
    *ppbody = (char *)malloc(ACK_BODY_LEN);
    float load = (float)obj_params->GetObjNum()/config->GetObjMax()*100;
    snprintf(*ppbody, ACK_BODY_LEN, "{\"code\":0,\"msg\":\"success\",\"data\":{\"load\":%f}}", load);
}

static void request_add_gb28181(struct evhttp_request* req, void* arg) {
    request_first_stage;
}

static void request_add_gat1400(struct evhttp_request* req, void* arg) {
    request_first_stage;
    CommonParams* params = (CommonParams* )arg;
    Restful* rest = (Restful* )params->argc;
    char* buf = (char* )params->arga;
    MediaServer* media = rest->media;
    ObjParams* obj_params = media->GetObjParams();
    int id = GetIntValFromJson(buf, "id");
    if(id < 0) {
        AppWarn("get id failed, %s", buf);
        return;
    }
    auto _obj = obj_params->GetObj(id);
    if(_obj != nullptr) {
        AppWarn("obj %d already exist", id);
        return;
    }
    auto obj = std::make_shared<Gat1400>(media);
    obj->SetId(id);
    obj->SetParams(buf);
    obj_params->Put2ObjQue(obj);
}

static void request_add_file(struct evhttp_request* req, void* arg) {
    request_first_stage;
}

/**********************************************************
http/ws:
{
  "id":99,
  "data":{
    "port":8098,
    "token":"xxxxxx"
  }
} 
Note:
 -- "token" also can be from login, the difference is who generate it
**********************************************************/
static void request_add_http(struct evhttp_request* req, void* arg) {
    request_first_stage;
}

static void request_add_ws(struct evhttp_request* req, void* arg) {
    request_first_stage;
}

/**********************************************************
{
  "id":99,
} 
**********************************************************/
static void request_del_obj(struct evhttp_request* req, void* arg) {
    request_first_stage;
    CommonParams* params = (CommonParams* )arg;
    Restful* rest = (Restful* )params->argc;
    char* buf = (char* )params->arga;
    MediaServer* media = rest->media;
    ObjParams* obj_params = media->GetObjParams();
    int id = GetIntValFromJson(buf, "id");
    if(id < 0) {
        AppWarn("get id failed, %s", buf);
        return;
    }
    obj_params->DelFromObjQue(id);
}

/**********************************************************
{
  "id":99,
  "data":{
    "task":"yolov3",
    "params":{
        "preview": "hls"/"http-flv"/"0",
        "record": "mp4"/"h264"/"0"
    }
  }
} 
Note:
 -- "params" is private data, it isn't necessary, and doesn't has any standard
 -- All params appeared in restful have dynamic attribute
**********************************************************/
static void request_start_task(struct evhttp_request* req, void* arg) {
    request_first_stage;
    CommonParams* params = (CommonParams* )arg;
    Restful* rest = (Restful* )params->argc;
    char* buf = (char* )params->arga;
    MediaServer* media = rest->media;
    ObjParams* obj_params = media->GetObjParams();
    int id = GetIntValFromJson(buf, "id");
    auto task_name = GetStrValFromJson(buf, "data", "task");
    auto obj = obj_params->GetObj(id);
    if(id < 0 || task_name == nullptr || obj == nullptr) {
        AppWarn("get obj task failed, %s", buf);
        return;
    }
    auto _task = obj->GetTask(task_name.get());
    if(_task != nullptr) {
        AppWarn("obj %d task %s already exist", id, task_name.get());
        return;
    }
    auto task = std::make_shared<TaskParams>(obj);
    task->SetTaskName(task_name.get());
    auto _params = GetObjBufFromJson(buf, "data", "params");
    if(_params != nullptr) {
        task->SetParams(_params.get());
    }
    obj->Put2TaskQue(task);
}

/**********************************************************
{
  "id":99,
  "data":{
    "task":"yolov3"
  }
} 
**********************************************************/
static void request_stop_task(struct evhttp_request* req, void* arg) {
    request_first_stage;
    CommonParams* params = (CommonParams* )arg;
    Restful* rest = (Restful* )params->argc;
    char* buf = (char* )params->arga;
    MediaServer* media = rest->media;
    ObjParams* obj_params = media->GetObjParams();
    int id = GetIntValFromJson(buf, "id");
    auto task_name = GetStrValFromJson(buf, "data", "task");
    auto obj = obj_params->GetObj(id);
    if(id < 0 || task_name == nullptr || obj == nullptr) {
        AppWarn("get obj task failed, %s", buf);
        return;
    }
    obj->DelFromTaskQue(task_name.get());
}

static void request_task_support(struct evhttp_request* req, void* arg) {
    request_first_stage;
}

static void request_system_status(struct evhttp_request* req, void* arg) {
    request_first_stage;
    CommonParams* params = (CommonParams* )arg;
    char** ppbody = (char **)params->argb;
    Restful* rest = (Restful* )params->argc;
    MediaServer* media = rest->media;
    *ppbody = (char *)malloc(ACK_BODY_LEN);
    // master detect slave restart status from system_init
    snprintf(*ppbody, ACK_BODY_LEN, "{\"code\":0,\"msg\":\"success\","
             "\"data\":{\"system_init\":%d}}", media->system_init);
}

static int ObjStatus(std::shared_ptr<Object> obj, void* arg) {
    cJSON *fld;
    cJSON *data_root = (cJSON *)arg;
    cJSON_AddItemToArray(data_root, fld = cJSON_CreateObject());
    cJSON_AddNumberToObject(fld, "id", obj->GetId());
    cJSON_AddNumberToObject(fld, "status", 1);
    return 0;
}

static void request_obj_status(struct evhttp_request* req, void* arg) {
    request_first_stage;
    CommonParams* params = (CommonParams* )arg;
    char** ppbody = (char **)params->argb;
    Restful* rest = (Restful* )params->argc;
    MediaServer* media = rest->media;
    ObjParams* obj_params = media->GetObjParams();

    cJSON* root = cJSON_CreateObject();
    cJSON* data_root = cJSON_CreateObject();
    cJSON* obj_root = cJSON_CreateArray();
    cJSON_AddStringToObject(root, "code", "0");
    cJSON_AddStringToObject(root, "msg", "success");
    cJSON_AddItemToObject(root, "data", data_root);
    cJSON_AddItemToObject(data_root, "obj", obj_root);
    obj_params->TraverseObjQue(obj_root, ObjStatus);
    *ppbody = cJSON_Print(root);
    cJSON_Delete(root);
}

static UrlMap rest_url_map[] = {
    // HTTP POST
    {"/api/system/login",       request_login},
    {"/api/system/logout",      request_logout},
    {"/api/system/init",        request_system_init},
    {"/api/system/set/output",  request_set_output},
    {"/api/obj/add/rtsp",       request_add_rtsp},
    {"/api/obj/add/rtmp",       request_add_rtmp},
    {"/api/obj/add/gb28181",    request_add_gb28181},
    {"/api/obj/add/gat1400",    request_add_gat1400},
    {"/api/obj/add/file",       request_add_file},
    {"/api/obj/add/http",       request_add_http},
    {"/api/obj/add/ws",         request_add_ws},
    {"/api/obj/del",            request_del_obj},
    {"/api/task/start",         request_start_task},
    {"/api/task/stop",          request_stop_task},
    // HTTP GET
    {"/api/task/support",       request_task_support},
    {"/api/system/status",      request_system_status},
    {"/api/obj/status",         request_obj_status},
    {NULL, NULL}
};

SlaveRestful::SlaveRestful(MediaServer* _media)
    :Restful(_media) {
    ConfigParams* config = media->GetConfig();
    port = config->GetSRestPort();
}

UrlMap* SlaveRestful::GetUrl(void) {
    return rest_url_map;
}

const char* SlaveRestful::GetType(void) {
    return "slave";
}

