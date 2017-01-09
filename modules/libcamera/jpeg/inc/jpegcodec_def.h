/*****************************************************************************
** File name:	   JpegCodec_def.h
** Author:         Yi Wang                                                   *
** DATE:           05/14/2007                                                *
** Copyright:      20067 Spreatrum, Incoporated. All Rights Reserved.        *
** Description:    This file defines the basic structure and macro           *
******************************************************************************
******************************************************************************
**                   Edit    History                                         *
**---------------------------------------------------------------------------*
** DATE          NAME             DESCRIPTION                                *
** 13/12/2007    Yi Wang		  Create.                                    *
*****************************************************************************/
#ifndef _JPEGCODEC_DEF_H_
#define _JPEGCODEC_DEF_H_

#include "sci_types.h"
#include "jpeg_fw_def.h"
#include "jpeg_jfif.h"
#include "jpeg_common.h"



#define PROGRESSIVE_SUPPORT 0

/*down sample*/
#define DOWN_SAMPLE_DIS	0
#define DOWN_SAMPLE_EN	1

#define JPEG_FW_LUM_ID			0
#define JPEG_FW_CHR_ID			1
#define JPEG_FW_Y_ID			0
#define JPEG_FW_U_ID			1
#define JPEG_FW_V_ID			2

#define JPEG_FW_DCTSIZE2		64
#define JPEG_FW_BLOCK_SIZE		8

#define JPEG_FW_MAX3(a, b, c)	\
	MAX((a), MAX((b), (c)))

#if defined(_SIMULATION_)
	#if defined(JPEG_DEC)
		#define STRM_PINGPANG_BUF_SIZE   		(3*1024*1024) //(400*1024)
		#define YUV_PINGPANG_BFR_SIZE			(800*1024) //(200*1024)
	#else
		#if defined(JPEG_ENC)
				#define STRM_PINGPANG_BUF_SIZE   (1024*500-1)//(1024)//(1024*256-1)
				#define YUV_PINGPANG_BFR_SIZE			(640*480*2)//(16*1024)//(640*480*2)
		#endif
	#endif
#endif

