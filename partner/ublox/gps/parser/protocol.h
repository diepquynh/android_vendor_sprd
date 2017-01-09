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
 * $Id: protocol.h 49130 2011-07-04 14:28:49Z andrea.foni $
 ******************************************************************************/
#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include "database.h"

class CProtocol
{
public:
	typedef enum 
	{
		UNKNOWN		= -1,
		UBX			=  0,
		NMEA		=  1
	} PROTOCOL_t;

	CProtocol(void) {}
	virtual ~CProtocol(void) {}
	virtual int Parse(unsigned char* pBuffer, int iSize) = 0;
	virtual void Process(unsigned char* pBuffer, int iSize, CDatabase* pDatabase) = 0;
	virtual PROTOCOL_t GetType(void) = 0;
};

#endif //__PROTOCOL_H__
