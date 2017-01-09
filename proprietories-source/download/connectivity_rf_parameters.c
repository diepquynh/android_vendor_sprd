/**
 * connectivity_rf_parameters.c --- parse ini files
 *
 * Copyright (C) 2015 Spreadtrum Communications Inc.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#include "connectivity_rf_parameters.h"

#undef LOG_TAG
#define LOG_TAG "RF_PARA"
#include <cutils/log.h>
#include <cutils/properties.h>

#define _WLAN_CALI_DEBUG_
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#ifdef GET_MARLIN_CHIPID
#define BSL_REP_IS_2331_AA 0xAA /* 0xAA */
#define BSL_REP_IS_2331_BA 0xBA /* 0xBA */
#endif

#define RFDBG ALOGD

#define SECTION(x) ("[SETCTION " #x "]\n")
#define SECTION_LINE                                                           \
  ("#------------------------------------------------------------------------" \
  "--------\n")

#define SNPRINTVAR(z, format, ...) \
  snprintf(z, snprintf(NULL, 0, format, __VA_ARGS__) + 1, format, __VA_ARGS__)

#define SNPRINT(z, str) snprintf(z, snprintf(NULL, 0, str) + 1, str)

#define CONFIG_MAX_SIZE 16 * 1024
#define CALI_MAX_SIZE 16 * 1024
#define CONFIGURE_TYPE 1
#define CALIBRATION_TYPE 2

typedef struct {
  char itm[64];
  int par[256];
  int num;
} nvm_cali_cmd;

#ifdef GET_MARLIN_CHIPID
struct img_flag {
  int is_combine_flag;  // 0: is not combine image: 1: is combine image
  int which_img_flag;   // 1: is the one image, valid
  // 2: is the second image, just
};
extern struct img_flag marlin_img_flag;
#endif

static char *SYSTEM_WIFI_CONFIG_FILE = "/system/etc/connectivity_configure.ini";
static char *SYSTEM_WIFI_CALI_FILE = "/system/etc/connectivity_calibration.ini";

static char *SYSTEM_WIFI_BA_CONFIG_FILE =
    "/system/etc/marlinba/connectivity_configure.ini";
static char *SYSTEM_WIFI_BA_CALI_FILE =
    "/system/etc/marlinba/connectivity_calibration.ini";

static char *WIFI_CALI_FILE_WRITE_BACK =
    "/productinfo/connectivity_calibration.back.ini";

