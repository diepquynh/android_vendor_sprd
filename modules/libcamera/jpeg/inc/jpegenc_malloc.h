/******************************************************************************
 ** File Name:      JpegEnc_malloc.h                                          *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           03/13/2009                                                *
 ** Copyright:      2009 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the malloc function interfaces of       *
 **					jpeg encoder											  *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 01/23/2007    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _JPEGENC_MALLOC_H_
#define _JPEGENC_MALLOC_H_
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "sc8830_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C"
    {
#endif

PUBLIC void *JpegEnc_ExtraMemAlloc(uint32 mem_size);
PUBLIC void JpegEnc_FreeMem(void);
PUBLIC void JpegEnc_InitMem(JPEG_MEMORY_T *pBuffer);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End
#endif // _JPEGENC_MALLOC_H_
