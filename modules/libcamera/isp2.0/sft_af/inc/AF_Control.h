/* --------------------------------------------------------------------------
 * Confidential
 *                    SiliconFile Technologies Inc.
 *
 *                   Copyright (c) 2015 SFT Limited
 *                          All rights reserved
 *
 * Project  : SPRD_AF
 * Filename : AF_Control.h
 *
 * $Revision: v1.00
 * $Author 	: silencekjj 
 * $Date 	: 2015.02.11
 *
 * --------------------------------------------------------------------------
 * Feature 	: DW9804(VCM IC)
 * Note 	: vim(ts=4, sw=4)
 * ------------------------------------------------------------------------ */
#ifndef	__AF_CONTROL_H__
#define	__AF_CONTROL_H__

#include "AF_Common.h"
#include "sp_af_ctrl.h"

/* ------------------------------------------------------------------------ */

void AF_Write_I2C(		sft_af_handle_t handle, BYTE id, BYTE addr, BYTE val);
void AF_Write_DAC_I2C(	sft_af_handle_t handle, WORD val);
void AF_Read_DAC_I2C(	sft_af_handle_t handle);

#endif

/* ------------------------------------------------------------------------ */
/* end of this file															*/
/* ------------------------------------------------------------------------ */
