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
#include "amqp.h"
#include "amqp_tcp_socket.h"
#include "share.h"
#include "tensor.h"
#include "log.h"

typedef struct {
    int init;
    char host[128];
    int  port;
    char username[64];
    char password[64];
    char exchange[256];
    char routingkey[256];
    amqp_connection_state_t conn;
    pthread_mutex_t mtx;
} Rabbitmq;

static Rabbitmq mq = {0};
static int MqOpenConnect(Rabbitmq& mq, int timeoutsec) {
    int status;
    int  port = mq.port;
    amqp_socket_t *socket = NULL;
    char const * hostname = mq.host;
    int chn_opened = 0, conn_opened = 0;

    amqp_connection_state_t conn = amqp_new_connection();
    if(conn == NULL) {
        AppError(" amqp_new_connection failed");
        return -1;
    }
    socket = amqp_tcp_socket_new(conn);
    if(NULL == socket) {
        AppError(" amqp_tcp_socket_new failed");
        return -1;
    }
    if(timeoutsec>0) {
        struct timeval timeout = {timeoutsec,0};
        status = amqp_socket_open_noblock(socket, hostname, port, &timeout);
    }
    else {
        status = amqp_socket_open(socket, hostname, port);
    }
    if(status == AMQP_STATUS_OK) {
        //printf("opening TCP socket");
        conn_opened = 1;
    }
    amqp_rpc_reply_t rpc_reply = amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN,
                                 mq.username, mq.password);
    if(rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        printf("mq login %s:%d failed, ret:%d\n", hostname, port, rpc_reply.reply_type);
        goto new_err;
    }
    if(amqp_channel_open(conn, 1) != NULL) {
        chn_opened = 1;
    }
    rpc_reply = amqp_get_rpc_reply(conn);
    if(rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        AppError("get rpc reply failed, ret:%d", rpc_reply.reply_type);
        goto new_err;
    }
    mq.conn = conn;
    return 0;

new_err:
    if(chn_opened) {
        rpc_reply = amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
        if(rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
            AppError("close channel failed, ret:%d", rpc_reply.reply_type);
        }
    }
    if(conn_opened) {
        rpc_reply = amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
        if(rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
            AppError("close connction failed, ret:%d", rpc_reply.reply_type);
        }
    }
    int ret = amqp_destroy_connection(conn);
    if(ret != AMQP_STATUS_OK) {
        AppError("destroty connection failed, ret:%d", ret);
    }
    return -1;
}

static int MqCloseConnect(amqp_connection_state_t conn) {
    amqp_rpc_reply_t rpc_reply = amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
    if(rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        printf("mq, close channel failed, ret:%d\n", rpc_reply.reply_type);
    }
    rpc_reply = amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
    if(rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        AppError("close connction failed, ret:%d", rpc_reply.reply_type);
    }
    int ret = amqp_destroy_connection(conn);
    if(ret != AMQP_STATUS_OK) {
        AppError("destroty connection failed, ret:%d", ret);
    }
    return -1;
}

static int MqSendMsg(Rabbitmq& mq, char* buf) {
    char const* routingkey = mq.routingkey;
    char const* messagebody = buf;
    char const* exchange = mq.exchange;
    amqp_connection_state_t conn = mq.conn;

    amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
    props.content_type = amqp_cstring_bytes("text/plain");
    props.delivery_mode = 2; /* persistent delivery mode */
    int ret = amqp_basic_publish(conn, 1, amqp_cstring_bytes(exchange),
                                    amqp_cstring_bytes(routingkey), 0, 0,
                                    &props, amqp_cstring_bytes(messagebody));
    if(ret != AMQP_STATUS_OK) {
        AppError("basic publish failed, ret:%d", ret);
        return -1;
    }
    return 0;
}

static int MqSend(Rabbitmq& mq, char* buf) {
    if(mq.conn == NULL && MqOpenConnect(mq, 1) != 0) {
        return -1;
    }
    int ret = MqSendMsg(mq, buf);
    if(ret != 0) {
        MqCloseConnect(mq.conn);
        mq.conn = NULL;
        MqOpenConnect(mq, 1);
        if(mq.conn != NULL) {
            ret = MqSendMsg(mq, buf);
        }
        else {
            static int cnt = 0;
            if(cnt ++ % 1000 == 0) {
                AppError("mq open failed, %s:%d", mq.host, mq.port);
            }
        }
    }
    return ret;
}

extern "C" int MqInit(ElementData* data, char* params) {
    strncpy(data->input_name[0], "rabbitmq_input", sizeof(data->input_name[0]));
    if(mq.init) {
        return 0;
    }
    if(params == NULL) {
        AppWarn("params is null");
        return -1;
    }
    auto host = GetStrValFromJson(params, "host");
    int port = GetIntValFromJson(params, "port");
    auto username = GetStrValFromJson(params, "username");
    auto password = GetStrValFromJson(params, "password");
    auto exchange = GetStrValFromJson(params, "exchange");
    auto routingkey = GetStrValFromJson(params, "routingkey");
    if(host == nullptr || port <= 0 || username == nullptr || 
            password == nullptr || exchange == nullptr || routingkey == nullptr) {
        AppWarn("get mq params failed");
        return -1;
    }
    mq.port = port;
    strncpy(mq.host, host.get(), sizeof(mq.host));
    strncpy(mq.username, username.get(), sizeof(mq.username));
    strncpy(mq.password, password.get(), sizeof(mq.password));
    strncpy(mq.exchange, exchange.get(), sizeof(mq.exchange));
    strncpy(mq.routingkey, routingkey.get(), sizeof(mq.routingkey));
    pthread_mutex_init(&mq.mtx, NULL);
    mq.init = 1;
    return 0;
}

extern "C" IHandle MqStart(int channel, char* params) {
    return &mq;
}

extern "C" int MqProcess(IHandle handle, TensorData* data) {
    auto pkt = data->tensor_buf.input[0];
    char* json = pkt->_data;
    if(mq.init) {
        pthread_mutex_lock(&mq.mtx);
        MqSend(mq, json);
        pthread_mutex_unlock(&mq.mtx);
        //printf("##test, mq, %s\n", json);
    }
    return 0;
}

extern "C" int MqStop(IHandle handle) {
    return 0;
}

extern "C" int MqRelease(void) {
    // don't destroty when multi channel obj use it
    //pthread_mutex_destroy(&mq.mtx);
    return 0;
}

extern "C" int DylibRegister(DLRegister** r, int& size) {
    size = 1;
    DLRegister* p = (DLRegister*)calloc(size, sizeof(DLRegister));
    strncpy(p->name, "rabbitmq", sizeof(p->name));
    p->init = "MqInit";
    p->start = "MqStart";
    p->process = "MqProcess";
    p->stop = "MqStop";
    p->release = "MqRelease";
    *r = p;
    return 0;
}

