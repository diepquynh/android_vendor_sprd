#ifndef _ENG_OPT_H
#define _ENG_OPT_H

#include <stdio.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif
#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "ENGPC"
#include <utils/Log.h>
#define ENG_AT_LOG ALOGD

#define ENG_TRACING

#ifdef ENG_TRACING
#define ENG_LOG ALOGD
#else
//#define ENG_LOG  ALOGD
#define ENG_LOG(format, ...)
#endif

#define ENG_AT_CHANNEL

#if defined(ENGMODE_EUT_BCM)
// brcm的wlan方案

#define ENG_EUT "EUT"
#define ENG_EUT_REQ "EUT?"  // brcm
#define ENG_WIFI_BAND_REQ "BAND?"
#define ENG_WIFI_BAND "BAND"
#define ENG_WIFICH_REQ "CH?"
#define ENG_WIFICH "CH"
#define ENG_WIFITX_MODE_REQ "TXMODE?"
#define ENG_WIFITX_MODE "TXMODE"
#define ENG_WIFIMODE "MODE"  // brcm
#define ENG_WIFIRATIO_REQ "RATIO?"
#define ENG_WIFIRATIO "RATIO"
#define ENG_WIFITX_FACTOR_REQ "TXFAC?"
#define ENG_WIFITX_FACTOR "TXFAC"
#define ENG_WIFITX_PWRLV_REQ "TXPWRLV?"
#define ENG_WIFITX_PWRLV "TXPWRLV"
#define ENG_WIFITX_REQ "TX?"
#define ENG_WIFITX "TX"
#define ENG_WIFIRX_REQ "RX?"
#define ENG_WIFIRX "RX"
#define ENG_WIFI_CLRRXPACKCOUNT "CLRRXPACKCOUNT"
#define ENG_GPSSEARCH_REQ "SEARCH?"
#define ENG_GPSSEARCH "SEARCH"
#define ENG_GPSPRNSTATE_REQ "PRNSTATE?"
#define ENG_GPSSNR_REQ "SNR?"
#define ENG_GPSPRN "PRN"
//------------------------------------------------

#define ENG_WIFIRATE "RATE"
#define ENG_WIFIRATE_REQ "RATE?"
#define ENG_WIFITXGAININDEX "TXGAININDEX"
#define ENG_WIFITXGAININDEX_REQ "TXGAININDEX?"
#define ENG_WIFIRX_PACKCOUNT "RXPACKCOUNT?"
#define ENG_WIFIRSSI_REQ "RSSI?"

//------------------------------------------------
#define ENG_WIFIPKTLEN_REQ ("PKTLEN?")
#define ENG_WIFIPKTLEN ("PKTLEN")
#define ENG_WIFIPAYLOAD_REQ ("PAYLOAD?")
#define ENG_WIFIPAYLOAD ("PAYLOAD")
#define ENG_WIFIBANDWIDTH_REQ ("BW?")
#define ENG_WIFIBANDWIDTH ("BW")
#define ENG_WIFILNA_REQ ("LNA?")  // brcm power save
#define ENG_WIFILNA ("LNA")
#define ENG_WIFIPREAMBLE ("PREAMBLE")
//----------brcm BT------------------------------
#define ENG_BT_TXCH_REQ ("TXCH?")
#define ENG_BT_TXCH ("TXCH")

#define ENG_BT_RXCH_REQ ("RXCH?")
#define ENG_BT_RXCH ("RXCH")

#define ENG_BT_TXPATTERN_REQ ("TXPATTERN?")
#define ENG_BT_TXPATTERN ("TXPATTERN")

#define ENG_BT_RXPATTERN_REQ ("RXPATTERN?")
#define ENG_BT_RXPATTERN ("RXPATTERN")

#define ENG_BT_TXPKTTYPE_REQ ("TXPKTTYPE?")
#define ENG_BT_TXPKTTYPE ("TXPKTTYPE")

