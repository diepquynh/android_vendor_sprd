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
  \brief  time utility library

  Module for time functions, header
*/
/*******************************************************************************
 * $Id: ubx_timer.h 60939 2012-08-10 15:29:44Z philippe.kehl $
 ******************************************************************************/
#ifndef __UBX_TIMER_H__
#define __UBX_TIMER_H__

//! Helper function to return a reference time
/*! this function has to be used to calculate aging of of the NTP (inject_time)
	return the current refrence time.
*/
int64_t getMonotonicMsCounter(void);


#endif /* __UBX_TIMER_H__ */
