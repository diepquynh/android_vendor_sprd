/*****************************************************************************
** File name:	   JpegCodec_global.c
** Author:         Yi Wang                                                   *
** DATE:           13/12/2007                                                *
** Copyright:      20067 Spreatrum, Incoporated. All Rights Reserved.        *
** Description:    This file defines the global variants                     *
******************************************************************************
******************************************************************************
**                   Edit    History                                         *
**---------------------------------------------------------------------------*
** DATE          NAME             DESCRIPTION                                *
** 13/12/2007    Yi Wang		  Create.                                    *
*****************************************************************************/
#include "video_common.h"

#include "jpegcodec_def.h"

/************************************************************************/
/* internal ram                                                         */
/************************************************************************/
LOCAL JPEG_CODEC_T 	g_JpegEncCodec;
LOCAL JPEG_CODEC_T 	g_JpegDecCodec;
JPEG_PROGRESSIVE_INFO_T *g_JpegProgInfo;

uint32			g_encoded_stream_len;
uint8 			*g_jpeg_fw_buf_ptr;
uint32 			g_start_offset;
uint32			g_bsm_offset;
uint32			g_Decoded_bytes;
uint8			g_shift_bits;

BOOLEAN 		g_huff_tbl_malloced;
int32			g_flush_word_count;

JPEG_CODEC_T *Get_JPEGEncCodec()
{
	return &g_JpegEncCodec;
}

JPEG_CODEC_T *Get_JPEGDecCodec()
{
	return &g_JpegDecCodec;
}

JPEG_PROGRESSIVE_INFO_T *JPEGFW_GetProgInfo()
{
	return g_JpegProgInfo;
}

void JPEGFW_SetProgInfo(JPEG_PROGRESSIVE_INFO_T *progressive_info_ptr)
{
	g_JpegProgInfo = progressive_info_ptr;
}

void JPEGFW_InitHuffTblWithDefaultValue(JPEG_CODEC_T *jpeg_fw_codec)
{
	jpeg_fw_codec->tbl_map[Y_ID].dc_huff_tbl_id = LUM_ID;
	jpeg_fw_codec->tbl_map[Y_ID].ac_huff_tbl_id = LUM_ID;
	jpeg_fw_codec->tbl_map[U_ID].dc_huff_tbl_id = CHR_ID;
	jpeg_fw_codec->tbl_map[U_ID].ac_huff_tbl_id = CHR_ID;
	jpeg_fw_codec->tbl_map[V_ID].dc_huff_tbl_id = CHR_ID;
	jpeg_fw_codec->tbl_map[V_ID].ac_huff_tbl_id = CHR_ID;
}

#if defined(_SIMULATION_)
	volatile BOOLEAN			g_int_time_out;

	//decoder
	volatile BOOLEAN			g_int_yuv_buf0_full;
	volatile BOOLEAN			g_int_yuv_buf1_full;
	volatile BOOLEAN			g_int_stream_buf0_empty;
	volatile BOOLEAN			g_int_stream_buf1_empty;
	volatile BOOLEAN			g_int_vld_error;

	//encoder
	volatile BOOLEAN			g_int_yuv_buf0_empty;
	volatile BOOLEAN			g_int_yuv_buf1_empty;
	volatile BOOLEAN			g_int_stream_buf0_full;
	volatile BOOLEAN			g_int_stream_buf1_full;
	volatile BOOLEAN			g_int_vlc_done;

	uint8 			*g_cur_bs_bfr_ptr;
	uint8 			*g_y_bfr_ptr;
	uint8			*g_u_bfr_ptr;
	uint8			*g_v_bfr_ptr;
	uint32			g_rows_out;
	uint32			g_copy_data_y;
	uint32			g_copy_data_u;
	uint32			g_copy_data_v;
	uint32			jstream_len;

#endif //_SIMULATION_

#if _CMODEL_
	int32			g_mcuNum;
	uint8			*g_rgb888_buf;

	int32			g_DC_Diff;
	uint32			g_stream_offset;
	/*purpose:  remove the value which is used to save position of current pointer*/
	/*			and can use IRAM to process the bitstream manuiplation*/

	uint8			*g_inter_buf_bitstream;
	uint8			*g_inter_buf_bitstream_ptr;

	/*MCU buffer*/
	int16 			g_mcu_buf[MAX_MCU_NUM*64];
	/*Block buffer*/
	uint8 			g_mcu_org_buf[MAX_MCU_NUM*64];

	uint16   		g_block_num;
	uint16    		g_oldblock_id;

	uint32 			g_noneZeroFlag [6][2];

	uint32			g_error_id;			/*last error id*/
	int16			*g_blocks[6];		/*mcu block buffer*/
	uint8			*g_org_blocks[6];    /*mcu original block buffer*/
	uint8  			g_blocks_membership[MAX_MCU_NUM];/*which component the block blong to*/
	FILE *g_pf_debug;
#endif//_CMODEL_



