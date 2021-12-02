/******************************************************************************
 * Copyright (C) 2021 aistream <aistream@yeah.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#ifndef __AISTREAM_H__
#define __AISTREAM_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "master.h"
#include "slave.h"
#include "config.h"
#include "db.h"
#include "obj.h"
#include "rest.h"
#include "pipeline.h"
#include "db.h"
#include "obj.h"
#include "output.h"
#include "task.h"
#include "share.h"
#include "log.h"

#define CONFIG_FILE     "cfg/config.json"
#define SW_VERSION      "V0.01.2021112201"

class MediaServer {
public:
    MediaServer(void);
    ~MediaServer(void);
    void run(void);
    ConfigParams*   config;
    ObjParams*      objs;
    int             running;
private:
    MasterParams*   master;
    SlaveParams*    slave;
    DbParams*       db;
};

#endif

