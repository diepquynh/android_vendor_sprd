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

#ifndef _AUDIO_PARAM_
#define _AUDIO_PARAM_
#include "stdint.h"
#include "aud_proc.h"
#include "audio_xml_utils.h"

#define MAX_LINE_LEN 512
#define SPLIT "\n"
#define BACKSLASH "\\"
#define MAX_SOCKT_LEN 65535

#define CODEC_INFOR_CTL   "Aud Codec Info"
#define VBC_UL_MUTE    "VBC_UL_MUTE"
#define VBC_DL_MUTE    "VBC_DL_MUTE"
#define VBC_BT_SRC     "VBC_SRC_BT"

#define VBC_DSP_PROFILE_UPDATE   "DSP VBC Profile Update"
#define VBC_EQ_PROFILE_UPDATE    "Audio Structure Profile Update"
#define VBC_NXP_PROFILE_UPDATE   "NXP Profile Update"
#define VBC_CODEC_PROFILE_UPDATE   "CODEC Profile Update"
#define VBC_DSP_PROFILE_SELECT   "DSP VBC Profile Select"
#define VBC_EQ_PROFILE_SELECT    "Audio Structure Profile Select"
#define VBC_NXP_PROFILE_SELECT   "NXP Profile Select"
#define VBC_CODEC_PROFILE_SELECT   "CODEC Profile Select"

#define REALTEK_EXTEND_PARAM_UPDATE   "Realtek Extend Codec Param Update"
#define MODE_NAME_MAX_LEN   64
#define MODE_DEPTH_MAX_LEN   8

#define AUDIO_CODEC_GAIN_PARAM "/etc/audio_gain.xml"
#define AUDIO_CODEC_GAIN_TUNNING_PARAM "/data/local/media/audio_gain.xml"
#define AUDIO_CODEC_FIRST_NAME "audio_codec_param"

#define ENG_SET_RAM_OPS 0x01
#define ENG_SET_FLASH_OPS ENG_SET_RAM_OPS<<1
#define ENG_SET_REG_OPS ENG_SET_FLASH_OPS<<1
#define ENG_AUDIO_CODEC_OPS ENG_SET_REG_OPS<<1
#define ENG_AUDIO_VBCEQ_OPS ENG_AUDIO_CODEC_OPS<<1
#define ENG_DSP_VBC_OPS ENG_AUDIO_VBCEQ_OPS<<1
#define ENG_AUDIO_STRECTURE_OPS ENG_DSP_VBC_OPS<<1
#define ENG_NXP_OPS ENG_AUDIO_STRECTURE_OPS<<1
#define ENG_CODEC_OPS ENG_NXP_OPS<<1
#define ENG_PGA_OPS ENG_CODEC_OPS<<1
#define ENG_AUDIO_PROCESS_OPS ENG_PGA_OPS<<1

#define DSP_VBC_XML_TUNNING_PATH "/data/local/media/dsp_vbc.xml"
#define DSP_VBC_BIN_PATH "/data/local/media/dsp_vbc.bin"
#define DSP_VBC_XML_PATH "/etc/dsp_vbc.xml"

#define AUDIO_STRUCTURE_XML_TUNNING_PATH "/data/local/media/audio_structure.xml"
#define AUDIO_STRUCTURE_BIN_PATH "/data/local/media/audio_structure.bin"
#define AUDIO_STRUCTURE_XML_PATH "/etc/audio_structure.xml"
#define AUDIO_STRUCTURE_FIRST_NAME "audio_structure"

#define NXP_XML_TUNNING_PATH "/data/local/media/nxp.xml"
#define NXP_BIN_PATH "/data/local/media/nxp.bin"
#define NXP_XML_PATH "/etc/nxp.xml"

#define PGA_GAIN_XML_TUNNING_PATH "/data/local/media/audio_pga.xml"
#define PGA_GAIN_BIN_PATH "/data/local/media/audio_pga.bin"
#define PGA_GAIN_XML_PATH "/etc/audio_pga.xml"

