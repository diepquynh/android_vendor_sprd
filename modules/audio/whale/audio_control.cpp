#define LOG_TAG "audio_hw_control"
#define LOG_NDEBUG 0

#include "audio_control.h"
#include "audio_hw.h"
#include <tinyxml.h>
#include <cutils/log.h>
#include "fcntl.h"
#include "audio_param/audio_param.h"
#ifdef __cplusplus
extern "C" {
#endif
extern int set_dsp_volume(struct audio_control *ctl,int volume);

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

int close_all_control(struct audio_control *actl)
{
    int ret=0;
    struct audio_route * route = (struct audio_route *)&actl->route;
    pthread_mutex_lock(&actl->lock);
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

/*
    if(NULL !=route->pre_in_ctl){
        LOG_I("IN DEVICE :%s Route ON",route->pre_in_ctl->name);
        ret=apply_mixer_control(&(route->pre_in_ctl->ctl_on), "Route ON");
    }
*/
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
/*
    if(NULL !=route->pre_out_ctl){
        LOG_I("OUT DEVICE :%s Route ON",route->pre_out_ctl->name);
        ret=apply_mixer_control(&(route->pre_out_ctl->ctl_on), "Route ON");
    }
*/
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
            ret=mixer_ctl_set_value(mixer_ctl_off->ctl, 0, mixer_ctl_off->value);
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
    LOG_I("switch_device_route device=0x%x,in_device:%d", device,in_device);
    const char *cur_dev_name;
    struct device_route *cur = NULL;
    bool device_change=false;

    if((UC_LOOP |UC_CALL|UC_VOIP) & ctl->usecase){
        cur = get_device_route_withdevice(&(ctl->route.devices_route_call), device);
    }else{
        cur = get_device_route_withdevice(&(ctl->route.devices_route), device);
    }

    if(NULL == cur){
        LOG_I("switch_device_route not fined the devices: 0x%x",device);
        return -1;
    }
    
    if(in_device) {
        ctl->adev->in_devices=device;

        if(cur == ctl->route.pre_in_ctl){
            LOG_I("switch_device_route set the same devices");
            return 0;
        }
        if(NULL !=ctl->route.pre_in_ctl){
            LOG_I("IN  DEVICES %s Route OFF",ctl->route.pre_in_ctl->name);
            apply_mixer_control(&(ctl->route.pre_in_ctl->ctl_off), "Route OFF");
        }

       LOG_I("IN  DEVICES %s Route ON",cur->name);
        apply_mixer_control(&cur->ctl_on, "Route ON");

        ctl->route.pre_in_ctl=cur;
        ctl->adev->in_devices=device;

        device_change=true;
    }
    else {
        ctl->adev->out_devices=device;

        if(cur == ctl->route.pre_out_ctl){
            LOG_I("switch_device_route set the same devices");
            return 0;
        }

        if(NULL !=ctl->route.pre_out_ctl){
            LOG_I("OUT  DEVICES %s Route OFF",ctl->route.pre_out_ctl->name);
            apply_mixer_control(&(ctl->route.pre_out_ctl->ctl_off), "Route OFF");
        }
            LOG_I("OUT DEVICES %s Route ON",cur->name);
            apply_mixer_control(&cur->ctl_on, "Route ON");

        ctl->route.pre_out_ctl=cur;
        ctl->adev->out_devices=device;

        device_change=true;
    }

    if(true==device_change){
       select_audio_param(ctl->adev,false);
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

int set_vdg_gain(struct device_usecase_gain *dg_gain, int param_id, int volume){
    struct device_gain * gain=NULL;

    LOG_I("set_vdg_gain size:%d id:%d param_id:%dvolume:%d",dg_gain->gain_size,param_id,param_id,volume);
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

static int do_switch_out_devices(struct audio_control *actl,audio_devices_t device)
{
    return switch_device_route(actl,device,false);
}

static int do_switch_in_devices(struct audio_control *actl,audio_devices_t device)
{
    return switch_device_route(actl,device,true);
}

static int do_switch_devices(struct audio_control *actl)
{
    int aud_device_id = -1;
    unsigned int i = 0;
    pthread_mutex_lock(&actl->adev->lock);
    do_switch_out_devices(actl,actl->adev->out_devices);
    do_switch_in_devices(actl,actl->adev->in_devices);
    pthread_mutex_unlock(&actl->adev->lock);
    return 0;
}

static void *stream_routing_thread_entry(void *param)
{
    struct audio_control *actl = (struct audio_control *)param;
    pthread_attr_t attr;
    struct sched_param m_param;
    int newprio = 39;

    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    pthread_attr_getschedparam(&attr, &m_param);
    m_param.sched_priority = newprio;
    pthread_attr_setschedparam(&attr, &m_param);

    while(!actl->routing_mgr.is_exit) {
        LOG_I("stream_routing_thread looping now...");
        sem_wait(&actl->routing_mgr.device_switch_sem);
        do_switch_devices(actl);
        select_audio_param(actl->adev,false);
        LOG_I("stream_routing_thread looping done.");
    }
    LOG_W("stream_routing_thread_entry exit!!!");
    return 0;
}

int switch_fm_control(struct audio_control *ctl, bool on)
{
    return 0;
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
    for (unsigned int i = 0; i < pri->size; i++) {
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
    for (unsigned int i = 0; i < dsploop_ctl->size; i++) {
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

int switch_vbc_iis_route(struct audio_control *ctl,USECASE uc,bool on){
    struct device_route *cur = NULL;
    int ret=0;
    pthread_mutex_lock(&ctl->lock);
    switch(uc){
        case UC_CALL:
            cur = get_device_route_withname(&(ctl->route.vbc_iis), "voice_playback");
            break;
        case UC_VOIP:
            cur = get_device_route_withname(&(ctl->route.vbc_iis), "voip");
            break;
        case UC_FM:
            cur = get_device_route_withname(&(ctl->route.vbc_iis), "fm");
            break;
        case UC_MUSIC:
            cur = get_device_route_withname(&(ctl->route.vbc_iis), "playback");
            break;
        case UC_MM_RECORD:
            cur = get_device_route_withname(&(ctl->route.vbc_iis), "record");
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
    pthread_mutex_unlock(&ctl->lock);
    return 0;
}

int set_fm_volume(struct audio_control *ctl, int volume)
{
    struct device_gain * gain=NULL;
    struct device_usecase_gain *dg_gain=NULL;
    int profile_id=0;
    dg_gain=&(ctl->adev->dev_ctl->dg_gain);

    if(AUDIO_DEVICE_OUT_SPEAKER & ctl->adev->out_devices){
        profile_id=PROFILE_MODE_MUSIC_Handsfree_FM;
    }else{
        profile_id=PROFILE_MODE_MUSIC_Headset_FM;
    }

    for(int i=0;i<dg_gain->gain_size;i++){
        LOG_D("set_fm_volume:%s",dg_gain->dev_gain[i].name);
        gain=&(dg_gain->dev_gain[i]);
        if(gain->id==profile_id){
            LOG_I("apply_gain_control profile:%d",profile_id);
            for(int j=0;j<gain->ctl_size;j++){
                apply_gain_control(&(gain->ctl[j]),volume);
            }
            return 0;
        }
    }
    return 0;
}

void set_usecase(struct audio_control *actl, int usecase, bool on)
{
    LOG_I("set_usecase cur :0x%x usecase=0x%x  %s",actl->usecase, usecase, on ? "on" : "off");
    pthread_mutex_lock(&actl->lock);
    if (on) {
        actl->usecase |= usecase;
    } else {
        actl->usecase &= ~usecase;
    }
    pthread_mutex_unlock(&actl->lock);
}

bool is_usecase(struct audio_control *actl, int usecase)
{
    bool on=false;
    //LOG_V("get_usecase usecase=%x", usecase);
    pthread_mutex_lock(&actl->lock);
    if (actl->usecase & usecase) {
        on=true;
    } else {
        on=false;
    }
    pthread_mutex_unlock(&actl->lock);
    return on;
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

void select_devices(struct audio_control *actl)
{
    LOG_I("select_devices_signal starting... out_devices 0x%x in_devices 0x%x",
          actl->adev->out_devices, actl->adev->in_devices);
    sem_post(&actl->routing_mgr.device_switch_sem);
}

void select_devices_new(struct audio_control *actl,audio_devices_t device,int is_in)
{
    struct tiny_audio_device *adev=actl->adev;
    LOG_I("select_devices_new starting... devices 0x%x, is in %d", device, is_in);
    pthread_mutex_lock(&actl->lock);
    if(is_in) {
        adev->in_devices=device;
        do_switch_in_devices(actl,device);
    }
    else {
        adev->out_devices=device;
        do_switch_out_devices(actl,device);
    }
    pthread_mutex_unlock(&actl->lock);
}


#ifdef __cplusplus
}
#endif

