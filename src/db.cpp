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
#include "mongoc.h"

typedef struct {
    mongoc_uri_t *uri;
    mongoc_client_t *client;
} MongodbParams;

DbParams::DbParams(MediaServer* _media)
  : media(_media) {
    DBOpen();
}

DbParams::~DbParams(void) {
    DBClose();
}

int DbParams::DBOpen(void) {
    handle = nullptr;
    auto type = GetStrValFromFile(CONFIG_FILE, "system", "db", "type");
    auto host = GetStrValFromFile(CONFIG_FILE, "system", "db", "host");
    int port = GetIntValFromFile(CONFIG_FILE, "system", "db", "port");
    auto user = GetStrValFromFile(CONFIG_FILE, "system", "db", "user");
    auto password = GetStrValFromFile(CONFIG_FILE, "system", "db", "password");
    if(type == nullptr || host == nullptr || port < 0 || !strcmp(type.get(), "none")) {
        printf("db disabled\n");
        return 0;
    }
    if(strcmp(type.get(), "mongodb") != 0) {
        AppWarn("unsupport db type : %s", type.get());
        return -1;
    }

    bson_error_t error;
    char url[512] = {0};
    // mongodb://user:password@host:port/db
    strncat(url, "mongodb://", sizeof(url)-strlen(url)-1);
    if(user != nullptr && password != nullptr) {
        strncat(url, user.get(), sizeof(url)-strlen(url)-1);
        strncat(url, ":", sizeof(url)-strlen(url)-1);
        strncat(url, password.get(), sizeof(url)-strlen(url)-1);
        strncat(url, "@", sizeof(url)-strlen(url)-1);
    }
    strncat(url, host.get(), sizeof(url)-strlen(url)-1);
    strncat(url, ":", sizeof(url)-strlen(url)-1);
    snprintf(url+strlen(url), sizeof(url)-strlen(url)-1, "%d", port);
    strncat(url, "/", sizeof(url)-strlen(url)-1);
    strncat(url, DB_NAME, sizeof(url)-strlen(url)-1);

    mongoc_init();
    MongodbParams *p = (MongodbParams *)calloc(1, sizeof(MongodbParams));
    p->uri = mongoc_uri_new_with_error(url, &error);
    if(p->uri == NULL) {
        AppError("parase %s err", url);
        free(p);
        return -1;
    }
    p->client = mongoc_client_new_from_uri(p->uri);
    if(p->client == NULL) {
        AppError("new client failed, %s", url);
        free(p);
        return -1;
    }
    //mongoc_client_set_error_api(pDBParams->client, 2);
    //mongoc_client_set_appname(pDBParams->client, "connect-example");
    AppDebug("open %s success", url);
    handle = p;

    return 0;
}

int DBRead(DBHandle handle) {
    bson_error_t error;
    const char *json = "{\"test\":1}";
    bson_t *insert = bson_new_from_json((const uint8_t *)json, -1, &error);
    if(insert == NULL);
    return 0;
}

static bson_t* BconNew(const char* select, const char* val) {
    return BCON_NEW(select, "{", "$eq", BCON_UTF8(val), "}");
}

static bson_t* BconNew(const char* select, int32_t val) {
    return BCON_NEW(select, "{", "$eq", BCON_INT32(val), "}");
}

static bson_t* BconNewUpdate(const char* cmd, const char* select, int32_t val) {
    return BCON_NEW(cmd, "{", select, BCON_INT32(val), "}");
}

//static bson_t* BconNewUpdate(const char* cmd, const char* select, const char* key, const char* val) {
//    return BCON_NEW(cmd, "{", select, BCON_UTF8(val), "}");
//}

static bson_t* BconNewUpdateJson(const char* cmd, const char* select, const char* json) {
    bson_error_t error;
    bson_t* insert = NULL;
    insert = bson_new_from_json((const uint8_t *)json, -1, &error);
    if(insert == NULL) {
        AppError("bson from json failed, %s, %s", json, error.message);
        return NULL;
    }
    bson_t* update =  BCON_NEW(cmd, "{", select, BCON_DOCUMENT(insert), "}");
    bson_destroy(insert);
    return update;
}

static int _DBUpdate(DBHandle handle, const char* table, 
        char* json, bson_t* selector, const char* cmd, bool upsert) {
    bson_error_t error;
    mongoc_collection_t* collection = NULL;
    bson_t *insert = NULL, *update = NULL, *opts = NULL;
    MongodbParams* p = (MongodbParams* )handle;

    if(p == nullptr) {
        goto end;
    }
    opts = BCON_NEW ("upsert", BCON_BOOL(upsert));
    collection = mongoc_client_get_collection(p->client, DB_NAME, table);
    if(collection == NULL) {
        AppError("get collection failed, %s", table);
        goto end;
    }
    insert = bson_new_from_json((const uint8_t *)json, -1, &error);
    if(insert == NULL) {
        AppError("bson from json failed, %s, %s", json, error.message);
        goto end;
    }
    update = BCON_NEW(cmd, BCON_DOCUMENT(insert));
    if(update == NULL) {
        AppError("bson new failed, %s", json);
        goto end;
    }
    if(!mongoc_collection_update_one(collection, selector, update, opts, NULL, &error)) {
       AppError("update write failed, %s, %s, %s", table, error.message, json);
       goto end;
    }

end:
    if(opts != NULL) {
        bson_destroy(opts);
    }
    if(update != NULL) {
        bson_destroy(update);
    }
    if(insert != NULL) {
        bson_destroy(insert);
    }
    if(collection != NULL) {
        mongoc_collection_destroy(collection);
    }
    bson_destroy(selector);
    return 0;
}

