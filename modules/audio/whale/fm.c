#define LOG_TAG "audio_hw_fm"
#define LOG_NDEBUG 0

#include <errno.h>
#include <math.h>
#include "stdbool.h"
#include <cutils/log.h>

#include "audio_hw.h"
#include "audio_control.h"

#include <tinyalsa/asoundlib.h>

#define PORT_FM 4

static int do_fm_out_standby(struct tiny_audio_device *adev,void *out,AUDIO_HW_APP_T audio_app_type){
    struct tiny_stream_out* fm_out=(struct tiny_stream_out*)out;
    struct listnode *item=NULL;
    LOG_I("do_fm_out_standby");

    pthread_mutex_lock(&fm_out->lock);
    if(fm_out->pcm != NULL) {
        pcm_close(fm_out->pcm);
        fm_out->pcm = NULL;
    }

    if(UC_FM & adev->dev_ctl->usecase){
        set_usecase(adev->dev_ctl, UC_FM, false);
        switch_fm_control(adev->dev_ctl, false);
    }
    fm_out->standby=true;
    adev->stream_status &=~(1<<fm_out->audio_app_type);
    if(false==is_usecase(adev->dev_ctl,UC_CALL)){
        switch_vbc_iis_route(adev->dev_ctl,UC_FM,false);
    }
    pthread_mutex_unlock(&fm_out->lock);
    free(fm_out);
    return 0;
}

int start_fm(struct audio_control *ctl)
{
    struct tiny_audio_device *adev=(struct tiny_audio_device *)ctl->adev;
    struct pcm *pcm_fm_dl=NULL;
    struct tiny_stream_out *fm_out=NULL;
    LOG_I("start_fm: devices:%x rate:%d",ctl->pcm_handle.play[AUD_PCM_DIGITAL_FM].stop_threshold,ctl->pcm_handle.play[AUD_PCM_DIGITAL_FM].rate);

    if(false==is_usecase(adev->dev_ctl,UC_CALL)){
        switch_vbc_iis_route(adev->dev_ctl,UC_FM,true);
    }

/*
    if(UC_FM & ctl->usecase){
        LOG_W("fm aready open");
        return -1;
    }
*/
    //ctl->pcm_handle.play[AUD_PCM_DIGITAL_FM].stop_threshold=((-1/2) -1);
    dsp_sleep_ctrl(&adev->agdsp_ctl,true);
    pcm_fm_dl=pcm_open(0, ctl->pcm_handle.playback_devices[AUD_PCM_DIGITAL_FM], PCM_OUT,
                                        &(ctl->pcm_handle.play[AUD_PCM_DIGITAL_FM]));
    if (!pcm_is_ready(pcm_fm_dl)) {
        ALOGE("%s: cannot open pcm_fm_dl : %s", __func__,
              pcm_get_error(pcm_fm_dl));
        goto err;
    } else {
        if(pcm_start(pcm_fm_dl) != 0) {
            ALOGE("%s:pcm_start pcm_fm_dl start unsucessfully: %s", __func__,
                  pcm_get_error(pcm_fm_dl));
            goto err;
            
        }
    }

    fm_out=(struct tiny_stream_out *)calloc(1,
                                sizeof(struct tiny_stream_out));

    fm_out->pcm=pcm_fm_dl;
    fm_out->dev=adev;
    fm_out->standby=false;
    fm_out->audio_app_type=AUDIO_HW_APP_FM;
    adev->stream_status |=(1<<fm_out->audio_app_type);
    audio_add_output(adev,fm_out,AUDIO_HW_APP_FM,do_fm_out_standby);
    select_devices_new(ctl,adev->out_devices,0);
    set_usecase(ctl, UC_FM, true);
    switch_fm_control(ctl, true);
    LOG_I("start_fm success");
    return 0;

err:
    dsp_sleep_ctrl(&adev->agdsp_ctl,false);
    adev->stream_status &=~(1<<AUDIO_HW_APP_FM);
    if(NULL!=pcm_fm_dl){
        pcm_close(pcm_fm_dl);
    }

    if(NULL!=fm_out){
        free(fm_out);
    }

    if(false==is_usecase(adev->dev_ctl,UC_CALL)){
        switch_vbc_iis_route(adev->dev_ctl,UC_FM,false);
    }
    LOG_E("start fm failed, devices:%d",ctl->pcm_handle.playback_devices[AUD_PCM_DIGITAL_FM]);
    return -1;
}

int stop_fm(struct audio_control *ctl)
{
    struct tiny_audio_device *adev=(struct tiny_audio_device *)ctl->adev;
    LOG_I("stop_fm");
    force_out_standby(adev,AUDIO_HW_APP_FM);
    return 0;
}