#define ENG_BT_RXPKTTYPE_REQ ("RXPKTTYPE?")
#define ENG_BT_RXPKTTYPE ("RXPKTTYPE")

#define ENG_BT_TXPKTLEN_REQ ("TXPKTLEN?")
#define ENG_BT_TXPKTLEN ("TXPKTLEN")

#define ENG_BT_TXPWR_REQ ("TXPWR?")
#define ENG_BT_TXPWR ("TXPWR")

#define ENG_BT_RXGAIN_REQ ("RXGAIN?")
#define ENG_BT_RXGAIN ("RXGAIN")

#define ENG_BT_ADDRESS_REQ ("TESTADDRESS?")
#define ENG_BT_ADDRESS ("TESTADDRESS")

#define ENG_BT_RXDATA_REQ ("RXDATA?")

#define ENG_BT_TESTMODE_REQ ("TESTMODE?")
#define ENG_BT_TESTMODE ("TESTMODE")

#define ENG_TX_REQ ("TX?")
#define ENG_TX ("TX")
#define ENG_RX_REQ ("RX?")
#define ENG_RX ("RX")

typedef enum {
  EUT_REQ_INDEX = 0,  // brcm
  EUT_INDEX,
  /* for BRCM Bluetooth */
  /* BT TX CHANNEL */
  BT_TXCH_REQ_INDEX,
  BT_TXCH_INDEX,

  /* BT RX CHANNEL */
  BT_RXCH_REQ_INDEX,
  BT_RXCH_INDEX,

  /* BT TX PATTERN */
  BT_TXPATTERN_REQ_INDEX,  // 6
  BT_TXPATTERN_INDEX,

  /* BT RX PATTERN */
  BT_RXPATTERN_REQ_INDEX,
  BT_RXPATTERN_INDEX,

  /* TXPKTTYPE */
  BT_TXPKTTYPE_REQ_INDEX,  // 10
  BT_TXPKTTYPE_INDEX,

  /* RXPKTTYPE */
  BT_RXPKTTYPE_REQ_INDEX,
  BT_RXPKTTYPE_INDEX,

  /* TXPKTLEN */
  BT_TXPKTLEN_REQ_INDEX,  // 14
  BT_TXPKTLEN_INDEX,

  /* TXPWRLV */
  WIFITX_PWRLV_REQ_INDEX,
  WIFITX_PWRLV_INDEX,

  /* TXPWR */
  BT_TXPWR_REQ_INDEX,  // 16
  BT_TXPWR_INDEX,  // 17

  /* RX Gain */
  BT_RXGAIN_REQ_INDEX,
  BT_RXGAIN_INDEX,

  /* ADDRESS */
  BT_ADDRESS_REQ_INDEX,
  BT_ADDRESS_INDEX,

  /* RXDATA */
  BT_RXDATA_REQ_INDEX,

  /* TESTMODE */
  BT_TESTMODE_REQ_INDEX,  // 23
  BT_TESTMODE_INDEX,  // 24

  TX_REQ_INDEX,  // 25
  TX_INDEX,  // 26
  RX_REQ_INDEX,  // 27
  RX_INDEX,  // 28

  WIFI_BAND_REQ_INDEX,
  WIFI_BAND_INDEX,
  WIFICH_REQ_INDEX,
  WIFICH_INDEX,
  WIFITX_MODE_REQ_INDEX,
  WIFITX_MODE_INDEX,
  WIFIMODE_INDEX,  // brcm 8
  WIFIRATIO_REQ_INDEX,
  WIFIRATIO_INDEX,
  WIFITX_FACTOR_REQ_INDEX,
  WIFITX_FACTOR_INDEX,
  WIFITX_REQ_INDEX,
  WIFITX_INDEX,
  WIFIRX_REQ_INDEX,
  WIFIRX_INDEX,
  WIFIRX_PACKCOUNT_INDEX,
  WIFICLRRXPACKCOUNT_INDEX,
  GPSSEARCH_REQ_INDEX,
  GPSSEARCH_INDEX,
  GPSPRNSTATE_REQ_INDEX,
  GPSSNR_REQ_INDEX,
  GPSPRN_INDEX,
  //------------------------------------------------
  ENG_WIFIRATE_INDEX,
  ENG_WIFIRATE_REQ_INDEX,
  ENG_WIFITXGAININDEX_INDEX,
  ENG_WIFITXGAININDEX_REQ_INDEX,
  ENG_WIFIRSSI_REQ_INDEX,
  //------------------------------------------------
  /* Pkt Length */
  WIFIPKTLEN_REQ_INDEX,
  WIFIPKTLEN_INDEX,
  /* Band Width */
  WIFIBANDWIDTH_REQ_INDEX,  // 30
  WIFIBANDWIDTH_INDEX,
  /* LNA */
  WIFILNA_REQ_INDEX,
  WIFILNA_INDEX,

  WIFIPREAMBLE_INDEX,

  WIFITXMODE_REQ_INDEX,
  WIFITXMODE_INDEX,

} eut_cmd_enum;

