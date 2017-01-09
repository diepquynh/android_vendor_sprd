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
  \brief
*/
/*******************************************************************************
 * $Id: gps_thread.cpp 62713 2012-10-24 07:29:13Z jon.bowern $
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/time.h>
#include <assert.h>

#include "ubx_log.h"
#include "std_types.h"
#include "std_lang_def.h"
#include "std_macros.h"

#include "ubx_moduleIf.h"
#include "ubx_rilIf.h"
#include "ubx_xtraIf.h"
#include "ubx_agpsIf.h"
#include "ubx_timer.h"

#include "parserbuffer.h"
#include "protocolubx.h"
#include "protocolnmea.h"
#include "protocolunknown.h"

#include "ubxgpsstate.h"
#include "gps_thread.h"
#include "ubx_localDb.h"

#include "ubx_niIf.h"

#ifdef SUPL_ENABLED
#include "suplSMmanager.h"
#include <openssl/hmac.h>
#include "openssl/ssl.h"
#include <openssl/err.h>
#include <openssl/engine.h>
#include <openssl/conf.h>
#endif

////////////////////////////////////////////////////////////////////////////////
// Definitions & Types
#define MAX_UDP_PACKET_LEN 16384    //!< Dimension of the temporary buffer for reading a complete UDP packet
#define MIN_INTERVAL        1000     //!< Minimum interval time (in ms) receiver is capable of

// Debugging
#ifdef SUPL_ENABLED
//#define NI_TEST
#endif

///////////////////////////////////////////////////////////////////////////////
// Local functions
static void requestStartEventHandler(void* pContext);
static void requestStopEventHandler(void* pContext);

///////////////////////////////////////////////////////////////////////////////
// Local Data



static CSerialPort s_ser;			//!< Hardware interface class instance(serial port / usb file handle)
#if defined UDP_SERVER_PORT
static CUdpServer s_udp;			//!< UPD server class instance
#endif

static GpsControlEventInterface s_eventHandler =		//!< Gps control event interface implementation
{
	requestStart_cb:	requestStartEventHandler,		//!< 'Start' event handler
	requestStop_cb:		requestStopEventHandler			//!< 'Stop' event handler
};

///////////////////////////////////////////////////////////////////////////////
// Golbal Data
#ifndef UNDEBUG
pthread_t g_gpsDrvMainThread = 0;		// Thread debugging
#endif

///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// Control support

//! Signals to any waiting threads the last command has completed
/*!
  \param pControlThreadInfo	: Pointer to main thread data
  \param result				: Last command result to make available to interested parties
*/
static void signal_cmd_complete(ControlThreadInfo* pControlThreadInfo, int result)
{
    pControlThreadInfo->cmdResult = result;
    pthread_mutex_lock(&pControlThreadInfo->threadCmdCompleteMutex);
    pthread_cond_signal(&pControlThreadInfo->threadCmdCompleteCond);
    pthread_mutex_unlock(&pControlThreadInfo->threadCmdCompleteMutex);
}

static void engineStop(ControlThreadInfo* pControlThreadInfo)
{
	pthread_mutex_lock(&pControlThreadInfo->threadDataAccessMutex);
	LOGV("%s: (Begin) Client count %d", __FUNCTION__, pControlThreadInfo->clientCount);
	
	if (pControlThreadInfo->clientCount > 0)
	{
		pControlThreadInfo->clientCount--;

		if (pControlThreadInfo->clientCount == 0)
		{
			CUbxGpsState* pUbxGps = CUbxGpsState::getInstance();
			pUbxGps->lock();
			pUbxGps->onPrepareShutdown();
			pUbxGps->unlock();
			
			pControlThreadInfo->gpsState = GPS_STOPPING;
			pControlThreadInfo->stoppingTimeoutMs = getMonotonicMsCounter() + pUbxGps->getStoppingTimeoutMs();

			if (s_ser.isFdOpen())
			{
				CGpsIf::gpsStatus(GPS_STATUS_SESSION_END);
			}
		}
	}
	LOGV("%s: (End) Client count %d", __FUNCTION__, pControlThreadInfo->clientCount);
	pthread_mutex_unlock(&pControlThreadInfo->threadDataAccessMutex);
	
    // Do NOT signal command complete
    // This is send after response from device or timeout
}

