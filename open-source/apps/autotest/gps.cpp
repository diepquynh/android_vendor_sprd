// 
// Spreadtrum Auto Tester
//
// anli   2013-03-01
//
#include "type.h"
#include "gps.h"

#include <dlfcn.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>

#include <hardware/gps.h>


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--namespace sci_gps {
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
static void * processThread( void * param );
static void   location_callback(GpsLocation* loc);
static void   status_callback(GpsStatus* status);
static void   sv_status_callback(GpsSvStatus* sv_status);
static void   nmea_callback(GpsUtcTime timestamp, const char* nmea, int length);
static void   set_capabilities_callback(uint32_t capabilities);
static void   acquire_wakelock_callback();
static void   release_wakelock_callback();
static void   request_utc_time_callback();
static pthread_t create_thread_callback(const char* name, void (*start)(void *), void* arg);

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define GPS_ACT_UNKOWN	     0x0000
#define GPS_ACT_INIT_ING     0x0001
#define GPS_ACT_INIT_FAIL    0x0002
#define GPS_ACT_INIT_DONE    0x0004
#define	GPS_ACT_START        0x0008
#define	GPS_ACT_STOP         0x0010

//------------------------------------------------------------------------------
static struct hw_module_t   * s_hwModule = NULL;
static struct gps_device_t  * s_hwDev    = NULL;
static const  GpsInterface  * sGpsIntrfc = NULL;
static        int             sSVNum     = 0;
static volatile unsigned int  sAction    = GPS_ACT_UNKOWN;
static        pthread_mutex_t sMutex     = PTHREAD_MUTEX_INITIALIZER;
static        pthread_t       sThreadID  = -1;
static int prn[64] = {0};
static int snr[64] = {0};
//------------------------------------------------------------------------------

static GpsCallbacks sGpsCallbacks = {
    sizeof(GpsCallbacks),
    location_callback,
    status_callback,
    sv_status_callback,
    nmea_callback,
    set_capabilities_callback,
    acquire_wakelock_callback,
    release_wakelock_callback,
    create_thread_callback,
    request_utc_time_callback,
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int gpsOpen( void )
{
    FUN_ENTER;

	if( NULL != s_hwModule ) {
		AT_ASSERT( NULL != s_hwDev );
		DBGMSG("already init!\n");
		return 0;
	}

	sSVNum = 0;

	int err = hw_get_module(GPS_HARDWARE_MODULE_ID,
						(hw_module_t const**)&s_hwModule);
    if( 0 == err ) {
        err = s_hwModule->methods->open(s_hwModule, GPS_HARDWARE_MODULE_ID,
				(hw_device_t**)&s_hwDev);
        if (err == 0) {
            sGpsIntrfc = s_hwDev->get_gps_interface(s_hwDev);
        }
	}

	if( err || NULL == sGpsIntrfc ) {
		ERRMSG("error: %d\n", err);
        return -2;
	}

	sAction = GPS_ACT_INIT_ING;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&sThreadID, &attr, processThread, NULL);
	
    FUN_EXIT;
    return 0;
}

//------------------------------------------------------------------------------
int gpsStart( void )
{
	FUN_ENTER;
	pthread_mutex_lock(&sMutex);

	sSVNum = 0;

	int ret = 0;

	if( sAction & GPS_ACT_INIT_DONE ) {
		sGpsIntrfc->delete_aiding_data(65535);// set factory start mode
		ret = sGpsIntrfc->start();
		DBGMSG("gps interface start: %d\n", ret);
	} else {
		DBGMSG("gps initlizing...\n");
		sAction |= GPS_ACT_START;
	}

	pthread_mutex_unlock(&sMutex);
	FUN_EXIT;
	return ret;
}

int gpsGetSVNum( void )
{
	DBGMSG("gpsGetSVNum: sSVNum = %d\n", sSVNum);
	return sSVNum;
}

int gpsGet_PRN_SN_Num( int *p, int a[] )
{
	int  i = 0;
	*p = sSVNum;
	DBGMSG("gpsGet_PRN_SN_Num: sSVNum=%d\n", sSVNum);
	for(i=0;i<sSVNum;i++) {
		a[i] = snr[i];
		DBGMSG("gpsGet_PRN_SN_Num: *p=%d i=%d a=%d\n", *p, i, a[i]);
	}
	return 0;
}
//------------------------------------------------------------------------------
int gpsStop( void )
{
	FUN_ENTER;
	pthread_mutex_lock(&sMutex);

	int ret = 0;

	if( sAction & GPS_ACT_INIT_DONE ) {
		ret = sGpsIntrfc->stop();
		DBGMSG("gps interface stop: %d\n", ret);
	} else {
		DBGMSG("gps initlizing...\n");
		sAction &= ~GPS_ACT_START;
		sAction |= GPS_ACT_STOP;
	}

	sSVNum = 0;

	pthread_mutex_unlock(&sMutex);
	FUN_EXIT;
	return ret;

}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int gpsClose( void )
{
    FUN_ENTER;

	pthread_mutex_lock(&sMutex);
	sAction = GPS_ACT_UNKOWN;
	pthread_mutex_unlock(&sMutex);

	if( NULL != sGpsIntrfc ) {
		sGpsIntrfc->cleanup();
	}

	if( sThreadID >= 0 ) {
		WRNMSG("gps thread is running...\n");

		int cnt = 10;
        while( cnt-- ) {
			int pk_ret = pthread_kill(sThreadID, 0);
			if( ESRCH == pk_ret || EINVAL == pk_ret ) {
				break;
			} else {
				WRNMSG("gps thread is running!\n");
				usleep(100 * 1000);
			}
        }
	}
	sThreadID = -1;

	if( NULL != s_hwDev && NULL != s_hwDev->common.close ) {
		s_hwDev->common.close(&(s_hwDev->common));
	}

	/*Don't release the .so lib*/
	//if( NULL != s_hwModule ) {
	//	dlclose(s_hwModule->dso);
	//}

	sGpsIntrfc = NULL;
	s_hwDev    = NULL;
	s_hwModule = NULL;

    FUN_EXIT;
    return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void * processThread( void * param )
{
	FUN_ENTER;

	if( NULL == sGpsIntrfc ) {
		return (void *)-1;
	}

	pthread_mutex_lock(&sMutex);

	if( sAction & GPS_ACT_INIT_ING ) {
		pthread_mutex_unlock(&sMutex);

		DBGMSG("gps interface do init.\n");
		int err = sGpsIntrfc->init(&sGpsCallbacks);

		pthread_mutex_lock(&sMutex);

		sAction &= ~GPS_ACT_INIT_ING;
		if( err != 0 ) {
			ERRMSG("gps interface init error: %s\n", strerror(errno));

			sAction |= GPS_ACT_INIT_FAIL;
			pthread_mutex_unlock(&sMutex);
			return (void *)-2;
		} else {
			sAction |= GPS_ACT_INIT_DONE;
		}
	}

	if( sAction & GPS_ACT_START ) {
		sAction &= ~GPS_ACT_START;
		DBGMSG("gps interface do start.\n");
		if( sGpsIntrfc->start() != 0 ) {
			ERRMSG("gps interface start error: %s\n", strerror(errno));
			pthread_mutex_unlock(&sMutex);
			return (void *)-3;
		}
	}

	if( sAction & GPS_ACT_STOP ) {
		sAction &= ~GPS_ACT_STOP;
		DBGMSG("gps interface do stop.\n");
		if( sGpsIntrfc->stop() != 0 ) {
			ERRMSG("gps interface stop error: %s\n", strerror(errno));
		}
	}

	sThreadID = -1;

	pthread_mutex_unlock(&sMutex);
	
	FUN_EXIT;
	return NULL;
}

//------------------------------------------------------------------------------
void location_callback(GpsLocation* loc)
{
	FUN_ENTER;

	AT_ASSERT( NULL != loc );

	DBGMSG("loc: latitude  = %02f\n", loc->latitude);
	DBGMSG("loc: longitude = %02f\n", loc->longitude);
	DBGMSG("loc: altitude  = %02f\n", loc->altitude);
	DBGMSG("loc: speed     = %02f\n", loc->speed);
	DBGMSG("loc: bearing   = %02f\n", loc->bearing);
	DBGMSG("loc: accuracy  = %02f\n", loc->accuracy);
	//DBGMSG("loc: time      = %s\n",   ctime64(&(loc->timestamp)));

	FUN_EXIT;
}

//------------------------------------------------------------------------------
void status_callback(GpsStatus* status)
{
	FUN_ENTER;

	FUN_ENTER;

	AT_ASSERT( NULL != status );

	DBGMSG("GpsStatus: status  = %d\n", status->status);

	FUN_EXIT;
}

//------------------------------------------------------------------------------
void sv_status_callback(GpsSvStatus* sv_status)
{
	FUN_ENTER;

	AT_ASSERT( NULL != sv_status );

	sSVNum = sv_status->num_svs;

	DBGMSG("GpsSvStatus: num_svs  = %d\n", sv_status->num_svs);
	memset(prn, 0 , sizeof(prn));
	memset(snr, 0 , sizeof(snr));

	for( int i = 0; i < sv_status->num_svs; ++i ) {
		DBGMSG("GpsSvStatus lst[%d]: prn       = %d\n",   i, sv_status->sv_list[i].prn);
		DBGMSG("GpsSvStatus lst[%d]: snr       = %02f\n", i, sv_status->sv_list[i].snr);
		DBGMSG("GpsSvStatus lst[%d]: elevation = %02f\n", i, sv_status->sv_list[i].elevation);
		DBGMSG("GpsSvStatus lst[%d]: azimuth   = %02f\n", i, sv_status->sv_list[i].azimuth);
		prn[i] = sv_status->sv_list[i].prn;
		snr[i] = sv_status->sv_list[i].snr;
		DBGMSG("GpsSvStatus lst[%d]: prn=%d, snr=%d\n", i, prn[i], snr[i]);
	}

	FUN_EXIT;
}

//------------------------------------------------------------------------------
void nmea_callback(GpsUtcTime timestamp, const char* nmea, int length)
{
	//FUN_ENTER;

	//DBGMSG("time   = %s\n", ctime64(&timestamp));
	//DBGMSG("length = %d\n", length);
	if( length != 0 && NULL != nmea ) {
		DBGMSG("length = %d, nmea = %s\n", length, nmea);
	}

	//FUN_EXIT;
}

//------------------------------------------------------------------------------
void set_capabilities_callback(uint32_t capabilities)
{
	FUN_ENTER;

	FUN_EXIT;
}

//------------------------------------------------------------------------------
void acquire_wakelock_callback()
{
	FUN_ENTER;

	FUN_EXIT;
}

//------------------------------------------------------------------------------
void release_wakelock_callback()
{
	FUN_ENTER;

	FUN_EXIT;
}

//------------------------------------------------------------------------------
void request_utc_time_callback()
{
	FUN_ENTER;

	FUN_EXIT;
}

//------------------------------------------------------------------------------
typedef void (*PFUN_GPS_THREAD_CALLBACK)(void *);
static PFUN_GPS_THREAD_CALLBACK gps_thread_callback = NULL;

static void * gps_thread_callback_stub( void * prm )
{
	if( NULL != gps_thread_callback ) {
		gps_thread_callback(prm);
	}
	return NULL;
}

pthread_t create_thread_callback(const char* name, void (*start)(void *), void* arg)
{
	int ret = 0;
	FUN_ENTER;

	gps_thread_callback = start;

	pthread_t id = 0;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&id, &attr, gps_thread_callback_stub, arg);
	pthread_setname_np(id, name);

	FUN_EXIT;
	return id;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--} // namespace
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
