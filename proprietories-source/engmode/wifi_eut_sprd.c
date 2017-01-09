#include <stdlib.h>
#include <stdio.h>
#include "eut_opt.h"
#include <fcntl.h>
#include "engopt.h"

#include "wifi_eut_sprd.h"

#define NUM_ELEMS(x) (sizeof(x) / sizeof(x[0]))
#define wifi_eut_debug

#ifdef wifi_eut_debug

#define rsp_debug(rsp_)                               \
  do {                                                \
    ENG_LOG("%s(), rsp###:%s\n", __FUNCTION__, rsp_); \
  } while (0);

#define wifi_status_dump()                                             \
  do {                                                                 \
    ENG_LOG("%s(), eut_enter = %d, tx_start = %d, rx_start = %d\n",    \
            __FUNCTION__, g_wifi_data.eut_enter, g_wifi_data.tx_start, \
            g_wifi_data.rx_start);                                     \
  } while (0);

#else

#define rsp_debug(rsp_)

#endif

static unsigned int g_wifi_tx_count = 0;

static WIFI_ELEMENT g_wifi_data;
static WIFI_RATE g_wifi_rate_table[] = {
    {1, "DSSS-1"},   {2, "DSSS-2"},   {5, "CCK-5.6"},  {11, "CCK-11"},
    {6, "OFDM-6"},   {9, "OFDM-9"},   {12, "OFDM-12"}, {18, "OFDM-18"},
    {24, "OFDM-24"}, {36, "OFDM-36"}, {48, "OFDM-48"}, {54, "OFDM-54"},
    {7, "MCS-0"},    {13, "MCS-1"},   {19, "MCS-2"},   {26, "MCS-3"},
    {39, "MCS-4"},   {52, "MCS-5"},   {58, "MCS-6"},   {65, "MCS-7"},
};

static int mattch_rate_table_str(char *string) {
  int i;
  int ret = -1;
  for (i = 0; i < (int)NUM_ELEMS(g_wifi_rate_table); i++) {
    if (NULL != strstr(string, g_wifi_rate_table[i].name)) {
      ret = g_wifi_rate_table[i].rate;
      break;
    }
  }
  return ret;
}

static char *mattch_rate_table_index(int rate) {
  int i;
  int ret = -1;
  char *p = NULL;
  for (i = 0; i < (int)NUM_ELEMS(g_wifi_rate_table); i++) {
    if (rate == g_wifi_rate_table[i].rate) {
      p = g_wifi_rate_table[i].name;
      break;
    }
  }
  return p;
}

static int get_iwnpi_ret_status(void) {
  FILE *fp = NULL;
  char *str1 = NULL;
  char *str2 = NULL;
  int ret = -100;
  unsigned long len;
  char buf[TMP_BUF_SIZE] = {0};
  char tmp[6] = {0};
  // ENG_LOG("ADL entry %s(), TMP_FILE = %s", __func__, TMP_FILE);
  if (NULL == (fp = fopen(TMP_FILE, "r+"))) {
    ENG_LOG("no %s\n", TMP_FILE);
    return ret;
  }
  len = strlen(STR_RET_STATUS);
  while (!feof(fp)) {
    fgets(buf, TMP_BUF_SIZE, fp);
    str1 = strstr(buf, STR_RET_STATUS);
    str2 = strstr(buf, STR_RET_END);
    ENG_LOG("ADL %s(), buf = %s, str1 = %s, str2 = %s", __func__, buf, str1,
            str2);
    if ((NULL != str1) && (NULL != str2)) {
      len = (unsigned long)str2 - (unsigned long)str1 - len;
      if ((len >= 1) && (len <= 5)) {
        memcpy(tmp, str1 + strlen(STR_RET_STATUS), len);
        ret = atoi(tmp);
        break;
      }
    }
    memset(buf, 0, TMP_BUF_SIZE);
  }
  fclose(fp);
  return ret;
}

static int get_iwnpi_rssi_ret(void) {
  FILE *fp = NULL;
  char *str1 = NULL;
  char *str2 = NULL;
  int ret = -100;
  unsigned long len;
  char buf[TMP_BUF_SIZE] = {0};
  char tmp[6] = {0};

  if (NULL == (fp = fopen(TMP_FILE, "r+"))) {
    ENG_LOG("no %s\n", TMP_FILE);
    return ret;
  }

  len = strlen(STR_RET_RET);
  while (!feof(fp)) {
    fgets(buf, TMP_BUF_SIZE, fp);

    str1 = strstr(buf, STR_RET_RET);
    str2 = strstr(buf, STR_RET_END);
    ENG_LOG("ADL %s(), buf = %s, str1 = %s, str2 = %s", __func__, buf, str1,
            str2);

    if ((NULL != str1) && (NULL != str2)) {
      len = (unsigned long)str2 - (unsigned long)str1 - len;
      if ((len >= 1) && (len <= 5)) {
        memcpy(tmp, str1 + strlen(STR_RET_RET), len);
        ret = atoi(tmp);
        ENG_LOG("ADL %s(), tmp = %s, ret = %d", __func__, tmp, ret);

        break;
      }
    }
    memset(buf, 0, TMP_BUF_SIZE);
  }
  fclose(fp);
  return ret;
}

static int get_iwnpi_ret(char *tmp, char *start, char *end) {
  FILE *fp = NULL;
  char *str1 = NULL;
  char *str2 = NULL;
  int ret = -100;
  unsigned long len;
  char buf[TMP_BUF_SIZE] = {0};
  if ((NULL == tmp) || (NULL == start) || (NULL == end)) {
    ENG_LOG("%s(), par NULL\n", __FUNCTION__);
    return -1;
  }
  if (NULL == (fp = fopen(TMP_FILE, "r+"))) {
    ENG_LOG("no %s\n", TMP_FILE);
    return ret;
  }
  len = strlen(start);
  while (!feof(fp)) {
    fgets(buf, TMP_BUF_SIZE, fp);
    str1 = strstr(buf, start);
    str2 = strstr(buf, end);
    if ((NULL != str1) && (NULL != str2)) {
      len = (unsigned long)str2 - (unsigned long)str1 - len;
      if (len > 3) {
        memcpy(tmp, str1 + strlen(start), len);
        // sscanf(tmp, " rx_end_count=%d rx_err_end_count=%d fcs_fail_count=%d
        // ", &(cnt->rx_end_count), &(cnt->rx_err_end_count),
        // &(cnt->fcs_fail_count)  );
        ret = 0;
        break;
      }
    }
    memset(buf, 0, TMP_BUF_SIZE);
  }
  fclose(fp);
  return ret;
}

static int get_iwnpi_rxpktcnt(RX_PKTCNT *cnt) {
  FILE *fp = NULL;
  char *str1 = NULL;
  char *str2 = NULL;
  int ret = -100;
  unsigned long len;
  char buf[TMP_BUF_SIZE] = {0};
  char tmp[128] = {0};

  ENG_LOG("ADL entry %s()", __func__);
  if (NULL == (fp = fopen(TMP_FILE, "r+"))) {
    ENG_LOG("no %s\n", TMP_FILE);
    return ret;
  }

  len = strlen(STR_RET_REG_VALUE);
  while (!feof(fp)) {
    fgets(buf, TMP_BUF_SIZE, fp);
    str1 = strstr(buf, STR_RET_REG_VALUE);
    str2 = strstr(buf, STR_RET_END);
    if ((NULL != str1) && (NULL != str2)) {
      len = (unsigned long)str2 - (unsigned long)str1 - len;
      if (len > 10) {
        memcpy(tmp, str1 + strlen(STR_RET_REG_VALUE), len);
        sscanf(tmp, " rx_end_count=%d rx_err_end_count=%d fcs_fail_count=%d ",
               &(cnt->rx_end_count), &(cnt->rx_err_end_count),
               &(cnt->fcs_fail_count));
        ret = 0;
        break;
      }
    }
    memset(buf, 0, TMP_BUF_SIZE);
  }
  fclose(fp);

  ENG_LOG(
      "ADL leveling %s(), rx_end_count = %d, rx_err_end_count = %d, "
      "fcs_fail_count = %d",
      __func__, cnt->rx_end_count, cnt->rx_err_end_count, cnt->fcs_fail_count);
  return ret;
}

static int start_wifieut(char *rsp) {
  int ret;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN + 1] = {0};

  ENG_LOG("ADL entry %s(), line = %d", __func__, __LINE__);

  if (1 == g_wifi_data.eut_enter) {
    goto ok;
  }
