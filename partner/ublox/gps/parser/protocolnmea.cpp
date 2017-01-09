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
 * $Id: protocolnmea.cpp 49130 2011-07-04 14:28:49Z andrea.foni $
 ******************************************************************************/
#include "stdafx.h"

#include "protocolnmea.h"
#include "parserbuffer.h"
#include "gpsconst.h"

// filter out unresonable values
const double stdDevLimit	= 1000000.0; 
const double dopLimit		= 99.9;

int CProtocolNMEA::Parse(unsigned char* pBuffer, int iSize) 
{
	return ParseFunc(pBuffer, iSize);
}

int CProtocolNMEA::ParseFunc(unsigned char* pBuffer, int iSize) 
{
	// Start
	if (iSize == 0) 
		return CParserBuffer::WAIT;
	if (pBuffer[0] != NMEA_CHAR_SYNC)
		return CParserBuffer::NOT_FOUND;
	if (iSize == 1)
		return CParserBuffer::WAIT;
	int iMaxSize;
	if (pBuffer[1] == 'G')
	{	
		if (iSize == 2)
			return CParserBuffer::WAIT;
		if ((pBuffer[2] != 'P' /* GPS */ ) && (pBuffer[2] != 'L' /* GLO */  ) &&
			(pBuffer[2] != 'A' /* GAL */ ) && (pBuffer[2] != 'N' /* GNSS */ ))
			return CParserBuffer::NOT_FOUND;
		iMaxSize = NMEA_MAX_SIZE;
	}
	else
	{
		if (pBuffer[1] != 'P')
			return CParserBuffer::NOT_FOUND;
		iMaxSize = PUBX_MAX_SIZE;
	}
	// Payload
	for (int i = 1; (i < iSize); i ++)
	{
		if (i == iMaxSize)
			return CParserBuffer::NOT_FOUND;
		else if (pBuffer[i] == '\n')
		{
			// the nmea message is terminated with a cr lf squence, 
			// since we are toleran we allow a lf only also 
			// if it has a checksum, we have to test it 
			int iAsterix = (pBuffer[i-1] == '\r') ? i - 4: i - 3;
			if ((iAsterix > 0) && (pBuffer[iAsterix] == '*'))
			{
				// the checksum consists of two hex digits (usually in upper case, but we are tolerant)
				unsigned char highNibble = pBuffer[iAsterix + 1];
				unsigned char lowNibble  = pBuffer[iAsterix + 2];
				highNibble = (highNibble >= 'A' && highNibble <= 'F') ? highNibble - 'A' + 10 : 
							 (highNibble >= 'a' && highNibble <= 'f') ? highNibble - 'a' + 10 : 
							 (highNibble >= '0' && highNibble <= '9') ? highNibble - '0' : 0xFF ;
				lowNibble  = (lowNibble  >= 'A' && lowNibble  <= 'F') ? lowNibble  - 'A' + 10 : 
							 (lowNibble  >= 'a' && lowNibble  <= 'f') ? lowNibble  - 'a' + 10 : 
							 (lowNibble  >= '0' && lowNibble  <= '9') ? lowNibble  - '0' : 0xFF ;
				if (lowNibble <= 0xF && highNibble <= 0xF)
				{
					int calcCRC = 0;
					for (int i = 1; i < iAsterix; i++)
						calcCRC ^= pBuffer[i];
					// Checksumme der NMEA-Message holen
					int msgCRC = (highNibble << 4) | lowNibble;
					// Checksumme vergleichen
					if (msgCRC != calcCRC)
						return CParserBuffer::NOT_FOUND;
				}
				else
					return CParserBuffer::NOT_FOUND;
			}
			return i+1;
		}
		else if ((!isprint(pBuffer[i])) && (!isspace(pBuffer[i])))
		{
			// a message should contain printable characters only !
			// we are tolerant and tolerate spaces also
			return CParserBuffer::NOT_FOUND;
		}
	}
	return CParserBuffer::WAIT;
}

