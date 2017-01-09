/******************************************************************************
 ** File Name:      JpegEnc_frame.c                                            *
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

#if _CMODEL_

void JPEG_DecodeMCU_Baseline(void)
{
	vld_module();
	dct_module();
	mbio_module();

	g_vld_reg_ptr->DC_Y = g_pre_dc_value[0]&0x7ff;
	g_vld_reg_ptr->DC_UV = ((g_pre_dc_value[2]&0x7ff)<<16) | (g_pre_dc_value[1]&0x7ff);
}

extern void MBIO_INT_PROC(void);

int32 slice_num = 0;
int32 START_SW_DECODE(JPEG_CODEC_T *JpegCodec, uint32 num_of_rows)
{
	uint16 x = 0, y = 0;
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();
	uint8 input_mcu_info = jpeg_fw_codec->input_mcu_info;
	uint8 scale_factor = jpeg_fw_codec->scale_factor;
	uint32 mcu_num_y = (num_of_rows / (JpegCodec->mcu_height));
	uint32 mcu_num_x = JpegCodec->mcu_num_x;
// 	JPEG_CallBack_MCU_To_Frame CopyMCUToCoeff;
	uint8 *y_coeff = JpegCodec->mbio_bfr0_valid ? JpegCodec->YUV_Info_0.y_data_ptr : JpegCodec->YUV_Info_1.y_data_ptr;
	uint8 *u_coeff = JpegCodec->mbio_bfr0_valid ? JpegCodec->YUV_Info_0.u_data_ptr : JpegCodec->YUV_Info_1.u_data_ptr;
	uint8 *v_coeff = JpegCodec->mbio_bfr0_valid ? JpegCodec->YUV_Info_0.v_data_ptr : JpegCodec->YUV_Info_1.v_data_ptr;

	if(g_int_yuv_buf0_full == TRUE)
	{
		//assert(g_int_yuv_buf1_full == FALSE);
		y_coeff = JpegCodec->YUV_Info_1.y_data_ptr;
		u_coeff = JpegCodec->YUV_Info_1.u_data_ptr;
		v_coeff = JpegCodec->YUV_Info_1.v_data_ptr;
	}

	if(JpegCodec->is_first_slice)
	{
 		Init_downsample_fun();
	}

	switch(input_mcu_info)
	{
	case JPEG_FW_YUV444:
	//	CopyMCUToCoeff = CopyMCUToCoeff444_UV;
		g_block_num_in_one_mcu = 3;
		break;
	case JPEG_FW_YUV422:
	//	CopyMCUToCoeff = CopyMCUToCoeff422_UV;
		g_block_num_in_one_mcu = 4;
		break;
	case JPEG_FW_YUV411:
	//	CopyMCUToCoeff = CopyMCUToCoeff411_UV;
		g_block_num_in_one_mcu = 6;
		break;
	case JPEG_FW_YUV420:
	//	CopyMCUToCoeff = CopyMCUToCoeff420_UV;
		g_block_num_in_one_mcu = 6;
		break;
	case JPEG_FW_YUV400:
	//	CopyMCUToCoeff = CopyMCUToCoeff400_UV;
		g_block_num_in_one_mcu = 1;
		break;
	case JPEG_FW_YUV422_R:
	//	CopyMCUToCoeff = CopyMCUToCoeff422_UV_R;
		g_block_num_in_one_mcu = 4;
		break;
	case JPEG_FW_YUV411_R:
	//	CopyMCUToCoeff = CopyMCUToCoeff411_UV_R;
		g_block_num_in_one_mcu = 6;
		break;
	default:
		return JPEG_FAILED;
	}

	JPEG_TRACE("\nJPEG Decoding MCU, slice_num:%d, total mcu_num:%d...\n", slice_num++, mcu_num_y*mcu_num_x);
//	FPRINTF(g_pf_debug, "\nJPEG Decoding MCU, slice_num:%d, total mcu_num:%d...\n", slice_num++, mcu_num_y*mcu_num_x);

	/*for every MCU do following*/
	for (y=0; y<mcu_num_y; y++)
	{
		JPEG_TRACE("MCU_Num_Y:%d\n", y);
//		FPRINTF(g_pf_debug, "MCU_Num_Y:%d\n", y);
		for (x = 0; x<mcu_num_x; x++)
		{
			g_mcuNum++;
	//		g_mcuNum = y*mcu_num_x + x;
	// 		FPRINTF(g_pfHuffVal_JPEGDec, "mcuNumber: %d, mcu_y: %d, mcu_x: %d\n", g_mcuNum, y, x);
	//		printf("jstream_words:%0x, jremain_bit_num:%0x\n", jstream_words,jremain_bit_num);
			if(y == 3 && x == 55 /*&& slice_num == 15*/)
			{
				PRINTF("");
				//fclose(g_pfHuffVal_JPEGDec);
			//	fclose(g_pf_debug);
			}

			if(g_mcuNum == 15364)
			{
				PRINTF("");
			//	goto jpeg_end;
			}
			/*restart interval*/
			if (jpeg_fw_codec->restart_interval)
			{

				if (jpeg_fw_codec->restart_to_go == 0)
				{
					if(InPutRstMarker() != 0)
						return -1;
					jpeg_fw_codec->next_restart_num += 1;
					jpeg_fw_codec->next_restart_num &= 0x07;
					jpeg_fw_codec->restart_to_go = jpeg_fw_codec->restart_interval;
					g_pre_dc_value[0] = 0;
					g_pre_dc_value[1] = 0;
					g_pre_dc_value[2] = 0;
				}
				jpeg_fw_codec->restart_to_go--;
			}

			//INIT ZERO OF ORG_BUF;
			memset(g_org_blocks[0], 0, JPEG_FW_DCTSIZE2 * 6);
			memset(g_blocks[0], 0, JPEG_FW_DCTSIZE2*2*6);

			//g_glb_reg_ptr->VSP_CTRL0 = (((y&0x3f)<<8) | (x&0x3f));
			g_mbc_reg_ptr->MB_START = (((y&0x3ff)<<16) | (x&0x3ff));
			JPEG_DecodeMCU_Baseline();

			//copy MCU data to coeff buffer,Added by wangyi 2007/05/02
// 			CopyMCUToCoeff((uint8*)y_coeff, (uint8*)u_coeff, (uint8*)v_coeff, x, y, scale_factor);

			//update interface
			g_vld_reg_ptr->RESTART_MCU_CNT = (jpeg_fw_codec->restart_interval - jpeg_fw_codec->restart_to_go);
		}
	}