#ifdef SPRD_WCN_MARLIN
  ret = system("wcnd_cli wcn poweron");
  ENG_LOG("ADL %s(), callED system(wcnd_cli wcn poweron), ret = %d", __func__,
          ret);
#endif
  ret = system("rmmod sprdwl.ko");
  ENG_LOG("ADL %s(), callED system(rmmod sprdwl.ko), ret = %d", __func__, ret);

  sprintf(cmd, "insmod %s", WIFI_DRIVER_MODULE_PATH);
  ret = system(cmd);
  ENG_LOG(
      "ADL %s(), callED system(insmod %s), ret = %d",
      __func__, WIFI_DRIVER_MODULE_PATH, ret);
  if (ret < 0) {
    ENG_LOG("%s, insmod %s err\n", __func__, WIFI_DRIVER_MODULE_PATH);
    goto err;
  }

#if 0 /*must be close cose in NON-SIG mode */
    system("ifconfig wlan0 up");
#endif

  sprintf(cmd, "iwnpi wlan0 start > %s", TMP_FILE);
  ret = system(cmd);
  ENG_LOG("ADL %s(), callED system(%s), ret = %d", __func__, cmd, ret);
  if (ret < 0) {
    ENG_LOG("%s, no iwnpi\n", __func__);
    goto err;
  }

  ret = get_iwnpi_ret_status();
  if (0 != ret) {
    ENG_LOG("iwnpi wlan0 start cmd  err_code:%d", ret);
    goto err;
  }

ok:
  strcpy(rsp, EUT_WIFI_OK);
  memset(&g_wifi_data, 0x00, sizeof(WIFI_ELEMENT));
  g_wifi_data.eut_enter = 1;
  rsp_debug(rsp);

  /* call init(), must be eut_enter is 1 */
  wifi_eut_init();

  ENG_LOG("ADL leaving %s(), return 0", __func__);
  return 0;

err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  g_wifi_data.eut_enter = 0;
  rsp_debug(rsp);
  return -1;
}

static int stop_wifieut(char *rsp) {
  int ret;
  char cmd[100] = {0};

  ENG_LOG("ADL entry %s(), line = %d", __func__, __LINE__);

  wifi_status_dump();
  if (1 == g_wifi_data.tx_start) {
    sprintf(cmd, "iwnpi wlan0 tx_stop > %s", TMP_FILE);
    ret = system(cmd);
    ENG_LOG("ADL %s(), callED system(%s), ret = %d", __func__, cmd, ret);
    if (0 != ret) {
      goto noiwnpi;
    }
  }

  if (1 == g_wifi_data.rx_start) {
    sprintf(cmd, "iwnpi wlan0 rx_stop > %s", TMP_FILE);
    ret = system(cmd);
    ENG_LOG("ADL %s(), callED system(%s), ret = %d", __func__, cmd, ret);
    if (0 != ret) {
      goto noiwnpi;
    }
  }

  sprintf(cmd, "iwnpi wlan0 stop > %s", TMP_FILE);
  ret = system(cmd);
  ENG_LOG("ADL %s(), callED system(%s), ret = %d", __func__, cmd, ret);
  if (ret < 0) {
    goto noiwnpi;
  }

  ret = get_iwnpi_ret_status();
  if (0 != ret) {
    ENG_LOG("iwnpi wlan0 stop cmd  err_code:%d", ret);
    goto err;
  }

#if 0
    system("ifconfig wlan0 down");
#endif
  system("rmmod sprdwl.ko");
#ifdef SPRD_WCN_MARLIN
  ret = system("wcnd_cli wcn poweroff ");
  ENG_LOG("ADL %s(), callED system(wcnd_cli wcn poweroff ), ret = %d", __func__,
          ret);
#endif
  memset(&g_wifi_data, 0, sizeof(WIFI_ELEMENT));
  strcpy(rsp, EUT_WIFI_OK);
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s()", __func__);
  return 0;

noiwnpi:
  ENG_LOG("%s, no iwnpi\n", __FUNCTION__);
err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  rsp_debug(rsp);
  return -1;
}

int wifi_eut_set(int cmd, char *rsp) {
  ENG_LOG("entry %s(), cmd = %d", __func__, cmd);
  if (cmd == 1) {
    start_wifieut(rsp);
  } else if (cmd == 0) {
    stop_wifieut(rsp);
  } else {
    ENG_LOG("%s(), cmd don't support", __func__);
    sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  }
  return 0;
}

int wifi_eut_get(char *rsp) {
  ENG_LOG("ADL entry %s(), eut_enter = %d", __func__, g_wifi_data.eut_enter);
  sprintf(rsp, "%s%d", EUT_WIFI_REQ, g_wifi_data.eut_enter);
  rsp_debug(rsp);
  return 0;
}

int wifi_rate_set(char *string, char *rsp) {
  int ret = -1;
  int rate = -1;
  char cmd[100] = {0};
  if (0 == g_wifi_data.eut_enter) {
    ENG_LOG("%s(), wifi_eut_enter:%d", __FUNCTION__, g_wifi_data.eut_enter);
    goto err;
  }
  rate = mattch_rate_table_str(string);
  ENG_LOG("%s(), rate:%d", __FUNCTION__, rate);
  if (-1 == rate) goto err;
  sprintf(cmd, "iwnpi wlan0 set_rate %d > %s", rate, TMP_FILE);
  ret = system(cmd);
  if (ret < 0) {
    ENG_LOG("no iwnpi\n");
    goto err;
  }
  ret = get_iwnpi_ret_status();
  if (0 != ret) {
    ENG_LOG("%s(), set_rate ret:%d", __FUNCTION__, ret);
    goto err;
  }

  g_wifi_data.rate = rate;
  strcpy(rsp, EUT_WIFI_OK);
  rsp_debug(rsp);
  return 0;
err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  rsp_debug(rsp);
  return -1;
}

int wifi_rate_get(char *rsp) {
  int ret = -1;
  char *str = NULL;
  ENG_LOG("%s()...\n", __FUNCTION__);
  if (0 == g_wifi_data.rate) goto err;
  str = mattch_rate_table_index(g_wifi_data.rate);
  if (NULL == str) {
    ENG_LOG("%s(), don't mattch rate", __FUNCTION__);
    goto err;
  }

  sprintf(rsp, "%s%s", WIFI_RATE_REQ_RET, str);
  rsp_debug(rsp);
  return 0;
err:
  sprintf(rsp, "%s%s", WIFI_RATE_REQ_RET, "null");
  rsp_debug(rsp);
  return -1;
}

int wifi_channel_set(int ch, char *rsp) {
  int ret = -1;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN + 1] = {0x00};

  ENG_LOG("ADL entry %s(), ch = %d", __func__, ch);
  if (0 == g_wifi_data.eut_enter) {
    ENG_LOG("%s(), wifi_eut_enter = %d", __func__, g_wifi_data.eut_enter);
    goto err;
  }

  if ((ch < 1) || (ch > 14)) {
    ENG_LOG("%s(), channel num err\n", __func__);
    goto err;
  }

  sprintf(cmd, "iwnpi wlan0 set_channel %d > %s", ch, TMP_FILE);
  ret = system(cmd);
  ENG_LOG("ADL %s(), callED system(%s), ret = %d", __func__, cmd, ret);
  if (ret < 0) {
    ENG_LOG("no iwnpi\n");
    goto err;
  }
  ret = get_iwnpi_ret_status();
  if (0 != ret) {
    ENG_LOG("%s(), set_channel err ret:%d\n", __func__, ret);
    goto err;
  }

  g_wifi_data.channel = ch;
  strcpy(rsp, EUT_WIFI_OK);
  rsp_debug(rsp);
  return 0;
err:

  g_wifi_data.channel = 0;
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  rsp_debug(rsp);
  return -1;
}

int wifi_channel_get(char *rsp) {
  ENG_LOG("%s(), channel:%d\n", __FUNCTION__, g_wifi_data.channel);
  if (0 == g_wifi_data.channel) {
    goto err;
  }

  sprintf(rsp, "%s%d", WIFI_CHANNEL_REQ_RET, g_wifi_data.channel);
  rsp_debug(rsp);
  return 0;
err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  rsp_debug(rsp);
  return -1;
}

