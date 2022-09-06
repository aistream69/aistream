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
#include <thread>

int SendHttpReply(struct evhttp_request* req, int code, const char* buf) {
    struct evbuffer* evb;
    evb = evbuffer_new();
    if(buf != NULL) {
        evbuffer_add_printf(evb, "%s", buf);
    }
    else {
        evbuffer_add_printf(evb, "{\"code\":0,\"msg\":\"success\",\"data\":{}}");
    }
    evhttp_send_reply(req, code, "OK", evb);
    evbuffer_free(evb);
    return 0;
}

static void CloseCallback(struct evhttp_connection* connection, void* arg) {
    if(arg != NULL) {
        event_base_loopexit((struct event_base* )arg, NULL);
    }
}

static void ReadCallback(struct evhttp_request* req, void* arg) {
    ev_ssize_t len;
    unsigned char* buf;
    struct evbuffer* evbuf;
    HttpAck* ack = (HttpAck* )arg;
    struct event_base* base = (struct event_base* )ack->arg;

    if(req == NULL) {
        printf("req is null\n");
        return ;
    }
    evbuf = evhttp_request_get_input_buffer(req);
    len = evbuffer_get_length(evbuf);
    buf = evbuffer_pullup(evbuf, len);
    if(buf != NULL) {
        ack->buf = std::make_unique<char[]>(len+1);
        memcpy(ack->buf.get(), buf, len);
        ack->buf.get()[len] = 0;
    }
    evbuffer_drain(evbuf, len);
    event_base_loopexit(base, NULL);
}

static void ErrCallback(enum evhttp_request_error error, void* arg) {
    HttpAck* ack = (HttpAck* )arg;
    struct event_base* base = NULL;
    if(ack != NULL) {
        base = (struct event_base* )ack->arg;
        if(base) {
            event_base_loopexit(base, NULL);
        }
    }
}

static int HttpClient(enum evhttp_cmd_type cmd, const char* url, 
                      char* data, HttpAck* ack, int timeout_sec) {
    char buf[64], req_url[512];
    int ret = -1, len = 0, port;
    const char *host, *path, *query;
    struct timeval timeout;
    struct event_base* base = NULL;
    struct evdns_base* dnsbase =NULL;
    struct evhttp_uri* http_uri = NULL;
    struct evhttp_connection* evcon = NULL;
    struct evhttp_request* request = NULL;
    struct evkeyvalq* output_headers = NULL;
    struct evbuffer* output_buffer = NULL;

    http_uri = evhttp_uri_parse(url);
    if (NULL == http_uri) {
        printf("parse url failed, url:%s\n", url);
        goto end;
    }
    host = evhttp_uri_get_host(http_uri);
    if (NULL == host) {
        printf("parse host failed, url:%s\n", url);
        goto end;
    }
    port = evhttp_uri_get_port(http_uri);
    if (port == -1) {
        printf("parse port failed, url:%s\n", url);
        goto end;
    }
    path = evhttp_uri_get_path(http_uri);
    if(path == NULL) {
        printf("get path failed, url:%s\n", url);
        goto end;
    }
    if(strlen(path) == 0) {
        path = "/";
    }
    query = evhttp_uri_get_query(http_uri);
    if(NULL == query) {
        snprintf(req_url, sizeof(req_url) - 1, "%s", path);
    } else {
        snprintf(req_url, sizeof(req_url) - 1, "%s?%s", path, query);
    }

    base = event_base_new();
    if (NULL == base) {
        printf("create event base failed, url:%s\n", url);
        goto end;
    }
    dnsbase = evdns_base_new(base, EVDNS_BASE_INITIALIZE_NAMESERVERS);
    evcon = evhttp_connection_base_new(base, dnsbase, host, port);
    if (NULL == evcon) {
        printf("base new failed, url:%s\n", url);
        goto end;
    }
    ack->arg = base;
    evhttp_connection_set_retries(evcon, 3);
    evhttp_connection_set_timeout(evcon, timeout_sec);
    evhttp_connection_set_closecb(evcon, CloseCallback, base);
    request = evhttp_request_new(ReadCallback, ack);
    if(request == NULL) {
        printf("request new failed, url:%s\n", url);
        goto end;
    }
    evhttp_request_set_error_cb(request, ErrCallback);

    output_headers = evhttp_request_get_output_headers(request);
    evhttp_add_header(output_headers, "Host", host);
    evhttp_add_header(output_headers, "Connection", "keep-alive");
    evhttp_add_header(output_headers, "Content-Type", "application/json");
    if(data != NULL) {
        len = strlen(data);
        output_buffer = evhttp_request_get_output_buffer(request);
        evbuffer_add(output_buffer, data, len);
    }
    evutil_snprintf(buf, sizeof(buf)-1, "%lu", (long unsigned int)len);
    evhttp_add_header(output_headers, "Content-Length", buf);

    ret = evhttp_make_request(evcon, request, cmd, req_url);
    if (ret != 0) {
        printf("make request failed\n");
        goto end;
    }
    timeout.tv_sec = timeout_sec;
    timeout.tv_usec = 0;
    event_base_loopexit(base, &timeout);
    ret = event_base_dispatch(base);
    if(ret != 0) {
        printf("dispatch failed\n");
        goto end;
    }
    ret = 0;

end:
    if(evcon != NULL) {
        evhttp_connection_free(evcon);
    }
    if(dnsbase != NULL) {
        evdns_base_free(dnsbase, 0);
    }
    if(base != NULL) {
        event_base_free(base);
    }
    if(http_uri != NULL) {
        evhttp_uri_free(http_uri);
    }
    //if(request != NULL) {
    //    evhttp_request_free(request);
    //}
    return ret;
}

