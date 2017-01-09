
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
#include <semaphore.h>

#include "local_socket.h"
#include "../audiotester/audiotester_server.h"


#ifdef LOCAL_SOCKET_SERVER

#include "../debug/audio_debug.h"

#define LOG_TAG "audio_hw_socke_server"

extern int connect_audiotester_process(struct socket_handle *tunning_handle,int sockfd,int max_buffer_size,void *fun);
extern int disconnect_audiotester_process(struct socket_handle *tunning_handle);


#define LOCAL_SOCKET_CONNETC_MSG "local_socket_connect"

struct local_socket_res{
    int client_fd[LOCAL_SOCKET_CLIENT_MAX_NUMBER];
    int notify_pipe[2];
    pthread_mutex_t lock;
    void *dev;
};

static int handle_audio_cmd_data(void *_socket,
                          uint8_t *buf, int len, int sub_type, int data_state)
{
    int ret = 0;
    struct socket_handle *socket=(struct socket_handle *)_socket;
    struct local_socket_res *res=(struct local_socket_res *)socket->res;
    LOG_I("handle_audio_cmd_data len =%d  sub_type =%d data_state:%d %s",len,sub_type,data_state,buf);

    switch (sub_type) {
    case AUDIO_EXT_TEST_CMD: {
        if(NULL!=res->dev){
            ext_contrtol_process((struct tiny_audio_device *)res->dev,buf);
        }
        ret=send_buffer(_socket,AUDIO_EXT_TEST_CMD,DATA_STATUS_OK);
        break;
    }
    case SOCKET_TEST_CMD: {
        append_g_data_buf(_socket,buf);
        ret=send_buffer(_socket,SOCKET_TEST_CMD,DATA_STATUS_OK);
        break;
    }
    case DIS_CONNECT_AUDIOTESTER: {
        ret=disconnect_audiotester_process(socket);
        break;
    }
    default:
        send_error(socket,"the audio tunning command is unknown");
    }
    return ret;
}

static int handle_received_data(void *_socket, uint8_t *received_buf,
                         int rev_len)
{
    int ret, data_len;
    MSG_HEAD_T *msg_head;
    DATA_HEAD_T *data_command;
    uint8_t *audio_data_buf;
    struct socket_handle *socket=(struct socket_handle *)_socket;

    ret = check_diag_header_and_tail(received_buf, rev_len);
    if (ret != 0) {
        LOG_E("Diag Header or Tail error");
        ret=-3;
        return ret;
    }
    msg_head = (MSG_HEAD_T *)(received_buf + 1);

    LOG_I("seq:%08x len:%04x type:%02x subtype:%02x",
          msg_head->seq_num, msg_head->len, msg_head->type, msg_head->subtype);
    //0x99 is audio tunning specification define
    if (msg_head->type != AUDIO_CMD_TYPE) {
        LOG_E("the audio data's type is not equal 0X99, type=%d\n", msg_head->type);
        return -1;
    }

    if(received_buf[1 + msg_head->len] != 0x7e) {
        LOG_E("date format err:%x", received_buf[1 + msg_head->len]);
        return -2;
    }

    data_len = msg_head->len - sizeof(MSG_HEAD_T) - sizeof(DATA_HEAD_T);
    data_command = (DATA_HEAD_T *)(received_buf + sizeof(MSG_HEAD_T) + 1);
    audio_data_buf = received_buf + sizeof(MSG_HEAD_T) + sizeof(DATA_HEAD_T) + 1;
    *(audio_data_buf+data_len )=0x00;

    ret = handle_audio_cmd_data(_socket, audio_data_buf,
                                data_len, msg_head->subtype, data_command->data_state);


    LOG_I("handle_received_data exit");
    return ret;
}