int wifi_txgainindex_set(int index, char *rsp) {
  int ret = -1;
  char cmd[100] = {0};
  if (0 == g_wifi_data.eut_enter) {
    ENG_LOG("%s(), wifi_eut_enter:%d", __FUNCTION__, g_wifi_data.eut_enter);
    goto err;
  }
  ENG_LOG("%s(), index:%d\n", __FUNCTION__, index);
  sprintf(cmd, "iwnpi wlan0 set_tx_power %d > %s", index, TMP_FILE);
  ret = system(cmd);
  if (ret < 0) {
    ENG_LOG("no iwnpi\n");
    goto err;
  }
  ret = get_iwnpi_ret_status();
  if (0 != ret) {
    ENG_LOG("%s(), set_tx_power err ret:%d\n", __FUNCTION__, ret);
    goto err;
  }
  g_wifi_data.txgainindex = index;
  rsp_debug(rsp);
  strcpy(rsp, EUT_WIFI_OK);
  return 0;
err:
  g_wifi_data.txgainindex = -1;
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  rsp_debug(rsp);
  return -1;
}

int wifi_txgainindex_get(char *rsp) {
  int ret = -1;
  char cmd[100] = {0};
  int level_a, level_b;
  wifi_status_dump();
  if (0 == g_wifi_data.eut_enter) goto err;
  sprintf(cmd, "iwnpi wlan0 get_tx_power > %s", TMP_FILE);
  ret = system(cmd);
  if (ret < 0) {
    ENG_LOG("no iwnpi\n");
    goto err;
  }
  memset(cmd, 0, sizeof(cmd));
  ret = get_iwnpi_ret(cmd, STR_RET_RET, STR_RET_END);
  if (0 != ret) {
    ENG_LOG("%s(), get_tx_power run err\n", __FUNCTION__);
    goto err;
  }
  sscanf(cmd, "level_a:%d,level_b:%d\n", &level_a, &level_b);
  sprintf(rsp, "%s%d,%d", WIFI_TXGAININDEX_REQ_RET, level_a, level_b);
  rsp_debug(rsp);
  return 0;
err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  rsp_debug(rsp);
  return -1;
}

static int wifi_tx_start(char *rsp) {
  int ret;
  char cmd[128] = {0};

  ENG_LOG("ADL entry %s()", __func__);

  wifi_status_dump();
  if (1 == g_wifi_data.tx_start) {
    ENG_LOG("ADL %s(), tx_start is 1, goto ok.", __func__);
    goto ok;
  }

  if (1 == g_wifi_data.rx_start) {
    ENG_LOG("ADL %s(), Rx_start is 1, goto err. RX_START is 1", __func__);
    goto err;
  }

  if (1 == g_wifi_data.sin_wave_start) {
    ENG_LOG("ADL %s(), sin_wave_start is 1, goto err. SIN_WAVE_start is 1",
            __func__);
    goto err;
  }

  sprintf(cmd, "iwnpi wlan0 set_tx_count %d > %s", g_wifi_tx_count, TMP_FILE);
  ret = system(cmd);
  ENG_LOG("ADL %s(), callED system(%s), ret = %d", __func__, cmd, ret);
  if (ret < 0) {
    ENG_LOG("%s, no iwnpi\n", __func__);
    goto err;
  }

  /* reset to default, must be 0 */
  ENG_LOG("ADL %s(), reset to default, must be 0", __func__);
  g_wifi_tx_count = 0;

  ret = get_iwnpi_ret_status();
  if (0 != ret) {
    ENG_LOG("iwnpi wlan0 tx_start cmd  err_code:%d", ret);
    goto err;
  }

  sprintf(cmd, "iwnpi wlan0 tx_start > %s", TMP_FILE);
  ret = system(cmd);
  ENG_LOG("ADL %s(), callED system(%s), ret = %d", __func__, cmd, ret);

  if (ret < 0) {
    ENG_LOG("%s, no iwnpi\n", __func__);
    goto err;
  }

  ret = get_iwnpi_ret_status();
  if (0 != ret) {
    ENG_LOG("iwnpi wlan0 tx_start cmd  err_code:%d", ret);
    goto err;
  }

ok:
  g_wifi_data.tx_start = 1;
  strcpy(rsp, EUT_WIFI_OK);
  rsp_debug(rsp);
  return 0;

err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  g_wifi_data.tx_start = 0;
  rsp_debug(rsp);
  return -1;
}

static int wifi_tx_stop(char *rsp) {
  int ret;
  char cmd[128] = {0};

  ENG_LOG("ADL entry %s()", __func__);

  wifi_status_dump();

  if (0 == g_wifi_data.tx_start && 0 == g_wifi_data.sin_wave_start) {
    goto ok;
  }

  if (1 == g_wifi_data.rx_start) {
    goto err;
  }

  sprintf(cmd, "iwnpi wlan0 tx_stop > %s", TMP_FILE);
  ret = system(cmd);
  ENG_LOG("ADL %s(), callED system(%s), ret = %d", __func__, cmd, ret);
  if (ret < 0) {
    ENG_LOG("%s, no iwnpi\n", __FUNCTION__);
    goto err;
  }

  ret = get_iwnpi_ret_status();
  if (0 != ret) {
    ENG_LOG("iwnpi wlan0 tx_stop cmd  err_code:%d", ret);
    goto err;
  }
ok:
  g_wifi_data.tx_start = 0;
  g_wifi_data.sin_wave_start = 0;
  ENG_LOG("ADL %s(), case ok: set sin_wave_start & tx_start to 0. return 0",
          __func__);

  strcpy(rsp, EUT_WIFI_OK);
  rsp_debug(rsp);
  return 0;

err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  g_wifi_data.tx_start = 0;
  g_wifi_data.sin_wave_start = 0;
  ENG_LOG("ADL %s(), case err: set sin_wave_start & tx_start to 0. return -1",
          __func__);
  rsp_debug(rsp);
  return -1;
}

int wifi_tx_set(int command_code, int mode, int pktcnt, char *rsp) {
  ENG_LOG("ADL entry %s(), command_code = %d, mode = %d, pktcnt = %d", __func__,
          command_code, mode, pktcnt);

  if (0 == g_wifi_data.eut_enter) {
    ENG_LOG("ADL %s(), wifi_eut_enter = %d", __func__, g_wifi_data.eut_enter);
    sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
    return -1;
  }

  if ((1 == command_code) && (0 == g_wifi_data.rx_start)) {
    if (0 == mode) {
      /* continues */
      ENG_LOG("ADL %s(), set g_wifi_tx_count is 0", __func__);
      g_wifi_tx_count = 0;
    } else if (1 == mode) {
      if (pktcnt > 0) {
        ENG_LOG("ADL %s(), set g_wifi_tx_count is %d", __func__, pktcnt);
        g_wifi_tx_count = pktcnt;
      } else {
        ENG_LOG("ADL %s(), pktcnt is ERROR, pktcnt = %d", __func__, pktcnt);
        goto err;
      }
    }

    wifi_tx_start(rsp);
  } else if (0 == command_code) {
    wifi_tx_stop(rsp);
  } else {
    sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
    rsp_debug(rsp);
    return -1;
  }
  return 0;

err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  rsp_debug(rsp);
  return -1;
}

int wifi_tx_get(char *rsp) {
  ENG_LOG("%s()...\n", __FUNCTION__);
  sprintf(rsp, "%s%d", EUT_WIFI_TX_REQ, g_wifi_data.tx_start);
  rsp_debug(rsp);
  return 0;
}

static int wifi_rx_start(char *rsp) {
  int ret;
  char cmd[100] = {0};

  ENG_LOG("ADL entry %s(), rx_start = %d, tx_start = %d", __func__,
          g_wifi_data.rx_start, g_wifi_data.tx_start);

  wifi_status_dump();
  if (1 == g_wifi_data.rx_start) goto ok;
  if (1 == g_wifi_data.tx_start) goto err;

  sprintf(cmd, "iwnpi wlan0 rx_start > %s", TMP_FILE);

  ret = system(cmd);
  ENG_LOG("ADL %s(), callED system(%s), ret = %d", __func__, cmd, ret);
  if (ret < 0) {
    ENG_LOG("%s, no iwnpi\n", __FUNCTION__);
    goto err;
  }

  ret = get_iwnpi_ret_status();
  if (0 != ret) {
    ENG_LOG("iwnpi wlan0 rx_start cmd  err_code:%d", ret);
    goto err;
  }

ok:
  g_wifi_data.rx_start = 1;
  strcpy(rsp, EUT_WIFI_OK);
  rsp_debug(rsp);
  return 0;
err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  g_wifi_data.rx_start = 0;
  rsp_debug(rsp);
  return -1;
}

