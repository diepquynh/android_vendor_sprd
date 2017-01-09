#include "testitem.h"

#include <hardware/gps.h>
#include <hardware_legacy/power.h>

static void * processThread_show();
static void   location_callback(GpsLocation* loc);
static void   status_callback(GpsStatus* status);
static void   sv_status_callback(GpsSvStatus* sv_status);
static void   nmea_callback(GpsUtcTime timestamp, const char* nmea, int length);
static void   set_capabilities_callback(uint32_t capabilities);
static void   acquire_wakelock_callback();
static void   release_wakelock_callback();
static void   request_utc_time_callback();
#ifdef PLATFORM_VERSION7
static void   set_system_info_callback_func(const GnssSystemInfo* info);
static void   gnss_sv_status_callback_func(GnssSvStatus* sv_info);
#endif
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
static const  GpsInterface  * sGpsIntrfc = NULL;
static int sSVNum = 0;
static int last_sSVNum = 0;
static int sSvSnr[40] = {0};
static int sPreTest = 0;
static int sTimeout = 0;
static unsigned int nCount=0;
static time_t gOpenTime;
static int thread_run;
static unsigned int gps_is_enable = 0;
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
#ifdef PLATFORM_VERSION7
  set_system_info_callback_func,
  gnss_sv_status_callback_func,
#endif
};

int gpsOpen( void )
{
	struct gps_device_t* device;
	struct hw_module_t* module;
	int err,ret;

	sPreTest = 0;
	if(gps_is_enable == 1){
		LOGD("Gps is already enabled");
		return 0;
	}
	remove("data/gps_outdirfile.txt");
	system("rm /data/gps/log/*");

	if(sGpsIntrfc == NULL) {
		err = hw_get_module(GPS_HARDWARE_MODULE_ID, (hw_module_t const**)&module);
		if (err == 0) {
			LOGD("mmitest call hw_get_module() success");

			err = module->methods->open(module, GPS_HARDWARE_MODULE_ID,(hw_device_t**)&device);
			if (err == 0) {
				LOGD("mmitest call open() GPS MODULE success");
				sGpsIntrfc = device->get_gps_interface(device);
			}
		}

		if (sGpsIntrfc){
			LOGD("mmitest call get_gps_interface() success");
		}else{
			LOGD("mmitest call get_gps_interface() failed");
			return -1;
		}

		if(sGpsIntrfc->init(&sGpsCallbacks) != 0){
			LOGD("mmitest sGpsIntrfc->init(&sGpsCallbacks) failed");
			return -1;
		}
		sleep(1);
	}

	ret = sGpsIntrfc->start();
	if (ret != 0){
		LOGD("mmitest sGpsIntrfc->start() failed,ret = %d", ret);
	}
	else
		LOGD("mmitest sGpsIntrfc->start() success");

	gps_is_enable = 1;
	LOGD("mmitest gOpenTime=%ld", gOpenTime);
	sSVNum = 0;
	return 0;
}

int gpsClose( void )
{
	int ret = 0;

	if( NULL != sGpsIntrfc ) {
		ret = sGpsIntrfc->stop();
	}

	LOGD("sGpsIntrfc->stop() ret=%d", ret);
	sSVNum = 0;
	//sleep(1);

	//if( NULL != sGpsIntrfc ) {
	//	sGpsIntrfc->cleanup();
	//}
	//sGpsIntrfc = NULL;
	gps_is_enable = 0;
	return 0;
}

static int gpsGetSVNum( void )
{
	return sSVNum;
}

