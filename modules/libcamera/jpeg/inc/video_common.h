/******************************************************************************
** File Name:      video_commmon.h                                           *
** Author:         Xiaowei Luo                                               *
** DATE:           12/06/2007                                                *
** Copyright:      2007 Spreatrum, Incoporated. All Rights Reserved.         *
** Description:    common define for video codec.	     			          *
*****************************************************************************/
/******************************************************************************
**                   Edit    History                                         *
**---------------------------------------------------------------------------*
** DATE          NAME            DESCRIPTION                                 *
** 11/20/2007    Xiaowei Luo     Create.                                     *
*****************************************************************************/
#ifndef _VIDEO_COMMON_H_
#define _VIDEO_COMMON_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
//#include "sci_types.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

//#define JPEG_ENC
//#define JPEG_DEC
//#define _VSP_

#if defined(_SIMULATION_)
	#if defined(_VSP_)
		#define SIM_WITH_VSP  //may run in dvb board
	#else
		#define SIM_NO_VSP
		#if defined(_ARM_)
			#define SIM_IN_ADS
		#else
			#define SIM_IN_WIN
		#endif
	#endif
	#define _DEBUG_		0
#else
	#define RUN_IN_PLATFORM
	#define _DEBUG_     0
#endif

#if defined(SIM_IN_WIN)&&!defined(_LIB)
	#define _FW_TEST_VECTOR_ 0
#else
	#define _FW_TEST_VECTOR_ 0
#endif //defined(SIM_IN_WIN)&&!defined(_LIB)

#if defined(SIM_NO_VSP)
#define _CMODEL_   1
#else
#define _CMODEL_   0
#endif //defined(SIM_NO_VSP)
#define _CMODEL_   0

/*standard*/
typedef enum {
		ITU_H263 = 0,
		MPEG4,
		JPEG,
		FLV_H263,
		H264
		}VIDEO_FORMAT_E;

/*
	Bit define, for video
*/
#define V_BIT_0               0x00000001
#define V_BIT_1               0x00000002
#define V_BIT_2               0x00000004
#define V_BIT_3               0x00000008
#define V_BIT_4               0x00000010
#define V_BIT_5               0x00000020
#define V_BIT_6               0x00000040
#define V_BIT_7               0x00000080
#define V_BIT_8               0x00000100
#define V_BIT_9               0x00000200
#define V_BIT_10              0x00000400
#define V_BIT_11              0x00000800
#define V_BIT_12              0x00001000
#define V_BIT_13              0x00002000
#define V_BIT_14              0x00004000
#define V_BIT_15              0x00008000
#define V_BIT_16              0x00010000
#define V_BIT_17              0x00020000
#define V_BIT_18              0x00040000
#define V_BIT_19              0x00080000
#define V_BIT_20              0x00100000
#define V_BIT_21              0x00200000
#define V_BIT_22              0x00400000
#define V_BIT_23              0x00800000
#define V_BIT_24              0x01000000
#define V_BIT_25              0x02000000
#define V_BIT_26              0x04000000
#define V_BIT_27              0x08000000
#define V_BIT_28              0x10000000
#define V_BIT_29              0x20000000
#define V_BIT_30              0x40000000
#define V_BIT_31              0x80000000

//trace enable flag define

//test vector enable flag define


#if defined(_SIMULATION_)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define PRINTF printf
#define FPRINTF fprintf
#define _COMPUTE_PSNR_
#define SCI_ASSERT assert
#define SCI_PASSERT
#define EXIT  exit
#define IPRED_PRINTF //printf
#define TM_SendTestPointRequest
#define JPEG_PRINT
#define SCI_Sleep
#define SCI_MEMSET  memset
#define SCI_MEMCPY	memcpy
#define _TRACE_	0
#endif

//for macroblock
#define MB_SIZE					16
#define MB_CHROMA_SIZE			8
#define MB_SQUARE_SIZE			256

//for block
#define BLOCK_SIZE				8
#define BLOCK_SQUARE_SIZE		64
#define BLOCK_CNT				6

#define ABS(x) ((x) > 0 ? (x) : -(x))
#define Clip3(min,max,val) (((val)<(min))?(min):(((val)>(max))?(max):(val)))

#define mmax(aa,bb)		(((aa) > (bb)) ? (aa) : (bb))
#define mmin(aa,bb)		(((aa) < (bb)) ? (aa) : (bb))
#define ssign(x)		((x) > 0 ? 1 : (-1))
#define ssignof(x)		(((x) > 0) ? 1 : 0)
#define rounded(x)		(((x) > 0) ? (x + 0.5) :(x - 0.5))
#define IClip(Min, Max, Val) (((Val)<(Min))? (Min):(((Val)>(Max))? (Max):(Val)))
#define MEDIAN(aa, bb, cc) ((aa-bb)*(aa-cc)<=0)?aa :(((bb-cc)*(bb-aa)<=0)?bb:cc)




/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End
#endif  //_VIDEO_COMMON_H_
