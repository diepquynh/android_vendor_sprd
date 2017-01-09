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
  \brief  RRLP message manager interface

  Interface description of the module for managing RRLP messagges  sent 
  from the SPL to the SET
*/
/*******************************************************************************
 * $Id: rrlpdecod.h 60663 2012-08-01 15:06:00Z jon.bowern $
 * $HeadURL: http://svn.u-blox.ch/GPS/SOFTWARE/hmw/android/trunk/gps/supl/rrlpdecod.h $
 ******************************************************************************/

#ifndef __RRLPDECODE_H__
#define __RRLPDECODE_H__

#include "PDU.h"

///////////////////////////////////////////////////////////////////////////////
// Functions
struct PDU *rrlpDecode(unsigned char *pBuffer, int size);

#endif /* __RRLPDECODE_H__ */