static void handle_init(ControlThreadInfo* pControlThreadInfo)
{
	LOGV("%s (%u): Init state.", __FUNCTION__, (unsigned int) pthread_self());
    pControlThreadInfo->gpsState = GPS_STOPPED;
    signal_cmd_complete(pControlThreadInfo, 1);
}

static void handle_stopped(ControlThreadInfo* pControlThreadInfo)
{
	pthread_mutex_lock(&pControlThreadInfo->threadDataAccessMutex);
	LOGV("%s: (Begin)", __FUNCTION__);
	CUbxGpsState* pUbxGps = CUbxGpsState::getInstance();
	pUbxGps->lock();
    pUbxGps->onShutDown();
    pUbxGps->unlock();
    pControlThreadInfo->gpsState = GPS_STOPPED;
    CGpsIf::gpsStatus(GPS_STATUS_ENGINE_OFF);
    LOGV("%s: (End)", __FUNCTION__);
	pthread_mutex_unlock(&pControlThreadInfo->threadDataAccessMutex);
}

static void engineStart(ControlThreadInfo* pControlThreadInfo)
{
	pthread_mutex_lock(&pControlThreadInfo->threadDataAccessMutex);
	LOGV("%s: (Begin) Client count %d", __FUNCTION__, pControlThreadInfo->clientCount);
	pControlThreadInfo->clientCount++;

	if (pControlThreadInfo->clientCount == 1)
	{
		// Reset database as it will be out of date
		CMyDatabase::getInstance()->Reset();
		
		CUbxGpsState* pUbxGps = CUbxGpsState::getInstance();

		pUbxGps->lock();
		pUbxGps->onStartup();
		// agps / assist now online
		pUbxGps->checkAgpsDownload();
		pUbxGps->unlock();
		
		// request time from the ntp server
	#if (PLATFORM_SDK_VERSION >= 14 /* =4.0 */)
		CGpsIf::requestUtcTime();
	#endif

		/* inside the thread we already check alp file expiry
		   CXtraIf::requestDownload()
		   LOGE("%s : No AGPS xtra Callbacks", __FUNCTION__);
		*/ 
		
		pControlThreadInfo->gpsState = GPS_STARTED;
		pUbxGps->clearReceiverShutdownAck();
	 
		if (s_ser.isFdOpen())
		{
			CGpsIf::gpsStatus(GPS_STATUS_SESSION_BEGIN);
		}
		
#ifdef NI_TEST
		CNiIf::request(5000, NotificationType_notificationAndVerficationAllowedNA);
#endif
		
#ifdef SUPL_ENABLED
		GpsPositionMode posMode = CGpsIf::getInstance()->getMode();
		if (((posMode == GPS_POSITION_MODE_MS_BASED) || (posMode == GPS_POSITION_MODE_MS_ASSISTED)) &&
			(!suplActiveSessions()))
		{
			suplStartSetInitiatedAction();
		}
#endif
	}
	LOGV("%s: (End) Client count %d", __FUNCTION__, pControlThreadInfo->clientCount);
	pthread_mutex_unlock(&pControlThreadInfo->threadDataAccessMutex);
}

static bool handle_cmd(ControlThreadInfo* pControlThreadInfo, U1 cmd)
{
    bool exit = false;

	switch(cmd)
	{
		case CMD_START:
			engineStart(pControlThreadInfo);
			break;
			
		case CMD_STOP:
			engineStop(pControlThreadInfo);
			break;

#ifndef ANDROID_BUILD
		case CMD_EXIT:
			exit = true;
			break;
#endif
		default:
			LOGE("%s : Unexpected command %i", __FUNCTION__, cmd);
			break;            
	}

    return exit;
}

