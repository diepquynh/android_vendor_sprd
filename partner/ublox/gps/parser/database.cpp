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
 ******************************************************************************/
/*!
  \file
  \brief 
*/
/*******************************************************************************
 * $Id: database.cpp 59625 2012-06-20 11:34:02Z jon.bowern $
 ******************************************************************************/
#include "stdafx.h"

#include "database.h"
#include "gpsconst.h"

CDatabase::CDatabase(void)
{
	Reset();
}

CDatabase::STATE_t CDatabase::Commit(bool bClear /* = true */)
{
	CompletePosition();
	CompleteVelocity();
	CompleteAltitude();
	CompleteHeading();
	CompleteTimestamp();
	
	memcpy(varO,  varN, sizeof(varN));

	// set time commit time stamp 
	TIMESTAMP ts; 
	if (GetCurrentTimestamp(ts))
		varO[DATA_COMMIT_TIMESTAMP].Set(ts);

	// fill in an accuarcy if it is not available
	if (varO[DATA_ERROR_RADIUS_METERS].IsEmpty() && 
		varO[DATA_LONGITUDE_DEGREES].IsSet()     && 
		varO[DATA_LATITUDE_DEGREES].IsSet())
	{
		int i;
		double d;
		if (varO[DATA_UBX_POSITION_ECEF_ACCURACY].Get(d))
			varO[DATA_ERROR_RADIUS_METERS].Set(d);
		else if (varO[DATA_SATELLITES_USED_COUNT].Get(i))
		{
			// fake the error radius in case the error radius is not set
			// this should be filled with something more resonable here
			varO[DATA_ERROR_RADIUS_METERS].Set( (i >= 6) ?   5 : 
												(i >= 4) ?  10 : 
														   100 );
		}
	}
	// compute the last known good fix 
	if (varO[DATA_LONGITUDE_DEGREES].IsSet() && varO[DATA_LATITUDE_DEGREES].IsSet() && 
		varO[DATA_ERROR_RADIUS_METERS].IsSet() && varO[DATA_LOCAL_TIMESTAMP].IsSet())
	{
		// mandatory fields
		varO[DATA_LASTGOOD_LONGITUDE_DEGREES]				= varO[DATA_LONGITUDE_DEGREES];
		varO[DATA_LASTGOOD_LATITUDE_DEGREES]				= varO[DATA_LATITUDE_DEGREES];
		varO[DATA_LASTGOOD_ERROR_RADIUS_METERS]				= varO[DATA_ERROR_RADIUS_METERS];
		varO[DATA_LASTGOOD_TIMESTAMP]						= varO[DATA_LOCAL_TIMESTAMP];
		// optional fields 
		varO[DATA_LASTGOOD_ALTITUDE_ELLIPSOID_METERS]		= varO[DATA_ALTITUDE_ELLIPSOID_METERS];
		varO[DATA_LASTGOOD_ALTITUDE_ELLIPSOID_ERROR_METERS]	= varO[DATA_ALTITUDE_ELLIPSOID_ERROR_METERS];
		// set state to ready 
		vasS = STATE_READY;
	}
	else if (vasS == STATE_READY)
	{
		// fallback to reacq 
		vasS = STATE_REACQ;
	}
	// enable next line if you like noise
	if (bClear)
	{
		memset(varN, 0, sizeof(varN));
		memset(varM, 0, sizeof(varM));
	}
	return vasS;
}

void CDatabase::Reset(void)
{
	memset(varM, 0, sizeof(varM));
	memset(varN, 0, sizeof(varN));
	memset(varO, 0, sizeof(varO));
	vasS = STATE_NO_DATA;
}

bool CDatabase::CheckTime(int hour, int minute, double seconds)
{
	return (varN[DATA_TIME_HOUR].Check(hour))   && 
		   (varN[DATA_TIME_MINUTE].Check(minute)) && 
		   (varN[DATA_TIME_SECOND].Check(seconds));
}