static void *_tunning_thread(void *args)
{
    struct socket_handle *tunning=(struct socket_handle*)args;
    struct local_socket_res *res=(struct local_socket_res *)tunning->res;

    int ret, rev_len, data_len;

    int max_fd=0;

    fd_set rfds;

    int client_number=0;

    int i=0;
    LOG_E("%s", __func__);
    char pipe_msg[32]={0};

    tunning->wire_connected=true;

    max_fd=res->client_fd[0];
    while(true==tunning->wire_connected) {

        FD_ZERO(&rfds);
        FD_SET(res->notify_pipe[0], &rfds);
        max_fd=res->notify_pipe[0];

        for(i=0;i<LOCAL_SOCKET_CLIENT_MAX_NUMBER;i++){
            if(res->client_fd[i]>0){
                FD_SET(res->client_fd[i], &rfds);
                if(max_fd<res->client_fd[i]){
                    max_fd=res->client_fd[i];
                }
            }
        }

        LOG_I("begin select:0x%x",max_fd);
        ret = select(max_fd + 1, &rfds, NULL, NULL, NULL);
        if (ret <= 0) {
            LOG_E("select error:0x%x",max_fd);
            continue;
        }

        if(FD_ISSET(res->notify_pipe[0],&rfds)){
            int nfd=-1;
            memset(pipe_msg,0,sizeof(pipe_msg));
            pthread_mutex_lock(&res->lock);
            rev_len = read(res->notify_pipe[0], pipe_msg, sizeof(pipe_msg)-1);
            memset(pipe_msg,0,sizeof(pipe_msg));
            pthread_mutex_unlock(&res->lock);
            LOG_I("read pipe msg:%s len:%d",pipe_msg,rev_len);
        }

        for(i=0;i<LOCAL_SOCKET_CLIENT_MAX_NUMBER;i++){
            int nfd=res->client_fd[i];
            if(nfd>0){
                if (nfd != -1 && FD_ISSET(nfd, &rfds))
                    memset(tunning->audio_received_buf, 0, tunning->max_len);
                    rev_len =  read (nfd,tunning->audio_received_buf, tunning->max_len);
                    if(rev_len <= 0){
                        pthread_mutex_lock(&res->lock);
                        close(res->client_fd[i]) ;
                        res->client_fd[i]= -1 ;
                        pthread_mutex_unlock(&res->lock);
                        LOG_I("local socket client :%d disconnect",res->client_fd[i]);
                    }else{
                        LOG_I("client[%d]:%d",i,res->client_fd[i]);
                        tunning->sockfd=res->client_fd[i];
                        ret=tunning->process(tunning, tunning->audio_received_buf,rev_len);
                    }
            }
        }
    }
    return NULL;
}



static void *listen_thread(void *args)
{
    int ser_fd, tunning_fd, ret;
    socklen_t addrlen;
    pthread_t tunning_t;
    pthread_attr_t attr;
    struct socket_handle *local_socket=(struct socket_handle *)args;
    struct local_socket_res *res=(struct local_socket_res *)local_socket->res;
    int i=0;

    char pipe_msg[32]={0};
    int client_number=0;

    if(NULL==res){
        LOG_E("listen_thread res is NULL");
        return NULL;
    }

    ser_fd = socket_local_server(AUDIO_SOCKET_NAME, ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);

    if (ser_fd < 0) {
        LOG_E("can not create tunning server thread\n");
        return NULL;
    }

    pthread_attr_init(&attr);

    if((connect_audiotester_process(local_socket,tunning_fd,LOCAL_SOCKET_BUFFER_SIZE,handle_received_data)<0)){
        LOG_E("connect_audiotester_process failed\n");
        disconnect_audiotester_process(local_socket);
        return NULL;
    }

    //this is prevent process be killed by SIGPIPE
    signal(SIGPIPE, SIG_IGN);

    ret = pthread_create(&tunning_t, &attr, _tunning_thread, args);
    if (ret < 0) {
        LOG_E("create tunning thread failed");
        return NULL;
    }

    tunning_fd=-1;
    while (1) {
        LOG_I("waiting audiotester connect\n");
        if ( (tunning_fd=accept(ser_fd,NULL,NULL)) == -1)
        {
            LOG_E("accept tunning client failed\n");
            continue;
        }else{
            pthread_mutex_lock(&res->lock);
            client_number=-1;
            for(i=0;i<LOCAL_SOCKET_CLIENT_MAX_NUMBER;i++){
                 if(res->client_fd[i]<0){
                    res->client_fd[i]=tunning_fd;
                    LOG_I("client[%d]:%d connect success",i,tunning_fd);
                    tunning_fd=-1;
                    client_number=i;
                    break;
                 }
            }

            if(tunning_fd<0){
                memset(pipe_msg,0,sizeof(pipe_msg));
                sprintf(pipe_msg,"%s=0x%x",LOCAL_SOCKET_CONNETC_MSG,client_number);
                write(res->notify_pipe[1],pipe_msg,strlen(pipe_msg));
            }else{
                LOG_W("client_number:%d client can not connect success",client_number);
            }
            pthread_mutex_unlock(&res->lock);
        }
        tunning_fd=-1;
    }
    LOG_E("listen_thread exit!");
    pthread_attr_destroy(&attr);
    if (ser_fd) {
        close(ser_fd);
    }
    return NULL;
}

