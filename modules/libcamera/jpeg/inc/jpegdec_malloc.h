/******************************************************************************
 ** File Name:      JpegDec_malloc.h                                           *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           01/23/2007                                                *
 ** Copyright:      2006 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the malloc function interfaces of       *
 **					mp4 decoder												  *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 01/23/2007    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _JPEGDEC_MALLOC_H_
#define _JPEGDEC_MALLOC_H_
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

PUBLIC void *JpegDec_ExtraMemAlloc(uint32 mem_size);
PUBLIC void JpegDec_FreeNBytes(uint32 mem_size);
PUBLIC void JpegDec_FreeMem(void);
PUBLIC void JpegDec_InitMem(JPEG_MEMORY_T *pBuffer);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End
#endif // _JPEGDEC_MALLOC_H_
