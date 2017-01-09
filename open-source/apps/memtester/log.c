#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cutils/sockets.h>
#include <ctype.h>
#include <pthread.h>
#include <dirent.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <cutils/properties.h>
#include <utils/Log.h>
#include <signal.h>
#include <sys/time.h>
#include <fcntl.h>
#include "log.h"

int soc_fd = -1;

void ConnectSrv()
{
	int iCount = 10;
	MEMTEST_LOGD("ConnectSrv");
	soc_fd = socket_local_client(socket_name,
        ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);

    while(soc_fd < 0 && iCount-- >= 0)
    {
    	printf("create socket fail! repeat %d\n", iCount);

        usleep(100*1000);
        soc_fd = socket_local_client(socket_name,
            ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    }

    if (iCount <= 0) {
    	printf("connect srtmemtest fail!\n");
    }
}

void DisconnectSrv()
{
	if (soc_fd > 0) {
		close(soc_fd);
	}
}

int my_printf(const char *fmt, ...)
{
    char printf_buf[1024];
    va_list args;
    int printed;

    va_start(args, fmt);
    printed = vsprintf(printf_buf, fmt, args);
    va_end(args);

    soc_write(TAG_NORMAL, printf_buf);

    return printed;
}

int my_printf_tag(char *tag, const char *fmt, ...)
{
    char printf_buf[1024];
    va_list args;
    int printed;

    va_start(args, fmt);
    printed = vsprintf(printf_buf, fmt, args);
    va_end(args);

    soc_write(tag, printf_buf);

    return printed;
}

int my_fprint_f(int fd, const char *fmt, ...)
{
    char printf_buf[1024];
    va_list args;
    int printed;

    va_start(args, fmt);
    printed = vsprintf(printf_buf, fmt, args);
    va_end(args);

    soc_write(TAG_NORMAL, printf_buf);

    return printed;    
}

int my_fprint_f_tag(int fd, char *tag, const char *fmt, ...)
{
    char printf_buf[1024];
    va_list args;
    int printed;

    va_start(args, fmt);
    printed = vsprintf(printf_buf, fmt, args);
    va_end(args);

    soc_write(tag, printf_buf);

    return printed;
}

void soc_write(char *tag, char *format)
{
	char printf_buf[1024];
    int retNum = 0, outNum = 0;

    MEMTEST_LOGD("%s", format);

    if (format == NULL) return ;

    memset(printf_buf, 0, sizeof(printf_buf));
    strcat(printf_buf, tag);
    strcat(printf_buf, format);

    if (soc_fd <= 0 )
    {
    	printf("%s", format);
        return ;
    }

    outNum = strlen(printf_buf);
    retNum = write(soc_fd, printf_buf, outNum);
    if(retNum != outNum) {
        MEMTEST_LOGD("fail to write: retNum = %d, outNum = %d", retNum, outNum);
    }
}

