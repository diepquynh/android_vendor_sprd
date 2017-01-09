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
 * $Id: ubx_localDb.cpp 61086 2012-08-15 15:50:12Z jon.bowern $
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>

#include "ubx_log.h"
#include "std_types.h"
#include "std_lang_def.h"
#include "std_macros.h"
#include "ubx_timer.h"

#include "ubx_localDb.h"

static CMyDatabase s_database;

CMyDatabase::CMyDatabase()
{
	CDatabase();
	m_pGpsState = NULL;
	
	pthread_mutex_init(&m_timeIntervalMutex, NULL);
	m_timeInterval = 0;
	m_nextReportEpochMs = 0;
	// m_lastReportTime = time(NULL) * 1000; // Debug
	m_publishingAllowed = true;	// Publishing on by default;
}

CMyDatabase::~CMyDatabase()
{
	pthread_mutex_destroy(&m_timeIntervalMutex);
}

CMyDatabase* CMyDatabase::getInstance()
{
	return &s_database;
}

GpsUtcTime CMyDatabase::GetGpsUtcTime(void)
{
    TIMESTAMP ts;
    if (varO[DATA_UTC_TIMESTAMP].Get(ts))
    {
        struct tm ti;
        memset(&ti,0,sizeof(ti));
        ti.tm_year  = ts.wYear - 1900;
        ti.tm_mon   = ts.wMonth - 1;
        ti.tm_mday  = ts.wDay;
        ti.tm_hour  = ts.wHour;
        ti.tm_min   = ts.wMinute;
        ti.tm_sec   = ts.lMicroseconds / 1000000;
        unsigned long us = ts.lMicroseconds - ti.tm_sec * 1000000;
        ti.tm_isdst = -1;
            
        time_t t = mktime(&ti);
//            LOGV("%s: %s", __FUNCTION__, ctime(&t));

        // calc utc / local difference 
        time_t         now = time(NULL); 
        struct tm      tmLocal; 
        struct tm      tmUtc; 
        long           timeLocal, timeUtc; 
            
        gmtime_r( &now, &tmUtc ); 
        localtime_r( &now, &tmLocal ); 
        timeLocal =	tmLocal.tm_sec + 
            60*(tmLocal.tm_min + 
                60*(tmLocal.tm_hour + 
                    24*(tmLocal.tm_yday + 
                        365*tmLocal.tm_year))); 
        timeUtc =	tmUtc.tm_sec + 
            60*(tmUtc.tm_min + 
                60*(tmUtc.tm_hour + 
                    24*(tmUtc.tm_yday + 
                        365*tmUtc.tm_year))); 
        long utcDiff = timeUtc - timeLocal; 
//            LOGV("Time utcDiff: %li", utcDiff);            
            
        t -= utcDiff;
//            LOGV("%s: %s", __FUNCTION__, ctime(&t));
            
        GpsUtcTime gpsUtcTime = ((GpsUtcTime) t) * 1000 + us / 1000;
            
        return gpsUtcTime;
    }
    return 0;
}

