/*copyright (C) 2010 The Android Open Source Project
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


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <limits.h>

#include <linux/types.h>
#include <linux/ioctl.h>
#define __force
#define __bitwise
#define __user
#include <sound/asound.h>
#include "sound/compress_params.h"
#include "sound/compress_offload.h"
#include "tinycompress/tinycompress.h"



/**
macro define
**/
#define COMPR_ERR_MAX 128


/**
extern function
**/
extern int is_compress_ready(struct compress *compress);

/*Attention : the struct compress with tinycompress need keep same*/
struct compress {
    int fd;
    unsigned int flags;
    char error[COMPR_ERR_MAX];
    struct compr_config *config;
    int running;
    int max_poll_wait_ms;
    int nonblocking;
    unsigned int gapless_metadata;
    unsigned int next_track;
};

static int oops(struct compress *compress, int e, const char *fmt, ...)
{
    va_list ap;
    int sz;

    va_start(ap, fmt);
    vsnprintf(compress->error, COMPR_ERR_MAX, fmt, ap);
    va_end(ap);
    sz = strlen(compress->error);

    snprintf(compress->error + sz, COMPR_ERR_MAX - sz,
        ": %s", strerror(e));
    errno = e;

    return -1;
}

/*recomplete for compress_write*/
int offload_write(struct compress *compress, const void *buf, unsigned int size)
{
    if (NULL == compress || NULL == buf) {
        return -1;
    }

    struct snd_compr_avail avail;
    struct pollfd fds;
    int to_write = 0;   /* zero indicates we haven't written yet */
    int written, total = 0, ret;
    const char* cbuf = buf;
    const unsigned int frag_size = compress->config->fragment_size;

    if (!(compress->flags & COMPRESS_IN))
        return oops(compress, EINVAL, "Invalid flag set");
    if (!is_compress_ready(compress))
        return oops(compress, ENODEV, "device not ready");
    fds.fd = compress->fd;
    fds.events = POLLOUT;

    /*TODO: treat auto start here first */
    while (size) {
        if (ioctl(compress->fd, SNDRV_COMPRESS_AVAIL, &avail))
            return oops(compress, errno, "cannot get avail");

        /* We can write if we have at least one fragment available
         * or there is enough space for all remaining data
         */
        if (compress->nonblocking) {
            if(avail.avail == 0)
                return total;
        } else if ((avail.avail < frag_size) && (avail.avail < size)) {
            if (compress->nonblocking)
                return total;

            ret = poll(&fds, 1, compress->max_poll_wait_ms);
            if (fds.revents & POLLERR) {
                return oops(compress, EIO, "poll returned error!");
            }
            /* A pause will cause -EBADFD or zero.
             * This is not an error, just stop writing */
            if ((ret == 0) || (ret == -EBADFD))
                break;
            if (ret < 0)
                return oops(compress, errno, "poll error");
            if (fds.revents & POLLOUT) {
                continue;
            }
        }
        /* write avail bytes */
        if (size > avail.avail)
            to_write =  avail.avail;
        else
            to_write = size;
        written = write(compress->fd, cbuf, to_write);
        /* If play was paused the write returns -EBADFD */
        if (written == -EBADFD)
            break;
        if (written < 0)
            return oops(compress, errno, "write failed!");

        size -= written;
        cbuf += written;
        total += written;
    }
    return total;
}
