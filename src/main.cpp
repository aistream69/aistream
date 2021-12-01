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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "stream.h"

static void MainProcess(void) {
    pid_t pid;
    pid = fork();
    if(pid == -1) {
        AppError("fork failed");
        exit(-1);
    }
    else if(pid == 0) {
        MediaServer *media = new MediaServer;
        media->run();
        AppDebug("run ok");
        delete media;
        exit(0);
    }
}

static void SigHandler(const int signal) {
    int status;

    int quit_pid = wait(&status);
    if(quit_pid == -1) {
        printf("recv quit signal -1(%s), ignore it\n", strerror(errno));
        return;
    }
    if(WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        return;
    }
    AppWarning("recv quit pid %d(%s), restart it", quit_pid, strerror(errno));
    MainProcess();

    return;
}

int main(int argc, char *argv[]) {
    signal(SIGCHLD, SigHandler);
    MainProcess();
    while(1) {
        sleep(60);
    }
    return 0;
}