// jpeg_end:
	if(num_of_rows < jpeg_fw_codec->out_height)
	{
		JPEG_TRACE("Slice decoding end...\n");
	}else
	{
		JPEG_TRACE("JPEG Decoding Finished...\n\n");
	}

	MBIO_INT_PROC();

	return JPEG_SUCCESS;
}
#endif//_CMODEL_

uint32 JpegDec_Read_nBits(uint32 nbits)
{
	uint32 val;

	if(0 == nbits)
	{
		return 0;
	}

#if _CMODEL_
	val = read_nbits(nbits);
#endif
	JPG_WRITE_REG (JPG_BSM_REG_BASE+BSM_CFG2_OFFSET, (nbits << 24) | (0<<0), "BSM_CFG2: configure show n bits");
	val = JPG_READ_REG (JPG_BSM_REG_BASE+BSM_RDATA_OFFSET, "BSM_RDATA: show n bits from bitstream");

	JPG_WRITE_REG(JPG_BSM_REG_BASE+BSM_CFG2_OFFSET, (nbits << 24) | (1<<0), "BSM_CFG2: configure flush n bits");
	return val;
}

PUBLIC JPEG_RET_E JpegDec_CheckRowNumValid(uint32 num_of_rows, uint32 input_mcu_info)
{
	if((JPEG_FW_YUV400 == input_mcu_info) || (JPEG_FW_YUV411 == input_mcu_info)
		|| (JPEG_FW_YUV422 == input_mcu_info) || (JPEG_FW_YUV444 == input_mcu_info))
	{
		if(0 != (num_of_rows & 0x7))//times of 8 if yuv400, 411, 422, 444
		{
			JPEG_TRACE("Invalid raw_height, must be interger times of 8 or 16...\n");
			return JPEG_FAILED;
		}
	}else if((JPEG_FW_YUV420 == input_mcu_info) || (JPEG_FW_YUV422_R == input_mcu_info))
	{
		if(0 != (num_of_rows & 0xf))//times of 16 if yuv420 or 422R
		{
			JPEG_TRACE("Invalid raw_height, must be interger times of 8 or 16...\n");
			return JPEG_FAILED;
		}
	}else //yuv411_R
	{
		if(0 != (num_of_rows & 0x1f))//times of 32 if yuv411R
		{
			JPEG_TRACE("Invalid raw_height, must be interger times of 8 or 16...\n");
			return JPEG_FAILED;
		}
	}

	return JPEG_SUCCESS;
}
uint8 s_bs_last[100];

