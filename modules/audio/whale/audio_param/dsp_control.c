/*
* Copyright (C) 2010 The Android Open Source Project
* Copyright (C) 2012-2015, The Linux Foundation. All rights reserved.
*
* Not a Contribution, Apache license notifications and license are retained
* for attribution purposes only.
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

#include <utils/Log.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/select.h>
#include <fcntl.h>
#include <signal.h>
#include <semaphore.h>
#include <cutils/sockets.h>
#include "dsp_control.h"
#include "audio_debug.h"
#include "audio_hw.h"

#include "audio_control.h"
#include "atci.h"
#include <linux/ioctl.h>

#define LOG_TAG "audio_hw_dsp"
#define DSPLOG_CMD_MARGIC 'X'

#define DSPLOG_CMD_LOG_ENABLE                           _IOW(DSPLOG_CMD_MARGIC, 0, int)
#define DSPLOG_CMD_LOG_PATH_SET		             _IOW(DSPLOG_CMD_MARGIC, 1, int)
#define DSPLOG_CMD_LOG_PACKET_ENABLE		_IOW(DSPLOG_CMD_MARGIC, 2, int)
#define DSPLOG_CMD_PCM_PATH_SET		             _IOW(DSPLOG_CMD_MARGIC, 3, int)
#define DSPLOG_CMD_PCM_ENABLE		             _IOW(DSPLOG_CMD_MARGIC, 4, int)
#define DSPLOG_CMD_PCM_PACKET_ENABLE	      _IOW(DSPLOG_CMD_MARGIC, 5, int)
#define DSPLOG_CMD_DSPASSERT		                    _IOW(DSPLOG_CMD_MARGIC, 6, int)

#define SPRD_AUD_DSPASSERT_MEM "/dev/audio_dsp_mem"
#define SPRD_AUD_DSPASSERT_LOG "/dev/audio_dsp_log"
#define SPRD_AUD_DSPASSERT_PCM "/dev/audio_dsp_pcm"

#define AGDSP_ASSERT_NOTIFYMODEMD_MSG  "AGDSP Assert:"




int agdsp_send_msg(struct dsp_control_t * dsp_ctl,struct dsp_smsg *msg){
    int ret=-1;
    LOG_I("agdsp_send_msg:0x%x 0x%x 0x%x 0x%x",
        msg->command,
        msg->parameter0,
        msg->parameter1,
        msg->parameter2,
        msg->parameter3);
    memset(&dsp_ctl->msg,0,sizeof(struct dsp_smsg));
    ret=write(dsp_ctl->agdsp_pipd_fd,msg,sizeof(struct dsp_smsg));
    return 0;
}

int dsp_ctrl_dsp_assert_notify(struct dsp_control_t * dsp_ctl ,bool stop_phone,bool reset_modem,uint8_t * info,int info_len)
{
    const char *err_str = NULL;
    int ret = 0;
    ALOGE("peter:dsp_ctrl_notify_modemd in");

    if(((dsp_ctl->fd_dsp_assert_mem>0)  &&  (dsp_ctl->auto_reset_dsp == 0))
            || (dsp_ctl->dsp_assert_force_notify)){
        ret = ioctl(dsp_ctl->fd_dsp_assert_mem, DSPLOG_CMD_DSPASSERT, 1);
        ALOGE(" dsp_ctrl_notify_modemd notify slogd ret: %d",ret);
    }
    else {
        ALOGE("dsp_ctrl_notify_modemd notify slogd failed,dsp_ctl->fd_dsp_assert_mem  %d",dsp_ctl->fd_dsp_assert_mem);
    }
    if(reset_modem) {
        err_str = sendCmd(0, "AT+SPATASSERT=1");
    }
    if(stop_phone) {
        err_str = sendCmd(0, "ATH");
    }
    if(dsp_ctl->fd_modemd_notify ) {
        int len = 0;
        uint8_t *buf = NULL;
        len = sizeof(AGDSP_ASSERT_NOTIFYMODEMD_MSG) + info_len;
        buf = malloc(len);
        if(buf) {
            memset(buf, 0,len);
            strcat(buf,AGDSP_ASSERT_NOTIFYMODEMD_MSG);
            if(info) {
                strcat(buf,info);
            }
            ret=write(dsp_ctl->fd_modemd_notify,buf,sizeof(AGDSP_ASSERT_NOTIFYMODEMD_MSG) + info_len);
            ALOGE("dsp_ctrl_notify_modemd write ret  %d,%s",ret,buf);
        }
        else {
            ALOGE(" dsp_assert_notify malloc failed len is %d", len);
        }
        if(buf) {
            free(buf);
        }
    }
    else {
        ALOGE("dsp_ctrl_notify_modemd dsp_ctl->fd_modemd_notify %d",dsp_ctl->fd_modemd_notify);
    }
    return ret;
}

static int agdsp_net_msg_process(struct audio_control *dev_ctl,struct dsp_smsg *msg){
    struct voice_net_t net_infor;

    net_infor.net_mode=(aud_net_m)msg->parameter0;
    net_infor.wb_mode=msg->parameter1;

   return set_audioparam(dev_ctl,PARAM_NET_CHANGE,&net_infor,false);
}

static int agdsp_msg_process(struct dsp_control_t * agdsp_ctl ,struct dsp_smsg *msg){
    int ret=0;
    struct dsp_smsg _msg;
    int is_time_out = 0;
    LOG_I("agdsp_msg_process cmd:0x%x parameter:0x%x 0x%x 0x%x",
        msg->command,msg->parameter0,msg->parameter1,msg->parameter2);

    if(msg->channel!=2){
        LOG_W("agdsp_msg_process channel:%d",msg->channel);
        return 0;
    }

    switch(msg->command){
        case AGDSP_CMD_NET_MSG:
            agdsp_net_msg_process(agdsp_ctl->dev_ctl,msg);
            break;
        case AGDSP_CMD_TIMEOUT:
            ALOGE("dsp timeout!!!!!");
            is_time_out = 1;
        case AGDSP_CMD_ASSERT:
            {
                uint8_t tmp[40] = {0};
                int len = 0;
                aud_dsp_assert_set(agdsp_ctl->dev, true);
                ALOGE("dsp asserted timeout %d",is_time_out);
                len = sprintf(tmp,":%x,%x,%x,%x",msg->parameter0,msg->parameter1,msg->parameter2,msg->parameter3); 
                set_usecase(agdsp_ctl->dev_ctl, UC_AGDSP_ASSERT, true);
                if(is_time_out) {
                    if(is_usecase(agdsp_ctl->dev_ctl, UC_CALL)) {
                        dsp_ctrl_dsp_assert_notify(agdsp_ctl,1, agdsp_ctl->reset_modem, "time out", sizeof("time out"));
                    }
                    else
                        dsp_ctrl_dsp_assert_notify(agdsp_ctl,0, agdsp_ctl->reset_modem, "time out", sizeof("time out"));
                }
                else {
                    dsp_ctrl_dsp_assert_notify(agdsp_ctl,0,agdsp_ctl->reset_modem,tmp, len);
                }
                ALOGE("peter: force all standby a");
                force_all_standby(agdsp_ctl->dev);
                if(agdsp_ctl->auto_reset_dsp) {
                    agdsp_boot();
                }
                ALOGE("peter: force all standby e");
            }
            break;
        case AGDSP_CMD_STATUS_CHECK:
        case AGDSP_CMD_BOOT_OK:
            ALOGE("peter: check dsp status start");
            memcpy(&agdsp_ctl->msg,msg,sizeof(struct dsp_smsg));
            sem_post(&agdsp_ctl->rx.sem);
            set_usecase(agdsp_ctl->dev_ctl,UC_AGDSP_ASSERT, false);
            aud_dsp_assert_set(agdsp_ctl->dev, false);
            break;
        default:
            break;
    }
    return ret;
}



int agdsp_send_msg_test(void * arg,void * params,int opt){
    struct tiny_audio_device *adev = (struct tiny_audio_device *)arg ;
    struct dsp_smsg _msg;
    _msg.channel=2;
    _msg.command=opt;
    _msg.parameter0=opt+1;
    _msg.parameter1=opt+2;
    _msg.parameter2=opt+3;
    agdsp_send_msg(adev->dev_ctl->agdsp_ctl,&_msg);
    return 0;
}

bool agdsp_check_status(struct dsp_control_t * dsp_ctl){
    struct dsp_smsg _msg;
    _msg.channel=2;
    _msg.command=AGDSP_CMD_STATUS_CHECK;
    _msg.parameter0=AGDSP_CMD_STATUS_CHECK+1;
    _msg.parameter1=AGDSP_CMD_STATUS_CHECK+2;
    _msg.parameter2=AGDSP_CMD_STATUS_CHECK+3;
    agdsp_send_msg(dsp_ctl,&_msg);
    sem_wait(&dsp_ctl->rx.sem);
    if((_msg.command !=dsp_ctl->msg.command) ||
       (_msg.parameter0 !=dsp_ctl->msg.parameter0) ||
       (_msg.parameter1 !=dsp_ctl->msg.parameter1) ||
       (_msg.parameter2 !=dsp_ctl->msg.parameter2)
    ){
        LOG_E("check_agdsp_status failed send:0x%x 0x%x 0x%x 0x%x recv: 0x%x 0x%x 0x%x 0x%x",
            _msg.command,_msg.parameter0,_msg.parameter1,_msg.parameter2,
            dsp_ctl->msg.command,dsp_ctl->msg.parameter0,dsp_ctl->msg.parameter1,dsp_ctl->msg.parameter2);
        return false;
    }
    return true;
}

int agdsp_auto_reset(void * arg,void * params,int opt){
    struct tiny_audio_device *adev = (struct tiny_audio_device *)arg ;
    adev->dev_ctl->agdsp_ctl->auto_reset_dsp = opt;
    LOG_D(" agdsp_auto_reset auto_reset_dsp opt: %d", opt);
    return 0;
}

int agdsp_force_assert_notify(void * arg,void * params,int opt){
    struct tiny_audio_device *adev = (struct tiny_audio_device *)arg ;
    adev->dev_ctl->agdsp_ctl->dsp_assert_force_notify = opt;
    LOG_D(" agdsp_force_assert_notify lodsp_assert_force_notifyg set opt: %d", opt);
    return 0;
}

int agdsp_reboot(void * arg,void * params,int opt) {
    LOG_D(" agdsp_reboot  set opt: %d", opt);
    if(opt) {
        agdsp_boot();
    }
    return 0;
}

int agdsp_log_set(void * arg,void * params,int opt){
    struct tiny_audio_device *adev = (struct tiny_audio_device *)arg ;
    struct dsp_control_t *agdsp_ctl = adev->dev_ctl->agdsp_ctl;
    int ret = 0;
    if(agdsp_ctl->fd_dsp_assert_log > 0) {
        ret = ioctl(agdsp_ctl->fd_dsp_assert_log, DSPLOG_CMD_LOG_ENABLE, opt);
        ALOGE(" agdsp_log_set log set ret: %d, opt: %d",ret, opt);
    }
    return 0;
}

int agdsp_pcmdump_set(void * arg,void * params,int opt){
    struct tiny_audio_device *adev = (struct tiny_audio_device *)arg ;
    struct dsp_control_t *agdsp_ctl = adev->dev_ctl->agdsp_ctl;
    int ret = 0;
    if(agdsp_ctl->fd_dsp_assert_pcm > 0) {
        ret = ioctl(agdsp_ctl->fd_dsp_assert_pcm, DSPLOG_CMD_PCM_ENABLE, opt);
        ALOGE(" agdsp_pcmdump_set log set ret: %d, opt: %d",ret, opt);
    }
    return 0;
}

int agdsp_check_status_test(void * arg,void * params,int opt){
    struct tiny_audio_device *adev = (struct tiny_audio_device *)arg ;
    agdsp_check_status(adev->dev_ctl->agdsp_ctl);
    return 0;
}

static void *agdsp_pipe_process(void *arg){
    int result;
    struct str_parms *parms;
    int ret = 0;
    struct dsp_control_t * dsp_ctl=(struct dsp_control_t *)arg;
    struct dsp_smsg msg;

    LOG_I("begin to receive agdsp pipe message");
    memset(&msg,0,sizeof(struct dsp_smsg));
    ret = read(dsp_ctl->agdsp_pipd_fd,&msg,sizeof(struct dsp_smsg));
    if(ret < 0){
        LOG_E("read data err");
        usleep(2000000);
    }else{
        agdsp_msg_process(dsp_ctl,&msg);
    }

    LOG_D("agdsp_pipe_process exit");
    return NULL;
}


static void *dsp_ctrl_rx_thread_routine(void *arg)
{
    struct dsp_control_t * dsp_ctl=(struct dsp_control_t *)arg;

    pthread_attr_t attr;
    struct sched_param m_param;
    int newprio = 39;

    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    pthread_attr_getschedparam(&attr, &m_param);
    m_param.sched_priority = newprio;
    pthread_attr_setschedparam(&attr, &m_param);

    dsp_ctl->agdsp_pipd_fd = open(AGDSP_CTL_PIPE, O_RDWR);
    if(dsp_ctl->agdsp_pipd_fd < 0){
        LOG_E("%s, open pipe error!! ",__func__);
        return NULL;
    }

    while(!dsp_ctl->rx.is_exit) {
        agdsp_pipe_process(dsp_ctl);
    }
    LOG_W("dsp_ctrl_rx_thread_routine exit!!!");
    return NULL;
}

int send_cmd_to_dsp_thread(struct dsp_control_t *agdsp_ctl,int cmd,void* parameter){
    LOG_D("send_cmd_to_dsp_thread:%d %p",cmd,agdsp_ctl);
    pthread_mutex_lock(&agdsp_ctl->tx.lock);
    agdsp_ctl->tx.cmd=cmd;
    agdsp_ctl->tx.parameter=parameter;
    pthread_mutex_unlock(&agdsp_ctl->tx.lock);
    sem_post(&agdsp_ctl->tx.sem);
    return 0;
}

static void *dsp_ctrl_tx_thread_routine(void *arg)
{
    struct dsp_control_t * dsp_ctl=(struct dsp_control_t *)arg;
    pthread_attr_t attr;
    struct sched_param m_param;
    int newprio = 39;
    int ret = 0;

    LOG_D("dsp_ctrl_tx_thread_routine enter");
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    pthread_attr_getschedparam(&attr, &m_param);
    m_param.sched_priority = newprio;
    pthread_attr_setschedparam(&attr, &m_param);
    ret = pthread_setschedparam(dsp_ctl->tx.thread_id,SCHED_FIFO, &m_param);
    LOG_I("peter: pthread_setschedparam dsp_ctrl_tx_thread_routine ret %d", ret);

    while(!dsp_ctl->tx.is_exit) {
        LOG_I("dsp_ctrl_tx_thread_routine wait begin");
        sem_wait(&dsp_ctl->tx.sem);
        LOG_I("dsp_ctrl_tx_thread_routine wait end cmd:%d",dsp_ctl->tx.cmd);
        pthread_mutex_lock(&dsp_ctl->tx.lock);
        switch(dsp_ctl->tx.cmd){
            case AUDIO_CTL_STOP_VOICE:
                stop_voice_call(dsp_ctl->dev_ctl);
                break;
            case AUDIO_CTL_START_VOICE:{
                    start_voice_call(dsp_ctl->dev_ctl);
                }
                break;
            case AUDIO_TESTER_UPDATAE_AUDIO_PARAM_TO_RAM:    {
                    audio_param_dsp_cmd_t *res=(audio_param_dsp_cmd_t *)dsp_ctl->tx.parameter;
                    if(NULL!=res){
                        struct audio_control *ctl=(struct audio_control *)dsp_ctl->dev_ctl;
                        audiotester_updata_audioparam(res->res,res->opt,true);
                        free(res);
                        force_in_standby_unlock(ctl->adev,AUDIO_HW_APP_NORMAL_RECORD);
                    }
                }
                break;
            case AUDIO_TESTER_UPDATAE_AUDIO_PARAM_TO_FLASH:    {
                    audio_param_dsp_cmd_t *res=(audio_param_dsp_cmd_t *)dsp_ctl->tx.parameter;
                    if(NULL!=res){
                        struct audio_control *ctl=(struct audio_control *)dsp_ctl->dev_ctl;
                        audiotester_updata_audioparam(res->res,res->opt,false);
                        free(res);
                        force_in_standby_unlock(ctl->adev,AUDIO_HW_APP_NORMAL_RECORD);
                    }
                }
                break;
            case RIL_NET_MODE_CHANGE:
                break;
            default:
                break;
        }
        pthread_mutex_unlock(&dsp_ctl->tx.lock);
    }
    LOG_W("dsp_ctrl_tx_thread_routine exit");
    return 0;
}

static void *dsp_ctrl_modemd_notify_thread_routine(void *arg)
{
    int try_count=10;
    int fd = 0;
    struct dsp_control_t * dsp_ctl=(struct dsp_control_t *)arg;
    LOG_D("dsp_ctrl_modemd_notify_thread_routine enter");
    do {
        try_count--;
        fd = socket_local_client("modemd_engpc",
                                 ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
        if(fd < 0 ) {
            LOG_E("modem_monitor_routine:socket_local_client failed %d", errno);
            usleep(2000 * 1000);
        }
    } while((fd < 0) && (try_count > 0));

    if(fd<0){
        return NULL;
    }
    dsp_ctl->fd_modemd_notify = fd;
    LOG_W("dsp_ctrl_modemd_notify_thread_routine exit,dsp_ctl->fd_modemd_notify %d",dsp_ctl->fd_modemd_notify);
    return NULL;
}


int dsp_sleep_ctrl(struct dsp_control_t * dsp_ctl ,bool on_off){
    int ret=0;

    pthread_mutex_lock(&dsp_ctl->lock);
    if(NULL ==dsp_ctl->dsp_sleep_ctl){
        LOG_E("dsp_sleep_ctrl  failed mixer is null");
        ret= -1;
        goto exit;
    }

    if(on_off != dsp_ctl->agdsp_sleep_status){
        ret = mixer_ctl_set_value(dsp_ctl->dsp_sleep_ctl, 0, on_off);
        if (ret != 0) {
            LOG_E("dsp_sleep_ctrl Failed %d\n", on_off);
        }else{
            LOG_I("dsp_sleep_ctrl:%d",on_off);
            dsp_ctl->agdsp_sleep_status=on_off;
        }
    }else{
        LOG_D("dsp_sleep_ctrl  the same values:%d",on_off);
    }

exit:
    pthread_mutex_unlock(&dsp_ctl->lock);
    return ret;
}
void * dsp_ctrl_open(struct audio_control *dev_ctl)
{
    int ret=0;
    LOG_I("dsp_ctrl_open");
    struct dsp_control_t * dsp_ctl = (struct audio_control *)calloc(1, (sizeof(struct audio_control )));

    dsp_ctl->dev=dev_ctl->adev;
    dsp_ctl->dev_ctl = dev_ctl;

#ifdef AUDIO_DEBUG
    dsp_ctl->auto_reset_dsp = 0;
    dsp_ctl->reset_modem = 0;
    dsp_ctl->dsp_assert_force_notify = 1;
#else
    dsp_ctl->auto_reset_dsp = 1;
    dsp_ctl->reset_modem = 0;
    dsp_ctl->dsp_assert_force_notify = 0;
#endif

    if(NULL == dsp_ctl->dsp_sleep_ctl){
        dsp_ctl->dsp_sleep_ctl= mixer_get_ctl_by_name(dev_ctl->mixer, "agdsp_access_en");
    }

    if (pthread_mutex_init(&dsp_ctl->lock, NULL) != 0) {
        LOG_E("dsp_ctrl_open pthread_mutex_init,errno:%u,%s",
              errno, strerror(errno));
        ret=-1;
        goto err;
    }

    ret = sem_init(&dsp_ctl->rx.sem, 0, 0);
    if (ret) {
        ALOGE("sem_init falied, code is %s", strerror(errno));
        goto err;
    }

    dsp_ctl->rx.is_exit=false;

    ret=pthread_create((pthread_t *)(&dsp_ctl->rx.thread_id), NULL,
            dsp_ctrl_rx_thread_routine, dsp_ctl);
    if (ret) {
        LOG_E("vbc_ctrl_open rx failed %d", ret);
        ret=-3;
        goto err;
    }


    ret = sem_init(&dsp_ctl->tx.sem, 0, 0);
    if (ret) {
        ALOGE("sem_init falied, code is %s", strerror(errno));
        goto err;
    }

    if (pthread_mutex_init(&dsp_ctl->tx.lock, NULL) != 0) {
        LOG_E("dsp_ctrl_open pthread_mutex_init tx lock,errno:%u,%s",
              errno, strerror(errno));
        ret=-1;
        goto err;
    }

    dsp_ctl->tx.is_exit=false;
    dsp_ctl->tx.cmd=AUDIO_CTL_INVALID;

    ret=pthread_create((pthread_t *)(&dsp_ctl->tx.thread_id), NULL,
            dsp_ctrl_tx_thread_routine, dsp_ctl);
    if (ret) {
        LOG_E("vbc_ctrl_open rx failed %d", ret);
        ret=-3;
        goto err;
    }

    dsp_ctl->fd_dsp_assert_mem = open(SPRD_AUD_DSPASSERT_MEM,O_RDWR);
    if(dsp_ctl->fd_dsp_assert_mem <= 0) {
        LOG_E("dsp_ctl->fd_dsp_assert_mem open failed %d",dsp_ctl->fd_dsp_assert_mem);
    }

    dsp_ctl->fd_dsp_assert_log = open(SPRD_AUD_DSPASSERT_LOG,O_RDWR);
    if(dsp_ctl->fd_dsp_assert_log <= 0) {
        LOG_E("dsp_ctl->fd_dsp_assert_log open failed %d",dsp_ctl->fd_dsp_assert_log);
    }

    dsp_ctl->fd_dsp_assert_pcm = open(SPRD_AUD_DSPASSERT_PCM,O_RDWR);
    if(dsp_ctl->fd_dsp_assert_pcm <= 0) {
        LOG_E("dsp_ctl->fd_dsp_assert_pcm open failed %d",dsp_ctl->fd_dsp_assert_pcm);
    }
    ret=pthread_create((pthread_t *)(&dsp_ctl->tx.thread_id), NULL,
    dsp_ctrl_modemd_notify_thread_routine, dsp_ctl);
    if (ret) {
        LOG_E("vbc_ctrl_open rx failed %d", ret);
        ret=-3;
        goto err;
    }
    return dsp_ctl;
err:
    if(ret!=0){
        LOG_E("dsp_ctrl_open failed %d", ret);
    }
    return NULL;
}

void dsp_ctrl_close(struct dsp_control_t * dsp_ctl)
{
    LOG_I("dsp_ctrl_close enter");

    dsp_ctl->rx.is_exit=true;
    pthread_join(dsp_ctl->rx.thread_id, NULL);
    sem_destroy(&dsp_ctl->rx.sem);

    if(dsp_ctl->agdsp_pipd_fd>0){
        close(dsp_ctl->agdsp_pipd_fd);
        dsp_ctl->agdsp_pipd_fd=-1;
    }
    LOG_I("dsp_ctrl_close exit");
}