bool CDatabase::CheckDate(int year, int month, int day)
{
	return (varN[DATA_DATE_YEAR].Check(year))  && 
		   (varN[DATA_DATE_MONTH].Check(month)) && 
		   (varN[DATA_DATE_DAY].Check(day));
}

double CDatabase::Degrees360(double val)
{
	return (val <    0.0) ? val + 360.0 : 
		   (val >= 360.0) ? val - 360.0 : val; 
}

int CDatabase::ConvertPrn2NmeaSvid(int svid)
{
	// NMEA has reserved 32 svs for GPS
	if ((svid >= 1) && (svid <= 32))
		svid = svid;
	// NMEA has reserved 32 svs for SBAS
	// the svids 120-151 and higher are mapped to 33-64.
	else if ((svid >= 120) && (svid <= 151))
		svid = (svid - 120 + 33);
	// NMEA has reserved 32 SVs for GLONASS, 
	// slot number 1-24 are mapped to 65-88, 89-96 are reserved for on-orbit spares.
	else if ((svid >= 65) && (svid <= 88))
		svid = svid;
	// NMEA has not reserved SVs for GALILEO
	else
		svid = 0;
	return svid;
}

#define TIMEDIFFERENCE_UTC2GPS	315964800.0 // which is (double)mkgmtime(1980,1,6,0,0,0,0)

__drv_floatUsed double CDatabase::CalcLeapSeconds(double dUtcSec)
{
	const struct 
	{
		double dTime; 
		unsigned short wYear;
	    unsigned char byMonth;
		unsigned char byDay;
		double dLeapSecs;
	} LUT[] = {
		{ 1341100800, 2012, 7, 1, 16 },
		{ 1230764400, 2009, 1, 1, 15 },
		{ 1136073600, 2006, 1, 1, 14 },
		{  915148800, 1999, 1, 1, 13 },
		{  867715200, 1997, 7, 1, 12 },
		{  820454400, 1996, 1, 1, 11 },
		{  773020800, 1994, 7, 1, 10 },
		{  741484800, 1993, 7, 1,  9 },
		{  709948800, 1992, 7, 1,  8 },
		{  662688000, 1991, 1, 1,  7 },
		{  631152000, 1990, 1, 1,  6 },
		{  567993600, 1988, 1, 1,  5 },
		{  489024000, 1985, 7, 1,  4 },
		{  425865600, 1983, 7, 1,  3 },
		{  394329600, 1982, 7, 1,  2 },
		{  362793600, 1981, 7, 1,  1 },
		{  315532800, 1980, 1, 1,  0 }
	};
	unsigned int i;
	for (i=0;i<sizeof(LUT)/sizeof(*LUT);i++) {
	  if (dUtcSec > LUT[i].dTime) 
		  return LUT[i].dLeapSecs;
	}
	return 0;
}

