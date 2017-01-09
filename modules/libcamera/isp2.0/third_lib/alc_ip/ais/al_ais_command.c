
#include <string.h>
#include <stdlib.h>

#include "ae_ctrl_types.h"
#include "ae_log.h"

#include "al_ais_command.h"

#ifndef WIN32

#include <android/log.h>
#define  LOG_TAG    "alPrinter"
#if 1
#define  LOGD(str,args...)		__android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,"[%s():%d]"str,__FUNCTION__,__LINE__,##args)
#else
#define  LOGD(str,args...)
#endif
#if 1
#define  LOGD1(str,args...)		__android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,"[%s():%d]"str,__FUNCTION__,__LINE__,##args)
#else
#define  LOGD1(str,args...)
#endif
#if 1
#define  LOGD2(str,args...)		__android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,"[%s():%d]"str,__FUNCTION__,__LINE__,##args)
#else
#define  LOGD2(str,args...)
#endif

#else

#define  LOGD(str,args)
#define  LOGD1(str,args)
#define  LOGD2(str,args)

#endif

/*===========================================================================*/
UI_16	_ais_cmd_rtn_cvt(
						UI_16				auiRet		)
/*------------------------------------------------------------------------*//**

@return		[UI_16]	Command result
@note		AISの戻り値を、SPRD対応項目に変換を行う
*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/

	switch ( auiRet ) {
		case ALAIS_RET_OK:					/**< OK*/
			auiRet = AE_SUCCESS;
			break;
		case ALAIS_RET_NON_SUPPORT:			/**< 非サポートコマンド*/
		case ALAIS_RET_BUSY:				/**< BUSY*/
		case ALAIS_RET_INVALID_PRM:			/**< パラメータ不正*/
		case ALAIS_RET_INVALID_TUNE:		/**< チューニングデータサイズ不正*/
		case ALAIS_RET_INVALID_LINE:		/**< ラインデータサイズ不正*/
		case ALAIS_RET_UNNKOWN:				/**< 未定義のエラー*/
		case ALAIS_RET_WORK_UNDERFLOW:		/**< ワークメモリ不足*/
		case ALAIS_RET_EXT_IP_OVERLAAP:		/**< 外部IP重複*/
		case ALAIS_RET_EXT_IP_RANGEOVER:	/**< 外部IPセクション範囲エラー*/
		default:
			auiRet = AE_ERROR;
			break;

			/* Error系はすべて"AE_FREE_ERROR"で返す */
//			auiRet = AE_ERROR
//			auiRet = AE_PARAM_ERROR
//			auiRet = AE_PARAM_NULL
//			auiRet = AE_FUN_NULL
//			auiRet = AE_HANDLER_NULL
//			auiRet = AE_HANDLER_ID_ERROR
//			auiRet = AE_ALLOC_ERROR
//			auiRet = AE_FREE_ERROR;
	}

	return auiRet;
	
}/*--------------------- End of function -----------------------*/

/*=============================================================================
	 AIS Core - Command functions
==============================================================================*/

/*===========================================================================*/
#if 0
UI_16	 ais_cmd_set_awb_presetwb(
		 TT_AlAisInterface*  pptAisIf	 ,
		 UI_32				 puiIdx  )
/*------------------------------------------------------------------------*//**

@return		 void
@note

#define DAT_MWB_MODE_AUTO       0x0100
#define DAT_MWB_MODE_DL65       0x0200
#define DAT_MWB_MODE_F          0x0300
#define DAT_MWB_MODE_CWF        0x0400
#define DAT_MWB_MODE_A          0x0500
#define DAT_MWB_MODE_CLOUDY     0x0600
#define DAT_MWB_MODE_INDOOR     0x0700
*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/
	 TT_AlAisCmd				 attCmd;
	 TT_AlAisCmdSetColorMode2*	 aptCmd = (TT_AlAisCmdSetColorMode2*)&attCmd;

	enum {
		CAMERA_WB_MIN_MINUS_1 = -1,
		CAMERA_WB_AUTO = 0,
		CAMERA_WB_CUSTOM,
		CAMERA_WB_INCANDESCENT,
		CAMERA_WB_FLUORESCENT,
		CAMERA_WB_WARM_FLUORESCENT,
		CAMERA_WB_DAYLIGHT,
		CAMERA_WB_CLOUDY_DAYLIGHT,
		CAMERA_WB_TWILIGHT,
		CAMERA_WB_SHADE,
		CAMERA_WB_OFF,
		CAMERA_WB_MAX_PLUS_1
	};
	//Debug
	#define DAT_MWB_MODE_AUTO       0x0100
	#define DAT_MWB_MODE_DL65       0x0200
	#define DAT_MWB_MODE_F          0x0300
	#define DAT_MWB_MODE_CWF        0x0400
	#define DAT_MWB_MODE_A          0x0500
	#define DAT_MWB_MODE_CLOUDY     0x0600
	#define DAT_MWB_MODE_INDOOR     0x0700

	switch(puiIdx)
	{
		case DAT_MWB_MODE_AUTO:
			LOGD("ais_cmd_set_awb_presetwb: %x : DAT_MWB_MODE_AUTO",puiIdx);
			puiIdx = CAMERA_WB_AUTO;
	        break;
		case DAT_MWB_MODE_DL65:
			LOGD("ais_cmd_set_awb_presetwb: %x : DAT_MWB_MODE_DL65",puiIdx);
			puiIdx = CAMERA_WB_DAYLIGHT;
			break;
	    case DAT_MWB_MODE_F:
		    LOGD("ais_cmd_set_awb_presetwb: %x : DAT_MWB_MODE_F",puiIdx);
			puiIdx = CAMERA_WB_WARM_FLUORESCENT;
	        break;
		case DAT_MWB_MODE_CWF:
			LOGD("ais_cmd_set_awb_presetwb: %x : DAT_MWB_MODE_CWF",puiIdx);
			puiIdx = CAMERA_WB_FLUORESCENT;
			break;
		case DAT_MWB_MODE_A:
	        LOGD("ais_cmd_set_awb_presetwb: %x : DAT_MWB_MODE_A",puiIdx);
			puiIdx = CAMERA_WB_INCANDESCENT;
			break;
	    case DAT_MWB_MODE_CLOUDY:
		    LOGD("ais_cmd_set_awb_presetwb: %x : DAT_MWB_MODE_CLOUDY",puiIdx);
			puiIdx = CAMERA_WB_CLOUDY_DAYLIGHT;
	        break;
		case DAT_MWB_MODE_INDOOR:
			LOGD("ais_cmd_set_awb_presetwb: %x : DAT_MWB_MODE_INDOOR",puiIdx);
			puiIdx = CAMERA_WB_AUTO;
		    break;
	    default:
			LOGD("ais_cmd_set_awb_presetwb: %x : INVALID!!",puiIdx);
			puiIdx = CAMERA_WB_AUTO;
		    break;
    }

	 aptCmd->muiCmdId			  = ALAIS_SET_COLOR_MODE2;
	 aptCmd->muiMd				  = ALAIS_COLOR_MODE2_MD_PRESET;
	 aptCmd->muiAct 			  = puiIdx;

	 return AlAisIfSendCommand(pptAisIf  ,&attCmd);
}/*--------------------- End of function -----------------------*/
#endif
/*===========================================================================*/
UI_16	 ais_cmd_set_awb_bestshot(
		 TT_AlAisInterface*  pptAisIf	 ,
		 UI_08				 puiIdx  )
