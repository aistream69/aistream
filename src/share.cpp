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
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/sysinfo.h>
#include <ctype.h>
#include <execinfo.h>
#include <memory>
#include "share.h"
#include "cJSON.h"
#include "log.h"

static int CreateDir(const char *path) {
    char dirName[4096];
    strcpy(dirName, path);

    int len = strlen(dirName);
    if(dirName[len-1]!='/')
        strcat(dirName, "/");

    len = strlen(dirName);
    int i=0;
    for(i=1; i<len; i++) {
        if(dirName[i]=='/') {
            dirName[i] = 0;
            if(access(dirName, R_OK)!=0) {
                if(mkdir(dirName, 0755)==-1) {
                    AppError("create path %s failed, %s", dirName, strerror(errno));
                    return -1;
                }
            }
            dirName[i] = '/';
        }
    }
    return 0;
}

int DirCheck(const char *dir) {
    DIR *pdir = opendir(dir);
    if(pdir == NULL)
        return CreateDir(dir);
    else
        return closedir(pdir);   
}

int GetLocalIp(char host_ip[128]) {
    int sock;
    struct ifconf conf;
    struct ifreq *ifr;
    char buff[BUFSIZ];
    int num;
    int i;
    unsigned int u32_addr = 0;
    char str_ip[64] = {0};

    sock = socket(PF_INET, SOCK_DGRAM, 0);
    conf.ifc_len = BUFSIZ;
    conf.ifc_buf = buff;
    ioctl(sock, SIOCGIFCONF, &conf);
    num = conf.ifc_len / sizeof(struct ifreq);
    ifr = conf.ifc_req;
    for (i = 0; i < num; i++) {
        ioctl(sock, SIOCGIFFLAGS, ifr);
        u32_addr = ((struct sockaddr_in *)&ifr->ifr_addr)->sin_addr.s_addr;

        if (((ifr->ifr_flags & IFF_LOOPBACK) == 0) && (ifr->ifr_flags & IFF_UP)) {
            if(strstr(ifr->ifr_name,"docker") || 
                    strstr(ifr->ifr_name,"cicada") ||
                    strstr(ifr->ifr_name,":")) {
                continue;
            }

            inet_ntop(AF_INET, &u32_addr, str_ip, (socklen_t )sizeof(str_ip));

            if(strstr(ifr->ifr_name,"eth")||strstr(ifr->ifr_name,"em") ) {
                break;
            }
        }
        ifr++;
    }
    if(str_ip == NULL) {
        printf("str_ip is NULL\n");
        return -1;
    }
    strncpy(host_ip, str_ip, sizeof(str_ip));

    return 0;
}

