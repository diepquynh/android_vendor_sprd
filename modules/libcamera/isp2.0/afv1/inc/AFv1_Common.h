/*
 *******************************************************************************
 * $Header$
 *
 *  Copyright (c) 2016-2025 Spreadtrum Inc. All rights reserved.
 *
 *  +-----------------------------------------------------------------+
 *  | THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY ONLY BE USED |
 *  | AND COPIED IN ACCORDANCE WITH THE TERMS AND CONDITIONS OF SUCH  |
 *  | A LICENSE AND WITH THE INCLUSION OF THE THIS COPY RIGHT NOTICE. |
 *  | THIS SOFTWARE OR ANY OTHER COPIES OF THIS SOFTWARE MAY NOT BE   |
 *  | PROVIDED OR OTHERWISE MADE AVAILABLE TO ANY OTHER PERSON. THE   |
 *  | OWNERSHIP AND TITLE OF THIS SOFTWARE IS NOT TRANSFERRED.        |
 *  |                                                                 |
 *  | THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT   |
 *  | ANY PRIOR NOTICE AND SHOULD NOT BE CONSTRUED AS A COMMITMENT BY |
 *  | SPREADTRUM INC.                                                 |
 *  +-----------------------------------------------------------------+
 *
 * $History$
 * 
 *******************************************************************************
 */

/*!
 *******************************************************************************
 * Copyright 2016-2025 Spreadtrum, Inc. All rights reserved.
 *
 * \file
 * AF_Common.h
 *
 * \brief
 * common data for AF
 *
 * \date
 * 2016/01/03
 *
 * \author
 * Galen Tsai
 *
 *
 *******************************************************************************
 */

#ifndef __AFV1_COMMON_H__
#define  __AFV1_COMMON_H__

//=========================================================================//
// Include header
//=========================================================================//
#include <stdio.h>
#include <memory.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "AFv1_Type.h"


//=========================================================================//
// Public Definition
//=========================================================================//
/* ============================================================================================== */
/*1.System info*/	
#define VERSION             "8.4"
#define STRING(s) #s


/* ============================================================================================== */
/*2.function error code*/	
#define ERR_SUCCESS         0x0000
#define ERR_FAIL            0x0001
#define ERR_UNKNOW          0x0002


#define AF_SEARCH_DEBUG     0

#define SAF_FINE_SEARCH     1   //0:without fine search, 1:with fine search

//#define AfDebugPrint( str, args... )    ( !AF_SEARCH_DEBUG ) ? : printf( str, ##args );

#ifdef WIN32
#define AfDebugPrint(x) do { if (AF_SEARCH_DEBUG) printf x; } while (0) 
#define Printf printf 
#else 
extern ERRCODE (*ANDROID_LOG)(const char* format, ...); 
#define AfDebugPrint(x) do { if (AF_SEARCH_DEBUG) ANDROID_LOG x; } while (0) 
#define Printf (*ANDROID_LOG) 

#endif

#define START_F(func)		    ERRCODE err = ERR_SUCCESS; \
							    AfDebugPrint(("(%s)line(%d)Enter %s!\n", __FILE__,__LINE__, func));

#define END_F(func)             AfDebugPrint(("(%s)line(%d)Exit %s! Ret = %d\n", __FILE__, __LINE__, func, err));\
                                    return err;
#define ERR_LOG(func)           {\
                                AfDebugPrint(("(%s)line(%d) %s Error Happen!!! Ret = %d\n", __FILE__, __LINE__, func, err));\
                                goto EXIT;}
#define AF_LOG(func, data)      AfDebugPrint(("(%s)line(%d) %s %s\n", __FILE__, __LINE__, func, data))
#define Debug_log_Filter T_SPSMD

#define TOTAL_POS 1024


#define NORMAL_START_IDX	ROUGH_SAMPLE_NUM_L1
#define MAX_SAMPLE_NUM	    25
#define TOTAL_SAMPLE_NUM	29