/*------------------------------------------------------------------------*//**

@return		 void
@note
*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/
	 TT_AlAisCmd				 attCmd;
	 TT_AlAisCmdSetColorMode2*	 aptCmd = (TT_AlAisCmdSetColorMode2*)&attCmd;

	LOGD1("puiIdx=%d",puiIdx);
	 aptCmd->muiCmdId			 = ALAIS_SET_COLOR_MODE2;
	 aptCmd->muiMd				 = ALAIS_COLOR_MODE2_MD_AUTO;
	 aptCmd->muiAct 			 = puiIdx;
	 return AlAisIfSendCommand(pptAisIf  ,&attCmd);
}/*--------------------- End of function -----------------------*/

/*===========================================================================*/
#if 0
UI_16	 ais_cmd_set_awb_LockMode(
		 TT_AlAisInterface*  pptAisIf	 ,
		 UI_08				 puiMode	 )		 //0:unlock,1;lock
/*------------------------------------------------------------------------*//**

@return		 void
@note
*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/
	TT_AlAisCmd				 attCmd;
	TT_AlAisCmdSetColorLock* aptCmd = (TT_AlAisCmdSetColorLock*)&attCmd;

	LOGD1("puiMode=%d",puiMode);
	aptCmd->muiCmdId			 = ALAIS_SET_COLOR_LOCK;
	aptCmd->muiAwblc			 = puiMode;
	return AlAisIfSendCommand(pptAisIf  ,&attCmd);
}/*--------------------- End of function -----------------------*/
#endif
/*===========================================================================*/
UI_16	_ais_cmd_set_ae_IsoMode(
						TT_AlAisInterface*  pptAisIf	,
						UI_08				puiMode	 	)
/*------------------------------------------------------------------------*//**

@return		[UI_16]	Command result
@note
*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/

	TT_AlAisCmd					attCmd;
	TT_AlAisCmdSetExposIso* aptCmd = (TT_AlAisCmdSetExposIso*)&attCmd;

	aptCmd->muiCmdId			 = ALAIS_SET_EXPOS_ISO;
	aptCmd->muiMode				 = puiMode;

	AE_LOGI("[ALC_AE_TEST]_ais_cmd_set_ae_IsoMode = %d", puiMode);

	return AlAisIfSendCommand(pptAisIf  ,&attCmd);
}/*--------------------- End of function -----------------------*/

/*===========================================================================*/
UI_16	 ais_cmd_set_ae_maxiso(
		 TT_AlAisInterface*  pptAisIf	 ,
		 UI_08				 puiIdx 	 )
/*------------------------------------------------------------------------*//**

 @return		 void
 @note

 0:ALAIS_EXPOS_MAXISO_MODE_AUTO	 ISO Auto(MaxISo OFF)
 1:ALAIS_EXPOS_MAXISO_MODE_SV06p00	 MaxISO 200
 2:ALAIS_EXPOS_MAXISO_MODE_SV06p33	 MaxISO 252
 3:ALAIS_EXPOS_MAXISO_MODE_SV06p66	 MaxISO 317
 4:ALAIS_EXPOS_MAXISO_MODE_SV07p00	 MaxISO 400
 5:ALAIS_EXPOS_MAXISO_MODE_SV07p33	 MaxISO 504
 6:ALAIS_EXPOS_MAXISO_MODE_SV07p66	 MaxISO 635
 7:ALAIS_EXPOS_MAXISO_MODE_SV08p00	 MaxISO 800
 8:ALAIS_EXPOS_MAXISO_MODE_SV08p33	 MaxISO1008
 9:ALAIS_EXPOS_MAXISO_MODE_SV08p66	 MaxISO1270
10:ALAIS_EXPOS_MAXISO_MODE_SV09p00	 MaxISO1600

*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/
	TT_AlAisCmd				 attCmd;
	TT_AlAisCmdSetExposMaxiso* aptCmd = (TT_AlAisCmdSetExposMaxiso*)&attCmd;

	LOGD1("puiIdx=%d",puiIdx);
	aptCmd->muiCmdId			 = ALAIS_SET_EXPOS_MAXISO;
	aptCmd->muiMode			 = puiIdx;
	return AlAisIfSendCommand(pptAisIf  ,&attCmd);
}/*--------------------- End of function -----------------------*/

/*===========================================================================*/
/* 	EV Bias                                                              */
/*===========================================================================*/
#if 0
#define	AIS_CORE_BIAS_STEP	(25)		//	25step = ((+2EV)-(-2EV))*6step + 1
const SQ_32 fttAlAeCfiOpeEVB[AIS_CORE_BIAS_STEP] =
{						//from aec_ev_comp_one_over_six_tbl
	0xFFFEAAAB		,	/* 2^EV_Comp = 2^-12/9	*/	// 0
	0xFFFEC71D		,	/* 2^EV_Comp = 2^-11/9	*/
	0xFFFEE38F		,	/* 2^EV_Comp = 2^-10/9	*/
	0xFFFF0000		,	/* 2^EV_Comp = 2^ -9/9	*/
	0xFFFF1C72		,	/* 2^EV_Comp = 2^ -8/9	*/
	0xFFFF38E4		,	/* 2^EV_Comp = 2^ -7/9	*/
	0xFFFF5556		,	/* 2^EV_Comp = 2^ -6/9	*/
	0xFFFF71C8		,	/* 2^EV_Comp = 2^ -5/9	*/
	0xFFFF8E39		,	/* 2^EV_Comp = 2^ -4/9	*/
	0xFFFFAAAB		,	/* 2^EV_Comp = 2^ -3/9	*/
	0xFFFFC71D		,	/* 2^EV_Comp = 2^ -2/9	*/
	0xFFFFE38F		,	/* 2^EV_Comp = 2^ -1/9	*/
	0x00000000		,	/* 2^EV_Comp = 2^  0 	*/	// 12
	0x00001C71		,	/* 2^EV_Comp = 2^  1/9	*/
	0x000038E3		,	/* 2^EV_Comp = 2^  2/9	*/
	0x00005555		,	/* 2^EV_Comp = 2^  3/9	*/
	0x000071C7		,	/* 2^EV_Comp = 2^  4/9	*/
	0x00008E38		,	/* 2^EV_Comp = 2^  5/9	*/
	0x0000AAAA		,	/* 2^EV_Comp = 2^  6/9	*/
	0x0000C71C		,	/* 2^EV_Comp = 2^  7/9	*/
	0x0000E38E		,	/* 2^EV_Comp = 2^  8/9	*/
	0x00010000		,	/* 2^EV_Comp = 2^  9/9	*/
	0x00011C71		,	/* 2^EV_Comp = 2^ 10/9	*/
	0x000138E3		,	/* 2^EV_Comp = 2^ 11/9	*/
	0x00015555		,	/* 2^EV_Comp = 2^ 12/9	*/	// 24
};
#else
#define	AIS_CORE_BIAS_STEP	(15)	//	15step = ((+3EV)-(-3EV))
const SQ_32 fttAlAeCfiOpeEVB[AIS_CORE_BIAS_STEP] =
{						//from aec_ev_comp_one_over_six_tbl
		#if 0
	0xFFFEAAAB		,	/* 2^EV_Comp = 2^-18.00000000 / 9	*/	// 0
	0xFFFEDB6E		,	/* 2^EV_Comp = 2^-15.42857143 / 9	*/
	0xFFFF0C31		,	/* 2^EV_Comp = 2^-12.85714286 / 9	*/
	0xFFFF3CF4		,	/* 2^EV_Comp = 2^-10.28571429 / 9	*/
	0xFFFF6DB7		,	/* 2^EV_Comp = 2^- 7.71428571 / 9	*/
	0xFFFF9E7A		,	/* 2^EV_Comp = 2^- 5.14285714 / 9	*/
	0xFFFFCF3D		,	/* 2^EV_Comp = 2^- 2.57142857 / 9	*/
	0x00000000		,	/* 2^EV_Comp = 2^  0.00000000 / 9	*/	// 7
	0x000030C3		,	/* 2^EV_Comp = 2^  2.57142857 / 9	*/
	0x00006186		,	/* 2^EV_Comp = 2^  5.14285714 / 9	*/
	0x00009249		,	/* 2^EV_Comp = 2^  7.71428571 / 9	*/
	0x0000C30C		,	/* 2^EV_Comp = 2^ 10.28571429 / 9	*/
	0x0000F3CF		,	/* 2^EV_Comp = 2^ 12.85714286 / 9	*/
	0x00012492		,	/* 2^EV_Comp = 2^ 15.42857143 / 9	*/
	0x00015555		,	/* 2^EV_Comp = 2^ 18.00000000 / 9	*/	// 14
		#else
	0xFFFE0000		,	/* 2^EV_Comp = 2^-18 / 9	*/	// 0	"-3EV"
	0xFFFE0000		,	/* 2^EV_Comp = 2^-18 / 9	*/	//		"-3EV"
	0xFFFE5556		,	/* 2^EV_Comp = 2^-15 / 9	*/	//		
	0xFFFEAAAB		,	/* 2^EV_Comp = 2^-12 / 9	*/	//		"-2EV"
	0xFFFF0000		,	/* 2^EV_Comp = 2^- 9 / 9	*/	//		
	0xFFFF5556		,	/* 2^EV_Comp = 2^- 6 / 9	*/	//		"-1EV"
	0xFFFFAAAB		,	/* 2^EV_Comp = 2^- 3 / 9	*/	//		
	0x00000000		,	/* 2^EV_Comp = 2^  0 / 9	*/	// 7	"0EV"
	0x00005555		,	/* 2^EV_Comp = 2^  3 / 9	*/	//		
	0x0000AAAA		,	/* 2^EV_Comp = 2^  6 / 9	*/	//		"+1EV"
	0x00010000		,	/* 2^EV_Comp = 2^  9 / 9	*/	//		
	0x00015555		,	/* 2^EV_Comp = 2^ 12 / 9	*/	//		"+2EV"
	0x0001AAAA		,	/* 2^EV_Comp = 2^ 15 / 9	*/	//		
	0x00020000		,	/* 2^EV_Comp = 2^ 18 / 9	*/	//		"+3EV"
	0x00020000		,	/* 2^EV_Comp = 2^ 18 / 9	*/	// 14	"+3EV"
		#endif
};
#endif
/*===========================================================================*/
static UI_16	 ais_cmd_set_ae_ev_bias(
		 TT_AlAisInterface*  pptAisIf	 ,
		 SQ_32				 psqBias	 )
