// 
// Spreadtrum Auto Tester
//
// anli   2012-11-10
//
#include <pthread.h>
#include <stdlib.h>

#include <cutils/properties.h>
#include <hardware_legacy/wifi.h>

#include "type.h"
#include "wifi.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--namespace sci_wifi {
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//------------------------------------------------------------------------------
// enable or disable local debug
#define DBG_ENABLE_DBGMSG
#define DBG_ENABLE_WRNMSG
#define DBG_ENABLE_ERRMSG
#define DBG_ENABLE_INFMSG
#define DBG_ENABLE_FUNINF
#include "debug.h"
//------------------------------------------------------------------------------


#define WIFI_CMD_RETRY_NUM      3
#define WIFI_CMD_RETRY_TIME     1 // second
#define WIFI_MAX_AP             20

//------------------------------------------------------------------------------
static wifi_ap_t       sAPs[WIFI_MAX_AP];
static int             sStatus       = 0;
static int             sAPNum        = 0;
static pthread_mutex_t sMutxEvent    = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  sCondEvent    = PTHREAD_COND_INITIALIZER;
static volatile int    sEventLooping = 0;
static char mInterfaceCmd[64];
//------------------------------------------------------------------------------
static void * wifiEventLoop( void *param );
static void * wifiScanThread( void *param );
static int    wifiCommand(const char * cmder, char * replyBuf, int replySize);
static void   wifiParseLine(const char * begin, const char * end, struct wifi_ap_t * ap);
static int    wifiGetScanResult(struct wifi_ap_t * aps, int maxnum);
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int wifiGetStatus( void ) 
{
    DBGMSG("status = %d\n", sStatus);
    return sStatus;
}

int wifiOpen( void )
{
    FUN_ENTER;

	if( is_wifi_driver_loaded() ) {
		wifiClose();
	}

    if( wifi_load_driver() != 0 ) {
        ERRMSG("wifi_load_driver fail!\n");
        return -1;
    }

    if(wifi_start_supplicant(0) != 0 ) {
        wifi_unload_driver();
        ERRMSG("wifi_start_supplicant fail!\n");
        return -2;
    }

	int cnn_num = 5;
	int cnn_ret = -1;
	while( cnn_num-- ) {
		usleep(200 * 1000);

		if( wifi_connect_to_supplicant() != 0 ) {
			continue;
		} else {
			cnn_ret = 0;
			break;
		}
    }

	if( 0 != cnn_ret ) {
        wifi_stop_supplicant(0);
        wifi_unload_driver();
        ERRMSG("wifi_connect_to_supplicant fail!\n");
        return -3;
	}

    sStatus       = 0;
    sAPNum        = -1; 
    sEventLooping = 1;
    
    pthread_t      ptid;
    pthread_attr_t attr;
    pthread_attr_init (&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&ptid, &attr, wifiEventLoop, NULL);
    
    sStatus |= WIFI_STATUS_OPENED;
    FUN_EXIT;
    return 0;
}

int wifiAsyncScanAP( void )
{
    FUN_ENTER;
    if( !(sStatus & WIFI_STATUS_OPENED) ) {
        DBGMSG("wifi not opened\n");
        return -1;
    }
    if( sStatus & WIFI_STATUS_SCANNING ) {
        DBGMSG("already scannning...\n");
        return 0;
    }

    sStatus |= WIFI_STATUS_SCANNING;
    sStatus &= ~WIFI_STATUS_SCAN_END;
    
    pthread_t      ptid;
    pthread_attr_t attr;
    pthread_attr_init (&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&ptid, &attr, wifiScanThread, NULL);
    
    FUN_EXIT;
    return 0;
}

int wifiGetAPs(struct wifi_ap_t * aps, int maxnum)
{
    FUN_ENTER;
    if( sAPNum <= 0 ) {
        return -1;
    }
    
    int num = maxnum > sAPNum ? sAPNum : maxnum;
    memcpy(aps, sAPs, num * sizeof(wifi_ap_t));
    FUN_EXIT;
    return num;
}
//------------------------------------------------------------------------------

int wifiGetMACAddr( char * addr, int size )
{
    AT_ASSERT( addr != NULL );
    AT_ASSERT( size >  18 );
    
    char reply[256];
    int  len = sizeof reply;

    len = wifiCommand("DRIVER MACADDR", reply, len);
    if( len > 0 ) {
        for( int i = 0; i < len; ++i ) {
            if( reply[i] >= 'a' && reply[i] <= 'z' ) {
                reply[i] += ('A' - 'a');
            }
        }
        char * paddr = strstr(reply, "MACADDR = ");
        if( NULL != paddr ) {
            strncpy(addr, paddr + 10, size);
            return len - 10;
        }
    }
    
    INFMSG("wifiGetMACAddr fail: reply = %s\n", reply);
    return -1;
}

