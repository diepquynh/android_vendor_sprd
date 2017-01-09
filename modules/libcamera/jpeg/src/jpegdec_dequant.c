/******************************************************************************
 ** File Name:      JpegDec_dequant.c                                            *
 ** Author:         yi.wang													  *
 ** DATE:           07/12/2007                                                *
 ** Copyright:      2007 Spreadtrum, Incoporated. All Rights Reserved.        *
 ** Description:    Initialize the encoder									  *
 ** Note:           None                                                      *
******************************************************************************/
/******************************************************************************
 **                        Edit History                                       *
 ** ------------------------------------------------------------------------- *
 ** DATE           NAME             DESCRIPTION                               *
 ** 07/12/2007     yi.wang	         Create.                                  *
******************************************************************************/
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

//#if defined(JPEG_DEC)
//////////////////////////////////////////////////////////////////////////

uint8  *jpeg_fw_quant_tbl[2];
uint16 *jpeg_fw_quant_tbl_new[2];

PUBLIC void JPEGFW_InitQuantTbl(JPEG_QUALITY_E level)
{
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();

	SCI_ASSERT(jpeg_fw_codec != PNULL);

	jpeg_fw_codec->quant_tbl[JPEG_FW_LUM_ID] = jpeg_fw_lum_quant_tbl_default[level];
	jpeg_fw_codec->quant_tbl[JPEG_FW_CHR_ID] = jpeg_fw_chr_quant_tbl_default[level];

	jpeg_fw_codec->tbl_map[JPEG_FW_Y_ID].quant_tbl_id = JPEG_FW_LUM_ID;
	jpeg_fw_codec->tbl_map[JPEG_FW_U_ID].quant_tbl_id = JPEG_FW_CHR_ID;
	jpeg_fw_codec->tbl_map[JPEG_FW_V_ID].quant_tbl_id = JPEG_FW_CHR_ID;
}

static const int16 aanscales[DCTSIZE2] =
{
	/* precomputed values scaled up by 14 bits */
	16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520,
	22725, 31521, 29692, 26722, 22725, 17855, 12299,  6270,
	21407, 29692, 27969, 25172, 21407, 16819, 11585,  5906,
	19266, 26722, 25172, 22654, 19266, 15137, 10426,  5315,
	16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520,
	12873, 17855, 16819, 15137, 12873, 10114,  6967,  3552,
	8867, 12299, 11585, 10426,  8867,  6967,  4799,  2446,
	4520,  6270,  5906,  5315,  4520,  3552,  2446,  1247
};

#define MULTIPLY16V16(var1,var2)  ((var1) * (var2))

/* Dequantize a coefficient by multiplying it by the multiplier-table
* entry; produce an int result.  In this module, both inputs and result
* are 16 bits or less, so either int or short multiply will work.
*/
#define DEQUANTIZE(coef,quantval)  (((int32) (coef)) * (quantval))

#define LQ_CONST_BITS  8
#define LQ_PASS1_BITS  2

#define ONE	((int32) 1)
#define RIGHT_SHIFT(x,shft)  ((x) >> (shft))
#define IRIGHT_SHIFT(x,shft) ((x) >> (shft))

#define DESCALE(x,n)   ((int32)  RIGHT_SHIFT((x) + (ONE << ((n)-1)), n))
#define IDESCALE(x,n)  ((int32) IRIGHT_SHIFT((x) + (ONE << ((n)-1)), n))

#define IFAST_SCALE_BITS 12

PUBLIC JPEG_RET_E JPEGFW_AdjustQuantTbl_Dec()
{
	int32 time_out_flag = 0;
	uint8 tbl_id = 0;
	uint8 tbl_num = 0;
	uint32 qtable_addr = (uint32)INV_QUANT_TBL_ADDR;
	uint32 cmd = 0;
	int32 tmp = 0;
	int32 index1 = 0, index2 = 0;
	uint16 yuv_id = 0;
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();

	SCI_ASSERT(jpeg_fw_codec != PNULL);

	if(jpeg_fw_codec->num_components == 1)
	{
		tbl_num = 1;
	}else
	{
		tbl_num = 2;
	}

	if(!jpeg_fw_codec->progressive_mode)
	{
	        cmd = JPG_READ_REG(JPG_GLB_REG_BASE+GLB_CTRL_OFFSET, "DCAM_CFG: allow software to access the vsp buffer");
	        cmd |= (1<<2);
	        JPG_WRITE_REG(JPG_GLB_REG_BASE+GLB_CTRL_OFFSET, cmd, "DCAM_CFG: allow software to access the vsp buffer");
	        time_out_flag = JPG_READ_REG_POLL(JPG_GLB_REG_BASE+GLB_CTRL_OFFSET, (1<<8), (1<<8), TIME_OUT_CLK, "DCAM_CFG: polling dcam clock status");

		if(time_out_flag != 0)
		{
			SCI_TRACE_LOW("time_out_flag %s,%d",__FUNCTION__,__LINE__);
			return JPEG_FAILED;
		}

		for(tbl_id = 0; tbl_id < tbl_num; tbl_id++)
		{
			const uint8  *quant_tbl_current = PNULL;
			uint32 i = 0;

			yuv_id = jpeg_fw_codec->tbl_map[tbl_id].quant_tbl_id;
			quant_tbl_current = jpeg_fw_codec->quant_tbl[yuv_id];

			for(i=0; i < JPEG_FW_DCTSIZE2; i+=2)
			{
				index1 = jpeg_fw_ASIC_DCT_Matrix[i];
				index2 = jpeg_fw_ASIC_DCT_Matrix[i+1];	/*lint !e661 */

				tmp = ((quant_tbl_current[index1] & 0xFFFF) | (quant_tbl_current[index2]<<16));
				JPG_WRITE_REG(qtable_addr, tmp, "INV_QUANT_TAB_ADDR: Write Qtalbe into Qbuffer");
				qtable_addr += 4;
			#if _CMODEL_
				{
				//	FILE *qfile = fopen("D:/SC6800H/code/Firmware/jpeg_codec/simulation/VC/Dec_WinPrj/trace/dct/qtable.txt", "ab+");
				//	fprintf_oneWord_hex(qfile, tmp);
				//	fclose(qfile);
				}
			#endif//_CMODEL_
			}
		}

        	cmd = JPG_READ_REG(JPG_GLB_REG_BASE+GLB_CTRL_OFFSET, "DCAM_CFG: allow hardware to access the vsp buffer");
	        cmd = (cmd & ~0x4) ;
	        JPG_WRITE_REG(JPG_GLB_REG_BASE+GLB_CTRL_OFFSET, cmd, "DCAM_CFG: allow hardware to access the vsp buffer");
	        time_out_flag = JPG_READ_REG_POLL (JPG_GLB_REG_BASE+GLB_CTRL_OFFSET, 1, 1, TIME_OUT_CLK, "DCAM_CFG: polling dcam clock status");

		if(time_out_flag != 0)
		{
			time_out_flag = JPG_READ_REG (JPG_GLB_REG_BASE+GLB_CTRL_OFFSET, "DCAM_CFG: allow hardware to access the vsp buffer");
			SCI_TRACE_LOW("time_out_flag %s,%d %x",__FUNCTION__,__LINE__,time_out_flag);
			return JPEG_FAILED;
		}
	}else
	{
		JPEG_PROGRESSIVE_INFO_T *progressive_info = JPEGFW_GetProgInfo();
		int32 i;

		progressive_info->quant_tbl_new[0] = (int32 *)JpegDec_ExtraMemAlloc(sizeof(int32)*64);
		progressive_info->quant_tbl_new[1] = (int32 *)JpegDec_ExtraMemAlloc(sizeof(int32)*64);

		for(tbl_id = 0; tbl_id < tbl_num; tbl_id++)
		{
			const uint8 *quant;
			yuv_id = jpeg_fw_codec->tbl_map[tbl_id].quant_tbl_id;

			quant = jpeg_fw_codec->quant_tbl[yuv_id];

			if(progressive_info->low_quality_idct)
			{
				for(i = 0; i < JPEG_FW_DCTSIZE2; i++)
				{
					progressive_info->quant_tbl_new[tbl_id][i] =
						(int32)DESCALE(MULTIPLY16V16((int32)quant[i/*jpeg_fw_zigzag_tbl[i]*/], (int32)aanscales[i]), IFAST_SCALE_BITS);
				}
			}else
			{
				for(i = 0; i < JPEG_FW_DCTSIZE2; i++)
				{
					progressive_info->quant_tbl_new[tbl_id][jpeg_fw_zigzag_order[i]] = (int32)quant[i];
				}
			}
		}
	}

	return JPEG_SUCCESS;
}