/*------------------------------------------------------------------------*//**

@return		 void
@note


*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/
	 TT_AlAisCmd				 attCmd;
	 TT_AlAisCmdSetExposEvb2*	 aptCmd = (TT_AlAisCmdSetExposEvb2*)&attCmd;

	LOGD1("psqBias=%d",psqBias);
	 aptCmd->muiCmdId			 = ALAIS_SET_EXPOS_EVB2;
	 aptCmd->muiBias			 = (UI_32)psqBias;
	 return AlAisIfSendCommand(pptAisIf  ,&attCmd);
}/*--------------------- End of function -----------------------*/

/*===========================================================================*/
UI_16	 _ais_cmd_set_ae_Brightness(
		 TT_AlAisInterface*  pptAisIf	 ,
		 SI_32				 psiIdx 	 )
/*------------------------------------------------------------------------*//**

@return		 void
@note


*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/
#if 0
	 UI_32	 auiIdx	= (UI_32)(psiIdx + 12);		//	12 = 2EV + 6step
	 auiIdx	= (auiIdx >= AIS_CORE_BIAS_STEP)?(AIS_CORE_BIAS_STEP-1):(auiIdx);

	LOGD1("psiIdx=%d",psiIdx);
	 ais_set_ae_BracketList_Off(pptAisIf);
	 return ais_cmd_set_ae_ev_bias(pptAisIf, fttAlAeCfiOpeEVB[auiIdx]);
#else
	UI_32	 auiIdx	= (UI_32)(psiIdx + 0);		//	7 = 3EV
	auiIdx	= (auiIdx >= AIS_CORE_BIAS_STEP)?(AIS_CORE_BIAS_STEP-1):(auiIdx);

	LOGD1("psiIdx=%d",psiIdx);
	return ais_cmd_set_ae_ev_bias(pptAisIf, fttAlAeCfiOpeEVB[auiIdx]);
#endif
}/*--------------------- End of function -----------------------*/

/*===========================================================================*/
UI_16	 ais_set_ae_BracketList_Bias(
		 TT_AlAisInterface*  pptAisIf			 ,
		 SQ_32				 psqBias	 )
/*------------------------------------------------------------------------*//**

@return		 void
@note


*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/
	TT_AlAisCmd				 attCmd;
	TT_AlAisCmdSetExposAeb2*	 aptAeb = (TT_AlAisCmdSetExposAeb2*)&attCmd;

	LOGD1("psqBias=%d",psqBias);
	aptAeb->muiCmdId			 = ALAIS_SET_EXPOS_AEB;
	aptAeb->muiBias			 = (UI_32)psqBias;
	return AlAisIfSendCommand(pptAisIf  ,&attCmd);
}/*--------------------- End of function -----------------------*/

/*===========================================================================*/
UI_16	ais_set_ae_BracketList_Idx(
		TT_AlAisInterface *		pptAisIf			,
		SI_32					psiIdx				)
/*------------------------------------------------------------------------*//**

@return		 void
@note


*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/
	UI_32	 auiIdx	= (UI_32)(psiIdx);		//	12 = 2EV + 6step
	auiIdx			= (auiIdx >= AIS_CORE_BIAS_STEP)?(AIS_CORE_BIAS_STEP-1):(auiIdx);

	LOGD1("psiIdx=%d",psiIdx);
	return ais_set_ae_BracketList_Bias(pptAisIf ,fttAlAeCfiOpeEVB[auiIdx]);
}/*--------------------- End of function -----------------------*/

/*===========================================================================*/
UI_16	ais_set_ae_BracketList(
		TT_AlAisInterface *		pptAisIf		,
		int 	*				ppiList			,
		UI_16	 				puiCnt			)
/*------------------------------------------------------------------------*//**
@return		 void
@note
*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/
	UI_16		auiLc;

	LOGD1("puiCnt=%d",puiCnt);
	puiCnt				 	= (ppiList == (int *)0)?(0):(puiCnt);
	pptAisIf->muiAebCnt 	= (puiCnt > 5)?(5):(puiCnt);
//	CDBG("%s, puiCnt:%d ",__func__, puiCnt);
//	CDBG("%s, pptAisIf->muiAebCnt:%d ",__func__, pptAisIf->muiAebCnt);
//	CDBG("%s, ppiList[1]:%d ",__func__, ppiList[1]);
	if (pptAisIf->muiAebCnt == 0)
	{
		return 0;
	}
	for (auiLc = 0;	auiLc < pptAisIf->muiAebCnt; auiLc++)
	{
		pptAisIf->msqAebList[auiLc]	= fttAlAeCfiOpeEVB[ppiList[auiLc]];
	}
	return 0;
}/*--------------------- End of function -----------------------*/

