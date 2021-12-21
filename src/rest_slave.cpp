/******************************************************************************
 * Copyright (C) 2021 aistream <aistream@yeah.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#include "stream.h"
#include "rest.h"
#include "config.h"
#include "log.h"
#include "rtsp.h"
#include "gat1400.h"

static void ReleaseObjPtr(Object* obj) {
    printf("enter %s\n", __func__);
    delete obj;
}

static void ReleaseTaskPtr(TaskParams* task) {
    printf("enter %s\n", __func__);
    delete task;
}

static void request_login(struct evhttp_request* req, void* arg) {
    request_first_stage;
}

static void request_logout(struct evhttp_request* req, void* arg) {
    request_first_stage;
}

static void request_system_init(struct evhttp_request* req, void* arg) {
    request_first_stage;
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
    CommonParams *params = (CommonParams *)arg;
    Restful* rest = (Restful* )params->argc;
    char* buf = (char* )params->arga;
    MediaServer* media = rest->media;
    ObjParams* obj_params = media->GetObjParams();
    int id = GetIntValFromJson(buf, "id");
    int tcp_enable = GetIntValFromJson(buf, "data", "tcp_enable");
    auto url = GetStrValFromJson(buf, "data", "url");
    tcp_enable = tcp_enable < 0 ? 0 : tcp_enable;
    if(id < 0 || url == nullptr) {
        AppWarn("get id or url failed, %s", buf);
        return;
    }
    std::shared_ptr<Rtsp> obj(new Rtsp(media), ReleaseObjPtr);
    obj->SetId(id);
    obj->SetTcpEnable(tcp_enable);
    obj->SetRtspUrl(url.get());
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
    CommonParams *params = (CommonParams *)arg;
    Restful* rest = (Restful* )params->argc;
    char* buf = (char* )params->arga;
    MediaServer* media = rest->media;
    ObjParams* obj_params = media->GetObjParams();
    int id = GetIntValFromJson(buf, "id");
    if(id < 0) {
        AppWarn("get id failed, %s", buf);
        return;
    }
    std::shared_ptr<Gat1400> obj(new Gat1400(media), ReleaseObjPtr);
    obj->SetId(id);
    obj_params->Put2ObjQue(obj);
}

static void request_add_file(struct evhttp_request* req, void* arg) {
    request_first_stage;
}

/**********************************************************
{
  "id":99,
} 
**********************************************************/
static void request_del_obj(struct evhttp_request* req, void* arg) {
    request_first_stage;
    CommonParams *params = (CommonParams *)arg;
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
    "task":"yolov3"
  }
} 
**********************************************************/
static void request_start_task(struct evhttp_request* req, void* arg) {
    request_first_stage;
    CommonParams *params = (CommonParams *)arg;
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
    std::shared_ptr<TaskParams> task(new TaskParams, ReleaseTaskPtr);
    task->SetTaskName(task_name.get());
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
    CommonParams *params = (CommonParams *)arg;
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
    {"/api/obj/add/rtsp",       request_add_rtsp},
    {"/api/obj/add/rtmp",       request_add_rtmp},
    {"/api/obj/add/gb28181",    request_add_gb28181},
    {"/api/obj/add/gat1400",    request_add_gat1400},
    {"/api/obj/add/file",       request_add_file},
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


