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

  Module for time functions
*/
/*******************************************************************************
 * $Id: ubx_timer.cpp 60939 2012-08-10 15:29:44Z philippe.kehl $
 ******************************************************************************/
#include <time.h>
#include "std_types.h"
#include "ubx_timer.h"

int64_t getMonotonicMsCounter(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (int64_t)ts.tv_sec*1000 + ts.tv_nsec/1000000;
}	