CDatabase::STATE_t CMyDatabase::Commit(bool bClear)
{
    CDatabase::STATE_t state;

    // Store commit time in database
    state = CDatabase::Commit(bClear);
        
    //LOGV("Perform commit: clear %i   state %i", bClear, state);

    //LOGV("*** Epoch *** Now %lli, Last %lli, Interval %lli",
    //    now, lastReportTime, reportInterval);
                  
    // lastReportTime = time(NULL) * 1000; // Debug

    pthread_mutex_lock(&m_timeIntervalMutex);
    int report = false;
    //LOGV("%s : Ready to report", __FUNCTION__);
    if (m_nextReportEpochMs) 
    {   
        int64_t monotonicNow = getMonotonicMsCounter();
        if (monotonicNow >= m_nextReportEpochMs)
        {
            report = true;  // Yes we can report
            m_nextReportEpochMs = monotonicNow + m_timeInterval;
            //LOGV("%s : Next timer epoch (%lli)", __FUNCTION__, m_nextReportEpochMs);
        }
    }
    else
    {
        report = true;  // Yes we can report
        //LOGV("%s : Normal report epoch", __FUNCTION__);
    }
    pthread_mutex_unlock(&m_timeIntervalMutex);
        
    if ((m_pGpsState->gpsState == GPS_STARTED) && (report))
    {
        // Driver in the 'started' state, so ok to report to framework
        //LOGV("%s : >>> Reporting (%p)", __FUNCTION__, m_pGpsState->pGpsInterface);
        // Location
        if (CGpsIf::getInstance()->m_callbacks.location_cb) 
        {
            GpsLocation loc;
            memset(&loc, 0, sizeof(GpsLocation));
            loc.size = sizeof(GpsLocation);
            // position 
			if (state == STATE_READY)
			{
				//LOGV("%s : >>> Reporting Pos, Alt & Acc", __FUNCTION__);
				if (varO[DATA_LASTGOOD_LATITUDE_DEGREES].Get(loc.latitude) && varO[DATA_LASTGOOD_LONGITUDE_DEGREES].Get(loc.longitude))
					loc.flags |= GPS_LOCATION_HAS_LAT_LONG;
				if (varO[DATA_LASTGOOD_ALTITUDE_ELLIPSOID_METERS].Get(loc.altitude))
					loc.flags |= GPS_LOCATION_HAS_ALTITUDE;
				if (varO[DATA_LASTGOOD_ERROR_RADIUS_METERS].Get(loc.accuracy))
					loc.flags |= GPS_LOCATION_HAS_ACCURACY;
					
				// speed / heading
				if (varO[DATA_TRUE_HEADING_DEGREES].Get(loc.bearing))
					loc.flags |= GPS_LOCATION_HAS_BEARING;
				if (varO[DATA_SPEED_KNOTS].Get(loc.speed))
				{
					loc.speed *= (1.852 / 3.6); // 1 knots -> 1.852km / hr -> 1.852 * 1000 / 3600 m/s
					loc.flags |= GPS_LOCATION_HAS_SPEED;
				}
			}
			
            TIMESTAMP ts;
			memset(&ts, 0, sizeof(ts));
			varO[DATA_UTC_TIMESTAMP].Get(ts);
            logGps.write(0x00000002, "%04d%02d%02d%02d%02d%06.3f,%10.6f,%11.6f,%d #position(time_stamp,lat,lon,ttff)", 
						ts.wYear, ts.wMonth, ts.wDay, ts.wHour, ts.wMinute, 1e-6*ts.lMicroseconds, 
						loc.latitude, loc.longitude, 0/* todo need to write a ttff*/);
			loc.timestamp = GetGpsUtcTime();
			if ((loc.flags != 0) && (m_publishingAllowed))
            {
             	CGpsIf::getInstance()->m_callbacks.location_cb(&loc);
            }
        }
    
        if (CGpsIf::getInstance()->m_callbacks.sv_status_cb) 
        {
            int i;
            // Satellite status
            GpsSvStatus svStatus;
            memset(&svStatus, 0, sizeof(GpsSvStatus));
            svStatus.size = sizeof(GpsSvStatus);

                // we do publish the satellites to the android framework the qualcomm way (like done in nmea)
#define IS_GPS(prn)  ((prn >=  1 /* G1   */) && (prn <= 32 /* G32  */)) // GPS  publish G1 to G32
#define IS_SBAS(prn) ((prn >= 33 /* S120 */) && (prn <= 64 /* S151 */)) // SBAS publish S120 to S151, S152 to S158 are filtered in NMEA 
#define IS_GLO(prn)  ((prn >= 65 /* R1   */) && (prn <= 87 /* R24  */)) // GLO  publish R1 to R24, R25 to R32 of nmea is not supported by our GPS  
#define PRN_MASK(prn) (1ul << (prn - 1)) // convert to the prn mask

                // Satellites in view
                int num = 0;
            if (varO[DATA_SATELLITES_IN_VIEW].Get(num) && (num > 0))
            {
                int c = 0;
                for (i = 0; (i < num) && (c < GPS_MAX_SVS); i++)
                {
                    float az = 0.0f, el = 0.0f, snr = 0.0f;
                    int prn = 0;
                    bool azelOk = (varO[DATA_SATELLITES_IN_VIEW_ELEVATION_(i)].Get(el)  && (el >= 0.0f) && (el <= 90.0f)) &&
                        (varO[DATA_SATELLITES_IN_VIEW_AZIMUTH_(i)].Get(az)    && (az >= 0.0f) && (az <= 360.0f));
                    bool snrOk  = (varO[DATA_SATELLITES_IN_VIEW_STN_RATIO_(i)].Get(snr) && (snr > 0.0f));
                    prn         = (varO[DATA_SATELLITES_IN_VIEW_PRNS_(i)].Get(prn) && 
                                   (IS_GPS(prn) || IS_SBAS(prn) || IS_GLO(prn))) ? prn : 0;
                    if (azelOk || snrOk || prn) // if have information to share the fill the structure
                    {
                        int orbsta;
                        svStatus.sv_list[c].size = sizeof(GpsSvInfo);
                        svStatus.sv_list[c].prn		  = prn;
                        svStatus.sv_list[c].azimuth   = azelOk ? az : -1;
                        svStatus.sv_list[c].elevation = azelOk ? el : -1;
                        svStatus.sv_list[c].snr		  = snrOk ? snr : -1;
                        if (varO[DATA_UBX_SATELLITES_IN_VIEW_ORB_STA_(i)].Get(orbsta) && IS_GPS(prn))
                        { 
                            if (orbsta & 0x1 /* EPH */) svStatus.ephemeris_mask |= PRN_MASK(prn);
                            if (orbsta & 0x2 /* ALM */) svStatus.almanac_mask   |= PRN_MASK(prn);
                        }
                        c ++;
                    }
                }
                svStatus.num_svs = c;
                if (c)
                {
                    // Satellites used
                    num = 0;
                    if (varO[DATA_SATELLITES_USED_COUNT].Get(num) && (num > 0))
                    {
                        for (i = 0; i < num; i++)
                        {
                            int prn;
                            if (varO[DATA_SATELLITES_USED_PRNS_(i)].Get(prn) && IS_GPS(prn))
                                svStatus.used_in_fix_mask |= PRN_MASK(prn);
                        }
                    }
                    CGpsIf::getInstance()->m_callbacks.sv_status_cb(&svStatus);
                }
                svStatus.num_svs = c;
            }
        }
    }
    return state;
}


bool CMyDatabase::GetCurrentTimestamp(TIMESTAMP& rFT)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t tt = tv.tv_sec;
    unsigned long usec = tv.tv_usec;

	struct tm st;
    gmtime_r(&tt, &st);
    rFT.wYear		 = st.tm_year + 1900;	
    rFT.wMonth		 = st.tm_mon + 1;
    rFT.wDay		 = st.tm_mday;
    rFT.wHour		 = st.tm_hour;
    rFT.wMinute		 = st.tm_min;
    rFT.lMicroseconds = (unsigned long) st.tm_sec * 1000000 + usec;

    return true;
}

void CMyDatabase::setEpochInterval(int timeIntervalMs, int64_t nextReportEpochMs)
{
	pthread_mutex_lock(&m_timeIntervalMutex);
	
	m_timeInterval = timeIntervalMs;
	m_nextReportEpochMs = nextReportEpochMs;
	
	pthread_mutex_unlock(&m_timeIntervalMutex);
}

void CMyDatabase::publishOff(void) 
{ 
	m_publishingAllowed = false;
	LOGV("%s", __FUNCTION__);
};

void CMyDatabase::publishOn(void)
{ 
	m_publishingAllowed = true;
	LOGV("%s", __FUNCTION__);
};

