/*****************************************************************************
** File name:	   JpegCodec_global.h
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
#ifndef _JPEGCODEC_GLOBAL_H_
#define _JPEGCODEC_GLOBAL_H_

#include "jpegcodec_def.h"

/************************************************************************/
/* internal ram                                                         */
/************************************************************************/
extern uint32			g_encoded_stream_len;
extern uint8 			*g_jpeg_fw_buf_ptr;
extern uint32 			g_start_offset;
extern uint32			g_bsm_offset;
extern uint32			g_Decoded_bytes;
extern uint8			g_shift_bits;

extern BOOLEAN 		g_huff_tbl_malloced;
extern int32		g_flush_word_count;

#if defined(_SIMULATION_)
	extern volatile BOOLEAN			g_int_time_out;

	//decoder
	extern volatile BOOLEAN			g_int_yuv_buf0_full;
	extern volatile BOOLEAN			g_int_yuv_buf1_full;
	extern volatile BOOLEAN			g_int_stream_buf0_empty;
	extern volatile BOOLEAN			g_int_stream_buf1_empty;
	extern volatile BOOLEAN			g_int_vld_error;

	//encoder
	extern volatile BOOLEAN			g_int_yuv_buf0_empty;
	extern volatile BOOLEAN			g_int_yuv_buf1_empty;
	extern volatile BOOLEAN			g_int_stream_buf0_full;
	extern volatile BOOLEAN			g_int_stream_buf1_full;
	extern volatile BOOLEAN			g_int_vlc_done;



	extern uint8 			*g_cur_bs_bfr_ptr;
	extern uint8 			*g_y_bfr_ptr;
	extern uint8			*g_u_bfr_ptr;
	extern uint8			*g_v_bfr_ptr;
	extern uint32			g_rows_out;
	extern uint32			g_copy_data_y;
	extern uint32			g_copy_data_u;
	extern uint32			g_copy_data_v;
	extern uint32			jstream_len;

#endif

JPEG_CODEC_T *Get_JPEGEncCodec(void);
JPEG_CODEC_T *Get_JPEGDecCodec(void);
void JPEGFW_InitHuffTblWithDefaultValue(JPEG_CODEC_T *jpeg_fw_codec);
JPEG_PROGRESSIVE_INFO_T *JPEGFW_GetProgInfo();
void JPEGFW_SetProgInfo(JPEG_PROGRESSIVE_INFO_T *progressive_info_ptr);

#if _CMODEL_
	extern int32			g_mcuNum;
	extern uint8			*g_rgb888_buf;

	extern int32			g_DC_Diff;
	extern uint32			g_stream_offset;
	/*purpose:  remove the value which is used to save position of current pointer*/
	/*			and can use IRAM to process the bitstream manuiplation*/
	extern uint8 			*g_inter_buf_bitstream;
	extern uint8 			*g_inter_buf_bitstream_ptr;

	/*MCU buffer*/
	extern int16 			g_mcu_buf[MAX_MCU_NUM*64];
	/*Block buffer*/
	extern uint8 			g_mcu_org_buf[MAX_MCU_NUM*64];
	extern uint16   		g_block_num;
	extern uint16    		g_oldblock_id;

	extern uint32 			g_noneZeroFlag [6][2];

	extern	uint32			g_error_id;			/*last error id*/
	extern	int16			*g_blocks[6];		/*mcu block buffer*/
	extern	uint8			*g_org_blocks[6];    /*mcu original block buffer*/

	extern	uint8  			g_blocks_membership[MAX_MCU_NUM];/*which component the block blong to*/
	extern FILE *g_pf_debug;
#endif//_CMODEL_




#endif//_JPEGCODEC_GLOBAL_H_
