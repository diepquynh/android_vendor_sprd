// 
// Spreadtrum Auto Tester
//
// anli   2013-03-06
//
#include <dlfcn.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/poll.h>

#include <media/AudioSystem.h>
#include <system/audio.h>
#include <system/audio_policy.h>

#include "type.h"
#include "cmmb_mxd.h"
#include "perm.h"

#undef bool
#include "cmmb/mxd/man_cmmb.h"
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--namespace sci_cmmbmxd {
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
using namespace android;
//------------------------------------------------------------------------------
// enable or disable local debug
#define DBG_ENABLE_DBGMSG
#define DBG_ENABLE_WRNMSG
#define DBG_ENABLE_ERRMSG
#define DBG_ENABLE_INFMSG
#define DBG_ENABLE_FUNINF
#include "debug.h"

//------------------------------------------------------------------------------
#define DEV_LIB_NAME                    "libmxdcmmb.so"
#define CMMB_CONFIG_DEFAULT_FREQ        43
#define CMMB_CONFIG_DEFAULT_CHANNEL     605 // cctv news(cn)
#define CMMB_CONFIG_DEFAULT_START_TS    19
#define CMMB_CONFIG_DEFAULT_TS_COUNT    4
#define CMMB_CONFIG_DEFAULT_DM_CONFIG   0x6C

//------------------------------------------------------------------------------
typedef int  (* PFUN_DEV_OPEN)(MAN_CMMB_OPEN_PARAM* param);
typedef int  (* PFUN_GET_STATUS)(MAN_CMMB_DEVICE_STATUS *status);
typedef int  (* PFUN_GET_FREQ_RANGE)(MAN_CMMB_FREQUENCY_RANGE *pFrequencyRange);
typedef int  (* PFUN_SET_FREQ)(MAN_CMMB_FREQUENCY *pFrequency,int* list,int *count);
typedef int  (* PFUN_SELECT_SERVICE)(MAN_CMMB_SERVICE_PROP *pProp);
typedef int  (* PFUN_DEV_CLOSE)(void);
//------------------------------------------------------------------------------
static void *              sLibHandle    = NULL;
static PFUN_DEV_OPEN       sOpen         = NULL;
static PFUN_GET_STATUS     sGetStatus    = NULL;
static PFUN_GET_FREQ_RANGE sGetFreqRange = NULL;
static PFUN_SET_FREQ       sSetFreq      = NULL;
static PFUN_SELECT_SERVICE sSelectService= NULL;
static PFUN_DEV_CLOSE      sClose        = NULL;
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
int mxdOpen( void )
{
	FUN_ENTER;

	if( NULL != sLibHandle ) {
		WRNMSG("already opened!");
		return 0;
	}

	sLibHandle = dlopen(DEV_LIB_NAME, RTLD_NOW);
	if( NULL == sLibHandle ) {
		ERRMSG("dlopen '%s' error: %s\n", DEV_LIB_NAME, strerror(errno));
		return -1;
	}

	sOpen = (PFUN_DEV_OPEN)dlsym(sLibHandle, "tp_man_cmmb_open");
	AT_ASSERT( NULL != sOpen );
	sGetStatus = (PFUN_GET_STATUS)dlsym(sLibHandle, "tp_man_cmmb_get_device_status");
	AT_ASSERT( NULL != sGetStatus );
	sGetFreqRange = (PFUN_GET_FREQ_RANGE)dlsym(sLibHandle, "tp_man_cmmb_get_frequency_range");
	AT_ASSERT( NULL != sGetFreqRange );
	sSetFreq = (PFUN_SET_FREQ)dlsym(sLibHandle, "tp_man_cmmb_set_frequency");
	AT_ASSERT( NULL != sSetFreq );
	sSelectService = (PFUN_SELECT_SERVICE)dlsym(sLibHandle, "tp_man_cmmb_select_service");
	AT_ASSERT( NULL != sSelectService );
	sClose = (PFUN_DEV_CLOSE)dlsym(sLibHandle, "tp_man_cmmb_close");
	AT_ASSERT( NULL != sClose );

	MAN_CMMB_OPEN_PARAM prm = { 0 };
	if( sOpen(&prm) < 0 ) {
		ERRMSG("open error: %d\n", errno);

		dlclose(sLibHandle);
		sLibHandle = NULL;
		return -2;
	}

	FUN_EXIT;
	return 0;
}

//------------------------------------------------------------------------------
int mxdStart( void )
{
	FUN_ENTER;

	if( NULL == sLibHandle ) {
		WRNMSG("not open lib\n");
		return -1;
	}

	MAN_CMMB_FREQUENCY_RANGE freqRange;
	if( sGetFreqRange(&freqRange) >= 0 ) {
		DBGMSG("freq [%d - %d]\n", freqRange.ucFrequencyNoMin, freqRange.ucFrequencyNoMax);
	}

	MAN_CMMB_FREQUENCY freq;
	freq.ucFrequencyNo = CMMB_CONFIG_DEFAULT_FREQ;
	int freqLst[128];
	int freqCnt = 128;
    if( sSetFreq(&freq, freqLst, &freqCnt) >= 0 ) {
		DBGMSG("set freqNo to %d, return count = %d\n", freq.ucFrequencyNo, freqCnt);
		for( int i = 0; i < freqCnt; ++i ) {
			DBGMSG("[%d] = %d\n", i, freqLst[i]);
		}
	}
	int ret = 0;

	MAN_CMMB_SERVICE_PROP srvProp;
	srvProp.wServiceId = CMMB_CONFIG_DEFAULT_CHANNEL;
	srvProp.ucTSStart  = CMMB_CONFIG_DEFAULT_START_TS;
	srvProp.ucTSCount  = CMMB_CONFIG_DEFAULT_TS_COUNT;
	srvProp.ucDemod    = CMMB_CONFIG_DEFAULT_DM_CONFIG;
	ret = sSelectService(&srvProp);
	if( ret < 0 ) {
		ERRMSG("select service error: %d\n", ret);
	}

	MAN_CMMB_DEVICE_STATUS status;
	ret = sGetStatus(&status);
	if( ret < 0 ) {
		WRNMSG("get status error: %d\n", ret);
		return -2;
	}

	DBGMSG("cur freq = %d\n", status.ucCurrentFrequencyNo);
	DBGMSG("cur sign = %d\n", status.ucSignalStrength);

	FUN_EXIT;
	return 0;
}

//------------------------------------------------------------------------------
int mxdStop( void )
{
	FUN_ENTER;

	if( NULL != sClose ) {
		sClose();
		sClose = NULL;
	}

	if( NULL != sLibHandle ) {
		dlclose(sLibHandle);
		sLibHandle = NULL;
	}

	FUN_EXIT;
	return 0;
}

//------------------------------------------------------------------------------
int mxdClose( void )
{
	FUN_ENTER;

	if( NULL != sLibHandle ) {
		dlclose(sLibHandle);
		sLibHandle = NULL;
	}

	FUN_EXIT;
	return 0;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--} // namespace
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
