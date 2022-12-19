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
  const char* config_file = media->config_file.c_str();
  auto type = GetStrValFromFile(config_file, "system", "db", "type");
  auto name = GetStrValFromFile(config_file, "system", "db", "name");
  auto host = GetStrValFromFile(config_file, "system", "db", "host");
  int port = GetIntValFromFile(config_file, "system", "db", "port");
  auto user = GetStrValFromFile(config_file, "system", "db", "user");
  auto password = GetStrValFromFile(config_file, "system", "db", "password");
  if (type == nullptr || name == nullptr || host == nullptr
      || port < 0 || !strcmp(type.get(), "none")) {
    printf("db disabled\n");
    return 0;
  }
  if (strcmp(type.get(), "mongodb") != 0) {
    AppWarn("unsupport db type : %s", type.get());
    return -1;
  }
  strncpy(db_name, name.get(), sizeof(db_name));

  bson_error_t error;
  char url[512] = {0};
  // mongodb://user:password@host:port/db
  strncat(url, "mongodb://", sizeof(url)-strlen(url)-1);
  if (user != nullptr && password != nullptr) {
    strncat(url, user.get(), sizeof(url)-strlen(url)-1);
    strncat(url, ":", sizeof(url)-strlen(url)-1);
    strncat(url, password.get(), sizeof(url)-strlen(url)-1);
    strncat(url, "@", sizeof(url)-strlen(url)-1);
  }
  strncat(url, host.get(), sizeof(url)-strlen(url)-1);
  strncat(url, ":", sizeof(url)-strlen(url)-1);
  snprintf(url+strlen(url), sizeof(url)-strlen(url)-1, "%d", port);
  strncat(url, "/", sizeof(url)-strlen(url)-1);
  strncat(url, db_name, sizeof(url)-strlen(url)-1);

  mongoc_init();
  MongodbParams *p = (MongodbParams *)calloc(1, sizeof(MongodbParams));
  p->uri = mongoc_uri_new_with_error(url, &error);
  if (p->uri == NULL) {
    AppError("parase %s err", url);
    free(p);
    return -1;
  }
  p->client = mongoc_client_new_from_uri(p->uri);
  if (p->client == NULL) {
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
  if (insert == NULL);
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
  if (insert == NULL) {
    AppError("bson from json failed, %s, %s", json, error.message);
    return NULL;
  }
  bson_t* update =  BCON_NEW(cmd, "{", select, BCON_DOCUMENT(insert), "}");
  bson_destroy(insert);
  return update;
}

static int _DBUpdate(DBHandle handle, const char*name, const char* table,
                     char* json, bson_t* selector, const char* cmd, bool upsert) {
  bson_error_t error;
  mongoc_collection_t* collection = NULL;
  bson_t *insert = NULL, *update = NULL, *opts = NULL;
  MongodbParams* p = (MongodbParams* )handle;

  if (p == NULL) {
    goto end;
  }
  opts = BCON_NEW ("upsert", BCON_BOOL(upsert));
  collection = mongoc_client_get_collection(p->client, name, table);
  if (collection == NULL) {
    AppError("get collection failed, %s", table);
    goto end;
  }
  insert = bson_new_from_json((const uint8_t *)json, -1, &error);
  if (insert == NULL) {
    AppError("bson from json failed, %s, %s", json, error.message);
    goto end;
  }
  update = BCON_NEW(cmd, BCON_DOCUMENT(insert));
  if (update == NULL) {
    AppError("bson new failed, %s", json);
    goto end;
  }
  if (!mongoc_collection_update_one(collection, selector, update, opts, NULL, &error)) {
    AppError("update write failed, %s, %s, %s", table, error.message, json);
    goto end;
  }

end:
  if (opts != NULL) {
    bson_destroy(opts);
  }
  if (update != NULL) {
    bson_destroy(update);
  }
  if (insert != NULL) {
    bson_destroy(insert);
  }
  if (collection != NULL) {
    mongoc_collection_destroy(collection);
  }
  bson_destroy(selector);
  return 0;
}