#define AUDIO_PROCESS_XML_TUNNING_PATH "/data/local/media/audio_process.xml"
#define AUDIO_PROCESS_BIN_PATH "/data/local/media/audio_process.bin"
#define AUDIO_PROCESS_PATH "/etc/audio_process.xml"
#define AUDIO_PROCESS_FIRST_NAME "audio_process"

#define CODEC_XML_TUNNING_PATH "/data/local/media/codec.xml"
#define CODEC_BIN_PATH "/data/local/media/codec.bin"
#define CODEC_XML_PATH "/etc/codec.xml"

#define AUDIO_PARAM_FIRMWARE_NAME "audio_profile"
#define AUDIO_PARAM_FIRMWARE_PATH "/data/local/media/audio_profile"

#define AUDIO_PARAM_INFOR_PATH "/etc/audio_param_infor"
#define AUDIO_PARAM_INFOR_TUNNING_PATH "/data/local/media/audio_param_infor"

#define AUDIO_PARAM_RELTEK_EXTEND_PARAM_TUNNING_PATH "/data/local/media/realtek_codec.bin"
#define AUDIO_PARAM_RELTEK_EXTEND_PARAM_PATH "/etc/realtek_codec"
#define AUDIO_PARAM_RELTEK_EXTEND_XML_PARAM "/etc/realtek_codec.xml"

#define NUM_MODE     "num_mode"
#define STRUCT_SIZE   "struct_size"
#define SUB_LINE     "line"
#define MODE_NO   "mode"

#define AUDIO_COMMAND_BUF_SIZE 65536
#define AUDIO_TUNNING_PORT 9997 //zzj  for test
#define MAX_NAME_LENTH 20
#define AUDIO_CMD_TYPE  0x99
#define AUDIO_CMD_ERR "ERROR"
#define AUDIO_CMD_OK "OK"

#define PARAM_MAX_DEPTH   16
#define VBC_EQ_PARAM_MAX_SIZE 1024*4

#define TIME_ATTR "time"
#define VALUE "val"
#define VISIBLE "visible"
#define ID "id"
#define TYPE "t"
#define NAME "name"
#define STR_LEN "l"

#define U16 "u16"
#define U32 "u32"
#define U8  "u8"
#define STR "str"

#define ID "id"
#define BITS "bits"
#define OFFSETS "offset"

#define TRUE_STRING "true"

#define AUDIO_PARAM_UPDATE_MAX_SIZE 256
#define AUDIO_PARAM_UPLOAD_CTL "upload"
#define AUDIO_PARAM_SET_CTL "set"

typedef enum {
    SND_AUDIO_PARAM_PROFILE_START = 0,
    SND_AUDIO_PARAM_DSP_VBC_PROFILE_DSP = SND_AUDIO_PARAM_PROFILE_START,
    SND_AUDIO_PARAM_AUDIO_STRUCTURE_PROFILE,
    SND_AUDIO_PARAM_NXP_PROFILE,
    SND_AUDIO_PARAM_PGA_PROFILE,
    SND_AUDIO_PARAM_CODEC_PROFILE,
    SND_AUDIO_PARAM_RECORD_PROCESS_PROFILE,
    SND_AUDIO_PARAM_PROFILE_MAX
} SND_AUDIO_PARAM_PROFILE_USE_E;

enum {
    AUDIO_RECORD_MODE_HANDSFREE,
    AUDIO_RECORD_MODE_HEADSET,
    AUDIO_RECORD_MODE_BLUETOOTH,
    AUDIO_RECORD_MODE_MAX
};
#define AUDIO_PARAM_INVALID_32BIT_OFFSET 0xffffffff
#define AUDIO_PARAM_INVALID_8BIT_OFFSET 0xff

#define AUDIO_PARAM_PROFILE_MASK    ((1<<SND_AUDIO_PARAM_DSP_VBC_PROFILE_DSP)||(1<<SND_AUDIO_PARAM_AUDIO_STRUCTURE_PROFILE)||(1<<SND_AUDIO_PARAM_NXP_PROFILE))

typedef enum {
    SND_VBC_PROFILE_MODE_START = 0,
    SND_VBC_PROFILE_MODE_HEADSET = SND_VBC_PROFILE_MODE_START,
    SND_VBC_PROFILE_MODE_HEADSFREE,
    SND_VBC_PROFILE_MODE_HANDSET,
    SND_VBC_PROFILE_MODE_HANDSFREE,
    SND_VBC_PROFILE_MODE_MAX
} SND_VBC_PROFILE_MODE_E;