#elif defined(ENGMODE_EUT_SPRD)
// sprd的wlan方案
#define ENG_EUT ("EUT")
#define ENG_EUT_REQ ("EUT?")
#define ENG_WIFICH_REQ ("CH?")
#define ENG_WIFICH ("CH")

/* here is WIFI cmd */
#define ENG_WIFIMODE "MODE"
#define ENG_WIFIRATIO_REQ "RATIO?"
#define ENG_WIFIRATIO "RATIO"
#define ENG_WIFITX_FACTOR_REQ "TXFAC?"
#define ENG_WIFITX_FACTOR "TXFAC"

#define ENG_TX_REQ ("TX?")
#define ENG_TX ("TX")
#define ENG_RX_REQ ("RX?")
#define ENG_RX ("RX")
#define ENG_WIFI_CLRRXPACKCOUNT "CLRRXPACKCOUNT"
#define ENG_GPSSEARCH_REQ "SEARCH?"
#define ENG_GPSSEARCH "SEARCH"
#define ENG_GPSPRNSTATE_REQ "PRNSTATE?"
#define ENG_GPSSNR_REQ "SNR?"
#define ENG_GPSPRN "PRN"
//------------------------------------------------

#define ENG_WIFIRATE "RATE"
#define ENG_WIFIRATE_REQ "RATE?"
#define ENG_WIFITXGAININDEX "TXGAININDEX"
#define ENG_WIFITXGAININDEX_REQ "TXGAININDEX?"
#define ENG_WIFIRX_PACKCOUNT "RXPACKCOUNT?"
#define ENG_WIFIRSSI_REQ "RSSI?"
#define ENG_WIFILNA_REQ ("LNA?")
#define ENG_WIFILNA ("LNA")

#define ENG_WIFIBAND_REQ ("BAND?")
#define ENG_WIFIBAND ("BAND")

#define ENG_WIFIBANDWIDTH_REQ ("BW?")
#define ENG_WIFIBANDWIDTH ("BW")

#define ENG_WIFITXPWRLV_REQ ("TXPWRLV?")
#define ENG_WIFITXPWRLV ("TXPWRLV")

#define ENG_WIFIPKTLEN_REQ ("PKTLEN?")
#define ENG_WIFIPKTLEN ("PKTLEN")

#define ENG_WIFITXMODE_REQ ("TXMODE?")
#define ENG_WIFITXMODE ("TXMODE")

#define ENG_WIFIPREAMBLE_REQ ("PREAMBLE?")
#define ENG_WIFIPREAMBLE ("PREAMBLE")

#define ENG_WIFIPAYLOAD_REQ ("PAYLOAD?")
#define ENG_WIFIPAYLOAD ("PAYLOAD")

#define ENG_GUARDINTERVAL_REQ ("GUARDINTERVAL?")
#define ENG_GUARDINTERVAL ("GUARDINTERVAL")

#define ENG_MACFILTER_REQ ("MACFILTER?")
#define ENG_MACFILTER ("MACFILTER")