#define ROUGH_START_POS_L1  225
#define ROUGH_START_POS_L2  295
#define ROUGH_START_POS_L3  695
#define ROUGH_END_POS       895

#define ROUGH_SAMPLE_NUM_L1	4
#define ROUGH_SAMPLE_NUM_L2	16
#define ROUGH_SAMPLE_NUM_L3	9

#define ROUGH_SAMPLE_NUM	25         //MAX((ROUGH_SAMPLE_NUM_L3+ROUGH_SAMPLE_NUM_L2),(ROUGH_START_POS_L1+ROUGH_START_POS_L2))
#define FINE_SAMPLE_NUM		10
#define MAX_TIME_SAMPLE_NUM	100
#define FINE_INTERVAL		8
#define FINE_INIT_NUM		5

/*
#define NORMAL_START_IDX	5
#define MAX_SAMPLE_NUM	    25
#define TOTAL_SAMPLE_NUM	28
#define ROUGH_SAMPLE_NUM	25
#define FINE_SAMPLE_NUM		20
#define MAX_TIME_SAMPLE_NUM	100
#define FINE_INTERVAL		8
*/

#define TOTAL_AF_ZONE 9

#define MAX(a,b)  (((a) > (b)) ? (a) : (b))
#define MIN(a,b)  (((a) < (b)) ? (a) : (b))
  

//=========================================================================================//
// Public enum Instance
//=========================================================================================//
typedef enum _eAF_FILTER_TYPE
{
    T_SOBEL9				= 0,
	T_SOBEL5,
    T_SPSMD,	    		    
    T_FV0,					
    T_FV1,
    T_COV,
    T_TOTAL_FILTER_TYPE	    
        
}eAF_FILTER_TYPE;

typedef enum _eAF_OTP_TYPE
{
    T_LENS_BY_DEFAULT				= 0,
	T_LENS_BY_OTP,
    T_LENS_BY_TUNING,	    		         
}eAF_OTP_TYPE;

typedef enum _eAF_FV_RATIO_TYPE
{
    T_R_Total				= 0,
	T_R_J1,
    T_R_J2,	    		    
    T_R_J3,	
    T_R_Slope_Total,
    T_R_SAMPLE_NUM	    
        
}eAF_FV_RATIO_TYPE;


typedef enum _eAF_MODE {
  SAF = 0,               //single zone AF
  CAF,                   //continue AF    
  VAF,                   //Video CAF
  FAF,                   //Face AF
  MAF,                   //Multi zone AF 
  TMODE_1,               //Test mode 1
  Wait_Trigger           //wait for AF trigger
} eAF_MODE;

typedef enum _eAF_Triger_Type {
  RF_NORMAL = 0,        //noraml R/F search for AFT
  R_NORMAL,             //noraml Rough search for AFT
  F_NORMAL,             //noraml Fine search for AFT
  RF_FAST,              //Fast R/F search for AFT
  R_FAST,              //Fast Rough search for AFT
  F_FAST,              //Fast Fine search for AFT
  DEFOCUS,
} eAF_Triger_Type;

typedef enum _eSAF_Status {
  SAF_Status_Start = 0,
  SAF_Status_Init,
  SAF_Status_RSearch,
  SAF_Status_FSearch,
  SAF_Status_Move_Lens,
  SAF_Status_Stop  
} eSAF_Status;    

typedef enum _eCAF_Status {
  CAF_Status_Start = 0,
  CAF_Status_Init,
  CAF_Status_RSearch,
  CAF_Status_FSearch,
  CAF_Status_Move_Lens,
  CAF_Status_Stop  
} eCAF_Status;    

typedef enum _eSAF_Main_Process {
  SAF_Main_INIT = 0,
  SAF_Main_SEARCH,
  SAF_Main_DONE
} eSAF_Main_Process;  

