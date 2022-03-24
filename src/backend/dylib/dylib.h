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

#ifndef __AISTREAM_DYLIB_H__
#define __AISTREAM_DYLIB_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tensor.h"
#include "framework.h"

typedef int (*DLReg)(DLRegister**, int&);
typedef int (*DLInit)(ElementData*, char*);
typedef IHandle (*DLStart)(int, char*);
typedef int (*DLProcess)(IHandle, TensorData*);
typedef int (*DLStop)(IHandle);
typedef int (*DLNotify)(IHandle);
typedef int (*DLRelease)(void);

class DynamicLib : public Framework {
public:
    DynamicLib(void);
    ~DynamicLib(void);
    virtual int Init(char* path, ElementData* data, char* _params = NULL);
    virtual int Start(int channel, char* _params = NULL);
    virtual int Process(TensorData* data);
    virtual int Stop(void);
    virtual int Notify(void);
    virtual int Release(void);
private:
    DLRegister *params;
    void* dlhandle;
    IHandle handle;
    DLProcess process;
};

#endif

