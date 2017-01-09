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
 * $Id: protocolnmea.h 49130 2011-07-04 14:28:49Z andrea.foni $
 ******************************************************************************/
#ifndef __PROTOCOLNMEA_H__
#define __PROTOCOLNMEA_H__

#include "protocol.h"

class CProtocolNMEA : public CProtocol
{
public:
	enum { 
		NMEA_CHAR_SYNC = 36 /* '$' */,
		NMEA_MAX_SIZE  = 82 /* this is the limit of the NMEA standard */,
		PUBX_MAX_SIZE  = 512
	};

	int Parse(unsigned char* pBuffer, int iSize); 
	static int ParseFunc(unsigned char* pBuffer, int iSize); 
	void __drv_floatUsed Process(unsigned char* pBuffer, int iSize, CDatabase* pDatabase);
	PROTOCOL_t GetType(void) { return NMEA; }

protected:
	void __drv_floatUsed ProcessGBS(unsigned char* pBuffer, int iSize, CDatabase* pDatabase);
	void __drv_floatUsed ProcessGGA(unsigned char* pBuffer, int iSize, CDatabase* pDatabase);
	void __drv_floatUsed ProcessGLL(unsigned char* pBuffer, int iSize, CDatabase* pDatabase);
	void __drv_floatUsed ProcessGNS(unsigned char* pBuffer, int iSize, CDatabase* pDatabase);
	void __drv_floatUsed ProcessGRS(unsigned char* pBuffer, int iSize, CDatabase* pDatabase);
	void __drv_floatUsed ProcessGSA(unsigned char* pBuffer, int iSize, CDatabase* pDatabase);
	void __drv_floatUsed ProcessGST(unsigned char* pBuffer, int iSize, CDatabase* pDatabase);
	void __drv_floatUsed ProcessGSV(unsigned char* pBuffer, int iSize, CDatabase* pDatabase);
	void __drv_floatUsed ProcessRMC(unsigned char* pBuffer, int iSize, CDatabase* pDatabase);
	void __drv_floatUsed ProcessVTG(unsigned char* pBuffer, int iSize, CDatabase* pDatabase);
	void __drv_floatUsed ProcessZDA(unsigned char* pBuffer, int iSize, CDatabase* pDatabase);
	
	static __drv_floatUsed void SetLatLon(unsigned char* pBuffer, int iSize, CDatabase* pDatabase, int ix);
	static void SetStatus(unsigned char* pBuffer, int iSize, CDatabase* pDatabase, int ix);
	static void SetModeIndicator(unsigned char* pBuffer, int iSize, CDatabase* pDatabase, int ix);
	static __drv_floatUsed void CheckSetTime(unsigned char* pBuffer, int iSize, CDatabase* pDatabase, int ix);

	static int GetItemCount(unsigned char* pBuffer, int iSize);
	static const char* GetItem(int iIndex, unsigned char* pBuffer, int iSize);
	static char* FindPos(int iIndex, char* pStart, char* pEnd);
	static bool __drv_floatUsed GetItem(int iIndex, unsigned char* pBuffer, int iSize, double& dValue);
	static bool GetItem(int iIndex, unsigned char* pBuffer, int iSize, int& iValue, int iBase = 10);
	static bool GetItem(int iIndex, unsigned char* pBuffer, int iSize, char& ch);
	static bool MatchChar(const char* string, char ch, int& i);
	static __drv_floatUsed double Limit360(double);
	static double CalcAngle(double d);
	static bool __drv_floatUsed CalcLon(char ch, double& d);
	static bool __drv_floatUsed CalcLat(char ch, double& d);
	static bool CalcTime(double d, int& h, int& m, double& s);
	static bool CalcDate(int d, int& day, int& mon, int& yr);
};

#endif //__PROTOCOLNMEA_H__
