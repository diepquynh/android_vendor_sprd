/* --------------------------------------------------------------------------
 * Confidential
 *                    AcuteLogic Technologies Inc.
 *
 *                   Copyright (c) 2015 ALC Limited
 *                          All rights reserved
 *
 * Project  : SPRD_AF
 * Filename : ALC_AF_Data.h
 *
 * $Revision: v1.00
 * $Author 	:  
 * $Date 	: 2015.02.11
 *
 * --------------------------------------------------------------------------
 * Feature 	: DW9804(VCM IC)
 * Note 	: vim(ts=4, sw=4)
 * ------------------------------------------------------------------------ */
#ifndef	__ALC_AF_DATA_H__
#define	__ALC_AF_DATA_H__

#include "ALC_AF_Common.h"

/* ------------------------------------------------------------------------ */
//#define FILTER_SPRD
//#define FILTER_SPRD //for test

#define		ALC_LOG_ON_MAIN
#define		ALC_LOG_ON_INTERFACE

/* ------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------ */

// Step
#define 	MAX_FIRST_STEP					30
#define 	LAST_FIRST_STEP					MAX_FIRST_STEP - 1
#define 	MAX_LOOKUP_STEP					LAST_FIRST_STEP
#define 	MAX_FINE_STEP					15
#define 	LAST_FINE_STEP					MAX_FINE_STEP - 1
#define 	CAF_MAX_BUFF					30


#define 	ARRAY_LOW						0
#define 	ARRAY_MID						1
#define 	ARRAY_HIGH						2

// Rect Define
#define 	RECT_LEFT						0
#define 	RECT_TOP						1
#define 	RECT_RIGHT						2
#define 	RECT_BOTTOM						3

// Scene
#define 	OUT_SCENE						0
#define 	IND_SCENE						1
#define 	DAK_SCENE						2
#define 	SCENE_NUM						3

// PreFrameSkip
#define 	FINESCAN						0
#define 	FIRSTSCAN						1

// FAil
#define 	FAIL_MACRO						0
#define 	FAIL_DY							1

// Filter Type
#define 	AFILTER1						0
#define 	AFILTER2						1

// Region
#define 	AXIS_Y							0
#define 	AXIS_X							1

// NoCnt
#define 	NO_CNT_UP						0
#define 	NO_CNT_DN						1

// Fine Scan
#define 	MIN_FINESCAN_LMT				0
#define 	MAX_FINESCAN_LMT				1
#define 	RIGHT_FINESCAN					0
#define 	LEFT_FINESCAN					1

#define 	FIRST_MIN_STEP					0
#define 	FIRST_MAX_STEP					1
#define 	FIRST_SIZE_STEP					2

#define 	AE_LOCK							1
#define 	AWB_LOCK						2

#define 	FALSE							0
#define 	TRUE							1
//////////
#define 	DN_COUNT						0
#define 	UP_COUNT						1
#define 	PEAK_STEP						2
#define 	MIN_STEP						3
#define 	MAX_STEP						4
#define 	PRE_MAX_STEP					5

#define 	FAST_SEARCH						0	 
#define 	FAST_STOP						1	 
#define 	FAST_LIMIT						2	 

#define 	REGION1							0
#define 	REGION2							1
#define 	REGION3							2
#define 	REGION4							3
#define 	REGION5							4
#define 	REGION6							5
#define 	REGION7							6
#define 	REGION_NUM						7
#define 	REGION_FINE						REGION_NUM

#define 	REGION0_ISP						0
#define 	REGION1_ISP						1
#define 	REGION2_ISP						2
#define 	REGION3_ISP						3
#define 	REGION4_ISP						4
#define 	REGION5_ISP						5
#define 	REGION6_ISP						6
#define 	REGION7_ISP						7
#define 	REGION8_ISP						8
#define 	REGION9_ISP						9
#define 	REGION_NUM_ISP 					10

#define 	NORMAL_MODE						0
#define 	TOUCH_MODE						1

#define		CAF_SPD_0						0
#define		CAF_SPD_1						1
#define		CAF_SPD_2						2
#define		CAF_SPD_3						3
#define		CAF_SPD_4						4

// Motor Control
#define 	MOTOR_READ						0
#define 	MOTOR_WRITE						1

