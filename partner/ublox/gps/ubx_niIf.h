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
  \brief  Debug interface implementation

*/
/*******************************************************************************
 * $Id: ubx_niIf.h 60939 2012-08-10 15:29:44Z philippe.kehl $
 ******************************************************************************/
#ifndef __UBX_NIIF_H__
#define __UBX_NIIF_H__

#include "std_inc.h"
#include <semaphore.h>


class CNiIf
{
public:
	CNiIf();
    static const void* getIf(void) { return &s_interface; }
	static CNiIf* getInstance(void);
	
	// callbacks
#ifdef SUPL_ENABLED
	static void request(GpsNiNotification* pNotification);
#endif
    sem_t sem;

private:
	// interface
    static void init(GpsNiCallbacks* callbacks);
	static void respond(int notif_id, GpsUserResponseType user_response);

	// variables
    const static GpsNiInterface s_interface;
	GpsNiCallbacks m_callbacks;
	bool m_ready;
    pthread_t m_thread;
	
	//impelementation
	static void timoutThread(void *pThreadData);
};

#endif /* __UBX_DEBUGIF_H__ */
