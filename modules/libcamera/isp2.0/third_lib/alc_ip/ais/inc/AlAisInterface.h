/*---------------------------------------------------------------------------**
Copyright (C) 2013-2014 ACUTElogic Corp.
ACUTElogic Confidential. All rights reserved.

**---------------------------------------------------------------------------**
$HeadURL: http://localhost:50120/svn/SPREADTRUM/Projects/15_03_Z3_Android/trunk/Protected/Ais/AisCore/include/AlAisInterface.h $
$LastChangedDate:: 2015-08-19 22:35:40 #$
$LastChangedRevision: 384 $
$LastChangedBy: seino $
**------------------------------------------------------------------------*//**
AIS Wrapper API Header


	@file
	@author	Masashi Seino
	@date	2010/10/14 start
	@note	prefix
*//*-------------------------------------------------------------------------*/
#ifndef ALAIS_INTERFACE_H
#define ALAIS_INTERFACE_H
/*=============================================================================
	Include
==============================================================================*/
/*-----------------------------------------------------------**
	AL Module Header
**-----------------------------------------------------------*/
#include	"AlTypStd.h"				// ALC Standard Header of typedef
#include	"AlEmbStd.h"				// ALC Standard Header for embedded system

//#include	"AlAisApi.h"
#include	"AlAisCommand.h"
//#include	"AlAisTypedefPlatform.h"
/*=============================================================================
	Macro
==============================================================================*/
#if defined(__BORLANDC__) || \
    defined(__WIN32__)    || \
    defined(_WIN32)       || \
    defined(__WIN64__)    || \
    defined(_WIN64)       ||  \
    defined(_Wp64)
/*-----------	PC向けビルド環境	-----------*/

#define NO_QCOM_CODE
#define VC_TEST

#else

//	#define  LOGD(str,args...)
//	#define  LOGE(str,args...)
//
	#ifdef NDK_DBGLOG		//ndk log
		#define DEBUG_PRINT_AIS_PRINTER
		#include <android/log.h>
//		#define  LOG_TAG    "AIS_NDK_LOG"
//		#define  LOGD(str,args...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,"[%s:%d]"str,__FUNCTION__,__LINE__,##args)
//		#define  LOGE(str,args...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,"[%s:%d]"str,__FUNCTION__,__LINE__,##args)
	#endif
#endif

#define AIS_LOG_MAX_SIZE	(768)

//---------------------------------------------------------------------------**
///	Products update History:Note.
//---------------------------------------------------------------------------**
/*
2014.05.23:API-Add(Tone-Curve I/F)::Victara,Quark
*/

//---------------------------------------------------------------------------**
///	Products Lens Module ID
//---------------------------------------------------------------------------**
typedef enum
{
	eAlAisModuleMR4R		= 0	,					//MR4 Rear(Ghost0)		(10M,QC LED)
	eAlAisModuleMR4F		= 1	,					//MR4 Front(Ghost1) 	( 2M,NonLED)
	eAlAisModule_Victara0	= 2 ,					//X+1 Rear 				(IMX135 valid for Victara)
	eAlAisModule_Victara1	= 3 ,					//X+1 Front				(AR0261 vaild for Victara)
	eAlAisModule_Titan0		= 4 ,					//DVX Big Rear			(IMX179 valid for Titan)
	eAlAisModule_Titan1		= 5 ,					//DVX Big Front			(IMX132 valid for Titan)
	eAlAisModule_Quark0		= 6 ,					//Quantam Rear			(IMX220 valid for Quark)
	eAlAisModule_Quark1		= 7 ,					//Quantam Front			(AR0261 valid for Quark)
	eAlAisModule_Falcon0	= 8 ,					//DVXLTE Rear			(AR0543 valid for Falcon)
	eAlAisModule_Falcon1	= 9 ,					//DVXLTE Front			(A1040  valid for Falcon)
	eAlAisModule_Condor0	= 10,					//ELX Rear				(AR0543 valid for Condor)

	eAlAisModuleMax
} TE_AlAisModule;