// bSetFilter
#define 	FILTER_SEL_FILTER2   			0x80 // 0 : SPSMD, 1 : SOBEL 
#define 	FILTER_SEL_FILTER1				0x40 // 0 : SPSMD, 1 : SOBEL 
#define 	FILTER_SOBEL_SIZE      			0x20 // 1 : Enable 
#define		FILTER_SPSMD_CAL_MODE			0x10 // 0 : shift, 1 : square
#define 	FILTER_SPSMD_DIAGONAL			0x08 // 1 : Enable
#define 	FILTER_SPSMD_RGT_BOT			0x04 // 1 : Enable 
#define 	FILTER_SPSMD_TYPE 				0x03 // 0 : avg ,1 : cen , 2: med 

//////////////// Control Register //////////////
//bCtl[0]								   
#define 	ENB0_MOTOR_TYPE					0xF0  
#define 	ENB0_AE_AWB_STOP				0x08 
#define 	ENB0_SAF						0x04 
#define 	ENB0_USE_OTP					0x02 
#define 	ENB0_AF							0x01 

#define 	ENB0_MOTRO_TYPE_MASK			0xF0 

//bCtl[1] 
#define 	ENB1_FAST_SCAN_FINE				0x80 
#define 	ENB1_MACRO_MODE					0x40 
#define 	ENB1_CAF_FINE_SCAN_RECORD		0x20 
#define 	ENB1_CAF_FINE_SCAN				0x10 
#define 	ENB1_FDAF						0x08 
#define  	ENB1_CAF						0x04 
#define 	ENB1_TOUCH						0x02 
#define 	ENB1_AF_START					0x01 

#define 	ENB1_CLEAR_CANCEL				0xFC 
#define 	ENB1_TOUCH_START				0x03

//cnt[2]
#define 	ENB2_FAST_SCAN_FIRST			0x80 
#define 	ENB2_MTINIT_FIRST				0x40 
#define 	ENB2_CANCEL						0x20 
#define 	ENB2_MOTOR_MOVE_FIRST			0x10 
#define 	ENB2_AUTO_PRE_FRAME_SKIP_FIRST	0x08 
#define 	ENB2_AUTO_PRE_FRAME_SKIP_FINE	0x04 
#define 	ENB2_SKIP_FIRST_STEP_FIRST		0x02 
#define 	ENB2_SKIP_FIRST_STEP_FINE		0x01 

#define 	ENB_FAST_SCAN					0x80 
                            	                
//cnt[3]
#define 	ENB3_PRIORITY_FILTER2			0x80  
#define 	ENB3_CALC_FV_OPT				0x40
#define 	ENB3_CALC_FV					0x20 	
#define 	ENB3_REV_SCAN_FINE				0x10  
#define 	ENB3_NO_CHECK_AELOCK			0x08  
#define 	ENB3_NOT_WAIT_AE_LOCK_STATE		0x04  
#define 	ENB3_NO_CHECK_AWBLOCK			0x02  
#define 	ENB3_NOT_WAIT_AWB_LOCK_STATE	0x01  

//cnt[4]                      	
#define 	ENB4_MANUAL_STEP_SIZE_FINE		0x80  
#define 	ENB4_SEARCH_WEIGHT_FINE			0x40 	
#define 	ENB4_AUTO_SLEW_RATE				0x20  
#define 	ENB4_MOTOR_MANUAL				0x10  
#define 	ENB4_LOOKUP_TABLE_STEP			0x08  
#define 	ENB4_MANUAL_SCENE				0x04  
#define 	ENB4_SEL_SCENE					0x03 	
                            			
//cnt[5]                      			
#define 	ENB5_CALC_LCD_SIZE				0x80  
#define 	ENB5_SEARCH_WEIGHT_FIRST		0x40  
#define 	ENB5_SEARCH_OUT					0x20 	
#define 	ENB5_AGING_TEST					0x10 	
#define 	ENB5_ZERO_D2_BY_D1_FIRST		0x08 	
#define 	ENB5_OFFSET						0x04 	
#define 	ENB5_SEARCH_OUT_MACRO			0x02 	
#define 	ENB5_IGNORE_ZERO_STEP_FIRST		0x01 	

#define 	ENB_SEARCH_WEIGHT				0x40

//cnt[6]
#define 	ENB6_READ_MTVALUE				0x80  
#define 	ENB6_SEARCH_OUT_WO_AELOCK		0x40  
#define 	ENB6_ADT_FPS_AF 				0x20 	
#define 	ENB6_ADT_FPS_SIZE				0x10 	
#define 	ENB6_IGNORE_ZERO_STEP_FINE		0x08 
#define 	ENB6_SOFTLANDING				0x04 	
#define 	ENB6_SOFT_FINISH 				0x02 	
#define 	ENB6_AUTO_REGION 				0x01 	

