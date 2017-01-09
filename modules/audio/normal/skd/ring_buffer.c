/**@Code written in the kernel kfifo
 *
*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <cutils/log.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ring_buffer.h"
#define LOG_TAG "audio_hw_ring_buffer"
/*****************************************************************
    function     :__ring_buffer_len
    function     :get available buffer size
    author       :sprd
    date         :2015-11-23
*****************************************************************/
static uint32_t __ring_buffer_len(const struct ring_buffer *ring_buf)
{
    return (ring_buf->in - ring_buf->out);
}

/*****************************************************************
    function     :__ring_buffer_get
    function     :get data form ring buffer
    author       :sprd
    date         :2015-11-23
*****************************************************************/
static uint32_t __ring_buffer_get(struct ring_buffer *ring_buf, void *buffer,
                                  uint32_t size)
{
    uint32_t len = 0;
    size  = min(size, ring_buf->in - ring_buf->out);
    /* first get the data from fifo->out until the end of the buffer */
    len = min(size, ring_buf->size - (ring_buf->out & (ring_buf->size - 1)));
    memcpy(buffer, ring_buf->buffer + (ring_buf->out & (ring_buf->size - 1)),
           len);
    /* then get the rest (if any) from the beginning of the buffer */
    memcpy(buffer + len, ring_buf->buffer, size - len);
    ring_buf->out += size;
    ALOGD("__ring_buffer_get in:0x%x out:0x%x",ring_buf->in,ring_buf->out);
    return size;
}

/*****************************************************************
    function     :__ring_buffer_put
    function     :write data to ring buffer
    author       :sprd
    date         :2015-11-23
*****************************************************************/
static uint32_t __ring_buffer_put(struct ring_buffer *ring_buf, void *buffer,
                                  uint32_t size)
{
    uint32_t len = 0;
    size = min(size, ring_buf->size - ring_buf->in + ring_buf->out);
    /* first put the data starting from fifo->in to buffer end */
    len  = min(size, ring_buf->size - (ring_buf->in & (ring_buf->size - 1)));
    memcpy(ring_buf->buffer + (ring_buf->in & (ring_buf->size - 1)), buffer,
           len);
    /* then put the rest (if any) at the beginning of the buffer */
    memcpy(ring_buf->buffer, buffer + len, size - len);
    ring_buf->in += size;
    ALOGD("__ring_buffer_put in:0x%x out:0x%x",ring_buf->in,ring_buf->out);
    return size;
}

/*****************************************************************
    function     :ring_buffer_len
    function     :call __ring_buffer_len to get available buffer size
    author       :sprd
    date         :2015-11-23
*****************************************************************/
uint32_t ring_buffer_len(const struct ring_buffer *ring_buf)
{
    uint32_t len = 0;
    pthread_mutex_lock(&(ring_buf->lock));
    len = __ring_buffer_len(ring_buf);
    pthread_mutex_unlock(&(ring_buf->lock));
    return len;
}

uint32_t ring_buffer_get(struct ring_buffer *ring_buf, void *buffer, uint32_t
                         size)
{
    uint32_t ret;
    pthread_mutex_lock(&(ring_buf->lock));
    ret = __ring_buffer_get(ring_buf, buffer, size);

    if (ring_buf->in == ring_buf->out)
    { ring_buf->in = ring_buf->out = 0; }
    pthread_mutex_unlock(&(ring_buf->lock));
    return ret;
}

/*****************************************************************
    function     :ring_buffer_put
    function     :call __ring_buffer_put to write data to ring buffer
    author       :sprd
    date         :2015-11-23
*****************************************************************/
uint32_t ring_buffer_put(struct ring_buffer *ring_buf, void *buffer, uint32_t
                         size)
{
    uint32_t ret;
    pthread_mutex_lock(&(ring_buf->lock));
    ret = __ring_buffer_put(ring_buf, buffer, size);
    pthread_mutex_unlock(&(ring_buf->lock));
    return ret;
}

/*****************************************************************
    function     :ring_buffer_init
    function     :init ring buffer
    author       :sprd
    date         :2015-11-23
*****************************************************************/
struct ring_buffer *ring_buffer_init(uint32_t size,int zero_size)
{
    void *buffer = NULL;
    struct ring_buffer *ring_buf = NULL;
    if (!is_power_of_2(size)) {
        ALOGE("size must be power of 2 size:%d zero_size:%d\n",size,zero_size);
        return ring_buf;
    }

    if(zero_size>=size){
        ALOGE("zero_size(0x%x) need < size(0x%x)",zero_size,size);
        return ring_buf;
    }

    ALOGI("ring_buffer_init size:0x%x zero_size:0x%x",size,zero_size);

    buffer = (void *)malloc(size);
    if (!buffer) {
        ALOGE("Failed to malloc memory");
        goto err;
    }
    memset(buffer, 0, size);

    ring_buf = (struct ring_buffer *)malloc(sizeof(struct ring_buffer));
    if (!ring_buf) {
        ALOGE("Failed to malloc ring_buffer,errno:%u,%s",
              errno, strerror(errno));
        goto err;
    }
    memset(ring_buf, 0, sizeof(struct ring_buffer));
    ring_buf->buffer = buffer;
    ring_buf->size = size;
    ring_buf->in = zero_size;
    ring_buf->out = 0;
    if (pthread_mutex_init(&(ring_buf->lock), NULL) != 0) {
        ALOGE("Failed pthread_mutex_init,errno:%u,%s",
              errno, strerror(errno));
        goto err;
    }
    return ring_buf;

err:
    ring_buffer_free(ring_buf);
    return NULL;
}

/*****************************************************************
    function     :ring_buffer_free
    function     :free ring buffer
    author       :sprd
    date         :2015-11-23
*****************************************************************/
void ring_buffer_free(struct ring_buffer *ring_buf)
{
    if (NULL != ring_buf) {
        if (ring_buf->buffer) {
            free(ring_buf->buffer);
            ring_buf->buffer = NULL;
        }
        free(ring_buf);
        ring_buf = NULL;
    }
}