typedef enum {
    PROFILE_MODE_START = 0,

    PROFILE_MODE_AUDIO_Handset_GSM=PROFILE_MODE_START,
    PROFILE_MODE_AUDIO_Handset_TDMA,
    PROFILE_MODE_AUDIO_Handset_WCDMA_NB,
    PROFILE_MODE_AUDIO_Handset_WCDMA_WB,
    PROFILE_MODE_AUDIO_Handset_VOLTE_NB,
    PROFILE_MODE_AUDIO_Handset_VOLTE_WB,
    PROFILE_MODE_AUDIO_Handset_VOIP,

    PROFILE_MODE_AUDIO_Handsfree_GSM,
    PROFILE_MODE_AUDIO_Handsfree_TDMA,
    PROFILE_MODE_AUDIO_Handsfree_WCDMA_NB,
    PROFILE_MODE_AUDIO_Handsfree_WCDMA_WB,
    PROFILE_MODE_AUDIO_Handsfree_VOLTE_NB,
    PROFILE_MODE_AUDIO_Handsfree_VOLTE_WB,
    PROFILE_MODE_AUDIO_Handsfree_VOIP,

    PROFILE_MODE_AUDIO_Headset4P_GSM,
    PROFILE_MODE_AUDIO_Headset4P_TDMA,
    PROFILE_MODE_AUDIO_Headset4P_WCDMA_NB,
    PROFILE_MODE_AUDIO_Headset4P_WCDMA_WB,
    PROFILE_MODE_AUDIO_Headset4P_VOLTE_NB,
    PROFILE_MODE_AUDIO_Headset4P_VOLTE_WB,
    PROFILE_MODE_AUDIO_Headset4P_VOIP,

    PROFILE_MODE_AUDIO_Headset3P_GSM,
    PROFILE_MODE_AUDIO_Headset3P_TDMA,
    PROFILE_MODE_AUDIO_Headset3P_WCDMA_NB,
    PROFILE_MODE_AUDIO_Headset3P_WCDMA_WB,
    PROFILE_MODE_AUDIO_Headset3P_VOLTE_NB,
    PROFILE_MODE_AUDIO_Headset3P_VOLTE_WB,
    PROFILE_MODE_AUDIO_Headset3P_VOIP,

    PROFILE_MODE_AUDIO_BTHS_GSM,
    PROFILE_MODE_AUDIO_BTHS_TDMA,
    PROFILE_MODE_AUDIO_BTHS_WCDMA_NB,
    PROFILE_MODE_AUDIO_BTHS_WCDMA_WB,
    PROFILE_MODE_AUDIO_BTHS_VOLTE_NB,
    PROFILE_MODE_AUDIO_BTHS_VOLTE_WB,
    PROFILE_MODE_AUDIO_BTHS_VOIP,

    PROFILE_MODE_AUDIO_BTHSNREC_GSM,
    PROFILE_MODE_AUDIO_BTHSNREC_TDMA,
    PROFILE_MODE_AUDIO_BTHSNREC_WCDMA_NB,
    PROFILE_MODE_AUDIO_BTHSNREC_WCDMA_WB,
    PROFILE_MODE_AUDIO_BTHSNREC_VOLTE_NB,
    PROFILE_MODE_AUDIO_BTHSNREC_VOLTE_WB,
    PROFILE_MODE_AUDIO_BTHSNREC_VOIP,

    PROFILE_MODE_MUSIC_Headset_Playback,
    PROFILE_MODE_MUSIC_Headset_Record,
    PROFILE_MODE_MUSIC_Headset_FM,

    PROFILE_MODE_MUSIC_Handsfree_Playback,
    PROFILE_MODE_MUSIC_Handsfree_Record,
    PROFILE_MODE_MUSIC_Handsfree_FM,

    PROFILE_MODE_MUSIC_Headfree_Playback,
    PROFILE_MODE_MUSIC_Handset_Playback,

    PROFILE_MODE_MUSIC_Bluetooth_Record,

    PROFILE_MODE_LOOP_Handset_MainMic,
    PROFILE_MODE_LOOP_Handsfree_MainMic,
    PROFILE_MODE_LOOP_Handsfree_AuxMic,
    PROFILE_MODE_LOOP_Headset4P_HeadMic,
    PROFILE_MODE_LOOP_Headset3P_MainMic,
    PROFILE_MODE_MAX
}AUDIO_PARAM_PROFILE_MODE_E;

