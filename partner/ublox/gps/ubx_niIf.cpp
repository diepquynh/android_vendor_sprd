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
 * $Id: ubx_niIf.cpp 61083 2012-08-15 14:51:49Z michael.ammann $
 ******************************************************************************/

#include "ubx_niIf.h"

#ifdef SUPL_ENABLED
#include "suplSMmanager.h"
#endif

static CNiIf s_myIf;

const GpsNiInterface CNiIf::s_interface = { 
	size:           sizeof(GpsNiInterface),
	init:           init,
	respond:        respond,
};
 
CNiIf::CNiIf() 
{ 
	m_ready = false; 
	m_thread = (pthread_t)NULL; 
    sem_init(&sem, 0, 0);
} 

CNiIf* CNiIf::getInstance()
{
	return &s_myIf;
}

void CNiIf::init(GpsNiCallbacks* callbacks)
{
    if (s_myIf.m_ready)
        LOGE("CNiIf::%s : already initialized", __FUNCTION__);
    LOGV("CNiIf::%s :", __FUNCTION__);
	s_myIf.m_callbacks = *callbacks;
	s_myIf.m_ready = true;
}

void CNiIf::respond(int notif_id, GpsUserResponseType user_response)
{
    LOGV("CNiIf::%s : id=%d respond=%d(%s)", __FUNCTION__, 
		notif_id, user_response, _LOOKUPSTR(user_response, GpsUserResponseType));

#ifdef SUPL_ENABLED	
	SuplCmd_t cmd = SUPL_AUTH_DENIED;
	switch(user_response)
	{
	case GPS_NI_RESPONSE_ACCEPT:
		cmd = SUPL_AUTH_GRANT;
		break;
		
	case GPS_NI_RESPONSE_DENY:
		cmd = SUPL_AUTH_DENIED;
		break;
	
	case GPS_NI_RESPONSE_NORESP:
		break;
		
	default:
		break;
	}
	
	suplHandleAuthorization(notif_id, cmd);

    // Produce the semaphore
    sem_post(&s_myIf.sem);
#endif	
}


void CNiIf::timoutThread(void *pThreadData)
{
	LOGV("CNiIf::%s : ", __FUNCTION__);
	

#if 0
	while (1)
	{
		/* wakes up every second to check timed out requests */
		sleep(1);
		
		//pthread_mutex_lock(&loc_eng_ni_data.loc_ni_lock);
		if (not.timeout > 0)
		{
			not.timeout--;
			LOGD("%s : timeout=%d", __FUNCTION__, not.timeout);
			if (not.timeout == 0)
			{
				respond(not.notification_id, GPS_NI_RESPONSE_NORESP);
				pthread_exit(NULL);
			}    	
		}
		//pthread_mutex_unlock(&loc_eng_ni_data.loc_ni_lock);
	} /* while (1) */
#endif
}

#ifdef SUPL_ENABLED
void CNiIf::request(GpsNiNotification* pNotification)
{
    if (!s_myIf.m_ready)
    {
        LOGE("CNiIf::%s: class not initialized", __FUNCTION__);
        return;
    }
    LOGV("CNiIf::%s", __FUNCTION__);

//	if (!m_thread)
//		m_thread = s_myIf.m_callbacks.create_thread_cb("ni thread", CNiIf::timoutThread, NULL);

	s_myIf.m_callbacks.notify_cb(pNotification);
}

#endif


