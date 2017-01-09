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

#define VBC_DSP_PROFILE_UPDATE   "DSP VBC Profile Update"
#define VBC_EQ_PROFILE_UPDATE    "Audio Structure Profile Update"
#define VBC_NXP_PROFILE_UPDATE   "NXP Profile Update"
#define VBC_CODEC_PROFILE_UPDATE   "CODEC Profile Update"
#define VBC_DSP_PROFILE_SELECT   "DSP VBC Profile Select"
#define VBC_EQ_PROFILE_SELECT    "Audio Structure Profile Select"
#define VBC_NXP_PROFILE_SELECT   "NXP Profile Select"
#define VBC_CODEC_PROFILE_SELECT   "CODEC Profile Select"

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
#define DSP_VBC_FIRST_NAME "dsp_vbc"


#define AUDIO_STRUCTURE_XML_TUNNING_PATH "/data/local/media/audio_structure.xml"
#define AUDIO_STRUCTURE_BIN_PATH "/data/local/media/audio_structure.bin"
#define AUDIO_STRUCTURE_XML_PATH "/etc/audio_structure.xml"
#define AUDIO_STRUCTURE_FIRST_NAME "audio_structure"

#define NXP_XML_TUNNING_PATH "/data/local/media/nxp.xml"
#define NXP_BIN_PATH "/data/local/media/nxp.bin"
#define NXP_XML_PATH "/etc/nxp.xml"
#define NXP_FIRST_NAME "nxp"

#define PGA_GAIN_XML_TUNNING_PATH "/data/local/media/audio_pga.xml"
#define PGA_GAIN_BIN_PATH "/data/local/media/audio_pga.bin"
#define PGA_GAIN_XML_PATH "/etc/audio_pga.xml"
#define PGA_GAIN_FIRST_NAME "audio_pga"

#define AUDIO_PROCESS_XML_TUNNING_PATH "/data/local/media/audio_process.xml"
#define AUDIO_PROCESS_BIN_PATH "/data/local/media/audio_process.bin"
#define AUDIO_PROCESS_PATH "/etc/audio_process.xml"
#define AUDIO_PROCESS_FIRST_NAME "audio_process"

#define CODEC_XML_TUNNING_PATH "/data/local/media/codec.xml"
#define CODEC_BIN_PATH "/data/local/media/codec.bin"
#define CODEC_XML_PATH "/etc/codec.xml"
#define CODEC_FIRST_NAME "codec"