static int _DBUpdate2(DBHandle handle, const char*name, const char *table,
                      bson_t *selector, bson_t *update, bool upsert) {
  bson_error_t error;
  bson_t *opts = NULL;
  mongoc_collection_t *collection = NULL;
  MongodbParams *p = (MongodbParams *)handle;

  if (p == NULL) {
    goto end;
  }
  opts = BCON_NEW ("upsert", BCON_BOOL(upsert));
  collection = mongoc_client_get_collection(p->client, name, table);
  if (collection == NULL) {
    AppError("get collection failed, %s", table);
    goto end;
  }
  if (!mongoc_collection_update_one(collection, selector, update, opts, NULL, &error)) {
    AppError("update write failed, %s, %s", table, error.message);
    goto end;
  }
end:
  if (opts != NULL) {
    bson_destroy(opts);
  }
  if (collection != NULL) {
    mongoc_collection_destroy(collection);
  }
  bson_destroy(selector);
  bson_destroy(update);
  return 0;
}

DBTable DbParams::DBCreateTable(const char* table) {
  MongodbParams* p = (MongodbParams* )handle;
  if (p != NULL) {
    return mongoc_client_get_collection(p->client, db_name, table);
  } else {
    return NULL;
  }
}

int DbParams::DBDestroyTable(DBTable table) {
  mongoc_collection_t* collection = (mongoc_collection_t* )table;
  if (collection != NULL) {
    mongoc_collection_destroy(collection);
  }
  return 0;
}

int DbParams::DBInsert(DBTable table, char* json) {
  bson_error_t error;
  bson_t *insert = NULL;
  MongodbParams* p = (MongodbParams* )handle;
  mongoc_collection_t* collection = (mongoc_collection_t* )table;

  if (p == NULL) {
    goto end;
  }
  insert = bson_new_from_json((const uint8_t *)json, -1, &error);
  if (insert == NULL) {
    AppError("bson from json failed, %s, %s", json, error.message);
    goto end;
  }
  {
    std::unique_lock<std::mutex> lock(mtx);
    if (!mongoc_collection_insert_one(collection, insert, NULL, NULL, &error)) {
      AppError("insert failed, %s, %s", json, error.message);
      goto end;
    }
  }

end:
  if (insert != NULL) {
    bson_destroy(insert);
  }
  return 0;
}

int DbParams::DBInsert(const char* table, char* json) {
  bson_error_t error;
  bson_t *insert = NULL;
  mongoc_collection_t* collection = NULL;
  MongodbParams* p = (MongodbParams* )handle;

  if (p == NULL) {
    goto end;
  }
  insert = bson_new_from_json((const uint8_t *)json, -1, &error);
  if (insert == NULL) {
    AppError("bson from json failed, %s, %s", json, error.message);
    goto end;
  }
  {
    std::unique_lock<std::mutex> lock(mtx);
    collection = mongoc_client_get_collection(p->client, db_name, table);
    if (collection == NULL) {
      AppError("get collection failed, %s", table);
      goto end;
    }
    if (!mongoc_collection_insert_one(collection, insert, NULL, NULL, &error)) {
      AppError("insert failed, %s, %s, %s", table, error.message, json);
      goto end;
    }
  }

end:
  if (insert != NULL) {
    bson_destroy(insert);
  }
  if (collection != NULL) {
    mongoc_collection_destroy(collection);
  }
  return 0;
}

int DbParams::DBUpdate(const char* table, char* json, const char* select,
                       const char* val, const char* cmd, bool upsert) {
  bson_t* selector = BconNew(select, val);
  if (selector == NULL) {
    AppError("bson new selector failed, %s:%s", select, val);
    return -1;
  }
  return _DBUpdate(handle, db_name, table, json, selector, cmd, upsert);
}

