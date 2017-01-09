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
 * $Id: ubx_agpsIf.cpp 61785 2012-09-10 14:35:47Z michael.ammann $
 ******************************************************************************/

#include "string.h"
#include "ubx_agpsIf.h"

///////////////////////////////////////////////////////////////////////////////
// Definitions & Types

///////////////////////////////////////////////////////////////////////////////
// Local data
static CAgpsIf s_myIf;		//!< Private instance of the CAgpsIf class - 'singleton' 

const AGpsInterface CAgpsIf::s_interface = {		//!< Agps interface jump table
    size:					sizeof(AGpsInterface),
	init:					CAgpsIf::init,
    data_conn_open:			CAgpsIf::dataConnOpen,
    data_conn_closed:		CAgpsIf::dataConnClosed,
    data_conn_failed:		CAgpsIf::dataConnFailed,
    set_server:				CAgpsIf::setServer,
};

///////////////////////////////////////////////////////////////////////////////
CAgpsIf::CAgpsIf()
{
	m_ready = false;
	m_agpsServersCnt = 0;
#ifdef SUPL_ENABLED
	m_certificateFileName = NULL;
#endif	
}

CAgpsIf::~CAgpsIf()
{
#ifdef SUPL_ENABLED
	if (m_certificateFileName)
	{
		free(m_certificateFileName);
		m_certificateFileName = NULL;
	}		
#endif
}

CAgpsIf* CAgpsIf::getInstance()
{
	return &s_myIf;
}

void CAgpsIf::init(AGpsCallbacks* callbacks)
{
    if (s_myIf.m_ready)
        LOGE("CAgpsIf::%s : already initialized", __FUNCTION__);
    LOGV("CAgpsIf::%s :", __FUNCTION__);
    s_myIf.m_callbacks = *callbacks;
	s_myIf.m_ready = true;
}

int CAgpsIf::dataConnOpen(const char* apn)
{
    LOGD("CAgpsIf::%s : apn='%s'", __FUNCTION__, apn);

	return 0;
}


int CAgpsIf::dataConnClosed(void)
{
    LOGD("CAgpsIf::%s :", __FUNCTION__);

    return 0;
}

int CAgpsIf::dataConnFailed()
{
    LOGD("CAgpsIf::%s :", __FUNCTION__);

    return 0;
}


int CAgpsIf::setServer(AGpsType type, const char* hostname, int port)
{
	LOGD("CAgpsIf::%s : type=%i(%s) host=%s port=%i", __FUNCTION__, type, _LOOKUPSTR(type, AGpsType), hostname, port);
	int cnt = s_myIf.m_agpsServersCnt;
	if (cnt < NUM_AGPS_SERVERS)
	{
		s_myIf.m_agpsServers[cnt].type = type;
		s_myIf.m_agpsServers[cnt].port = port;
		strncpy(s_myIf.m_agpsServers[cnt].hostname, hostname, MAX_HOSTNAME);
		s_myIf.m_agpsServersCnt = cnt +1;
	}
	else
	{
		LOGE("CAgpsIf::%s : no space", __FUNCTION__);
	}
	
    return 0;
}

int CAgpsIf::numServer(AGpsType type)
{
	int i;
	int cnt = 0;
	for (i = 0; i < m_agpsServersCnt; i ++)
	{
		if (m_agpsServers[i].type == type)
			cnt ++;
	}
	LOGD("CAgpsIf::%s : num=%d", __FUNCTION__, cnt);
	return cnt;
}

#ifdef SUPL_ENABLED
void CAgpsIf::getSuplServerInfo(char** ppSuplServerAddress, int *pSuplPort)
{
	*ppSuplServerAddress = NULL;
	*pSuplPort = -1;

	int i;
	for (i = 0; i < m_agpsServersCnt; i ++)
	{
		if (m_agpsServers[i].type == AGPS_TYPE_SUPL)
		{
			*ppSuplServerAddress = m_agpsServers[i].hostname;
			*pSuplPort = m_agpsServers[i].port;
		}
	}
	LOGV("CAgpsIf::%s : Address:Port %s:%d", __FUNCTION__, *ppSuplServerAddress, *pSuplPort);
}
#endif
