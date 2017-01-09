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
 * $Id: uplsend.h 61586 2012-09-03 09:15:35Z jon.bowern $
 * $HeadURL: http://svn.u-blox.ch/GPS/SOFTWARE/hmw/android/trunk/gps/supl/uplsend.h $
 ******************************************************************************/

#ifndef __ULPSEND_H__
#define __ULPSEND_H__

#include "ULP-PDU.h"
#include <openssl/bio.h>
#include "ubx_localDb.h"
#include "ubx_rilIf.h"

///////////////////////////////////////////////////////////////////////////////
// Types & Definitions

//! structure passed as parameter to the function SuplEnd
typedef struct suplEndParam
{
    int sid;                //!< SET session Id
    SlpSessionID_t *slpd;   //!< SLP session Id
    StatusCode_t status;    //!< status
    int posEn;              //!< if 1, retrieve position
    int verEn;              //!< if 1, retrieve hash
    long long hash;         //!< hash code when requested
} suplEndParam_t;

//! structure passed as parameter to the function SuplPos
typedef struct suplPosParam
{
    int sid;                //!< SET session Id
    SlpSessionID_t *slpd;   //!< SLP session Id
    char *buffer;           //!< buffer containing the RRLP payload
    int size;               //!< size of the payload
} suplPosParam_t;

//! structure passed as parameter to the function SuplPosInit
typedef struct suplPosInitParam
{
    int sid;                		//!< SET session Id
    SlpSessionID_t *slpd;   		//!< SLP session Id
    int posEn;              		//!< if 1, send position to server
    int assEn;              		//!< if 1, ask for assisted data from server
    int verEn;              		//!< if 1, send hash to server
    long long hash;         		//!< hash code when requested
	PosMethod_t requestedPosMethod;	//!< Type of position method to request
	double lat;						//!< lat to send to server
	double lon;						//!< lon to send to server
} suplPosInitParam_t;

///////////////////////////////////////////////////////////////////////////////
// Functions
int sendSuplStart(BIO *bio, int sid);
int sendSuplEnd(BIO *bio, int sid, suplEndParam_t *pParam);
int sendSuplPos(BIO *bio, int sid, suplPosParam_t *pParam);
int sendSuplPosInit(BIO *bio, int sid, suplPosInitParam_t *pParam);
SlpSessionID_t *copySlpId(SlpSessionID_t *pOrig);

#endif /* __ULPSEND_H__ */
