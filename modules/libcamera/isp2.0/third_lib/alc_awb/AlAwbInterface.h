/*---------------------------------------------------------------------------**
Copyright (C) 2014 ACUTElogic Corp.
ACUTElogic Confidential. All rights reserved.

**---------------------------------------------------------------------------**
$HeadURL: http://localhost:50120/jja/svn/qct_kr/Projects/1406_sam_mega2/trunk/AlAwbCore/include/AlAwbInterface.h $
$LastChangedDate:: 2015-04-28 12:59:19 #$
$LastChangedRevision: 511 $
$LastChangedBy: seino $
**------------------------------------------------------------------------*//**
AWB lib Interface API Header

	@file
	@author	Masashi Seino
	@date	2014/06/11
	@note
*//*-------------------------------------------------------------------------*/
#ifndef ALAIS_WRAP_H
#define ALAIS_WRAP_H
/*=============================================================================
	Include
==============================================================================*/
/*-----------------------------------------------------------**
	AL Module Header
**-----------------------------------------------------------*/
#include	"AlTypStd.h"				// ALC Standard Header of typedef
#include	"AlEmbStd.h"				// ALC Standard Header for embedded system
#include	"AlCommandDefinition.h"
/*=============================================================================
	Macro
==============================================================================*/
#if defined(__BORLANDC__) || \
    defined(__WIN32__)    || \
    defined(_WIN32)       || \
    defined(__WIN64__)    || \
    defined(_WIN64)       ||  \
    defined(_Wp64)
/*-----------	for simulation on windows -----------*/

#define NO_QCOM_CODE
#define VC_TEST

#else

	#define  LOGD(str,args...)
	#define  LOGE(str,args...)

	#ifdef NDK_DBGLOG		//ndk log
		#define DEBUG_PRINT_AIS_PRINTER
		#include <android/log.h>
		#define  LOG_TAG    "AWB_NDK_LOG"
		#define  LOGD(str,args...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,"[%s:%d]"str,__FUNCTION__,__LINE__,##args)
		#define  LOGE(str,args...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,"[%s:%d]"str,__FUNCTION__,__LINE__,##args)
	#endif

#endif


#define AL_MAX_AWB_STATS_NUM  (32*32)
//LOG size Definition
#define AIS_LOG_MAX_SIZE	( 500 * 2)

#define AWB_OTP_NEW
/*=============================================================================
	Typedef
==============================================================================*/

//---------------------------------------------------------------------------**
///	Camera Mode
//---------------------------------------------------------------------------**
typedef enum
{
	eAlAwbCamMode_DscPrv	= 0	,					///<	DCS Preview
	eAlAwbCamMode_DscCap		,					///<	DSC Capture
	eAlAwbCamMode_Dvc			,					///<	DVC

	eAlAwbCamMode_Max
} TE_AlAwbCamMode;



/*----------------------------------------------------------------------------*/
/*	Multi Face Detect Table	*/
/*----------------------------------------------------------------------------*/
#define	ALAIS_MULTI_FD_MAX		5
typedef struct
{
	UQ_32			muqWinTop;		/**< Top-left			[Block:u32.00]*/
	UQ_32			muqWinLeft;		/**< Top-right			[Block:u32.00]*/
	UQ_32			muqWinBott;		/**< Bottom-left +1		[Block:u32.00]*/
	UQ_32			muqWinRight;	/**< Bottom-right +1	[Block:u32.00]*/
}TT_AlAwbMultFD;

/*----------------------------------------------------------------------------*/
/*	AWB Line Adjustment data interface	*/
/*----------------------------------------------------------------------------*/
typedef struct
{
#ifdef AWB_OTP_NEW
	UI_32					muiR;
	UI_32					muiG;
	UI_32					muiB;
#else
	float	mflRg;		/**< R/G*/
	float	mflBg;		/**< B/G*/
#endif
}TT_AlAwbRgbRatioData;