#define AEC_METERING_MAX_MODES 7 //2014/02/27 Seino@ALC This is Temp
/*===========================================================================*/
UI_16	 _ais_cmd_set_ae_weight(
		 TT_AlAisInterface*  pptAisIf	 ,
		 UI_08				 puiIdx 	 )
/*------------------------------------------------------------------------*//**

 @return		 void
 @note

 typedef enum {
   AEC_METERING_FRAME_AVERAGE,			q_unused
   AEC_METERING_CENTER_WEIGHTED,
   AEC_METERING_SPOT_METERING,
   AEC_METERING_SMART_METERING,			q_unused
   AEC_METERING_USER_METERING,			q_unused
   AEC_METERING_SPOT_METERING_ADV,
   AEC_METERING_CENTER_WEIGHTED_ADV,
   AEC_METERING_MAX_MODES				q_unused
 } aec_auto_exposure_mode_t;

 //====== Mode
    //#define	ALAIS_EXPOS_MET2_MODE_EVA				((UI_08)0)
    //#define	ALAIS_EXPOS_MET2_MODE_WEI				((UI_08)1)
    //#define	ALAIS_EXPOS_MET2_MODE_AVE				((UI_08)2)
    //#define	ALAIS_EXPOS_MET2_MODE_FACE				((UI_08)3)
    //#define	ALAIS_EXPOS_MET2_MODE_MAX				((UI_08)4)
//====== Weight
    //#define	ALAIS_EXPOS_MET2_WEIGHT_EVA				((UI_08)0)
    //#define	ALAIS_EXPOS_MET2_WEIGHT_CENT			((UI_08)1)
    //#define	ALAIS_EXPOS_MET2_WEIGHT_SPOT			((UI_08)2)
    //#define	ALAIS_EXPOS_MET2_WEIGHT_EVA_CENT		((UI_08)3)
    //#define	ALAIS_EXPOS_MET2_WEIGHT_EVA_H			((UI_08)4)
    //#define	ALAIS_EXPOS_MET2_WEIGHT_EVA_V			((UI_08)5)
    //#define	ALAIS_EXPOS_MET2_WEIGHT_MAX			((UI_08)6)

---------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/

	 const int fttAlAeCfiWeight[8][2] =
	 {
		 {ALAIS_EXPOS_MET2_MODE_AVE , ALAIS_EXPOS_MET2_WEIGHT_CENT},		//AEC_METERING_FRAME_AVERAGE = 0,
		 {ALAIS_EXPOS_MET2_MODE_FACE, ALAIS_EXPOS_MET2_WEIGHT_CENT},		//AEC_METERING_CENTER_WEIGHTED = 1,
		 {ALAIS_EXPOS_MET2_MODE_FACE, ALAIS_EXPOS_MET2_WEIGHT_SPOT},		//AEC_METERING_SPOT_METERING = 2,
		 {ALAIS_EXPOS_MET2_MODE_FACE, ALAIS_EXPOS_MET2_WEIGHT_CENT},		//AEC_METERING_SMART_METERING,
		 {ALAIS_EXPOS_MET2_MODE_FACE, ALAIS_EXPOS_MET2_WEIGHT_CENT},		//AEC_METERING_USER_METERING,
		 {ALAIS_EXPOS_MET2_MODE_FACE, ALAIS_EXPOS_MET2_WEIGHT_SPOT},		//AEC_METERING_SPOT_METERING_ADV = 5,
		 {ALAIS_EXPOS_MET2_MODE_FACE, ALAIS_EXPOS_MET2_WEIGHT_CENT},		//AEC_METERING_CENTER_WEIGHTED_ADV=6,
	 };
	 TT_AlAisCmd		 attCmd;
	 TT_AlAisCmdSetExposMet2* aptCmd = (TT_AlAisCmdSetExposMet2*)&attCmd;

	LOGD1("puiIdx=%d",puiIdx);
	puiIdx	= (puiIdx >= AEC_METERING_MAX_MODES)?(AEC_METERING_MAX_MODES-1):(puiIdx);

	 aptCmd->muiCmdId			 = ALAIS_SET_EXPOS_MET2;
	 aptCmd->muiMode			 = fttAlAeCfiWeight[puiIdx][0];
	 aptCmd->muiWeight			 = fttAlAeCfiWeight[puiIdx][1];
	 return AlAisIfSendCommand(pptAisIf  ,&attCmd);
}/*--------------------- End of function -----------------------*/

/*===========================================================================*/
UI_16	 ais_cmd_set_ae_Framing(
		 TT_AlAisInterface*  pptAisIf	 ,
		 UI_08				 puiType	 )		 //	Framing Type
/*------------------------------------------------------------------------*//**
		LED Sequence Start Trigger
@return		 void
@note

		0:	A:Side Shooting (Fwd)
		1:	B:Vertical Shooting (Rev)
		2:	C:Side Shooting (Rev)
		3:	D:Vertical Shooting (Fwd)

				A
			 D [cam] B
				C
*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/

#if 0
//This is not supported yet. //2014/02/27
	 pptAisIf->muiMetUpp	= puiType;

#endif
	LOGD1("puiType=%d",puiType);
	 return 0;
}/*--------------------- End of function -----------------------*/

/*===========================================================================*/
UI_16	 ais_cmd_set_ae_evp(
		 TT_AlAisInterface*  pptAisIf	 ,
		 UI_08				 puiIdx 	 )
/*------------------------------------------------------------------------*//**

@return		 void
@note


*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/
	 TT_AlAisCmd				 attCmd;
	 TT_AlAisCmdSetExposEvp* aptCmd = (TT_AlAisCmdSetExposEvp*)&attCmd;

	LOGD1("puiIdx=%d",puiIdx);
	 aptCmd->muiCmdId			 = ALAIS_SET_EXPOS_EVP;
	 aptCmd->muiMode			 = puiIdx;
	 return AlAisIfSendCommand(pptAisIf  ,&attCmd);
}/*--------------------- End of function -----------------------*/

/*===========================================================================*/
UI_16	 _ais_cmd_set_ae_bestshot(
		 TT_AlAisInterface*  pptAisIf	 ,
		 UI_08				 puiIdx  )
