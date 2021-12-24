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

static int SendHttpReply(struct evhttp_request *req, int code, char *buf) {
    struct evbuffer *evb;
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

int request_cb(struct evhttp_request *req, void (*http_task)(struct evhttp_request *, void *), void *arg) {
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
    if(cmdtype != NULL) {
        //printf("received a %s request for %s\n", cmdtype, url);
    }
    headers = evhttp_request_get_input_headers(req);
    for (header = headers->tqh_first; header;
        header = header->next.tqe_next) {
        //printf("head %s:%s\n", header->key, header->value);
    }
    buf = evhttp_request_get_input_buffer(req);
    while (evbuffer_get_length(buf)) {
        int n = evbuffer_remove(buf, cbuf, POST_BUF_MAX);
        if (n >= POST_BUF_MAX) {
            AppWarn("content length is too large, %d", n);
            cbuf[POST_BUF_MAX - 1] = 0;
        }
    }
    if(strcmp(cmdtype, "GET") != 0) {
        AppDebug("%s:%s", url, cbuf);
    }
    if(http_task != NULL) {
        CommonParams params;
        params.arga = cbuf;
        params.argb = &pbody;
        params.argc = arg;
        params.argd = &code;
        http_task((struct evhttp_request *)(-1), &params);
    }
    SendHttpReply(req, code, pbody);
    if(pbody != NULL) {
        free(pbody);
    }

    return 0;
}

Restful::Restful(MediaServer *_media)
  : media(_media) {
}

Restful::~Restful(void) {
}

static int HttpTask(UrlMap *url_map, int port, void *arg) {
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
    //evhttp_set_gencb(http, request_gencb, NULL);
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
    AppDebug("%s:%d start ...", config->LocalIp(), port);
    HttpTask(url_map, port, rest);
    AppDebug("run ok");
}

void Restful::start(void) {
    std::thread t(&RestApiThread, this);
    t.detach();
}