//cnt[7]
//#define 	ENB7_							0x80  
//#define 	ENB7_							0x40  
//#define 	ENB7_FINE_SCAN_DAK				0x20 	
//#define 	ENB7_FINE_SCAN_IND				0x10 	
#define 	ENB7_FINE_SCAN					0x08 	
#define 	ENB7_WRITE_AF_DEBUG				0x04 	
#define 	ENB7_DEBUG_AF_ORG				0x02 	
#define 	ENB7_DEBUG_REG_READ				0x01 

#define 	ENB7_NORMAL_FUNC_LOCK			0x1C 

//cnt[8]
//#define 	ENB8_							0x80  
//#define 	ENB8_							0x40  
//#define 	ENB8_							0x20 	
//#define 	ENB8_							0x10 	
//#define 	ENB8_							0x08 	
//#define 	ENB8_							0x04 	
//#define 	ENB8_							0x02 	
#define 	ENB8_FD_CENTER_FIRST			0x01 

//cnt[9]
#define 	ENB9_CAF_STABLE_FV				0x80  
#define 	ENB9_CAF_CHECK_REAL_DIFF		0x40  
#define 	ENB9_CAF_LOOKUP_TABLE_STEP		0x20 	
#define 	ENB9_CAF_DOUBLE_MOTOR_POS		0x10 	
#define 	ENB9_CAF_DIV_DIRECTION3			0x08	
#define 	ENB9_CAF_DIV_DIRECTION2			0x04	
#define 	ENB9_CAF_DIV_DIRECTION1			0x02	
#define 	ENB9_CAF_DIV_DIRECTION0			0x01	

// Region Enable
#define 	ENABLE_REGION_0					0x0001
#define 	ENABLE_REGION_1					0x0002
#define 	ENABLE_REGION_2					0x0004
#define 	ENABLE_REGION_3					0x0008
#define 	ENABLE_REGION_4					0x0010
#define 	ENABLE_REGION_5					0x0020
#define 	ENABLE_REGION_6					0x0040
#define 	ENABLE_REGION_7					0x0080
#define 	ENABLE_REGION_8					0x0100
#define 	ENABLE_REGION_9					0x0200


#define 	SEL_VCM_DRIVER_DW9807			0



//////////////////// bVCM_Ctl //////////////////
// bVCM_Ctl[0]
#define 	VCM_CTL0_RING1					0x80
#define 	VCM_CTL0_RING0					0x40
//#define 	VCM_CTL0_						0x20								 
//#define 	VCM_CTL0_						0x10
//#define 	VCM_CTL0_						0x08
//#define 	VCM_CTL0_						0x04
#define 	VCM_CTL0_RING_EN 				0x02
#define 	VCM_CTL0_RESET_EN 				0x01

// bVCM_Ctl[1]
#define 	VCM_CTL1_PS1  					0x80
#define 	VCM_CTL1_PS0					0x40
#define		VCM_CTL1_RC5					0x20
#define		VCM_CTL1_RC4					0x10
#define		VCM_CTL1_RC3					0x08
#define		VCM_CTL1_RC2					0x04
#define 	VCM_CTL1_RC1					0x02
#define 	VCM_CTL1_RC0 					0x01
#define 	VCM_CTL1_PS_MASK 				0xC0	
#define 	VCM_CTL1_RC_MASK 				0x3F

// bVCM_Ctl[2]
//#define 	VCM_CTL2_  						0x80
//#define 	VCM_CTL2_ 						0x40
//#define 	VCM_CTL2_						0x20
//#define 	VCM_CTL2_						0x10
//#define 	VCM_CTL2_						0x08
//#define 	VCM_CTL2_						0x04
//#define 	VCM_CTL2_						0x02
//#define 	VCM_CTL2_ 						0x01

// bVCM_Ctl[3]
//#define 	VCM_CTL3_  						0x80
//#define 	VCM_CTL3_ 						0x40
//#define 	VCM_CTL3_						0x20
//#define 	VCM_CTL3_						0x10
//#define 	VCM_CTL3_						0x08
//#define 	VCM_CTL3_						0x04
//#define 	VCM_CTL3_						0x02
//#define 	VCM_CTL3_ 						0x01

////////////////////Read Only///////////////////
// bProcess       		
//#define 	PROCESS_						0x80 	
//#define 	PROCESS_FD_AF					0x40  
//#define 	PROCESS_RECORD_TOUCH			0x20  
//#define 	PROCESS_RECORD					0x10  
//#define 	PROCESS_CAF						0x08  
//#define 	PROCESS_TOUCH					0x04  
//#define 	PROCESS_SAF						0x03  


