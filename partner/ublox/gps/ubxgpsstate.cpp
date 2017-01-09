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
  \file State Managment for UBX gps receiver
  \brief

  This class contain the state managment for the u-blox gps receiver. 
  It handles sending of commands to the receiver, collection and sending of 
  aiding information. 

  It has implemented the folwing feature: 
  - Time and Position Injection with UBX-AID-INI
  - Local Aiding of Ephemeris, Almanac, Ionossphere, UTC data and Health
  - AssistNow Autonomous Aiding 
  - AssistNow Offline (Server based, not Flash based)
  - Configuration of the receiver (e.g Rate, Baudrate, Messages)
*/
/*******************************************************************************
 * $Id: ubxgpsstate.cpp 62676 2012-10-22 14:45:53Z jon.bowern $
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <sys/stat.h>
#include <ctype.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cutils/properties.h>
#include <string.h>
#include <pthread.h>

#ifndef ANDROID_BUILD
// Needed for Linux build
#include <malloc.h>
#include <string.h>
#define _LIBS_CUTILS_UIO_H
#endif

#include "ubx_messageDef.h"
#include "ubx_timer.h"
#include "ubx_log.h"
#include "ubx_cfg.h"
#include "ubxgpsstate.h"
#include "protocolubx.h"
#include "ubx_agpsIf.h"

#include <linux/ioctl.h>

#define GPSCTL_IOM            'g'

#define GPSCTL_IOC_SET_POWER           _IOW(GPSCTL_IOM, 0x00, short)
#define GPSCTL_IOC_SET_CLK             _IOW(GPSCTL_IOM, 0x01, short)
#define GPSCTL_IOC_RESET               _IOW(GPSCTL_IOM, 0x02, short)
#define GPSCTL_IOC_ONOFF               _IOW(GPSCTL_IOM, 0x03, short)

///////////////////////////////////////////////////////////////////////////////
// Types & Definitions
#define AIDING_DATA_FILE			"/data/aiding.ubx"
#define SERPORT_DEFAULT				"/dev/ttyO3"
#define SERPORT_BAUDRATE_DEFAULT	9600
#define SHUTDOWN_TIMEOUT_DEFAULT    5		//!< 5 seconds
#define XTRA_POLL_INTERVAL_DEFUALT  20		//!< 20 hours

#ifdef SUPL_ENABLED
#define MSA_RESPONSE_DELAY_DEFAULT 	10		//!< Default timeout (in seconds) to response with psedo ranges for MSA session
#define NI_UI_TIMEOUT_DEFAULT		120		//!< Default timeout (in seconds) to display NI Notify/verify dialog
#define NI_RESPONSE_TIMEOUT			75		//!< Default timeout (in seconds) to respond to an NI request
#endif

///////////////////////////////////////////////////////////////////////////////
// Static data
static CUbxGpsState s_ubxGpsState;

    
///////////////////////////////////////////////////////////////////////////////
//! Constructor
CUbxGpsState::CUbxGpsState()
{
	memset(&m_Db, 0, sizeof(m_Db));
    m_Db.dbAssistChanged = false;
    m_Db.dbAssistCleared = true;
	m_Db.rateMs = 1000;  
    
	CCfg cfg;
	cfg.load("/system/etc/u-blox.conf");
	
	m_pSerialDevice 	= strdup(	cfg.get("SERIAL_DEVICE", 		SERPORT_DEFAULT) );
	m_baudRate 					= 	cfg.get("BAUDRATE" , 			SERPORT_BAUDRATE_DEFAULT);
	m_baudRateDef 				= 	cfg.get("BAUDRATE_DEF" , 		SERPORT_BAUDRATE_DEFAULT);
	m_pAlpTempFile		= strdup(	cfg.get("ALP_TEMP" , 			AIDING_DATA_FILE) );
	m_stoppingTimeoutMs 		= 	cfg.get("STOP_TIMEOUT", 		SHUTDOWN_TIMEOUT_DEFAULT) * 1000;
	m_xtraPollInterval 			= 	cfg.get("XTRA_POLL_INTERVAL", 	XTRA_POLL_INTERVAL_DEFUALT) * 60 * 60 * 1000;
	m_persistence				=	cfg.get("PERSISTENCE", 			1);
	m_receiverShutdownAck 		= 	false;
	
#ifdef SUPL_ENABLED
	m_almanacRequest 			= (bool) cfg.get("SUPL_ALMANAC_REQUEST", 			false);
	m_utcModelRequest 			= (bool) cfg.get("SUPL_UTC_MODEL_REQUEST", 			false);
	m_ionosphericModelRequest 	= (bool) cfg.get("SUPL_IONOSPHERIC_MODEL_REQUEST", 	false);
	m_dgpsCorrectionsRequest 	= (bool) cfg.get("SUPL_DGPS_CORRECTIONS_REQUEST", 	false);
	m_refLocRequest 			= (bool) cfg.get("SUPL_REF_LOC_REQUEST", 			false);
	m_refTimeRequest 			= (bool) cfg.get("SUPL_REF_TIME_REQUEST", 			false);
	m_acquisitionAssistRequest 	= (bool) cfg.get("SUPL_AQUISITION_ASSIST_REQUEST", 	false);
	m_realTimeIntegrityRequest 	= (bool) cfg.get("SUPL_TIME_INTEGRITY_REQUEST", 	false);
	m_navigationModelRequest 	= (bool) cfg.get("SUPL_NAVIGATIONAL_MODEL_REQUEST", false);
	m_fakePhone 				= (bool) cfg.get("SUPL_FAKE_PHONE_CONNECTION", 		false);
	m_msaResponseDelay 			= 		 cfg.get("SUPL_MSA_RESPONSE_DELAY", 		MSA_RESPONSE_DELAY_DEFAULT);
	m_niUiTimeout 				= 		 cfg.get("SUPL_NI_UI_TIMEOUT", 				NI_UI_TIMEOUT_DEFAULT);
	m_niResponseTimeout			= 		 cfg.get("SUPL_NI_RESPONSE_TIMEOUT",		NI_RESPONSE_TIMEOUT);
	CAgpsIf::getInstance()->setCertificateFileName(cfg.get("SUPL_CACERT", (const char*)NULL)); 
	m_logSuplMessages			= (bool) cfg.get("SUPL_LOG_MESSAGES", false);
	m_cmccLogActive				= (bool) cfg.get("SUPL_CMCC_LOGGING", false);
	m_suplMsgToFile				= (bool) cfg.get("SUPL_MSG_TO_FILE", false);
#endif	

	m_pSer = NULL;
#if defined UDP_SERVER_PORT	
	m_pUdpServer = NULL;
	m_udpPort 					= cfg.get("UDP_SERVER_PORT", 				UDP_SERVER_PORT);
#endif	

	m_agpsThreadParam.server 	= strdup( cfg.get("UBX_HOST", "") ); 
	m_agpsThreadParam.port		= cfg.get("UBX_PORT", 46434);
	char brand[PROPERTY_VALUE_MAX];
	char model[PROPERTY_VALUE_MAX];
	char serial[PROPERTY_VALUE_MAX];
	property_get("ro.product.brand", brand, "");
	property_get("ro.product.model", model, "");
	property_get("ro.product.serial", serial, "");
	if (strcmp(serial,"")==0)
		snprintf(m_agpsThreadParam.request, MAX_REQUEST, "and=%s.%s", brand, model);
	else
		snprintf(m_agpsThreadParam.request, MAX_REQUEST, "and=%s.%s.%s", brand, model, serial);
	m_agpsThreadParam.request[MAX_REQUEST-1] = '\0';
	m_agpsThreadParam.active = false;
	m_agpsThreadParam.timeoutLastRequest = getMonotonicMsCounter() - (60 * 120 * 1000);
		
	pthread_mutex_init(&m_ubxStateMutex, NULL);
}

///////////////////////////////////////////////////////////////////////////////
// Destructor
CUbxGpsState::~CUbxGpsState()
{
	// write back the data to the file 
	deleteAidData(GPS_DELETE_ALL); // this frees all the buffers
    
	if (m_pSerialDevice) 	free(m_pSerialDevice);
	if (m_pAlpTempFile) 	free(m_pAlpTempFile);
	if (m_agpsThreadParam.server) 	free(m_agpsThreadParam.server);
	
	pthread_mutex_destroy(&m_ubxStateMutex);
}

CUbxGpsState* CUbxGpsState::getInstance()
{
	return &s_ubxGpsState;
}

/*******************************************************************************
 * Thread safety
 *******************************************************************************/

void CUbxGpsState::lock(void)
{
	pthread_mutex_lock(&m_ubxStateMutex);
}

void CUbxGpsState::unlock(void)
{
	pthread_mutex_unlock(&m_ubxStateMutex);
}
	
/*******************************************************************************
 * STARTUP on SHUTDOWN
 *******************************************************************************/

