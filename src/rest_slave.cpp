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
#include "gat1400.h"

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

// http get
static void request_system_alive(struct evhttp_request* req, void* arg) {
    request_first_stage;
    CommonParams* params = (CommonParams* )arg;
    char** ppbody = (char **)params->argb;
    Restful* rest = (Restful* )params->argc;
    MediaServer* media = rest->media;
    *ppbody = (char *)malloc(256);
    // master can detect slave restart status from system_init
    snprintf(*ppbody, 256, "{\"code\":0,\"msg\":\"success\",\"data\":{\"system_init\":%d}}", media->system_init);
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
    MediaServer* media = rest->media;
    ObjParams* obj_params = media->GetObjParams();
    int id = GetIntValFromJson(buf, "id");
    auto url = GetStrValFromJson(buf, "data", "url");
    //int tcp_enable = GetIntValFromJson(buf, "data", "tcp_enable");
    //tcp_enable = tcp_enable < 0 ? 0 : tcp_enable;
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
    //obj->SetTcpEnable(tcp_enable);
    //obj->SetRtspUrl(url.get());
    obj->SetParams(buf);
    obj_params->Put2ObjQue(obj);
}

static void request_add_rtmp(struct evhttp_request* req, void* arg) {
    request_first_stage;
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
        "preview":{"enable":0},
        "rabbitmq":{"enable":1,"host":"10.0.0.10","port":5672}
    }
  }
} 
Note:
 -- "params" is private data, it isn't necessary, and doesn't has any standard
 -- Every element default enable is 1
 -- In pipeline config, rabbitmq element also has it's default params
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

static UrlMap rest_url_map[] = {
    {"/api/system/login",       request_login},
    {"/api/system/logout",      request_logout},
    {"/api/system/init",        request_system_init},
    {"/api/system/alive",       request_system_alive},
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
    {"/api/task/support",       request_task_support},
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


