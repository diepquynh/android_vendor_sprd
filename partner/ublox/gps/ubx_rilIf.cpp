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
  \brief  RIL interface implementation

*/
/*******************************************************************************
 * $Id: ubx_rilIf.cpp 62156 2012-09-27 09:27:57Z jon.bowern $
 ******************************************************************************/

#include "ubx_rilIf.h"
#include "string.h"
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>

#include "ubxgpsstate.h"

#ifdef SUPL_ENABLED
#include "suplSMmanager.h"

#endif

///////////////////////////////////////////////////////////////////////////////
// Definitions & Types
#define RIL_NO_NETWORK	(-1)

///////////////////////////////////////////////////////////////////////////////
// Static data
static CRilIf s_myIf;	//!< Private instance of the CRilIf class - 'singleton' 

 
const AGpsRilInterface CRilIf::s_interface = 		//!< RIL interface jump table
{
    size:                           sizeof(AGpsRilInterface),
    init:                           CRilIf::init,
    set_ref_location:               CRilIf::setRefLocation,
    set_set_id:                     CRilIf::setSetId,
    ni_message:                     CRilIf::niMessage,
    update_network_state:           CRilIf::updateNetworkState,
#if (PLATFORM_SDK_VERSION >= 14 /* =4.0 */)
    update_network_availability:    CRilIf::updateNetworkAvailability,
#endif
};

///////////////////////////////////////////////////////////////////////////////
//! Constructor for CRilIf class
CRilIf::CRilIf() 
{ 
	m_ready = false; 
	memset(&m_refLoc, 0, sizeof(m_refLoc));
	strcpy(m_setidImsi, "");
	strcpy(m_setidMsisdn, "");
	m_networkType = RIL_NO_NETWORK;
    sem_init(&sem, 0, 0);
#if (PLATFORM_SDK_VERSION >= 14 /* =4.0 */)
	m_available = 0;
#else
	m_available = 1;			// Always true for Android < 4.0
#endif
} 

///////////////////////////////////////////////////////////////////////////////
//! Retrieve singleton class instance
/*!
  \return	: Pointer to singleton CRilIf class instance
*/
CRilIf* CRilIf::getInstance()
{
	return &s_myIf;
}

///////////////////////////////////////////////////////////////////////////////
//! RIL interface 'init' function implementation
/*! Framework calls this function on initialisation of the gps driver
  \param callbacks	: Pointer to jump table implementing RIM API into framework
*/
void CRilIf::init(AGpsRilCallbacks* callbacks)
{
    if (s_myIf.m_ready)
        LOGE("CRilIf::%s : already initialized", __FUNCTION__);
    LOGV("CRilIf::%s :", __FUNCTION__);
    s_myIf.m_callbacks = *callbacks;
	s_myIf.m_ready = true;
}

///////////////////////////////////////////////////////////////////////////////
//! RIL interface 'set_ref_location' implementation
/*! Framework calls this function when it has been instructed to give the gps 
    driver reference location information. Usually this when the driver has called
    the function 'request_refloc' on the AGPS framework interface.
  \param agps_reflocation	: Pointer to a framework defined agps reference 
                              location data structure
  \param sz_struct			: Size of the passed structure
*/
void CRilIf::setRefLocation(const AGpsRefLocation *agps_reflocation, size_t sz_struct)
{
	if (agps_reflocation->type == AGPS_REG_LOCATION_TYPE_MAC)
		LOGV("CRilIf::%s : size=%d type=%d(%s) -> mac=%02X-%02X-%02X-%02X-%02X-%02X", __FUNCTION__, 
				sz_struct, agps_reflocation->type, _LOOKUPSTR(agps_reflocation->type, AGpsRefLocation),
				agps_reflocation->u.mac.mac[0], agps_reflocation->u.mac.mac[1], 
				agps_reflocation->u.mac.mac[2], agps_reflocation->u.mac.mac[3], 
				agps_reflocation->u.mac.mac[4], agps_reflocation->u.mac.mac[5]);
	else if ((agps_reflocation->type == AGPS_REF_LOCATION_TYPE_GSM_CELLID) ||
			 (agps_reflocation->type == AGPS_REF_LOCATION_TYPE_UMTS_CELLID))
		LOGV("CRilIf::%s : size=%d type=%d(%s) -> type=%d mcc=%d mnc=%d lac=%d cid=%d", __FUNCTION__, 
				sz_struct, agps_reflocation->type, _LOOKUPSTR(agps_reflocation->type, AGpsRefLocation),
				agps_reflocation->u.cellID.type, // random, not filled by framework (random)
				agps_reflocation->u.cellID.mcc, agps_reflocation->u.cellID.mnc, 
				agps_reflocation->u.cellID.lac, // in 3g lac is discarded 
				agps_reflocation->u.cellID.cid);
	else
		LOGV("CRilIf::%s : size=%d type=%d(%s)", __FUNCTION__, 
				sz_struct, agps_reflocation->type, _LOOKUPSTR(agps_reflocation->type, AGpsRefLocation));
				
	s_myIf.m_refLoc = *agps_reflocation;	// make a copy

    // Produce the semaphore
    sem_post(&s_myIf.sem);
}

