/*
 * Copyright 2012 The Android Open Source Project
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
#include "bt_hci_bdroid.h"
#include "bt_vendor_sprd.h"

#if (HW_NEED_END_WITH_HCI_RESET == TRUE)
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
    HC_BT_HDR *p_evt_buf = (HC_BT_HDR *) p_mem;
    char        *p_name, *p_tmp;
    uint8_t     *p, status;
    uint16_t    opcode;

    status = *((uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_STATUS_RET_BYTE);
    p = (uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_OPCODE;
    STREAM_TO_UINT16(opcode,p);

    ALOGI("%s Opcode:0x%04X Status: %d", __FUNCTION__, opcode, status);

    if (bt_vendor_cbacks)
    {
        /* Must free the RX event buffer */
        bt_vendor_cbacks->dealloc(p_evt_buf);

        /* Once epilog process is done, must call callback to notify caller */
        bt_vendor_cbacks->epilog_cb(BT_VND_OP_RESULT_SUCCESS);
    }
}

void hw_pskey_cback(void *p_mem)
{
    HC_BT_HDR *p_evt_buf = (HC_BT_HDR *) p_mem;
    uint8_t code = *((uint8_t *)(p_evt_buf + 1));
    uint8_t len = *((uint8_t *)(p_evt_buf + 1) + 1);
    uint8_t status = *((uint8_t *)(p_evt_buf + 1) + 2);

    ALOGI("%s pskey response: [0x%02X, 0x%02X, 0x%02X]", __func__, code, len, status);
    if (bt_vendor_cbacks)
    {
        /* Must free the RX event buffer */
        bt_vendor_cbacks->dealloc(p_evt_buf);
    }
    ALOGI("dealloc psk");

    sprd_pskey_response_cb(status == 0);
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
    HC_BT_HDR  *p_buf = NULL;
    uint8_t     *p;

    ALOGI("hw_epilog_process");

    /* Sending a HCI_RESET */
    if (bt_vendor_cbacks)
    {
        /* Must allocate command buffer via HC's alloc API */
        p_buf = (HC_BT_HDR *) bt_vendor_cbacks->alloc(BT_HC_HDR_SIZE + \
                                                       HCI_CMD_PREAMBLE_SIZE);
    }

    if (p_buf)
    {
        ALOGI("hw_epilog_process send hci reset");
        p_buf->event = MSG_STACK_TO_HC_HCI_CMD;
        p_buf->offset = 0;
        p_buf->layer_specific = 0;
        p_buf->len = HCI_CMD_PREAMBLE_SIZE;

        p = (uint8_t *) (p_buf + 1);
        UINT16_TO_STREAM(p, HCI_RESET);
        *p = 0; /* parameter length */

        /* Send command via HC's xmit_cb API */
        bt_vendor_cbacks->xmit_cb(HCI_RESET, p_buf, hw_epilog_cback);
    }
    else
    {
        ALOGI("hw_epilog_process dont send hci reset");
        if (bt_vendor_cbacks)
        {
            ALOGE("vendor lib epilog process aborted [no buffer]");
            bt_vendor_cbacks->epilog_cb(BT_VND_OP_RESULT_FAIL);
        }
    }
}
#endif // (HW_NEED_END_WITH_HCI_RESET == TRUE)

