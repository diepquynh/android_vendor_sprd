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
#include <system/audio.h>
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

 int out_offload_set_callback(struct audio_stream_out *stream,
                                    stream_callback_t callback, void *cookie);
 int out_offload_pause(struct audio_stream_out *stream);
 int out_offload_resume(struct audio_stream_out *stream);
 int out_offload_drain(struct audio_stream_out *stream,
                             audio_drain_type_t type );
 int out_offload_flush(struct audio_stream_out *stream);
  ssize_t out_write_compress(struct audio_stream_out *out,
                                  const void *buffer,
                                  size_t bytes);
  bool audio_is_offload_support_format(audio_format_t format);
 int audio_get_offload_codec_id(audio_format_t format);
 int audio_start_compress_output(struct audio_stream_out *out);
 int audio_get_compress_metadata(struct audio_stream_out *out,
                                       struct str_parms *parms);
 int audio_send_offload_cmd(struct audio_stream_out *out,
                                  AUDIO_OFFLOAD_CMD_T command);
 void *audio_offload_thread_loop(struct audio_stream_out *param);
 int audio_offload_create_thread(struct audio_stream_out *out);
 int audio_offload_destroy_thread(struct audio_stream_out *out);
 AUDIO_OFFLOAD_MIXER_CARD_T audio_get_offload_mixer_card(struct audio_stream_out *out);
 void dump_pcm(const void *pBuffer, size_t aInBufSize);

#endif // AUDIO_OFFLOAD_H

