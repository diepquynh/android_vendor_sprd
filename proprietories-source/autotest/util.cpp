// 
// Spreadtrum Auto Tester
//
// anli   2013-02-28
//

#include "type.h"
#include "util.h"

#include <dirent.h>
#include <signal.h>
#include <stdlib.h>

#include <cutils/properties.h>

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--namespace sci_util {
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
int utilEnableService( const char * svcName )
{
	int  ret = -1;
	int  num = 3;
	char propName[128];
	char status[PROPERTY_VALUE_MAX];

	snprintf(propName, 128, "init.svc.%s", svcName);
	while( num-- && property_get(propName, status, NULL) > 0 )
	{
		DBGMSG("svc '%s' status = %s\n", svcName, status);

		if( 0 == strcmp(status, "running") ) {
			ret = 0;
			break;
		}

		property_set("ctl.start", svcName);
		usleep(200 * 1000);
	}

	return ret;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int utilDisableService( const char * svcName )
{
	int  ret = -1;
	int  num = 3;
	char propName[128];
	char status[PROPERTY_VALUE_MAX];

	snprintf(propName, 128, "init.svc.%s", svcName);
	while( num-- && property_get(propName, status, NULL) > 0 )
	{
		DBGMSG("svc '%s' status = %s\n", svcName, status);

		if( 0 == strcmp(status, "stopped") ) {
			ret = 0;
			break;
		}

		property_set("ctl.stop", svcName);
		usleep(200 * 1000);
	}

	return ret;
}

int utilGetPidByName( const char * exeName )
{
	DIR * dirBlck = opendir("/proc");
    if( NULL == dirBlck ) {
        ERRMSG("opendir '/proc' fail: %s(%d)\n", strerror(errno), errno);
        return -1;
    }

    int pid = -1;

    struct dirent *de;
    while( (de = readdir(dirBlck)) ) {
        if (de->d_name[0] == '.' || DT_DIR != de->d_type )
            continue;

		//DBGMSG("d_name = %s\n", de->d_name);

		char fc = de->d_name[0];
		if( fc < '0' || fc > '9' ) {
			continue;
		}

		char statusFile[128];
        snprintf(statusFile, 128, "/proc/%s/status", de->d_name);

		FILE * fp = fopen(statusFile, "r");
        if( NULL == fp ) {
			ERRMSG("Error opening '%s' (%s)", statusFile, strerror(errno));
			continue;
		}

		char pid_name[128];
		char * pread = fgets(pid_name, 128, fp);
		fclose(fp);

		if( NULL == pread ) {
			ERRMSG("Error fgets '%s' (%s)", statusFile, strerror(errno));
            continue;
        }

		//DBGMSG("exeName  = %s\n", exeName);
		//DBGMSG("pid_name = %s\n", pid_name);
		if( strstr(pid_name, exeName) != NULL ) {
			pid = strtol(de->d_name, NULL, 10);
			break;
		}
    }

    closedir(dirBlck);
    return pid;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--} // namespace
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
