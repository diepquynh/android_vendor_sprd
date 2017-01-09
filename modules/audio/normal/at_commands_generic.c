
#ifdef ANDROID5X
#include "AtChannel.h"
#else
#include "sprd_atci.h"
#endif

#define VOICECALL_VOLUME_MAX_UI	6

enum {
     ROUTE_BIT = 0,
     VOLUME_BIT = 1,
     MICMUTE_BIT = 2,
     DOWNLINKMUTE_BIT = 3,
     AUDIOLOOP_BIT = 4,
     USERCASE_BIT = 5,
     EXTRAVOLUME_BIT = 6,
     BTSAMPLE_BIT = 7,
     SPPCMDUMP_BIT=8,
     MAX_BIT = 9,
};
static pthread_mutex_t  ATlock = PTHREAD_MUTEX_INITIALIZER;         //eng cannot handle many at commands once
static int at_cmd_routeDev(struct tiny_audio_device *adev,char* route,T_AT_CMD* at);

int do_cmd_dual(int modemId, int simId, struct tiny_audio_device *adev)
{
    const char *err_str = NULL;
    int indx = 0;
    T_AT_CMD process_at_cmd = {0};
    int dirty_count= 0;
    int max_pri = 0;
    int max_pri_bit = 0;
    uint8_t dirty_indx = 0;
    uint8_t cmd_queue_pri[MAX_AT_CMD_TYPE] = {0};
    uint8_t cmd_bit = 0;
    pthread_mutex_lock(&ATlock);
    dirty_count = __builtin_popcount(adev->at_cmd_vectors->at_cmd_dirty);
    if(dirty_count != 0){
        memcpy(&process_at_cmd, adev->at_cmd_vectors, sizeof(T_AT_CMD ));
        memset(adev->at_cmd_vectors, 0x00, sizeof(T_AT_CMD ));
    }
    pthread_mutex_unlock(&ATlock);

    ALOGV("do_cmd_dual Switch incall AT command dirty_count:[%d] : [%d] :[%0x]", dirty_count,sizeof(process_at_cmd.at_cmd)/sizeof(process_at_cmd.at_cmd[0]),process_at_cmd.at_cmd_dirty);
    for(dirty_indx = 0;dirty_indx < dirty_count;dirty_indx++){
        max_pri = process_at_cmd.at_cmd_priority[0];
        max_pri_bit = 0;
        for(indx=0;indx < sizeof(process_at_cmd.at_cmd)/sizeof(process_at_cmd.at_cmd[0]);indx++){
            if(max_pri < process_at_cmd.at_cmd_priority[indx]){
                max_pri = process_at_cmd.at_cmd_priority[indx];
                max_pri_bit = indx;
            }
        }
        process_at_cmd.at_cmd_priority[max_pri_bit] = 0;
        cmd_queue_pri[dirty_indx] = max_pri_bit;
        ALOGV("do_cmd_dual Switch incall AT command dirty Bit :[%x] ", max_pri_bit);
    }
    for(indx=0;indx < dirty_count;indx++){
        cmd_bit = cmd_queue_pri[dirty_count-indx-1];
        ALOGV("do_cmd_dual Switch incall AT command [%d][%d][%s][%d] ", modemId,simId,&(process_at_cmd.at_cmd[cmd_bit]),cmd_bit);
        adev->routeDev = at_cmd_routeDev(adev,&(process_at_cmd.at_cmd[cmd_bit]),&process_at_cmd);
#ifdef ANDROID5X
        err_str = sendAt(modemId, simId, &(process_at_cmd.at_cmd[cmd_bit]));
#else
        err_str = sendCmd(simId, &(process_at_cmd.at_cmd[cmd_bit]));
#endif
        ALOGV("do_cmd_dual Switch incall AT command [%s][%s] ", &(process_at_cmd.at_cmd[cmd_bit]), err_str);
    }
    return 0;
}
static uint8_t process_priority(struct tiny_audio_device *adev,int bit){
    int indx = 0;
    int max_priority = adev->at_cmd_vectors->at_cmd_priority[0];
    int pri = 0;
    for(indx = 0;indx < sizeof(adev->at_cmd_vectors->at_cmd)/sizeof(adev->at_cmd_vectors->at_cmd[0]);indx++)
    {
        if(max_priority < adev->at_cmd_vectors->at_cmd_priority[indx]){
          max_priority = adev->at_cmd_vectors->at_cmd_priority[indx];
        }
    }
    pri = max_priority + 1;
    return pri;
}

static void do_voice_command(struct tiny_audio_device *adev)
{
    ALOGV("do_voice_command: E,%d,%d,%0x,dirty:%0x",sizeof(T_AT_CMD),sizeof(adev->at_cmd_vectors->at_cmd[0]),&(adev->at_cmd_vectors->at_cmd[0]),adev->at_cmd_vectors->at_cmd_dirty);
    do_cmd_dual(st_vbc_ctrl_thread_para->adev->cp_type, android_sim_num, adev);
    ALOGE("do_voice_command: X");
}