//for clip opteration
uint8			s_iclip[1024];
uint8			*s_pClip_table;

/************************************************************************/
/*                                                                      */
/************************************************************************/
PUBLIC void Initialize_Clip()
{
	short i = 0;
	s_pClip_table = s_iclip+512;
	for (i= -512; i<512; i++)
		s_pClip_table[i] = (i<0) ? 0 : ((i>255) ? 255 : i);
}



/* Some C compilers fail to reduce "FIX(constant)" at compile time, thus
* causing a lot of useless floating-point operations at run time.
* To get around this we use the following pre-calculated constants.
* If you change CONST_BITS you may want to add appropriate values.
* (With a reasonable C compiler, you can just rely on the FIX() macro...)
*/
#define LQ_FIX_1_082392200  ((int32)  277)		/* FIX(1.082392200) */
#define LQ_FIX_1_414213562  ((int32)  362)		/* FIX(1.414213562) */
#define LQ_FIX_1_847759065  ((int32)  473)		/* FIX(1.847759065) */
#define LQ_FIX_2_613125930  ((int32)  669)		/* FIX(2.613125930) */

/* Multiply a int32 variable by an int32 constant, and immediately
* descale to yield a int32 result.
*/
#define LQ_MULTIPLY(var,const)  ((int32) DESCALE((var) * (const), LQ_CONST_BITS))

