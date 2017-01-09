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
 * $Id: protocolunknown.h 49130 2011-07-04 14:28:49Z andrea.foni $
 ******************************************************************************/
#ifndef __PROTOCOLUNKNOWN_H__
#define __PROTOCOLUNKNOWN_H__

#include "protocol.h"


class CProtocolUnknown : public CProtocol
{
public:
	int Parse(unsigned char* pBuffer, int iSize);
	void Process(unsigned char* pBuffer, int iSize, CDatabase* pDatabase);
	PROTOCOL_t GetType(void) { return UNKNOWN; }
};


#endif //__PROTOCOLUNKNOWN_H__