#define ALAIS_AWB_LINE_ADJUSTMENT_OFF	0
#define ALAIS_AWB_LINE_ADJUSTMENT_ON	1
typedef struct
{
	UI_32					muiMode;				/**< Mode 0:off,1:on 2:On(ref data provided by sys)*/
	TT_AlAwbRgbRatioData		mttLo;				/**< Lo Temp*/
	TT_AlAwbRgbRatioData		mttHi;				/**< Hi Temp*/

	//-----
	TT_AlAwbRgbRatioData		mttRefLo;				/**< Lo Temp*/
	TT_AlAwbRgbRatioData		mttRefHi;				/**< Hi Temp*/
}TT_AlAwbAwbLineData;

typedef struct
{
	UI_32					muiMode;				/**< Mode 0:off,1:on 2:On(ref data provided by sys)*/

	UI_16*					mpiOtp;					/**< */
	UI_16*					mpiOtpRef;
	UI_32					muiWidth;
	UI_32					muiHight;
}TT_AlLscLineData;


typedef struct
{
	UI_32	muiR[AL_MAX_AWB_STATS_NUM];	/**< WD data R per block */
	UI_32	muiG[AL_MAX_AWB_STATS_NUM];	/**< WD data G per block */
	UI_32	muiB[AL_MAX_AWB_STATS_NUM];	/**< WD data B per block */
	UI_32	muiC[AL_MAX_AWB_STATS_NUM];	/**< level Gate count	 */
}TT_AlAwbStats;



typedef enum
{
	eAlAwbFlash_state_Off = 0,		//Off
	eAlAwbFlash_state_Main,			//Main
	eAlAwbFlash_state_touch,		//Pre flash
	eAlAwbFlash_state_touchMain,	//for Front flash
	eAlAwbFlash_state_touchMovie,	//for Movie			//reserved for refactoring
}TE_AlAwbFlashState;



/*----------------------------------------------------------------------------*/
/*	AIS Interface Table (I/O Data)	*/
/*----------------------------------------------------------------------------*/


/*------------------------------------------------------------------------*/
/*	output data	*/
/*------------------------------------------------------------------------*/
typedef struct tg_AlAwbOut
{
	UI_32			muiWbg[3];								// wb gain
	float			mflCcMatrix[9];							// CCmatrix
	SI_32			msiCtemp;								// color temperture
	SI_32			msiAwbInOut;

	//------------------------
	// for Log
	UI_32*			mpiLog;
	UI_32			muiLogSize;
}TT_AlAwbOut;

typedef struct tg_AlLscOut
{
	UI_16*			mpiTbl;			// LscTable
	UI_16			muiSize;		// LscTable size(array num)
	//------------------------
	// for Log
	UI_32*			mpiLog;
	UI_32			muiLogSize;
}TT_AlLscOut;



/*------------------------------------------------------------------------*/
/*	Input data	*/
/*------------------------------------------------------------------------*/
typedef struct tg_AlAwbInterfaceIn
{
	//------------------------
	//	Operation mode
	TE_AlAwbCamMode	muiCamMode;		//0:Preview,1:Capture,2:Movie
	UI_32			muiOpType;		//Non-ZSL, ZSL, camcoder
	//------------------------
	//	Statistics Data
//	VP_32			mpvWdAe;		//pointer of statistics for AE
//	VP_32			mpvWdFlk;		//pointer of statistics for Flicer detection
	TT_AlAwbStats*	mptWdAwb;		//pointer of statistics for AWB
	//------------------------
	//	Statistics Info here
	UI_16			muiBlkH;		//Statistics block Horizontal number per frame
	UI_16			muiBlkV;		//Statistics block Vertical number per frame
	UI_16			muiBlkPixH;		//pixel number Horizontal per block
	UI_16			muiBlkPixV;		//pixel number Vertical per block

#if 0
	//------------------------
	//	Face / touch AE (ROI Infomation)
	UI_08			muiFaceMode;					/**< Face/touchAE  ON:0x01/OFF:0x00*/
	SI_32			msiObjRatio;					/**< Area Weight */
	UI_16			muiMltFDNum;					/**< window number	*/
	TT_AlAwbMultFD	mttMltFDWnd[ALAIS_MULTI_FD_MAX];/**< window definition 	*/
#endif
	//------------------------
	// AEC infomation
	float				mflBv;
	float				mflAv;
	float	 			mflTv;
	float	 			mflSv;
	float	 			mflEv;
	TE_AlAwbFlashState	msiFlash_state;	//0:Off, 1:Main , 2: Pre
	//for LED Ratio
	float				mflLedOff;
	float	 			mflLedLow;
	float	 			mflLedHi;
	// 20141219
#ifdef AL_CAMCORDER_LED_TORCH
	UI_32				muiAecLedMode;		//1=CAMCORDER & LED_TORCH	MapRangeをIndoor固定に
#endif	//#ifdef AL_CAMCORDER_LED_TORCH
	//------------------------
	// Line Data
	TT_AlAwbAwbLineData mttAwbLineData;
	TT_AlLscLineData	mttLscLineData;
}TT_AlAwbInterfaceIn;


