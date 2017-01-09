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
  \brief  GPS global IF

*/
/*******************************************************************************
 * $Id: ubx_moduleIf.h 61086 2012-08-15 15:50:12Z jon.bowern $
 ******************************************************************************/

#ifndef __UBX_MODULEIF_H__
#define __UBX_MODULEIF_H__

#include "std_inc.h"

///////////////////////////////////////////////////////////////////////////////

class CGpsIf
{
public:
	CGpsIf();
	static struct hw_module_methods_t s_hwModuleMethods;
    static const GpsInterface *getIf(struct gps_device_t* /*dev*/) 
		{ return &s_interface; }
	
	static CGpsIf* getInstance(void);
	
	GpsPositionMode getMode(void) { return m_mode; };
	uint32_t getCapabilities(void) { return m_capabilities; };
	
	// operations
	static void gpsStatus(GpsStatusValue gpsStatusValue);
	static void nmea(const char* data, int length);
#if (PLATFORM_SDK_VERSION >= 14 /* =4.0 */)
	static void requestUtcTime(void);
#endif

private:
	// interface hw module 
	static int hwModuleOpen(const struct hw_module_t* module, 
							char const* name, struct hw_device_t** device);
	static int hwModuleClose(struct hw_device_t* device);
	
	// interface
 	static int  init(GpsCallbacks* callbacks);
	static int  start(void);
	static int  stop(void);
	static void cleanup(void);
	static int  injectTime(GpsUtcTime timeGpsUtc, 
				int64_t timeReference, int uncertainty);
	static int  injectLocation(double latitude, double longitude, float accuracy);
	static void deleteAidingData(GpsAidingData f);
	static int  setPositionMode(GpsPositionMode mode, GpsPositionRecurrence recurrence,
								uint32_t min_interval, uint32_t preferred_accuracy, uint32_t preferred_time);
	static const void* getExtension(const char* name);
	
	// variables	
    const static GpsInterface s_interface;
	bool m_ready;
	GpsPositionMode m_mode;
	GpsStatusValue m_lastStatusValue;
	uint32_t m_capabilities;
	
public:
	GpsCallbacks m_callbacks;
};

#endif /* __UBX_MODULEIF_H__ */
