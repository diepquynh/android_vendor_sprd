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
 * $Id: ubx_rilIf.h 61478 2012-08-29 10:51:14Z jon.bowern $
 ******************************************************************************/
#ifndef __UBX_RILIF_H__
#define __UBX_RILIF_H__

#include <arpa/inet.h>
#include <semaphore.h>

#include "std_inc.h"

///////////////////////////////////////////////////////////////////////////////

#define SETID_MAX_SIZE 30			//!< Size of Phone ID strings
#define REQUEST_REF_LOC_TIMEOUT	2	//!< Timeout (in seconds) to wait on request location ref semaphore

///////////////////////////////////////////////////////////////////////////////

class CRilIf
{
public:
    CRilIf();
    static const void* getIf(void) { return &s_interface; }
	static CRilIf* getInstance(void);
	
	const AGpsRefLocation* getRefLocation(void) { return &m_refLoc; };
	const char* getIsmi(void) { return m_setidImsi; };
	const char* getMsisdn(void) { return m_setidMsisdn; };
	struct in_addr getClientIP(void);
	void setClientIp(const char *pIpAddress);
	void requestCellInfo(void);
	bool isSimPresent(void);
	bool isConnected(void);
	bool isAvailable(void) { return m_available > 0; };

public:
    sem_t sem;					//!< Semaphore allowing calls to framework 'request' functions to 
								//!< to synchronise with framework 'set' callback from framework 
	
private:
    // Interface
	static void init(AGpsRilCallbacks* callbacks);
    static void setRefLocation(const AGpsRefLocation *agps_reflocation, size_t sz_struct);
    static void setSetId(AGpsSetIDType type, const char* setid);
    static void niMessage(uint8_t *msg, size_t len);
    static void updateNetworkState(int connected, int type, int roaming, const char* extra_info);
#if (PLATFORM_SDK_VERSION >= 14 /* =4.0 */)
    static void updateNetworkAvailability(int avaiable, const char* apn);
#endif

	// Helpers
	static in_addr readIpAddress(const char* pDeviceName);
	void requestSetId(uint32_t flags);
    void requestRefLoc(uint32_t flags);
	
private:
	// Variables
	static const AGpsRilInterface s_interface;		//!< Jump table of functions implementing the RIL interface to the driver
    AGpsRilCallbacks m_callbacks;					//!< Jump table of RIL API functions provided by framework
	bool m_ready;									//!< RIL interface initialised flag
	
	AGpsRefLocation m_refLoc;						//!< Reference location info from framework
	char m_setidMsisdn[SETID_MAX_SIZE];				//!< MSISDN ID info from framework
	char m_setidImsi[SETID_MAX_SIZE];				//!< IMSI ID info from framework
	int m_networkType;								//!< Connected network type - Told to us by framework
	int m_available;								//!< Allowed to use network flag
 };

#endif /* __UBX_RILIF_H__ */

