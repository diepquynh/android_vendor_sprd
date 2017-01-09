//#include <stdlib.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "engopt.h"


int  eng_open_mode( const char*  pathname, int  options, int  mode )
{
    return 0;
}

int  eng_open( const char*  pathname, int  options )
{
    return 0;
}

int eng_bind(int fd)
{
    return 0;
}

int eng_connect(int fd)
{
    return 0;
}
int eng_listen(int fd)
{
    return 0;
}


int  eng_shutdown(int fd)
{
    int ret = shutdown(fd,SHUT_RDWR);
    return ret;
}

int  eng_close(int fd)
{
    return close(fd);
}

int  eng_read(int  fd, void*  buf, size_t  len)
{
    return read(fd, buf, len);
}

int  eng_write(int  fd, const void*  buf, size_t  len)
{
    return write(fd, buf, len);
}
int   eng_lseek(int  fd, int  pos, int  where)
{
    return lseek(fd, pos, where);
}

int    eng_unlink(const char*  path)
{
    return  unlink(path);
}

int  eng_creat(const char*  path, int  mode)
{
    return 0;
}

int  eng_socket_accept(int  serverfd, struct sockaddr*  addr, socklen_t  *addrlen)
{
    return  accept( serverfd, addr, addrlen );
}

int  eng_thread_create( eng_thread_t  *pthread, eng_thread_func_t  start, void*  arg )
{
    pthread_attr_t   attr;

    pthread_attr_init (&attr);
    //    pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
    return pthread_create( pthread, &attr, start, arg );
}
