/*******************************************************************************
 *
 * Copyright (C) u-blox ag
 * u-blox ag, Thalwil, Switzerland
 *
 * All rights reserved.
 *
 * This  source  file  is  the  sole  property  of  u-blox  AG. Reproduction or
 * utilization of this source in whole or part is forbidden without the written 
 * consent of u-blox AG.
 *
 *******************************************************************************
 *
 * Project: PE_AND
 *
 ******************************************************************************/
/*!
  \file
  \brief  GPS main function

  Module for framework interface definition
*/
/*******************************************************************************
 * $Id: ubx_moduleIf.cpp 61826 2012-09-11 15:10:48Z jon.bowern $
 ******************************************************************************/

#include <errno.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#ifndef ANDROID_BUILD
// Needed for Linux build
#include <malloc.h>
#endif

#include "std_types.h"
#include "ubx_log.h"

#include "ubx_debugIf.h"
#include "ubx_rilIf.h"
#include "ubx_niIf.h"
#include "ubx_xtraIf.h"
#include "ubx_agpsIf.h"
#include "ubx_moduleIf.h"

#include "gps_thread.h"

static ControlThreadInfo  s_controlThreadInfo;
static pthread_t s_mainControlThread = (pthread_t)NULL;

/*******************************************************************************
 * HAL MODULE
 ******************************************************************************/

hw_module_t HAL_MODULE_INFO_SYM = { // hardware/libhardware/include/hardware/hardware.h
	tag:					HARDWARE_MODULE_TAG,        // uint32_t
    version_major:			2,                          // uint16_t
    version_minor:			0,                          // uint16_t
    id:						GPS_HARDWARE_MODULE_ID,     // const char *
    name:					"u-blox GPS/GNSS library",  // const char *
    author:					"u-blox AG - Switzerland",  // const char *
    methods:				&CGpsIf::s_hwModuleMethods, // struct hw_module_methods_t *
    dso:					NULL,                       // module's dso
    reserved:				{0}                         // uint32_t *, padding
};

struct hw_module_methods_t CGpsIf::s_hwModuleMethods =
{
    open: CGpsIf::hwModuleOpen // open a specific device
};

int CGpsIf::hwModuleOpen(const struct hw_module_t* module, 
						 char const* name, struct hw_device_t** device)
{
    struct gps_device_t *dev = new gps_device_t;
    memset(dev, 0, sizeof(*dev));
    dev->common.tag			= HARDWARE_DEVICE_TAG;
    dev->common.version		= 0;
    dev->common.module		= (struct hw_module_t*) module;
    dev->common.close		= CGpsIf::hwModuleClose;
    dev->get_gps_interface	= CGpsIf::getIf;
    *device = (struct hw_device_t*) dev;

    return 0;
}

int CGpsIf::hwModuleClose(struct hw_device_t* device)
{
    delete device;
    return 0;
}

/*******************************************************************************
 * INTERFACE
 ******************************************************************************/

static CGpsIf s_myIf;

const GpsInterface CGpsIf::s_interface = {
    size:                   sizeof(GpsInterface),
    init:                   CGpsIf::init,
    start:                  CGpsIf::start,
    stop:                   CGpsIf::stop,
    cleanup:                CGpsIf::cleanup,
    inject_time:            CGpsIf::injectTime,
    inject_location:		CGpsIf::injectLocation,
    delete_aiding_data:		CGpsIf::deleteAidingData,
    set_position_mode:		CGpsIf::setPositionMode,
    get_extension:			CGpsIf::getExtension,
};

CGpsIf::CGpsIf()
{
	m_ready = false;
	m_mode = GPS_POSITION_MODE_MS_BASED;
	m_lastStatusValue = GPS_STATUS_NONE;
}

CGpsIf* CGpsIf::getInstance()
{
	return &s_myIf;
}