//! on Statup Sequence 
/*! Exectute the sequence needed to restart the receiver, this includes the following steps.
    - powering up the gps (optional)
	- restore the local aiding to a file
    - enableing gps receiver
	- configure the receiver
	- do all the aiding 

*/
void CUbxGpsState::onStartup(void)
{
	logGps.txt(0x00000000, "#gps start");
	
    // Switch power on to the gps device
    if(powerOn())
	{
		usleep(150000); //waiting chipset power on
		setBaudRate();
	}
    
    // load any previous aiding data
    loadAiding();
	
	// turn on the gps, we assume that the gps is turned off
	writeUbxCfgRst(0x09/*gps start*/, 0);

    // Wait 50 msec before sending the rest...
    usleep(50000);

	// Configuration 
	// ---------------------------------------
	
	// set the desired navigation rate 
	writeUbxCfgRate(); 
	// enable additional messages and protocols this gives accuracies
	writeUbxCfgMsg(0xF0, 0x07);	    // enable NMEA-GST message
	writeUbxCfgMsg(0x01, 0x30);	    // enable UBX-NAV-SVINFO message
	writeUbxCfgMsg(0x01, 0x06);	    // enable UBX-NAV-SOL message
	writeUbxCfgMsg(0x01, 0x07);	    // enable UBX-NAV-PVT message
	// enable all the adining information 
	writeUbxCfgMsg(0x0B, 0x30);		// enable UBX-AID-ALM message
	writeUbxCfgMsg(0x0B, 0x31);		// enable UBX-AID-EPH message
	writeUbxCfgMsg(0x0B, 0x32);		// enable UBX-AID-ALP message
	writeUbxCfgMsg(0x0B, 0x33);		// enable UBX-AID-AOP message
#ifdef SUPL_ENABLED	
	writeUbxCfgMsg(0x02, 0x12);		// enable UBX-RXM-MEAS message for SUPL MS-Assist
#endif

	// Aiding 
	// ---------------------------------------
	// do time and position aiding
	writeUbxAidIni();
	// eventually push the alp header to the device such that it that we have a file
	writeUbxAlpHeader();
	// send all the aiding data available here
	sendAidData(true, true, true, true);
	// finally enable aop (might already be enabled by the aiding above
	writeUbxNavX5AopEnable();

}

//! on Prepare Shutdown Sequence 
/*! Exectute the sequence needed to initiate the shutdown, this includes the following steps.
    - polling of local aiding data
    - turning off the gps

*/
void CUbxGpsState::onPrepareShutdown()
{
    // Send stop gps msg
    // As this msg is sent after aiding data polling msgs, then the ack for this msgs
    // should appear after the responsing aiding msgs, and hence signify that shutdown
    // is complete.
    writeUbxCfgRst(0x08/*gps stop*/, 0);
}

//! on Shutdown Sequence 
/*! Exectute the sequence needed to finalize the shutdown, this includes the following steps.
	- save the local aiding to a file
    - powering down the gps (optional)

	\param state    GPS state variable
*/
void CUbxGpsState::onShutDown()
{
    // save aiding data
    saveAiding();    

    // Switch power off to the gps device
	powerOff();  
	
	logGps.txt(0x00000001, "#gps stop");
}

//! on Initialise Sequence 
/*! Exectute the sequence needed to initialise the device, this includes the following steps.
    - turning off the gps
	- powering down the gps (optional)

	\param state    GPS state variable
*/
void CUbxGpsState::onInit(void)
{
    // Make sure gps receiver is stopped
    writeUbxCfgRst(0x08/*gps stop*/, 0);
    
    // and make sure receiever is powered off (optional)
    powerOff();
}

/*******************************************************************************
 * MEASUREMENT RATE CONFIGURATION 
 *******************************************************************************/

//! set the mesurement rate
/*! put the requested measurement rate into our local configuration db, 
    the gps receiver can be switched of while we a receiveing such a request

	\param rateMs the mesaurement requested configuration 
*/ 
void CUbxGpsState::putRate(int rateMs)
{
    LOGV("Put Rate rate=%d", rateMs);
    m_Db.rateMs = rateMs;
}

//! send measurement rate to the receiver
/*! configure the measurement rate of the receiver

	\return         true if sucessfull, false otherwise
*/ 
bool CUbxGpsState::writeUbxCfgRate()
{
    // Send CFG-RATE command
    GPS_UBX_CFG_RATE_t Payload;
    Payload.measRate = m_Db.rateMs;
    Payload.navRate = 1;
    Payload.timeRef = 1;
    LOGV("Send CFG-RATE rate=%d", Payload.measRate);
    return writeUbx(0x06, 0x08, &Payload, sizeof(Payload));
}

//! Set the baud rate for the receiver and the host's serial port
void CUbxGpsState::setBaudRate()
{
  int baudRate = getBaudRate();

  if (baudRate != getBaudRateDefault())
     {
      // reconfigure the baud rate 
        usleep(100000);
        m_pSer->setbaudrate(getBaudRateDefault());
        usleep(100000);
        writeUbxCfgPort(1 /* UART1 */, baudRate);
        usleep(100000); // 1/10 second
        m_pSer->setbaudrate(baudRate);
     }

}

/*******************************************************************************
 * LOCAL AIDING: EPH,HUI,ALM,AOP
 *******************************************************************************/

//! send local aiding data to the receiver
/*! send the local adining data to the receiver

	\param bEph     set to true if Ephemeris shall be aided
	\param bAop     set to true if AssistNow Autonomous shall be aided
	\param bAlm     set to true if Almanac shall be aided
	\param bHui     set to true if Health/UTC/Ionosphere shall be aided
*/ 
void CUbxGpsState::sendAidData(bool bEph, bool bAop, bool bAlm, bool bHui)
{
	BUF_t* p = NULL;
	if (bEph)
	{
		for (int svix = 0; svix < NUM_GPS_SVS; svix ++)
		{
			p = &m_Db.sv[svix].eph;
			if ((p->p != NULL) && (p->i>0))
            {
                if (m_pSer->writeSerial(p->p,p->i) == (int)p->i)
				{
//                    LOGV("Send Eph G%d size %d", svix+1, p->i);
				}
#if defined UDP_SERVER_PORT
                m_pUdpServer->sendPort(p->p,p->i);
#endif
            }
		}
	}
	if (bAop)
	{
		for (int svix = 0; svix < NUM_GPS_SVS; svix ++)
		{
			p = &m_Db.sv[svix].aop;
			if ((p->p != NULL) && (p->i>0))
            {
                if (m_pSer->writeSerial(p->p,p->i) == (int)p->i)
				{
//                    LOGV("Send Aop G%d size %d", svix+1, p->i);
				}
#if defined UDP_SERVER_PORT
                m_pUdpServer->sendPort(p->p,p->i);
#endif
            }
		}
	}
	if (bAlm)
	{
		for (int svix = 0; svix < NUM_GPS_SVS; svix ++)
		{
			p = &m_Db.sv[svix].alm;
			if ((p->p != NULL) && (p->i>0))
            {
                if (m_pSer->writeSerial(p->p,p->i) == (int)p->i)
				{
//                    LOGV("Send Alm G%d size %d", svix+1, p->i);
				}
#if defined UDP_SERVER_PORT
                m_pUdpServer->sendPort(p->p,p->i);
#endif
            }
		}
	}
	if (bHui)
	{
		p = &m_Db.hui;
        if ((p->p != NULL) && (p->i>0))
        {
            if (m_pSer->writeSerial(p->p,p->i) == (int)p->i)
			{
//                LOGV("Send Hui size %d", p->i);
			}
#if defined UDP_SERVER_PORT
            m_pUdpServer->sendPort(p->p,p->i);
#endif
        }
	}
}

//! poll local aiding data from the receiver
/*! request the local adining data from the receiver

	\param bEph     set to true if Ephemeris shall be polled
	\param bAop     set to true if AssistNow Autonomous shall be polled
	\param bAlm     set to true if Almanac shall be polled
	\param bHui     set to true if Health/UTC/Ionosphere shall be polled
*/ 
void CUbxGpsState::pollAidData(bool bEph, bool bAop, bool bAlm, bool bHui)
{
	if (bEph) writeUbx(0x0B, 0x31, NULL, 0);
	if (bAop) writeUbx(0x0B, 0x33, NULL, 0);
	if (bAlm) writeUbx(0x0B, 0x30, NULL, 0);
	if (bHui) writeUbx(0x0B, 0x02, NULL, 0);
}

