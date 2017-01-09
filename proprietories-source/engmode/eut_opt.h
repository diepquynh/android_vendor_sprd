#include <stdio.h>
#include <stdlib.h>
#include <utils/Log.h>

#ifndef LOG_TAG
#define LOG_TAG "SPRDENG"
#endif

#define EUT_BT_OK ("+SPBTTEST:OK")
#define EUT_BT_ERROR ("+SPBTTEST:ERR=")
#define EUT_BT_REQ ("+SPBTTEST:EUT=")
// BLE
#define EUT_BLE_OK ("+SPBLETEST:OK")
#define EUT_BLE_ERROR ("+SPBLETEST:ERR=")
#define EUT_BLE_REQ ("+SPBLETEST:EUT=")

#define EUT_WIFI_OK ("+SPWIFITEST:OK")
#define EUT_WIFI_ERROR ("+SPWIFITEST:ERR=")
#define EUT_WIFI_REQ ("+SPWIFITEST:EUT=")
#define EUT_WIFI_TX_REQ ("+SPWIFITEST:TX=")
#define EUT_WIFI_RX_REQ ("+SPWIFITEST:RX=")
#define EUT_WIFI_CH_REQ ("+SPWIFITEST:CH=")
#define EUT_WIFI_RATIO_REQ ("+SPWIFITEST:RATIO=")
#define EUT_WIFI_TXFAC_REQ ("+SPWIFITEST:TXFAC=")
#define EUT_WIFI_RXPACKCOUNT_REQ ("+SPWIFITEST:RXPACKCOUNT=")
#define EUT_WIFI_BAND_REQ		("+SPWIFITEST:BAND=")
#define EUT_WIFI_TX_MODE_REQ	("+SPWIFITEST:TXMODE=")
#define EUT_WIFI_TX_PWRLV_REQ	("+SPWIFITEST: TXPWRLV =")
#define EUT_WIFI_PKT_LEN_REQ ("+SPWIFITEST:PKTLEN=")
#define EUT_WIFI_BW_REQ ("+SPWIFITEST:BW=")

#define EUT_GPS_OK "+SPGPSTEST:OK"
#define EUT_GPS_ERROR "+SPGPSTEST:ERR="
#define EUT_GPS_REQ "+SPGPSTEST:EUT="
#define EUT_GPS_PRN_REQ "+SPGPSTEST:PRN="
#define EUT_GPS_SNR_REQ "+SPGPSTEST:SNR="
#define EUT_GPS_SEARCH_REQ "+SPGPSTEST:SEARCH="
#define EUT_GPS_SNR_NO_EXIST "NO_EXIST"
#define EUT_GPS_NO_FOUND_STAELITE "NO_FOUND_SATELLITE"
#define EUT_GPS_SV_ID "SV_ID="
#define EUT_GPS_SV_NUMS "SV_NUM="

#define EUT_WIFIERR_RATIO (150)
#define EUT_WIFIERR_TXFAC (151)
#define EUT_WIFIERR_TXFAC_SETRATIOFIRST (152)
#define EUT_GPSERR_SEARCH (153)
#define EUT_GPSERR_PRNSTATE (154)
#define EUT_GPSERR_PRNSEARCH (155)
//------------------------------------------------
#define EUT_BT_OK ("+SPBTTEST:OK")
#define EUT_BT_ERROR ("+SPBTTEST:ERR=")
#define EUT_BT_REQ ("+SPBTTEST:EUT=")

#define EUT_BLE_OK ("+SPBLETEST:OK")
#define EUT_BLE_ERROR ("+SPBLETEST:ERR=")
#define EUT_BLE_REQ ("+SPBLETEST:EUT=")

typedef enum {
  WIFI_MODE_11B = 1,
  WIFI_MODE_11G,
  WIFI_MODE_11N,
} WIFI_WORKE_MODE;

/* band width */
typedef enum {
  WIFI_BW_20MHZ = 0,
  WIFI_BW_40MHZ,
  WIFI_BW_80MHZ,
  WIFI_BW_160MHZ,
  WIFI_BW_INVALID = 0xff
} wifi_bw;

/* preamble type */
typedef enum {
  WIFI_PREAMBLE_TYPE_NORMAL = 0,         /* normal */
  WIFI_PREAMBLE_TYPE_CCK_SHORT,          /* cck short */
  WIFI_PREAMBLE_TYPE_80211_MIX_MODE,     /* 802.11 mixed mode */
  WIFI_PREAMBLE_TYPE_80211N_GREEN_FIELD, /* 802.11n green field */
  WIFI_PREAMBLE_TYPE_80211AC_FIELD,
  WIFI_PREAMBLE_TYPE_MAX_VALUE = 0xff
} wifi_eut_preamble_type;