nvm_name_table g_config_table[] = {
    // [SETCTION 0]Marlin config Version info
    WIFI_CONFIG_TABLE("conf_version", config_version, 1),

    // [SETCTION 1]wifi TX Power tx power control: tx_power_control_t
    WIFI_CONFIG_TABLE("data_rate_power", tx_power_control.data_rate_power, 1),
    WIFI_CONFIG_TABLE("channel_num", tx_power_control.channel_num, 1),
    WIFI_CONFIG_TABLE("channel_range", tx_power_control.channel_range, 1),
    WIFI_CONFIG_TABLE("b_tx_power_dr0", tx_power_control.b_tx_power_dr0, 1),
    WIFI_CONFIG_TABLE("b_tx_power_dr1", tx_power_control.b_tx_power_dr1, 1),
    WIFI_CONFIG_TABLE("g_tx_power_dr0", tx_power_control.g_tx_power_dr0, 1),
    WIFI_CONFIG_TABLE("g_tx_power_dr1", tx_power_control.g_tx_power_dr1, 1),
    WIFI_CONFIG_TABLE("g_tx_power_dr2", tx_power_control.g_tx_power_dr2, 1),
    WIFI_CONFIG_TABLE("g_tx_power_dr3", tx_power_control.g_tx_power_dr3, 1),
    WIFI_CONFIG_TABLE("n_tx_power_dr0", tx_power_control.n_tx_power_dr0, 1),
    WIFI_CONFIG_TABLE("n_tx_power_dr1", tx_power_control.n_tx_power_dr1, 1),
    WIFI_CONFIG_TABLE("n_tx_power_dr2", tx_power_control.n_tx_power_dr2, 1),
    WIFI_CONFIG_TABLE("n_tx_power_dr3", tx_power_control.n_tx_power_dr3, 1),
    WIFI_CONFIG_TABLE("power_reserved", tx_power_control.power_reserved, 1),

    // [SETCTION 2]wifi PHY/RF reg init: init_register_t
    WIFI_CONFIG_TABLE("phy0_init_num", init_register.phy0_init_num, 1),
    WIFI_CONFIG_TABLE("init_phy0_regs", init_register.init_phy0_regs, 2),
    WIFI_CONFIG_TABLE("phy1_init_num", init_register.phy1_init_num, 1),
    WIFI_CONFIG_TABLE("init_phy1_regs", init_register.init_phy1_regs, 2),
    WIFI_CONFIG_TABLE("rf_init_num", init_register.rf_init_num, 1),
    WIFI_CONFIG_TABLE("init_rf_regs", init_register.init_rf_regs, 4),
    WIFI_CONFIG_TABLE("reserved_w16_num", init_register.reserved_w16_num, 1),
    WIFI_CONFIG_TABLE("reserved_w16_regs", init_register.reserved_w16_regs, 2),
    WIFI_CONFIG_TABLE("reserved_w32_num", init_register.reserved_w32_num, 1),
    WIFI_CONFIG_TABLE("reserved_w16_regs", init_register.reserved_w32_regs, 2),

    // [SETCTION 3]wifi enhance config: enhance_config_t
    WIFI_CONFIG_TABLE("tpc_enable", enhance_config.tpc_enable, 1),
    WIFI_CONFIG_TABLE("power_save_key", enhance_config.power_save_key, 1),
    WIFI_CONFIG_TABLE("enhance_reserved", enhance_config.enhance_reserved, 1),

    // [SETCTION 4]Wifi/BT/lte coex config: coex_config_t
    WIFI_CONFIG_TABLE("CoexExcutionMode", coex_config.CoexExcutionMode, 1),
    WIFI_CONFIG_TABLE("CoexWifiScanCntPerChannel",
                      coex_config.CoexWifiScanCntPerChannel, 1),
    WIFI_CONFIG_TABLE("CoexWifiScanDurationOneTime",
                      coex_config.CoexWifiScanDurationOneTime, 1),
    WIFI_CONFIG_TABLE("CoexScoPeriodsToBlockDuringDhcp",
                      coex_config.CoexScoPeriodsToBlockDuringDhcp, 1),
    WIFI_CONFIG_TABLE("CoexA2dpDhcpProtectLevel",
                      coex_config.CoexA2dpDhcpProtectLevel, 1),
    WIFI_CONFIG_TABLE("CoexScoperiodsToBlockDuringEap",
                      coex_config.CoexScoperiodsToBlockDuringEap, 1),
    WIFI_CONFIG_TABLE("CoexA2dpEapProtectLevel",
                      coex_config.CoexA2dpEapProtectLevel, 1),
    WIFI_CONFIG_TABLE("CoexScoPeriodsToBlockDuringWifiJoin",
                      coex_config.CoexScoPeriodsToBlockDuringWifiJoin, 1),
    WIFI_CONFIG_TABLE("CoexA2dpWifiJoinProtectLevel",
                      coex_config.CoexA2dpWifiJoinProtectLevel, 1),
    WIFI_CONFIG_TABLE("CoexEnterPMStateTime", coex_config.CoexEnterPMStateTime,
                      2),
    WIFI_CONFIG_TABLE("CoexAclA2dpBtWorkTime",
                      coex_config.CoexAclA2dpBtWorkTime, 2),
    WIFI_CONFIG_TABLE("CoexAclA2dpWifiWorkTime",
                      coex_config.CoexAclA2dpWifiWorkTime, 2),
    WIFI_CONFIG_TABLE("CoexAclNoA2dpBtWorkTime",
                      coex_config.CoexAclNoA2dpBtWorkTime, 2),
    WIFI_CONFIG_TABLE("CoexAclNoA2dpWifiWorkTime",
                      coex_config.CoexAclNoA2dpWifiWorkTime, 2),
    WIFI_CONFIG_TABLE("CoexAclMixBtWorkTime", coex_config.CoexAclMixBtWorkTime,
                      2),
    WIFI_CONFIG_TABLE("CoexAclMixWifiWorkTime",
                      coex_config.CoexAclMixWifiWorkTime, 2),
    WIFI_CONFIG_TABLE("CoexPageInqBtWorkTime",
                      coex_config.CoexPageInqBtWorkTime, 2),
    WIFI_CONFIG_TABLE("CoexPageInqWifiWorkTime",
                      coex_config.CoexPageInqWifiWorkTime, 2),
    WIFI_CONFIG_TABLE("CoexScoSchema", coex_config.CoexScoSchema, 2),
    WIFI_CONFIG_TABLE("CoexDynamicScoSchemaEnable",
                      coex_config.CoexDynamicScoSchemaEnable, 2),
    WIFI_CONFIG_TABLE("CoexScoPeriodsBtTakeAll",
                      coex_config.CoexScoPeriodsBtTakeAll, 2),
    WIFI_CONFIG_TABLE("CoexLteTxAdvancedTime",
                      coex_config.CoexLteTxAdvancedTime, 2),
    WIFI_CONFIG_TABLE("CoexLteOneSubFrameLen",
                      coex_config.CoexLteOneSubFrameLen, 2),
    WIFI_CONFIG_TABLE("CoexLteTxTimerLen", coex_config.CoexLteTxTimerLen, 2),
    WIFI_CONFIG_TABLE("CoexLteTxTimerFrameHeadLen",
                      coex_config.CoexLteTxTimerFrameHeadLen, 2),
    WIFI_CONFIG_TABLE("CoexLteStrategyFlag", coex_config.CoexLteStrategyFlag,
                      2),
    WIFI_CONFIG_TABLE("CoexWifiDegradePowerValue",
                      coex_config.CoexWifiDegradePowerValue, 2),
    WIFI_CONFIG_TABLE("CoexBtDegradePowerValue",
                      coex_config.CoexBtDegradePowerValue, 2),
    WIFI_CONFIG_TABLE("CoexWifi2300TxSpur2Lte",
                      coex_config.CoexWifi2300TxSpur2Lte[0], 2),
    WIFI_CONFIG_TABLE("CoexWifi2310TxSpur2Lte",
                      coex_config.CoexWifi2310TxSpur2Lte[0], 2),
    WIFI_CONFIG_TABLE("CoexWifi2320TxSpur2Lte",
                      coex_config.CoexWifi2320TxSpur2Lte[0], 2),
    WIFI_CONFIG_TABLE("CoexWifi2330TxSpur2Lte",
                      coex_config.CoexWifi2330TxSpur2Lte[0], 2),
    WIFI_CONFIG_TABLE("CoexWifi2340TxSpur2Lte",
                      coex_config.CoexWifi2340TxSpur2Lte[0], 2),
    WIFI_CONFIG_TABLE("CoexWifi2350TxSpur2Lte",
                      coex_config.CoexWifi2350TxSpur2Lte[0], 2),
    WIFI_CONFIG_TABLE("CoexWifi2360TxSpur2Lte",
                      coex_config.CoexWifi2360TxSpur2Lte[0], 2),
    WIFI_CONFIG_TABLE("CoexWifi2370TxSpur2Lte",
                      coex_config.CoexWifi2370TxSpur2Lte[0], 2),
    WIFI_CONFIG_TABLE("CoexWifi2380TxSpur2Lte",
                      coex_config.CoexWifi2380TxSpur2Lte[0], 2),
    WIFI_CONFIG_TABLE("CoexWifi2390TxSpur2Lte",
                      coex_config.CoexWifi2390TxSpur2Lte[0], 2),
    WIFI_CONFIG_TABLE("CoexWifi2400TxSpur2Lte",
                      coex_config.CoexWifi2400TxSpur2Lte[0], 2),
    WIFI_CONFIG_TABLE("CoexLteTxSpur2Wifi2300",
                      coex_config.CoexLteTxSpur2Wifi2300[0], 2),
    WIFI_CONFIG_TABLE("CoexLteTxSpur2Wifi2310",
                      coex_config.CoexLteTxSpur2Wifi2310[0], 2),
    WIFI_CONFIG_TABLE("CoexLteTxSpur2Wifi2320",
                      coex_config.CoexLteTxSpur2Wifi2320[0], 2),
    WIFI_CONFIG_TABLE("CoexLteTxSpur2Wifi2330",
                      coex_config.CoexLteTxSpur2Wifi2330[0], 2),
    WIFI_CONFIG_TABLE("CoexLteTxSpur2Wifi2340",
                      coex_config.CoexLteTxSpur2Wifi2340[0], 2),
    WIFI_CONFIG_TABLE("CoexLteTxSpur2Wifi2350",
                      coex_config.CoexLteTxSpur2Wifi2350[0], 2),
    WIFI_CONFIG_TABLE("CoexLteTxSpur2Wifi2360",
                      coex_config.CoexLteTxSpur2Wifi2360[0], 2),
    WIFI_CONFIG_TABLE("CoexLteTxSpur2Wifi2370",
                      coex_config.CoexLteTxSpur2Wifi2370[0], 2),
    WIFI_CONFIG_TABLE("CoexLteTxSpur2Wifi2380",
                      coex_config.CoexLteTxSpur2Wifi2380[0], 2),
    WIFI_CONFIG_TABLE("CoexLteTxSpur2Wifi2390",
                      coex_config.CoexLteTxSpur2Wifi2390[0], 2),
    WIFI_CONFIG_TABLE("CoexLteTxSpur2Wifi2400",
                      coex_config.CoexLteTxSpur2Wifi2400[0], 2),
    WIFI_CONFIG_TABLE("CoexReserved", coex_config.CoexReserved, 2),

    // [SETCTION 5]Wifi&BT public config
    WIFI_CONFIG_TABLE("public_reserved", public_config.public_reserved, 1),
};

