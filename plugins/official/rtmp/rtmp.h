#ifndef __IVASDK_RTMP_H__
#define __IVASDK_RTMP_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "obj.h"
#include "share.h"
#include "log.h"

class Rtmp : public Object {
public:
    Rtmp(MediaServer* _media):Object(_media) {
        memset(lib, 0, sizeof(lib));
    }
    ~Rtmp(void) {}
    virtual char* GetPath(char* path) {
        auto name = GetStrValFromFile(path, "rtmp", "lib");
        if(name == nullptr) {
            AppWarn("get path failed, %s", path);
            return lib;
        }
        strncpy(lib, name.get(), sizeof(lib));
        return lib;
    }
private:
    char lib[256];
};

#endif
