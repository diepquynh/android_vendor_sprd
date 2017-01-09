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
 * Project: SUPL
 *
 ******************************************************************************/
/*!
  \file
  \brief  SET state Machine administrator interface

  Managing and allocating of the state machines for SUPL SET
*/
/*******************************************************************************
 * $Id: suplSMmanager.h 62713 2012-10-24 07:29:13Z jon.bowern $
 * $HeadURL: http://svn.u-blox.ch/GPS/SOFTWARE/hmw/android/trunk/gps/supl/suplSMmanager.h $
 ******************************************************************************/
#ifndef __SUPLSMMANAGER_H__
#define __SUPLSMMANAGER_H__

#include <unistd.h>
#include "rrlpmanager.h"

///////////////////////////////////////////////////////////////////////////////
// Types & Definitions

//! Commands to Supl state machine
typedef enum
{
	SUPL_NO_CMD,			//!< No command present. Use message input data to determine action
    SUPL_TIMEOUT,     	 	//!< Timeout has occurred
    SUPL_POS_AVAIL,      	//! The GPS position is available with the desired QOP
    SUPL_ASK_FOR_AGPS,  	//!< UI asking for ASGP using SUPL server: SET Init
    SUPL_AUTH_GRANT,     	//!< UI granting authorization for a NI session
    SUPL_AUTH_DENIED,    	//!< UI denying authorization for a NI session
	SUPL_MSA_DATA_AVAIL, 	//!< MS-ASSIST data available
	SUPL_NETWORK_CONNECTED,	//!< Network is now available
} SuplCmd_t;


///////////////////////////////////////////////////////////////////////////////
// Functions
void suplRegisterEventCallbacks(GpsControlEventInterface *pEventInterface, void* pContext);
int suplAddUplListeners(fd_set *pRfds, int *pMaxFd);
int suplReadUplSock(fd_set *pRfds);
void suplStartSetInitiatedAction(void);
void suplHandleNetworkInitiatedAction(char *buffer, int size);
void suplCheckPendingActions(void);
void suplCheckForSiAction(void);
void suplHandleAuthorization(int sid, SuplCmd_t cmd);
bool suplActiveSessions(void);

#endif /* __SUPLSMMANAGER_H__ */
