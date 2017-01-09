#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <sys/stat.h>
#include <errno.h>
#include <cutils/properties.h>
#include <pthread.h>
#include <utils/Log.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <cutils/sockets.h>

#define CMD_SERVICE "cmd_skt"
#define MAX_BUF_LEN 4096

void *root_cmd_parser(void *arg)
{
     int service_fd, connect_fd;
     socklen_t server_len, client_len;
     static char recv_buf[MAX_BUF_LEN];
     static char read_buf[MAX_BUF_LEN];
     static char send_buf[MAX_BUF_LEN];
     struct sockaddr_un client_address;

     unlink(CMD_SERVICE);

     service_fd = socket_local_server(CMD_SERVICE,ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);

     if ((connect_fd = accept(service_fd, (struct sockaddr *)&client_address, &client_len)) < 0)
     {
          ALOGD("accept socket failed!");
          close(service_fd);
          return NULL;
     }

     ALOGD("rootcmd_parser service is waitting!\n");

     while(1)
     {
          memset(recv_buf, '\0', sizeof(recv_buf));
          if(read(connect_fd, recv_buf, sizeof(recv_buf)) <= 0)
          {
               ALOGD("recv cmd error!\n");
               break;
          }

          strcat(recv_buf," 2>&1");

          FILE *file = popen(recv_buf, "r");
          if(file == NULL)
          {
               ALOGD("cannot creat Result file!\n");
               write(connect_fd,"Result\n",strlen("Result\n"));
               break;
          }

          memset(send_buf, '\0', sizeof(send_buf));
          if(fread(send_buf, sizeof(char), sizeof(send_buf), file) <= 0)
          {
               ALOGD("read command echo error!\n");
          }

          while(!feof(file))
          {
               memset(read_buf, '\0', sizeof(read_buf));
               if(fread(read_buf, sizeof(char), sizeof(read_buf), file) <= 0)
               {
                    ALOGD("read command echo error!\n");
                    break;
               }
               strcat(send_buf,read_buf);
          }

          pclose(file);
          strcat(send_buf,"Result\n");
          if(write(connect_fd, send_buf, strlen(send_buf)) <= 0)
          {
               ALOGD("send cmd echo failed!\n");
          }

          //ALOGD("sendbuf is %.*s \n",strlen(send_buf), send_buf);
     }

     close(service_fd);
     return NULL;
}

int main(int argc, char *argv[])
{
     pthread_t tid_rootcmd_monitor;
     pthread_create(&tid_rootcmd_monitor,NULL,root_cmd_parser,NULL);
     pthread_join(tid_rootcmd_monitor , NULL);
     return 0;
}