//! handle new messages from the receiver
/*!	This is the main routine which is called whenever a new UBX message was received in the parser. 

	\param pMsg     the pointer to the complete message (includes frameing and payload)
	\param iMsg     the size of the complete message (includes frameing and payload)
*/
void CUbxGpsState::onNewUbxMsg(GPS_THREAD_STATES state, const unsigned char* pMsg, unsigned int iMsg)
{
    unsigned char clsId = pMsg[2];
	unsigned char msgId = pMsg[3];
    
    //LOGV("%s : Received %x %x %x %x %x %x", __FUNCTION__, pMsg[2], pMsg[3], pMsg[4], pMsg[5], pMsg[6], pMsg[7]);
    if ((clsId == 0x0B) && (iMsg >= 9))
    {
        // save the aiding data in a local database
	    if (0x02 == msgId)
		{
			// Health / UTC / Ionosphere paramters 
			if (replaceBuf(&m_Db.hui, pMsg, iMsg))
            {
                m_Db.dbAssistChanged = true;
				LOGV("Got new Hui");
            }
		}
		else if (0x01 == msgId)
		{
			onNewUbxAidIni(pMsg, iMsg);
		}
        else if ((0x32 == msgId) && (state != -1))
		{
			// AssistNow Offline
			onNewUbxAlpMsg(pMsg,iMsg);
		}
		else
		{
			unsigned int svix = pMsg[6] - 1;
        	if ((0x30 == msgId) && (iMsg > 16) && (svix < NUM_GPS_SVS))
			{
				// Almanac
				if (replaceBuf(&m_Db.sv[svix].alm, pMsg, iMsg))
                {
                    m_Db.dbAssistChanged = true;
					LOGV("Got Alm G%d", svix+1);
                }
			}
			else if ((0x31 == msgId) && (iMsg > 16) && (svix < NUM_GPS_SVS))
			{
				// Ephemeris parameters
				if (replaceBuf(&m_Db.sv[svix].eph, pMsg, iMsg))
                {

                    m_Db.dbAssistChanged = true;
					LOGV("Got Eph G%d", svix+1);
                }
			}
			else if ((0x33 == msgId) && (iMsg > 9) && (svix < NUM_GPS_SVS))
			{
				// AssistNow Offline parameters
				if (replaceBuf(&m_Db.sv[svix].aop, pMsg, iMsg))
                {
                    m_Db.dbAssistChanged = true;
					LOGV("Got Aop G%d", svix+1);
                }
			}
		}
    }
    else if ((clsId == 0x05) && (msgId == 0x01) && (iMsg == 10))
    {
        time_t now = time(NULL);
		char* s = ctime(&now);
		s[strlen(s)-1] = '\0';
        // ACK-ACK received
        if ((pMsg[6] == 0x06) && (pMsg[7] == 0x04))
        {
            // Acknowledging last CFG-RST
            LOGV("%s : Ack of last CFG-RST state %i (%s) - ", __FUNCTION__, state, s);
            
            if (state == GPS_STOPPING)
            {
                m_receiverShutdownAck = true;
                LOGV("%s : Ack of last CFG-RST - Stopping sequence can now complete", __FUNCTION__);
            }
        }
		else 
		{
			LOGV("%s : Ack received %02X-%02X (%s)", __FUNCTION__, pMsg[6], pMsg[7], s);
		}        
    }
}

//! delete local aiding in the local database 
/*!	Delete the local aiding data in our database, this may also free the buffers. 
	It does not send a message to the receiver to clear the receivers nvs data, 
	this has to be done separately,

	\param flags the things to clear, see gps.h for a definition
*/
void CUbxGpsState::deleteAidData(GpsAidingData flags)
{
	if (flags & GPS_DELETE_POSITION)
		memset(&m_Db.pos, 0, sizeof(m_Db.pos));
	if (flags & GPS_DELETE_TIME)
		memset(&m_Db.time, 0, sizeof(m_Db.time));
	for (int svix = 0; svix < NUM_GPS_SVS; svix ++)
	{
		if (flags & GPS_DELETE_EPHEMERIS)
		{
			freeBuf(&m_Db.sv[svix].eph);
			//LOGV("Clr Eph G%d", svix+1);
		}
		if (flags & GPS_DELETE_SADATA)
		{
			freeBuf(&m_Db.sv[svix].aop);
			//LOGV("Clr Aop G%d", svix+1);
		}
		if (flags & GPS_DELETE_ALMANAC)
		{
			freeBuf(&m_Db.sv[svix].alm);
			//LOGV("Clr Alm G%d", svix+1);
		}
	}
	if (flags & (GPS_DELETE_IONO|GPS_DELETE_UTC|GPS_DELETE_HEALTH))
	{
		freeBuf(&m_Db.hui);
		//LOGV("Clr Hui");
	}
	if (flags & (GPS_DELETE_SVDIR|GPS_DELETE_SVSTEER))
	{
		freeBuf(&m_Db.alp.file);
		//LOGV("Clr Apl");
	}

    m_Db.dbAssistChanged = true;
}

//! Enable AOP in the receiver
/*!	Send a UBX-CFG-NAVX5 message to the receiver to enable the AssistNow Autonomous Feature.

	\param state    GPS state variable
	\return         true if sucessfull, false otherwise
*/
bool CUbxGpsState::writeUbxNavX5AopEnable()
{
	GPS_UBX_CFG_NAV5X_t Payload;
	memset(&Payload, 0, sizeof(Payload));
	Payload.mask1 = 0x4000; // config AOP
	Payload.aopFlags = 0x01; // enable AOP
	Payload.aopOrbMaxError = 0; // use default
	
	return writeUbx(0x06, 0x23, &Payload, sizeof(Payload));
}

/*******************************************************************************
 * LOCAL AIDING: TIME POSITION
 *******************************************************************************/

//! Helper function to return a reference time
/*! this function has to be used to calculate aging of of the NTP (inject_time)
	return the current refrence time.
*/
int64_t CUbxGpsState::currentRefTimeMs(void)
{
	return getMonotonicMsCounter();
}	

//! put time information into the local database.
/*! Put the time information received from a SNTP request into the local database. 
	This data is then used by #writeUbxAidIni to achieve time transfer from the network 
	to the device.

	\param timeUtcMs The current utc time in milliseconds as define in gps.h
	\param timeRefMs the reference time in milliseconds of the SNTP request use together with #currentRefTimeMs to age the timeUtcMs
	\param accMs The time accuracy in milliseconds of this information (half the rount trip time). 

*/
void CUbxGpsState::putTime(GpsUtcTime timeUtcMs, int64_t timeRefMs, int accMs)
{
	time_t tUtc = timeUtcMs/1000;
	char s[20];
	struct tm t;
	strftime(s, 20, "%Y.%m.%d %H:%M:%S", gmtime_r(&tUtc, &t)); 
	LOGV("Got Utc %s.%03d Ref %lli Now %lli Acc %.3f ms", s, (int)(timeUtcMs%1000), timeRefMs, currentRefTimeMs(), accMs*0.001);
	m_Db.time.timeUtcMs = timeUtcMs;
	m_Db.time.timeRefMs = timeRefMs;
	m_Db.time.accMs		= accMs;
    
    m_Db.dbAssistChanged = true;
}

//! put the position information into the local database 
/*! Put the position information received from Android into the local database. 
	This data is then used by #writeUbxAidIni to achieve position aiding from the network
	The function also recored the time of reception so that it could be aged.

	\param latDeg the Latitude in Degrees
	\param latDeg the Longitude in Degrees
	\param accM the accuracy in meters of this information 
*/	
void CUbxGpsState::putPos(double latDeg, double lonDeg, float accM)
{
	LOGV("Got Lat %.6f Lon %.6f Acc %f", latDeg, lonDeg, (double)accM);
	m_Db.pos.latDeg = latDeg;
	m_Db.pos.lonDeg = lonDeg;
	m_Db.pos.accM   = accM;
	m_Db.pos.timeRefMs = currentRefTimeMs();
    
    m_Db.dbAssistChanged = true;
}