static void voice_command_signal(struct tiny_audio_device *adev,const  char* at_cmd,int index)
{
    ALOGV("voice_command_signal: E");
    sem_post(&adev->voice_command_mgr.device_switch_sem);
    ALOGE("voice_command_signal: X");
}
static void push_voice_command(char *at_cmd,int bit){

    ALOGV("push_voice_command: E");
    ALOGV("push_voice_command: at_cmd:%s,bit:%d,precmd:%s,len:%d",at_cmd,bit,&(s_adev->at_cmd_vectors->at_cmd[bit]),sizeof(s_adev->at_cmd_vectors->at_cmd[bit]));
    pthread_mutex_lock(&ATlock);
    s_adev->at_cmd_vectors->at_cmd_dirty |= (0x01 << bit);
    memset(&(s_adev->at_cmd_vectors->at_cmd[bit]),0x00,sizeof(s_adev->at_cmd_vectors->at_cmd[bit]));
    strncpy(&(s_adev->at_cmd_vectors->at_cmd[bit]),at_cmd,strlen(at_cmd));
    s_adev->at_cmd_vectors->at_cmd_priority[bit]  = process_priority(s_adev,bit);
    ALOGE("%s: post:%s,priority:%d",__func__,&(s_adev->at_cmd_vectors->at_cmd[bit]) ,s_adev->at_cmd_vectors->at_cmd_priority[bit]);
    pthread_mutex_unlock(&ATlock);
    voice_command_signal(s_adev,at_cmd,bit);
    ALOGE("push_voice_command: X,at_cmd:%s,bit:%d,postcmd:%s",at_cmd,bit,&(s_adev->at_cmd_vectors->at_cmd[bit]));
}

static void push_route_command(char *at_cmd,int bit,int out){
    ALOGV("push_route_command: E");
    ALOGE("push_route_command: at_cmd:%s,bit:%d,precmd:%s,len:%d",at_cmd,bit,&(s_adev->at_cmd_vectors->at_cmd[bit]),sizeof(s_adev->at_cmd_vectors->at_cmd[bit]));
    pthread_mutex_lock(&ATlock);
    s_adev->at_cmd_vectors->at_cmd_dirty |= (0x01 << bit);
    memset(&(s_adev->at_cmd_vectors->at_cmd[bit]),0x00,sizeof(s_adev->at_cmd_vectors->at_cmd[bit]));
    strncpy(&(s_adev->at_cmd_vectors->at_cmd[bit]),at_cmd,strlen(at_cmd));
    s_adev->at_cmd_vectors->routeDev  = out;
    s_adev->at_cmd_vectors->at_cmd_priority[bit]  = process_priority(s_adev,bit);
    ALOGE("%s: post:%s,priority:%d",__func__,&(s_adev->at_cmd_vectors->at_cmd[bit]) ,s_adev->at_cmd_vectors->at_cmd_priority[bit]);
    pthread_mutex_unlock(&ATlock);
    voice_command_signal(s_adev,at_cmd,bit);
    ALOGE("push_route_command: X,at_cmd:%s,bit:%d,postcmd:%s",at_cmd,bit,&(s_adev->at_cmd_vectors->at_cmd[bit]));
}
// 0x80 stands for 8KHz(NB) sampling rate BT Headset.
// 0x40 stands for 16KHz(WB) sampling rate BT Headset.
static int config_bt_dev_type(int bt_headset_type, cp_type_t cp_type, int cp_sim_id,struct tiny_audio_device *adev)
{
    char *at_cmd = NULL;

    if (bt_headset_type == VX_NB_SAMPLING_RATE) {
        at_cmd = "AT+SSAM=128";
    } else if (bt_headset_type == VX_WB_SAMPLING_RATE) {
        at_cmd = "AT+SSAM=64";
    }
    if(at_cmd)
        push_voice_command(at_cmd,BTSAMPLE_BIT);
    usleep(10000);
    return 0;
}

static int at_cmd_cp_pcm_dump(char *at_cmd)
{
    ALOGI("%s : at_cmd: %s",__func__,at_cmd);
    push_voice_command(at_cmd,SPPCMDUMP_BIT);
    usleep(10000);
    return 0;
}

static int at_cmd_routeDev(struct tiny_audio_device *adev,char* route,T_AT_CMD* at)
{
    char *prefix = "AT+SSAM=";
    int len = strlen(prefix);
    int dev = 0;
    if(0 == strncmp(prefix,route,len)){
        dev = at->routeDev;
        ALOGW("%s %s routeDev:%x",__func__,route,dev);
    }
    return dev;
}

