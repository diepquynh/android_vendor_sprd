
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdint.h>
#include <sys/types.h>
#include <semaphore.h>
#include <pthread.h>
#include <cutils/log.h>
#include <cutils/sockets.h>
#include "../audiotester/audiotester_server.h"

#define AUDIO_SOCKET_NAME "audio_local_socket"
#define SHELL_DBG(...)   fprintf(stdout, ##__VA_ARGS__)

extern int send_audio_command(int fd, int sub_cmd,char *cmd_buf,int cmd_size);
extern int receive_audio_command(int fd, char *cmd_buf,int max_size);
extern int  audio_create_local_socket(void);

#define LOG_SOCKET_TEST_NUMBER 8

int main(int argc, char *argv[])
{
    int  socket_fd[LOG_SOCKET_TEST_NUMBER]={-1};
    char buf[256]={0};
    int send_count=0;
    int receive_count=0;
    int ret=-1;
    int i=0;

    for(i=0;i<LOG_SOCKET_TEST_NUMBER;i++){
        socket_fd[i]=audio_create_local_socket();
        sleep(1);
    }
    while(1){

        for(i=0;i<LOG_SOCKET_TEST_NUMBER;i++){
            if(socket_fd[i]<0){
                SHELL_DBG("client[%d] disconnect :0x%x\n",i,socket_fd[i]);
                continue;
            }

            sprintf(buf,"client[%d] send[%d]\n",socket_fd[i],send_count);

            ret=send_audio_command(socket_fd[i],SOCKET_TEST_CMD, buf, strlen(buf));
            SHELL_DBG(" send :ret:0x%x <%s>\n",ret,buf);
            if(ret>0){
            memset(buf,0,sizeof(buf));
            ret=receive_audio_command(socket_fd[i], buf, sizeof(buf));
            SHELL_DBG(" receive :ret:0x%x <%s>\n",ret,buf);
            }else{
                SHELL_DBG(" send[%d] err\n",i,socket_fd[i]);
            }
        }

        SHELL_DBG("sleep\n");
        send_count++;
        sleep(2);
    }
    return 0;
}

