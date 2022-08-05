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
#include <thread>
#include <unistd.h>
#include <fcntl.h>
#include <event.h>
#include <evhttp.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/thread.h>
#include "tensor.h"
#include "config.h"
#include "share.h"
#include "log.h"

typedef struct {
    int id;
    int frame_id;
    std::mutex mtx;
    std::condition_variable condition;
    std::queue<Packet*> _queue;
    int queue_len_max;
    std::thread* t;
    int port;
    int thread_num;
    int timeout_sec;
    int running;
} HttpServer;

typedef struct {
    char* body;
    int size;
    HttpServer* http;
} HttpParams;

typedef struct {
    char* buf;
    int size;
    std::unique_ptr<char[]> user_data;
} HttpFilee;

static NginxParams nginx;
static char local_ip[128] = {0};
static int SendHttpReply2(struct evhttp_request* req, int code, const char* url) {
    struct evbuffer* evb;
    evb = evbuffer_new();
    if(url != NULL) {
        evbuffer_add_printf(evb, "{\"code\":0,\"msg\":\"success\",\"data\":{\"img_path\":\"%s\"}}", url);
    }
    else {
        evbuffer_add_printf(evb, "{\"code\":0,\"msg\":\"success\",\"data\":{}}");
    }
    evkeyvalq* outhead = evhttp_request_get_output_headers(req);
    evhttp_add_header(outhead, "Access-Control-Allow-Origin", "*");
    evhttp_add_header(outhead, "Access-Control-Allow-Credentials", "true");
    evhttp_add_header(outhead, "Access-Control-Allow-Methods", "*");
    evhttp_send_reply(req, code, "OK", evb);
    evbuffer_free(evb);
    return 0;
}

extern "C" int HttpInit(ElementData* data, char* params) {
    data->queue_len = GetIntValFromFile(CONFIG_FILE, "img", "queue_len");
    if(data->queue_len < 0) {
        data->queue_len = 50;
    }
    if(strlen(local_ip) > 0) {
        return 0;
    }
    auto localhost = GetStrValFromFile(CONFIG_FILE, "system", "localhost");
    if(localhost != nullptr) {
        strncpy(local_ip, localhost.get(), sizeof(local_ip));
    }
    else {
        GetLocalIp(local_ip);
    }
    NginxInit(nginx);
    return 0;
}

static int HttpBindSocket(int port) {
    int r, fd;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0) {
        AppWarn("socket failed");
        return -1;
    }

    int one = 1;
    int backlog = 10240;
    r = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char* )&one, sizeof(int));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    r = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
    if(r < 0) {
        AppWarn("bind failed");
        return -1;
    }
    r = listen(fd, backlog);
    if(r < 0) {
        AppWarn("listen failed");
        return -1;
    }
    int flags;
    if((flags = fcntl(fd, F_GETFL, 0)) < 0
        || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        AppWarn("fset failed");
        return -1;
    }

    return fd;
}

static int GetNextLine(char* input, char* line, int size) {
    int len = 0;
    int flag = 0;
    size -= 1;
    for(int i = 0; i < size; i ++) {
        if(input[i] == '\0') {
            break;
        }
        if(input[i] == '\r' || input[i] == '\n') {
            flag = 1;
        }
        if(flag && input[i] != '\r' && input[i] != '\n') {
            break;
        }
        line[i] = input[i];
        len ++;
    }
    line[len] = '\0';
    return len;
}

static char* GetStrLine(char* input, char* boundary) {
    char* line = NULL;
    char buf[256];
    int cnt = 100;
    do {
        int len = GetNextLine(input, buf, sizeof(buf));
        if(len == 0) {
            break;
        }
        char* p = strstr(buf, boundary);
        if(p != NULL) {
            line = input;
            break;
        }
        input += len;
    } while(cnt --);
    return line;
}

static int GetFileHead(char* body, char* boundary, HttpFilee& http_file) {
    char buf[256];
    char head[512] = {0};
    int form_data_num = 2;

    do {
        char* p = strstr(body, boundary);
        if(p == NULL) {
            break;
        }
        int len = GetNextLine(p, buf, sizeof(buf));
        if(len == 0) {
            break;
        }
        p += len;
        len = GetNextLine(p, buf, sizeof(buf));
        if(len == 0) {
            break;
        }
        p += len;
        if(strstr(buf, "Content-Disposition") == NULL || 
                strstr(buf, "form-data") == NULL || 
                strstr(buf, "name=\"head\"") == NULL) {
            body = p;
            continue;
        }
        char* p1 = GetStrLine(p, boundary);
        if(p1 == NULL) {
            break;
        }
        len = p1 - p;
        if(len >= (int)sizeof(head)) {
            printf("warning, get file head failed, %d>%ld\n", len, sizeof(head));
            break;
        }
        memcpy(head, p, len);
        break;
    } while(--form_data_num);
    if(*head == 0) {
        return -1;
    }
    http_file.size = GetIntValFromJson(head, "filesize");
    if(http_file.size < 0) {
        printf("warning, get file size failed, %s", head);
        return -1;
    }
    http_file.user_data = GetObjBufFromJson(head, "userdata");

    return 0;
}
    