///////////////////////////////////////////////////////////////////////////////
//! Handle the device shut down process
/*! During the 'stopping' / device shutdown state/phase, this function checks
    to see if all the device acknowledgements have been received or the shutdown
    timeout has expired. If either of these conditions are true, the gps is put
    into the 'stopped' state.
  \param pControlThreadInfo	: Pointer to main control thread data
*/
static void handle_device_shutdown(ControlThreadInfo* pControlThreadInfo)
{
    int timeOut = (getMonotonicMsCounter() > pControlThreadInfo->stoppingTimeoutMs);
    CUbxGpsState* pUbxGps = CUbxGpsState::getInstance();
	
    if (pUbxGps->getReceiverShutdownAck() || timeOut)
    {
        LOGD("%s : All aiding data received %i    timeout %i",
             __FUNCTION__, 
             pUbxGps->getReceiverShutdownAck(), 
             timeOut);
        handle_stopped(pControlThreadInfo);
    }
}

static void reportStatus(ControlThreadInfo* pState)
{
	switch (pState->gpsState)
	{
		case GPS_STARTED:
			CGpsIf::gpsStatus(GPS_STATUS_SESSION_BEGIN);
			break;

		case GPS_STOPPING:
			CGpsIf::gpsStatus(GPS_STATUS_SESSION_END);
			break;

		case GPS_STOPPED:
			CGpsIf::gpsStatus(GPS_STATUS_ENGINE_OFF);
			break;

		default:
			// Shouldn't happen
			assert(0);
			break;
	}
}

static bool handleCmdInput(ControlThreadInfo* pState, fd_set* pRfds)
{
	bool finish = false;
	
	if(FD_ISSET(pState->cmdPipes[0], pRfds))
	{
		// Command received
		U1 cmd;
		if (read(pState->cmdPipes[0], &cmd, 1) == -1)
		{
			LOGE("%s : Cmd pipe read failure (%i)", __FUNCTION__, errno);
		}
		else
		{
			LOGV("%s (%u): Cmd received (%i)", __FUNCTION__, (unsigned int) pthread_self(), cmd);
			if (handle_cmd(pState, cmd))
			{
#ifndef ANDROID_BUILD                    
				LOGV("%s : Exit thread cmd received", __FUNCTION__);
				finish = true;
#endif
			}
		}
	}
	
	return finish;
}

#if defined UDP_SERVER_PORT
static void handleUdpInput(fd_set &rfds)
{
	if (s_udp.fdIsSet(rfds))
	{
		char tmpbuf[MAX_UDP_PACKET_LEN];
		int len = s_udp.recvPort(tmpbuf, sizeof(tmpbuf));
//                LOGV("%s: Received something over UDP! %d", __FUNCTION__, len);
		if (len > 0)
		{
			// UBX or PUBX data - forward to GPS
			if (s_ser.writeSerial(tmpbuf,len) != len)
			{
				LOGE("unable to write %i to a master",len);
			}
		}
	}
}
#endif

static void connectReceiver(ControlThreadInfo* pState, fd_set& rfds, int& rMaxFd)
{
	CUbxGpsState* pUbxGps = CUbxGpsState::getInstance();
	
	LOGV("%s: Open/Reopen the serial port", __FUNCTION__); 
	/* try to open/reopen the serial port for the GPS listener */
	if (!s_ser.openSerial(pUbxGps->getSerialDevice(), pUbxGps->getBaudRateDefault(), 1))
	{
		// Device not present (unplugged)
		CGpsIf::gpsStatus(GPS_STATUS_ENGINE_OFF);
		LOGE("Failing to reopen the serial port"); 
	}
	else
	{
		// Serial port opened/reopened - Baud rate needs to be set
		pUbxGps->setBaudRate();
		s_ser.fdSet(rfds, rMaxFd);

		CUbxGpsState* pUbxGps = CUbxGpsState::getInstance();
		pUbxGps->lock();
		if (pState->gpsState == GPS_STARTED)
		{
			LOGV("%s: Start receiver ", __FUNCTION__); 
			pUbxGps->onInit();
		}
		else
		{
			LOGV("%s: Init receiver ", __FUNCTION__); 
			pUbxGps->onInit();
		}
		pUbxGps->unlock();
		
		// Report appropriate status
		reportStatus(pState);
	}
}