void JPEG_SWIDCT_LOW_Quality(int16 *coef_block, uint8 *output_buf, const int32 *quantptr)
{
	int32 tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
	int32 tmp10, tmp11, tmp12, tmp13;
	int32 z5, z10, z11, z12, z13;
	int16 *inptr;
	int32 *wsptr;
	uint8 *outptr;
	int32 ctr;
	int32 workspace[DCTSIZE2]; /* buffers data between passes */

	/* Pass 1: process columns from input, store into work array. */
	inptr = coef_block;
	wsptr = workspace;

	for(ctr = DCTSIZE; ctr > 0; ctr--)
	{
	/* Due to quantization, we will usually find that many of the input
     * coefficients are zero, especially the AC terms.  We can exploit this
     * by short-circuiting the IDCT calculation for any column in which all
     * the AC terms are zero.  In that case each output is equal to the
     * DC coefficient (with scale factor as needed).
     * With typical images and quantization tables, half or more of the
     * column DCT calculations can be simplified this way.
     */
		if(inptr[DCTSIZE*1] == 0 && inptr[DCTSIZE*2] ==0 &&
			inptr[DCTSIZE*3] == 0 && inptr[DCTSIZE*4] ==0 &&
			inptr[DCTSIZE*5] == 0 && inptr[DCTSIZE*6] ==0 &&
			inptr[DCTSIZE*7] == 0)
		{
			 /* AC terms all zero */
			int32 dcval = (int32)DEQUANTIZE(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]);

			wsptr[DCTSIZE*0] = dcval; wsptr[DCTSIZE*1] = dcval;
			wsptr[DCTSIZE*2] = dcval; wsptr[DCTSIZE*3] = dcval;
			wsptr[DCTSIZE*4] = dcval; wsptr[DCTSIZE*5] = dcval;
			wsptr[DCTSIZE*6] = dcval; wsptr[DCTSIZE*7] = dcval;

			inptr++;/* advance pointers to next column */
			quantptr++;
			wsptr++;

			continue;
		}

		 /* Even part */
		tmp0 = DEQUANTIZE(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]);
		tmp1 = DEQUANTIZE(inptr[DCTSIZE*2], quantptr[DCTSIZE*2]);
		tmp2 = DEQUANTIZE(inptr[DCTSIZE*4], quantptr[DCTSIZE*4]);
		tmp3 = DEQUANTIZE(inptr[DCTSIZE*6], quantptr[DCTSIZE*6]);

		tmp10 = tmp0 + tmp2;/* phase 3 */
		tmp11 = tmp0 - tmp2;

		tmp13 = tmp1 + tmp3;/* phases 5-3 */
		tmp12 = LQ_MULTIPLY(tmp1 - tmp3, LQ_FIX_1_414213562) - tmp13;/* 2*c4 */

		tmp0 = tmp10 + tmp13;/* phase 2 */
		tmp3 = tmp10 - tmp13;
		tmp1 = tmp11 + tmp12;
		tmp2 = tmp11 - tmp12;

		/* Odd part */
		tmp4 = DEQUANTIZE(inptr[DCTSIZE*1], quantptr[DCTSIZE*1]);
		tmp5 = DEQUANTIZE(inptr[DCTSIZE*3], quantptr[DCTSIZE*3]);
		tmp6 = DEQUANTIZE(inptr[DCTSIZE*5], quantptr[DCTSIZE*5]);
		tmp7 = DEQUANTIZE(inptr[DCTSIZE*7], quantptr[DCTSIZE*7]);

		z13 = tmp6 + tmp5;/* phase 6 */
		z10 = tmp6 - tmp5;
		z11 = tmp4 + tmp7;
		z12 = tmp4 - tmp7;

		tmp7 = z11 + z13;/* phase 5 */
		tmp11 = LQ_MULTIPLY(z11 - z13, LQ_FIX_1_414213562);/* 2*c4 */

		z5 = LQ_MULTIPLY(z10 + z12, LQ_FIX_1_847759065);/* 2*c2 */
		tmp10 = LQ_MULTIPLY(z12, LQ_FIX_1_082392200) - z5;/* 2*(c2-c6) */
		tmp12 = LQ_MULTIPLY(z10, -LQ_FIX_2_613125930) + z5;/* -2*(c2+c6) */

		tmp6 = tmp12 - tmp7;/* phase 2 */
		tmp5 = tmp11 - tmp6;
		tmp4 = tmp10 + tmp5;

		wsptr[DCTSIZE*0] = (int32)(tmp0 + tmp7);
		wsptr[DCTSIZE*7] = (int32)(tmp0 - tmp7);
		wsptr[DCTSIZE*1] = (int32)(tmp1 + tmp6);
		wsptr[DCTSIZE*6] = (int32)(tmp1 - tmp6);
		wsptr[DCTSIZE*2] = (int32)(tmp2 + tmp5);
		wsptr[DCTSIZE*5] = (int32)(tmp2 - tmp5);
		wsptr[DCTSIZE*4] = (int32)(tmp3 + tmp4);
		wsptr[DCTSIZE*3] = (int32)(tmp3 - tmp4);

		inptr++;/* advance pointers to next column */
		quantptr++;
		wsptr++;
	}

	/* Pass 2: process rows from work array, store into output array. */
	/* Note that we must descale the results by a factor of 8 == 2**3, */
	/* and also undo the PASS1_BITS scaling. */
	wsptr = workspace;
	for(ctr = 0; ctr < DCTSIZE; ctr++)
	{
		outptr = output_buf + ctr * 8;
		/* Rows of zeroes can be exploited in the same way as we did with columns.
     * However, the column calculation has created many nonzero AC terms, so
     * the simplification applies less often (typically 5% to 10% of the time).
     * On machines with very fast multiplication, it's possible that the
     * test takes more time than it's worth.  In that case this section
     * may be commented out.
     */
		if(wsptr[1] == 0 && wsptr[2] == 0 && wsptr[3] == 0 && wsptr[4] == 0 &&
			wsptr[5] == 0 && wsptr[6] == 0 && wsptr[7] == 0)
		{
			/* AC terms all zero */
			uint8 dcval = s_pClip_table[IDESCALE(wsptr[0], LQ_PASS1_BITS+3)+128];

			outptr[0] = dcval;	outptr[1] = dcval;
			outptr[2] = dcval;	outptr[3] = dcval;
			outptr[4] = dcval;	outptr[5] = dcval;
			outptr[6] = dcval;	outptr[7] = dcval;

			wsptr += DCTSIZE;/* advance pointer to next row */
			continue;
		}

		/* Even part */
		tmp10 = ((int32)wsptr[0] + (int32)wsptr[4]);
		tmp11 = ((int32)wsptr[0] - (int32)wsptr[4]);

		tmp13 = ((int32)wsptr[2] + (int32)wsptr[6]);
		tmp12 = LQ_MULTIPLY((int32)wsptr[2] - (int32)wsptr[6], LQ_FIX_1_414213562) - tmp13;

		tmp0 = tmp10 + tmp13;
		tmp3 = tmp10 - tmp13;
		tmp1 = tmp11 + tmp12;
		tmp2 = tmp11 - tmp12;

		/* Odd part */
		z13 = (int32)wsptr[5] + (int32)wsptr[3];
		z10 = (int32)wsptr[5] - (int32)wsptr[3];
		z11 = (int32)wsptr[1] + (int32)wsptr[7];
		z12 = (int32)wsptr[1] - (int32)wsptr[7];

		tmp7 = z11 + z13;/* phase 5 */
		tmp11 = LQ_MULTIPLY(z11 - z13, LQ_FIX_1_414213562); /* 2*c4 */

		z5 = LQ_MULTIPLY(z10 + z12, LQ_FIX_1_847759065);/* 2*c2 */
		tmp10 = LQ_MULTIPLY(z12, LQ_FIX_1_082392200) - z5;/* 2*(c2-c6) */
		tmp12 = LQ_MULTIPLY(z10, -LQ_FIX_2_613125930) + z5;/* -2*(c2+c6) */

		tmp6 = tmp12 - tmp7;/* phase 2 */
		tmp5 = tmp11 - tmp6;
		tmp4 = tmp10 + tmp5;

		/* Final output stage: scale down by a factor of 8 and range-limit */
		outptr[0] = s_pClip_table[IDESCALE(tmp0 + tmp7, LQ_PASS1_BITS+3)+128];
		outptr[7] = s_pClip_table[IDESCALE(tmp0 - tmp7, LQ_PASS1_BITS+3)+128];
		outptr[1] = s_pClip_table[IDESCALE(tmp1 + tmp6, LQ_PASS1_BITS+3)+128];
		outptr[6] = s_pClip_table[IDESCALE(tmp1 - tmp6, LQ_PASS1_BITS+3)+128];
		outptr[2] = s_pClip_table[IDESCALE(tmp2 + tmp5, LQ_PASS1_BITS+3)+128];
		outptr[5] = s_pClip_table[IDESCALE(tmp2 - tmp5, LQ_PASS1_BITS+3)+128];
		outptr[4] = s_pClip_table[IDESCALE(tmp3 + tmp4, LQ_PASS1_BITS+3)+128];
		outptr[3] = s_pClip_table[IDESCALE(tmp3 - tmp4, LQ_PASS1_BITS+3)+128];

		wsptr += DCTSIZE;/* advance pointer to next row */
	}
}

