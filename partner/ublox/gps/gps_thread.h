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
 * $Id: gps_thread.h 62713 2012-10-24 07:29:13Z jon.bowern $
 ******************************************************************************/

#ifndef __GPS_THREAD_H__
#define __GPS_THREAD_H__

#include <pthread.h>
#include "ubx_moduleIf.h"

///////////////////////////////////////////////////////////////////////////////
// Definitions & Types

typedef  enum {
	CMD_NONE,
	CMD_START,
	CMD_STOP,
#ifndef ANDROID_BUILD
    CMD_EXIT
#endif    
} THREAD_CMDS;

typedef enum
{
	GPS_UNKNOWN,
	GPS_STARTED,
	GPS_STOPPING,
	GPS_STOPPED
} GPS_THREAD_STATES;

typedef struct {										//!< Main Gps (control) thread data structure containing
														//!< thread state & communication info
	GPS_THREAD_STATES       gpsState;					//!< Main state of Gps driver
	int                     cmdPipes[2];				//!< Handles to command communication pipes to main thread
	int                     cmdResult;					//!< Communication command result field
	pthread_cond_t          threadCmdCompleteCond;		//!< Mutex used in thread synchronisation
	pthread_mutex_t         threadCmdCompleteMutex;		//!< Condition used in thread synchronisation
	int64_t                 stoppingTimeoutMs;			//!< Timeout value when 'stopping'
	int						clientCount;				//!< Count of how many 'clients' are using the driver
														//!< Should the framework + number of NI sessions active
	pthread_mutex_t			threadDataAccessMutex;		//!< Mutex to control the access to data in this structure
} ControlThreadInfo;

typedef void (* requestStart)(void* pContext);		//!< Function prototype for 'Start' request event hander
typedef void (* requestStop)(void* pContext);		//!< Function prototype for 'Stop' request event handler

typedef struct 							//!< Event handler interface
{
    requestStart requestStart_cb;		//!< 'Start' request event handler
    requestStop requestStop_cb;			//!< 'Stop' request event handler
} GpsControlEventInterface;

///////////////////////////////////////////////////////////////////////////////
// API

void ubx_thread(void *pThreadData);

void gps_state_inject_time(GpsUtcTime timeUtcGps, int64_t timeReference, int uncertainty);
void gps_state_inject_location(double latitude, double longitude, float accuracy);
void gps_state_delete_aiding_data(GpsAidingData flags);
void gps_state_set_interval(uint32_t min_interval);
void gps_state_agps_injectData(const char* data, int length);

void controlThreadInfoInit(ControlThreadInfo* pControlThreadInfo);
void controlThreadInfoRelease(ControlThreadInfo* pControlThreadInfo);
bool controlThreadInfoSendCmd(ControlThreadInfo* pControlThreadInfo, THREAD_CMDS cmd);
void controlThreadInfoSetIF(ControlThreadInfo* pControlThreadInfo, CGpsIf* pInterface);

#ifndef UNDEBUG
extern pthread_t g_gpsDrvMainThread;
#endif

#endif /* __GPS_THREAD_H__ */
