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

#include "audio_hw.h"
#include "audio_control.h"
#define LOG_TAG "audio_hw_voice"

/*
 Extern function declaration
*/
extern struct tiny_stream_out * get_output_stream_unlock(struct tiny_audio_device *adev,AUDIO_HW_APP_T audio_app_type);


/*
 Function implement
*/
static int do_voice_out_standby(struct tiny_audio_device *adev,void *out,AUDIO_HW_APP_T audio_app_type){
    struct tiny_stream_out *voice_out=(struct tiny_stream_out*)out;
    struct audio_control *actl=adev->dev_ctl;
    struct listnode *item=NULL;
    LOG_I("do_voice_out_standby:%p",voice_out);

    set_mdg_mute(adev->dev_ctl,UC_CALL,true);

    if(voice_out->pcm_modem_ul) {
        LOG_I("do_voice_out_standby close:%p",voice_out->pcm_modem_ul);
        pcm_close(voice_out->pcm_modem_ul);
        voice_out->pcm_modem_ul = NULL;
    }
    if(voice_out->pcm_modem_dl) {
        LOG_I("do_voice_out_standby close:%p",voice_out->pcm_modem_dl);
        pcm_close(voice_out->pcm_modem_dl);
        voice_out->pcm_modem_dl = NULL;
    }


    voice_out->standby=true;
    adev->stream_status &=~(1<<voice_out->audio_app_type);

    set_usecase(actl, UC_CALL, false);
    set_audioparam(adev->dev_ctl,PARAM_USECASE_CHANGE,NULL,false);
    LOG_I("do_voice_out_standby exit");
    return 0;
}

int voice_open(void *dev){
    struct tiny_audio_device *adev=(struct tiny_audio_device *)dev;
    struct tiny_stream_out *voice_out=NULL;

    voice_out=(struct tiny_stream_out *)calloc(1,
                                sizeof(struct tiny_stream_out));
    voice_out->pcm=NULL;
    voice_out->dev=adev;
    voice_out->standby=true;
    voice_out->audio_app_type=AUDIO_HW_APP_CALL;
    voice_out->standby_fun = do_voice_out_standby;

    voice_out->pcm_modem_ul=NULL;
    voice_out->pcm_modem_dl=NULL;

    if (pthread_mutex_init(&(voice_out->lock), NULL) != 0) {
        LOG_E("Failed pthread_mutex_init voice_out->lock,errno:%u,%s",
              errno, strerror(errno));
        goto err;
    }

    audio_add_output(adev,voice_out);

    return 0;

err:

    if(NULL!=voice_out){
        free(voice_out);
    }
    return -1;
}

