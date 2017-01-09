// 
// Spreadtrum Auto Tester
//
// anli   2012-11-10
//
#include <dirent.h>

#include <hardware_legacy/vibrator.h>

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

//------------------------------------------------------------------------------
int vibOpen()
{
	return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int vibTurnOn( int timeout_s  )
{
    if( 0 == vibrator_on(timeout_s * 1000) ) {
        INFMSG("vibrator on %d s\n", timeout_s );
        return 0;
    } else {
        ERRMSG("vibrator on error: %s(%d)\n", strerror(errno), errno);
        return -1;
    }
}

int vibTurnOff( void )
{
    if( 0 == vibrator_off() ) {
        return 0;
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
