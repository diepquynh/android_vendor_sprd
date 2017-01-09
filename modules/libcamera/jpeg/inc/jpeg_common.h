/******************************************************************************
 ** File Name:       jpeg_common.h                                         	  *
 ** Author:          Frank.Yang		                                          *
 ** DATE:            08/25/2008                                               *
 ** Copyright:      2008 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:                                                              *
 ******************************************************************************

 ******************************************************************************
 **                        Edit History                                       *
 ** ------------------------------------------------------------------------- *
 ** DATE              NAME             DESCRIPTION                            *
 ******************************************************************************/
#ifndef _JPEG_COMMON_H_
#define _JPEG_COMMON_H_

#include "jpeg_types.h"

/*component id*/
#define LUM_ID		0
#define CHR_ID		1
#define Y_ID		0
#define U_ID		1
#define V_ID		2

#define DCTSIZE2 	64
#define DCTSIZE		8

#define INOUT_BUF_SIZE 		0

#define MAX_MCU_NUM			6
#define MAX_SCAN_NUM    	16
#define MAX_COMPS_IN_SCAN 	4
#define NUM_HUFF_TBLS		4

#define MAX3(a, b, c)	\
	MAX((a), MAX((b), (c)))

#define MAX_BITS_SIZE	16
#define AC_SYMBOL_NUM	256
#define DC_SYMBOL_NUM	12
#define HUFF_FIRST_READ 8

//#define CLIP(v) ( (v)<0 ? 0 : ((v) > 255 ? 255 : (v)) )

#define RES_MEM_SIZE  			0
#define NORMAL_JPEG_MEM_SIZE	(15 * 1024)

#define	JPEG_FW_FIFO_LEN					64

typedef struct {
	uint8 	*bits;				/* bits[k] = # of symbols with codes of */
	uint8 	*huffval;			/* The symbols, in order of incr code length*/
} HUFF_TBL_T;

//
typedef struct{
	uint8	h_ratio;
	uint8	v_ratio;
}SAMPLE_RATIO_T;

typedef struct{
	uint8	dc_huff_tbl_id;
	uint8	ac_huff_tbl_id;
	uint16	quant_tbl_id;
}TBL_MAP_T;

/* Basic info about one component (color channel). */
typedef struct
{
	/* These values are fixed over the whole image, they are read from the SOF marker.*/
	uint8 component_id;/* identifier for this component (0..255) */
	uint8 component_index;/* its index in SOF or cinfo->comp_info[] */
	uint8 quant_tbl_no;/* quantization table selector (0..3) */
	uint8 h_samp_factor;/* horizontal sampling factor (1..4) */

	uint8 v_samp_factor;/* vertical sampling factor (1..4) */
 /* These values may vary between scans, they are read from the SOS marker. */
	uint8 dc_tbl_no;/* DC entropy table selector (0..3) */
	uint8 ac_tbl_no;/* AC entropy table selector (0..3) */
	uint8 res;

/* These values are computed before starting a scan of the component. */
	int32 MCU_width;/* number of blocks per MCU, horizontally */
	int32 MCU_height;/* number of blocks per MCU, vertically */
} jpeg_component_info;
#endif
