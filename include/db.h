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

#ifndef __AISTREAM_DB_H__
#define __AISTREAM_DB_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef void* DBHandle;
typedef void* DBTable;

class MediaServer;
class DbParams {
public:
    DbParams(MediaServer* _media);
    ~DbParams(void);
    DBTable DBCreateTable(const char* table);
    int DBDestroyTable(DBTable table);
    int DBInsert(DBTable table, char* json);
    int DBInsert(const char* table, char* json); // with multi thread mutex, not best perfermance
    int DBUpdate(const char* table, char* json, const char* select, 
                 const char* val, const char* cmd = "$set", bool upsert = true);
    int DBUpdate(const char* table, char* json, const char* select_a, const char* val_a, 
                 const char* select_b, int val_b, const char* cmd = "$set", bool upsert = true);
    int DBUpdate(const char* table, char* json, const char* select, 
                 int val, const char* cmd = "$set", bool upsert = true);
    int DBUpdate(const char* table, const char* select, int val, 
                 const char* _update, const char* _val, const char* cmd = "$set", bool upsert = true);
    int DBUpdate(const char* table, const char* select, int val, 
                 const char* _update, int _val, const char* cmd = "$set", bool upsert = true);
    int DBDel(const char* table, const char* select, const char* val);
    int DBDel(const char* table, const char* select_a, const char* val_a, 
              const char* select_b, int val_b);
    int DBDel(const char* table, const char* select, int val);
    int DBTraverse(const char* table, void* arg, int (*cb)(char* buf, void* arg));
    int DBQuery(const char* table, int start_time, int stop_time, std::vector<int> id_vec, 
                int skip, int limit, int64_t* count, void* arg, int (*cb)(char* buf, void* arg));
    MediaServer* media;
private:
    int DBOpen(void);
    int DBClose(void);
    char db_name[256];
    std::mutex mtx;
    DBHandle handle;
};

#endif

