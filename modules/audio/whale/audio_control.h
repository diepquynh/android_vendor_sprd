#ifndef _AUDIO_CONTROL_H_
#define _AUDIO_CONTROL_H_

#include "stdint.h"
#include <tinyalsa/asoundlib.h>
#include "audio_xml_utils.h"
#include "audio_hw.h"
#include "tinycompress/tinycompress.h"

#define AUDIO_ROUTE_PATH "/etc/audio_route.xml"
#define AUDIO_DEVICES_MAGAGER_PATH  "/etc/device_id.xml"
#define AUDIO_PCM_MAGAGER_PATH  "/etc/audio_pcm.xml"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UC_CALL     = 0x1,
    UC_VOIP     = 0x2,
    UC_FM       = 0x4,
    UC_MUSIC    = 0x8,
    UC_LOOP     = 0x10,
    UC_MM_RECORD = 0x20,
} USECASE;

struct device_use_case_t {
    USECASE use;
    const char *case_name;
};
static const struct device_use_case_t  device_usecase_table[] = {
    {UC_CALL,           "call"},
    {UC_VOIP,           "voip"},
    {UC_FM,             "fm"},
    {UC_MUSIC,          "music"},
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
    AUD_PCM_MM_FAST,
    AUD_PCM_OFF_LOAD,
    AUD_PCM_MODEM_UL,
    AUD_PCM_MODEM_DL,
    AUD_PCM_DSP_LOOP,
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
    "mm_fast",
    "off_load",
    "modem_ul",
    "modem_dl",
    "dsp_loop"
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
    struct device_route_handler devices_route_call;    // for call
    struct private_control priv_ctl;
    struct private_dsploop_control dsploop_ctl;
    struct device_route_handler vbc_iis;
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

    int switch_route;
};



struct audio_control {
    struct mixer *mixer;
    struct cards cards;
    struct audio_route route;
    struct device_usecase_gain dg_gain;
    struct sprd_codec_mixer_t codec;
    int usecase;
    int fm_volume;
    int voice_volume;
    struct stream_routing_manager routing_mgr;
    struct tiny_audio_device *adev;

    struct pcm_handle_t pcm_handle;

    //use for set dsp volume
    struct mixer_ctl *dsp_volume_ctl;
    struct mixer_ctl *voice_ul_mute_ctl;
    struct mixer_ctl *voice_dl_mute_ctl;
    int codec_type;
    pthread_mutex_t lock; 
};

int start_fm(struct audio_control *ctl);
int stop_fm(struct audio_control *ctl);
int switch_fm_control(struct audio_control *ctl, bool on);

struct audio_control *init_audio_control(struct tiny_audio_device
        *adev);/* define in Audio_parse.cpp */
void free_audio_control(struct audio_control
                        *control);/* define in Audio_parse.cpp */

//int switch_device_route(struct audio_control *ctl, int device, bool in_device);
int switch_fm_control(struct audio_control *ctl, bool on);
int set_fm_volume(struct audio_control *ctl, int index);

int set_private_control(struct private_control *pri, const char *priv_name,
                        int value);
//int get_private_control(struct audio_control *ctl, const char *priv_name);
void set_usecase(struct audio_control *actl, int usecase, bool on);
bool is_usecase(struct audio_control *actl, int usecase);

int stream_routing_manager_create(struct audio_control *actl);
void stream_routing_manager_close(struct audio_control *actl);
void select_devices(struct audio_control *actl);

int vb_effect_profile_apply(struct audio_control *actl);

//int parse_audio_gain(struct audio_control *control, param_group_t param_root);
//void free_audio_gain(struct audio_control *control);
int close_all_control(struct audio_control *actl);
int close_in_control(struct audio_control *actl);
int close_in_control(struct audio_control *actl);
int set_vdg_gain(struct device_usecase_gain *dg_gain, int param_id, int volume);
int switch_vbc_iis_route(struct audio_control *ctl,USECASE uc,bool on);
#ifdef __cplusplus
}
#endif

#endif //_AUDIO_CONTROL_H_