// bStatus0[0]
#define 	FLAG0_AE_AWB_STOP				0x80 
#define 	FLAG0_SAF_START_MOVE			0x40 
#define 	FLAG0_FIRST_SCANNING			0x20 
#define 	FLAG0_FINE_SCANNING				0x10 
//#define 	FLAG0_							0x08 
//#define 	FLAG0_							0x04 		
#define 	FLAG0_AF_FINISHED				0x02 
#define 	FLAG0_AF_FAIL					0x01 

#define 	FLAG0_AGING_OVER				0x8F
#define 	FLAG0_CLEAR_OVER				0x83
#define 	FLAG0_FUNC_STOP					0xC0
                        			
// bStatus1[1]       		
#define 	FLAG1_OVER_DOWN					0x80 	
#define 	FLAG1_STEP_MAX					0x40  
#define 	FLAG1_AF_MACRO_FAIL				0x20  
#define 	FLAG1_AF_FV_DY_FAIL				0x10  
#define 	FLAG1_AF_YMEAN_FAIL				0x08  
#define 	FLAG1_LOCK_FAIL					0x04  
#define 	FLAG1_SCENE						0x03  
                            		
// bStatus2[2]          	
#define 	FLAG2_USE_FINE_DATA1			0x80  
#define 	FLAG2_USE_R6_DATA1   			0x40  
#define 	FLAG2_USE_R5_DATA1   			0x20  
#define 	FLAG2_USE_R4_DATA1   			0x10  
#define 	FLAG2_USE_R3_DATA1   			0x08  
#define 	FLAG2_USE_R2_DATA1   			0x04  
#define 	FLAG2_USE_R1_DATA1   			0x02  
#define 	FLAG2_USE_R0_DATA1   			0x01  
                            	             
// bStatus[3]            	             
#define 	FLAG3_USE_FINE_DATA2			0x80  
#define 	FLAG3_USE_R6_DATA2  			0x40  
#define 	FLAG3_USE_R5_DATA2  			0x20  
#define 	FLAG3_USE_R4_DATA2  			0x10  
#define 	FLAG3_USE_R3_DATA2  			0x08  
#define 	FLAG3_USE_R2_DATA2  			0x04  
#define 	FLAG3_USE_R1_DATA2  			0x02  
#define 	FLAG3_USE_R0_DATA2  			0x01  

// bStatus[4]            	
//#define 	FLAG4_							0x80  
#define 	FLAG4_STOP_FASTSCAN_FINE		0x40  
#define 	FLAG4_INIT_POS_MOTOR			0x20  
//#define 	FLAG4_		   					0x10  
#define 	FLAG4_AE_LOCK  					0x08 	
//#define 	FLAG4_					  		0x04  
//#define 	FLAG4_					  		0x02  
//#define 	FLAG4_   						0x01  
                            	             
// bStatus[5]            	             
#define 	FLAG5_AUTO_CANCEL				0x80  
#define 	FLAG5_STOP_FASTSCAN_FIRST		0x40  
//#define 	FLAG5_ 							0x20  
//#define 	FLAG5_							0x10  
//#define 	FLAG5_  						0x08  
#define 	FLAG5_MOTOR_MOVING  			0x04  
#define 	FLAG5_TOUCH_AE_PASS 			0x03  

#define 	FLAG_STOP_FASTSCAN				0x40

// bStatus[6]            	             
//#define 	FLAG5_ 						  	0x80  
//#define 	FLAG5_ 						 	0x40  
//#define 	FLAG5_ 							0x20  
//#define 	FLAG5_							0x10  
//#define 	FLAG5_  						0x08  
//#define 	FLAG5_  						0x04  
//#define 	FLAG5_ 							0x02  
//#define 	FLAG5_ 							0x01  


///////////////////////////////////////////////////////


// bMaxFvFlag    [0]            	
//#define 	Reserved 						0x80
#define 	FLAG_MAX_R6_DATA1 	  			0x40 
#define 	FLAG_MAX_R5_DATA1 	  			0x20 
#define 	FLAG_MAX_R4_DATA1   			0x10 
#define 	FLAG_MAX_R3_DATA1   			0x08 
#define 	FLAG_MAX_R2_DATA1   			0x04 
#define 	FLAG_MAX_R1_DATA1   			0x02 
#define 	FLAG_MAX_R0_DATA1   			0x01 
                            	            
