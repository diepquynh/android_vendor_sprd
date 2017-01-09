#include "testitem.h"

static wifi_ap_t       sAPs[WIFI_MAX_AP];
static int             sStatus       = 0;
static int             sAPNum        = 0;
static sem_t g_sem;
static pthread_mutex_t sMutxEvent    = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  sCondEvent    = PTHREAD_COND_INITIALIZER;
static volatile int    sEventLooping = 0;
static char mInterfaceCmd[64];
extern int text_rows;

void wifiParseLine(const char * begin, const char * end, struct wifi_ap_t * ap)
{
    const char * pcur = begin,* ps;
    char siglev[32],freq[32];
    int  sli = 0,fn = 0;
    size_t i = 0;
    int bi;
    // bssid
    while( *pcur != '\t' && pcur < end ) {
        if( i < sizeof(ap->smac) - 1 ) {
            ap->smac[i++] = *pcur;
        }
        pcur++;
    }
    ap->smac[i] = 0;

    pcur++;
    if( '\t' == *pcur ) {
        pcur++;
    }
    // frequency
    while( *pcur != '\t' && pcur < end ) {
	 freq[fn++] = *pcur;
        pcur++;
    }
    siglev[fn] = 0;
    ap->frequency= strtol(freq, NULL, 10);

    pcur++;
    if( '\t' == *pcur ) {
        pcur++;
    }
    // signal level
    while( *pcur != '\t' && pcur < end ) {
		if( sli < 31 ) {
			siglev[sli++] = *pcur;
		}
        pcur++;
    }
    siglev[sli] = 0;
    ap->sig_level = strtol(siglev, NULL, 10);

    pcur++;
    if( '\t' == *pcur ) {
        pcur++;
    }
    // flags
    while( *pcur != '\t' && pcur < end ) {
        pcur++;
    }
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
    LOGD("mac = %s, name = %s, sig = %d, freq = %d", ap->smac, ap->name, ap->sig_level, ap->frequency);
}

static int wcnd_down_network_interface(const char *ifname)
{
    ifc_init();
    if (ifc_down(ifname)){
        LOGE("Error downing interface: %s", strerror(errno));
    }
    ifc_close();

    return 0;
}


static char * wifiStrSkipSpace(const char * pfirst, const char * plast)
{
    if( pfirst == NULL || plast == NULL || *pfirst != ' ' )
        return (char*)-1;

    while( *pfirst++ == ' ' && pfirst < plast ) {
        // nothing
    }
    if( pfirst >= plast ) {
        LOGD("first = %p, last = %p\n", pfirst, plast);
        return NULL;
    }

    return (char *)pfirst;
}

//------------------------------------------------------------------------------
static char * wifiStrFindChar(const char * pfirst, const char * plast, char val)
{
    if( pfirst == NULL ||  plast == NULL )
		return (char*)-1;

    while(*pfirst++ != val && pfirst < plast) {
        // nothing
    }
    if( pfirst >= plast ) {
        LOGD("first = %p, last = %p\n", pfirst, plast);
        return NULL;
    }
    return (char *)pfirst;
}


int wifiGetStatus( void )
{
    LOGD("status = %d", sStatus);
    return sStatus;
}

//------------------------------------------------------------------------------
int wifiCommand(const char * cmder, char * replyBuf, int replySize)
{
    size_t replyLen;
    int fail = -1;
    int i = 0;

    if( cmder == NULL ||replyBuf == NULL || replySize <= 0 )
	return -1;

    snprintf(mInterfaceCmd, sizeof(mInterfaceCmd), "IFNAME=wlan0 %s", cmder);
    LOGD("'mInterfaceCmd=%s!", mInterfaceCmd);
    for(i = 0; i < WIFI_CMD_RETRY_NUM; ++i ) {
        replyLen = (size_t)(replySize - 1);
        if( wifi_command(mInterfaceCmd, replyBuf, &replyLen) != 0 ) {
            LOGE("'%s'(%d): error", mInterfaceCmd, i);
            sleep(WIFI_CMD_RETRY_TIME);
            continue;
        } else {
            fail = 0;
            break;
        }
    }

    if( fail ) {
        LOGD("'%s' retry %d, always fail!", cmder, WIFI_CMD_RETRY_NUM);
        return -1;
    }

    replyBuf[replyLen] = 0;

    return replyLen;
}

void * wifiEventLoop( void *param )
{
    LOGD("---- wifiEventLoop enter ----");
    char evt[EVT_MAX_LEN + 1];
    int len = 0;

	pthread_detach(pthread_self()); 	//free by itself
    evt[EVT_MAX_LEN] = 0;
    while( sEventLooping ) {
        evt[0] = 0;
        len = wifi_wait_for_event(evt,EVT_MAX_LEN);

        if( (len > 0) &&(NULL != strstr(evt, "SCAN-RESULTS")) ) {
            LOGD(".... scan complete ....");
            pthread_mutex_lock(&sMutxEvent);
            pthread_cond_signal(&sCondEvent);
            pthread_mutex_unlock(&sMutxEvent);
        }

        if( NULL != strstr(evt, "TERMINATING") ) {
            break;
        }
    }

    LOGD("---- wifiEventLoop exit ----");
    return NULL;
}