void start_audio_local_server(struct tiny_audio_device *adev)
{
    pthread_t ser_id;
    pthread_attr_t attr;
    int ret;

    struct local_socket_res *res;
    int i=0;

    char pipe_msg[32]={0};
    int client_number=0;

    res=(struct local_socket_res *)malloc(sizeof(struct local_socket_res));
    if(NULL==res){
        LOG_E("start_audio_local_server malloc failed");
        return ;
    }


    for(i=0;i<LOCAL_SOCKET_CLIENT_MAX_NUMBER;i++){
        res->client_fd[i]=-1;
    }
    res->notify_pipe[0]=-1;
    res->notify_pipe[1]=-1;

    pthread_mutex_init(&res->lock, NULL);

    if(pipe(res->notify_pipe) < 0)
    {
      LOG_E("pipe error!\n");
      return;
    }
    res->dev=adev;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    adev->local_socket.res=res;
    ret = pthread_create(&ser_id, &attr, listen_thread,&( adev->local_socket));
    if (ret < 0) {
        LOG_E("create audio tunning server thread failed");
        return;
    }
    pthread_attr_destroy(&attr);
}
#endif /* LOCAL_SOCKET_SERVER */


#ifdef LOCAL_SOCKET_CLIENT

#define LOG_TAG "audio_hw_socke_client"

int  audio_create_local_socket(void)
{
    int  socket_fd = -1;

    socket_fd = socket_local_client(AUDIO_SOCKET_NAME,
                             ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    if(socket_fd < 0 ) {
        ALOGE("modem_monitor_routine:socket_local_client failed %d", errno);
    }else{
        ALOGI("connected success!");
    }

    return socket_fd;
}

int send_audio_command(int fd, int sub_cmd,char *cmd_buf,int cmd_size)
{
    int index = 0;
    int i=0;
    char *send_buf = NULL;
    MSG_HEAD_T m_head;
    DATA_HEAD_T data_head;

    send_buf=(char *)malloc(cmd_size+sizeof(MSG_HEAD_T)+sizeof(DATA_HEAD_T)+16);
    if(NULL==send_buf){
        ALOGE("send_command malloc failed");
        return -1;
    }

    *(send_buf + index) = 0x7e;
    index += 1;
    m_head.type = 0x99;//must not be modified.
    m_head.subtype = sub_cmd;
    m_head.len = sizeof(MSG_HEAD_T) + sizeof(DATA_HEAD_T) +cmd_size;
//    ALOGD("m_head.len=%d", m_head.len);

    memcpy(send_buf + index, &m_head, sizeof(MSG_HEAD_T));
    index += sizeof(MSG_HEAD_T);

    memcpy(send_buf + index, &data_head, sizeof(DATA_HEAD_T));
    index += sizeof(DATA_HEAD_T);

    memcpy(send_buf + index, cmd_buf, cmd_size);
    index += cmd_size;

    *(send_buf + index) = 0x7e;
    index += 1;

    int len = write(fd, send_buf, index);
//    ALOGD("write len = %d,index=%d", len, index);
    free(send_buf);
    return index;
}

int receive_audio_command(int fd, char *cmd_buf,int max_size)
{
    MSG_HEAD_T *m_head;
    DATA_HEAD_T *data_head;
    char *rece_buf=NULL;
    int rece_buf_size=max_size+2+sizeof(MSG_HEAD_T)+sizeof(DATA_HEAD_T);
    int ret=0;

    rece_buf=(char *)malloc(rece_buf_size);
    if(NULL==rece_buf){
        ALOGE("receive_audio_command malloc buffer failed");
        return -1;
    }

    memset(rece_buf,0,rece_buf_size);

    ret=read(fd, rece_buf, rece_buf_size);
    if(ret>=(2+sizeof(MSG_HEAD_T)+sizeof(DATA_HEAD_T))){
        m_head=(MSG_HEAD_T *)(rece_buf+1);
        data_head=(DATA_HEAD_T *)(rece_buf+sizeof(MSG_HEAD_T)+1);
        memcpy(cmd_buf,rece_buf+sizeof(MSG_HEAD_T)+sizeof(DATA_HEAD_T)+1,ret-(2+sizeof(MSG_HEAD_T)+sizeof(DATA_HEAD_T)));
        ret=data_head->data_state;
    }

    if(NULL!=rece_buf){
        free(rece_buf);
   }
    ALOGD("receive_audio_command state:%d buf:%s",ret,cmd_buf);
    return ret;
}
#endif /* LOCAL_SOCKET_CLIENT */
