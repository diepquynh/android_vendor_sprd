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
#include "dsp_control.h"
#include "audio_debug.h"
#include "audio_hw.h"

#include "audio_control.h"

#define LOG_TAG "audio_hw_dsp"

int set_net_mode(void * dev,struct str_parms *parms,int mode, char * val){
    int net_mode=0;
    int stream_status=0;
    bool mode_change=false;
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev ;
    struct dsp_control_t * dsp_ctl=&(adev->agdsp_ctl);
    LOG_I("set_net_mode pre:%d now:%d",dsp_ctl->net_mode,mode);

    pthread_mutex_lock(&dsp_ctl->lock);
    net_mode = dsp_ctl->net_mode;
    if(net_mode!=mode){
        mode_change=true;
        dsp_ctl->net_mode = mode;

    }
    pthread_mutex_unlock(&dsp_ctl->lock);

    if(true==mode_change){
        pthread_mutex_lock(&adev->lock);
        stream_status =adev->stream_status;
        pthread_mutex_unlock(&adev->lock);

        if(stream_status & (1<<AUDIO_HW_APP_CALL)){
            send_cmd_to_dsp_thread(adev,RIL_NET_MODE_CHANGE,NULL);
        }else{
            LOG_D("set_net_mode:not calling,stream_status:0x%x",stream_status);
        }
    }

    return 0;
}

static int agdsp_msg_process(struct tiny_audio_device *adev,struct dsp_smsg *msg){
    int ret=0;
    LOG_I("agdsp_msg_process cmd:0x%x parameter:0x%x 0x%x 0x%x",
        msg->command,msg->parameter0,msg->parameter1,msg->parameter2);
    switch(msg->command){
        case 0:
            break;
        case AGDSP_CMD_STATUS_CHECK:
            memcpy(&adev->agdsp_ctl.msg,msg,sizeof(struct dsp_smsg));
            sem_post(&adev->agdsp_ctl.rx.sem);
            break;
        default:
            break;
    }
    return ret;
}