typedef enum _eCAF_Main_Process {
  CAF_Main_INIT = 0,
  CAF_Main_SEARCH,
  CAF_Main_DONE
} eCAF_Main_Process;

typedef enum _eSAF_Search_Process {
  SAF_Search_INIT               = 0,  
  SAF_Search_SET_ROUGH_SEARCH,
  SAF_Search_ROUGH_SEARCH,
  SAF_Search_ROUGH_HAVE_PEAK,
  SAF_Search_SET_FINE_SEARCH,
  SAF_Search_FINE_SEARCH,
  SAF_Search_FINE_HAVE_PEAK,
  SAF_Search_NO_PEAK,
  SAF_Search_DONE,
  SAF_Search_DONE_BY_UI,
  SAF_Search_TOTAL,
  
} eSAF_Search_Process;    

typedef enum _eCAF_Search_Process {
  CAF_Search_INIT               = 0,  
  CAF_Search_SET_ROUGH_SEARCH,
  CAF_Search_ROUGH_SEARCH,
  CAF_Search_ROUGH_HAVE_PEAK,
  CAF_Search_SET_FINE_SEARCH,
  CAF_Search_FINE_SEARCH,
  CAF_Search_FINE_HAVE_PEAK,
  CAF_Search_NO_PEAK,
  CAF_Search_DONE,
  CAF_Search_DONE_BY_UI,
  CAF_Search_TOTAL,
  
} eCAF_Search_Process;

typedef enum _eAF_Result {
  AF_INIT      = 0,
  AF_FIND_HC_PEAK,         //high confidence peak
  AF_FIND_LC_PEAK,         //low confidence peak
  AF_NO_PEAK,
  AF_TOO_FAR,
  AF_TOO_NEAR,
  AF_REVERSE,
  AF_SKY,
  AF_UNKNOW  
} eAF_Result;  

typedef enum _e_LOCK {
  LOCK      = 0,
  UNLOCK,         
} e_LOCK;  

typedef enum _e_DIR {
  FAR2NEAR      = 0,
  NEAR2FAR,         
} e_DIR;  

typedef enum _e_RESULT {
  NO_PEAK      = 0,
  HAVE_PEAK,         
} e_RESULT;  

typedef enum _e_AF_TRIGGER {
  NO_TRIGGER      = 0,
  AF_TRIGGER,         
} e_AF_TRIGGER;  

typedef enum _e_AF_BOOL {
  AF_FALSE      = 0,
  AF_TRUE,         
} e_AF_BOOL;  

typedef enum _e_AF_AE_Gain {
  AE_1X      = 0,
  AE_2X,
  AE_4X,
  AE_8X,
  AE_16X,
  AE_32X,
  AE_Gain_Total,
} e_AF_AE_Gain; 

//=========================================================================================//
// Public Structure Instance
//=========================================================================================//
//#pragma pack(push, 1)

typedef struct _AE_Report
{
	uint8   bAEisConverge;//flag: check AE is converged or not
    int16   AE_BV;        //brightness value
    uint16  AE_EXP;       //exposure time (ms)
    uint16  AE_Gain;      //X128: gain1x = 128
    uint32  AE_Pixel_Sum; //AE pixel sum which needs to match AF blcok 
    uint16  AE_Idx;       //AE exposure level
} AE_Report;

