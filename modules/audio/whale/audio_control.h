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

#ifndef _AUDIO_CONTROL_H_
#define _AUDIO_CONTROL_H_

#include "stdint.h"
#include <tinyalsa/asoundlib.h>
#include "audio_xml_utils.h"
#include "audio_hw.h"
#include "tinycompress/tinycompress.h"
#include "audio_param/audio_param.h"

typedef enum {
    AUDIO_NET_UNKNOWN=0x0,
    AUDIO_NET_TDMA=0x1<<0,
    AUDIO_NET_GSM=0x1<<1,
    AUDIO_NET_WCDMA=0x1<<2,
    AUDIO_NET_VOLTE=0x1<<4,
    AUDIO_NET_LOOP=0x1<<5,
}aud_net_m;

#define AUDIO_ROUTE_PATH "/etc/audio_route.xml"
#define AUDIO_DEVICES_MAGAGER_PATH  "/etc/device_id.xml"
#define AUDIO_PCM_MAGAGER_PATH  "/etc/audio_pcm.xml"

#define CODEC_SPK_MUTE (1<<0)
#define CODEC_EXT_SPK_MUTE (1<<1)
#define CODEC_HANDSET_MUTE (1<<2)
#define CODEC_HEADPHONE_MUTE (1<<3)

#define DSP_DA0_MDG_MUTE (1<<4)
#define DSP_DA1_MDG_MUTE (1<<5)
#define AUDIO_MDG_MUTE (1<<6)

#define CODEC_MIC_MUTE (1<<8)
#define DSP_VOICE_UL_MUTE (1<<16)
#define DSP_VOICE_DL_MUTE (1<<17)

typedef enum {
    UC_CALL     = 0x1,
    UC_VOIP     = 0x2,
    UC_FM       = 0x4,
    UC_NORMAL_PLAYBACK    = 0x8,
    UC_LOOP     = 0x10,
    UC_MM_RECORD = 0x20,
    UC_AGDSP_ASSERT = 0x40,
    UC_FM_RECORD = 0x80,
    UC_BT_VOIP = 0x100,
    UC_DEEP_BUFFER_PLAYBACK = 0x200,
    UC_OFFLOAD_PLAYBACK =0x400,
    UC_VOICE_RECORD = 0x800,
    UC_VOIP_RECORD = 0x1000,
    UC_AUDIO_TEST = 0x2000,
    UC_BT_RECORD = 0x4000,
    UC_VBC_PCM_DUMP     = 0x8000,
    UC_UNKNOWN = 0x00000000,
} USECASE;

struct device_use_case_t {
    USECASE use;
    const char *case_name;
};

enum {
    PARAM_OUTDEVICES_CHANGE,
    PARAM_INDEVICES_CHANGE,
    PARAM_BT_NREC_CHANGE,
    PARAM_NET_CHANGE,
    PARAM_USECASE_CHANGE,
    PARAM_USECASE_DEVICES_CHANGE,
    PARAM_VOICE_VOLUME_CHANGE,
    PARAM_FM_VOLUME_CHANGE,
    PARAM_AUDIOTESTER_CHANGE,
};

enum {
    AUD_DEVICE_NONE,
    AUD_OUT_SPEAKER,
    AUD_OUT_HEADPHONE,
    AUD_OUT_HEADPHONE_SPEAKER,
    AUD_OUT_EARPIECE,
    AUD_OUT_SCO,
    AUD_IN_BULITION_MIC,
    AUD_IN_BACK_MIC,
    AUD_IN_HEADSET_MIC,
    AUD_IN_DUAL_MIC,
    AUD_IN_SCO,
    AUD_DEVICE_MAX,
};

struct device_map {
    int sprd_device;
    const char *device_name;
};


struct dev_condition_t {
    unsigned int check_size;
    unsigned int *condition;
};

struct  dev_description_t {
    char *device_name;
    struct dev_condition_t dev;
};

enum {
    AUD_PCM_VOIP = 0,
    AUD_PCM_CALL,
    AUD_PCM_BT_VOIP,
    AUD_PCM_DIGITAL_FM,
    AUD_PCM_MM_NORMAL,
    AUD_PCM_DEEP_BUFFER,
    AUD_PCM_OFF_LOAD,
    AUD_PCM_MODEM_UL,
    AUD_PCM_MODEM_DL,
    AUD_PCM_DSP_LOOP,
    AUD_PCM_BT_RECORD,
    AUD_PCM_MAX,
};
enum {
    AUD_SPRD_2731S_CODEC_TYPE = 0,
    AUD_REALTEK_CODEC_TYPE,
    AUD_CODEC_TYPE_MAX,
};

