// 
// Spreadtrum Auto Tester
//
// anli   2013-01-23
//
#include "type.h"
#include "sensor.h"
#include <dlfcn.h>
#include <fcntl.h>
#include <string.h>

#include <binder/IServiceManager.h>
#include <gui/Sensor.h>
#include <gui/SensorManager.h>
#include <gui/SensorEventQueue.h>
#include <hardware/sensors.h>


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--namespace sci_snsr {
using namespace android;
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
static struct sensors_module_t      * sSensorModule = NULL;
static struct sensors_poll_device_t * sSensorDevice = NULL;
static struct sensor_t const        * sSensorList   = NULL;
static int                            sSensorCount  = 0;
static sp<SensorEventQueue>           sQue;
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int sensorOpen( void )
{
    FUN_ENTER;

	if( NULL != sSensorModule ) {
		AT_ASSERT( NULL != sSensorDevice );
		DBGMSG("already init!\n");
		return 0;
	}

	sp<IServiceManager> sm = defaultServiceManager();
/*
    LOGI("ServiceManager: %p\n", sm.get());

	Vector<String16> vls = sm->listServices();
	for( int i = 0; i < vls.size(); ++i ) {
		String8 sname(vls[i]);
		DBGMSG("service name = %s\n", sname.string());
	}
*/
	//sp<IBinder> ssrv = sm->checkService(String16("sensorservice"));
	sp<IBinder> ssrv = sm->checkService(String16("Sensor Service Test"));
	if( NULL != ssrv.get() ) {
		DBGMSG("sensorservice exist.\n");

		/*SensorManager& mgr(SensorManager::getInstance()); */
    SensorManager& mgr(SensorManager::getInstanceForPackage(String16("Sensor Service Test")));

		sQue = mgr.createEventQueue();
/*
		Sensor const* const* list = NULL;
		ssize_t count = mgr.getSensorList(&list);

		DBGMSG("numSensors = %d\n", int(count));
		for( ssize_t i = 0; ((sQue.get() != NULL) && (i < count)); ++i ) {
			DBGMSG("sensor name = %s\n", list[i]->getName().string());

			Sensor const* sensor = mgr.getDefaultSensor(list[i]->getType());
			if( NULL != sensor ) {
				sQue->disableSensor(sensor);
			}
		}
*/
	}

	int err = hw_get_module(SENSORS_HARDWARE_MODULE_ID,
						(hw_module_t const**)&sSensorModule);
    if( 0 == err ) {
        err = sensors_open(&sSensorModule->common, &sSensorDevice);
		if( err ) {
			ERRMSG("sensors_open error: %d\n", err);
			return -1;
		}

		sSensorCount = sSensorModule->get_sensors_list(sSensorModule, &sSensorList);
		DBGMSG("sensor count = %d\n", sSensorCount);
		for( int i = 0; i < sSensorCount; ++i ) {
			DBGMSG("sensor[%d]: name = %s, type = %d\n", i, sSensorList[i].name,
				sSensorList[i].type);

			//sSensorDevice->activate(sSensorDevice, sSensorList[i].handle, 0);
		}
	} else {
		ERRMSG("hw_get_module('%s') error: %d\n", SENSORS_HARDWARE_MODULE_ID, err);
        return -2;
	}

    FUN_EXIT;
    return 0;
}

//------------------------------------------------------------------------------
int sensorActivate( int type )
{
	FUN_ENTER;

	int ret = -1;
	int handle = -1;
	for( int i = 0; i < sSensorCount; ++i ) {
		if( type == sSensorList[i].type ) {
			handle = sSensorList[i].handle;
			break;
		}
	}

	if( handle >= 0 ) {
		ret = sSensorDevice->activate(sSensorDevice, handle, 1);
		sSensorDevice->activate(sSensorDevice, handle, 0);
		if( ret < 0 && sQue.get() != NULL ) {
			//SensorManager& mgr(SensorManager::getInstance());
      SensorManager& mgr(SensorManager::getInstanceForPackage(String16("Sensor Service Test")));

			Sensor const* sensor = mgr.getDefaultSensor(type);
			if( NULL != sensor ) {
				sQue->disableSensor(sensor);
				usleep(20 * 1000);
				ret = (NO_ERROR == sQue->enableSensor(sensor)) ? 0 : -1;
			}
		}

		INFMSG("Activate: type = %d, ret = %d\n", type, ret);
	}

	FUN_EXIT;
	return ret;
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int sensorClose( void )
{
    FUN_ENTER;

	if( NULL != sSensorModule ) {
		dlclose(sSensorModule->common.dso);

		sSensorModule = NULL;
		sSensorDevice = NULL;
	}

	if( NULL != sQue.get() ) {
		sQue.clear();
	}

    FUN_EXIT;
    return 0;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--} // namespace
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
