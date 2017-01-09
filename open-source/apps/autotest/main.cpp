//
// Spreadtrum Auto Tester
//
// anli   2012-11-09
//
#include "type.h"
#include "autotest.h"

//------------------------------------------------------------------------------
// enable or disable local debug
#define DBG_ENABLE_DBGMSG
#define DBG_ENABLE_WRNMSG
#define DBG_ENABLE_ERRMSG
#define DBG_ENABLE_INFMSG
#define DBG_ENABLE_FUNINF
#include "debug.h"
//------------------------------------------------------------------------------
//#define AUTOTST_DEVLP_DBG
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
int main( int argc, char *argv[] )
{
	//dbgMsg2FileEnable(1);
	dbgLogcat2File("/data/logcat.log");
	//
	//DBGMSG("........ main enter ........\n");

#ifdef AUTOTST_DEVLP_DBG
#pragma message("complie for developer debug")
	WRNMSG(".... running for developer debug ....\n");

	// for local test & debug
	autotest_dbgtest(argc, argv);
#else

	autotest_main(argc, argv);

#endif // AUTOTST_DEVLP_DBG

	//DBGMSG("........ main exit ........\n");
    return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