nvm_name_table g_cali_table[] = {
    // [SETCTION 0]Marlin cali Version info
    WIFI_CALI_TABLE("cali_version", cali_version, 1),

    // [SETCTION 1]Calibration Config: cali_config_t
    WIFI_CALI_TABLE("is_calibrated", cali_config.is_calibrated, 1),
    WIFI_CALI_TABLE("rc_cali_en", cali_config.rc_cali_en, 1),
    WIFI_CALI_TABLE("dcoc_cali_en", cali_config.dcoc_cali_en, 1),
    WIFI_CALI_TABLE("txiq_cali_en", cali_config.txiq_cali_en, 1),
    WIFI_CALI_TABLE("rxiq_cali_en", cali_config.rxiq_cali_en, 1),
    WIFI_CALI_TABLE("txpower_cali_en", cali_config.txpower_cali_en, 1),
    WIFI_CALI_TABLE("dpd_cali_en", cali_config.dpd_cali_en, 1),
    WIFI_CALI_TABLE("config_reserved", cali_config.config_reserved[0], 1),

    // [SETCTION 2]rc calibration data: rctune_cali_t
    WIFI_CALI_TABLE("rctune_value", rctune_cali.rctune_value, 1),
    WIFI_CALI_TABLE("rc_cali_reserved", rctune_cali.rctune_reserved[0], 1),

    // [SETCTION 3]doco calibration data: dcoc_cali_t
    WIFI_CALI_TABLE("dcoc_cali_code", dcoc_cali.dcoc_cali_code[0], 2),
    WIFI_CALI_TABLE("dcoc_reserved", dcoc_cali.dcoc_reserved[0], 4),

    // [SETCTION 4]txiq calibration data: txiq_cali_t
    WIFI_CALI_TABLE("rf_txiq_c11", txiq_cali.rf_txiq_c11, 4),
    WIFI_CALI_TABLE("rf_txiq_c12", txiq_cali.rf_txiq_c12, 4),
    WIFI_CALI_TABLE("rf_txiq_c22", txiq_cali.rf_txiq_c22, 4),
    WIFI_CALI_TABLE("rf_txiq_dc", txiq_cali.rf_txiq_dc, 4),
    WIFI_CALI_TABLE("txiq_reserved", txiq_cali.txiq_reserved[0], 4),

    // [SETCTION 5]rxiq calibration data: rxiq_cali_t
    WIFI_CALI_TABLE("rf_rxiq_coef21_22", rxiq_cali.rf_rxiq_coef21_22, 4),
    WIFI_CALI_TABLE("rf_rxiq_coef11_12", rxiq_cali.rf_rxiq_coef11_12, 4),
    WIFI_CALI_TABLE("rxiq_reserved", rxiq_cali.rxiq_reserved[0], 4),

    // [SETCTION 6]txpower calibration data: txpower_cali_t
    WIFI_CALI_TABLE("txpower_psat_temperature",
                    txpower_cali.txpower_psat_temperature, 4),
    WIFI_CALI_TABLE("txpower_psat_gainindex",
                    txpower_cali.txpower_psat_gainindex, 1),

    WIFI_CALI_TABLE("txpower_psat_power", txpower_cali.txpower_psat_power, 2),
    WIFI_CALI_TABLE("txpower_psat_backoff", txpower_cali.txpower_psat_backoff,
                    1),
    WIFI_CALI_TABLE("txpower_psat_upper_limit",
                    txpower_cali.txpower_psat_upper_limit, 1),
    WIFI_CALI_TABLE("txpower_psat_lower_limit",
                    txpower_cali.txpower_psat_lower_limit, 1),

    WIFI_CALI_TABLE("txpower_freq_delta_gainindex",
                    txpower_cali.txpower_freq_delta_gainindex[0], 1),
    WIFI_CALI_TABLE("txpower_temperature_delta_gm_channel1",
                    txpower_cali.txpower_temperature_delta_gm_channel1[0], 1),
    WIFI_CALI_TABLE("txpower_temperature_delta_gm_channel2",
                    txpower_cali.txpower_temperature_delta_gm_channel2[0], 1),
    WIFI_CALI_TABLE("txpower_temperature_delta_gm_channel3",
                    txpower_cali.txpower_temperature_delta_gm_channel3[0], 1),
    WIFI_CALI_TABLE("txpower_temperature_delta_gm_channel4",
                    txpower_cali.txpower_temperature_delta_gm_channel4[0], 1),
    WIFI_CALI_TABLE("txpower_temperature_delta_gm_channel5",
                    txpower_cali.txpower_temperature_delta_gm_channel5[0], 1),
    WIFI_CALI_TABLE("txpower_temperature_delta_gm_channel6",
                    txpower_cali.txpower_temperature_delta_gm_channel6[0], 1),
    WIFI_CALI_TABLE("txpower_temperature_delta_gm_channel7",
                    txpower_cali.txpower_temperature_delta_gm_channel7[0], 1),
    WIFI_CALI_TABLE("txpower_temperature_delta_gm_channel8",
                    txpower_cali.txpower_temperature_delta_gm_channel8[0], 1),
    WIFI_CALI_TABLE("txpower_temperature_delta_gm_channel9",
                    txpower_cali.txpower_temperature_delta_gm_channel9[0], 1),
    WIFI_CALI_TABLE("txpower_temperature_delta_gm_channel10",
                    txpower_cali.txpower_temperature_delta_gm_channel10[0], 1),
    WIFI_CALI_TABLE("txpower_temperature_delta_gm_channel11",
                    txpower_cali.txpower_temperature_delta_gm_channel11[0], 1),
    WIFI_CALI_TABLE("txpower_temperature_delta_gm_channel12",
                    txpower_cali.txpower_temperature_delta_gm_channel12[0], 1),
    WIFI_CALI_TABLE("txpower_temperature_delta_gm_channel13",
                    txpower_cali.txpower_temperature_delta_gm_channel13[0], 1),
    WIFI_CALI_TABLE("txpower_temperature_delta_gm_channel14",
                    txpower_cali.txpower_temperature_delta_gm_channel14[0], 1),
    WIFI_CALI_TABLE("txpower_reserved", txpower_cali.txpower_reserved[0], 4),

    // [SETCTION 7]DPD calibration data: dpd_cali_t
    WIFI_CALI_TABLE("dpd_cali_channel_num", dpd_cali.dpd_cali_channel_num, 1),
    WIFI_CALI_TABLE("dpd_cali_channel", dpd_cali.dpd_cali_channel[0], 1),
    WIFI_CALI_TABLE("channel1_dpd_cali_table",
                    dpd_cali.channel1_dpd_cali_table[0], 4),
    WIFI_CALI_TABLE("channel2_dpd_cali_table",
                    dpd_cali.channel2_dpd_cali_table[0], 4),
    WIFI_CALI_TABLE("channel3_dpd_cali_table",
                    dpd_cali.channel3_dpd_cali_table[0], 4),
    WIFI_CALI_TABLE("dpd_reserved", dpd_cali.dpd_reserved[0], 4),

    // [SETCTION 8]RF parameters data: rf_para_t
    WIFI_CALI_TABLE("rf_ctune", rf_para.rf_ctune[0], 1),
    WIFI_CALI_TABLE("rf_reserved", rf_para.rf_reserved[0], 4),

    // [SETCTION 9]RF parameters data: tpc_cfg_t
    WIFI_CALI_TABLE("tpc_cfg", tpc_cfg.tpc_cfg[0], 4),
    WIFI_CALI_TABLE("tpc_reserved", tpc_cfg.tpc_reserved[0], 4),
};

