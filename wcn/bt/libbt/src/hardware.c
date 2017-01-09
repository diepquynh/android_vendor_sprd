/******************************************************************************
 *
 *  Copyright (C) 2016 Spreadtrum Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  Filename:      hardware.c
 *
 *  Description:   Contains controller-specific functions, like
 *                      firmware patch download
 *                      low power mode operations
 *
 ******************************************************************************/

#define LOG_TAG "bt_hwcfg"

#include <utils/Log.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>
#include <cutils/properties.h>
#include <stdlib.h>
#include <string.h>
#include "bt_hci_bdroid.h"
#include "bt_vendor_sprd.h"
#include "hci_audio.h"
#include "userial.h"
#include "userial_vendor.h"
#include "comm.h"
#include "upio.h"


/******************************************************************************
**  Constants & Macros
******************************************************************************/

#ifndef BTHW_DBG
#define BTHW_DBG TRUE
#endif

#if (BTHW_DBG == TRUE)
#define BTHWDBG(param, ...) {ALOGD(param, ## __VA_ARGS__);}
#else
#define BTHWDBG(param, ...) {}
#endif

#define SCO_INTERFACE_PCM 0
#define SCO_INTERFACE_I2S 1

/* one byte is for enable/disable
      next 2 bytes are for codec type */
#define SCO_CODEC_PARAM_SIZE 3

/******************************************************************************
**  Local type definitions
******************************************************************************/

/* low power mode parameters */
typedef struct {
    uint8_t sleep_mode;                     /* 0(disable),1(UART),9(H5) */
    uint8_t host_stack_idle_threshold;      /* Unit scale 300ms/25ms */
    uint8_t host_controller_idle_threshold; /* Unit scale 300ms/25ms */
    uint8_t bt_wake_polarity;               /* 0=Active Low, 1= Active High */
    uint8_t host_wake_polarity;             /* 0=Active Low, 1= Active High */
    uint8_t allow_host_sleep_during_sco;
    uint8_t combine_sleep_mode_and_lpm;
    uint8_t enable_uart_txd_tri_state; /* UART_TXD Tri-State */
    uint8_t sleep_guard_time;          /* sleep guard time in 12.5ms */
    uint8_t wakeup_guard_time;         /* wakeup guard time in 12.5ms */
    uint8_t txd_config;                /* TXD is high in sleep state */
    uint8_t pulsed_host_wake;          /* pulsed host wake if mode = 1 */
} bt_lpm_param_t;

/* Firmware re-launch settlement time */
typedef struct {
    const char *chipset_name;
    const uint32_t delay_time;
} fw_settlement_entry_t;

/******************************************************************************
**  Externs
******************************************************************************/

void hw_config_cback(void *p_evt_buf);

/******************************************************************************
**  Static variables
******************************************************************************/

static bt_lpm_param_t lpm_param = {LPM_SLEEP_MODE,
                                   LPM_IDLE_THRESHOLD,
                                   LPM_HC_IDLE_THRESHOLD,
                                   LPM_BT_WAKE_POLARITY,
                                   LPM_HOST_WAKE_POLARITY,
                                   LPM_ALLOW_HOST_SLEEP_DURING_SCO,
                                   LPM_COMBINE_SLEEP_MODE_AND_LPM,
                                   LPM_ENABLE_UART_TXD_TRI_STATE,
                                   0, /* not applicable */
                                   0, /* not applicable */
                                   0, /* not applicable */
                                   LPM_PULSED_HOST_WAKE
                                  };

/******************************************************************************
**  Controller Initialization Static Functions
******************************************************************************/

/*******************************************************************************
**
** Function         hw_config_cback
**
** Description      Callback function for controller configuration
**
** Returns          None
**
*******************************************************************************/
void hw_config_cback(void *p_mem)
{
    UNUSED(p_mem);
}

/******************************************************************************
**   LPM Static Functions
******************************************************************************/

/*****************************************************************************
**   Hardware Configuration Interface Functions
*****************************************************************************/

/*******************************************************************************
**
** Function        hw_lpm_enable
**
** Description     Enalbe/Disable LPM
**
** Returns         TRUE/FALSE
**
*******************************************************************************/
uint8_t hw_lpm_enable(uint8_t turn_on)
{
    UNUSED(turn_on);
    bt_vendor_cbacks->lpm_cb(BT_VND_OP_RESULT_SUCCESS);
    return 0;
}

