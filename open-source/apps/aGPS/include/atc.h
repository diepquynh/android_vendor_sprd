/*
 *  AT Channel
 *
 *  Copyright (C) 2013 Spreadtrum Communications Inc.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef __ATC_H
#define __ATC_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include <parser.h>
#ifdef ANDROID
#define LOG_TAG "ATDistributer"
#include <cutils/log.h>
#define dprintf(format, ...) \
	if (debug_mode) ALOGD(format, ##__VA_ARGS__)
#else
#define dprintf(format, ...) \
	if (debug_mode) printf(format, ##__VA_ARGS__)
#endif
#define MAX_AT_RESPONSE (8 * 1024)
//#define DEBUG_SIPC

/** a singly-lined list of intermediate responses */
typedef struct ATLine  {
	struct ATLine *p_next;
	char *line;
} ATLine;

/** Free this with at_response_free() */
typedef struct {
	int success;              /* true if final response indicates
								 success (eg "OK") */
	char *finalResponse;      /* eg OK, ERROR */
	ATLine  *p_intermediates; /* any intermediate responses */
} ATResponse;

#define mmalloc malloc
typedef struct work_item_tag {
	struct work_item_tag *next;
	void *data;
	struct timeval time_queued;
} work_item_t;

typedef struct work_queue_tag {
	work_item_t *head;
	work_item_t *tail;
	int item_count;
} work_queue_t;

struct at_channel {
#ifdef DEBUG_SIPC
	int dfd;
	char sipc_buf[1024*8];
#endif
	int fd;
	work_queue_t *queue; /* For unsolicited & async response */
	pthread_mutex_t queue_mutex;
	pthread_mutex_t write_mutex;
	pthread_mutex_t s_command_mutex;
	pthread_mutex_t response_cond_mutex;
	pthread_cond_t  response_cond;
	ATResponse atrep;
	const char *name;
	char s_ATBuffer[MAX_AT_RESPONSE + 1];
	char *s_ATBufferCur;
};

typedef int (*at_response_cb)(const char *at, int len);
struct at_unso_node {
	char *prefix;
	at_response_cb cb;
};

#define NUM_ELEMS(x) (sizeof(x) / sizeof(x[0]))
#define ATC_DEVICE_DEFAULT "/dev/stty_w21"
extern int debug_mode;
struct at_channel *atc_init(const char *dev, int mode, int fd);
int atc_send(struct at_channel *atc, char *at, size_t len, int sync, int timeout_ms/* millisecond */);
int atc_register_unsolicited(struct at_channel *atc, char *prefix_unso, at_response_cb cb);
#endif