// bMaxFvFlag    [1]            	            
//#define 	Reserved 						0x80
#define 	FLAG_MAX_R6_DATA2  				0x40 
#define 	FLAG_MAX_R5_DATA2  				0x20 
#define 	FLAG_MAX_R4_DATA2  				0x10 
#define 	FLAG_MAX_R3_DATA2  				0x08 
#define 	FLAG_MAX_R2_DATA2  				0x04 
#define 	FLAG_MAX_R1_DATA2  				0x02 
#define 	FLAG_MAX_R0_DATA2  				0x01 

////////////////////////////////////////////////////

// State Struct
#define 	STATE_FAIL_RETURN				0x80
#define 	STATE_OVER						0x40
#define 	STATE_FINESCAN					0x20
#define 	STATE_FINE_PREFRAMESKIP			0x10
#define 	STATE_FINESCAN_INIT			   	0x08
#define 	STATE_FIRSTSCAN				   	0x04
#define 	STATE_FIRST_PREFRAMESKIP		0x02
#define 	STATE_INITIAL					0x01
#define 	STATE_NOTHING					0x00

#define 	STATE_SCAN						0x24
#define 	STATE_PREFRAMESKIP			   	0x12
#define 	STATE_OVER_ALL					0xC0

#define 	STATE_INC_COUNT					0x7F

//////////////// CAF ///////////////
// bSystem            	             
//#define 	SYSTEM_						  	0x80  
//#define 	SYSTEM_							0x40  
//#define 	SYSTEM_							0x20  
//#define 	SYSTEM_							0x10  
//#define 	SYSTEM_							0x08  
//#define 	SYSTEM_							0x04  
//#define 	SYSTEM_							0x02  
#define 	SYSTEM_FIRST_INIT		 		0x01  


// bCAFState
//#define 	STATE_CAF_ 					  	0x80  
#define 	STATE_CAF_WAIT_LOCK 		 	0x40  
#define 	STATE_CAF_SCAN_FINISH   		0x20  
#define 	STATE_CAF_SCANNING_FINE			0x10  
#define 	STATE_CAF_SCANNING 				0x08  
#define 	STATE_CAF_SCAN_INIT				0x04  
#define 	STATE_CAF_WAIT_STABLE			0x02  
#define 	STATE_CAF_LOCK					0x01  
#define 	STATE_CAF_NOTHING				0x00  
//
#define 	CAF_LOCK						0
#define 	CAF_UNLOCK						1

// bCAFStatus[1]
#define 	STATUS0_CAF_NEXT_WAIT_STABLE 	0x80  
#define 	STATUS0_CAF_RECORD_FINISH	 	0x40  
#define 	STATUS0_CAF_FD_AF 				0x20  
#define 	STATUS0_CAF_USE_F1				0x10  
#define 	STATUS0_CAF_UNLOCK_LUM		  	0x08  
#define 	STATUS0_CAF_MAX_PEAK			0x04  
#define 	STATUS0_CAF_RETURN1				0x02  
#define 	STATUS0_CAF_DIR_REVERSE			0x01 

// bCAFStatus[2]
#define 	STATUS1_CAF_RECORD_TOUCH 		0x80  
#define 	STATUS1_CAF_RECORD 			 	0x40  
#define 	STATUS1_CAF_FIRST_TIME 			0x20  
#define 	STATUS1_CAF_FDAF				0x10  
#define 	STATUS1_CAF_FINISH_NOTICE	  	0x08  
#define 	STATUS1_CAF_LOCK_AF				0x04  
//#define 	STATUS1_CAF_ 					0x02  
//#define 	STATUS1_CAF_					0x01 


// ISR_INPUT
#define 	ISR_RUNING_AF					0
#define 	ISR_START_SAF					1
#define 	ISR_START_TOUCH					2
#define 	ISR_START_ALGORITHM				3
#define 	ISR_STOP_ALGORITHM				4
// ISR_OUTPUT
#define 	ISR_MOVING_AF					0
#define 	ISR_FINISHED_AF_FAIL			1
#define 	ISR_FINISHED_AF_PASS			2


#define 	AF_CMD_LONG_PRESS_CAPTURE		1
#define 	AF_CMD_TOUCH_AF					2
#define 	AF_CMD_SHORT_PRESS_CAPTURE		3
#define 	AF_CMD_TOUCH_AF_TAP				4
#define 	AF_CMD_CAPTURE					5


/////////////////////////////////////////////
#define 	FD_NOTHING						0
#define 	FD_WAITING						1
#define 	FD_SCANNING						2
//#define 	FD_SCANNING



#endif
/* ------------------------------------------------------------------------ */
/* end of this file															*/
/* ------------------------------------------------------------------------ */