int wifiGetRssi( int *rssi )
{
    char reply[256];
    int  len = sizeof reply;
    int  ret = -1;

    len = wifiCommand("DRIVER RSSI", reply, len); // -APPROX  SIGNAL_POLL
    // reply comes back in the form "<SSID> rssi XX" where XX is the
    // number we're interested in.  if we're associating, it returns "OK".
    // beware - <SSID> can contain spaces.
    if( len > 0 && strcmp(reply, "OK") != 0) {
        INFMSG("wifiGetRssi: len = %d, rep = %s\n", len, reply);
        // beware of trailing spaces
        char* end = reply + strlen(reply);
        while (end > reply && end[-1] == ' ') {
            end--;
        }
        *end = 0;

        char* lastSpace = strrchr(reply, ' ');
        // lastSpace should be preceded by "rssi" and followed by the value
        if (lastSpace && !strncmp(lastSpace - 4, "rssi", 4)) {
            sscanf(lastSpace + 1, "%d", rssi);
            ret = 0;
        }
    }
    
    return ret;
}

int wifiSyncScanAP(struct wifi_ap_t * aps, int maxnum)
{
    AT_ASSERT( aps != NULL );
    AT_ASSERT( maxnum >  0 );
    
    char reply[64];
    int  len;

#if 1
    //--len = sizeof reply;
    //--wifiCommand("BLACKLIST clear", reply, len);

    reply[0] = 0;
    len = sizeof reply;
    //-- don't care result anli 2013-03-05i
    /*modify by sam.sun,20140702, reason: for solve coverity issue, just check the return value*/
    if( (wifiCommand("DRIVER SCAN-ACTIVE", reply, len) <= 0) || (NULL == strstr(reply, "OK")) ) {
        ERRMSG("scan passive fail: %s\n", reply);
         //return -1;
    }
#endif

	reply[0] = 0;
    len = sizeof reply;
    if( (wifiCommand("SCAN", reply, len) <= 0) || (NULL == strstr(reply, "OK")) ) {
        ERRMSG("scan fail: %s\n", reply);
        return -2;
    }

    struct timespec to;
    to.tv_nsec = 0;
    to.tv_sec  = time(NULL) + 4;
        
    pthread_mutex_lock(&sMutxEvent);
    if( 0 == pthread_cond_timedwait(&sCondEvent, &sMutxEvent, &to) ) {
        DBGMSG("wait done.\n");
    } else {
        WRNMSG("wait timeout or error: %s!\n", strerror(errno));
    }
    pthread_mutex_unlock(&sMutxEvent);
    
    int ret = wifiGetScanResult(aps, maxnum);
    FUN_EXIT;
    return ret;
}

int wifiGetScanResult(struct wifi_ap_t * aps, int maxnum)
{
    FUN_ENTER;
    AT_ASSERT( aps != NULL );
    AT_ASSERT( maxnum >  0 );
    
    char reply[4096];
    int  len;
    
    len = wifiCommand("SCAN_RESULTS", reply, sizeof reply);
    if( (len <= 0) || (NULL == strstr(reply, "bssid")) ) {
        ERRMSG("scan results fail: %s\n", reply);
        return -2;
    }
    
    //--INFMSG("[len = %d]%s\n", len, reply);
    //FILE * pf = fopen("/mnt/sdcard/ap.txt", "w");
    //if( pf ) {
    //    fwrite(reply, 1, len, pf);
    //    fclose(pf);
    //}
    
    char * pcur = reply;
    char * pend = reply + len;

    // skip first line(end with '\n'): bssid / frequency / signal level / flags / ssid
    while( *pcur++ != '\n' && pcur < pend ) {
        // nothing
    }

    //
    int num = 0;
    for( ; (num < maxnum) && (pcur < pend); ++num ) {
        
        char * line = pcur;
        while( *pcur != '\n' && pcur++ < pend ) {
            // nothing
        }
        
        wifiParseLine(line, pcur, aps + num);
        // skip \n
        pcur++;
    }
    FUN_EXIT;
    return num;
}