void * wifiScanThread( void *param )
{
    int retryNum = 3;

	pthread_detach(pthread_self()); 	//free by itself
    while( retryNum-- ) {
		sAPNum = wifiSyncScanAP(sAPs, WIFI_MAX_AP);
		if( 0 == sAPNum ) {
			usleep(1400 * 1000);
			LOGD("---- wifi retry scan ----");
		} else {
			break;
		}
    }

    sStatus &= ~WIFI_STATUS_SCANNING;
    if( sAPNum > 0 ) {
		sStatus |= WIFI_STATUS_SCAN_END;
    } else {

    }

    LOGD("---- wifiScanThread exit: num = %d,sStatus=%d ----", sAPNum,sStatus);
    sem_post(&g_sem);

    return NULL;
}

int wifiGetScanResult(struct wifi_ap_t * aps, int maxnum)
{
    char reply[4096];
    int  len;
    int num = 0;
    char * pcur,* pend,* line;

    if(aps == NULL || maxnum <= 0 )
		return -1;

    LOGD("---- wifiGetScanResult enter");
    len = wifiCommand("SCAN_RESULTS", reply, sizeof(reply));
    if( (len <= 0) || (NULL == strstr(reply, "bssid")) ) {
        LOGD("scan results fail: %s", reply);
        return -2;
    }
    pcur = reply;
    pend = reply + len;
    // skip first line(end with '\n'): bssid / frequency / signal level / flags / ssid
    while( *pcur++ != '\n' && pcur < pend ) {
        // nothing
    }
    for( ; (num < maxnum) && (pcur < pend); ++num ) {
        line = pcur;
        while( *pcur != '\n' && pcur++ < pend ) {
            // nothing
        }
        wifiParseLine(line, pcur, aps+num);
        // skip \n
        pcur++;
    }
    LOGD("---- wifiGetScanResult exit. num = %d",num);
    return num;
}

int wifiSyncScanAP(struct wifi_ap_t * aps, int maxnum)
{
    char reply[64];
    int  len;
    int ret = 0;
    struct timespec to;

    if( aps == NULL ||  maxnum <=  0 )
        return -1;

    LOGD("---- wifiSyncScanAP enter ----");
#if 0
    reply[0] = 0;
    len = sizeof(reply);
    //-- don't care result anli 2013-03-05i
    /*modify by sam.sun,20140702, reason: for solve coverity issue, just check the return value*/
    if( (wifiCommand("DRIVER SCAN-ACTIVE", reply, len) <= 0) || (NULL == strstr(reply, "OK")) ) {
        LOGD("scan passive fail: %s", reply);
         //return -1;
    }
#endif

    reply[0] = 0;
    len = sizeof(reply);
    if( (wifiCommand("SCAN", reply, len) <= 0) || (NULL == strstr(reply, "OK")) ) {
        LOGD("scan fail: %s", reply);
        return -2;
    }

    to.tv_nsec = 0;
    to.tv_sec  = time(NULL) + 4;

    pthread_mutex_lock(&sMutxEvent);
    if( 0 == pthread_cond_timedwait(&sCondEvent, &sMutxEvent, &to) ) {
        LOGD("wait done.");
    } else {
        LOGE("wait timeout or error!");
    }
    pthread_mutex_unlock(&sMutxEvent);

    ret = wifiGetScanResult(aps, maxnum);
    LOGD("---- wifiSyncScanAP exit ----");
    return ret;
}

int eng_wifi_open( void )
{
    int cnn_num = 5;
    int cnn_ret = -1;
    int ret = 0;
    pthread_t      ptid;
    pthread_attr_t attr;

    LOGD("---- wifi_open enter ----");
    if(is_wifi_driver_loaded() ) {
        wifiClose();
    }

    if((wifi_load_driver() )!= 0 ) {
        LOGE("wifi_load_driver fail!");
        return -1;
    }
    LOGD("------ start supplicant ------");

    if(wifi_start_supplicant(0) != 0 ) {
        wifi_unload_driver();
        LOGE("wifi_start_supplicant fail!");
    }
    LOGD("------ supplicant start finish ------");

    while( cnn_num-- ) {
       usleep(200 * 1000);
       if( wifi_connect_to_supplicant() != 0 ) {
            LOGE("wifi_connect_to_supplicant the %d th fail!",cnn_num);
            continue;
       } else {
            cnn_ret = 0;
            break;
       }
    }
    LOGD("------ after supplicant connect ------");
    if( 0 != cnn_ret ) {
       wifi_stop_supplicant(0);
       wifi_unload_driver();
       LOGE("wifi_connect_to_supplicant finally fail!");
       return -3;
    }

    sStatus       = 0;
    sAPNum        = -1;
    sEventLooping = 1;

    pthread_attr_init (&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&ptid, &attr, wifiEventLoop, NULL);

    sStatus |= WIFI_STATUS_OPENED;
    sem_post(&g_sem);

    LOGD("---- wifi_open exit ----");
    return 0;
}