//---------------------------------------------------------------------------**
///	USER SYSTEM LOGCAT LEVEL
//---------------------------------------------------------------------------**
typedef enum
{
	eAlAisLogcatError		= 0	,					//Debug level:alPrinter::(0=Error,1=warning,2=Verbose)
	eAlAisLogcatWarning		= 1	,					//Debug level:alPrinter::(0=Error,1=warning,2=Verbose)
	eAlAisLogcatVerbose		= 2	,					//Debug level:alPrinter::(0=Error,1=warning,2=Verbose)
	eAlAisLogcatLocatTest0		,					//Debug level:alPrinter::AIS Local Test(reserved)
	eAlAisLogcatLocatTest1		,					//Debug level:alPrinter::AIS Local Test(reserved)
	eAlAisLogcatLocatTest2		,					//Debug level:alPrinter::AIS Local Test(reserved)
	eAlAisLogcatLocatTest3		,					//Debug level:alPrinter::AIS Local Test(reserved)
	eAlAisLogcatLocatTest4		,					//Debug level:alPrinter::AIS Local Test(reserved)
	eAlAisLogcatLocatTest5		,					//Debug level:alPrinter::AIS Local Test(reserved)
	eAlAisLogcatLocatTest6		,					//Debug level:alPrinter::AIS Local Test(reserved)
	eAlAisLogcatLocatTest7		,					//Debug level:alPrinter::AIS Local Test(reserved)
	eAlAisLogcatLocatTest8		,					//Debug level:alPrinter::AIS Local Test(reserved)
	eAlAisLogcatLocatTest9		,					//Debug level:alPrinter::AIS Local Test(reserved)

													//memo(reserved-ALC)
													//!?"ANDROID_LOG_FATAL"
													//!?"ANDROID_LOG_ERROR"
													//!?"ANDROID_LOG_WARN"
													//!?"ANDROID_LOG_VERBOSE"
													//!?"ANDROID_LOG_INFO"
													//!?"ANDROID_LOG_DEBUG"

	eAlAisLogcatMax
} TE_AlAisLogcat;

//---------------------------------------------------------------------------**
///	Android CameraParameter(WhiteBarance Mode)
//---------------------------------------------------------------------------**
typedef enum
{
	eAlAisCAMERA_WB_MIN_MINUS_1				,		// Auto(AisAwb=Auto)
	eAlAisCAMERA_WB_AUTO				= 1	,		// Default Wb = Auto(AisAwb=Auto)
	eAlAisCAMERA_WB_CUSTOM					,		//
	eAlAisCAMERA_WB_INCANDESCENT			,		// High priority
	eAlAisCAMERA_WB_FLUORESCENT				,		// High priority
	eAlAisCAMERA_WB_DAYLIGHT				,		// High priority
	eAlAisCAMERA_WB_CLOUDY_DAYLIGHT			,		// High priority
	eAlAisCAMERA_WB_TWILIGHT				,		//
	eAlAisCAMERA_WB_SHADE					,		//
	eAlAisCAMERA_WB_OFF						,		//
	eAlAisCAMERA_WB_MAX_PLUS_1				,		//

	eAlAisCAMERA_WB_Max
} TE_config3a_wb_t;