typedef struct
{
	UI_32			muiModuleNo;	//Lens module No.	for select tuning parameter
	UI_32			muiStructSize;	//This　Structer Size for check version matching
	/*------------------------------------------------------------------------*/
	/*	Input data	*/
	/*------------------------------------------------------------------------*/
	TT_AlAwbInterfaceIn mttIn;
	/*------------------------------------------------------------------------*/
	/*	output data	*/
	/*------------------------------------------------------------------------*/
	TT_AlAwbOut mttOut;
	TT_AlAwbOut mttOutLed;

	TT_AlLscOut mttOutLsc;
	/*------------------------------------------------------------------------*/
	/*	for keep internal status	*/
	/*------------------------------------------------------------------------*/
	VP_32			mpvPriv;		/*Private lib instanse*/
}TT_AlAwbInterface;

/*=============================================================================
	Prototype
==============================================================================*/

/*===========================================================================*/
EXTERN	void		AlAwbInterfaceInit(
					TT_AlAwbInterface*		pptInterface )
/*------------------------------------------------------------------------*//**

@return         void
@note
@attention None
*//*-------------------------------------------------------------------------*/
;//
/*===========================================================================*/
EXTERN	void		AlAwbInterfaceDestroy(
					TT_AlAwbInterface*		pptInterface )
/*------------------------------------------------------------------------*//**

@return         void
@note
@attention None
*//*-------------------------------------------------------------------------*/
;

/*===========================================================================*/
EXTERN void		AlAwbInterfaceReset(
				TT_AlAwbInterface*		pptInterface)	// [I ] Mode 0: Monitor mode /1: Capture mode
/*------------------------------------------------------------------------*//**

@return         void
@note
@attention None
*//*-------------------------------------------------------------------------*/
;//

/*===========================================================================*/
EXTERN void		AlAwbInterfaceMain(
				TT_AlAwbInterface*		pptInterface)
/*------------------------------------------------------------------------*//**

@return         void
@note
@attention None
*//*-------------------------------------------------------------------------*/
;//
/*===========================================================================*/
EXTERN UI_16	AlAwbInterfaceSendCommand(
				TT_AlAwbInterface*		pptInterface	,
				TT_AlAisCmd*			pptCmd			)
/*------------------------------------------------------------------------*//**

@return         void
@note
@attention None
*//*-------------------------------------------------------------------------*/
;//

/*===========================================================================*/
EXTERN void		AlAwbInterfaceShowVersion(
				TT_AlAwbInterface*		pptInterface	)
/*------------------------------------------------------------------------*//**
Liblary version show
@return         void
@note
@attention None
*//*-------------------------------------------------------------------------*/
;//
/*=============================================================================
	Variable
==============================================================================*/
#endif	//#ifndef ALAIS_WRAP_H