static const char  *audio_codec_chip_name[AUD_CODEC_TYPE_MAX] = {
    "2731S",
    "Realtek",
};

static const char  *pcm_config_name[AUD_PCM_MAX] = {
    "voip",
    "call",
    "bt_voip",
    "digital_fm",
    "mm_normal",
    "deep_buffer",
    "off_load",
    "modem_ul",
    "modem_dl",
    "dsp_loop",
    "bt_record"
};
enum {
    AUD_PCM_ATTRIBUTE_CHANNELS = 0,
    AUD_PCM_ATTRIBUTE_RATE,
    AUD_PCM_ATTRIBUTE_PERIOD_SIZE,
    AUD_PCM_ATTRIBUTE_PERIOD_COUNT,
    AUD_PCM_ATTRIBUTE_FORMAT,
    AUD_PCM_ATTRIBUTE_START_THRESHOLD,
    AUD_PCM_ATTRIBUTE_STOP_THRESHOLD,
    AUD_PCM_ATTRIBUTE_SILENCE_THRESHOLD,
    AUD_PCM_ATTRIBUTE_AVAIL_MIN,
    AUD_PCM_ATTRIBUTE_DEVICES,
    AUD_PCM_ATTRIBUTE_MAX,
};


#define AUDIO_CONTROL_MAX_SIZE  128

static  const char *pcm_config_attribute[AUD_PCM_ATTRIBUTE_MAX] = {
    "channels",
    "rate",
    "period_size",
    "period_count",
    "format",
    "start_threshold",
    "stop_threshold",
    "silence_threshold",
    "avail_min",
    "device",
};

static const  char *device_none = "none";

#define  VBC_DAC0_DG "VBC DAC0 DG Set"
#define  VBC_DAC1_DG "VBC DAC1 DG Set"

#define  VBC_DAC0_MDG "VBC DAC0 DSP MDG Set"
#define  VBC_DAC1_MDG "VBC DAC1 DSP MDG Set"

#define  VBC_DAC0_SMTHDG "VBC DAC0 SMTHDG Set"
#define  VBC_DAC1_SMTHDG "VBC DAC1 SMTHDG Set"

struct mixer_control {
    char *name;
    struct mixer_ctl *ctl;
    int value;
    int val_count;
    char *strval; 
};

struct device_control {
    char *name;
    int ctl_size;
    struct mixer_control *ctl;
};

struct device_transition {
    char *name;
    struct device_control trans_ctl;
};

struct device_route {
    char *name;
    int devices;
    struct device_control ctl_on;
    struct device_control ctl_off;
    int trans_size;
    struct device_transition *trans;
};

struct device_route_handler {
    struct device_route *route;
    unsigned int size;
};

struct gain_mixer_control {
    char *name;
    struct mixer_ctl *ctl;
    int volume_size;
    int *volume_value;
};

struct private_control {
    int size;
    struct device_control *priv;
};


struct dsploop_control {
    int type;
    int ctl_size;
    struct mixer_control *ctl;
};

struct private_dsploop_control {
    int size;
    struct dsploop_control *dsp_ctl;
};

struct audio_route {
    struct device_route_handler devices_route;    // for normal
    struct private_control priv_ctl;
    struct private_dsploop_control dsploop_ctl;
    struct device_route_handler vbc_iis;
    struct device_route_handler vbc_pcm_dump;
    struct device_route *pre_in_ctl;
    struct device_route *pre_out_ctl;
};


struct device_gain {
    char *name;
    int ctl_size;
    int id;
    struct gain_mixer_control *ctl;
};

struct device_usecase_gain {
    int gain_size;
    struct device_gain *dev_gain;
    struct mixer *mixer;
};


struct cards {
    int s_tinycard;
    int s_vaudio;
    int s_vaudio_w;
    int s_vaudio_lte;
    int s_voip;
    int s_bt_sco;
};

struct voice_handle_t {
    bool call_start;
    bool call_connected;
    bool call_prestop;
    struct pcm *pcm_modem_dl;
    struct pcm *pcm_modem_ul;
};

