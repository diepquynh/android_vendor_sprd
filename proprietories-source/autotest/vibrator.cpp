// 
// Spreadtrum Auto Tester
//
// anli   2012-11-10
//
#include <dirent.h>

#include <hardware/vibrator.h>

#include "type.h"
#include "vibrator.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--namespace sci_vib {
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


//------------------------------------------------------------------------------
#define VIB_STATE_OFF 0
#define VIB_STATE_ON 1
static hw_module_t * s_hwModule = NULL;
static hw_device_t * s_hwDev    = NULL;
static vibrator_device_t* sVibDevice = NULL;
static int sVibState = VIB_STATE_OFF;

//------------------------------------------------------------------------------
static  int vibHalLoad(void)
{
    int err = 0;

    hw_module_t* module;
    hw_device_t* device;

    INFMSG("Loading VIBRATOR HAL lib + extensions");

    err = hw_get_module(VIBRATOR_HARDWARE_MODULE_ID, (hw_module_t const**)&s_hwModule);
    if (err == 0)
    {
        err = s_hwModule->methods->open(s_hwModule, VIBRATOR_HARDWARE_MODULE_ID, (hw_device_t**)&s_hwDev);
        if (err == 0) {
            sVibDevice = (vibrator_device_t *)s_hwDev;
        }
    }

    DBGMSG("VIBRATOR HAL library loaded (%s)", strerror(err));

    return err;
}

int vibOpen()
{
    if (sVibState == VIB_STATE_OFF) {
        if ( vibHalLoad() < 0 ) {
            ERRMSG("VIBRATOR load lib Fail");
            return -1;
        }
    } else {
        if (NULL == sVibDevice) {
            ERRMSG("sVibDevice=NULL");
            return -1;
        }
    }
    sVibState = VIB_STATE_ON;
	return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int vibTurnOn( int timeout_s  )
{
  if(sVibDevice != NULL) {
    if( 0 == sVibDevice->vibrator_on((struct vibrator_device *)sVibDevice, timeout_s * 1000) ) {
        INFMSG("vibrator on %d s\n", timeout_s );
        return 0;
    } else {
        ERRMSG("vibrator on error: %s(%d)\n", strerror(errno), errno);
        return -1;
    }
  } else {
    return -1;
  }
}

int vibTurnOff( void )
{
  if(sVibDevice != NULL) {
    if( 0 == sVibDevice->vibrator_off((struct vibrator_device *)sVibDevice) ) {
        return 0;
    } else {
        return -1;
    }
  } else {
    return -1;
  }
}

int vibClose()
{
	return 0;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--} // namespace
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
