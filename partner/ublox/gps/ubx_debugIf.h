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
 * $Id: ubx_debugIf.h 60939 2012-08-10 15:29:44Z philippe.kehl $
 ******************************************************************************/
#ifndef __UBX_DEBUGIF_H__
#define __UBX_DEBUGIF_H__

#include "std_inc.h"

class CDebugIf
{
public:
    static const void *getIf() { return &s_interface; }

private:
	// interface
    static size_t getInternalState(char* buffer, size_t bufferSize);

	// variables
    const static GpsDebugInterface s_interface;
};

#endif /* __UBX_DEBUGIF_H__ */