static int find_type(char key) {
  if ((key >= 'a' && key <= 'w') || (key >= 'y' && key <= 'z') ||
      (key >= 'A' && key <= 'W') || (key >= 'Y' && key <= 'Z') || ('_' == key))
    return 1;
  if ((key >= '0' && key <= '9') || ('-' == key)) return 2;
  if (('x' == key) || ('X' == key) || ('.' == key)) return 3;
  if ((key == '\0') || ('\r' == key) || ('\n' == key) || ('#' == key)) return 4;
  return 0;
}

static void get_cmd_par(char *str, nvm_cali_cmd *cmd) {
  int i, j, bufType, cType, flag;
  char tmp[128];
  char c;
  bufType = -1;
  cType = 0;
  flag = 0;
  memset(cmd, 0, sizeof(nvm_cali_cmd));
  for (i = 0, j = 0;; i++) {
    c = str[i];
    cType = find_type(c);
    if ((1 == cType) || (2 == cType) || (3 == cType)) {
      tmp[j] = c;
      j++;
      if (-1 == bufType) {
        if (2 == cType)
          bufType = 2;
        else
          bufType = 1;
      } else if (2 == bufType) {
        if (1 == cType) bufType = 1;
      }
      continue;
    }
    if (-1 != bufType) {
      tmp[j] = '\0';

      if ((1 == bufType) && (0 == flag)) {
        snprintf(cmd->itm, strlen(tmp) + 1, "%s", tmp);
        flag = 1;
      } else {
        cmd->par[cmd->num] = strtol(tmp, NULL, 0);
        cmd->num++;
      }
      bufType = -1;
      j = 0;
    }
    if (0 == cType) continue;
    if (4 == cType) return;
  }
  return;
}

