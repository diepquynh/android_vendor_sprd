#include <stdio.h>
#include <stdlib.h>
#include "eut_opt.h"
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include "eng_pcclient.h"
//#include "eng_log2kmsg.h"
//#include "engopt.h"

#ifndef NUM_ELEMS(x)
#define NUM_ELEMS(x) (sizeof(x) / sizeof(x[0]))
#endif
#define COUNTER_BUF_SIZE 2048

#define WIFI_RATE_REQ_RET "+SPWIFITEST:RATE="
#define EUT_WIFI_RSSI_REQ_RET "+SPWIFITEST:RSSI="
#define WIFI_TXGAININDEX_REQ_RET "+SPWIFITEST:TXGAININDEX="

#define EUT_WIFI_START "START"
#define EUT_WIFI_TXTEST "TXTEST"
#define EUT_WIFI_RXTEST "RXTEST"
#define EUT_WIFI_CWTEST "CWTEST"
#define EUT_WIFI_STOP "STOP"
#define EUT_WIFI_TXSTOP "TXSTOP"
#define EUT_WIFI_RXPKTCNT "RXPKTCNT"

#ifndef WIFI_DRIVER_FW_PATH_PARAM
#define WIFI_DRIVER_FW_PATH_PARAM "/sys/module/bcmdhd/parameters/firmware_path"
#endif
#ifndef WIFI_DRIVER_FW_PATH_MFG
//#define WIFI_DRIVER_FW_PATH_MFG "system/vendor/firmware/fw_bcmdhd_mfg.bin"
// aaaaaaaaaaaaaaaaa
#define WIFI_DRIVER_FW_PATH_MFG "/vendor/firmware/fw_bcmdhd_mfg.bin"
#endif

#ifdef STR(str)
#undef STR
#endif
#define STR(str) str

static int wifieut_state = 0;
static int wifiwork_mode;
static float ratio_p;
static int channel_p;
static int tx_power_factor;
static int tx_power_factor_p;
static int tx_power_lv = -1;
static int tx_state;
static int tx_tone = 1;

static int rx_state;
static int rx_ratio;
static int rx_channel;
static int rx_power;
static long rx_packcount;
static long rxmfrmocast_priv;
static long rxmfrmocast_next;
static long rxdfrmocast;

static int set_band = 0;
static int sinpwrlv = 40;
static int wifitxmod = 1;
static int set_bw = 0;
static int set_pktlen = 1024;
static int s_bandwidth;
static int s_preamble_type;
static int s_rate_index = 1;

char cmd_set_ratio[20];
char cmd_set_rate_bw[60] = {0};
char counter_respon[COUNTER_BUF_SIZE];


struct eng_wifi_eutops wifi_eutops ={   wifieut,
    wifieut_req,
    wifiband,
    wifiband_req,
    wifi_tx_mode_bcm,
    wifi_tx_mode_req,
    wifi_tx_pwrlv,
    wifi_tx_pwrlv_req,
    wifi_tx,
    set_wifi_tx_factor,
    wifi_rx,
    set_wifi_mode,
    set_wifi_ratio,
    set_wifi_ch,
    wifi_tx_req,
    wifi_tx_factor_req,
    wifi_rx_req,
    wifi_ratio_req,
    wifi_ch_req,
    wifi_rxpackcount,
    wifi_clr_rxpackcount,
    set_wifi_rate,				//int (*set_wifi_rate)(char *, char *);
    wifi_rate_req,				//int (*wifi_rate_req)(char *);
    set_wifi_txgainindex,		//int (*set_wifi_txgainindex)(int , char *);
    wifi_txgainindex_req,		//int (*wifi_txgainindex_req)(char *);
    wifi_rssi_req,				//int (*wifi_rssi_req)(char *);
    wifi_pkt_len_set,
    wifi_pkt_len_get,
    wifi_bw_get,
    wifi_bw_set

};

struct wifi_ratio2mode {
  int mode;
  float ratios[10];
};

struct wifi_ratio2mode mode_list[] = {
    {WIFI_MODE_11B, {1, 2, 5.5, 11}},
    {WIFI_MODE_11G, {6, 9, 12, 18, 24, 36, 48, 54}},
    {WIFI_MODE_11N, {6.5, 13, 19.5, 26, 39, 52, 58.5, 65}}};

static int get_reflect_factor(int factor, int mode);
static long parse_packcount(char *filename);
int wifi_cw(char *result);

typedef struct {
  int index;
  char *rate;
  char *name;
} WIFI_RATE;

