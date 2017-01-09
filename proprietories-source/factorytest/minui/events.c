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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/poll.h>

#include <linux/input.h>
#include "minui.h"

#define MAX_DEVICES 16

static struct pollfd ev_fds[MAX_DEVICES];
static unsigned ev_count = 0;

struct dirent *testde;

int ev_init(void)
{
    DIR *dir;
    struct dirent *de;
    int fd;

    dir = opendir("/dev/input");
    testde=readdir(dir);
    if(dir != 0) {
        while((de = readdir(dir))) {
            if(strncmp(de->d_name,"event",5)) continue;
            fd = openat(dirfd(dir), de->d_name, O_RDONLY);
		if(fd < 0) continue;
            //LOGD("ev_fds[ev_count].fd = %d", fd);
            ev_fds[ev_count].fd = fd;
            ev_fds[ev_count].events = POLLIN;
            ev_count++;
            if(ev_count == MAX_DEVICES) break;
        }
    }

    return 0;
}

void ev_exit(void)
{
    while (ev_count > 0) {
        close(ev_fds[--ev_count].fd);
    }
}

int ev_get(struct input_event *ev, unsigned dont_wait)
{
	int r;
	unsigned n;
	do {
		r = poll(ev_fds, ev_count, dont_wait ? 0 : -1);
		if(r > 0) {
			for(n = 0; n < ev_count; n++) {
				if(ev_fds[n].revents & POLLIN) {
					r = read(ev_fds[n].fd, ev, sizeof(*ev));
					if(r == sizeof(*ev)){
							LOGV("touchevent ev_fds[%d].fd=%d", n,ev_fds[n].fd);
							return ev_fds[n].fd;
					}
				}
			}
		}
	} while(dont_wait == 0);

	return -1;
}