static int wifi_nvm_set_cmd(nvm_name_table *pTable, nvm_cali_cmd *cmd,
                            void *p_data) {
  int i;
  unsigned char *p;
  if ((1 != pTable->type) && (2 != pTable->type) && (4 != pTable->type)) {
    RFDBG("%s, pTable->type err\n", __func__);
    return -1;
  }

  p = (unsigned char *)(p_data) + pTable->mem_offset;
#ifdef _WLAN_CALI_DEBUG_
  RFDBG(
      "###[g_table]%s, offset:%lu, num:%d, value: "
      "%d %d %d %d %d %d %d %d %d %d %d %d\n",
      pTable->itm, pTable->mem_offset, cmd->num, cmd->par[0], cmd->par[1],
      cmd->par[2], cmd->par[3], cmd->par[4], cmd->par[5], cmd->par[6],
      cmd->par[7], cmd->par[8], cmd->par[9], cmd->par[10], cmd->par[11]);
#endif
  for (i = 0; i < cmd->num; i++) {
    if (1 == pTable->type)
      *((unsigned char *)p + i) = (unsigned char)(cmd->par[i]);
    else if (2 == pTable->type)
      *((unsigned short *)p + i) = (unsigned short)(cmd->par[i]);
    else if (4 == pTable->type)
      *((unsigned int *)p + i) = (unsigned int)(cmd->par[i]);
    else
      RFDBG("%s, type err\n", __func__);
  }
  return 0;
}

static nvm_name_table *wifi_nvm_config_table_match(nvm_cali_cmd *cmd) {
  int i;
  nvm_name_table *pTable = NULL;
  int len = sizeof(g_config_table) / sizeof(nvm_name_table);
  if (NULL == cmd) return NULL;
  for (i = 0; i < len; i++) {
    if (NULL == g_config_table[i].itm) continue;
    if (0 != strcmp(g_config_table[i].itm, cmd->itm)) continue;
    pTable = &g_config_table[i];
    break;
  }
  return pTable;
}

static nvm_name_table *wifi_nvm_cali_table_match(nvm_cali_cmd *cmd) {
  int i;
  nvm_name_table *pTable = NULL;
  int len = sizeof(g_cali_table) / sizeof(nvm_name_table);
  if (NULL == cmd) return NULL;
  for (i = 0; i < len; i++) {
    if (NULL == g_cali_table[i].itm) continue;
    if (0 != strcmp(g_cali_table[i].itm, cmd->itm)) continue;
    pTable = &g_cali_table[i];
    break;
  }
  return pTable;
}

static int wifi_nvm_buf_operate(char *pBuf, int file_len, const int type,
                                void *p_data) {
  int i, p;
  nvm_cali_cmd cmd;
  nvm_name_table *pTable = NULL;
  if ((NULL == pBuf) || (0 == file_len)) return -1;
  for (i = 0, p = 0; i < file_len; i++) {
    if (('\n' == *(pBuf + i)) || ('\r' == *(pBuf + i)) ||
        ('\0' == *(pBuf + i))) {
      if (5 <= (i - p)) {
        get_cmd_par((pBuf + p), &cmd);
        if (type == 1) {
          pTable = wifi_nvm_config_table_match(&cmd);
        } else if (type == 2) { /*calibration*/
          pTable = wifi_nvm_cali_table_match(&cmd);
        } else {
          RFDBG("%s(): unknow type\n", __func__);
          return -1;
        }

        if (NULL != pTable) {
          wifi_nvm_set_cmd(pTable, &cmd, p_data);
        }
      }
      p = i + 1;
    }
  }
  return 0;
}

int wifi_nvm_parse(const char *path, const int type, void *p_data) {
  int fd, ret;
  short len = 0;
  char pBuf[CONFIG_MAX_SIZE];

  RFDBG("open & read & parse %s\n", path);
  fd = open(path, O_RDONLY, 0666);
  if (fd < 0) {
    RFDBG("open %s fail[%d]\n", path, fd);
    return -1;
  }

  len = read(fd, pBuf, CONFIG_MAX_SIZE);
  if (len < 0) {
    RFDBG("read %s fail[%d]\n", path, len);
    close(fd);
    return -2;
  }
  close(fd);

  ret = wifi_nvm_buf_operate(pBuf, len, type, p_data);
  if (ret < 0) {
    RFDBG("parse %s fail[%d]\n", path, ret);
    return -2;
  }
  return 0;
}

static int add_to_file(char *buf, int len) {
  int fd, ret;

  fd = open(WIFI_CALI_FILE_WRITE_BACK, O_CREAT | O_WRONLY | O_TRUNC, 0666);
  if (fd < 0) {
    RFDBG("open %s fail[%d]\n", WIFI_CALI_FILE_WRITE_BACK, fd);
    return -1;
  }
  ret = write(fd, buf, len);
  if (ret < 0) {
    RFDBG("write %s fail[%d]\n", WIFI_CALI_FILE_WRITE_BACK, ret);
    close(fd);
    return -1;
  }
  close(fd);
  return 0;
}

