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

#ifndef _DSP_CONTROL_H_
#define _DSP_CONTROL_H_
#include <pthread.h>
#include <stdint.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <stdbool.h>
#include <cutils/str_parms.h>
#include "audio_control.h"

#define AGDSP_CTL_PIPE "/dev/audio_pipe"

struct dsp_routing_manager {
    pthread_t thread_id;
    bool is_exit;
    sem_t sem;
    int cmd;

    void*  parameter;
    pthread_mutex_t lock;
};

typedef enum {
    AGDSP_INVALID  = 0x0,
    AGDSP_STOP     = 0x1,
    AGDSP_LOAD     = 0x2,
    AGDSP_START    = 0x3,
    AGDSP_ASSERT   = 0x4,
} AGDSP_STATU;

struct dsp_smsg {
    uint16_t        command;    /* command */
    uint16_t        channel;    /* channel index */
    uint32_t        parameter0;    /* msg parameter0 */
    uint32_t        parameter1;    /* msg parameter1 */
    uint32_t        parameter2;    /* msg parameter2 */
    uint32_t        parameter3;    /* msg parameter3 */
};

typedef enum {
    AUDIO_CTL_INVALID  = 0x0,
    AUDIO_CTL_START_VOICE     = 0x1,
    AUDIO_CTL_STOP_VOICE     = 0x2,
    AUDIO_CTL_CLOSE_CODEC    = 0x3,
    AUDIO_TESTER_UPDATAE_AUDIO_PARAM_TO_RAM    = 0x4,
    AUDIO_TESTER_UPDATAE_AUDIO_PARAM_TO_FLASH    = 0x5,
    RIL_NET_MODE_CHANGE   = 0x6,

} AUDIO_CTL_CMD;

struct dsp_control_t {
    struct dsp_routing_manager rx;
    struct dsp_routing_manager tx;
    pthread_mutex_t lock;

    int agdsp_pipd_fd;

    AGDSP_STATU status;
    void *dev;
    struct dsp_smsg msg;

    struct mixer_ctl *dsp_sleep_ctl;
    bool agdsp_sleep_status;
    int fd_modemd_notify;
    int fd_dsp_assert_mem;
    int fd_dsp_assert_log;
    int fd_dsp_assert_pcm;
    void * dev_ctl;
    int auto_reset_dsp;
    int reset_modem;
    int dsp_assert_force_notify;
};

typedef enum {
    UART_LOG     = 0x1,
    USB_LOG     = 0x2,
} DSPBOOT_LOG;

typedef enum {
    AGDSP_CMD_NET_MSG = 0x0,
    AGDSP_CMD_ASSERT = 0x25,
    AGDSP_CMD_TIMEOUT=0x26,
    AGDSP_CMD_STATUS_CHECK     = 0x1234,
    AGDSP_CMD_BOOT_OK =0xbeee,
} AGDSP_CMD;

#define AGDSP_MSG_SIZE  sizeof(struct dsp_smsg)

#ifdef __cplusplus
extern "C" {
#endif

int set_net_mode(void * dev,struct str_parms *parms,int mode, char * val);
void * dsp_ctrl_open(struct audio_control *dev_ctl);
void dsp_ctrl_close(struct dsp_control_t * dsp_ctl);
int dsp_sleep_ctrl(struct dsp_control_t * dsp_ctl ,bool on_off);
int agdsp_send_msg(struct dsp_control_t * dsp_ctl,struct dsp_smsg *msg);

#ifdef __cplusplus
}
#endif
#endif