//! do send position and time aiding to the receiver
/*!	Send a UBX-AID-INI message to the receiver. This implemenation of the message 
    support the following aiding: 
	- position 
	- time

	\return         true if sucessfull, false otherwise
*/
bool CUbxGpsState::writeUbxAidIni()
{
	GPS_UBX_AID_INI_U5__t Payload;
	memset(&Payload, 0, sizeof(Payload));
	int64_t timeNowMs = currentRefTimeMs();
	// position aiding 
	if (m_Db.pos.accM > 0)
	{
		Payload.ecefXOrLat = (I4)(m_Db.pos.latDeg * 1e7); // deg -> deg*1e-7
		Payload.ecefYOrLon = (I4)(m_Db.pos.lonDeg * 1e7); // deg -> deg*1e-7
		Payload.posAcc     = (U4)(m_Db.pos.accM   * 1e2); // m -> cm
		Payload.flags	  |= GPS_UBX_AID_INI_U5__FLAGS_POS_MASK | 
							 GPS_UBX_AID_INI_U5__FLAGS_LLA_MASK | 
							 GPS_UBX_AID_INI_U5__FLAGS_ALTINV_MASK;
		LOGV("Send Ini Lat %d Lon %d Acc %dcm", Payload.ecefXOrLat, Payload.ecefYOrLon, Payload.posAcc );
	}
	// time aiding 
	if (m_Db.time.accMs > 0)
	{
		int64_t timeUtcMs = m_Db.time.timeUtcMs;
		int64_t ageMs = (timeNowMs - m_Db.time.timeRefMs);
		int64_t accNs = (MSTIME_ACC_COMM + m_Db.time.accMs) * 1000000 + ageMs; // about 1 us / second aging
		const int64_t oneMinuteNs  = 60ll * 1000000000ll;
		const int oneWeekMs = 1000 * 60 *60 * 24 * 7;
		const int leapSecMs = 1000 * 16; // leap second 22.10.2012 is 16 seconds
		int64_t timeGpsMs = timeUtcMs + ageMs + leapSecMs;
		// subtract offset between gps and utc time
		timeGpsMs	   -= 315964800000ll; // which is mkgmtime(1980,1,6,0,0,0,0)*1000
		if (accNs < oneMinuteNs)
		{
			Payload.wn     = (timeGpsMs / oneWeekMs);
			Payload.tow    = (timeGpsMs % oneWeekMs);
			Payload.towNs  = 0;
//			Payload.tAccMs = (accNs / 1000000);
//			Payload.tAccNs = (accNs % 1000000);
            /* Use not so precise accuracy, it would help when next leap second will arrive... */
			Payload.tAccMs = 2000;
			Payload.tAccNs = 0;
			Payload.flags |= GPS_UBX_AID_INI_U5__FLAGS_TIME_MASK;
		}

		time_t tUtc = timeUtcMs/1000;
		char s[20];
		struct tm t;
		strftime(s, 20, "%Y.%m.%d %H:%M:%S", gmtime_r(&tUtc, &t)); 
		LOGV("Send Ini UTC %s.%03d GPS %d:%09d Acc %d.%06dms Age %dms ", s, (int)(timeUtcMs%1000), Payload.wn, Payload.tow, 
				(int)Payload.tAccMs,  (int)Payload.tAccNs, (int)ageMs);
	}
	// only need to do something if the is at least one part of the information available
	if (Payload.flags != 0)
	{
		return writeUbx(0x0B, 0x01, &Payload, sizeof(Payload));
	}
	return false;
}

//! handle a aid ini message
/*!	Parse a UBX-AID-INI message to the receiver. This implemenation of the message 
    support the following aiding: 
	- position 
	- time

	\param pMsg the pointer to the complete message (includes frameing and payload)
	\param iMsg the size of the complete message (includes frameing and payload)
	\return true if sucessfull, false otherwise
*/
bool CUbxGpsState::onNewUbxAidIni(const unsigned char* pMsg, unsigned int iMsg)
{
	GPS_UBX_AID_INI_U5__t Payload;
	if (iMsg != 8+sizeof(Payload))
		return false;
	memcpy(&Payload, &pMsg[6], sizeof(Payload));
	// position aiding 
	if (Payload.flags & GPS_UBX_AID_INI_U5__FLAGS_POS_MASK)
	{
		if (!(Payload.flags & GPS_UBX_AID_INI_U5__FLAGS_LLA_MASK))
		{
			//convertXYZtoLLH
		}
/*		Payload.flags	  |= GPS_UBX_AID_INI_U5__FLAGS_POS_MASK | 
							 GPS_UBX_AID_INI_U5__FLAGS_LLA_MASK | 
							 GPS_UBX_AID_INI_U5__FLAGS_ALTINV_MASK;*/
		LOGV("Got Ini XYZ %d/%d/%d Acc %dcm", Payload.ecefXOrLat, Payload.ecefYOrLon, Payload.ecefZOrAlt, Payload.posAcc );
	}
	// time aiding 
	if (Payload.flags & GPS_UBX_AID_INI_U5__FLAGS_TIME_MASK)
	{
		double tow = Payload.tow +    1e-6 * Payload.towNs;
		double acc = Payload.tAccMs + 1e-6 * Payload.tAccNs;
		LOGV("Got Ini Time %d:%16.9f Acc %10.6fms", Payload.wn, 1e-3 * tow, acc);
	}
	// only need to do something if the is at least one part of the information available
	return true;
}

/*******************************************************************************
 * ASSISTNOW OFFLINE / ALMANAC PLUS 
 *******************************************************************************/

// Struture of the Assistnow Offline / Almanac Plus data
typedef struct {
	U4 magic;                       // magic word must be 0x015062b5
	U2 reserved0[32];
	U2 size;                        // size of full file, incl. header, in units of 4 bytes
	U2 reserved1;
	U4 reserved2;
	U4 p_tow;                       // Start of prediction Time of week
	U2 p_wno;                       // Start of prediction Weeknumber
	U2 duration;                    // Nominal/Typical duration of predictions, in units of 10 minutes.
} ALP_FILE_HEADER_t;

//! handle new Assistnow Offline / Almanac Plus messages from the receiver
/*!	This is the main routine which is called whenever a new UBX_AID-ALP message was received in the parser. 

	\param pMsg     the pointer to the complete message (includes frameing and payload)
	\param iMsg     the size of the complete message (includes frameing and payload)
	\return         true if sucessfull, false otherwise
*/
bool CUbxGpsState::onNewUbxAlpMsg(const unsigned char* pMsg, unsigned int iMsg)
{
	GPS_UBX_AID_ALPSRV_CLI_t cli;
	if ((m_Db.alp.file.p != NULL) && (m_Db.alp.file.i > 0)) 
	{
		memcpy(&cli, &pMsg[6], sizeof(cli));
		if (cli.idSize >= sizeof(cli))
		{
			unsigned int size   = (unsigned int)cli.size << 1;
			unsigned int offset = (unsigned int)cli.ofs  << 1;
//			LOGV("Got Alp Request idsize %d type %d ofs %d size %d fileId %d", cli.idSize, cli.type, cli.ofs, cli.size, cli.fileId);
			if ( (size > 0) && (offset < m_Db.alp.file.i) )
			{
				if (offset + size >= m_Db.alp.file.i)	// if we exceed the file size 
					size = m_Db.alp.file.i - offset;		//   then truncate
				if (cli.type == 0xFF)
				{
					if (m_Db.alp.fileId == cli.fileId)
					{
						LOGV("Update Alp fileId %d offset %d size %d", cli.fileId, offset, size);
                        memcpy(&m_Db.alp.file.p[offset], &pMsg[6+cli.idSize], size);
                        m_Db.dbAssistChanged = true;
						return true;
					}
					else
					{
						LOGE("Update Alp failed fileId %d does not match %d offset %d size %d", cli.fileId, m_Db.alp.fileId, offset, size);
						return writeUbxAlpHeader();
					}
				}
			}
			else
			{
				LOGE("Request Alp out of range fileId %d offset %d size %d", cli.fileId, offset, size);
				return writeUbxAlpHeader();
			}
			GPS_UBX_AID_ALPSRV_SRV_t srv;
			memset(&srv, 0, sizeof(srv));
			memcpy(&srv, &pMsg[6], cli.idSize);
            srv.fileId = m_Db.alp.fileId;
			srv.dataSize = size;
//			LOGV("Send Alp Answer idsize %d type %d ofs %d size %d fileId %d datasize %d, %02X %02X %08X", 
//						srv.idSize, srv.type, srv.ofs, srv.size, srv.fileId, 
//						srv.dataSize, srv.id1, srv.id2, srv.id3);
			return writeUbx(0x0B, 0x32, &srv, srv.idSize, &m_Db.alp.file.p[offset], size);
		}
	}
	return false;
}

//! put the Assistnow Offline / Alamanac Plus data into the local database
/*!	Put the Assistnow Offline / Alamanac Plus data into the local database and check / validate 
	the content. Very simple check on the header are done. And if of the file ist saved in the db.
	The time of reception is recorded to later check if a a file update shal be requested.

	\param pData The pointer to the complete ALP file
	\param iData The size of the complete ALP file
	\return true if sucessfull, false otherwise
*/
bool CUbxGpsState::putAlpFile(const unsigned char* pData, unsigned int iData)
{
	if (iData < (int)sizeof(ALP_FILE_HEADER_t))
		LOGE("Alp size %d too small", iData);
	else
	{
		ALP_FILE_HEADER_t head;
		memcpy(&head, pData, sizeof(head));
		if (head.magic != 0x015062b5)
			LOGE("Alp magic bad %08X", head.magic);
		else if (head.size*4 != iData)
			LOGE("Alp size bad %d != %d", head.size*4, iData);
		else if (replaceBuf(&m_Db.alp.file, pData, iData))
		{
			m_Db.alp.fileId ++;
			m_Db.alp.timeRefMs = currentRefTimeMs();
            m_Db.dbAssistChanged = true;
			LOGV("Got Alp file %d size %d at", m_Db.alp.fileId, iData);
			LOGV("Info Alp time %d:%06d dur %.3fdays\n", head.p_wno, head.p_tow, (head.duration * 10.0) / (60.0 * 24.0));
			return true;
		}
	}
	return false;
}

