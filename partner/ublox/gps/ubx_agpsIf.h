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
  \brief  Agps interface implementation

*/
/*******************************************************************************
 * $Id: ubx_agpsIf.h 61785 2012-09-10 14:35:47Z michael.ammann $
 ******************************************************************************/
#ifndef __UBX_AGPSIF_H__
#define __UBX_AGPSIF_H__

#include "std_inc.h"

class CAgpsIf
{
public:
	CAgpsIf();
	~CAgpsIf();
    static const void *getIf() { return &s_interface; }
	// operations
	static CAgpsIf* getInstance(void);
	
#ifdef SUPL_ENABLED	
	void getSuplServerInfo(char** ppSuplServerAddress, int *pSuplPort);
	void setCertificateFileName(const char* pCert) 
	{ 
		if (m_certificateFileName)
		{
			free(m_certificateFileName);
			m_certificateFileName = NULL;
		}
		m_certificateFileName = pCert ? strdup(pCert) : NULL;
	};
	const char* getCertificateFileName(void) { return m_certificateFileName; };
	bool isTlsActive(void) { return (m_certificateFileName != NULL); };
#endif

protected:
	// interface
 	static void init(AGpsCallbacks* callbacks);
	static int dataConnOpen(const char* apn);
	static int dataConnClosed(void);
	static int dataConnFailed(void);
	static int setServer(AGpsType type, const char* hostname, int port);
	
	// variables	
    const static AGpsInterface s_interface;
	AGpsCallbacks m_callbacks;
	bool m_ready;
	
	// implementation (server name list)
	int numServer(AGpsType type);
	#define NUM_AGPS_SERVERS	16
	#define MAX_HOSTNAME		256
	typedef struct AGPS_SERVER_DATA_s
	{ 
		AGpsType type;
		char hostname[MAX_HOSTNAME];
		int port;
	} AGPS_SERVER_DATA_t;
	AGPS_SERVER_DATA_t		m_agpsServers[NUM_AGPS_SERVERS];
	int						m_agpsServersCnt;
	
#ifdef SUPL_ENABLED		
	char* m_certificateFileName;
#endif
};

#endif /* __UBX_AGPSIF_H__ */