int eng_wifi_get_scan_result( void )
{
    LOGD("---- wifi_scan enter ----");
    if( !(sStatus & WIFI_STATUS_OPENED) ) {
        LOGD("wifi not opened");
        return -1;
    }
    if( sStatus & WIFI_STATUS_SCANNING ) {
        sem_post(&g_sem);
        LOGD("already scannning...");
        return 0;
    }

    sStatus |= WIFI_STATUS_SCANNING;
    sStatus &= ~WIFI_STATUS_SCAN_END;

    pthread_t      ptid;
    pthread_attr_t attr;
    pthread_attr_init (&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&ptid, &attr, wifiScanThread, NULL);

    LOGD("---- wifi_scan exit ----");
    return 0;
}

int wifiClose( void )
{
    sEventLooping = 0;
    LOGD("---- wifi close ----");
    wifi_stop_supplicant(0);
    wcnd_down_network_interface("wlan0");
    wifi_unload_driver();
    wifi_close_supplicant_connection();
    LOGD("---- wifi close done ----");
    sStatus = 0;

    memset(sAPs, 0, WIFI_MAX_AP * sizeof(wifi_ap_t));
    sAPNum = 0;

    return 0;
}
static void wifi_show_result()
{
    int i = 0;
    int start_x = 1, start_y = 3;
    int dwx = 0,dwy = 0;
    char crow[1024] = {0};
    //show title
    ui_set_color(CL_YELLOW);
    ui_show_text(start_y, start_x, "bssid  freq  signal  ssid");
    gr_flip();

    start_y+=1;
    for(i = 0; i < sAPNum; i++){
        ui_set_color(i%3+2);
        memset(crow,0,sizeof(crow));
        sprintf(crow,"%s %d",sAPs[i].smac, sAPs[i].frequency);
        ui_show_text(start_y, start_x,crow);
        start_y++;
        memset(crow,0,sizeof(crow));
        sprintf(crow,"%d %s",sAPs[i].sig_level,sAPs[i].name);
        ui_show_text(start_y, start_x,crow);
        start_y++;
        if ((start_y + 1) > text_rows){
            break;
        }
    }
    gr_flip();
}

int eng_wifi_scan_start(void)
{
	int wifiStatus = 0,ret=0;

	sem_init(&g_sem, 0, 0);

	wifiStatus = wifiGetStatus();
	 if( !(WIFI_STATUS_OPENED & wifiStatus) ) {
            if( eng_wifi_open() < 0 ) {
                ret = RL_FAIL;
		   return -1;
            }
	}

	sem_wait(&g_sem);
	wifiStatus = wifiGetStatus();
	if( !(WIFI_STATUS_SCAN_END & wifiStatus) ) {
            if( !(WIFI_STATUS_SCANNING & wifiStatus) ) {
                 if( eng_wifi_get_scan_result() < 0){
			return -1;
                 }
            }
	}
	LOGD("eng_wifi_scan_start exit");
	return 0;
}

int test_wifi_start(void)
{
	LOGD("enter");
	int i=0;
	int ret=0;
	int midrow = gr_fb_height()/CHAR_HEIGHT/2;
	int midcol = gr_fb_width()/CHAR_WIDTH/2;
	int wifiStatus = 0;

	ui_fill_locked();
	ui_show_title(MENU_TEST_WIFI);
	ui_set_color(CL_WHITE);
	ui_show_text(2, 0, TEXT_BT_SCANING);
	gr_flip();

	if(eng_wifi_scan_start() < 0){
		ret = RL_FAIL;
		goto out;
	}

	sem_wait(&g_sem);
	wifiStatus = wifiGetStatus();
	if(WIFI_STATUS_SCAN_END & wifiStatus) {
		if( sAPNum > 0){
			wifi_show_result();
			ui_set_color(CL_WHITE);
			ui_show_text(midrow, midcol-strlen(TEXT_TEST_PASS)/2, TEXT_TEST_PASS);
			gr_flip();
			ret= RL_PASS;
		}else{
			ret= RL_FAIL;
		}
	}
	sleep(1);
out:
	if(RL_FAIL == ret){
		ui_set_color(CL_WHITE);
		ui_show_text(midrow, midcol-strlen(TEXT_TEST_FAIL)/2, TEXT_TEST_FAIL);
		gr_flip();
	}
	wifiClose();
	save_result(CASE_TEST_WIFI,ret);
	return ret;
}

int test_wifi_pretest(void)
{
	int ret;
	if(sAPNum > 0)
		ret= RL_PASS;
	else
		ret= RL_FAIL;

	save_result(CASE_TEST_WIFI,ret);
	return ret;
}
