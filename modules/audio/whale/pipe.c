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

    if (cmd == NULL) {
        return -1;
    }

    if (fd < 0) {
        fd = open(AUDIO_EXT_CONTROL_PIPE, O_WRONLY | O_NONBLOCK);
    }

    if (fd < 0) {
        return -1;
    } else {
        do {
            ret = write(fd, cmd, bytes);
            if (ret > 0) {
                if (ret <= bytes) {
                    bytes -= ret;
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