static int wifi_rx_stop(char *rsp) {
  int ret;
  char cmd[100] = {0};

  ENG_LOG("ADL entry %s(), rx_start = %d, tx_start = %d", __func__,
          g_wifi_data.rx_start, g_wifi_data.tx_start);

  wifi_status_dump();
  if (0 == g_wifi_data.rx_start) goto ok;
  if (1 == g_wifi_data.tx_start) goto err;
  sprintf(cmd, "iwnpi wlan0 rx_stop > %s", TMP_FILE);

  ret = system(cmd);
  ENG_LOG("ADL %s(), callED system(%s), ret = %d", __func__, cmd, ret);

  if (ret < 0) {
    ENG_LOG("%s, no iwnpi\n", __FUNCTION__);
    goto err;
  }
  ret = get_iwnpi_ret_status();
  if (0 != ret) {
    ENG_LOG("iwnpi wlan0 rx_stop cmd  err_code:%d", ret);
    goto err;
  }

ok:
  g_wifi_data.rx_start = 0;
  strcpy(rsp, EUT_WIFI_OK);
  rsp_debug(rsp);
  return 0;
err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  g_wifi_data.rx_start = 0;
  rsp_debug(rsp);
  return -1;
}

int wifi_rx_set(int command_code, char *rsp) {
  ENG_LOG("ADL entry %s(), command_code = %d", __func__, command_code);
  if (0 == g_wifi_data.eut_enter) {
    ENG_LOG("%s(), wifi_eut_enter:%d", __func__, g_wifi_data.eut_enter);
    sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
    return -1;
  }

  if ((command_code == 1) && (0 == g_wifi_data.tx_start)) {
    wifi_rx_start(rsp);
  } else if (command_code == 0) {
    wifi_rx_stop(rsp);
  } else {
    sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
    rsp_debug(rsp);
    return -1;
  }
  return 0;
}

int wifi_rx_get(char *rsp) {
  ENG_LOG("%s()...\n", __FUNCTION__);
  sprintf(rsp, "%s%d", EUT_WIFI_RX_REQ, g_wifi_data.rx_start);
  rsp_debug(rsp);
  return 0;
}

int wifi_rssi_get(char *rsp) {
  int ret;
  char cmd[100] = {0};
  wifi_status_dump();
  if (1 == g_wifi_data.tx_start) {
    goto err;
  }
  sprintf(cmd, "iwnpi wlan0 get_rssi > %s", TMP_FILE);
  ret = system(cmd);
  if (ret < 0) {
    ENG_LOG("%s, no iwnpi\n", __FUNCTION__);
    goto err;
  }

  ret = get_iwnpi_int_ret();
  if (-100 == ret) {
    ENG_LOG("iwnpi wlan0 get_rssi cmd  err_code:%d", ret);
    goto err;
  }

  sprintf(rsp, "%s%d", EUT_WIFI_RSSI_REQ_RET, ret);
  rsp_debug(rsp);
  return 0;
err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  rsp_debug(rsp);
  return -1;
}

int wifi_rxpktcnt_get(char *rsp) {
  int ret = -1;
  RX_PKTCNT *cnt = NULL;
  char cmd[100] = {0};

  ENG_LOG("ADL entry %s()", __func__);

  wifi_status_dump();
  if ((1 == g_wifi_data.tx_start)) {
    goto err;
  }

  sprintf(cmd, "iwnpi wlan0 get_rx_ok > %s", TMP_FILE);
  ret = system(cmd);
  ENG_LOG("ADL %s(), callED system(%s), ret = %d", __func__, cmd, ret);
  if (ret < 0) {
    ENG_LOG("%s, no iwnpi\n", __FUNCTION__);
    goto err;
  }

  cnt = &(g_wifi_data.cnt);
  ret = get_iwnpi_rxpktcnt(cnt);
  ENG_LOG("ADL %s(), callED get_iwnpi_rxpktcnt(), ret = %d", __func__, ret);
  if (0 != ret) {
    ENG_LOG("%s, no iwnpi\n", __FUNCTION__);
    goto err;
  }

  sprintf(rsp, "%s%d,%d,%d", EUT_WIFI_RXPKTCNT_REQ_RET,
          g_wifi_data.cnt.rx_end_count, g_wifi_data.cnt.rx_err_end_count,
          g_wifi_data.cnt.fcs_fail_count);
  rsp_debug(rsp);
  return 0;
err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  rsp_debug(rsp);
  return -1;
}

/********************************************************************
*   name   wifi_eut_init
*   ---------------------------
*   description: init some global's value when enter EUT mode
*   ----------------------------
*
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
static int wifi_eut_init(void) {
  int ret = -1;
  char rsp[WIFI_EUT_COMMAND_RSP_MAX_LEN + 1] = {0x00};

  ENG_LOG("ADL entry %s()", __func__);
  ret = wifi_channel_set(WIFI_EUT_DEFAULT_CHANNEL, rsp);
  ENG_LOG("ADL %s(), called wifi_channel_set(), ret = %d", __func__, ret);

  ENG_LOG("ADL leaving %s(), ret = %d", __func__, ret);
  return ret;
}

/********************************************************************
*   name   get_iwnpi_int_ret
*   ---------------------------
*   description: get int type result of Chip's command
*   ----------------------------
*   para        IN/OUT      type            note
*
*   ----------------------------------------------------
*   return
*   -100:error
*   other:return value
*   ------------------
*   other:
*
********************************************************************/
static int get_iwnpi_int_ret(void) {
  FILE *fp = NULL;
  char *str1 = NULL;
  char *str2 = NULL;
  int ret = WIFI_INT_INVALID_RET;
  char buf[TMP_BUF_SIZE] = {0};
  unsigned char ret_cnt = 0;

  ENG_LOG("ADL entry %s()", __func__);
  if (NULL == (fp = fopen(TMP_FILE, "r+"))) {
    ENG_LOG("no %s\n", TMP_FILE);
    return ret;
  }

  while (!feof(fp)) {
    fgets(buf, TMP_BUF_SIZE, fp);
    str1 = strstr(buf, STR_RET_RET);
    str2 = strstr(buf, STR_RET_END);

    ENG_LOG("ADL %s(), buf = %s, str1 = %s, str2 = %s", __func__, buf, str1,
            str2);

    if ((NULL != str1) && (NULL != str2)) {
      ret_cnt = sscanf(buf, "ret: %d:end",
                       &ret); /* must be match MARCO STR_RET_RET STR_RET_END */
      ENG_LOG("ADL %s(), ret_cnt = %d", __func__, ret_cnt);
      if (ret_cnt > 0) {
        ENG_LOG("ADL %s(), ret = %d, break", __func__, ret);
        break;
      }
    }
    memset(buf, 0, TMP_BUF_SIZE);
  }
  fclose(fp);

  ENG_LOG("ADL leaving %s(), ret = %d", __func__, ret);
  return ret;
}

/********************************************************************
*   name   get_iwnpi_string_ret
*   ---------------------------
*   description: get string type result of Chip's command
*   ----------------------------
*   para        IN/OUT      type            note
*   start       IN          const char *    string start by this param
*   end         IN          const char *    string end by this param
*   ----------------------------------------------------
*   return
*   -100:error
*   other:return value
*   ------------------
*   other:
*
********************************************************************/
static int get_iwnpi_string_ret(const char *start, const char *end,
                                char *outstr, unsigned char out_len) {
  FILE *fp = NULL;
  char *str1 = NULL;
  char *str2 = NULL;
  int ret = 0; /* must be 0*/
  unsigned long len = 0;
  char buf[TMP_BUF_SIZE] = {0};
  char tmp[TMP_BUF_SIZE] = {0x00};

  ENG_LOG("ADL entry %s(), start = %s, end = %s", __func__, start, end);
  if (NULL == (fp = fopen(TMP_FILE, "r+"))) {
    ENG_LOG("no %s\n", TMP_FILE);
    return WIFI_INT_INVALID_RET;
  }

  len = strlen(start);
  while (!feof(fp)) {
    fgets(buf, TMP_BUF_SIZE, fp);
    ENG_LOG("ADL %s(), buf = %s", __func__, buf);

    str1 = strstr(buf, start);
    str2 = strstr(buf, end);
    if ((NULL != str1) && (NULL != str2)) {
      len = (unsigned long)str2 - (unsigned long)str1 - len;
      if ((len >= 1) && (len <= 128)) {
        memcpy(tmp, str1 + strlen(start), len);

        break;
      }
    }
    memset(buf, 0x00, TMP_BUF_SIZE);
  }
  fclose(fp);

  memset(outstr, 0x00, out_len);
  memcpy(outstr, tmp, out_len);

  ENG_LOG("ADL leaving %s(), outstr = %s, ret = %d", __func__, outstr, ret);
  return ret;
}