#define HQ_CONST_BITS  13
#define HQ_PASS1_BITS  2
/* Some C compilers fail to reduce "FIX(constant)" at compile time, thus
* causing a lot of useless floating-point operations at run time.
* To get around this we use the following pre-calculated constants.
* If you change CONST_BITS you may want to add appropriate values.
* (With a reasonable C compiler, you can just rely on the FIX() macro...)
*/
#define HQ_FIX_0_298631336  ((int32)  2446)	/* FIX(0.298631336) */
#define HQ_FIX_0_390180644  ((int32)  3196)	/* FIX(0.390180644) */
#define HQ_FIX_0_541196100  ((int32)  4433)	/* FIX(0.541196100) */
#define HQ_FIX_0_765366865  ((int32)  6270)	/* FIX(0.765366865) */
#define HQ_FIX_0_899976223  ((int32)  7373)	/* FIX(0.899976223) */
#define HQ_FIX_1_175875602  ((int32)  9633)	/* FIX(1.175875602) */
#define HQ_FIX_1_501321110  ((int32)  12299)	/* FIX(1.501321110) */
#define HQ_FIX_1_847759065  ((int32)  15137)	/* FIX(1.847759065) */
#define HQ_FIX_1_961570560  ((int32)  16069)	/* FIX(1.961570560) */
#define HQ_FIX_2_053119869  ((int32)  16819)	/* FIX(2.053119869) */
#define HQ_FIX_2_562915447  ((int32)  20995)	/* FIX(2.562915447) */
#define HQ_FIX_3_072711026  ((int32)  25172)	/* FIX(3.072711026) */

#define MULTIPLY16C16(var,const)  ((var) * (const))

/* Multiply an int32 variable by an int32 constant to yield an int32 result.
* For 8-bit samples with the recommended scaling, all the variable
* and constant values involved are no more than 16 bits wide, so a
* 16x16->32 bit multiply can be used instead of a full 32x32 multiply.
* For 12-bit samples, a full 32-bit multiplication will be needed.
*/
#define HQ_MULTIPLY(var,const)  MULTIPLY16C16(var,const)

