/******************************************************************************
 ** File Name:      JpegEnc_header.h                                            *
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
#ifndef _JPEGENC_HEADER_H_
#define _JPEGENC_HEADER_H_
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

/* Emit a marker code */
PUBLIC void PutMarker(JPEG_MARKER_E mark);
PUBLIC void OutPutRstMarker(void);
PUBLIC JPEG_RET_E PutAPP0(void);
PUBLIC JPEG_RET_E PutAPP1(APP1_T *app1_t);
PUBLIC JPEG_RET_E PutSOF0(void);
PUBLIC JPEG_RET_E PutSOS(void);
PUBLIC JPEG_RET_E PutQuantTbl(void);
PUBLIC JPEG_RET_E PutHuffTbl(void);
PUBLIC JPEG_RET_E WriteDRI(void);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End
#endif //_JPEGENC_HEADER_H_
