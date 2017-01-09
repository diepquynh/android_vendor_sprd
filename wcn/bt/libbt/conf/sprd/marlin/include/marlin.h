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

#ifndef LIBBT_CONF_SPRD_MARLIN_INCLUDE_MARLIN_H_
#define LIBBT_CONF_SPRD_MARLIN_INCLUDE_MARLIN_H_

#include "bt_vendor_sprd.h"

#define PSKEY_PRELOAD_SIZE 0x04
#define PSKEY_PREAMBLE_SIZE 0xD8
#define HCI_PSKEY 0xFCA0
#define HCI_VSC_ENABLE_COMMMAND 0xFCA1
#define START_STOP_CMD_SIZE 3
#define FW_NODE_BYTE 6
#define FW_DATE_Y_BYTE 10
#define FW_DATE_M_BYTE 9
#define FW_DATE_D_BYTE 8
#define FW_DEFAULT_PROP "FFFF.FFFF.FF.FF"
#define FW_PROP_NAME "bluetooth.fw.ver"

#define MARLIN_HW_VER_BA "BA"
#define VENDOR_BA_LIB_CONF_FILE \
  "/system/etc/marlinba/connectivity_configure.ini"

/*  start bt with dual/classic/ble mode */
enum { DUAL_MODE = 0, CLASSIC_MODE, LE_MODE };
/*  vsc_enable_command parameter : enable or disable */
enum { DISABLE_BT = 0, ENABLE_BT };

typedef struct {
    uint32_t pskey_cmd;

    uint8_t g_dbg_source_sink_syn_test_data;
    uint8_t g_sys_sleep_in_standby_supported;
    uint8_t g_sys_sleep_master_supported;
    uint8_t g_sys_sleep_slave_supported;

    uint32_t default_ahb_clk;
    uint32_t device_class;
    uint32_t win_ext;

    uint32_t g_aGainValue[6];
    uint32_t g_aPowerValue[5];

    uint8_t feature_set[16];
    uint8_t device_addr[6];

    uint8_t g_sys_sco_transmit_mode;  // true tramsmit by uart, otherwise by share
    // memory
    uint8_t g_sys_uart0_communication_supported;  // true use uart0, otherwise use
    // uart1 for debug
    uint8_t edr_tx_edr_delay;
    uint8_t edr_rx_edr_delay;

    uint16_t g_wbs_nv_117;

    uint32_t is_wdg_supported;

    uint32_t share_memo_rx_base_addr;

    // uint32_t  share_memo_tx_base_addr;
    uint16_t g_wbs_nv_118;
    uint16_t g_nbv_nv_117;

    uint32_t share_memo_tx_packet_num_addr;
    uint32_t share_memo_tx_data_base_addr;

    uint32_t g_PrintLevel;

    uint16_t share_memo_tx_block_length;
    uint16_t share_memo_rx_block_length;
    uint16_t share_memo_tx_water_mark;

    // uint16_t  share_memo_tx_timeout_value;
    uint16_t g_nbv_nv_118;

    uint16_t uart_rx_watermark;
    uint16_t uart_flow_control_thld;
    uint32_t comp_id;
    uint16_t pcm_clk_divd;

    uint16_t  br_edr_diff_reserved;
    uint32_t  g_aBRChannelpwrvalue[8];
    uint32_t  g_aEDRChannelpwrvalue[8];
    uint32_t  g_aLEPowerControlFlag;
    uint16_t  g_aLEChannelpwrvalue[8];
} pskey_config_t;

const bt_adapter_module_t *get_adapter_module(void);

#endif  // LIBBT_CONF_SPRD_MARLIN_INCLUDE_MARLIN_H_
