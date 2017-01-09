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

#ifndef LIBBT_CONF_SPRD_SR2351_INCLUDE_SR2351_H_
#define LIBBT_CONF_SPRD_SR2351_INCLUDE_SR2351_H_

#include "bt_vendor_sprd.h"

typedef struct {
    uint8_t pskey_cmd;  // add h4 cmd 5 means pskey cmd

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

    uint8_t g_sys_sco_transmit_mode;              // 0: DMA 1: UART 2:SHAREMEM
    uint8_t g_sys_uart0_communication_supported;  // true use uart0, otherwise use
    // uart1 for debug
    uint8_t edr_tx_edr_delay;
    uint8_t edr_rx_edr_delay;

    uint32_t g_PrintLevel;
    uint16_t uart_rx_watermark;
    uint16_t uart_flow_control_thld;
    uint32_t comp_id;
    uint16_t pcm_clk_divd;
    uint16_t half_word_reserved;
    uint32_t pcm_config;
    /**********bt&wif public*********************/
    uint8_t ref_clk;
    uint8_t FEM_status;
    uint8_t gpio_cfg;
    uint8_t gpio_PA_en;
    uint8_t wifi_tx;
    uint8_t bt_tx;
    uint8_t wifi_rx;
    uint8_t bt_rx;
    uint8_t wb_lna_bypass;
    uint8_t gain_LNA;
    uint8_t IL_wb_lna_bypass;
    uint8_t Rx_adaptive;
    uint16_t up_bypass_switching_point0;
    uint16_t low_bypass_switching_point0;
    /***************************************/
    uint32_t bt_reserved[4];
} pskey_config_t;

const bt_adapter_module_t *get_adapter_module(void);

#endif  // LIBBT_CONF_SPRD_SR2351_INCLUDE_SR2351_H_
