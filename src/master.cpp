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

static auto GetLowestSlave(MasterParams* master) {
    std::shared_ptr<SlaveParam> slave = nullptr;
    master->m_slave_mtx.lock();
    for(size_t i = 0; i < master->m_slave_vec.size(); i++) {
        auto _slave = master->m_slave_vec[i];
        if(_slave->alive && _slave->load.total_load < SLAVE_LOAD_MAX) {
            if(slave == nullptr || _slave->load.total_load < slave->load.total_load) {
                slave = _slave;
            }
        }
    }
    master->m_slave_mtx.unlock();
    return slave;
}

static bool AssignObjToSlave(auto obj, MasterParams* master) {
    char url[256];
    HttpAck ack = {nullptr};

    // get lowest slave
    auto slave = GetLowestSlave(master);
    if(slave == nullptr) {
        //AppWarn("get lowest slave failed, id:%d", obj->id);
        return false;
    }
    // add obj to slave
    auto type = GetStrValFromJson(obj->params.get(), "type");
    if(type == nullptr) {
        AppWarn("get obj type failed, id:%d", obj->id);
        return false;
    }
    snprintf(url, sizeof(url), "http://%s:%d/api/obj/add/%s", 
             slave->ip, slave->rest_port, type.get());
    HttpPost(url, obj->params.get(), &ack);
    // update load
    if(ack.buf != nullptr) {
        double load = GetDoubleValFromJson(ack.buf.get(), "data", "load");
        if(load >= 0) {
            slave->load.total_load = load;
        }
        else {
            AppWarn("slave:%s, get load from ack failed, %s", slave->ip, ack.buf.get());
        }
        obj->slave = slave;
        obj->status = 1;
        AppDebug("assign obj %d to %s:%d", obj->id, slave->ip, slave->rest_port);
    }
    else {
        slave->alive = false;
        AppWarn("slave %s:%d is offline", slave->ip, slave->rest_port);
        return false;
    }

    return true;
}

static int HttpStart(auto obj, char* buf, const char* url, HttpAck* ack, MasterParams* master) {
    char _url[256];
    if(obj->slave == nullptr) {
        if(AssignObjToSlave(obj, master) != true) {
            printf("assign obj %d to slave failed\n", obj->id);
            obj->status = 0;
            return -1;
        }
    }
    if(!obj->slave->alive) {
        obj->status = 0;
        return -1;
    }
    snprintf(_url, sizeof(_url), "http://%s:%d%s", obj->slave->ip, obj->slave->rest_port, url);
    HttpPost(_url, buf, ack);
    return 0;
}

static int HttpStop(auto obj, char* buf, const char* url, 
                    HttpAck* ack, MasterParams* master, int timeout_sec = 5) {
    char _url[256];
    if(obj->slave == nullptr) {
        //printf("http stop, obj slave is null, id:%d\n", obj->id);
        return -1;
    }
    if(!obj->slave->alive) {
        return -1;
    }
    snprintf(_url, sizeof(_url), "http://%s:%d%s", obj->slave->ip, obj->slave->rest_port, url);
    HttpPost(_url, buf, ack, timeout_sec);
    return 0;
}

static int HttpToSlave(char* buf, const char* url, MasterParams* master) {
    char _url[256];
    HttpAck ack = {nullptr};
    master->m_slave_mtx.lock();
    for(size_t i = 0; i < master->m_slave_vec.size(); i++) {
        auto slave = master->m_slave_vec[i];
        if(!slave->alive) {
            continue;
        }
        snprintf(_url, sizeof(_url), "http://%s:%d%s", slave->ip, slave->rest_port, url);
        HttpPost(_url, buf, &ack);
    }
    master->m_slave_mtx.unlock();
    return 0;
}

static int SlaveSystemInit(auto slave, MasterParams* master) {
    char url[256];
    char buf[384];
    HttpAck ack = {nullptr};
    const char *sys_url = "/api/system/init";

    int timeout_sec = slave->alive ? 3 : 1;
    snprintf(url, sizeof(url), "http://%s:%d%s", slave->ip, slave->rest_port, sys_url);
    snprintf(buf, sizeof(buf), "{\"ip\":\"%s\",\"internet_ip\":\"%s\"}", slave->ip, slave->internet_ip);
    HttpPost(url, buf, &ack, timeout_sec);
    if(ack.buf == nullptr) {
        slave->alive = false;
        return -1;
    }
    slave->alive = true;

    if(master->output == nullptr) {
        return -1;
    }
    sys_url = "/api/system/set/output";
    snprintf(url, sizeof(url), "http://%s:%d%s", slave->ip, slave->rest_port, sys_url);
    HttpPost(url, master->output.get(), &ack, timeout_sec);

    return 0;
}

