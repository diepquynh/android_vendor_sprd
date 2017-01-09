/**@Code written in the kernel kfifo
 *
*/
#ifndef RING_BUFFER_H
#define RING_BUFFER_H
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <pthread.h>

#define is_power_of_2(x) ((x) != 0 && (((x) & ((x) - 1)) == 0))
#define min(a, b) (((a) < (b)) ? (a) : (b))

struct ring_buffer {
    void         *buffer;
    uint32_t     size;
    uint32_t     in;
    uint32_t     out;
    pthread_mutex_t lock;
};

uint32_t ring_buffer_len(const struct ring_buffer *ring_buf);
uint32_t ring_buffer_get(struct ring_buffer *ring_buf, void *buffer, uint32_t size);
uint32_t ring_buffer_put(struct ring_buffer *ring_buf, void *buffer, uint32_t size);
struct ring_buffer *ring_buffer_init(uint32_t size,int zero_size);
void ring_buffer_free(struct ring_buffer *ring_buf);
#endif
