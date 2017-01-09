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

#include <sys/select.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <cutils/log.h>
#include <cutils/str_parms.h>
#include <cutils/properties.h>
#include "audio_debug.h"
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <expat.h>
#include <audio_hw.h>
#include "audio_control.h"

#define LOG_TAG "audio_hw_ext_control"

extern int audPlayerOpen( int outdev, int sampleRate, int stereo, int sync );
extern int audPlayerPlay( const unsigned char * data, int size );
extern int audPlayerStop( void );
extern int audPlayerClose( void );
extern int audRcrderOpen( int indev, int sampleRate );
extern int audRcrderRecord( unsigned char * data, int size );
extern int audRcrderStop( void );
extern int audRcrderClose( void );
extern int sprd_audiorecord_test_start(int devices);
extern int sprd_audiorecord_test_read(int size);
extern int sprd_audiorecord_test_stop(void);
extern int sprd_audiotrack_test_start(int channel,int devices, char *buf, int size);
extern int sprd_audiotrack_test_stop(void);

extern int audio_out_devices_test(void * adev,struct str_parms *parms,bool is_start);
extern int audio_in_devices_test(void * adev,struct str_parms *parms,bool is_start);
extern int sprd_audioloop_test(void * adev,struct str_parms *parms,bool is_start);
extern int sprd_audio_dev_laohua_test(void * adev,struct str_parms *parms);
extern int audio_dsp_loop(void * adev,struct str_parms *parms,bool is_start);
extern int agdsp_send_msg_test(void * arg,void * params,int opt);
extern int agdsp_check_status_test(void * arg,void * params,int opt);
extern int audio_agdsp_reset(void * dev,struct str_parms *parms,bool is_start);
extern int sprd_setStreamVolume(audio_stream_type_t stream,int level);
extern int ext_setVoiceMode(void * dev,struct str_parms *parms,int mode, char * val);
extern int agdsp_log_set(void * arg,void * params,int opt);
extern int agdsp_auto_reset(void * arg,void * params,int opt);
extern int agdsp_force_assert_notify(void * arg,void * params,int opt);
extern int agdsp_log_set(void * arg,void * params,int opt);
extern int agdsp_pcmdump_set(void * arg,void * params,int opt);
extern int agdsp_reboot(void * arg,void * params,int opt);
extern int vbc_playback_dump(void *dev,struct str_parms *parms,int opt, char * val);

typedef int  (*AUDIO_EXT_CONTROL_FUN)(void *adev,struct str_parms *parms,int opt, char * value);
struct ext_control_t{
    char *cmd_string;
    AUDIO_EXT_CONTROL_FUN fun;
};


int set_wb_mode(void * dev,struct str_parms *parms,int mode, char * val){
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev ;
    struct voice_net_t net_infor;
    net_infor.net_mode=(aud_net_m)adev->dev_ctl->param_res.net_mode;
    net_infor.wb_mode=mode;
    return set_audioparam(adev->dev_ctl,PARAM_NET_CHANGE,&net_infor,false);
}

int set_net_mode(void * dev,struct str_parms *parms,int mode, char * val){
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev ;
    struct voice_net_t net_infor;
    net_infor.net_mode=(aud_net_m)mode;
    net_infor.wb_mode=adev->dev_ctl->param_res.wb_mode;
    return set_audioparam(adev->dev_ctl,PARAM_NET_CHANGE,&net_infor,false);
}

static int audio_loglevel_ctrl(void *dev,struct str_parms *parms,int opt, char * val){
    log_level=opt;
    LOG_I("audio_loglevel_ctrl:%d",log_level);
    return log_level;
}

static int audio_codec_mute(void *dev,struct str_parms *parms,int opt, char * val){
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev;
    LOG_I("audio_codec_mute:%d",opt);
    set_codec_mute(adev->dev_ctl,opt);
    return opt;
}