int HttpPost(const char* url, char* data, HttpAck* ack, int timeout_sec) {
    return HttpClient(EVHTTP_REQ_POST, url, data, ack, timeout_sec);
}

int HttpGet(const char* url, HttpAck* ack, int timeout_sec) {
    return HttpClient(EVHTTP_REQ_GET, url, NULL, ack, timeout_sec);
}

int request_cb(struct evhttp_request* req, void (*http_task)(struct evhttp_request* , void* ), void* arg) {
    char *url;
    int code = HTTP_OK;
    char *pbody = NULL;
    const char *cmdtype = NULL;
    struct evkeyvalq *headers;
    struct evkeyval *header;
    struct evbuffer *buf;
    char cbuf[POST_BUF_MAX] = {0};

    switch (evhttp_request_get_command(req)) {
        case EVHTTP_REQ_GET: cmdtype = "GET"; break;
        case EVHTTP_REQ_POST: cmdtype = "POST"; break;
        case EVHTTP_REQ_HEAD: cmdtype = "HEAD"; break;
        case EVHTTP_REQ_PUT: cmdtype = "PUT"; break;
        case EVHTTP_REQ_DELETE: cmdtype = "DELETE"; break;
        case EVHTTP_REQ_OPTIONS: cmdtype = "OPTIONS"; break;
        case EVHTTP_REQ_TRACE: cmdtype = "TRACE"; break;
        case EVHTTP_REQ_CONNECT: cmdtype = "CONNECT"; break;
        case EVHTTP_REQ_PATCH: cmdtype = "PATCH"; break;
        default: cmdtype = "unknown"; break;
    }
    url = (char *)evhttp_request_get_uri(req);
    //if(cmdtype != NULL && req->remote_host != NULL && strcmp(url, "/api/system/status") != 0) {
    //    printf("recv a %s request, url:%s, remote:%s\n", cmdtype, url, req->remote_host);
    //}
    headers = evhttp_request_get_input_headers(req);
    for (header = headers->tqh_first; header;
        header = header->next.tqe_next) {
        //printf("head %s:%s\n", header->key, header->value);
    }
    buf = evhttp_request_get_input_buffer(req);
    while (evbuffer_get_length(buf)) {
        int n = evbuffer_remove(buf, cbuf, POST_BUF_MAX);
        if (n >= POST_BUF_MAX) {
            printf("content length is too large, %d\n", n);
            cbuf[POST_BUF_MAX - 1] = 0;
        }
    }
    if(strcmp(cmdtype, "GET") != 0 && strcmp(url, "/api/data/query") != 0) {
        Restful* rest = (Restful* )arg;
        AppDebug("%s,%s:%s", rest->GetType(), url, cbuf);
    }
    if(http_task != NULL) {
        CommonParams params;
        params.arga = cbuf;
        params.argb = &pbody;
        params.argc = arg;
        params.argd = &code;
        params.arge = url;
        http_task((struct evhttp_request *)(-1), &params);
    }
    SendHttpReply(req, code, pbody);
    if(pbody != NULL) {
        free(pbody);
    }

    return 0;
}

void CheckErrMsg(const char* msg, char** ppbody) {
    int len = strlen(msg);
    if(len > 0) {
        printf("%s\n", msg);
        *ppbody = (char* )malloc(len+64);
        snprintf(*ppbody, len+64, "{\"code\":-1,\"msg\":\"%s\",\"data\":{}}", msg);
    }
}

void SetAckMsg(const char* msg, char** ppbody) {
    int len = strlen(msg);
    if(len > 0) {
        *ppbody = (char* )malloc(len+64);
        snprintf(*ppbody, len+64, "{\"code\":-1,\"msg\":\"%s\",\"data\":{}}", msg);
    }
}

Restful::Restful(MediaServer* _media)
  : media(_media) {
}

Restful::~Restful(void) {
}

void request_gencb(struct evhttp_request* req, void* arg);
static int HttpTask(UrlMap* url_map, int port, void* arg) {
    struct event_base *base;
    struct evhttp *http;
    struct evhttp_bound_socket *handle;
    base = event_base_new();
    if(!base) {
        AppError("couldn't create an event_base: exiting");
        return -1;
    }
    /* Create a new evhttp object to handle requests. */
    http = evhttp_new(base);
    if (!http) {
        AppError("couldn't create evhttp. Exiting.");
        return -1;
    }

    for(int i = 0; ; i ++) {
        UrlMap *p = url_map + i;
        p->arg = arg;
        if(p->cb != NULL) {
            evhttp_set_cb(http, p->url, p->cb, p);
        }
        else {
            break;
        }
    }
    evhttp_set_gencb(http, request_gencb, arg);

    /* Now we tell the evhttp what port to listen on */
    handle = evhttp_bind_socket_with_handle(http, "0.0.0.0", port);
    if (!handle) {
        AppError("couldn't bind to port %d", (int)port);
        return -1;
    }
    event_base_dispatch(base);
    return 0;
}

static void RestApiThread(Restful* rest) {
    int port = rest->GetPort();
    UrlMap* url_map = rest->GetUrl();
    ConfigParams* config = rest->media->GetConfig();
    sleep(1);
    AppDebug("%s, %s:%d start ...", rest->GetType(), config->LocalIp(), port);
    HttpTask(url_map, port, rest);
    AppDebug("run ok");
}

void Restful::Start(void) {
    std::thread t(&RestApiThread, this);
    t.detach();
}