/********************************************************************
*   name   get_iwnpi_tx_power_ret
*   ---------------------------
*   description: get int type result of Chip's command
*   ----------------------------
*   para        IN/OUT      type            note
*
*   ----------------------------------------------------
*   return
*   -100:error
*   other:return value
*   ------------------
*   other:
*
********************************************************************/
static int get_iwnpi_tx_power_ret(int *lva, int *lvb) {
  FILE *fp = NULL;
  char *str1 = NULL;
  char *str2 = NULL;
  int ret = 0;
  char buf[TMP_BUF_SIZE] = {0};
  unsigned char ret_cnt = 0;
  int level_a = 0;
  int level_b = 0;

  ENG_LOG("ADL entry %s()", __func__);
  if (NULL == (fp = fopen(TMP_FILE, "r+"))) {
    ENG_LOG("no %s\n", TMP_FILE);
    ret = WIFI_INT_INVALID_RET;
    return ret;
  }

  while (!feof(fp)) {
    fgets(buf, TMP_BUF_SIZE, fp);
    str1 = strstr(buf, STR_RET_RET);
    str2 = strstr(buf, STR_RET_END);

    ENG_LOG("ADL %s(), buf = %s, str1 = %s, str2 = %s", __func__, buf, str1,
            str2);

    if ((NULL != str1) && (NULL != str2)) {
      ret_cnt =
          sscanf(buf, "ret: level_a:%d,level_b:%d:end", &level_a,
                 &level_b); /* must be match MARCO STR_RET_RET STR_RET_END */
      ENG_LOG("ADL %s(), ret_cnt = %d", __func__, ret_cnt);
      if (ret_cnt > 0) {
        ENG_LOG("ADL %s(), ret = %d, break", __func__, ret);
        break;
      }
    }
    memset(buf, 0, TMP_BUF_SIZE);
  }
  fclose(fp);

  *lva = level_a;
  *lvb = level_b;

  ENG_LOG("ADL leaving %s(), level_a = %d, level_b = %d, ret = %d", __func__,
          level_a, level_b, ret);
  return ret;
}

/********************************************************************
*   name   wifi_lna_set
*   ---------------------------
*   description: Set lna switch of WLAN Chip
*   ----------------------------
*   para        IN/OUT      type            note
*   lna_on_off  IN          int             lna's status
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wifi_lna_set(int lna_on_off, char *rsp) {
  int ret = -1;
  int rate = -1;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN + 1] = {0x00};

  ENG_LOG("ADL entry %s(), lna_on_off = %d", __func__, lna_on_off);

  if (0 == g_wifi_data.eut_enter) {
    ENG_LOG("%s(), wifi_eut_enter:%d", __func__, g_wifi_data.eut_enter);
    goto err;
  }

  if (1 == lna_on_off) {
    sprintf(cmd, "iwnpi wlan0 lna_on > %s", TMP_FILE);
  } else if (0 == lna_on_off) {
    sprintf(cmd, "iwnpi wlan0 lna_off > %s", TMP_FILE);
  } else {
    ENG_LOG("%s(), lna_on_off is ERROR", __func__);
    goto err;
  }

  ENG_LOG("ADL %s(), cmd = %s", __func__, cmd);
  ret = system(cmd);
  if (ret < 0) {
    ENG_LOG("no iwnpi\n");
    goto err;
  }
  ret = get_iwnpi_ret_status();
  if (0 != ret) {
    ENG_LOG("%s(), set_rate ret:%d", __FUNCTION__, ret);
    goto err;
  }

  strcpy(rsp, EUT_WIFI_OK);
  rsp_debug(rsp);
  return 0;
err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  rsp_debug(rsp);

  return -1;
}

/********************************************************************
*   name   wifi_lna_get
*   ---------------------------
*   description: get Chip's LNA status by iwnpi command
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wifi_lna_get(char *rsp) {
  int ret = -1;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN + 1] = {0x00};

  ENG_LOG("ADL entry %s()", __func__);
  wifi_status_dump();

  if (0 == g_wifi_data.eut_enter) {
    ENG_LOG("%s(), wifi_eut_enter = %d", __func__, g_wifi_data.eut_enter);
    goto err;
  }

  sprintf(cmd, "iwnpi wlan0 lna_status > %s", TMP_FILE);
  ret = system(cmd);
  if (ret < 0) {
    ENG_LOG("%s, no iwnpi\n", __func__);
    goto err;
  }

  ret = get_iwnpi_int_ret();
  if (WIFI_INT_INVALID_RET == ret) {
    ENG_LOG("iwnpi wlan0 lna_status cmd, err_code = %d", ret);
    goto err;
  }

  sprintf(rsp, "%s%d", WIFI_LNA_STATUS_REQ_RET, ret);
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return 0", __func__, rsp);
  return 0;
err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
  return -1;
}

/********************************************************************
*   name   wifi_band_set
*   ---------------------------
*   description: set Chip's band(2.4G/5G) by iwnpi command set_band
*   ----------------------------
*   para        IN/OUT      type            note
*   band        IN          wifi_band       band number.
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   band:2.4G/5G
*
********************************************************************/
int wifi_band_set(wifi_band band, char *rsp) {
  int ret = -1;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN + 1] = {0x00};

  ENG_LOG("ADL entry %s(), band = %d", __func__, band);
  if (0 == g_wifi_data.eut_enter) {
    ENG_LOG("%s(), wifi_eut_enter =%d", __func__, g_wifi_data.eut_enter);
    goto err;
  }

  if (!(WIFI_BAND_2_4G == band || WIFI_BAND_5G == band)) {
    ENG_LOG("%s(), band num is err, band = %d", __func__, band);
    goto err;
  }

  snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 set_band %d > %s", band,
           TMP_FILE);

  ret = system(cmd);
  ENG_LOG("%s(), callED system, cmd = %s, ret = %d", __func__, cmd, ret);
  if (ret < 0) {
    ENG_LOG("ADL %s(), no iwnpi", __func__);
    goto err;
  }

  ret = get_iwnpi_ret_status();

  if (0 != ret) {
    ENG_LOG("%s(), set_band err ret:%d\n", __func__, ret);
    goto err;
  }

  strcpy(rsp, EUT_WIFI_OK);
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return 0", __func__, rsp);
  return 0;
err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
  return -1;
}

/********************************************************************
*   name   wifi_band_get
*   ---------------------------
*   description: Get Chip's band(2.4G/5G) by iwnpi command from wifi RF
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   band:2.4G/5G
*
********************************************************************/
int wifi_band_get(char *rsp) {
  int ret = -1;
  wifi_band band = 0;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN + 1] = {0x00};

  ENG_LOG("ADL entry %s()", __func__);

  snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 get_band > %s",
           TMP_FILE);
  ret = system(cmd);
  ENG_LOG("ADL %s(), called system(%s), ret = %d", __func__, cmd, ret);

  if (ret < 0) {
    ENG_LOG("ADL %s(), no iwnpi", __func__);
    goto err;
  }

  band = get_iwnpi_int_ret();
  if (WIFI_INT_INVALID_RET == ret) {
    ENG_LOG("iwnpi wlan0 get_band cmd, err_code = %d", ret);
    goto err;
  }

  snprintf(rsp, WIFI_EUT_COMMAND_RSP_MAX_LEN, "%s%d", WIFI_BAND_REQ_RET, band);
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return 0", __func__, rsp);
  return 0;

err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
  return -1;
}

/********************************************************************
*   name   wifi_bandwidth_set
*   ---------------------------
*   description: set Chip's band width by iwnpi command set_band_width
*   ----------------------------
*   para        IN/OUT      type            note
*   band        IN          wifi_bandwidth  band width.
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   20 40 80 160
********************************************************************/
int wifi_bandwidth_set(wifi_bandwidth band_width, char *rsp) {
  int ret = -1;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN + 1] = {0x00};

  ENG_LOG("ADL entry %s(), band_width = %d", __func__, band_width);
  if (0 == g_wifi_data.eut_enter) {
    ENG_LOG("%s(), wifi_eut_enter = %d", __func__, g_wifi_data.eut_enter);
    goto err;
  }

  if (band_width > WIFI_BANDWIDTH_160MHZ) {
    ENG_LOG("%s(), band num is err, band_width = %d", __func__, band_width);
    goto err;
  }

  snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 set_bandwidth %d > %s",
           band_width, TMP_FILE);

  ret = system(cmd);
  ENG_LOG("%s(), called system(%s), ret = %d", __func__, cmd, ret);
  if (ret < 0) {
    ENG_LOG("ADL %s(), no iwnpi", __func__);
    goto err;
  }

  ret = get_iwnpi_ret_status();

  if (0 != ret) {
    ENG_LOG("%s(), set_bandwidth err ret = %d", __func__, ret);
    goto err;
  }

  strcpy(rsp, EUT_WIFI_OK);
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return 0", __func__, rsp);
  return 0;
