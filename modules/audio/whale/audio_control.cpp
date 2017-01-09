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

#define LOG_TAG "audio_hw_control"
#define LOG_NDEBUG 0

#include "audio_control.h"
#include "audio_hw.h"
#include <tinyxml.h>
#include <cutils/log.h>
#include "fcntl.h"
#include "audio_param/audio_param.h"
#include "tinyalsa_util.h"
#include "audio_parse.h"
#include "audio_param/audio_param.h"
#ifdef __cplusplus
extern "C" {
#endif
extern int set_dsp_volume(struct audio_control *ctl,int volume);
extern int disconnect_audiotester_process(struct socket_handle *tunning_handle);
static void do_select_device(struct audio_control *actl,AUDIO_HW_APP_T audio_app_type,audio_devices_t device,bool is_in,bool update_param);

static int do_switch_in_devices(struct audio_control *actl,AUDIO_HW_APP_T audio_app_type,audio_devices_t device);
static int set_vbc_bt_src_unlock(struct audio_control *actl, int rate);
static int select_audio_param_unlock(struct audio_control *dev_ctl,struct audio_param_res *param_res);
static uint8_t get_record_param(audio_devices_t in_devices);
static int is_voip_active(int usecase);
static int is_call_active(int usecase);
static int set_voice_mic(struct audio_control *actl,AUDIO_HW_APP_T audio_app_type,audio_devices_t device);
static int set_vbc_bt_src_without_lock(struct audio_control *actl, int rate);

#define SPRD_MAIN_MIC_PATH_SWITCH (1<<0)
#define SPRD_BACK_MIC_PATH_SWITCH (1<<1)
#define SPRD_HEADSET_MIC_PATH_SWITCH (1<<2)

static struct device_route *get_device_route_withname(struct device_route_handler
        *route_handler, const char *name)
{
    for (unsigned int i = 0; i < route_handler->size; i++) {
        if((route_handler->route+i) && route_handler->route[i].name && name){
            if (strcmp(route_handler->route[i].name, name) == 0) {
                return &route_handler->route[i];
            }
        }
    }
    return NULL;
}

static struct device_route *get_device_route_withdevice(struct device_route_handler  *route_handler, int devices){
    for (unsigned int i = 0; i < route_handler->size; i++) {
        if(route_handler->route+i){
            if( (route_handler->route[i].devices!=-1) &&
                        (devices ==  route_handler->route[i].devices)){
                return &route_handler->route[i];
            }
        }
    }
    return NULL;
}

static struct device_control *get_trans_control(struct device_route *route,
        const char *name)
{
    LOG_D("get_trans_control route devices name:%s   trans_size:%d  %s",
          route->name, route->trans_size, name);
    for (int i = 0; i < route->trans_size; i++) {
        LOG_D("get_trans_control name:%s", route->trans[i].name);
        if (strcmp(route->trans[i].name, name) == 0) {
            LOG_D("get_trans_control name:%s return", route->trans[i].name);
            return &route->trans[i].trans_ctl;
        }
    }
    return NULL;
}

static int apply_mixer_control(struct device_control *dev_ctl, const char *info)
{
    int ret=-1;
    LOG_D("apply_mixer_control %s %s ",dev_ctl->name,info);
    for (int i = 0; i < dev_ctl->ctl_size; i++) {
        struct mixer_control *mixer_ctl = &dev_ctl->ctl[i];
        if(mixer_ctl->strval!=NULL){
            ret = mixer_ctl_set_enum_by_string(mixer_ctl->ctl, mixer_ctl->strval);
            if (ret != 0) {
                LOG_E("Failed to set '%s' to '%s'\n",mixer_ctl->name, mixer_ctl->strval);
            } else {
                LOG_I("Set '%s' to '%s'\n",mixer_ctl->name, mixer_ctl->strval);
            }
        }else{
            if(1!=mixer_ctl->val_count){
                int val[2]={0};
                val[0]=mixer_ctl->value;
                val[1]=mixer_ctl->value;
                ret=mixer_ctl_set_array(mixer_ctl->ctl,val,2);
            }else{
            ret=mixer_ctl_set_value(mixer_ctl->ctl, 0, mixer_ctl->value);
            }
            if (ret != 0) {
                LOG_E("Failed to set '%s'to %d\n",
                        mixer_ctl->name, mixer_ctl->value);
            } else {
                LOG_I("Set '%s' to %d\n",
                        mixer_ctl->name, mixer_ctl->value);
            }
        }
    }
    return ret;
}

static int close_all_control_unlock(struct audio_control *actl)
{
    int ret=0;
    struct audio_route * route = (struct audio_route *)&actl->route;
    if(NULL !=route->pre_in_ctl){
        LOG_I("IN DEVICE :%s Route OFF",route->pre_in_ctl->name);
        ret=apply_mixer_control(&(route->pre_in_ctl->ctl_off), "Route OFF");
        route->pre_in_ctl=NULL;
    }

    if(NULL !=route->pre_out_ctl){
        LOG_I("OUT DEVICE :%s Route OFF",route->pre_out_ctl->name);
        ret=apply_mixer_control(&(route->pre_out_ctl->ctl_off), "Route OFF");
        route->pre_out_ctl=NULL;
    }
    return ret;
}

int close_all_control(struct audio_control *actl)
{
    int ret=0;
    pthread_mutex_lock(&actl->lock);
    ret=close_all_control_unlock(actl);
    pthread_mutex_unlock(&actl->lock);
    return ret;
}

int close_out_control(struct audio_control *actl)
{
    int ret=0;
    struct audio_route * route = (struct audio_route *)&actl->route;
    pthread_mutex_lock(&actl->lock);
    if(NULL !=route->pre_out_ctl){
        LOG_I("OUT DEVICE :%s Route OFF",route->pre_out_ctl->name);
        ret=apply_mixer_control(&(route->pre_out_ctl->ctl_off), "Route OFF");
        route->pre_out_ctl=NULL;
    }

    actl->out_devices=0;
    pthread_mutex_unlock(&actl->lock);
    return ret;
}

int close_in_control(struct audio_control *actl)
{
    int ret=0;
    struct audio_route * route = (struct audio_route *)&actl->route;
    LOG_D("close_in_control");
    pthread_mutex_lock(&actl->lock);
    if(NULL !=route->pre_in_ctl){
        LOG_I("IN DEVICE :%s Route OFF",route->pre_in_ctl->name);
        ret=apply_mixer_control(&(route->pre_in_ctl->ctl_off), "Route OFF");
        route->pre_in_ctl=NULL;
    }
    actl->in_devices=0;
    pthread_mutex_unlock(&actl->lock);
    return ret;
}

/**should add lock if call this function without any locks*/
static int _apply_mixer_control(struct device_control *dev_ctl,
    struct device_control *pre_dev_ctl)
{
    int i=0,j=0,ret=0;
    struct mixer_control *mixer_ctl=NULL;
    struct mixer_control *mixer_ctl_off=NULL;
    bool mixer_find=false;
    LOG_D("_apply_mixer_control ON:%s size:%d",dev_ctl->name,dev_ctl->ctl_size);
    for(i=0;i<dev_ctl->ctl_size;i++){
        mixer_ctl = &dev_ctl->ctl[i];
        mixer_find = false;
        for(j=0;j<pre_dev_ctl->ctl_size;j++){
            mixer_ctl_off = &pre_dev_ctl->ctl[j];
            if(mixer_ctl->ctl==mixer_ctl_off->ctl){
                mixer_find=true;
                LOG_D("same mixer:%s",mixer_ctl_off->name);
                break;
            }
        }

        if(true==mixer_find){
            continue;
        }

        if(mixer_ctl->strval!=NULL){
            ret = mixer_ctl_set_enum_by_string(mixer_ctl->ctl, mixer_ctl->strval);
            if (ret != 0) {
                LOG_E("Failed to set '%s' to '%s'\n",mixer_ctl->name, mixer_ctl->strval);
            } else {
                LOG_I("Set '%s' to '%s'\n",mixer_ctl->name, mixer_ctl->strval);
            }
        }else{
            if(1!=mixer_ctl->val_count){
                int val[2]={0};
                val[0]=mixer_ctl->value;
                val[1]=mixer_ctl->value;
                ret=mixer_ctl_set_array(mixer_ctl->ctl,val,2);
            }else{
                ret=mixer_ctl_set_value(mixer_ctl->ctl, 0, mixer_ctl->value);
            }
            if (ret != 0) {
                LOG_E("Failed to set '%s'to %d\n",
                        mixer_ctl->name, mixer_ctl->value);
            } else {
                LOG_I("Set '%s' to %d\n",
                        mixer_ctl->name, mixer_ctl->value);
            }
        }
    }

    LOG_D("_apply_mixer_control OFF:%s size:%d",pre_dev_ctl->name,pre_dev_ctl->ctl_size);
    for(i=0;i<pre_dev_ctl->ctl_size;i++){
        mixer_ctl_off = &pre_dev_ctl->ctl[i];
        mixer_find = false;
        for(j=0;j<dev_ctl->ctl_size;j++){
            mixer_ctl= &dev_ctl->ctl[j];
            if(mixer_ctl->ctl==mixer_ctl_off->ctl){
                LOG_D("same mixer:%s",mixer_ctl_off->name);
                mixer_find=true;
                break;
            }
        }

        if(true==mixer_find){
            continue;
        }

        if(mixer_ctl_off->strval!=NULL){
            ret = mixer_ctl_set_enum_by_string(mixer_ctl_off->ctl, mixer_ctl_off->strval);
            if (ret != 0) {
                LOG_E("Failed to set '%s' to '%s'\n",mixer_ctl_off->name, mixer_ctl_off->strval);
            } else {
                LOG_I("Set '%s' to '%s'\n",mixer_ctl_off->name, mixer_ctl_off->strval);
            }
        }else{
            if (1!=mixer_ctl_off->val_count) {
                int val[2]={0};
                val[0]=mixer_ctl_off->value;
                val[1]=mixer_ctl_off->value;
                ret=mixer_ctl_set_array(mixer_ctl_off->ctl,val,2);
            } else {
                ret=mixer_ctl_set_value(mixer_ctl_off->ctl, 0, mixer_ctl_off->value);
            }
            if (ret != 0) {
                LOG_E("Failed to set '%s'to %d\n",
                        mixer_ctl_off->name, mixer_ctl_off->value);
            } else {
                LOG_I("Set '%s' to %d\n",
                        mixer_ctl_off->name, mixer_ctl_off->value);
            }
        }
    }

    return ret;
}

static int switch_device_route(struct audio_control *ctl, int device,
                               bool in_device)
{
    LOG_D("switch_device_route device=0x%x,in_device:%d", device,in_device);
    const char *cur_dev_name;
    struct device_route *cur = NULL;

    cur = get_device_route_withdevice(&(ctl->route.devices_route), device);

    if(NULL == cur){
        LOG_I("switch_device_route not fined the devices: 0x%x",device);
        return -1;
    }

    if(in_device) {
        if(cur == ctl->route.pre_in_ctl){
            LOG_D("switch_device_route set the same devices");
            return 0;
        }

        if(NULL !=ctl->route.pre_in_ctl){
            LOG_I("IN DEVICES %s Route OFF %s Route ON",ctl->route.pre_in_ctl->name,cur->name);
            _apply_mixer_control(&cur->ctl_on,&(ctl->route.pre_in_ctl->ctl_off));
        }else{
            LOG_I("IN DEVICES %s Route ON",cur->name);
            apply_mixer_control(&cur->ctl_on, "Route ON");
        }

        ctl->route.pre_in_ctl=cur;
    }
    else {
        if(cur == ctl->route.pre_out_ctl){
            LOG_I("switch_device_route set the same devices");
            return 0;
        }

        if(NULL !=ctl->route.pre_out_ctl){
            LOG_I("OUT DEVICES %s Route OFF %s Route ON",ctl->route.pre_out_ctl->name,cur->name);
            _apply_mixer_control(&cur->ctl_on,&(ctl->route.pre_out_ctl->ctl_off));
        }else{
            LOG_I("OUT DEVICES %s Route ON",cur->name);
            apply_mixer_control(&cur->ctl_on, "Route ON");
        }

        ctl->route.pre_out_ctl=cur;
    }
    return 0;
}

static int apply_gain_control(struct gain_mixer_control *gain_ctl, int volume)
{
    int ctl_volume = 0;
    int volume_array[2]={0};
    if (gain_ctl->volume_size == 1 || volume == -1) {
        ctl_volume = gain_ctl->volume_value[0];
    } else if (volume < gain_ctl->volume_size) {
        ctl_volume = gain_ctl->volume_value[volume];
    } else {
        LOG_W("volume is too big ctl_volume:%d max:%d",volume,gain_ctl->volume_size);
        ctl_volume = gain_ctl->volume_value[gain_ctl->volume_size - 1];
    }
    volume_array[0]=ctl_volume;
    volume_array[1]=ctl_volume;
    mixer_ctl_set_array(gain_ctl->ctl, volume_array, 2);
    LOG_I("Apply Gain Control [%s. %d]", gain_ctl->name, ctl_volume);
    return 0;
}

static int _set_mdg_mute(struct mixer_ctrl_t *mute_ctl,bool mute){
    int ret=-1;
    int vol_array[2]={0};

    if(mute==mute_ctl->value){
        LOG_W("_set_mdg_mute:set the same value:%d",mute_ctl->value);
        return 0;
    }

    if(true==mute){
        vol_array[0]=1;
    }else{
        vol_array[0]=0;
    }
    vol_array[1]=1024;

    if(NULL!=mute_ctl){
        ret=mixer_ctl_set_array(mute_ctl->mixer, vol_array, 2);
        if (ret != 0) {
            LOG_E("_set_mdg_mute Failed :%d\n",mute);
        }else{
            LOG_I("_set_mdg_mute:%p %d",mute_ctl,mute);
            mute_ctl->value=mute;
        }
    }else{
        LOG_W("_set_mdg_mute mute_ctl is null");
    }
    return ret;
}


int set_mdg_mute(struct audio_control *actl,int usecase,bool on){
    int ret=-1;
    struct mute_control *mute=&actl->mute;
    LOG_D("set_mdg_mute:%d pre_mute:0x%x %p usecase:%d",on,mute->mute_status,mute,usecase);

    pthread_mutex_lock(&actl->lock);
    if(UC_UNKNOWN==actl->usecase){
        LOG_W("set_mdg_mute failed");
        goto exit;
    }

    switch(usecase){
        case UC_DEEP_BUFFER_PLAYBACK:
            ret = _set_mdg_mute(&mute->audio_mdg_mute,on);
            break;

        case UC_OFFLOAD_PLAYBACK:
            if(false==on){
                ret = _set_mdg_mute(&mute->dsp_da0_mdg_mute,on);
            }else{
                if(false==is_usecase_unlock(actl, UC_NORMAL_PLAYBACK)){
                    ret = _set_mdg_mute(&mute->dsp_da0_mdg_mute,on);
                }
            }
            break;

        case UC_NORMAL_PLAYBACK:
            if(false==on){
                ret = _set_mdg_mute(&mute->dsp_da0_mdg_mute,on);
            }else{
                if(false==is_usecase_unlock(actl, UC_OFFLOAD_PLAYBACK)){
                    ret = _set_mdg_mute(&mute->dsp_da0_mdg_mute,on);
                }
            }
            break;

        case UC_LOOP:
        case UC_VOIP:
        case UC_BT_VOIP:
        case UC_CALL:
            ret = _set_mdg_mute(&mute->dsp_da1_mdg_mute,on);
            break;

        default:
            break;
    }

exit:
    pthread_mutex_unlock(&actl->lock);
    return ret;
}

int set_vdg_gain(struct device_usecase_gain *dg_gain, int param_id, int volume){
    struct device_gain * gain=NULL;

    LOG_I("UPDATE_PARAM_VDG:%s volme:%d",get_audio_param_name(param_id),volume);

    LOG_D("set_vdg_gain size:%d param_id:%d volume:%d",dg_gain->gain_size,param_id,volume);
    for(int i=0;i<dg_gain->gain_size;i++){
        LOG_D("set_vdg_gain:%s",dg_gain->dev_gain[i].name);
        gain=&(dg_gain->dev_gain[i]);
        if(gain->id==param_id){
            LOG_D("set_vdg_gain profile:%d",param_id);
            for(int j=0;j<gain->ctl_size;j++){
                apply_gain_control(&(gain->ctl[j]),volume);
            }
            return 0;
        }
    }
    LOG_E("set_vdg_gain Didn't found the Device Gain for profile_id=%d", param_id);
    return -1;
}

static int do_switch_out_devices(struct audio_control *actl,AUDIO_HW_APP_T audio_app_type,audio_devices_t device)
{
    int in_devices=0;
    int ret=0;

    ret=switch_device_route(actl,device,false);
    if(ret<0){
        if(device&AUDIO_DEVICE_OUT_WIRED_HEADSET){
            device=AUDIO_DEVICE_OUT_WIRED_HEADSET;
        }else if(device&AUDIO_DEVICE_OUT_WIRED_HEADPHONE){
            device=AUDIO_DEVICE_OUT_WIRED_HEADPHONE;
        }else if(device&AUDIO_DEVICE_OUT_EARPIECE){
            device=AUDIO_DEVICE_OUT_EARPIECE;
        }else{
            device=actl->out_devices;
        }
        LOG_I("do_switch_out_devices 0x%x failed, try again switch to 0x%x",device, actl->out_devices);
        switch_device_route(actl,device,false);
    }

    if(AUDIO_DEVICE_OUT_ALL_SCO&device){
        if(NULL !=actl->route.pre_in_ctl){
            apply_mixer_control(&(actl->route.pre_in_ctl->ctl_off), "Route OFF");
            actl->route.pre_in_ctl =NULL;
        }
        //enable bt src after turn on bt_sco
        set_vbc_bt_src_unlock(actl,actl->param_res.bt_infor.samplerate);
    }

    return ret;
}

static int do_switch_in_devices(struct audio_control *actl,AUDIO_HW_APP_T audio_app_type,audio_devices_t device)
{
    // open fm record route
    if(audio_app_type==AUDIO_HW_APP_FM_RECORD){
        device=AUDIO_DEVICE_IN_FM_TUNER;
        LOG_I("do_switch_in_devices in_devices:%x",device);
    }
    return switch_device_route(actl,device,true);
}
static void *stream_routing_thread_entry(void *param)
{
    struct audio_control *actl = (struct audio_control *)param;
    pthread_attr_t attr;
    struct sched_param m_param;
    int newprio = 39;
    struct listnode *item;

    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    pthread_attr_getschedparam(&attr, &m_param);
    m_param.sched_priority = newprio;
    pthread_attr_setschedparam(&attr, &m_param);

   while(!actl->routing_mgr.is_exit)
    {
        struct switch_device_cmd *cmd = NULL;

        pthread_mutex_lock(&actl->cmd_lock);
         if (list_empty(&actl->switch_device_cmd_list)) {
            LOG_D(" switch_device_cmd_list is empty,wait");
            pthread_mutex_unlock(&actl->cmd_lock);
            sem_wait(&actl->routing_mgr.device_switch_sem);
            continue;
        }

        item = list_head(&actl->switch_device_cmd_list);
        cmd = node_to_item(item, struct switch_device_cmd, node);
        list_remove(item);
        pthread_mutex_unlock(&actl->cmd_lock);
        LOG_D("stream_routing_thread_entry process cmd in ");

         switch(cmd->cmd) {
         case SWITCH_DEVICE_ASYNC:
         case SWITCH_DEVICE_SYNC:
            do_select_device(actl,cmd->audio_app_type,cmd->device,cmd->is_in,cmd->update_param);
            break;
         default:
            break;
         }
        if (cmd->is_sync)
            sem_post(&cmd->sync_sem);
        else
            free(cmd);
         LOG_D("stream_routing_thread_entry process cmd out ");
    }

     while (!list_empty(&actl->switch_device_cmd_list)) {
        struct switch_device_cmd *cmd = NULL;

        pthread_mutex_lock(&actl->cmd_lock);
        item = list_head(&actl->switch_device_cmd_list);
        cmd = node_to_item(item, struct switch_device_cmd, node);
        list_remove(item);
        pthread_mutex_unlock(&actl->cmd_lock);
        if (cmd->is_sync)
            sem_post(&cmd->sync_sem);
        else
            free(cmd);
    }

    return NULL;
}

/*
    int get_private_control(struct audio_control *ctl, const char *priv_name)
    {
    for (int i = 0; i < private->size; i++) {
        if (strcmp(private->priv[i].name, priv_name) == 0) {
            return mixer_ctl_get_value(private->priv[i].ctl->ctl, 0);
        }
    }
    LOG_E("Can not find the Private control :%s", priv_name);
    return -1;

    }
*/


int apply_private_control(struct private_control *pri, const char *priv_name)
{
    LOG_I("apply_private_control [%s]", priv_name);
    for (int i = 0; i < pri->size; i++) {
        if (strcmp(pri->priv[i].name, priv_name) == 0) {
            apply_mixer_control(&(pri->priv[i]),priv_name);
            return 0;
        }
    }
    LOG_E("Can not find the Private control :%s", priv_name);
    return -1;
}

int apply_dsploop_control(struct private_dsploop_control *dsploop_ctl, int type)
{
    LOG_I("apply_dsploop_control type:%d", type);
    struct device_control dev_ctl;
    for (int i = 0; i < dsploop_ctl->size; i++) {
        if(type==dsploop_ctl->dsp_ctl[i].type){
            dev_ctl.ctl_size=dsploop_ctl->dsp_ctl[i].ctl_size;
            dev_ctl.ctl=dsploop_ctl->dsp_ctl[i].ctl;
            dev_ctl.name=NULL;
            apply_mixer_control(&dev_ctl,"dsploop");
            return 0;
        }
    }
    LOG_E("Can not find the dsploop control type:%x", type);
    return -1;
}

int set_vbc_dump_control(struct audio_control *ctl, const char * dump_name, bool on){
    struct device_route *cur = NULL;
    int ret=0;
    cur = get_device_route_withname(&(ctl->route.vbc_pcm_dump), dump_name);

    if(NULL == cur){
        LOG_I("set_vbc_dump_control not fined iis ctl :%s",dump_name);
        ret=-1;
        goto out;
    }

    if(true==on){
        apply_mixer_control(&cur->ctl_on, "vbc_iis on");
    }else{
        apply_mixer_control(&cur->ctl_off, "vbc_iis off");
    }

out:
    return ret;
}

int switch_vbc_iis_route(struct audio_control *ctl,USECASE uc,bool on){
    struct device_route *cur = NULL;
    int ret=0;
    switch(uc){
        case UC_CALL:
            cur = get_device_route_withname(&(ctl->route.vbc_iis), "voice_playback");
            break;
        case UC_VOIP:
            cur = get_device_route_withname(&(ctl->route.vbc_iis), "voip");
            break;
        case UC_BT_VOIP:
            LOG_D("do nothing for BT_VOIP");
            break;
        case UC_FM:
            cur = get_device_route_withname(&(ctl->route.vbc_iis), "fm");
            break;
        case UC_DEEP_BUFFER_PLAYBACK:
        case UC_OFFLOAD_PLAYBACK:
        case UC_NORMAL_PLAYBACK:
            cur = get_device_route_withname(&(ctl->route.vbc_iis), "playback");
            break;
        case UC_MM_RECORD:
            cur = get_device_route_withname(&(ctl->route.vbc_iis), "record");
            break;
        case UC_BT_RECORD:
            cur = get_device_route_withname(&(ctl->route.vbc_iis), "bt_record");
            break;
        case UC_LOOP:
            cur = get_device_route_withname(&(ctl->route.vbc_iis), "dsploop");
            break;
        default:
            break;
    }

    if(NULL == cur){
        LOG_I("switch_vbc_iis_route not fined iis ctl usecase:0x%x:",uc);
        ret=-1;
        goto out;
    }

    if(true==on){
        apply_mixer_control(&cur->ctl_on, "vbc_iis on");
    }else{
        apply_mixer_control(&cur->ctl_off, "vbc_iis off");
    }

out:
    return 0;
}

static int vbc_iis_loop_enable(struct audio_control *ctl,bool on){
    int ret=0;
    if(AUD_REALTEK_CODEC_TYPE == ctl->codec_type){
        LOG_I("do nothing vbc_iis_loop_enable\n");
        return 0;
    }
//    pthread_mutex_lock(&ctl->lock);
    if(NULL == ctl->vbc_iis_loop){
        ctl->vbc_iis_loop= mixer_get_ctl_by_name(ctl->mixer, "VBC IIS Master Setting");
    }

    if(NULL == ctl->vbc_iis_loop){
        LOG_E("vbc_iis_loop_enable:%d Failed,vbc_iis_loop is null\n",on);
        ret=-1;
        goto out;
    }

    if(true==on){
        ret = mixer_ctl_set_enum_by_string(ctl->vbc_iis_loop,"loop");
    }else{
        ret = mixer_ctl_set_enum_by_string(ctl->vbc_iis_loop,"disable_loop");
    }

    if (ret != 0) {
        LOG_E("vbc_iis_loop_enable:%d Failed \n",on);
        ret=-1;
    }else{
        LOG_I("vbc_iis_loop_enable:%d",on);
    }
out:
//    pthread_mutex_unlock(&ctl->lock);
    return ret;
}

static int vbc_iis_digital_top_enable(struct audio_control *ctl,bool on){
    int ret=0;

    if(AUD_REALTEK_CODEC_TYPE == ctl->codec_type){

        if(NULL == ctl->vbc_iis_loop){
            ctl->vbc_iis_loop= mixer_get_ctl_by_name(ctl->mixer, "VBC IIS Master Setting");
        }

        if(NULL == ctl->vbc_iis_loop){
            LOG_E("vbc_iis_digital_top_enable:%d Failed,vbc_iis_loop is null\n",on);
            ret=-1;
            goto out;
        }

        if(true==on){
            ret = mixer_ctl_set_enum_by_string(ctl->vbc_iis_loop,"iis0");
        }else{
            ret = mixer_ctl_set_enum_by_string(ctl->vbc_iis_loop,"disable_iis0");
        }

        if (ret != 0) {
            LOG_E("vbc_iis_digital_top_enable:%d Failed \n",on);
            ret=-1;
        }else{
            LOG_D("vbc_iis_digital_top_enable:%d",on);
        }
        return 0;
    }

out:
    return ret;
}

int set_fm_volume(struct audio_control *ctl, int volume)
{
    struct device_gain * gain=NULL;
    struct device_usecase_gain *dg_gain=NULL;
    int profile_id=0;
    int ret = -1;
    pthread_mutex_lock(&ctl->lock);
    dg_gain=&(ctl->adev->dev_ctl->dg_gain);
    ctl->fm_volume=volume;
    if(ctl->fm_mute) {
        LOG_I("set_fm_volume ,fm_mute:%d", ctl->fm_mute);
        pthread_mutex_unlock(&ctl->lock);
        return 0;
    }

    if(false==is_usecase_unlock(ctl,UC_FM)){
        LOG_I("set_fm_volume line:%d",__LINE__);
        pthread_mutex_unlock(&ctl->lock);
        return 0;
    }
    ret = set_audioparam_unlock(ctl,PARAM_FM_VOLUME_CHANGE, &volume,false);
    pthread_mutex_unlock(&ctl->lock);

    return ret;
}

int set_fm_mute(struct audio_control *dev_ctl, bool mute)
{
    int ret = -1;
    pthread_mutex_lock(&dev_ctl->lock);
    LOG_I("%s, mute:%d", __func__, mute);

    if(mute) {
        int volume = 0;
        dev_ctl->fm_mute = true;
        if(false==is_usecase_unlock(dev_ctl,UC_FM)){
            pthread_mutex_unlock(&dev_ctl->lock);
            return 0;
        }

        ret = set_audioparam_unlock(dev_ctl,PARAM_FM_VOLUME_CHANGE, &volume, false);
    } else {
        dev_ctl->fm_mute = false;
        if(false==is_usecase_unlock(dev_ctl,UC_FM)){
            pthread_mutex_unlock(&dev_ctl->lock);
            return 0;
        }

        ret = set_audioparam_unlock(dev_ctl,PARAM_FM_VOLUME_CHANGE, &dev_ctl->fm_volume, false);
    }
    pthread_mutex_unlock(&dev_ctl->lock);
    return ret;
}

bool is_usecase_unlock(struct audio_control *actl, int usecase)
{
    bool on=false;
    if (actl->usecase & usecase) {
        on=true;
    } else {
        on=false;
    }
    return on;
}

int dev_ctl_iis_set(struct audio_control *ctl, int usecase,int on){
    struct audio_control *dev_ctl = ctl;

    if(on) {
        switch(usecase) {
            case UC_CALL:
                if(false==is_usecase_unlock(dev_ctl,UC_CALL)) {
                    vbc_iis_loop_enable(dev_ctl,false);
                    if((true==is_usecase_unlock(dev_ctl,UC_BT_RECORD))
                        ||(ctl->out_devices&AUDIO_DEVICE_OUT_ALL_SCO)){
                        LOG_I("VBC need switch to bt");
                    }else{
                        switch_vbc_iis_route(dev_ctl,UC_CALL,true);
                    }
                }
            break;
            case UC_VOIP_RECORD:
                    vbc_iis_loop_enable(dev_ctl,false);
                    break;
            case UC_VOIP:
                if((false==is_usecase_unlock(dev_ctl,UC_CALL))
                    &&(false==is_voip_active(dev_ctl->usecase))){
                    switch_vbc_iis_route(dev_ctl,UC_VOIP,true);
                    if(true==is_usecase_unlock(dev_ctl,UC_MM_RECORD|UC_VOIP_RECORD)){
                        vbc_iis_loop_enable(dev_ctl,false);
                    }else{
                        vbc_iis_loop_enable(dev_ctl,true);
                        LOG_I("dev_ctl_iis_set voip_start  uc:%x",dev_ctl->usecase);
                    }
                }
            break;
            case UC_BT_VOIP:
                if((false==is_usecase_unlock(dev_ctl,UC_CALL))
                    &&(false==is_voip_active(dev_ctl->usecase))){
                    switch_vbc_iis_route(dev_ctl,UC_BT_VOIP,true);
                    if(true==is_usecase_unlock(dev_ctl,UC_MM_RECORD|UC_VOIP_RECORD)){
                        vbc_iis_loop_enable(dev_ctl,false);
                    }else{
                        vbc_iis_loop_enable(dev_ctl,true);
                        LOG_I("dev_ctl_iis_set voip_start  uc:%x",dev_ctl->usecase);
                    }
                }

            break;
            case UC_DEEP_BUFFER_PLAYBACK:
                if((false==is_usecase_unlock(dev_ctl,UC_CALL))
                    &&(false==is_voip_active(dev_ctl->usecase))) {
                        switch_vbc_iis_route(dev_ctl,UC_DEEP_BUFFER_PLAYBACK,true);
                }
                break;
            case UC_NORMAL_PLAYBACK:
                if((false==is_usecase_unlock(dev_ctl,UC_CALL))
                    &&(false==is_voip_active(dev_ctl->usecase))) {
                        switch_vbc_iis_route(dev_ctl,UC_NORMAL_PLAYBACK,true);
                }
                break;
            case UC_OFFLOAD_PLAYBACK:
                if((false==is_usecase_unlock(dev_ctl,UC_CALL))
                    &&(false==is_voip_active(dev_ctl->usecase))) {
                        switch_vbc_iis_route(dev_ctl,UC_OFFLOAD_PLAYBACK,true);
                }
            break;
            case UC_MM_RECORD:
                if((false==is_voip_active(dev_ctl->usecase))
                    &&(false==is_usecase_unlock(dev_ctl,UC_FM_RECORD))
                    &&(false==is_usecase_unlock(dev_ctl,UC_MM_RECORD))) {
                        vbc_iis_loop_enable(dev_ctl,false);
                        switch_vbc_iis_route(dev_ctl,UC_MM_RECORD,true);
                }
            break;
            case UC_BT_RECORD:
                    vbc_iis_loop_enable(dev_ctl,false);
                    switch_vbc_iis_route(dev_ctl,UC_BT_RECORD,true);
                    set_vbc_bt_src_unlock(dev_ctl,dev_ctl->param_res.bt_infor.samplerate);
            break;
            case UC_FM:
                if((false==is_usecase_unlock(dev_ctl,UC_CALL))
                    &&(false==is_voip_active(dev_ctl->usecase))
                    &&(false==is_usecase_unlock(dev_ctl,UC_FM))) {
                        switch_vbc_iis_route(dev_ctl,UC_FM,true);
                }
            break;
            case UC_LOOP:
                if((false==is_usecase_unlock(dev_ctl,UC_CALL))
                    &&(false==is_voip_active(dev_ctl->usecase))
                    &&(false==is_usecase_unlock(dev_ctl,UC_LOOP))) {
                        switch_vbc_iis_route(dev_ctl,UC_LOOP,true);
                }
            break;
            case UC_FM_RECORD:
                if((false==is_usecase_unlock(dev_ctl,UC_CALL))
                    &&(false==is_voip_active(dev_ctl->usecase))
                    &&(false==is_usecase_unlock(dev_ctl,UC_FM))) {
                        switch_vbc_iis_route(dev_ctl,UC_FM,true);
                }
            break;
            default:
                LOG_E("dev_ctl_iis_set, error usecase %d",usecase);

        }
    }else{
        if((usecase == UC_VOIP_RECORD) && (is_voip_active(dev_ctl->usecase))){
            vbc_iis_loop_enable(dev_ctl,true);
        }

        if((usecase == UC_BT_RECORD)
            && (0==(dev_ctl->out_devices&AUDIO_DEVICE_OUT_ALL_SCO))){

            /* disable src before disable bt record*/
            LOG_I("trun of bt record devices");
            set_vbc_bt_src_unlock(dev_ctl,48000);
        }
    }

    return 0;
}

static int clear_playback_param_state(struct audio_param_res *param_res){
    param_res->cur_playback_dg_id=0xff;
    param_res->cur_vbc_playback_id=0xff;
    LOG_I("clear_playback_param_state");
    return 0;
}

static int clear_fm_param_state(struct audio_param_res *param_res){
    param_res->cur_fm_dg_id=0xff;
    param_res->cur_fm_dg_volume=0xff;

    param_res->cur_vbc_id=0xff;
    LOG_I("clear_fm_param_state");
    return 0;
}

static int clear_voice_param_state(struct audio_param_res *param_res){
    param_res->cur_voice_dg_id=0xff;
    param_res->cur_voice_dg_volume=0xff;

    param_res->cur_dsp_id=0xff;
    param_res->cur_dsp_volume=0xff;

    param_res->cur_vbc_id=0xff;
    LOG_I("clear_voice_param_state");
    return 0;
}

static int clear_record_param_state(struct audio_param_res *param_res){
    param_res->cur_record_dg_id=0xff;
    param_res->cur_vbc_id=0xff;
    LOG_I("clear_record_param_state");
    return 0;
}

static int clear_audio_param_state(struct audio_param_res  *param_res)
{
    param_res->usecase =0;
    clear_playback_param_state(param_res);
    clear_fm_param_state(param_res);
    clear_voice_param_state(param_res);
    clear_record_param_state(param_res);

    param_res->cur_codec_p_id = 0xff ;
    param_res->cur_codec_p_volume = 0xff ;
    param_res->cur_codec_c_id = 0xff ;

    return 0;
}


int set_usecase(struct audio_control *actl, int usecase, bool on)
{
    int ret = 0;
    LOG_I("set_usecase cur :0x%x usecase=0x%x  %s",actl->usecase, usecase, on ? "on" : "off");

    pthread_mutex_lock(&actl->lock);
    if(on) {
        ret = dsp_sleep_ctrl(actl->agdsp_ctl,true);
        if (0 != ret) {
            LOG_E("dsp_sleep_ctrl true failed %s", __func__);
            goto exit;
    }
        if(actl->usecase  & usecase) {
            goto exit;
        }
    }
    else {
        if(!(actl->usecase  & usecase)) {
            goto exit;
        }
    }

    if(actl->usecase  & UC_CALL){
        if((usecase==UC_BT_RECORD)&&(0==(actl->out_devices&AUDIO_DEVICE_OUT_ALL_SCO))){
            LOG_I("set_usecase UC_BT_RECORD Failed");
            goto exit;
        } else if((usecase==UC_MM_RECORD)&&(actl->out_devices&AUDIO_DEVICE_OUT_ALL_SCO)){
            LOG_I("set_usecase UC_MM_RECORD Failed");
            goto exit;
        }
    }

    ret = dev_ctl_iis_set(actl, usecase, on);
    if (on) {
        actl->usecase |= usecase;
    } else {
        actl->usecase &= ~usecase;
        if((0==(actl->usecase & (UC_CALL|UC_VOIP|UC_LOOP|UC_BT_VOIP|UC_BT_RECORD)))&&
            (0==(actl->out_devices&AUDIO_DEVICE_OUT_ALL_SCO))){
            vbc_iis_digital_top_enable(actl,false);
        }

        if(0==(actl->usecase & (UC_CALL|UC_VOIP|UC_BT_VOIP))){
            vbc_iis_loop_enable(actl,false);
        }
        LOG_D("set_usecase usecase:%x",actl->usecase);
    }

    if(UC_UNKNOWN==actl->usecase){
        vbc_iis_digital_top_enable(actl,false);
        close_all_control_unlock(actl);
        ret = dsp_sleep_ctrl(actl->agdsp_ctl,false);
        if (0 != ret) {
            LOG_E("%s dsp_sleep_ctrl false failed", __func__);
            goto exit;
        }
    } else if (UC_FM == actl->usecase) {
        struct dsp_smsg msg;
        msg.channel=11;
        msg.command=11;
        msg.parameter0=0;
        msg.parameter1=0;
        msg.parameter2=0;
        agdsp_send_msg(actl->agdsp_ctl,&msg);
     }
    pthread_mutex_unlock(&actl->lock);

    return ret;

exit:
    pthread_mutex_unlock(&actl->lock);
    return -1;
}

bool is_usecase(struct audio_control *actl, int usecase)
{
    bool on=false;
    pthread_mutex_lock(&actl->lock);
    on = is_usecase_unlock(actl,usecase);
    pthread_mutex_unlock(&actl->lock);
    return on;
}

static int set_vbc_bt_src_unlock(struct audio_control *actl, int rate){
    int ret=0;
    if(NULL==actl->bt_src){
        LOG_E("set_vbc_bt_src_unlock failed,can not get mixer:VBC_BT_SRC");
    }else{
        LOG_I("set_vbc_bt_src_unlock:%d",rate);
        ret=mixer_ctl_set_value(actl->bt_src, 0, rate);
    }
    return ret;
}

static int set_vbc_bt_src_without_lock(struct audio_control *actl, int rate){
    int ret=-ENOSYS;
    actl->param_res.bt_infor.samplerate=rate;

    if(AUDIO_DEVICE_OUT_ALL_SCO&actl->out_devices){
        ret=set_vbc_bt_src_unlock(actl,actl->param_res.bt_infor.samplerate);
    }else if((AUDIO_DEVICE_IN_ALL_SCO&((~AUDIO_DEVICE_BIT_IN)&actl->in_devices))
        &&(is_usecase_unlock(actl, UC_BT_RECORD))){
        ret=set_vbc_bt_src_unlock(actl,actl->param_res.bt_infor.samplerate);
    }
    return ret;
}

int set_vbc_bt_src(struct audio_control *actl, int rate){
    int ret=-ENOSYS;
    pthread_mutex_lock(&actl->lock);
    ret=set_vbc_bt_src_without_lock(actl,rate);
    pthread_mutex_unlock(&actl->lock);
    return ret;
}

int set_vbc_bt_nrec(struct audio_control *actl, bool nrec){
    struct dev_bluetooth_t bt_infor;
    bt_infor.bluetooth_nrec=nrec;
    return set_audioparam(actl,PARAM_BT_NREC_CHANGE,&bt_infor,false);
}

static int _set_codec_mute(struct mixer_ctrl_t *mute_ctl,bool mute){
    int ret=-1;

    if(NULL==mute_ctl){
        return -1;
    }

    if(mute==mute_ctl->value){
        LOG_D("_set_codec_mute:the same value");
        return 0;
    }

    if(NULL!=mute_ctl->mixer){
        ret=mixer_ctl_set_value(mute_ctl->mixer, 0, mute);
        if (ret != 0) {
            LOG_E("_set_codec_mute Failed :%d\n",mute);
        }
        mute_ctl->value=mute;
    }else{
        LOG_D("_set_codec_mute mute_ctl is null");
    }
    return ret;
}

int set_mic_mute(struct audio_control *actl, bool on){
    struct mute_control *mute=&actl->mute;
    LOG_D("set_mic_mute:%d pre_mute:0x%x %p",on,mute->mute_status,mute);
    return 0;
}

int set_codec_mute(struct audio_control *actl,bool on){
    struct mute_control *mute=&actl->mute;
    LOG_I("set_codec_mute:%d pre_mute:0x%x %p",on,mute->mute_status,mute);

    pthread_mutex_lock(&actl->lock);
    if(UC_UNKNOWN==actl->usecase){
        LOG_W("set_codec_mute failed");
        goto exit;
    }

    _set_codec_mute(&mute->spk_mute,on);
    _set_codec_mute(&mute->spk2_mute,on);
    _set_codec_mute(&mute->handset_mute,on);
    _set_codec_mute(&mute->headset_mute,on);

exit:
    pthread_mutex_unlock(&actl->lock);
    return 0;
}

static int init_codec_mute(struct mixer *mixer,struct mute_control *mute,struct mute_control_name *mute_name){

    mute->mute_status = 0;

    if(mixer==NULL){
        LOG_E("init_codec_mute failed");
        return -1;
    }

    if((NULL==mute->spk_mute.mixer) && (NULL!= mute_name->spk_mute)){
        mute->spk_mute.mixer=mixer_get_ctl_by_name(mixer,mute_name->spk_mute);
        free(mute_name->spk_mute);
        mute_name->spk_mute=NULL;
    }
    mute->spk_mute.value=-1;

    if((NULL==mute->spk2_mute.mixer) && (NULL!= mute_name->spk2_mute)){
        mute->spk2_mute.mixer=mixer_get_ctl_by_name(mixer,mute_name->spk2_mute);
        free(mute_name->spk2_mute);
        mute_name->spk2_mute=NULL;
    }
    mute->spk2_mute.value=-1;

    if((NULL==mute->handset_mute.mixer) && (NULL!= mute_name->handset_mute)){
        mute->handset_mute.mixer=mixer_get_ctl_by_name(mixer,mute_name->handset_mute);
        free(mute_name->handset_mute);
        mute_name->handset_mute=NULL;
    }
    mute->handset_mute.value=-1;

    if((NULL==mute->headset_mute.mixer) && (NULL!= mute_name->headset_mute)){
        mute->headset_mute.mixer=mixer_get_ctl_by_name(mixer,mute_name->headset_mute);
        free(mute_name->headset_mute);
        mute_name->headset_mute=NULL;
    }
    mute->headset_mute.value=-1;

    if((NULL==mute->linein_mute.mixer) && (NULL!= mute_name->linein_mute)){
        mute->linein_mute.mixer=mixer_get_ctl_by_name(mixer,mute_name->linein_mute);
        free(mute_name->linein_mute);
        mute_name->linein_mute=NULL;
    }
    mute->linein_mute.value=-1;

    return 0;
}

static int init_mdg_mute(struct mixer *mixer,struct mute_control *mute,struct mute_control_name *mute_name){
    if((NULL==mute->dsp_da0_mdg_mute.mixer) && (NULL!= mute_name->dsp_da0_mdg_mute)){
        mute->dsp_da0_mdg_mute.mixer=mixer_get_ctl_by_name(mixer,mute_name->dsp_da0_mdg_mute);
        free(mute_name->dsp_da0_mdg_mute);
        mute_name->dsp_da0_mdg_mute=NULL;
    }
    mute->dsp_da0_mdg_mute.value=-1;

    if((NULL==mute->dsp_da1_mdg_mute.mixer) && (NULL!= mute_name->dsp_da1_mdg_mute)){
        mute->dsp_da1_mdg_mute.mixer=mixer_get_ctl_by_name(mixer,mute_name->dsp_da1_mdg_mute);
        free(mute_name->dsp_da1_mdg_mute);
        mute_name->dsp_da1_mdg_mute=NULL;
    }
    mute->dsp_da1_mdg_mute.value=-1;

    if((NULL==mute->audio_mdg_mute.mixer) && (NULL!= mute_name->audio_mdg_mute)){
        mute->audio_mdg_mute.mixer=mixer_get_ctl_by_name(mixer,mute_name->audio_mdg_mute);
        free(mute_name->audio_mdg_mute);
        mute_name->audio_mdg_mute=NULL;
    }
    mute->audio_mdg_mute.value=-1;
    return 0;
}

int set_voice_dl_mute(struct audio_control *actl, bool mute){
    int ret=-ENOSYS;
    pthread_mutex_lock(&actl->lock);
    if(is_usecase_unlock(actl,UC_CALL)){
        if(NULL==actl->voice_ul_mute_ctl){
            LOG_E("set_voice_dl_mute failed,can not get mixer:VBC_DL_MUTE");
        }else{
            if(mute){
                ret=mixer_ctl_set_enum_by_string(actl->voice_dl_mute_ctl, "enable");
                actl->mute.mute_status|= DSP_VOICE_DL_MUTE;
            }else{
                ret=mixer_ctl_set_enum_by_string(actl->voice_dl_mute_ctl, "disable");
                actl->mute.mute_status &= ~DSP_VOICE_DL_MUTE;
            }
        }
    }
    pthread_mutex_unlock(&actl->lock);
    return ret;
}

int set_voice_ul_mute(struct audio_control *actl, bool mute){
    int ret=-ENOSYS;
    pthread_mutex_lock(&actl->lock);
    if(is_usecase_unlock(actl,UC_CALL)){
        if(NULL==actl->voice_ul_mute_ctl){
            LOG_E("set_voice_ul_mute failed,can not get mixer:VBC_UL_MUTE");
        }else{
            if(mute){
                ret=mixer_ctl_set_enum_by_string(actl->voice_ul_mute_ctl, "enable");
                actl->mute.mute_status |= DSP_VOICE_UL_MUTE;
            }else{
                ret=mixer_ctl_set_enum_by_string(actl->voice_ul_mute_ctl, "disable");
                actl->mute.mute_status &= ~DSP_VOICE_UL_MUTE;
            }
        }
    }
    pthread_mutex_unlock(&actl->lock);
    return ret;
}
int stream_routing_manager_create(struct audio_control *actl)
{
    int ret;

    actl->routing_mgr.is_exit = false;
    /* init semaphore to signal thread */
    ret = sem_init(&actl->routing_mgr.device_switch_sem, 0, 0);
    if (ret) {
        LOG_E("sem_init falied, code is %s", strerror(errno));
        return ret;
    }
    /* create a thread to manager the device routing switch.*/
    ret = pthread_create(&actl->routing_mgr.routing_switch_thread, NULL,
                         stream_routing_thread_entry, (void *)actl);

    list_init(&actl->switch_device_cmd_list);
    if (ret) {
        LOG_E("pthread_create falied, code is %s", strerror(errno));
        return ret;
    }

    return ret;
}

void stream_routing_manager_close(struct audio_control *actl)
{
    actl->routing_mgr.is_exit = true;
    /* release associated thread resource.*/
    sem_destroy(&actl->routing_mgr.device_switch_sem);
}

static void do_select_device(struct audio_control *actl,AUDIO_HW_APP_T audio_app_type,audio_devices_t device,bool is_in,bool update_param)
{
    int ret=0;

    pthread_mutex_lock(&actl->lock);
    if(is_in) {
        actl->in_devices=device;
        if(((true==actl->agdsp_ctl->agdsp_sleep_status) && (actl->usecase!=0))
            ||(AUDIO_HW_APP_INVALID==audio_app_type)){

            do_switch_in_devices(actl,audio_app_type,device);

            if((audio_app_type==AUDIO_HW_APP_BT_RECORD)
                && (AUDIO_DEVICE_IN_ALL_SCO&((~AUDIO_DEVICE_BIT_IN)&device))){
                if(actl->adev->bt_wbs){
                    set_vbc_bt_src_without_lock(actl->adev->dev_ctl,16000);
                    LOG_D("set src bt record  rate  wbs  audio_app_type %d",audio_app_type);
                } else{
                    set_vbc_bt_src_without_lock(actl->adev->dev_ctl,8000);
                    LOG_D("set src bt record  rate  nbs  audio_app_type %d",audio_app_type);
                }
            }

            if(true==update_param){
                set_audioparam_unlock(actl,PARAM_INDEVICES_CHANGE,&device,false);
            }
        }
    }
    else {
        if(((true==actl->agdsp_ctl->agdsp_sleep_status) && (actl->usecase!=0))
            ||(AUDIO_HW_APP_INVALID==audio_app_type)){
            if((0==(AUDIO_DEVICE_OUT_ALL_SCO&device))
                &&(AUDIO_DEVICE_OUT_ALL_SCO&actl->out_devices)){
                //bt src need disable before turn off bt_sco
                set_vbc_bt_src_unlock(actl,48000);
                if((actl->usecase&UC_BT_VOIP) && (actl->usecase&UC_VOIP_RECORD)){
                    if(device&AUDIO_DEVICE_OUT_WIRED_HEADSET){
                        actl->in_devices=AUDIO_DEVICE_OUT_WIRED_HEADSET;
                    }else{
                        actl->in_devices=AUDIO_DEVICE_IN_BUILTIN_MIC;
                    }
                    do_switch_in_devices(actl,AUDIO_HW_APP_VOIP_RECORD,actl->in_devices);
                }
            }

            if((audio_app_type==AUDIO_HW_APP_CALL) ||
                ((AUDIO_HW_APP_PRIMARY==audio_app_type) &&(actl->usecase&UC_CALL))){
                set_voice_mic(actl,audio_app_type,device);
            }

            ret = do_switch_out_devices(actl,audio_app_type,device);
            if(0 == ret)
                actl->out_devices=device;

            if(true==update_param){
                set_audioparam_unlock(actl,PARAM_OUTDEVICES_CHANGE,&device,false);
            }
        } else {
            actl->out_devices=device;
        }
    }
    pthread_mutex_unlock(&actl->lock);
}
static void  *select_device_cmd_send(struct audio_control *actl,AUDIO_HW_APP_T audio_app_type,audio_devices_t device,bool is_in,bool update_param, bool sync)
{
     struct switch_device_cmd *cmd = (struct switch_device_cmd *)calloc(1,
                                    sizeof(struct switch_device_cmd));

     pthread_mutex_lock(&actl->cmd_lock);
     sem_init(&cmd->sync_sem, 0, 0);
     if (sync) {
        cmd->is_sync = 1;
        cmd->cmd = SWITCH_DEVICE_SYNC;
     } else {
        cmd->is_sync = 0;
        cmd->cmd = SWITCH_DEVICE_ASYNC;
     }
     cmd->audio_app_type = audio_app_type;
     cmd->device = device;
     cmd->is_in = is_in;
     cmd->update_param = update_param;
     LOG_D("cmd is %d, audio_app_type is %d, device is %#x", cmd->cmd, cmd->audio_app_type, cmd->device);
     list_add_tail(&actl->switch_device_cmd_list, &cmd->node);
     pthread_mutex_unlock(&actl->cmd_lock);
     sem_post(&actl->routing_mgr.device_switch_sem);

     return cmd;
}

int select_devices_new(struct audio_control *actl, AUDIO_HW_APP_T audio_app_type, audio_devices_t device, bool is_in, bool update_param, bool sync)
{

    struct switch_device_cmd *cmd = NULL;

    LOG_D("select_devices_new devices 0x%x, is in %d app type:%d sync is %d", device, is_in,audio_app_type, sync);

    cmd = (struct switch_device_cmd *)select_device_cmd_send(actl,audio_app_type,device, is_in, update_param, sync);
    if (sync) {
        sem_wait(&cmd->sync_sem);
        free(cmd);
    }

    return 0;
}

int dev_ctl_get_in_pcm_config(struct audio_control *dev_ctl, int app_type, int * dev, struct pcm_config * config)
{
    int pcm_devices = 0;
    int ret = 0;

    if(config == NULL){
        return -1;
    }
    switch(app_type){
        case AUDIO_HW_APP_CALL_RECORD:
            LOG_I("dev_ctl_get_in_pcm_config AUDIO_HW_APP_CALL_RECORD");
            pcm_devices=dev_ctl->pcm_handle.record_devices[AUD_PCM_CALL];
            memcpy(config, &dev_ctl->pcm_handle.record[AUD_PCM_CALL], sizeof(struct pcm_config));
            break;
        case AUDIO_HW_APP_VOIP_RECORD:
            LOG_I("dev_ctl_get_in_pcm_config AUDIO_HW_APP_VOIP_RECORD");
            pcm_devices=dev_ctl->pcm_handle.record_devices[AUD_PCM_VOIP];
            memcpy(config, &dev_ctl->pcm_handle.record[AUD_PCM_VOIP], sizeof(struct pcm_config));
            config->period_size=1280;
            break;
        case AUDIO_HW_APP_FM_RECORD:
            LOG_I("dev_ctl_get_in_pcm_config AUDIO_HW_APP_FM_RECORD");
            pcm_devices=dev_ctl->pcm_handle.record_devices[AUD_PCM_DIGITAL_FM];
            memcpy(config, &dev_ctl->pcm_handle.record[AUD_PCM_DIGITAL_FM], sizeof(struct pcm_config));
            break;
        case AUDIO_HW_APP_VOIP_BT:
            LOG_I("dev_ctl_get_in_pcm_config AUDIO_HW_APP_VOIP_BT");
            pcm_devices=dev_ctl->pcm_handle.record_devices[AUD_PCM_BT_VOIP];
            memcpy(config, &dev_ctl->pcm_handle.record[AUD_PCM_BT_VOIP], sizeof(struct pcm_config));
            break;
        case AUDIO_HW_APP_BT_RECORD:
            LOG_I("dev_ctl_get_in_pcm_config AUDIO_HW_APP_BT_RECORD");
            pcm_devices=dev_ctl->pcm_handle.record_devices[AUD_PCM_BT_RECORD];
            memcpy(config, &dev_ctl->pcm_handle.record[AUD_PCM_BT_RECORD], sizeof(struct pcm_config));
            break;
        case AUDIO_HW_APP_NORMAL_RECORD:
            LOG_I("dev_ctl_get_in_pcm_config AUDIO_HW_APP_NORMAL_RECORD");
            pcm_devices=dev_ctl->pcm_handle.record_devices[AUD_PCM_MM_NORMAL];
            memcpy(config, &dev_ctl->pcm_handle.record[AUD_PCM_MM_NORMAL], sizeof(struct pcm_config));
            break;
        default:
            LOG_E("dev_ctl_get_in_pcm_config stream type:0x%x",app_type);
            ret = -1;
            break;
    }
    *dev = pcm_devices;
    return ret;
}


int dev_ctl_get_out_pcm_config(struct audio_control *dev_ctl,int app_type, int * dev, struct pcm_config *config)
{
    int pcm_devices = 0;
    int ret = 0;
    if(config == NULL){
        return -1;
    }
     switch(app_type){
        case AUDIO_HW_APP_CALL:
            pcm_devices=dev_ctl->pcm_handle.playback_devices[AUD_PCM_CALL];
            memcpy(config, &dev_ctl->pcm_handle.play[AUD_PCM_CALL], sizeof(struct pcm_config));
            LOG_I("dev_ctl_get_out_pcm_config UC_CALL:%p %d %d",config,config->period_count,config->period_size)
            break;
        case AUDIO_HW_APP_VAUDIO:
            pcm_devices=dev_ctl->pcm_handle.playback_devices[AUD_PCM_VOIP];
            memcpy(config, &dev_ctl->pcm_handle.play[AUD_PCM_VOIP], sizeof(struct pcm_config));
            LOG_I("dev_ctl_get_out_pcm_config UC_VOIP:%p %d %d",config,config->period_count,config->period_size)
            break;
        case AUDIO_HW_APP_VOIP:
        case AUDIO_HW_APP_VOIP_BT:
            pcm_devices=dev_ctl->pcm_handle.playback_devices[AUD_PCM_VOIP];
            memcpy(config, &dev_ctl->pcm_handle.play[AUD_PCM_VOIP], sizeof(struct pcm_config));
            config->period_size=1280;
            LOG_I("dev_ctl_get_out_pcm_config AUD_PCM_VOIP:%p %d %d",config,config->period_count,config->period_size);
            break;
        case AUDIO_HW_APP_PRIMARY:
            pcm_devices=dev_ctl->pcm_handle.playback_devices[AUD_PCM_MM_NORMAL];
            memcpy(config, &dev_ctl->pcm_handle.play[AUD_PCM_MM_NORMAL], sizeof(struct pcm_config));
#ifdef AUDIO_24BIT_PLAYBACK_SUPPORT
            if (dev_ctl->config.support_24bits)
                config->format =  PCM_FORMAT_S16_LE;
#endif
            LOG_I("dev_ctl_get_out_pcm_config AUD_PCM_MM_NORMAL:%p %d %d",config,config->period_count,config->period_size);
            break;
        case AUDIO_HW_APP_DEEP_BUFFER:
            pcm_devices=dev_ctl->pcm_handle.playback_devices[AUD_PCM_DEEP_BUFFER];
            memcpy(config, &dev_ctl->pcm_handle.play[AUD_PCM_DEEP_BUFFER], sizeof(struct pcm_config));
#ifdef AUDIO_24BIT_PLAYBACK_SUPPORT
            if (dev_ctl->config.support_24bits)
                config->format =  PCM_FORMAT_S24_LE;
#endif
            LOG_I("dev_ctl_get_out_pcm_config AUDIO_HW_APP_DEEP_BUFFER:%p %d %d",config,config->period_count,config->period_size)
            break;
        case AUDIO_HW_APP_DIRECT:
        default:
            LOG_E("dev_ctl_get_out_pcm_config stream type:0x%x",app_type);
            ret = -1;
            break;
    }
    *dev = pcm_devices;
    return ret;
}

 int set_offload_volume( struct audio_control *dev_ctl, float left,
                           float right){
    int mdg_arr[2] = {0};
    int max=0;
    int ret =0;
    pthread_mutex_lock(&dev_ctl->lock);
    if(is_usecase_unlock(dev_ctl,UC_OFFLOAD_PLAYBACK)){
        if(NULL==dev_ctl->offload_dg){
            dev_ctl->offload_dg = mixer_get_ctl_by_name(dev_ctl->mixer, "OFFLOAD DG Set");
        }

        if(dev_ctl->offload_dg) {
            max = mixer_ctl_get_range_max(dev_ctl->offload_dg);
            mdg_arr[0] = max * left;
            mdg_arr[1] = max * right;
            LOG_I("set_offload_volume left=%f,right=%f, max=%d", left, right, max);
            ret= mixer_ctl_set_array(dev_ctl->offload_dg, (void *)mdg_arr, 2);
        } else {
            LOG_E("set_offload_volume cannot get offload_dg ctrl");
        }
    }
    pthread_mutex_unlock(&dev_ctl->lock);
    return ret;
}

static  int apply_audio_profile_param_firmware(struct mixer_ctl *eq_select,int id){
    struct mixer *mixer;
    int ret=-1;

    if(NULL !=eq_select){
        ret = mixer_ctl_set_value(eq_select, 0, id);
        if (ret != 0) {
            LOG_E("apply_audio_profile_param_firmware Failed \n");
        }else{
            LOG_D("apply_audio_profile_param_firmware ret:%x val:%x",ret,id);
        }
    }else{
        LOG_E("apply_audio_profile_param_firmware failed");
    }
    return 0;
}

static int set_sprd_output_devices_param(struct sprd_codec_mixer_t *codec, struct sprd_code_param_t *param,
    int vol_index,int out_devices){
    int ret=0;

    if(param==NULL){
        return -1;
    }

#ifdef AUDIO_SPEAKER_USE_HEADSET_CHANNEL
    if(out_devices & AUDIO_DEVICE_OUT_SPEAKER){
        out_devices =AUDIO_DEVICE_OUT_WIRED_HEADPHONE;
    }
#endif

    LOG_I("set_sprd_output_devices_param vol_index:%d out_devices:%d",vol_index,out_devices);

    if(out_devices & AUDIO_DEVICE_OUT_EARPIECE){
        if(NULL != codec->ear_playback_volume) {
            ret = mixer_ctl_set_value(codec->ear_playback_volume, 0,
                                      param->ear_playback_volume[vol_index]);
            if (ret != 0) {
                LOG_E("set_sprd_output_devices_param set ear_playback_volume failed");
            }else{
                LOG_D("set_sprd_output_devices_param set ear_playback_volume :0x%x",param->ear_playback_volume[vol_index]);
            }
        }

        if(NULL != codec->dacl_playback_volume) {
            ret = mixer_ctl_set_value(codec->dacl_playback_volume, 0,
                                      param->dacl_playback_volume[vol_index]);
            if (ret != 0) {
                LOG_E("set_sprd_output_devices_param set dacl_playback_volume failed");
            }else{
                LOG_D("set_sprd_output_devices_param set dacl_playback_volume :0x%x",param->dacl_playback_volume[vol_index]);
            }
        }
    }

    if(out_devices & AUDIO_DEVICE_OUT_SPEAKER){
        if(NULL != codec->dacs_playback_volume) {
            ret = mixer_ctl_set_value(codec->dacs_playback_volume, 0,
                                      param->dacs_playback_volume[vol_index]);
            if (ret != 0) {
                LOG_E("set_sprd_output_devices_param set dacs_playback_volume failed");
            }else{
                LOG_D("set_sprd_output_devices_param set dacs_playback_volume :0x%x",param->dacs_playback_volume[vol_index]);
            }
        }

        if(NULL != codec->spkl_playback_volume) {
            ret = mixer_ctl_set_value(codec->spkl_playback_volume, 0,
                                      param->spkl_playback_volume[vol_index]);
            if (ret != 0) {
                LOG_E("set_sprd_output_devices_param set spkl_playback_volume failed");
            }else{
                LOG_D("set_sprd_output_devices_param set spkl_playback_volume :0x%x",param->spkl_playback_volume[vol_index]);
            }
        }

        if(NULL != codec->spkr_playback_volume) {
            ret = mixer_ctl_set_value(codec->spkr_playback_volume, 0,
                                      param->spkr_playback_volume[vol_index]);
            if (ret != 0) {
                LOG_E("set_sprd_output_devices_param set spkr_playback_volume failed");
            }else{
                LOG_D("set_sprd_output_devices_param set spkr_playback_volume :0x%x",param->spkr_playback_volume[vol_index]);
            }
        }

        if(NULL != codec->inner_pa) {
            ret = mixer_ctl_set_value(codec->inner_pa, 0,
                                      param->inter_pa_config);
            if (ret != 0) {
                LOG_E("set_sprd_output_devices_param set inner_pa failed");
            }else{
                LOG_D("set_sprd_output_devices_param set inner_pa :0x%x",param->inter_pa_config);
            }
        }
    }

    if(out_devices & (AUDIO_DEVICE_OUT_WIRED_HEADPHONE|AUDIO_DEVICE_OUT_WIRED_HEADSET)){
        if(NULL != codec->dacs_playback_volume) {
            ret = mixer_ctl_set_value(codec->dacl_playback_volume, 0,
                                      param->dacl_playback_volume[vol_index]);
            if (ret != 0) {
                LOG_E("set_sprd_output_devices_param set dacl_playback_volume failed");
            }else{
                LOG_D("set_sprd_output_devices_param set dacl_playback_volume :0x%x",param->dacl_playback_volume[vol_index]);
            }
        }

        if(NULL != codec->dacs_playback_volume) {
            ret = mixer_ctl_set_value(codec->dacr_playback_volume, 0,
                                      param->dacr_playback_volume[vol_index]);
            if (ret != 0) {
                LOG_E("set_sprd_output_devices_param set dacr_playback_volume failed");
            }else{
                LOG_D("set_sprd_output_devices_param set dacr_playback_volume :0x%x",param->dacr_playback_volume[vol_index]);
            }
        }

        if(NULL != codec->hpl_playback_volume) {
            ret = mixer_ctl_set_value(codec->hpl_playback_volume, 0,
                                      param->hpl_playback_volume[vol_index]);
            if (ret != 0) {
                LOG_E("set_sprd_output_devices_param set hpl_playback_volume failed");
            }else{
                LOG_D("set_sprd_output_devices_param set hpl_playback_volume :0x%x",param->hpl_playback_volume[vol_index]);
            }
        }
        if(NULL != codec->hpr_playback_volume) {
            ret = mixer_ctl_set_value(codec->hpr_playback_volume, 0,
                                      param->hpr_playback_volume[vol_index]);
            if (ret != 0) {
                LOG_E("set_sprd_output_devices_param set hpr_playback_volume failed");
            }else{
                LOG_D("set_sprd_output_devices_param set hpr_playback_volume :0x%x",param->hpr_playback_volume[vol_index]);
            }
        }

        if(NULL != codec->hp_inner_pa) {
            ret = mixer_ctl_set_value(codec->hp_inner_pa, 0,
                                      param->inter_hp_pa_config);
            if (ret != 0) {
                LOG_E("set_sprd_output_devices_param set hp_inner_pa failed");
            }else{
                LOG_D("set_sprd_output_devices_param set hp_inner_pa :0x%x",param->inter_hp_pa_config);
            }
        }
    }

    return ret;
}

static int set_sprd_input_devices_param( struct sprd_codec_mixer_t *codec, struct sprd_code_param_t *param,int mic_switch){
    int ret=0;

    if(0==mic_switch){
        return 0;
    }

    if(mic_switch & SPRD_MAIN_MIC_PATH_SWITCH) {
        if(NULL != codec->mic_boost) {
            ret = mixer_ctl_set_value(codec->mic_boost, 0, param->mic_boost);
            if (ret != 0) {
                LOG_E("set_sprd_input_devices_param set mic_boost failed");
            }else{
                LOG_D("set_sprd_input_devices_param set mic_boost :0x%x",param->mic_boost);
            }
        }
    }

    if(mic_switch & SPRD_BACK_MIC_PATH_SWITCH) {
        if(NULL != codec->auxmic_boost) {
            ret = mixer_ctl_set_value(codec->auxmic_boost, 0, param->mic_boost);
            if (ret != 0) {
                LOG_E("set_sprd_input_devices_param set auxmic_boost failed");
            }else{
                LOG_D("set_sprd_input_devices_param set auxmic_boost :0x%x",param->mic_boost);
            }
        }
    }

    if(mic_switch & SPRD_HEADSET_MIC_PATH_SWITCH) {
        if(NULL != codec->headmic_boost) {
            ret = mixer_ctl_set_value(codec->headmic_boost, 0, param->mic_boost);
            if (ret != 0) {
                LOG_E("set_sprd_input_devices_param set headmic_boost failed");
            }else{
                LOG_D("set_sprd_input_devices_param set headmic_boost :0x%x",param->mic_boost);
            }
        }
    }

    if(NULL != codec->adcl_capture_volume) {
        ret = mixer_ctl_set_value(codec->adcl_capture_volume, 0,
                                  param->adcl_capture_volume);
            if (ret != 0) {
                LOG_E("set_sprd_input_devices_param set adcl_capture_volume failed");
            }else{
                LOG_D("set_sprd_input_devices_param set adcl_capture_volume :0x%x",param->adcl_capture_volume);
            }
    }

    if(NULL != codec->adcr_capture_volume) {
        ret = mixer_ctl_set_value(codec->adcr_capture_volume, 0,
                                  param->adcr_capture_volume);
        if (ret != 0) {
            LOG_E("set_sprd_input_devices_param set adcr_capture_volume failed");
        }else{
            LOG_D("set_sprd_input_devices_param set adcr_capture_volume :0x%x",param->adcr_capture_volume);
        }
    }

    return 0;
}

/*volume start form 0 */
int set_dsp_volume(struct audio_control *ctl,int volume){
    int ret=0;
    LOG_D("set_dsp_volume:%d",volume);

    return set_audioparam(ctl,PARAM_VOICE_VOLUME_CHANGE, &volume,false);
}

static void * get_ap_audio_param(AUDIO_PARAM_T  *audio_param,int param_id)
{
    int32_t offset=0;
    AUDIOVBCEQ_PARAM_T *param=&audio_param->param[SND_AUDIO_PARAM_RECORD_PROCESS_PROFILE];
    struct param_infor_t  *param_infor=&audio_param->infor->data[SND_AUDIO_PARAM_RECORD_PROCESS_PROFILE];

    if((param->data==NULL)||(param_id>=PROFILE_MODE_MAX)){
        return NULL;
    }

    offset=param_infor->offset[param_id];

    if(AUDIO_PARAM_INVALID_32BIT_OFFSET==offset){
        LOG_E("get_ap_audio_param failed offset:%d %x",offset,offset);
        return NULL;
    }else{
        LOG_I("get_ap_audio_param:%d %d %p",param_id,offset,param->data);
        return  (param->data+offset);
    }
}

static int get_voice_mic_select(AUDIO_PARAM_T  *audio_param,int param_id){
    int16_t mic_swicth=0;
    int16_t * mic_param=NULL;
    mic_param=(int16_t*)get_ap_audio_param(audio_param,param_id);

    if(NULL!=mic_param){
        mic_swicth=*mic_param;
    }
    LOG_I("get_voice_mic_select param_id:%d %d",param_id,mic_swicth);
    return mic_swicth;
}

static int get_default_voice_mic_select(audio_devices_t device){
    int mic_select=0;

    if(device &AUDIO_DEVICE_OUT_ALL_SCO){
        mic_select =0;
    }else if(AUDIO_DEVICE_OUT_WIRED_HEADSET==device){
        mic_select = SPRD_HEADSET_MIC_PATH_SWITCH;
    }else{
        mic_select = SPRD_BACK_MIC_PATH_SWITCH|SPRD_MAIN_MIC_PATH_SWITCH;
    }
    return mic_select;
}

static int get_normal_mic_select(uint32_t in_devices){
    int mic_switch=0;//bit 0:main mic, bit1:back mic, bit2:headset mic

    int in_dev = in_devices & ~AUDIO_DEVICE_BIT_IN;
    if(in_dev&AUDIO_DEVICE_IN_WIRED_HEADSET){
        mic_switch=SPRD_HEADSET_MIC_PATH_SWITCH;
    }else{
        if(in_dev&AUDIO_DEVICE_IN_BUILTIN_MIC){
            mic_switch=SPRD_MAIN_MIC_PATH_SWITCH;
        }

        if(in_dev&AUDIO_DEVICE_IN_BACK_MIC){
            mic_switch|=SPRD_BACK_MIC_PATH_SWITCH;
        }
    }
    return mic_switch;
}


static int is_voice_active(int usecase)
{
    return (usecase & (UC_CALL|UC_VOIP|UC_LOOP|UC_BT_VOIP));
}
static int is_voip_active(int usecase)
{
    return usecase & (UC_VOIP|UC_BT_VOIP);
}
static int is_call_active(int usecase)
{
    return usecase & UC_CALL;
}
static int is_loop_active(int usecase)
{
    return usecase & UC_LOOP;
}
static int is_playback_active(int usecase)
{
    return usecase & (UC_NORMAL_PLAYBACK|UC_DEEP_BUFFER_PLAYBACK|UC_OFFLOAD_PLAYBACK);
}
static int is_fm_active(int usecase)
{
    return usecase & UC_FM;
}
static int is_record_active(int usecase)
{
    return usecase & (UC_MM_RECORD | UC_BT_RECORD);
}

static uint32_t make_param_kcontrl_value(uint8_t offset, uint8_t param_id, uint8_t dsp_case)
{
    return ((offset<<24)|(param_id<<16)|dsp_case);
}

static uint8_t get_audio_param_offset(AUDIO_PARAM_T  *audio_param,int param_type,uint8_t param_id){
    AUDIOVBCEQ_PARAM_T *param=&audio_param->param[param_type];
    struct param_infor_t  *param_infor=&audio_param->infor->data[param_type];
    uint8_t default_offset=param_id;
    uint8_t param_offset=default_offset;

    if(param_id>=PROFILE_MODE_MAX){
        return AUDIO_PARAM_INVALID_8BIT_OFFSET;
    }
    if(audio_param->infor==NULL){
        param_offset=default_offset;
        LOG_W("get_audio_param_offset infor is null return");
        goto out;
    }

    if(NULL!=param->data){
        if(param_infor->param_struct_size!=param->param_struct_size){
            LOG_W("get_audio_param_offset type:%d param_id:%d failed param_struct_size:%x %x",
                param_type,param_id,param->param_struct_size,param_infor->param_struct_size);
            param_offset=default_offset;
            goto out;
        }
    }else{
        if(param_infor->param_struct_size<=0){
            param_offset=default_offset;
            LOG_W("get_audio_param_offset type:%d param_id:%d failed param_struct_size:%x",
                param_type,param_id,param_infor->param_struct_size);
            goto out;
        }
    }

    if(AUDIO_PARAM_INVALID_32BIT_OFFSET==param_infor->offset[param_id]){
        LOG_I("get_audio_param_offset invalid offset, param_id:%d type:%d name:%s",param_id,param_type,get_audio_param_name(param_id));
        param_offset=AUDIO_PARAM_INVALID_8BIT_OFFSET;
        goto out;
    }

    param_offset=(uint8_t)(param_infor->offset[param_id]/param_infor->param_struct_size);

    if((param_infor->offset[param_id]%param_infor->param_struct_size)){
        LOG_E("get_audio_param_offset error param_id:%d type:%d name:%s offset:0x%x",
        param_id,param_type,get_audio_param_name(param_id),param_infor->offset[param_id]);
    }

    if(param_offset>=PROFILE_MODE_MAX){
        LOG_I("get_audio_param_offset not support param_id:%d type:%d name:%s",param_id,param_type,get_audio_param_name(param_id));
        param_offset=AUDIO_PARAM_INVALID_8BIT_OFFSET;
        goto out;
    }

    if(param_offset!=default_offset){
        LOG_I("get_audio_param_offset type:%d param_offset:%d default_offset:%d",param_type,param_offset,default_offset);
    }

out:
    return param_offset;
}


static struct sprd_code_param_t * get_sprd_codec_param(AUDIO_PARAM_T  *audio_param,uint8_t param_id){
    AUDIOVBCEQ_PARAM_T *codec=&audio_param->param[SND_AUDIO_PARAM_CODEC_PROFILE];
    uint8_t  param_offset=get_audio_param_offset(audio_param,SND_AUDIO_PARAM_CODEC_PROFILE,param_id);

    if(AUDIO_PARAM_INVALID_8BIT_OFFSET!=param_offset){
        return (struct sprd_code_param_t * )(codec->data+(param_offset*codec->param_struct_size));
    }else{
        return NULL;
    }
}


static int  set_vbc_param(struct audio_control *dev_ctl,uint8_t offset, uint8_t param_id, uint8_t dsp_case) {
    AUDIO_PARAM_T  *audio_param = dev_ctl->audio_param;
    struct audio_param_res  *param_res = &(dev_ctl->param_res);
    int ret = 0;
    uint32_t param = 0;

    param = make_param_kcontrl_value(offset,  param_id,  dsp_case);
    LOG_D("set_vbc_param:%d case:%d %d %d",param_id,dsp_case,param_res->cur_vbc_playback_id,param_res->cur_vbc_id);
    if((DAI_ID_NORMAL_OUTDSP_PLAYBACK==dsp_case)||(DAI_ID_FAST_P==dsp_case)
        ||(DAI_ID_OFFLOAD==dsp_case)){
        if(param_res->cur_vbc_playback_id != param_id){
            ret=apply_audio_profile_param_firmware(
                audio_param->select_mixer[SND_AUDIO_PARAM_DSP_VBC_PROFILE_DSP],
                param);
             LOG_I("UPDATE_PARAM_VBC_PLAY:%s play:%d dsp_case:%d",get_audio_param_name(param_id),param_res->cur_vbc_playback_id,dsp_case);
             param_res->cur_vbc_playback_id = param_id;
        }
    }else{
        if(param_res->cur_vbc_id != param_id) {
            ret=apply_audio_profile_param_firmware(
                audio_param->select_mixer[SND_AUDIO_PARAM_DSP_VBC_PROFILE_DSP],
                param);
            if(DAI_ID_NORMAL_OUTDSP_CAPTURE!=dsp_case){
                param_res->cur_vbc_playback_id = param_id;
            }
            LOG_I("UPDATE_PARAM_VBC_MASTER:%s play:%d dsp_case:%d",get_audio_param_name(param_id),param_res->cur_vbc_playback_id,dsp_case);
            param_res->cur_vbc_id = param_id;
        }
    }
    return ret;
}

static uint8_t get_voice_param(audio_devices_t devices, bool is_nrec,aud_net_m net_mode,bool wb_mode)
{
    int ret = 0;
    char name[128]={0};
    uint8_t param_id = PROFILE_MODE_MAX;

    strcat((char *)name ,"Audio");

    if(devices&(AUDIO_DEVICE_OUT_BLUETOOTH_SCO|AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET)){
        if(is_nrec==true)
            strcat((char *)name ,"\\BTHSNREC");
        else
            strcat((char *)name ,"\\BTHS");
    }else if(devices&AUDIO_DEVICE_OUT_WIRED_HEADSET){
        strcat((char *)name ,"\\Headset4P");
    }else if(devices&AUDIO_DEVICE_OUT_WIRED_HEADPHONE){
        strcat((char *)name ,"\\Headset3P");
    }else if(devices&AUDIO_DEVICE_OUT_SPEAKER){
        strcat((char *)name ,"\\Handsfree");
    }else if(devices&AUDIO_DEVICE_OUT_EARPIECE){
        strcat((char *)name ,"\\Handset");
    }else{
        strcat((char *)name ,"\\None");
        ret = -1;
        goto out;
    }

    switch(net_mode){
        case AUDIO_NET_GSM:
            strcat((char *)name ,"\\GSM");
            break;
        case AUDIO_NET_TDMA:
            strcat((char *)name ,"\\TDMA");
            break;
        case AUDIO_NET_WCDMA:
            if(wb_mode==false){
                strcat((char *)name ,"\\WCDMA_NB");
            }else{
                strcat((char *)name ,"\\WCDMA_WB");
            }
            break;
        case AUDIO_NET_VOLTE:
            if(wb_mode==false){
                strcat((char *)name ,"\\VOLTE_NB");
            }else{
                strcat((char *)name ,"\\VOLTE_WB");
            }
            break;
        case AUDIO_NET_LOOP:
            strcat((char *)name ,"\\Loopback");
            break;
        default:
            strcat((char *)name ,"\\None");
            ret = -1;
            break;
    }

    if(!ret){
        param_id = get_audio_param_id(name);
    }

out:
    return param_id;
}

static uint8_t get_voip_param(audio_devices_t devices, bool is_nrec){
    int ret = 0;
    char name[128]={0};
    uint8_t param_id = PROFILE_MODE_MAX;

    strcat((char *)name ,"Audio");

    if(devices&(AUDIO_DEVICE_OUT_BLUETOOTH_SCO|AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET)){
        if(is_nrec==true)
            strcat((char *)name ,"\\BTHSNREC");
        else
            strcat((char *)name ,"\\BTHS");
    }else if(devices&AUDIO_DEVICE_OUT_WIRED_HEADSET){
        strcat((char *)name ,"\\Headset4P");
    }else if(devices&AUDIO_DEVICE_OUT_WIRED_HEADPHONE){
        strcat((char *)name ,"\\Headset3P");
    }else if(devices&AUDIO_DEVICE_OUT_SPEAKER){
        strcat((char *)name ,"\\Handsfree");
    }else if(devices&AUDIO_DEVICE_OUT_EARPIECE){
        strcat((char *)name ,"\\Handset");
    }else{
        strcat((char *)name ,"\\None");
        ret = -1;
        goto out;
    }
    strcat((char *)name ,"\\VOIP");

     if(!ret) {
        param_id = get_audio_param_id(name);
    }
    LOG_I("get_voip_param:%s %x",name,param_id);
out:
    return param_id;
}

uint8_t get_loopback_param(audio_devices_t out_devices,audio_devices_t in_devices)
{
    int ret = 0;
    char name[128]={0};
    uint8_t param_id = PROFILE_MODE_MAX;

    strcat((char *)name ,"Loopback");
    if(out_devices&AUDIO_DEVICE_OUT_WIRED_HEADSET){
        strcat((char *)name ,"\\Headset4P");
    }else if(out_devices&AUDIO_DEVICE_OUT_WIRED_HEADPHONE){
        strcat((char *)name ,"\\Headset3P");
    }else if(out_devices&AUDIO_DEVICE_OUT_SPEAKER){
        strcat((char *)name ,"\\Handsfree");
    }else if(out_devices&AUDIO_DEVICE_OUT_EARPIECE){
        strcat((char *)name ,"\\Handset");
    }else{
        strcat((char *)name ,"\\None");
        ret = -1;
        goto out;
    }

    if(AUDIO_DEVICE_IN_BUILTIN_MIC == in_devices){
        strcat((char *)name ,"\\MainMic");
    }else if(AUDIO_DEVICE_IN_BACK_MIC == in_devices){
        strcat((char *)name ,"\\AuxMic");
    }else if(AUDIO_DEVICE_IN_WIRED_HEADSET == in_devices){
        strcat((char *)name ,"\\HeadMic");
    }else{
        strcat((char *)name ,"\\None");
        ret = -1;
    }

    if(!ret) {
        param_id = get_audio_param_id(name);
    }

out:
    return param_id;
}

static uint8_t get_playback_param(audio_devices_t out_devices)
{
    uint8_t param_id = PROFILE_MODE_MAX;

    switch(out_devices){
        case AUDIO_DEVICE_OUT_EARPIECE:
            param_id=PROFILE_MODE_MUSIC_Handset_Playback;
        break;
        case AUDIO_DEVICE_OUT_SPEAKER:
            param_id=PROFILE_MODE_MUSIC_Handsfree_Playback;
        break;
        case AUDIO_DEVICE_OUT_WIRED_HEADSET:
        case AUDIO_DEVICE_OUT_WIRED_HEADPHONE:
            param_id=PROFILE_MODE_MUSIC_Headset_Playback;
        break;
        case AUDIO_DEVICE_OUT_WIRED_HEADSET|AUDIO_DEVICE_OUT_SPEAKER:
        case AUDIO_DEVICE_OUT_WIRED_HEADPHONE|AUDIO_DEVICE_OUT_SPEAKER:
            param_id=PROFILE_MODE_MUSIC_Headfree_Playback;
        break;
        default:
            param_id = PROFILE_MODE_MAX;
        break;
    }
    return param_id;
}

static uint8_t get_fm_param(audio_devices_t out_devices)
{
    uint8_t param_id =PROFILE_MODE_MAX;

    if(out_devices&(AUDIO_DEVICE_OUT_WIRED_HEADSET|AUDIO_DEVICE_OUT_WIRED_HEADPHONE)){
        param_id=PROFILE_MODE_MUSIC_Headset_FM;
    }else if(out_devices&AUDIO_DEVICE_OUT_SPEAKER){
        param_id=PROFILE_MODE_MUSIC_Handsfree_FM;
    }
    return param_id;
}

static uint8_t get_record_param(audio_devices_t in_devices)
{
    char name[128]={0};
    uint8_t param_id = -1;

    strcat((char *)name ,"Music");

    if(AUDIO_DEVICE_IN_WIRED_HEADSET == in_devices){
       strcat((char *)name ,"\\Headset");
    } else if (AUDIO_DEVICE_IN_ALL_SCO == in_devices){
       strcat((char *)name ,"\\Bluetooth");
    } else {
       strcat((char *)name ,"\\Handsfree");
    }
    strcat((char *)name ,"\\Record");
    param_id = get_audio_param_id(name);
    return param_id;
}

void * get_ap_record_param(AUDIO_PARAM_T  *audio_param,audio_devices_t in_devices)
{
    uint8_t param_id =get_record_param(in_devices);
    return  get_ap_audio_param(audio_param,param_id);
}
static int set_voice_dg_param(struct audio_control *dev_ctl,int param_id,int volume){

    int ret = 0;
    struct audio_param_res  *param_res = &(dev_ctl->param_res);
    LOG_D("set_voice_dg_param:%d cur_voice_dg_id:%d volume:%d",
        param_id,param_res->cur_voice_dg_id,volume);
    if ((param_id != param_res->cur_voice_dg_id)
     || (param_res->cur_voice_dg_volume != volume)) {
        ret = set_vdg_gain(&dev_ctl->dg_gain,param_id,volume);
        param_res->cur_voice_dg_id = param_id;
        param_res->cur_voice_dg_volume = volume;
    }

    return ret;
}

static int set_fm_dg_param(struct audio_control *dev_ctl,int param_id,int volume){

    int ret = 0;
    struct audio_param_res  *param_res = &(dev_ctl->param_res);
    LOG_D("set_voice_dg_param:%d cur_fm_dg_id:%d volume",
        param_id,param_res->cur_fm_dg_id,volume);
    if((param_id != param_res->cur_fm_dg_id)
         || (param_res->cur_fm_dg_volume != volume)) {
        ret = set_vdg_gain(&dev_ctl->dg_gain, param_id, volume);
        param_res->cur_fm_dg_id = param_id;
        param_res->cur_fm_dg_volume = volume;
    }

    return ret;
}

static int set_record_dg_param(struct audio_control *dev_ctl,int param_id,int volume){

    int ret = 0;
    struct audio_param_res  *param_res = &(dev_ctl->param_res);
    LOG_D("set_record_dg_param:%d cur_record_dg_id:%d volume",
        param_id,param_res->cur_record_dg_id,volume);
    if (param_id != param_res->cur_record_dg_id) {
        ret = set_vdg_gain(&dev_ctl->dg_gain, param_id, volume);
        param_res->cur_record_dg_id =param_id;
    }

    return ret;
}


static int set_play_dg_param(struct audio_control *dev_ctl,int param_id,int volume){

    int ret = 0;
    struct audio_param_res  *param_res = &(dev_ctl->param_res);
    LOG_D("set_play_dg_param:%d cur_playback_dg_id:%d volume:%d",
        param_id,param_res->cur_playback_dg_id,volume);
    if (param_id != param_res->cur_playback_dg_id) {
        ret = set_vdg_gain(&dev_ctl->dg_gain, param_id, volume);
        param_res->cur_playback_dg_id = param_id;
    }

    return ret;
}

static int  set_dsp_param(struct audio_control *dev_ctl, uint8_t param_id, uint8_t dsp_case,int volume) {
    AUDIO_PARAM_T  *audio_param = dev_ctl->audio_param;
    struct audio_param_res  *param_res = &(dev_ctl->param_res);
    int ret = 0;
    bool update_volume=false;

    if(param_res->cur_dsp_id != param_id){
        uint32_t param = 0;

        uint8_t offset=(uint8_t)get_audio_param_offset(dev_ctl->audio_param,SND_AUDIO_PARAM_AUDIO_STRUCTURE_PROFILE,param_id);
        if(AUDIO_PARAM_INVALID_8BIT_OFFSET!=offset){
            param = make_param_kcontrl_value(offset,  param_id,  dsp_case);
            LOG_I("UPDATE_PARAM_DSP:%s volume:%d offset:0x%x",get_audio_param_name(param_id),volume,offset);
            ret =apply_audio_profile_param_firmware(
                audio_param->select_mixer[SND_AUDIO_PARAM_AUDIO_STRUCTURE_PROFILE],
                param);
        }

        offset=(uint8_t)get_audio_param_offset(dev_ctl->audio_param,SND_AUDIO_PARAM_NXP_PROFILE,param_id);
        if(AUDIO_PARAM_INVALID_8BIT_OFFSET!=offset){
            param = make_param_kcontrl_value(offset,  param_id,  dsp_case);
            ret =apply_audio_profile_param_firmware(
                audio_param->select_mixer[SND_AUDIO_PARAM_NXP_PROFILE],
                param);
            param_res->cur_dsp_id=param_id;
            update_volume=true;
        }
    }

    LOG_D("set_dsp_param cur_dsp_volume:%d %d",param_res->cur_dsp_volume,volume);
    if((param_res->cur_dsp_volume != volume)||(true==update_volume)) {
        ret=mixer_ctl_set_value(dev_ctl->dsp_volume_ctl, 0, (volume+1));
        if (ret != 0) {
            LOG_E("set_dsp_volume Failed volume:%d\n",(volume+1));
        }else{
            LOG_I("set_dsp_param DSP_VOLUME:%d",(volume+1));
        }
        param_res->cur_dsp_volume = volume;
    }
    return ret;
}

static int  set_codec_playback_param(struct audio_control *dev_ctl,uint8_t param_id,uint32_t out_devices,int volume) {
    int ret = 0;
    AUDIO_PARAM_T  *audio_param = dev_ctl->audio_param;
    struct audio_param_res  *param_res = &(dev_ctl->param_res);
    LOG_D("set_codec_playback_param:0x%x out_devices:%d volume:%d",param_id,out_devices, volume);

        if(AUD_REALTEK_CODEC_TYPE == dev_ctl->codec_type){
            if(param_res->cur_codec_p_id != param_id) {
                uint8_t offset=get_audio_param_offset(audio_param,SND_AUDIO_PARAM_CODEC_PROFILE,param_id);
                if(AUDIO_PARAM_INVALID_8BIT_OFFSET!=offset){
                    LOG_I("UPDATE_PARAM_CODEC_PLAY:%s volume:%d",get_audio_param_name(param_id),volume);
                    ret =apply_audio_profile_param_firmware(
                        audio_param->select_mixer[SND_AUDIO_PARAM_CODEC_PROFILE],
                        offset);
                    param_res->cur_codec_p_id = param_id;
                }
            }
        }else{
            if((param_res->cur_codec_p_id != param_id) ||(param_res->cur_codec_p_volume != volume)) {
                struct sprd_code_param_t*  codec_param = get_sprd_codec_param(audio_param,param_id);
                if(NULL!=codec_param){
                    LOG_I("UPDATE_PARAM_CODEC_PLAY:%s cur_codec_p_volume:%d volume:%d",get_audio_param_name(param_id),param_res->cur_codec_p_volume,
                        volume);
                    ret = set_sprd_output_devices_param(&dev_ctl->codec,codec_param,volume,out_devices);
                    param_res->cur_codec_p_id = param_id;
                    param_res->cur_codec_p_volume = volume;
                }
            }
     }
    return 0;
}
static int  set_codec_record_param(struct audio_control *dev_ctl,uint8_t param_id,int mic_select) {
    int ret = 0;
    AUDIO_PARAM_T  *audio_param = dev_ctl->audio_param;
    struct audio_param_res  *param_res = &(dev_ctl->param_res);

    if(AUD_REALTEK_CODEC_TYPE == dev_ctl->codec_type){
        if(param_res->cur_codec_c_id != param_id) {
            uint8_t offset=get_audio_param_offset(audio_param,SND_AUDIO_PARAM_CODEC_PROFILE,param_id);
            if(AUDIO_PARAM_INVALID_8BIT_OFFSET!=offset){
                LOG_I("UPDATE_PARAM CODEC_INPUT:%s",get_audio_param_name(param_id));
                ret =apply_audio_profile_param_firmware(
                    audio_param->select_mixer[SND_AUDIO_PARAM_CODEC_PROFILE],
                    offset);
                param_res->cur_codec_c_id = param_id;
            }
        }
    }else{
        struct sprd_code_param_t*  codec_param = get_sprd_codec_param(audio_param,param_id);
        LOG_D("set_codec_record_param:0x%x cur_codec_c_id:%d mic_select:0x%x",param_id,
            param_res->cur_codec_c_id,mic_select);

        if((param_res->cur_codec_c_id != param_id) && (NULL!=codec_param)){
            LOG_I("UPDATE_PARAM_CODEC_INPUT:%s mic_switch:%d",get_audio_param_name(param_id),mic_select);
            ret = set_sprd_input_devices_param(&dev_ctl->codec,codec_param,mic_select);
            param_res->cur_codec_c_id = param_id;
        }
    }
    return ret;
}

static int set_voice_mic(struct audio_control *actl,AUDIO_HW_APP_T audio_app_type,audio_devices_t device){
    int param_id=0;
    int mic_select=0;
    int in_device=0;
    struct audio_param_res * param_res=&actl->param_res;

    param_id = get_voice_param(device, param_res->bt_infor.bluetooth_nrec,
        param_res->net_mode,param_res->wb_mode);

    if(param_id<PROFILE_MODE_MAX){
        mic_select = get_voice_mic_select(actl->audio_param, param_id);//mic config with audio param
    }

    if(0==(device&AUDIO_DEVICE_OUT_ALL_SCO)){
        if((mic_select&(SPRD_MAIN_MIC_PATH_SWITCH|SPRD_BACK_MIC_PATH_SWITCH|SPRD_HEADSET_MIC_PATH_SWITCH))==0){
            mic_select = get_default_voice_mic_select(device);
        }
    }

    if(mic_select& SPRD_MAIN_MIC_PATH_SWITCH){
        in_device|=AUDIO_DEVICE_IN_BUILTIN_MIC;
    }

    if(mic_select & SPRD_BACK_MIC_PATH_SWITCH){
        in_device|=AUDIO_DEVICE_IN_BACK_MIC;
    }

    if(mic_select & SPRD_HEADSET_MIC_PATH_SWITCH){
        in_device|=AUDIO_DEVICE_IN_WIRED_HEADSET;
    }

    if(in_device!=0){
        actl->in_devices=in_device;
        do_switch_in_devices(actl,audio_app_type,actl->in_devices);
    }

    LOG_I("set_voice_mic in_device:0x%x mic_select:%d",in_device,mic_select);
    return in_device;
}

int select_audio_param_unlock(struct audio_control *dev_ctl,struct audio_param_res  *param_res){
    uint8_t param_id = 0;
    int16_t dsp_case = 0;
    int volume = 0;
    uint32_t mic_select = 0;
    int ret = 0;
    uint8_t offset=0;
    if(is_voice_active(param_res->usecase)) {

        if(param_res->out_devices&AUDIO_DEVICE_OUT_ALL_SCO){
            volume=0;    //BT_SCO volume is cotrolled  by bt headset, set  audio param volume to 0
        }else{
            volume=param_res->voice_volume;
        }

        if(is_call_active(param_res->usecase)) {
            param_id = get_voice_param(param_res->out_devices, param_res->bt_infor.bluetooth_nrec,
            param_res->net_mode,
            param_res->wb_mode);
            dsp_case=DAI_ID_VOICE;
        }
        else  if(is_voip_active(param_res->usecase)) {
            param_id = get_voip_param(param_res->out_devices,param_res->bt_infor.bluetooth_nrec);
            dsp_case =DAI_ID_VOIP;
        }else  if(is_loop_active(param_res->usecase)) {
            param_id = get_loopback_param(param_res->out_devices, param_res->in_devices);
            dsp_case =DAI_ID_LOOP;
            volume=0;
        }

        if(param_id>=PROFILE_MODE_MAX){
            LOG_W("get_voice_param failed");
            return -1;
        }

        offset=(uint8_t)get_audio_param_offset(dev_ctl->audio_param,SND_AUDIO_PARAM_DSP_VBC_PROFILE_DSP,param_id);
        if(AUDIO_PARAM_INVALID_8BIT_OFFSET!=offset){
            ret = set_vbc_param(dev_ctl, offset, param_id,  dsp_case);
        }

        ret = set_voice_dg_param(dev_ctl,param_id,volume);

        ret = set_dsp_param(dev_ctl, param_id, dsp_case,volume);

        if(0==(param_res->out_devices&AUDIO_DEVICE_OUT_ALL_SCO)){
            ret = set_codec_playback_param(dev_ctl, param_id, param_res->out_devices, volume);

            if(is_call_active(param_res->usecase)){
                audio_devices_t in_device=AUDIO_DEVICE_NONE;

                mic_select = get_voice_mic_select(dev_ctl->audio_param,  param_id);

                if((mic_select&(SPRD_MAIN_MIC_PATH_SWITCH|SPRD_BACK_MIC_PATH_SWITCH|SPRD_HEADSET_MIC_PATH_SWITCH))==0){
                    mic_select = get_default_voice_mic_select(param_res->out_devices);
                }

                if(mic_select& SPRD_MAIN_MIC_PATH_SWITCH){
                    in_device|=AUDIO_DEVICE_IN_BUILTIN_MIC;
                }

                if(mic_select & SPRD_BACK_MIC_PATH_SWITCH){
                    in_device|=AUDIO_DEVICE_IN_BACK_MIC;
                }

                if(mic_select & SPRD_HEADSET_MIC_PATH_SWITCH){
                    in_device|=AUDIO_DEVICE_IN_WIRED_HEADSET;
                }

                if(AUDIO_DEVICE_NONE!=in_device){
                    do_switch_in_devices(dev_ctl,AUDIO_HW_APP_CALL,in_device);
                }

            }else if(is_voip_active(param_res->usecase) ||is_loop_active(param_res->usecase)){
                mic_select = get_normal_mic_select(param_res->in_devices);
            }

            ret = set_codec_record_param(dev_ctl, param_id, mic_select);
        }

        if ((is_call_active(param_res->usecase)) && (UC_MM_RECORD&param_res->usecase)){
            param_id = get_record_param(param_res->in_devices);
            if(param_id>=PROFILE_MODE_MAX){
                LOG_W("get_record_param failed");
                return -1;
            }
            dsp_case=DAI_ID_NORMAL_OUTDSP_CAPTURE;
            offset=(uint8_t)get_audio_param_offset(dev_ctl->audio_param,SND_AUDIO_PARAM_DSP_VBC_PROFILE_DSP,param_id);
            if(AUDIO_PARAM_INVALID_8BIT_OFFSET!=offset){
                ret = set_vbc_param(dev_ctl, offset, param_id,  dsp_case);
            }

            if(0==(param_res->out_devices&AUDIO_DEVICE_OUT_ALL_SCO)){
                mic_select = get_normal_mic_select(param_res->in_devices);
                ret = set_codec_record_param(dev_ctl, param_id, mic_select);
            }

            if (param_id != param_res->cur_record_dg_id) {
                ret = set_record_dg_param(dev_ctl, param_id, 0);
                param_res->cur_record_dg_id =param_id;
            }
        }

        if (is_playback_active(param_res->usecase)){
            param_id = get_playback_param(param_res->out_devices);
            if(param_id>=PROFILE_MODE_MAX){
                LOG_W("get_playback_param failed");
            }else{
                ret = set_play_dg_param(dev_ctl, param_id, 0);
                dsp_case=DAI_ID_NORMAL_OUTDSP_PLAYBACK;
                offset=(uint8_t)get_audio_param_offset(dev_ctl->audio_param,SND_AUDIO_PARAM_DSP_VBC_PROFILE_DSP,param_id);
                if(AUDIO_PARAM_INVALID_8BIT_OFFSET!=offset){
                    ret = set_vbc_param(dev_ctl, offset, param_id, dsp_case);
                }
            }
        }
    }else{
        if(is_fm_active(param_res->usecase)){
            volume=param_res->fm_volume;
            param_id = get_fm_param(param_res->out_devices);
            if(param_id>=PROFILE_MODE_MAX){
                LOG_W("get_fm_param failed");
                return -1;
            }

            dsp_case=DAI_ID_FM;

            offset=(uint8_t)get_audio_param_offset(dev_ctl->audio_param,SND_AUDIO_PARAM_DSP_VBC_PROFILE_DSP,param_id);
            if(AUDIO_PARAM_INVALID_8BIT_OFFSET!=offset){
                ret = set_vbc_param(dev_ctl, offset, param_id,  dsp_case);
            }

            ret = set_codec_playback_param(dev_ctl, param_id, param_res->out_devices,0);
            ret = set_fm_dg_param(dev_ctl,param_id,param_res->fm_volume);
        }else if(is_playback_active(param_res->usecase) ) {
            param_id = get_playback_param(param_res->out_devices);
            if(param_id>=PROFILE_MODE_MAX){
                LOG_W("get_playback_param failed");
                return -1;
            }
            ret = set_play_dg_param(dev_ctl, param_id, 0);
            dsp_case=DAI_ID_NORMAL_OUTDSP_PLAYBACK;
            offset=(uint8_t)get_audio_param_offset(dev_ctl->audio_param,SND_AUDIO_PARAM_DSP_VBC_PROFILE_DSP,param_id);
            if(AUDIO_PARAM_INVALID_8BIT_OFFSET!=offset){
                ret = set_vbc_param(dev_ctl, offset, param_id,  dsp_case);
            }
            ret = set_codec_playback_param(dev_ctl, param_id, param_res->out_devices, 0);
        }

        if(is_record_active(param_res->usecase)) {
            param_id = get_record_param(param_res->in_devices);
            if(param_id>=PROFILE_MODE_MAX){
                LOG_W("get_record_param failed");
                return -1;
            }
            dsp_case=DAI_ID_NORMAL_OUTDSP_CAPTURE;
            offset=(uint8_t)get_audio_param_offset(dev_ctl->audio_param,SND_AUDIO_PARAM_DSP_VBC_PROFILE_DSP,param_id);
            if(AUDIO_PARAM_INVALID_8BIT_OFFSET!=offset){
                ret = set_vbc_param(dev_ctl, offset, param_id,  dsp_case);
            }
            mic_select = get_normal_mic_select(param_res->in_devices);
            ret = set_codec_record_param(dev_ctl, param_id, mic_select);
            if (param_id != param_res->cur_record_dg_id) {
                ret = set_record_dg_param(dev_ctl, param_id, 0);
                param_res->cur_record_dg_id =param_id;
            }
        }
    }
    return 0;
}

static int audio_param_state_process(struct audio_param_res  *param_res,int usecase_off){
    if((0==(param_res->usecase&(UC_NORMAL_PLAYBACK|UC_DEEP_BUFFER_PLAYBACK|UC_OFFLOAD_PLAYBACK)))
        &&(usecase_off&((UC_NORMAL_PLAYBACK|UC_DEEP_BUFFER_PLAYBACK|UC_OFFLOAD_PLAYBACK))))
        {
        clear_playback_param_state(param_res);
    }

    if(((param_res->usecase&UC_FM)==0) &&(usecase_off&UC_FM)){
        clear_fm_param_state(param_res);
    }

    if(((param_res->usecase&(UC_VOIP|UC_CALL|UC_LOOP|UC_BT_VOIP))==0)
        &&(usecase_off&((UC_VOIP|UC_CALL|UC_LOOP|UC_BT_VOIP)))){
        clear_voice_param_state(param_res);
    }

    if(((param_res->usecase&UC_MM_RECORD)==0)
        &&(usecase_off&UC_MM_RECORD)){
        clear_record_param_state(param_res);
    }

    if(0==param_res->usecase){
        param_res->out_devices=0;
        param_res->in_devices=0;

        param_res->cur_codec_p_id = 0xff ;
        param_res->cur_codec_p_volume = 0xff ;
        param_res->cur_codec_c_id = 0xff ;
    }
    return 0;
}

static bool param_state_check(struct audio_control *dev_ctl,int type, void *param_change){
    bool update=false;
    struct audio_param_res  *param_res = NULL;
    param_res = &dev_ctl->param_res;

    switch(type){
        case PARAM_OUTDEVICES_CHANGE:{
                audio_devices_t *devices=(audio_devices_t *)param_change;
                if(param_res->out_devices != *devices) {
                    param_res->out_devices = *devices;
                    update=true;
                }
            }
            break;
        case PARAM_INDEVICES_CHANGE:{
                audio_devices_t *devices=(audio_devices_t *)param_change;
                if(param_res->in_devices != *devices) {
                    param_res->in_devices = *devices;
                    update=true;
                }
            }
            break;
        case PARAM_BT_NREC_CHANGE:{
                struct dev_bluetooth_t * bt_infor=(struct dev_bluetooth_t * )param_change;
                if(param_res->bt_infor.bluetooth_nrec != bt_infor->bluetooth_nrec) {
                    param_res->bt_infor.bluetooth_nrec = bt_infor->bluetooth_nrec;
                    update=true;
                }
            }
            break;
        case PARAM_NET_CHANGE:{
                struct voice_net_t * net_infor=(struct voice_net_t*)param_change;
                if((param_res->net_mode != net_infor->net_mode) || (param_res->wb_mode !=
                net_infor->wb_mode)){
                    param_res->net_mode = net_infor->net_mode;
                    param_res->wb_mode=net_infor->wb_mode;
                    update=true;
                }
            }
            break;
        case PARAM_USECASE_DEVICES_CHANGE:
                if((param_res->out_devices!=dev_ctl->out_devices)||
                    (param_res->in_devices!=dev_ctl->in_devices)){
                    update=true;
                }
                param_res->out_devices=dev_ctl->out_devices;
                param_res->in_devices=dev_ctl->in_devices;
        case PARAM_USECASE_CHANGE:{
                int usecase_off=0;
                if(param_res->usecase != dev_ctl->usecase){
                    usecase_off=param_res->usecase &(~dev_ctl->usecase);
                    param_res->usecase = dev_ctl->usecase;
                    audio_param_state_process(&dev_ctl->param_res,usecase_off);
                    if(0==param_res->usecase){
                        update=false;
                    }else{
                        update=true;
                    }
                }
            }
            break;
        case PARAM_VOICE_VOLUME_CHANGE:{
                int *volume=(int *)param_change;
                if(param_res->voice_volume != *volume) {
                    param_res->voice_volume=*volume;
                    update=true;
                }
            }
            break;
        case PARAM_FM_VOLUME_CHANGE:{
                int *volume=(int *)param_change;
                if(param_res->fm_volume != *volume) {
                    param_res->fm_volume=*volume;
                    update=true;
                }
            }
            break;
        case PARAM_AUDIOTESTER_CHANGE:
            /* clear playback param  */
            param_res->cur_playback_dg_id=0xff;
            param_res->cur_vbc_playback_id=0xff;

            /* clear fm param  */
            param_res->cur_fm_dg_id=0xff;
            param_res->cur_vbc_id=0xff;

            /* clear voice param  */
            param_res->cur_voice_dg_id=0xff;
            param_res->cur_dsp_id=0xff;

            /* clear record param  */
            param_res->cur_record_dg_id=0xff;

            /* clear codec param  */
            param_res->cur_codec_p_id=0xff;
            param_res->cur_codec_c_id=0xff;
            param_res->cur_codec_p_volume=0xff;
            update=true;
            break;
        default:
            LOG_W("param_state_check type:%d error",type);
            break;
    }

    return update;
}

int set_audioparam_unlock(struct audio_control *dev_ctl,int type, void *param_change,int force){
    bool update=false;
    struct audio_param_res  *param_res = NULL;
    param_res = &dev_ctl->param_res;
    int ret=-1;
    update=param_state_check(dev_ctl,type,param_change);

    if((true==update)||(true==force)){
        ret = select_audio_param_unlock(dev_ctl,param_res);
    }
    return ret;
}

int set_audioparam(struct audio_control *dev_ctl,int type, void *param_change,int force){
    int ret=-1;
    pthread_mutex_lock(&dev_ctl->lock);
    ret=set_audioparam_unlock(dev_ctl,type, param_change,force);
    pthread_mutex_unlock(&dev_ctl->lock);
    return ret;
}

int free_audio_param(AUDIO_PARAM_T * param){
    int profile=0;
    disconnect_audiotester_process(&(param->tunning));
    for(profile=0;profile<SND_AUDIO_PARAM_PROFILE_MAX;profile++){
        if(param->param[profile].data!=NULL){
            free(param->param[profile].data);
            param->param[profile].data=NULL;
        }
        param->param[profile].num_mode=0;
        param->param[profile].param_struct_size=0;
        if(param->param[profile].xml.param_root!=NULL){
            release_xml_handle(&(param->param[profile].xml));
        }
    }
    return 0;
}

int init_audio_param( struct tiny_audio_device *adev)
{
    int ret = 0;
    AUDIO_PARAM_T  *audio_param=&(adev->audio_param);
    memset(audio_param,0,sizeof(AUDIO_PARAM_T));
    audio_param->agdsp_ctl=adev->dev_ctl->agdsp_ctl;
    audio_param->dev_ctl=adev->dev_ctl;
    audio_param->config=&adev->dev_ctl->config;
    audio_param->audio_param_update=false;
    int profile=0;

    /*audio struct volume*/
    adev->dev_ctl->dsp_volume_ctl=mixer_get_ctl_by_name(adev->dev_ctl->mixer, "VBC_VOLUME");

    /* dsp vbc */
    audio_param->update_mixer[SND_AUDIO_PARAM_DSP_VBC_PROFILE_DSP]=
        mixer_get_ctl_by_name(adev->dev_ctl->mixer, VBC_DSP_PROFILE_UPDATE);

    audio_param->select_mixer[SND_AUDIO_PARAM_DSP_VBC_PROFILE_DSP]=
        mixer_get_ctl_by_name(adev->dev_ctl->mixer, VBC_DSP_PROFILE_SELECT);

    /* audio structure  */
    audio_param->update_mixer[SND_AUDIO_PARAM_AUDIO_STRUCTURE_PROFILE]=
        mixer_get_ctl_by_name(adev->dev_ctl->mixer, VBC_EQ_PROFILE_UPDATE);

    audio_param->select_mixer[SND_AUDIO_PARAM_AUDIO_STRUCTURE_PROFILE]=
        mixer_get_ctl_by_name(adev->dev_ctl->mixer, VBC_EQ_PROFILE_SELECT);

    /* NXP  */
    audio_param->update_mixer[SND_AUDIO_PARAM_NXP_PROFILE]=
        mixer_get_ctl_by_name(adev->dev_ctl->mixer, VBC_NXP_PROFILE_UPDATE);

    audio_param->select_mixer[SND_AUDIO_PARAM_NXP_PROFILE]=
        mixer_get_ctl_by_name(adev->dev_ctl->mixer, VBC_NXP_PROFILE_SELECT);

    /* codec */
    if(adev->dev_ctl->codec_type==AUD_REALTEK_CODEC_TYPE){
        audio_param->update_mixer[SND_AUDIO_PARAM_CODEC_PROFILE]=
            mixer_get_ctl_by_name(adev->dev_ctl->mixer, VBC_CODEC_PROFILE_UPDATE);

        audio_param->select_mixer[SND_AUDIO_PARAM_CODEC_PROFILE]=
            mixer_get_ctl_by_name(adev->dev_ctl->mixer, VBC_CODEC_PROFILE_SELECT);
    }

    ret= init_sprd_audio_param(audio_param,check_updata_audioparam(audio_param));

    for(profile=0;profile<SND_AUDIO_PARAM_PROFILE_MAX;profile++){
        release_xml_handle(&(audio_param->param[profile].xml));
    }
    return ret;

}

struct audio_control *init_audio_control(struct tiny_audio_device *adev)
{
    int card_num, ret;
    int try_count=0;
    struct audio_control *control;
    struct mixer_ctl * codec_infor=NULL;
    int i=0;
    control = (struct audio_control *) malloc(sizeof(struct audio_control));
    if (!control) {
        LOG_E("init_audio_control malloc audio route failed");
        goto err_calloc;
    }

    memset(control,0,sizeof(struct audio_control));

    LOG_I("init_audio_control");
    pthread_mutex_init(&control->lock, NULL);
    pthread_mutex_init(&control->cmd_lock, NULL);

    control->adev = adev;
    control->usecase = UC_UNKNOWN;
    control->dg_gain.dev_gain=NULL;
    control->dg_gain.gain_size=0;
    control->fm_volume=0.0;
    control->voice_volume=0;
    control->music_volume=0;
    control->fm_mute = false;

    control->vbc_dump.is_exit=true;
    ret= parse_audio_config(control);

try_open:
    if(NULL!=control->config.card_name){
        card_num = get_snd_card_number(control->config.card_name);
    }else{
        card_num = get_snd_card_number(CARD_SPRDPHONE);
    }

    control->mixer = mixer_open(card_num);
    if (!control->mixer) {
        LOG_E("init_audio_control Unable to open the mixer, aborting.");
        try_count++;
        if(try_count<=3){
            sleep(1);
            goto try_open;
        }
        goto err_mixer_open;
    }
    control->dg_gain.mixer=control->mixer;
    adev->mixer=control->mixer;

    ret = parse_audio_route(control);

    ret = parse_audio_pcm_config(&(control->pcm_handle));

    init_codec_mute(control->mixer,&control->mute,&control->config.mute);
    init_mdg_mute(control->mixer,&control->mute,&control->config.mute);

    control->cards.s_tinycard = card_num;

    ret = stream_routing_manager_create(control);
    if (ret != 0) {
        LOG_E("init_audio_control stream_routing_manager_create failed ");
        goto err_mixer_open;
    }

    control->voice_ul_mute_ctl = mixer_get_ctl_by_name(control->mixer, VBC_UL_MUTE);
    control->voice_dl_mute_ctl = mixer_get_ctl_by_name(control->mixer, VBC_DL_MUTE);
    control->bt_src = mixer_get_ctl_by_name(control->mixer, VBC_BT_SRC);
    codec_infor=mixer_get_ctl_by_name(control->mixer, CODEC_INFOR_CTL);
    control->codec_type=-1;
    if(NULL!=codec_infor){
        const char *chip_str=NULL;
        chip_str=tinymix_get_enum(codec_infor);
        if(NULL!=chip_str){
            for(i=0;i<AUD_CODEC_TYPE_MAX;i++){
                if(strncmp(chip_str,audio_codec_chip_name[i],strlen(audio_codec_chip_name[i]))==0){
                    control->codec_type=i;
                    break;
                }
            }
        }
    }
    control->param_res.codec_type=control->codec_type;
    control->param_res.bt_infor.bluetooth_nrec=false;
    control->param_res.bt_infor.samplerate=8000;

    if(AUD_SPRD_2731S_CODEC_TYPE == control->codec_type){
        ret = init_sprd_codec_mixer(&(control->codec),control->mixer);
    }

    control->agdsp_ctl = (dsp_control_t*)dsp_ctrl_open(control);
    control->audio_param=&adev->audio_param;

    audio_param_state_process(&control->param_res,UC_NORMAL_PLAYBACK|UC_DEEP_BUFFER_PLAYBACK
        |UC_OFFLOAD_PLAYBACK|UC_FM|UC_VOIP|UC_BT_VOIP|UC_CALL|UC_LOOP|UC_MM_RECORD);

    return control;

err_mixer_open:
    free(control);
    control = NULL;
err_calloc:
    return NULL;
}


void free_audio_control(struct audio_control *control)
{
    if (control->mixer) {
        mixer_close(control->mixer);
        control->mixer = NULL;
    }
    free_private_control(&(control->route.priv_ctl));
    free_device_route(&(control->route.devices_route));
}

#ifdef __cplusplus
}
#endif