#if 1//_CMODEL_ && !defined(_LIB) //for rtl simulation.
extern uint32 head_byte_len;
#endif

PUBLIC JPEG_RET_E JpegDec_SetFlushWordFlag(int32 flag)
{
	g_flush_word_count = flag;
	return JPEG_SUCCESS;
}

PUBLIC JPEG_RET_E START_HW_DECODE(JPEG_CODEC_T *jpeg_fw_codec, uint32 num_of_rows)
{
	uint32 cmd = 0;
	uint32 pingpang_buf_status = 0;
	uint32 slice_mcu_num_y = 0;
	uint32 total_slice_mcu_num = 0;//need decoded in this slice.
	uint32 vld_mcu_num = 0;
	uint32 global_mcu_num_y = 0;
	uint32 bytes_need_flush = 0;
	uint32 input_mcu_info = jpeg_fw_codec->input_mcu_info;
	JPEG_RET_E ret;

	ret = JpegDec_CheckRowNumValid(num_of_rows, input_mcu_info);

	if(ret != JPEG_SUCCESS)
	{
		return ret;
	}

	slice_mcu_num_y = num_of_rows / (jpeg_fw_codec->mcu_height);
	total_slice_mcu_num =  slice_mcu_num_y * (jpeg_fw_codec->mcu_num_x);

	if(SWITCH_MODE == jpeg_fw_codec->work_mode)
	{
		global_mcu_num_y = slice_mcu_num_y;
		vld_mcu_num = total_slice_mcu_num;
	}else //alone mode
	{
		global_mcu_num_y = jpeg_fw_codec->mcu_num_y;
		vld_mcu_num = (jpeg_fw_codec->mcu_num_x) * (jpeg_fw_codec->mcu_num_y);
	}

	//VSP_CFG1
	jpeg_fw_codec->uv_interleaved = 1;
	cmd = JPG_READ_REG(JPG_GLB_REG_BASE+MB_CFG_OFFSET,"MB_CFG_OFFSET");
	cmd |=(((!jpeg_fw_codec->uv_interleaved)<<27)) |(jpeg_fw_codec->input_mcu_info<<24) | ((global_mcu_num_y & 0x3ff) << 12) | (jpeg_fw_codec->mcu_num_x & 0x3ff);
	JPG_WRITE_REG(JPG_GLB_REG_BASE+MB_CFG_OFFSET, cmd, "MB_CFG: input mcu info, slice mcu number x and y");

	//BSM Module cfg
	pingpang_buf_status = ((jpeg_fw_codec->bsm_buf0_valid<<1) | (jpeg_fw_codec->bsm_buf1_valid));
	cmd = ((uint32)pingpang_buf_status<<30) | ((jpeg_fw_codec->pingpang_buf_len+3) >> 2);
	JPG_WRITE_REG(JPG_BSM_REG_BASE+BSM_CFG0_OFFSET, cmd, "BSM_CFG0: buffer0 for read, and the buffer size");
#if _CMODEL_
if((SWITCH_MODE == jpeg_fw_codec->work_mode) || (jpeg_fw_codec->is_first_slice && (ALONE_MODE == jpeg_fw_codec->work_mode)))
{
	g_bs_pingpang_bfr0 = jpeg_fw_codec->stream_0;
	g_bs_pingpang_bfr1 = jpeg_fw_codec->stream_1;
	init_bsm();
}
#endif

	//polling bsm status
	ret = JPG_READ_REG_POLL(JPG_BSM_REG_BASE+BSM_STS0_OFFSET, V_BIT_31, V_BIT_31, TIME_OUT_CLK, "BSM_DEBUG: polling bsm status");
    cmd = JPG_READ_REG(JPG_BSM_REG_BASE+BSM_STS0_OFFSET,"MB_CFG_OFFSET");
    SCI_TRACE_LOW("%s,%d BSM_STS0_OFFSET %x ret %d",__FUNCTION__,__LINE__,cmd,ret);
#if _CMODEL_ && !defined(_LIB) //for rtl simulation.
	bytes_need_flush = head_byte_len%4;
#endif

	//flush bytes
	flush_unalign_bytes(bytes_need_flush);

	if(g_flush_word_count)
	{
		int i = 0;

		cmd = (32<<24) | 1;

		for (i = 0; i < g_flush_word_count; i++)
		{
			if(JPG_READ_REG_POLL(JPG_BSM_REG_BASE+BSM_STS0_OFFSET, 1<<3, 1<<3, TIME_OUT_CLK,
				"polling bsm fifo fifo depth >= 8 words for gob header"))
			{
				JPEG_PRINT("%s" , ("[START_HW_DECODE] flush failed, i = %d", i));
				return JPEG_FAILED;
			}

			JPG_WRITE_REG(JPG_BSM_REG_BASE+BSM_CFG2_OFFSET, cmd, "BSM_CFG2: flush one word");
		}

		g_flush_word_count = 0;
	}

#if 1
{
	int32 i;
       int32 unalign_bytes = head_byte_len&0x03;SCI_TRACE_LOW("unalign_bytes %d",unalign_bytes);
	for ( i = 0; i < unalign_bytes; i++)
	{
		JPG_READ_REG_POLL (JPG_BSM_REG_BASE+BSM_STS0_OFFSET, V_BIT_3, V_BIT_3, TIME_OUT_CLK,  "polling bsm fifo depth >= 8 words for PVOP MB header");
		JPG_WRITE_REG(JPG_BSM_REG_BASE+BSM_CFG2_OFFSET, (8 << 24) | (1<<0), "BSM_CFG2: configure flush n bits");
	}
}
#endif


	if(SWITCH_MODE == jpeg_fw_codec->work_mode)
	{
		//flush the not-byte-aligned bits used in previous slice.
		uint32 bitoffset = jpeg_fw_codec->bitstream_offset%8;
		JpegDec_Read_nBits(bitoffset);

		SCI_MEMCPY(s_bs_last, (uint8*)jpeg_fw_codec->stream_0+jpeg_fw_codec->pingpang_buf_len -100, 100); //back up the last 100 byte of bitstrm bfr 0.
	}

	//MBIO
	cmd = ((jpeg_fw_codec->mbio_bfr1_valid<<1)|(jpeg_fw_codec->mbio_bfr0_valid));
	JPG_WRITE_REG(JPG_MBIO_REG_BASE+BUF_STS_OFFSET, cmd, "BUF_STS_OFFSET: set which mbio buffer is valid now.");
	JPG_WRITE_REG(JPG_MBIO_REG_BASE+MCU_NUM_OFFSET, total_slice_mcu_num & 0xfffff, "MCU_NUM_OFFSET: configure the total mcu number");
	JPG_WRITE_REG(JPG_MBIO_REG_BASE+CTRL_OFFSET, 1, "CTRL_OFFSET: SW configuration is completed, start!");


	//start VLD
	JPG_WRITE_REG(JPG_VLD_REG_BASE+JPEG_RESTART_MCU_CNT_OFFSET, jpeg_fw_codec->restart_mcu_cnt, "VLD_JPEG_RESTART_MCU_CNT: restart_mcu_cnt");
	JPG_WRITE_REG(JPG_VLD_REG_BASE+JPEG_RESTART_MCU_INTV_OFFSET, jpeg_fw_codec->restart_interval, "VLD_JPEG_RESTART_MCU_INTV: restart_interval");
	//config DC pred for Y, U, V
	JPG_WRITE_REG(JPG_VLD_REG_BASE+JPEG_DC_Y_OFFSET, jpeg_fw_codec->dc_pred_y, "VLD_JPEG_DC_Y: config DC_PRED for Y");
	JPG_WRITE_REG(JPG_VLD_REG_BASE+JPEG_DC_UV_OFFSET, jpeg_fw_codec->dc_pred_uv, "VLD_JPEG_DC_UV: config DC_PRED for U and V");
	JPG_WRITE_REG(JPG_VLD_REG_BASE+JPEG_TOTAL_MCU_OFFSET, vld_mcu_num&0xfffff, "VLD_JPEG_CFG0: configure total mcu number in this slice");
	JPG_WRITE_REG(JPG_VLD_REG_BASE+VLD_CTRL_OFFSET, 1, "VLD_CTL: Stard VLD Module");

#if _CMODEL_
	START_SW_DECODE(jpeg_fw_codec, num_of_rows);
#endif//_CMODEL_

	return JPEG_SUCCESS;
}