static int agdsp_send_msg(struct dsp_control_t * dsp_ctl,struct dsp_smsg *msg){
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

int agdsp_send_msg_test(void * arg,void * params,int opt){
    struct tiny_audio_device *adev = (struct tiny_audio_device *)arg ;
    struct dsp_smsg _msg;
    _msg.command=opt;
    _msg.parameter0=opt+1;
    _msg.parameter1=opt+2;
    _msg.parameter2=opt+3;
    agdsp_send_msg(&adev->agdsp_ctl,&_msg);
    return 0;
}

bool agdsp_check_status(struct dsp_control_t * dsp_ctl){
    struct dsp_smsg _msg;
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

int agdsp_check_status_test(void * arg,void * params,int opt){
    struct tiny_audio_device *adev = (struct tiny_audio_device *)arg ;
    agdsp_check_status(&adev->agdsp_ctl);
    return 0;
}

static void *agdsp_pipe_process(void *arg){
    int result;
    struct str_parms *parms;
    int ret = 0;
    struct dsp_control_t * dsp_ctl=(struct dsp_control_t *)arg;
    struct tiny_audio_device *adev = (struct tiny_audio_device *)(dsp_ctl->dev);
    struct dsp_smsg msg;

    LOG_I("begin to receive agdsp pipe message");
    memset(&msg,0,sizeof(struct dsp_smsg));
    ret = read(dsp_ctl->agdsp_pipd_fd,&msg,sizeof(struct dsp_smsg));
    if(ret < 0){
        LOG_E("read data err");
    }else{
        agdsp_msg_process(adev,&msg);
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
        return -1;
    }

    while(!dsp_ctl->rx.is_exit) {
        agdsp_pipe_process(dsp_ctl);
    }
    LOG_W("dsp_ctrl_rx_thread_routine exit!!!");
    return 0;
}

int send_cmd_to_dsp_thread(struct dsp_control_t *agdsp_ctl,int cmd,void* parameter){
    LOG_D("send_cmd_to_dsp_thread:%d",cmd);
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
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dsp_ctl->dev;
    pthread_attr_t attr;
    struct sched_param m_param;
    int newprio = 39;

    LOG_D("dsp_ctrl_tx_thread_routine enter");
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    pthread_attr_getschedparam(&attr, &m_param);
    m_param.sched_priority = newprio;
    pthread_attr_setschedparam(&attr, &m_param);

    while(!dsp_ctl->tx.is_exit) {
        LOG_I("dsp_ctrl_tx_thread_routine wait begin");
        sem_wait(&dsp_ctl->tx.sem);
        LOG_I("dsp_ctrl_tx_thread_routine wait end cmd:%d",dsp_ctl->tx.cmd);
        pthread_mutex_lock(&dsp_ctl->tx.lock);
        switch(dsp_ctl->tx.cmd){
            case AUDIO_CTL_STOP_VOICE:
                stop_voice_call(adev->dev_ctl);
                break;
            case AUDIO_CTL_START_VOICE:
                start_voice_call(adev->dev_ctl);
                break;
            case AUDIO_TESTER_UPDATAE_AUDIO_PARAM_TO_RAM:    {
                    audio_param_dsp_cmd_t *res=(audio_param_dsp_cmd_t *)dsp_ctl->tx.parameter;
                    if(NULL!=res){
                        audiotester_updata_audioparam(res->res,res->opt,true);
                        free(res);
                        set_dsp_volume(adev->dev_ctl,adev->dev_ctl->voice_volume);
                    }
                }
                break;
            case AUDIO_TESTER_UPDATAE_AUDIO_PARAM_TO_FLASH:    {
                    audio_param_dsp_cmd_t *res=(audio_param_dsp_cmd_t *)dsp_ctl->tx.parameter;
                    if(NULL!=res){
                        audiotester_updata_audioparam(res->res,res->opt,false);
                        free(res);
                        set_dsp_volume(adev->dev_ctl,adev->dev_ctl->voice_volume);
                    }
                }
                break;
            case RIL_NET_MODE_CHANGE:
                select_audio_param(adev,false);
                break;
            default:
                break;
        }
        pthread_mutex_unlock(&dsp_ctl->tx.lock);
    }
    LOG_W("dsp_ctrl_tx_thread_routine exit");
    return 0;
}

int dsp_sleep_ctrl(struct dsp_control_t * dsp_ctl ,bool on_off){
    int ret=0;

    if((NULL !=dsp_ctl->dsp_sleep_ctl) && (on_off != dsp_ctl->agdsp_sleep_status)){
        ret = mixer_ctl_set_value(dsp_ctl->dsp_sleep_ctl, 0, on_off);
        if (ret != 0) {
            LOG_E("dsp_sleep_ctrl Failed \n");
        }else{
            LOG_I("dsp_sleep_ctrl:%d",on_off);
        }
        dsp_ctl->agdsp_sleep_status=on_off;
    }else{
        LOG_E("dsp_sleep_ctrl  failed mixer is %d on_off:%d",dsp_ctl->dsp_sleep_ctl,on_off);
    }

    return ret;
}
int dsp_ctrl_open(void * arg)
{
    int ret=0;
    LOG_I("dsp_ctrl_open");
    struct tiny_audio_device *adev = (struct tiny_audio_device *)arg ;
    struct dsp_control_t * dsp_ctl = &(adev->agdsp_ctl);

    dsp_ctl->dev=adev;

    if(NULL == dsp_ctl->dsp_sleep_ctl){
        dsp_ctl->dsp_sleep_ctl= mixer_get_ctl_by_name(adev->dev_ctl->mixer, "agdsp_access_en");
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

err:
    if(ret!=0){
        LOG_E("dsp_ctrl_open failed %d", ret);
    }
    return ret;
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