static WIFI_RATE g_wifi_rate_table[] = 
{
	{1, "1", "DSSS-1"},
	{2, "2", "DSSS-2"},
	{3, "5.5", "CCK-5.5"},
	{4, "11", "CCK-11"},
	{5, "6", "OFDM-6"},
	{6, "9", "OFDM-9"},
	{7, "12", "OFDM-12"},
	{8, "18", "OFDM-18"},
	{9, "24", "OFDM-24"},
	{10, "36", "OFDM-36"},
	{11, "48", "OFDM-48"},
	{12, "54", "OFDM-54"},
	{13, "0", "MCS-0"},
	{14, "1", "MCS-1"},
	{15, "2", "MCS-2"},
	{16, "3", "MCS-3"},
	{17, "4", "MCS-4"},
	{18, "5", "MCS-5"},
	{19, "6", "MCS-6"},
	{20, "7", "MCS-7"},
	{21, "8", "MCS-8"},
	{22, "9", "MCS-9"},
};

static int mattch_rate_table_str(char *string) {
  int i;
  int ret = 0;
  for (i = 0; i < (int)NUM_ELEMS(g_wifi_rate_table); i++) {
    if (NULL != strstr(string, g_wifi_rate_table[i].name)) {
      ret = g_wifi_rate_table[i].index;
      break;
    }
  }
  return ret;
}

static char *mattch_rate_table_index(int index) {
  int i;
  char *p = NULL;
  for (i = 0; i < (int)NUM_ELEMS(g_wifi_rate_table); i++) {
    if (index == g_wifi_rate_table[i].index) {
      p = g_wifi_rate_table[i].name;
      break;
    }
  }
  return p;
}

static char *mattch_rate_table_rate(int index) {
  int i;
  char *p = NULL;
  for (i = 0; i < (int)NUM_ELEMS(g_wifi_rate_table); i++) {
    if (index == g_wifi_rate_table[i].index) {
      p = g_wifi_rate_table[i].rate;
      break;
    }
  }
  return p;
}

int wifieut(int command_code, char *rsp) {
  ALOGI("wifieut");
  if (command_code == 1)
    start_wifieut(rsp);
  else if (command_code == 0)
    end_wifieut(rsp);
  return 0;
}
int wifieut_req(char *rsp) {
  sprintf(rsp, "%s%d", EUT_WIFI_REQ, wifieut_state);
  return 0;
}
int wifi_tx(int command_code, char *rsp) {
  ALOGI("wifi_tx");
  if (wifitxmod == 0 && command_code == 1) {
    wifi_cw(rsp);
  } else {
    if (command_code == 1)
      start_wifi_tx(rsp);
    else if (command_code == 0)
      end_wifi_tx(rsp);
  }
  return 0;
}
int wifi_rx(int command_code, char *rsp) {
  ALOGI("wifi_rx");
  if (command_code == 1)
    start_wifi_rx(rsp);
  else if (command_code == 0)
    end_wifi_rx(rsp);
  return 0;
}

/*
PM    set driver power management mode:
        0: CAM (constantly awake)
        1: PS  (power-save)
        2: FAST PS mode
*/
int wifi_pm(int command_code, char *result) {
  ALOGI("wifi_pm");
  int error = -1;
  if (command_code == 0) {
    error = system("wl PM 0");
    ALOGI("wl PM 0");
    if (error == -1 || error == 127) {
      ALOGE("=== wifi_pm failed on cmd : wl PM 0 ===\n");
      sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
      return -1;
    } else {
      error = system("wl mpc 0");
      ALOGI("wl PM 0");
      if (error == -1 || error == 127) {
        ALOGE("=== wifi_pm failed on cmd : wl mpc 0 ===\n");
        sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
        return -1;
      } else {
        error = system("wl scansuppress 1");
        ALOGI("wl scansuppress 1");
        if (error == -1 || error == 127) {
          ALOGE("=== wifi_pm failed on cmd : wl scansuppress 1 ===\n");
          sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
          return -1;
        }
      }
    }
  } else if (command_code == 1) {
    error = system("wl PM 2");
    ALOGI("wl PM 2");
    if (error == -1 || error == 127) {
      ALOGE("=== wifi_pm failed on cmd : wl PM 2 ===\n");
      sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
      return -1;
    } else {
      error = system("wl mpc 1");
      ALOGI("wl PM 1");
      if (error == -1 || error == 127) {
        ALOGE("=== wifi_pm failed on cmd : wl mpc 1 ===\n");
        sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
        return -1;
      } else {
        error = system("wl scansuppress 0");
        ALOGI("wl scansuppress 0");
        if (error == -1 || error == 127) {
          ALOGE("=== wifi_pm failed on cmd : wl scansuppress 0 ===\n");
          sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
          return -1;
        }
      }
    }
  } else {
    ALOGE("=== wifi_pm failed ,cmd = %d is not supported!\n", command_code);
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    return -1;
  }
  ALOGI("=== WIFIEUT wifi_pm  succeed! ===\n");
  strcpy(result, EUT_WIFI_OK);
  return 0;
}