static int audiotester_enable(void *dev,struct str_parms *parms,int opt, char * val){
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev;
    LOG_I("audio_tester_enable:%d",opt);
    pthread_mutex_lock(&adev->lock);
    if((opt) && (false == adev->audio_param.tunning.running)){
        start_audio_tunning_server(adev);
    }
    pthread_mutex_unlock(&adev->lock);
    return opt;
}


static int audio_playbackdump_ctrl(void *dev,struct str_parms *parms,int opt, char * val){
    struct playback_dump *playback;
    int ret=-1;
    struct tiny_audio_device *adev=(struct tiny_audio_device *)dev;
    playback=&(adev->dev_ctl->config.dump.playback);
    char value[128];
    if(opt==0){
        ret = str_parms_get_str(parms,"dump_point", value, sizeof(value));
        if(ret >= 0){
            if(strncmp(value,"offload",strlen("offload"))){
                playback->offload.hex_enable=0;
                if(playback->offload.hex_fd>0){
                    close(playback->offload.hex_fd);
                    playback->offload.hex_fd=-1;
                }
            }else if(strncmp(value,"normal",strlen("normal"))){
                playback->normal.hex_enable=0;
                if(playback->normal.hex_fd>0){
                    close(playback->normal.hex_fd);
                    playback->normal.hex_fd=-1;
                }
            }
        } 
    }else{
        ret = str_parms_get_str(parms,"dump_point", value, sizeof(value));
        if(ret >= 0){
            if(strncmp(value,"offload",strlen("offload"))){
                playback->offload.hex_enable=1;
                if(playback->offload.hex_fd<0){
                    playback->offload.hex_fd=open(playback->offload.hex_file_name, O_RDWR | O_CREAT,
                    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                    if(playback->offload.hex_fd < 0) {
                        LOG_E("open %s failed\n",playback->offload.hex_file_name);
                        playback->offload.hex_enable=0;
                    }
                }
            }else if(strncmp(value,"normal",strlen("normal"))){
                playback->normal.hex_enable=1;
                if(playback->normal.hex_fd<0){
                    playback->normal.hex_fd=open(playback->normal.hex_file_name, O_RDWR | O_CREAT,
                    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                    if(playback->normal.hex_fd < 0) {
                        LOG_E("open %s failed\n",playback->normal.hex_file_name);
                        playback->normal.hex_enable=0;
                    }
                }
            }
        } 
    }
    return 0;
}

static int audio_recorddump_ctrl(void *dev,struct str_parms *parms,int opt, char * val){
    struct record_dump *record;
    int ret=-1;
    struct tiny_audio_device *adev=(struct tiny_audio_device *)dev;
    record=&(adev->dev_ctl->config.dump.record);
    char value[128];
    if(opt==0){
        ret = str_parms_get_str(parms,"dump_point", value, sizeof(value));
        if(ret >= 0){
            if(strncmp(value,"normal",strlen("normal"))){
                record->normal.hex_enable=0;
                if(record->normal.hex_fd>0){
                    close(record->normal.hex_fd);
                    record->normal.hex_fd=-1;
                }
            }
        } 
    }else{
        ret = str_parms_get_str(parms,"dump_point", value, sizeof(value));
        if(ret >= 0){
            if(strncmp(value,"normal",strlen("normal"))){
                record->normal.hex_enable=1;
                if(record->normal.hex_fd<0){
                    record->normal.hex_fd=open(record->normal.hex_file_name, O_RDWR | O_CREAT,
                    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                    if(record->normal.hex_fd < 0) {
                        LOG_E("open %s failed\n",record->normal.hex_file_name);
                        record->normal.hex_enable=0;
                    }
                }
            }
        } 
    }
    return 0;
}

static int set_bt_samplerate(void * dev,struct str_parms *parms,int rate, char * val){
    int ret=-1;
    struct tiny_audio_device *adev=(struct tiny_audio_device *)dev;
    LOG_D("set_bt_samplerate:0x%x",rate);
    set_vbc_bt_src(adev->dev_ctl,rate);
    return ret;
}

static int set_bt_wbs(void * dev,struct str_parms *parms,int rate, char * val){
    int ret=-1;
    struct tiny_audio_device *adev=(struct tiny_audio_device *)dev;
    LOG_D("set_bt_wbs:%s",val);
    if (strcmp(val, "on") == 0) {
        set_vbc_bt_src(adev->dev_ctl,16000);
        adev->bt_wbs = true;
    }
    else {
        set_vbc_bt_src(adev->dev_ctl,8000);
        adev->bt_wbs = false;
    }
    return ret;
}

static int set_bt_nrec(void * dev,struct str_parms *parms,int opt, char * val){
    int ret=-1;
    struct tiny_audio_device *adev=(struct tiny_audio_device *)dev;
    LOG_D("set_bt_nrec:%s",val);
    if (strcmp(val, "on") == 0)
        set_vbc_bt_nrec(adev->dev_ctl,false);//Bluetooth headset don't support nrec
    else
        set_vbc_bt_nrec(adev->dev_ctl,true);//Bluetooth headset support nrec
    return ret;
}


static int set_in_stream_route(void * dev,struct str_parms *parms,int devices, char * val){
    int ret=-1;
    struct tiny_audio_device *adev=(struct tiny_audio_device *)dev;
    LOG_D("set_in_stream_route:0x%x",devices);
    pthread_mutex_lock(&adev->lock);
    ret=select_devices_new(adev->dev_ctl,AUDIO_HW_APP_INVALID,devices,true,true, true);
    pthread_mutex_unlock(&adev->lock);
    return ret;
}

static int set_out_stream_route(void * dev,struct str_parms *parms,int devices, char * val){
    int ret=-1;
    struct tiny_audio_device *adev=(struct tiny_audio_device *)dev;
    LOG_D("set_out_stream_route:0x%x",devices);
    pthread_mutex_lock(&adev->lock);
    ret=select_devices_new(adev->dev_ctl,AUDIO_HW_APP_INVALID,devices,false,true, true);
    pthread_mutex_unlock(&adev->lock);
    return ret;
}

static int connect_audio_devices(void * dev,struct str_parms *parms,int devices, char * val){
    int ret=-1;
    struct tiny_audio_device *adev=(struct tiny_audio_device *)dev;
    LOG_D("connect_audio_devices:0x%x",devices);
    pthread_mutex_lock(&adev->lock);
    if(AUDIO_DEVICE_BIT_IN & devices){
        adev->dev_ctl->in_devices=devices;
    }else{
        ret=select_devices_new(adev->dev_ctl,AUDIO_HW_APP_INVALID,devices,false,true, true);
        if (is_usecase(adev->dev_ctl,UC_LOOP |UC_CALL|UC_VOIP)) {
            if(adev->dev_ctl->out_devices !=AUDIO_DEVICE_OUT_WIRED_HEADSET){
                adev->dev_ctl->in_devices =0;
                if(adev->dev_ctl->config.mic_switch & (1<<0)){
                    adev->dev_ctl->in_devices|=AUDIO_DEVICE_IN_BUILTIN_MIC;
                }

                if(adev->dev_ctl->config.mic_switch & (1<<1)){
                    adev->dev_ctl->in_devices|=AUDIO_DEVICE_IN_BACK_MIC;
                }
            }else{
                adev->dev_ctl->in_devices=AUDIO_DEVICE_IN_WIRED_HEADSET;
            }
            select_devices_new(adev->dev_ctl,AUDIO_HW_APP_INVALID,adev->dev_ctl->in_devices,false,true, true);
        }
    }
    pthread_mutex_unlock(&adev->lock);
    return ret;
}

static int disconnect_audio_devices(void * dev,struct str_parms *parms,int devices, char * val){
    int ret=-1;
    struct tiny_audio_device *adev=(struct tiny_audio_device *)dev;

    if(0==(AUDIO_DEVICE_BIT_IN&devices)){
        if(AUDIO_DEVICE_OUT_ALL_SCO&devices){
            set_vbc_bt_src(adev->dev_ctl,8000);
            set_vbc_bt_nrec(adev->dev_ctl,false);
        }
    }

    return ret;
}
static int sprd_audiotrack_test(void * adev,struct str_parms *parms,bool is_start, char * val){
    int channel=0;
    int devices=0;
    char value[AUDIO_EXT_CONTROL_PIPE_MAX_BUFFER_SIZE];
    int ret = -1;
    int val_int = 0;
    int data_size=0;
    char *data=NULL;
    LOG_D("%s %d",__func__,__LINE__);
    if(true==is_start){
        ret = str_parms_get_str(parms,"autotest_outdevices", value, sizeof(value));
        if(ret >= 0){
            LOG_D("%s %d",__func__,__LINE__);
            devices = strtoul(value,NULL,0);
            LOG_D("%s %d %d",__func__,__LINE__,devices);
        }

        ret = str_parms_get_str(parms,"autotest_channels", value, sizeof(value));
        if(ret >= 0){
            channel = strtoul(value,NULL,0);
            LOG_D("%s %d %d",__func__,__LINE__,channel);
        }

        ret = str_parms_get_str(parms,"autotest_datasize", value, sizeof(value));
        if(ret >= 0){
            data_size = strtoul(value,NULL,0);
            LOG_D("%s %d %d",__func__,__LINE__,data_size);

            if(data_size>0){
                data=(char *)malloc(data_size);
                if(NULL == data){
                    LOG_E("sprd_audiotrack_test malloc failed");
                    data_size=0;
                }
            }
        }

        ret = str_parms_get_str(parms,"autotest_data", value, sizeof(value));
        if(ret >= 0){
            if(NULL==data){
                LOG_E("autotest_data NULL ERR");
            }

            int size =string_to_hex(data,value,data_size);
            if(data_size!=size){
                LOG_E("autotest_data ERR:%x %x",size,data_size);
            }
        }

        ret=sprd_audiotrack_test_start(channel,devices,data,data_size);
        if(NULL!=data){
            free(data);
        }
    }else{
        ret=sprd_audiotrack_test_stop();
    }
    return ret;
error:
    return -1;
}

static int sprd_audiorecord_test(void * adev,struct str_parms *parms,int opt, char * val){
    int channel=0;
    int devices=0;
    char value[128];
    int ret = -1;
    int val_int = 0;
    int data_size=0;
    LOG_D("sprd_audiorecord_test opt:%d",opt);

    switch(opt){
        case 1:{
            ret = str_parms_get_str(parms,"autotest_indevices", value, sizeof(value));
            if(ret >= 0){
                LOG_D("%s %d",__func__,__LINE__);
                devices = strtoul(value,NULL,0);
                LOG_D("%s %d %d",__func__,__LINE__,devices);
            }
            ret=sprd_audiorecord_test_start(devices);
        }
            break;
        case 2:
            ret = str_parms_get_str(parms,"autotest_datasize", value, sizeof(value));
            if(ret >= 0){
                LOG_D("%s %d",__func__,__LINE__);
                data_size = strtoul(value,NULL,0);
                LOG_D("%s %d %d",__func__,__LINE__,devices);
            }
            ret=sprd_audiorecord_test_read(data_size);
            break;
        case 3:
            ret=sprd_audiorecord_test_stop();
            break;
        default:
            break;
    }
    return ret;
error:
    return -1;
}

static int ext_handleFm(void * dev,struct str_parms *parms,int opt, char * val){
    int ret=-1;
    struct tiny_audio_device *adev=(struct tiny_audio_device *)dev;
    LOG_I("ext_handleFm:0x%x",opt);
    if (opt) {
        start_fm(adev->dev_ctl);
    } else {
        bool fm_record=false;
        pthread_mutex_lock(&adev->lock);
        fm_record=adev->fm_record;
        adev->fm_record=false;
        pthread_mutex_unlock(&adev->lock);
        if(true==fm_record){
            force_in_standby(adev,AUDIO_HW_APP_FM);
        }
        stop_fm(adev->dev_ctl);
    }
    return ret;
}

static int ext_set_fm_volume(void * dev,struct str_parms *parms,int volume, char * val){
    int ret=-1;
    struct tiny_audio_device *adev=(struct tiny_audio_device *)dev;
    LOG_D("ext_set_fm_volume:0x%x",volume);
    pthread_mutex_lock(&adev->lock);
    if(NULL!=adev->dev_ctl){
        set_fm_volume(adev->dev_ctl,volume);
    }
    pthread_mutex_unlock(&adev->lock);
    return ret;
}

static int ext_handle_fmMute(void * dev,struct str_parms *parms,int opt, char * val){
    int ret = -1;
    struct tiny_audio_device *adev=(struct tiny_audio_device *)dev;
    LOG_I("ext_handle_fmMute, mute:%d", opt);
    if(opt){
        ret = set_fm_mute(adev->dev_ctl, 1);
    } else {
        ret = set_fm_mute(adev->dev_ctl, 0);
    }

    return ret;
}

static int ext_handle_fmrecord(void * dev,struct str_parms *parms,int opt, char * val){
    struct tiny_audio_device *adev=(struct tiny_audio_device *)dev;
    LOG_D("ext_handle_fmrecord:0x%s usecase:0x%x",val,adev->dev_ctl->usecase);
    bool fm_record_start=false;

    if(opt==0){
        LOG_I("ext_handle_fmrecord fm_record stop")
        force_in_standby(adev,AUDIO_HW_APP_NORMAL_RECORD);
        force_in_standby(adev,AUDIO_HW_APP_FM);
        pthread_mutex_lock(&adev->lock);
        adev->fm_record = false;
        pthread_mutex_unlock(&adev->lock);
        return 0;
    }

    if(opt && (adev->dev_ctl->usecase & UC_FM)){
        if(false == adev->fm_record){
            fm_record_start=true;
        }
        adev->fm_record=true;
        pthread_mutex_unlock(&adev->lock);
    }

    if(true==fm_record_start){
        force_in_standby(adev,AUDIO_HW_APP_NORMAL_RECORD);
        force_in_standby(adev,AUDIO_HW_APP_FM);
    }
    return 0;
}

static int set_screen_state(void * dev,struct str_parms *parms,int state, char * val){
    int ret=-1;
    struct tiny_audio_device *adev=(struct tiny_audio_device *)dev;
    LOG_D("ext_screen_state:0x%x",state);
    pthread_mutex_lock(&adev->lock);
    if (strcmp(val, AUDIO_PARAMETER_VALUE_ON) == 0) {
        adev->low_power = false;
    } else {
        adev->low_power = true;
    }
    pthread_mutex_unlock(&adev->lock);
    return ret;
}

static int audio_voip_enable(void * dev,struct str_parms *parms,int opt, char * val){
    struct tiny_audio_device *adev=(struct tiny_audio_device *)dev;
    LOG_D("audio_voip_enable:0x%s",val);

    pthread_mutex_lock(&adev->lock);

    if(strcmp(val, "false") == 0){
        LOG_I("audio_voip_enable voip_stop")
        adev->voip_start = false;
    }

    if(strcmp(val, "true") == 0){
        LOG_I("audio_voip_enable voip_start")
        adev->voip_start=true;
    }

    pthread_mutex_unlock(&adev->lock);
    return 0;
}

static int ext_setStreamVolume(void * dev,struct str_parms *parms,int volume, char * val){
    int ret=-1;
    struct tiny_audio_device *adev=(struct tiny_audio_device *)dev;
    LOG_D("ext_setStreamVolume:0x%x",volume);
    int vol_index=0;
    audio_stream_type_t stream=AUDIO_STREAM_MUSIC;

    if(adev->stream_status & (1<<AUDIO_HW_APP_OFFLOAD)){
        stream=AUDIO_STREAM_MUSIC;
    }else if(adev->stream_status & (1<<AUDIO_HW_APP_CALL)){
        stream=AUDIO_STREAM_VOICE_CALL;
    }else{
        stream=AUDIO_STREAM_MUSIC;
        vol_index=0;
    }
    sprd_setStreamVolume(stream,volume);
    return ret;
}
/*
int set_net_mode(void * dev,struct str_parms *parms,int mode, char * val){
    int net_mode=0;
    int stream_status=0;
    bool mode_change=false;
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev ;
    struct dsp_control_t * dsp_ctl=adev->dev_ctl->agdsp_ctl;

    pthread_mutex_lock(&adev->dev_ctl->lock);
    net_mode = adev->dev_ctl->param_res.net_mode;
    if(net_mode!=mode){
        mode_change=true;
        adev->dev_ctl->param_res.net_mode = mode;
    }
    pthread_mutex_unlock(&adev->dev_ctl->lock);

    if(true==mode_change){
        pthread_mutex_lock(&adev->lock);
        stream_status =adev->stream_status;
        pthread_mutex_unlock(&adev->lock);

        if(stream_status & (1<<AUDIO_HW_APP_CALL)){
            send_cmd_to_dsp_thread(dsp_ctl,RIL_NET_MODE_CHANGE,NULL);
        }else{
            LOG_D("set_net_mode:not calling,stream_status:0x%x",stream_status);
        }
    }

    return 0;
}
*/
const struct ext_control_t  ext_contrl_table[] = {
    {"loglevel",                        audio_loglevel_ctrl},
    {"screen_state",                    set_screen_state},
    {"sprd_voip_start",                 audio_voip_enable},
    {"playbackdump",                    audio_playbackdump_ctrl},
    {"recorddump",                      audio_recorddump_ctrl},
    {"test_in_stream_route",            set_in_stream_route},
    {"test_out_stream_route",           set_out_stream_route},
    {"test_stream_route",               set_out_stream_route},
//  {"connect",                         connect_audio_devices},
    {"disconnect",                      disconnect_audio_devices},
    {"autotest_audiotracktest",         sprd_audiotrack_test},
    {"autotest_audiorecordtest",        sprd_audiorecord_test},
    {"autotest_audiolooptest",          sprd_audioloop_test},
    {"huaweilaohua_audio_loop_test",    sprd_audio_dev_laohua_test},
    {"autotest_audio_input_test",       audio_in_devices_test},
    {"out_devices_test",                audio_out_devices_test},
    {"bt_samplerate",                   set_bt_samplerate},
    {"bt_headset_nrec",                 set_bt_nrec},
    {"bt_wbs",                          set_bt_wbs},
    {"handleFm",                        ext_handleFm},
    {"fm_record",                       ext_handle_fmrecord},
    {"FM_Volume",                       ext_set_fm_volume},
    {"FM_mute",                         ext_handle_fmMute},
    {"dsp_loop",                        audio_dsp_loop},
    {"agdsp_reset",                     audio_agdsp_reset},
    {"agdsp_msg",                       agdsp_send_msg_test},
    {"agdsp_check",                     agdsp_check_status_test},
    {"agdsp_autoreset",                 agdsp_auto_reset},
    {"agdsp_logset",                    agdsp_log_set},
    {"agdsp_dumpset",                   agdsp_pcmdump_set},
    {"agdsp_forcenotify",               agdsp_force_assert_notify},
    {"agdsp_reboot",                    agdsp_reboot},
    {"setStreamVolume",                 ext_setStreamVolume},
//  {"ril_net",                         set_net_mode},
    {"ril_net_test",                    set_net_mode},
    {"set_mode",                        ext_setVoiceMode},
    {"audio_config",                    audio_config_ctrl},
    {"audio_wb",                        set_wb_mode},
    {"audio_net",                       set_net_mode},
    {"codec_mute",                      audio_codec_mute},
    {"AudioTester_enable",              audiotester_enable},
    {"vbc_pcm_dump",                    vbc_playback_dump},
};

int ext_contrtol_process(struct tiny_audio_device *adev,const char *cmd_string){
    int i=0;
    int ret=0;
    unsigned char value[AUDIO_EXT_CONTROL_PIPE_MAX_BUFFER_SIZE]={0};
    struct str_parms *parms=NULL;
    AUDIO_EXT_CONTROL_FUN fun=NULL;
    int size=sizeof(ext_contrl_table)/sizeof(struct ext_control_t);
    int val_int=-1;
    LOG_I("ext_contrtol_process:%s",cmd_string);
    parms = str_parms_create_str(cmd_string);
    for(i=0;i<size;i++){
        ret = str_parms_get_str(parms,ext_contrl_table[i].cmd_string,value,sizeof(value));
        if(ret>0){
            val_int =0;
            val_int = strtoul(value,NULL,0);
            fun=ext_contrl_table[i].fun;
            if(NULL!=fun){
                ret=fun(adev,parms,val_int,value);
            }
            memset(value,0x00,sizeof(value));
        }
    }

    str_parms_destroy(parms);
    return ret;
}

static int read_noblock_l(int fd,int8_t *buf,int bytes){
    int ret = 0;
    ret = read(fd,buf,bytes);
    return ret;
}

static void empty_command_pipe(int fd){
    char buff[16];
    int ret;
    do {
        ret = read(fd, &buff, sizeof(buff));
    } while (ret > 0 || (ret < 0 && errno == EINTR));
}

static void *control_audio_loop_process(void *arg){
    int pipe_fd,max_fd;
    fd_set fds_read;
    int result;
    void* data;
    struct str_parms *parms;
    int ret = 0;
    struct tiny_audio_device *adev = (struct tiny_audio_device *)arg;

    if(access(AUDIO_EXT_CONTROL_PIPE, R_OK) ==0){
        pipe_fd = open(AUDIO_EXT_CONTROL_PIPE, O_RDWR);
    }else{
        pipe_fd = open(AUDIO_EXT_DATA_CONTROL_PIPE, O_RDWR);
    }

    if(pipe_fd < 0){
        LOG_E("%s, open pipe error!! ",__func__);
        return NULL;
    }
    max_fd = pipe_fd + 1;
    if((fcntl(pipe_fd,F_SETFL,O_NONBLOCK)) <0){
        LOG_E("set flag RROR --------");
    }
    data = (char*)malloc(AUDIO_EXT_CONTROL_PIPE_MAX_BUFFER_SIZE);
    if(data == NULL){
        LOG_E("malloc data err");
        return NULL;
    }
    LOG_I("begin to receive audio control message");
    while(1){
        FD_ZERO(&fds_read);
        FD_SET(pipe_fd,&fds_read);
        result = select(max_fd,&fds_read,NULL,NULL,NULL);
        if(result < 0){
            LOG_E("select error ");
            continue;
        }
        if(FD_ISSET(pipe_fd,&fds_read) <= 0 ){
            LOG_E("SELECT OK BUT NO fd is set");
            continue;
        }
        memset(data,0,AUDIO_EXT_CONTROL_PIPE_MAX_BUFFER_SIZE);
        if(read_noblock_l(pipe_fd,data,1024) < 0){
            LOG_E("read data err");
            empty_command_pipe(pipe_fd);
        }else{
            ext_contrtol_process(adev,data);
        }
    }
    free(data);
    return NULL;
}

int ext_control_init(struct tiny_audio_device *adev){
    pthread_t control_audio_loop;

    if(access(AUDIO_EXT_CONTROL_PIPE, R_OK) != 0){
        if(access(AUDIO_EXT_DATA_CONTROL_PIPE, R_OK) != 0){
            if (mkfifo(AUDIO_EXT_DATA_CONTROL_PIPE,S_IFIFO|0666) <0) {
                if (errno != EEXIST) {
                    LOG_E("%s create audio fifo error %s\n",__FUNCTION__,strerror(errno));
                    return -1;
                }
            }

            if(chmod(AUDIO_EXT_DATA_CONTROL_PIPE, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH) != 0) {
                LOG_E("%s Cannot set RW to \"%s\": %s", __FUNCTION__,AUDIO_EXT_DATA_CONTROL_PIPE, strerror(errno));
            }
        }
    }

    if(pthread_create(&control_audio_loop, NULL, control_audio_loop_process, (void *)adev)) {
        LOG_E("control_audio_loop thread creating failed !!!!");
        return -2;
    }
    return 0;
}