int start_voice_call(struct audio_control *actl)
{
    struct tiny_audio_device *adev=(struct tiny_audio_device *)actl->adev;
    struct tiny_stream_out *voice_out=NULL;
    int in_devices = 0;
    int ret=0;

    LOG_I("start_voice_call %x %x rate:%d",actl->pcm_handle.playback_devices[AUD_PCM_MODEM_DL],
        actl->pcm_handle.play[AUD_PCM_MODEM_DL].stop_threshold,actl->pcm_handle.play[AUD_PCM_MODEM_DL].rate);

    if(true==is_usecase(adev->dev_ctl,UC_CALL)){
        LOG_W("start_voice_call aready open");
        return 0;
    }

    pthread_mutex_lock(&adev->lock);

    voice_out=get_output_stream_unlock(adev,AUDIO_HW_APP_CALL);
    if(NULL==voice_out){
        set_voice_status(adev,VOICE_INVALID_STATUS);
        pthread_mutex_unlock(&adev->lock);
        return -1;
    }

    set_voice_status(adev,VOICE_PRE_START_STATUS);
    force_out_standby_unlock(adev, AUDIO_HW_APP_VOIP);
    force_out_standby_unlock(adev, AUDIO_HW_APP_PRIMARY);
    force_out_standby_unlock(adev, AUDIO_HW_APP_DEEP_BUFFER);
    force_in_standby_unlock(adev,AUDIO_HW_APP_VOIP_RECORD);
    if(true==is_usecase(adev->dev_ctl,UC_BT_RECORD)){
        force_in_standby_unlock(adev, AUDIO_HW_APP_BT_RECORD);
    }

    adev->stream_status |=(1<<voice_out->audio_app_type);

    pthread_mutex_lock(&voice_out->lock);

    ret=set_usecase(actl, UC_CALL, true);
    if(ret<0){
        LOG_W("set_usecase failed");
        adev->stream_status &=~(1<<AUDIO_HW_APP_CALL);
        set_voice_status(adev,VOICE_INVALID_STATUS);
        pthread_mutex_unlock(&voice_out->lock);
        pthread_mutex_unlock(&adev->lock);
        return -1;
    }

    LOG_E("voice open card:%d",adev->dev_ctl->cards.s_tinycard);
    voice_out->pcm_modem_dl = pcm_open(adev->dev_ctl->cards.s_tinycard, actl->pcm_handle.playback_devices[AUD_PCM_MODEM_DL], PCM_OUT,
                                    &(actl->pcm_handle.play[AUD_PCM_MODEM_DL]));
    if (!pcm_is_ready(voice_out->pcm_modem_dl)) {
        LOG_E("voice:cannot open pcm_modem_dl : %s",
              pcm_get_error(voice_out->pcm_modem_dl));
        goto Err;
    }

    voice_out->pcm_modem_ul = pcm_open(adev->dev_ctl->cards.s_tinycard, actl->pcm_handle.playback_devices[AUD_PCM_MODEM_UL], PCM_IN,
                                    &(actl->pcm_handle.play[AUD_PCM_MODEM_UL]));
    if (!pcm_is_ready(voice_out->pcm_modem_ul)) {
        LOG_E("voice:cannot open pcm_modem_ul : %s",
              pcm_get_error(voice_out->pcm_modem_ul));
        goto Err;
    }

    select_devices_new(actl,voice_out->audio_app_type,actl->out_devices,false,false,true);

    if( 0 != pcm_start(voice_out->pcm_modem_dl)) {
        LOG_E("pcm dl start unsucessfully err:%s",pcm_get_error(voice_out->pcm_modem_dl));
        goto Err;
    }
    if( 0 != pcm_start(voice_out->pcm_modem_ul)) {
        LOG_E("pcm ul start unsucessfully err:%s",pcm_get_error(voice_out->pcm_modem_ul));
        goto Err;
    }
    voice_out->standby=false;

    set_mdg_mute(adev->dev_ctl,UC_CALL,false);

    pthread_mutex_unlock(&voice_out->lock);
    set_voice_status(adev,VOICE_START_STATUS);
    set_dsp_volume(adev->dev_ctl,adev->voice_volume);
    pthread_mutex_unlock(&adev->lock);

    set_audioparam(adev->dev_ctl,PARAM_USECASE_DEVICES_CHANGE,NULL,false);

    LOG_I("start_voice_call success,out:%p",voice_out);
    return 0;

Err:

    adev->stream_status &=~(1<<AUDIO_HW_APP_CALL);
    LOG_E("start_voice_call failed");
    if(NULL!=voice_out->pcm_modem_ul){
        pcm_close(voice_out->pcm_modem_ul);
        voice_out->pcm_modem_ul=NULL;
    }
    if(NULL!=voice_out->pcm_modem_dl){
        pcm_close(voice_out->pcm_modem_dl);
        voice_out->pcm_modem_dl=NULL;
    }

    voice_out->standby=true;
    set_voice_status(adev,VOICE_INVALID_STATUS);
    set_usecase(actl, UC_CALL, false);
    set_audioparam(adev->dev_ctl,PARAM_USECASE_CHANGE,NULL,false);
    pthread_mutex_unlock(&voice_out->lock);
    pthread_mutex_unlock(&adev->lock);
    return -1;
}

int stop_voice_call(struct audio_control *ctl)
{
    struct tiny_stream_out * out = NULL;
    struct tiny_audio_device *adev=(struct tiny_audio_device *)ctl->adev;

    pthread_mutex_lock(&adev->lock);
    out = get_output_stream_unlock(adev,AUDIO_HW_APP_CALL);
    if(out) {
        set_voice_status(adev,VOICE_PRE_STOP_STATUS);
        force_in_standby_unlock(adev,AUDIO_HW_APP_CALL_RECORD);
        do_output_standby(out);
        pthread_mutex_lock(&ctl->lock);
        ctl->param_res.net_mode=AUDIO_NET_UNKNOWN;
        pthread_mutex_unlock(&ctl->lock);
        set_voice_status(adev,VOICE_STOP_STATUS);
    }else{
        LOG_W("stop_voice_call failed");
    }
    pthread_mutex_unlock(&adev->lock);
    return 0;
}

bool is_bt_voice(void *dev){
    struct tiny_stream_out * out = NULL;
    struct tiny_audio_device *adev=(struct tiny_audio_device *)dev;
    bool ret=false;

    if(is_call_active_unlock(adev)){
        out = get_output_stream_unlock(adev,AUDIO_HW_APP_CALL);
        if(out) {
            pthread_mutex_lock(&out->lock);
            if(out->devices&AUDIO_DEVICE_OUT_ALL_SCO){
                ret=true;
            }
            pthread_mutex_unlock(&out->lock);
        }
    }
    return ret;
}
