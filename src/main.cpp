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
#include "stream.h"

static void MainProcess(void) {
    pid_t pid;
    pid = fork();
    if(pid == -1) {
        AppError("fork failed");
        exit(-1);
    }
    else if(pid == 0) {
        MediaServer *media = new MediaServer();
        media->Run();
        AppDebug("run ok");
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
    if(errno == ENOENT) {
        AppWarn("recv quit pid %d(%s), ignore it", quit_pid, strerror(errno));
        return;
    }
    AppWarn("recv quit pid %d(%s), restart it", quit_pid, strerror(errno));
    MainProcess();

    return;
}

int main(int argc, char *argv[]) {
    DirCheck("log");
    AppDebug("Built:%s %s, version:%s, aistream start ...", __TIME__, __DATE__, SW_VERSION);
    signal(SIGCHLD, SigHandler);
    MainProcess();
    while(1) {
        if(!access("aistream.stop", F_OK)) {
            break;
        }
        sleep(2);
    }
    return 0;
}