/*******************************************************************************
**
** Function        hw_lpm_get_idle_timeout
**
** Description     Calculate idle time based on host stack idle threshold
**
** Returns         idle timeout value
**
*******************************************************************************/
uint32_t hw_lpm_get_idle_timeout(void)
{
    uint32_t timeout_ms;

    /* set idle time to be LPM_IDLE_TIMEOUT_MULTIPLE times of
     * host stack idle threshold (in 300ms/25ms)
     */
    timeout_ms =
        (uint32_t)lpm_param.host_stack_idle_threshold * LPM_IDLE_TIMEOUT_MULTIPLE;

    timeout_ms *= 300;

    return timeout_ms;
}

/*******************************************************************************
**
** Function        hw_lpm_set_wake_state
**
** Description     Assert/Deassert BT_WAKE
**
** Returns         None
**
*******************************************************************************/
void hw_lpm_set_wake_state(uint8_t wake_assert)
{
    uint8_t state = (wake_assert) ? UPIO_ASSERT : UPIO_DEASSERT;

    upio_set(UPIO_BT_WAKE, state, lpm_param.bt_wake_polarity);
}

/*****************************************************************************
**   Sample Codes Section
*****************************************************************************/

#if (HW_END_WITH_HCI_RESET == TRUE)
/*******************************************************************************
**
** Function         hw_epilog_cback
**
** Description      Callback function for Command Complete Events from HCI
**                  commands sent in epilog process.
**
** Returns          None
**
*******************************************************************************/
void hw_epilog_cback(void *p_mem)
{
    HC_BT_HDR *p_evt_buf = (HC_BT_HDR *)p_mem;
    uint8_t *p, status;
    uint16_t opcode;

    status = *((uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_STATUS_RET_BYTE);
    p = (uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_OPCODE;
    STREAM_TO_UINT16(opcode, p);

    BTHWDBG("%s Opcode:0x%04X Status: %d", __FUNCTION__, opcode, status);

    if (bt_vendor_cbacks) {
        /* Must free the RX event buffer */
        bt_vendor_cbacks->dealloc(p_evt_buf);

        /* Once epilog process is done, must call epilog_cb callback
           to notify caller */
        bt_vendor_cbacks->epilog_cb(BT_VND_OP_RESULT_SUCCESS);
    }
}

/*******************************************************************************
**
** Function         hw_epilog_process
**
** Description      Sample implementation of epilog process
**
** Returns          None
**
*******************************************************************************/
void hw_epilog_process(void)
{

    BTHWDBG("hw_epilog_process");

    const bt_adapter_module_t *adapter_module = get_adapter_module();
    if (adapter_module && adapter_module->epilog_process) {
        adapter_module->epilog_process();
    } else {
        HC_BT_HDR *p_buf = NULL;
        uint8_t *p;
        /* Sending a HCI_RESET */
        if (bt_vendor_cbacks) {
            /* Must allocate command buffer via HC's alloc API */
            p_buf = (HC_BT_HDR *)bt_vendor_cbacks->alloc(BT_HC_HDR_SIZE +
                                                         HCI_CMD_PREAMBLE_SIZE);
        }

        if (p_buf) {
            p_buf->event = MSG_STACK_TO_HC_HCI_CMD;
            p_buf->offset = 0;
            p_buf->layer_specific = 0;
            p_buf->len = HCI_CMD_PREAMBLE_SIZE;

            p = (uint8_t *)(p_buf + 1);
            UINT16_TO_STREAM(p, HCI_RESET);
            *p = 0; /* parameter length */

            /* Send command via HC's xmit_cb API */
            bt_vendor_cbacks->xmit_cb(HCI_RESET, p_buf, hw_epilog_cback);
        } else {
            if (bt_vendor_cbacks) {
                ALOGE("vendor lib epilog process aborted [no buffer]");
                bt_vendor_cbacks->epilog_cb(BT_VND_OP_RESULT_FAIL);
            }
        }
    }
}

int hw_preload_pskey(void *arg)
{
    const bt_adapter_module_t *adapter_module = get_adapter_module();
#if (BT_VND_STACK_PRELOAD == TRUE)
    UNUSED(arg);
    return adapter_module->pskey_preload(NULL);

#else
    return adapter_module->pskey_preload(arg);
#endif
}

#endif  // (HW_END_WITH_HCI_RESET == TRUE)
