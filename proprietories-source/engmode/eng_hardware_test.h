#ifndef _ENG_HARDWARE_TEST_H
#define _ENG_HARDWARE_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

#include	"eut_opt.h"

#define SOCKET_BUF_LEN	1024
#define OPEN_WIFI   1
#define CLOSE_WIFI  0
#define OPEN_BT   1
#define CLOSE_BT  0

#define TEST_OK	"OK"
#define TEST_ERROR	"Fail"

#define OK_LEN  strlen(TEST_OK)
#define ERROR_LEN   strlen(TEST_ERROR)

#define TYPE_OFFSET 7
#define CMD_OFFSET 7

#define SPRD_WIFI	1
#define SPRD_BT		2
#define CLOSE_SOCKET    3



#define WIFI_EUT_UP  "iwnpi wlan0 ifaceup"
#define WIFI_EUT_DOWN  "iwnpi wlan0 ifacedown"

#define WIFI_EUT_START  "START"
#define WIFI_EUT_STOP  "STOP"
#define WIFI_EUT_SET_CHANNEL  "SET_CHANNEL"
#define WIFI_EUT_SET_RATE  "SET_RATE"
#define WIFI_EUT_SET_BAND "SET_BAND"
#define WIFI_EUT_BANDWIDTH  "SET_BW"
#define WIFI_EUT_SET_PREAMBLE  "SET_PREAMBLE"
#define WIFI_EUT_SET_MODE "SET_MODE"
#define WIFI_EUT_SET_TXTONE "SET_TXTONE"
    
    //for tx
#define WIFI_EUT_TX_START  "TX_START"
#define WIFI_EUT_TX_STOP  "TX_STOP"
#define WIFI_EUT_CW_START  "SIN_WAVE"
#define WIFI_EUT_SET_POWER  "TX_POWER"
#define WIFI_EUT_SET_LENGTH  "no support"
#define WIFI_EUT_SET_COUNT  "no support"

#define WIFI_EUT_GUARDINTERVAL  "no support"
    
    //for rx
#define WIFI_EUT_RX_START  "RX_START"
#define WIFI_EUT_RX_STOP "RX_STOP"
#define WIFI_EUT_GET_RXOK  "GET_RXOK"
    
    //for reg_wr
#define WIFI_EUT_READ "iwnpi wlan0 get_reg "
#define WIFI_EUT_WRITE  "iwnpi wlan0 set_reg "
    
#define CMD_POWER_SAVE  "POWERSAVE_MODE"
#define CMD_DISABLED_POWER_SAVE "iwnpi wlan0 lna_off"
#define CMD_GET_POWER_SAVE_STATUS  "iwnpi wlan0 lna_status"






/* Unsigned fixed width types */
typedef uint8_t CsrUint8;
typedef uint16_t CsrUint16;
typedef uint32_t CsrUint32;

/* Signed fixed width types */
typedef int8_t CsrInt8;
typedef int16_t CsrInt16;
typedef int32_t CsrInt32;

/* Boolean */
typedef CsrUint8 CsrBool;


/* MAC address */
typedef struct
{
    CsrUint8 a[6];
} CsrWifiMacAddress;

typedef CsrUint8 CsrWifiPtestPreamble;


typedef struct ptest_cmd{
	CsrUint16                       type;    
	CsrUint16                       band;             
        CsrUint16                       channel;          
        CsrUint16                       sFactor;  
	union{	
		struct {
        		CsrUint16                       frequency;        
        		CsrInt16                        frequencyOffset;  
        		CsrUint16                       amplitude;  
		}ptest_cw;
		struct {
        		CsrUint16                       rate;       
        		CsrUint16                       powerLevel;       
        		CsrUint16                       length;           
        		CsrBool                         enable11bCsTestMode;
        		CsrUint32                       interval;         
        		CsrWifiMacAddress               destMacAddr;      
        		CsrWifiPtestPreamble            preamble;         
		}ptest_tx;
		struct {
        		CsrUint16                       frequency;        
        		CsrBool                         filteringEnable;  
		}ptest_rx;
	};
}PTEST_CMD_T;



#ifdef __cplusplus
}
#endif

#endif
