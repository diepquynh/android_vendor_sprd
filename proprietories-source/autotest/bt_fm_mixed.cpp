//
// Spreadtrum Auto Tester
//
// anli   2012-11-10
//

//note:
//        add code to match Marlin & 2341A chip and support FM,BT coexist.
//        2015/01/22 By zhangyj

#include <pthread.h>
#include <stdlib.h>

#include <hardware/bluetooth.h>
#include <hardware/bt_fm.h>
#include <hardware/bt_av.h>
#include <hardware/bt_gatt.h>
#include <hardware/bt_gatt_client.h>
#include <hardware/bt_gatt_server.h>
#include <hardware/bt_gatt_types.h>
#include <hardware/bt_hf.h>
#include <hardware/bt_hh.h>
#include <hardware/bt_hl.h>
#include <hardware/bt_pan.h>
#include <hardware/bt_rc.h>
#include <hardware/bt_sock.h>
//#include "btif_util.h"
#include <hardware/fm.h>

#include <media/AudioRecord.h>
#include <media/AudioSystem.h>
#include <media/AudioTrack.h>
#include <media/mediarecorder.h>
#include <system/audio.h>
#include <system/audio_policy.h>
#include <time.h>


#include <cutils/properties.h>

#include "type.h"
#include "bt.h"
#include "util.h"
#include "perm.h"
extern int SendAudioTestCmd(const uchar * cmd,int bytes);
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--namespace sci_fm {
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
using namespace android;
using namespace at_perm;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--namespace sci_bt {
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//------------------------------------------------------------------------------
// enable or disable local debug
#define DBG_ENABLE_DBGMSG
#define DBG_ENABLE_WRNMSG
#define DBG_ENABLE_ERRMSG
#define DBG_ENABLE_INFMSG
#define DBG_ENABLE_FUNINF
#include "debug.h"


enum fmStatus {
FM_STATE_DISABLED,
FM_STATE_ENABLED,
FM_STATE_PLAYING,
FM_STATE_STOPED,
FM_STATE_PANIC,
};

//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
static const bt_bdaddr_t  BADDR_ANY    = {{0, 0, 0, 0x00, 0x00, 0x00}};
static const bt_bdaddr_t  BADDR_LOCAL  = {{0, 0, 0, 0xff, 0xff, 0xff}};

static pthread_mutex_t sMutxExit    = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  sCondExit    = PTHREAD_COND_INITIALIZER;
static int             sInqStatus   = BT_STATUS_INQUIRE_UNK;
static bdremote_t      sRemoteDev[MAX_SUPPORT_RMTDEV_NUM];
static int             sRemoteDevNum = 0;

static int cb_counter;
static int lock_count;
static timer_t timer;
static alarm_cb saved_callback;
static void *saved_data;
//------------------------------------------------------------------------------
static void * btInquireThread( void *param );
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static hw_module_t * s_hwModule = NULL;
static hw_device_t * s_hwDev    = NULL;

static bluetooth_device_t* sBtDevice = NULL;
static const bt_interface_t* sBtInterface = NULL;
static const btfm_interface_t* sFmInterface = NULL;
static bt_state_t sBtState = BT_STATE_OFF;
static bt_state_t sFmState = BT_RADIO_OFF;
static bool set_wake_alarm(uint64_t delay_millis, bool, alarm_cb cb, void *data);
static int acquire_wake_lock(const char *);
static int release_wake_lock(const char *);
static int sFmStatus = FM_STATE_PANIC;
//------------------------------------------------------------------------------
//Callbacks;
//------------------------------------------------------------------------------

void btfmEnableCallback (int status)  {  if (status == BT_STATUS_SUCCESS) sFmStatus = FM_STATE_ENABLED; DBGMSG("Enable callback, status: %d", sFmStatus); }
void btfmDisableCallback (int status) { if (status == BT_STATUS_SUCCESS) sFmStatus = FM_STATE_DISABLED;   DBGMSG("Disable callback, status: %d", sFmStatus); }
void btfmtuneCallback (int status, int rssi, int snr, int freq) { DBGMSG("Tune callback, status: %d, freq: %d", status, freq); }
void btfmMuteCallback (int status, BOOLEAN isMute){ DBGMSG("Mute callback, status: %d, isMute: %d", status, isMute); }
void btfmSearchCallback (int status, int rssi, int snr, int freq){ DBGMSG("Search callback, status: %d", status); }
void btfmSearchCompleteCallback(int status, int rssi, int snr, int freq){ DBGMSG("Search complete callback"); }
void btfmAudioModeCallback(int status, int audioMode){ DBGMSG("Audio mode change callback, status: %d, audioMode: %d", status, audioMode); }
void btfmAudioPathCallback(int status, int audioPath){ DBGMSG("Audio path change callback, status: %d, audioPath: %d", status, audioPath); }
void btfmVolumeCallback(int status, int volume){ DBGMSG("Volume change callback, status: %d, volume: %d", status, volume); }
void btDeviceFoundCallback(int num_properties, bt_property_t *properties);
void btDiscoveryStateChangedCallback(bt_discovery_state_t state);
void btAdapterStateChangedCallback(bt_state_t state);

int scan_enable = BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE;
bt_property_t scan_enable_property = {
  BT_PROPERTY_ADAPTER_SCAN_MODE,
  1,
  &scan_enable,
};

static bt_os_callouts_t stub = {
  sizeof(bt_os_callouts_t),
  set_wake_alarm,
  acquire_wake_lock,
  release_wake_lock,
};

static bt_callbacks_t btCallbacks = {
    sizeof(bt_callbacks_t),
    btAdapterStateChangedCallback, /*adapter_state_changed */
    NULL, /*adapter_properties_cb */
    NULL, /* remote_device_properties_cb */
    btDeviceFoundCallback, /* device_found_cb */
    btDiscoveryStateChangedCallback, /* discovery_state_changed_cb */
    NULL, /* pin_request_cb  */
    NULL, /* ssp_request_cb  */
    NULL, /*bond_state_changed_cb */
    NULL, /* acl_state_changed_cb */
    NULL, /* thread_evt_cb */
    NULL, /*dut_mode_recv_cb */
//    NULL, /*authorize_request_cb */
    NULL, /* le_test_mode_cb */
    NULL,
#if defined (SPRD_WCNBT_MARLIN) || defined (SPRD_WCNBT_SR2351)
    NULL,
#endif

};

static btfm_callbacks_t fmCallback = {
    sizeof (btfm_callbacks_t),
    btfmEnableCallback,             // btfm_enable_callback
    btfmDisableCallback,            // btfm_disable_callback
    btfmtuneCallback,               // btfm_tune_callback
    btfmMuteCallback,               // btfm_mute_callback
    btfmSearchCallback,             // btfm_search_callback
    btfmSearchCompleteCallback,     // btfm_search_complete_callback
    NULL,                           // btfm_af_jump_callback
    btfmAudioModeCallback,          // btfm_audio_mode_callback
    btfmAudioPathCallback,          // btfm_audio_path_callback
    NULL,                           // btfm_audio_data_callback
    NULL,                           // btfm_rds_mode_callback
    NULL,                           // btfm_rds_type_callback
    NULL,                           // btfm_deemphasis_callback
    NULL,                           // btfm_scan_step_callback
    NULL,                           // btfm_region_callback
    NULL,                           // btfm_nfl_callback
    btfmVolumeCallback,             //btfm_volume_callback
    NULL,                           // btfm_rds_data_callback
    NULL,                           // btfm_rtp_data_callback
    NULL,                           //btfm_rssi_callback
    NULL,                           //btfm_snr_callback
    NULL,                           //btfm_rf_mute_callback
};
static bool set_wake_alarm(uint64_t delay_millis, bool, alarm_cb cb, void *data) {
/*  saved_callback = cb;
  saved_data = data;

  struct itimerspec wakeup_time;
  memset(&wakeup_time, 0, sizeof(wakeup_time));
  wakeup_time.it_value.tv_sec = (delay_millis / 1000);
  wakeup_time.it_value.tv_nsec = (delay_millis % 1000) * 1000000LL;
  timer_settime(timer, 0, &wakeup_time, NULL);
  return true;
*/
    static timer_t timer;
  static bool timer_created;

  if (!timer_created) {
    struct sigevent sigevent;
    memset(&sigevent, 0, sizeof(sigevent));
    sigevent.sigev_notify = SIGEV_THREAD;
    sigevent.sigev_notify_function = (void (*)(union sigval))cb;
    sigevent.sigev_value.sival_ptr = data;
    timer_create(CLOCK_MONOTONIC, &sigevent, &timer);
    timer_created = true;
  }

  struct itimerspec new_value;
  new_value.it_value.tv_sec = delay_millis / 1000;
  new_value.it_value.tv_nsec = (delay_millis % 1000) * 1000 * 1000;
  new_value.it_interval.tv_sec = 0;
  new_value.it_interval.tv_nsec = 0;
  timer_settime(timer, 0, &new_value, NULL);

  return true;
}
void btAdapterStateChangedCallback(bt_state_t state)
{
    if(state == BT_STATE_OFF || state == BT_STATE_ON)
    {
        sBtState = state;
        DBGMSG("BT Adapter State Changed: %d",state);
#ifdef SPRD_WCNBT_MARLIN
        if(state == BT_STATE_ON)
            sBtInterface->set_adapter_property(&scan_enable_property);
#endif
    } else if(state == BT_RADIO_OFF || state == BT_RADIO_ON) {
        DBGMSG("FM Adapter State Changed: %d",state);
#ifdef SPRD_WCNBT_MARLIN
        if (state != BT_RADIO_ON) return;
            sFmInterface = (btfm_interface_t*)sBtInterface->get_fm_interface();
            int retVal = sFmInterface->init(&fmCallback);
            retVal = sFmInterface->enable(96);
#endif
    } else {
         DBGMSG("err State Changed: %d",state);
    }
}
static int acquire_wake_lock(const char *) {
  /*if (!lock_count)
    lock_count = 1;
    */
  return BT_STATUS_SUCCESS;
}

static int release_wake_lock(const char *) {
 /* if (lock_count)
    lock_count = 0;
    */
  return BT_STATUS_SUCCESS;
}
static char *btaddr2str(const bt_bdaddr_t *bdaddr, bdstr_t *bdstr)
{
    char *addr = (char *) bdaddr->address;

    sprintf((char*)bdstr, "%02x:%02x:%02x:%02x:%02x:%02x",
                       (int)addr[0],(int)addr[1],(int)addr[2],
                       (int)addr[3],(int)addr[4],(int)addr[5]);
    return (char *)bdstr;
}

void btDeviceFoundCallback(int num_properties, bt_property_t *properties)
{
	int i = 0;
	int j = 0;
	char* pName = NULL;
	int nameLen = 0;
	bt_bdaddr_t * pBdaddr = NULL;
	DBGMSG("BT found device, got her (%d)  properties",num_properties);
	if(sRemoteDevNum >= MAX_SUPPORT_RMTDEV_NUM){
		DBGMSG("BT inquiry device list was full!");
		return;
	}

	for(i = 0; i < num_properties; i++){
		switch(properties[i].type){
			case BT_PROPERTY_BDNAME:{
				pName = (char*)properties[i].val;
				nameLen = properties[i].len;
			}continue;

			case BT_PROPERTY_BDADDR:{
				pBdaddr = (bt_bdaddr_t *)properties[i].val;
				for(j = 0; j < sRemoteDevNum; j++){
					if((pBdaddr->address[0] ==  sRemoteDev[j].addr_u8[0])
					&&(pBdaddr->address[1] ==  sRemoteDev[j].addr_u8[1])
					&&(pBdaddr->address[2] ==  sRemoteDev[j].addr_u8[2])
					&&(pBdaddr->address[3] ==  sRemoteDev[j].addr_u8[3])
					&&(pBdaddr->address[4] ==  sRemoteDev[j].addr_u8[4])
					&&(pBdaddr->address[5] ==  sRemoteDev[j].addr_u8[5])){
						DBGMSG("BT inquiry device was OLD:%d:%d:%d:%d:%d:%d",
							pBdaddr->address[0],pBdaddr->address[1],pBdaddr->address[2],
							pBdaddr->address[3],pBdaddr->address[4],pBdaddr->address[5]);
						return;
					}
				}
			}continue;

			default:
				;
		}
	}
	DBGMSG("BT inquiry device was NEW:%d:%d:%d:%d:%d:%d",
	pBdaddr->address[0],pBdaddr->address[1],pBdaddr->address[2],
	pBdaddr->address[3],pBdaddr->address[4],pBdaddr->address[5]);
	btaddr2str(pBdaddr, (bdstr_t *)(sRemoteDev[sRemoteDevNum].addr_str));
	sRemoteDev[sRemoteDevNum].addr_u8[0] = pBdaddr->address[0];
	sRemoteDev[sRemoteDevNum].addr_u8[1] = pBdaddr->address[1];
	sRemoteDev[sRemoteDevNum].addr_u8[2] = pBdaddr->address[2];
	sRemoteDev[sRemoteDevNum].addr_u8[3] = pBdaddr->address[3];
	sRemoteDev[sRemoteDevNum].addr_u8[4] = pBdaddr->address[4];
	sRemoteDev[sRemoteDevNum].addr_u8[5] = pBdaddr->address[5];

	memcpy(sRemoteDev[sRemoteDevNum].name, pName, nameLen);

	memset(&sRemoteDev[sRemoteDevNum].name[(nameLen -1)],
			0,
			(MAX_REMOTE_DEVICE_NAME_LEN - nameLen));

	sRemoteDevNum++;
	DBGMSG("BT totally found (%d) device(s).",sRemoteDevNum);
}

void btDiscoveryStateChangedCallback(bt_discovery_state_t state)
{
	switch(state){
		case BT_DISCOVERY_STOPPED:{
			sInqStatus = BT_STATUS_INQUIRE_END;
			DBGMSG("BT Discovery State Changed: BT_DISCOVERY_STOPPED");
			}break;

		case BT_DISCOVERY_STARTED:{
			sInqStatus = BT_STATUS_INQUIRING;
			DBGMSG("BT Discovery State Changed: BT_DISCOVERY_STARTED");
			}break;

		default:
			;
	}
}

//------------------------------------------------------------------------------
//BT Callbacks END
//------------------------------------------------------------------------------

static  int btHalLoad(void)
{
    int err = 0;

    hw_module_t* module;
    hw_device_t* device;

    INFMSG("Loading HAL lib + extensions");

    err = hw_get_module(BT_HARDWARE_MODULE_ID, (hw_module_t const**)&s_hwModule);
    if (err == 0)
    {
        err = s_hwModule->methods->open(s_hwModule, BT_HARDWARE_MODULE_ID, (hw_device_t**)&s_hwDev);
        if (err == 0) {
            sBtDevice = (bluetooth_device_t *)s_hwDev;
            sBtInterface = sBtDevice->get_bluetooth_interface();
        }
    }

    DBGMSG("HAL library loaded (%s)", strerror(err));

    return err;
}

static int btCheckRtnVal(bt_status_t status)
{
    if (status != BT_STATUS_SUCCESS)
    {
        DBGMSG("HAL REQUEST FAILED ");
		return -1;
    }
    else
    {
        DBGMSG("HAL REQUEST SUCCESS");
		return 0;
    }
}

static int btInit(void)
{
    INFMSG("INIT BT ");
	int retVal = (bt_status_t)sBtInterface->init(&btCallbacks);
	DBGMSG("BT init: %d", retVal);
	if((BT_STATUS_SUCCESS == retVal)||(BT_STATUS_DONE == retVal)){
		retVal = (bt_status_t)sBtInterface->set_os_callouts(&stub);
		if((BT_STATUS_SUCCESS == retVal)||(BT_STATUS_DONE == retVal))
		{
			return (0);
		}
		else
		{
		    return (-1);
		}
	}else{
		return (-1);
	}
}


int btOpen( void )
{
    int counter = 0;
    if ((sFmStatus == FM_STATE_PANIC) || (sFmStatus == FM_STATE_DISABLED)) {
        if ( btHalLoad() < 0 ) {
			ERRMSG("BT load lib Fail");
            return -1;
        }
    } else {
        if (NULL == sBtInterface || NULL == sBtDevice) {
			ERRMSG("sBtInterface=%s,sBtDevice=%s", NULL == sBtInterface?"NULL":"Not NULL",
				NULL == sBtDevice?"NULL":"Not NULL");
            return -1;
        }
    }
    if ( btInit() < 0 ) {
        return 0;
    }

	//sBtInterface->disable();
	//utilDisableService("dbus");

	int ret = sBtInterface->enable(false);

	if( btCheckRtnVal((bt_status_t)ret) ) {
		ERRMSG("BT enable Fail(%d)!\n", ret);
	}

	while (counter++ < 3 && BT_STATE_ON != sBtState) sleep(1);

	if(sBtState == BT_STATE_ON)
		ret = BT_STATUS_SUCCESS;
	else
		ret = -1;
	sBtState = BT_STATE_ON;
	sRemoteDevNum=0;
	memset(&sRemoteDev, 0,
		   (sizeof(bdremote_t)*MAX_SUPPORT_RMTDEV_NUM));

	sInqStatus = BT_STATUS_INQUIRE_UNK;

	return (ret);

}

//------------------------------------------------------------------------------
int btClose(void)
{
	int ret    = 0;
	int counter = 0;
	sInqStatus = BT_STATUS_INQUIRE_UNK;

	ret = sBtInterface->disable();

    while (counter++ < 3 && BT_STATE_OFF != sBtState) sleep(1);

	if(sBtState == BT_STATE_OFF)
		ret = BT_STATUS_SUCCESS;
	else
		ret = -1;
	if( ret ) {
		ERRMSG("BT disable Fail(%d)!\n", ret);
	} else {
		INFMSG("BT disable OK\n");
	}

	sBtState = BT_STATE_OFF;
	return ret;
}


int btIsOpened( void )
{
	if(!sBtInterface)
	{
		sBtState = BT_STATE_OFF;
	}
	return (sBtState);
}

int btGetLocalVersion( struct hci_ver_t *ver )
{
	AT_ASSERT( ver != NULL );

	if (BT_STATE_ON != btIsOpened()) {
		ERRMSG("Device is not available\n");
		return -1;
	}

	//did noting;

	return (BT_STATUS_SUCCESS);

}

int btGetRssi( int *rssi )
{
	AT_ASSERT( rssi != NULL );

	if (BT_STATE_ON != btIsOpened()) {
		ERRMSG("Device is not available\n");
		return -1;
	}
	//did noting
	return (BT_STATUS_SUCCESS);
}

int btAsyncInquire(void)
{
	if (BT_STATE_ON!= btIsOpened()) {
		ERRMSG("Device is not available\n");
		return -1;
	}
	sRemoteDevNum = 0;
	sBtInterface->start_discovery();

	return (BT_STATUS_SUCCESS);
}


//------------------------------------------------------------------------------
int btGetInquireStatus( void )
{
    return sInqStatus;
}

void btSetInquireStatus(int status)
{
    sInqStatus = status;
}

//------------------------------------------------------------------------------
int btGetInquireResult( bdremote_t * bdrmt, int maxnum )
{
	if( BT_STATUS_INQUIRE_END == sInqStatus || BT_STATUS_INQUIRING == sInqStatus){
		int num = ( maxnum > sRemoteDevNum ) ? sRemoteDevNum : maxnum;

		if(num > 0){
			memcpy(bdrmt, sRemoteDev, num * sizeof(bdremote_t));
		}
		DBGMSG("BT found device num = %d\n", num);
		DBGMSG("BT first found dev = %x:%x:%x:%x:%x:%x",
			bdrmt[0].addr_u8[0],bdrmt[0].addr_u8[1],bdrmt[0].addr_u8[2],
			bdrmt[0].addr_u8[3],bdrmt[0].addr_u8[4],bdrmt[0].addr_u8[5]);
		return num;
	}else{
		DBGMSG("BT get INQ result failed! sInqStatus = %d",sInqStatus);
		return sInqStatus;
	}

}

#ifdef SPRD_WCNBT_MARLIN
//------------------------------------------------------------------------------
//FM
//------------------------------------------------------------------------------

int fmOpenEx( void )
{

    DBGMSG("Try to open fm \n");
    if (FM_STATE_ENABLED == sFmStatus || FM_STATE_PLAYING == sFmStatus) return 0; // fm has been opened
    if (sBtState == BT_STATE_OFF) {
        if ( btHalLoad() < 0 ) {
            return -1;
        }
    } else {
        if (NULL == sBtInterface || NULL == sBtDevice) {
            return -1;
        }
    }

    if ( btInit() < 0 ) {
        return -1;
    }
    sBtInterface->enableRadio();
    DBGMSG("Enable radio okay, try to get fm interface \n");
    {
        char  cmd_buf[100] ={0};
        int fm_audio_volume = 11;
        sprintf(cmd_buf, "autotest_fmtest=1");
        SendAudioTestCmd((const uchar*)cmd_buf,sizeof(cmd_buf));
    }
    WRNMSG("Fm open okay \n");
    return 0;
}

//
#define V4L2_CID_PRIVATE_BASE           0x8000000
#define V4L2_CID_PRIVATE_TAVARUA_STATE  (V4L2_CID_PRIVATE_BASE + 4)

#define V4L2_CTRL_CLASS_USER            0x980000
#define V4L2_CID_BASE                   (V4L2_CTRL_CLASS_USER | 0x900)
#define V4L2_CID_AUDIO_VOLUME           (V4L2_CID_BASE + 5)
#define V4L2_CID_AUDIO_MUTE             (V4L2_CID_BASE + 9)

//------------------------------------------------------------------------------
int fmPlayEx( uint freq )
{
    if( NULL == s_hwDev ) {
        ERRMSG("not opened!\n");
        return -1;
    }

    status_t   status;
    String8 fm_volume("FM_Volume=11");
    String8 fm_mute("FM_Volume=0");

#if 0

    AudioTrack atrk;

    atrk.set(AUDIO_STREAM_FM, 44100, AUDIO_FORMAT_PCM_16_BIT, AUDIO_CHANNEL_OUT_STEREO);
    atrk.start();

    AudioTrack::Buffer buffer;
    buffer.frameCount = 64;
    status = atrk.obtainBuffer(&buffer, 1);

    if (status == NO_ERROR) {
    memset(buffer.i8, 0, buffer.size);
    atrk.releaseBuffer(&buffer);
    } else if (status != TIMED_OUT && status != WOULD_BLOCK) {
    ERRMSG(fmt,arg...)("cannot write to AudioTrack: status = %d\n", status);
    }
    atrk.stop();

#endif


    int counter = 0;

    while (sFmStatus != FM_STATE_ENABLED && counter++ < 3) sleep(1);

    if (sFmStatus != FM_STATE_ENABLED) {
         ERRMSG("fm service has not enabled, status: %d", sFmStatus);
         return -1;
    }

    sFmInterface->tune(freq * 10);
    sFmInterface->set_audio_path(0x02);
    sFmInterface->set_volume(32);
    {
        char  cmd_buf[100] ={0};
        int fm_audio_volume = 11;
        sprintf(cmd_buf, "autotest_fmtest=2");
        SendAudioTestCmd((const uchar*)cmd_buf,sizeof(cmd_buf));
    }
    DBGMSG("Fm play okay \n");
    return 0;
}

//------------------------------------------------------------------------------
int fmStopEx( void )
{
    if( NULL != s_hwDev ) {
        char  cmd_buf[100] ={0};
        int fm_audio_volume = 11;
        sprintf(cmd_buf, "autotest_fmtest=0");
        SendAudioTestCmd((const uchar*)cmd_buf,sizeof(cmd_buf));
        sFmStatus = FM_STATE_STOPED;
    }

    DBGMSG("Stop okay \n");
    return 0;
}

int fmCloseEx( void )
{
    int counter = 0;

    if (sFmInterface)
        sFmInterface->disable();
    else
        return -1;

    while (counter++ < 3 && FM_STATE_DISABLED != sFmStatus) sleep(1);
    if (FM_STATE_DISABLED != sFmStatus) return -1;

    if (sFmInterface) sFmInterface->cleanup();
    if (sBtInterface) {
         sBtInterface->disableRadio();
    }

    sFmStatus = FM_STATE_DISABLED;

    DBGMSG("Close successful.");

    return 0;
}
#endif
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--} // namespace
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