PUBLIC void JpegDec_BackupLast100ByteBs(uint8 *bitstrm_ptr, uint32 pingpang_buf_len)
{
	bitstrm_ptr += pingpang_buf_len;/*lint !e613*/

	if (pingpang_buf_len > 100)
	{
		pingpang_buf_len = 100;
	}

	SCI_MEMCPY(s_bs_last, (uint8*)bitstrm_ptr -pingpang_buf_len, pingpang_buf_len); //back up the last 100 byte of bitstrm bfr 0.
}

PUBLIC JPEG_RET_E JpegDec_GetBsBitOffset(uint32 bitstrm_bfr_switch_cnt)
{
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();
	uint32 total_read_bytes;
	uint32 bsm_fifo_depth;
	uint32 remain_bytes_in_destuffing_module;
	uint8 *current_bs_ptr;
	uint32 byteNum_after_destuffing = 0; //destuffing by software.
	uint32 bsm_sts0_reg;
	uint32 remain_bits_in_bsshifter = 0;
	uint32 actual_byteNum_after_hw_destuffing;
	uint32 bs_byte_offset = 0;
	uint32 destuffing_cnt = 0;
	uint32 curr_bs_bfr_id = 0;
	uint32 bs_bfr_addr;

	total_read_bytes = JPG_READ_REG(JPG_BSM_REG_BASE+BSM_CFG1_OFFSET, " ") &0xfffff;
	total_read_bytes = total_read_bytes*4; //byte

	bsm_sts0_reg = JPG_READ_REG(JPG_BSM_REG_BASE+BSM_STS0_OFFSET, " ");
	curr_bs_bfr_id = (bsm_sts0_reg >> 4) & 0x01;

	if(0 == curr_bs_bfr_id)
	{
		bs_bfr_addr = (unsigned long)jpeg_fw_codec->stream_0;
	}else
	{
		bs_bfr_addr = (unsigned long)jpeg_fw_codec->stream_1;
	}

#if 0//defined(SIM_IN_WIN)
{
	//print bistream
	uint32 i = 0;
	FILE *fp = fopen("d:/test.dat", "wb");
	for(i = 0; i < total_read_bytes; i++)
	{
		fprintf(fp, "%02x ", *((uint8*)jpeg_fw_codec->stream_0+i));

		if((i+1)%16 == 0)
		{
			fprintf(fp, "\n");
		}
	}
	fclose(fp);

	//calculate the total stuffing count
	{
		uint32 stuffing_total_cnt = 0;
		uint8 *ptr = jpeg_fw_codec->stream_0;
		extern uint32 g_destuffing_cnt;

		for(i = 0; i < total_read_bytes-1; i++)
		{
			if((*ptr == 0xff) && (*(ptr+1) == 0x00))
			{
				stuffing_total_cnt++;
			}
			ptr++;
		}

		if(stuffing_total_cnt != g_destuffing_cnt)
		{
			stuffing_total_cnt = g_destuffing_cnt;
			printf("Error in bsm-model!");
		}
	}
}
#endif

	bsm_fifo_depth = bsm_sts0_reg & 0xf;
	remain_bytes_in_destuffing_module = (bsm_sts0_reg>>8) & 0x3;
	remain_bits_in_bsshifter = 32 - ((bsm_sts0_reg>>12)&0x1f);
	current_bs_ptr = (uint8*)((unsigned long)bs_bfr_addr) + total_read_bytes -1; // byte, the position of last byte which read into de-stuffing module.
	actual_byteNum_after_hw_destuffing = ((bsm_fifo_depth+2)*4+remain_bytes_in_destuffing_module + (remain_bits_in_bsshifter + 7)/8);

	//PRINTF("%d, %d, ", total_read_bytes, actual_byteNum_after_hw_destuffing);

	if(total_read_bytes >= actual_byteNum_after_hw_destuffing)
	{
		while(byteNum_after_destuffing != actual_byteNum_after_hw_destuffing)
		{
			if((*current_bs_ptr == 0x00) && (*(current_bs_ptr-1) == 0xff))
			{
				current_bs_ptr -= 2;
				destuffing_cnt++;
			}else
			{
				current_bs_ptr--;
			}

			byteNum_after_destuffing++;
		}

		bs_byte_offset = (uint32)((unsigned long)current_bs_ptr) - (uint32)((unsigned long)bs_bfr_addr)+1;
	}else
	{
		int32 offset = actual_byteNum_after_hw_destuffing-total_read_bytes;
		int32 bs_array_offset = 0;

		SCI_MEMCPY(s_bs_last, s_bs_last+100-offset, offset);
		SCI_MEMCPY(s_bs_last+offset, (void *)((unsigned long)bs_bfr_addr), total_read_bytes);

		bs_array_offset = actual_byteNum_after_hw_destuffing-1;
		while(byteNum_after_destuffing != actual_byteNum_after_hw_destuffing)
		{
			if((s_bs_last[bs_array_offset] == 0x00) && (s_bs_last[bs_array_offset-1] == 0xff))
			{
				bs_array_offset -= 2;
				destuffing_cnt++;
			}else
			{
				bs_array_offset--;
			}

			byteNum_after_destuffing++;
		}

		bs_byte_offset = (bs_array_offset+1) - offset;
	}

	jpeg_fw_codec->bitstream_offset = ((bitstrm_bfr_switch_cnt*(uint32)jpeg_fw_codec->pingpang_buf_len+ bs_byte_offset)*8+ ((remain_bits_in_bsshifter + 7)/8)*8-(remain_bits_in_bsshifter));

#if 0//_CMODEL_
	{
		extern uint32 g_destuffing_cnt;

		if(	jpeg_fw_codec->bitstream_offset != ((g_destuffing_cnt-destuffing_cnt)*8 + s_pBsmr_ctrl_reg->BSM_TOTAL_BITS))
		{
			bs_byte_offset = bs_byte_offset;
		// 	jpeg_fw_codec->bitstream_offset = (g_destuffing_cnt*8 + s_pBsmr_ctrl_reg->TOTAL_BITS);

			printf("error!!");
		}
	}
#endif
	return JPEG_SUCCESS;
}

