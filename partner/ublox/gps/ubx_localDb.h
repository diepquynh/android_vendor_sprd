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
  \brief  GPS global IF

*/
/*******************************************************************************
 * $Id: ubx_localDb.h 60865 2012-08-08 15:51:06Z jon.bowern $
 ******************************************************************************/

#ifndef __UBX_LOCALDB_H__
#define __UBX_LOCALDB_H__

#include "database.h"
#include "gps_thread.h"

///////////////////////////////////////////////////////////////////////////////

class CMyDatabase : public CDatabase
{
protected:
    ControlThreadInfo* m_pGpsState;
    //int64_t m_lastReportTime; // Debug
	
	// time interval delay
	pthread_mutex_t         m_timeIntervalMutex;
	int                     m_timeInterval;
	int64_t                 m_nextReportEpochMs;
	bool                    m_publishingAllowed;

public:
    CMyDatabase();
	~CMyDatabase();

	static CMyDatabase* getInstance();
    GpsUtcTime GetGpsUtcTime(void);

	virtual STATE_t Commit(bool bClear = true);

	bool GetCurrentTimestamp(TIMESTAMP& rFT);
	void setEpochInterval(int timeIntervalMs, int64_t nextReportEpochMs);
	void setGpsState(ControlThreadInfo* pGpsState) { m_pGpsState = pGpsState; };
	int64_t getNextReportEpoch(void) { return m_nextReportEpochMs; };
	void publishOff(void);
	void publishOn(void);
	
	template<typename T> bool getData(DATA_t data, T &v)
	{
		if (data < DATA_NUM)
			return varO[data].Get(v);
		return false;
	}
};



#endif /* __UBX_LOCALDB_H__ */
