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
/*
#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>
#include <event2/dns.h>
*/

class MediaServer;
class Restful {
public:
    Restful(MediaServer* _media);
    ~Restful(void);
    void start(void);
    virtual void test(void){}
private:
    MediaServer* media;
};

class MasterRestful : public Restful {
public:
    MasterRestful(MediaServer* media):Restful(media){}
    virtual void test(void);
};

class SlaveRestful : public Restful {
public:
    SlaveRestful(MediaServer* media):Restful(media){}
    virtual void test(void);
};

typedef struct {
    const char *url;
    //void (*cb)(struct evhttp_request *, void *);
    void *arg;
} UrlMap;

#endif