/*------------------------------------------------------------------------*//**
@return		 void
@note
		00.00 AUTO_prv		00.01 AUTO_cap		00.02 AUTO_mov
		01.03 LANDSCAPE_prv	01.04 LANDSCAPE_cap	01.05 LANDSCAPE_mov
		02.06 NIGHT_prv		02.07 NIGHT_cap		02.08 NIGHT_mov
		03.09 PORTRAIT_prv	03.10 PORTRAIT_cap	03.11 PORTRAIT_mov
		04.12 SPORTS_prv	04.13 SPORTS_cap	04.14 SPORTS_mov
		05.15 ANTISHAKE_prv	05.16 ANTISHAKE_cap	05.17 ANTISHAKE_mov
		06.18 FIREWORKS_prv	06.19 FIREWORKS_cap	06.20 FIREWORKS_mov
*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/
	 UI_16	 auiRet;

//----------------------------------------------------------------------
// Temporary here
	 typedef struct
	 {
		 UI_08	 muiEvp;
		 UI_08	 muiMet;
		 UI_32	 muiEvb;
	 }TT_AeBestShotTbl;
	 const TT_AeBestShotTbl  attBestShotTbl[] =
	 {	 //EVP			,Weigth	,Bias
		{	((0)+0)		,0		,7,},		//	AE_SCENE_NORMAL = 0x00,
		{	((2)+0)		,1		,3,},		//	AE_SCENE_NIGHT,
		{	((0)+0)		,1		,3,},		//	AE_SCENE_SPORT,
		{	((3)+0)		,1		,3,},		//	AE_SCENE_PORTRAIT,
		{	((1)+0)		,1		,3,},		//	AE_SCENE_LANDSPACE,
/*
		 {	((0)+0)	,1		,6,},//CAMERA_BESTSHOT_OFF = 0,
		 {	((0)+0)	,1		,6,},//CAMERA_BESTSHOT_AUTO = 1,
		 {	((1)+0)	,1		,6,},//CAMERA_BESTSHOT_LANDSCAPE = 2,
		 {	((1)+0)	,1		,6,},//CAMERA_BESTSHOT_SNOW,
		 {	((1)+0)	,1		,6,},//CAMERA_BESTSHOT_BEACH,
		 {	((1)+0)	,1		,6,},//CAMERA_BESTSHOT_SUNSET = 5,
		 {	((2)+0)	,1		,6,},//CAMERA_BESTSHOT_NIGHT=6,
		 {	((3)+0)	,1		,6,},//CAMERA_BESTSHOT_PORTRAIT=7,
		 {	((0)+0)	,1		,6,},//CAMERA_BESTSHOT_BACKLIGHT,
		 {	((4)+0)	,1		,6,},//CAMERA_BESTSHOT_SPORTS=9,
		 {	((5)+0)	,1		,6,},//CAMERA_BESTSHOT_ANTISHAKE,
		 {	((0)+0)	,1		,6,},//CAMERA_BESTSHOT_FLOWERS,
		 {	((2)+0)	,1		,6,},//CAMERA_BESTSHOT_CANDLELIGHT,
		 {	((6)+0)	,1		,6,},//CAMERA_BESTSHOT_FIREWORKS,
		 {	((0)+0)	,1		,6,},//CAMERA_BESTSHOT_PARTY,
		 {	((2)+0)	,1		,6,},//CAMERA_BESTSHOT_NIGHT_PORTRAIT,
		 {	((0)+0)	,1		,6,},//CAMERA_BESTSHOT_THEATRE,
		 {	((0)+0)	,1		,6,},//CAMERA_BESTSHOT_ACTION=17,
		 {	((0)+0)	,1		,6,},//CAMERA_BESTSHOT_AR,
		 {	((7)+0)	,1		,6,},//FHD 60fps,	=19
		 {	((8)+0)	,1		,6,},//UHD,		=20
*/
	 };
 //----------------------------------------------------------------------
	LOGD1("_ais_cmd_set_ae_bestshot : puiIdx=%d",puiIdx);
	 auiRet  = ais_cmd_set_ae_ev_bias(pptAisIf,attBestShotTbl[puiIdx].muiEvb);
	 auiRet |= _ais_cmd_set_ae_weight(pptAisIf,attBestShotTbl[puiIdx].muiMet);
	 auiRet |= ais_cmd_set_ae_evp(pptAisIf,attBestShotTbl[puiIdx].muiEvp);

	 return auiRet;
}/*--------------------- End of function -----------------------*/

/*===========================================================================*/
UI_16	 ais_cmd_set_ae_setExpMode(
		 TT_AlAisInterface*  pptAisIf	 ,
		 UI_16				 puiMode  )		 //  Exposure Mode Index
/*------------------------------------------------------------------------*//**

@return		 void
@note
*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/
	static	UI_16	fuiAisCmdSetExpMode[2] =
	{
		ALAIS_EXPOS_EXP_MODE_AUTO	,					//	EV Program Auto Mode
		ALAIS_EXPOS_EXP_MODE_TVP						//	TV priority
	};
	TT_AlAisCmd				 attCmd;
	TT_AlAisCmdSetExposExp*	 aptCmd = (TT_AlAisCmdSetExposExp*)&attCmd;

	LOGD1("puiMode=%d",puiMode);
	puiMode	= (puiMode >= 2)?(1):(puiMode);
	aptCmd->muiCmdId			 = ALAIS_SET_EXPOS_EXP;
	aptCmd->muiMode 			 = (UI_08)fuiAisCmdSetExpMode[puiMode];			//	exposure mode
	return AlAisIfSendCommand(pptAisIf  ,&attCmd);
}/*--------------------- End of function -----------------------*/

/*===========================================================================*/
UI_16	 _ais_cmd_set_ae_setFpsCtrl(
						TT_AlAisInterface*  pptAisIf	,
						UI_16				puiMinFps  	,		 //  min fps [u08.08] format
						UI_16				puiMaxFps  	)		 //  max fps [u08.08] format
/*------------------------------------------------------------------------*//**

@return		 void
@note
*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/

	TT_AlAisCmd				 attCmd;
	TT_AlAisCmdSetExposFps_ctrl*	 aptCmd = (TT_AlAisCmdSetExposFps_ctrl*)&attCmd;
	
	aptCmd->muiCmdId			 = ALAIS_SET_EXPOS_FPS_CTRL;
	aptCmd->muiMinfps 			 = (UI_32)(puiMinFps<<8);	 //convert to s15.16(alfix) from s31.00(mtek)
	aptCmd->muiMaxfps 			 = (UI_32)(puiMaxFps<<8);	 //convert to s15.16(alfix) from s31.00(mtek)
	
	AE_LOGI("[ALC_AE_TEST]_ais_cmd_set_ae_FpsCtrl min: %d, max: %d", puiMinFps, puiMaxFps);
	
	return AlAisIfSendCommand(pptAisIf  ,&attCmd);
	
}/*--------------------- End of function -----------------------*/


/*===========================================================================*/
UI_16	 ais_cmd_set_ae_setBacklightCorrection(
		 TT_AlAisInterface*  pptAisIf	 ,
		 UI_08				 puiOnOff	 )		 //0:off,1:On
/*------------------------------------------------------------------------*//**

@return		 void
@note
*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/
	TT_AlAisCmd				 attCmd;
	TT_AlAisCmdSetExposBlc* aptCmd = (TT_AlAisCmdSetExposBlc*)&attCmd;

	LOGD1("puiOnOff=%d",puiOnOff);
	aptCmd->muiCmdId			 = ALAIS_SET_EXPOS_BLC;
	aptCmd->muiMode			 = puiOnOff;
	return AlAisIfSendCommand(pptAisIf  ,&attCmd);
}/*--------------------- End of function -----------------------*/

/*===========================================================================*/
UI_16	_ais_cmd_set_ae_LockMode(
						TT_AlAisInterface*  pptAisIf	,
						UI_08				puiMode		)
/*------------------------------------------------------------------------*//**

@return		[UI_16]	Command result
@note
*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/

	TT_AlAisCmd					attCmd;
	TT_AlAisCmdSetColorLock*	aptCmd = (TT_AlAisCmdSetColorLock*)&attCmd;

	aptCmd->muiCmdId			 = ALAIS_SET_EXPOS_LOCK;
	aptCmd->muiAwblc			 = puiMode;

	AE_LOGI("[ALC_AE_TEST]ais_cmd_set_ae_LockMode Lock = %d", puiMode);

	return AlAisIfSendCommand(pptAisIf  ,&attCmd);
}/*--------------------- End of function -----------------------*/

