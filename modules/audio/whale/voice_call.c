#include "audio_hw.h"
#include "audio_control.h"
#define LOG_TAG "audio_hw_voice"

static int do_voice_out_standby(struct tiny_audio_device *adev,void *out,AUDIO_HW_APP_T audio_app_type){
    struct tiny_stream_out *voice_out=(struct tiny_stream_out*)out;
    struct audio_control *actl=adev->dev_ctl;
    struct listnode *item=NULL;
    LOG_I("do_voice_out_standby:%p",voice_out);

    pthread_mutex_lock(&voice_out->lock);
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

    adev->audio_param.current_param=0;
    pthread_mutex_destroy(&(voice_out->lock));

    voice_out->standby=true;
    adev->stream_status &=~(1<<voice_out->audio_app_type);
    pthread_mutex_unlock(&voice_out->lock);
    free(voice_out);
    if(actl->usecase & UC_CALL) {
        set_usecase(actl, UC_CALL, false);
    }
    switch_vbc_iis_route(actl,UC_CALL,false);
    usleep(300*1000);
    LOG_I("do_voice_out_standby exit");
    return 0;
}

int start_voice_call(struct audio_control *actl)
{
    struct tiny_audio_device *adev=(struct tiny_audio_device *)actl->adev;
    struct tiny_stream_out *voice_out=NULL;
    struct pcm *pcm_modem_dl=NULL;
    struct pcm *pcm_modem_ul=NULL;
    LOG_I("start_voice_call %x %x rate:%d",actl->pcm_handle.playback_devices[AUD_PCM_MODEM_DL],
        actl->pcm_handle.play[AUD_PCM_MODEM_DL].stop_threshold,actl->pcm_handle.play[AUD_PCM_MODEM_DL].rate);

    if(true==is_usecase(adev->dev_ctl,UC_CALL)){
        LOG_W("start_voice_call aready open");
        return 0;
    }

    pthread_mutex_lock(&adev->lock);
    set_usecase(actl, UC_CALL, true);
     pthread_mutex_unlock(&adev->lock);
    force_out_standby(adev,AUDIO_HW_APP_PRIMARY);
    force_out_standby(adev,AUDIO_HW_APP_FAST);
    dsp_sleep_ctrl(&adev->agdsp_ctl,true);

    pthread_mutex_lock(&adev->lock);

    voice_out=(struct tiny_stream_out *)calloc(1,
                                sizeof(struct tiny_stream_out));

    voice_out->audio_app_type=AUDIO_HW_APP_CALL;


    adev->stream_status |=(1<<voice_out->audio_app_type);
    switch_vbc_iis_route(actl,UC_CALL,true);

    /*Send at command to dsp side */
    set_dsp_volume(adev->dev_ctl,adev->dev_ctl->voice_volume);

    pcm_modem_dl = pcm_open(0, actl->pcm_handle.playback_devices[AUD_PCM_MODEM_DL], PCM_OUT,
                                    &(actl->pcm_handle.play[AUD_PCM_MODEM_DL]));
    if (!pcm_is_ready(pcm_modem_dl)) {
        LOG_E("voice:cannot open pcm_modem_dl : %s",
              pcm_get_error(pcm_modem_dl));
        goto Err;
    }

    pcm_modem_ul = pcm_open(0, actl->pcm_handle.playback_devices[AUD_PCM_MODEM_UL], PCM_IN,
                                    &(actl->pcm_handle.play[AUD_PCM_MODEM_UL]));
    if (!pcm_is_ready(pcm_modem_ul)) {
        LOG_E("voice:cannot open pcm_modem_ul : %s",
              pcm_get_error(pcm_modem_ul));
        goto Err;
    }
    if( 0 != pcm_start(pcm_modem_dl)) {
        LOG_E("pcm dl start unsucessfully err:%s",pcm_get_error(pcm_modem_dl));
        goto Err;
    }
    if( 0 != pcm_start(pcm_modem_ul)) {
        LOG_E("pcm ul start unsucessfully err:%s",pcm_get_error(pcm_modem_ul));
        goto Err;
    }

    voice_out->pcm_modem_ul=pcm_modem_ul;
    voice_out->pcm_modem_dl=pcm_modem_dl;
    voice_out->dev=adev;
    voice_out->standby=false;

    if (pthread_mutex_init(&(voice_out->lock), NULL) != 0) {
        LOG_E("Failed pthread_mutex_init voice_out->lock,errno:%u,%s",
              errno, strerror(errno));
        goto Err;
    }

    adev->stream_status |=(1<<AUDIO_HW_APP_CALL);
    audio_add_output(adev,voice_out,AUDIO_HW_APP_CALL,do_voice_out_standby);

    close_in_control(adev->dev_ctl);
    close_out_control(adev->dev_ctl);
    select_audio_param(actl->adev,true);
    select_devices_new(actl,adev->out_devices,0);
    set_dsp_volume(adev->dev_ctl,adev->dev_ctl->voice_volume);
    pthread_mutex_unlock(&adev->lock);

    LOG_I("start_voice_call success,out:%p",voice_out);
    return 0;

Err:
    dsp_sleep_ctrl(&adev->agdsp_ctl,false);
    adev->stream_status &=~(1<<AUDIO_HW_APP_CALL);
    LOG_E("start_voice_call failed");
    if(NULL!=pcm_modem_ul){
        pcm_close(pcm_modem_ul);
    }
    if(NULL!=pcm_modem_dl){
        pcm_close(pcm_modem_dl);
    }

    if(NULL!=voice_out){
        pthread_mutex_destroy(&(voice_out->lock));
        free(voice_out);
    }
    set_usecase(actl, UC_CALL, false);
    switch_vbc_iis_route(actl,UC_CALL,false);
    pthread_mutex_unlock(&adev->lock);
    return -1;
}

int stop_voice_call(struct audio_control *ctl)
{
    struct tiny_audio_device *adev=(struct tiny_audio_device *)ctl->adev;
    force_out_standby(adev,AUDIO_HW_APP_CALL);
    return 0;
}