//--------------------------------------------------------------------------------

static void releaseGpsThreadResources(ControlThreadInfo* pControlThreadInfo)
{
    s_ser.closeSerial();
#if defined UDP_SERVER_PORT
    s_udp.closeUdp();
#endif
    if (pControlThreadInfo->cmdPipes[0] != -1) 
		close(pControlThreadInfo->cmdPipes[0]);
    pControlThreadInfo->cmdPipes[0] = -1;
    if (pControlThreadInfo->cmdPipes[1] != -1) 
		close(pControlThreadInfo->cmdPipes[1]);
    pControlThreadInfo->cmdPipes[1] = -1;
}

///////////////////////////////////////////////////////////////////////////////
//! Main gps driver control thread
/*!
  \param pThreadData	: Pointer to thread data
*/
void ubx_thread(void *pThreadData)
{
    ControlThreadInfo* pState = (ControlThreadInfo*) pThreadData;    

	assert(g_gpsDrvMainThread == 0);
#ifndef UNDEBUG	
	g_gpsDrvMainThread = pthread_self();			// For debugging threads
#endif
	time_t now = time(NULL);
	CMyDatabase* pDatabase = CMyDatabase::getInstance();
	
#if defined UDP_SERVER_PORT
    time_t timeoutPts = now;      			//!< for virtual serial status check
#endif 
    time_t timeoutLastValidMessage = now;	//!< timeout for serial port 
	time_t timeoutLastXtraRequest = 0;

	pDatabase->setGpsState(pState);
	
    CProtocolUBX  protocolUBX;
    CProtocolNMEA protocolNmea;
    CProtocolUnknown protocolUnknown;
    CParserBuffer parser;               // declare after protocols, so destructor called before protocol destructors

    // setup parser buffer
    parser.Register(&protocolUBX);
    parser.Register(&protocolNmea);
    parser.RegisterUnknown(&protocolUnknown);

    LOGV("%s (%u): Gps background thread started", __FUNCTION__, (unsigned int) pthread_self()); 
	CUbxGpsState* pUbxGps = CUbxGpsState::getInstance();
    
#ifdef SUPL_ENABLED	
	suplRegisterEventCallbacks(&s_eventHandler, pState);
    OpenSSL_add_all_algorithms();
    SSL_library_init();
	LOGV("%s: SSL initialised", __FUNCTION__); 
#endif

	pUbxGps->setSerial(&s_ser);
#if defined UDP_SERVER_PORT
	pUbxGps->setUdp(&s_udp);
#endif	

#if defined UDP_SERVER_PORT
    if ((s_udp.openLocalPort(pUbxGps->getUdpPort())) < 0)
    {
        LOGE("unable to open local port");
        LOGE("Exiting the thread"); 
        releaseGpsThreadResources(pState);
        signal_cmd_complete(pState, 0);      // Signal init fail
        return;
    }
#endif

    if (pipe(pState->cmdPipes) == -1)
    {
        LOGE("%s : Could not create cmd pipes (%i)", __FUNCTION__, errno);
        releaseGpsThreadResources(pState);
        signal_cmd_complete(pState, 0);      // Signal init fail
        return;
    }

    handle_init(pState);    // also turn off the device when the thread starts
                            // and complete (signal init handler function)

    while(1)
    {
        fd_set rfds;
        int maxFd = 0;

        /* Initialize the select mask */
        FD_ZERO(&rfds);

        /* selecting the gps device & cmd input queue */
        if (!s_ser.fdSet(rfds, maxFd))
        {
			// Serial channel to receiver not open
			connectReceiver(pState, rfds, maxFd);
        }
        FD_SET(pState->cmdPipes[0], &rfds);  

        if (pState->cmdPipes[0]+1 > maxFd) 
            maxFd = pState->cmdPipes[0]+1; 
        
#if defined UDP_SERVER_PORT
        /* Selecting UDP port connections */
        s_udp.fdSet(rfds, maxFd);
#endif

#ifdef SUPL_ENABLED
		suplAddUplListeners(&rfds, &maxFd);
#endif
        /* make the select */
		struct timeval tv;		/* and setup the timeout to 1.0 seconds */
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        int res = select(maxFd, &rfds, NULL, NULL, &tv);
        now = time(NULL);
        
#if defined UDP_SERVER_PORT
        if ((now - timeoutPts) >= 1)
        {
            /* Check UDP connections */
            s_udp.checkPort(0);

            /* update pseudoterminal timeout */
            timeoutPts = now;
        }
#endif
        
        if (res > 0)
        {
            // There is some input in the serial port
            if (s_ser.fdIsSet(rfds))
            {
                // fill the parser with new data 
                int iSize = s_ser.readSerial(parser.GetPointer(), parser.GetSpace());
				
                if (iSize)
                {
				    int iMsg;
					unsigned char* pMsg;
				    CProtocol* pProtocol;

                    // we read it so update the size
                    parser.Append(iSize);
                    // Parse ...
                    while (parser.Parse(pProtocol, pMsg, iMsg))
                    {
#if defined UDP_SERVER_PORT
                        /* put the UBX token */
                        s_udp.sendPort(pMsg, iMsg);
#endif                            
  						// redirecting
						if (pProtocol == &protocolUBX)
						{
					        timeoutLastValidMessage = now;
                            //LOGV("MSG UBX %02X-%02X-%02X-%02X-%02X-%02X (size %d)\n", pMsg[2], pMsg[3], pMsg[4], pMsg[5], pMsg[6], pMsg[7], iMsg);
                            pUbxGps->lock();
							pUbxGps->onNewUbxMsg(pState->gpsState, pMsg, iMsg);
							pUbxGps->unlock();
						}
                        else if (pProtocol == &protocolNmea)
						{
							timeoutLastValidMessage = now;
                            if ((CGpsIf::getInstance()->m_callbacks.nmea_cb) && 
                                (getMonotonicMsCounter() >= pDatabase->getNextReportEpoch()))
                            {
#if 1   
                                struct timeval tv;
                                gettimeofday(&tv, NULL);
                                GpsUtcTime gpsUtcTime = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
#else   
                                GpsUtcTime gpsUtcTime = pDatabase->GetGpsUtcTime();
#endif  
                                CGpsIf::getInstance()->m_callbacks.nmea_cb(gpsUtcTime, (const char*)pMsg, iMsg);
                            }
                            //LOGV("MSG %*.*s\n", iMsg-2, iMsg-2, pMsg);
						}
                        else 
						{
                            LOGV("%s: MSG UNKNOWN size %d\n", __FUNCTION__, iMsg);
                        }
						// PRINTF("size %5d ", iMsg);
                        // ... and Process
                        pProtocol->Process(pMsg, iMsg, pDatabase);
                        parser.Remove(iMsg);
                    }
					
                    parser.Compact();
                }
                else
                {
                    LOGE( "%s: read error %d", __FUNCTION__, iSize);
                    s_ser.closeSerial();
                }
                
				// check if we have not received usefull message and may need to resync the baudrate
                int twiceRate = (2 * pUbxGps->getRate() / 1000);
                int validMsgThreshold = twiceRate < 1 ? 1 : twiceRate;
                int baudRate = pUbxGps->getBaudRate();
				
				if (((now - timeoutLastValidMessage) > validMsgThreshold) && 
                    (baudRate != pUbxGps->getBaudRateDefault()))
				{
                    LOGV("%s : Resetting serial baud rate - diff (%li)  threshold (%i)", __FUNCTION__, 
                         (now - timeoutLastValidMessage),
                         twiceRate);
					// switch to the original default baudrate (on init this is not needed)
					s_ser.setbaudrate(pUbxGps->getBaudRateDefault());
					usleep(50000);
					// reconfigure the baud rate 
					pUbxGps->setBaudRate();
                    
                    timeoutLastValidMessage = now;
				}
            }

#if defined UDP_SERVER_PORT
            /* UDP PORT READ HANDLING */
			handleUdpInput(rfds);
#endif /* UDP_SERVER_PORT */

#ifdef SUPL_ENABLED
			suplReadUplSock(&rfds);			// Check and process any incoming SUPL data
#endif
			if (handleCmdInput(pState, &rfds))
			{
				break;		// Will only happen when using test harness
			}
        }
        else 
		{
//            LOGV("%s : No input timeout", __FUNCTION__);
        }
#ifdef SUPL_ENABLED
        suplCheckPendingActions();		// Check for any actions on existing SUPL sessions
#endif
        if (pState->gpsState == GPS_STOPPING)
        {
            handle_device_shutdown(pState);
        }
        else if (pState->gpsState == GPS_STARTED)
        {
            // Request a new ALP file if it seems outdated
            pUbxGps->lock();
            bool ok = pUbxGps->checkAlpFile();
            pUbxGps->unlock();
            
            if (!ok && ((now - timeoutLastXtraRequest) >= 60/*one minute*/))
            {
				CXtraIf::requestDownload();
                timeoutLastXtraRequest = now;
            }
        }
    }
    
    // Should never get too.
#ifndef ANDROID_BUILD
	ERR_remove_state(0);
	EVP_cleanup();
	CRYPTO_cleanup_all_ex_data();
	ERR_free_strings();
	ENGINE_cleanup();
	CONF_modules_unload(1);
	CONF_modules_free();
	sk_SSL_COMP_free (SSL_COMP_get_compression_methods()); 
 
    LOGV("%s : Left main loop", __FUNCTION__);
    releaseGpsThreadResources(pState);
    LOGV("%s : Thread exiting", __FUNCTION__);
#endif	
    return;
}

