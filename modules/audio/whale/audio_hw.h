#ifndef _AUDIO_HW_H_
#define _AUDIO_HW_H_
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>

#include <cutils/log.h>
#include <cutils/str_parms.h>
#include <cutils/properties.h>
#include <hardware/hardware.h>
#include <system/audio.h>
#include <hardware/audio.h>
#include <tinyalsa/asoundlib.h>
#include <audio_utils/resampler.h>
#include <tinycompress/tinycompress.h>
#include "audio_offload.h"
#include "audio_debug.h"
#include "ring_buffer.h"
#include "audio_param/dsp_control.h"
#include "audio_param/audio_param.h"
#include "record_process/record_nr_api.h"

#define PRIVATE_VBC_DA_EQ_PROFILE            "da eq profile"
#define PRIVATE_VBC_EQ_SWITCH                "eq switch"
#define PRIVATE_VBC_DA_EQ_SWITCH             "da eq switch"
#define PRIVATE_VBC_EQ_UPDATE                "eq update"
#define PRIVATE_VBC_CONTROL              "vb control"

/* ALSA cards for sprd */
#define CARD_SPRDPHONE "sprdphone4adnc"
#define CARD_VAUDIO    "VIRTUAL AUDIO"
#define CARD_VAUDIO_W  "VIRTUAL AUDIO W"
#define CARD_VAUDIO_LTE  "saudiolte"
#define CARD_SCO    "saudiovoip"
#define CARD_BT_SCO    "all-i2s"

typedef enum {
    DAI_ID_NORMAL_OUTDSP_PLAYBACK=0,
    DAI_ID_NORMAL_OUTDSP_CAPTURE,
    DAI_ID_NORMAL_WITHDSP,
    DAI_ID_FAST_P,
    DAI_ID_OFFLOAD,
    DAI_ID_VOICE,
    DAI_ID_VOIP,
    DAI_ID_FM,
    DAI_ID_FM_C_WITHDSP,
    DAI_ID_VOICE_CAPTURE,
    DAI_ID_LOOP ,
} VBC_DAI_ID_T;


/* ALSA ports for sprd */
#define PORT_MM 0
#define PORT_MODEM 1
#define PORT_FM 4
#define PORT_MM_C 0

typedef enum {
    AUDIO_HW_APP_INVALID = -1,
    AUDIO_HW_APP_PRIMARY = 0,
    AUDIO_HW_APP_OFFLOAD =1,
    AUDIO_HW_APP_CALL =2,
    AUDIO_HW_APP_FM =3,
    AUDIO_HW_APP_VOIP =4,
    AUDIO_HW_APP_VAUDIO =5,
    AUDIO_HW_APP_FAST=6,
    AUDIO_HW_APP_DIRECT=7,
    AUDIO_HW_APP_NORMAL_RECORD=8,
    AUDIO_HW_APP_DSP_LOOP=9,
} AUDIO_HW_APP_T;

typedef enum {
    AUD_NET_GSM_MODE,
    AUD_NET_TDMA_MODE,
    AUD_NET_WCDMA_MODE,
    AUD_NET_WCDMA_NB_MODE,
    AUD_NET_WCDMA_WB_MODE,
    AUD_NET_VOLTE_MODE,
    AUD_NET_VOLTE_NB_MODE,
    AUD_NET_VOLTE_WB_MODE,
    AUD_NET_MAX,
} AUDIO_NET_M;

static const struct pcm_config default_normal_config = {
    .channels = 2,
    .rate = 48000,
    .period_size = 1280,
    .period_count = 2,
    .format = PCM_FORMAT_S16_LE,
};

