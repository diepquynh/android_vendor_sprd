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
#include <fcntl.h>
#include <dirent.h>
#include <sys/poll.h>
#include "common.h"
#include <linux/input.h>

#include "minui.h"

#define MAX_DEVICES 16

static struct pollfd ev_fds[MAX_DEVICES];
static unsigned ev_count = 0;
static char filename[MAX_DEVICES][10]={0};
static int tp_width = 0;
static int tp_height = 0;
int ev_init(void)
{
	DIR *dir;
	struct dirent *de;
	int fd;

	dir = opendir("/dev/input");
	if(dir != 0) {
		while((de = readdir(dir))) {
			//            fprintf(stderr,"/dev/input/%s\n", de->d_name);
			if(strncmp(de->d_name,"event",5)) continue;
			fd = openat(dirfd(dir), de->d_name, O_RDONLY);
			if(fd < 0) continue;
			strcpy(filename[ev_count], de->d_name);
			ev_fds[ev_count].fd = fd;
			ev_fds[ev_count].events = POLLIN;
			ev_count++;
			if(ev_count == MAX_DEVICES) break;
		}
	}
       closedir(dir) ;
       return 0;
}
int get_tp_resolution(char *filename)
{
	int fd;
	struct input_absinfo absinfo_x;
	struct input_absinfo absinfo_y;
	tp_width = gr_fb_width();
	tp_height = gr_fb_height();
	char filepath[30]={0};
	sprintf(filepath,"/dev/input/%s",filename);
	fd = open(filepath, O_RDWR);
	if(fd < 0) {
		printf("can not open dev\n");
		return -1;
	}

	if(ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &absinfo_x)) {
		printf("can not get absinfo\n");
		close(fd);
		return -1;
	}

	if(ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &absinfo_y)) {
		printf("can not get absinfo\n");
		close(fd);
		return -1;
	}
	tp_width = absinfo_x.maximum;
	tp_height = absinfo_y.maximum;
	LOGD("absinfo_x.minimum = %d\nabsinfo_x.maximum = %d\n", absinfo_x.minimum, absinfo_x.maximum);
	LOGD("absinfo_y.minimum = %d\nabsinfo_y.maximum = %d\n", absinfo_y.minimum, absinfo_y.maximum);
	close(fd);
	return 0;
}
void ev_exit(void)
{
	while (ev_count > 0) {
		close(ev_fds[--ev_count].fd);
	}
}

int ev_get(struct input_event *ev, int dont_wait)
{
	int r;
	unsigned n;
	static ev_flag=1;
	do {

		r = poll(ev_fds, ev_count, dont_wait);
		if(r > 0) {
			for(n = 0; n < ev_count; n++) {
				if(ev_fds[n].revents & POLLIN) {
					r = read(ev_fds[n].fd, ev, sizeof(*ev));
					if(r == sizeof(*ev)){
						if(ev->type == EV_ABS) {
							if(ev_flag){
								get_tp_resolution(filename[n]);
								ev_flag=0;
							}
							if(ev->code == ABS_MT_POSITION_X)
								ev->value = ev->value*gr_fb_width()/tp_width;
							if(ev->code == ABS_MT_POSITION_Y)
								ev->value = ev->value*gr_fb_height()/tp_height;
						}
						return 0;
					}
				}
			}
		}
	} while(dont_wait == -1);

	return -1;
}