//! check if the new Assistnow Offline / Alamanac Plus file shall be requested. 
/*! check if the new Assistnow Offline / Alamanac Plus file shall be requested. 
    A very simple timeout after reception is used to request a new file.  
	
	\note the duration of the data in the current file may exceed this timeout. 
	But we want to keep the data uptodate to acheive best accuracy. 

	\return true if the file is not yet old, false if it is old or not available
*/
bool CUbxGpsState::checkAlpFile(void)
{
	if (!m_Db.alp.file.p || (m_Db.alp.file.i < sizeof(ALP_FILE_HEADER_t)))
		return false;       // No ALP file present, so ALP can be requested
        
	int64_t timeNowMs = currentRefTimeMs();
	return (timeNowMs - m_Db.alp.timeRefMs) < m_xtraPollInterval;
}

//! Push the Assistnow Offline / Alamanac Plus header to the device.
/*! Push the Assistnow Offline / Alamanac Plus header to the device. 
	This function is used at startup to push the knowledge of the file / file id to the receiver.  
	
	\return         true if sucessfull, false otherwise
*/
bool CUbxGpsState::writeUbxAlpHeader()
{
	if (!m_Db.alp.file.p || (m_Db.alp.file.i < sizeof(ALP_FILE_HEADER_t)))
		return false;
	GPS_UBX_AID_ALPSRV_SRV_t srv;
	memset(&srv, 0, sizeof(srv));
	srv.idSize   = sizeof(srv);
	srv.type 	 = 1; // header need to be type 1
	srv.ofs      = 0 >> 1;
	srv.size     = sizeof(ALP_FILE_HEADER_t) >> 1;
	srv.fileId   = m_Db.alp.fileId;
	srv.dataSize = sizeof(ALP_FILE_HEADER_t);
	LOGV("Send Alp Header idsize %d type %d ofs %d size %d fileId %d datasize %d, %02X %02X %04X", 
				srv.idSize, srv.type, srv.ofs, srv.size, srv.fileId, 
				srv.dataSize, srv.id1, srv.id2, srv.id3);
	return writeUbx(0x0B, 0x32, &srv, srv.idSize, &m_Db.alp.file.p[0], sizeof(ALP_FILE_HEADER_t));
}

/*******************************************************************************
 * ASSIST NOW ONLINE
 *******************************************************************************/

void CUbxGpsState::checkAgpsDownload(void)
{
	// Check the Agps (AssistNow Online) thread is not running... 
	LOGV("CUbxGpsState::%s : Checking Agps (Online) server", __FUNCTION__);
	if ((m_agpsThreadParam.server != NULL) && (*m_agpsThreadParam.server != '\0'))
	{
		if (!m_agpsThreadParam.active)
		{
			int64_t deltaTime = getMonotonicMsCounter() - m_agpsThreadParam.timeoutLastRequest;
			LOGV("CUbxGpsState::%s : Agps delta timeout is %lld", __FUNCTION__, deltaTime);
			if (deltaTime >= (60 * 100*1000)) /* Timeout of 100 minutes */
			{
				LOGV("CUbxGpsState::%s : Request AssistNow online", __FUNCTION__);
				m_agpsThreadParam.active = true;
				if (pthread_create( &m_agpsThreadParam.thread, NULL, 
					CUbxGpsState::agpsDownloadThread, &m_agpsThreadParam) == 0)
					m_agpsThreadParam.active = false;
			}
			else
			{
				LOGV("CUbxGpsState::%s : Assistance data old less than 100 minutes", __FUNCTION__);
			}
		}
		else
		{
			LOGV("CUbxGpsState::%s : Agps thread still active!", __FUNCTION__);
		}
	}
	else
	{
		LOGV("CUbxGpsState::%s : Agps (Online) server not defined", __FUNCTION__);
	}
}

//! request aiding iformation from a Assist Now Server / Poxy Server
void* CUbxGpsState::agpsDownloadThread(void *pThreadData)
{
	AGPS_THREAD_DATA_t* data = (AGPS_THREAD_DATA_t*) pThreadData;    

	// get the ip address 
	struct sockaddr_in server;
	memset( &server, 0, sizeof (server));
	unsigned long addr = inet_addr(data->server);
	if (addr != INADDR_NONE)  // numeric IP address
		memcpy((char *)&server.sin_addr, &addr, sizeof(addr));
	else
	{
		// get by host name
		struct hostent* host_info = gethostbyname(data->server);
		if (host_info != NULL) 
			memcpy((char *)&server.sin_addr, host_info->h_addr, host_info->h_length);
		else
		{
			LOGE("CUbxGpsState::%s : Cannot resolve server %s", __FUNCTION__, data->server);
			data->active = false;
			return NULL;
		}
	}
	server.sin_family = AF_INET;
	server.sin_port	  = htons( data->port ); 
	// create the socket 
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == -1) 
	{
		LOGE("CUbxGpsState::%s : Cannot create socket", __FUNCTION__);
		data->active = false;
		return NULL;
	}
	// establish connection to the server
	if (connect( sock, (struct sockaddr*)&server, sizeof( server)) == -1)
	{
		LOGE("CUbxGpsState::%s : Cannot connect server", __FUNCTION__);
		close(sock);
		data->active = false;
		return NULL;
	}

    // send the http get request
	LOGV("Send %s", data->request);
	send(sock, data->request, strlen(data->request), 0);
	// now receive the header 
	int iSize = 0;
	char header[1024];
	char* p = header;
	do 
	{
		int b = recv(sock, p, 1, 0);
		if (b < 0)
			break;
		else if ((b > 0) && (*p != '\r'))
		{
			p ++;
			iSize ++;
		}
	} while ((iSize < (int)(sizeof(header)-1)) && ((iSize < 2) || (p[-2] != '\n') || (p[-1] != '\n')));
	if (iSize < (int)(sizeof(header)-1))
	{
		*p = '\0';
		LOGV("CUbxGpsState::%s Got Online header\n%s", __FUNCTION__, header);
		// check the two mandatory fields length and type
		char* pLength = strstr(header, "Content-Length: ");
		char* pType   = strstr(header, "Content-Type: ");
		if (pLength && pType)
		{
			pType += 14;
			pLength += 16;
			int iSize = atoi(pLength);
			// check the content type, can be either txt (an error message) or ubx protocol (aiding data)
			bool bUBX  = strncasecmp(pType, "application/ubx", 15)==0;
			bool bTxt  = strncasecmp(pType, "text/plain",      10)==0;
			if ((bUBX || bTxt) && (iSize > 0))
			{
				char* pData = (char*)malloc(iSize);
				if (pData != NULL) 
				{
					int iRecv = recv(sock, pData, iSize, MSG_WAITALL);
					if (iRecv==iSize)
					{
						if (bUBX)
						{
							LOGV("Got agps data of size %d", iSize);
							injectDataAgpsOnlineData(pData, iSize);

                            /* Mark the time when the assistance data has been inject */
                            data->timeoutLastRequest = getMonotonicMsCounter();
                            LOGV("Agps timeout reload %lld", data->timeoutLastRequest);
						}
						else if (bTxt)
							LOGW("CUbxGpsState::%s : Got message:\n%s", __FUNCTION__, pData);
					}
					else 
						LOGE("%s : agps size does not match Content Length %d/%d", __FUNCTION__, iRecv, iSize);
					// free the momory
					free(pData);
				}
				else 
					LOGE("CUbxGpsState::%s : No memory for agps data of size %d", __FUNCTION__, iSize);
			}
			else 
				LOGE("CUbxGpsState::%s : Invalid Content Type", __FUNCTION__);
		}
		else 
			LOGE("CUbxGpsState::%s : Invalid Header", __FUNCTION__);
	}
	else 
		LOGE("CUbxGpsState::%s : Header Size", __FUNCTION__);
	// we are done close socket
	close(sock);
	// Mark the thread is not running
	data->active = false;
	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//! Handles the injection of agps (AssistNow Online) data in to the receiver
/*!
  \param data	: Pointer to buffer containing data
  \param length	: Length of buffer
*/
void CUbxGpsState::injectDataAgpsOnlineData(const char* data, int length)
{
	CUbxGpsState* pUbxGps = CUbxGpsState::getInstance();
    pUbxGps->lock();
	int iMsg;
	do 
	{
		iMsg = CProtocolUBX::ParseFunc((unsigned char*)data, length);
		if (iMsg > 0) 
		{
			pUbxGps->onNewUbxMsg((GPS_THREAD_STATES) -1, (const unsigned char*)data, iMsg);
			length -= iMsg;
			data += iMsg;
		}
	}
	while (iMsg > 0);
	if (length == 0)
		pUbxGps->sendAidData(true, false, true, true);
	else
		LOGE("Part of the Agps data seems to be invalid %d", length);
	pUbxGps->unlock();
}