#define AUDIO_PARAM_FIRMWARE_NAME "audio_profile"
#define AUDIO_PARAM_FIRMWARE_PATH "/data/local/media/audio_profile"
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
};

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
    PROFILE_MODE_AUDIO_Handset_Loopback,

    PROFILE_MODE_AUDIO_Handsfree_GSM,
    PROFILE_MODE_AUDIO_Handsfree_TDMA,
    PROFILE_MODE_AUDIO_Handsfree_WCDMA_NB,
    PROFILE_MODE_AUDIO_Handsfree_WCDMA_WB,
    PROFILE_MODE_AUDIO_Handsfree_VOLTE_NB,
    PROFILE_MODE_AUDIO_Handsfree_VOLTE_WB,
    PROFILE_MODE_AUDIO_Handsfree_VOIP,
    PROFILE_MODE_AUDIO_Handsfree_Loopback,

    PROFILE_MODE_AUDIO_Headset4P_GSM,
    PROFILE_MODE_AUDIO_Headset4P_TDMA,
    PROFILE_MODE_AUDIO_Headset4P_WCDMA_NB,
    PROFILE_MODE_AUDIO_Headset4P_WCDMA_WB,
    PROFILE_MODE_AUDIO_Headset4P_VOLTE_NB,
    PROFILE_MODE_AUDIO_Headset4P_VOLTE_WB,
    PROFILE_MODE_AUDIO_Headset4P_VOIP,
    PROFILE_MODE_AUDIO_Headset4P_Loopback,

    PROFILE_MODE_AUDIO_Headset3P_GSM,
    PROFILE_MODE_AUDIO_Headset3P_TDMA,
    PROFILE_MODE_AUDIO_Headset3P_WCDMA_NB,
    PROFILE_MODE_AUDIO_Headset3P_WCDMA_WB,
    PROFILE_MODE_AUDIO_Headset3P_VOLTE_NB,
    PROFILE_MODE_AUDIO_Headset3P_VOLTE_WB,
    PROFILE_MODE_AUDIO_Headset3P_VOIP,
    PROFILE_MODE_AUDIO_Headset3P_Loopback,

    PROFILE_MODE_AUDIO_BTHSD8K_GSM,
    PROFILE_MODE_AUDIO_BTHSD8K_TDMA,
    PROFILE_MODE_AUDIO_BTHSD8K_WCDMA,
    PROFILE_MODE_AUDIO_BTHSD8K_VOLTE,
    PROFILE_MODE_AUDIO_BTHSD8K_VOIP,

    PROFILE_MODE_AUDIO_BTHSD16K_GSM,
    PROFILE_MODE_AUDIO_BTHSD16K_TDMA,
    PROFILE_MODE_AUDIO_BTHSD16K_WCDMA,
    PROFILE_MODE_AUDIO_BTHSD16K_VOLTE,
    PROFILE_MODE_AUDIO_BTHSD16K_VOIP,

    PROFILE_MODE_AUDIO_BTHSNRECD8K_GSM,
    PROFILE_MODE_AUDIO_BTHSNRECD8K_TDMA,
    PROFILE_MODE_AUDIO_BTHSNRECD8K_WCDMA,
    PROFILE_MODE_AUDIO_BTHSNRECD8K_VOLTE,
    PROFILE_MODE_AUDIO_BTHSNRECD8K_VOIP,

    PROFILE_MODE_AUDIO_BTHSNRECD16K_GSM,
    PROFILE_MODE_AUDIO_BTHSNRECD16K_TDMA,
    PROFILE_MODE_AUDIO_BTHSNRECD16K_WCDMA,
    PROFILE_MODE_AUDIO_BTHSNRECD16K_VOLTE,
    PROFILE_MODE_AUDIO_BTHSNRECD16K_VOIP,

    PROFILE_MODE_MUSIC_Headset_Playback,
    PROFILE_MODE_MUSIC_Headset_Record,
    PROFILE_MODE_MUSIC_Headset_FM,

    PROFILE_MODE_MUSIC_Handsfree_Playback,
    PROFILE_MODE_MUSIC_Handsfree_Record,
    PROFILE_MODE_MUSIC_Handsfree_FM,

    PROFILE_MODE_MUSIC_Headfree_Playback,
    PROFILE_MODE_MUSIC_Handset_Playback,

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

#ifdef __cplusplus
extern "C" {
#endif

struct audio_param_file_t{
    int profile;
    const char *tunning_file;
    const char *bin_file;
    const char *src_file;
    const char *first_name;
};

struct audio_record_proc_param {
    RECORDEQ_CONTROL_PARAM_T record_eq;
    DP_CONTROL_PARAM_T dp_control;
    NR_CONTROL_PARAM_T nr_control;
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
};


struct vbc_fw_header {
    char magic[FIRMWARE_MAGIC_MAX_LEN];
    /*total num of profile */
    int32_t num_mode;
    /* total mode num in each profile */
    int32_t len; /* size of each mode in profile */
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
    struct tunning_debug *debug;//used for dump socket buffer

    struct mixer_ctl *update_mixer[SND_AUDIO_PARAM_PROFILE_MAX];
    struct mixer_ctl *select_mixer[SND_AUDIO_PARAM_PROFILE_MAX];
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

    int spkl_playback_volume[9];
    int spkr_playback_volume[9];

    int hpl_playback_volume[9];
    int hpr_playback_volume[9];

    int ear_playback_volume[9];

    int inter_pa_config;
    int inter_hp_pa_config;

    int dacl_playback_volume[9];
    int dacr_playback_volume[9];
    int dacs_playback_volume[9];
};

struct audio_param_mode_t {
    AUDIO_PARAM_PROFILE_MODE_E mode;
    const char *name;
};
extern const struct audio_param_file_t  audio_param_file_table[];
int init_audio_param(struct tiny_audio_device *adev);

int load_xml_handle(struct xml_handle *xmlhandle, const char *xmlpath,
                    const char *first_name);
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
int select_audio_param(struct tiny_audio_device *adev,bool force);
int clear_audio_param(AUDIO_PARAM_T  *audio_param);
int reload_sprd_audio_pga_param(AUDIO_PARAM_T *param);
int init_sprd_audio_param(AUDIO_PARAM_T  *audio_param, int force);
const char * tinymix_get_enum(struct mixer_ctl *ctl);
#ifdef __cplusplus
}
#endif
#endif //_AUDIO_PARAM_T
