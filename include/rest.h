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

#ifndef __AISTREAM_REST_H__
#define __AISTREAM_REST_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>
#include <event2/dns.h>
#include "log.h"

#define POST_BUF_MAX    1024

typedef struct {
    const char *url;
    void (*cb)(struct evhttp_request *, void *);
    void *arg;
} UrlMap;

#define request_first_stage                         \
do {                                                \
    if(req == NULL) {                               \
        AppWarn("req is null");                 \
        return ;                                    \
    }                                               \
    if(req != (struct evhttp_request *)(-1)) {      \
        UrlMap *p = (UrlMap *)arg;                  \
        request_cb(req, p->cb, p->arg);             \
        return ;                                    \
    }                                               \
} while(0)

typedef struct {
    std::unique_ptr<char[]> buf;
    void *arg;
} HttpAck;

class MediaServer;
class Restful {
public:
    Restful(MediaServer* _media);
    ~Restful(void);
    void Start(void);
    virtual int GetPort(void) {return 8098;}
    virtual UrlMap* GetUrl(void){return NULL;}
    virtual const char* GetType(void){return "";}
    MediaServer* media;
private:
};

class MasterRestful : public Restful {
public:
    MasterRestful(MediaServer* _media);
    virtual int GetPort(void) {return port;}
    virtual UrlMap* GetUrl(void);
    virtual const char* GetType(void);
private:
    int port;
};

class SlaveRestful : public Restful {
public:
    SlaveRestful(MediaServer* _media);
    virtual int GetPort(void) {return port;}
    virtual UrlMap* GetUrl(void);
    virtual const char* GetType(void);
private:
    int port;
};

int HttpPost(const char* url, char* data, HttpAck* ack, int timeout_sec = 3);
int HttpGet(const char* url, HttpAck* ack, int timeout_sec = 3);
int request_cb(struct evhttp_request* req, void (*http_task)(struct evhttp_request *, void *), void* arg);
void CheckErrMsg(const char* err_msg, char** ppbody);

#endif

