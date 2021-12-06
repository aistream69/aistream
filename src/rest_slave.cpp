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

static void request_login(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void request_logout(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void request_system_init(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void request_add_rtsp(struct evhttp_request *req, void *arg) {
    request_first_stage;
    CommonParams *params = (CommonParams *)arg;
    Restful* rest = (Restful* )params->argc;
    MediaServer* media = rest->media;
    AppDebug("##test, running:%d, port:%d", media->running, rest->GetPort());
}

static void request_add_rtmp(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void request_add_gb28181(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void request_add_gat1400(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void request_add_file(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void request_del_obj(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void request_start_task(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void request_stop_task(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void request_task_support(struct evhttp_request *req, void *arg) {
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
    ConfigParams* config = media->config;
    port = config->slave_rest_port;
}

UrlMap* SlaveRestful::GetUrl(void) {
    return rest_url_map;
}