#if PROGRESSIVE_SUPPORT
long jpeg_div_round_up(long a, long b)
{
	/* Compute a/b rounded up to next integer, ie, ceil(a/b) */
	/* Assumes a >= 0, b > 0 */
	return (a + b -1L)/b;
}



void JpedDec_Update_SOS(uint8 scan_index)
{
	int32 ci;
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();
	JPEG_PROGRESSIVE_INFO_T *progressive_info = JPEGFW_GetProgInfo();

	progressive_info->cur_scan = scan_index;

	//Update the cinfo.Ss etc...
	progressive_info->Ss = progressive_info->buf_storage[scan_index].Ss;
	progressive_info->Se = progressive_info->buf_storage[scan_index].Se;
	progressive_info->Ah = progressive_info->buf_storage[scan_index].Ah;
	progressive_info->Al = progressive_info->buf_storage[scan_index].Al;
	progressive_info->comps_in_scan = progressive_info->buf_storage[scan_index].comps_in_scan;

	for(ci = 0; ci < progressive_info->comps_in_scan; ci++)
	{
		progressive_info->cur_comp_info[ci] = &(progressive_info->buf_storage[scan_index].cur_comp_info[ci]);
	}

	if(progressive_info->comps_in_scan == 1)
	{
		progressive_info->MCU_per_col = progressive_info->cur_comp_info[0]->v_samp_factor;
		progressive_info->MCU_per_row = (uint16)jpeg_div_round_up(jpeg_fw_codec->width*progressive_info->cur_comp_info[0]->h_samp_factor, jpeg_fw_codec->ratio[0].h_ratio*JPEG_FW_BLOCK_SIZE);

	}else
	{
		progressive_info->MCU_per_col = 1;
		progressive_info->MCU_per_row = jpeg_fw_codec->mcu_num_x;
	}

	/*update the bitstream info*/
	Update_Global_Bitstrm_Info(&(progressive_info->buf_storage[scan_index].address));
}