/*******************************************************************************
 * BUFFER HELPERS
 *******************************************************************************/

//! simple helper function to allocate and store a buffer in the database 
/*! Put the date into a buffer, this function checks if the data is the same as pervious data.
    If not it allocate a new buffer and copies the content of the new data. the it repalces the 
	old data with the new one and frees the memory of the old data.

	\param pBuf the buffer to replace 
	\param pMsg the pointer to the data to be stored
	\param iMsg the size of the data to be stored
	\return true if sucessfull, false if failed ot data was equal to already stored data
*/
bool CUbxGpsState::replaceBuf(BUF_t* pBuf, const unsigned char* pMsg, unsigned int iMsg)
{
	if ((pBuf != NULL) && (pMsg != NULL))
    {
		if ((pBuf->p != NULL) && (pBuf->i == iMsg) && (0 == memcmp(pBuf->p, pMsg, iMsg)))
			return false; // we are done already 

		unsigned char* p  = (unsigned char*)malloc(iMsg);    
        if (p)
        {
			memcpy(p, pMsg, iMsg);
			if (pBuf->p != NULL)
				free(pBuf->p);
            pBuf->p = p;
			pBuf->i = iMsg;
			return true;
        }
    }
	return false;
}

//! simple helper to free a buffer 
/*! simple helper to free a buffer

	\param pBuf the buffer to be freed.
*/
void CUbxGpsState::freeBuf(BUF_t* pBuf)
{
	if ((pBuf != NULL) && (pBuf->p != NULL))
    {
        free(pBuf->p);
		pBuf->p = NULL;
		pBuf->i = 0;
    }
}

/*******************************************************************************
 * UBX PROTOCOL: OTHERS
 *******************************************************************************/

//! Enable UBX or NMEA message on the current port.
/*! write a UBX-CFG-MSG message to the receiver
	
	\param clsId    the class Id of the message to be enabled
	\param msgId    the message Id of the message to be enabled
	\return         true if sucessfull, false otherwise 
*/
bool CUbxGpsState::writeUbxCfgMsg(int clsId, int msgId)
{
	GPS_UBX_CFG_MSG_SETCURRENT_t Payload;
	memset(&Payload, 0, sizeof(Payload));
	Payload.classType	= clsId;
	Payload.msgID		= msgId;
	Payload.rate		= 1;
	LOGV("Send CFG-MSG id=%02X-%02X enable", clsId, msgId);
    return writeUbx(0x06, 0x01, &Payload, sizeof(Payload));
}

//! Change the Baudrate of a port.
/*! write a UBX-CFG-PORT message to the receiver
	
	\param portId   the port Id to be configured (usually this is UART1)
	\param baudRate the baudrate to be configured 
	\return         true if sucessfull, false otherwise 
*/
bool CUbxGpsState::writeUbxCfgPort(int portId, int baudRate)
{
	GPS_UBX_CFG_PRT_UART_U5_t Payload;
	memset(&Payload, 0, sizeof(Payload));
	Payload.portID			= portId;
	Payload.mode			= 0x000008D0;
	Payload.baudRate		= baudRate;
	Payload.inProtoMask		= 0x0007;
	Payload.outProtoMask	= 0x0003;
	LOGV("Send CFG-Port baudrate=%d", baudRate);
    return writeUbx(0x06, 0x00, &Payload, sizeof(Payload));
}

//! Change the operating mode of the receiver or reset the receiver.
/*! write a UBX-CFG-RST message to the receiver
	
	\param resetMode    the resetMode to be executed (8=stop,9=start,2=gps reset)
	\param flags        the information to be cleared set to 0 if nothing needs to be cleared.
	\return             true if sucessfull, false otherwise 
*/
bool CUbxGpsState::writeUbxCfgRst(int resetMode, GpsAidingData flags)
{

	unsigned short ubxFlags = 0;
	if (flags == GPS_DELETE_ALL) 
		ubxFlags = 0xFFFF;
    else
	{
		if (flags & GPS_DELETE_EPHEMERIS)	ubxFlags |= GPS_UBX_CFG_RST_NAVBBRMASK_EPH_MASK;
        if (flags & GPS_DELETE_ALMANAC)		ubxFlags |= GPS_UBX_CFG_RST_NAVBBRMASK_ALM_MASK;
        if (flags & GPS_DELETE_POSITION)	ubxFlags |= GPS_UBX_CFG_RST_NAVBBRMASK_POS_MASK;
        if (flags & GPS_DELETE_TIME)		ubxFlags |= GPS_UBX_CFG_RST_NAVBBRMASK_RTC_MASK;
        if (flags & GPS_DELETE_IONO)		ubxFlags |= GPS_UBX_CFG_RST_NAVBBRMASK_KLOB_MASK;
        if (flags & GPS_DELETE_UTC)			ubxFlags |= GPS_UBX_CFG_RST_NAVBBRMASK_UTC_MASK;
        if (flags & GPS_DELETE_HEALTH)		ubxFlags |= GPS_UBX_CFG_RST_NAVBBRMASK_HEALTH_MASK;
        if (flags & GPS_DELETE_SADATA)		ubxFlags |= (1<<15) /* AOP DATA */;
    }
    // Send CFG-RST command
    GPS_UBX_CFG_RST_t Payload;
    memset(&Payload, 0, sizeof(Payload));
	Payload.resetMode  = resetMode; // controlled GPS only reset 
    Payload.navBbrMask = ubxFlags;
	return writeUbx(0x06, 0x04, &Payload, sizeof(Payload));
}

#ifdef SUPL_ENABLED
void CUbxGpsState::sendEph(const void* pData, int size)
{
	writeUbx(UBXID_AID_EPH >> 8, UBXID_AID_EPH & 0xFF, pData, size);
}

void CUbxGpsState::sendAidingData(GPS_UBX_AID_INI_U5__t *pAidingData)
{
	writeUbx(UBXID_AID_INI >> 8, UBXID_AID_INI & 0xFF, pAidingData, sizeof(GPS_UBX_AID_INI_U5__t));
#if 0
    unsigned char *pMsg = (unsigned char *) pAidingData;
    for (unsigned int i=0; i<sizeof(GPS_UBX_AID_INI_U5__t); i++)
    {
        LOGV("%.2X", pMsg[i]);
    }
#endif
}

void CUbxGpsState::sendUtcModel(GPS_UBX_AID_HUI_t *pUtcModel)
{
	writeUbx(UBXID_AID_HUI >> 8, UBXID_AID_HUI & 0xFF, pUtcModel, sizeof(GPS_UBX_AID_HUI_t));
}

#endif



/*******************************************************************************
 * UBX PROTOCOL HELPERS
 *******************************************************************************/

//! calculate the ubx checksum / fletecher-16 on a buffer
/*! calculate the ubx checksum / fletecher-16 on a buffer
	\param crc the checksum seed and result 
	\param pData the pointer to the buffer over which to calculate the crc
	\param iData the size of the buffer over which to calculate the crc
*/
void CUbxGpsState::crcUbx(unsigned char crc[2], const unsigned char* pData, int iData)
{
    unsigned int crcA = crc[0];
    unsigned int crcB = crc[1];
    if (iData > 0)
    {
		do 
		{
			crcA += *pData++;
			crcB += crcA;
		} 
		while (--iData);
    }
    crc[0] = (unsigned char)crcA;
    crc[1] = (unsigned char)crcB;
}

//! Write a UBX message to the receiver
/*! Write a UBX message to the receiver with a specific class/message id and for a certain payload. 

	\param clsId    the class Id of the message to be sent
	\param msgId    the message Id of the message to be sent
	\param pData0   the pointer to the first part of the payload to be sent (can be NULL)
	\param iData0   the size of the first part of the payload to be sent (can be 0)
	\param pData1   the pointer to the second part of the payload to be sent (can be NULL)
	\param iData1   the size of the second part of the payload to be sent (can be 0)
	\return         true if sucessfull, false otherwise 
*/
bool CUbxGpsState::writeUbx(unsigned char classID, unsigned char msgID, 
						    const void* pData0, int iData0, const void* pData1, int iData1)
{
	unsigned char head[6];
	unsigned char crc[2] = {0,0};
	int iData = iData0 + iData1;
    unsigned char udpBuf[iData + 8];
    unsigned char *pUdpBuf = udpBuf;
	// assemble and create the header
	head[0] = 0xB5;
    head[1] = 'b';
    head[2] = classID;
    head[3] = msgID;
    head[4] = (unsigned char)iData;
    head[5] = (unsigned char)(iData >> 8);
	if (m_pSer->writeSerial(head, sizeof(head)) != sizeof(head))
		return false;
    crcUbx(crc, &head[2], sizeof(head)-2);

    memcpy(pUdpBuf, head, sizeof(head));
    pUdpBuf+=sizeof(head);

	// add/send the first part of payload 
	if ((pData0 != NULL) && (iData0 > 0))
	{
		if (m_pSer->writeSerial((void *) pData0, iData0) != iData0)
			return false;
		crcUbx(crc, (const unsigned char*)pData0, iData0);
	}

    memcpy(pUdpBuf, pData0, iData0);
    pUdpBuf+=iData0;

	// add/send the second part of payload 
	if ((pData1 != NULL) && (iData1 > 0))
	{
		if (m_pSer->writeSerial((void *) pData1, iData1) != iData1)
			return false;
		crcUbx(crc, (const unsigned char*)pData1, iData1);
	}

    memcpy(pUdpBuf, pData1, iData1);
    pUdpBuf+=iData1;

	// and finally send the crc
	if (m_pSer->writeSerial(crc, sizeof(crc)) != sizeof(crc))
		return false;

    memcpy(pUdpBuf, crc, sizeof(crc));

#if defined UDP_SERVER_PORT
    m_pUdpServer->sendPort(udpBuf, sizeof(udpBuf));
#endif
	// LOGV("Send UBX-%02X-%02X size %d+8 ok", classID, msgID, iData);
	return true;
}