///////////////////////////////////////////////////////////////////////////////
//! Injects a time into the receiver
/*!
  \param timeUtcGps		: Utc time
  \param timeReference	: Time reference
  \param uncertainty	: Uncertaincy
*/
void gps_state_inject_time(GpsUtcTime timeUtcGps, int64_t timeReference, int uncertainty)
{
	CUbxGpsState* pUbxGps = CUbxGpsState::getInstance();
	
    pUbxGps->lock();
    pUbxGps->putTime(timeUtcGps, timeReference, uncertainty);
    pUbxGps->writeUbxAidIni();// Send AID-INI command
	pUbxGps->unlock();
}

///////////////////////////////////////////////////////////////////////////////
//! Injects a location into the receiver
/*!
  \param latitude	: Latitude part of location
  \param longitude	: Longitude part of location
  \param accuracy	: Accuracy of location
*/
void gps_state_inject_location(double latitude, double longitude, float accuracy)
{
	CUbxGpsState* pUbxGps = CUbxGpsState::getInstance();

    pUbxGps->lock();
	pUbxGps->putPos(latitude, longitude, accuracy);
    pUbxGps->writeUbxAidIni();
	pUbxGps->unlock();
}

///////////////////////////////////////////////////////////////////////////////
//! Deletes aiding data in the receiver
/*!
  \param flags	: Flags indicating which data to delete
*/
void gps_state_delete_aiding_data(GpsAidingData flags)
{
	CUbxGpsState* pUbxGps = CUbxGpsState::getInstance();
	
    pUbxGps->lock();
	pUbxGps->deleteAidData(flags);
    pUbxGps->writeUbxCfgRst(0x02/*reset gps only*/, flags);
	pUbxGps->unlock();
}