//---------------------------------------------------------------------------**
///	Android CameraParameter(BestShot Mode)
//---------------------------------------------------------------------------**
typedef enum
{
	eAlAisCAMERA_BESTSHOT_OFF			= 0	,		//Default Best Shot Mode = Auto(AisAwb=Auto)
	eAlAisCAMERA_BESTSHOT_AUTO			= 1	,		//Auto(AisAwb=Auto)
	eAlAisCAMERA_BESTSHOT_LANDSCAPE		= 2	,
	eAlAisCAMERA_BESTSHOT_SNOW				,
	eAlAisCAMERA_BESTSHOT_BEACH				,
	eAlAisCAMERA_BESTSHOT_SUNSET			,
	eAlAisCAMERA_BESTSHOT_NIGHT				,
	eAlAisCAMERA_BESTSHOT_PORTRAIT			,
	eAlAisCAMERA_BESTSHOT_BACKLIGHT			,
	eAlAisCAMERA_BESTSHOT_SPORTS			,
	eAlAisCAMERA_BESTSHOT_ANTISHAKE			,
	eAlAisCAMERA_BESTSHOT_FLOWERS			,
	eAlAisCAMERA_BESTSHOT_CANDLELIGHT		,
	eAlAisCAMERA_BESTSHOT_FIREWORKS			,
	eAlAisCAMERA_BESTSHOT_PARTY				,
	eAlAisCAMERA_BESTSHOT_NIGHT_PORTRAIT	,
	eAlAisCAMERA_BESTSHOT_THEATRE			,
	eAlAisCAMERA_BESTSHOT_ACTION			,
	eAlAisCAMERA_BESTSHOT_AR				,
	eAlAisCAMERA_BESTSHOT_MAX				,

	eAlAisCAMERA_BESTSHOT_Max
} TE_camera_bestshot_mode_type;

//---------------------------------------------------------------------------**
///	AE Setteled
//---------------------------------------------------------------------------**
typedef enum
{
	eAlAisIfAe_Resv		= 0	,					///<	(reserved)
	eAlAisIfAe_Settled		,					///<	AE Settled
	eAlAisIfAe_NoSettl		,					///<	AE Not Settle

	eAlAisIfAe_SettlMax
} TE_AlAisIfAeSettle;

//---------------------------------------------------------------------------**
///	Camera Mode
//---------------------------------------------------------------------------**
typedef enum
{
	eAlAisIfCamMode_DscPrv	= 0	,					///<	DCS Preview(ZSL)
	eAlAisIfCamMode_DscCap		,					///<	DSC Capture(NoZSL):(reserved)
	eAlAisIfCamMode_Dvc		,					///<	DVC

	eAlAisIfCamMode_Max
} TE_AlAisIfCamMode;

//---------------------------------------------------------------------------**
///	Evaluation metering Upper direction.
//---------------------------------------------------------------------------**
typedef	enum
{
	eAlAisIfAeMetDir_A_000	= 0	,					///<	  0:^	  A
	eAlAisIfAeMetDir_B_270		,					///<	270:>	D[ ]B
	eAlAisIfAeMetDir_C_180		,					///<	180:v	  C
	eAlAisIfAeMetDir_D_090		,					///<	 90:<

	eAlAisIfAeMetDir_Max
} TE_AlAisIfAeMetDir;
//---------------------------------------------------------------------------**
///	Exposure Channel
//---------------------------------------------------------------------------**

typedef enum
{
	eAlAisIfExpChnl_M		= 0	,						///<	Movie/Preview
	eAlAisIfExpChnl_S			,						///<	Still            :(reserved)
	eAlAisIfExpChnl_C			,						///<	current mode(old):(reserved)
#if 0
	eAlAisIfExpChnl_M_PUSH	,						///<	Movie/Preview Backup:(reserved)
	eAlAisIfExpChnl_M_LED_FLASH,						///<	Movie/Preview LED_FLASH Backup

	eAlAisIfExpChnl_B_0		,						///<	Bracket 0 Responce
	eAlAisIfExpChnl_B_1		,						///<	Bracket 1 Responce
	eAlAisIfExpChnl_B_2		,						///<	Bracket 2 Responce

	eAlAisIfExpChnl_Reserved5	,						///<	reserved(ALC)
	eAlAisIfExpChnl_Reserved4	,						///<	reserved(ALC)
	eAlAisIfExpChnl_Reserved3	,						///<	reserved(ALC)
	eAlAisIfExpChnl_Reserved2	,						///<	reserved(ALC)
	eAlAisIfExpChnl_Reserved1	,						///<	reserved(ALC)
	eAlAisIfExpChnl_Reserved0	,						///<	reserved(ALC)
#endif	
	eAlAisIfExpChnl_Max
} TE_AlAisIfExpChnl;


