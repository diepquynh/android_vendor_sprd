#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "common.h"

static int log_fd = -1;
void log_init(void)
{
	static const char *name = "/dev/__kmsg__";
	int val;
	if (mknod(name, S_IFCHR | 0600, (1 << 8) | 11) == 0) {
		log_fd = open(name, O_WRONLY);
		if(log_fd < 0){
			LOGE("%s: open file/dev/__kmsg__ fail.\n",__func__);
			return;
		}
		val=fcntl(log_fd, F_SETFD, FD_CLOEXEC);
		if (val == -1) {
			LOGE("%s: file lock fails.\n",__func__);
			close(log_fd);
			log_fd =-1;
			unlink(name);
			return;
	}
		unlink(name);
	}
}

#define LOG_BUF_MAX 512
#define LOG_LEVEL 4

void log_write(int level, const char *fmt, ...)
{
	char buf[LOG_BUF_MAX];
	va_list ap;

	if(level > LOG_LEVEL) return;

	if (log_fd < 0) return;

	va_start(ap, fmt);
	vsnprintf(buf, LOG_BUF_MAX, fmt, ap);
	buf[LOG_BUF_MAX - 1] = 0;
	va_end(ap);
	write(log_fd, buf, strlen(buf));
}