///////////////////////////////////////////////////////////////////////////////
//! Sets the interval for the driver to report to the framework
/*!
  \param min_interval	: Reporting interval in ms
*/
void gps_state_set_interval(uint32_t min_interval)
{
    int64_t nextReportEpochMs = 0;     // default - no driver intervention
	int timeInterval = 0;
	
    if (min_interval < MIN_INTERVAL)
    {
        // Below minimun, set to receiver minimum, with no driver intervention
        min_interval = MIN_INTERVAL;
    }
    else if ((min_interval >= MIN_INTERVAL) && (min_interval <= 2000))
    {
        // Receiver can cope with these values.
    }
    else
    {
        // Too fast for receiver, so driver will intervene to extend
        LOGW("%s : Interval (%i) too big - Driver will intervene", __FUNCTION__, min_interval);
        timeInterval = min_interval;
        nextReportEpochMs = getMonotonicMsCounter();
        min_interval = 1000;
    }
    
	CMyDatabase* pDatabase = CMyDatabase::getInstance();
	pDatabase->setEpochInterval(timeInterval, nextReportEpochMs);
	
	CUbxGpsState* pUbxGps = CUbxGpsState::getInstance();
    pUbxGps->lock();
    pUbxGps->putRate(min_interval);
    pUbxGps->writeUbxCfgRate();
    pUbxGps->unlock();
}

