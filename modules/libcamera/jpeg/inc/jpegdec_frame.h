/******************************************************************************
 ** File Name:      JpegDec_frame.h                                            *
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
#ifndef _JPEGDEC_FRAME_H_
#define _JPEGDEC_FRAME_H_
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

PUBLIC JPEG_RET_E START_HW_DECODE(JPEG_CODEC_T *jpeg_fw_codec, uint32 num_of_rows);
PUBLIC JPEG_RET_E JpegDec_GetBsBitOffset(uint32 bitstrm_bfr_switch_cnt);
PUBLIC JPEG_RET_E START_SW_DECODE_PROGRESSIVE(JPEG_CODEC_T *jpeg_fw_codec, uint32 num_of_rows);
PUBLIC JPEG_RET_E JpegDec_CheckRowNumValid(uint32 num_of_rows, uint32 input_mcu_info);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End
#endif //_JPEGDEC_FRAME_H_