void CProtocolNMEA::Process(unsigned char* pBuffer, int iSize, CDatabase* pDatabase)
{
	// enable next line if you like noise
	if		(strncmp((char*)&pBuffer[3], "GBS", 3) == 0)	ProcessGBS(pBuffer, iSize, pDatabase);
	else if (strncmp((char*)&pBuffer[3], "GGA", 3) == 0)	ProcessGGA(pBuffer, iSize, pDatabase);
	else if (strncmp((char*)&pBuffer[3], "GLL", 3) == 0)	ProcessGLL(pBuffer, iSize, pDatabase);
	else if (strncmp((char*)&pBuffer[3], "GNS", 3) == 0)	ProcessGNS(pBuffer, iSize, pDatabase);
	else if (strncmp((char*)&pBuffer[3], "GRS", 3) == 0)	ProcessGRS(pBuffer, iSize, pDatabase);
	else if (strncmp((char*)&pBuffer[3], "GSA", 3) == 0)	ProcessGSA(pBuffer, iSize, pDatabase);
	else if (strncmp((char*)&pBuffer[3], "GST", 3) == 0)	ProcessGST(pBuffer, iSize, pDatabase);
	else if (strncmp((char*)&pBuffer[3], "GSV", 3) == 0)	ProcessGSV(pBuffer, iSize, pDatabase);
	else if (strncmp((char*)&pBuffer[3], "RMC", 3) == 0)	ProcessRMC(pBuffer, iSize, pDatabase);
	else if (strncmp((char*)&pBuffer[3], "VTG", 3) == 0)	ProcessVTG(pBuffer, iSize, pDatabase);
	else if (strncmp((char*)&pBuffer[3], "ZDA", 3) == 0)	ProcessZDA(pBuffer, iSize, pDatabase);
}

void CProtocolNMEA::ProcessGBS(unsigned char* pBuffer, int iSize, CDatabase* pDatabase)
{
	pDatabase->MsgOnce(CDatabase::MSG_NMEA_GBS);
	// Time
	CheckSetTime(pBuffer, iSize, pDatabase, 1);
/*
	double d, e;
	// we do publish the nav accuarcies in these fileds 
	// but the NMEA spec says we should publish the "expected error due to bias, with noise = 0"
	if (GetItem(2, pBuffer, iSize, d) && GetItem(3, pBuffer, iSize, e))
	{
		d = sqrt(d*d + e*e);
		pDatabase->Set(CDatabase::DATA_ERROR_RADIUS_METERS, d);
	}
	if (GetItem(4, pBuffer, iSize, d))
	{
		pDatabase->Set(CDatabase::DATA_ALTITUDE_SEALEVEL_ERROR_METERS, d);
		pDatabase->Set(CDatabase::DATA_ALTITUDE_ELLIPSOID_ERROR_METERS, d);
	}
*/
}

void CProtocolNMEA::ProcessGGA(unsigned char* pBuffer, int iSize, CDatabase* pDatabase)
{
	pDatabase->MsgOnce(CDatabase::MSG_NMEA_GGA);
	double d;
	int i;
	char ch;
	// Time
	CheckSetTime(pBuffer, iSize, pDatabase, 1);
	// Position
	SetLatLon(pBuffer, iSize, pDatabase, 2 /* .. 5 */);
	// NMEA: GGA quality indicator
	// '0' = Fix not available or invalid
	// '1' = GPS SPS Mode, fix valid
	// '2' = Diffetential GPS, SPS Mode, fix valid
	// '3' = GPS PPS Mode, fix valid
	// '4' = Real Time Kinematic, System used RTK with fixed integers
	// '5' = Float RTK, Satellite system used in RTK mode, floating integers
	// '6' = Estimated (dead reckoning) Mode
	// '7' = Manual Input Mode
	// '8' = Simulator Mode
	if (GetItem(6, pBuffer, iSize, i) && ((i >= 0) && (i <= 8)))
		pDatabase->Set(CDatabase::DATA_FIX_QUALITY, i); // todo map >=3 to 0..2
	// used SVS
	if (GetItem(7, pBuffer, iSize, i) && (i >= 0))
		pDatabase->Set(CDatabase::DATA_SATELLITES_USED_COUNT, i);
	// horizontal DOP
	if (GetItem(8, pBuffer, iSize, d) && (d < dopLimit))
		pDatabase->Set(CDatabase::DATA_HORIZONAL_DILUTION_OF_PRECISION, d);
	// altitude
	if (GetItem(9, pBuffer, iSize, d) && GetItem(10, pBuffer, iSize, ch) && (toupper(ch) == 'M'))
	{
		pDatabase->Set(CDatabase::DATA_ALTITUDE_SEALEVEL_METERS, d);
		//pDatabase->Set(CDatabase::DATA_ALTITUDE_ANTENNA_SEALEVEL_METERS, d); // todo
	}
	// geodial separation
	if (GetItem(11, pBuffer, iSize, d) && GetItem(12, pBuffer, iSize, ch) && (toupper(ch) == 'M'))
		pDatabase->Set(CDatabase::DATA_GEOIDAL_SEPARATION, d);
	// dgps age
	if (GetItem(13, pBuffer, iSize, i) && (i >= 0))
		pDatabase->Set(CDatabase::DATA_DGPS_DATA_AGE, i);
	// dgps station id
	if (GetItem(14, pBuffer, iSize, i) && (i >= 0) && (i <= 1023))
		pDatabase->Set(CDatabase::DATA_DIFFERENTIAL_REFERENCE_STATION_ID, i);
}