///////////////////////////////////////////////////////////////////////////////
//! RIL interface 'set_set_id' implementation
/*! Framework calls this function when it has been instructed to give the gps 
    driver ID information. Usually this when the driver has called
    the function 'request_setid' on the AGPS framework interface.
  \param type	: Identifier type
  \param setid	: Pointer to test string identifier
*/
void CRilIf::setSetId(AGpsSetIDType type, const char* setid)
{
	if (!setid) setid = "";
    LOGV("CRilIf::%s : type=%d(%s) setid='%s'", __FUNCTION__, type, _LOOKUPSTR(type, AGpsSetIDType), setid);
	if (type == AGPS_SETID_TYPE_NONE)
	{
		// Do nothing
	}
	else
	{
		size_t len = strlen(setid);
		if (len > SETID_MAX_SIZE - 1)
		{
			LOGE("%s: Supplied setid too big '%s' (%i)", __FUNCTION__, setid, len);
		}
		else if (type == AGPS_SETID_TYPE_IMSI)
		{
			strncpy(s_myIf.m_setidImsi, setid, SETID_MAX_SIZE);
		}
		else if (type == AGPS_SETID_TYPE_MSISDN)
		{
			strncpy(s_myIf.m_setidMsisdn, setid, SETID_MAX_SIZE);
		}
		else
		{
			LOGE("%s: Unknown setid type %d '%s'", __FUNCTION__, type, setid);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
//! RIL interface 'ni_message' implementation
/*! Framework calls this function when nit wants to pass a Network Initiated
    message to the Gps driver. The driver processes this as a Supl initiation.
  \param 	: Pointer to received message buffer
  \param 	: Size of buffer
*/
void CRilIf::niMessage(uint8_t *msg, size_t len)
{
    LOGV("CRilIf::%s msg len %i:", __FUNCTION__, len);
#ifdef SUPL_ENABLED	
	// Assumption is that the msg from the network is a SUPL message
	suplHandleNetworkInitiatedAction((char*) msg, len);
#endif
}

///////////////////////////////////////////////////////////////////////////////
//! RIL interface 'updateNetworkState' implementation
/*! The framework calls this function when it wants to report the networking 
    state of the platform
  \param connected	: 1 if connected to a network, 0 if not.
  \param type		: Type of network.
  \param roaming	: 1 if on a roaming network, 0 if not.
  \param extra_info	: Pointer to extra info buffer.
*/
void CRilIf::updateNetworkState(int connected, int type, int roaming, const char* extra_info)
{
	LOGV("CRilIf::%s : connected=%d type=%d(%s) roaming=%d extra_info='%s'", __FUNCTION__,
         connected,
         type, _LOOKUPSTR(type, AgpsRilNetworkType),
         roaming,
         extra_info
        );

	s_myIf.m_networkType = connected ? type : RIL_NO_NETWORK;
}

#if (PLATFORM_SDK_VERSION >= 14 /* =4.0 */)
///////////////////////////////////////////////////////////////////////////////
//! RIL interface 'update_network_state' implementation
/*! Framework call this function when it wants to report additional information
    regarding the networking state of the platform
  \param avaiable	: 0 if network connection can not be used, 1 if it can
  \param apn		: Pointer to string containing Access Point Name
*/
void CRilIf::updateNetworkAvailability(int avaiable, const char* apn)
{
	LOGV("CRilIf::%s : avaiable=%d apn='%s'", __FUNCTION__,
         avaiable,
         apn
        );
	s_myIf.m_available = avaiable;
}
#endif

///////////////////////////////////////////////////////////////////////////////
//! Requests ID information from the framework
/*! Handles requests to call the framework to request ID information
    Implements 'phone faking' to set pretend ID parameters directly and avoids
    calling the framework which won't yield anything useful.
  \param setid	: Type of ID to request from framework
*/
void CRilIf::requestSetId(uint32_t flags)
{
    if (!m_ready)
    {
        LOGE("CRilIf::%s : class not initialized", __FUNCTION__);
        return;
    }
    LOGV("CRilIf::%s : flags=%d(%s)", __FUNCTION__, flags, _LOOKUPSTRX(flags,AgpsRilRequestSetId));

#ifdef SUPL_ENABLED	
	if (CUbxGpsState::getInstance()->getFakePhone())
	{
		// Phone fake - Sets up ID parameters without calling framework
		LOGV("CRilIf::%s : Faking phone ID", __FUNCTION__);
		const char* setid = "";
		AGpsSetIDType type = AGPS_SETID_TYPE_NONE;
		if ((flags & AGPS_RIL_REQUEST_SETID_IMSI) == AGPS_RIL_REQUEST_SETID_IMSI)
		{
			type = AGPS_SETID_TYPE_IMSI;
#if 0
			setid = "2280215121973630";
#else
			setid = "460001831429979";
#endif
		}
		else if ((flags & AGPS_RIL_REQUEST_SETID_MSISDN) == AGPS_RIL_REQUEST_SETID_MSISDN)
		{
			type = AGPS_SETID_TYPE_MSISDN;
			setid = "380561234567";
		}
		setSetId(type, setid);
	}
	else
#endif
		m_callbacks.request_setid(flags);
}

///////////////////////////////////////////////////////////////////////////////
//! Request location reference information from the framework
/*! Handles requests to call the framework to request reference location information
    Implements 'phone faking' to set pretend ref location  parameters directly and avoids
    calling the framework which won't yield anything useful.
  \param flags	: Type of location information to request from the framework
*/
void CRilIf::requestRefLoc(uint32_t flags)
{
    if (!m_ready) 
    {
        LOGE("CRilIf::%s : class not initialized", __FUNCTION__);
        return;
    }
	LOGV("CRilIf::%s : flags=%d(%s)", __FUNCTION__, flags, _LOOKUPSTRX(flags,AgpsRilRequestRefLoc));
	
#ifdef SUPL_ENABLED	
	if (CUbxGpsState::getInstance()->getFakePhone())
	{
		// Phone fake - Sets up reference location parameters without calling framework
		AGpsRefLocation refLoc;
		memset(&refLoc, 0, sizeof(refLoc));
		if ((flags & AGPS_RIL_REQUEST_REFLOC_CELLID) == AGPS_RIL_REQUEST_REFLOC_CELLID)
		{
#if 0
			refLoc.type = AGPS_REF_LOCATION_TYPE_GSM_CELLID;
			refLoc.u.cellID.type = 0/*random*/;
			refLoc.u.cellID.mcc = 230;
			refLoc.u.cellID.mnc = 120;
			refLoc.u.cellID.lac = 99;
			refLoc.u.cellID.cid = 123;
#elif 0
			// spreadtrum phone / shanghai
			refLoc.type = AGPS_REF_LOCATION_TYPE_UMTS_CELLID;
			refLoc.u.cellID.type = 0/*random*/;
			refLoc.u.cellID.mcc = 460;
			refLoc.u.cellID.mnc  = 0;
			refLoc.u.cellID.lac  = 0;
			refLoc.u.cellID.cid = 545;
#else
			// switzerland
			refLoc.type = AGPS_REF_LOCATION_TYPE_GSM_CELLID;
			refLoc.u.cellID.type = 0/*random*/;
			refLoc.u.cellID.mcc = 228;
			refLoc.u.cellID.mnc = 1;
			refLoc.u.cellID.lac = 1010;
			refLoc.u.cellID.cid = 20777;
			
			// Reigate (2G) - UK
//			refLoc.type = AGPS_REF_LOCATION_TYPE_GSM_CELLID;
//			refLoc.u.cellID.type = 0/*random*/;
//			refLoc.u.cellID.mcc = 234;
//			refLoc.u.cellID.mnc = 30;
//			refLoc.u.cellID.lac = 682;
//			refLoc.u.cellID.cid = 3612;

			// Reigate alternative (2G) - UK
//			refLoc.type = AGPS_REF_LOCATION_TYPE_GSM_CELLID;
//			refLoc.u.cellID.type = 0/*random*/;
//			refLoc.u.cellID.mcc = 234;
//			refLoc.u.cellID.mnc = 15;
//			refLoc.u.cellID.lac = 142;
//			refLoc.u.cellID.cid = 24946;

			// Reigate (3G) - UK
//			refLoc.type = AGPS_REF_LOCATION_TYPE_UMTS_CELLID;
//			refLoc.u.cellID.type = 0/*random*/;
//			refLoc.u.cellID.mcc = 234;
//			refLoc.u.cellID.mnc = 30;
//			refLoc.u.cellID.lac = 0;
//			refLoc.u.cellID.cid = 9243701;
#endif
		}
		else if ((flags & AGPS_RIL_REQUEST_REFLOC_MAC) == AGPS_RIL_REQUEST_REFLOC_MAC)
		{
			// refLoc.type = AGPS_REG_LOCATION_TYPE_MAC;
			// refLoc.u.mac.mac[0] = 
			// refLoc.u.mac.mac[1] = 
			// refLoc.u.mac.mac[2] = 
			// refLoc.u.mac.mac[3] = 
			// refLoc.u.mac.mac[4] = 
			// refLoc.u.mac.mac[6] = 
		}
		setRefLocation(&refLoc, sizeof(refLoc));
	}
	else
#endif	
		m_callbacks.request_refloc(flags);
}

///////////////////////////////////////////////////////////////////////////////
//! Retrieves the IP address of this platform
/*! depending on the type of data network platform is connected to, retrieves
    the IP address of the corresponding networking adaptor
  \return	: IP address of this platform
*/
in_addr CRilIf::getClientIP(void)
{
	struct in_addr addr;
	memset(&addr, 0, sizeof(addr));
	
	switch(m_networkType)
	{
	case RIL_NO_NETWORK:
		addr = readIpAddress("eth0");
		break;
		
	case AGPS_RIL_NETWORK_TYPE_WIFI:
		addr = readIpAddress("wlan0");
		break;
	
	default:
		break;
	}

	LOGV("%s: %s (Type = %i)", __FUNCTION__, inet_ntoa(addr), m_networkType);
	
	return addr;
}	

///////////////////////////////////////////////////////////////////////////////
//! Retrieves the IP address assigned to a newtorking device
/*!
  \param pDeviceName	: Pointer to the name of the network device
  \return				: IP address of device
*/
in_addr CRilIf::readIpAddress(const char* pDeviceName)
{
	 int fd;
	 struct ifreq ifr;

	 memset(&ifr, 0, sizeof(ifr));
	 fd = socket(AF_INET, SOCK_DGRAM, 0);

	 /* I want to get an IPv4 IP address */
	 ifr.ifr_addr.sa_family = AF_INET;

	 /* I want IP address for device */
	 strncpy(ifr.ifr_name, pDeviceName, IFNAMSIZ - 1);
	 ioctl(fd, SIOCGIFADDR, &ifr);
	 close(fd);
	 
	 return ((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr;
}

///////////////////////////////////////////////////////////////////////////////
//! Collect cell information
/*! Initiates calls to framework to retrieve all cell information

*/
void CRilIf::requestCellInfo(void)
{
	requestRefLoc(AGPS_RIL_REQUEST_REFLOC_MAC | AGPS_RIL_REQUEST_REFLOC_CELLID);

    // Be sure the reference location is got...
	struct timespec timeOut;
	timeOut.tv_sec = time(NULL) + REQUEST_REF_LOC_TIMEOUT;
	timeOut.tv_nsec = 0;
	
	sem_timedwait(&sem, &timeOut);	// Timeout needed because 'request' may not
									// cause corresponding 'set' if error occurs

	// First clear existing data as platform my have had SIM removed
	strcpy(m_setidImsi, "");
	strcpy(m_setidMsisdn, "");
	
	// Now request current cell info
	requestSetId(AGPS_RIL_REQUEST_SETID_IMSI);
	requestSetId(AGPS_RIL_REQUEST_SETID_MSISDN);
}

///////////////////////////////////////////////////////////////////////////////
//! Determine state of connection to data network
/*!
  \return	: true if connected to some kind of network, false if not
*/
bool CRilIf::isConnected(void)
{
	if (isAvailable())
	{
		return (m_networkType != RIL_NO_NETWORK);
	}
	
	return false;
}

///////////////////////////////////////////////////////////////////////////////
//! determines if a SIMM card is present
/*!
  \return	: true if a SIMM is present, false if not
*/
bool CRilIf::isSimPresent(void)
{
	return ((strcmp(m_setidImsi, "") != 0) || (strcmp(m_setidMsisdn, "") != 0));
}
