/******************************************************************************
 ** File Name:      JpegDec_out.h                                            *
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
#ifndef _JPEGDEC_OUT_H_
#define _JPEGDEC_OUT_H_
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

void JPEGFW_OutMCU444(uint8 *y_coeff, uint8 *uv_coeff, uint16 x, uint16 y, uint8 scale_down_factor);
void JPEGFW_OutMCU420(uint8 *y_coeff, uint8 *uv_coeff, uint16 x, uint16 y, uint8 scale_down_factor);
void JPEGFW_OutMCU400(uint8 *y_coeff, uint8 *uv_coeff, uint16 x, uint16 y, uint8 scale_down_factor);
void JPEGFW_OutMCU411(uint8 *y_coeff, uint8 *uv_coeff, uint16 x, uint16 y, uint8 scale_down_factor);
void JPEGFW_OutMCU422(uint8 *y_coeff, uint8 *uv_coeff, uint16 x, uint16 y, uint8 scale_down_factor);

typedef void (*JPEGFW_MCU_To_Frame)(uint8 *y_coeff, uint8 *uv_coeff, uint16 x, uint16 y, uint8 scale_down_factor);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End
#endif //_JPEGDEC_FRAME_H_

