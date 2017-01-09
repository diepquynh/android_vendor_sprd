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
 * $Id: ubxgpsstate.h 62676 2012-10-22 14:45:53Z jon.bowern $
 ******************************************************************************/

#ifndef __UBXGPSSTATE_H__
#define __UBXGPSSTATE_H__

#include "std_inc.h"
#include "ubx_serial.h"
#include "ubx_udpServer.h"
#include "gps_thread.h"
#include "ubx_messageDef.h"


class CUbxGpsState
{
public:
	// Constructor 
	CUbxGpsState();
	// Destructor
	~CUbxGpsState();

	static CUbxGpsState* getInstance(void);
	
	// Event handling 
	void onStartup(void);
	void onNewUbxMsg(GPS_THREAD_STATES state, const unsigned char* pMsg, unsigned int iMsg);
    void onPrepareShutdown(void);
	void onShutDown(void);
	void onInit(void);
	void clearReceiverShutdownAck(void) { m_receiverShutdownAck = false; };
	bool getReceiverShutdownAck(void) { return m_receiverShutdownAck; };
	
	// Navigation Rate Control
	void putRate(int rateMs);
    int getRate(void) { return m_Db.rateMs; };
	bool writeUbxCfgRate(void);

	// Local Aiding EPH,ALM,AOP,HUI
	void sendAidData(bool bEph, bool bAop, bool bAlm, bool bHui);
	void pollAidData(bool bEph, bool bAop, bool bAlm, bool bHui);
	void deleteAidData(GpsAidingData flags);
	bool writeUbxNavX5AopEnable(void);

	// Assist Now Offline
	bool checkAlpFile(void);
	bool putAlpFile(const unsigned char* pData, unsigned int iData);
	bool writeUbxAlpHeader(void);

	// implementation (assist now online)
	void checkAgpsDownload(void);
	static void* agpsDownloadThread(void *pThreadData);
	
	// Position And Time Aiding/Injection
	void putTime(GpsUtcTime timeUtcMs, int64_t timeRefMs, int accMs);
	void putPos(double latDeg, double lonDeg, float accM);
	bool onNewUbxAidIni(const unsigned char* pMsg, unsigned int iMsg);
	bool writeUbxAidIni(void);

	void setSerial(CSerialPort* pSer) { m_pSer = pSer; };
#if defined UDP_SERVER_PORT
	void setUdp(CUdpServer* pUdpServer) { m_pUdpServer = pUdpServer; };
#endif	

#if defined UDP_SERVER_PORT    
    int getUdpPort(void) { return m_udpPort; };
#endif    
    const char* getSerialDevice(void) { return m_pSerialDevice; };
    int getBaudRate(void) { return m_baudRate; };
    int getBaudRateDefault(void) { return m_baudRateDef; };
    const char* getAlpFilename(void) { return m_pAlpTempFile; };
    int getStoppingTimeoutMs(void) { return m_stoppingTimeoutMs; };
    int getXtraPollingInterval(void) { return m_xtraPollInterval; };

	// Threading help
	void lock(void);
	void unlock(void);
	
	// Further Commands
	bool writeUbxCfgRst(int resetMode, unsigned short flags);
	bool writeUbxCfgPort(int portId, int baudRate);
	bool writeUbxCfgMsg(int clsId, int msgId);
	void setBaudRate();
	
#ifdef SUPL_ENABLED	
	void sendEph(const void* pData, int size);
	void sendAidingData(GPS_UBX_AID_INI_U5__t *pAidingData);
	void sendUtcModel(GPS_UBX_AID_HUI_t *pUtcModel);
	