/*----------------------------------------------------------------------------*/
/*	AWB Line Adjustment data interface	*/
/*----------------------------------------------------------------------------*/
typedef struct 
{
	UI_32					muiR;
	UI_32					muiG;
	UI_32					muiB;
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


/*------------------------------------------------------------------------*/
/*	output data	for SPRD */
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

/*----------------------------------------------------------------------------*/
/*	Multi Face Detect Table	*/
/*----------------------------------------------------------------------------*/
#define	ALAIS_MULTI_FD_MAX_LOCAL		5
typedef struct
{
	UQ_32			muqWinTop;		/**< [u32.00]*/
	UQ_32			muqWinLeft;		/**< [u32.00]*/
	UQ_32			muqWinBott;		/**< [u32.00]*/
	UQ_32			muqWinRight;	/**< [u32.00]*/
}TT_AlAisMultFD_LOCAL;


/*=============================================================================
	Typedef
==============================================================================*/
/*----------------------------------------------------------------------------*/
/*	AIS Interface Table (I/O Data)	*/
/*----------------------------------------------------------------------------*/
typedef struct
{
	/*------------------------------------------------------------------------*/
	/*	Input data	*/
	/*------------------------------------------------------------------------*/
	TE_AlAisIfCamMode	muiCamMode;		//0:Preview,1:Capture,2:Movie30fps,3:Movie60fps
	UI_32			muiOpType;		//Non-ZSL, ZSL, camcoder:(reserved)
	UI_16			muiFcsReq;		//LED Focusing Seq Trigger(0:Off,1:Request)
	UI_16			muiLedReq;		//LED TTL Request  Trigger(0:Off,1:Request)
	UI_16			muiPebReq;		//ZSL PEB Request  Trigger(0:Off,1:Request):use by HDR-ON function
	UI_16			muiLedSta;		//LED Status(0:Off,bit0=1:Tourch,bit1=1:Flash,bit15=1:Configuration in Progress(0x8000))
	UI_16			muiFlashReq;	//LED (NEW)Input to start or stop a "Flash" Sequence(0:Stop/1:Start)
	UI_16			muiEvShiftReq;	//Exposure value Shift Request  Trigger(0:Off,1:Request):use by ALTM function
	//------------------------
	//	Statistics Data
	VP_32			mpvWdAe;		//UI_32 ExposureStats[V-block-size][H-block-size]	:sum or ave
	VP_32			mpvWdFlk;		//UI_32 ExposureStats[V-block-size]					:sum or ave
	VP_32			mpvWdAwb;		//UQ_32 AWBStats:TT_AlAisAwbWdBufInfo
									//
									//Acutelogic standard AWB stats type.
									//	typedef struct
									//	{
									//		SI_32	msiR;	/**< data R per block */
									//		SI_32	msiGr;	/**< data G per block */
									//		SI_32	msiGb;	/**< data G per block */
									//		SI_32	msiB;	/**< data B per block */
									//	}TT_AlAwbWdUnit;
									//	
									//	typedef struct
									//	{
									//		TT_AlAwbWdUnit		mttAwb[V-block-size][H-block-size];
									//		UI_32				muiAwbCnt[V-block-size][H-block-size];
									//	}TT_AlAisAwbWdBufInfo;
									//
	//------------------------
	//	Select Control camera.
	UI_32			muiModuleNo;	//	Products Lens Module ID::(UI_32)TE_AlAisModule
	//------------------------
	//	Statistics Info here
	UI_16			muiBlkH;		//Set the number of Statistics horizontal block.(=64 fixed)
	UI_16			muiBlkV;		//Set the number of Statistics vertical block.(=48 fixed)
	UI_16			muiBlkPixH;		//Set the number of the horizontal pixels per statistics 1 block.
	UI_16			muiBlkPixV;		//Set the number of the vertical pixels per statistics 1 block.
	UI_16			muiBlkPixH_Offset;	//Set the number of the horizontal pixels offset of statistics.
	UI_16			muiBlkPixV_Offset;	//Set the number of the vertical pixels offset of statistics.

	UI_16			muiMetUpp;		//	Metering Upper Side
	UI_32			muiHDTime;		//100*us[u32.00]:(reserved)
	//------------------------
	//	Shading table here
	VP_32*			mflBaseShdTbl[3]; //Set the address of the shading three tables.
									  //
									  //Acutelogic standard AWB stats type.
									  //float [4][muiBaseShdV][muiBaseShdH]:: [4]=0:R,1:Gr,2:Gb,3:B
									  //
	UI_32			muiBaseShdH;	//Set the number of the 1 shading tables horizontal data.(=17 fixed)
	UI_32			muiBaseShdV;	//Set the number of the 1 shading tables vertical data.(=13 fixed)
	//------------------------
	//	AWB/LSC OTP Info here
	VP_32*			muOtpAwbTbl;	//Set the address of the AWB OTP table.
	VP_32*			muOtpShdTbl;	//Set the address of the ASC/HSC OTP table.(reserved)
	//------------------------
	//	Face/Touch-AE
	UI_08			muiFaceMode;	//ROI mode:(0:OFF, 1:FaceDetect-ON, 2:TouchAE-ON)
	UQ_32			muqWinTop;		//Vertical top-left position of the statistics area(Fixed point:16.16)(0x00000000 - 0x000F0000)
	UQ_32			muqWinLeft;		//Horizontal top-left position of the statistics area(Fixed point:16.16)(0x00000000 - 0x000F0000)
	UQ_32			muqWinBott;		//Vertical Bottom-right position of the statistics area(Fixed point:16.16)(0x00000000 - 0x000F0000)
	UQ_32			muqWinRight;	//Horizontal Bottom-right position of the statistics area(Fixed point:16.16)(0x00000000 - 0x000F0000)
	SI_32			msiObjRatio;	//ROI Metering area weight ratio(Signed int:S31.0)(0x00000001 - 0x00000064)
	UI_16					muiMltFDNum;								/**< Face Window Count	*/
	TT_AlAisMultFD_LOCAL	mttMltFDWnd[ALAIS_MULTI_FD_MAX_LOCAL];		/**< Face Rect 	*/
	//------------------------
	//	Zoom-Frame
	UI_08			muiZoomMode;	//Zoom mode:(0:OFF, 1:ON)
	UQ_32			muqZoomWinTop;	//Vertical top-left position of the statistics area(Fixed point:16.16)(0x00000000 - 0x000F0000)
	UQ_32			muqZoomWinLeft;	//Horizontal top-left position of the statistics area(Fixed point:16.16)(0x00000000 - 0x000F0000)
	UQ_32			muqZoomWinBott;	//Vertical Bottom-right position of the statistics area(Fixed point:16.16)(0x00000000 - 0x000F0000)
	UQ_32			muqZoomWinRight;//Horizontal Bottom-right position of the statistics area(Fixed point:16.16)(0x00000000 - 0x000F0000)
	//------------------------
	//	mainly used by a HDR function.
	UI_08			muiLvcMode;		//Lvc Mode(=GTM function):(0:Disable/1:Enable)
	UI_08			muiTargetRate;	//luminance target rate:(1-90[%]:0x01 - 0x5A)
									//Note: Setting is enable only at the time of LVC-MODE disable.
	//------------------------
	//	AE Bracket(=HDR-ON/ALTM function)
	SQ_32			msqAebList[5];	//EV Bias value list(APEX value:-3.0::0xFFFD0000 - +3.0:0x00030000)
	UI_16			muiAebCnt;		//EV Bias value list count(0x03: EV0,EV+,EV-)(=3 fixed)
	//------------------------
	//	mainly used by a AF interface.
	UI_16			muiAeLock;		//AIS(AE)Lock/Unlock:(1:Lock/0:Unlock)
	UI_16			muiAwbLock;		//AIS(AWB)Lock/Unlock:(1:Lock/0:Unlock)
	UI_08			muiAeSpeedRate;	//Convergence speed rate:(1-100[%]:0x01 - 0x64::Max speed is 100%)
	UQ_32			muqAfDistanceOptimal;	//Af Distance:Optimal(unit:meter)
	UQ_32			muqAfDistanceFar;		//Af Distance:Far(unit:meter)
	UQ_32			muqAfDistanceNear;		//Af Distance:Near(unit:meter)
	//------------------------
	//	Sensor Parameter
	SQ_32			msqMaxFPS;		// Effective value of FPS.(Max Frame Rate)[s15.16]
	UI_32			muiLineTime;	// ns/10
	UI_32			muiFrameLine;
	//------------------------
	//	Set Command Parameter
	UI_16			muiCurrEvpNo;					//	Current EV Program Number
	//	Flash mode
	UI_16			muiDualFlashMode;				//Single : 0, Dual : 1
	/*------------------------------------------------------------------------*/
	/*	output data	*/
	/*------------------------------------------------------------------------*/
	float			mflExifTime;		// time[sec](reserved)
	float			mflExifGain;		// gain[dB](reserved)
	float			mflSv;				// Sv[APEX]
	/*------------------------------------------------------------------------*/
	float			mflExpTime[eAlAisIfExpChnl_Max][5];		// time[sec]
	UI_16			muiExpLine[eAlAisIfExpChnl_Max][5];		// Line(HD) count
	float			mflSensor_gain[eAlAisIfExpChnl_Max][5];	// gain[dB]

#if 1
	float			mflWbg[3];			//AWB WB GAIN
	float			mflCcMatrix[9];		//AWB CC Matrix
	SI_32			msiCtemp;			//AWB corol-Temperature
	SI_32			msiAwbInOut;		//AWB (reserved)
#else
	TT_AlAwbOut		mttAwbOut;
	TT_AlAwbOut		mttAwbLedOut;
#endif

	float			mflBv;				//BV

	TT_AlLscOut		mttLscOut;

#if 1
	UI_16			muiAisExpIdx;		//(reserved):exp_index     - X = 23.44977*BV
	UI_16			muiAisLuxIdx;		//(reserved):lux_index     - X = 23.44977*BV_targ = 23.44977*BV + MV
	UI_16			muiAisIndIdx;		//(reserved):indoor_index  - X = 23.44977*BV_THR_INDOOR
	UI_16			muiAisOudIdx;		//(reserved):outdoor_index - X = 23.44977*BV_THR_OUTDOOR
	float			mflAisTrgLuma;		//(reserved):luma_targer   = Y_target = 255 * AE_Target[%]
	float			mflAisFrmLuma;		//(reserved):frame_luma    = Y_target * 2^MV
	float			mflAisLux;
//	SI_32			msiTargetLuma;
#endif
#if 0
//not support yet
	VP_32*			mptGamma;							//	Tone(Gamma)Table(12bit->12bit):Victara,Quark
	UI_08			mpiTone[ALAIS_GAMMA_TONE_TBL_CNT];	//	Tone(Gamma)Table(10bit->8bit)
	UI_16			mpiToneW[ALAIS_GAMMA_TONE_TBL_CNT];	//	Tone(Gamma)Table(1xbit->1xbit):reserved
#endif
	//------------------------
	//	mainly used by a AF interface.
	SQ_32			msqAeErrorRratio;	//the ratio between current exposure and the exposure target.
										//(If the target, the current is 0[%]):(0x00000000～0x00640000).
										//100% of tone error is 1EV or more.
	SQ_32			msqAeCurrentBv;		//the current exposure value(=BV).
	SQ_32			msqAeTargetBv;		//the target exposure value(=BV).
	//------------------------
	//	LED Flash
	UI_16			muiAeSettle;		//	AE Settle Status		::Bool(True:complete,False:inprogress)

	UI_16			muiLedDiming;		//(reserved):LED Dimming Status		::Bool(True:inprogress,False:complete)
	UI_16			muiLedBright;		//(reserved):LED Flash Brightness	::Index
	float			mflLedTime;			//(reserved):LED Flash Time			::time[sec]
	UI_16			muiStatsRdy;		//	to indicate the Pre-Flash statistics have been collected.
	//------------------------
	// Dual LED
	UI_16			muiLedCoolBright;
	UI_16			muiLedWarmBright;

	/*------------------------------------------------------------------------*/
	/*	ADD AIS Interface data	*/
	/*------------------------------------------------------------------------*/
															//It is effective in a range of EV Program.
	UI_32			muiSetMinFpsRange;						//Set Min Fps Range = 15-60:Min<=Max
	UI_32			muiSetMaxFpsRange;						//Set Max Fps Range = 15-60:Max>=Min
	//------------------------
	//	Android Camera Parameter
	UI_32			muiSetWbMode;							//Set WhiteBarance Mode(TE_config3a_wb_t)
	UI_32			muiSetBestShotMode;						//Set BestShot Mode(TE_camera_bestshot_mode_type)
															//
															//Exposure Compensation Spec = Android Parameter.
	SI_32			muiSetExposureCompensationIndex;		//Set Exposure Compensation Index(0,+n,-n)
	SI_32			muiSetExposureCompensationDenominator;	//Set Exposure Compensation Denominator(Index/Denominator)
	UI_32			muiSetIsoGain;							//Set Iso Gain Value(0=auto,val=160/200/400/800/1600/デバイス範囲外の指定は近い値を使用する...)

	//---------------------------------------------------------------------------**
	///	AE Metering Upper Position
	UI_32			muiSetMeteringDirection;				//Set evaluation metering Upper direction.(TE_AlAisAeMetDir)

	//---------------------------------------------------------------------------**
	///	AE Metering Value
	SQ_32			msqFrameAveBV;							//AisLib Output AverageBV
	SQ_32			msqFrameAveLux;							//AisLib Output AverageLux(reserved:Formula is undecided)

	//---------------------------------------------------------------------------**
	///	AIS ActiveROI Mode
	UI_16			muiActiveRoiMode;						//ActiveRoi Mode(=An automatic center weight adjustment function in the lowlight.)
															//(0:Disable/1:Enable)
	//------------------------
	//	AWB Result Data
	UI_16			muiDetectCtemp;							// AWB Detect(Frame) Color Temp
	UI_16			muiCurrentCtemp;						// AWB Current Color Temp
	UI_16			muiDetectX;								// AWB Detect xy
	UI_16			muiDetectY;								// 
	UI_16			muiCurrentX;							// AWB Current xy
	UI_16			muiCurrentY;							// 

	//------------------------
	// LED Flash Device ID
	UI_32			muiLedDevID;							// LED Device ID
															// 0: Original(5000K), 1:New(5500K)
	UI_32			muiReserveDevID0;						// Reserve for LED Part to Part Correction
	UI_32			muiReserveDevID1;						// Reserve for LED Part to Part Correction

	// Preflash Information
	UI_32			muiPreLedIndex_0;						//(reserved) Output : LED_0 strength for Pre Flash
	UI_32			muiPreLedIndex_1;						//(reserved) Output : LED_1 strength for Pre Flash
	UI_32			muiMaxPreLedIndex_0;					//(reserved) Input  : Maximum LED_0 strength for Pre Flash
	UI_32			muiMaxPreLedIndex_1;					//(reserved) Input  : Maximum LED_1 strength for Pre Flash

	// Main Flash Information
	UI_32			muiLedIndex_0;							//           Output : LED_0 strength for Main Flash
	UI_32			muiLedIndex_1;							//(reserved) Output : LED_1 strength for Main Flash
	UI_32			muiMaxLedIndex_0;						//(reserved) Input  : Maximum LED_0 strength for Main Flash
	UI_32			muiMaxLedIndex_1;						//(reserved) Input  : Maximum LED_1 strength for Main Flash


	//  AIS IP.
	UI_32			muiAisMode;								//(reserved) AIS Mode:
															//(reserved)  0: Enable Full Function(AE/AWB)
															//(reserved)  1: Enable AIS AWB
															//(reserved)  2: Enable AIS AWB and LED Flash sequence

	// Input from qc_aec(for Calculate BV)
	struct {
		float	mflExpTime;									//(reserved) Exposure Time
		float	mflAperture;								//(reserved) Aperture Value
		float	mflGain;									//(reserved) Gain
		float	mflEvBias;									//(reserved) Exposure value Bias
	} mttExpVal;

	//------------------------
	// Input from qc_aec(for Led light ratio)
	float	mflLedRatio;									//(reserved) The Ratio of LED flash light and Environment Light
															//           Ratio == 1.0 : 100% LED Flash Light
															//           Ratio == 0.0 : 100% Environment Light

	//------------------------
	//	ALC LED Debug Parameter
	UI_32			muiAlcLedDebugFlashMode;				//(reserved) Single : 0, 2StageFlash : 1, ValiableFlash : 2, DualFlash : 3
	UI_32			muiMinPreLedIndex_0;					//(reserved) Input  : Minimum LED_0 strength for Pre Flash
	UI_32			muiMinLedIndex_0;						//(reserved) Input  : Minimum LED_0 strength for Main Flash

	//------------------------
	//	Yobi API(reserved)
	UI_16			muiYob16Api[ 5];						//(reserved)
	UI_32			muiYobiApi[15];							//(reserved)
	VP_32*			mptYobiApi[10];							//(reserved)
	/*------------------------------------------------------------------------*/
	/*	AIS data	*/
	/*------------------------------------------------------------------------*/
	UI_32*			mpiLog;
	UI_32			muiLogSize;
	TT_PointUI16*	mptXyMap;			//16x16 block x,y data
	/*callback funcution for accsess to system from AIS*/
		//nothing
//	UI_32			muiLux;

	/*Ais lib instance*/
	VP_32				mpvAisIf;
	TT_AlAwbAwbLineData mttAwbLineData;	//(reserved)
	TT_AlLscLineData		mttLscLineData;

	//debug
	UI_32			muiDebug[8];

}TT_AlAisInterface;

/*=============================================================================
	Prototype
==============================================================================*/

/*===========================================================================*/
EXTERN	void		AlAisIfInit(
					TT_AlAisInterface*		pptInterface )
/*------------------------------------------------------------------------*//**

@return         void
@note
@attention None
*//*-------------------------------------------------------------------------*/
;//
/*===========================================================================*/
EXTERN	void		AlAisIf_Destroy(
					TT_AlAisInterface*		pptInterface )
/*------------------------------------------------------------------------*//**

@return         void
@note
@attention None
*//*-------------------------------------------------------------------------*/
;

/*===========================================================================*/
EXTERN void		AlAisIfReset(
				TT_AlAisInterface*		pptInterface)	// [I ] Mode 0: Monitor mode /1: Capture mode
/*------------------------------------------------------------------------*//**

@return         void
@note
@attention None
*//*-------------------------------------------------------------------------*/
;//

/*===========================================================================*/
EXTERN void		AlAisIfMain(
				TT_AlAisInterface*		pptInterface)
/*------------------------------------------------------------------------*//**

@return         void
@note
@attention None
*//*-------------------------------------------------------------------------*/
;//

/*===========================================================================*/
EXTERN UI_16		AlAisIfSendCommand(
					TT_AlAisInterface*		pptInterface	,
					TT_AlAisCmd*			pptCmd			)
/*------------------------------------------------------------------------*//**

@return         UI_16
@note
@attention None
*//*-------------------------------------------------------------------------*/
;//


/*===========================================================================*/
EXTERN void		AlAisIf_getMetalog(
				TT_AlAisInterface*		pptInterface	)
/*------------------------------------------------------------------------*//**

@return         void
@note
@attention None
*//*-------------------------------------------------------------------------*/
;
#if 0 //def VC_TEST
/*===========================================================================*/
EXTERN TT_AlAisInfo*	AlAisIf_GetCurretInfo(
						TT_AlAisInterface*		pptInterface	)
/*------------------------------------------------------------------------*//**

@return         void
@note
@attention None
*//*-------------------------------------------------------------------------*/
;//
#endif	//#ifdef VC_TEST

/*=============================================================================
	Variable
==============================================================================*/
#endif	//#ifndef ALAIS_INTERFACE_H

