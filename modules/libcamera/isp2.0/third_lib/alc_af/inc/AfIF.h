#ifndef _AFIF_H
#define _AFIF_H
#define ALC_AF_DEFOCUS_TEST		(0) //for defocus test
//#define PROTDEF 1

//#if PROTDEF 1

#include "AlTypStd.h"
#include "AlEmbStd.h"

//#endif

/* Multi Spot AF Window ROI */
typedef struct {
	UI_16	muiHpos;				/* Horizontal position */
	UI_16	muiVpos;				/* Vertical position */
	UI_16	muiHsize;				/* Window width */
	UI_16	muiVsize;				/* Window hight */
} TT_AfIfBufRoi;

/* Criteria for scene */
typedef struct {
	SI_32	msiSceneAuto;				/* Auto */
	SI_32	msiSceneNight;				/* Night */
	SI_32	msiScenePortrait;			/* Portrate */
	SI_32	msiSceneLandscape;			/* Landscape */
	SI_32	msiSceneSunet;				/* Sunrise / Sunset */
	SI_32	msiSceneParty;				/* party */
	SI_32	msiSceneMotion;				/* Moving object / Sport / Pets */
	SI_32	msiSceneBeach;				/* Beach */
	SI_32	msiSceneFireworks;			/* Fireworks */
	SI_32	msiSceneCandleLight;			/* Candle */
} TT_AfIfScene;

/* Compensation value for VCM inf. */
typedef struct {
	UI_16	muiStartupInf;				/* Set the driver IC MCU value of startup moving lens for individual module */
	UI_16	muiInf;					/* Set the driver IC MCU value of moving lens at infinity focus position for individual module */
	UI_16	muiMacro;				/* Set the driver IC MCU value of moving lens at macro focus position for individual module */
	UI_16	muiScanRangeInf;			/* AF scan range should be wider than calibrated inf and macro position. The start position of AF inf is stored hear. */
	UI_16	muiScanRangeMacro;			/* The end position of AF macro is storedd hear. */
	UI_08	muiCalbrationDirection;			/* 0x00 for sideway, 0x01 for up */
} TT_AfIfVcmCalibrationData;

/* ROI for 9 window */
typedef struct {
	TT_AfIfBufRoi	muiAfWindowMulti0;	/* Multi Spot AF Window ROI (Left upper) */
	TT_AfIfBufRoi	muiAfWindowMulti1;	/* Multi Spot AF Window ROI (center upper) */
	TT_AfIfBufRoi	muiAfWindowMulti2;	/* Multi Spot AF Window ROI (Right upper) */
	TT_AfIfBufRoi	muiAfWindowMulti3;	/* Multi Spot AF Window ROI (Left middle) */
	TT_AfIfBufRoi	muiAfWindowMulti4;	/* Multi Spot AF Window ROI (Center middle) */
	TT_AfIfBufRoi	muiAfWindowMulti5;	/* Multi Spot AF Window ROI (Right middle) */
	TT_AfIfBufRoi	muiAfWindowMulti6;	/* Multi Spot AF Window ROI (Left Lower) */
	TT_AfIfBufRoi	muiAfWindowMulti7;	/* Multi Spot AF Window ROI (Center lower) */
	TT_AfIfBufRoi	muiAfWindowMulti8;	/* Multi Spot AF Window ROI (Right lower) */
} TT_AfIfBufWindowMulti;

/* AF or AE evaluation value */
typedef struct {
	UI_32	muiAfEvaluationValue0;		/* AF or AE evaluation value (Upper Left) */
	UI_32	muiAfEvaluationValue1;		/* AF or AE evaluation value (Upper Center) */
	UI_32	muiAfEvaluationValue2;		/* AF or AE evaluation value (Upper Right) */
	UI_32	muiAfEvaluationValue3;		/* AF or AE evaluation value (Middle Left) */
	UI_32	muiAfEvaluationValue4;		/* AF or AE evaluation value (Middle Center) */
	UI_32	muiAfEvaluationValue5;		/* AF or AE evaluation value (MIddle Right) */
	UI_32	muiAfEvaluationValue6;		/* AF or AE evaluation value (Lower Left) */
	UI_32	muiAfEvaluationValue7;		/* AF or AE evaluation value (Lower Center) */
	UI_32	muiAfEvaluationValue8;		/* AF or AE evaluation value (Lower Right) */
} TT_AfIfBufEvaluationValue;

