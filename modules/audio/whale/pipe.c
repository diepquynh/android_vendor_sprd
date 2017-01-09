/*
* Copyright (C) 2010 The Android Open Source Project
* Copyright (C) 2012-2015, The Linux Foundation. All rights reserved.
*
* Not a Contribution, Apache license notifications and license are retained
* for attribution purposes only.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <cutils/log.h>
#include <fcntl.h>
#include "debug/audio_debug.h"

#define LOG_TAG "audio_hw_pipe"

int SendAudioTestCmd(const char *cmd, int bytes) {
    int fd = -1;
    int ret = -1;
    int bytes_to_read = bytes;
    char *write_ptr=NULL;
    int writebytes=0;
    if (cmd == NULL) {
        return -1;
    }

    if (fd < 0) {
        fd = open(AUDIO_EXT_CONTROL_PIPE, O_WRONLY | O_NONBLOCK);
        if (fd < 0) {
            fd = open(AUDIO_EXT_DATA_CONTROL_PIPE, O_WRONLY | O_NONBLOCK);
        }
    }

    if (fd < 0) {
        return -1;
    } else {
        write_ptr=cmd;
        writebytes=0;
        do {
            writebytes = write(fd, write_ptr, bytes);
            if (ret > 0) {
                if (writebytes <= bytes) {
                    bytes -= writebytes;
                    write_ptr+=writebytes;
                }
            } else if ((!((errno == EAGAIN) || (errno == EINTR))) || (0 == ret)) {
                ALOGE("pipe write error %d, bytes read is %d", errno, bytes_to_read - bytes);
                break;
            } else {
                ALOGD("pipe_write_warning: %d, ret is %d", errno, ret);
            }
        } while (bytes);
    }

    if (fd > 0) {
        close(fd);
    }

    if (bytes == bytes_to_read)
        return ret;
    else
        return (bytes_to_read - bytes);
}