struct btsco_i2s_t {
    int cpuindex;
    char *ctl_file;
    int is_switch;
    int i2s_index;
    int fd;
};

#define MAX_GAIN_DEVICE_NAME_LEN 60


struct _compr_config {
    __u32 fragment_size;
    __u32 fragments;
    int devices;
};

struct pcm_handle_t {
    int                 playback_devices[AUD_PCM_MAX];
    struct pcm_config   play[AUD_PCM_MAX];
    int                 record_devices[AUD_PCM_MAX];
    struct pcm_config   record[AUD_PCM_MAX];

    struct _compr_config compress;
};

struct sprd_codec_mixer_t {
    struct mixer_ctl *mic_boost;
    struct mixer_ctl *auxmic_boost;
    struct mixer_ctl *headmic_boost;

    struct mixer_ctl *adcl_capture_volume;
    struct mixer_ctl *adcr_capture_volume;

    struct mixer_ctl *spkl_playback_volume;
    struct mixer_ctl *spkr_playback_volume;

    struct mixer_ctl *hpl_playback_volume;
    struct mixer_ctl *hpr_playback_volume;

    struct mixer_ctl *ear_playback_volume;

    struct mixer_ctl *inner_pa;

    struct mixer_ctl *hp_inner_pa;

    struct mixer_ctl *dacs_playback_volume;

    struct mixer_ctl *dacl_playback_volume;

    struct mixer_ctl *dacr_playback_volume;
};

struct routing_manager {
    pthread_t   routing_switch_thread;
    bool        is_exit;
    sem_t       device_switch_sem;
};

struct dev_bluetooth_t {
    bool bluetooth_nrec;
    int samplerate;
};

struct voice_net_t {
    aud_net_m net_mode;
    bool wb_mode;
};

struct audio_param_res {
    int usecase;
    aud_net_m net_mode;
    bool wb_mode;
    audio_devices_t in_devices;
    audio_devices_t out_devices;
    uint8_t voice_volume;
    struct dev_bluetooth_t bt_infor;
    bool bt_nrec;
    uint8_t codec_type;

    /*fm*/
    uint8_t cur_fm_dg_id;
    uint8_t cur_fm_dg_volume;
    uint8_t fm_volume;

    /*record*/
    uint8_t cur_record_dg_id;

    /*play*/
    uint8_t cur_vbc_playback_id;
    uint8_t cur_playback_dg_id;

    /* voice */
    /*dg*/
    uint8_t cur_voice_dg_id;
    uint8_t cur_voice_dg_volume;

    /*dsp*/
    uint8_t cur_dsp_id;
    uint8_t cur_dsp_volume;

    /*vbc*/
    uint8_t cur_vbc_id;

    uint8_t cur_voice_volume;

    /* codec param id */
    uint8_t cur_codec_p_id;
    uint8_t cur_codec_p_volume;
    uint8_t cur_codec_c_id;
};

struct mixer_ctrl_t {
    int value;
    struct mixer_ctl * mixer;
};

struct mute_control {
    struct mixer_ctrl_t spk_mute;
    struct mixer_ctrl_t spk2_mute;
    struct mixer_ctrl_t handset_mute;
    struct mixer_ctrl_t headset_mute;
    struct mixer_ctrl_t linein_mute;

    struct mixer_ctrl_t dsp_da0_mdg_mute;
    struct mixer_ctrl_t dsp_da1_mdg_mute;
    struct mixer_ctrl_t audio_mdg_mute;
    int mute_status;
};

typedef enum {
    VBC_INVALID_DMUP          = 0,
    VBC_DAC0_MIXER_DMUP   = (1<<0),
    VBC_DAC0_A1_DMUP         = (1<<1),
    VBC_DAC0_A2_DMUP         = (1<<2),
    VBC_DAC0_A3_DMUP         = (1<<3),
    VBC_DAC0_A4_DMUP         = (1<<4),

    VBC_DAC1_MIXER_DMUP   = (1<<5),
    VBC_DAC1_V1_DMUP         = (1<<6),
    VBC_DAC1_V2_DMUP         = (1<<7),
    VBC_DAC1_V3_DMUP         = (1<<8),
    VBC_DAC1_V4_DMUP         = (1<<9),

    VBC_DISABLE_DMUP          = (1<<24),
} PCM_DUMP_TYPE;

