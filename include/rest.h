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
        AppWarning("req is null");                 \
        return ;                                    \
    }                                               \
    if(req != (struct evhttp_request *)(-1)) {      \
        UrlMap *p = (UrlMap *)arg;                  \
        request_cb(req, p->cb, p->arg);             \
        return ;                                    \
    }                                               \
} while(0)

class MediaServer;
class Restful {
public:
    Restful(MediaServer* _media);
    ~Restful(void);
    void start(void);
    virtual int GetPort(void) {return 8098;}
    virtual UrlMap* GetUrl(void){return NULL;}
    MediaServer* media;
private:
    void RestApiThread(void);
};

class MasterRestful : public Restful {
public:
    MasterRestful(MediaServer* _media):Restful(_media){}
    virtual int GetPort(void) {return port;}
    virtual UrlMap* GetUrl(void);
private:
    int port;
};

class SlaveRestful : public Restful {
public:
    SlaveRestful(MediaServer* _media);
    virtual int GetPort(void) {return port;}
    virtual UrlMap* GetUrl(void);
private:
    int port;
};

int request_cb(struct evhttp_request *req, void (*http_task)(struct evhttp_request *, void *), void *arg);

#endif

