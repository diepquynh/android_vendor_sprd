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
#ifndef _JPEGDEC_BITSTREAM_H_
#define _JPEGDEC_BITSTREAM_H_
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "sc8830_video_header.h"
#include "jpg_bsm.h"
#include "jpg_drv_sc8830.h"

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C"
    {
#endif

extern uint32 			s_jstream_words;
extern uint32 			s_jremain_bit_num;

#define CHECK_BIT_BUFFER(nbits) \
{ if(s_jremain_bit_num < (nbits)) {\
		JPEG_Fill_Bit_Buffer();}\
}

#define PEEK_BITS(nbits) (((int32)(s_jstream_words >> (s_jremain_bit_num - (nbits)))) & ((1<<(nbits))-1))
#define DROP_BITS(nbits) (s_jremain_bit_num -= (nbits))
#define JPEG_GETBITS(nbits) (((int) (s_jstream_words >> (s_jremain_bit_num -= (nbits)))) & ((1<<(nbits))-1))

void JPEG_Fill_Bit_Buffer(void);
//uint8 huff_DECODE_Progressive(d_derived_tbl *tbl, int32 min_bits);
//void Update_Global_Bitstrm_Info(bitstream_info *pBitstrmInfo);
//void Update_Local_Bitstrm_Info(bitstream_info *pBitstrmInfo);
int32 check_RstMarker(void);

PUBLIC JPEG_RET_E  JpegDec_InitBitream(JPEG_DEC_INPUT_PARA_T  *jpeg_dec_input);
PUBLIC uint32 get_header_len(JPEG_DEC_INPUT_PARA_T  *jpeg_dec_input);
PUBLIC BOOLEAN get_char(uint8 *value_ptr);
PUBLIC BOOLEAN get_short_word(uint16 *value_ptr);
PUBLIC BOOLEAN get_word(uint32 *value_ptr);
PUBLIC uint32 is_res_jpg_file(void);
PUBLIC BOOLEAN skip_n_byte(uint32 n);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End
#endif //_JPEGDEC_BITSTREAM_H_