/*=======3====================================================================*/
UI_16	 ais_cmd_set_ae_ExpSpeed(
		 TT_AlAisInterface*  pptAisIf	 ,
		 UI_08				 puiSpeed	 )		 //	0-100:(Slow-Fast)
/*------------------------------------------------------------------------*//**

@return		 void
@note
*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/
	TT_AlAisCmd				 attCmd;
	TT_AlAisCmdSetExposSpd* aptCmd = (TT_AlAisCmdSetExposSpd*)&attCmd;

	LOGD1("puiSpeed=%d",puiSpeed);
	aptCmd->muiCmdId			 = ALAIS_SET_EXPOS_SPD;
	aptCmd->muiMode			 = puiSpeed;
	return AlAisIfSendCommand(pptAisIf  ,&attCmd);
}/*--------------------- End of function -----------------------*/

/*===========================================================================*/
UI_16	 ais_cmd_get_ae_ExpSpeed(
		 TT_AlAisInterface*  pptAisIf	 ,
		 UI_08*				 puiSpeed	 )		 //	0-100:(Slow-Fast)
/*------------------------------------------------------------------------*//**

@return		 void
@note
*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/
	TT_AlAisCmd				 attCmd;
	TT_AlAisCmdSetExposSpd* aptCmd = (TT_AlAisCmdGetExposSpd*)&attCmd;
	UI_16	 auiRet;

	LOGD1("puiSpeed=%d",puiSpeed);
	aptCmd->muiCmdId			 = ALAIS_GET_EXPOS_SPD;
	aptCmd->muiMode			 = 0;
	auiRet = AlAisIfSendCommand(pptAisIf  ,&attCmd);
	*puiSpeed	=	aptCmd->muiMode;
	return auiRet;
}/*--------------------- End of function -----------------------*/

/*===========================================================================*/
UI_16	 _ais_cmd_set_ae_Target(
		 TT_AlAisInterface*  pptAisIf	 ,
		 UI_08				 puiTarget	 )		 //	3-36[U08.00]
/*------------------------------------------------------------------------*//**

@return		 void
@note
		Target:17%
		ais_cmd_set_ae_Target	(&ais_obj->fttInterface,17);

*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/

	TT_AlAisCmd				 attCmd;
	TT_AlAisCmdSetExposTarget* aptCmd = (TT_AlAisCmdSetExposTarget*)&attCmd;


	aptCmd->muiCmdId			 = ALAIS_SET_EXPOS_TARGET;

	if( 3 <= puiTarget )
	{
		aptCmd->muiRate				 = 3;
	}
	else if( puiTarget >= 36)
	{
		aptCmd->muiRate				 = 36;
	}
	else
	{
		aptCmd->muiRate				 = puiTarget;
	}

	AE_LOGI("[ALC_AE_TEST]ais_cmd_set_ae_Target=%d",aptCmd->muiRate);

	return AlAisIfSendCommand(pptAisIf  ,&attCmd);

}/*--------------------- End of function -----------------------*/

#define	AIS_CORE_FLK_MODE_MAX	4
/*===========================================================================*/
UI_16	_ais_cmd_set_ae_FlickerMode(
						TT_AlAisInterface*  pptAisIf	,
						UI_08				puiMode		)
/*------------------------------------------------------------------------*//**

@return		[UI_16]	Command result
@note
*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/

	TT_AlAisCmd					attCmd;
	TT_AlAisCmdSetExposFlk* aptCmd = (TT_AlAisCmdSetExposFlk*)&attCmd;

	aptCmd->muiCmdId			 = ALAIS_SET_EXPOS_FLK;
	aptCmd->muiMode				 = puiMode;

	AE_LOGI("[ALC_AE_TEST]_ais_cmd_set_ae_FlickerMode = %d", puiMode);

	return AlAisIfSendCommand(pptAisIf  ,&attCmd);
}/*--------------------- End of function -----------------------*/

/*===========================================================================*/
UI_16	 ais_cmd_set_ae_FcsSeqTrigg(
		 TT_AlAisInterface*  pptAisIf	 ,
		 UI_08				 puiOnOff	 )		 //0:Off,1:On
/*------------------------------------------------------------------------*//**
		Focusing Sequence Start Trigger
@return		 void
@note
*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/
	LOGD1("puiOnOff=%d",puiOnOff);
	pptAisIf->muiFcsReq	= puiOnOff;
	LOGD("aei-led] seq ais_cmd_set_ae_FcsSeqTrigg = %d",puiOnOff);
	return 0;
}/*--------------------- End of function -----------------------*/

/*===========================================================================*/
UI_16	 ais_set_LedSeqTrigg(
		 TT_AlAisInterface*  pptAisIf	 ,
		 UI_08				 puiOnOff	 )		 //0:Off,1:On
/*------------------------------------------------------------------------*//**
		LED Sequence Start Trigger
@return		 void
@note
*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/
	LOGD1("puiOnOff=%d",puiOnOff);
	LOGD("ais_set_LedSeqTrigg called");
	LOGD("aei-led] seq ais_set_LedSeqTrigg = %d",puiOnOff);
	pptAisIf->muiLedReq	= puiOnOff;
	return 0;
}/*--------------------- End of function -----------------------*/

/*===========================================================================*/
UI_16	 ais_set_PebSeqTrigg(
		 TT_AlAisInterface*  pptAisIf	 ,
		 UI_08				 puiOnOff	 )		 //0:Off,1:On
/*------------------------------------------------------------------------*//**
		PEB(ZSL Preview Exp Bracket) Sequence Start Trigger
@return		 void
@note
*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/
	LOGD1("puiOnOff=%d",puiOnOff);
	pptAisIf->muiPebReq		= puiOnOff;
	return 0;
}/*--------------------- End of function -----------------------*/


/*===========================================================================*/
UI_16	 ais_set_zsl_mode(
		 TT_AlAisInterface*  pptAisIf	 ,
		 UI_08				 puiOnOff	 )		 //0:ff,1:On
/*------------------------------------------------------------------------*//**

@return		 void
@note


*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/
	//TT_AlAisCmd				 attCmd;
	//TT_AlAisCmdSetExposZsl* aptCmd = (TT_AlAisCmdSetExposZsl*)&attCmd;

	//puiOnOff	= (pptAisIf->muiCamMode == eAlAisCamMode_Dvc)?(0):(puiOnOff);

	LOGD1("puiOnOff=%d",puiOnOff);
	pptAisIf->muiOpType			 = puiOnOff;

	LOGD("[HDR] Op Type: %d", pptAisIf->muiOpType);
	//aptCmd->muiCmdId			 = ALAIS_SET_EXPOS_ZSL;
	//aptCmd->muiMode			 = puiOnOff;
	return 0;//AlAisIfSendCommand(pptAisIf  ,&attCmd);
}/*--------------------- End of function -----------------------*/

static SQ_32 ais_convert_GrBase2GBase(SQ_32 psqRB_by_Gr, SQ_32 psqGb_by_Gr)
{
           SQ_32 asqK;

           asqK = AlFixDiv(AL_FIX_VAL(2.0), AL_FIX_VAL(1.0)+psqGb_by_Gr);

           return  AlFixMul(psqRB_by_Gr, asqK);
}