static void gps_show_result(unsigned int result)
{
	char buffer[64];
	int row = 4;
	int i;

	if(result == 1){
		ui_clear_rows(row, 2);
		memset(buffer, 0, sizeof(buffer));
		sprintf(buffer, GPS_TEST_PASS);
	}else if(result == 0){
		ui_clear_rows(row, 2);
		memset(buffer, 0, sizeof(buffer));
		sprintf(buffer, GPS_TEST_FAILED);

	}else if(result == 2){
		ui_clear_rows(row, 2);
		memset(buffer, 0, sizeof(buffer));
		sprintf(buffer, "%s, %ds", GPS_TESTING, sTimeout);
	}else{
		// Must never go to here
		LOGD("wrong show result");
	}
	ui_set_color(CL_WHITE);
	row = ui_show_text(row, 0, buffer);
	memset(buffer, 0, sizeof(buffer));
	row++;
	ui_clear_rows(row, last_sSVNum+1);
	ui_set_color(CL_WHITE);
	sprintf(buffer, "GPS NUM: %d",sSVNum);
	row = ui_show_text(row, 0, buffer);
	gr_flip();
	for(i=0;i<sSVNum;i++){
        memset(buffer, 0, sizeof(buffer));
        sprintf(buffer, "GPS SNR: %d",sSvSnr[i]);
        row = ui_show_text(row, 0, buffer);
	}
	gr_flip();
}


int gps_result=0;
static void * processThread_show()
{
	int snr_count=0;
	int snr_count1=0;
	int i;
	sSVNum=0;
	time_t now_time=time(NULL);

	sTimeout = GPS_TEST_TIME_OUT;
	while(thread_run == 1){
		now_time = time(NULL);
		LOGD("now_time = %ld, open_time=%ld", now_time,gOpenTime);
		// Catch the Star
		if(sSVNum >= 4){
			for(i=0;i<sSVNum;i++){
				if(sSvSnr[i]>=35)
				snr_count++;
			}
			if(snr_count>0){
				gps_show_result(1);
				LOGD("mmitest GPS Test PASS Catch the Star");
				ui_push_result(RL_PASS);
				sleep(3);
				break;
			}
		}
		if(now_time - gOpenTime > GPS_TEST_TIME_OUT){
			// Timeout to catch the star, gps test failed
			gps_show_result(0);
			LOGD("mmitest GPS Test FAILED");
			ui_push_result(RL_FAIL);
			sleep(1);
			break;
		}else{
			gps_show_result(2);
			LOGD("mmitest GPS Testing ......");
			sleep(1);
			sTimeout--;
		}
	}

	FUN_EXIT;
	return NULL;
}

static void location_callback(GpsLocation* loc)
{
	FUN_ENTER;

	if( NULL != loc )
	{

		SPRD_DBG("loc: latitude  = %02f", loc->latitude);
		SPRD_DBG("loc: longitude = %02f", loc->longitude);
		SPRD_DBG("loc: altitude  = %02f", loc->altitude);
		SPRD_DBG("loc: speed     = %02f", loc->speed);
		SPRD_DBG("loc: bearing   = %02f", loc->bearing);
		SPRD_DBG("loc: accuracy  = %02f", loc->accuracy);
		//SPRD_DBG("loc: time      = %s\n",   ctime64(&(loc->timestamp)));
	}
	FUN_EXIT;
}

static void status_callback(GpsStatus* status)
{

	FUN_ENTER;

	if(NULL != status )
		SPRD_DBG("GpsStatus: status  = %d", status->status);

	FUN_EXIT;
}

static void sv_status_callback(GpsSvStatus* sv_status)
{
	int i=0;
	FUN_ENTER;
	//LOGD("mmitest GpsSvStatus");
	if( NULL != sv_status )
	{
		last_sSVNum = sSVNum;
		sSVNum = sv_status->num_svs;
		sPreTest = sSVNum;

		//LOGD("mmitest GpsSvStatus: num_svs  = %d\n", sv_status->num_svs);
		for(; i < sv_status->num_svs; ++i ) {
			sSvSnr[i]=sv_status->sv_list[i].snr;
			SPRD_DBG("GpsSvStatus lst[%d]: prn       = %d",   i, sv_status->sv_list[i].prn);
			SPRD_DBG("GpsSvStatus lst[%d]: snr       = %02f", i, sv_status->sv_list[i].snr);
			SPRD_DBG("GpsSvStatus lst[%d]: elevation = %02f", i, sv_status->sv_list[i].elevation);
			SPRD_DBG("GpsSvStatus lst[%d]: azimuth   = %02f", i, sv_status->sv_list[i].azimuth);
		}
	}
	FUN_EXIT;
}

