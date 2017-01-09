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


 #define MAC_ERROR    "FF:FF:FF:FF:FF:FF"
 #define BT_MAC_FILE  "/data/misc/bluedroid/btmac.txt"
 #define BT_MAC_FILE_TEMP  "/data/misc/bluedroid/btmac.txt"
 #define GET_BTMAC_ATCMD "AT+SNVM=0,401"
 #define GET_BTPSKEY_ATCMD "AT+SNVM=0,415"
 #define SET_BTMAC_ATCMD  "AT+SNVM=1,401"
 #define BT_RAND_MAC_LENGTH   17

 // used to store BT pskey structure and default values
 #define BT_PSKEY_STRUCT_FILE "/system/lib/modules/pskey_bt.txt"
 #define BT_PSKEY_FILE  "/system/lib/modules/pskey_bt.txt"

 typedef unsigned char uint8;
 typedef unsigned int uint32;
 typedef unsigned short uint16;
 typedef unsigned char   BOOLEAN;
 #define BT_ADDRESS_SIZE    6

// add by longting.zhao for pskey NV
// pskey file structure
typedef struct SPRD_BT_PSKEY_INFO_T{
    uint8	pskey_cmd;//add h4 cmd 5 means pskey cmd
    uint8   g_dbg_source_sink_syn_test_data;
    uint8   g_sys_sleep_in_standby_supported;
    uint8   g_sys_sleep_master_supported;
    uint8   g_sys_sleep_slave_supported;
    uint32  default_ahb_clk;
    uint32  device_class;
    uint32  win_ext;
    uint32  g_aGainValue[6];
    uint32  g_aPowerValue[5];
    uint8   feature_set[16];
    uint8   device_addr[6];
    uint8  g_sys_sco_transmit_mode; //0: DMA 1: UART 2:SHAREMEM
    uint8  g_sys_uart0_communication_supported; //true use uart0, otherwise use uart1 for debug
    uint8 edr_tx_edr_delay;
    uint8 edr_rx_edr_delay;
    uint32 g_PrintLevel;
    uint16 uart_rx_watermark;
    uint16 uart_flow_control_thld;
    uint32 comp_id;
    uint16 pcm_clk_divd;
    uint16 half_word_reserved;
    uint32 pcm_config;
    /**********bt&wif public*********************/
    uint8 ref_clk;
    uint8 FEM_status;
    uint8 gpio_cfg;
    uint8 gpio_PA_en;
    uint8 wifi_tx;
    uint8 bt_tx;
    uint8 wifi_rx;
    uint8 bt_rx;
    uint8 wb_lna_bypass;
    uint8 gain_LNA;
    uint8 IL_wb_lna_bypass;
    uint8 Rx_adaptive;
    uint16 up_bypass_switching_point0;
    uint16 low_bypass_switching_point0;
    /***************************************/
    uint32  reserved[4];
}BT_PSKEY_CONFIG_T;

extern int bt_getPskeyFromFile(void *pData);

#endif /* BT_VENDOR_SPRD_H */

