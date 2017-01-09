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
 * $Id: ubx_xtraIf.cpp 61086 2012-08-15 15:50:12Z jon.bowern $
 ******************************************************************************/

#include "ubx_xtraIf.h"
#include "ubx_moduleIf.h"
#include "ubxgpsstate.h"

///////////////////////////////////////////////////////////////////////////////

static CXtraIf s_myIf;

const GpsXtraInterface CXtraIf::s_interface = {
    size:					sizeof(GpsXtraInterface),
	init:					CXtraIf::init,
	inject_xtra_data:		CXtraIf::injectData,
};

CXtraIf::CXtraIf() 
{ 
	m_ready = false; 
} 

int CXtraIf::init(GpsXtraCallbacks* callbacks)
{
    if (s_myIf.m_ready)
        LOGE("CXtraIf::%s: already initialized", __FUNCTION__);
    LOGV("CXtraIf::%s :", __FUNCTION__);
    s_myIf.m_callbacks = *callbacks;
	s_myIf.m_ready = true;
	return 0;
}

int CXtraIf::injectData(char* data, int length)
{
	LOGV("CXtraIf::%s : length=%d", __FUNCTION__, length);

	CUbxGpsState* pUbxGps = CUbxGpsState::getInstance();
    pUbxGps->lock();
	if (pUbxGps->putAlpFile((const unsigned char*)data, length))
	{
        pUbxGps->writeUbxAlpHeader();
	}
	pUbxGps->unlock();
	
	return 0;
}

void CXtraIf::requestDownload(void)
{
    if (!s_myIf.m_ready)
    {
        LOGE("CXtraIf::%s: class not initialized", __FUNCTION__);
        return;
    }
    LOGV("CXtraIf::%s", __FUNCTION__);
    s_myIf.m_callbacks.download_request_cb();
}