static std::shared_ptr<SlaveParam> GetSlave(char* ip, MasterParams* master) {
    std::shared_ptr<SlaveParam> slave = nullptr;
    master->m_slave_mtx.lock();
    for(size_t i = 0; i < master->m_slave_vec.size(); i++) {
        auto _slave = master->m_slave_vec[i];
        if(!strncmp(_slave->ip, ip, sizeof(_slave->ip))) {
            slave = _slave;
            break;
        }
    }
    master->m_slave_mtx.unlock();
    return slave;
}

static auto MAddSlave(char* buf, MasterParams* master) {
    std::shared_ptr<SlaveParam> slave = nullptr;
    auto ip = GetStrValFromJson(buf, "ip");
    auto internet_ip = GetStrValFromJson(buf, "internet_ip");
    int rest_port = GetIntValFromJson(buf, "rest_port");
    if(ip == nullptr || rest_port < 0) {
        AppWarn("get ip/port failed");
        return slave;
    }
    auto _slave = GetSlave(ip.get(), master);
    if(_slave != nullptr) {
        AppWarn("slave %s exist", ip.get());
        return slave;
    }
    slave = std::make_shared<SlaveParam>();
    slave->params = std::make_unique<char[]>(strlen(buf)+1);
    strcpy(slave->params.get(), buf);
    strncpy(slave->ip, ip.get(), sizeof(slave->ip));
    if(internet_ip != nullptr) {
        strncpy(slave->internet_ip, internet_ip.get(), sizeof(slave->internet_ip));
    }
    slave->rest_port = rest_port;
    master->m_slave_mtx.lock();
    master->m_slave_vec.push_back(slave);
    master->m_slave_mtx.unlock();
    return slave;
}

static int MDelSlave(char* ip, MasterParams* master) {
    master->m_slave_mtx.lock();
    for(auto itr = master->m_slave_vec.begin(); itr != master->m_slave_vec.end(); ++itr) {
        if(!strncmp((*itr)->ip, ip, sizeof((*itr)->ip))) {
            (*itr)->alive = false;
            master->m_slave_vec.erase(itr);
            break;
        }
    }
    master->m_slave_mtx.unlock();
    return 0;
}

static std::shared_ptr<MObjParam> GetMObj(int id, MasterParams* master) {
    std::shared_ptr<MObjParam> obj = nullptr;
    master->m_obj_mtx.lock();
    for(size_t i = 0; i < master->m_obj_vec.size(); i++) {
        auto _obj = master->m_obj_vec[i];
        if(_obj->id == id) {
            obj = _obj;
            break;
        }
    }
    master->m_obj_mtx.unlock();
    return obj;
}

static auto MAddObj(char* buf, MasterParams* master) {
    auto obj = std::make_shared<MObjParam>();
    obj->id = GetIntValFromJson(buf, "id");
    obj->params = std::make_unique<char[]>(strlen(buf)+1);
    strcpy(obj->params.get(), buf);
    master->m_obj_mtx.lock();
    master->m_obj_vec.push_back(obj);
    master->m_obj_mtx.unlock();
    return obj;
}

static int MDelObj(int id, MasterParams* master) {
    master->m_obj_mtx.lock();
    for(auto itr = master->m_obj_vec.begin(); itr != master->m_obj_vec.end(); ++itr) {
        if((*itr)->id == id) {
            master->m_obj_vec.erase(itr);
            break;
        }
    }
    master->m_obj_mtx.unlock();
    return 0;
}

void MObjParam::AddTask(char* params) {
    auto _params = std::make_shared<std::string>(params);
    m_task_mtx.lock();
    m_task_vec.push_back(_params);
    m_task_mtx.unlock();
}

void MObjParam::DelTask(char* name) {
    m_task_mtx.lock();
    for(auto itr = m_task_vec.begin(); itr != m_task_vec.end(); ++itr) {
        const char* _params = (*itr)->c_str();
        auto _name = GetStrValFromJson((char* )_params, "task");
        if(_name != nullptr && !strcmp(name, _name.get())) {
            m_task_vec.erase(itr);
            break;
        }
    }
    m_task_mtx.unlock();
}

