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
#include "atv.h"
#include "camera.h"
#include "perm.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--namespace sci_atv {
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
using namespace android;
using namespace at_perm;
//------------------------------------------------------------------------------
// enable or disable local debug
#define DBG_ENABLE_DBGMSG
#define DBG_ENABLE_WRNMSG
#define DBG_ENABLE_ERRMSG
#define DBG_ENABLE_INFMSG
#define DBG_ENABLE_FUNINF
#include "debug.h"

//------------------------------------------------------------------------------
#define DEV_LIB_NAME  "libjniNtvDev.so"
#define DEV_WIDTH     320
#define DEV_HEIGHT    240

//------------------------------------------------------------------------------
typedef uint (* PFUN_DEV_POWERON)(void);
typedef void (* PFUN_DEV_SET_REGION)(uint region, uint *schl, uint *echl);
typedef uint (* PFUN_DEV_SCAN_ONE_CHL)(uint chlID);
typedef uint (* PFUN_DEV_FAST_SET_CHL)(uint chlID);
typedef uint (* PFUN_DEV_POWEROFF)(void);
//------------------------------------------------------------------------------
static void *                sLibHandle  = NULL;
static PFUN_DEV_POWERON      sPowerOn    = NULL;
static PFUN_DEV_SET_REGION   sSetRegion  = NULL;
static PFUN_DEV_SCAN_ONE_CHL sScanOneChl = NULL;
static PFUN_DEV_FAST_SET_CHL sFastSetChl = NULL;
static PFUN_DEV_POWEROFF     sPowerOff   = NULL;
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
int atvOpen( void )
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

	sPowerOn    = (PFUN_DEV_POWERON)dlsym(sLibHandle, "atv_nmi600_poweron_init");
	AT_ASSERT( NULL != sPowerOn );
	sSetRegion  = (PFUN_DEV_SET_REGION)dlsym(sLibHandle, "atv_nmi600_set_region");
	AT_ASSERT( NULL != sSetRegion );
	sScanOneChl = (PFUN_DEV_SCAN_ONE_CHL)dlsym(sLibHandle, "atv_nmi600_scan_one_channel");
	AT_ASSERT( NULL != sScanOneChl );
	sFastSetChl = (PFUN_DEV_FAST_SET_CHL)dlsym(sLibHandle, "atv_nmi600_fast_set_channel");
	AT_ASSERT( NULL != sFastSetChl );
	sPowerOff   = (PFUN_DEV_POWEROFF)dlsym(sLibHandle, "atv_nmi600_poweroff_deinit");
	AT_ASSERT( NULL != sPowerOff );

	if( camOpen(CAM_ID_ATV, DEV_WIDTH, DEV_HEIGHT) < 0 ) {
		ERRMSG("camOpen '%d' error: %s\n", CAM_ID_ATV, strerror(errno));
		dlclose(sLibHandle);
		sLibHandle = NULL;
		return -2;
	}

	permInstallService(NULL);

	AudioSystem::setDeviceConnectionState(AUDIO_DEVICE_OUT_FM_SPEAKER,
            AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE, "");
    AudioSystem::setDeviceConnectionState(AUDIO_DEVICE_OUT_FM_HEADSET,
            AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE, "");

	FUN_EXIT;
	return 0;
}

//------------------------------------------------------------------------------
int atvPlay( void )
{
	FUN_ENTER;

	if( NULL == sLibHandle ) {
		WRNMSG("not opened!");
		return -1;
	}

	uint done = sPowerOn();
	DBGMSG("power on done = %d\n", done);

	uint chl, schl, echl;
	sSetRegion(1, &schl, &echl);

	for(chl = schl; chl <= echl; ++chl )
	{
		if( 1 == sScanOneChl(chl) )
		{
			DBGMSG("scaned channel = %d\n", chl);
			break;
		}
	}

	done = sFastSetChl(chl);
	DBGMSG("fast set chl done = %d\n", done);

	int ret = camStart();
	AudioSystem::setDeviceConnectionState(AUDIO_DEVICE_OUT_FM_HEADSET,
            AUDIO_POLICY_DEVICE_STATE_AVAILABLE, "");

	FUN_EXIT;
	return ret;
}

//------------------------------------------------------------------------------
int atvStop( void )
{
	FUN_ENTER;

	camStop();

	if( NULL != sPowerOff ) {
		sPowerOff();
	}

	AudioSystem::setDeviceConnectionState(AUDIO_DEVICE_OUT_FM_HEADSET,
            AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE, "");

	FUN_EXIT;
	return 0;
}

//------------------------------------------------------------------------------
int atvClose( void )
{
	FUN_ENTER;

	camClose();

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