JPEG_RET_E JPEG_DecodeMCULine_Progressive()
{
	uint8 i = 0;
	uint8 yoffset, xoffset;
	int16 *block;
	int16 **org_mcu = NULL;
	uint16 block_id;
	uint32 ci;
	int16 start_col_block, blocksperrow;
	int16 yindex, xindex;
	jpeg_component_info *compptr = NULL;
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();
	JPEG_PROGRESSIVE_INFO_T *progressive_info = JPEGFW_GetProgInfo();

	memset(progressive_info->block_line[0], 0, 64*sizeof(int16)*jpeg_fw_codec->mcu_num_x*jpeg_fw_codec->ratio[0].h_ratio*jpeg_fw_codec->ratio[0].v_ratio);
	memset(progressive_info->block_line[1], 0, 64*sizeof(int16)*jpeg_fw_codec->mcu_num_x);
	memset(progressive_info->block_line[2], 0, 64*sizeof(int16)*jpeg_fw_codec->mcu_num_x);

	while(i < progressive_info->input_scan_number)
	{
		//update the environment of progressive decoding
		JpedDec_Update_SOS(i);

		for(yoffset = 0; yoffset < progressive_info->MCU_per_col; yoffset++)
		{
			for(xoffset = 0; xoffset < progressive_info->MCU_per_row; xoffset++)
			{
				if(xoffset == 1)
				{
					JPEG_TRACE("");
				}
				block_id = 0;
				for(ci = 0; ci < progressive_info->comps_in_scan; ci++)
				{
					compptr = progressive_info->cur_comp_info[ci];
					start_col_block = xoffset * compptr->MCU_width;
					blocksperrow = jpeg_fw_codec->mcu_num_x*compptr->h_samp_factor * JPEG_FW_DCTSIZE2;

					for(yindex = 0; yindex < compptr->MCU_height; yindex++)
					{
						block = progressive_info->block_line[compptr->component_id -1] + (yindex+yoffset)*blocksperrow+
							start_col_block*JPEG_FW_DCTSIZE2;
						for(xindex = 0; xindex < compptr->MCU_width; xindex++)
						{
							progressive_info->blocks[block_id++] = block;
							block += JPEG_FW_DCTSIZE2;

						}
					}
				}

				org_mcu = progressive_info->blocks;

				if((*progressive_info->buf_storage[i].entropy.decode_mcu)(org_mcu) != TRUE)
				{
					return JPEG_FAILED;
				}
			}
		}

		//update the bitstream info;
		Update_Local_Bitstrm_Info(&(progressive_info->buf_storage[i].address));

		i++;
	}


	return JPEG_SUCCESS;
}