static int SystemInitCB(char* buf, void* arg) {
    MediaServer *media = (MediaServer *)arg;
    MasterParams* master = media->GetMaster();
    auto output = GetObjBufFromJson(buf, "output", "data");
    if(output != nullptr) {
        master->output = std::make_unique<char[]>(strlen(output.get())+1);
        strcpy(master->output.get(), output.get());
    }
    return 0;
}

static void GetSlaveObj(auto slave) {
    char url[256];
    HttpAck ack = {nullptr};
    const char *_url = "/api/obj/status";
    snprintf(url, sizeof(url), "http://%s:%d%s", slave->ip, slave->rest_port, _url);
    HttpGet(url, &ack);
    if(ack.buf != nullptr) {
        int size = 0;
        auto array = GetArrayBufFromJson(ack.buf.get(), size, "data", "obj");
        if(array == nullptr || size == 0) {
            return;
        }
        for(int i = 0; i < size; i++) {
            auto arrbuf = GetBufFromArray(array.get(), i);
            if(arrbuf == NULL) {
                continue;
            }
            int id = GetIntValFromJson(arrbuf.get(), "id");
            if(id >= 0) {
                slave->obj_id_vec.push_back(id);
            }
        }
    }
}

static int SlaveCB(char* buf, void* arg) {
    MediaServer *media = (MediaServer *)arg;
    MasterParams* master = media->GetMaster();

    auto ip = GetStrValFromJson(buf, "ip");
    if(ip == nullptr) {
        return -1;
    }
    auto slave = MAddSlave(buf, master);
    if(slave == nullptr) {
        return -1;
    }

    char url[256];
    HttpAck ack = {nullptr};
    const char* sys_url = "/api/system/status";
    snprintf(url, sizeof(url), "http://%s:%d%s", slave->ip, slave->rest_port, sys_url);
    HttpGet(url, &ack);
    if(ack.buf != nullptr) {
        slave->alive = true;
        int system_init = GetIntValFromJson(ack.buf.get(), "data", "system_init");
        // when restart master and slave is running, don't init repeatly
        if(system_init == 0) {
            SlaveSystemInit(slave, master);
        }
        // get slave assigned obj(master restarted), it will be used in GetSlaveById, 
        GetSlaveObj(slave);
    }
    AppDebug("ip:%s,port:%d, alive:%d", slave->ip, slave->rest_port, slave->alive);

    return 0;
}

static auto GetSlaveById(int id, MasterParams* master) {
    std::shared_ptr<SlaveParam> slave = nullptr;
    master->m_slave_mtx.lock();
    for(size_t i = 0; i < master->m_slave_vec.size()&&slave == nullptr; i++) {
        auto _slave = master->m_slave_vec[i];
        for(size_t j = 0; j < _slave->obj_id_vec.size(); j++) {
            int _id = _slave->obj_id_vec[j];
            if(id == _id) {
                slave = _slave;
                break;
            }
        }
    }
    master->m_slave_mtx.unlock();
    return slave;
}

static int ObjCB(char* buf, void* arg) {
    MediaServer *media = (MediaServer *)arg;
    MasterParams* master = media->GetMaster();

    auto _buf = DelJsonObj(buf, "_id");
    if(_buf != nullptr) {
        buf = _buf.get();
    }
    // add obj
    auto obj = MAddObj(buf, master);
    if(obj == nullptr) {
        AppWarn("add obj failed");
        return -1;
    }
    // check if this obj is already running in someone obj,
    // avoid adding obj to slave repeatly
    obj->slave = GetSlaveById(obj->id, master);

    // add task
    int size = 0;
    auto array = GetArrayBufFromJson(buf, size, "data", "task");
    if(array == nullptr || size == 0) {
        return 0;
    }
    for(int i = 0; i < size; i++) {
        auto _params = GetBufFromArray(array.get(), i);
        if(_params == nullptr) {
            continue;
        }
        obj->AddTask(_params.get());
    }

    return 0;
}

/**********************************************************
{
  "username": "admin",
  "password": "123456"
}
**********************************************************/
static void request_login(struct evhttp_request* req, void* arg) {
    request_first_stage;
    CommonParams* params = (CommonParams* )arg;
    char* buf = (char* )params->arga;
    auto username = GetStrValFromJson(buf, "username");
    auto password = GetStrValFromJson(buf, "password");
    if(username == nullptr || password == nullptr || 
            strcmp(username.get(), "admin") != 0 || 
            strcmp(password.get(), "123456") != 0) {
        CheckErrMsg("username/password is error", (char** )params->argb);
    }
}