void CProtocolNMEA::ProcessGLL(unsigned char* pBuffer, int iSize, CDatabase* pDatabase)
{
	pDatabase->MsgOnce(CDatabase::MSG_NMEA_GLL);
	// Time
	CheckSetTime(pBuffer, iSize, pDatabase, 5);
	// Position
	SetLatLon(pBuffer, iSize, pDatabase, 1 /* .. 4 */ );
	// Status
	SetStatus(pBuffer, iSize, pDatabase, 6);
	// Mode Indicator
	SetModeIndicator(pBuffer, iSize, pDatabase, 7);
}

void CProtocolNMEA::ProcessGNS(unsigned char* pBuffer, int iSize, CDatabase* pDatabase)
{
	pDatabase->MsgOnce(CDatabase::MSG_NMEA_GNS);
	double d;
	int i;
	// Time
	CheckSetTime(pBuffer, iSize, pDatabase, 1);
	// Position
	SetLatLon(pBuffer, iSize, pDatabase, 2 /* .. 5 */);
#if 0 // windows 7 does not expect parsing this
	{ 
		// NMEA: GNS mode indicator two chars GPS / Glonass
		// A = Autonomous mode
		// D = Differential Mode
		// E = Estimated (dead-reckoning) mode
		// M = Manual Input Mode
		// S = Simulated Mode
		// N = Data Not Valid
		// further fields not mappable to 
		// P = Precise.
		// R = Real Time Kinematic
		// F = Float RTK
		char* pEnd = (char*) &pBuffer[iSize];
		char* pPos = FindPos(6, (char*)pBuffer, pEnd);
		// find the start
		if (pPos && (pEnd > pPos))
		{
			// if not GPS the use Glonass
			char ch = ((pPos[0] == 'N') && (pEnd > pPos + 1)) ? pPos[1] : pPos[0];
			if (MatchChar("ADEMSN", ch, i))
				pDatabase->Set(CDatabase::DATA_GPS_SELECTION_MODE, i);
		}
	}
#endif
	// used SVS
	if (GetItem(7, pBuffer, iSize, i) && (i >= 0))
		pDatabase->Set(CDatabase::DATA_SATELLITES_USED_COUNT, i);
	// horizontal DOP
	if (GetItem(8, pBuffer, iSize, d) && (d < dopLimit))
		pDatabase->Set(CDatabase::DATA_HORIZONAL_DILUTION_OF_PRECISION, d);
	// altitude
	if (GetItem(9, pBuffer, iSize, d))
		pDatabase->Set(CDatabase::DATA_ALTITUDE_SEALEVEL_METERS, d);
	// geodial separation
	if (GetItem(10, pBuffer, iSize, d))
		pDatabase->Set(CDatabase::DATA_GEOIDAL_SEPARATION, d);
	// dgps age
	if (GetItem(11, pBuffer, iSize, i) && (i >= 0))
		pDatabase->Set(CDatabase::DATA_DGPS_DATA_AGE, i);
	// dgps station id
	if (GetItem(12, pBuffer, iSize, i) && (i >= 0) && (i <= 1023))
		pDatabase->Set(CDatabase::DATA_DIFFERENTIAL_REFERENCE_STATION_ID, i);
}

