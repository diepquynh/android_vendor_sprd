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
 * $Id: ubx_debugIf.cpp 60939 2012-08-10 15:29:44Z philippe.kehl $
 ******************************************************************************/

#include "ubx_debugIf.h"

static CDebugIf s_myIf;

const GpsDebugInterface CDebugIf::s_interface = {
    size:                   sizeof(GpsDebugInterface),
    get_internal_state:     getInternalState,
};

size_t CDebugIf::getInternalState(char* buffer, size_t bufferSize)
{
    LOGV("CDebugIf::%s : size=%d", __FUNCTION__, bufferSize);
    return 0;
}
