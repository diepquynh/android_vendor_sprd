#ifndef AUDIO_OFFLOAD_H
#define AUDIO_OFFLOAD_H
#include "cutils/list.h"
#include "sys/resource.h"
#include <system/thread_defs.h>
#include <cutils/sched_policy.h>
#include <sys/prctl.h>
#include <stdbool.h>
#include "tinycompress/tinycompress.h"
#include "sound/compress_params.h"
#include <hardware/audio.h>
#include "audio_hw.h"
/* Flags used to indicate current application type */
typedef enum {
    AUDIO_OUTPUT_DESC_NONE = 0,
    AUDIO_OUTPUT_DESC_PRIMARY = 0x1,
    AUDIO_OUTPUT_DESC_OFFLOAD = 0x10
} AUDIO_OUTPUT_DESC_T;

typedef enum {
    AUDIO_OFFLOAD_CMD_EXIT,               /* exit compress offload thread loop*/
    AUDIO_OFFLOAD_CMD_DRAIN,              /* send a full drain request to driver */
    AUDIO_OFFLOAD_CMD_PARTIAL_DRAIN,      /* send a partial drain request to driver */
    AUDIO_OFFLOAD_CMD_WAIT_FOR_BUFFER    /* wait for buffer released by driver */
} AUDIO_OFFLOAD_CMD_T;

/* Flags used to indicate current offload playback state */
typedef enum {
    AUDIO_OFFLOAD_STATE_STOPED,
    AUDIO_OFFLOAD_STATE_PLAYING,
    AUDIO_OFFLOAD_STATE_PAUSED
} AUDIO_OFFLOAD_STATE_T;

typedef enum {
    AUDIO_OFFLOAD_MIXER_INVALID = -1,
    AUDIO_OFFLOAD_MIXER_CARD0 = 0,      //ap main
    AUDIO_OFFLOAD_MIXER_CARD1,          //ap compress
    AUDIO_OFFLOAD_MIXER_CARD2          //cp compress-mixer
} AUDIO_OFFLOAD_MIXER_CARD_T;

struct audio_offload_cmd {
    struct listnode node;
    AUDIO_OFFLOAD_CMD_T cmd;
    int data[];
};

#define AUDIO_OFFLOAD_PLAYBACK_VOLUME_MAX 0x2000

#define PRIVATE_VBC_SWITCH            "vb control"
#define VBC_ARM_CHANNELID               2

#endif // AUDIO_OFFLOAD_H