static void request_logout(struct evhttp_request* req, void* arg) {
    request_first_stage;
}

static void request_system_init(struct evhttp_request* req, void* arg) {
    request_first_stage;
    CommonParams* params = (CommonParams* )arg;
    Restful* rest = (Restful* )params->argc;
    MediaServer* media = rest->media;
    DbParams* db = media->GetDB();

    char tmp[512];
    const char* buf = "{}";
    snprintf(tmp, sizeof(tmp), "{\"init\":{\"flag\":1,\"data\":%s}}", buf);
    db->DBUpdate("system", tmp, "init.flag", 1);
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
    char tmp[512];
    CommonParams* params = (CommonParams* )arg;
    char* buf = (char* )params->arga;
    Restful* rest = (Restful* )params->argc;
    MediaServer* media = rest->media;
    DbParams* db = media->GetDB();
    MasterParams* master = media->GetMaster();

    master->output = std::make_unique<char[]>(strlen(buf)+1);
    strcpy(master->output.get(), buf);
    HttpToSlave(buf, "/api/system/set/output", master);
    snprintf(tmp, sizeof(tmp), "{\"output\":{\"flag\":1,\"data\":%s}}", buf);
    db->DBUpdate("system", tmp, "output.flag", 1);
}

/**********************************************************
{
    "ip":"192.168.0.100",
    "rest_port":8082,
    "internet_ip":"8.8.8.8"
}
**********************************************************/
static void request_add_slave(struct evhttp_request* req, void* arg) {
    request_first_stage;
    char err_msg[128] = {0};
    CommonParams* params = (CommonParams* )arg;
    char* buf = (char* )params->arga;
    Restful* rest = (Restful* )params->argc;
    MediaServer* media = rest->media;
    DbParams* db = media->GetDB();
    MasterParams* master = media->GetMaster();

    auto ip = GetStrValFromJson(buf, "ip");
    auto slave = MAddSlave(buf, master);
    if(slave == nullptr || ip == nullptr) {
        snprintf(err_msg, sizeof(err_msg), "add slave failed");
        goto end;
    }
    SlaveSystemInit(slave, master);
    db->DBUpdate("slave", buf, "ip", ip.get());
end:
    CheckErrMsg(err_msg, (char **)params->argb);
}

/**********************************************************
{
    "ip":"192.168.0.100"
}
**********************************************************/
static void request_del_slave(struct evhttp_request* req, void* arg) {
    request_first_stage;
    char err_msg[128] = {0};
    CommonParams* params = (CommonParams* )arg;
    char* buf = (char* )params->arga;
    Restful* rest = (Restful* )params->argc;
    MediaServer* media = rest->media;
    DbParams* db = media->GetDB();
    MasterParams* master = media->GetMaster();

    auto ip = GetStrValFromJson(buf, "ip");
    if(ip == nullptr) {
        snprintf(err_msg, sizeof(err_msg), "get ip failed");
        goto end;
    }
    MDelSlave(ip.get(), master);
    db->DBDel("slave", "ip", ip.get());
end:
    CheckErrMsg(err_msg, (char **)params->argb);
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
    char err_msg[128] = {0};
    CommonParams* params = (CommonParams* )arg;
    char* buf = (char* )params->arga;
    Restful* rest = (Restful* )params->argc;
    MediaServer* media = rest->media;
    DbParams* db = media->GetDB();
    MasterParams* master = media->GetMaster();

    int id;
    std::shared_ptr<MObjParam> obj;
    auto _buf = AddStrJson(buf, "rtsp", "type");
    if(_buf == nullptr) {
        snprintf(err_msg, sizeof(err_msg), "parse json failed");
        goto end;
    }
    buf = _buf.get();
    id = GetIntValFromJson(buf, "id");
    if(id <= 0) {
        snprintf(err_msg, sizeof(err_msg), "get id failed");
        goto end;
    }
    obj = GetMObj(id, master);
    if(obj != nullptr) {
        snprintf(err_msg, sizeof(err_msg), "obj %d exist", id);
        goto end;
    }
    if(MAddObj(buf, master) == nullptr) {
        snprintf(err_msg, sizeof(err_msg), "add obj failed");
        goto end;
    }
    db->DBUpdate("obj", buf, "id", id);
end:
    CheckErrMsg(err_msg, (char **)params->argb);
}