/* VCM compensation coefficient at near end. */
typedef struct {
	UI_16	muiStartupInfRatio;			/* Set the driver IC MCU value of startup moving lens for individual module */
	UI_16	muiInfRatio;				/* Set the driver IC MCU value of moving lens at infinity focus position for individual module */
	UI_16	muiMacroRatio;				/* Set the driver IC MCU value of moving lens at macro focus position for individual module */
	UI_16	muiScanRangeInfRatio;			/* AF scan range should be wider than calibrated inf and macro position. The start position of AF inf is stored hear. */
	UI_16	muiScanRangeMacroRatio;			/* The end position of AF macro is storedd hear. */
} TT_AfIfVcmRatio;

/* Criteria for mode*/
typedef struct {
	TT_AfIfScene	mttAfStill;			/* Criteria for Still */
	TT_AfIfScene	mttAfPreview;			/* Criteria for Preview */
	TT_AfIfScene	mttAfMovie;			/* Criteria for Movie */
} TT_AfIfBufAfModeC;

/* Adjustment data for VCM */
typedef struct {
	TT_AfIfVcmCalibrationData	mttVcmCalibration;	/* Compensation value for VCM inf. */
} TT_AfIfBufVcmCalibration;

/* System information such as Gyro, Acc etc. */	
typedef struct {
	SI_32	msiFV;					/* Focus Value */
	SI_32	msiPS;					/* Luminance */
	SI_32	msiAccelerometer;			/* Accelerometer sensor */
	SI_32	msiGyro;				/* Gyro sensor */
	SI_32	msiTilt;				/* Tilt sensor */
	SI_32	msiReserve;				/* other sensor */
	UI_08	muiStatus;				/* status Scene */
} TT_AfIfBufSensorInfo;

/* AFmode */
typedef struct {
	UI_08	muiAfMode;				/* Single / Continuos / AF Lock */
	UI_08	muiAfSingleMode;			/* Sweep / Single pass / Multi pass / other */
	UI_08	muiAfContMode;				/* Preview / Camcorder */
	SI_32	msiAfContSpecified;			/* Face or given ROI priority Focus */
	SI_32	msiAfContObjTrck;			/* others */
} TT_AfIfBufAfMode;

/* AF evaluation value Window setting */
typedef struct {
	UI_08	muiAfWindowMode;			/* Auto Focus mode ( Spot / Multi / Face / Object / Auto ) */
	UI_08	muiDirOfRoiInformation;			/* 0 : Host -> AF IP / 1 : AF IP ->Å@Host */
	TT_AfIfBufRoi	mttAfSingleSpotRoi;		/* Single mode Spot ROI */
	TT_AfIfBufWindowMulti	muiAfMult3x3Roi;	/* ROI for 9 window */
	SI_32	msiAfMultiSpotCenterWeighted;		/* center priolity */
} TT_AfIfBufWindow;

/* AF assiste light */
typedef struct {
	UI_08	muiAfAssistLightControl;		/* 0: Off / 1:On */
	SI_32	Reserved;				/* reserve */
} TT_AfIfBufAssistLight;

/* External device info. */
typedef struct {
	SI_32	PhaseDetector;				/* Phase contrast sensor */
	SI_32	FlyingSensor;				/* Laser sensor */
} TT_AfIfBufExternalDevice;

/* camera setting info. */
typedef struct {
	UI_08	muiAfCameraSceneMode;			/* Scene mode info. (Auto/Landscape/Portrait/Sport/etc.) */
	SI_32	muiAfTemperature;			/* Lens or system temperature */
	SI_32	muiAfTv;				/* Time Value */
	SI_32	muiAfAv;				/* Aperture Value */
	SI_32	muiAfSv;				/* Speed Value */
	SI_32	muiAfEv;				/* Exposure Value */
	UI_32	muiAfHistgram[256];			/* Luminance Histgram Data */
	SI_16	muiVdCount;				/* VD count */
	UI_32	muiAfFps;				/* FPS */
	UI_32	muiAfIso;				/* ISO */
} TT_AfIfBufCameraInformation;

/* AF evaluation value */
typedef struct {
	TT_AfIfBufEvaluationValue	mttLuminance;	/* AE evaluation value */
	TT_AfIfBufEvaluationValue	mttContL;	/* AF evaluation valueÅ@Contast Low */
	TT_AfIfBufEvaluationValue	mttContH;	/* AF evaluation valueÅ@Contast High */
} TT_AfIfBufAfEvaluationValue;

/* VCM adjustment value and compensation coefficient. */
typedef struct {
	TT_AfIfVcmRatio	mttVcmCalibrationRatio;		/* VCM compensation coefficient at near end. */
} TT_AfIfBufVcmCalibrationRatio;

typedef struct {
	SI_32	msiFixedFilter;				/* Filter setting for AF evaluation value */
	UI_08	Reserved[20];				/* Evaluation Function : tunable */
} TT_AfIfBufFilterControl;