///////////////////////////////////////////////////////////////////////////////
//! Initialises the main thread's control data structure
/*!
  \param pControlThreadInfo	: Pointer to main thread data
*/
void controlThreadInfoInit(ControlThreadInfo* pControlThreadInfo)
{
	memset(pControlThreadInfo, 0, sizeof(ControlThreadInfo));
	
    pControlThreadInfo->cmdPipes[0] = -1;
    pControlThreadInfo->cmdPipes[1] = -1;
    pControlThreadInfo->gpsState = GPS_UNKNOWN;

    pthread_mutex_init(&pControlThreadInfo->threadCmdCompleteMutex, NULL);
    pthread_cond_init(&pControlThreadInfo->threadCmdCompleteCond, NULL);
	
	pthread_mutex_init(&pControlThreadInfo->threadDataAccessMutex, NULL);
}

///////////////////////////////////////////////////////////////////////////////
//! Releases any resources in the main thread's control data
/*!
  \param pControlThreadInfo	: Pointer to main thread data
*/
void controlThreadInfoRelease(ControlThreadInfo* pControlThreadInfo)
{
	pthread_mutex_destroy(&pControlThreadInfo->threadCmdCompleteMutex);
	pthread_cond_destroy(&pControlThreadInfo->threadCmdCompleteCond);
	pthread_mutex_destroy(&pControlThreadInfo->threadDataAccessMutex);
}

///////////////////////////////////////////////////////////////////////////////
//! Sends a command to the main thread
/*!
  \param pControlThreadInfo	: Pointer to main thread data
  \param cmd				: Command to send to main thread
*/
bool controlThreadInfoSendCmd(ControlThreadInfo* pControlThreadInfo, THREAD_CMDS cmd)
{   
    U1 c = cmd;
    if (write(pControlThreadInfo->cmdPipes[1], &c, 1) == -1)
    {
        LOGE("%s : Could not write to cmd pipe (%i)", __FUNCTION__, errno);
		return false;
    }
	return true;
}

///////////////////////////////////////////////////////////////////////////////
//! Implimentation of a requestStart interface function on GpsControlEventInterface
/*!
  \param pContext	: Pointer to the interface context
*/
static void requestStartEventHandler(void* pContext)
{
	LOGV("%s: Called", __FUNCTION__);
	engineStart((ControlThreadInfo *) pContext);
}

///////////////////////////////////////////////////////////////////////////////
//! Implimentation of a requestStop interface function on GpsControlEventInterface
/*!
  \param pContext	: Pointer to the interface context
*/
static void requestStopEventHandler(void* pContext)
{
	LOGV("%s: Called", __FUNCTION__);
	engineStop((ControlThreadInfo *) pContext);
}


