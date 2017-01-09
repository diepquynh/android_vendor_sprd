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

#define LOG_TAG "bt_chip_vendor"

#include <utils/Log.h>
#include <string.h>
#include <cutils/properties.h>
#include "sr2351.h"
#include "bt_hci_bdroid.h"
#include "upio.h"

// pskey file structure default value
pskey_config_t sr2351_pskey = {0};

const conf_entry_t sr2351_table[] = {
    CONF_ITEM_TABLE(pskey_cmd, 0, sr2351_pskey, 1),
    CONF_ITEM_TABLE(g_dbg_source_sink_syn_test_data, 0, sr2351_pskey, 1),
    CONF_ITEM_TABLE(g_sys_sleep_in_standby_supported, 0, sr2351_pskey, 1),
    CONF_ITEM_TABLE(g_sys_sleep_master_supported, 0, sr2351_pskey, 1),
    CONF_ITEM_TABLE(g_sys_sleep_slave_supported, 0, sr2351_pskey, 1),

    CONF_ITEM_TABLE(default_ahb_clk, 0, sr2351_pskey, 1),
    CONF_ITEM_TABLE(device_class, 0, sr2351_pskey, 1),
    CONF_ITEM_TABLE(win_ext, 0, sr2351_pskey, 1),

    CONF_ITEM_TABLE(g_aGainValue, 0, sr2351_pskey, 6),
    CONF_ITEM_TABLE(g_aPowerValue, 0, sr2351_pskey, 5),

    CONF_ITEM_TABLE(feature_set, 0, sr2351_pskey, 16),
    CONF_ITEM_TABLE(device_addr, 0, sr2351_pskey, 6),

    CONF_ITEM_TABLE(g_sys_sco_transmit_mode, 0, sr2351_pskey, 1),
    CONF_ITEM_TABLE(g_sys_uart0_communication_supported, 0, sr2351_pskey, 1),
    CONF_ITEM_TABLE(edr_tx_edr_delay, 0, sr2351_pskey, 1),
    CONF_ITEM_TABLE(edr_rx_edr_delay, 0, sr2351_pskey, 1),

    CONF_ITEM_TABLE(g_PrintLevel, 0, sr2351_pskey, 1),

    CONF_ITEM_TABLE(uart_rx_watermark, 0, sr2351_pskey, 1),
    CONF_ITEM_TABLE(uart_flow_control_thld, 0, sr2351_pskey, 1),

    CONF_ITEM_TABLE(comp_id, 0, sr2351_pskey, 1),
    CONF_ITEM_TABLE(pcm_clk_divd, 0, sr2351_pskey, 1),

    CONF_ITEM_TABLE(half_word_reserved, 0, sr2351_pskey, 1),
    CONF_ITEM_TABLE(pcm_config, 0, sr2351_pskey, 1),

    CONF_ITEM_TABLE(ref_clk, 0, sr2351_pskey, 1),
    CONF_ITEM_TABLE(FEM_status, 0, sr2351_pskey, 1),
    CONF_ITEM_TABLE(gpio_cfg, 0, sr2351_pskey, 1),
    CONF_ITEM_TABLE(gpio_PA_en, 0, sr2351_pskey, 1),
    CONF_ITEM_TABLE(wifi_tx, 0, sr2351_pskey, 1),
    CONF_ITEM_TABLE(bt_tx, 0, sr2351_pskey, 1),
    CONF_ITEM_TABLE(wifi_rx, 0, sr2351_pskey, 1),
    CONF_ITEM_TABLE(bt_rx, 0, sr2351_pskey, 1),
    CONF_ITEM_TABLE(wb_lna_bypass, 0, sr2351_pskey, 1),
    CONF_ITEM_TABLE(gain_LNA, 0, sr2351_pskey, 1),
    CONF_ITEM_TABLE(IL_wb_lna_bypass, 0, sr2351_pskey, 1),
    CONF_ITEM_TABLE(Rx_adaptive, 0, sr2351_pskey, 1),
    CONF_ITEM_TABLE(up_bypass_switching_point0, 0, sr2351_pskey, 1),
    CONF_ITEM_TABLE(low_bypass_switching_point0, 0, sr2351_pskey, 1),

    CONF_ITEM_TABLE(bt_reserved, 0, sr2351_pskey, 4),
    {0, 0, 0, 0, 0}
};