void CProtocolNMEA::ProcessGRS(unsigned char* pBuffer, int iSize, CDatabase* pDatabase)
{
	pDatabase->MsgOnce(CDatabase::MSG_NMEA_GRS);
	// Time
	CheckSetTime(pBuffer, iSize, pDatabase, 1);
	// Mode
	// Residuals
}


void CProtocolNMEA::ProcessGSA(unsigned char* pBuffer, int iSize, CDatabase* pDatabase)
{
	pDatabase->MsgOnce(CDatabase::MSG_NMEA_GSA);
	double d;
	int i;
	char ch;
	if (GetItem(1, pBuffer, iSize, ch) && MatchChar("MA", ch, i))
		pDatabase->Set(CDatabase::DATA_GPS_OPERATION_MODE, i);
	// GSA navigation mode
	// '1' = Fix not available
	// '2' = 2D/DR
	// '3' = 3D
	if (GetItem(2, pBuffer, iSize, i) && (i >= 1) && (i <= 3))
		pDatabase->Set(CDatabase::DATA_FIX_TYPE, i - 1 /*M$ why add 1*/);
	// SVS
	unsigned char svsUsed = 0;
	for (int ix = 0; (ix < 12) && (svsUsed < CDatabase::MAX_SATELLITES_USED); ix ++)
	{
		int i;
		if (GetItem(3+ix, pBuffer, iSize, i))
		{	
			//if ((i >= 33) && (i <= 64)) i += 120-33;
			pDatabase->Set(DATA_SATELLITES_USED_PRNS_(svsUsed), i);
			svsUsed ++;
		}
	}
	pDatabase->Set(CDatabase::DATA_SATELLITES_USED_COUNT, svsUsed);
	// DOP
	if (GetItem(15, pBuffer, iSize, d) && (d < dopLimit))
		pDatabase->Set(CDatabase::DATA_POSITION_DILUTION_OF_PRECISION, d);
	if (GetItem(16, pBuffer, iSize, d) && (d < dopLimit))
		pDatabase->Set(CDatabase::DATA_HORIZONAL_DILUTION_OF_PRECISION, d);
	if (GetItem(17, pBuffer, iSize, d) && (d < dopLimit))
		pDatabase->Set(CDatabase::DATA_VERTICAL_DILUTION_OF_PRECISION, d);
}

void CProtocolNMEA::ProcessGST(unsigned char* pBuffer, int iSize, CDatabase* pDatabase)
{
	double d, e;
	pDatabase->MsgOnce(CDatabase::MSG_NMEA_GST);
	// Time
	CheckSetTime(pBuffer, iSize, pDatabase, 1);
	// RMS
	// Std Dev Maj
	// Std Dev Min
	// Orient
	// Std Dev Lat / Lon 
	if (GetItem(6, pBuffer, iSize, d) && (d < stdDevLimit) && 
		GetItem(7, pBuffer, iSize, e) && (d < stdDevLimit))
	{
		d = sqrt(d*d + e*e);
		pDatabase->Set(CDatabase::DATA_ERROR_RADIUS_METERS, d);
	}
	// Std Dev Alt	
	if (GetItem(8, pBuffer, iSize, d) && (d < stdDevLimit))
	{
		pDatabase->Set(CDatabase::DATA_ALTITUDE_SEALEVEL_ERROR_METERS, d);
		pDatabase->Set(CDatabase::DATA_ALTITUDE_ELLIPSOID_ERROR_METERS, d);
	}
}

