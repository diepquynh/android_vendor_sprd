/* --------------------------------------------------------------------------
 * Confidential
 *                    SiliconFile Technologies Inc.
 *
 *                   Copyright (c) 2015 SFT Limited
 *                          All rights reserved
 *
 * Project  : SPRD_AF
 * Filename : AF_Main.h
 *
 * $Revision: v1.00
 * $Author 	: silencekjj 
 * $Date 	: 2015.02.11
 *
 * --------------------------------------------------------------------------
 * Feature 	: DW9804(VCM IC)
 * Note 	: vim(ts=4, sw=4)
 * ------------------------------------------------------------------------ */
#ifndef	__AF_MAIN_H__
#define	__AF_MAIN_H__

/* ------------------------------------------------------------------------ */

#include "AF_Common.h"
#include "sp_af_ctrl.h"
/* ------------------------------------------------------------------------ */
void SFT_AF_LOG_DEBUG(struct sft_af_ctrl_ops *af_ops, BYTE bEnb, const char* format, ...);
BYTE AF_GetFv(			sft_af_handle_t handle);
void AF_SetFvTh(		sft_af_handle_t handle, BYTE bCAF);
BYTE AF_ISR(			sft_af_handle_t handle);
void AF_SoftLanding(	sft_af_handle_t handle);
void AF_SetDefault(		sft_af_handle_t handle);
/* ------------------------------------------------------------------------ */

#endif

/* ------------------------------------------------------------------------ */
/* end of this file															*/
/* ------------------------------------------------------------------------ */
