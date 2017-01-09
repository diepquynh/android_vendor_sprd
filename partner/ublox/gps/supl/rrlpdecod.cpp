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
  \brief  RRLP message decode

  Module for decoding RRLP messagges sent from the SLP to the SET
*/
/*******************************************************************************
 * $Id: rrlpdecod.cpp 62578 2012-10-17 12:14:28Z jon.bowern $
 * $HeadURL: http://svn.u-blox.ch/GPS/SOFTWARE/hmw/android/trunk/gps/supl/rrlpdecod.cpp $
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <malloc.h>

#include "rrlpdecod.h"
#include "ubx_log.h"

/////////////////////////////////////////////////////////////////////////////////////////
// Types & Definitions

// Debug aid
//#define SUPL_LOG_RRLP

/////////////////////////////////////////////////////////////////////////////////////////
// Functions

//! Decode a RRLP message
/*! Function for decoding an incoming RRLP message. It will accept the payload of the 
    message as argment, and it will return a pointer to a structure repesenting the 
    decoded message. 
  \param pBuffer : Pointer to the buffer conrtaining the received RRLP message to decode
  \param size    : Size of the buffer
  \return Pointer to the decoded PDU/RRLP message
*/
struct PDU *rrlpDecode(unsigned char *pBuffer, int size)
{
	asn_dec_rval_t rval;
    PDU_t *pMsg;

    pMsg = (PDU_t *) MC_CALLOC(sizeof(PDU_t),1);
    if (pMsg == NULL)
	{
        LOGE("%s: allocation error", __FUNCTION__);
	}

#ifdef SUPL_LOG_RRLP
    FILE *f = fopen("/data/rrlp.bin", "w");
	if (f)
	{
		int res = 0;
		if ((res = fwrite(pBuffer,1,size,f)) != size)
		{
			LOGE("%s: error in writing to the file %d out of %d",__FUNCTION__, res,size);
		}
		else
		{
			LOGI("%s: saved to file %d bytes",__FUNCTION__, size);
		}
		fclose(f);
	}
#endif

	LOGV("%s: inBuffer %p, inSize %i", __FUNCTION__, pBuffer, size);
    rval = uper_decode_complete(0, 
                                &asn_DEF_PDU,
                                (void **) &pMsg,
                                pBuffer,
                                size);
	logRRLP(pMsg, true);
    switch(rval.code) 
	{
    case RC_OK:
        LOGV("%s: succeed", __FUNCTION__);
//        asn_fprint(stdout,&asn_DEF_PDU, pMsg);
        break;
		
    case RC_FAIL:
        LOGW("%s: Decoding failure... %d", __FUNCTION__, rval.consumed);
        ASN_STRUCT_FREE(asn_DEF_PDU, pMsg);
        pMsg = NULL;
        break;
		
    case RC_WMORE:
        LOGW("%s: Want more? %d",__FUNCTION__, rval.consumed);
        //ASN_STRUCT_FREE(asn_DEF_PDU, pMsg);
        //pMsg = NULL;
		// But will keep going anyway
        break;
    }

    return pMsg;
}