int wlan_save_cali_data_to_file(wifi_cali_cp_t *data_cp) {
  char buf[CALI_MAX_SIZE];
  memset(buf, 0, sizeof(buf));
  char *p = buf;
  int i, j;
  int ret;
  char calibrating[128];
  int is_calibrated;

  wifi_cali_t *data = &data_cp->wifi_cali;

  if (data == NULL) return -1;

  p += SNPRINT(p, SECTION(0));
  p += SNPRINT(p, SECTION_LINE);
  p += SNPRINT(p, "# Marlin cali Version info\n");
  p += SNPRINT(p, SECTION_LINE);
  SNPRINTVAR(p, "cali_version = %d\n\n", data->cali_version);

  cali_config_t *cc = &data->cali_config;
  property_get("debug.connectivity.calibrating", calibrating, "false");
  if (!strncmp(calibrating, "true", 4)) {
    is_calibrated = 0;
  } else {
    is_calibrated = cc->is_calibrated;
  }

  p += SNPRINT(p, SECTION(1));
  p += SNPRINT(p, SECTION_LINE);
  p += SNPRINT(p, "# Calibration Config\n");
  p += SNPRINT(p, SECTION_LINE);
  p += SNPRINTVAR(p, "is_calibrated   = %d\n", is_calibrated);
  p += SNPRINTVAR(p, "rc_cali_en      = %d\n", cc->rc_cali_en);
  p += SNPRINTVAR(p, "dcoc_cali_en    = %d\n", cc->dcoc_cali_en);
  p += SNPRINTVAR(p, "txiq_cali_en    = %d\n", cc->txiq_cali_en);
  p += SNPRINTVAR(p, "rxiq_cali_en    = %d\n", cc->rxiq_cali_en);
  p += SNPRINTVAR(p, "txpower_cali_en = %d\n", cc->txpower_cali_en);
  p += SNPRINTVAR(p, "dpd_cali_en     = %d\n", cc->dpd_cali_en);
  p += SNPRINTVAR(p, "config_reserved = %d, %d, %d, %d\n\n",
                  cc->config_reserved[0], cc->config_reserved[1],
                  cc->config_reserved[2], cc->config_reserved[3]);
  rctune_cali_t *rc = &data->rctune_cali;
  p += SNPRINT(p, SECTION(2));
  p += SNPRINT(p, SECTION_LINE);
  p += SNPRINT(p, "# rc calibration data\n");
  p += SNPRINT(p, SECTION_LINE);
  p += SNPRINTVAR(p, "rctune_value    = 0x%x\n", rc->rctune_value);
  p += SNPRINTVAR(p, "rc_cali_reserved= 0x%x, 0x%x, 0x%x\n\n",
                  rc->rctune_reserved[0], rc->rctune_reserved[1],
                  rc->rctune_reserved[2]);
  dcoc_cali_t *dc = &data->dcoc_cali;
  unsigned short *dc_dcc = &dc->dcoc_cali_code[0];
  unsigned int *dc_dr = &dc->dcoc_reserved[0];
  p += SNPRINT(p, SECTION(3));
  p += SNPRINT(p, SECTION_LINE);
  p += SNPRINT(p, "# doco calibration data\n");
  p += SNPRINT(p, SECTION_LINE);
  p += SNPRINT(p, "dcoc_cali_code    = ");
  for (i = 0; i < 18; i++) {
    p += SNPRINTVAR(p, "0x%x", *dc_dcc);
    if (i != 18 - 1) p += SNPRINT(p, ", ");
    dc_dcc++;
  }
  p += SNPRINT(p, "\n");
  p += SNPRINTVAR(p, "dcoc_reserved   = 0x%x, 0x%x, 0x%x, 0x%x\n\n", dc_dr[0],
                  dc_dr[1], dc_dr[2], dc_dr[3]);
  txiq_cali_t *txc = &data->txiq_cali;
  unsigned int *txc_txr = &txc->txiq_reserved[0];
  p += SNPRINT(p, SECTION(4));
  p += SNPRINT(p, SECTION_LINE);
  p += SNPRINT(p, "# txiq calibration data\n");
  p += SNPRINT(p, SECTION_LINE);
  p += SNPRINTVAR(p, "rf_txiq_c11     = 0x%x\n", txc->rf_txiq_c11);
  p += SNPRINTVAR(p, "rf_txiq_c12     = 0x%x\n", txc->rf_txiq_c12);
  p += SNPRINTVAR(p, "rf_txiq_c22     = 0x%x\n", txc->rf_txiq_c22);
  p += SNPRINTVAR(p, "rf_txiq_dc      = 0x%x\n", txc->rf_txiq_dc);
  p += SNPRINTVAR(p, "txiq_reserved   = 0x%x, 0x%x, 0x%x, 0x%x\n", txc_txr[0],
                  txc_txr[1], txc_txr[2], txc_txr[3]);
  p += SNPRINT(p, "\n");

  rxiq_cali_t *rxc = &data->rxiq_cali;
  unsigned int *rxc_rxr = &rxc->rxiq_reserved[0];
  p += SNPRINT(p, SECTION(5));
  p += SNPRINT(p, SECTION_LINE);
  p += SNPRINT(p, "# rxiq calibration data\n");
  p += SNPRINT(p, SECTION_LINE);
  p += SNPRINTVAR(p, "rf_rxiq_coef21_22   = 0x%x\n", rxc->rf_rxiq_coef21_22);
  p += SNPRINTVAR(p, "rf_rxiq_coef11_12   = 0x%x\n", rxc->rf_rxiq_coef11_12);
  p += SNPRINTVAR(p, "rxiq_reserved       = 0x%x, 0x%x\n", rxc_rxr[0],
                  rxc_rxr[1]);
  p += SNPRINT(p, "\n");

  char *ch[14];
  txpower_cali_t *txpc = &data->txpower_cali;
  char *txpc_fdg = &txpc->txpower_freq_delta_gainindex[0];
  unsigned int *txpc_tr = &txpc->txpower_reserved[0];
  ch[0] = &txpc->txpower_temperature_delta_gm_channel1[0];
  ch[1] = &txpc->txpower_temperature_delta_gm_channel2[0];
  ch[2] = &txpc->txpower_temperature_delta_gm_channel3[0];
  ch[3] = &txpc->txpower_temperature_delta_gm_channel4[0];
  ch[4] = &txpc->txpower_temperature_delta_gm_channel5[0];
  ch[5] = &txpc->txpower_temperature_delta_gm_channel6[0];
  ch[6] = &txpc->txpower_temperature_delta_gm_channel7[0];
  ch[7] = &txpc->txpower_temperature_delta_gm_channel8[0];
  ch[8] = &txpc->txpower_temperature_delta_gm_channel9[0];
  ch[9] = &txpc->txpower_temperature_delta_gm_channel10[0];
  ch[10] = &txpc->txpower_temperature_delta_gm_channel11[0];
  ch[11] = &txpc->txpower_temperature_delta_gm_channel12[0];
  ch[12] = &txpc->txpower_temperature_delta_gm_channel13[0];
  ch[13] = &txpc->txpower_temperature_delta_gm_channel14[0];
  p += SNPRINT(p, SECTION(6));
  p += SNPRINT(p, SECTION_LINE);
  p += SNPRINT(p, "# txpower calibration data\n");
  p += SNPRINT(p, SECTION_LINE);
  p += SNPRINTVAR(p, "txpower_psat_temperature   = %d\n",
                  txpc->txpower_psat_temperature);
  p += SNPRINTVAR(p, "txpower_psat_gainindex   = %d\n",
                  txpc->txpower_psat_gainindex);
  p += SNPRINTVAR(p, "txpower_psat_power   = %d\n", txpc->txpower_psat_power);
  p += SNPRINTVAR(p, "txpower_psat_backoff   = %d\n",
                  txpc->txpower_psat_backoff);
  p += SNPRINTVAR(p, "txpower_psat_upper_limit   = %d\n",
                  txpc->txpower_psat_upper_limit);
  p += SNPRINTVAR(p, "txpower_psat_lower_limit   = %d\n",
                  txpc->txpower_psat_lower_limit);
  p += SNPRINT(p, "txpower_freq_delta_gainindex = ");
  for (i = 0; i < 14; i++) {
    p += SNPRINTVAR(p, "%d", *txpc_fdg);
    if (i != 14 - 1) p += SNPRINT(p, ", ");
    txpc_fdg++;
  }
  p += SNPRINT(p, "\n");
  for (i = 0; i < 14; i++) {
    p += SNPRINTVAR(p, "txpower_temperature_delta_gm_channel%d = ", i + 1);
    for (j = 0; j < 32; j++) {
      p += SNPRINTVAR(p, "%d", ch[i][j]);
      if (j != 32 - 1) p += SNPRINT(p, ", ");
    }
    p += SNPRINT(p, "\n");
  }
  p += SNPRINTVAR(p, "txpower_reserved   = %d, %d, %d, %d\n", txpc_tr[0],
                  txpc_tr[1], txpc_tr[2], txpc_tr[3]);

  dpd_cali_t *dpd = &data->dpd_cali;
  unsigned int *dpd_ch[3];
  dpd_ch[0] = &dpd->channel1_dpd_cali_table[0];
  dpd_ch[1] = &dpd->channel2_dpd_cali_table[0];
  dpd_ch[2] = &dpd->channel3_dpd_cali_table[0];

  p += SNPRINT(p, SECTION(7));
  p += SNPRINT(p, SECTION_LINE);
  p += SNPRINT(p, "# DPD calibration data\n");
  p += SNPRINT(p, SECTION_LINE);
  p +=
      SNPRINTVAR(p, "dpd_cali_channel_num   = %d\n", dpd->dpd_cali_channel_num);
  p += SNPRINTVAR(p, "dpd_cali_channel   = %d, %d, %d\n",
                  dpd->dpd_cali_channel[0], dpd->dpd_cali_channel[1],
                  dpd->dpd_cali_channel[2]);
  for (i = 0; i < 3; i++) {
    p += SNPRINTVAR(p, "channel%d_dpd_cali_table = ", i + 1);
    for (j = 0; j < 182; j++) {
      p += SNPRINTVAR(p, "0x%x", dpd_ch[i][j]);
      if (j != 182 - 1) p += SNPRINT(p, ", ");
    }
    p += SNPRINT(p, "\n");
  }
  p += SNPRINTVAR(p, "dpd_reserved   = 0x%x, 0x%x, 0x%x, 0x%x\n",
                  dpd->dpd_reserved[0], dpd->dpd_reserved[1],
                  dpd->dpd_reserved[2], dpd->dpd_reserved[3]);

  rf_para_t *rp = &data->rf_para;
  unsigned char *rp_rfc = &rp->rf_ctune[0];
  p += SNPRINT(p, SECTION(8));
  p += SNPRINT(p, SECTION_LINE);
  p += SNPRINT(p, "# RF parameters data\n");
  p += SNPRINT(p, SECTION_LINE);
  p += SNPRINT(p, "rf_ctune = ");
  for (i = 0; i < 14; i++) {
    p += SNPRINTVAR(p, "%d", *rp_rfc);
    if (i != 14 - 1) p += SNPRINT(p, ", ");
    rp_rfc++;
  }
  p += SNPRINT(p, "\n");
  p += SNPRINTVAR(p, "rf_reserved = %d, %d, %d, %d\n\n", rp->rf_reserved[0],
                  rp->rf_reserved[1], rp->rf_reserved[2], rp->rf_reserved[3]);

  tpc_cfg_t *tpc = &data->tpc_cfg;
  unsigned int *tpc_cfg = &tpc->tpc_cfg[0];
  p += SNPRINT(p, SECTION(9));
  p += SNPRINT(p, SECTION_LINE);
  p += SNPRINT(p, "# TPC Configuration data\n");
  p += SNPRINT(p, SECTION_LINE);
  p += SNPRINT(p, "tpc_cfg = ");
  for (i = 0; i < 37; i++) {
    p += SNPRINTVAR(p, "0x%x", *tpc_cfg);
    if (i != 37 - 1) p += SNPRINT(p, ", ");
    tpc_cfg++;
  }
  p += SNPRINT(p, "\n");
  p += SNPRINTVAR(p, "tpc_reserved = 0x%x, 0x%x, 0x%x, 0x%x\n\n",
                  tpc->tpc_reserved[0], tpc->tpc_reserved[1],
                  tpc->tpc_reserved[2], tpc->tpc_reserved[3]);
  p += SNPRINT(p, "\n");

#ifdef _WLAN_CALI_DEBUG_
  RFDBG("\n%s\n", buf);
#endif
  ret = add_to_file(buf, (int)(p - buf));
  if (ret < 0) {
    RFDBG("%s(), write buf to file\n", __func__);
    return -1;
  }
  return 0;
}

