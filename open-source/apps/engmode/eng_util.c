#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <termios.h>
#include "eng_util.h"

int eng_open_dev(char* dev, int mode)
{
    int fd;
    struct termios ser_settings;

    fd = open(dev, mode);
    if(fd < 0)
        return -1;

    if(isatty(fd)) {
        tcgetattr(fd, &ser_settings);
        cfmakeraw(&ser_settings);
        tcsetattr(fd, TCSANOW, &ser_settings);
    }

    return fd;
}