typedef struct _AF_FV_DATA
{
    uint8   MaxIdx[MAX_SAMPLE_NUM];         //index of max statistic value
    uint8   MinIdx[MAX_SAMPLE_NUM];         //index of min statistic value

    uint8   BPMinIdx[MAX_SAMPLE_NUM];       //index of min statistic value before peak
    uint8   APMinIdx[MAX_SAMPLE_NUM];       //index of min statistic value after peak
    
    uint8   ICBP[MAX_SAMPLE_NUM];           //increase count before peak 
    uint8   DCBP[MAX_SAMPLE_NUM];           //decrease count before peak
    uint8   EqBP[MAX_SAMPLE_NUM];           //equal count before peak  
    uint8   ICAP[MAX_SAMPLE_NUM];           //increase count after peak
    uint8   DCAP[MAX_SAMPLE_NUM];           //decrease count after peak
    uint8   EqAP[MAX_SAMPLE_NUM];           //equal count after peak    
    int8    BPC[MAX_SAMPLE_NUM];            //ICBP - DCBP  
    int8    APC[MAX_SAMPLE_NUM];            //ICAP - DCAP
    
    uint64  Val[MAX_SAMPLE_NUM];            //statistic value
    uint64  Start_Val;                      //statistic value of start position
    
    uint16  BP_R10000[T_R_SAMPLE_NUM][MAX_SAMPLE_NUM];    //FV ratio X 10000 before peak   
    uint16  AP_R10000[T_R_SAMPLE_NUM][MAX_SAMPLE_NUM];    //FV ratio X 10000 after peak

    uint8   Search_Result[MAX_SAMPLE_NUM];  //search result of focus data
    uint16  peak_pos[MAX_SAMPLE_NUM];       //peak position of focus data
    uint64  PredictPeakFV[MAX_SAMPLE_NUM];            //statistic value
    
}AF_FV_DATA;

typedef struct _AF_FV
{
    AF_FV_DATA   Filter[T_TOTAL_FILTER_TYPE];  
    AE_Report    AE_Rpt[MAX_SAMPLE_NUM];
   

}AF_FV;
#pragma pack(push,1)
typedef struct _AF_FILTER_TH
{
    uint16   UB_Ratio_TH[T_R_SAMPLE_NUM];   //The Up bound threshold of FV ratio, [0]:total, [1]:3 sample, [2]:5 sample, [3]:7 sample
    uint16   LB_Ratio_TH[T_R_SAMPLE_NUM];   //The low bound threshold of FV ratio, [0]:total, [1]:3 sample, [2]:5 sample, [3]:7 sample
    uint32   MIN_FV_TH;                     //The threshold of FV to check the real curve
}AF_FILTER_TH;

typedef struct _AF_TH
{
    AF_FILTER_TH   Filter[T_TOTAL_FILTER_TYPE];          

}AF_TH;
#pragma pack(pop)

typedef struct _SAF_SearchData
{
    uint8  SAF_RS_TotalFrame;               //total work frames during rough search
    uint8  SAF_FS_TotalFrame;               //total work frames during fine search

    uint8  SAF_RS_LensMoveCnt;              //total lens execute frame during rough search
    uint8  SAF_FS_LensMoveCnt;              //total lens execute frame during fine search

    uint8  SAF_RS_StatisticCnt;             //total statistic frame during rough search
    uint8  SAF_FS_StatisticCnt;             //total statistic frame during fine search

    uint8  SAF_RS_MaxSearchTableNum;        //maxima number of search table during rough search
    uint8  SAF_FS_MaxSearchTableNum;        //maxima number of search table during fine search

    uint8  SAF_RS_DIR;                      //direction of rough search:Far to near or near to far
    uint8  SAF_FS_DIR;                      //direction of fine search:Far to near or near to far

    uint8  SAF_Skip_Frame;                  //skip this frame or not

    uint16  SAF_RSearchTable[ROUGH_SAMPLE_NUM];
    uint16  SAF_RSearchTableByTuning[TOTAL_SAMPLE_NUM];
    uint16  SAF_FSearchTable[FINE_SAMPLE_NUM];

    AF_FV   SAF_RFV;                               //The FV data of rough search
    AF_FV   SAF_FFV;                               //The FV data of fine search
   
    AF_FV   SAF_Pre_RFV;                           //The last time FV data of rough search
    AF_FV   SAF_Pre_FFV;                           //The last time FV data of fine search

    AF_TH   SAF_RS_TH;                             //The threshold of rough search
    AF_TH   SAF_FS_TH;                             //The threshold of fine search

        
}SAF_SearchData;