void CProtocolNMEA::ProcessGSV(unsigned char* pBuffer, int iSize, CDatabase* pDatabase)
{
	int iMessage, iNumber;
	if (GetItem(1, pBuffer, iSize, iNumber) && GetItem(2, pBuffer, iSize, iMessage) && 
		(iMessage > 0) && (iNumber > 0) && (iMessage <= iNumber))
	{
		if (iMessage == iNumber) // when done set number 
			pDatabase->MsgOnce(CDatabase::MSG_NMEA_GSV_1);
		int iChannels;
		if (GetItem(3, pBuffer, iSize, iChannels))
		{
			pDatabase->Set(CDatabase::DATA_SATELLITES_IN_VIEW, iChannels);
			if (iChannels > 0)
			{
				int ix, ixInView;
				for ( ix = 0,     ixInView = ix + (iMessage - 1) * 4; 
					 (ix < 4) && (ixInView < iChannels) && (ixInView < CDatabase::MAX_SATELLITES_IN_VIEW); 
					  ix++,       ixInView ++) 
				{
					int i;
					// prn
					if (GetItem(4*ix+4, pBuffer, iSize, i))
					{
						double d, az, el;
						//if ((i >= 33) && (i <= 64))	i += 120-33;
						pDatabase->Set(DATA_SATELLITES_IN_VIEW_PRNS_(ixInView), i);
						// cno
						if (GetItem(4*ix+7, pBuffer, iSize, d) && (d > 0.0) && (d < 70.0))
							pDatabase->Set(DATA_SATELLITES_IN_VIEW_STN_RATIO_(ixInView), d);
						// el / az
						if ( GetItem(4*ix+5, pBuffer, iSize, el) && (el >=  -90.0) && (el <=  90.0) && 
							 GetItem(4*ix+6, pBuffer, iSize, az) && (az >= -180.0) && (az <= 360.0)
							 // && (el || az) /* some receivers report 0/0 if az cannot be determined*/
							 )
						{
							pDatabase->Set(DATA_SATELLITES_IN_VIEW_ELEVATION_(ixInView), el);
							pDatabase->Set(DATA_SATELLITES_IN_VIEW_AZIMUTH_(ixInView),	 CDatabase::Degrees360(az));
						}
					}
				}
			}
		}
	}
}

void CProtocolNMEA::ProcessRMC(unsigned char* pBuffer, int iSize, CDatabase* pDatabase)
{
	pDatabase->MsgOnce(CDatabase::MSG_NMEA_RMC);
	char ch;
	double d, second;
	int year, month, day, hour, minute, i;
	// Time / Date
	bool bTimeOk = GetItem(1, pBuffer, iSize, d) && CalcTime(d, hour, minute, second);
	bool bDateOk = GetItem(9, pBuffer, iSize, i) && CalcDate(i, day, month, year);
	if (bDateOk)
	{
		if (year < 80)
			year += 2000;
		else 
			year += 1900;
	}
	if ((bTimeOk && !pDatabase->CheckTime(hour,minute,second)) ||
		(bDateOk && !pDatabase->CheckDate(year,month,day)))
	{
		pDatabase->Commit();			
	}
	if (bTimeOk)
	{
		pDatabase->Set(CDatabase::DATA_TIME_HOUR,   hour);
		pDatabase->Set(CDatabase::DATA_TIME_MINUTE, minute);
		pDatabase->Set(CDatabase::DATA_TIME_SECOND, second);
	}
	if (bDateOk)
	{
		pDatabase->Set(CDatabase::DATA_DATE_YEAR,  year);
		pDatabase->Set(CDatabase::DATA_DATE_MONTH, month);
		pDatabase->Set(CDatabase::DATA_DATE_DAY,   day);
	}
	// RMC/GLL status
	SetStatus(pBuffer, iSize, pDatabase, 2);
	// Lat / Lon
	SetLatLon(pBuffer, iSize, pDatabase, 3 /* .. 6 */);
	// SOG
	if (GetItem(7, pBuffer, iSize, d) && (d >= 0.0))
		pDatabase->Set(CDatabase::DATA_SPEED_KNOTS,  d);
	// COG
	if (GetItem(8, pBuffer, iSize, d) && (d >= -180.0) && (d <= 360.0))
	{
		d = (d < 0.0) ? d + 360.0 : d;
		pDatabase->Set(CDatabase::DATA_TRUE_HEADING_DEGREES,  d);
	}
	// COG Mag
	if (GetItem(10, pBuffer, iSize, d) && GetItem(11, pBuffer, iSize, ch) && (d >= 0.0) && (d <= 180.0))
	{
		ch = (char) toupper(ch); // be tolerant
		// (E)ast subtracts from true course
		// (W)est adds to true course
		if (ch == 'W')
			pDatabase->Set(CDatabase::DATA_MAGNETIC_VARIATION,  d);
		else if (ch == 'E')
			pDatabase->Set(CDatabase::DATA_MAGNETIC_VARIATION, -d);
	}
	// Mode Indicator
	SetModeIndicator(pBuffer, iSize, pDatabase, 12);
}

