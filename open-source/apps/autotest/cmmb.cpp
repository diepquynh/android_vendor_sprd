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
#include "cmmb.h"
#include "cmmb_mxd.h"
#include "camera.h"
#include "perm.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--namespace sci_cmmb {
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
//------------------------------------------------------------------------------
#define CMMB_VENDOR_NO       0
#define CMMB_VENDOR_MAXSEND  1

static int sCmmbVendor = CMMB_VENDOR_NO;
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int cmmbOpen( void )
{
	FUN_ENTER;

	int ret = -1;
	do {
		sCmmbVendor = CMMB_VENDOR_NO;

		if( mxdOpen() >= 0 ) {
			// maxsend cmmb
			sCmmbVendor = CMMB_VENDOR_MAXSEND;
			ret = 0;
			break;
		}

	} while( 0 );

	FUN_EXIT;
	return ret;
}

//------------------------------------------------------------------------------
int cmmbStart( void )
{
	FUN_ENTER;

	int ret = -1;
	switch( sCmmbVendor ) {
	case CMMB_VENDOR_MAXSEND:
		ret = mxdStart();
		break;
	default:
		break;
	}
	
	FUN_EXIT;
	return ret;
}

//------------------------------------------------------------------------------
int cmmbStop( void )
{
	FUN_ENTER;

	int ret = -1;
	switch( sCmmbVendor ) {
	case CMMB_VENDOR_MAXSEND:
		ret = mxdStop();
		break;
	default:
		break;
	}

	FUN_EXIT;
	return ret;
}

//------------------------------------------------------------------------------
int cmmbClose( void )
{
	FUN_ENTER;

	int ret = -1;
	switch( sCmmbVendor ) {
	case CMMB_VENDOR_MAXSEND:
		ret = mxdClose();
		break;
	default:
		break;
	}

	FUN_EXIT;
	return ret;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--} // namespace
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