typedef struct _CAF_SearchData
{  
    uint8  CAF_RS_TotalFrame;
    uint8  CAF_FS_TotalFrame;

    uint16  CAF_RSearchTable[ROUGH_SAMPLE_NUM];
    uint16  CAF_FSearchTable[FINE_SAMPLE_NUM];
    
    AF_FV   CAF_RFV;  
    AF_FV   CAF_FFV;
   
    AF_FV   CAF_Pre_RFV;  
    AF_FV   CAF_Pre_FFV;
    
}CAF_SearchData;

#pragma pack(push,1)
typedef struct _AF_Scan_Table
{  
    //Rough search
    uint16 POS_L1;
    uint16 POS_L2;
    uint16 POS_L3;
    uint16 POS_L4;

    uint16 Sample_num_L1_L2;
    uint16 Sample_num_L2_L3;
    uint16 Sample_num_L3_L4;

    uint16 Normal_Start_Idx;
    uint16 Rough_Sample_Num;


    //Fine search
    uint16 Fine_Sample_Num;
    uint16 Fine_Search_Interval;
    uint16 Fine_Init_Num;
    
}AF_Scan_Table;


typedef struct _AF_Tuning_Para
{
    //AF Scan Table
    AF_Scan_Table Scan_Table_Para[AE_Gain_Total];

    //AF Threshold
    AF_TH   SAF_RS_TH[AE_Gain_Total];      //The threshold of rough search
    AF_TH   SAF_FS_TH[AE_Gain_Total];      //The threshold of fine search

    int dummy[200];
}AF_Tuning_Para;

typedef struct _Lens_Info
{
    //Lens Info
    uint16 Lens_MC_MIN;     //minimal mechenical position 
    uint16 Lens_MC_MAX;     //maximal mechenical position 
    uint16 Lens_INF;        //INF position 
    uint16 Lens_MACRO;      //MACRO position
    uint16 Lens_Hyper;      //Hyper Focus position
    uint16 One_Frame_Max_Move;  //skip one frame if move position is bigger than TH

}Lens_Info;

typedef struct _AF_Tuning
{
    //Lens Info
    Lens_Info Lens_Para;
   
    AF_Tuning_Para SAFTuningPara;   //SAF parameters
    AF_Tuning_Para CAFTuningPara;   //CAF parameters
    AF_Tuning_Para VCAFTuningPara;  //Video CAF parameters
    
    uint16 dummy[200];
}AF_Tuning;
#pragma pack(pop)

typedef struct _CAF_Tuning_Para
{ 
    int dummy;
}CAF_Tuning_Para;

typedef struct _SAF_INFO
{
   eAF_Triger_Type Cur_AFT_Type;                    //the search method
   uint8 SAF_Main_Process;                          //the process state of SAF main
   uint8 SAF_Search_Process;                        //the process state of SAF search
   uint8 SAF_Status;
   uint8 SAF_RResult;                               //rough search result
   uint8 SAF_FResult;                               //fine search result
   uint8 SAF_Total_work_frame;                      //total work frames during SAF work
   uint8 SAF_AE_Gain;                               //AE gain
   uint16 SAF_Start_POS;                            //focus position before AF work
   uint16 SAF_Cur_POS;                              //current focus positon
   uint16 SAF_Final_POS;                            //final move positon
   uint16 SAF_Final_VCM_POS;                        //final move positon load from VCM
   uint16 SAF_RPeak_POS;                            //Peak positon of rough search
   uint16 SAF_FPeak_POS;                            //Peak positon of fine search
   uint64 SAF_SYS_TIME_ENTER[MAX_TIME_SAMPLE_NUM];  //save each time while entering SAF search 
   uint64 SAF_SYS_TIME_EXIT[MAX_TIME_SAMPLE_NUM];   //save each time while entering SAF search 
   Lens_Info Lens_Para;                             //current lens parameters
   AF_Scan_Table SAF_Scan_Table_Para;               //current scan table parameters
}SAF_INFO;