void CProtocolNMEA::ProcessVTG(unsigned char* pBuffer, int iSize, CDatabase* pDatabase)
{
	pDatabase->MsgOnce(CDatabase::MSG_NMEA_VTG);
	double d;
	char ch;
	// COG (true)
	if (GetItem(1, pBuffer, iSize, d) && (d >= -180.0) && (d <= 360.0) && 
		GetItem(2, pBuffer, iSize, ch) && (toupper(ch) == _T('T')))
		pDatabase->Set(CDatabase::DATA_TRUE_HEADING_DEGREES, CDatabase::Degrees360(d));
	// COG (magnetic) 
	if (GetItem(3, pBuffer, iSize, d) && (d >= -180.0) && (d <= 360.0) && 
		GetItem(4, pBuffer, iSize, ch) && (toupper(ch) == _T('M')))
		pDatabase->Set(CDatabase::DATA_MAGNETIC_HEADING_DEGREES, CDatabase::Degrees360(d));
	// SOG (knots) 
	if (GetItem(5, pBuffer, iSize, d) && (d >= 0.0) && 
		GetItem(6, pBuffer, iSize, ch) && (toupper(ch) == _T('N')))
	{
		pDatabase->Set(CDatabase::DATA_SPEED_KNOTS, d);
	}
	// SOG (km/hr) 
	else if (GetItem(7, pBuffer, iSize, d) && (d >= 0.0) && 
			 GetItem(8, pBuffer, iSize, ch) && (toupper(ch) == _T('K')))
	{
		pDatabase->Set(CDatabase::DATA_SPEED_KNOTS, d * 3600.0 / METERS_PER_NAUTICAL_MILE);
	}
	// Mode Indicator
	SetModeIndicator(pBuffer, iSize, pDatabase, 9);
}

void CProtocolNMEA::ProcessZDA(unsigned char* pBuffer, int iSize, CDatabase* pDatabase)
{
	pDatabase->MsgOnce(CDatabase::MSG_NMEA_ZDA);
	double d, second;
	int year, month, day, hour, minute;
	bool bTimeOk = GetItem(1, pBuffer, iSize, d) && 
				   CalcTime(d, hour, minute, second);
	bool bDateOk = GetItem(2, pBuffer, iSize, day) && 
				   GetItem(3, pBuffer, iSize, month) && 
				   GetItem(4, pBuffer, iSize, year);
	if ((bTimeOk && !pDatabase->CheckTime(hour,minute,second)) ||
		(bDateOk && !pDatabase->CheckDate(year,month,day)))
	{
		pDatabase->Commit();			
	}
	if (bTimeOk)
	{
		pDatabase->Set(CDatabase::DATA_TIME_HOUR,   hour);
		pDatabase->Set(CDatabase::DATA_TIME_MINUTE, minute);
		pDatabase->Set(CDatabase::DATA_TIME_SECOND, second);
	}
	if (bDateOk)
	{
		pDatabase->Set(CDatabase::DATA_DATE_YEAR,  year);
		pDatabase->Set(CDatabase::DATA_DATE_MONTH, month);
		pDatabase->Set(CDatabase::DATA_DATE_DAY,   day);
	}
}

// HELPER for NMEA decoding 

void CProtocolNMEA::SetLatLon(unsigned char* pBuffer, int iSize, CDatabase* pDatabase, int ix)
{
	double d;
	char ch;
	if (GetItem(ix, pBuffer, iSize, d)   && GetItem(ix+1, pBuffer, iSize, ch) && CalcLat(ch, d))
		pDatabase->Set(CDatabase::DATA_LATITUDE_DEGREES,  d);
	// Position Longitude
	if (GetItem(ix+2, pBuffer, iSize, d) && GetItem(ix+3, pBuffer, iSize, ch) && CalcLon(ch, d))
		pDatabase->Set(CDatabase::DATA_LONGITUDE_DEGREES, d);
}	

