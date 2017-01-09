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
#ifndef _JPEGDEC_PVLD_H_
#define _JPEGDEC_PVLD_H_
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "jpegcodec_def.h"
#include "jpegdec_bitstream.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C"
    {
#endif

#if PROGRESSIVE_SUPPORT
#define HUFF_DECODE(result,tbl,slowlabel) \
{   register int32 nb, look; \
	if (s_jremain_bit_num < HUFF_FIRST_READ) { \
    JPEG_Fill_Bit_Buffer();\
    if (s_jremain_bit_num < HUFF_FIRST_READ) { \
	nb = 1; goto slowlabel; }\
	} \
	look = PEEK_BITS(HUFF_FIRST_READ); \
	if ((nb = tbl->look_nbits[look]) != 0) { \
    DROP_BITS(nb); \
    result = tbl->look_sym[look]; \
	} else { \
    nb = HUFF_FIRST_READ+1; \
slowlabel: \
    result=huff_DECODE_Progressive(tbl,nb);\
} \
}

uint32 JPEG_Generate_Entry_Point_Map_Progressive(void);
#endif
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End
#endif //_JPEGDEC_PVLD_H_