static int GetFile(char* body, char* boundary, HttpFilee& http_file) {
    char buf[256];
    int form_data_num = 2;

    do {
        char* p = strstr(body, boundary);
        if(p == NULL) {
            break;
        }
        int len = GetNextLine(p, buf, sizeof(buf));
        if(len == 0) {
            break;
        }
        p += len;
        len = GetNextLine(p, buf, sizeof(buf));
        if(len == 0) {
            break;
        }
        p += len;
        if(strstr(buf, "Content-Disposition") == NULL || 
                strstr(buf, "form-data") == NULL || 
                strstr(buf, "name=\"file\"") == NULL) {
            body = p;
            continue;
        }
        char* flag = strstr(p, "\r\n\r\n");
        if(flag == NULL) {
            printf("warning, get file start flag failed\n");
            break;
        }
        http_file.buf = flag + 4;
        break;
    } while(--form_data_num);

    return http_file.buf == NULL;
}

static int GetHttpFile(char* body, char* boundary, HttpFilee& http_file) {
    if(GetFileHead(body, boundary, http_file) != 0) {
        printf("warning, get file head failed\n");
        return -1;
    }
    if(GetFile(body, boundary, http_file) != 0) {
        printf("warning, get http file failed\n");
        return -1;
    }
    //WriteFile("test.bin", http_file.buf, http_file.size, "wb");

    return 0;
}

static void Copy2Queue(HttpFilee& http_file, HttpServer* http, char url[URL_LEN]) {
    HeadParams params = {0};
    if(http_file.user_data != nullptr) {
        params.ptr_size = strlen(http_file.user_data.get()) + 1;
        params.ptr = new char[params.ptr_size + URL_LEN];
        strcpy(params.ptr, http_file.user_data.get());
        strncpy(params.ptr + params.ptr_size, url, URL_LEN);
    }
    params.frame_id = ++http->frame_id;
    std::unique_lock<std::mutex> lock(http->mtx);
    if(http->_queue.size() < (size_t)http->queue_len_max) {
        auto _packet = new Packet(http_file.buf, http_file.size, &params);
        http->_queue.push(_packet);
    }
    else {
        printf("warning,http,id:%d, put to queue failed, quelen:%ld\n", 
                http->id, http->_queue.size());
    }
    http->condition.notify_one();
}

static void SaveFile(HttpFilee& http_file, HttpServer* http, char url[URL_LEN]) {
    char date[32];
    struct tm _time;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &_time);
    snprintf(date, sizeof(date), "%d%02d%02d", _time.tm_year + 1900, _time.tm_mon + 1, _time.tm_mday);
    snprintf(url, URL_LEN, "%s/image/%s/%d/%ld_%ld.jpg", 
            nginx.workdir, date, http->id, tv.tv_sec, tv.tv_usec);
    WriteFile(url, http_file.buf, http_file.size, "wb");
    snprintf(url, URL_LEN, "http://%s:%d/image/%s/%d/%ld_%ld.jpg", 
            local_ip, nginx.http_port, date, http->id, tv.tv_sec, tv.tv_usec);
}

static void HttpRequest(struct evhttp_request* req, void* arg) {
    int n, content_length;
    int code = HTTP_INTERNAL;
    struct evbuffer* input_buf;
    const char* content_type, * _content_length;
    HttpParams* http_params = (HttpParams* )arg;
    char boundary[256], url[URL_LEN];
    HttpFilee http_file = {0};

    int cmd = evhttp_request_get_command(req); 
    if(cmd != EVHTTP_REQ_POST) {
        printf("warning, http req cmd is not post, %d\n", cmd);
        goto end;
    }
    // multipart/form-data; boundary=------------------------9bb3818dac2868fa
    content_type = evhttp_find_header(evhttp_request_get_input_headers(req), "Content-Type");
    if(content_type == NULL || strlen(content_type) >= sizeof(boundary) 
            || strstr(content_type, "multipart/form-data") == NULL) {
        printf("warning, get Content-Type or multipart/form-data failed\n");
        goto end;
    }
    n = sscanf(content_type, "%*s%*[^=]=%s", boundary);
    if(n != 1) {
        printf("warning, get boundary failed\n");
        goto end;
    }
    _content_length = evhttp_find_header(evhttp_request_get_input_headers(req), "Content-Length");
    if(_content_length == NULL) {
        printf("warning, get Content-Length failed\n");
        goto end;
    }
    content_length = atoi(_content_length);
    if(content_length > http_params->size) {
        if(http_params->body != NULL) {
            free(http_params->body);
        }
        http_params->size = (content_length>>10<<10) + 1024;
        http_params->body = (char* )malloc(http_params->size);
        if(http_params->body == NULL) {
            AppError("malloc %d failed", http_params->size);
            goto end;
        }
    }

    n = -1;
    input_buf = evhttp_request_get_input_buffer(req);
    while(evbuffer_get_length(input_buf)) {
        n = evbuffer_remove(input_buf, http_params->body, http_params->size);
    }
    if(n < 0) {
        printf("warning, get input buffer failed\n");
        goto end;
    }
    if(GetHttpFile(http_params->body, boundary, http_file) != 0) {
        printf("warning, get http file failed, Content-Length:%d\n", content_length);
        goto end;
    }
    SaveFile(http_file, http_params->http, url);
    Copy2Queue(http_file, http_params->http, url);
    code = HTTP_OK;
end:
    SendHttpReply2(req, code, url);
}

