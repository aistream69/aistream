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

#ifndef __AISTREAM_SHARE_H__
#define __AISTREAM_SHARE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    void *arga;
    void *argb;
    void *argc;
    void *argd;
    void *arge;
    int  val;
} CommonParams;

int DirCheck(const char *dir);
int GetLocalIp(char host_ip[128]);
char *ReadFile2Buf(const char *filename);
int ReadFile(const char *filename, void *buf, int size);
int ReadFile2(const char *filename, void *buf, int max);
int GetIntValFromJson(char *buf, const char *name1, const char *name2=NULL, const char *name3=NULL);

#endif

