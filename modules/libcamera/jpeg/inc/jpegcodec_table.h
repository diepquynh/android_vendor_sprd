/******************************************************************************
** File Name:      JpegEnc_vlc.h                                                *
** Author:         yi.wang		                                              *
** Date:           07/19/2007		                                          *
** Copyright:      2007 Spreadtrum, Incoporated. All Rights Reserved.         *
** Description:    define the huffman codec operation interface               *
*******************************************************************************

******************************************************************************
**                        Edit History                                       *
** ------------------------------------------------------------------------- *
** DATE           NAME             DESCRIPTION                               *
** 07/19/2007     Yi.Wang          Create.                                   *
******************************************************************************/
#ifndef _JPEGCODEC_TABLE_H_
#define _JPEGCODEC_TABLE_H_

#include "jpegcodec_def.h"



/*default vaule for luminance component*/
extern  uint8 jpeg_fw_lum_dc_bits_default[MAX_BITS_SIZE+1];
extern  uint8 jpeg_fw_lum_dc_huffvalue_default[DC_SYMBOL_NUM];
extern  uint8 jpeg_fw_lum_ac_bits_default[MAX_BITS_SIZE+1];
extern  uint8 jpeg_fw_lum_ac_huffvalue_default[AC_SYMBOL_NUM];
/*default vaule for chroma component*/
extern  uint8 jpeg_fw_chr_dc_bits_default[MAX_BITS_SIZE+1];
extern  uint8 jpeg_fw_chr_dc_huffvalue_default[DC_SYMBOL_NUM];
extern  uint8 jpeg_fw_chr_ac_bits_default[MAX_BITS_SIZE+1];
extern  uint8 jpeg_fw_chr_ac_huffvalue_default[AC_SYMBOL_NUM];


extern const uint32 jpeg_fw_hufftab_dcLuma [12];
extern const uint32 jpeg_fw_hufftab_dcChroam [12];

extern const uint8  jpeg_fw_zigzag_order[64+16];
extern const uint8  jpeg_fw_ASIC_DCT_Matrix[64];

extern		 uint32	g_huffTab[162];
extern		 uint32 g_huff_tab_enc[162];
extern const uint32 jpeg_fw_vld_default_max_code[66];
extern const uint32	jpeg_fw_vld_default_huffTab[162];

#include "sci_types.h"
#include "jpegcodec_global.h"

extern uint8 jpeg_fw_lum_quant_tbl_default[5][64];
extern uint8 jpeg_fw_chr_quant_tbl_default[5][64];

//
extern uint16 jpeg_fw_new_lum_quant_tbl_default[5][64];
extern uint16 jpeg_fw_new_chr_quant_tbl_default[5][64];
extern uint8  jpeg_fw_new_lum_quant_tbl_default_shift[5][64];
extern uint8  jpeg_fw_new_chr_quant_tbl_default_shift[5][64];


#endif//_JPEGCODEC_TABLE_H_
