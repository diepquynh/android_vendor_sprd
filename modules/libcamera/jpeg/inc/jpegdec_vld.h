/******************************************************************************
 ** File Name:      JpegDec_vld.h                                            *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           12/14/2006                                                *
 ** Copyright:      2006 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the operation interfaces of macroblock  *
 **                 operation of mp4 deccoder.                                *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _JPEGDEC_VLD_H_
#define _JPEGDEC_VLD_H_
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "jpegcodec_def.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C"
    {
#endif

#define DC_LEN1_OFFSET			0
#define DC_LEN2_OFFSET			2
#define DC_LEN3_OFFSET			6
#define DC_LEN4_OFFSET			14
#define DC_LEN5_OFFSET			26
#define DC_LEN6_OFFSET			38
#define DC_LEN7_OFFSET			50
#define DC_LEN8_OFFSET			62
#define DC_LEN9_OFFSET			74
#define DC_LEN10_OFFSET		86
#define DC_LEN11_OFFSET		98
#define DC_LEN12_OFFSET		110
#define DC_LEN13_OFFSET		122
#define DC_LEN14_OFFSET		134
#define DC_LEN15_OFFSET		146
#define DC_LEN16_OFFSET		158

PUBLIC void JPEGFW_configure_vld_reg_jpegDec (void);
PUBLIC void JPEGFW_build_hufftab_jpegDec(void);
PUBLIC void JPEGFW_InitHuffTbl(void);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End
#endif //_JPEGDEC_VLD_H_