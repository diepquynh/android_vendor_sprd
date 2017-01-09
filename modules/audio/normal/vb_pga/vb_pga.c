#define LOG_TAG "audio_hw_pga"
#include <utils/Log.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include "audio_pga.h"
#include <tinyalsa/asoundlib.h>

//#ifdef __cplusplus
//extern "c"
//{
//#endif


//#define MY_DEBUG

#ifdef MY_DEBUG
#define MY_TRACE    LOGW
#else
#define MY_TRACE
#endif

/* ALSA cards for sprd */
#define CARD_SPRDPHONE "sprdphone"
/* mixer control */
#define MIXER_CTL_INNER_PA_CONFIG   "Inter PA Config"


static int fd_audio_para = -1;
static int tiny_card_num = -1;

typedef struct {
    unsigned short adc_pga_gain_l;
    unsigned short adc_pga_gain_r;
    unsigned short pa_config;
    unsigned short fm_pa_config;
    uint32_t fm_pga_gain_l;
    uint32_t fm_pga_gain_r;
    uint32_t dac_pga_gain_l;
    uint32_t dac_pga_gain_r;
    uint32_t devices;
    uint32_t mode;
}pga_gain_nv_t;

static int32_t GetAudio_mode_number_from_nv(AUDIO_TOTAL_T *aud_params_ptr)
{
    int32_t lmode = 0;
    if(!strcmp((char *)aud_params_ptr->audio_nv_arm_mode_info.ucModeName, "Headset")){
        lmode = 0;
    }else if(!strcmp((char *)aud_params_ptr->audio_nv_arm_mode_info.ucModeName, "Headfree")){
        lmode = 1;
    }else if(!strcmp((char *)aud_params_ptr->audio_nv_arm_mode_info.ucModeName, "Handset")){
        lmode = 2;
    }else if(!strcmp((char *)aud_params_ptr->audio_nv_arm_mode_info.ucModeName, "Handsfree")){
        lmode = 3;
    }else{
        ALOGE("%s modename(%s) is not support,error \n",__func__,aud_params_ptr->audio_nv_arm_mode_info.ucModeName);
    }
    return lmode;
}

static int  GetAudio_pga_nv(AUDIO_TOTAL_T *aud_params_ptr, pga_gain_nv_t *pga_gain_nv, uint32_t vol_level)
{
    if((NULL == aud_params_ptr) || (NULL == pga_gain_nv)){
        ALOGE("%s aud_params_ptr or pga_gain_nv is NULL",__func__);
        return -1;
    }
    pga_gain_nv->adc_pga_gain_l = aud_params_ptr->audio_nv_arm_mode_info.tAudioNvArmModeStruct.reserve[AUDIO_NV_CAPTURE_GAIN_INDEX];    //43
    pga_gain_nv->adc_pga_gain_r = pga_gain_nv->adc_pga_gain_l;

    pga_gain_nv->dac_pga_gain_l = aud_params_ptr->audio_nv_arm_mode_info.tAudioNvArmModeStruct.app_config_info_set.app_config_info[0].arm_volume[vol_level];
    pga_gain_nv->dac_pga_gain_r = pga_gain_nv->dac_pga_gain_l;

    pga_gain_nv->fm_pga_gain_l  = (aud_params_ptr->audio_nv_arm_mode_info.tAudioNvArmModeStruct.reserve[AUDIO_NV_FM_GAINL_INDEX]
        | ((aud_params_ptr->audio_nv_arm_mode_info.tAudioNvArmModeStruct.reserve[AUDIO_NV_FM_DGAIN_INDEX]<<16) & 0xffff0000));  //18,19
    pga_gain_nv->fm_pga_gain_r  = pga_gain_nv->fm_pga_gain_l;
    pga_gain_nv->pa_config = aud_params_ptr->audio_nv_arm_mode_info.tAudioNvArmModeStruct.reserve[AUDIO_NV_INTPA_GAIN_INDEX];    //45
    pga_gain_nv->fm_pa_config =
    aud_params_ptr->audio_nv_arm_mode_info.tAudioNvArmModeStruct.reserve[AUDIO_NV_FM_INTPA_GAIN_INDEX];//47
    ALOGW("vb_pga.c %s, dac_pga_gain_l:0x%x adc_pga_gain_l:0x%x fm_pga_gain_l:0x%x fm_pga_gain_r:0x%x pa_config:0x%x, 0x%x(fm), vol_level:0x%x ",
        __func__,pga_gain_nv->dac_pga_gain_l,pga_gain_nv->adc_pga_gain_l,
        pga_gain_nv->fm_pga_gain_l,pga_gain_nv->fm_pga_gain_r,
        pga_gain_nv->pa_config,pga_gain_nv->fm_pa_config,vol_level);
    return 0;
}