	// SUPL requests
	int getAlamanacRequest(void) { return m_almanacRequest; };
	int getUtcModelRequest(void) { return m_utcModelRequest; };
	int getIonosphericModelRequest(void) { return m_ionosphericModelRequest; };
	int getDgpsCorrectionsRequest(void) { return m_dgpsCorrectionsRequest; };
	int getRefLocRequest(void) { return m_refLocRequest; };
	int getRefTimeRequest(void) { return m_refTimeRequest; };
	int getAcquisitionAssistRequest(void) { return m_acquisitionAssistRequest; };
	int getRealTimeIntegrityRequest(void) { return m_realTimeIntegrityRequest; };
	int getNavigationModelRequest(void) { return m_navigationModelRequest; };
	int getFakePhone(void) { return m_fakePhone; };
	int getMsaResponseDelay(void) { return m_msaResponseDelay; };
	int getNiUiTimeout(void) { return m_niUiTimeout; };
	int getNiResponseTimeout(void) { return m_niResponseTimeout; };
	bool getLogSuplMessages(void) { return m_logSuplMessages; };
	bool getCmccLogActive(void) { return m_cmccLogActive; };
	bool getSuplMsgToFile(void) { return m_suplMsgToFile; };
#endif

protected:
    char* m_pSerialDevice;		//!< Serial device path connecting Gps receiver
    int m_baudRate;				//!< General baud rate to communicate with receiver
    int m_baudRateDef;			//!< Initial baud rate to communicate with receiver
    char* m_pAlpTempFile;		//!< Path and filename to store alp data
    int m_stoppingTimeoutMs;	//!< Maximum time (in ms) to wait for receiver acknowlegements during 'stopping'
    int m_xtraPollInterval;		//!< Interval between polling AssistNow Offline server (in ms)
	bool m_receiverShutdownAck;	//!< True if receiver's shutdown acknowledgement has been received
	int m_persistence;			//!< Persistence flag. True - save alp database to file. False - don't
	
	CSerialPort* m_pSer;		//!< Pointer to serial communications class instance
#if defined UDP_SERVER_PORT    
    int m_udpPort;				//!< Port to communicate with 
	CUdpServer* m_pUdpServer;	//!< Pointer to UDP communications class instance
#endif
	pthread_mutex_t m_ubxStateMutex;	//!< Mutext controlling access to ubxState
    
	// Handle AssistNow Online messages 
	#define MAX_REQUEST 512
	typedef struct AGPS_THREAD_DATA_s
	{
		char*		server;
		int			port;
		char		request[MAX_REQUEST];
		pthread_t   thread;
		bool		active;
		int64_t 	timeoutLastRequest; // Time of last succesful injected agps data in msec (monotonic system timer)
	} AGPS_THREAD_DATA_t;
	AGPS_THREAD_DATA_t	m_agpsThreadParam;
    
	// Handle AssistNow Offline messages 
	bool onNewUbxAlpMsg(const unsigned char* pMsg, unsigned int iMsg);
	// Return the current Reference Time 
	static int64_t currentRefTimeMs(void);

	enum 
	{ 
		NUM_GPS_SVS				= 32,		//!< maximum number of SV supported (just GPS) 
		MSTIME_ACC_COMM			= 200		//!< Default Accuarcy added to the time aiding
	};

	//! Helper Struct for buffer allocation 
	typedef struct BUF_s 
	{
		unsigned char* p;		//!< Pointer to start of buffer 
		unsigned int i;			//!< Length of buffer
	} BUF_t;
	// Helper function to allocate and free buffers
	static bool replaceBuf(BUF_t* buf, const unsigned char* pMsg, unsigned int iMsg);
	static void freeBuf(BUF_t* pBuf);
	
	//! Per Satellite Database entry
	typedef struct DBSV_s
	{
		BUF_t eph;		//!< Ephemeris complete UBX-AID-EPH messages 
		BUF_t alm;		//!< Almanac complete UBX-AID-ALM messages 
		BUF_t aop;		//!< AssistNow Autonomous complete UBX-AID-AOP messages for each SV
	} DBSV_t;
	
	//! Assistnow Offline ALP database
	typedef struct DBALP_s 
	{
		BUF_t file;			//!< The ALP data file
		U2 fileId;			//!< The file id chosen, incremented with each new file
		int64_t timeRefMs;	//!< The time when the file was received
	} DBALP_t;
		
	//! Position Aiding database
	typedef struct DBPOS_s
	{
		double latDeg;		//!< Latitude in deg
		double lonDeg;		//!< Longitude in deg
		int    accM;		//!< Accuarcy in m
		int64_t timeRefMs;	//!< The time when the file was received
	} DBPOS_t;
	
