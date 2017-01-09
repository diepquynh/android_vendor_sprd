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

#include<stdbool.h>
#ifndef _AUDIO_DEBUG_H_
#define _AUDIO_DEBUG_H_
#ifdef __cplusplus
extern "C" {
#endif

enum {
    AUD_FM_DIGITAL_TYPE = 0,
    AUD_FM_ANALOG_TYPE,
    AUD_FM_ANALOGVBC_TYPE,
    AUD_TYPE_MAX,
};

static const char  *fm_type_name[AUD_TYPE_MAX] = {
    "Digital",
    "Analog",
    "AnalogVbc",
};

struct param_tunning_debug_handle {
    bool txt_enable;
    int txt_fd;
    bool hex_enable;
    int hex_fd;
    bool err_enable;
    int err_fd;
};

struct tunning_debug {
    struct param_tunning_debug_handle rx_debug;
    struct param_tunning_debug_handle tx_debug;
};

struct param_pcm_dump_handle {
    bool hex_enable;
    int hex_fd;
    char *hex_file_name;
};

struct playback_dump {
    struct param_pcm_dump_handle normal;
    struct param_pcm_dump_handle offload;
    struct param_pcm_dump_handle dsploop;
    struct param_pcm_dump_handle deep_buffer;
};

struct record_dump {
    struct param_pcm_dump_handle normal;
    struct param_pcm_dump_handle vbc;
    struct param_pcm_dump_handle process;
    struct param_pcm_dump_handle nr;
    struct param_pcm_dump_handle mixer_vbc;
    struct param_pcm_dump_handle dsploop;
};

struct hex_dump_handle {
    struct record_dump record;
    struct playback_dump playback;
    struct tunning_debug tunning;
};


struct record_config_handle {
    bool record_nr_enable;
    bool record_process_enable;
};

struct mute_control_name {
    char * spk_mute;
    char * spk2_mute;
    char * handset_mute;
    char * headset_mute;
    char * linein_mute;

    char * dsp_da0_mdg_mute;
    char * dsp_da1_mdg_mute;
    char * audio_mdg_mute;
};

struct audio_config_handle {
    int log_level;
    struct hex_dump_handle dump;
    struct record_config_handle record;
    int fm_type;
    char *card_name;
    int mic_switch;
    bool support_24bits;
    struct mute_control_name mute;
};

extern int log_level;
#define LOG_V(...)  ALOGV_IF(log_level >= 5,__VA_ARGS__);
#define LOG_D(...)  ALOGD_IF(log_level >= 4,__VA_ARGS__);
#define LOG_I(...)  ALOGI_IF(log_level >= 3,__VA_ARGS__);
#define LOG_W(...)  ALOGW_IF(log_level >= 2,__VA_ARGS__);
#define LOG_E(...)  ALOGE_IF(log_level >= 1,__VA_ARGS__);
#define AUDIO_DEBUG_CONFIG_PATH "/etc/audio_config.xml"
#define AUDIO_DEBUG_CONFIG_TUNNING_PATH "/data/local/media/audio_config.xml"
#define AUDIO_DEBUG_CONFIG_FIRSTNAME "audio_debug"
#define DEBUG_POINT  do{LOG_I("%s %d",__func__,__LINE__);}while(0)

int parse_audio_config(struct audio_control *dev_ctl);
#define AUDIO_EXT_CONTROL_PIPE "/dev/pipe/mmi.audio.ctrl"
#define AUDIO_EXT_DATA_CONTROL_PIPE "/data/local/media/mmi.audio.ctrl"
#define AUDIO_EXT_CONTROL_PIPE_MAX_BUFFER_SIZE  1024

int audio_config_ctrl(void *dev,struct str_parms *parms,int opt, char * kvpair);

#ifdef __cplusplus
}
#endif
#endif