/* 48000 pcm data */
static const unsigned char s_pcmdata_mono[] = {
    0x00,0x00,0x26,0x40,0x60,0x08,0x9a,0x3f,0x9b,0x10,0xf7,0x3d,0x8c,0x18,0x44,0x3b,
    0x13,0x20,0x8f,0x37,0x0e,0x27,0xe5,0x32,0x5c,0x2d,0x5d,0x2d,0xe5,0x32,0x0d,0x27,
    0x8e,0x37,0x13,0x20,0x44,0x3b,0x8d,0x18,0xf6,0x3d,0x9a,0x10,0x9a,0x3f,0x5f,0x08,
    0x26,0x40,0x00,0x00,0x9a,0x3f,0xa0,0xf7,0xf7,0x3d,0x66,0xef,0x44,0x3b,0x73,0xe7,
    0x8e,0x37,0xed,0xdf,0xe5,0x32,0xf3,0xd8,0x5c,0x2d,0xa4,0xd2,0x0d,0x27,0x1b,0xcd,
    0x13,0x20,0x72,0xc8,0x8d,0x18,0xbc,0xc4,0x9a,0x10,0x0a,0xc2,0x5f,0x08,0x66,0xc0,
    0x00,0x00,0xda,0xbf,0xa1,0xf7,0x66,0xc0,0x65,0xef,0x09,0xc2,0x73,0xe7,0xbc,0xc4,
    0xed,0xdf,0x72,0xc8,0xf2,0xd8,0x1b,0xcd,0xa4,0xd2,0xa3,0xd2,0x1b,0xcd,0xf4,0xd8,
    0x72,0xc8,0xed,0xdf,0xbc,0xc4,0x73,0xe7,0x09,0xc2,0x66,0xef,0x66,0xc0,0xa1,0xf7,
    0xda,0xbf,0x00,0x00,0x67,0xc0,0x5f,0x08,0x09,0xc2,0x9a,0x10,0xbc,0xc4,0x8d,0x18,
    0x71,0xc8,0x12,0x20,0x1b,0xcd,0x0d,0x27,0xa3,0xd2,0x5d,0x2d,0xf3,0xd8,0xe4,0x32,
    0xed,0xdf,0x8e,0x37,0x74,0xe7,0x45,0x3b,0x65,0xef,0xf7,0x3d,0xa0,0xf7,0x99,0x3f,
};

/* 48000 pcm data */
static const unsigned char s_pcmdata_left[] = {
    0x01,0x00,0x00,0x00,0x5f,0x08,0x00,0x00,0x9a,0x10,0x00,0x00,0x8c,0x18,0x00,0x00,
    0x14,0x20,0x00,0x00,0x0d,0x27,0x00,0x00,0x5d,0x2d,0x00,0x00,0xe5,0x32,0x00,0x00,
    0x8f,0x37,0x00,0x00,0x44,0x3b,0x00,0x00,0xf7,0x3d,0x00,0x00,0x99,0x3f,0x00,0x00,
    0x27,0x40,0x00,0x00,0x9a,0x3f,0x00,0x00,0xf7,0x3d,0x00,0x00,0x44,0x3b,0x00,0x00,
    0x8e,0x37,0x00,0x00,0xe5,0x32,0x00,0x00,0x5c,0x2d,0x00,0x00,0x0d,0x27,0x00,0x00,
    0x13,0x20,0x00,0x00,0x8d,0x18,0x00,0x00,0x9a,0x10,0x00,0x00,0x60,0x08,0x00,0x00,
    0x01,0x00,0x00,0x00,0xa0,0xf7,0x00,0x00,0x66,0xef,0x00,0x00,0x73,0xe7,0x00,0x00,
    0xed,0xdf,0x00,0x00,0xf2,0xd8,0x00,0x00,0xa4,0xd2,0x00,0x00,0x1b,0xcd,0x00,0x00,
    0x71,0xc8,0x00,0x00,0xbb,0xc4,0x00,0x00,0x09,0xc2,0x00,0x00,0x66,0xc0,0x00,0x00,
    0xd9,0xbf,0x00,0x00,0x66,0xc0,0x00,0x00,0x08,0xc2,0x00,0x00,0xbc,0xc4,0x00,0x00,
    0x72,0xc8,0x00,0x00,0x1c,0xcd,0x00,0x00,0xa4,0xd2,0x00,0x00,0xf2,0xd8,0x00,0x00,
    0xed,0xdf,0x00,0x00,0x74,0xe7,0x00,0x00,0x65,0xef,0x00,0x00,0xa1,0xf7,0x00,0x00,
};