static void *DispatchThread(void* arg) {
  event_base_dispatch((struct event_base* )arg);
  return NULL;
}

static int StartServer(HttpServer* http) {
    const char* url[] = {
        "/file-resnet50",
    };
    int fd = HttpBindSocket(http->port);
    if(fd < 0) {
        AppError("bind socket failed, port:%d", http->port);
        return -1;
    }
    pthread_t pid[http->thread_num];
    evthread_use_pthreads();
    HttpParams* http_params = (HttpParams* )calloc(http->thread_num, sizeof(HttpParams));
    for(int i = 0; i < http->thread_num; i++) {
        struct event_base* base = event_init();
        if(base == NULL) {
            AppError("event base init failed, port:%d", http->port);
            return -1;
        }
        struct evhttp* httpd = evhttp_new(base);
        if(httpd == NULL) {
            AppError("event new http failed, port:%d", http->port);
            return -1;
        }
        int r = evhttp_accept_socket(httpd, fd);
        if(r != 0) {
            AppError("event accept socket failed, port:%d", http->port);
            return -1;
        }
        http_params[i].http = http;
        for(size_t j = 0; j < sizeof(url)/sizeof(const char*); j ++) {
            evhttp_set_cb(httpd, url[j], HttpRequest, http_params + i);
        }
        if(pthread_create(&pid[i], NULL, DispatchThread, base) != 0) {
            AppError("create dispatch thread failed");
        }
        else {
            pthread_detach(pid[i]);
        }
    }
    AppDebug("threads:%d, http://ip:%d/file listen ...", http->thread_num, http->port);

    return 0;
}

extern "C" IHandle HttpStart(int channel, char* params) {
    if(params == NULL) {
        AppWarn("id:%d, params is null", channel);
        return NULL;
    }
    HttpServer* http = new HttpServer();
    http->id = channel;
    http->queue_len_max = GetIntValFromFile(CONFIG_FILE, "img", "queue_len");
    http->timeout_sec = GetIntValFromFile(CONFIG_FILE, "system", "task_timeout_sec");
    if(http->timeout_sec < 0) {
        http->timeout_sec = 10;
    }
    else {
        http->timeout_sec /= 3;
    }
    http->queue_len_max = http->queue_len_max > 0 ? http->queue_len_max : 50;
    http->port = GetIntValFromJson(params, "port");
    http->thread_num = GetIntValFromJson(params, "threads");
    if(http->port < 0 || http->thread_num < 0) {
        AppWarn("id:%d, get port/threads from params failed", channel);
        return NULL;
    }
    http->running = 1;
    StartServer(http);
    return http;
}

extern "C" int HttpProcess(IHandle handle, TensorData* data) {
    HttpServer* http = (HttpServer* )handle;
    std::unique_lock<std::mutex> lock(http->mtx);
    http->condition.wait_for(lock, std::chrono::seconds(http->timeout_sec));
    //if(!http->running) {
    //    return -1;
    //}
    if(!http->_queue.empty()) {
        data->tensor_buf.output = http->_queue.front();
        http->_queue.pop();
    }
    return 0;
}

// As a system service, there is no need to stop
extern "C" int HttpStop(IHandle handle) {
#if 0
    HttpServer* http = (HttpServer*)handle;
    if(http == NULL) {
        AppWarn("http is null");
        return -1;
    }
    http->running = 0;
    if(http->t != nullptr) {
        if(http->t->joinable()) {
            http->t->join();
        }
        delete http->t;
        http->t = nullptr;
    }
    std::unique_lock<std::mutex> lock(http->mtx);
    while(!http->_queue.empty()) {
        Packet* pkt = http->_queue.front();
        http->_queue.pop();
        if(pkt != nullptr) {
            delete pkt;
        }
    }
    lock.unlock();
    delete http;
#endif
    return 0;
}

extern "C" int HttpRelease(void) {
    return 0;
}

extern "C" int DylibRegister(DLRegister** r, int& size) {
    size = 1; // reserved
    DLRegister* p = (DLRegister*)calloc(size, sizeof(DLRegister));
    strncpy(p->name, "httpfile", sizeof(p->name));
    p->init = "HttpInit";
    p->start = "HttpStart";
    p->process = "HttpProcess";
    p->stop = "HttpStop";
    p->release = "HttpRelease";
    *r = p;
    return 0;
}