int DbParams::DBUpdate(const char* table, char* json, const char* select_a, const char* val_a,
                       const char* select_b, int val_b, const char* cmd, bool upsert) {
  bson_t* selector = BCON_NEW(select_a, "{", "$eq", BCON_UTF8(val_a), "}",
                              select_b, "{", "$eq", BCON_INT32(val_b), "}");
  if (selector == NULL) {
    AppError("bson new selector failed, %s:%s,%s:%d", select_a, val_a, select_b, val_b);
    return -1;
  }
  return _DBUpdate(handle, db_name, table, json, selector, cmd, upsert);
}

int DbParams::DBUpdate(const char* table, char* json, const char* select,
                       int val, const char* cmd, bool upsert) {
  bson_t *selector = BconNew(select, val);
  if (selector == NULL) {
    AppError("bson new selector failed, %s:%d", select, val);
    return -1;
  }
  return _DBUpdate(handle, db_name, table, json, selector, cmd, upsert);
}

int DbParams::DBUpdate(const char* table, const char* select, int val,
                       const char* _update, const char* _val, const char* cmd, bool upsert) {
  bson_t *selector = BconNew(select, val);
  if (selector == NULL) {
    AppError("bson new selector failed, %s:%d", select, val);
    return -1;
  }
  bson_t *update = BconNewUpdateJson(cmd, _update, _val);
  if (update == NULL) {
    AppError("bson new failed, %s:%s", _update, _val);
    bson_destroy(selector);
    return -1;
  }
  return _DBUpdate2(handle, db_name, table, selector, update, upsert);
}

int DbParams::DBUpdate(const char* table, const char* select, int val,
                       const char* _update, int _val, const char *cmd, bool upsert) {
  bson_t *selector = BconNew(select, val);
  if (selector == NULL) {
    AppError("bson new selector failed, %s:%d", select, val);
    return -1;
  }
  bson_t *update = BconNewUpdate(cmd, _update, _val);
  if (update == NULL) {
    AppError("bson new failed, %s:%d", _update, _val);
    bson_destroy(selector);
    return -1;
  }
  return _DBUpdate2(handle, db_name, table, selector, update, upsert);
}

static int _DBDel(DBHandle handle, const char* name, const char *table, bson_t *selector) {
  bson_error_t error;
  mongoc_collection_t* collection = NULL;
  MongodbParams* p = (MongodbParams* )handle;

  if (p == NULL) {
    goto end;
  }
  collection = mongoc_client_get_collection(p->client, name, table);
  if (collection == NULL) {
    AppError("get collection failed, %s", table);
    goto end;
  }
  if (!mongoc_collection_delete_one(collection, selector, NULL, NULL, &error)) {
    AppError("del failed, %s, %s", table, error.message);
    goto end;
  }
end:
  if (collection != NULL) {
    mongoc_collection_destroy(collection);
  }
  bson_destroy(selector);

  return 0;
}

int DbParams::DBDel(const char* table, const char* select, const char* val) {
  bson_t* selector = NULL;
  selector = BconNew(select, val);
  if (selector == NULL) {
    AppError("bson new selector failed, %s:%s", select, val);
    return -1;
  }
  return _DBDel(handle, db_name, table, selector);
}

int DbParams::DBDel(const char* table, const char* select_a,
                    const char* val_a, const char* select_b, int val_b) {
  bson_t* selector = BCON_NEW(select_a, "{", "$eq", BCON_UTF8(val_a), "}",
                              select_b, "{", "$eq", BCON_INT32(val_b), "}");
  if (selector == NULL) {
    AppError("bson new selector failed, %s:%s,%s:%d", select_a, val_a, select_b, val_b);
    return -1;
  }
  return _DBDel(handle, db_name, table, selector);
}

int DbParams::DBDel(const char* table, const char* select, int val) {
  bson_t *selector = NULL;
  selector = BconNew(select, val);
  if (selector == NULL) {
    AppError("bson new selector failed, %s:%d", select, val);
    return -1;
  }
  return _DBDel(handle, db_name, table, selector);
}