int wifi_cw(char *result) {
  char cmd_set[60] = {0};
  char cmd_set_channel[30] = {0};
  ALOGI("wifi_cw");
  char band = 'a';
  if (set_band == 0) {
    band = 'b';
  } else if (set_band == 1) {
    band = 'a';
  }
  sprintf(cmd_set_channel, "wl channel %d", channel_p);

  int error = system("wl down");
  ALOGI("wl down");
  if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
    ALOGE("=== wifi_cw test failed on cmd : wl down ===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    goto cw_out;
  }
  error = system("wl mpc 0");
  ALOGI("wl mpc 0");
  if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
    ALOGE("=== wifi_cw test failed on cmd : wl mpc 0 ===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    goto cw_out;
  }
  error = system("wl phy_watchdog 0");
  ALOGI("wl phy_watchdog 0");
  if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
    ALOGE("=== wifi_cw test failed on cmd : wl mpc 0 ===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    goto cw_out;
  }
  error = system("wl up");
  ALOGI("wl up");
  if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
    ALOGE("=== wifi_cw test failed on cmd : wl up ===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    goto cw_out;
  }
  error = system("wl country ALL");
  ALOGI("wl country ALL");
  if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
    ALOGE("=== wifi_cw test failed on cmd : wl country ALL ===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    goto cw_out;
  }
  sprintf(cmd_set, "wl band %c", band);
  error = system(cmd_set);
  ALOGI("%s", cmd_set);
  if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
    ALOGE("=== wifi_cw test failed on cmd : wl band ===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    goto cw_out;
  }
  error = system(cmd_set_channel);
  ALOGI("%s", cmd_set_channel);
  if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
    ALOGE("=== wifi_cw test failed on cmd : wl  channel %d===\n", channel_p);
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    goto cw_out;
  }
  sprintf(cmd_set, "wl phy_tx_tone %d", tx_tone);
  error = system(cmd_set);
  ALOGI("%s", cmd_set);
  if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
    ALOGE("=== wifi_cw test failed on cmd : wl phy_tx_tone 400===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    goto cw_out;
  }
  sprintf(cmd_set, "wl phy_txpwrindex %d", sinpwrlv);
  error = system(cmd_set);
  ALOGI("%s", cmd_set);
  if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
    ALOGE("=== wifi_cw test failed on cmd : wl phy_watchdog 0===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    goto cw_out;
  } else {
    ALOGI("=== WIFIEUT CW test succeed! ===\n");
    strcpy(result, EUT_WIFI_OK);
    tx_state = 1;
  }
  return 0;
cw_out:
  return 0;
}

int start_wifieut(char *result) {
  ALOGI("start_wifieut--------0315--------------");

  int error = system("ifconfig wlan0 down");
  ALOGI("ifconfig wlan0 down :%d", error);

  if (-1 == error) {
    ALOGI("ifconfig wlan0 down system error");
  } else {
    if (WIFEXITED(error)) {
      if (WEXITSTATUS(error) == 0) {
        ALOGI("run successfully \n");
      } else {
        ALOGI("run failed:%d \n", WEXITSTATUS(error));
      }
    } else {
      ALOGI("exit code %d \n", WIFEXITED(error));
    }
  }

  if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
    ALOGE("=== start_wifieut test failed on cmd : ifconfig wlan0 down ===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
  } else {
    error = system("echo -n " STR(WIFI_DRIVER_FW_PATH_MFG) " > " STR(
        WIFI_DRIVER_FW_PATH_PARAM));
    ALOGI(
        "echo -n /vendor/firmware/fw_bcmdhd_mfg.bin > "
        "/sys/module/bcmdhd/parameters/firmware_path :%d",
        error);
    if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
      ALOGE("=== start_wifieut test failed on cmd : echo -n " STR(
          WIFI_DRIVER_FW_PATH_MFG) " > " STR(WIFI_DRIVER_FW_PATH_PARAM) "\n");
      sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    } else {
      error = system("ifconfig wlan0 up");
      ALOGI("ifconfig wlan0 up :%d", error);
      if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
        ALOGE(
            "=== start_wifieut test failed on cmd : ifconfig wlan0 down ===\n");
        sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
      } else {
        ALOGI("=== WIFIEUT test succeed! ===\n");
        wifieut_state = 1;
        strcpy(result, EUT_WIFI_OK);
      }
    }
  }
  return 0;
}