static int _DBUpdate2(DBHandle handle, const char *table, 
                      bson_t *selector, bson_t *update, bool upsert) {
    bson_error_t error;
    bson_t *opts = NULL;
    mongoc_collection_t *collection = NULL;
    MongodbParams *p = (MongodbParams *)handle;

    if(p == nullptr) {
        goto end;
    }
    opts = BCON_NEW ("upsert", BCON_BOOL(upsert));
    collection = mongoc_client_get_collection(p->client, DB_NAME, table);
    if(collection == NULL) {
        AppError("get collection failed, %s", table);
        goto end;
    }
    if(!mongoc_collection_update_one(collection, selector, update, opts, NULL, &error)) {
       AppError("update write failed, %s, %s", table, error.message);
       goto end;
    }
end:
    if(opts != NULL) {
        bson_destroy(opts);
    }
    if(collection != NULL) {
        mongoc_collection_destroy(collection);
    }
    bson_destroy(selector);
    bson_destroy(update);
    return 0;
}

int DbParams::DBUpdate(const char* table, char* json, const char* select, 
                       const char* val, const char* cmd, bool upsert) {
    bson_t* selector = BconNew(select, val);
    if(selector == NULL) {
        AppError("bson new selector failed, %s:%s", select, val);
        return -1;
    }
    return _DBUpdate(handle, table, json, selector, cmd, upsert);
}

int DbParams::DBUpdate(const char* table, char* json, const char* select, 
                       int val, const char* cmd, bool upsert) {
    bson_t *selector = BconNew(select, val);
    if(selector == NULL) {
        AppError("bson new selector failed, %s:%d", select, val);
        return -1;
    }
    return _DBUpdate(handle, table, json, selector, cmd, upsert);
}

int DbParams::DBUpdate(const char* table, const char* select, int val, 
                       const char* _update, const char* _val, const char* cmd, bool upsert) {
    bson_t *selector = BconNew(select, val);
    if(selector == NULL) {
        AppError("bson new selector failed, %s:%d", select, val);
        return -1;
    }
    bson_t *update = BconNewUpdateJson(cmd, _update, _val);
    if(update == NULL) {
        AppError("bson new failed, %s:%s", _update, _val);
        bson_destroy(selector);
        return -1;
    }
    return _DBUpdate2(handle, table, selector, update, upsert);
}

int DbParams::DBUpdate(const char* table, const char* select, int val, 
                       const char* _update, int _val, const char *cmd, bool upsert) {
    bson_t *selector = BconNew(select, val);
    if(selector == NULL) {
        AppError("bson new selector failed, %s:%d", select, val);
        return -1;
    }
    bson_t *update = BconNewUpdate(cmd, _update, _val);
    if(update == NULL) {
        AppError("bson new failed, %s:%d", _update, _val);
        bson_destroy(selector);
        return -1;
    }
    return _DBUpdate2(handle, table, selector, update, upsert);
}

static int _DBDel(DBHandle handle, const char *table, bson_t *selector) {
    bson_error_t error;
    mongoc_collection_t* collection = NULL;
    MongodbParams* p = (MongodbParams* )handle;

    if(p == nullptr) {
        goto end;
    }
    collection = mongoc_client_get_collection(p->client, DB_NAME, table);
    if(collection == NULL) {
        AppError("get collection failed, %s", table);
        goto end;
    }
    if(!mongoc_collection_delete_one(collection, selector, NULL, NULL, &error)) {
       AppError("del failed, %s, %s", table, error.message);
       goto end;
    }
end:
    if(collection != NULL) {
        mongoc_collection_destroy(collection);
    }
    bson_destroy(selector);

    return 0;
}

int DbParams::DBDel(const char* table, const char* select, const char* val) {
    bson_t* selector = NULL;
    selector = BconNew(select, val);
    if(selector == NULL) {
        AppError("bson new selector failed, %s:%s", select, val);
        return -1;
    }
    return _DBDel(handle, table, selector);
}

int DbParams::DBDel(const char* table, const char* select, int val) {
    bson_t *selector = NULL;
    selector = BconNew(select, val);
    if(selector == NULL) {
        AppError("bson new selector failed, %s:%d", select, val);
        return -1;
    }
    return _DBDel(handle, table, selector);
}

int DbParams::DbTraverse(const char* table, void* arg, int (*cb)(char* buf, void* arg)) {
    char* str;
    bson_t query;
    const bson_t* doc;
    mongoc_cursor_t* cursor = NULL;
    mongoc_collection_t* collection = NULL;
    MongodbParams* p = (MongodbParams *)handle;

    if(p == nullptr) {
        return 0;
    }
    bson_init(&query);
    collection = mongoc_client_get_collection(p->client, DB_NAME, table);
    if(collection == NULL) {
        AppError("get collection failed, %s", table);
        goto end;
    }
    cursor = mongoc_collection_find_with_opts(collection, &query, NULL, NULL);
    while(mongoc_cursor_next(cursor, &doc)) {
        str = bson_as_json(doc, NULL);
        if(cb != NULL) {
            cb(str, arg);
        }
        bson_free(str);
    }
end:
    if(cursor != NULL) {
        mongoc_cursor_destroy(cursor);
    }
    if(collection != NULL) {
        mongoc_collection_destroy(collection);
    }
    bson_destroy(&query);
    return 0;
}

int DbParams::DBClose(void) {
    MongodbParams* p = (MongodbParams* )handle;
    if(p == nullptr) {
        return 0;
    }
    if(p->uri != NULL) {
        mongoc_uri_destroy(p->uri);
    }
    if(p->client != NULL) {
        mongoc_client_destroy(p->client);
    }
    mongoc_cleanup();
    free(p);
    handle = nullptr;
    return 0;
}