	//! Time Aiding database 
	typedef struct DBTIME_s
	{
		int64_t timeUtcMs;	//!< The UTC Time  ms
		int64_t timeRefMs;	//!< The local reference Time in ms (use currentRefTimeMs to age)
		int     accMs;		//!< The accuracy in ms 
	} DBTIME_t;

	//! The full assistance database 
	struct DB_s 
	{
		DBSV_t		sv[NUM_GPS_SVS];	//!< per Satellite Database 
		BUF_t		hui;				//!< Health/UTC/Ionosphere complete UBX-AID-HUI message
		DBALP_t		alp;				//!< Assistnow Offline ALP database
		DBPOS_t		pos;				//!< Position Aiding database
		DBTIME_t	time;				//!< Time Aiding database
		int			rateMs;				//!< The requested measurement rate
        bool        dbAssistChanged;    //!< Flag indicating if the DB assistance data has changed since it was last loaded
        bool        dbAssistCleared;    //!< Flag indicating if the DB assistance data is cleared 
	} m_Db;

#ifdef SUPL_ENABLED
	int m_almanacRequest;				//!< If true, request almanac assistance data in Supl transaction 
	int m_utcModelRequest;				//!< If true, request utc model assistance data in Supl transaction
	int m_ionosphericModelRequest;		//!< If true, request iono model assistance data in Supl transaction
	int m_dgpsCorrectionsRequest;		//!< If true, request dgps correction assistance data in Supl transaction
	int m_refLocRequest;				//!< If true, request reference location assistance data in Supl transaction
	int m_refTimeRequest;				//!< If true, request reference time assistance data in Supl transaction
	int m_acquisitionAssistRequest;		//!< If true, request aquisition assistance data in Supl transaction
	int m_realTimeIntegrityRequest;		//!< If true, request real time integrity assistance data in Supl transaction
	int m_navigationModelRequest;		//!< If true, request navigational model assistance data in Supl transaction
	int m_fakePhone;					//!< Fake phone info flag. True - Use contrived IMSI, MCC, MNC, LAC etc. False - get from framework
	int m_msaResponseDelay;				//!< Time (in seconds) to wait before sending pseudo ranges to Supl server in MSA transaction
	int m_niUiTimeout;					//!< Time out (in seconds) for a Network Initiated UI response
	int m_niResponseTimeout;			//!< Time out (in seconds) for a NI Notify/Verify dialog response
	bool m_logSuplMessages;				//!< If true, log SUPL & RRLP messages
	bool m_cmccLogActive;				//!< If true, generate CMCC logging
	bool m_suplMsgToFile;				//!< If true, redirect Supl & RRLP messages to log file
#endif	
	
	// UBX Message creation and writing 
	static void crcUbx(unsigned char crc[2], const unsigned char* pData, int iData);
	bool writeUbx(unsigned char classID, unsigned char msgID, 
				  const void* pData0, int iData0, 
				  const void* pData1 = NULL, int iData1 = 0);
                        
    // Power handling
    static bool powerOn(void);
    static void powerOff(void);
    
	// Load/Saving of aiding data
    bool loadAiding(void);
    static bool loadDatabase(int fd, struct DB_s* pDb);
    static bool loadBuffer(int fd, BUF_t* pBuffer);
    static bool loadBuffer(int fd, DBALP_t* pBuffer);
    static bool loadBuffer(int fd, DBSV_t* pBuffer);
    static bool loadBuffer(int fd, unsigned char* pBuffer, int length);

    void saveAiding(void);
    static bool saveDatabase(int fd, struct DB_s* pDb);
    static bool saveBuffer(int fd, BUF_t* pBuffer);
    static bool saveBuffer(int fd, DBALP_t* pBuffer);
    static bool saveBuffer(int fd, DBSV_t* pBuffer);
    static bool saveBuffer(int fd, unsigned char* pBuffer, int length);
	
	// Agps
	static void injectDataAgpsOnlineData(const char* data, int length);
};


#endif /* __UBXGPSSTATE_H__ */