int get_connectivity_config_param(wifi_config_t *p) {
  int ret = 0;
  char config_ini_path[100] = {0};

  if (p == NULL) return -1;
#ifdef GET_MARLIN_CHIPID
  RFDBG("%s(), marlin.hardware.version=0x%x\n", __func__,
        marlin_img_flag.which_img_flag);
  if (BSL_REP_IS_2331_BA == marlin_img_flag.which_img_flag) {
    if (!access(SYSTEM_WIFI_BA_CONFIG_FILE, F_OK | R_OK)) {
      snprintf(config_ini_path, strlen(SYSTEM_WIFI_BA_CONFIG_FILE) + 1, "%s",
               SYSTEM_WIFI_BA_CONFIG_FILE);
    } else {
      RFDBG("%s(), SYSTEM_WIFI_BA_CONFIG_FILE is not exist\n", __func__);
      snprintf(config_ini_path, strlen(SYSTEM_WIFI_CONFIG_FILE) + 1, "%s",
               SYSTEM_WIFI_CONFIG_FILE);
    }
  } else {
    snprintf(config_ini_path, strlen(SYSTEM_WIFI_CONFIG_FILE) + 1, "%s",
             SYSTEM_WIFI_CONFIG_FILE);
  }
#else
  snprintf(config_ini_path, strlen(SYSTEM_WIFI_CONFIG_FILE) + 1, "%s",
           SYSTEM_WIFI_CONFIG_FILE);
#endif
  RFDBG("%s(), prepare to access to %s. \n", __func__, config_ini_path);
  ret = wifi_nvm_parse(config_ini_path, CONFIGURE_TYPE, (void *)p);
  if (0 != ret) {
    RFDBG("%s(),parse %s, err[%d]\n", __func__, config_ini_path, ret);
    return -1;
  }
  RFDBG("%s(), access to %s success. \n", __func__, config_ini_path);
  return 0;
}

