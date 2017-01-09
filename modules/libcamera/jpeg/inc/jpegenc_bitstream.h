/******************************************************************************
 ** File Name:      JpegEnc_bitstream.h                                            *
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
#ifndef _JPEGENC_BITSTREAM_H_
#define _JPEGENC_BITSTREAM_H_
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "jpegcodec_def.h"
#include "sc8830_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C"
    {
#endif

#if _CMODEL_
#else
__inline void JPEGFW_PutBits(uint32 val, uint32 nbits);
/*{
	READ_REG_POLL(VSP_BSM_REG_BASE+BSM_READY_OFF, 1, 1, TIME_OUT_CLK, "BSM_READY: polling bsm rfifo ready");

	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG2_OFF, (nbits << 24) , "BSM_CFG2: configure write n bits");
	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_WDATA_OFF, val, "BSM_WDATA: write val(n bits) to bitstream, auto-stuffing");
}*/
#endif //_CMODEL_

PUBLIC void JPEGFW_PutC(uint8 ch);
PUBLIC void JPEGFW_PutW(uint16 w);
PUBLIC void JPEGFW_PutW_II(uint16 w);
PUBLIC void JPEGFW_PutBits32_II(uint32 val, uint32 nbits);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End
#endif //_JPEGENC_BITSTREAM_H_