void JPEG_SWIDCT_High_Quality(int16 *coef_block, uint8 *output_buf, const int32 *quantptr)
{
	int32 tmp0, tmp1, tmp2, tmp3;
	int32 tmp10, tmp11, tmp12, tmp13;
	int32 z1, z2, z3, z4, z5;
	int16 *inptr;
	int32 *wsptr;
	uint8 *outptr;
	int32 ctr;
	int32 workspace[DCTSIZE2]; /* buffers data between passes */

	/* Pass 1: process columns from input, store into work array. */
	/* Note results are scaled up by sqrt(8) compared to a true IDCT; */
	/* furthermore, we scale the results by 2**PASS1_BITS. */
	inptr = coef_block;
	wsptr = workspace;

	for(ctr = DCTSIZE; ctr > 0; ctr--)
	{
		/* Due to quantization, we will usually find that many of the input
		 * coefficients are zero, especially the AC terms.  We can exploit this
		 * by short-circuiting the IDCT calculation for any column in which all
		 * the AC terms are zero.  In that case each output is equal to the
		 * DC coefficient (with scale factor as needed).
		 * With typical images and quantization tables, half or more of the
		 * column DCT calculations can be simplified this way.
		 */
		if(inptr[DCTSIZE*1] == 0 && inptr[DCTSIZE*2] ==0 &&
			inptr[DCTSIZE*3] == 0 && inptr[DCTSIZE*4] ==0 &&
			inptr[DCTSIZE*5] == 0 && inptr[DCTSIZE*6] ==0 &&
			inptr[DCTSIZE*7] == 0)
		{
			 /* AC terms all zero */
			int32 dcval = DEQUANTIZE(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]) << HQ_PASS1_BITS;

			wsptr[DCTSIZE*0] = dcval; wsptr[DCTSIZE*1] = dcval;
			wsptr[DCTSIZE*2] = dcval; wsptr[DCTSIZE*3] = dcval;
			wsptr[DCTSIZE*4] = dcval; wsptr[DCTSIZE*5] = dcval;
			wsptr[DCTSIZE*6] = dcval; wsptr[DCTSIZE*7] = dcval;

			inptr++;/* advance pointers to next column */
			quantptr++;
			wsptr++;

			continue;
		}

		/* Even part: reverse the even part of the forward DCT. */
		/* The rotator is sqrt(2)*c(-6). */
		z2 = DEQUANTIZE(inptr[DCTSIZE*2], quantptr[DCTSIZE*2]);
		z3 = DEQUANTIZE(inptr[DCTSIZE*6], quantptr[DCTSIZE*6]);

		z1 = HQ_MULTIPLY(z2 + z3, HQ_FIX_0_541196100);
		tmp2 = z1 + HQ_MULTIPLY(z3, -HQ_FIX_1_847759065);
		tmp3 = z1 + HQ_MULTIPLY(z2, HQ_FIX_0_765366865);

		z2 = DEQUANTIZE(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]);
		z3 = DEQUANTIZE(inptr[DCTSIZE*4], quantptr[DCTSIZE*4]);

		tmp0 = (z2 + z3) << HQ_CONST_BITS;
		tmp1 = (z2 - z3) << HQ_CONST_BITS;

		tmp10 = tmp0 + tmp3;
		tmp13 = tmp0 - tmp3;
		tmp11 = tmp1 + tmp2;
		tmp12 = tmp1 - tmp2;

		/* Odd part per figure 8; the matrix is unitary and hence its
		 * transpose is its inverse.  i0..i3 are y7,y5,y3,y1 respectively.
		 */
		tmp0 = DEQUANTIZE(inptr[DCTSIZE*7], quantptr[DCTSIZE*7]);
		tmp1 = DEQUANTIZE(inptr[DCTSIZE*5], quantptr[DCTSIZE*5]);
		tmp2 = DEQUANTIZE(inptr[DCTSIZE*3], quantptr[DCTSIZE*3]);
		tmp3 = DEQUANTIZE(inptr[DCTSIZE*1], quantptr[DCTSIZE*1]);

		z1 = tmp0 + tmp3;
		z2 = tmp1 + tmp2;
		z3 = tmp0 + tmp2;
		z4 = tmp1 + tmp3;
		z5 = HQ_MULTIPLY(z3 + z4, HQ_FIX_1_175875602);  /* sqrt(2) * c3 */

		tmp0 = HQ_MULTIPLY(tmp0, HQ_FIX_0_298631336); /* sqrt(2) * (-c1+c3+c5-c7) */
		tmp1 = HQ_MULTIPLY(tmp1, HQ_FIX_2_053119869); /* sqrt(2) * ( c1+c3-c5+c7) */
		tmp2 = HQ_MULTIPLY(tmp2, HQ_FIX_3_072711026); /* sqrt(2) * ( c1+c3+c5-c7) */
		tmp3 = HQ_MULTIPLY(tmp3, HQ_FIX_1_501321110); /* sqrt(2) * ( c1+c3-c5-c7) */
		z1 = HQ_MULTIPLY(z1, - HQ_FIX_0_899976223); /* sqrt(2) * (c7-c3) */
		z2 = HQ_MULTIPLY(z2, - HQ_FIX_2_562915447); /* sqrt(2) * (-c1-c3) */
		z3 = HQ_MULTIPLY(z3, - HQ_FIX_1_961570560); /* sqrt(2) * (-c3-c5) */
		z4 = HQ_MULTIPLY(z4, - HQ_FIX_0_390180644); /* sqrt(2) * (c5-c3) */

		z3 += z5;
		z4 += z5;

		tmp0 += (z1 + z3);
		tmp1 += (z2 + z4);
		tmp2 += (z2 + z3);
		tmp3 += (z1 + z4);

		/* Final output stage: inputs are tmp10..tmp13, tmp0..tmp3 */
		wsptr[DCTSIZE*0] = (int32) IDESCALE(tmp10 + tmp3, HQ_CONST_BITS-HQ_PASS1_BITS);
		wsptr[DCTSIZE*7] = (int32) IDESCALE(tmp10 - tmp3, HQ_CONST_BITS-HQ_PASS1_BITS);
		wsptr[DCTSIZE*1] = (int32) IDESCALE(tmp11 + tmp2, HQ_CONST_BITS-HQ_PASS1_BITS);
		wsptr[DCTSIZE*6] = (int32) IDESCALE(tmp11 - tmp2, HQ_CONST_BITS-HQ_PASS1_BITS);
		wsptr[DCTSIZE*2] = (int32) IDESCALE(tmp12 + tmp1, HQ_CONST_BITS-HQ_PASS1_BITS);
		wsptr[DCTSIZE*5] = (int32) IDESCALE(tmp12 - tmp1, HQ_CONST_BITS-HQ_PASS1_BITS);
		wsptr[DCTSIZE*3] = (int32) IDESCALE(tmp13 + tmp0, HQ_CONST_BITS-HQ_PASS1_BITS);
		wsptr[DCTSIZE*4] = (int32) IDESCALE(tmp13 - tmp0, HQ_CONST_BITS-HQ_PASS1_BITS);

		inptr++;/* advance pointers to next column */
		quantptr++;
		wsptr++;
	}

	/* Pass 2: process rows from work array, store into output array. */
	/* Note that we must descale the results by a factor of 8 == 2**3, */
	/* and also undo the PASS1_BITS scaling. */
	wsptr = workspace;
	for(ctr = 0; ctr < DCTSIZE; ctr++)
	{
		outptr = output_buf + ctr * 8;
		/* Rows of zeroes can be exploited in the same way as we did with columns.
		 * However, the column calculation has created many nonzero AC terms, so
		 * the simplification applies less often (typically 5% to 10% of the time).
		 * On machines with very fast multiplication, it's possible that the
		 * test takes more time than it's worth.  In that case this section
		 * may be commented out.
		 */
		if(wsptr[1] == 0 && wsptr[2] == 0 && wsptr[3] == 0 && wsptr[4] == 0 &&
			wsptr[5] == 0 && wsptr[6] == 0 && wsptr[7] == 0)
		{
			/* AC terms all zero */
			uint8 dcval = s_pClip_table[IDESCALE((int32)wsptr[0], HQ_PASS1_BITS+3)+128];

			outptr[0] = dcval;	outptr[1] = dcval;
			outptr[2] = dcval;	outptr[3] = dcval;
			outptr[4] = dcval;	outptr[5] = dcval;
			outptr[6] = dcval;	outptr[7] = dcval;

			wsptr += DCTSIZE;/* advance pointer to next row */
			continue;
		}

		/* Even part: reverse the even part of the forward DCT. */
		/* The rotator is sqrt(2)*c(-6). */
		z2 = (int32) wsptr[2];
		z3 = (int32) wsptr[6];

		z1 = HQ_MULTIPLY(z2 + z3, HQ_FIX_0_541196100);
		tmp2 = z1 + HQ_MULTIPLY(z3, - HQ_FIX_1_847759065);
		tmp3 = z1 + HQ_MULTIPLY(z2, HQ_FIX_0_765366865);

		tmp0 = ((int32) wsptr[0] + (int32) wsptr[4]) << HQ_CONST_BITS;
		tmp1 = ((int32) wsptr[0] - (int32) wsptr[4]) << HQ_CONST_BITS;

		tmp10 = tmp0 + tmp3;
		tmp13 = tmp0 - tmp3;
		tmp11 = tmp1 + tmp2;
		tmp12 = tmp1 - tmp2;

		/* Odd part per figure 8; the matrix is unitary and hence its
		 * transpose is its inverse.  i0..i3 are y7,y5,y3,y1 respectively.
		 */

		tmp0 = (int32) wsptr[7];
		tmp1 = (int32) wsptr[5];
		tmp2 = (int32) wsptr[3];
		tmp3 = (int32) wsptr[1];

		z1 = tmp0 + tmp3;
		z2 = tmp1 + tmp2;
		z3 = tmp0 + tmp2;
		z4 = tmp1 + tmp3;
		z5 = HQ_MULTIPLY(z3 + z4, HQ_FIX_1_175875602); /* sqrt(2) * c3 */

		tmp0 = HQ_MULTIPLY(tmp0, HQ_FIX_0_298631336); /* sqrt(2) * (-c1+c3+c5-c7) */
		tmp1 = HQ_MULTIPLY(tmp1, HQ_FIX_2_053119869); /* sqrt(2) * ( c1+c3-c5+c7) */
		tmp2 = HQ_MULTIPLY(tmp2, HQ_FIX_3_072711026); /* sqrt(2) * ( c1+c3+c5-c7) */
		tmp3 = HQ_MULTIPLY(tmp3, HQ_FIX_1_501321110); /* sqrt(2) * ( c1+c3-c5-c7) */
		z1 = HQ_MULTIPLY(z1, - HQ_FIX_0_899976223); /* sqrt(2) * (c7-c3) */
		z2 = HQ_MULTIPLY(z2, - HQ_FIX_2_562915447); /* sqrt(2) * (-c1-c3) */
		z3 = HQ_MULTIPLY(z3, - HQ_FIX_1_961570560); /* sqrt(2) * (-c3-c5) */
		z4 = HQ_MULTIPLY(z4, - HQ_FIX_0_390180644); /* sqrt(2) * (c5-c3) */

		z3 += z5;
		z4 += z5;

		tmp0 += z1 + z3;
		tmp1 += z2 + z4;
		tmp2 += z2 + z3;
		tmp3 += z1 + z4;

		/* Final output stage: inputs are tmp10..tmp13, tmp0..tmp3 */

		outptr[0] = s_pClip_table[IDESCALE(tmp10 + tmp3, HQ_CONST_BITS+HQ_PASS1_BITS+3)+128];
		outptr[7] = s_pClip_table[IDESCALE(tmp10 - tmp3, HQ_CONST_BITS+HQ_PASS1_BITS+3)+128];
		outptr[1] = s_pClip_table[IDESCALE(tmp11 + tmp2, HQ_CONST_BITS+HQ_PASS1_BITS+3)+128];
		outptr[6] = s_pClip_table[IDESCALE(tmp11 - tmp2, HQ_CONST_BITS+HQ_PASS1_BITS+3)+128];
		outptr[2] = s_pClip_table[IDESCALE(tmp12 + tmp1, HQ_CONST_BITS+HQ_PASS1_BITS+3)+128];
		outptr[5] = s_pClip_table[IDESCALE(tmp12 - tmp1, HQ_CONST_BITS+HQ_PASS1_BITS+3)+128];
		outptr[3] = s_pClip_table[IDESCALE(tmp13 + tmp0, HQ_CONST_BITS+HQ_PASS1_BITS+3)+128];
		outptr[4] = s_pClip_table[IDESCALE(tmp13 - tmp0, HQ_CONST_BITS+HQ_PASS1_BITS+3)+128];

		wsptr += DCTSIZE;/* advance pointer to next row */
	}
}

