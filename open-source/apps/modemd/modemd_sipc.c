#define LOG_TAG     "MODEMD"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <cutils/properties.h>
#include <utils/Log.h>
#include <cutils/sockets.h>
#include "modemd.h"
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include "modemd_ext.h"

int sp_modem_state = MODEM_READY;
pthread_mutex_t sp_state_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t sp_cond = PTHREAD_COND_INITIALIZER;

static int epollfd = -1;
static int wakealarm_fd = -1;
static bool g_b_wake_locking = false;
static int assert_fd = 0;

#ifdef SECURE_BOOT_ENABLE
#define SECURE_IMG_OFFSET    1024
#endif

/******************************************************
 *
 ** sipc interface begin
 *
 *****************************************************/
extern void exit_modemd(void)
{
	MODEMD_LOGD("exit_modemd!!!");
	exit(-1);
}

int loop_info_sockclients(const char* buf, const int len)
{
    int i, ret;

    /* info socket clients that modem is assert/hangup/blocked */
    for(i = 0; i < MAX_CLIENT_NUM; i++) {
        MODEMD_LOGD("client_fd[%d]=%d\n",i, client_fd[i]);
        if(client_fd[i] >= 0) {
            ret = write(client_fd[i], buf, len);
            MODEMD_LOGD("write %d bytes to client_fd[%d]:%d",
                    len, i, client_fd[i]);
            if(ret < 0) {
                MODEMD_LOGE("reset client_fd[%d]=-1",i);
                close(client_fd[i]);
                client_fd[i] = -1;
            }
        }
    }

    return 0;
}

