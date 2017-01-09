/******************************************************************************
 ** File Name:      JpegDec_init.h                                            *
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
#ifndef _JPEGDEC_INIT_H_
#define _JPEGDEC_INIT_H_
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

PUBLIC JPEG_RET_E JPEG_FWInitDecInput(JPEG_DEC_INPUT_PARA_T *jpeg_dec_input);
PUBLIC void JpegDec_HwTopRegCfg(void);
PUBLIC void JpegDec_HwSubModuleCfg(uint32 header_length);
PUBLIC void JPEGFW_AllocMCUBuf(void);
PUBLIC void JpegDec_HwTopUpdateYUVAddr(uint32 y_phy_addr,uint32_t u_phy_addr,uint32_t v_phy_addr);


/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End
#endif //_JPEGDEC_INIT_H_