struct vbc_dump_ctl {
    bool is_exit;
    pthread_t   thread;
    struct pcm_config config;
    int dump_fd;
    int pcm_devices;
    const char * dump_name;
};

struct audio_control {
    struct mixer *mixer;
    struct mixer_ctl *vbc_iis_loop;
    struct mixer_ctl *offload_dg;
    struct cards cards;
    struct audio_route route;
    struct device_usecase_gain dg_gain;
    struct sprd_codec_mixer_t codec;
    struct audio_param_res param_res;
    int usecase;
    int fm_volume;
    bool fm_mute;
    int voice_volume;
    float music_volume;
    struct routing_manager routing_mgr;
    struct tiny_audio_device *adev;

    struct pcm_handle_t pcm_handle;

    //use for set dsp volume
    struct mixer_ctl *dsp_volume_ctl;
    struct mixer_ctl *voice_ul_mute_ctl;
    struct mixer_ctl *voice_dl_mute_ctl;
    struct mixer_ctl *bt_src;
    int codec_type;
    pthread_mutex_t lock;
    struct dsp_control_t *agdsp_ctl;
    AUDIO_PARAM_T  *audio_param;

    audio_devices_t out_devices;
    audio_devices_t in_devices;
    struct audio_config_handle config;
    struct mute_control mute;

    struct listnode switch_device_cmd_list;
    pthread_mutex_t cmd_lock;
    struct vbc_dump_ctl vbc_dump;
};

#ifdef __cplusplus
extern "C" {
#endif

int start_fm(struct audio_control *ctl);
int stop_fm(struct audio_control *ctl);
struct audio_control *init_audio_control(struct tiny_audio_device
        *adev);/* define in Audio_parse.cpp */
void free_audio_control(struct audio_control
                        *control);/* define in Audio_parse.cpp */
int set_fm_volume(struct audio_control *ctl, int index);
int set_private_control(struct private_control *pri, const char *priv_name,
                        int value);
int set_usecase(struct audio_control *actl, int usecase, bool on);
bool is_usecase(struct audio_control *actl, int usecase);
int stream_routing_manager_create(struct audio_control *actl);
void stream_routing_manager_close(struct audio_control *actl);
int vb_effect_profile_apply(struct audio_control *actl);
int close_all_control(struct audio_control *actl);
int close_in_control(struct audio_control *actl);
int close_in_control(struct audio_control *actl);
int set_vdg_gain(struct device_usecase_gain *dg_gain, int param_id, int volume);
int switch_vbc_iis_route(struct audio_control *ctl,USECASE uc,bool on);
void free_audio_control(struct audio_control *control);
struct audio_control *init_audio_control(struct tiny_audio_device *adev);
int init_audio_param( struct tiny_audio_device *adev);
int set_dsp_volume(struct audio_control *ctl,int volume);
int set_dsp_volume_update(struct audio_control *ctl);
int dev_ctl_iis_set(struct audio_control *ctl, int usecase,int on);
int set_offload_volume( struct audio_control *dev_ctl, float left,
                           float right);
int set_voice_ul_mute(struct audio_control *actl, bool mute);
int set_voice_dl_mute(struct audio_control *actl, bool mute);
int set_vbc_bt_src(struct audio_control *actl, int rate);
int set_vbc_bt_nrec(struct audio_control *actl, bool nrec);
int set_codec_mute(struct audio_control *actl,bool on);
int set_mic_mute(struct audio_control *actl, bool on);
int set_mdg_mute(struct audio_control *actl,int usecase,bool on);
int set_codec_volume(struct audio_control *dev_ctl,int param_id,int vol_index);
struct pcm_config * dev_ctl_get_pcm_config(struct audio_control *dev_ctl,int app_type, int * dev);
int set_audioparam(struct audio_control *dev_ctl,int type, void *param_change,int force);
int set_audioparam_unlock(struct audio_control *dev_ctl,int type, void *param_change,int force);
uint8_t get_loopback_param(audio_devices_t out_devices,audio_devices_t in_devices);
void *get_ap_record_param(AUDIO_PARAM_T  *audio_param,audio_devices_t in_devices);
int set_vbc_dump_control(struct audio_control *ctl, const char * dump_name, bool on);
bool is_usecase_unlock(struct audio_control *actl, int usecase);
#ifdef __cplusplus
}
#endif

#endif //_AUDIO_CONTROL_H_