//------------------------------------------------------------------------------
int wifiClose( void )
{   
    FUN_ENTER;
    sEventLooping = 0;

    wifi_stop_supplicant(0);
    wifi_unload_driver();
    wifi_close_supplicant_connection();
    
    sStatus = 0;

	memset(sAPs, 0, WIFI_MAX_AP * sizeof(wifi_ap_t));
	sAPNum = 0;

    FUN_EXIT;
    return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void * wifiEventLoop( void *param )
{
    DBGMSG("---- wifiEventLoop enter ----\n");
    
    #define EVT_MAX_LEN 127
    char evt[EVT_MAX_LEN + 1];
    int len = 0;  
    
    evt[EVT_MAX_LEN] = 0;
    while( sEventLooping ) {
        evt[0] = 0;
        len = wifi_wait_for_event(evt,EVT_MAX_LEN);
        //INFMSG("event: %s\n", evt);
/*
		if( NULL != strstr(evt, "BSS-ADDED") ) {

            DBGMSG(".... bss aaded ....\n");
            pthread_mutex_lock(&sMutxEvent);
            pthread_cond_signal(&sCondEvent);
            pthread_mutex_unlock(&sMutxEvent);
        }
*/
        if( (len > 0) &&(NULL != strstr(evt, "SCAN-RESULTS")) ) {
            //char reply[64];
            //wifiCommand("AP_SCAN 1", reply, 64);

            DBGMSG(".... scan complete ....\n");
            pthread_mutex_lock(&sMutxEvent);
            pthread_cond_signal(&sCondEvent);
            pthread_mutex_unlock(&sMutxEvent);
        }

        if( NULL != strstr(evt, "TERMINATING") ) {
            break;
        } 
    }
    
    DBGMSG("---- wifiEventLoop exit ----\n");
    return NULL;
}

void * wifiScanThread( void *param )
{
    DBGMSG("---- wifiScanThread enter ----\n");
    int retryNum = 3;
	while( retryNum-- ) {
		sAPNum = wifiSyncScanAP(sAPs, WIFI_MAX_AP);
		if( 0 == sAPNum ) {
			usleep(1400 * 1000);
			DBGMSG("---- wifi retry scan ----\n");
		} else {
			break;
		}
	}
    
    sStatus &= ~WIFI_STATUS_SCANNING;
	if( sAPNum > 0 ) {
		sStatus |= WIFI_STATUS_SCAN_END;
	} else {

	}
    DBGMSG("---- wifiScanThread exit: num = %d ----\n", sAPNum);
    return NULL;
}

//------------------------------------------------------------------------------
int wifiCommand(const char * cmder, char * replyBuf, int replySize)
{
    AT_ASSERT( cmder != NULL );
    AT_ASSERT( replyBuf != NULL );
    AT_ASSERT( replySize > 0 );
    
    size_t replyLen;
    
    int fail = -1;
    
    snprintf(mInterfaceCmd, sizeof(mInterfaceCmd), "IFNAME=wlan0 %s", cmder);
    for( int i = 0; i < WIFI_CMD_RETRY_NUM; ++i ) {
        replyLen = (size_t)(replySize - 1);

        if( wifi_command(mInterfaceCmd, replyBuf, &replyLen) != 0 ) {
            WRNMSG("'%s'(%d): error, %s(%d)\n", mInterfaceCmd, i, strerror(errno), errno);
            sleep(WIFI_CMD_RETRY_TIME);
            continue;
        } else {
            fail = 0;
            break;
        }
    }
    
    if( fail ) {
        ERRMSG("'%s' retry %d, always fail!\n", cmder, WIFI_CMD_RETRY_NUM);
        return -1;
    }
    
    replyBuf[replyLen] = 0;
/*    
    char * pend = replyBuf + replyLen;
    while( pend-- > replyBuf ) {
        if( '\n' != *pend && '\r' != *pend ) {
            break;
        }
        *pend = 0;
        replyLen--;
    }
*/    
    //INFMSG("%s, len = %d\n", replyBuf, replyLen);
    return replyLen;
}

//------------------------------------------------------------------------------
void wifiParseLine(const char * begin, const char * end, struct wifi_ap_t * ap)
{
    const char * pcur = begin;
    size_t i = 0;
    // bssid
    while( *pcur != '\t' && pcur < end ) {
        if( i < sizeof(ap->smac) - 1 ) {
            ap->smac[i++] = *pcur;
        }
        pcur++;
    }
    ap->smac[i] = 0;
    int bi = 5;
    char * ps = ap->smac;
    ap->bmac[bi--] = strtol(ps, NULL, 16);
    while( bi >= 0 ) {
        while( *ps && ':' != *ps++ );
        ap->bmac[bi--] = strtol(ps, NULL, 16);
    }
    //INFMSG("mac = %s\n", ap->mac);
    AT_ASSERT( '\t' == *pcur );
    pcur++; 
    if( '\t' == *pcur ) {
        pcur++;
    }
    // frequency
    while( *pcur != '\t' && pcur < end ) {
        pcur++;
    }
    AT_ASSERT( '\t' == *pcur );
    pcur++;
    if( '\t' == *pcur ) {
        pcur++;
    }
    // signal level
	char siglev[32];
	int  sli = 0;
    while( *pcur != '\t' && pcur < end ) {
		if( sli < 31 ) {
			siglev[sli++] = *pcur;
		}
        pcur++;
    }
    AT_ASSERT( '\t' == *pcur );
	siglev[sli] = 0;
	//INFMSG("signal = %s\n", siglev);
	ap->sig_level = strtol(siglev, NULL, 10);

    pcur++;
    if( '\t' == *pcur ) {
        pcur++;
    }
    // flags
    while( *pcur != '\t' && pcur < end ) {
        pcur++;
    }
    AT_ASSERT( '\t' == *pcur );
    pcur++;
    if( '\t' == *pcur ) {
        pcur++;
    }
    // ssid
    i = 0;
    while( *pcur != '\t' && pcur < end ) {
        if( i < sizeof(ap->name) - 1 ) {
            ap->name[i++] = *pcur;
        }
        pcur++;
    }
    ap->name[i] = 0;
    //INFMSG("name = %s\n", ap->name);
    INFMSG("mac = %s, name = %s, sig = %d\n", ap->smac, ap->name, ap->sig_level);
/*    
    char bssid[64], freq[16], sig[16], flag[64], ssid[64];
    sscanf(begin, "%s\t%s\t%s\t%s\t%s", bssid, freq, sig, flag, ssid);
    
    strncpy(ap->smac, bssid, sizeof(ap->smac) - 1);
    strncpy(ap->name, ssid, sizeof(ap->name) - 1);
    
    INFMSG("mac = %s, name = %s\n", ap->smac, ap->name);
*/    
/*   
    const char * pitm = begin;
    const char * pcur = begin;
    size_t i = 0;
    // bssid
    while( *pcur != '\t' && *pcur != ' ' && pcur < end ) {
        if( i < sizeof(ap->smac) - 1 ) {
            ap->mac[i++] = *pcur;
        }
        pcur++;
    }
    ap->mac[i] = 0;
    //INFMSG("mac = %s\n", ap->smac);
    while( (*pcur == '\t' || *pcur == ' ') && pcur++ < end ) {
        // nothing
    }
    // frequency
    while( *pcur != '\t' && *pcur != ' ' && pcur++ < end ) {
        // nothing
    }
    while( (*pcur == '\t' || *pcur == ' ') && pcur++ < end ) {
        // nothing
    }
    // signal level
    while( *pcur != '\t' && *pcur != ' ' && pcur++ < end ) {
        // nothing
    }
    while( (*pcur == '\t' || *pcur == ' ') && pcur++ < end ) {
        // nothing
    }
    // flags
    while( *pcur != '\t' && *pcur != ' ' && pcur++ < end ) {
        // nothing
    }
    while( (*pcur == '\t' || *pcur == ' ') && pcur++ < end ) {
        // nothing
    }
    // ssid
    i = 0;
    while( *pcur != '\t' && *pcur != ' ' && pcur < end ) {
        if( i < sizeof(ap->name) - 1 ) {
            ap->name[i++] = *pcur;
        }
        pcur++;
    }
    ap->name[i] = 0;
    //INFMSG("name = %s\n", ap->name);
    while( (*pcur == '\t' || *pcur == ' ') && pcur++ < end ) {
        // nothing
    }
    INFMSG("mac = %s, name = %s\n", ap->smac, ap->name);
*/    
}

static char * wifiStrSkipSpace(const char * pfirst, const char * plast)
{
    AT_ASSERT( pfirst != NULL );
    AT_ASSERT( plast != NULL );
    AT_ASSERT( *pfirst == ' ' );
    
    while( *pfirst++ == ' ' && pfirst < plast ) {
        // nothing
    }
    if( pfirst >= plast ) {
        ERRMSG("first = %p, last = %p\n", pfirst, plast);
        return NULL;
    }
    
    return (char *)pfirst;
}

//------------------------------------------------------------------------------
static char * wifiStrFindChar(const char * pfirst, const char * plast, char val)
{
    AT_ASSERT( pfirst != NULL );
    AT_ASSERT( plast != NULL );

    while(*pfirst++ != val && pfirst < plast) {
        // nothing
    }
    if( pfirst >= plast ) {
        ERRMSG("first = %p, last = %p\n", pfirst, plast);
        return NULL;
    }
    return (char *)pfirst;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--} // namespace
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