typedef struct _CAF_INFO
{
   uint8 CAF_mode;
   
    
}CAF_INFO;

typedef struct _SAF_Data
{
    SAF_INFO sAFInfo;
    SAF_SearchData sAFSearchData;
      
}SAF_Data;

typedef struct _CAF_Data
{
    CAF_INFO cAFInfo;
    CAF_SearchData cAFSearchData;
    CAF_Tuning_Para cAFTuningPara;
    
    
}CAF_Data;



typedef struct _AF_OTP_Data
{
    uint8 bIsExist;
    uint16 INF;
    uint16 MACRO;
    
} AF_OTP_Data;


typedef struct _AF_Ctrl_Ops
{
	ERRCODE (*statistics_wait_cal_done)(void *cookie);
	ERRCODE (*statistics_get_data)(uint64 fv[T_TOTAL_FILTER_TYPE], void *cookie);
	ERRCODE (*lens_get_pos)(uint16 *pos, void *cookie);
	ERRCODE (*lens_move_to)(uint16 pos, void *cookie);
	ERRCODE (*lens_wait_stop)(void *cookie);
    ERRCODE (*lock_ae)(e_LOCK bisLock, void *cookie);
    ERRCODE (*lock_awb)(e_LOCK bisLock,void *cookie);
    ERRCODE (*lock_lsc)(e_LOCK bisLock,void *cookie);
    ERRCODE (*get_sys_time)(uint64* pTime,void *cookie);
    ERRCODE (*get_ae_report)(AE_Report* pAE_rpt, void *cookie);
    ERRCODE (*set_af_exif)(const void* pAF_data,void *cookie);
    ERRCODE (*sys_sleep_time)(uint16 sleep_time,void *cookie);
    ERRCODE (*get_otp_data)(AF_OTP_Data* pAF_OTP, void *cookie);
    ERRCODE (*get_motor_pos)(uint16 *motor_pos, void *cookie);
    ERRCODE (*set_motor_sacmode)(void *cookie);
    ERRCODE (*binfile_is_exist)(uint8* bisExist, void *cookie);
    ERRCODE (*af_log)(const char* format, ...);
    ERRCODE (*af_start_notify)(eAF_MODE  AF_mode, void *cookie);
    ERRCODE (*af_end_notify)(eAF_MODE  AF_mode, void *cookie);
    


	void *cookie;
} AF_Ctrl_Ops;

typedef struct _AF_Trigger_Data
{
    uint8 bisTrigger;
    eAF_Triger_Type AF_Trigger_Type;
    eAF_MODE  AFT_mode;
    
} AF_Trigger_Data;


typedef struct _AF_Win
{
    uint16  Set_Zone_Num;               //FV zone number
    uint16  AF_Win_X[TOTAL_AF_ZONE];    
    uint16  AF_Win_Y[TOTAL_AF_ZONE];
    uint16  AF_Win_W[TOTAL_AF_ZONE];
    uint16  AF_Win_H[TOTAL_AF_ZONE];   
    
}AF_Win;

typedef struct _AF_Data
{
    eAF_MODE  AF_mode;
    eAF_MODE  Pre_AF_mode;
    AF_Trigger_Data AFT_Data;
    SAF_Data sAF_Data;
    CAF_Data cAF_Data;   
    AE_Report   AE_Rpt[MAX_TIME_SAMPLE_NUM];
    AF_OTP_Data AF_OTP;
    AF_Win AF_Win_Data;
    uint32 vcm_register;
    int8 AF_Version[10];
    AF_Tuning AF_Tuning_Data;
	unsigned int dump_log;
    AF_Ctrl_Ops AF_Ops;
}AF_Data;



//#pragma pack(pop)


#endif