/**********************************************************
{
  "id":99,
} 
**********************************************************/
static void request_del_obj(struct evhttp_request* req, void* arg) {
    request_first_stage;
    char err_msg[128] = {0};
    CommonParams* params = (CommonParams* )arg;
    char* buf = (char* )params->arga;
    Restful* rest = (Restful* )params->argc;
    MediaServer* media = rest->media;
    DbParams* db = media->GetDB();
    MasterParams* master = media->GetMaster();

    HttpAck ack = {nullptr};
    std::shared_ptr<MObjParam> obj;
    int id = GetIntValFromJson(buf, "id");
    if(id < 0) {
        snprintf(err_msg, sizeof(err_msg), "get id failed");
        goto end;
    }
    obj = GetMObj(id, master);
    if(obj == nullptr) {
        AppWarn("get obj %d exist", id);
        goto end;
    }
    HttpStop(obj, buf, "/api/obj/del", &ack, master, 30);
    if(ack.buf != nullptr) {
        double load = GetDoubleValFromJson(ack.buf.get(), "data", "load");
        if(load >= 0) {
            obj->slave->load.total_load = load;
        }
    }
    MDelObj(id, master);
    db->DBDel("obj", "id", id);
end:
    CheckErrMsg(err_msg, (char **)params->argb);
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
**********************************************************/
static void request_start_task(struct evhttp_request* req, void* arg) {
    request_first_stage;
    char err_msg[128] = {0};
    CommonParams* params = (CommonParams* )arg;
    char* buf = (char* )params->arga;
    Restful* rest = (Restful* )params->argc;
    MediaServer* media = rest->media;
    DbParams* db = media->GetDB();
    MasterParams* master = media->GetMaster();

    HttpAck ack = {nullptr};
    std::shared_ptr<MObjParam> obj;
    int id = GetIntValFromJson(buf, "id");
    auto _params = GetObjBufFromJson(buf, "data");
    if(id < 0 || _params == nullptr) {
        snprintf(err_msg, sizeof(err_msg), "get id or task params failed");
        goto end;
    }
    obj = GetMObj(id, master);
    if(obj == nullptr) {
        AppWarn("get obj %d failed", id);
        goto end;
    }
    obj->AddTask(_params.get());
    HttpStart(obj, buf, "/api/task/start", &ack, master);
    db->DBUpdate("obj", "id", id, "data.task", _params.get(), "$push");
end:
    CheckErrMsg(err_msg, (char **)params->argb);
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
    char err_msg[128] = {0};
    CommonParams* params = (CommonParams* )arg;
    char* buf = (char* )params->arga;
    Restful* rest = (Restful* )params->argc;
    MediaServer* media = rest->media;
    DbParams* db = media->GetDB();
    MasterParams* master = media->GetMaster();

    HttpAck ack = {nullptr};
    std::shared_ptr<MObjParam> obj;
    int id = GetIntValFromJson(buf, "id");
    auto task = GetStrValFromJson(buf, "data", "task");
    auto _data = GetObjBufFromJson(buf, "data");
    if(id < 0 || task == nullptr || _data == nullptr) {
        snprintf(err_msg, sizeof(err_msg), "get id/task failed");
        goto end;
    }
    obj = GetMObj(id, master);
    if(obj == nullptr) {
        AppWarn("get obj %d failed", id);
        goto end;
    }
    obj->DelTask(task.get());
    HttpStop(obj, buf, "/api/task/stop", &ack, master, 30);
    db->DBUpdate("obj", "id", id, "data.task", _data.get(), "$pull");
end:
    CheckErrMsg(err_msg, (char **)params->argb);
}

static void request_task_support(struct evhttp_request* req, void* arg) {
    request_first_stage;
}

static UrlMap rest_url_map[] = {
    {"/api/system/login",       request_login},
    {"/api/system/logout",      request_logout},
    {"/api/system/init",        request_system_init},
    {"/api/system/set/output",  request_set_output},
    {"/api/system/slave/add",   request_add_slave},
    {"/api/system/slave/del",   request_del_slave},
    {"/api/obj/add/rtsp",       request_add_rtsp},
    {"/api/obj/del",            request_del_obj},
    {"/api/task/start",         request_start_task},
    {"/api/task/stop",          request_stop_task},
    {"/api/task/support",       request_task_support},
    {NULL, NULL}
};

MasterRestful::MasterRestful(MediaServer* _media)
    :Restful(_media) {
    ConfigParams* config = media->GetConfig();
    port = config->GetMRestPort();
}

UrlMap* MasterRestful::GetUrl(void) {
    return rest_url_map;
}

const char* MasterRestful::GetType(void) {
    return "master";
}

MasterParams::MasterParams(MediaServer* _media)
  : media(_media) {
    output = nullptr;
}

MasterParams::~MasterParams(void) {
}

static void ResetSlaveObj(auto slave, MasterParams* master) {
    master->m_obj_mtx.lock();
    for(size_t i = 0; i < master->m_obj_vec.size(); i++) {
        auto obj = master->m_obj_vec[i];
        if(obj->slave == slave) {
            obj->slave = nullptr;
        }
    }
    master->m_obj_mtx.unlock();
}

static void UpdateSlaveStatus(auto slave, HttpAck& ack, MasterParams* master) {
    // offline detection
    if(ack.buf == nullptr) {
        slave->offline_cnt ++;
        if(slave->offline_cnt >= 3) {
            if(slave->alive) {
                AppWarn("slave %s:%d is offline", slave->ip, slave->rest_port);
                ResetSlaveObj(slave, master);
            }
            slave->alive = false;
        }
        return;
    }
    if(!slave->alive) {
        // online detection
        AppDebug("slave %s:%d is online", slave->ip, slave->rest_port);
        SlaveSystemInit(slave, master);
    }
    else {
        // restart detection
        int system_init = GetIntValFromJson(ack.buf.get(), "data", "system_init");
        if(system_init == 0) {
            AppWarn("detected slave %s:%d restart", slave->ip, slave->rest_port);
            SlaveSystemInit(slave, master);
            ResetSlaveObj(slave, master);
        }
    }
    slave->alive = true;
    slave->offline_cnt = 0;
}

void MasterParams::SlaveThread(void) {
    char url[256];
    const char* sys_url = "/api/system/status";
    while(media->running) {
        m_slave_mtx.lock();
        for(size_t i = 0; i < m_slave_vec.size(); i++) {
            auto slave = m_slave_vec[i];
            if(slave->offline_cnt > 1 && slave->offline_cnt % 60 != 0) {
                continue;
            }
            HttpAck ack = {nullptr};
            int timeout_sec = slave->alive ? 3 : 1;
            snprintf(url, sizeof(url), "http://%s:%d%s", slave->ip, slave->rest_port, sys_url);
            HttpGet(url, &ack, timeout_sec);
            UpdateSlaveStatus(slave, ack, this);
        }
        m_slave_mtx.unlock();
        sleep(10);
    }
    AppDebug("run ok");
}

static void UpdateObjTask(auto obj, MasterParams* master) {
    if(obj->m_task_vec.size() == 0) {
        return;
    }
    if(obj->slave != nullptr && obj->slave->alive) {
        return;
    }
    if(AssignObjToSlave(obj, master) != true) {
        //app_warning("assign obj %d to slave failed", obj->id);
        obj->status = 0;
        return;
    }

    char buf[512];
    HttpAck ack = {nullptr};
    obj->m_task_mtx.lock();
    for(size_t i = 0; i < obj->m_task_vec.size(); i++) {
        auto task = obj->m_task_vec[i];
        snprintf(buf, sizeof(buf), "{\"id\":%d,\"data\":%s}", obj->id, task->c_str());
        HttpStart(obj, buf, "/api/task/start", &ack, master);
    }
    obj->m_task_mtx.unlock();
}

void MasterParams::ObjThread(void) {
    while(media->running) {
        m_obj_mtx.lock();
        for(size_t i = 0; i < m_obj_vec.size(); i++) {
            auto obj = m_obj_vec[i];
            UpdateObjTask(obj, this);
        }
        m_obj_mtx.unlock();
        sleep(3);
    }
    AppDebug("run ok");
}

void MasterParams::Start(void) {
    DbParams* db = media->GetDB();
    sleep(3); // wait slave restful start ok
    db->DbTraverse("system", media, SystemInitCB);
    db->DbTraverse("slave", media, SlaveCB);
    db->DbTraverse("obj", media, ObjCB);
    std::thread t1(&MasterParams::SlaveThread, this);
    t1.detach();
    std::thread t2(&MasterParams::ObjThread, this);
    t2.detach();
    AppDebug("obj total:%ld", m_obj_vec.size());
    // start master restful
    MasterRestful* rest = new MasterRestful(media);
    rest->Start();
}

