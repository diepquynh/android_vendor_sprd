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

#define LOG_TAG "audio_hw_fm"

#include <errno.h>
#include <math.h>
#include "stdbool.h"
#include <cutils/log.h>

#include "audio_hw.h"
#include "audio_control.h"

#include <tinyalsa/asoundlib.h>

#define PORT_FM 4


/*
 Extern function declaration
*/
extern struct tiny_stream_out * get_output_stream_unlock(struct tiny_audio_device *adev,AUDIO_HW_APP_T audio_app_type);


static int do_fm_out_standby(struct tiny_audio_device *adev,void *out,AUDIO_HW_APP_T audio_app_type){
    struct tiny_stream_out* fm_out=(struct tiny_stream_out*)out;
    LOG_I("do_fm_out_standby");

    if(fm_out->pcm != NULL) {
        pcm_close(fm_out->pcm);
        fm_out->pcm = NULL;
    }
    fm_out->standby=true;
    set_usecase(adev->dev_ctl, UC_FM, false);
    set_audioparam(adev->dev_ctl,PARAM_USECASE_CHANGE,NULL,false);
    return 0;
}

int fm_open(void *dev){
    struct tiny_audio_device *adev=(struct tiny_audio_device *)dev;
    struct tiny_stream_out *fm_out=NULL;

    fm_out=(struct tiny_stream_out *)calloc(1,
                                sizeof(struct tiny_stream_out));
    fm_out->pcm=NULL;
    fm_out->dev=adev;
    fm_out->standby=true;
    fm_out->audio_app_type=AUDIO_HW_APP_FM;
    fm_out->standby_fun = do_fm_out_standby;

    if (pthread_mutex_init(&(fm_out->lock), NULL) != 0) {
        LOG_E("Failed pthread_mutex_init fm_out->lock,errno:%u,%s",
              errno, strerror(errno));
        goto err;
    }

    audio_add_output(adev,fm_out);

    return 0;

err:

    if(NULL!=fm_out){
        free(fm_out);
    }
    return -1;
}


int start_fm(struct audio_control *ctl)
{
    struct tiny_audio_device *adev=(struct tiny_audio_device *)ctl->adev;
    struct tiny_stream_out *fm_out=NULL;
    int ret = 0;

    pthread_mutex_lock(&adev->lock);

    if(is_usecase(adev->dev_ctl, UC_FM)){
        pthread_mutex_unlock(&adev->lock);
        return 0;
    }

    fm_out = get_output_stream_unlock(adev,AUDIO_HW_APP_FM);
    if(NULL==fm_out){
        pthread_mutex_unlock(&adev->lock);
        return -1;
    }

    LOG_I("start_fm: devices:%x rate:%d",ctl->pcm_handle.play[AUD_PCM_DIGITAL_FM].stop_threshold,ctl->pcm_handle.play[AUD_PCM_DIGITAL_FM].rate);
    ret = set_usecase(ctl, UC_FM, true);
    if(ret < 0){
        LOG_E("set usecase failed");
        pthread_mutex_unlock(&adev->lock);
        return -1;
    }

    pthread_mutex_lock(&fm_out->lock);
    fm_out->pcm=pcm_open(adev->dev_ctl->cards.s_tinycard, ctl->pcm_handle.playback_devices[AUD_PCM_DIGITAL_FM], PCM_OUT,
                                        &(ctl->pcm_handle.play[AUD_PCM_DIGITAL_FM]));
    if (!pcm_is_ready(fm_out->pcm)) {
        ALOGE("%s: cannot open pcm_fm_dl : %s", __func__, pcm_get_error(fm_out->pcm));
        goto err;
    } 
    if(pcm_start(fm_out->pcm) != 0) {
        ALOGE("%s:pcm_start pcm_fm_dl start unsucessfully: %s", __func__, pcm_get_error(fm_out->pcm));
        goto err;
    }

    fm_out->standby=false;
    pthread_mutex_unlock(&fm_out->lock);

    select_devices_new(ctl,fm_out->audio_app_type,ctl->out_devices,false,false,true);
    set_audioparam(adev->dev_ctl,PARAM_USECASE_DEVICES_CHANGE,NULL,false);
    LOG_I("start_fm success");
    pthread_mutex_unlock(&adev->lock);
    return 0;

err:

    if(NULL!=fm_out->pcm){
        pcm_close(fm_out->pcm);
        fm_out->pcm=NULL;
    }
    fm_out->standby=true;

    pthread_mutex_unlock(&fm_out->lock);
    set_usecase(ctl, UC_FM, false);
    set_audioparam(adev->dev_ctl,PARAM_USECASE_CHANGE,NULL,false);
    LOG_E("start fm failed, devices:%d",ctl->pcm_handle.playback_devices[AUD_PCM_DIGITAL_FM]);
    pthread_mutex_unlock(&adev->lock);
    return -1;
}

int stop_fm(struct audio_control *ctl)
{
    struct tiny_stream_out * out = NULL;
    struct tiny_audio_device *adev=(struct tiny_audio_device *)ctl->adev;
    LOG_I("stop_fm");
    pthread_mutex_lock(&adev->lock);
    out = get_output_stream_unlock(adev,AUDIO_HW_APP_FM);
    if(out) {
        do_output_standby(out);
    }else{
        LOG_W("stop_fm failed");
    }
    pthread_mutex_unlock(&adev->lock);
    return 0;
}
