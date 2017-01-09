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
 * $Id: protocolunknown.cpp 49130 2011-07-04 14:28:49Z andrea.foni $
 ******************************************************************************/
#include "stdafx.h"

#include "protocolunknown.h"
#include "parserbuffer.h"

int CProtocolUnknown::Parse(unsigned char* /*pBuffer*/, int /*iSize*/)
{
	return CParserBuffer::NOT_FOUND;
}
void CProtocolUnknown::Process(unsigned char* /*pBuffer*/, int /*iSize*/, CDatabase* /*pDatabase*/)
{
}