/*******************************************************************************
 * FILE SAVING & RESTORING
 *******************************************************************************/

//! Loads the temporary stored aiding data
/*! Loads the temporay file containing the last know aiding data, and transfers
    it to the receiver. 

    \return true if all aiding data sucessfully loaded, false otherwise 
*/
bool CUbxGpsState::loadAiding(void)
{
    if ((m_Db.dbAssistCleared) && (m_persistence))
    {
        LOGV("%s : Loading last aiding data from %s", __FUNCTION__, m_pAlpTempFile);
        
        int fd = open(m_pAlpTempFile, O_RDONLY);
        
        if (fd == -1)
        {
            // Failed
            LOGV("%s : Can not load aiding data from %s : %i", __FUNCTION__, m_pAlpTempFile, errno);
            return false;
        }
        
        if (loadDatabase(fd, &m_Db))
        {
            m_Db.dbAssistCleared = false;
            LOGV("%s : Aiding data loaded successfully", __FUNCTION__);
        }
        else
        {
            LOGV("%s : Failed to load aiding data", __FUNCTION__);
        }
        
        close(fd);
    }
    else
    {
        LOGV("%s : Not loaded - DB not cleared", __FUNCTION__);
    }
    
    
    return true;
}

//! Reads data from a file into a DB_s structure
/*! The DB_s structure is populated one field at a time.
    To populate each field, a corresponding 'loadBuffer' function is called for handling that field type. 

	\param fd the file handle of the file to read
	\param pDb a pointer to the structure to populate
	\return true if all fields were populated, false otherwise 
*/
bool CUbxGpsState::loadDatabase(int fd, struct DB_s* pDb)
{
    LOGV("%s : Loading DB 'hui'", __FUNCTION__);
    if (!loadBuffer(fd, &pDb->hui))
    {
        LOGV("%s : Failed to load database 'hui' field", __FUNCTION__);
        return false;
    }
    
    LOGV("%s : Loading DB 'apl'", __FUNCTION__);
    if (!loadBuffer(fd, &pDb->alp))
    {
        LOGV("%s : Failed to load database 'alp' field", __FUNCTION__);
        return false;
    }

    LOGV("%s : Loading DB 'pos'", __FUNCTION__);
    if (!loadBuffer(fd, (unsigned char*) &pDb->pos, sizeof(pDb->pos)))
    {
        LOGV("%s : Failed to load database 'pos' field", __FUNCTION__);
        return false;
    }

    LOGV("%s : Loading DB 'time'", __FUNCTION__);
    if (!loadBuffer(fd, (unsigned char*) &pDb->time, sizeof(pDb->time)))
    {
        LOGV("%s : Failed to load database 'time' field", __FUNCTION__);
        return false;
    }

    // Save ok so far, so continue
    for(int i = 0; i < NUM_GPS_SVS; i++)
    {
        //LOGV("%s : Loading DB 'sv'[%i]", __FUNCTION__, i);
        if (!loadBuffer(fd, &pDb->sv[i]))
        {
            LOGV("%s : Failed to load database sv[%i] field", __FUNCTION__, i);
            return false;               // Failed, give up
        }
    }
       
    LOGV("%s : Aiding database loaded", __FUNCTION__);
    return true;
}

//! Reads data from a file into a BUF_t structure
/*! The BUF_t structure is populated one field at a time.
    To populate each field, a corresponding 'loadBuffer' function is called for handling that field type. 

	\param fd the file handle of the file to read
	\param pBuffer a pointer to the structure to populate
	\return true if all fields were populated, false otherwise 
*/
bool CUbxGpsState::loadBuffer(int fd, BUF_t* pBuffer)
{
    unsigned int length = 0;
    
    //LOGV("%s : Loading BUF_t 'i'", __FUNCTION__);
    //LOGV("%s : Initial Buf  p %p  i %i", __FUNCTION__, pBuffer->p, pBuffer->i);
    if (!loadBuffer(fd, (unsigned char*) &length, sizeof(length)))
    {
        LOGV("%s : Failed to load buffer length", __FUNCTION__);
        return false;
    }
    
    unsigned char* p = NULL;
    if (length > 0)
    {
        p = (unsigned char*) malloc(length);
        if (!p)
        {
            LOGV("%s : Failed to allocate buffer", __FUNCTION__);
            return false;
        }
        //LOGV("%s : Loading BUF_t 'p'", __FUNCTION__);
        if (!loadBuffer(fd, p, length))
        {
            LOGV("%s : Failed to load buffer", __FUNCTION__);
            free(p);
            return false;
        }
    }
    
    free(pBuffer->p);
    pBuffer->p = p;
    pBuffer->i = length;

    return true;
}

//! Reads data from a file into a DBALP_t structure
/*! The DBALP_t structure is populated one field at a time.
    To populate each field, a corresponding 'loadBuffer' function is called for handling that field type. 

	\param fd the file handle of the file to read
	\param pBuffer a pointer to the structure to populate
	\return true if all fields were populated, false otherwise 
*/
bool CUbxGpsState::loadBuffer(int fd, DBALP_t* pBuffer)
{
    //LOGV("%s : Loading DBALP_t 'file'", __FUNCTION__);
    if (!loadBuffer(fd, &pBuffer->file))
    {
        LOGV("%s : Failed to load DBALP_t 'file' field", __FUNCTION__);
        return false;
    }
    
    //LOGV("%s : Loading DBALP_t 'fileId'", __FUNCTION__);
    if (!loadBuffer(fd, (unsigned char*) &pBuffer->fileId, sizeof(pBuffer->fileId)))
    {
        LOGV("%s : Failed to load DBALP_t 'fileId' field", __FUNCTION__);
        return false;
    }
    
    //LOGV("%s : Loading DBALP_t 'timeRefMs'", __FUNCTION__);
    if (!loadBuffer(fd, (unsigned char*) &pBuffer->timeRefMs, sizeof(pBuffer->timeRefMs)))
    {
        LOGV("%s : Failed to load DBALP_t 'timeRefMs' field", __FUNCTION__);
        return false;
    }
    
    return true;
}

//! Reads data from a file into a DBSV_t structure
/*! The DBSV_t structure is populated one field at a time.
    To populate each field, a corresponding 'loadBuffer' function is called for handling that field type. 

	\param fd the file handle of the file to read
	\param pBuffer a pointer to the structure to populate
	\return true if all fields were populated, false otherwise 
*/
bool CUbxGpsState::loadBuffer(int fd, DBSV_t* pBuffer)
{
    //LOGV("%s : Loading DBSV_t 'eph'", __FUNCTION__);
    if (!loadBuffer(fd, &pBuffer->eph))
    {
        LOGV("%s : Failed to load DBSV_t 'eph' field", __FUNCTION__);
        return false;
    }
    
    //LOGV("%s : Loading DBSV_t 'alm'", __FUNCTION__);
    if (!loadBuffer(fd, &pBuffer->alm))
    {
        LOGV("%s : Failed to load DBSV_t 'alm' field", __FUNCTION__);
        return false;
    }

    //LOGV("%s : Loading DBSV_t 'aop'", __FUNCTION__);
    if (!loadBuffer(fd, &pBuffer->aop))
    {
        LOGV("%s : Failed to load DBSV_t 'aop' field", __FUNCTION__);
        return false;
    }

    return true;
}

//! Reads data from a file into memory
/*! This is the lowest level loadBuffer fuction which reads the file and puts the data into a memory buffer
    which typically repesents a field within a bigger structure.

	\param fd the file handle of the file to read
	\param pBuffer a pointer to memory to populate
    \param length the number of bytes to read into the buffer
	\return true if all bytes were read, false otherwise 
*/
bool CUbxGpsState::loadBuffer(int fd, unsigned char* pBuffer, int length)
{
    //LOGV("%s : Loading stream of %i bytes", __FUNCTION__, length);
    if (read(fd, pBuffer, length) == length)
        return true;
        
    return false;
}