int CGpsIf::init(GpsCallbacks* callbacks)
{
    if (s_myIf.m_ready)
	{
        LOGE("CGpsIf::%s : already initialized", __FUNCTION__);
		return 0;	// Report success since we are already initialised
	}
	
    LOGV("CGpsIf::%s :", __FUNCTION__);
	s_myIf.m_callbacks = *callbacks;

	controlThreadInfoInit(&s_controlThreadInfo);
    
	LOGD("CGpsIf::%s (%u): Initializing - pid %i", __FUNCTION__, (unsigned int) pthread_self(), getpid());
	
	s_myIf.m_capabilities = GPS_CAPABILITY_SCHEDULING;
#if (PLATFORM_SDK_VERSION >= 14 /* =4.0 */)
	s_myIf.m_capabilities |= GPS_CAPABILITY_ON_DEMAND_TIME;
#endif
#ifdef SUPL_ENABLED
	s_myIf.m_capabilities |=  GPS_CAPABILITY_MSB | GPS_CAPABILITY_MSA;
#endif
	LOGV("CGpsIf::%s : set_capabilities=%d(%s)", __FUNCTION__, s_myIf.m_capabilities, _LOOKUPSTRX(s_myIf.m_capabilities, GpsCapabilityFlags));
	s_myIf.m_callbacks.set_capabilities_cb(s_myIf.m_capabilities);
	
    s_mainControlThread = s_myIf.m_callbacks.create_thread_cb("gps thread", ubx_thread, &s_controlThreadInfo);
    pthread_cond_wait(&s_controlThreadInfo.threadCmdCompleteCond,
                      &s_controlThreadInfo.threadCmdCompleteMutex);
    s_myIf.m_ready = s_controlThreadInfo.cmdResult;
	
	LOGD("CGpsIf::%s Initialized complete: result %i", __FUNCTION__, s_myIf.m_ready);
    if (!s_myIf.m_ready)
    {
        // Init failed -  release resources
		controlThreadInfoRelease(&s_controlThreadInfo);
    }
	gpsStatus(GPS_STATUS_ENGINE_OFF);
	return s_myIf.m_ready  ? 0 : 1;
}

int CGpsIf::start(void)
{
    LOGV("CGpsIf::%s (%u):", __FUNCTION__, (unsigned int) pthread_self());
	
	if (s_myIf.m_ready)
	{
		return controlThreadInfoSendCmd(&s_controlThreadInfo, CMD_START) ? 0 : 1;
	}

	LOGE("CGpsIf::%s : Not initialised", __FUNCTION__);
	return 1;
}

int CGpsIf::stop(void)
{
    LOGV("CGpsIf::%s (%u):", __FUNCTION__, (unsigned int) pthread_self());
	
	if (s_myIf.m_ready)
	{
		return controlThreadInfoSendCmd(&s_controlThreadInfo, CMD_STOP) ? 0 : 1;
	}

	LOGE("CGpsIf::%s : Not initialised", __FUNCTION__);
	return 1;
}

void CGpsIf::cleanup(void)
{
    LOGD("CGpsIf::%s (%u):", __FUNCTION__, (unsigned int) pthread_self()); 
	
	if (s_myIf.m_ready)
	{
		controlThreadInfoSendCmd(&s_controlThreadInfo, CMD_STOP);
	}
	else
	{
		LOGE("CGpsIf::%s : Not initialised", __FUNCTION__);
	}
}

int CGpsIf::injectTime(GpsUtcTime timeGpsUtc, int64_t timeReference, int uncertainty)
{
	time_t tUtc = timeGpsUtc/1000;
	char s[20];
	struct tm t;
	strftime(s, 20, "%Y.%m.%d %H:%M:%S", gmtime_r(&tUtc, &t));
	
	LOGV("CGpsIf::%s : timeGpsUtc=%s.%03d timeReference=%lli uncertainty=%.3f ms", 
				__FUNCTION__, s, (int)(timeGpsUtc%1000), timeReference,uncertainty*0.001);
	
	if (s_myIf.m_ready)
	{
		gps_state_inject_time(timeGpsUtc, timeReference, uncertainty);
		return 0;
	}

	LOGE("CGpsIf::%s : Not initialised", __FUNCTION__);
    return 1;
}

int CGpsIf::injectLocation(double latitude, double longitude, float accuracy)
{
    LOGV("CGpsIf::%s : latitude=%.6f longitude=%.6f accuracy=%.2f", 
				__FUNCTION__, latitude, longitude, accuracy);
	if (s_myIf.m_ready)
	{
		gps_state_inject_location(latitude, longitude, accuracy);
		return 0;
	}
	
	LOGE("CGpsIf::%s : Not initialised", __FUNCTION__);
    return 1;
}

void CGpsIf::deleteAidingData(GpsAidingData flags)
{
    LOGD("CGpsIf::%s : flags=0x%X", __FUNCTION__, flags);
	if (s_myIf.m_ready)
	{
		gps_state_delete_aiding_data(flags);
	}
	else
	{
		LOGE("CGpsIf::%s : Not initialised", __FUNCTION__);
	}	
}

