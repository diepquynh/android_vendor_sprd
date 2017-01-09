/*
 * Copyright (C) 2007 The Android Open Source Project
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

#include <linux/input.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <linux/input.h>

#include "key_common.h"
#include "debug.h"

extern "C"
{
extern int ev_init(void);
extern int ev_get(struct input_event *ev, unsigned dont_wait);
}
// Key event input queue
static pthread_mutex_t key_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t key_queue_cond = PTHREAD_COND_INITIALIZER;
static int key_queue[256], key_queue_len = 0;
static volatile char key_pressed[KEY_MAX + 1];

extern void touch_handle_input(int, struct input_event*);
// Reads input events, handles special hot keys, and adds to the key queue.
static void *input_thread(void *cookie)
{
    int rel_sum = 0;
    int fake_key = 0;
	int fd = -1;
    for (;;) {
        // wait for the next key event
        struct input_event ev;
        do {
            fd = ev_get(&ev, 0);
			LOGD("eventtype %d,eventcode %d,eventvalue %d",ev.type,ev.code,ev.value);
			if(fd != -1) {
				//touch_handle_input(fd, &ev);
			}
            if (ev.type == EV_SYN) {
                continue;
            } else if (ev.type == EV_REL) {
                if (ev.code == REL_Y) {
                    // accumulate the up or down motion reported by
                    // the trackball.  When it exceeds a threshold
                    // (positive or negative), fake an up/down
                    // key event.
                    rel_sum += ev.value;
                    if (rel_sum > 3) {
                        fake_key = 1;
                        ev.type = EV_KEY;
                        ev.code = KEY_DOWN;
                        ev.value = 1;
                        rel_sum = 0;
                    } else if (rel_sum < -3) {
                        fake_key = 1;
                        ev.type = EV_KEY;
                        ev.code = KEY_UP;
                        ev.value = 1;
                        rel_sum = 0;
                    }
                }
            } else {
                rel_sum = 0;
            }
        }
		while (ev.type != EV_KEY || ev.code > KEY_MAX);
        pthread_mutex_lock(&key_queue_mutex);
        if (!fake_key) {
            // our "fake" keys only report a key-down event (no
            // key-up), so don't record them in the key_pressed
            // table.
            key_pressed[ev.code] = ev.value;
			LOGD("%s: %d\n",__FUNCTION__, ev.value);
        }
        fake_key = 0;
        const int queue_max = sizeof(key_queue) / sizeof(key_queue[0]);
        if (ev.value > 0 && key_queue_len < queue_max && ev.type == EV_KEY && ev.code != 0x14a) {
            key_queue[key_queue_len++] = ev.code;
            pthread_cond_signal(&key_queue_cond);
        }
        pthread_mutex_unlock(&key_queue_mutex);

    }
    return NULL;
}

void key_init(void)
{
    ev_init();

    pthread_t t;
    pthread_create(&t, NULL, input_thread, NULL);
}

int key_pass_or_fail(int key)
{
	if(key == 114)
		return RL_PASS;
	else if (key == 116)
		return RL_FAIL;
	else
		return RL_NA;
}

int pass_or_fail(void)
{
	int key = -1;
	key = ui_wait_key_simp();
	return key_pass_or_fail(key);
}

int ui_wait_key_simp(void)
{
	struct timespec ntime;
	ntime.tv_sec = time(NULL)+ 10;
	ntime.tv_nsec = 0;

	return ui_wait_key(&ntime);
}

int ui_wait_key_sec(int second)
{
	struct timespec ntime;
	ntime.tv_sec = time(NULL)+ second;
	ntime.tv_nsec = 0;

	return ui_wait_key(&ntime);
}

int ui_wait_key(struct timespec *ntime)
{
    int ret=0;
    int key=-1;

    pthread_mutex_lock(&key_queue_mutex);
	while (key_queue_len == 0) {
		ret = pthread_cond_timedwait(&key_queue_cond, &key_queue_mutex,ntime);
		if(ret == ETIMEDOUT) {
			break;
		}
	}

    if (0 == ret) {
		key = key_queue[0];
		memcpy(&key_queue[0], &key_queue[1], sizeof(int) * --key_queue_len);
    } else
		key = -1;
    pthread_mutex_unlock(&key_queue_mutex);
    LOGD("[%s]: key=%d,ret=%d\n", __FUNCTION__, key, ret);
    return key;
}

int ui_key_pressed(int key)
{
    // This is a volatile static array, don't bother locking
    return key_pressed[key];
}

void ui_clear_key_queue()
{
    pthread_mutex_lock(&key_queue_mutex);
    key_queue_len = 0;
    pthread_mutex_unlock(&key_queue_mutex);
}

int ui_read_key(void)
{
	int key;
	if(0 == key_queue_len)
		return -1;
	pthread_mutex_lock(&key_queue_mutex);
	key = key_queue[0];
	memcpy(&key_queue[0], &key_queue[1], sizeof(int) * --key_queue_len);
	pthread_mutex_unlock(&key_queue_mutex);
	return key;
}