err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
  return -1;
}

/********************************************************************
*   name   wifi_bandwidth_get
*   ---------------------------
*   description: Get Chip's band width by iwnpi command from wifi RF
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   band width:20 40 80 160
*
********************************************************************/
int wifi_bandwidth_get(char *rsp) {
  int ret = -1;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN + 1] = {0x00};
  wifi_bandwidth band_width = WIFI_BAND_MAX_VALUE;

  ENG_LOG("ADL entry %s()", __func__);

  snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 get_bandwidth > %s",
           TMP_FILE);
  ret = system(cmd);
  ENG_LOG("ADL %s(), called system(%s), ret = %d", __func__, cmd, ret);

  if (ret < 0) {
    ENG_LOG("ADL %s(), no iwnpi", __func__);
    goto err;
  }

  band_width = get_iwnpi_int_ret();
  if (WIFI_INT_INVALID_RET == ret) {
    ENG_LOG("iwnpi wlan0 get_band cmd, err_code = %d", ret);
    goto err;
  }

  snprintf(rsp, WIFI_EUT_COMMAND_RSP_MAX_LEN, "%s%d", WIFI_BANDWIDTH_REQ_RET,
           band_width);
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return 0", __func__, rsp);
  return 0;

err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
  return -1;
}

/********************************************************************
*   name   wifi_tx_power_level_set
*   ---------------------------
*   description: set Chip's tx_power by iwnpi command set_tx_power
*   ----------------------------
*   para        IN/OUT      type            note
*   tx_power    IN          int             tx power
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_tx_power_level_set(int tx_power, char *rsp) {
  int ret = -1;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN + 1] = {0x00};

  ENG_LOG("ADL entry %s(), tx_power = %d", __func__, tx_power);
  if (0 == g_wifi_data.eut_enter) {
    ENG_LOG("%s(), wifi_eut_enter = %d", __func__, g_wifi_data.eut_enter);
    goto err;
  }

  if (tx_power < WIFI_TX_POWER_MIN_VALUE ||
      tx_power > WIFI_TX_POWER_MAX_VALUE) {
    ENG_LOG("%s(), tx_power's value is invalid, tx_power = %d", __func__,
            tx_power);
    goto err;
  }

  snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 set_tx_power %d > %s",
           tx_power, TMP_FILE);
  ret = system(cmd);
  ENG_LOG("ADL %s(), call system(%s), ret = %d", __func__, cmd, ret);

  if (ret < 0) {
    ENG_LOG("no iwnpi\n");
    goto err;
  }

  ret = get_iwnpi_ret_status();
  if (0 != ret) {
    ENG_LOG("%s(), set_tx_power_level err ret:%d\n", __func__, ret);
    goto err;
  }

  strcpy(rsp, EUT_WIFI_OK);
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return 0", __func__, rsp);
  return 0;

err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
  return -1;
}

/********************************************************************
*   name   wifi_tx_power_level_get
*   ---------------------------
*   description: get Chip's tx power leaving by iwnpi command get_tx_power
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_tx_power_level_get(char *rsp) {
  int ret = -1;
  int level_a = 0;
  int level_b = 0;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN + 1] = {0x00};

  ENG_LOG("ADL entry %s()", __func__);

  snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 get_tx_power > %s",
           TMP_FILE);
  ret = system(cmd);
  ENG_LOG("ADL %s(), called system(%s), ret = %d", __func__, cmd, ret);

  if (ret < 0) {
    ENG_LOG("ADL %s(). no iwnpi", __func__);
    goto err;
  }

  get_iwnpi_tx_power_ret(&level_a, &level_b);

  snprintf(rsp, WIFI_EUT_COMMAND_RSP_MAX_LEN, "%s%d,%d",
           WIFI_TX_POWER_LEVEL_REQ_RET, level_a, level_b);
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return 0", __func__, rsp);
  return 0;

err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
  return -1;
}

/********************************************************************
*   name   wifi_pkt_length_set
*   ---------------------------
*   description: set Chip's pkt length by iwnpi command set_pkt_length
*   ----------------------------
*   para        IN/OUT      type            note
*   pkt_len     IN          int             pkt length
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_pkt_length_set(int pkt_len, char *rsp) {
  int ret = -1;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN + 1] = {0x00};

  ENG_LOG("ADL entry %s(), pkt_len = %d", __func__, pkt_len);
  if (0 == g_wifi_data.eut_enter) {
    ENG_LOG("%s(), wifi_eut_enter = %d", __func__, g_wifi_data.eut_enter);
    goto err;
  }

  if (pkt_len < WIFI_PKT_LENGTH_MIN_VALUE ||
      pkt_len > WIFI_PKT_LENGTH_MAX_VALUE) {
    ENG_LOG("%s(), tx_power's value is invalid, pkt_len = %d", __func__,
            pkt_len);
    goto err;
  }

  snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 set_pkt_length %d > %s",
           pkt_len, TMP_FILE);
  ret = system(cmd);
  ENG_LOG("ADL %s(), call system(%s), ret = %d", __func__, cmd, ret);

  if (ret < 0) {
    ENG_LOG("no iwnpi\n");
    goto err;
  }

  ret = get_iwnpi_ret_status();
  if (0 != ret) {
    ENG_LOG("%s(), set_pkt_length err ret:%d\n", __func__, ret);
    goto err;
  }

  strcpy(rsp, EUT_WIFI_OK);
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return 0", __func__, rsp);
  return 0;

err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
  return -1;
}

/********************************************************************
*   name   wifi_pkt_length_get
*   ---------------------------
*   description: get Chip's pkt length by iwnpi command get_tx_power
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_pkt_length_get(char *rsp) {
  int ret = -1;
  int pkt_length = 0;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN + 1] = {0x00};

  ENG_LOG("ADL entry %s()", __func__);

  snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 get_pkt_length > %s",
           TMP_FILE);
  ret = system(cmd);
  ENG_LOG("ADL %s(), call system(%s), ret = %d", __func__, cmd, ret);

  if (ret < 0) {
    ENG_LOG("no iwnpi\n");
    goto err;
  }

  pkt_length = get_iwnpi_int_ret();
  if (WIFI_INT_INVALID_RET == ret) {
    ENG_LOG("iwnpi wlan0 get_pkt_length cmd, err_code = %d", ret);
    goto err;
  }

  snprintf(rsp, WIFI_EUT_COMMAND_RSP_MAX_LEN, "%s%d", WIFI_PKT_LENGTH_REQ_RET,
           pkt_length);
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return 0", __func__, rsp);
  return 0;

err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
  return -1;
}

/********************************************************************
*   name   wifi_tx_mode_set
*   ---------------------------
*   description: set Chip's tx mode by iwnpi command set_tx_mode
*   ----------------------------
*   para        IN/OUT      type            note
*   tx_mode     IN          wifi_tx_mode    tx mode
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_tx_mode_set(wifi_tx_mode tx_mode, char *rsp) {
  int ret = -1;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN + 1] = {0x00};

  ENG_LOG("ADL entry %s(), tx_mode = %d", __func__, tx_mode);
  if (0 == g_wifi_data.eut_enter) {
    ENG_LOG("%s(), wifi_eut_enter = %d", __func__, g_wifi_data.eut_enter);
    goto err;
  }

  if (tx_mode > WIFI_TX_MODE_LOCAL_LEAKAGE) {
    ENG_LOG("%s(), tx_power's value is invalid, tx_mode = %d", __func__,
            tx_mode);
    goto err;
  }

  /* if tx mode is CW,must be tx stop when tx start is 1, rather, call sin_wave
   */
  if (WIFI_TX_MODE_CW == tx_mode) {
    ENG_LOG("ADL %s(), tx_mode is CW, tx_start = %d", __func__,
            g_wifi_data.tx_start);
    if (1 == g_wifi_data.tx_start) {
      snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 tx_stop > %s",
               TMP_FILE);
      ret = system(cmd);
      ENG_LOG("ADL %s(), called system(%s), ret = %d", __func__, cmd, ret);

      if (ret < 0) {
        ENG_LOG("ADL %s(), no iwnpi", __func__);
        goto err;
      }

      ret = get_iwnpi_ret_status();
      if (0 != ret) {
        ENG_LOG("%s(), set_tx_mode, err ret = %d", __func__, ret);
        goto err;
      }

      ENG_LOG("ADL %s(), set tx_start to 0", __func__);
      g_wifi_data.tx_start = 0;
    }

    snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 sin_wave > %s",
             TMP_FILE);
    ret = system(cmd);
    ENG_LOG("ADL %s(), called system(%s), ret = %d", __func__, cmd, ret);

    if (ret < 0) {
      ENG_LOG("ADL %s(), no iwnpi", __func__);
      goto err;
    }

    ret = get_iwnpi_ret_status();
    if (0 != ret) {
      ENG_LOG("%s(), set_tx_mode, err ret = %d", __func__, ret);
      goto err;
    }

    g_wifi_data.sin_wave_start = 1;
    ENG_LOG("ADL %s(), set sin_wave_start is 1.", __func__);

  } else /* other mode, send value to CP2 */
  {
    ENG_LOG("ADL %s(), tx_mode is other mode, mode = %d", __func__, tx_mode);
    snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 set_tx_mode %d > %s",
             tx_mode, TMP_FILE);
    ret = system(cmd);
    ENG_LOG("ADL %s(), called system(%s), ret = %d", __func__, cmd, ret);

    if (ret < 0) {
      ENG_LOG("ADL %s(), no iwnpi", __func__);
      goto err;
    }

    ret = get_iwnpi_ret_status();
    if (0 != ret) {
      ENG_LOG("%s(), set_tx_mode, err ret = %d", __func__, ret);
      goto err;
    }
  }

  strcpy(rsp, EUT_WIFI_OK);
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return 0", __func__, rsp);
  return 0;