/* 48000 pcm data */
static const unsigned char s_pcmdata_right[] = {
    0x00,0x00,0x26,0x40,0x00,0x00,0x9a,0x3f,0x00,0x00,0xf6,0x3d,0x00,0x00,0x44,0x3b,
    0x00,0x00,0x8e,0x37,0x00,0x00,0xe5,0x32,0x00,0x00,0x5d,0x2d,0x00,0x00,0x0d,0x27,
    0x00,0x00,0x14,0x20,0x00,0x00,0x8d,0x18,0x00,0x00,0x9a,0x10,0x00,0x00,0x60,0x08,
    0x00,0x00,0x00,0x00,0x00,0x00,0xa1,0xf7,0x00,0x00,0x66,0xef,0x00,0x00,0x73,0xe7,
    0x00,0x00,0xec,0xdf,0x00,0x00,0xf3,0xd8,0x00,0x00,0xa4,0xd2,0x00,0x00,0x1b,0xcd,
    0x00,0x00,0x72,0xc8,0x00,0x00,0xbc,0xc4,0x00,0x00,0x09,0xc2,0x00,0x00,0x66,0xc0,
    0x00,0x00,0xdb,0xbf,0x00,0x00,0x66,0xc0,0x00,0x00,0x09,0xc2,0x00,0x00,0xbc,0xc4,
    0x00,0x00,0x71,0xc8,0x00,0x00,0x1b,0xcd,0x00,0x00,0xa4,0xd2,0x00,0x00,0xf3,0xd8,
    0x00,0x00,0xee,0xdf,0x00,0x00,0x73,0xe7,0x00,0x00,0x65,0xef,0x00,0x00,0xa0,0xf7,
    0x00,0x00,0x00,0x00,0x00,0x00,0x60,0x08,0x00,0x00,0x9a,0x10,0x00,0x00,0x8c,0x18,
    0x00,0x00,0x13,0x20,0x00,0x00,0x0d,0x27,0x00,0x00,0x5d,0x2d,0x00,0x00,0xe5,0x32,
    0x00,0x00,0x8e,0x37,0x00,0x00,0x44,0x3b,0x00,0x00,0xf7,0x3d,0x00,0x00,0x9a,0x3f,
};
/**
    container_of - cast a member of a structure out to the containing structure
    @ptr:    the pointer to the member.
    @type:   the type of the container struct this is embedded in.
    @member: the name of the member within the struct.

*/
#define container_of(ptr, type, member) ({      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})


/* constraint imposed by VBC: all period sizes must be multiples of 160 */
#define VBC_BASE_FRAME_COUNT 160
/* number of base blocks in a short period (low latency) */
#define SHORT_PERIOD_MULTIPLIER 8 /* 29 ms */
/* number of frames per short period (low latency) */
#define SHORT_PERIOD_SIZE (VBC_BASE_FRAME_COUNT * SHORT_PERIOD_MULTIPLIER)
/* number of short periods in a long period (low power) */
#define LONG_PERIOD_MULTIPLIER 3 /* 87 ms */
/* number of frames per long period (low power) */
#define LONG_PERIOD_SIZE (SHORT_PERIOD_SIZE * LONG_PERIOD_MULTIPLIER)
/* number of periods for low power playback */
#define PLAYBACK_LONG_PERIOD_COUNT 2
/* number of pseudo periods for low latency playback */
#define PLAYBACK_SHORT_PERIOD_COUNT 4
/* number of periods for capture */
#define CAPTURE_PERIOD_COUNT 2
/* minimum sleep time in out_write() when write threshold is not reached */
#define MIN_WRITE_SLEEP_US 5000

#define RESAMPLER_BUFFER_FRAMES (SHORT_PERIOD_SIZE * 2)
#define RESAMPLER_BUFFER_SIZE (4 * RESAMPLER_BUFFER_FRAMES)

#define DEFAULT_OUT_SAMPLING_RATE 48000
#define DEFAULT_IN_SAMPLING_RATE  8000
#define DEFAULT_FM_SRC_SAMPLING_RATE 32000

/* sampling rate when using MM low power port */
#define MM_LOW_POWER_SAMPLING_RATE 48000
/* sampling rate when using MM full power port */
#define MM_FULL_POWER_SAMPLING_RATE 48000
/* sampling rate when using VX port for narrow band */
#define VX_NB_SAMPLING_RATE 8000
/* sampling rate when using VX port for wide band */
#define VX_WB_SAMPLING_RATE 16000

#define FM_VOLUME_MAX 15

#define RECORD_POP_MIN_TIME    500   // ms