void sr2351_pskey_dump(void *arg)
{
    UNUSED(arg);
    pskey_config_t *p = &sr2351_pskey;
    ALOGI("pskey_cmd: 0x%X", p->pskey_cmd);
    ALOGI("g_dbg_source_sink_syn_test_data: 0x%02X",
          p->g_dbg_source_sink_syn_test_data);
    ALOGI("g_sys_sleep_in_standby_supported: 0x%02X",
          p->g_sys_sleep_in_standby_supported);
    ALOGI("g_sys_sleep_master_supported: 0x%02X",
          p->g_sys_sleep_master_supported);
    ALOGI("g_sys_sleep_slave_supported: 0x%02X", p->g_sys_sleep_slave_supported);

    ALOGI("default_ahb_clk: 0x%08X", p->default_ahb_clk);
    ALOGI("device_class: 0x%08X", p->device_class);
    ALOGI("win_ext: 0x%08X", p->win_ext);

    ALOGI("g_aGainValue: 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X",
          p->g_aGainValue[0], p->g_aGainValue[1], p->g_aGainValue[2],
          p->g_aGainValue[3], p->g_aGainValue[4], p->g_aGainValue[5]);
    ALOGI("g_aPowerValue: 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X",
          p->g_aPowerValue[0], p->g_aPowerValue[1], p->g_aPowerValue[2],
          p->g_aPowerValue[3], p->g_aPowerValue[4]);

    ALOGI(
        "feature_set(0~7): 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, "
        "0x%02X, 0x%02X",
        p->feature_set[0], p->feature_set[1], p->feature_set[2],
        p->feature_set[3], p->feature_set[4], p->feature_set[5],
        p->feature_set[6], p->feature_set[7]);
    ALOGI(
        "feature_set(8~15): 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, "
        "0x%02X, 0x%02X",
        p->feature_set[8], p->feature_set[9], p->feature_set[10],
        p->feature_set[11], p->feature_set[12], p->feature_set[13],
        p->feature_set[14], p->feature_set[15]);
    ALOGI("device_addr: 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X",
          p->device_addr[0], p->device_addr[1], p->device_addr[2],
          p->device_addr[3], p->device_addr[4], p->device_addr[5]);

    ALOGI("g_sys_sco_transmit_mode: 0x%02X", p->g_sys_sco_transmit_mode);
    ALOGI("g_sys_uart0_communication_supported: 0x%02X",
          p->g_sys_uart0_communication_supported);
    ALOGI("edr_tx_edr_delay: 0x%02X", p->edr_tx_edr_delay);
    ALOGI("edr_rx_edr_delay: 0x%02X", p->edr_rx_edr_delay);
    ALOGI("g_PrintLevel: 0x%08X", p->g_PrintLevel);
    ALOGI("uart_rx_watermark: 0x%04X", p->uart_rx_watermark);
    ALOGI("uart_flow_control_thld: 0x%04X", p->uart_flow_control_thld);
    ALOGI("comp_id: 0x%08X", p->comp_id);
    ALOGI("pcm_clk_divd: 0x%04X", p->pcm_clk_divd);
    ALOGI("half_word_reserved: 0x%04X", p->half_word_reserved);
    ALOGI("pcm_config: 0x%08X", p->pcm_config);

    ALOGI("ref_clk: 0x%02X", p->ref_clk);
    ALOGI("FEM_status: 0x%02X", p->FEM_status);
    ALOGI("gpio_cfg: 0x%02X", p->gpio_cfg);
    ALOGI("gpio_PA_en: 0x%02X", p->gpio_PA_en);
    ALOGI("wifi_tx: 0x%02X", p->wifi_tx);
    ALOGI("bt_tx: 0x%02X", p->bt_tx);
    ALOGI("wifi_rx: 0x%02X", p->wifi_rx);
    ALOGI("bt_rx: 0x%02X", p->bt_rx);
    ALOGI("wb_lna_bypass: 0x%02X", p->wb_lna_bypass);
    ALOGI("gain_LNA: 0x%02X", p->gain_LNA);
    ALOGI("IL_wb_lna_bypass: 0x%02X", p->IL_wb_lna_bypass);
    ALOGI("Rx_adaptive: 0x%02X", p->Rx_adaptive);
    ALOGI("up_bypass_switching_point0: 0x%04X", p->up_bypass_switching_point0);
    ALOGI("low_bypass_switching_point0: 0x%04X", p->low_bypass_switching_point0);

    ALOGI("bt_reserved(0~4): 0x%08X, 0x%08X, 0x%08X, 0x%08X", p->bt_reserved[0],
          p->bt_reserved[1], p->bt_reserved[2], p->bt_reserved[3]);
}

static int sr2351_pskey_preload(void *arg)
{
    int ret, fd;
    fd_set input;
    unsigned char buf[10] = {0};
    struct timeval tv = {.tv_sec = 5, .tv_usec = 0};
    fd = *((int *)arg);
    ALOGD("hw_pskey_vnd_preload fd: %d", fd);
    ALOGD("hw_pskey_vnd_preload enter");
    ret = write(fd, (char *)&sr2351_pskey, sizeof(pskey_config_t));
    if ((ret < 0) || (ret != sizeof(pskey_config_t))) {
        ALOGI("write pskey failed: %d(%s)", ret, strerror(errno));
        return -1;
    }

    ALOGD("wait pskey response");
    do {
        FD_ZERO(&input);
        FD_SET(fd, &input);

        ret = select(fd + 1, &input, NULL, NULL, &tv);
        if (ret > 0) {
            ALOGD("read pskey response");
            ret = read(fd, buf, sizeof(buf));
            ALOGD("got pskey lne: %d", ret);
            if (ret <= 0) {
                return -1;
            } else if (buf[0] != 0x05) {
                int i;
                ALOGE("received unknow response: ");
                for (i = 0; i < ret; i++) {
                    ALOGE("unknow ack[%d]: 0x%02x", i, buf[i]);
                }
                return -1;
            }
            break;
        } else if (ret < 0) {
            ALOGE("select failed: %d(%s)", ret, strerror(errno));
            return -1;
        } else if (ret == 0) {
            ALOGE("read pskey timeout: %d(%s)", ret, strerror(errno));
            return -1;
        }
    } while (1);
    ALOGD("hw_pskey_vnd_preload out");
    return 0;
}

static bt_adapter_module_t sr2351_module = {
    .name = "sr2351",
    .pskey = &sr2351_pskey,
    .tab = (conf_entry_t *)&sr2351_table,
    .pskey_preload = sr2351_pskey_preload,
    .epilog_process = NULL,
    .pskey_dump = sr2351_pskey_dump,
    .get_conf_file = NULL
};

const bt_adapter_module_t *get_adapter_module(void)
{
    return &sr2351_module;
}
