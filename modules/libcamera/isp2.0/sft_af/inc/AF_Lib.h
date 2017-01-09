/* --------------------------------------------------------------------------
 * Confidential
 *                    SiliconFile Technologies Inc.
 *
 *                   Copyright (c) 2015 SFT Limited
 *                          All rights reserved
 *
 * Project  : SPRD_AF
 * Filename : AF_Lib.h
 *
 * $Revision: v1.00
 * $Author 	: silencekjj 
 * $Date 	: 2015.02.11
 *
 * --------------------------------------------------------------------------
 * Feature 	: DW9804(VCM IC)
 * Note 	: vim(ts=4, sw=4)
 * ------------------------------------------------------------------------ */
#ifndef	__AF_LIB_H__
#define	__AF_LIB_H__

/* ------------------------------------------------------------------------ */

#include "AF_Common.h"

/* ------------------------------------------------------------------------ */
void SFT_AF_LOG_DBG(		struct sft_af_ctrl_ops *af_ops, BYTE bEnb, const char* format, ...);
void AF_SaveFv( 			AFDATA *afCtl, 	AFDATA_RO *afv, AFDATA_FV *fv, DWORD* inputFv, struct sft_af_ctrl_ops* ops );
void AF_Scan_Core( 			AFDATA *afCtl, 	AFDATA_RO *afv, AFDATA_FV *fv, struct sft_af_ctrl_ops* ops);
WORD AF_Control_Core( 		sft_af_handle_t handle, AFDATA *afCtl, 	AFDATA_RO *afv, BYTE bWrite, struct sft_af_ctrl_ops	*ops);
BYTE AF_Find_Peak( 	 		AFDATA *afCtl, 	AFDATA_RO *afv);
void AF_CalcPreFrameSkipCnt(AFDATA *afCtl, 	AFDATA_RO *afv); 
void AF_InitAF(				AFDATA *afCtl, 	AFDATA_RO *afv, BYTE aeYmean, 	BYTE aeState , BYTE awbState);
void AF_ADT_FPS_AF(			AFDATA *afCtl, 	AFDATA_RO *afv);
void AF_ResetReg(							AFDATA_RO *afv, AFDATA_FV *fv);
void AF_CalcStepFirst(		AFDATA *afCtl, 	AFDATA_RO *afv, BYTE bScene);
void AF_CheckPreFrameSkip(					AFDATA_RO *afv);
void AF_FineScanInit(		AFDATA *afCtl, 	AFDATA_RO *afv, AFDATA_FV *fv);
void AF_Over(				AFDATA *afCtl, 	AFDATA_RO *afv);
void AF_FineScan(			AFDATA *afCtl, 	AFDATA_RO *afv, AFDATA_FV *fv, 	struct sft_af_ctrl_ops* ops );
void AF_CAF(				AFDATA *afCtl, 	AFDATA_RO *afv, AFDATA_FV *fv, 	BYTE bStableAe, struct sft_af_ctrl_ops* ops);
void AF_LoadReg(			AFDATA *afCtl, 	AFDATA_RO *afv, AFDATA_FV *fv);
BYTE AF_Prepare(			AFDATA *afCtl, 	AFDATA_RO *afv, DWORD dwFps, DWORD dwAeLum, BYTE bScene);
void AF_Touch_Record(		AFDATA *afCtl, 	AFDATA_RO *afv, AFDATA_FV *fv);
void AF_Touch_Initial(		AFDATA *afCtl, 	AFDATA_RO *afv, 				struct win_coord winTouch, struct win_coord *winRegion, WORD *ispSize);
void AF_Start_Single(		AFDATA *afCtl, 	AFDATA_RO *afv, AFDATA_FV *fv);
void AF_FirstScan(			AFDATA *afCtl, 	AFDATA_RO *afv, AFDATA_FV *fv, 	struct sft_af_ctrl_ops* ops );
void AF_CAF_Restart(		AFDATA *afCtl, 	AFDATA_RO *afv, AFDATA_FV *fv, 	WORD ms_Delay);
BYTE AF_Capture_Func(		AFDATA *afCtl, 	AFDATA_RO *afv, AFDATA_FV *fv);
BYTE AF_Debug(				AFDATA *afCtl, 	AFDATA_RO *afv, AFDATA_FV *fv);
BYTE AF_Main_ISR(			AFDATA *afCtl, 	AFDATA_RO *afv, AFDATA_FV *fv, 	BYTE bControl);
BYTE AF_Fail_Return(		AFDATA *afCtl, 	AFDATA_RO *afv,	BYTE bInit);
void AF_Mode_Record(		AFDATA *afCtl,	AFDATA_RO *afv,	BYTE bRecord);
BYTE AF_FD(					AFDATA *afCtl, 	AFDATA_RO *afv, WORD *wImgSize, WORD *wFDSize,	struct sft_af_ctrl_ops* ops);
BYTE AF_Set_Capture(						AFDATA_RO *afv,	BYTE bScene);
void AF_Calc_Auto_Region(	AFDATA *afCtl, 	AFDATA_RO *afv,	BYTE bScene, struct win_coord *winRegion, WORD *ispSize, WORD wEnbRegion, struct sft_af_ctrl_ops* ops);
WORD AF_Calc_Fail_Pos(		AFDATA *afCtl, 	AFDATA_RO *afv);
/////////////////////// AF_Control ///////////////////////

void AF_Control_CAF(		AFDATA *afCtl, 	AFDATA_RO *afv, AFDATA_FV *fv,	BYTE bEnb);

#endif

/* ------------------------------------------------------------------------ */
/* end of this file															*/
/* ------------------------------------------------------------------------ */
