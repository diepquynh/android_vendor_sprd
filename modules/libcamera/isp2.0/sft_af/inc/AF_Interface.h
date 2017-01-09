/* --------------------------------------------------------------------------
 * Confidential
 *                    SiliconFile Technologies Inc.
 *
 *                   Copyright (c) 2015 SFT Limited
 *                          All rights reserved
 *
 * Project  : SPRD_AF
 * Filename : AF_Interface.h
 *
 * $Revision: v1.00
 * $Author 	: silencekjj 
 * $Date 	: 2015.02.11
 *
 * --------------------------------------------------------------------------
 * Feature 	: AP TShark2
 * Note 	: vim(ts=4, sw=4)
 * ------------------------------------------------------------------------ */
#ifndef	__AF_INTERFACE_H__
#define	__AF_INTERFACE_H__

#include "AF_Common.h"
#include "AF_Lib.h"
#include "sp_af_ctrl.h"

/* ------------------------------------------------------------------------ */
void AF_Start_Debug(		sft_af_handle_t handle);
void AF_Stop_Debug(			sft_af_handle_t handle);
void AF_Start(				sft_af_handle_t handle, BYTE bTouch);
void AF_Running(			sft_af_handle_t handle);
void AF_Stop(				sft_af_handle_t handle);
void AF_Prv_Start_Notice(	sft_af_handle_t handle);
void AF_Deinit_Notice(		sft_af_handle_t handle);
void AF_Set_Mode(sft_af_handle_t handle, uint32_t mode);
#endif

/* ------------------------------------------------------------------------ */
/* end of this file							*/
/* ------------------------------------------------------------------------ */