typedef struct {
	SI_32	msiModuleCharactorStepSize;		/* VCM module character step size */
	UI_08	Reserved[20];				/* Step position Tunable */
} TT_AfIfStepSize;

/* Criteria for sensor ( Acc, Gyro, etc ) */
typedef struct {
	SI_32	msiFV;						/* Focus Value */
	SI_32	msiPS;						/* Liminance */
	SI_32	msiAccelerometer;				/* Accelerometer */
	SI_32	msiGyro;					/* Gyro */
	SI_32	msiTilt;					/* Tilt */
	SI_32	msiReserve;					/* reserved */
} TT_AfIfBufSensorInfoCriteria;

/* Criteria of AF mode */
typedef struct {
	TT_AfIfBufAfModeC	mttAfPeakSeach;			/* Criteria for Single AF mode */
	TT_AfIfBufAfModeC	mttAfContDirJdg;		/* Criteria for Continuous AF wobbling */
} TT_AfIfBufAfModeCriteria;

/* Criteria for Assist Light */
typedef struct {
	SI_32	muiAfAssistLightControl;			/* Criteria for Assist Light */
	SI_32	Reserved;					/* reserved */
} TT_AfIfBufAssistLightCriteria;

/* Log output */
typedef struct {
	UI_16	muiVdCount;						/* VD count */
	SI_16	muiFocusPosition;					/* Focus Pos */
	SI_32	muiTv;							/* Time Value */
	SI_32	muiAv;							/* Aperture Value */
	SI_32	muiSv;							/* Speed value */
	SI_32	muiEv;							/* Exposure value */
	UI_32	muiHistgram[256];					/* Luminance Histgram Data */
	TT_AfIfBufEvaluationValue	mttLuminance;			/* AE evaluation value */
	TT_AfIfBufEvaluationValue	mttContL;			/* AF evaluation value ( Contast Low ) */
	TT_AfIfBufEvaluationValue	mttContH;			/* AFevaluation value ( Contast High ) */
	UI_08	muiStatus;						/* AF Status */
	SI_32	marFilterLevelControl;
} TT_AfIfBufLogData;

/* AF control Buffer */
typedef struct {
	TT_AfIfBufVcmCalibration	mttVcmCalibration;		/* Adjustment data for VCM */
	TT_AfIfBufRoi	mttZoomRoi;					/* Digital zoom area information */
	TT_AfIfBufRoi	mttFocusRoi;					/* AF window information */
	TT_AfIfBufSensorInfo	mttSensorInfo;				/* System information such as Gyro, Acc etc. */
	TT_AfIfBufAfMode	mttAfAfMode;				/* AFmode */
	TT_AfIfBufWindow	mttAfWindow;				/* AF evaluation value Window setting */
	TT_AfIfBufAssistLight	mttAfAssistLight;			/* AF assiste light */
	TT_AfIfBufExternalDevice	mttAfExternalDevice;		/* External device info. */
	TT_AfIfBufCameraInformation	mttAfCameraInfo;		/* camera setting info. */
	TT_AfIfBufAfEvaluationValue	mttAfEvaluationValue;		/* AF evaluation value */
} TT_AfIfBufInfo;

/* Threshould value for AF control */
typedef struct {
	TT_AfIfBufVcmCalibrationRatio	mttVcmCalibrationRatio;			/* VCM adjustment value and compensation coefficient. */
	TT_AfIfBufFilterControl	mttFilterControl;				/* Filter setting */
	TT_AfIfStepSize	mttStepSize;						/* Focus Step Size setting */
	TT_AfIfBufSensorInfoCriteria	mttSensorInfoCriteria;			/* Criteria for sensor ( Acc, Gyro, etc ) */
	TT_AfIfBufAfModeCriteria	mttAfModeCriteria;			/* Criteria of AF mode */
	TT_AfIfBufAssistLightCriteria	msiAfAssistLightControlCriteria;	/* Criteria for Assist Light */
} TT_AfIfBufAdjust;

/* Log Data Buffer */
typedef struct {
	TT_AfIfBufLogData	mttLogData;						/* Log output */
} TT_AfIfBufLog;

/* Exif maker note Buffer */
typedef struct {
	SI_16	muiStepInformation;							/* Focus Pos */
	TT_AfIfBufVcmCalibration	mttVcmCalibration;				/* Adjustment value for VCM  */
	TT_AfIfBufVcmCalibrationRatio	mttVcmCalibrationRatio;				/* Compensation value for VCM */
	UI_08	muiAfMode;								/* Single / Continuos / AF Lock */
	TT_AfIfBufRoi	mttAfRoi;							/* AF window information */
	UI_08	muiAfStatus;
} TT_AfIfBufExifMakerNote;