/*===========================================================================**

for LED Sequence

**===========================================================================*/

/*===========================================================================*/
float	 ais_get_bv(
		TT_AlAisInterface*  pptAisIf	 )
/*------------------------------------------------------------------------*//**
Get BV for judge someting
@return		 SQ_32
@note
*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/
	LOGD("[LED]ais_get_bv called : BV=%f",pptAisIf->mflBv);
	return pptAisIf->mflBv;
}/*--------------------- End of function -----------------------*/


/*===========================================================================*/
void	ais_set_preflash_index(
		TT_AlAisInterface*  pptAisIf	 ,
		UI_16				 puiIndexL	 ,	/** [I ] index of Lo Ctemp LED*/
		UI_16				 puiIndexH	 )	/** [I ] index of Hi Ctemp LED*/
/*------------------------------------------------------------------------*//**
Preflash index setting to AIS

@return		 void
@note
	Optional (if AF light/Preflash index Variable)

*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/
	LOGD("[LED]ais_set_preflash_index called : L=%d H=%d",puiIndexL,puiIndexH);
}/*--------------------- End of function -----------------------*/

/*===========================================================================*/
void	ais_set_torch_trigger(
		TT_AlAisInterface*  pptAisIf	)
/*------------------------------------------------------------------------*//**
AF sequence trigger to AIS

@return		 void
@note
	call LED Torch ON and AF Start timming

*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/
	LOGD("[LED]ais_set_torch_trigger called");
}/*--------------------- End of function -----------------------*/

/*===========================================================================*/
UI_08	ais_get_ae_convergence_status(
		TT_AlAisInterface*  pptAisIf	)
/*------------------------------------------------------------------------*//**
Get AE cpmvergence stauts

@return		 status
	0: stable
	1: unstable
@note

*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/
	LOGD("[LED]ais_get_ae_convergence_status called : %d",pptAisIf->muiAeSettle);
	return (UI_08)pptAisIf->muiAeSettle;
}/*--------------------- End of function -----------------------*/

/*===========================================================================*/
void	ais_set_preflash_trigger(
		TT_AlAisInterface*  pptAisIf	)
/*------------------------------------------------------------------------*//**
LED preflash sequence trigger to AIS

@return		 void
@note
	call LED Torch ON and AF Start timming

*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/
	LOGD("[LED]ais_set_preflash_trigger called");
}/*--------------------- End of function -----------------------*/

/*===========================================================================*/
UI_08	ais_get_preflash_status(
		TT_AlAisInterface*  pptAisIf	)
/*------------------------------------------------------------------------*//**
LED preflash sequence status

@return		 status
 0: idle /complete
 1: inprogress

@note
	Optional
*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/
	LOGD("[LED]ais_get_preflash_status called");
	return 0;
}/*--------------------- End of function -----------------------*/

/*===========================================================================*/
void	ais_get_flash_control_info(
		TT_AlAisInterface*  pptAisIf	,
		UI_16*				ppiIndexL	, /** [ O] index of Lo Ctemp LED*/
		UI_16*				ppiIndexH	, /** [ O] index of Hi Ctemp LED*/
		float*				ppfTime	 	) /** [ O] flash time [sec] */
/*------------------------------------------------------------------------*//**
Get LED flash index

@return		 void
@note

*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/
	*ppiIndexL	= 4;
	*ppiIndexH	= 4;
	*ppfTime	= 0.033 * 2 ; // 2frame

	LOGD("[LED]ais_get_flash_control_info called: L=%d H=%d ,time=%f",*ppiIndexL,*ppiIndexH,*ppfTime);
}/*--------------------- End of function -----------------------*/


/*===========================================================================*/
UI_32	ais_get_flash_level(
		TT_AlAisInterface*  pptAisIf	)
/*------------------------------------------------------------------------*//**
Get LED flash index

@return		 
@note

*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/
	LOGD1("[LED] called:");
	return (UI_32)pptAisIf->muiLedBright;
}/*--------------------- End of function -----------------------*/




/*===========================================================================*/
void	_ais_set_touch_ae(
		TT_AlAisInterface*  pptAisIf	,
		SI_32				psiLeft		,		//	i4Left	(0 ~ + resolution_info.frame_size.w 必須)
		SI_32				psiRight	,		//	i4Right
		SI_32				psiTop		,		//	i4Top	(0 ~ + resolution_info.frame_size.h 必須)
		SI_32				psiBottom	,		//	i4Bottom
		SI_32				psiFrameW	,		//		resolution_info.frame_size.w
		SI_32				psiFrameH	)		//		resolution_info.frame_size.h
/*------------------------------------------------------------------------*//**
Set Touch AE

@return		 
@note

*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/
	#define ALC_STAT_H (16)
	#define ALC_STAT_V (16)

	SI_32		asiLeft		=	((float)(psiLeft  +0))/((float)psiFrameW/ALC_STAT_H)	+	0.5f;
	SI_32		asiRight	=	((float)(psiRight +0))/((float)psiFrameW/ALC_STAT_H)	+	0.5f;
	SI_32		asiTop		=	((float)(psiTop   +0))/((float)psiFrameH/ALC_STAT_V)	+	0.5f;
	SI_32		asiBottom	=	((float)(psiBottom+0))/((float)psiFrameH/ALC_STAT_V)	+	0.5f;
	SI_32		asiCenterX	=	(asiLeft+asiRight)>>1;
	SI_32		asiCenterY	=	(asiTop+asiBottom)>>1;

	AE_LOGI("[Touch AE]_ais_set_touch_ae called : ");

	if( asiLeft		< 0 )			asiLeft		=	0;
	if( asiTop		< 0 )			asiTop		=	0;
	if( asiRight	> ALC_STAT_H )	asiTop		=	ALC_STAT_H;
	if( asiBottom	> ALC_STAT_V )	asiTop		=	ALC_STAT_V;

	if( (asiRight-asiLeft)	>	8 )	{
		if( asiCenterX	>	ALC_STAT_H-4 )	asiCenterX	=	ALC_STAT_H-4;
		if( asiCenterX	<	4 )				asiCenterX	=	4;
		asiLeft		=	asiCenterX	-	4;
		asiRight	=	asiCenterX	+	4;
	}
	if( (asiBottom-asiTop)	>	8 )	{
		if( asiCenterY	>	ALC_STAT_V-4 )	asiCenterY	=	ALC_STAT_H-4;
		if( asiCenterY	<	4 )				asiCenterY	=	4;
		asiTop		=	asiCenterY	-	4;
		asiBottom	=	asiCenterY	+	4;
	}
	if( (asiRight-asiLeft)	<	3 )	{
		if( asiCenterX	>	ALC_STAT_H-1 )	asiCenterX	=	ALC_STAT_H-2;
		if( asiCenterX	<	1 )				asiCenterX	=	1;
		asiLeft		=	asiCenterX	-	1;
		asiRight	=	asiCenterX	+	2;
	}
	if( (asiBottom-asiTop)	<	3 )	{
		if( asiCenterY	>	ALC_STAT_V-1 )	asiCenterY	=	ALC_STAT_V-2;
		if( asiCenterY	<	1 )				asiCenterY	=	1;
		asiTop		=	asiCenterY	-	1;
		asiBottom	=	asiCenterY	+	2;
	}

	pptAisIf->muiFaceMode	=	2;		//(0:OFF, 1:FaceDetect-ON, 2:TouchAE-ON)
	pptAisIf->msiObjRatio	=	100;
#if 0
	pptAisIf->muiMltFDNum	=	1;

	pptAisIf->mttMltFDWnd[0].muqWinTop		=	asiTop		;
	pptAisIf->mttMltFDWnd[0].muqWinLeft		=	asiLeft		;
	pptAisIf->mttMltFDWnd[0].muqWinBott		=	asiBottom	;
	pptAisIf->mttMltFDWnd[0].muqWinRight	=	asiRight	;
#else
	pptAisIf->muqWinTop		=	asiTop		;
	pptAisIf->muqWinLeft	=	asiLeft		;
	pptAisIf->muqWinBott	=	asiBottom	;
	pptAisIf->muqWinRight	=	asiRight	;
#endif

	#undef ALC_STAT_H
	#undef ALC_STAT_V
}/*--------------------- End of function -----------------------*/

