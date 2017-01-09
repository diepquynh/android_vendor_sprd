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

#ifndef BT_VENDOR_SPRD_H
#define BT_VENDOR_SPRD_H

#include "bt_vendor_lib.h"
//#include "vnd_buildcfg.h"
#include "userial_vendor.h"
#include "utils.h"

#ifndef FALSE
#define FALSE  0
#endif

#ifndef TRUE
#define TRUE   (!FALSE)
#endif

// File discriptor using Transport
extern int fd;

extern bt_hci_transport_device_type bt_hci_transport_device;

extern bt_vendor_callbacks_t *bt_vendor_cbacks;
/* HW_NEED_END_WITH_HCI_RESET

    code implementation of sending a HCI_RESET command during the epilog
    process. It calls back to the callers after command complete of HCI_RESET
    is received.

    Default TRUE .
*/
#ifndef HW_NEED_END_WITH_HCI_RESET
#define HW_NEED_END_WITH_HCI_RESET TRUE
#endif

#define HCI_RESET  0x0C03
#define HCI_CMD_PREAMBLE_SIZE 3
#define HCI_EVT_CMD_CMPL_STATUS_RET_BYTE   5
#define HCI_EVT_CMD_CMPL_OPCODE        3

#define NUM_OF_DEVS 1

 #define GET_BTMAC_ATCMD "AT+SNVM=0,401"
 #define GET_BTPSKEY_ATCMD "AT+SNVM=0,415"
 #define SET_BTMAC_ATCMD  "AT+SNVM=1,401"


#define HCI_PSKEY   0x1C01

#define HCI_TYPE_COMMAND        (0x0100)
#define HCI_COMMAND_PSKEY      (0x1C01)
#define HCI_TYPE_EVENT              (0x04)
#define HCI_EVENT_PKSEY            (0x6F)
#define HCI_PKSEY_RS_OK            (0x00)
#define HCI_PKSEY_LEN                (0x01)

#define PSKEY_PRELOAD_SIZE    0x04
#define PSKEY_PREAMBLE_SIZE    0xA2

#define UINT8_TO_STREAM(p, u8)   {*(p)++ = (uint8)(u8);}
#define UINT24_TO_STREAM(p, u24) {*(p)++ = (uint8)(u24); *(p)++ = (uint8)((u24) >> 8); *(p)++ = (uint8)((u24) >> 16);}

#define DATMISC_MAC_ADDR_PATH    "/data/misc/bluedroid/btmac.txt"
#define MAC_ADDR_BUF_LEN    (strlen("FF:FF:FF:FF:FF:FF"))
#define MAC_ADDR_FILE_LEN    25
#define MAC_ADDR_LEN    6

 typedef unsigned char uint8;
 typedef unsigned int uint32;
 typedef unsigned short uint16;


typedef struct SPRD_BT_PSKEY_INFO_T{
    uint32	 pskey_cmd;

    uint8    g_dbg_source_sink_syn_test_data;
    uint8    g_sys_sleep_in_standby_supported;
    uint8    g_sys_sleep_master_supported;
    uint8    g_sys_sleep_slave_supported;

    uint32  default_ahb_clk;
    uint32  device_class;
    uint32  win_ext;

    uint32  g_aGainValue[6];
    uint32  g_aPowerValue[5];

    uint8    feature_set[16];
    uint8    device_addr[6];

    uint8    g_sys_sco_transmit_mode; //true tramsmit by uart, otherwise by share memory
    uint8    g_sys_uart0_communication_supported; //true use uart0, otherwise use uart1 for debug
    uint8    edr_tx_edr_delay;
    uint8    edr_rx_edr_delay;

    uint16  g_wbs_nv_117;

    uint32  is_wdg_supported;

    uint32  share_memo_rx_base_addr;

   // uint32  share_memo_tx_base_addr;
    uint16  g_wbs_nv_118;
    uint16  g_nbv_nv_117;

    uint32  share_memo_tx_packet_num_addr;
    uint32  share_memo_tx_data_base_addr;

    uint32  g_PrintLevel;

    uint16  share_memo_tx_block_length;
    uint16  share_memo_rx_block_length;
    uint16  share_memo_tx_water_mark;

    //uint16  share_memo_tx_timeout_value;
    uint16  g_nbv_nv_118;

    uint16  uart_rx_watermark;
    uint16  uart_flow_control_thld;
    uint32  comp_id;
    uint16  pcm_clk_divd;

    uint32  reserved[8];
}BT_PSKEY_CONFIG_T;

extern int bt_getPskeyFromFile(void *pData);

void sprd_pskey_response_cb(int ok);

#endif /* BT_VENDOR_SPRD_H */