PUBLIC JPEG_RET_E START_SW_DECODE_PROGRESSIVE(JPEG_CODEC_T *jpeg_fw_codec, uint32 num_of_rows)
{
	uint16 x = 0, y = 0;
	uint8 input_mcu_info = jpeg_fw_codec->input_mcu_info;
	uint8 scale_factor = jpeg_fw_codec->scale_factor;
	uint32 mcu_num_y = (num_of_rows / (jpeg_fw_codec->mcu_height));
	uint32 mcu_num_x = jpeg_fw_codec->mcu_num_x;
	int32 block_id;
	int32 luma_blk_num;
	int32 chroma_blk_num;
	int16 *block;
	uint8 *rgiDst;
	uint32 ci;

	const int32 *quant;
	uint8 *y_coeff = jpeg_fw_codec->mbio_bfr0_valid ? jpeg_fw_codec->YUV_Info_0.y_data_ptr : jpeg_fw_codec->YUV_Info_1.y_data_ptr;
	uint8 *uv_coeff = jpeg_fw_codec->mbio_bfr0_valid ? jpeg_fw_codec->YUV_Info_0.u_data_ptr : jpeg_fw_codec->YUV_Info_1.u_data_ptr;
	JPEG_PROGRESSIVE_INFO_T *progressive_info = JPEGFW_GetProgInfo();
	JPEGFW_MCU_To_Frame MCUToFrm;

	int32 luma_h_ratio = jpeg_fw_codec->ratio[0].h_ratio;
	int32 luma_v_ratio = jpeg_fw_codec->ratio[0].v_ratio;
	int32 offset;

	if(jpeg_fw_codec->is_first_slice)
	{
	//	Init_downsample_fun();
	}

	switch(input_mcu_info)
	{
	case JPEG_FW_YUV444:
		MCUToFrm = JPEGFW_OutMCU444;
		progressive_info->block_num = 3;
		luma_blk_num = 1;
		chroma_blk_num = 2;
		break;
	case JPEG_FW_YUV422:
		MCUToFrm = JPEGFW_OutMCU422;
		progressive_info->block_num = 4;
		luma_blk_num = 2;
		chroma_blk_num = 2;
		break;
	case JPEG_FW_YUV411:
		MCUToFrm = JPEGFW_OutMCU411;
		progressive_info->block_num = 6;
		luma_blk_num = 4;
		chroma_blk_num = 2;
		break;
	case JPEG_FW_YUV420:
		MCUToFrm = JPEGFW_OutMCU420;
		progressive_info->block_num = 6;
		luma_blk_num = 4;
		chroma_blk_num = 2;
		break;
	case JPEG_FW_YUV400:
		MCUToFrm = JPEGFW_OutMCU400;
		progressive_info->block_num = 1;
		luma_blk_num = 1;
		chroma_blk_num = 0;
		break;
//	case JPEG_FW_YUV422_R:
//		MCUToFrm = JPEGFW_OutMCU444;
//		progressive_info->block_num = 4;
//		break;
//	case JPEG_FW_YUV411_R:
//		MCUToFrm = JPEGFW_OutMCU444;
//		progressive_info->block_num = 6;
//		break;
	default:
		return JPEG_FAILED;
	}

	JPEG_TRACE("\nJPEG Decoding MCU...\n");
	for(y = 0; y < mcu_num_y; y++)
	{
		if(y == 20 )
		{
			JPEG_TRACE("");
		}

		JPEG_TRACE("MCU_Num_Y:%d\n", y);

		JPEG_DecodeMCULine_Progressive();

		for(x = 0; x < mcu_num_x; x++)
		{
			if(y == 0x9 && x == 0)
			{
				JPEG_TRACE("");
			}

			//make vld to fill the io_buf
			for(block_id = 0; block_id < progressive_info->block_num; block_id++)
			{
				if(block_id < luma_blk_num)
				{
					offset = ((block_id/luma_h_ratio)*jpeg_fw_codec->mcu_num_x+x)*luma_h_ratio+(block_id%luma_v_ratio);
					block = progressive_info->block_line[0]+offset*JPEG_FW_DCTSIZE2;
				}else
				{
					block = progressive_info->block_line[block_id - luma_blk_num+1]+x*JPEG_FW_DCTSIZE2;
				}

				rgiDst = progressive_info->org_blocks[block_id];

				ci = progressive_info->blocks_membership[block_id];
				quant = progressive_info->quant_tbl_new[jpeg_fw_codec->tbl_map[ci].quant_tbl_id];

				//dequant has been performed in idct transformation
	 			(*progressive_info->jpeg_transform)(block, rgiDst, quant);
			}

			//copy MCU data to coeff buffer,Added by wangyi 2007/05/02
			 MCUToFrm((uint8*)y_coeff, (uint8*)uv_coeff, x, y, scale_factor);
		}
	}

// jpeg_end:
	JPEG_TRACE("JPEG Decoding Finished...\n\n");

#if _CMODEL_
	MBIO_INT_PROC();
#endif

	return JPEG_SUCCESS;
}
#endif
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