void CProtocolNMEA::SetStatus(unsigned char* pBuffer, int iSize, CDatabase* pDatabase, int ix)
{
	int i;
	char ch;
	// NMEA: RMC/GLL status
	// 'A' = Data valid
	// 'V' = Navigation receiver warning (inValid)
	if (GetItem(ix, pBuffer, iSize, ch) && MatchChar("AV", ch, i))
		pDatabase->Set(CDatabase::DATA_GPS_STATUS, i + 1 /* M$ why add 1 */);
}

void CProtocolNMEA::SetModeIndicator(unsigned char* pBuffer, int iSize, CDatabase* pDatabase, int ix)
{
	int i;
	char ch;
	// Mode Indicator
	// NMEA: GLL/RMC/VTG positioning mode indicator (NMEA ver >= 2.3)
	// 'N' = Data not valid
	// 'E' = Estimated (dead reckoning) mode
	// 'D' = Differential mode
	// 'A' = Autonomous mode
	// 'M' = Manual input mode
	// 'S' = Simulator mode
	if (GetItem(ix, pBuffer, iSize, ch) && MatchChar("ADEMSN", ch, i))
		pDatabase->Set(CDatabase::DATA_GPS_SELECTION_MODE, i);
}

void CProtocolNMEA::CheckSetTime(unsigned char* pBuffer, int iSize, CDatabase* pDatabase, int ix)
{
	double d, second;
	int hour, minute;
	bool bTimeOk = GetItem(ix, pBuffer, iSize, d) && CalcTime(d, hour, minute, second);
	if (bTimeOk && !pDatabase->CheckTime(hour,minute,second))
	{
		pDatabase->Commit();			
	}
	if (bTimeOk)
	{
		pDatabase->Set(CDatabase::DATA_TIME_HOUR,   hour);
		pDatabase->Set(CDatabase::DATA_TIME_MINUTE, minute);
		pDatabase->Set(CDatabase::DATA_TIME_SECOND, second);
	}
}	

int CProtocolNMEA::GetItemCount(unsigned char* pBuffer, int iSize)
{
	char* pEnd = (char*) &pBuffer[iSize];
	// find the start
	int iCount = 0;
	for (char* pPos = (char*) pBuffer; (pPos < pEnd); pPos ++)
	{
		if (*pPos == ',')
			iCount ++;
	}
	return iCount;
}

#if 0
const char* CProtocolNMEA::GetItem(int iIndex, unsigned char* pBuffer, int iSize)
{
	// copy the item to the static buffer
	static char s_pBuffer[NMEA_MAX_SIZE+1];
	// find the start
	char* pEnd = (char*) &pBuffer[iSize];
	char* pPos;
	for (pPos = (char*) pBuffer; (pPos < pEnd) && (iIndex > 0); pPos ++)
	{
		if (*pPos == ',')
			iIndex --;
	}
	if ((iIndex == 0) && (pPos < pEnd))
	{
		// start found check end
		char* pStart = pPos;
		for (; (pPos < pEnd); pPos ++)
		{
			if ((*pPos == ',') || (*pPos == '*') || (*pPos == '\r') || (*pPos == '\n'))
			{
				// end sequence found
				int iLen = (int)(pPos - pStart);
				// copy the string
				ASSERT(iLen < NMEA_MAX_SIZE);
#ifdef _UNICODE
				VERIFY(::Multiunsigned charToWideChar(CP_ACP, 0, (LPCSTR) pStart, iLen, s_pBuffer, NMEA_MAXIMUMSIZE));
#else
				CopyMemory(s_pBuffer, pStart, iLen);
#endif
				// terminate the string
				s_pBuffer[iLen] = _T('\0');
				return s_pBuffer;
			}
		}
	}
	// no data
	return NULL;
}
#endif

char* CProtocolNMEA::FindPos(int iIndex, char* pStart, char* pEnd)
{
	// find the start
	for (; (pStart < pEnd) && (iIndex > 0); pStart ++)
	{
		if (*pStart == ',')
			iIndex --;
	}
	// found and check bounds
	if ((iIndex == 0) && (pStart < pEnd) && 
		(*pStart != ',') && (*pStart != '*') && (*pStart != '\r') && (*pStart != '\n'))
		return pStart;
	else 
		return NULL;
}

