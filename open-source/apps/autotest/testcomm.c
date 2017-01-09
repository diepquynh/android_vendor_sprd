//#include "testitem.h"
#include <linux/input.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <getopt.h>

int find_input_dev(int mode, const char *event_name)
{
	int fd = -1;
	int ret = -1;
	const char *dirname = "/dev/input";
	char devname[PATH_MAX];
	char *filename;
	DIR *dir;
	struct dirent *de;
	char name[128];

	dir = opendir(dirname);
	if (dir == NULL) {
		return -1;
	}

	strcpy(devname, dirname);
	filename = devname + strlen(devname);
	*filename++ = '/';

	while ((de = readdir(dir))) {
		if ((de->d_name[0] == '.' && de->d_name[1] == '\0') ||
				(de->d_name[0] == '.' && de->d_name[1] == '.'  &&
				 de->d_name[2] == '\0')) {
			/* ignore .(current) and ..(top) directory */
			continue;
		}
		strcpy(filename, de->d_name);
		fd = open(devname, mode);

		if (fd >= 0) {
			memset(name, 0, sizeof(name));
			if (ioctl(fd, EVIOCGNAME(sizeof(name) - 1), &name) < 0) {

			} else {
				if (!strcmp(name, event_name)) {
					ret = fd; //get the sensor name from the event
					goto END;
				}
			}
			close(fd);
		}
	}
END:
	closedir(dir);

	return ret;
}