int DbParams::DBTraverse(const char* table, void* arg, int (*cb)(char* buf, void* arg)) {
  char* str;
  bson_t query;
  const bson_t* doc;
  bson_t* opts = NULL;
  mongoc_cursor_t* cursor = NULL;
  mongoc_collection_t* collection = NULL;
  MongodbParams* p = (MongodbParams *)handle;

  if (p == NULL) {
    return 0;
  }
  bson_init(&query);
  opts = BCON_NEW("projection", "{", "_id", BCON_BOOL(false), "}");
  collection = mongoc_client_get_collection(p->client, db_name, table);
  if (collection == NULL) {
    AppError("get collection failed, %s", table);
    goto end;
  }
  cursor = mongoc_collection_find_with_opts(collection, &query, opts, NULL);
  while (mongoc_cursor_next(cursor, &doc)) {
    str = bson_as_json(doc, NULL);
    if (cb != NULL) {
      cb(str, arg);
    }
    bson_free(str);
  }
end:
  if (cursor != NULL) {
    mongoc_cursor_destroy(cursor);
  }
  if (collection != NULL) {
    mongoc_collection_destroy(collection);
  }
  if (opts != NULL) {
    bson_destroy(opts);
  }
  bson_destroy(&query);
  return 0;
}

int DbParams::DBQuery(const char* table, int start_time,
                      int stop_time, std::vector<int> id_vec,
                      int skip, int limit, int64_t* count,
                      void* arg, int (*cb)(char* buf, void* arg)) {
  char* str;
  bson_t id;
  const bson_t* doc;
  bson_error_t error;
  bson_t* query = NULL;
  bson_t* opts = NULL;
  mongoc_cursor_t* cursor = NULL;
  mongoc_collection_t* collection = NULL;
  MongodbParams* p = (MongodbParams *)handle;

  if (p == NULL) {
    return 0;
  }
  bson_init(&id);
  for (size_t i = 0; i < id_vec.size(); i ++) {
    BCON_APPEND(&id, "0", BCON_INT32(id_vec[i]));
  }
  query = BCON_NEW("data.timestamp", "{",
                   "$gte", BCON_INT32(start_time),
                   "$lte", BCON_INT32(stop_time), "}",
                   "data.id", "{", "$in", BCON_ARRAY(&id), "}");
  if (skip < 0 || limit < 0) {
    opts = BCON_NEW("projection", "{", "_id", BCON_BOOL(false), "}");
  } else {
    opts = BCON_NEW("projection", "{", "_id", BCON_BOOL(false), "}",
                    "skip", BCON_INT64(skip),
                    "limit", BCON_INT64(limit));
  }
  collection = mongoc_client_get_collection(p->client, db_name, table);
  if (collection == NULL) {
    AppError("get collection failed, %s", table);
    goto end;
  }
  if (count != NULL) {
    *count = mongoc_collection_count_documents(collection, query, NULL, NULL, NULL, &error);
    if (*count < 0) {
      AppError("count failed, %s", error.message);
    }
  }
  cursor = mongoc_collection_find_with_opts(collection, query, opts, NULL);
  while (mongoc_cursor_next(cursor, &doc)) {
    str = bson_as_json(doc, NULL);
    if (cb != NULL) {
      cb(str, arg);
    }
    bson_free(str);
  }
end:
  if (cursor != NULL) {
    mongoc_cursor_destroy(cursor);
  }
  if (collection != NULL) {
    mongoc_collection_destroy(collection);
  }
  if (query != NULL) {
    bson_destroy(query);
  }
  if (opts != NULL) {
    bson_destroy(opts);
  }
  bson_destroy(&id);
  return 0;
}

int DbParams::DBClose(void) {
  MongodbParams* p = (MongodbParams* )handle;
  if (p == NULL) {
    return 0;
  }
  if (p->uri != NULL) {
    mongoc_uri_destroy(p->uri);
  }
  if (p->client != NULL) {
    mongoc_client_destroy(p->client);
  }
  mongoc_cleanup();
  free(p);
  handle = nullptr;
  return 0;
}