/* VCM control Buffer */
typedef struct {
	SI_16	msiFocusPosition;							/* Focus Pos */
	SI_16	msiControlDelay;							/* Delay from VD */
	SI_32	muiVcmStepControl;							/* Step control for VCM ( depends on VCM Driver IC ) */
} TT_AfIfBufVcmDrive;

/* Static memory Area [5000Byte] */
//typedef struct {
//	UI_08	mttTunableItems[5000];						/* Static memory */
//} TT_AfIfBufIPStatic;

/* AF IP Static Buffer (Common) */
typedef struct {
	UI_32 af_current_data1[9] ; 	//AF Filter1 data
	UI_32 af_current_data2[9] ; 	//AF Filter2 data

	UI_16 af_current_position ;	//AF current position
	UI_16 af_start_position ;	//AF start position
	UI_16 af_end_position ;		//AF end position
	UI_16 af_far_position ;		//AF far side position
	UI_16 af_near_position ;	//AF near side position
	UI_16 af_inf_position ;		//INF af position
	UI_16 af_macro_position ;	//MAcro af position
	UI_16 af_2m_position ;		//2m af position
	UI_16 af_1m_position ;		//1m af position
	UI_16 af_50cm_position ;	//50cm af position
	UI_16 af_30cm_position ;	//30cm af position
	UI_16 af_20cm_position ;	//20cm af position
	UI_16 af_10cm_position ;	//10cm af position
	UI_16 af_8cm_position ;		//8cm af position
	UI_16 af_real_position ;	//
	UI_16 af_old_position ;		//
	
	UI_08 af_status ;		//Single AF status
	UI_08 caf_status ;		//Continuous AF status
	UI_08 af_result ;		//
	UI_32 just_data ;		//Just Position AF data
	
	UI_08 data_sel[10] ; 		//
	UI_08 quick_af_enable ;		//Quick AF enable
	UI_08 quick_af_flag ;		//Quick AF flag
	UI_08 saf_use_flag ;		//
	UI_08 saf_use_flag2 ;		//
	
	UI_16 init ; 			//
	UI_16 loop ; 			//
	UI_08 stable_cnt ; 		//
	
	UI_08 inf_macro_rev ; 		//
	
	UI_16 vd_cnt ;			//VD Count
	UI_16 vd_cnt_start ;		//Single AF start VD Count
	UI_16 vd_cnt_end ;		//Single AF end VD Count
	
	UI_16 vcm_slope ;		//Motor driver slppe control value
	UI_32 log_data[20] ;		//LOG AF data tes_kano_0821
	
	UI_16 defocus_status ;		//Defocus status tes_kano_0821
	UI_16 defocus_wait ; 		//Defocus wait count tes_kano_0821
	
	//UI_16 fd_af_start_cnt ;		//Face AF start count tes_kano_0825
	
	UI_16 af_finish ;		//AF finish triger
	UI_16 caf_finish ;		//CAF finish triger
	
	UI_08 hpf_sw ;			//HPF sw
	UI_08 ae_stabl ;		//AE Stable
	UI_08 ae_lock_request ;		//AE lock request
	UI_08 ae_unlock_request ;		//AE un-lock request
	
	UI_16 timer_cnt ;		//timer count
	UI_08 timer_reset ;		//timer reset
	
	UI_32 after_saf_af_data ;
	UI_32 after_saf_lumi_average ;
	UI_08 after_saf_use_data_flag ;
	
} TT_ACO_AF;

/* AF IP Static Buffer (Single AF) */
typedef struct {
	UI_16 af_def_position ; 	//AF default position
	UI_08 af_area_num ; 		//AF Area number
	UI_08 af_wait ; 		//AF Wait count
	UI_08 af_cnt ; 			//AF Scan count
	UI_08 af_num ; 			//AF Scan total count
	UI_08 af_step ; 		//AF step
	UI_32 af_data1[10][64] ; 	//AF data1 buffer
	UI_32 max_data1[10] ; 		//AF max data1 buffer
	UI_08 max_cnt1[10] ; 		//AF max count1 buffer
	UI_32 min_data1[10] ; 		//AF min data1 buffe
	UI_08 min_cnt1[10] ; 		//AF min count1 buffer (no-use)
	UI_16 just_pos_real ; 		//Just pint position
	UI_08 af_area_real ; 		//Just pint position area
	
	UI_32 af_data2[10][64] ; 	//AF data2 buffer
	UI_32 max_data2[10] ; 		//AF max data2 buffer
	UI_08 max_cnt2[10] ; 		//AF max count2 buffer
	UI_32 min_data2[10] ; 		//AF min data2 buffe
	UI_08 min_cnt2[10] ; 		//AF min count2 buffer (no-use)

//	UI_16 af_move_position ; 	//
//	UI_08 af_move_pos_flag ; 	//

//	UI_08 multi_pass_flag ; 	//
	UI_08 full_scan_flag ; 		//
	UI_08 sweep_flag ; 		//
	UI_08 vari_step_flag ; 		//
	UI_08 vari_step_cnt ; 		//
	UI_08 af_direction ; 		//AF Scan direction
} TT_SIN_AF;