static int SetAudio_gain_4eng(struct audio_pga *pga, pga_gain_nv_t *pga_gain_nv, AUDIO_TOTAL_T *aud_params_ptr)
{
    int card_id = 0;
    int32_t lmode = 0;
    struct mixer *mixer = NULL;
    struct mixer_ctl *pa_config_ctl = NULL;
    ALOGD("vb_pga.c  %s", __func__);
    if((NULL == aud_params_ptr) || (NULL == pga_gain_nv) || (NULL == pga)){
        ALOGE("%s aud_params_ptr or pga_gain_nv or audio_pga is NULL",__func__);
        return -1;
    }
    card_id = get_snd_card_number(CARD_SPRDPHONE);
    if (card_id < 0){
        ALOGE("%s get_snd_card_number error(%d)",__func__,card_id);
        return -1;
    }
    mixer = mixer_open(card_id);
    if (!mixer) {
        ALOGE("%s Unable to open the mixer, aborting.",__func__);
        return -1;
    }
    pa_config_ctl = mixer_get_ctl_by_name(mixer, MIXER_CTL_INNER_PA_CONFIG);
    lmode = GetAudio_mode_number_from_nv(aud_params_ptr);
    ALOGD("vb_pga.c  %s, mode: %d", __func__, lmode);
    if(0 == lmode){ //Headset
        audio_pga_apply(pga,pga_gain_nv->fm_pga_gain_l,"linein-hp-l");
        audio_pga_apply(pga,pga_gain_nv->fm_pga_gain_r,"linein-hp-r");
        audio_pga_apply(pga,pga_gain_nv->dac_pga_gain_l,"headphone-l");
        audio_pga_apply(pga,pga_gain_nv->dac_pga_gain_r,"headphone-r");
        audio_pga_apply(pga,pga_gain_nv->adc_pga_gain_l,"capture-l");
        audio_pga_apply(pga,pga_gain_nv->adc_pga_gain_r,"capture-r");
    }else if(1 == lmode){   //Headfree
        audio_pga_apply(pga,pga_gain_nv->dac_pga_gain_l,"headphone-spk-l");
        audio_pga_apply(pga,pga_gain_nv->dac_pga_gain_r,"headphone-spk-r");
        mixer_ctl_set_value(pa_config_ctl, 0, pga_gain_nv->pa_config);
    }else if(2 == lmode){   //Handset
        audio_pga_apply(pga,pga_gain_nv->dac_pga_gain_l,"earpiece");
        audio_pga_apply(pga,pga_gain_nv->adc_pga_gain_l,"capture-l");
        audio_pga_apply(pga,pga_gain_nv->adc_pga_gain_r,"capture-r");
    }else if(3 == lmode){   //Handsfree
        audio_pga_apply(pga,pga_gain_nv->fm_pga_gain_l,"linein-spk-l");
        audio_pga_apply(pga,pga_gain_nv->fm_pga_gain_r,"linein-spk-r");
        audio_pga_apply(pga,pga_gain_nv->dac_pga_gain_l,"speaker-l");
        audio_pga_apply(pga,pga_gain_nv->dac_pga_gain_r,"speaker-r");
        mixer_ctl_set_value(pa_config_ctl, 0, pga_gain_nv->pa_config);
        audio_pga_apply(pga,pga_gain_nv->adc_pga_gain_l,"capture-l");
        audio_pga_apply(pga,pga_gain_nv->adc_pga_gain_r,"capture-r");
    }
    mixer_close(mixer);
    ALOGW("%s, set cp mode(0x%x) ",__func__,lmode);
    return 0;
}


int SetAudio_pga_parameter_eng(AUDIO_TOTAL_T *aud_params_ptr, unsigned int params_size, uint32_t vol_level)
{
    int ret = 0;
    int card_id;
    pga_gain_nv_t pga_gain_nv;
    struct mixer *mixer;
    struct audio_pga *pga;
    memset(&pga_gain_nv,0,sizeof(pga_gain_nv_t));

    if (sizeof(AUDIO_TOTAL_T) != params_size) {
        ALOGE("%s, Error: params_size = %d, total size = %d", __func__,params_size, sizeof(AUDIO_TOTAL_T));
        return -1;
    }
    ret = GetAudio_pga_nv(aud_params_ptr,&pga_gain_nv,vol_level);
    if(ret < 0){
        return -1;
    }
    card_id = get_snd_card_number(CARD_SPRDPHONE);
    ALOGW("%s card_id = %d", __func__,card_id);
    if (card_id < 0)
        return -1;
    mixer = mixer_open(card_id);
    if (!mixer) {
        ALOGE("%s Failed to open mixer",__func__);
        return -1;
    }
    pga = audio_pga_init(mixer);
    if (!pga) {
        mixer_close(mixer);
        ALOGE("%s Warning: Unable to locate PGA from XML.",__func__);
        return -1;
    }
    ret = SetAudio_gain_4eng(pga,&pga_gain_nv,aud_params_ptr);
    if(ret < 0){
        mixer_close(mixer);
        return -1;
    }
    mixer_close(mixer);
    return 0;
}


