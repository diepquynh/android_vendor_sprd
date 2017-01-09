/* [Vendor_19/40] */
#include <sys/select.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>
#include <cutils/log.h>
#include <errno.h>  

#define LOG_TAG "ISP_GUARD"

static void empty_command_pipe(int fd){
    char buff[16];
    int ret;
    do {
        ret = read(fd, &buff, sizeof(buff));
    } while (ret > 0 || (ret < 0 && errno == EINTR));
}

int main(int argc, char **argv){

    int pipe_fd,max_fd;
    fd_set fds_read;
    int result;
    int count;
    void* data;
    int val_int;
    struct str_parms *parms;
    char value[30];
    int ret = 0;
    int retdump;
    pid_t tid;
    struct sched_param param;


    param.sched_priority = 99;
    pipe_fd = open("/dev/isp_pipe/pipe_1", O_RDWR);
    if(pipe_fd < 0){
        ALOGE("%s, open pipe error!! \n",__func__);
        return -1;
    }
    max_fd = pipe_fd + 1;
    if((fcntl(pipe_fd,F_SETFL,O_NONBLOCK)) <0){
        ALOGE("set flag RROR --------\n");
    }
    data = (char*)malloc(1024);
    if(data == NULL){
        ALOGE("malloc data err \n");
        return -1;
    }
    while(1){
        FD_ZERO(&fds_read);
        FD_SET(pipe_fd,&fds_read);
        result = select(max_fd,&fds_read,NULL,NULL,NULL);
        if(result < 0){
            ALOGE("select error \n");
            continue;
        }
        if(FD_ISSET(pipe_fd,&fds_read) <= 0 ){
            ALOGE("SELECT OK BUT NO fd is set \n");
            continue;
        }
        memset(data,0,1024);
        read(pipe_fd,&tid,sizeof(pid_t));
        if(count < 0){
            ALOGE("read data err \n");
            empty_command_pipe(pipe_fd);
            continue;
        }
        ALOGD("tid:%lu \n",tid);
        ret = sched_setscheduler(tid,SCHED_FIFO,&param);
    }
    ALOGD("[3A Porting]End of set_proc_priority.c \n");
    return 0;
}
