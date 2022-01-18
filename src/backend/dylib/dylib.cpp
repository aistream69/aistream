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

typedef int (*DLReg)(DLRegister**, int&);
typedef int (*DLInit)(void);
typedef IHandle (*DLStart)(int);
typedef int (*DLProcess)(IHandle, TensorData*);
typedef int (*DLStop)(IHandle);
typedef int (*DLRelease)(void);

DynamicLib::DynamicLib(void) {
}

DynamicLib::~DynamicLib(void) {
}

int DynamicLib::Init(void) {
    char *error;
    DLReg reg;
    const char* path = "./plugins/official/libpreview.so";
    void* handle = dlopen(path, RTLD_LAZY);
    if(!handle) {
        AppWarn("open %s failed, %s", path, dlerror());
        return -1;
    }
    dlerror();

    reg = (DLReg)dlsym(handle, "DylibRegister");
    if((error = dlerror()) != NULL) {
        AppWarn("get func failed, %s", error);
        return -1;
    }
    int size = 0;
    DLRegister *params = NULL;
    reg(&params, size);
    if(params == NULL) {
        AppWarn("dl register failed, %s", path);
        return -1;
    }
    AppDebug("dl register success, size:%d", size);
    for(int i = 0; i < size; i ++) {
        AppDebug("i:%d, init:%s,start:%s,process:%s", i, params->init, params->start, params->process);
    }

    DLInit init;
    init = (DLInit)dlsym(handle, params->init);
    if((error = dlerror()) != NULL) {
        AppWarn("get func %s failed, %s", params->init, error);
        return -1;
    }
    init();
    free(params);
    return 0;
}

int DynamicLib::Start(char* path) {
    AppDebug("##test");
    return 0;
}

int DynamicLib::Process(void) {
    AppDebug("##test");
    // get input , notify
    // process
    // copy to output
    return 0;
}

int DynamicLib::Stop(void) {
    return 0;
}

int DynamicLib::Release(void) {
    return 0;
}