#define CONST_BITS  13
#define PASS1_BITS  2

#define FIX_0_211164243  ((int32)  1730)	/* FIX(0.211164243) */
#define FIX_0_509795579  ((int32)  4176)	/* FIX(0.509795579) */
#define FIX_0_601344887  ((int32)  4926)	/* FIX(0.601344887) */
#define FIX_0_720959822  ((int32)  5906)	/* FIX(0.720959822) */
#define FIX_0_765366865  ((int32)  6270)	/* FIX(0.765366865) */
#define FIX_0_850430095  ((int32)  6967)	/* FIX(0.850430095) */
#define FIX_0_899976223  ((int32)  7373)	/* FIX(0.899976223) */
#define FIX_1_061594337  ((int32)  8697)	/* FIX(1.061594337) */
#define FIX_1_272758580  ((int32)  10426)	/* FIX(1.272758580) */
#define FIX_1_451774981  ((int32)  11893)	/* FIX(1.451774981) */
#define FIX_1_847759065  ((int32)  15137)	/* FIX(1.847759065) */
#define FIX_2_172734803  ((int32)  17799)	/* FIX(2.172734803) */
#define FIX_2_562915447  ((int32)  20995)	/* FIX(2.562915447) */
#define FIX_3_624509785  ((int32)  29692)	/* FIX(3.624509785) */

// #define MULTIPLY16C16(var,const)  (((int16) (var)) * ((int16) (const)))
#define MULTIPLY(var,const)  MULTIPLY16C16(var,const)