static int at_cmd_route(struct tiny_audio_device *adev)
{
    const char *at_cmd = NULL;
    if ((adev->mode != AUDIO_MODE_IN_CALL) && (!adev->voip_start)) {
        ALOGE("Error: NOT mode_in_call, current mode(%d)", adev->mode);
        return -1;
    }

    if (adev->out_devices & (AUDIO_DEVICE_OUT_WIRED_HEADSET | AUDIO_DEVICE_OUT_WIRED_HEADPHONE)) {
#ifndef AUDIO_OLD_MODEM
        if(!headset_no_mic())
            at_cmd = "AT+SSAM=2";
        else
            at_cmd = "AT+SSAM=4";
#else
        at_cmd = "AT+SSAM=2";
#endif
    } else if (adev->out_devices & (AUDIO_DEVICE_OUT_BLUETOOTH_SCO
                                | AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET
                                | AUDIO_DEVICE_OUT_BLUETOOTH_SCO_CARKIT)) {
        if (adev->bluetooth_type)
            config_bt_dev_type(adev->bluetooth_type,
                    st_vbc_ctrl_thread_para->adev->cp_type, android_sim_num,adev);

        if (adev->bluetooth_nrec) {
            at_cmd = "AT+SSAM=5";
        } else {
            at_cmd = "AT+SSAM=6";
        }
    } else if (adev->out_devices & AUDIO_DEVICE_OUT_SPEAKER) {
        at_cmd = "AT+SSAM=1";
    } else {
        at_cmd = "AT+SSAM=0";
    }
    push_route_command(at_cmd,ROUTE_BIT,adev->out_devices);
    return 0;
}

int at_cmd_volume(float vol, int mode)
{
    char buf[16];
    char *at_cmd = buf;
    int volume = vol * VOICECALL_VOLUME_MAX_UI + 1;

    if (volume >= VOICECALL_VOLUME_MAX_UI) volume = VOICECALL_VOLUME_MAX_UI;
    ALOGI("%s mode=%d ,volume=%d, android vol:%f ", __func__,mode,volume,vol);
    snprintf(at_cmd, sizeof buf, "AT+VGR=%d", volume);

    push_voice_command(at_cmd,VOLUME_BIT);
    return 0;
}

int at_cmd_mic_mute(bool mute)
{
    const char *at_cmd;
    ALOGW("audio at_cmd_mic_mute %d", mute);
    if (mute) at_cmd = "AT+CMUT=1";
    else at_cmd = "AT+CMUT=0";

    push_voice_command(at_cmd,MICMUTE_BIT );
    return 0;
}

int at_cmd_downlink_mute(bool mute)
{
    char r_buf[32];
    const char *at_cmd;
    ALOGW("audio at_cmd_downlink_mute set %d", mute);
    if (mute){
        at_cmd = "AT+SDMUT=1";
    }
    else{
        at_cmd = "AT+SDMUT=0";
    }
    push_voice_command(at_cmd,DOWNLINKMUTE_BIT);
    return 0;
}

int at_cmd_audio_loop(int enable, int mode, int volume,int loopbacktype,int voiceformat,int delaytime)
{
    char buf[89];
    char *at_cmd = buf;
    if(volume >9) {
        volume = 9;
    }
    ALOGW("audio at_cmd_audio_loop enable:%d,mode:%d,voluem:%d,loopbacktype:%d,voiceformat:%d,delaytime:%d",
            enable,mode,volume,loopbacktype,voiceformat,delaytime);

    snprintf(at_cmd, sizeof buf, "AT+SPVLOOP=%d,%d,%d,%d,%d,%d", enable,mode,volume,loopbacktype,voiceformat,delaytime);

    push_voice_command(at_cmd,AUDIOLOOP_BIT );
    return 0;

}

int at_cmd_cp_usecase_type(audio_cp_usecase_t type)
{
    char buf[89];
    char *at_cmd = buf;
    if(type > AUDIO_CP_USECASE_MAX) {
        type = AUDIO_CP_USECASE_VOIP_1;
    }
    ALOGW("%s, type:%d ",__func__,type);

    snprintf(at_cmd, sizeof buf, "AT+SPAPAUDMODE=%d", type);

    push_voice_command(at_cmd,USERCASE_BIT );
    return 0;

}

/* This function is for samsung's extra volume solution, according to cp, extraVolume has a minimum:0x1000 */
int at_cmd_extra_volume(bool enable, int extraVolume)
{
    char buf[89];
    char *at_cmd = buf;
    if(extraVolume < 0) {
        ALOGW("%s, wrong extraVolume(%d), set to default 0x1000 ",__func__, extraVolume);
        extraVolume = 0x1000;
    }
    ALOGW("%s, enable:%d, extraVolume:0x%x ",__func__, enable, extraVolume);

    snprintf(at_cmd, sizeof buf, "AT+SPAUDIOCONFIG=extravgr,%d,%d", enable, extraVolume);

    push_voice_command(at_cmd,EXTRAVOLUME_BIT );
    return 0;
}