std::unique_ptr<char[]> ReadFile2Buf(const char *filename) {
    int size;
    std::unique_ptr<char[]> buf = nullptr;
    FILE *fp = fopen(filename, "rb");
    if(fp == NULL) {
        AppError("fopen %s failed", filename);
        return buf;
    }
    fseek(fp, 0L, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    buf = std::make_unique<char[]>(size+1);
    memset(buf.get(), 0, size+1);
    if(fread(buf.get(), 1, size, fp)){
    }
    fclose(fp);
    return buf;
}

int ReadFile(const char *filename, void *buf, int size) {
    FILE *fp = fopen(filename, "rb");
    if(fp == NULL) {
        AppError("fopen %s failed", filename);
        return -1;
    }
    int n = fread(buf, 1, size, fp);
    fclose(fp);
    return n;
}

int ReadFile2(const char *filename, void *buf, int max) {
    struct stat f_stat;
    FILE *fp = fopen(filename, "rb");
    if(fp == NULL) {
        AppError("fopen %s failed", filename);
        return -1;
    }
    if(stat(filename, &f_stat) == 0) {
        if(f_stat.st_size > max) {
            return -1;
        }
    }
    int n = fread(buf, 1, f_stat.st_size, fp);
    fclose(fp);
    return n;
}

int WriteFile(const char *filename, void *buf, int size, const char *mode) {
    FILE *fp = fopen(filename, mode);
    if(fp == NULL) {
        AppError("fopen %s failed", filename);
        return -1;
    }
    if(buf != NULL) {
        fwrite(buf, 1, size, fp);
    }
    fclose(fp);
    return 0;
}

int GetIntValFromJson(char *buf, const char *name1, const char *name2, const char *name3) {
    int val = -1;
    char name[256];
    cJSON *root, *pSub1, *pSub2, *pSub3;
    root = cJSON_Parse(buf);
    if(root == NULL) {
        AppError("cJSON_Parse err, buf:%s", buf);
        goto end;
    }
    if(name1 == NULL) {
        AppError("name1 is null");
        goto end;
    }
    strncpy(name, name1, sizeof(name));
    pSub1 = cJSON_GetObjectItem(root, name);
    if(pSub1 == NULL) {
        //printf("get json null, %s\n", name);
        goto end;
    }
    if(name2 == NULL) {
        val = pSub1->valueint;
        goto end;
    }
    strncpy(name, name2, sizeof(name));
    pSub2 = cJSON_GetObjectItem(pSub1, name);
    if(pSub2 == NULL) {
        //printf("get json null, %s\n", name);
        goto end;
    }
    if(name3 == NULL) {
        val = pSub2->valueint;
        goto end;
    }
    strncpy(name, name3, sizeof(name));
    pSub3 = cJSON_GetObjectItem(pSub2, name);
    if(pSub3 == NULL) {
        //printf("get json null, %s\n", name);
        goto end;
    }
    val = pSub3->valueint;
end:
    if(root != NULL) {
        cJSON_Delete(root);
    }
    return val;
}

std::unique_ptr<char[]> GetStrValFromJson(char *buf, 
        const char *name1, const char *name2, const char *name3) {
    char name[256];
    char *tmp = NULL;
    cJSON *root, *pSub1, *pSub2, *pSub3;
    root = cJSON_Parse(buf);
    if(root == NULL) {
        AppError("cJSON_Parse err, buf:%s", buf);
        goto end;
    }
    if(name1 == NULL) {
        //printf("get json null, %s\n", name);
        goto end;
    }
    strncpy(name, name1, sizeof(name));
    pSub1 = cJSON_GetObjectItem(root, name);
    if(pSub1 == NULL) {
        //printf("get json null, %s\n", name);
        goto end;
    }
    if(name2 == NULL) {
        tmp = pSub1->valuestring;
        goto end;
    }
    strncpy(name, name2, sizeof(name));
    pSub2 = cJSON_GetObjectItem(pSub1, name);
    if(pSub2 == NULL) {
        //printf("get json null, %s\n", name);
        goto end;
    }
    if(name3 == NULL) {
        tmp = pSub2->valuestring;
        goto end;
    }
    strncpy(name, name3, sizeof(name));
    pSub3 = cJSON_GetObjectItem(pSub2, name);
    if(pSub3 == NULL) {
        //printf("get json null, %s\n", name);
        goto end;
    }
    tmp = pSub3->valuestring;
end:
    std::unique_ptr<char[]> val = nullptr;
    if(tmp != NULL) {
        int len = strlen(tmp) + 1;
        val = std::make_unique<char[]>(len);
        strcpy(val.get(), tmp);
    }
    if(root != NULL) {
        cJSON_Delete(root);
    }
    return val;
}

int GetIntValFromFile(const char *filename, const char *name1, const char *name2, const char *name3) {
    auto buf = ReadFile2Buf(filename);
    if(buf == nullptr) {
        AppWarn("%s, ReadFile2Buf failed", filename);
        return -1;
    }
    return GetIntValFromJson(buf.get(), name1, name2, name3);
}

std::unique_ptr<char[]>  GetStrValFromFile(const char *filename, 
        const char *name1, const char *name2, const char *name3) {
    auto buf = ReadFile2Buf(filename);
    if(buf == NULL) {
        AppWarn("%s, ReadFile2Buf failed", filename);
        return nullptr;
    }
    return GetStrValFromJson(buf.get(), name1, name2, name3);
}

std::unique_ptr<char[]> GetArrayBufFromJson(char *buf, int &size, 
            const char *name1, const char *name2, const char *name3) {
    char name[256];
    cJSON *pArray = NULL;
    cJSON *root, *pSub1, *pSub2, *pSub3;
    root = cJSON_Parse(buf);
    if(root == NULL) {
        AppError("cJSON_Parse err, buf:%s", buf);
        goto end;
    }
    if(name1 == NULL) {
        AppError("name1 is null");
        goto end;
    }
    strncpy(name, name1, sizeof(name));
    pSub1 = cJSON_GetObjectItem(root, name);
    if(pSub1 == NULL) {
        //printf("get json null, %s\n", name);
        goto end;
    }
    if(name2 == NULL) {
        pArray = pSub1;
        goto end;
    }
    strncpy(name, name2, sizeof(name));
    pSub2 = cJSON_GetObjectItem(pSub1, name);
    if(pSub2 == NULL) {
        //printf("get json null, %s\n", name);
        goto end;
    }
    if(name3 == NULL) {
        pArray = pSub2;
        goto end;
    }
    strncpy(name, name3, sizeof(name));
    pSub3 = cJSON_GetObjectItem(pSub2, name);
    if(pSub3 == NULL) {
        //printf("get json null, %s\n", name);
        goto end;
    }
    pArray = pSub3;
end:
    std::unique_ptr<char[]> val = nullptr;
    if(pArray != NULL) {
        size = cJSON_GetArraySize(pArray);
        char* tmp = cJSON_Print(pArray);
        int len = strlen(tmp) + 1;
        val = std::make_unique<char[]>(len);
        strcpy(val.get(), tmp);
        free(tmp);
    }
    else {
        //printf("pArray is null\n");
    }
    if(root != NULL) {
        cJSON_Delete(root);
    }
    return val;
}

std::unique_ptr<char[]> GetArrayBufFromFile(const char *filename, int &size, 
                    const char *name1, const char *name2, const char *name3) {
    auto buf = ReadFile2Buf(filename);
    if(buf == nullptr) {
        AppWarn("%s, ReadFile2Buf failed", filename);
        return nullptr;
    }
    return GetArrayBufFromJson(buf.get(), size, name1, name2, name3);
}

std::unique_ptr<char[]> GetBufFromArray(char *buf, int index) {
    cJSON *root, *pSub;
    std::unique_ptr<char[]> val = nullptr;
    root = cJSON_Parse(buf);
    if(root == NULL) {
        AppError("cJSON_Parse err, buf:%s", buf);
        return val;
    }
    int size = cJSON_GetArraySize(root);
    if(index >= 0 && index < size) {
        pSub = cJSON_GetArrayItem(root, index);
        char* tmp = cJSON_Print(pSub);
        int len = strlen(tmp) + 1;
        val = std::make_unique<char[]>(len);
        strcpy(val.get(), tmp);
        free(tmp);
    }
    cJSON_Delete(root);
    return val;
}

std::unique_ptr<char[]> GetObjBufFromJson(char *buf, 
        const char *name1, const char *name2, const char *name3) {
    char name[256];
    char *tmp = NULL;
    cJSON *root, *pSub1, *pSub2, *pSub3;
    root = cJSON_Parse(buf);
    if(root == NULL) {
        AppError("cJSON_Parse err, buf:%s", buf);
        goto end;
    }
    if(name1 == NULL) {
        //printf("get json null, %s\n", name);
        goto end;
    }
    strncpy(name, name1, sizeof(name));
    pSub1 = cJSON_GetObjectItem(root, name);
    if(pSub1 == NULL) {
        //printf("get json null, %s\n", name);
        goto end;
    }
    if(name2 == NULL) {
        tmp = cJSON_Print(pSub1);
        goto end;
    }
    strncpy(name, name2, sizeof(name));
    pSub2 = cJSON_GetObjectItem(pSub1, name);
    if(pSub2 == NULL) {
        //printf("get json null, %s\n", name);
        goto end;
    }
    if(name3 == NULL) {
        tmp = cJSON_Print(pSub2);
        goto end;
    }
    strncpy(name, name3, sizeof(name));
    pSub3 = cJSON_GetObjectItem(pSub2, name);
    if(pSub3 == NULL) {
        //printf("get json null, %s\n", name);
        goto end;
    }
    tmp = cJSON_Print(pSub3);
end:
    std::unique_ptr<char[]> val = nullptr;
    if(tmp != NULL) {
        int len = strlen(tmp) + 1;
        val = std::make_unique<char[]>(len);
        strcpy(val.get(), tmp);
    }
    if(root != NULL) {
        cJSON_Delete(root);
    }
    return val;
}

double GetDoubleValFromJson(char *buf, const char *name1, const char *name2, const char *name3) {
    char name[256];
    double val = -1;
    cJSON *root, *pSub1, *pSub2, *pSub3;

    root = cJSON_Parse(buf);
    if(root == NULL) {
        AppError("parse json err, buf:%s", buf);
        goto end;
    }
    if(name1 == NULL) {
        AppError("name1 is null");
        goto end;
    }
    strncpy(name, name1, 256);
    pSub1 = cJSON_GetObjectItem(root, name);
    if(pSub1 == NULL) {
        //printf("get json null, %s\n", name);
        goto end;
    }
    if(name2 == NULL) {
        val = pSub1->valuedouble;
        goto end;
    }
    strncpy(name, name2, 256);
    pSub2 = cJSON_GetObjectItem(pSub1, name);
    if(pSub2 == NULL) {
        //printf("get json null, %s\n", name);
        goto end;
    }
    if(name3 == NULL) {
        val = pSub2->valuedouble;
        goto end;
    }
    strncpy(name, name3, 256);
    pSub3 = cJSON_GetObjectItem(pSub2, name);
    if(pSub3 == NULL) {
        //printf("get json null, %s\n", name);
        goto end;
    }
    val = pSub3->valuedouble;
end:
    if(root != NULL) {
        cJSON_Delete(root);
    }
    return val;
}

double GetDoubleValFromFile(const char *filename, 
        const char *name1, const char *name2, const char *name3) {
    auto buf = ReadFile2Buf(filename);
    if(buf == nullptr) {
        AppWarn("%s, ReadFile2Buf failed", filename);
        return -1;
    }
    return GetDoubleValFromJson(buf.get(), name1, name2, name3);
}

int GetFileSize(const char *filename) {
    struct stat statbuf;
    stat(filename, &statbuf);
    return statbuf.st_size;
}

std::unique_ptr<char[]> AddStrJson(char *buf, const char *val, 
        const char *name1, const char *name2, const char *name3) {
    const char *name;
    cJSON *pSub = NULL;
    cJSON *root, *pSub1, *pSub2;

    root = cJSON_Parse(buf);
    if(root == NULL) {
        AppError("parse json err, buf:%s", buf);
        goto end;
    }
    if(name1 == NULL) {
        AppError("name1 is null");
        goto end;
    }
    if(name2 == NULL) {
        pSub = root;
        name = name1;
        goto end;
    }
    pSub1 = cJSON_GetObjectItem(root, name1);
    if(pSub1 == NULL) {
        //printf("get json null, %s\n", name1);
        goto end;
    }
    if(name3 == NULL) {
        pSub = pSub1;
        name = name2;
        goto end;
    }
    pSub2 = cJSON_GetObjectItem(pSub1, name2);
    if(pSub2 == NULL) {
        //printf("get json null, %s\n", name2);
        goto end;
    }
    pSub = pSub2;
    name = name3;
end:
    std::unique_ptr<char[]> _val = nullptr;
    if(pSub != NULL) {
        cJSON_AddStringToObject(pSub, name, val);
        char *tmp = cJSON_Print(root);
        _val = std::make_unique<char[]>(strlen(tmp)+1);
        strcpy(_val.get(), tmp);
        free(tmp);
    }
    if(root != NULL) {
        cJSON_Delete(root);
    }
    return _val;
}

std::unique_ptr<char[]> DelJsonObj(char *buf,
        const char *name1, const char *name2, const char *name3) {
    const char *name;
    cJSON *pSub = NULL;
    cJSON *root, *pSub1, *pSub2;

    root = cJSON_Parse(buf);
    if(root == NULL) {
        AppError("parse json err, buf:%s", buf);
        goto end;
    }
    if(name1 == NULL) {
        AppError("name1 is null");
        goto end;
    }
    if(name2 == NULL) {
        pSub = root;
        name = name1;
        goto end;
    }
    pSub1 = cJSON_GetObjectItem(root, name1);
    if(pSub1 == NULL) {
        //printf("get json null, %s\n", name1);
        goto end;
    }
    if(name3 == NULL) {
        pSub = pSub1;
        name = name2;
        goto end;
    }
    pSub2 = cJSON_GetObjectItem(pSub1, name2);
    if(pSub2 == NULL) {
        //printf("get json null, %s\n", name2);
        goto end;
    }
    pSub = pSub2;
    name = name3;
end:
    std::unique_ptr<char[]> _val = nullptr;
    if(pSub != NULL) {
        cJSON_DeleteItemFromObject(pSub, name);
        char *tmp = cJSON_Print(root);
        _val = std::make_unique<char[]>(strlen(tmp)+1);
        strcpy(_val.get(), tmp);
        free(tmp);
    }
    if(root != NULL) {
        cJSON_Delete(root);
    }
    return _val;
}

static int GetNginxPort(char* nginx_config_path, int& port) {
    char buf[256];
    FILE* fp = fopen(nginx_config_path, "rb");
    if(fp == NULL) {
        AppWarn("fopen %s failed", nginx_config_path);
        return -1;
    }
    while(fgets(buf, sizeof(buf), fp)!=NULL) {
        if(strstr(buf, "listen") == NULL) {
            continue;
        }
        sscanf(buf, "%*s%d;", &port);
        break;
    }
    fclose(fp);
    return 0;
}

static int GetNginxRoot(char* nginx_config_path, char* workdir) {
    char buf[URL_LEN];
    FILE* fp = fopen(nginx_config_path, "rb");
    if(fp == NULL) {
        AppWarn("fopen %s failed", nginx_config_path);
        return -1;
    }
    while(fgets(buf, sizeof(buf), fp)!=NULL) {
        if(strstr(buf, "root") == NULL) {
            continue;
        }
        sscanf(buf, "%*s%*[ ]%[^;]", workdir);
        break;
    }
    fclose(fp);
    return 0;
}

void NginxInit(NginxParams& nginx) {
    char cfg_file[512];
    auto nginx_path = GetStrValFromFile(CONFIG_FILE, "system", "nginx_path");
    if(nginx_path == nullptr) {
        nginx.http_port = -1;
        AppWarn("get nginx path failed");
        return;
    }
    //snprintf(cfg_file, sizeof(cfg_file), "%s/conf/servers/hls.conf", nginx_path.get());
    snprintf(cfg_file, sizeof(cfg_file), "%s/conf/nginx.conf", nginx_path.get());
    GetNginxPort(cfg_file, nginx.http_port);
    if(nginx.http_port == 0) {
        nginx.http_port = 8080;
        AppWarn("get nginx http port failed, use default %d", nginx.http_port);
    }
    GetNginxRoot(cfg_file, nginx.workdir);
    if(strlen(nginx.workdir) == 0) {
        strncpy(nginx.workdir, "/home/ubuntu/webdir/webpages", sizeof(nginx.workdir));
        AppWarn("get nginx root path failed, use default %s", nginx.workdir);
    }
}

void HangUp(void) {
    AppWarn("hang up");
    while(1) {
        sleep(10);
    }
}

