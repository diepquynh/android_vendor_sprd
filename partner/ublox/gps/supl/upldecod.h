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
 * Project: SUPL
 *
 ******************************************************************************/
/*!
  \file
  \brief  ULP message manager interface

  Interface description of the module for managing SUPL messagges  sent 
  from the SET to the SPL
*/
/*******************************************************************************
 * $Id: upldecod.h 60663 2012-08-01 15:06:00Z jon.bowern $
 * $HeadURL: http://svn.u-blox.ch/GPS/SOFTWARE/hmw/android/trunk/gps/supl/upldecod.h $
 ******************************************************************************/

#ifndef __UPLDECOD_H__
#define __UPLDECOD_H__

#include "ULP-PDU.h"

///////////////////////////////////////////////////////////////////////////////
// Functions
struct ULP_PDU *uplDecode(char *pBuffer, int size);
int extractSid(struct ULP_PDU *pMsg);

#endif /* __UPLDECOD_H__ */