#define _ARM_
#ifndef _ARM_
#include "stdlib.h"
#include "stdio.h"
#define JPEG_PRINTF		printf
#define JPEG_TRACE      JPEG_PRINTF
#define JPEG_ERROR(err_id, arg) \
		g_error_id = err_id; \
		JPEG_PRINTF("JPEG ERROR: %s, %s, %d\n",(#err_id), __FILE__, __LINE__); \
		JPEG_PRINTF(##arg); \
		JPEG_PRINTF("\n")

#define JPEG_ASSERT(expr) \
		if(!(expr)) { \
		JPEG_PRINTF( "JPEG Assertion failed! %s:%d %s\n", \
		__FILE__,__LINE__,(#expr));  \
		}
#else//define ARM
#define JPEG_PRINTF //NULL
#define JPEG_TRACE
#define JPEG_ERROR
#define JPEG_ASSERT
#endif//

typedef enum {ALONE_MODE = 0, SWITCH_MODE}WORK_MODE_E;

#define DC_DIFF (1024)
#define MAX_BLK_NUM  (512*512)
#define MEM_SIZE  (1024*20)
#define BITSTREAM_SIZE (1024*256)

#if defined(_SIMULATION_)
	#define DATA_RAM_BASE				SDRAM_START_ADDR //(0x04000000 + 256*1024)       //0x04040000	internal memory
	#define YUV_BUF0_OUT_BASE			(DATA_RAM_BASE + 64*1024)     //0x04050000	yuvbuf0
	#define YUV_BUF1_OUT_BASE			(YUV_BUF0_OUT_BASE + 512*1024)//0x040d0000  yuvbuf1
	#define STREAM_BUF0_BASE        	(YUV_BUF1_OUT_BASE + 512*1024)//0x04150000  stream0

	#if defined(JPEG_ENC)
		#define STREAM_BUF1_BASE        (STREAM_BUF0_BASE + 255*1024)  //0x04158000  stream1
		#define BIT_STREAM_BASE 		(0x041d0000)//(STREAM_BUF1_BASE + 32*1024)  //0x04160000	bitstream:max=640k
	#elif defined(JPEG_DEC)
		#define STREAM_BUF1_BASE        (STREAM_BUF0_BASE + 64*1024)  //0x04160000  stream1
		#define BIT_STREAM_BASE 		(0x04180000)//(STREAM_BUF1_BASE + 32*1024)  //0x04160000	bitstream:max=640k
	#else
	#endif

	#define Y_BUF_BASE					(0x04000000 + 2 * 1024*1024) //0x04200000	y buf
	#define UV_BUF_BASE					(0x04000000 + 3 * 1024*1024) //0x04200000	uv buf

	#define RGB_ADDR_BASE				(0x04200000)
#endif //SMALL_SYS

typedef void (*JPEG_CallBack_MCU_To_Frame)(uint8 *y_coeff, uint8 *u_coeff, uint8 *v_coeff, uint16 x, uint16 y, uint8 scale_down_factor);
typedef void (*JPEG_CallBack_Frame_To_MCU)(uint8 *y_coeff, uint8 *u_coeff, uint8 *v_coeff, uint16 x, uint16 y);
typedef void (*JPEG_TRANSFORM_FUN)(int16 *block_coeff, uint8 *output_buf, const int32 *quantptr);

/**---------------------------------------------------------------------------*
**                         Macro define
**---------------------------------------------------------------------------*/

#if PROGRESSIVE_SUPPORT
typedef struct
{
	uint8		*src_buf;		/* start of buffer */
	uint32      	src_buf_len;   /* src mem buffer len */
	uint32		bytes_in_buf;
	uint32		jstream_words;
	uint32		jremain_bit_num;
	uint32      	read_write_bytes;
} bitstream_info;

/* Derived data constructed for each Huffman table */

#define HUFF_LOOKAHEAD 	8	/* # of bits of lookahead */

typedef struct {
  /* Basic tables: (element [0] of each array is unused) */
  int32 maxcode[18];		/* largest code of length k (-1 if none) */
  /* (maxcode[17] is a sentinel to ensure jpeg_huff_decode terminates) */
  int32 valoffset[17];		/* huffval[] offset for codes of length k */
  /* valoffset[k] = huffval[] index of 1st symbol of code length k, less
   * the smallest code of length k; so given a code of length k, the
   * corresponding symbol is huffval[code + valoffset[k]]
   */
    /* Link to public Huffman table (needed only in jpeg_huff_decode) */
  HUFF_TBL_T *pub;

  /* Lookahead tables: indexed by the next HUFF_LOOKAHEAD bits of
   * the input data stream.  If the next Huffman code is no more
   * than HUFF_LOOKAHEAD bits long, we can obtain its length and
   * the corresponding symbol directly from these tables.
   */
  int32 look_nbits[1<<HUFF_LOOKAHEAD]; /* # bits, or 0 if too long */
  uint8 look_sym[1<<HUFF_LOOKAHEAD]; /* symbol, or unused */
} d_derived_tbl;

typedef struct
{
	uint16		restarts_to_go;/* MCUs left in this restart interval */
	uint16		next_restart_num;
	uint32		last_dc_value[MAX_COMPS_IN_SCAN];
	uint32		EOBRUN;
	 /* Pointers to derived tables (these workspaces have image lifespan) */
 	d_derived_tbl * vld_table[NUM_HUFF_TBLS];
  	d_derived_tbl * ac_derived_tbl; /* active table during an AC scan */
	BOOLEAN     (*decode_mcu) (int16 **MCU_data);
} phuff_entropy_info;

typedef struct
{
	bitstream_info address;
	uint8 comps_in_scan;/* number of components encoded in this scan */
	jpeg_component_info cur_comp_info[MAX_COMPS_IN_SCAN];/* their SOF/comp_info[] indexes */
	int16 id[3];
	uint16 Ss, Se;/* progressive JPEG spectral selection parms */
	uint16 Ah, Al;/* progressive JPEG successive approx. parms */
	phuff_entropy_info entropy;
} JPEG_SOS_T;
#endif//PROGRESSIVE_SUPPORT

typedef struct jpeg_codec_tag
{
	uint32			restart_interval;
	uint32			restart_to_go;
	uint32			next_restart_num;
	uint32 			dc_pred_y;
	uint32			dc_pred_uv;
	uint32			restart_mcu_cnt;
	uint32			bitstream_offset;

	uint16			width;				/*image real width*/
	uint16			height;				/*image real height*/

	uint16			c_width;				/*width of coeff*/
	uint16			c_height;			/*height of coeff*/

	uint16			out_width;			//output image width
	uint16			out_height;			//output image height

	SAMPLE_RATIO_T	ratio[3];				/*sample ratio*/
	uint8			input_mcu_info;		/*sample format*/
	uint8			out_mcu_info;

	JPEG_QUALITY_E	compress_level;		//
	uint8			RST_Count;

	TBL_MAP_T		tbl_map[3];			/*table map for three component*/

	/*MCU info*/
	uint16 			mcu_num_x;		/*mcu number per row*/
	uint16 			mcu_num_y;		/*mcu number per col*/

	uint16			mcu_height;
	uint16			mcu_width;

	BOOLEAN			progressive_mode;
	uint8			num_components;
	uint8			scale_factor;

	void			*stream_0;						/*point to JPEG stream_0*/
	void			*stream_1;						/*point to JPEG stream_0*/
	int32			pingpang_buf_len;			//ping_pang buf size
	int32			decoded_stream_len;		//stream length
	int32			encoded_stream_len;		//

	/*huffman information*/
	HUFF_TBL_T 		dc_huff_tbl[NUM_HUFF_TBLS];		/*dc huffman table*/
	HUFF_TBL_T 		ac_huff_tbl[NUM_HUFF_TBLS];		/*ac huffman table*/

	/*quantization table*/
	uint8  		*quant_tbl[3];		/*quantization table*/
	uint16		*quant_tbl_new[3];
	uint8		*quant_tbl_shift[3];

	YUV_FORMAT_T	YUV_Info_0;
	YUV_FORMAT_T	YUV_Info_1;
	int32		y_interleaved;
	int32		uv_interleaved;

	JINF_READ_FILE_FUNC read;
	uint8			out_put_dataType;
	uint8 			using_default_huff_tab;
	uint8 			using_default_quant_tab;
	uint8			work_mode; //0: only decoder/encoder, 1: switch decoder and encoder.

	BOOLEAN			mbio_bfr0_valid; //0: invalid, 1: valid
	BOOLEAN			mbio_bfr1_valid; //0: invalid, 1: valid
	BOOLEAN 		bsm_buf0_valid;//0: invalid, 1: valid
	BOOLEAN			bsm_buf1_valid;//0: invalid, 1: valid
	BOOLEAN			is_first_slice;
	BOOLEAN			is_last_slice;
	uint8				  comp_id_map[3];      /*index map for three component of one scan*/
	jpeg_component_info * comp_info;
	uint8			      comps_in_scan;
	int16				  Ss;
	int16				  Se;
	int16				  Ah;
	int16				  Al;
	uint8				  input_scan_number;
	int8				  is_res_file;
	int16			  	  resv;
	uint32                stream_buf_id;
	uint32                buf_id;
	uint32                total_slice_num;
	uint32                slice_num;
	int			          fd ;
	void                  *jpg_addr;
	unsigned long			      addr;
	uint32                stream_switch_num;
	uint8                 *g_stream_buf_ptr;
}JPEG_CODEC_T;

typedef struct jpeg_progressive_info_tag
{
	int16			Ss;
	int16			Se;
	int16			Ah;
	int16			Al;

	uint8			input_scan_number;
	uint8			cur_scan;//current scan number;
	uint8			comps_in_scan;
	uint16			MCU_per_col;

	uint16			MCU_per_row;
	uint16			new_mcu_line;

	uint8				comp_id_map[3];      /*index map for three component of one scan*/

	jpeg_component_info * comp_info;
#if PROGRESSIVE_SUPPORT
	JPEG_SOS_T		*buf_storage;//store the all content of sos
#endif
	jpeg_component_info *cur_comp_info[MAX_COMPS_IN_SCAN];

	uint8			low_quality_idct;
	uint32			DC_Diff;
	JPEG_TRANSFORM_FUN jpeg_transform;

	uint16			block_num;
	uint8  			blocks_membership[MAX_MCU_NUM];/*which component the block blong to*/

	int16			*block_line[3]; //

	int16			*blocks[6];		/*mcu block buffer*/
	uint8			*org_blocks[6];    /*mcu original block buffer*/
	int32			pre_dc_value[3];	/*store pre block value for DPCM coding*/

	int32			*quant_tbl_new[2];
}JPEG_PROGRESSIVE_INFO_T;

#endif//_JPEGCODEC_DEF_H_