//! Saves aiding data to a temporary file.
/*! Saves the current aiding data to a temporary file, ready to be loaded back into
    the receiver the next time it is started.
*/
void CUbxGpsState::saveAiding(void)
{
    if ((m_Db.dbAssistChanged) && (m_persistence))
    {
        LOGV("%s : Saving aiding data to %s", __FUNCTION__, m_pAlpTempFile);

        int fd = open(m_pAlpTempFile, O_CREAT | O_WRONLY | O_TRUNC);
        
        if (fd == -1)
        {
            // failed
            LOGV("%s : Can not save aiding data to %s : %i", __FUNCTION__, m_pAlpTempFile, errno);
            return;
        }
  
        // Save the database
        if (saveDatabase(fd, &m_Db))
        {
            // Success
            close(fd);
            chmod(m_pAlpTempFile, S_IRUSR | S_IWUSR);
            LOGV("%s : Aiding data saved succesfully", __FUNCTION__);
            
            m_Db.dbAssistChanged = false;
        }
        else
        {
            // Failed
            close(fd);
            remove(m_pAlpTempFile);
            LOGV("%s : Aiding data save failed - temp file removed", __FUNCTION__);
        }
    }
    else
    {
        LOGV("%s : Not saved - DB not changed", __FUNCTION__);
    }
}

//! Write data to a file into a DB_s structure
/*! The DB_s structure is written one field at a time.
    To write each field, a corresponding 'saveBuffer' function is called for handling that field type. 

	\param fd the file handle of the file to write too
	\param pDb a pointer to the structure to write
	\return true if all fields were written, false otherwise 
*/
bool CUbxGpsState::saveDatabase(int fd, struct DB_s* pDb)
{
    LOGV("%s : saving DB 'hui'", __FUNCTION__); 
    if (!saveBuffer(fd, &pDb->hui))
    {
        LOGV("%s : Failed to save database hui field", __FUNCTION__);
        return false;
    }  
    
    LOGV("%s : saving DB 'alp'", __FUNCTION__); 
    if (!saveBuffer(fd, &pDb->alp))
    {
        LOGV("%s : Failed to save database alp field", __FUNCTION__);
        return false;
    }

    LOGV("%s : saving DB 'pos'", __FUNCTION__); 
    if (!saveBuffer(fd, (unsigned char*) &pDb->pos, sizeof(pDb->pos)))
    {
        LOGV("%s : Failed to save database pos field", __FUNCTION__);
        return false;
    }
    
    LOGV("%s : saving DB 'time'", __FUNCTION__); 
    if (!saveBuffer(fd, (unsigned char*) &pDb->time, sizeof(pDb->time)))
    {
        LOGV("%s : Failed to save database time field", __FUNCTION__);
        return false;
    }
 
    for(int i = 0; i < NUM_GPS_SVS; i++)
    {
        LOGV("%s : saving DB 'sv'[%i]", __FUNCTION__, i); 
        if (!saveBuffer(fd, &pDb->sv[i]))
        {
            LOGV("%s : Failed to save database sv[%i] field", __FUNCTION__, i);
            return false;               // Failed, give up
        }
    }
    
    return true;
}

//! Write data to a file from a DBSV_t structure
/*! Each field in the DBSV_t structure is individually written to the file.
    To write each field, a corresponding 'saveBuffer' function is called for handling that field type. 

	\param fd the file handle of the file to write too
	\param pBuffer a pointer to the structure to write to the file
	\return true if all fields were written, false otherwise 
*/
bool CUbxGpsState::saveBuffer(int fd, DBSV_t* pBuffer)
{
    //LOGV("%s : saving DBSV_t 'eph'", __FUNCTION__); 
    if (!saveBuffer(fd, &pBuffer->eph))
    {
        LOGV("%s : Failed to save DBSV_t 'eph' field", __FUNCTION__);
        return false;

    }
    
    //LOGV("%s : saving DBSV_t 'alm'", __FUNCTION__); 
    if (!saveBuffer(fd, &pBuffer->alm))
    {
        LOGV("%s : Failed to save DBSV_t 'alm' field", __FUNCTION__);
        return false;
    }
        
    //LOGV("%s : saving DBSV_t 'aop'", __FUNCTION__); 
    if (!saveBuffer(fd, &pBuffer->aop))
    {
        LOGV("%s : Failed to save DBSV_t 'aop' field", __FUNCTION__);
        return false;
    }
    
    return true;
}

//! Write data to a file from a BUF_t structure
/*! Each field in the BUF_t structure is individually written to the file.
    To write each field, a corresponding 'saveBuffer' function is called for handling that field type. 

	\param fd the file handle of the file to write too
	\param pBuffer a pointer to the structure to write to the file
	\return true if all fields were written, false otherwise 
*/
bool CUbxGpsState::saveBuffer(int fd, BUF_t* pBuffer)
{
    //LOGV("%s : saving BUF_t 'i'", __FUNCTION__); 
    if (!(saveBuffer(fd, (unsigned char*) &pBuffer->i, sizeof(pBuffer->i))))
    {
        LOGV("%s : Failed to save BUF_t length", __FUNCTION__);
        return false;
    }

    //LOGV("%s : saving BUF_t 'p'", __FUNCTION__);     
    if (!saveBuffer(fd, pBuffer->p, pBuffer->i))
    {
        LOGV("%s : Failed to save BUF_t buffer", __FUNCTION__);
        return false;
    }
    
    return true;
}

//! Write data to a file from a DBALP_t structure
/*! Each field in the DBALP_t structure is individually written to the file.
    To write each field, a corresponding 'saveBuffer' function is called for handling that field type. 

	\param fd the file handle of the file to write too
	\param pBuffer a pointer to the structure to write to the file
	\return true if all fields were written, false otherwise 
*/
bool CUbxGpsState::saveBuffer(int fd, DBALP_t* pBuffer)
{
    //LOGV("%s : saving DBALP_t 'file'", __FUNCTION__); 
    if (!saveBuffer(fd, &pBuffer->file))
    {
        LOGV("%s : Failed to save DBALP_t 'file' field", __FUNCTION__);
        return false;
    }
        
    //LOGV("%s : saving DBALP_t 'fileId'", __FUNCTION__); 
    if (!saveBuffer(fd, (unsigned char*) &pBuffer->fileId, sizeof(pBuffer->fileId)))
    {
        LOGV("%s : Failed to save DBALP_t 'fileId' field", __FUNCTION__);
        return false;
    }
    
    //LOGV("%s : saving DBALP_t 'timeRefMs'", __FUNCTION__); 
    if (!saveBuffer(fd, (unsigned char*) &pBuffer->timeRefMs, sizeof(pBuffer->timeRefMs)))
    {
        LOGV("%s : Failed to save DBALP_t 'timeRefMs' field", __FUNCTION__);
        return false;
    }
    
    return true;
}

//! Write data from memory to a file
/*! This is the lowest level loadBuffer fuction which reads the file and puts the data into a memory buffer
    which typically repesents a field within a bigger structure.

	\param fd the file handle of the file to write too
	\param pBuffer a pointer to memory containing data to write
    \param length the number of bytes to write to the file
	\return true if all bytes were written, false otherwise 
*/
bool CUbxGpsState::saveBuffer(int fd, unsigned char* pBuffer, int length)
{
    //LOGV("%s : saving buffer of length %i", __FUNCTION__, length);
    if (length > 0)
    {
        if (write(fd, pBuffer, length) != length)
            return false;
    }
    
    return true;
}


/*******************************************************************************
 * POWER
 *******************************************************************************/

//! Switch the power on to the receiver.
bool CUbxGpsState::powerOn(void)
{  
    int val = 1; 
    int gpsctl_fd;
    
    gpsctl_fd = open("/dev/gpsctl",O_WRONLY);
    if(gpsctl_fd){
              if (-1 == ioctl(gpsctl_fd, GPSCTL_IOC_SET_POWER, &val)) {
	             LOGE("GPSCTL_IOC_SET_POWER fail - %s",strerror(errno));
             }    
             LOGV("power on ok");
    } else{
	     LOGE("open /dev/gpsctl fail");
    }    
    close(gpsctl_fd);

    return true;		// Return true if power was successfully turned on
}

//! Switch the power off to the receiver.
void CUbxGpsState::powerOff(void)
{
    int val = 0; 
    int gpsctl_fd; 

    gpsctl_fd = open("/dev/gpsctl",O_WRONLY);
    if(gpsctl_fd) {
               if (-1 == ioctl(gpsctl_fd, GPSCTL_IOC_SET_POWER, &val)) {
                      LOGE("GPSCTL_IOC_SET_POWER fail");
               }    
             LOGV("power off ok");
    } else {
            LOGE("open /dev/gpsctl fail");
    }    
    close(gpsctl_fd);
}
