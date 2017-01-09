
#ifndef __WIFI_EUT_SHARK_H__
#define __WIFI_EUT_SHARK_H__


#include "stdio.h"
#include "stdlib.h"

//----------------------------------------------------------------
#define TMP_BUF_SIZE                        (128)

#define STR_RET_STATUS                      ("ret: status ")
#define STR_RET_REG_VALUE                   ("ret: reg value:")
#define STR_RET_MAC_VALUE                   ("ret: mac: ")
#define STR_RET_RET                         ("ret: ")
#define STR_RET_END                         (":end")
#define TMP_FILE                            ("/productinfo/wifi_npi_data.log")
#define WIFI_RATE_REQ_RET                   ("+SPWIFITEST:RATE=")
#define WIFI_CHANNEL_REQ_RET                ("+SPWIFITEST:CH=")
#define WIFI_TXGAININDEX_REQ_RET            ("+SPWIFITEST:TXGAININDEX=")
#define EUT_WIFI_RXPKTCNT_REQ_RET           ("+SPWIFITEST:RXPACKCOUNT=")
#define EUT_WIFI_RSSI_REQ_RET               ("+SPWIFITEST:RSSI=")
#define WIFI_LNA_STATUS_REQ_RET             ("+SPWIFITEST:LNA=")
#define WIFI_BAND_REQ_RET                   ("+SPWIFITEST:BAND=")
#define WIFI_BANDWIDTH_REQ_RET              ("+SPWIFITEST:BW=")
#define WIFI_TX_POWER_LEVEL_REQ_RET         ("+SPWIFITEST:TXPWRLV=")
#define WIFI_PKT_LENGTH_REQ_RET             ("+SPWIFITEST:PKTLEN=")
#define WIFI_TX_MODE_REQ_RET                ("+SPWIFITEST:TXMODE=")
#define WIFI_PREAMBLE_REQ_RET               ("+SPWIFITEST:PREAMBLE=")
#define WIFI_PAYLOAD_REQ_RET                ("+SPWIFITEST:PAYLOAD=")
#define WIFI_GUARDINTERVAL_REQ_RET          ("+SPWIFITEST:GUARDINTERVAL=")
#define WIFI_MAC_FILTER_REQ_RET             ("+SPWIFITEST:MACFILTER=")
#define WIFI_INT_INVALID_RET                (-100)

#define WIFI_EUT_COMMAND_MAX_LEN            (128)
#define WIFI_EUT_COMMAND_RSP_MAX_LEN        (128)
#define WIFI_EUT_DEFAULT_CHANNEL            (1)

/* TX Power Level */
#define WIFI_TX_POWER_MIN_VALUE             (0)
#define WIFI_TX_POWER_MAX_VALUE             (2127)

/* PKT Length */
#define WIFI_PKT_LENGTH_MIN_VALUE           (0)
#define WIFI_PKT_LENGTH_MAX_VALUE           (65535)

#define WIFI_MAC_STR_MAX_LEN                (12+5)

//----------------------------------------------------------------

typedef struct
{
    unsigned int rx_end_count;
    unsigned int rx_err_end_count;
    unsigned int fcs_fail_count;
}RX_PKTCNT;

typedef struct
{
    int eut_enter;
    int rate;
    int channel;
    int tx_start;
    int rx_start;
    int sin_wave_start;
    int txgainindex;
    RX_PKTCNT cnt;
} WIFI_ELEMENT;

typedef struct
{
    int rate;
    char *name;
} WIFI_RATE;


/* TX Mode */
typedef enum
{
    WIFI_TX_MODE_CW = 0,
    WIFI_TX_MODE_DUTY_CYCLE,            /* 100% duty cycle */
    WIFI_TX_MODE_CARRIER_SUPPRESSION,   /* carrier suppression */
    WIFI_TX_MODE_LOCAL_LEAKAGE,         /* local leakage */
    WIFI_TX_MODE_INVALID = 0xff
} wifi_tx_mode;

