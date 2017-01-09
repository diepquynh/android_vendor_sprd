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
  \brief  ULP message decode

  Module for decoding SUPL messagges sent from the SLP to the SET
*/
/*******************************************************************************
 * $Id: upldecod.cpp 62578 2012-10-17 12:14:28Z jon.bowern $
 * $HeadURL: http://svn.u-blox.ch/GPS/SOFTWARE/hmw/android/trunk/gps/supl/upldecod.cpp $
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <time.h>
#include <malloc.h>

#include "upldecod.h"
#include "uplsend.h"
#include "ubx_log.h"

///////////////////////////////////////////////////////////////////////////////
//! Decode a SUPL message
/*! Function for decoding an incoming SUPL message. it will accept the payload of the 
    message as argment, and it will return a pointer to the received structure. 
  \param pBuffer     : payload recieved
  \param size       : size of the payload
  \return             pointer to ULP PDU message
*/
struct ULP_PDU *uplDecode(char *pBuffer, int size)
{
	asn_dec_rval_t rval;
    ULP_PDU_t *pMsg;

    pMsg = (ULP_PDU_t *) MC_CALLOC(sizeof(ULP_PDU_t),1);
    if (pMsg == NULL)
	{
        LOGE("%s: allocation error", __FUNCTION__);
	}

    rval = uper_decode_complete(0, 
                                &asn_DEF_ULP_PDU,
                                (void **) &pMsg,
                                pBuffer,
                                size);

	logSupl(pMsg, true);
    switch(rval.code) {
    case RC_OK:
        LOGV("%s: succeed", __FUNCTION__);
//        asn_fprint(stdout,&asn_DEF_ULP_PDU, pMsg);
        break;
    case RC_FAIL:
        LOGW("%s: Decoding failure... %d", __FUNCTION__, rval.consumed);
        ASN_STRUCT_FREE(asn_DEF_ULP_PDU, pMsg);
        pMsg = NULL;
        break;
    case RC_WMORE:
        LOGW("%s: Want more? %d", __FUNCTION__, rval.consumed);
        ASN_STRUCT_FREE(asn_DEF_ULP_PDU, pMsg);
        pMsg = NULL;
        break;
    }

    return pMsg;
}

///////////////////////////////////////////////////////////////////////////////
//! Extract the session ID
/*! Function for extracting the session ID from a Supl message
  \param pMsg : pointer to the Supl message
  \return Extracted session ID
*/
int extractSid(struct ULP_PDU *pMsg)
{
    /* Verify the prence of the session ID field */
    if (pMsg->sessionID.setSessionID == NULL)
    {
        LOGW("%s: No session ID field... must be a SUPL INIT", __FUNCTION__);
        if ( pMsg->message.present != UlpMessage_PR_msSUPLINIT )
        {
            LOGE("%s: session ID not present AND not SUPL INIT!!!!", __FUNCTION__);
        }
        return 0;
    }

    /* returning the SET session ID, the one we consider for the session */
    return (pMsg->sessionID.setSessionID->sessionId);
}
