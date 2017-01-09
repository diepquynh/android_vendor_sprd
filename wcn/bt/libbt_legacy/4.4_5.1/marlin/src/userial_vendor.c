/*
 * Copyright 2012 The Android Open Source Project
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

/******************************************************************************
 *
 *  Filename:      userial_vendor.c
 *
 *  Description:   Contains vendor-specific userial functions
 *
 ******************************************************************************/

#define LOG_TAG "bt_userial_vendor"

#include <utils/Log.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include "bt_vendor_sprd.h"
#include "userial_vendor.h"

/*uart*/
static char s_bt_dev_smd[] =  "/dev/ttyS0";

int bt_hci_init_transport (int *bt_fd)
{
    struct termios   term;
    uint32_t baud;
    int fd = -1;
    int retry = 0;

    fd = open("/dev/ttyS0", (O_RDWR | O_NOCTTY));
    ALOGD("open /dev/ttyS0");
    baud = B3000000;


    if (-1 == fd)
    {
        ALOGE("init_transport: Cannot open %s: %s\n",s_bt_dev_smd,strerror(errno));
        return -1;
    }

    if (tcflush(fd, TCIOFLUSH) < 0)
    {
        ALOGE("init_uart: Cannot flush sttybt0\n");
        close(fd);
        return -1;
    }

    if (tcgetattr(fd, &term) < 0)
    {
        ALOGE("init_uart: Error while getting attributes\n");
        close(fd);
        return -1;
    }

    cfmakeraw(&term);

    /* JN: Do I need to make flow control configurable, since 4020 cannot
    * disable it?
    */
    term.c_cflag |= (CRTSCTS | CLOCAL | CREAD);
    term.c_iflag &= ~(IXOFF);
    if (tcsetattr(fd, TCSANOW, &term) < 0)
    {
        ALOGE("init_uart: Error while getting attributes\n");
        close(fd);
        return -1;
    }

    /* set input/output baudrate */
    cfsetospeed(&term, baud);
    cfsetispeed(&term, baud);
    if (tcsetattr(fd, TCSANOW, &term) < 0)
    {
        ALOGE("init_uart: Error while setting attributes baud \n");
        close(fd);
        return -1;
    }
    ALOGI("Done intiailizing UART\n");

    *bt_fd = fd;
    return 0;
}

int bt_hci_deinit_transport(int bt_fd)
{
     struct termios   term;
     if(bt_fd == -1)
       return TRUE;
     if (tcgetattr(bt_fd, &term) < 0)
     {
        ALOGE("close_uart: Error while getting attributes\n");
        close(bt_fd);
        return -1;
     }
     term.c_cflag &= ~(CRTSCTS);
     if (tcsetattr(bt_fd, TCSANOW, &term) < 0)
     {
         ALOGE("close_uart: Error while getting attributes\n");
         close(bt_fd);
         return -1;
     }
     close(bt_fd);
     ALOGE("UART CLOSE success \n");
     return TRUE;
}
