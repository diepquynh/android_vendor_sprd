#include<stdbool.h>
#ifndef _AUDIO_DEBUG_H_
#define _AUDIO_DEBUG_H_
#ifdef __cplusplus
extern "C" {
#endif
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
};

struct record_dump {
    struct param_pcm_dump_handle normal;
    struct param_pcm_dump_handle vbc;
    struct param_pcm_dump_handle process;
    struct param_pcm_dump_handle nr;
    struct param_pcm_dump_handle dsploop;
};

struct audio_debug_handle {
    struct tunning_debug tunning;
    int log_level;
    struct record_dump record;
    struct playback_dump playback;
    char *card_name;
};

struct audio_config_handle {
    bool record_nr_enable;
    bool record_process_enable;
};

extern int log_level;
#define LOG_V(...)  ALOGV_IF(log_level >= 5,__VA_ARGS__);
#define LOG_D(...)  ALOGD_IF(log_level >= 4,__VA_ARGS__);
#define LOG_I(...)  ALOGI_IF(log_level >= 3,__VA_ARGS__);
#define LOG_W(...)  ALOGW_IF(log_level >= 2,__VA_ARGS__);
#define LOG_E(...)  ALOGE_IF(log_level >= 1,__VA_ARGS__);
#define AUDIO_DEBUG_CONFIG_PATH "/etc/audio_hal_debug.xml"
#define AUDIO_DEBUG_CONFIG_TUNNING_PATH "/data/local/media/audio_hal_debug.xml"
#define AUDIO_DEBUG_CONFIG_FIRSTNAME "audio_debug"
#define DEBUG_POINT  do{LOG_I("%s %d",__func__,__LINE__);}while(0)

int parse_audio_debug(struct tiny_audio_device *adev);
#define AUDIO_EXT_CONTROL_PIPE "/dev/pipe/mmi.audio.ctrl"
#define AUDIO_EXT_CONTROL_PIPE_MAX_BUFFER_SIZE  1024
#ifdef __cplusplus
}
#endif
#endif