/* AF IP Static Buffer (Continuous F) */
typedef struct {
	UI_16 caf_just_position ; 	//CAF just position
//	UI_08 caf_step_1 ;		//CAF step1
//	UI_08 caf_step_2 ;		//CAF step2
	UI_16 af_def_position ;		//CAF default position
//	UI_08 caf_total_wob_cnt ;	//
	UI_08 caf_df_cnt ;		//
	UI_32 caf_data_before ;		//Before CAF data
	UI_32 caf_data_old ;		//Old CAF data
	UI_08 caf_wait ;		//CAF wait count
	UI_08 caf_cnt ;			//
	UI_08 caf_start_cnt ;		//
	UI_08 caf_start_cnt_10v ;	//
	UI_08 caf_start_cnt_old ;	//
	UI_08 caf_start_cnt_old2 ;	//
	UI_08 caf_start_cnt_lum1 ;	//
	UI_08 caf_start_cnt_lum2 ;	//
	UI_08 caf_start_cnt_lum3 ;	//
	UI_08 caf_start_cnt_lum4 ;	//
	UI_16 caf_position[16] ;	//CAF position Buffer
	UI_32 caf_data[32] ;		//CAF data Buffer
//	UI_32 caf_data2[9][32] ;	//
//	UI_08 caf_data_slope[3] ;	//
//	UI_08 not_found_cnt ;		// 
//	UI_08 caf_inf_direction ;	//
	UI_08 caf_inf_direction2 ;	//INF Scan flag2
//	UI_08 caf_direction_flag ;	//
	UI_08 caf_macro_direction ;	//
	UI_16 caf_lumi_rate_before[9] ;	//r
	UI_16 caf_lumi_rate_old[9] ;	//
	UI_32 caf_lumi_average_before ;	//
	UI_08 caf_max_use ;		//MAX use flag
	UI_32 caf_max_data ;		//CAF max value
	UI_32 caf_min_data ;		//CAF min value
	UI_16 wob_step ;		//Wobling step
	UI_08 wob_forward_cnt ;		//Wobling forward count
	UI_08 wob_far_cnt ;		//Wobling far count
	UI_08 wob_near_cnt ;		//Wobling near count
	UI_08 wob_rev_cnt ;		//Wobling reverce count
	UI_08 big_change_flag ;		//big change flag
	UI_08 retry_flag ;		//retry flag
	UI_32 caf_start_data ;		//CAF start data
//	UI_08 caf_area_num ;		//
//	UI_32 caf_lumi_df_sum_before ;	//
//	UI_32 caf_lumi_df_sum_old ;	//
//	UI_08 caf_log_num ;		//LOG Number
//	UI_16 caf_log_position[2] ;	//LOG AF position
//	UI_32 caf_log_data[10] ;		//LOG AF data
	UI_16 caf_wob_start ;		//CAF start cause
	UI_16 caf_scn_chg[10] ;
	UI_08 zero_cnt ;		//zero count
	UI_08 ae_stabl_cnt ;		//AE stable count
} TT_CON_AF;

/* Host I/F data structure */
typedef struct {
	TT_AfIfBufInfo	mttInfo;			/* AF control Buffer */
	TT_AfIfBufAdjust	mttAdjust;		/* Threshould value for AF control */
	TT_AfIfBufLog	mttLog;				/* Log Data Buffer */
	TT_AfIfBufExifMakerNote	mttExif;	/* Exif maker note Buffer */
	TT_AfIfBufVcmDrive	mttVcmDrive;	/* VCM control Buffer */
//	TT_AfIfBufIPStatic	mttIPStatic;	/* Static memory Area [5000Byte] */
	TT_ACO_AF		mttAcoAf;	/* AF IP Static Buffer (Common) */
	TT_SIN_AF		mttSinAf;	/* AF IP Static Buffer (Single AF) */
	TT_CON_AF		mttConAf;	/* AF IP Static Buffer (Continuous F) */
} TT_AfIfBuf;

#endif // _AFIF_H