void CDatabase::CompleteTimestamp(void)
{
	TIMESTAMP st;
	double seconds;
	memset(&st, 0, sizeof(st));		// clear first
	if (varN[DATA_DATE_YEAR].Get(st.wYear)		&& varN[DATA_DATE_MONTH].Get(st.wMonth) && 
		varN[DATA_DATE_DAY].Get(st.wDay)		&& varN[DATA_TIME_HOUR].Get(st.wHour) && 
		varN[DATA_TIME_MINUTE].Get(st.wMinute)	&& varN[DATA_TIME_SECOND].Get(seconds))
	{
		st.lMicroseconds = (unsigned long)(seconds * 1e6);
		varN[DATA_UTC_TIMESTAMP].Set(st);
	}
#if 0
	int week; 
	double tow;
	TIMESTAMP st;
	if (varN[DATA_UBX_GPSTIME_WEEK].Get(tow) && varN[DATA_UBX_GPSTIME_TOW].Get(tow))
	{
		double dTime = week * 604800.0 + tow;
		dTime = dTime + TIMEDIFFERENCE_UTC2GPS;
		dTime = dTime - CalcLeapSeconds(dTime);
		// time is now UTC seconds since 1970
		

	}
	else if (varN[DATA_UTC_TIMESTAMP].Get(st) &&
			 varN[DATA_UBX_GPSTIME_WEEK].IsEmpty() && 
			 varN[DATA_UBX_GPSTIME_TOW].IsEmpty())
	{
		// calculate the time since 1970 -> mkgmtime does not exist ...
		const int _days[] = { -1, 30, 58, 89, 119, 150, 180, 211, 242, 272, 303, 333, 364 };
		int imonth = (int)st.wMonth;
		int idays = (int)st.wDay + _days[imonth - 1];
		int iyear = (int)(st.wYear-1900);
		if ( !(iyear & 3) && (imonth > 2) )
			idays++;
		#define _BASE_YEAR         70
		#define _LEAP_YEAR_ADJUST  17
		double dTime = ((((iyear - _BASE_YEAR) * 365 + ((iyear - 1) >> 2) - _LEAP_YEAR_ADJUST + idays)
					     * 24.0 + st.wHour) * 60.0 + st.wMinute) * 60.0 + st.wMicroseconds * 1.0e-6;
		// time is now UTC seconds since 1970
		dTime = dTime + CalcLeapSeconds(dTime);
		dTime = dTime - TIMEDIFFERENCE_UTC2GPS; 
		// dTime is the time in GPS frame (Jan 6th, 1980)
		if (dTime >= 0)
		{
			week = (int)(dTime / 604800.0)
			tow = dTime - week * 604800.0;
			varN[DATA_UBX_GPSTIME_WEEK].Set(week);
			varN[DATA_UBX_GPSTIME_TOW].Set(tow);
		}
	}
#endif
}

void CDatabase::CompletePosition(void)
{
	double X, Y;
	if (varN[DATA_UBX_POSITION_ECEF_X].Get(X) && 
		varN[DATA_UBX_POSITION_ECEF_Y].Get(Y))
	{
		double Z;
		double dLon = atan2(Y,X);
		// Set Longitude if needed
		if (varN[DATA_LONGITUDE_DEGREES].IsEmpty())
			varN[DATA_LONGITUDE_DEGREES].Set(dLon * DEGREES_PER_RADIAN);
		if (varN[DATA_UBX_POSITION_ECEF_Z].Get(Z))
		{
			double p = sqrt(X * X + Y * Y);
			double T = atan2(Z * A, p * B);
			double sinT = sin(T);
			double cosT = cos(T);
			double dLat = atan2(Z + E2SQR * B * sinT * sinT * sinT, p - E1SQR * A * cosT * cosT * cosT);
			// Set Latitude if needed
			if (varN[DATA_LATITUDE_DEGREES].IsEmpty())
				varN[DATA_LATITUDE_DEGREES].Set(dLat * DEGREES_PER_RADIAN);
			// Set Altitude if needed
			if (varN[DATA_ALTITUDE_ELLIPSOID_METERS].IsEmpty())
			{
				// handle the poles
				double dAlt;
				if (p == 0.0)
					dAlt = fabs(Z) - B;
				else
				{
					double sinF = sin(dLat);
					double cosF = cos(dLat);
					double N =  A*A / sqrt(A*A * cosF*cosF + B*B * sinF*sinF);
					dAlt = p / cosF - N;
				}
				varN[DATA_ALTITUDE_ELLIPSOID_METERS].Set(dAlt);
			}
		}
	}
}

