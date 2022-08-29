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

#ifndef __AISTREAM_SHARE_H__
#define __AISTREAM_SHARE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <condition_variable>
#include "config.h"

typedef struct {
    void *arga;
    void *argb;
    void *argc;
    void *argd;
    void *arge;
    int  val;
} CommonParams;

void HangUp(void);
int DirCheck(const char *dir);
int GetLocalIp(char host_ip[128]);
void NginxInit(NginxParams& nginx);
int DelOldFile(const char* dir, uint32_t max_sec, int layer, const char* suffix);
std::unique_ptr<char[]> ReadFile2Buf(const char *filename);
int ReadFile(const char *filename, void *buf, int size);
int ReadFile2(const char *filename, void *buf, int max);
int WriteFile(const char *filename, void *buf, int size, const char *mode);
int GetFileSize(const char *filename);
int GetIntValFromJson(char *buf, 
        const char *name1, const char *name2=NULL, const char *name3=NULL);
int GetIntValFromFile(const char *filename, 
        const char *name1, const char *name2=NULL, const char *name3=NULL);
double GetDoubleValFromJson(char *buf,
        const char *name1, const char *name2=NULL, const char *name3=NULL);
double GetDoubleValFromFile(const char *filename, 
        const char *name1, const char *name2=NULL, const char *name3=NULL);
std::unique_ptr<char[]> GetStrValFromJson(char *buf, 
        const char *name1, const char *name2=NULL, const char *name3=NULL);
std::unique_ptr<char[]> GetStrValFromFile(const char *filename, 
        const char *name1, const char *name2=NULL, const char *name3=NULL);
std::unique_ptr<char[]> GetObjBufFromJson(char *buf, 
        const char *name1, const char *name2=NULL, const char *name3=NULL);
std::unique_ptr<char[]> GetArrayBufFromJson(char *buf, int &size, 
        const char *name1, const char *name2=NULL, const char *name3=NULL);
std::unique_ptr<char[]> GetArrayBufFromFile(const char *filename, int &size, 
        const char *name1, const char *name2=NULL, const char *name3=NULL);
std::unique_ptr<char[]> AddStrJson(char *buf, const char *val, 
        const char *name1, const char *name2=NULL, const char *name3=NULL);
std::unique_ptr<char[]> DelJsonObj(char *buf,
        const char *name1, const char *name2=NULL, const char *name3=NULL);
std::unique_ptr<char[]> GetBufFromArray(char *buf, int index);

#endif