void *detect_modem_blocked(void *par)
{
    int soc_fd, i, ret;
    int numRead, loop_fd;
    char *modem = NULL;
    char socket_name[256] = {0}, loop_dev[256] = {0};
    char prop[256], buf[256], buffer[256];
    int is_reset, is_assert = 0;

    if(par == NULL){
        MODEMD_LOGE("%s: input parameter is NULL!", __func__);
        return NULL;
    } else{
        modem = (char *)par;
    }

    snprintf(PHS_SOCKET_NAME,sizeof(PHS_SOCKET_NAME), "phs%s" ,modem);
    strcpy(socket_name, PHS_SOCKET_NAME);

reconnect:
    MODEMD_LOGD("%s: try to connect socket %s...", __func__, socket_name);
    soc_fd = socket_local_client(socket_name, ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);

    while(soc_fd < 0) {
        usleep(10*1000);
        soc_fd = socket_local_client(socket_name, ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    }
    MODEMD_LOGD("%s: connect socket %s success", __func__, socket_name);

    for(;;) {
        memset(buf, 0, sizeof(buf));
        MODEMD_LOGD("%s: begin detect blocked event on socket %s...", __func__, socket_name);
        do {
            numRead = read(soc_fd, buf, sizeof(buf)-1);
        } while(numRead < 0 && errno == EINTR);
        if(numRead <= 0) {
            MODEMD_LOGE("%s: read numRead=%d, error: %s", __func__, numRead, strerror(errno));
            pthread_mutex_lock(&sp_state_mutex);
            if(sp_modem_state != MODEM_READY) {
                MODEMD_LOGD("%s: wait for %s modem ready ...", __func__, modem);
                pthread_cond_wait(&sp_cond, &sp_state_mutex);
                MODEMD_LOGD("%s: %s modem ready, wake up", __func__, modem);
            }
            pthread_mutex_unlock(&sp_state_mutex);
            close(soc_fd);
            goto reconnect;
        }
        MODEMD_LOGD("%s: read numRead=%d, buf=%s", __func__, numRead, buf);

        if(strstr(buf, "Modem Blocked") != NULL) {
            pthread_mutex_lock(&sp_state_mutex);
            if(sp_modem_state != MODEM_READY) {
                pthread_mutex_unlock(&sp_state_mutex);
                continue;
            }
            sp_modem_state = MODEM_ASSERT;
            pthread_mutex_unlock(&sp_state_mutex);
            if (is_external_modem()) {
                goto raw_reset;
            }
        } else {
            MODEMD_LOGD("%s: read invalid string from socket %s", __func__, socket_name);
            continue;
        }

        is_assert = 1;
        snprintf(SP_LOOP_PROP,sizeof(SP_LOOP_PROP),"ro.modem.%s.loop" ,modem);
        MODEMD_LOGE("%s: loop dev %s", __func__, SP_LOOP_PROP);
        if(strcmp(modem,"t")==0) {
            property_get(SP_LOOP_PROP, loop_dev, DEFAULT_TD_LOOP_DEV);
        } else if(strcmp(modem,"w")==0) {
            property_get(SP_LOOP_PROP, loop_dev, DEFAULT_W_LOOP_DEV);
        } else if(strcmp(modem,"tl")==0) {
            property_get(SP_LOOP_PROP, loop_dev, DEFAULT_TL_LOOP_DEV);
        } else if(strcmp(modem,"lf")==0) {
            property_get(SP_LOOP_PROP, loop_dev, DEFAULT_LF_LOOP_DEV);
        } else if(strcmp(modem,"l")==0) {
            property_get(SP_LOOP_PROP, loop_dev, DEFAULT_L_LOOP_DEV);
        } else {
            close(soc_fd);
            MODEMD_LOGE("%s: invalid modem type, exit", __func__);
            return NULL;
        }

        loop_fd = open(loop_dev, O_RDWR | O_NONBLOCK);
        MODEMD_LOGD("%s: open loop dev: %s, fd = %d", __func__, loop_dev, loop_fd);
        if (loop_fd < 0) {
            MODEMD_LOGE("%s: open %s failed, error: %s", __func__, loop_dev, strerror(errno));
            goto raw_reset;
        }
        ret = write(loop_fd, "AT", 3);
        if(ret < 0) {
            MODEMD_LOGE("%s: write %s failed, error:%s", __func__, loop_dev, strerror(errno));
            close(loop_fd);
            goto raw_reset;
        }
        usleep(100*1000);
        memset(buffer, 0, sizeof(buffer));
        do {
            ret = read(loop_fd, buffer, sizeof(buffer)-1);
        } while(ret < 0 && errno == EINTR);
        if (ret <= 0) {
            MODEMD_LOGE("%s: read %d return %d, errno = %s", __func__, loop_fd , ret, strerror(errno));
            close(loop_fd);
            goto raw_reset;
        }
        if(!strcmp(buffer, "AT")) {
            MODEMD_LOGD("%s: loop spipe %s is OK", __func__, loop_dev);
        }
        close(loop_fd);

raw_reset:
        system("echo load_modem_img >/sys/power/wake_lock");
        g_b_wake_locking = true;
        /* info socket clients that modem is blocked */
        MODEMD_LOGE("Info all the sock clients that modem is blocked");
        loop_info_sockclients(buf, numRead);

        /* reset or not according to property */
        memset(prop, 0, sizeof(prop));
        property_get(MODEM_RESET_PROP, prop, "0");
        is_reset = atoi(prop);
        stop_service(modem, 0);
        if(is_reset) {
            MODEMD_LOGD("%s: reset is enabled, reload modem...", __func__);
            if (is_external_modem()) {
               get_ext_modem_if()->load_modem_image();
            } else {
                // will send msg to modem control to reload the modem img
                MODEMD_LOGD("%s: send reset msg to modem control", __func__);
                if(control_fd >0)
                {
                    write(control_fd, "Prepare Reset", sizeof("Prepare Reset"));
                }
                else
                {
                    MODEMD_LOGE("%s: control_fd = %d!", __func__, control_fd);
                }
                is_assert = 0;
            }
        } else {
            MODEMD_LOGD("%s: reset is not enabled , not reset", __func__);
        }
        system("echo load_modem_img >/sys/power/wake_unlock");
        g_b_wake_locking = false;
    }
    close(soc_fd);
    return (void*) NULL;
}
static const char s_reset_cmd[2] = {0x7a, 0x0a};
static char diag_chan[256], log_chan[256], mkbuf[256];

static void modemd_read_empty_log(void)
{
   char buf[2048] = {0};
   int ret = 0;
   int count = 0;
   int fd =0;
   struct timeval timeout;
   fd_set rfds;

   MODEMD_LOGD("max read 1s, read log start!");

   fd = open_modem_dev(log_chan);

   if(fd<=0){
	   MODEMD_LOGE("MODEMD cannot open %s\n", log_chan);
	   return;
   }
   FD_ZERO(&rfds);
   FD_SET(fd, &rfds);

   for(;;){
	   timeout.tv_sec = 1;
	   timeout.tv_usec=0;
       do {
           ret = select(fd+1, &rfds, NULL, NULL, &timeout);
       } while(ret == -1 && errno == EINTR);
       if(ret < 0){
           //MODEMD_LOGE("select error: %s", strerror(errno));
           continue;
       }else if(ret == 0){
           MODEMD_LOGD(" time out, read log over!");
           break;
       }else{
            //MODEMD_LOGD("one time read log start");
            do
            {
                count = read(fd, buf, sizeof(buf));
                //MODEMD_LOGD("read log count = %d",count);
            }
            while(count > 0);
            continue;
       }
   }
   close(fd);
}

static void modemd_write_reset_cmd(void)
{
    int diag_fd =0, w_cnt = 0;

    // write 'z' to cp
    diag_fd= open(diag_chan, O_RDWR | O_NONBLOCK);
    if(diag_fd < 0) {
        MODEMD_LOGE("MODEMD cannot open %s\n", diag_chan);
    }
    else
    {
        MODEMD_LOGD("ready write diag cmd = %s!", s_reset_cmd);
        w_cnt = write(diag_fd, s_reset_cmd, sizeof(s_reset_cmd));
        MODEMD_LOGD("MODEMD write diag_chan:%d ,%s\n", w_cnt, strerror(errno));
        close(diag_fd);
    }
}

int detect_clients_dispose_state(int cause){
    fd_set rfds;
    char buf[256] = {0};
    int  i = 0;
    int retry_times = 3;
    int  ret =0;
    int  max_fd = -1;
    int  count =0;
    int  fd =-1;
    int  result =0;
    struct timeval timeout;

    MODEMD_LOGD("enter detect_clients_dispose_state");

    for(;retry_times > 0;retry_times--){

        fd = -1;
        max_fd= -1;
        timeout.tv_sec=2;
        timeout.tv_usec=0;
        FD_ZERO(&rfds);
        for(i =0;i<MAX_CLIENT_NUM;i++){
            if(client_fd[i] >= 0){
                FD_SET(client_fd[i], &rfds);
                MODEMD_LOGD("listen fd %d",client_fd[i]);
                if(client_fd[i] > max_fd){
                    max_fd = client_fd[i];
                }
            }
        }
        if(max_fd == -1 ){
            return result;
        }

        do {
            ret = select(max_fd+1, &rfds, NULL, NULL, &timeout);
        } while(ret == -1 && errno == EINTR);

        if(ret < 0){
            MODEMD_LOGE("select error: %s", strerror(errno));
            continue;
        }else if(ret == 0){
            MODEMD_LOGE("select timeout");
        }else{
            for(i =0;i<MAX_CLIENT_NUM;i++){
                if (FD_ISSET(client_fd[i], &rfds)) {
                    fd =client_fd[i];
                    break;   //Just slog minidump send data to modemd
                }
            }
            count = read(fd, buf, sizeof(buf)-1);
            if (count <= 0) {
                close(fd);
                client_fd[i] = -1;
                MODEMD_LOGE("read %d return %d, error: %s", fd, count, strerror(errno));
                continue;
            }
            buf[count] ='\0';
            MODEMD_LOGD("read response %s from %d", buf,fd);
            switch (cause){
                case MINIDUMP:  // for mini dump
                    if (strstr(buf, "MINIDUMP")) {
                        result = 1;
                    }
                    break;
                default :
                    result = 0;
            }
        }
        break;
    }
    return result;
}

/******************************************************
 *
 ** sipc interface end
 *
 *****************************************************/