/* Band */
typedef enum
{
    WIFI_BAND_2_4G = 0,
    WIFI_BAND_5G,
    WIFI_BAND_MAX_VALUE = 0xff
} wifi_band;

/* band width */
typedef enum
{
    WIFI_BANDWIDTH_20MHZ = 0,
    WIFI_BANDWIDTH_40MHZ,
    WIFI_BANDWIDTH_80MHZ,
    WIFI_BANDWIDTH_160MHZ,
    WIFI_BANDWIDTH_INVALID = 0xff
} wifi_bandwidth;

/* preamble type */
typedef enum
{
    PREAMBLE_TYPE_NORMAL = 0,           /* normal */
    PREAMBLE_TYPE_CCK_SHORT,            /* cck short */
    PREAMBLE_TYPE_80211_MIX_MODE,       /* 802.11 mixed mode */
    PREAMBLE_TYPE_80211N_GREEN_FIELD,   /* 802.11n green field */
    PREAMBLE_TYPE_MAX_VALUE = 0xff
} wifi_preamble_type;

/* payload */
typedef enum
{
    PAYLOAD_1111 = 0,                   /* 1111 */
    PAYLOAD_0000,                       /* 0000 */
    PAYLOAD_1010,                       /* 1010 */
    PAYLOAD_RANDOM,                     /* random */
    PAYLOAD_MAX_VALUE = 0xff
} wifi_payload;

/* guard interval */
typedef enum
{
    GUARD_INTERVAL_400NS = 0,           /* 400ns */
    GUARD_INTERVAL_800NS,               /* 800ns */
    GUARD_INTERVAL_MAX_VALUE = 0xff
} wifi_guard_interval;

/*******************Function Declaration************************/
/*-----------------static function-----------------------------*/
static int wifi_eut_init(void);
static int get_iwnpi_int_ret(void);
static int get_iwnpi_string_ret(const char *start, const char *end, char *outstr, unsigned char out_len);

/*-----------------extern function-----------------------------*/
int wifi_eut_set(int cmd, char *rsp);
int wifi_eut_get(char *rsp);

int wifi_rate_set(char *string, char *rsp);
int wifi_rate_get(char *rsp);

int wifi_channel_set(int ch,  char *rsp);
int wifi_channel_get(char *rsp);

int wifi_txgainindex_set(int index, char *rsp);
int wifi_txgainindex_get(char *rsp);

int wifi_tx_set(int command_code, int mode, int pktcnt, char *rsp);

int wifi_tx_get(char *rsp);

int wifi_rx_set(int command_code, char *rsp);
int wifi_rx_get(char *rsp);

int wifi_rxpktcnt_get(char *rsp);

int wifi_rssi_get(char *rsp);

int wifi_lna_set(int lna_on_off, char *rsp);
int wifi_lna_get(char *rsp);

int wifi_band_set(wifi_band band, char *rsp);
int wifi_band_get(char *rsp);

int wifi_bandwidth_set(wifi_bandwidth band_width, char *rsp);
int wifi_bandwidth_get(char *rsp);

int wifi_tx_power_level_set(int tx_power, char *rsp);
int wifi_tx_power_level_get(char *rsp);

int wifi_pkt_length_set(int pkt_len, char *rsp);
int wifi_pkt_length_get(char *rsp);

int wifi_tx_mode_set(wifi_tx_mode tx_mode, char *rsp);
int wifi_tx_mode_get(char *rsp);

int wifi_preamble_set(wifi_preamble_type preamble_type, char *rsp);
int wifi_preamble_get(char *rsp);

int wifi_payload_set(wifi_payload payload, char *rsp);
int wifi_payload_get(char *rsp);

int wifi_guardinterval_set(wifi_guard_interval gi, char *rsp);
int wifi_guardinterval_get(char *rsp);

int wifi_mac_filter_set(int on_off, const char *mac, char *rsp);
int wifi_mac_filter_get(char *rsp);

/***************************************************************/


#endif /*__ENG_AT_H__*/