void CDatabase::CompleteVelocity(void)
{
	double X,Y,Long;
	if (varN[DATA_UBX_VELOCITY_ECEF_VX].Get(X) && 
		varN[DATA_UBX_VELOCITY_ECEF_VY].Get(Y) && 
		varN[DATA_LONGITUDE_DEGREES].Get(Long))
	{
		Long *= RADIANS_PER_DEGREE;
		double sinL = sin(Long);
		double cosL = cos(Long);
		double ve = - X * sinL + Y * cosL;
		double Z,Lat; 
		if (varN[DATA_UBX_VELOCITY_ECEF_VZ].Get(Z) && 
			varN[DATA_LATITUDE_DEGREES].Get(Lat))
		{
			Lat *= RADIANS_PER_DEGREE;
			double sinF = sin(Lat);
			double cosF = cos(Lat);
			double vn = - X * sinF * cosL - Y * sinF * sinL + Z * cosF;
			//double vd = - X * cosF * cosL - Y * cosF * sinL - Z * sinF;
			double speed2 = vn*vn + ve*ve;
			if (varN[DATA_SPEED_KNOTS].IsEmpty())
			{
				double speed = sqrt(speed2);
				varN[DATA_SPEED_KNOTS].Set(speed / METERS_PER_NAUTICAL_MILE);
			}
			if (varN[DATA_TRUE_HEADING_DEGREES].IsEmpty() && (speed2 > (1.0*1.0)))
			{
				double cog = atan2(ve, vn);
				varN[DATA_TRUE_HEADING_DEGREES].Set(cog * DEGREES_PER_RADIAN);
			}
		}
	}
}

void CDatabase::CompleteAltitude(void)
{
	// complete altitudes and geodial separation if one of the fields is missing
	double sep = 0.0, ell = 0.0, msl = 0.0;
	bool bSep = varN[DATA_GEOIDAL_SEPARATION].Get(sep);
	bool bMsl = varN[DATA_ALTITUDE_SEALEVEL_METERS].Get(msl);
	bool bEll = varN[DATA_ALTITUDE_ELLIPSOID_METERS].Get(ell);
	if      (!bEll && bMsl && bSep)
		varN[DATA_ALTITUDE_ELLIPSOID_METERS].Set(sep + msl);
	else if (!bMsl && bSep && bEll)
		varN[DATA_ALTITUDE_SEALEVEL_METERS].Set(ell - sep);
	else if (!bSep && bEll && bMsl)
		varN[DATA_GEOIDAL_SEPARATION].Set(ell - msl);
}

void CDatabase::CompleteHeading(void)
{
	// complete headings and mag separation if one of the fields is missing
	double var = 0.0, th = 0.0, mh = 0.0;
	bool bVar = varN[DATA_MAGNETIC_VARIATION].Get(var);
	bool bTh  = varN[DATA_MAGNETIC_HEADING_DEGREES].Get(mh);
	bool bMh  = varN[DATA_TRUE_HEADING_DEGREES].Get(th);
	if      (!bVar && bTh && bMh)
		varN[DATA_MAGNETIC_VARIATION].Set(Degrees360(mh - th));
	else if (!bMh && bTh && bVar)
		varN[DATA_MAGNETIC_HEADING_DEGREES].Set(Degrees360(th + var));
	else if (!bTh && bMh && bVar)
		varN[DATA_TRUE_HEADING_DEGREES].Set(Degrees360(mh - var));
}

void CDatabase::MsgOnce(MSG_t msg)
{
	if (msg < MSG_NUM)
	{
		if (varM[msg])
		{
			//printf("MsgOnce(%d) -> Commit()\n", msg);
			Commit();
		}
		varM[msg] = true;
	}
	TIMESTAMP ts; 
	if (GetCurrentTimestamp(ts))
	{	
		if (varN[DATA_LOCAL_TIMESTAMP].IsEmpty())
			varN[DATA_LOCAL_TIMESTAMP].Set(ts);
		varN[DATA_LOCALX_TIMESTAMP].Set(ts);
	}	
}

// Debugging Stuff / Tools to dump the Database 

#define DUMP_X(ix, txt, fmt, fmtbad) 	\
{										\
	double d; 							\
	if (pVar[ix].Get(d))				\
		Printf(txt fmt, d);				\
	else 								\
		Printf(txt fmtbad);				\
}