void hw_pskey_send(BT_PSKEY_CONFIG_T * bt_par)
{
    HC_BT_HDR  *p_buf = NULL;
    uint8_t     *p;
    int i = 0;

    ALOGI("%s", __func__);

    /* Sending a HCI_RESET */
    if (bt_vendor_cbacks)
    {
        /* Must allocate command buffer via HC's alloc API */
        p_buf = (HC_BT_HDR *) bt_vendor_cbacks->alloc(BT_HC_HDR_SIZE + \
                                                       HCI_CMD_PREAMBLE_SIZE + PSKEY_PREAMBLE_SIZE);
    }

    if (p_buf)
    {
        ALOGI("hw_pskey_send send pskey");
        p_buf->event = MSG_STACK_TO_HC_HCI_CMD;
        p_buf->offset = 0;
        p_buf->layer_specific = 0;
        p_buf->len = HCI_CMD_PREAMBLE_SIZE + PSKEY_PREAMBLE_SIZE;

        p = (uint8_t *) (p_buf + 1);
        UINT16_TO_STREAM(p, HCI_PSKEY);
        *p = PSKEY_PREAMBLE_SIZE; /* parameter length */

        p++;


        UINT8_TO_STREAM(p, bt_par->g_dbg_source_sink_syn_test_data);
        UINT8_TO_STREAM(p, bt_par->g_sys_sleep_in_standby_supported);
        UINT8_TO_STREAM(p, bt_par->g_sys_sleep_master_supported);
        UINT8_TO_STREAM(p, bt_par->g_sys_sleep_slave_supported);

        UINT32_TO_STREAM(p, bt_par->default_ahb_clk);
        UINT32_TO_STREAM(p, bt_par->device_class);
        UINT32_TO_STREAM(p, bt_par->win_ext);

        for (i = 0; i < 6; i++) {
            UINT32_TO_STREAM(p, bt_par->g_aGainValue[i]);
        }
        for (i = 0; i < 5; i++) {
            UINT32_TO_STREAM(p, bt_par->g_aPowerValue[i]);
        }

        for (i = 0; i < 16; i++) {
            UINT8_TO_STREAM(p, bt_par->feature_set[i]);
        }
        for (i = 0; i < 6; i++) {
            UINT8_TO_STREAM(p, bt_par->device_addr[i]);
        }

        UINT8_TO_STREAM(p, bt_par->g_sys_sco_transmit_mode);
        UINT8_TO_STREAM(p, bt_par->g_sys_uart0_communication_supported);
        UINT8_TO_STREAM(p, bt_par->edr_tx_edr_delay);
        UINT8_TO_STREAM(p, bt_par->edr_rx_edr_delay);

        UINT16_TO_STREAM(p, bt_par->g_wbs_nv_117);

        UINT32_TO_STREAM(p, bt_par->is_wdg_supported);

        UINT32_TO_STREAM(p, bt_par->share_memo_rx_base_addr);
        //UINT32_TO_STREAM(p, bt_par->share_memo_tx_base_addr);
        UINT16_TO_STREAM(p, bt_par->g_wbs_nv_118);
        UINT16_TO_STREAM(p, bt_par->g_nbv_nv_117);

        UINT32_TO_STREAM(p, bt_par->share_memo_tx_packet_num_addr);
        UINT32_TO_STREAM(p, bt_par->share_memo_tx_data_base_addr);

        UINT32_TO_STREAM(p, bt_par->g_PrintLevel);

        UINT16_TO_STREAM(p, bt_par->share_memo_tx_block_length);
        UINT16_TO_STREAM(p, bt_par->share_memo_rx_block_length);
        UINT16_TO_STREAM(p, bt_par->share_memo_tx_water_mark);
        //UINT16_TO_STREAM(p, bt_par->share_memo_tx_timeout_value);
        UINT16_TO_STREAM(p, bt_par->g_nbv_nv_118);

        UINT16_TO_STREAM(p, bt_par->uart_rx_watermark);
        UINT16_TO_STREAM(p, bt_par->uart_flow_control_thld);
        UINT32_TO_STREAM(p, bt_par->comp_id);
        UINT16_TO_STREAM(p, bt_par->pcm_clk_divd);


        for (i = 0; i < 8; i++) {
            UINT32_TO_STREAM(p, bt_par->reserved[i]);
        }

        /* Send command via HC's xmit_cb API */
        bt_vendor_cbacks->xmit_cb(HCI_PSKEY, p_buf, hw_pskey_cback);
    }
    else
    {
        ALOGI("hw_pskey_send dont send pskey");
        if (bt_vendor_cbacks)
        {
            ALOGE("vendor lib hw_pskey_send aborted [no buffer]");
        }
    }
}
