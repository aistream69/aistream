#ifndef __AISTREAM_H__
#define __AISTREAM_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONFIG_FILE     "config.json"
#define SW_VERSION      "V0.01.2021112201"

typedef void* Params;

typedef struct {
    Params stream;      // AiStream
} MasterParams;
typedef struct {
    Params stream;      // AiStream
} SlaveParams;
typedef struct {
    Params stream;      // AiStream
} RestParams;
typedef struct {
    Params stream;      // AiStream
} DbParams;
typedef struct {
    Params stream;      // AiStream
} OutputParams;

typedef struct {
    Params stream;      // AiStream
} SystemParams;

typedef struct {
    Params stream;      // AiStream
} ConfigParams;

typedef struct {
    Params master;      // MasterParams
    Params slave;       // SlaveParams
    Params rest;        // RestParams
    Params objs;        // ObjParams
    Params tasks;       // TaskParams
    Params config;      // ConfigParams
    Params db;          // DbParams
    Params output;      // OutputParams
    Params system;      // SystemParams
    int running;
} AiStream;

#endif