int get_connectivity_cali_param(wifi_cali_t *p) {
  int ret = 0;
  char cali_ini_path[100] = {0};
  if (p == NULL) return -1;

#ifdef GET_MARLIN_CHIPID
  RFDBG("%s(), marlin.hardware.version=0x%x\n", __func__,
        marlin_img_flag.which_img_flag);
  if (BSL_REP_IS_2331_BA == marlin_img_flag.which_img_flag) {
    if (!access(SYSTEM_WIFI_BA_CALI_FILE, F_OK | R_OK)) {
      snprintf(cali_ini_path, strlen(SYSTEM_WIFI_BA_CALI_FILE) + 1, "%s",
               SYSTEM_WIFI_BA_CALI_FILE);
    } else {
      RFDBG("%s(), SYSTEM_WIFI_BA_CALI_FILE is not exist", __func__);
      snprintf(cali_ini_path, strlen(SYSTEM_WIFI_CALI_FILE) + 1, "%s",
               SYSTEM_WIFI_CALI_FILE);
    }
  } else {
    snprintf(cali_ini_path, strlen(SYSTEM_WIFI_CALI_FILE) + 1, "%s",
             SYSTEM_WIFI_CALI_FILE);
  }
#else
  snprintf(cali_ini_path, strlen(SYSTEM_WIFI_CALI_FILE) + 1, "%s",
           SYSTEM_WIFI_CALI_FILE);
#endif
  RFDBG("%s(), prepare to access to %s. \n", __func__, cali_ini_path);
  ret = wifi_nvm_parse(cali_ini_path, CALIBRATION_TYPE, (void *)p);
  if (0 != ret) {
    RFDBG("%s(),parse %s, err[%d]\n", __func__, cali_ini_path, ret);
    return -1;
  }
  RFDBG("%s(), access to %s success. \n", __func__, cali_ini_path);
  return 0;
}

int get_connectivity_rf_param(wifi_rf_t *p) {
  int ret = 0;
  ret = get_connectivity_config_param(&p->wifi_config);
  if (ret < 0) {
    RFDBG("%s(), get config param fail\n", __func__);
    return -1;
  }
  ret = get_connectivity_cali_param(&p->wifi_cali);
  if (ret < 0) {
    RFDBG("%s(),get cali param fail\n", __func__);
    return -2;
  }
  return 0;
}
