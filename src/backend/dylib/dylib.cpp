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
#include <dlfcn.h>
#include "dylib.h"
#include "tensor.h"
#include "log.h"

DynamicLib::DynamicLib(void) {
    params = NULL;
    handle = NULL;
    dlhandle = NULL;
    process = NULL;
}

DynamicLib::~DynamicLib(void) {
    if(params != NULL) {
        free(params);
    }
}

int DynamicLib::Init(char* path, ElementData* data) {
    DLReg reg;
    char *error;
    dlhandle = dlopen(path, RTLD_LAZY);
    if(!dlhandle) {
        AppWarn("open %s failed, %s", path, dlerror());
        return -1;
    }
    dlerror();

    reg = (DLReg)dlsym(dlhandle, "DylibRegister");
    if((error = dlerror()) != NULL) {
        AppWarn("get func failed, %s", error);
        return -1;
    }
    int size = 0;
    reg(&params, size);
    if(params == NULL) {
        AppWarn("dl register failed, %s", path);
        return -1;
    }
    //AppDebug("dl register success, path:%s", path);

    DLInit init = (DLInit)dlsym(dlhandle, params->init);
    if((error = dlerror()) != NULL) {
        AppWarn("get func %s failed, %s", params->init, error);
        return -1;
    }
    init(data);

    return 0;
}

int DynamicLib::Start(int channel, char* ele_params) {
    char *error;

    if(params == NULL || dlhandle == NULL) {
        return -1;
    }
    DLStart start = (DLStart)dlsym(dlhandle, params->start);
    if((error = dlerror()) != NULL) {
        AppWarn("get func %s failed, %s", params->start, error);
        return -1;
    }
    handle = start(channel, ele_params);
    if(handle == NULL) {
        AppWarn("dl start failed, %d, %s", channel, params->start);
        return -1;
    }
    process = (DLProcess)dlsym(dlhandle, params->process);
    if((error = dlerror()) != NULL) {
        AppWarn("get func %s failed, %s", params->process, error);
        return -1;
    }

    return 0;
}

int DynamicLib::Process(TensorData* data) {
    process(handle, data);
    return 0;
}

int DynamicLib::Stop(void) {
    char *error;

    if(params == NULL || dlhandle == NULL) {
        return -1;
    }
    DLStop stop = (DLStop)dlsym(dlhandle, params->stop);
    if((error = dlerror()) != NULL) {
        AppWarn("get func %s failed, %s", params->stop, error);
        return -1;
    }
    stop(handle);
    handle = NULL;

    return 0;
}

int DynamicLib::Release(void) {
    char *error;

    if(params == NULL || dlhandle == NULL) {
        return -1;
    }
    DLRelease release = (DLRelease)dlsym(dlhandle, params->release);
    if((error = dlerror()) != NULL) {
        AppWarn("get func %s failed, %s", params->release, error);
        return -1;
    }
    release();
    if(dlhandle != NULL) {
        dlclose(dlhandle);
        dlhandle = NULL;
    }

    return 0;
}