void JPEG_SWIDCT_4X4(int16 *coef_block, uint8 *output_buf, const int32 *quantptr)
{
	int32 tmp0, tmp2, tmp10, tmp12;
	int32 z1, z2, z3, z4;
	int16 *inptr;
	int32 * wsptr;
	uint8 *outptr;
	int32 ctr;
	int32 workspace[DCTSIZE*4];	/* buffers data between passes */

	/* Pass 1: process columns from input, store into work array. */

	inptr = coef_block;
	wsptr = workspace;
	for (ctr = DCTSIZE; ctr > 0; inptr++, quantptr++, wsptr++, ctr--)
	{
		/* Don't bother to process column 4, because second pass won't use it */
		if (ctr == DCTSIZE-4)
		  continue;
		if (inptr[DCTSIZE*1] == 0 && inptr[DCTSIZE*2] == 0 &&
			inptr[DCTSIZE*3] == 0 && inptr[DCTSIZE*5] == 0 &&
			inptr[DCTSIZE*6] == 0 && inptr[DCTSIZE*7] == 0)
		{
			/* AC terms all zero; we need not examine term 4 for 4x4 output */
			int dcval = DEQUANTIZE(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]) << PASS1_BITS;

			wsptr[DCTSIZE*0] = dcval;
			wsptr[DCTSIZE*1] = dcval;
			wsptr[DCTSIZE*2] = dcval;
			wsptr[DCTSIZE*3] = dcval;

			continue;
		}

		/* Even part */

		tmp0 = DEQUANTIZE(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]);
		tmp0 <<= (CONST_BITS+1);

		z2 = DEQUANTIZE(inptr[DCTSIZE*2], quantptr[DCTSIZE*2]);
		z3 = DEQUANTIZE(inptr[DCTSIZE*6], quantptr[DCTSIZE*6]);

		tmp2 = MULTIPLY(z2, FIX_1_847759065) + MULTIPLY(z3, -FIX_0_765366865);

		tmp10 = tmp0 + tmp2;
		tmp12 = tmp0 - tmp2;

		/* Odd part */

		z1 = DEQUANTIZE(inptr[DCTSIZE*7], quantptr[DCTSIZE*7]);
		z2 = DEQUANTIZE(inptr[DCTSIZE*5], quantptr[DCTSIZE*5]);
		z3 = DEQUANTIZE(inptr[DCTSIZE*3], quantptr[DCTSIZE*3]);
		z4 = DEQUANTIZE(inptr[DCTSIZE*1], quantptr[DCTSIZE*1]);

		tmp0 = MULTIPLY(z1, - FIX_0_211164243) /* sqrt(2) * (c3-c1) */
			+ MULTIPLY(z2, FIX_1_451774981) /* sqrt(2) * (c3+c7) */
			+ MULTIPLY(z3, - FIX_2_172734803) /* sqrt(2) * (-c1-c5) */
			+ MULTIPLY(z4, FIX_1_061594337); /* sqrt(2) * (c5+c7) */

		tmp2 = MULTIPLY(z1, - FIX_0_509795579) /* sqrt(2) * (c7-c5) */
			+ MULTIPLY(z2, - FIX_0_601344887) /* sqrt(2) * (c5-c1) */
			+ MULTIPLY(z3, FIX_0_899976223) /* sqrt(2) * (c3-c7) */
			+ MULTIPLY(z4, FIX_2_562915447); /* sqrt(2) * (c1+c3) */

		/* Final output stage */

		wsptr[DCTSIZE*0] = (int) DESCALE(tmp10 + tmp2, CONST_BITS-PASS1_BITS+1);
		wsptr[DCTSIZE*3] = (int) DESCALE(tmp10 - tmp2, CONST_BITS-PASS1_BITS+1);
		wsptr[DCTSIZE*1] = (int) DESCALE(tmp12 + tmp0, CONST_BITS-PASS1_BITS+1);
		wsptr[DCTSIZE*2] = (int) DESCALE(tmp12 - tmp0, CONST_BITS-PASS1_BITS+1);
	}

	/* Pass 2: process 4 rows from work array, store into output array. */

	wsptr = workspace;
	for (ctr = 0; ctr < 4; ctr++)
	{
		outptr = output_buf + ctr * 4;
		/* It's not clear whether a zero row test is worthwhile here ... */

		if (wsptr[1] == 0 && wsptr[2] == 0 && wsptr[3] == 0 &&
			wsptr[5] == 0 && wsptr[6] == 0 && wsptr[7] == 0)
		{
			/* AC terms all zero */
			uint8 dcval = s_pClip_table[(int32)DESCALE((int32) wsptr[0], PASS1_BITS+3)+128];

			outptr[0] = dcval;
			outptr[1] = dcval;
			outptr[2] = dcval;
			outptr[3] = dcval;

			wsptr += DCTSIZE;		/* advance pointer to next row */
			continue;
		}

		/* Even part */

		tmp0 = ((int32) wsptr[0]) << (CONST_BITS+1);

		tmp2 = MULTIPLY((int32) wsptr[2], FIX_1_847759065)
			+ MULTIPLY((int32) wsptr[6], - FIX_0_765366865);

		tmp10 = tmp0 + tmp2;
		tmp12 = tmp0 - tmp2;

		/* Odd part */

		z1 = (int32) wsptr[7];
		z2 = (int32) wsptr[5];
		z3 = (int32) wsptr[3];
		z4 = (int32) wsptr[1];

		tmp0 = MULTIPLY(z1, - FIX_0_211164243) /* sqrt(2) * (c3-c1) */
			+ MULTIPLY(z2, FIX_1_451774981) /* sqrt(2) * (c3+c7) */
			+ MULTIPLY(z3, - FIX_2_172734803) /* sqrt(2) * (-c1-c5) */
			+ MULTIPLY(z4, FIX_1_061594337); /* sqrt(2) * (c5+c7) */

		tmp2 = MULTIPLY(z1, - FIX_0_509795579) /* sqrt(2) * (c7-c5) */
			+ MULTIPLY(z2, - FIX_0_601344887) /* sqrt(2) * (c5-c1) */
			+ MULTIPLY(z3, FIX_0_899976223) /* sqrt(2) * (c3-c7) */
			+ MULTIPLY(z4, FIX_2_562915447); /* sqrt(2) * (c1+c3) */

		/* Final output stage */

		outptr[0] = s_pClip_table[(int32) DESCALE(tmp10 + tmp2, CONST_BITS+PASS1_BITS+3+1)+128];
		outptr[3] = s_pClip_table[(int32) DESCALE(tmp10 - tmp2, CONST_BITS+PASS1_BITS+3+1)+128];
		outptr[1] = s_pClip_table[(int32) DESCALE(tmp12 + tmp0, CONST_BITS+PASS1_BITS+3+1)+128];
		outptr[2] = s_pClip_table[(int32) DESCALE(tmp12 - tmp0, CONST_BITS+PASS1_BITS+3+1)+128];

		wsptr += DCTSIZE;		/* advance pointer to next row */
	}
}