struct eng_bt_eutops {
  int (*bteut)(int, char *);
  int (*bteut_req)(char *);
};

struct eng_wifi_eutops{
    int (*wifieut)(int,char *);
    int	(*wifieut_req)(char *);
    int (*wifiband)(int,char *);
    int	(*wifiband_req)(char *);
    int (*wifi_tx_mode_bcm)(int,char *);
    int	(*wifi_tx_mode_req)(char *);
    int (*wifi_tx_pwrlv)(int,char *);
    int	(*wifi_tx_pwrlv_req)(char *);
    int (*wifi_tx)(int,char *);
    int (*set_wifi_tx_factor)(long ,char *);
    int (*wifi_rx)(int,char *);
    int (*set_wifi_mode)(char *,char *);
    int (*set_wifi_ratio)(float,char *);
    int (*set_wifi_ch)(int,char *);
    int (*wifi_tx_req)(char *);
    int (*wifi_tx_factor_req)(char *);
    int (*wifi_rx_req)(char *);
    int (*wifi_ratio_req)(char *);
    int (*wifi_ch_req)(char *);
    int (*wifi_rxpackcount)(char *);
    int (*wifi_clr_rxpackcount)(char *);
    int (*set_wifi_rate)(char *, char *);
    int (*wifi_rate_req)(char *);
    int (*set_wifi_txgainindex)(int , char *);
    int (*wifi_txgainindex_req)(char *);
    int (*wifi_rssi_req)(char *);
    int(*wifi_pkt_len_set)(int, char *);
    int(*wifi_pkt_len_get)(char *);
    int(*wifi_bw_get)(char *);
    int(*wifi_bw_set)(wifi_bw, char *);
    int (*wifi_pm)(int ,char *);
    int(*wifi_eut_preamble_set)(wifi_eut_preamble_type, char *);
};

struct eng_gps_eutops {
  int (*gpseut)(int, char *);
  int (*gpseut_req)(char *);
  int (*gps_search)(int, char *);
  int (*gps_search_req)(char *);
  int (*gps_setprn)(int, char *);
  int (*gps_prnstate_req)(char *);
  int (*gps_snr_req)(char *);
};
int bteut(int command_code, char *rsp);
static int bteut_req(char *);
static int start_bteut(char *);
static int end_bteut(char *);
static int wifieut_req(char *);
int wifieut(int command_code,char *rsp);
int wifiband(int band, char *rsp);
int wifiband_req(char *rsp);
int wifi_tx_mode_bcm(int tx_mode, char *rsp);
int wifi_tx_mode_req(char *rsp);
int wifi_tx_pwrlv(int pwrlv, char *rsp);
int wifi_tx_pwrlv_req(char *rsp);
int wifi_tx(int command_code,char *rsp);
int wifi_rx(int command_code,char *rsp);
int set_wifi_tx_factor_83780(int factor,char *result);
int set_wifi_ratio(float ratio,char *result);
int set_wifi_ch(int ch,char *result);
int wifi_rxpackcount(char *result);
int wifi_clr_rxpackcount(char *result);
int set_wifi_rate(char *string, char *rsp);
int wifi_rate_req(char *rsp);
int set_wifi_txgainindex(int index, char *rsp);
int wifi_txgainindex_req(char *rsp);
int wifi_rssi_req(char *rsp);
int wifi_pm(int command_code,char *result);
int wifi_eut_preamble_set(wifi_eut_preamble_type preamble_type, char *rsp);
int wifi_pkt_len_set(int pkt_len, char *rsp);
int wifi_pkt_len_get(char *rsp);
int wifi_bw_get(char *rsp);
int wifi_bw_set(wifi_bw band_width, char *rsp);
static int start_bteut(char *result);
static int start_wifieut(char *result);
static int end_wifieut(char *result);
static int start_wifi_tx(char *result);
static int end_wifi_tx(char *result);
int  set_wifi_tx_factor(long factor,char *result);
static int start_wifi_rx(char *result);
static int end_wifi_rx(char *result);
static int set_wifi_modu(char *modu, char *result);
static int set_wifi_mode(char *mode, char *result);
static int wifi_tx_req(char *result);
static int wifi_tx_factor_req(char *result);
static int wifi_rx_req(char *result);
static int wifi_ratio_req(char *result);
static int wifi_ch_req(char *result);
static int gps_eut(int command_code, char *rsp);
static int start_gpseut(char *result);
static int end_gpseut(char *result);
static int gps_search(int command_code, char *result);
static int start_gpssearch(char *result);
static int end_gpssearch(char *result);
static int gpseut_req(char *result);
static int set_gps_prn(int prn, char *result);
static int gps_search_req(char *result);
static int gps_prnstate_req(char *result);
static int gps_snr_req(char *result);
static int reflect_factor(int start, int end, int factor);