/*===========================================================================*/
void	_ais_reset_touch_ae(
		TT_AlAisInterface*  pptAisIf	)
/*------------------------------------------------------------------------*//**
Reset Touch AE

@return		 
@note

*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/
	AE_LOGI("[Touch AE]_ais_reset_touch_ae called : ");

	pptAisIf->muiFaceMode	=	0;		//(0:OFF, 1:FaceDetect-ON, 2:TouchAE-ON)
	pptAisIf->msiObjRatio	=	0;
	pptAisIf->muiMltFDNum	=	0;
}/*--------------------- End of function -----------------------*/




/*===========================================================================*/
void	_ais_set_face_detection_posi(
		TT_AlAisInterface*  pptAisIf	,
		UI_32				puiIdx		,		//	0 ~ 4/*ALAIS_MULTI_FD_MAX-1*/
		SI_32				psiLeft		,		//	i4Left	(0 ~ + resolution_info.frame_size.w 必須)
		SI_32				psiRight	,		//	i4Right
		SI_32				psiTop		,		//	i4Top	(0 ~ + resolution_info.frame_size.h 必須)
		SI_32				psiBottom	,		//	i4Bottom
		SI_32				psiFrameW	,		//		resolution_info.frame_size.w
		SI_32				psiFrameH	)		//		resolution_info.frame_size.h
/*------------------------------------------------------------------------*//**
Set Face Detect Position

@return		 
@note

*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/
	AE_LOGI("[FaceDetect]_ais_set_face_detection_posi called : ");

	#define ALC_STAT_H (16)
	#define ALC_STAT_V (16)

	SI_32		asiLeft		=	((float)(psiLeft  +0))/((float)psiFrameW/ALC_STAT_H)	+	0.5f;
	SI_32		asiRight	=	((float)(psiRight +0))/((float)psiFrameW/ALC_STAT_H)	+	0.5f;
	SI_32		asiTop		=	((float)(psiTop   +0))/((float)psiFrameH/ALC_STAT_V)	+	0.5f;
	SI_32		asiBottom	=	((float)(psiBottom+0))/((float)psiFrameH/ALC_STAT_V)	+	0.5f;
	SI_32		asiCenterX	=	(asiLeft+asiRight)>>1;
	SI_32		asiCenterY	=	(asiTop+asiBottom)>>1;

	if( asiLeft		< 0 )			asiLeft		=	0;
	if( asiTop		< 0 )			asiTop		=	0;
	if( asiRight	> ALC_STAT_H )	asiTop		=	ALC_STAT_H;
	if( asiBottom	> ALC_STAT_V )	asiTop		=	ALC_STAT_V;

	if( (asiRight-asiLeft)	>	8 )	{
		if( asiCenterX	>	ALC_STAT_H-4 )	asiCenterX	=	ALC_STAT_H-4;
		if( asiCenterX	<	4 )				asiCenterX	=	4;
		asiLeft		=	asiCenterX	-	4;
		asiRight	=	asiCenterX	+	4;
	}
	if( (asiBottom-asiTop)	>	8 )	{
		if( asiCenterY	>	ALC_STAT_V-4 )	asiCenterY	=	ALC_STAT_H-4;
		if( asiCenterY	<	4 )				asiCenterY	=	4;
		asiTop		=	asiCenterY	-	4;
		asiBottom	=	asiCenterY	+	4;
	}
	if( (asiRight-asiLeft)	<	3 )	{
		if( asiCenterX	>	ALC_STAT_H-1 )	asiCenterX	=	ALC_STAT_H-2;
		if( asiCenterX	<	1 )				asiCenterX	=	1;
		asiLeft		=	asiCenterX	-	1;
		asiRight	=	asiCenterX	+	2;
	}
	if( (asiBottom-asiTop)	<	3 )	{
		if( asiCenterY	>	ALC_STAT_V-1 )	asiCenterY	=	ALC_STAT_V-2;
		if( asiCenterY	<	1 )				asiCenterY	=	1;
		asiTop		=	asiCenterY	-	1;
		asiBottom	=	asiCenterY	+	2;
	}

//	pptAisIf->muiFaceMode	=	0;		//(0:OFF, 1:FaceDetect-ON, 2:TouchAE-ON)
//	pptAisIf->msiObjRatio	=	100;
	pptAisIf->muiMltFDNum	=	0;

	puiIdx	=	(puiIdx >= ALAIS_MULTI_FD_MAX_LOCAL-1) ? (ALAIS_MULTI_FD_MAX_LOCAL-1) : (puiIdx);
	pptAisIf->mttMltFDWnd[puiIdx].muqWinTop		=	asiTop		;
	pptAisIf->mttMltFDWnd[puiIdx].muqWinLeft	=	asiLeft		;
	pptAisIf->mttMltFDWnd[puiIdx].muqWinBott	=	asiBottom	;
	pptAisIf->mttMltFDWnd[puiIdx].muqWinRight	=	asiRight	;

	#undef ALC_STAT_H
	#undef ALC_STAT_V
}/*--------------------- End of function -----------------------*/
/*===========================================================================*/
UI_16	_ais_set_face_detection(
		TT_AlAisInterface*  pptAisIf	,
		UI_32				puiNum		)		//	1 ~ 5/*ALAIS_MULTI_FD_MAX*/
/*------------------------------------------------------------------------*//**
Set Face Detect

@return		 
@note

*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/
	LOGD("[Face Detect]_ais_set_face_detection called : %d",puiNum);

	UI_16	auiRet =  ALAIS_RET_INVALID_PRM;

	if( (puiNum < 1) || (puiNum > ALAIS_MULTI_FD_MAX_LOCAL-1) ) {
		return	auiRet;
	}
	auiRet	=	ALAIS_RET_OK;

	pptAisIf->muiFaceMode	=	1;		//(0:OFF, 1:FaceDetect-ON, 2:TouchAE-ON)
	pptAisIf->msiObjRatio	=	100;
	pptAisIf->muiMltFDNum	=	puiNum;

	return	auiRet;
}
/*===========================================================================*/
void	_ais_reset_face_detection(
		TT_AlAisInterface*  pptAisIf	)
/*------------------------------------------------------------------------*//**
Reset Face Detect

@return		 
@note

*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/
	LOGD("[Face Detect]ais_reset_face_detection called : ");

	pptAisIf->muiFaceMode	=	0;		//(0:OFF, 1:FaceDetect-ON, 2:TouchAE-ON)
	pptAisIf->msiObjRatio	=	0;
	pptAisIf->muiMltFDNum	=	0;
}/*--------------------- End of function -----------------------*/