void JPEG_SWIDCT_2X2(int16 *coef_block, uint8 *output_buf, const int32 *quantptr)
{
	int32 tmp0, tmp10, z1;
	int16 *inptr;
	int32 *wsptr;
	uint8 *outptr;
    int32 ctr;
	int32 workspace[DCTSIZE*2];	/* buffers data between passes */

	 /* Pass 1: process columns from input, store into work array. */

	inptr = coef_block;
	wsptr = workspace;
	for (ctr = DCTSIZE; ctr > 0; inptr++, quantptr++, wsptr++, ctr--)
	{
		/* Don't bother to process columns 2,4,6 */
		if (ctr == DCTSIZE-2 || ctr == DCTSIZE-4 || ctr == DCTSIZE-6)
			continue;
		if (inptr[DCTSIZE*1] == 0 && inptr[DCTSIZE*3] == 0 &&
			inptr[DCTSIZE*5] == 0 && inptr[DCTSIZE*7] == 0)
		{
			/* AC terms all zero; we need not examine terms 2,4,6 for 2x2 output */
			int32 dcval = DEQUANTIZE(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]) << PASS1_BITS;

			wsptr[DCTSIZE*0] = dcval;
			wsptr[DCTSIZE*1] = dcval;

			continue;
		}

		/* Even part */
		z1 = DEQUANTIZE(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]);
		tmp10 = z1 << (CONST_BITS+2);

		/* Odd part */
		z1 = DEQUANTIZE(inptr[DCTSIZE*7], quantptr[DCTSIZE*7]);
		tmp0 = MULTIPLY(z1, - FIX_0_720959822); /* sqrt(2) * (c7-c5+c3-c1) */
		z1 = DEQUANTIZE(inptr[DCTSIZE*5], quantptr[DCTSIZE*5]);
		tmp0 += MULTIPLY(z1, FIX_0_850430095); /* sqrt(2) * (-c1+c3+c5+c7) */
		z1 = DEQUANTIZE(inptr[DCTSIZE*3], quantptr[DCTSIZE*3]);
		tmp0 += MULTIPLY(z1, - FIX_1_272758580); /* sqrt(2) * (-c1+c3-c5-c7) */
		z1 = DEQUANTIZE(inptr[DCTSIZE*1], quantptr[DCTSIZE*1]);
		tmp0 += MULTIPLY(z1, FIX_3_624509785); /* sqrt(2) * (c1+c3+c5+c7) */

		/* Final output stage */
		wsptr[DCTSIZE*0] = (int) DESCALE(tmp10 + tmp0, CONST_BITS-PASS1_BITS+2);
		wsptr[DCTSIZE*1] = (int) DESCALE(tmp10 - tmp0, CONST_BITS-PASS1_BITS+2);
	}

	/* Pass 2: process 2 rows from work array, store into output array. */
	wsptr = workspace;
	for (ctr = 0; ctr < 2; ctr++)
	{
		outptr = output_buf + ctr * 2;
		/* It's not clear whether a zero row test is worthwhile here ... */
		if (wsptr[1] == 0 && wsptr[3] == 0 && wsptr[5] == 0 && wsptr[7] == 0)
		{
		  /* AC terms all zero */
		  uint8 dcval = s_pClip_table[(int32) DESCALE((int32) wsptr[0], PASS1_BITS+3)+128];

		  outptr[0] = dcval;
		  outptr[1] = dcval;

		  wsptr += DCTSIZE;		/* advance pointer to next row */
		  continue;
		}

		/* Even part */
        tmp10 = ((int32) wsptr[0]) << (CONST_BITS+2);

		/* Odd part */
	    tmp0 = MULTIPLY((int32) wsptr[7], - FIX_0_720959822) /* sqrt(2) * (c7-c5+c3-c1) */
			+ MULTIPLY((int32) wsptr[5], FIX_0_850430095) /* sqrt(2) * (-c1+c3+c5+c7) */
			+ MULTIPLY((int32) wsptr[3], - FIX_1_272758580) /* sqrt(2) * (-c1+c3-c5-c7) */
			+ MULTIPLY((int32) wsptr[1], FIX_3_624509785); /* sqrt(2) * (c1+c3+c5+c7) */

		/* Final output stage */
		outptr[0] = s_pClip_table[(int32) DESCALE(tmp10 + tmp0, CONST_BITS+PASS1_BITS+3+2)+128];
		outptr[1] = s_pClip_table[(int32) DESCALE(tmp10 - tmp0, CONST_BITS+PASS1_BITS+3+2)+128];

		wsptr += DCTSIZE;		/* advance pointer to next row */
	}
}

/*
 * Perform dequantization and inverse DCT on one block of coefficients,
 * producing a reduced-size 1x1 output block.
 */
void JPEG_SWIDCT_1X1(int16 *coef_block, uint8 *output_buf, const int32 *quantptr)
{
	int32 dcval;

	/* We hardly need an inverse DCT routine for this: just take the
   * average pixel value, which is one-eighth of the DC coefficient.
   */
	dcval = DEQUANTIZE(coef_block[0], quantptr[0]);
	dcval = (int32) DESCALE((int32) dcval, 3);

	output_buf[0]/*[0]*/ = s_pClip_table[dcval+128];
}

/************************************************************************/
/* Init the transform function                                          */
/************************************************************************/
PUBLIC void JPEGFW_InitTransFun(JPEG_PROGRESSIVE_INFO_T *progressive_info_ptr)
{
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();

	progressive_info_ptr->low_quality_idct = 0;
	if(jpeg_fw_codec->scale_factor == 0)
	{
		progressive_info_ptr->low_quality_idct = 1;
		progressive_info_ptr->jpeg_transform = JPEG_SWIDCT_LOW_Quality;
		progressive_info_ptr->DC_Diff = 8192;
	}else if(jpeg_fw_codec->scale_factor == 1)
	{
 		progressive_info_ptr->jpeg_transform = JPEG_SWIDCT_4X4;
	}else if(jpeg_fw_codec->scale_factor == 2)
	{
 		progressive_info_ptr->jpeg_transform = JPEG_SWIDCT_2X2;
	}else if(jpeg_fw_codec->scale_factor == 3)
	{
 		progressive_info_ptr->jpeg_transform = JPEG_SWIDCT_1X1;
	}
}

//////////////////////////////////////////////////////////////////////////
//#endif //JPEG_DEC
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End
