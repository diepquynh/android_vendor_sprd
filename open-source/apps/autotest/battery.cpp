// 
// Spreadtrum Auto Tester
//
// anli   2012-11-10
//
#include <fcntl.h>
#include <stdlib.h>

#include "type.h"
#include "battery.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--namespace sci_bat {
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

static const char DEV_STOPCHARGER[] = "/sys/class/power_supply/battery/stop_charge";
static const char DEV_CHARGERING[]  = "/sys/class/power_supply/battery/status";
static const char DEV_CHARGER_VOL[] = "/sys/class/power_supply/battery/charger_voltage";

static const char DEV_CHARGER_REAL_TIME_CURR[] = "/sys/class/power_supply/battery/real_time_current";
static const char DEV_CHARGER_FGU_CURR[] = "/sys/class/power_supply/sprdfgu/fgu_current";


static const int  sQueryInterval = 300;  // ms
static const int  sChargerMinVol = 4200; // mV
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int batOpen( void )
{
	return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int batEnableCharger( int enb )
{
	FUN_ENTER;

    int fd = open(DEV_STOPCHARGER, O_WRONLY);
    if( fd < 0 ) {
        ERRMSG("open '%s' error: %s\n", DEV_STOPCHARGER, strerror(errno));
        return fd;
    }
    
    int ret = 0;
    char buf[4];
    if( enb ) buf[0] = '0'; 
	else      buf[0] = '1';
	
	buf[1] = '\0';
    if( write(fd, buf, 2) < 0 ) {
        ret = -1;
        ERRMSG("write error: %s\n", strerror(errno));
    }
    close(fd);
	
    usleep(sQueryInterval * 1000);

	FUN_EXIT;
    return ret;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int batIsCharging( void )
{
	FUN_ENTER;

	#define RETRY_NUM 3
	int vol = 0;
    int ret = 0;
	int num = RETRY_NUM;
	for( int i = 0; i < num; ++i ) {

		int fd = open(DEV_CHARGER_VOL, O_RDONLY);
		if( fd < 0 ) {
			ERRMSG("open '%s' error: %s\n", DEV_STOPCHARGER, strerror(errno));
			return fd;
		}
    
		char buf[64] = "\0";
		ret = read(fd, buf, sizeof buf);
		close(fd);

		if( ret < 0 ) {
			ERRMSG("read charger vol error: %s\n", strerror(errno));
			break;
		}
		DBGMSG("charger vol = %s\n", buf);
		vol += atoi(buf);

		usleep(sQueryInterval * 1000);
	}

	DBGMSG("vol = %d, num = %d\n", vol, num);

	vol /= num;
	if( vol >= sChargerMinVol ) {
		ret = 0;
	} else {
		ERRMSG("vol = %d, min = %d\n", vol, sChargerMinVol);
		ret = -1;
	}

	FUN_EXIT;
    return ret;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int batClose( void )
{
	return 0;
}


//------------------------------------------------------------------------------

int batGetRealTimeCurrent( void )
{
	FUN_ENTER;
	#define RETRY_NUM 20
	#define SLEEP_MS  25
	#define COMPARE_MA 50

	int curr = 0;
	int ret = 0;
	int num = RETRY_NUM;
	for( int i = 0; i < num; ++i ) {

		int fd = open(DEV_CHARGER_REAL_TIME_CURR, O_RDONLY);
		if( fd < 0 ) {
			ERRMSG("open '%s' error: %s\n", DEV_CHARGER_REAL_TIME_CURR, strerror(errno));
			return fd;
		}

		char buf[64] = "\0";
		ret = read(fd, buf, sizeof buf);
		close(fd);

		if( ret < 0 ) {
			ERRMSG("read RealTimeCurrent error: %s\n", strerror(errno));
			break;
		}
		DBGMSG("RealTimeCurrent buf = %s\n", buf);
		curr += atoi(buf);

		usleep(SLEEP_MS * 1000);
	}

	DBGMSG("curr = %d,num = %d\n", curr,num);

	curr /= num;
	if( curr > COMPARE_MA ) {
		ret = 0;
	} else {
		ERRMSG("curr = %d, min = %d\n", curr, COMPARE_MA);
		ret = -1;
	}

	FUN_EXIT;
	return ret;
}


int batGetFGUCurrent( void )
{
	FUN_ENTER;
	int ret = 0;
	int fgu_current = 0;

	int fd = open(DEV_CHARGER_FGU_CURR, O_RDONLY);
	if( fd < 0 ) {
		ERRMSG("open '%s' error: %s\n", DEV_CHARGER_FGU_CURR, strerror(errno));
		return fd;
	}

	char buf[64] = "\0";
	ret = read(fd, buf, sizeof buf);
	close(fd);

	if( ret < 0 ) {
		ERRMSG("read batGetFGUCurrent error: %s\n", strerror(errno));
		ret = -1;
		return ret;
	}
	DBGMSG("charger buf = %s\n", buf);
	fgu_current = atoi(buf);

	DBGMSG("fgu_current = %d\n", fgu_current);

	if( fgu_current > 0 ) {
		ret = 0;
	} else {
		ret = -1;
	}

	FUN_EXIT;
	return ret;
}


int batStatus( void )
{
	FUN_ENTER;
	int ret = 0;

	if(!batGetRealTimeCurrent()||!batGetFGUCurrent()){
		ret=0;
	}
	else{
		ret=-1;
	}

	FUN_EXIT;
	return ret;
}

//------------------------------------------------------------------------------

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--} // namespace
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