#define MAX_STOP_THRESHOLD ((unsigned int)-1)/2-1

#define VOIP_CAPTURE_STREAM     0x1
#define VOIP_PLAYBACK_STREAM    0x2

#define MAX_AT_CMD_LENGTH   32
#define MAX_AT_CMD_TYPE  8

#define BT_SCO_UPLINK_IS_STARTED        (1 << 0)
#define BT_SCO_DOWNLINK_IS_EXIST        (1 << 1)
#define BT_SCO_DOWNLINK_OPEN_FAIL       (1 << 8)

typedef int  (*AUDIO_OUTPUT_STANDBY_FUN)(void *adev,void *out,AUDIO_HW_APP_T audio_app_type);

struct stream_routing_manager {
    pthread_t   routing_switch_thread;
    bool        is_exit;
    sem_t       device_switch_sem;
};

typedef struct {
    timer_t timer_id;
    bool created;
} voip_timer_t;


struct bt_sco_thread_manager {
    bool             thread_is_exit;
    pthread_t        dup_thread;
    pthread_mutex_t  dup_mutex;
    pthread_mutex_t  cond_mutex;
    pthread_cond_t   cond;
    sem_t            dup_sem;
    volatile bool    dup_count;
    volatile bool    dup_need_start;
};


typedef enum {
    AUDIO_CP_USECASE_VOICE  = 0,
    AUDIO_CP_USECASE_VT,
    AUDIO_CP_USECASE_VOIP_1,
    AUDIO_CP_USECASE_VOIP_2,
    AUDIO_CP_USECASE_VOIP_3,
    AUDIO_CP_USECASE_VOIP_4,
    AUDIO_CP_USECASE_MAX,
} audio_cp_usecase_t;

enum aud_dev_test_m {
    TEST_AUDIO_IDLE =0,
    TEST_AUDIO_OUT_DEVICES,
    TEST_AUDIO_IN_DEVICES,
    TEST_AUDIO_IN_OUT_LOOP,
    TEST_AUDIO_OUT_IN_LOOP,
};

typedef enum {
    AUDIO_NET_GSM =0,
    AUDIO_NET_TDMA,
    AUDIO_NET_WCDMA_NB,
    AUDIO_NET_WCDMA_WB,
    AUDIO_NET_VOLTE_NB,
    AUDIO_NET_VOLTE_WB,
    AUDIO_NET_LOOP,
}aud_net_m;

struct audiotest_config
{
    struct pcm_config config;
    unsigned char * file;
    int fd;
    void * function;
    int card;
};

struct in_test_t
{
    struct audiotest_config config;
    pthread_t thread;
    int state;
    sem_t   sem;
    bool isloop;
    struct dev_test_t *dev_test;
    struct ring_buffer *ring_buf;
};

struct out_test_t
{
    struct audiotest_config config;
    pthread_t thread;
    int state;
    sem_t   sem;
    bool isloop;
    struct dev_test_t *dev_test;
    struct ring_buffer *ring_buf;
};

struct loop_test_t
{
    struct audiotest_config config;
    pthread_t thread;
    int state;
    sem_t   sem;
    struct tiny_audio_device *adev;
};

struct dev_test_t {
    struct in_test_t in_test;
    struct out_test_t  out_test;
    struct loop_test_t  loop_test;
    int state;
    struct tiny_audio_device *adev;
    int test_mode;
};

struct loop_ctl_t
{
    pthread_t thread;
    sem_t   sem;
    bool is_exit;
    pthread_mutex_t lock;
    void *dev;
    bool state;
    struct pcm *pcm;
    struct ring_buffer *ring_buf;
};

struct dsp_loop_t {
    struct loop_ctl_t in;
    struct loop_ctl_t out;
    bool state;
};

struct dev_bluetooth_t {
    bool bluetooth_nrec;
    int samplerate;
};

struct tiny_audio_device {
    struct audio_hw_device hw_device;

    pthread_mutex_t lock;       /* see note below on mutex acquisition order */
    struct mixer *mixer;
    struct audio_control *dev_ctl;
    struct dev_test_t dev_test;
    audio_mode_t call_mode;
    audio_devices_t out_devices;
    audio_devices_t in_devices;
    bool low_power;
    bool mic_mute;
    bool bluetooth_nrec;
    int bluetooth_type;
    bool master_mute;
    int routeDev;
    int audio_outputs_state;
    float voice_volume;