//------------------------------------------------

typedef enum {
  EUT_REQ_INDEX = 0,  // sprd
  EUT_INDEX,
  WIFICH_REQ_INDEX,
  WIFICH_INDEX,
  WIFIMODE_REQ_INDEX,
  WIFIMODE_INDEX,  // sprd
  WIFIRATIO_REQ_INDEX,
  WIFIRATIO_INDEX,
  WIFITX_FACTOR_REQ_INDEX,
  WIFITX_FACTOR_INDEX,
  TX_REQ_INDEX,  // 10
  TX_INDEX,
  RX_REQ_INDEX,
  RX_INDEX,
  WIFIRX_PACKCOUNT_INDEX,
  WIFICLRRXPACKCOUNT_INDEX,
  GPSSEARCH_REQ_INDEX,
  GPSSEARCH_INDEX,
  GPSPRNSTATE_REQ_INDEX,
  GPSSNR_REQ_INDEX,
  GPSPRN_INDEX,  // 20
                 //------------------------------------------------
  ENG_WIFIRATE_INDEX,
  ENG_WIFIRATE_REQ_INDEX,
  ENG_WIFITXGAININDEX_INDEX,
  ENG_WIFITXGAININDEX_REQ_INDEX,
  ENG_WIFIRSSI_REQ_INDEX,
  //------------------------------------------------
  /* LNA */
  WIFILNA_REQ_INDEX,
  WIFILNA_INDEX,

  /* Band */
  WIFIBAND_REQ_INDEX,
  WIFIBAND_INDEX,

  /* Band Width */
  WIFIBANDWIDTH_REQ_INDEX,  // 30
  WIFIBANDWIDTH_INDEX,

  /* Tx Power Level */
  WIFITXPWRLV_REQ_INDEX,
  WIFITXPWRLV_INDEX,

  /* Pkt Length */
  WIFIPKTLEN_REQ_INDEX,
  WIFIPKTLEN_INDEX,

  /* TX Mode */
  WIFITXMODE_REQ_INDEX,
  WIFITXMODE_INDEX,

  /* Preamble */
  WIFIPREAMBLE_REQ_INDEX,
  WIFIPREAMBLE_INDEX,

  /* Payload */
  WIFIPAYLOAD_REQ_INDEX,  // 40
  WIFIPAYLOAD_INDEX,

  /* Guard Interval */
  WIFIGUARDINTERVAL_REQ_INDEX,
  WIFIGUARDINTERVAL_INDEX,

  /* MAC Filter */
  WIFIMACFILTER_REQ_INDEX,
  WIFIMACFILTER_INDEX,

} eut_cmd_enum;
#endif

typedef enum {
  BT_MODULE_INDEX = 0,
  WIFI_MODULE_INDEX,
  GPS_MODULE_INDEX,
  BLE_MODULE_INDEX
} eut_modules;

struct eut_cmd {
  int index;
  char* name;
};

typedef enum {
  CONNECT_UART = 0,
  CONNECT_USB,
  CONNECT_PIPE,
} eng_connect_type;

typedef enum {
  ENG_DIAG_RECV_TO_AP,
  ENG_DIAG_RECV_TO_CP,
} eng_diag_state;

typedef enum {
  ENG_LOG_NO_WAIT,
  ENG_LOG_WAIT_END,
} eng_log_state;

struct eng_param {
  int califlag;
  int engtest;
  char cp_type[32]; /*td: CP_TD; wcdma:CP_WCDMA*/
  int connect_type; /*usb:CONNECT_USB ; uart:CONNECT_UART*/
  int nativeflag;   /*0: vlx, CP directly communicates with PC tool
                     *1: native, AP directly communicates with PC tool  */
  int normal_cali;
};

typedef pthread_t eng_thread_t;

typedef void* (*eng_thread_func_t)(void* arg);

int eng_thread_create(eng_thread_t* pthread, eng_thread_func_t start,
                      void* arg);

#ifdef __cplusplus
}
#endif

#endif
