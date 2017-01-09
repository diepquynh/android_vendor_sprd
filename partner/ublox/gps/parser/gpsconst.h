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
 * $Id: gpsconst.h 49130 2011-07-04 14:28:49Z andrea.foni $
 ******************************************************************************/
#ifndef __GPSCONST_H__
#define __GPSCONST_H__

const double Pi		= 3.1415926535898;		// WGS 84 value of pi

const double F		= (1/298.2572235630);
const double A		= 6378137.0;
const double B		= (A * (1-F));
const double E1SQR	= ((A*A - B*B) / (A*A));
const double E2SQR	= ((A*A - B*B) / (B*B));

const double RADIANS_PER_DEGREE = (Pi / 180.0);
const double DEGREES_PER_RADIAN	= (180.0 / Pi);
const double SECONDS_PER_RADIAN	= (DEGREES_PER_RADIAN * 3600.0);

const double METERS_PER_NAUTICAL_MILE	= (1853.32055);
const double LAT_METERS_PER_DEGREE		= (METERS_PER_NAUTICAL_MILE * 60.0);
const double KNOTS_PER_METER			= (0.3048 * 6076.0);

const double C_SOL	= 299792458.0;

#if 0
// Param: Utc seconds since midnight, January 1, 1970
inline double CalcLeapSeconds(double dUtcSec)
{
	const struct 
	{
		double dTime; 
		unsigned short wYear;
		unsigned char byMonth;
		unsigned char byDay;
		double dLeapSecs;
	} LUT[] = {
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
	int i;

#if 0
	// this code should be used to verify the new leap seconds in the table
	for (i=0;i<sizeof(LUT)/sizeof(*LUT);i++)
	{
		struct tm ti;
		memset(&ti, 0, sizeof(struct tm));
		ti.tm_year = LUT[i].wYear - 1900;
		ti.tm_mon = LUT[i].byMonth - 1;
		ti.tm_mday = LUT[i].byDay;
		TRACE("{ %10d, %d, %d, %d, %2.0f }\n", (time_t)mktime(&ti) - _timezone, LUT[i].wYear, LUT[i].byMonth, LUT[i].byDay, LUT[i].dLeapSecs);
	}
#endif

	for (i=0;i<sizeof(LUT)/sizeof(*LUT);i++) {
	  if (dUtcSec > LUT[i].dTime) 
		  return LUT[i].dLeapSecs;
	}
	return 0;
}
#endif

#endif	// __GPSCONST_H__