bool CProtocolNMEA::GetItem(int iIndex, unsigned char* pBuffer, int iSize, double& dValue)
{
	char* pEnd = (char*) &pBuffer[iSize];
	char* pPos = FindPos(iIndex, (char*)pBuffer, pEnd);
	// find the start
	if (!pPos || (pEnd <= pPos))
		return false;
	char* pTemp;
	// M$ specific - because the strtod function uses a strlen we make sure that 
	// the string is zero terminated, this ensures correct behaviour of the function 
	char chEnd = pEnd[-1];
	pEnd[-1] = '\0';
	dValue = strtod(pPos, &pTemp);
	// restore the last character
	pEnd[-1] = chEnd;
	return (pTemp > pPos);
}

bool CProtocolNMEA::GetItem(int iIndex, unsigned char* pBuffer, int iSize, int& iValue, int iBase /*=10*/)
{
	char* pEnd = (char*) &pBuffer[iSize];
	char* pPos = FindPos(iIndex, (char*)pBuffer, pEnd);
	// find the start
	if (!pPos)
		return false;
	char* pTemp;
	iValue = (int) strtol(pPos, &pTemp, iBase);
	return (pTemp > pPos);
}

bool CProtocolNMEA::GetItem(int iIndex, unsigned char* pBuffer, int iSize, char& chValue)
{
	char* pEnd = (char*) &pBuffer[iSize];
	char* pPos = FindPos(iIndex, (char*)pBuffer, pEnd);
	// find the start
	if (!pPos)
		return false;
	// skip leading spaces
	while ((pPos < pEnd) && isspace(*pPos))
		pPos++;
	// check bound
	if ((pPos < pEnd) && 
		(*pPos != ',') && (*pPos != '*') && (*pPos != '\r') && (*pPos != '\n'))
	{
		chValue = *pPos;
		return true;
	}
	return false;
}

// special helpers for converting items in nmea formats

bool CProtocolNMEA::MatchChar(const char* string, char ch, int& i)
{
	if (ch)
	{
		const char* p;
		ch = (char) toupper(ch); // be tolerant
		p = strchr(string, ch);
		if (p)
		{
			i = (int)(p - string); // calc index
			return true;
		}
	}
	return false;
}

double CProtocolNMEA::CalcAngle(double d)
{
	// converts an angle in the format dddmm.mmmm to radians
	int t;
	t = (int) (d / 100);
	return (((d - (t * 100)) / 60) + t);
}

bool CProtocolNMEA::CalcLon(char ch, double& d)
{
	// be tolerant
	ch = (char) toupper(ch);
	// convert lon in nmea format to degrees
	if (ch == _T('E'))
		d = CalcAngle(d);
	else if (ch == _T('W'))
		d = - CalcAngle(d);
	else
		return false;
	if (d > 180.0)
		d -= 360.0;
	return (d >= -180.0) && (d <= 180.0);
}

bool CProtocolNMEA::CalcLat(char ch, double& d)
{
	// be tolerant
	ch = (char) toupper(ch);
	// convert lat in nmead format to degrees
	if (ch == _T('N'))
		d = CalcAngle(d);
	else if (ch == _T('S'))
		d = - CalcAngle(d);
	else
		return false;
	if (d > 180.0)
		d -= 360.0;
	return (d >= -180.0) && (d <= 180.0);
}

bool CProtocolNMEA::CalcTime(double d, int& h, int& m, double& s)
{
	// extracts hh, mm and ss.ssss from a value in the format hhmmss.ssss
	if ((d >= 1000000.0) || (d < 0.0))
		return false;
	int t = (int) d / 100;
	m = t % 100;
	h = (t - m) / 100;
	s = d - t * 100;
	return (h >= 0) && (h <= 23) && (m >= 0) && (m < 60) && (s >= 0.0) && (s < 60.0);
}

bool CProtocolNMEA::CalcDate(int d, int& day, int& mon, int& yr)
{
	// extracts yy, mm and dd from a value in the format ddmmyy
	if ((d > 999999) || (d < 0))
		return false;
	yr = d % 100;
	d /= 100;
	mon = d % 100;
	d /= 100;
	day = d % 100;
	return (day >= 1) && (day <= 31) && (mon >= 1) && (mon <= 12) && (yr >= 0) && (yr <= 99);
}