err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
  return -1;
}

/********************************************************************
*   name   wifi_tx_mode_get
*   ---------------------------
*   description: get Chip's tx mode by iwnpi command tx_mode
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_tx_mode_get(char *rsp) {
  int ret = -1;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN + 1] = {0x00};
  wifi_tx_mode tx_mode = WIFI_TX_MODE_INVALID;

  ENG_LOG("ADL entry %s()", __func__);

  snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 get_tx_mode > %s",
           TMP_FILE);
  ret = system(cmd);
  ENG_LOG("ADL %s(), called system(%s), ret = %d", __func__, cmd, ret);

  if (ret < 0) {
    ENG_LOG("ADL %s(), no iwnpi", __func__);
    goto err;
  }

  tx_mode = (wifi_tx_mode)get_iwnpi_int_ret();
  if (WIFI_INT_INVALID_RET == ret) {
    ENG_LOG("iwnpi wlan0 get_tx_mode cmd, err_code = %d", ret);
    goto err;
  }

  snprintf(rsp, WIFI_EUT_COMMAND_RSP_MAX_LEN, "%s%d", WIFI_TX_MODE_REQ_RET,
           tx_mode);
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return 0", __func__, rsp);
  return 0;

err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
  return -1;
}

/********************************************************************
*   name   wifi_preamble_set
*   ---------------------------
*   description: set Chip's preamble type by iwnpi command set_preamble
*   ----------------------------
*   para            IN/OUT      type                    note
*   preamble_type   IN          wifi_preamble_type      tx power
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_preamble_set(wifi_preamble_type preamble_type, char *rsp) {
  int ret = -1;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN + 1] = {0x00};

  ENG_LOG("ADL entry %s(), preamble_type = %d", __func__, preamble_type);
  if (0 == g_wifi_data.eut_enter) {
    ENG_LOG("%s(), wifi_eut_enter = %d", __func__, g_wifi_data.eut_enter);
    goto err;
  }

  if (preamble_type > PREAMBLE_TYPE_80211N_GREEN_FIELD) {
    ENG_LOG("%s(), preamble's value is invalid, preamble_type = %d", __func__,
            preamble_type);
    goto err;
  }

  snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 set_preamble %d > %s",
           preamble_type, TMP_FILE);
  ret = system(cmd);
  ENG_LOG("ADL %s(), called system(%s), ret = %d", __func__, cmd, ret);

  if (ret < 0) {
    ENG_LOG("ADL %s(), no iwnpi", __func__);
    goto err;
  }

  ret = get_iwnpi_ret_status();
  if (0 != ret) {
    ENG_LOG("%s(), set_preamble err ret = %d", __func__, ret);
    goto err;
  }

  strcpy(rsp, EUT_WIFI_OK);
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return 0", __func__, rsp);
  return 0;

err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
  return -1;
}

/********************************************************************
*   name   wifi_preamble_get
*   ---------------------------
*   description: get Chip's preamble type by iwnpi command get_preamble
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_preamble_get(char *rsp) {
  int ret = -1;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN + 1] = {0x00};
  wifi_preamble_type preamble_type = PREAMBLE_TYPE_MAX_VALUE;

  ENG_LOG("ADL entry %s()", __func__);

  snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 get_preamble > %s",
           TMP_FILE);
  ret = system(cmd);
  ENG_LOG("ADL %s(), called system(%s), ret = %d", __func__, cmd, ret);

  if (ret < 0) {
    ENG_LOG("ADL %s(). no iwnpi", __func__);
    goto err;
  }

  preamble_type = (wifi_tx_mode)get_iwnpi_int_ret();
  if (WIFI_INT_INVALID_RET == ret) {
    ENG_LOG("iwnpi wlan0 get_tx_mode cmd, err_code = %d", ret);
    goto err;
  }

  snprintf(rsp, WIFI_EUT_COMMAND_RSP_MAX_LEN, "%s%d", WIFI_PREAMBLE_REQ_RET,
           (int)preamble_type);
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return 0", __func__, rsp);
  return 0;

err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
  return -1;
}

/********************************************************************
*   name   wifi_payload_set
*   ---------------------------
*   description: set Chip's payload by iwnpi command set_payload
*   ----------------------------
*   para            IN/OUT      type                    note
*   payload         IN          wifi_payload            payload
*   rsp             OUT         char *                  response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_payload_set(wifi_payload payload, char *rsp) {
  int ret = -1;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN + 1] = {0x00};

  ENG_LOG("ADL entry %s(), payload = %d", __func__, payload);
  if (0 == g_wifi_data.eut_enter) {
    ENG_LOG("%s(), wifi_eut_enter = %d", __func__, g_wifi_data.eut_enter);
    goto err;
  }

  if (payload > PAYLOAD_RANDOM) {
    ENG_LOG("%s(), payload's value is invalid, payload = %d", __func__,
            payload);
    goto err;
  }

  snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 set_payload %d > %s",
           payload, TMP_FILE);
  ret = system(cmd);
  ENG_LOG("ADL %s(), called system(%s), ret = %d", __func__, cmd, ret);

  if (ret < 0) {
    ENG_LOG("ADL %s(), no iwnpi", __func__);
    goto err;
  }

  ret = get_iwnpi_ret_status();
  if (0 != ret) {
    ENG_LOG("%s(), set_payload err ret = %d", __func__, ret);
    goto err;
  }

  strcpy(rsp, EUT_WIFI_OK);
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return 0", __func__, rsp);
  return 0;

err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
  return -1;
}

/********************************************************************
*   name   wifi_payload_get
*   ---------------------------
*   description: get Chip's payload by iwnpi command get_payload
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_payload_get(char *rsp) {
  int ret = -1;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN + 1] = {0x00};
  wifi_payload payload = PAYLOAD_MAX_VALUE;

  ENG_LOG("ADL entry %s()", __func__);

  snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 get_payload > %s",
           TMP_FILE);
  ret = system(cmd);
  ENG_LOG("ADL %s(), called system(%s), ret = %d", __func__, cmd, ret);

  if (ret < 0) {
    ENG_LOG("ADL %s(). no iwnpi", __func__);
    goto err;
  }

  payload = (wifi_tx_mode)get_iwnpi_int_ret();
  if (WIFI_INT_INVALID_RET == ret) {
    ENG_LOG("iwnpi wlan0 get_tx_mode cmd, err_code = %d", ret);
    goto err;
  }

  snprintf(rsp, WIFI_EUT_COMMAND_RSP_MAX_LEN, "%s%d", WIFI_PAYLOAD_REQ_RET,
           (int)payload);
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return 0", __func__, rsp);
  return 0;

err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
  return -1;
}

/********************************************************************
*   name   wifi_guardinterval_set
*   ---------------------------
*   description: set Chip's guard interval by iwnpi command set_guard_interval
*   ----------------------------
*   para            IN/OUT      type                    note
*   gi              IN          wifi_guard_interval     guard interval
*   rsp             OUT         char *                  response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_guardinterval_set(wifi_guard_interval gi, char *rsp) {
  int ret = -1;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN + 1] = {0x00};

  ENG_LOG("ADL entry %s(), gi = %d", __func__, gi);
  if (0 == g_wifi_data.eut_enter) {
    ENG_LOG("%s(), wifi_eut_enter = %d", __func__, g_wifi_data.eut_enter);
    goto err;
  }

  if (gi > GUARD_INTERVAL_MAX_VALUE) {
    ENG_LOG("%s(), guard interval's value is invalid, gi = %d", __func__, gi);
    goto err;
  }

  snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN,
           "iwnpi wlan0 set_guard_interval %d > %s", gi, TMP_FILE);
  ret = system(cmd);
  ENG_LOG("ADL %s(), called system(%s), ret = %d", __func__, cmd, ret);

  if (ret < 0) {
    ENG_LOG("ADL %s(), no iwnpi", __func__);
    goto err;
  }

  ret = get_iwnpi_ret_status();
  if (0 != ret) {
    ENG_LOG("%s(), set_guard_interval err ret = %d", __func__, ret);
    goto err;
  }

  strcpy(rsp, EUT_WIFI_OK);
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return 0", __func__, rsp);
  return 0;

err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
  return -1;
}

/********************************************************************
*   name   wifi_guardinterval_get
*   ---------------------------
*   description: get Chip's guard interval by iwnpi command get_guard_interval
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_guardinterval_get(char *rsp) {
  int ret = -1;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN + 1] = {0x00};
  wifi_guard_interval gi = GUARD_INTERVAL_MAX_VALUE;

  ENG_LOG("ADL entry %s()", __func__);

  snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 get_guard_interval > %s",
           TMP_FILE);
  ret = system(cmd);
  ENG_LOG("ADL %s(), called system(%s), ret = %d", __func__, cmd, ret);

  if (ret < 0) {
    ENG_LOG("ADL %s(). no iwnpi", __func__);
    goto err;
  }

  gi = (wifi_tx_mode)get_iwnpi_int_ret();
  if (WIFI_INT_INVALID_RET == ret) {
    ENG_LOG("iwnpi wlan0 get_tx_mode cmd, err_code = %d", ret);
    goto err;
  }

  snprintf(rsp, WIFI_EUT_COMMAND_RSP_MAX_LEN, "%s%d",
           WIFI_GUARDINTERVAL_REQ_RET, (int)gi);
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return 0", __func__, rsp);
  return 0;

err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
  return -1;
}

/********************************************************************
*   name   wifi_mac_filter_set
*   ---------------------------
*   description: set Chip's mac filter by iwnpi command set_macfilter
*   ----------------------------
*   para            IN/OUT      type                    note
*   on_off          IN          int                     on or off mac filter
*function
*   mac             IN          const char*             mac addr as string
*format
*   rsp             OUT         char *                  response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_mac_filter_set(int on_off, const char *mac, char *rsp) {
  int ret = -1;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN + 1] = {0x00};
  char mac_str[WIFI_MAC_STR_MAX_LEN + 1] = {0x00};

  ENG_LOG("ADL entry %s(), on_off = %d, mac = %s", __func__, on_off, mac);
  if (0 == g_wifi_data.eut_enter) {
    ENG_LOG("%s(), wifi_eut_enter = %d", __func__, g_wifi_data.eut_enter);
    goto err;
  }

  if (on_off < 0 || on_off > 1) {
    ENG_LOG("%s(), on_off's value is invalid, on_off = %d", __func__, on_off);
    goto err;
  }

  /* parese mac as string if on_off is 1 */
  if (1 == on_off) {
    unsigned char len = 0;

    if (strlen(mac) < (WIFI_MAC_STR_MAX_LEN + 2 + strlen("\r\n"))) {
      ENG_LOG("%s(), mac's length is invalid, len = %lu", __func__,
              strlen(mac));
      goto err;
    }

    if (mac && *mac == '\"') {
      /* skip first \" */
      mac++;
    }

    while (mac && *mac != '\"') {
      mac_str[len++] = *mac;
      mac++;
    }

    /* first, execute iwnpi wlan0 set_macfilter to set enable or disable */
    snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 set_macfilter %d > %s",
             on_off, TMP_FILE);

    ret = system(cmd);
    ENG_LOG("ADL %s(), callED system(%s), ret = %d", __func__, cmd, ret);
    if (ret < 0) {
      ENG_LOG("ADL %s(), no iwnpi", __func__);
      goto err;
    }

    ret = get_iwnpi_ret_status();
    if (0 != ret) {
      ENG_LOG("%s(), callED set_macfilter is ERROR, ret = %d", __func__, ret);
      goto err;
    }

    /* second, execute iwnpi wlan0 set_mac set MAC assress */
    memset(cmd, 0x00, WIFI_EUT_COMMAND_MAX_LEN);
    snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 set_mac %s > %s",
             mac_str, TMP_FILE);

    ret = system(cmd);
    ENG_LOG("ADL %s(), callED system(%s), ret = %d", __func__, cmd, ret);

    if (ret < 0) {
      ENG_LOG("ADL %s(), no iwnpi", __func__);
      goto err;
    }

  } else if (0 == on_off) {
    snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 set_macfilter %d > %s",
             on_off, TMP_FILE);

    ret = system(cmd);
    ENG_LOG("ADL %s(), callED system(%s), ret = %d", __func__, cmd, ret);

    if (ret < 0) {
      ENG_LOG("ADL %s(), no iwnpi", __func__);
      goto err;
    }

    ret = get_iwnpi_ret_status();
    if (0 != ret) {
      ENG_LOG("%s(), callED set_macfilter is ERROR, ret = %d", __func__, ret);
      goto err;
    }
  }

  strcpy(rsp, EUT_WIFI_OK);
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return 0", __func__, rsp);
  return 0;

