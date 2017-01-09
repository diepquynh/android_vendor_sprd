/******************************************************************************
 ** File Name:      JpegDec_dequant.h                                            *
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
#ifndef _JPEGDEC_DEQUANT_H_
#define _JPEGDEC_DEQUANT_H_
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

extern uint8  *jpeg_fw_quant_tbl[2];
extern uint16 *jpeg_fw_quant_tbl_new[2];

PUBLIC void JPEGFW_InitQuantTbl(JPEG_QUALITY_E level);
PUBLIC JPEG_RET_E JPEGFW_AdjustQuantTbl_Dec();
PUBLIC void JPEGFW_InitTransFun(JPEG_PROGRESSIVE_INFO_T *progressive_info_ptr);
PUBLIC void Initialize_Clip();
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End
#endif //_JPEGDEC_DEQUANT_H_