#define DUMP_TS(ix)														\
{																		\
	TIMESTAMP ts;														\
	if (pVar[ix].Get(ts))												\
	{																	\
		unsigned long s  = ts.lMicroseconds / 1000000;					\
		unsigned long us = ts.lMicroseconds - s * 1000000;				\
		Printf( "time %02u:%02u:%02lu.%06lu date %02u.%02u.%04u ", 		\
					ts.wHour, ts.wMinute, s, us,						\
					ts.wDay, ts.wMonth, ts.wYear);						\
	}																	\
	else																\
		Printf( "time ??:??:??.?????? date ??.??.???? ");				\
}

void CDatabase::Dump(CVar* pVar)
{
	int i;
	Printf("pc "); DUMP_TS(DATA_LOCAL_TIMESTAMP); DUMP_TS(DATA_LOCALX_TIMESTAMP); printf("\n");
	DUMP_TS(DATA_UTC_TIMESTAMP);
	DUMP_X(DATA_TIME_HOUR,							"time ",	"%02.0f",	"??");
	DUMP_X(DATA_TIME_MINUTE,						":",		"%02.0f",	"??");
	DUMP_X(DATA_TIME_SECOND,						":",		"%06.3f ",	"??.??? ");
	DUMP_X(DATA_DATE_DAY,							"date ",	"%02.0f",	"??");
	DUMP_X(DATA_DATE_MONTH,							".",		"%02.0f",	"??");
	DUMP_X(DATA_DATE_YEAR,							".",		"%04.0f ",	"???? ");
	DUMP_X(DATA_UBX_TTAG,							"ttag ",	"%11.3f",	"      ?.???");
	Printf("\n");
	DUMP_X(DATA_LATITUDE_DEGREES,					"lat ",		"%11.6f ",	"   ?.?????? ");
	DUMP_X(DATA_LONGITUDE_DEGREES,					"lon ",		"%10.6f ",	 "  ?.?????? ");
	DUMP_X(DATA_ERROR_RADIUS_METERS,				"err ",		"%7.2f ",	"   ?.?? ");
	Printf("\n");
	DUMP_X(DATA_ALTITUDE_ELLIPSOID_METERS,			"alt ",		"%7.2f ",	"   ?.?? ");
	DUMP_X(DATA_ALTITUDE_ELLIPSOID_ERROR_METERS,	"err ",		"%7.2f ",	"   ?.?? ");
	DUMP_X(DATA_ALTITUDE_SEALEVEL_METERS,			"msl ",		"%7.2f ",	"   ?.?? ");
	DUMP_X(DATA_ALTITUDE_SEALEVEL_ERROR_METERS,		"err ",		"%7.2f ",	"   ?.?? ");
	DUMP_X(DATA_GEOIDAL_SEPARATION,					"sep ",		"%6.2f ",	"  ?.?? ");
	Printf("\n");
	DUMP_X(DATA_SPEED_KNOTS,						"knots ",	"%7.2f ",	"   ?.?? ");
	DUMP_X(DATA_TRUE_HEADING_DEGREES,				"head ",	"%06.2f ",	"???.?? ");
	DUMP_X(DATA_MAGNETIC_HEADING_DEGREES,			"headmag ",	"%06.2f ",	"???.?? ");
	DUMP_X(DATA_MAGNETIC_VARIATION,					"magvar ",	"%06.2f ",	"???.?? ");
	Printf("\n");
	DUMP_X(DATA_POSITION_DILUTION_OF_PRECISION,		"pdop ",	"%7.2f ",	"   ?.?? ");
	DUMP_X(DATA_HORIZONAL_DILUTION_OF_PRECISION,	"hdop ",	"%7.2f ",	"   ?.?? ");
	DUMP_X(DATA_VERTICAL_DILUTION_OF_PRECISION,		"vdop",		"%7.2f ",	"   ?.?? ");
	Printf("\nfix: ");
	DUMP_X(DATA_FIX_QUALITY,						"qual ",	"%1.0f ",	"? ");
	DUMP_X(DATA_FIX_TYPE,							"type ",	"%1.0f ",	"? ");
	Printf("gps: ");
	DUMP_X(DATA_GPS_OPERATION_MODE,					"op ",		"%1.0f ",	"? ");
	DUMP_X(DATA_GPS_SELECTION_MODE,					"sel ",		"%1.0f ",	"? ");
	DUMP_X(DATA_GPS_STATUS,							"stat ",	"%1.0f ",	"? ");
	Printf("dgps: ");
	DUMP_X(DATA_DIFFERENTIAL_REFERENCE_STATION_ID,	"station ",	"%04.0f ",	"   ? ");
	DUMP_X(DATA_DGPS_DATA_AGE,						"age ",		"%4.0f ",	"   ? ");
	
	Printf("\n");
	DUMP_X(DATA_SATELLITES_USED_COUNT,				"used ",	"%2.0f ",	" ? ");
	Printf("prn ");
	for (i = 0; i < CDatabase::MAX_SATELLITES_USED; i ++)
		DUMP_X(DATA_SATELLITES_USED_PRNS_(i),		"",			"%2.0f ",	"");
	Printf("\n");
	DUMP_X(DATA_SATELLITES_IN_VIEW,					"in view ",	"%2.0f ",	" ? ");
	Printf("\nprn ");
	for (i = 0; i < CDatabase::MAX_SATELLITES_IN_VIEW; i ++)
		DUMP_X(DATA_SATELLITES_IN_VIEW_PRNS_(i),		"",		"%5.0f ",	"      ");
	Printf("\norb ");
	for (i = 0; i < CDatabase::MAX_SATELLITES_IN_VIEW; i ++)
		DUMP_X(DATA_UBX_SATELLITES_IN_VIEW_ORB_STA_(i),	"",		"%5.0f ",	"      ");
	Printf("\naz  ");
	for (i = 0; i < CDatabase::MAX_SATELLITES_IN_VIEW; i ++)
		DUMP_X(DATA_SATELLITES_IN_VIEW_AZIMUTH_(i),		"",		"%5.1f ",	"      ");
	Printf("\nel  ");
	for (i = 0; i < CDatabase::MAX_SATELLITES_IN_VIEW; i ++)
		DUMP_X(DATA_SATELLITES_IN_VIEW_ELEVATION_(i),	"",		"%5.1f ",	"      ");
	Printf("\ncno ");
	for (i = 0; i < CDatabase::MAX_SATELLITES_IN_VIEW; i ++)
		DUMP_X(DATA_SATELLITES_IN_VIEW_STN_RATIO_(i),	"",		"%5.1f ",	"      ");
	Printf("\n");
	
	if (pVar == varO)
	{
		Printf("commit "); DUMP_TS(DATA_COMMIT_TIMESTAMP); Printf("\n");
		Printf("last good "); DUMP_TS(DATA_LASTGOOD_TIMESTAMP); Printf("\n");
		DUMP_X(DATA_LASTGOOD_LATITUDE_DEGREES,					"lat ",		"%11.6f ",	"   ?.?????? ");
		DUMP_X(DATA_LASTGOOD_LONGITUDE_DEGREES,					"lon ",		"%10.6f ",	 "  ?.?????? ");
		DUMP_X(DATA_LASTGOOD_ERROR_RADIUS_METERS,				"err ",		"%7.2f ",	"   ?.?? ");
		Printf("\n");
		DUMP_X(DATA_LASTGOOD_ALTITUDE_ELLIPSOID_METERS,			"alt ",		"%7.2f ",	"   ?.?? ");
		DUMP_X(DATA_LASTGOOD_ALTITUDE_ELLIPSOID_ERROR_METERS,	"err ",		"%7.2f ",	"   ?.?? ");
	}
	// done
	Printf("\n");
}