err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
  return -1;
}

/********************************************************************
*   name   wifi_mac_filter_get
*   ---------------------------
*   description: get Chip's mac filter switch and mac addr if on, by iwnpi
*command get_macfilter
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_mac_filter_get(char *rsp) {
  int ret = -1;
  int on_off = 0;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN + 1] = {0x00};

  ENG_LOG("ADL entry %s()", __func__);

  /* inquery mac filter switch */
  snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 get_macfilter > %s",
           TMP_FILE);
  ret = system(cmd);
  ENG_LOG("ADL %s(), called system(%s), ret = %d", __func__, cmd, ret);

  if (ret < 0) {
    ENG_LOG("ADL %s(). no iwnpi", __func__);
    goto err;
  }

  ret = get_iwnpi_int_ret();
  if (WIFI_INT_INVALID_RET == ret) {
    ENG_LOG("iwnpi wlan0 lna_status cmd, err_code = %d", ret);
    goto err;
  }

  on_off = ret;

  if (1 == on_off) {
    char mac_str[WIFI_MAC_STR_MAX_LEN + 1] = {0x00};

    /* the mac filter switch is on, so get MAC address by iwnpi get_mac command
     */
    memset(cmd, 0x00, WIFI_EUT_COMMAND_MAX_LEN);

    snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 get_mac > %s",
             TMP_FILE);
    ret = system(cmd);
    ENG_LOG("ADL %s(), called system(%s), ret = %d", __func__, cmd, ret);

    ret = get_iwnpi_string_ret(STR_RET_MAC_VALUE, STR_RET_END, mac_str,
                               WIFI_MAC_STR_MAX_LEN);
    if (WIFI_INT_INVALID_RET == ret) {
      ENG_LOG("iwnpi wlan0 lna_status cmd, err_code = %d", ret);
      goto err;
    }

    snprintf(rsp, WIFI_EUT_COMMAND_RSP_MAX_LEN, "%s%d,\"%s\"",
             WIFI_MAC_FILTER_REQ_RET, on_off, mac_str);
  } else if (0 == on_off) {
    snprintf(rsp, WIFI_EUT_COMMAND_RSP_MAX_LEN, "%s%d", WIFI_MAC_FILTER_REQ_RET,
             on_off);
  } else {
    ENG_LOG("%s(), on_off's value is invalid, on_off = %d", __func__, on_off);
    goto err;
  }

  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return 0", __func__, rsp);
  return 0;

err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  rsp_debug(rsp);

  ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
  return -1;
}

/********************************end of the
 * file*************************************/
