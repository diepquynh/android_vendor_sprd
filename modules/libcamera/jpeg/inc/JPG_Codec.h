/******************************************************************************
 ** File Name:      jcodec.h                                                  *
 ** Author:         Zhemin.Lin                                                *
 ** Date:           2004/07/19                                                *
 ** Copyright:      2004 Spreadtrum, Incoporated. All Rights Reserved.        *
 ** Description:    define the jpeg codec structure & the interface           *
 ******************************************************************************

 ******************************************************************************
 **                        Edit History                                       *
 ** ------------------------------------------------------------------------- *
 ** DATE           NAME             DESCRIPTION                               *
 ** 2004/07/19     Zhemin.Lin       Create.                                   *
 ******************************************************************************/

#ifndef JPEG_CODEC_H
#define JPEG_CODEC_H


#include <stdio.h>
#include <stdlib.h>
#include "string.h"
//#include "memory.h"

#include "JPG_JFIF.h"
#include "JPG_Init.h"
#include "JPG_Stream.h"
#include "JPG_Huff.h"
#include "JPG_Quant.h"
#include "JPG_DCTxIDCT.h"
#include "JPG_DCT_Fast.h"
#include "dal_jpeg.h"
/************************************************************************/
/* MACRO Define                                                         */
/************************************************************************/
/*component id*/
#define LUM_ID		0
#define CHR_ID		1
#define Y_ID		0
#define U_ID		1
#define V_ID		2

#define DCTSIZE2 64

//
#define INOUT_BUF_SIZE 512

//#define FAST_DCT

//#ifdef FAST_DCT
//#define DC_DIFF 8192
//#else
//#define DC_DIFF 1024
//#endif

/*define return value*/
#define JPEG_SUCCESS	0
#define JPEG_FAILED		(-1)

/*define error id*/
#define JPEG_EID_NONE	0
#define JPEG_EID_MISSHUFF		1
#define JPEG_EID_MISSQUANT		2
#define JPEG_EID_IMGSIZE		3
#define JPEG_EID_SAMPLEFORMAT	4
#define JPEG_EID_COMPNUM		5
#define JPEG_EID_JFIFLEN		6
#define JPEG_EID_COLORSPACE		7
#define JPEG_EID_SAMPLEPRCISION	8
#define JPEG_EID_JFIFVERSION	9
#define JPEG_EID_NOTJFIF		10
#define JPEG_EID_MARKNOTSUPPORT	11
#define JPEG_EID_FILETYPE		12
#define JPEG_EID_ENCOUNTEOI		13
#define JPEG_EID_FILEOP			14
#define JPEG_EID_MEMOP			15

#define MAX_MCU_NUM	6

#define MAX3(a, b, c)	\
	MAX((a), MAX((b), (c)))

#define CLIP(v) ( (v)<0 ? 0 : ((v) > 255 ? 255 : (v)) )

/************************************************************************/
/* Struct Definition	                                                    */
/************************************************************************/
//
typedef void(* JPEG_CallBack)(uint8 *y_coeff, uint8 *u_coeff, uint8 *v_coeff, uint16 x, uint16 y);
//
typedef struct{
	uint8	h_ratio;
	uint8	v_ratio;
}SAMPLE_RATIO_T;

typedef struct{
	uint8	dc_huff_tbl_id;
	uint8	ac_huff_tbl_id;
	uint8	quant_tbl_id;
}TBL_MAP_T;


typedef struct {
	/*image format*/
	uint16			width;				/*image real width*/
	uint16			height;				/*image real height*/
	uint8			sample_format;		/*sample format, FORMAT_YUV411 FORMAT_YUV422*/
	SAMPLE_RATIO_T	ratio[3];			/*sample ratio*/
	TBL_MAP_T		tbl_map[3];			/*table map for three component*/
	/*coeff size*/
	uint16			c_width;			/*width of coeff*/
	uint16			c_height;			/*height of coeff*/
	/*density*/
	uint16			xdensity;			/*X density*/
	uint16			ydensity;			/*Y density*/
	/*huffman information*/
	HUFF_TBL_T 		dc_huff_tbl[2];		/*dc huffman table*/
	HUFF_TBL_T 		ac_huff_tbl[2];		/*ac huffman table*/
	/*quantization table*/
	const uint8		*quant_tbl[2];		/*quantization table*/
	uint16			*quant_tbl_new[2];
	/*MCU info*/
	uint16 			mcu_num_x;			/*mcu number per row*/
	uint16 			mcu_num_y;			/*mcu number per col*/
	uint16			mcu_height;
	uint16			mcu_width;
	uint16			h_ratio_max;
	uint16			v_ratio_max;
	int32 			*blocks[6];		/*mcu block buffer*/
	//added by wangyi//
	uint8			*org_blocks[6];    /*mcu original block buffer*/
	uint8  			blocks_membership[MAX_MCU_NUM];/*which component the block blong to*/
	int16 pre_dc_value[3];				/*store pre block value for DPCM coding*/
	/*stream*/
	void *stream;						/*point to JPEG stream*/
	uint32			error_id;			/*last error id*/
	uint32			restart_interval;
	uint32			restart_to_go;
	uint32			next_restart_num;
	uint32			tmp_file_mode;		/*if 1, it means, it's means it's a tmp file mode*/
	uint32			do_dequant;
	FILE			*stream_file;
	uint32          i_words;
	uint32          i_bits;
	uint8			block_num;
	JPEG_ENC_DATA_TYPE_E	enc_data_type;
	JPEG_DATA_ENDIAN_E		endian_type;
	JPEG_DEC_DATA_TYPE_E	dec_data_type;
	JPEG_DCT_MODE_E			dct_mode;		//normal or fast dct/idct
	JPEG_DEC_DATA_TYPE_E	dec_out_data_type;
	uint32 buf_id;

}JPEG_CODEC_T;

extern JPEG_CODEC_T g_JpegCodec;

extern int32 g_mcu_buf[64*MAX_MCU_NUM];
extern uint8 g_mcu_org_buf[64*MAX_MCU_NUM];
/*zigzag table*/
extern const uint8 zigzag_order[64+16];

extern uint8 jpeg_cache_buf[INOUT_BUF_SIZE+8];

extern uint16 g_block_num;
extern uint16 g_oldblock_id;
extern uint16 jstream_op_mode;
extern uint8  *jstream_buf;
extern uint8  *g_inter_buf;
extern uint8  *g_inter_buf_ptr;
extern uint32 read_write_bytes;
extern uint32 bytes_in_buf;
extern uint32 jstream_len;
extern uint32 jstream_current_pos;
extern uint32 jstream_words;
extern uint32 jremain_bit_num;
extern const int16 aanscales[];
extern int32 UV_multiple;

void JPEG_EncodeMCU(void);

uint32 JPEG_Encode(
				   uint32  y_coeff,
				   uint32  u_coeff,
				   uint32  v_coeff);

/*decoder operation interface*/

void JPEG_DecodeMCU(void);

uint32 JPEG_Decode(
				   uint32  y_coeff,
				   uint32  u_coeff,
				   uint32  v_coeff);



#endif