int CGpsIf::setPositionMode(GpsPositionMode mode, GpsPositionRecurrence recurrence,
			uint32_t min_interval, uint32_t preferred_accuracy, uint32_t preferred_time)
{
	LOGD("CGpsIf::%s (%u): mode=%i(%s) recurrence=%i(%s) min_interval=%u preferred_accuracy=%u preferred_time=%u", 
			__FUNCTION__,
			(unsigned int) pthread_self(),
            mode, _LOOKUPSTR(mode, GpsPositionMode),
            recurrence, _LOOKUPSTR(recurrence, GpsPositionRecurrence),
            min_interval,
            preferred_accuracy,
            preferred_time); 
	if (s_myIf.m_ready)
	{
		s_myIf.m_mode = mode;
		
		min_interval = min_interval ? min_interval : 1000;
		gps_state_set_interval(min_interval);
	}
	else
	{
		LOGE("CGpsIf::%s : Not initialised", __FUNCTION__);
	}
	
    return 0;
}

const void* CGpsIf::getExtension(const char* name)
{
    LOGD("%s : name='%s'", __FUNCTION__, name); 
	if (!strcmp(name, AGPS_INTERFACE))		return CAgpsIf::getIf();
    if (!strcmp(name, AGPS_RIL_INTERFACE))	return CRilIf::getIf();
    if (!strcmp(name, GPS_DEBUG_INTERFACE))	return CDebugIf::getIf();
    if (!strcmp(name, GPS_NI_INTERFACE))	return CNiIf::getIf();
    if (!strcmp(name, GPS_XTRA_INTERFACE))	return CXtraIf::getIf();
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// operations

void CGpsIf::gpsStatus(GpsStatusValue gpsStatusValue)
{
	if (!s_myIf.m_ready)
    {
        LOGE("CGpsIf::%s: class not initialized", __FUNCTION__);
        return;
    }
	LOGV("CGpsIf::%s: gpsStatusValue=%d(%s)", __FUNCTION__, 
				gpsStatusValue, _LOOKUPSTR(gpsStatusValue, GpsStatusValue));
	if (gpsStatusValue != s_myIf.m_lastStatusValue)
	{
		s_myIf.m_lastStatusValue = gpsStatusValue;
		if (gpsStatusValue == GPS_STATUS_SESSION_END)
		{
			GpsSvStatus svStatus;
			memset(&svStatus, 0, sizeof(GpsSvStatus));
			svStatus.size = sizeof(GpsSvStatus);
			s_myIf.m_callbacks.sv_status_cb(&svStatus);
		}
		GpsStatus gpsStatus;
		gpsStatus.size = sizeof(gpsStatus);
		gpsStatus.status = gpsStatusValue;
		s_myIf.m_callbacks.status_cb(&gpsStatus);
	}
}

void CGpsIf::nmea(const char* data, int length)
{
	if (!s_myIf.m_ready)
    {
        LOGE("CGpsIf::%s: class not initialized", __FUNCTION__);
        return;
    }
	struct timeval tv;
    gettimeofday(&tv, NULL);
    GpsUtcTime gpsUtcTime = tv.tv_sec*1000+tv.tv_usec/1000;
    s_myIf.m_callbacks.nmea_cb(gpsUtcTime, data, length);
}

#if (PLATFORM_SDK_VERSION >= 14 /* =4.0 */)
void CGpsIf::requestUtcTime(void)
{
	if (!s_myIf.m_ready)
    {
        LOGE("CGpsIf::%s: class not initialized", __FUNCTION__);
        return;
    }
	LOGV("CGpsIf::%s : ", __FUNCTION__);
	s_myIf.m_callbacks.request_utc_time_cb();
}
#endif


///////////////////////////////////////////////////////////////////////////////
// Debug / Testing support

#ifndef ANDROID_BUILD

extern "C" void endControlThread(void)
{
    LOGD("CGpsIf::%s : Send thread exit command", __FUNCTION__);
    bool ok = controlThreadInfoSendCmd(&s_controlThreadInfo, CMD_EXIT);
    pthread_join(s_mainControlThread, NULL);
	s_mainControlThread = NULL;
    LOGD("CGpsIf::%s : Thread exited ok=%d", __FUNCTION__, ok);
}
#endif