#define DEFAULT_MIN_BUFFER_SIZE    4096

#define FIRMWARE_MAGIC_MAX_LEN       (16)
#define DSP_VBC_FIRMWARE_MAGIC_ID        ("DSP_VBC")
#define DSP_VBC_PROFILE_VERSION          (0x00000002)
#define DSP_VBC_PROFILE_CNT_MAX          (50)
#define DSP_VBC_PROFILE_NAME_MAX         (32)

#define VBC_DA_EFFECT_PARAS_LEN         (20+72*2)
#define VBC_AD_EFFECT_PARAS_LEN         (2+ 43*2)
#define VBC_EFFECT_PROFILE_CNT          (4)

#define CODEC_MAX_VOLUME 9
struct dsp_loop_param {
    int16_t type;
    int16_t delay;//ms
};

#ifdef __cplusplus
extern "C" {
#endif

struct audio_param_file_t{
    int profile;
    const char *tunning_file;
    const char *bin_file;
    const char *src_file;
};

struct audio_record_proc_param {
    RECORDEQ_CONTROL_PARAM_T record_eq;
    DP_CONTROL_PARAM_T dp_control;
    NR_CONTROL_PARAM_T nr_control;
};

struct audio_ap_voice_param{
    int16_t mic_swicth;
};

struct audio_ap_param{
    struct audio_ap_voice_param voice[PROFILE_MODE_AUDIO_BTHSNREC_VOIP-PROFILE_MODE_AUDIO_Handset_GSM+1];
    struct audio_record_proc_param record[AUDIO_RECORD_MODE_MAX];
    struct dsp_loop_param loop[PROFILE_MODE_LOOP_Headset3P_MainMic-PROFILE_MODE_LOOP_Handset_MainMic+1];
};
struct xml_handle {
    param_doc_t param_doc;
    param_group_t param_root;
    char *first_name;
};

#define SET_AUDIO_PARAM_THREAD_NO 10
#define AUDIO_PARAM_THREAD_PROCESS_BUF 128

typedef int  (*AUDIO_SOCKET_PROCESS_FUN)(void *dev, uint8_t *received_buf,
                         int rev_len);

struct socket_handle {

    uint8_t * audio_received_buf;
    uint8_t * audio_cmd_buf;
    uint8_t * time_buf;
    int sockfd;
    int seq;
    int rx_packet_len;
    int rx_packet_total_len;
    bool wire_connected;
    int diag_seq;

    uint8_t * data_buf;
    uint8_t * send_buf;
    int max_len;
    int cur_len;
    int data_state;

    void *res;
    AUDIO_SOCKET_PROCESS_FUN process;
    bool running;
    bool param_sync[SND_AUDIO_PARAM_PROFILE_MAX];
    void *audio_config_xml;
    int update_flag;
};


struct vbc_fw_header {
    char magic[FIRMWARE_MAGIC_MAX_LEN];
    /*total num of profile */
    int32_t num_mode;
    /* total mode num in each profile */
    int32_t len; /* size of each mode in profile */
};

struct param_infor_t {
    int32_t offset[PROFILE_MODE_MAX];
    int32_t param_struct_size;
};

struct param_infor {
    struct param_infor_t data[SND_AUDIO_PARAM_PROFILE_MAX];
    unsigned int param_sn;
};

typedef struct {
    int version;
    int32_t param_struct_size;
    char *data;
    int num_mode;
    struct xml_handle xml;
} AUDIOVBCEQ_PARAM_T;


typedef struct {
    int opt;
    void *res;
} audio_param_dsp_cmd_t;

typedef struct {
    int mode; //0:init first 1:normal 2:tunning
    struct vbc_fw_header header[SND_AUDIO_PARAM_PROFILE_MAX];
    int fd_bin[SND_AUDIO_PARAM_PROFILE_MAX];
    AUDIOVBCEQ_PARAM_T param[SND_AUDIO_PARAM_PROFILE_MAX];
    struct socket_handle tunning;
    int current_param;
    pthread_mutex_t audio_param_lock;

    struct dsp_control_t *agdsp_ctl;//used send cmd to dsp thread
    struct audio_control *dev_ctl;//used control devices in audiotester
    struct audio_config_handle *config;

    struct mixer_ctl *update_mixer[SND_AUDIO_PARAM_PROFILE_MAX];
    struct mixer_ctl *select_mixer[SND_AUDIO_PARAM_PROFILE_MAX];
    struct param_infor *infor;
    unsigned int backup_param_sn;
    unsigned int tunning_param_sn;
    bool audio_param_update;
} AUDIO_PARAM_T;


struct audio_vbc_eq_handle {
    struct audio_record_proc_param audio_record_p[2];
};

struct param_head_t {
    int param_type;
    int message;
    int param_num;
    int param_size;
};

struct sprd_code_param_t {
    int mic_boost;

    int adcl_capture_volume;
    int adcr_capture_volume;

    int spkl_playback_volume[CODEC_MAX_VOLUME];
    int spkr_playback_volume[CODEC_MAX_VOLUME];

    int hpl_playback_volume[CODEC_MAX_VOLUME];
    int hpr_playback_volume[CODEC_MAX_VOLUME];

    int ear_playback_volume[CODEC_MAX_VOLUME];

    int inter_pa_config;
    int inter_hp_pa_config;

    int dacs_playback_volume[CODEC_MAX_VOLUME];
    int dacl_playback_volume[CODEC_MAX_VOLUME];
    int dacr_playback_volume[CODEC_MAX_VOLUME];
};

struct audio_param_mode_t {
    AUDIO_PARAM_PROFILE_MODE_E mode;
    const char *name;
};
struct realtek_extend_header {
    int16_t size;
    uint8_t start_mode;
    uint8_t end_mode;
};
struct realtek_extend_reg {
    int16_t addr;
    int16_t val;
    int16_t mask;
};
struct _realtek_extend {
    struct realtek_extend_header header;
    struct realtek_extend_reg *reg;
};
struct realtek_extend {
    int16_t count;
    int16_t size;
    struct _realtek_extend *data;
};
extern const struct audio_param_file_t  audio_param_file_table[];


int load_xml_handle(struct xml_handle *xmlhandle, const char *xmlpath);
void release_xml_handle(struct xml_handle *xmlhandle);
bool is_file_exist(const char *path);
void free_audio_gain(struct device_usecase_gain *gain);
void dump_data(char *buf, int len);
int save_audio_param_to_bin(AUDIO_PARAM_T *param, int profile);
int get_ele_value(param_group_t Element);
int read_audio_param_for_element(char *data,param_group_t Element);
int upload_audio_param_firmware(AUDIO_PARAM_T * audio_param);
int upload_audio_profile_param_firmware(AUDIO_PARAM_T * audio_param,int profile);
int get_audio_param_mode_name(char *str);
uint8_t get_audio_param_id(const char *name);
int clear_audio_param(AUDIO_PARAM_T  *audio_param);
int reload_sprd_audio_pga_param_withflash(AUDIO_PARAM_T *param);
int reload_sprd_audio_pga_param_withram(AUDIO_PARAM_T *param);
int init_sprd_audio_param(AUDIO_PARAM_T  *audio_param, bool force);
const char * tinymix_get_enum(struct mixer_ctl *ctl);
int free_sprd_audio_pga_param(struct device_usecase_gain *use_gain);
const char * get_audio_param_name(uint8_t param_id);
int reload_sprd_audio_process_param_withflash(AUDIO_PARAM_T *audio_param);
int init_sprd_audio_param_from_xml(AUDIO_PARAM_T *param,int profile);
bool check_updata_audioparam(AUDIO_PARAM_T  *audio_param);
int save_audio_param_infor(struct param_infor *infor);
#ifdef __cplusplus
}
#endif
#endif //_AUDIO_PARAM_T