static void nmea_callback(GpsUtcTime timestamp, const char* nmea, int length)
{
	FUN_ENTER;
	if( length != 0 && NULL != nmea ) {
		SPRD_DBG("length = %d, nmea = %s", length, nmea);
	}
	FUN_EXIT;
}

static void set_capabilities_callback(uint32_t capabilities)
{
	FUN_ENTER;
	SPRD_DBG("cap = %d",capabilities);
	FUN_EXIT;
}

static void acquire_wakelock_callback()
{
	FUN_ENTER;
	acquire_wake_lock(PARTIAL_WAKE_LOCK, GPSNATIVETEST_WAKE_LOCK_NAME);
	FUN_EXIT;
}

static void release_wakelock_callback()
{
	FUN_ENTER;
	release_wake_lock(GPSNATIVETEST_WAKE_LOCK_NAME);
	FUN_EXIT;
}

static void request_utc_time_callback()
{
	FUN_ENTER;

	FUN_EXIT;
}
#ifdef PLATFORM_VERSION7
//------------------------------------------------------------------------------
void set_system_info_callback_func(const GnssSystemInfo* info)
{
	FUN_ENTER;

	FUN_EXIT;
}

//------------------------------------------------------------------------------
void gnss_sv_status_callback_func(GnssSvStatus* sv_info)
{
	FUN_ENTER;

	FUN_EXIT;
}
#endif
typedef void (*PFUN_GPS_THREAD_CALLBACK)(void *);
static PFUN_GPS_THREAD_CALLBACK gps_thread_callback = NULL;

static void * gps_thread_callback_stub( void * prm )
{
	FUN_ENTER;
	if( NULL != gps_thread_callback ) {
		gps_thread_callback(prm);
	}
	FUN_EXIT;
	return NULL;
}

pthread_t create_thread_callback(const char* name, void (*start)(void *), void* arg)
{
	int ret = 0;
	FUN_ENTER;
/*
	gps_thread_callback = start;

	pthread_t id = 0;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&id, &attr, gps_thread_callback_stub, arg);
	pthread_setname_np(id, name);
*/
  pthread_t pid;
  LOGD("%s called\n",__FUNCTION__);
  pthread_create(&pid, NULL,(void *)start, arg);

	FUN_EXIT;
	return pid;
}

int test_gps_pretest(void)
{
	int ret;
	if(sPreTest >= 1)
		ret= RL_PASS;
	else
		ret= RL_FAIL;

	save_result(CASE_TEST_GPS,ret);
	return ret;
}

int test_gps_start(void)
{
	int ret;
	pthread_t t1;
	int row = 2;

	ui_fill_locked();
	ui_show_title(MENU_TEST_GPS);
	ui_set_color(CL_WHITE);
	row = ui_show_text(row, 0, TEXT_WAIT_TIPS);
	gr_flip();

	ret = gpsOpen();
	if( ret < 0){
		LOGD("gps open failed ret = %d",ret);
		return -1;
	}
	gOpenTime=time(NULL);
	thread_run = 1;
	pthread_create(&t1, NULL, (void*)processThread_show, NULL);
	usleep(10*1000);
	ret = ui_handle_button(TEXT_PASS,NULL,TEXT_FAIL);//, TEXT_GOBACK
	thread_run = 0;
	pthread_join(t1,NULL);
	gpsClose();

	save_result(CASE_TEST_GPS,ret);
	return ret;
}


