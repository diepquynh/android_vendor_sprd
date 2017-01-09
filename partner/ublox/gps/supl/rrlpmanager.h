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
  \brief  RRLP proecssingSET state Machine administrator interface

  Managing and allocating of the state machines for SUPL SET
*/
/*******************************************************************************
 * $Id: rrlpmanager.h 62618 2012-10-19 01:59:41Z michael.ammann $
 * $HeadURL: http://svn.u-blox.ch/GPS/SOFTWARE/hmw/android/trunk/gps/supl/rrlpmanager.h $
 ******************************************************************************/
#ifndef __RRLPMANAGER_H__
#define __RRLPMANAGER_H__

#include "ubx_localDb.h"

///////////////////////////////////////////////////////////////////////////////
// Types & Definitions

typedef enum 
{
	RESP_NONE = 0,
	RESP_POSITION_DATA,
	RESP_MSA_DATA,
} RrlpRespData_t;

//! structure passed as parameter to the function rrlpProcessing
typedef struct
{
    int responseTime;       		//!< requested responce time
    int referenceNumber;    		//!< RRLP session reference number to be given in the answer
    int reqAccuracy;        		//!< Requested horizontal accuracy (in cm)
	RrlpRespData_t responseType;	//!< Type of response needed
	bool assistDataReceived;		//!< Flag if assistance data has been received
} aux_t;

///////////////////////////////////////////////////////////////////////////////
// Functions

char *rrlpProcessing(int sid, unsigned char *inBuffer, int inSize, int *pOutSize, aux_t *pAux);
char *buildRrlpPosResp(int *pOutSize, int referenceNumber, double lat, double lon, double alt, double tow);
char* buildRrlpPosRespTimeout(int *pOutSize, int referenceNumber);
char* buildRrlpMsaPosResp(int sid, int *pOutSize, int referenceNumber);

#endif /* __RRLPMANAGER_H__ */