int end_wifieut(char *result) {
  int error = system("ifconfig wlan0 down");
  if (error == -1 || error == 127) {
    ALOGE("=== start_wifieut test failed on cmd : ifconfig wlan0 down ===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
  } else {
    wifieut_state = 0;
    strcpy(result, EUT_WIFI_OK);
  }
  return 0;
}

int start_wifi_tx(char *result) {
  ALOGI("start_wifi_tx-----------------");
  char cmd_set_channel[20] = {0};
  char cmd_set_pwrlv[25] = {0};
  char cmd_set_band[60] = {0};
  char band = 'b';
  if (set_band == 0) {
    band = 'b';
  } else if (set_band == 1) {
    band = 'a';
  }

  if (channel_p <= 14) {
    sprintf(cmd_set_channel, "wl channel %d", channel_p);
  } else {
    if (set_bw == 0)
      sprintf(cmd_set_channel, "wl channel %d/%d", channel_p, s_bandwidth);
    else
      sprintf(cmd_set_channel, "wl chanspec %d/%d", channel_p, s_bandwidth);
  }
  if(tx_power_lv == -1){
	sprintf(cmd_set_pwrlv, "wl txpwr1 %d", tx_power_lv);
  }
  else{
  	sprintf(cmd_set_pwrlv, "wl txpwr1 -o -d %d", tx_power_lv);
  }
  sprintf(cmd_set_band, "wl band %c", band);
  wifi_sync_rate_bw();

  int error = system("wl down");
  ALOGI("wl down: %d", error);
  if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
    ALOGE("=== start_wifi_tx test failed on cmd : wl down===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    goto tx_out;
  }
  error = system("wl mpc 0");
  ALOGI("wl mpc 0");
  if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
    ALOGE("=== start_wifi_tx test failed on cmd : wl mpc 0 ===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    goto tx_out;
  }
  error = system("wl phy_watchdog 0");
  ALOGI("wl phy_watchdog 0");
  if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
    ALOGE("=== start_wifi_tx test failed on cmd : wl phy_watchdog ===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    goto tx_out;
  }
  error = system("wl up");
  ALOGI("wl up");
  if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
    ALOGE("=== start_wifi_tx test failed on cmd : wl up ===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    goto tx_out;
  }
  error = system("wl country ALL");
  ALOGI("wl country ALL");
  if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
    ALOGE("=== start_wifi_tx test failed on cmd : wl country ALL ===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    goto tx_out;
  }
  error = system(cmd_set_band);
  ALOGI("%s", cmd_set_band);
  if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
    ALOGE("=== start_wifi_tx test failed on cmd : wl band ===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    goto tx_out;
  }
  error = system(cmd_set_rate_bw);
  ALOGI("%s", cmd_set_rate_bw);
  if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
    ALOGE(
        "=== start_wifi_tx test failed on cmd : wl 2g_rate -r 11 -b 20 ===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    goto tx_out;
  }
  error = system(cmd_set_channel);
  ALOGI("%s", cmd_set_channel);
  if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
    ALOGE("=== start_wifi_tx test failed on cmd : wl channel ===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    goto tx_out;
  }
  error = system("wl phy_forcecal 1");
  ALOGI("wl phy_forcecal 1");
  if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
    ALOGE("=== start_wifi_tx test failed on cmd : wl phy_forcecal 1 ===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    goto tx_out;
  }
  error = system("wl phy_activecal");
  ALOGI("wl phy_activecal");
  if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
    ALOGE("=== start_wifi_tx test failed on cmd :wl phy_activecal ===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    goto tx_out;
  }
  error = system("wl scansuppress 1");
  ALOGI("wl scansuppress 1");
  if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
    ALOGE("=== start_wifi_tx test failed on cmd : wl scansuppress 1 ===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    goto tx_out;
  }
  error = system(cmd_set_pwrlv);
  ALOGI("%s", cmd_set_pwrlv);
  if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
    ALOGE("=== start_wifi_tx test failed on cmd : wl txpwr1===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    goto tx_out;
  }
  error = system("wl pkteng_stop tx");
  ALOGI("wl pkteng_stop tx");
  if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
    ALOGE("=== start_wifi_tx test failed on cmd : wl pkteng_stop tx ===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    goto tx_out;
  }
  error = system("wl pkteng_start 00:11:22:33:44:55 tx 100 1024 0");
  ALOGI("wl pkteng_start 00:11:22:33:44:55 tx 100 1024 0");
  if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
    ALOGE("=== start_wifi_tx test failed on cmd : wl pkteng_start ===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    goto tx_out;
  }
  ALOGI("=== WIFIEUT test succeed! ===\n");
  strcpy(result, EUT_WIFI_OK);
  tx_state = 1;

  return 0;

tx_out:
  return 0;
}

int end_wifi_tx(char *result) {
  int error = system("wl down");
  ALOGI("end_wifi_tx");
  if (error == -1 || error == 127) {
    ALOGE("=== end_wifi_tx test failed on cmd : wl down ===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
  } else {
    ALOGI("=== end_wifi_tx test succeed! ===\n");
    strcpy(result, EUT_WIFI_OK);
    tx_state = 0;
  }
  return 0;
}

int start_wifi_rx(char *result) {
  ALOGI("start_wifi_rx-----------------");
  char cmd_set_channel[30] = {0};
  char cmd_rx_set_band[60] = {0};
  char rx_band = 'b';
  char *req = NULL;
  if (set_band == 0) {
    rx_band = 'b';
  } else if (set_band == 1) {
    rx_band = 'a';
  }
  sprintf(cmd_rx_set_band, "wl band %c", rx_band);
  if (channel_p <= 14) {
    sprintf(cmd_set_channel, "wl channel %d", channel_p);
  } else {
    if (set_bw == 0)
      sprintf(cmd_set_channel, "wl channel %d/%d", channel_p, s_bandwidth);
    else
      sprintf(cmd_set_channel, "wl chanspec %d/%d", channel_p, s_bandwidth);
  }
  // wifi_sync_rate_bw();
  // wifi_pm(0,req);
  int error = system("wl down");
  ALOGI("wl down");
  if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
    ALOGE("=== start_wifi_rx test failed on cmd : wl down ===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    goto rx_out;
  }
  error = system("wl mpc 0");
  ALOGI("wl mpc 0");
  if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
    ALOGE("=== start_wifi_rx test failed on cmd : wl mpc 0 ===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    goto rx_out;
  }
  error = system("wl phy_watchdog 0");
  ALOGI("wl phy_watchdog 0");
  if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
    ALOGE("=== start_wifi_rx test failed on cmd : wl phy_watchdog 0===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    goto rx_out;
  }
  error = system("wl up");
  ALOGI("wl up");
  if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
    ALOGE("=== start_wifi_tx test failed on cmd : wl up===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    goto rx_out;
  }
  error = system("wl country ALL");
  ALOGI("wl country ALL");
  if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
    ALOGE("=== start_wifi_rx test failed on cmd : wl country ALL ===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    goto rx_out;
  }
  error = system(cmd_rx_set_band);
  ALOGI("%s", cmd_rx_set_band);
  if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
    ALOGE("=== start_wifi_rx test failed on cmd : wl band ===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    goto rx_out;
  }
  error = system(cmd_set_channel);
  ALOGI("%s", cmd_set_channel);
  if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
    ALOGE("=== start_wifi_tx test failed on cmd : wl channel =\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    goto rx_out;
  }
  error = system("wl phy_forcecal 1");
  ALOGI("wl phy_forcecal 1");
  if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
    ALOGE("=== start_wifi_rx test failed on cmd : wl phy_forcecal 1 ===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    goto rx_out;
  }
  error = system("wl phy_activecal");
  ALOGI("wl phy_activecal");
  if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
    ALOGE("=== start_wifi_rx test failed on cmd : phy_activecal ===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    goto rx_out;
  }
  error = system("wl scansuppress 1");
  ALOGI("wl scansuppress 1");
  if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
    ALOGE("=== start_wifi_rx test failed on cmd : wl scansuppress 1===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    goto rx_out;
  }
  error = system("wl pkteng_start 00:11:22:33:44:55 rx");
  ALOGE("wl pkteng_start 00:11:22:33:44:55 rx\n");
  if (!WIFEXITED(error) || WEXITSTATUS(error) || (-1 == error)) {
    ALOGE(
        "=== start_wifi_rx test failed on cmd : wl pkteng_start "
        "00:11:22:33:44:55 rx ===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
    goto rx_out;
  }
  rx_state = 1;
  strcpy(result, EUT_WIFI_OK);
  return 0;
rx_out:
  return 0;
}

int end_wifi_rx(char *result) {
  int error = system("wl down");
  ALOGI("end_wifi_rx");
  if (error == -1 || error == 127) {
    ALOGE("=== end_wifi_rx test failed on cmd : wl down ===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
  } else {
    ALOGI("=== end_wifi_rx  succeed! ===\n");
    strcpy(result, EUT_WIFI_OK);
    rx_state = 0;
  }
  return 0;
}

int set_wifi_ratio(float ratio, char *result) {
  ratio_p = ratio;
  ALOGI("=== set_wifi_ratio----> %f === %f", ratio_p, ratio);
  int i, m, j, n;
  int break_tag = 0;
  n = (int)NUM_ELEMS(mode_list);
  for (i = 0; i < n; i++) {
    for (j = 0; j < (int)NUM_ELEMS(mode_list[i].ratios); j++) {
      if (ratio == mode_list[i].ratios[j]) {
        wifiwork_mode = mode_list[i].mode;
        m = j;
        ALOGI("find the ratio %f  wifiwork_mode=%d", ratio, wifiwork_mode);
        break_tag = 1;
        break;
      }
    }
    if (break_tag) break;
    if (i == n - 1) {
      sprintf(result, "%s%d", EUT_WIFI_ERROR, EUT_WIFIERR_RATIO);
      ratio_p = 0;

      goto fun_out;
    }
  }
  switch (wifiwork_mode) {
    case WIFI_MODE_11B:
    case WIFI_MODE_11G:
      sprintf(cmd_set_ratio, "wl rate %2.1f", ratio_p);
      ALOGI("set ratio cmd is %s", cmd_set_ratio);
      break;
    case WIFI_MODE_11N:
      sprintf(cmd_set_ratio, "wl nrate -m %d", m);
      ALOGI("set ratio cmd is %s", cmd_set_ratio);
      break;
  }
  strcpy(result, EUT_WIFI_OK);
fun_out:
  return 0;
}
int set_wifi_ch(int ch, char *result) {
  channel_p = ch;
  ALOGI("=== set_wifi_ch ----> %d ===\n", channel_p);
  strcpy(result, "+SPWIFITEST:OK");
  return 0;
}

// for *#*#83780#*#*
int set_wifi_tx_factor_83780(int factor, char *result) {
  tx_power_factor = factor;
  ALOGI("=== set_wifi_tx_factor----> %d===\n", tx_power_factor);
  if (tx_power_factor) {
    strcpy(result, EUT_WIFI_OK);
  } else {
    sprintf(result, "%s%d", EUT_WIFI_ERROR, EUT_WIFIERR_TXFAC_SETRATIOFIRST);
  }

  return 0;
}

int set_wifi_tx_factor(long factor, char *result) {
  if ((factor >= 1) && (factor <= 32767)) {
    tx_power_factor = get_reflect_factor(factor, wifiwork_mode);
    tx_power_factor_p = factor;
    ALOGI("=== set_wifi_tx_factor----> %d===\n", tx_power_factor);
    if (tx_power_factor) {
      strcpy(result, EUT_WIFI_OK);
    } else {
      sprintf(result, "%s%d", EUT_WIFI_ERROR, EUT_WIFIERR_TXFAC_SETRATIOFIRST);
    }
  } else {
    sprintf(result, "%s%d", EUT_WIFI_ERROR, EUT_WIFIERR_TXFAC);
  }
  return 0;
}
static int get_reflect_factor(int factor, int mode) {
  int reflect_factor_p;
  ALOGI("get_reflect_factor factor=%d mode=%d", factor, mode);
  switch (mode) {
    case WIFI_MODE_11B:
      reflect_factor_p = reflect_factor(6, 16, factor);
      break;
    case WIFI_MODE_11G:
      reflect_factor_p = reflect_factor(6, 10, factor);
      break;
    case WIFI_MODE_11N:
      reflect_factor_p = reflect_factor(6, 15, factor);
      break;
    default:
      return -1;
  }
  return reflect_factor_p;
}
static int reflect_factor(int start, int end, int factor) {
  int n = end - start + 1;
  return (factor / 32767) * n + start;
}

int set_wifi_mode(char *mode, char *result) { return 0; }
static int wifi_tx_req(char *result) {
  sprintf(result, "%s%d", EUT_WIFI_TX_REQ, tx_state);
  return 0;
}
static int wifi_tx_factor_req(char *result) {
  sprintf(result, "%s%d", EUT_WIFI_TXFAC_REQ, tx_power_factor_p);
  return 0;
}
static int wifi_rx_req(char *result) {
  sprintf(result, "%s%d", EUT_WIFI_RX_REQ, rx_state);
  return 0;
}
static int wifi_ratio_req(char *result) {
  sprintf(result, "%s%f", EUT_WIFI_RATIO_REQ, ratio_p);
  return 0;
}
static int wifi_ch_req(char *result) {
  sprintf(result, "%s%d", EUT_WIFI_CH_REQ, channel_p);
  return 0;
}
int wifi_rxpackcount(char *result) {
  int error = system("wl counters > /data/data/wlancounters.txt");
  ALOGI("wifi_rxpackcount");
  if (error == -1 || error == 127) {
    ALOGE("=== wifi_rxpackcount on cmd : wl counter===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
  } else {
    ALOGI("=== wifi_rxpackcount! ===\n");
    strcpy(result, EUT_WIFI_OK);
    rxmfrmocast_next = parse_packcount("/data/data/wlancounters.txt");
    rx_packcount = rxmfrmocast_next - rxmfrmocast_priv;
    rxmfrmocast_priv = rxmfrmocast_next;
    system("rm /data/data/wlancounters.txt");
    sprintf(result, "%s:rx_end_count=%ld:end", EUT_WIFI_RXPACKCOUNT_REQ,
            rx_packcount);
    ALOGI("resutl=%s", result);
  }
  return 0;
}
int wifi_clr_rxpackcount(char *result) {
  rx_packcount = 0;
  int error = system("wl counters > /data/data/wlancounters.txt");
  ALOGI("wl counters");
  if (error == -1 || error == 127) {
    ALOGE("=== start_wifi_rx test failed on cmd : wl counters===\n");
    sprintf(result, "%s%d", EUT_WIFI_ERROR, error);
  } else {
    rxmfrmocast_priv = parse_packcount("/data/data/wlancounters.txt");
  }
  strcpy(result, EUT_WIFI_OK);
  return 0;
}
static long parse_packcount(char *filename) {
  int fd, n, len;
  char packcount[20];
  memset(counter_respon, 0, COUNTER_BUF_SIZE);
  fd = open(filename, O_RDWR | O_NONBLOCK);
  if (fd < 0) {
    ALOGE("=== open file  %s error===\n", filename);
  } else {
    n = read(fd, counter_respon, COUNTER_BUF_SIZE);
    close(fd);
  }
  char *p = strstr(counter_respon, "pktengrxdmcast");
  if (p != NULL) {
    char *q = strstr(p, " ");
    char *s = strstr(q, "txmpdu_sgi");
    ALOGE("=== open file parse_packcount entry s= %s\n", s);
    len = s - q - 1;
    memcpy(packcount, q + 1, len);
    // return 0 ;
    return atol(packcount);
  } else {
    ALOGE("=== open file  %s error===\n", filename);
    return 0;
  }
}

int set_wifi_rate(char *string, char *rsp) {
  ALOGI("%s()...%s\n", __FUNCTION__, string);
  if (0 == wifieut_state) {
    ALOGE("%s(), wifieut_state:%d", __FUNCTION__, wifieut_state);
    sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
    return -1;
  }

  s_rate_index = mattch_rate_table_str(string);
  strcpy(rsp, EUT_WIFI_OK);
  return 0;
}

int wifi_rate_req(char *rsp) {
  int ret = -1;
  char *str = NULL;
  ALOGI("%s()...\n", __FUNCTION__);
  if (0 == s_rate_index) {
    ALOGE("%s(), s_rate_index is 0", __FUNCTION__);
    goto err;
  }
  str = mattch_rate_table_index(s_rate_index);
  if (NULL == str) {
    ALOGE("%s(), don't mattch rate", __FUNCTION__);
    goto err;
  }
  sprintf(rsp, "%s%s", WIFI_RATE_REQ_RET, str);
  return 0;
err:
  sprintf(rsp, "%s%s", WIFI_RATE_REQ_RET, "null");
  return -1;
}

int set_wifi_txgainindex(int index, char *rsp) {
  if (0 == wifieut_state) {
    ALOGE("%s(), wifieut_state:%d", __FUNCTION__, wifieut_state);
    goto err;
  }
  ALOGI("%s(), index:%d\n", __FUNCTION__, index);
  return set_wifi_tx_factor_83780(index, rsp);

err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  return -1;
}

int wifi_txgainindex_req(char *rsp) {
  if (0 == wifieut_state) {
    ALOGE("%s(), wifieut_state:%d", __FUNCTION__, wifieut_state);
    goto err;
  }

  sprintf(rsp, "%s%d", WIFI_TXGAININDEX_REQ_RET, tx_power_factor);
  return 0;
err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  return -1;
}

int wifi_rssi_req(char *rsp) {
  char buf[100] = {0};
  FILE *fp;
  int rssi = -100;

  if (1 == tx_state) {
    ALOGE("wifi_rssi_req(),tx_state:%d", tx_state);
    goto err;
  }

  if ((fp = popen("wl rssi", "r")) == NULL) {
    ALOGE("=== wifi_rssi_req popen() fail ===\n");
    sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "popen");
    return -1;
  }
  while (fgets(buf, sizeof(buf), fp) != NULL) {
    if (buf[0] == '-' || (buf[0] >= '0' && buf[0] <= '9')) {
      rssi = atoi(buf);
      break;
    }
  }
  pclose(fp);

  if (-100 == rssi) {
    ALOGE("get_rssi cmd  err");
    goto err;
  }

  sprintf(rsp, "%s0x%x", EUT_WIFI_RSSI_REQ_RET, rssi);
err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  return -1;
}

int wifiband(int band, char *rsp) {
  ALOGI("set band %d", band);
  if (0 == wifieut_state) {
    ALOGE("%s(), wifieut_state:%d", __FUNCTION__, wifieut_state);
    sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
    return -1;
  }
  set_band = band;
  strcpy(rsp, EUT_WIFI_OK);
  return 0;
}
int wifiband_req(char *rsp) {
  ALOGI("band req");
  sprintf(rsp, "%s%d", EUT_WIFI_BAND_REQ, set_band);
  return 0;
}

int wifi_tx_mode_bcm(int tx_mode, char *rsp) {
  ALOGI("set tx mode %d", tx_mode);
  if (0 == wifieut_state) {
    ALOGE("%s(), wifieut_state:%d", __FUNCTION__, wifieut_state);
    sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
    return -1;
  }
  wifitxmod = tx_mode;
  strcpy(rsp, EUT_WIFI_OK);
  return 0;
}
int wifi_tx_mode_req(char *rsp) {
  ALOGI("tx_mode req");
  sprintf(rsp, "%s%d", EUT_WIFI_TX_MODE_REQ, wifitxmod);
  return 0;
}

int wifi_tx_pwrlv(int pwrlv, char *rsp) {
  ALOGI("set wifi_tx_pwrlv %d", pwrlv);
  if (0 == wifieut_state) {
    ALOGE("%s(), wifieut_state:%d", __FUNCTION__, wifieut_state);
    sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
    return -1;
  }

  //set_wifi_tx_factor((long)pwrlv, rsp);
  tx_power_lv = pwrlv;
  sinpwrlv = pwrlv;
  strcpy(rsp, EUT_WIFI_OK);

  return 0;
}
int wifi_tx_pwrlv_req(char *rsp) {
  ALOGI("wifi_tx_pwrlv req");
  if (wifitxmod == 0) {
    sprintf(rsp, "%s%d,%d", EUT_WIFI_TX_PWRLV_REQ, sinpwrlv, sinpwrlv);
  } else {
    sprintf(rsp, "%s%d,%d", EUT_WIFI_TX_PWRLV_REQ, tx_power_lv, tx_power_lv);
  }
  return 0;
}

int wifi_pkt_len_set(int pkt_len, char *rsp) {
  ALOGI(" %s(), pkt_len = %d", __func__, pkt_len);
  set_pktlen = pkt_len;
  strcpy(rsp, EUT_WIFI_OK);

  return 0;
}

int wifi_pkt_len_get(char *rsp) {
  int pkt_len = 1024;
  ALOGI("%s()", __func__);
  sprintf(rsp, "%s%d", EUT_WIFI_PKT_LEN_REQ, set_pktlen);
  return 0;
}
int wifi_bw_get(char *rsp) {
  ALOGI(" %s(), ", __func__);

  sprintf(rsp, "%s%d", EUT_WIFI_BW_REQ, set_bw);
  return 0;
}

int wifi_bw_set(wifi_bw band_width, char *rsp) {
  ALOGI(" %s(), band_width = %d", __func__, band_width);
  if (band_width > WIFI_BW_160MHZ) {
    ALOGI("%s(), band num is err, band_width = %d", __func__, band_width);
    return -1;
  }
  set_bw = band_width;
  switch (set_bw) {
    case WIFI_BW_20MHZ:
      s_bandwidth = 20;
      break;
    case WIFI_BW_40MHZ:
      s_bandwidth = 40;
      break;
    case WIFI_BW_80MHZ:
      s_bandwidth = 80;
      break;
    case WIFI_BW_160MHZ:
      s_bandwidth = 160;
      break;
    default:
      sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
      return -1;
  }
  strcpy(rsp, EUT_WIFI_OK);
  return 0;
}

int wifi_eut_preamble_set(wifi_eut_preamble_type preamble_type, char *rsp) {
  int ret = -1;

  ALOGI("ADL entry %s(), preamble_type = %d", __func__, preamble_type);

  if (preamble_type > WIFI_PREAMBLE_TYPE_MAX_VALUE) {
    ALOGI("%s(), preamble's value is invalid, preamble_type = %d", __func__,
          preamble_type);
    goto err;
  }
  s_preamble_type = preamble_type;
  strcpy(rsp, EUT_WIFI_OK);
  ALOGI("ADL leaving %s(), rsp = %s, return 0", __func__, rsp);
  return 0;

err:
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  ALOGI("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
  return -1;
}

int wifi_tone_set(int tone, char *rsp) {
  ALOGI(" %s(), tone = %d", __func__, tone);
  tx_tone = tone;
  strcpy(rsp, EUT_WIFI_OK);

  return 0;
}

int wifi_sync_rate_bw() {
  ALOGI(" %s(), preamble:%d;bandwidth: %d;channel:%d;band:%d;rate_index:%d",
        __func__, s_preamble_type, s_bandwidth, channel_p, set_band,
        s_rate_index);
  switch (s_preamble_type) {
    case WIFI_PREAMBLE_TYPE_NORMAL:
    case WIFI_PREAMBLE_TYPE_CCK_SHORT:
      ALOGI("%s() set 11b,11g mode", __func__);
      if (((channel_p > 0) && (channel_p <= 14)) && (s_rate_index <= 12) &&
          (s_bandwidth == 20))
        sprintf(cmd_set_rate_bw, "wl 2g_rate -r %s -b 20",
                mattch_rate_table_rate(s_rate_index));
      else
        ALOGI("%s() invalid channel or band or rate in 11b,11g mode", __func__);
      break;
    case WIFI_PREAMBLE_TYPE_80211_MIX_MODE:
    case WIFI_PREAMBLE_TYPE_80211N_GREEN_FIELD:
      ALOGI("%s() set 11n mode", __func__);
      if ((set_band == 0) || ((channel_p > 0) && (channel_p <= 14)))
        sprintf(cmd_set_rate_bw, "wl 2g_rate -h %s -b %d",
                mattch_rate_table_rate(s_rate_index), s_bandwidth);
      else if ((set_band == 1) || ((channel_p > 14)))
        sprintf(cmd_set_rate_bw, "wl 5g_rate -h %s -b %d",
                mattch_rate_table_rate(s_rate_index), s_bandwidth);
      else
        ALOGI("%s() invalid channel or band or rate in 11n mode", __func__);
      break;
    case WIFI_PREAMBLE_TYPE_80211AC_FIELD:
      ALOGI("%s() set 11ac mode", __func__);
      sprintf(cmd_set_rate_bw, "wl 5g_rate -v %s -b %d",
              mattch_rate_table_rate(s_rate_index), s_bandwidth);
      break;
    default:
      ALOGI("%s() sync rate bw band failed !!!!!!", __func__);
      return -1;
  }

  return 0;
}
