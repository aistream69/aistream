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

#ifndef __AISTREAM_TASK_H__
#define __AISTREAM_TASK_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class TaskParams {
public:
    TaskParams(void);
    ~TaskParams(void);
    void SetTaskName(char *_name) {strncpy(name, _name, sizeof(name));}
    char *GetTaskName(void) {return name;}
private:
    char name[256];
};

#endif