    AUDIO_PARAM_T  audio_param;
    struct audio_debug_handle debug;

    volatile int bt_sco_state;
    struct bt_sco_thread_manager bt_sco_manager;
    struct audio_control *control;
    int loop_delay;
    int loop_type;
    struct pcm *pcm_modem_dl;
    struct dev_bluetooth_t bt_infor;
    int offload_on;

    struct listnode active_out_list;
    struct listnode active_input_list;
    int stream_status;
    struct dsp_loop_t loop_ctl;
    struct dsp_control_t agdsp_ctl;
    bool voip_start;
    bool fm_record;

    aud_net_m net_mode;

    struct audio_config_handle config;
#ifdef LOCAL_SOCKET_SERVER
    struct socket_handle local_socket;
#endif

    int current_param;
};

struct tiny_stream_out {
    struct audio_stream_out stream;

    pthread_mutex_t lock;       /* see note below on mutex acquisition order */
    struct pcm_config *config;
    int requested_rate;
    struct pcm *pcm;
    struct resampler_itfe *resampler;
    char *buffer;

    int is_voip;
    int is_bt_sco;

    struct listnode node;
    AUDIO_OUTPUT_STANDBY_FUN standby_fun;

    bool standby;
    audio_devices_t devices;
    audio_output_flags_t flags;
    bool low_power;
    struct tiny_audio_device *dev;

    int written;
    struct compr_config compress_config;
    struct compress *compress;
    struct compr_gapless_mdata gapless_mdata;
    stream_callback_t audio_offload_callback;
    void *audio_offload_cookie;
    AUDIO_HW_APP_T audio_app_type;         /* 0:primary; 1:offload */
    AUDIO_OFFLOAD_STATE_T
    audio_offload_state;    /* AUDIO_OFFLOAD_STOPED; PLAYING; PAUSED */
    unsigned int offload_format;
    unsigned int offload_samplerate;
    unsigned int offload_channel_mask;
    bool is_offload_compress_started;      /* flag indicates compress_start state */
    bool is_offload_need_set_metadata;     /* flag indicates to set the metadata to driver */
    bool is_audio_offload_thread_blocked;  /* flag indicates processing the command */
    int is_offload_nonblocking;            /* flag indicates to use non-blocking write */
    pthread_cond_t audio_offload_cond;
    pthread_t audio_offload_thread;
    struct listnode audio_offload_cmd_list;

    /*used for voice call*/
    struct pcm *pcm_modem_ul;
    struct pcm *pcm_modem_dl;
};

struct tiny_stream_in {
    struct audio_stream_in stream;

    pthread_mutex_t lock;       /* see note below on mutex acquisition order */
    struct pcm_config *config;
    struct pcm *pcm;
    struct pcm *mux_pcm;
    bool is_fm;
    int is_voip;
    int is_bt_sco;
    int standby;
    unsigned int requested_rate;
    unsigned int requested_channels;
    struct resampler_itfe *resampler;
    struct resampler_buffer_provider buf_provider;
    size_t frames_in;
    int16_t *buffer;
    int16_t *nr_buffer;
    audio_devices_t devices;
    bool pop_mute;
    int pop_mute_bytes;
    int16_t *proc_buf;
    size_t proc_buf_size;
    size_t proc_frames_in;
    int read_status;

    bool active_rec_proc;
    record_nr_handle rec_nr_handle;
    struct tiny_audio_device *dev;

    struct listnode node;
    AUDIO_OUTPUT_STANDBY_FUN standby_fun;
    AUDIO_HW_APP_T audio_app_type;         /* 0:primary; 1:offload */
};
bool init_rec_process(struct audio_stream_in *stream, int sample_rate);
int aud_rec_do_process(void *buffer, size_t bytes, void *tmp_buffer,
                              size_t tmp_buffer_bytes);
void force_out_standby(struct tiny_audio_device *adev,AUDIO_HW_APP_T audio_app_type);
void force_in_standby(struct tiny_audio_device *adev,AUDIO_HW_APP_T audio_app_type);
int GetAudio_InMode_number_from_device(int in_dev);
#endif //_AUDIO_HW_H_
