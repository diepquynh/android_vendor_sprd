/******************************************************************************
 ** File Name:      JpegDec_interface.c                                            *
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

#if !defined(_SIMULATION_)
//#include "os_api.h"
#endif

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C"
    {
#endif

//#if defined(JPEG_DEC)
//////////////////////////////////////////////////////////////////////////

PUBLIC void JPEGFW_configure_vld_reg_jpegDec (void);
PUBLIC void JPEGFW_build_hufftab_jpegDec(void);
PUBLIC void JPEGFW_InitHuffTbl(void);
PUBLIC JPEG_RET_E  JPEG_FWParseHead(JPEG_DEC_INPUT_PARA_T  *jpeg_dec_input)
{
	uint32 i;
	JPEG_RET_E ret;
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();
	int fd;
	void *jpg_addr;

	fd = jpeg_fw_codec->fd;
	jpg_addr = jpeg_fw_codec->jpg_addr;
	SCI_MEMSET(jpeg_fw_codec, 0, sizeof(JPEG_CODEC_T));
	jpeg_fw_codec->fd = fd;
	jpeg_fw_codec->jpg_addr = jpg_addr;
	//@cr170390, add by shan.he
	g_huff_tbl_malloced = FALSE; //??? leon

	JpegDec_InitMem(&(jpeg_dec_input->dec_buf));

	jpeg_fw_codec->using_default_huff_tab = TRUE;
	jpeg_fw_codec->using_default_quant_tab = TRUE;

	jpeg_fw_codec->quant_tbl[0] = PNULL;
	jpeg_fw_codec->quant_tbl[1] = PNULL;
	for(i = 0; i < NUM_HUFF_TBLS; i++)
	{
		jpeg_fw_codec->dc_huff_tbl[i].bits = PNULL;
		jpeg_fw_codec->dc_huff_tbl[i].huffval = PNULL;
		jpeg_fw_codec->ac_huff_tbl[i].bits = PNULL;
		jpeg_fw_codec->ac_huff_tbl[i].huffval = PNULL;
	}

	jpeg_fw_codec->restart_interval = 0x3ffff;//set max value for default.
	jpeg_fw_codec->restart_to_go = jpeg_fw_codec->restart_interval;

	ret = JpegDec_InitBitream(jpeg_dec_input);
	if(ret != JPEG_SUCCESS)
	{
		return ret;
	}

	if (is_res_jpg_file())
	{
		jpeg_fw_codec->is_res_file = TRUE;
		ret = ParseHead_Res(jpeg_dec_input);
		jpeg_dec_input->header_len = 12;
	}else
	{
		jpeg_fw_codec->is_res_file = FALSE;
		ret = ParseHead(jpeg_dec_input);
		jpeg_dec_input->header_len = get_header_len(jpeg_dec_input);
	}

	if(ret != JPEG_SUCCESS)
	{
		return ret;
	}

	return JPEG_SUCCESS;
}

/************************************************************************/
/*JpegDec_FwInit                                                        */
/************************************************************************/
PUBLIC JPEG_RET_E JPEG_HWDecInit(JPEG_DEC_INPUT_PARA_T *jpeg_dec_input)
{
	JPEG_RET_E ret = JPEG_SUCCESS;
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();
	int32 bNeedInit = FALSE;

	jpeg_fw_codec->is_first_slice = jpeg_dec_input->is_first_slice;

	if((ALONE_MODE == jpeg_fw_codec->work_mode) /*only decoder*/
		||((SWITCH_MODE == jpeg_fw_codec->work_mode) &&jpeg_fw_codec->is_first_slice) /*switch decoder and decoder*/)
	{
		bNeedInit = TRUE;
	}

	//init firmware codec info
	if(bNeedInit)
	{
	#if _CMODEL_
	//reset the global variable;
		g_rows_out= 0;
		//mbio buffer property.
		g_int_yuv_buf0_full = FALSE;
		g_int_yuv_buf1_full = FALSE;
	#endif
		ret = JPEG_FWInitDecInput(jpeg_dec_input);

		if(ret != JPEG_SUCCESS)
		{
			return ret;
		}

		g_huff_tbl_malloced = FALSE;
		g_flush_word_count = 0;

	}

	if(!jpeg_fw_codec->progressive_mode)
	{
		//init vsp global register.
		JpegDec_HwTopRegCfg();

		/*enable sub module*/
		JpegDec_HwSubModuleCfg(jpeg_dec_input->header_len);

		if(bNeedInit)
		{
			if(!jpeg_fw_codec->using_default_huff_tab)
			{
				JPEGFW_InitHuffTbl();

				/*build huffman table*/
				JPEGFW_build_hufftab_jpegDec ();
			}
		}

		/*configure VLD register, max register for DC and AC*/
		JPEGFW_configure_vld_reg_jpegDec();

		configure_huff_tab(g_huffTab, 162);
	}else
	{
	#if PROGRESSIVE_SUPPORT
		JPEG_PROGRESSIVE_INFO_T *progressive_info_ptr;

		progressive_info_ptr = (JPEG_PROGRESSIVE_INFO_T *)JpegDec_ExtraMemAlloc(sizeof(JPEG_PROGRESSIVE_INFO_T));
		progressive_info_ptr->buf_storage = (JPEG_SOS_T*)JpegDec_ExtraMemAlloc(MAX_SCAN_NUM * sizeof(JPEG_SOS_T));

		progressive_info_ptr->Ss = jpeg_fw_codec->Ss;
		progressive_info_ptr->Se = jpeg_fw_codec->Se;
		progressive_info_ptr->Ah = jpeg_fw_codec->Ah;
		progressive_info_ptr->Al = jpeg_fw_codec->Al;
		progressive_info_ptr->input_scan_number = jpeg_fw_codec->input_scan_number;
		progressive_info_ptr->comp_info = jpeg_fw_codec->comp_info;
		progressive_info_ptr->cur_scan = 0;
		progressive_info_ptr->comps_in_scan		= jpeg_fw_codec->comps_in_scan;
		SCI_MEMCPY(&(progressive_info_ptr->comp_id_map[0]), &(jpeg_fw_codec->comp_id_map[0]), 3*(sizeof(uint8)));

		JPEGFW_SetProgInfo(progressive_info_ptr);
#if _CMODEL_
		VSP_Init_CModel();
#endif
		JPEGFW_AllocMCUBuf();
		JPEGFW_InitTransFun(progressive_info_ptr);
		Initialize_Clip();
		//Scan the bitstream and generate entry point map;
		ret = JPEG_Generate_Entry_Point_Map_Progressive();

		if(JPEG_SUCCESS != ret)
		{
			return ret;
		}
	#endif
	}

	/*check if quant_tbl is null*/
	if(jpeg_fw_codec->using_default_quant_tab)
	{
		JPEG_TRACE("using default quant table\n");
		JPEGFW_InitQuantTbl(jpeg_dec_input->quant_level/*COMPRESS_LEVEL_NORMAL*/);
	}

	/*set quantization table*/
	ret = JPEGFW_AdjustQuantTbl_Dec();

	return ret;
}

PUBLIC JPEG_RET_E JPEG_HWDecStart(uint32 num_of_rows, JPEG_DEC_OUTPUT_PARA_T *output_para_ptr)
{
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();
	JPEG_RET_E ret = JPEG_SUCCESS;

	SCI_ASSERT(output_para_ptr != PNULL);
	SCI_ASSERT(jpeg_fw_codec != PNULL);

	if((2 == jpeg_fw_codec->scale_factor) &&
		(((JPEG_FW_YUV444 == jpeg_fw_codec->input_mcu_info)&&(!jpeg_fw_codec->progressive_mode)) ||
		 (JPEG_FW_YUV400 == jpeg_fw_codec->input_mcu_info) ||
		 (JPEG_FW_YUV422_R == jpeg_fw_codec->input_mcu_info) ||
		 (JPEG_FW_YUV411_R == jpeg_fw_codec->input_mcu_info) ))
	{
		JPEG_TRACE("Invalid scale down factor, which must be 0 ~ 1\n");
		return JPEG_FAILED;
	}

#if 0
{
	uint32 y_addr = (uint32)(jpeg_fw_codec->YUV_Info_0.y_data_ptr);
	uint32 u_addr = (uint32)(jpeg_fw_codec->YUV_Info_0.u_data_ptr);
	uint32 v_addr = (uint32)(jpeg_fw_codec->YUV_Info_0.v_data_ptr);

	uint32 cmd = u_addr - y_addr;
	//uint32 endian_sel = 0;

		//now, for uv_interleaved
//	cmd >>= 2; //word unit
	//endian_sel = VSP_READ_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, "red endian sel offset");		//add by shan.he
//	VSP_WRITE_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, (cmd), "configure yu offset");
	//SCI_TRACE_LOW("JPEG_HWDecStart: endian:d.\n ", endian_sel);
/*3plane */
//	cmd = (((v_addr - u_addr)>> 2))&0x3fffffff; //word unit
//	VSP_WRITE_REG(VSP_AHBM_REG_BASE+AHBM_V_ADDR_OFFSET, cmd, "configure uv offset");
}
#endif

	if(!jpeg_fw_codec->progressive_mode)
	{
		ret = START_HW_DECODE(jpeg_fw_codec, num_of_rows);
	}else
	{
	#if PROGRESSIVE_SUPPORT
		ret = START_SW_DECODE_PROGRESSIVE(jpeg_fw_codec, num_of_rows);
	#endif
	}

	output_para_ptr->output_width = (jpeg_fw_codec->c_width >> jpeg_fw_codec->scale_factor);
	output_para_ptr->output_height = (jpeg_fw_codec->c_height >> jpeg_fw_codec->scale_factor);

	return ret;
}

PUBLIC JPEG_RET_E JpegDec_FwReadoutDecInfo(uint32 *bitstrm_byte_offset_ptr, uint32 bitstrm_bfr_switch_cnt)
{
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();
	JPEG_RET_E ret = JPEG_SUCCESS;

	JPG_READ_REG_POLL(JPG_MBIO_REG_BASE+BUF_STS_OFFSET, 0x4, 0x4, TIME_OUT_CLK, "polling the jpeg decoding end!");

	jpeg_fw_codec->restart_mcu_cnt = JPG_READ_REG(JPG_VLD_REG_BASE+JPEG_RESTART_MCU_CNT_OFFSET, "get restart_mcu_cnt");
	jpeg_fw_codec->dc_pred_y = JPG_READ_REG(JPG_VLD_REG_BASE+JPEG_DC_Y_OFFSET, "readout DC_PRED for Y");
	jpeg_fw_codec->dc_pred_uv = JPG_READ_REG(JPG_VLD_REG_BASE+JPEG_DC_UV_OFFSET, "readout DC_PRED for UV");

	//get bitstream_offset
	ret = JpegDec_GetBsBitOffset(bitstrm_bfr_switch_cnt);

	*bitstrm_byte_offset_ptr = (jpeg_fw_codec->bitstream_offset>>3);

	//PRINTF("%d\n", *bitstrm_byte_offset_ptr);

	return ret;
}

PUBLIC void JPEGDEC_HWUpdateMBIOBufInfo(void)
{
#if 0   //NOT NEED config uv_offset on SHARK.
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();
	uint32 y_addr = (uint32)(jpeg_fw_codec->YUV_Info_1.y_data_ptr);
	uint32 u_addr = (uint32)(jpeg_fw_codec->YUV_Info_1.u_data_ptr);
	uint32 v_addr = (uint32)(jpeg_fw_codec->YUV_Info_1.v_data_ptr);

	uint32 cmd = u_addr - y_addr;
	uint32 endian_sel = 0;
	cmd = u_addr - y_addr;

/*  cmd >>= 2; //word unit
	endian_sel = VSP_READ_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, "red endian sel offset");		//add by shan.he
	VSP_WRITE_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, (cmd<<4)|(endian_sel & 0xf), "configure uv offset");
	SCI_TRACE_LOW("JPEG_HWDecStart: endian:d.\n ", endian_sel);
*/
	cmd >>= 2; //word unit

//	VSP_WRITE_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, (cmd), "configure yu offset");

/*	cmd = (((v_addr - u_addr)>> 2))&0x3fffffff; //word unit
	VSP_WRITE_REG(VSP_AHBM_REG_BASE+AHBM_V_ADDR_OFFSET, cmd, "configure uv offset");*/
#endif
}

/*decode the defined number of MCU under frame mode*/
PUBLIC JPEG_RET_E JPEG_HWDecStartMCUSynchro(uint32 num_of_rows, JPEG_DEC_OUTPUT_PARA_T *output_para_ptr)
{
	uint16 slice_mcu_num, remained_mcu_num;
	uint32 cmd;
	uint32 j = 0;
	int32 vsp_time_out_cnt = 0;
	int32 max_time_out_cnt = 0xFF;
	uint32 num_slice = 0;
	uint32 remained_rows;
	uint32 mbio_total_mcu_num = 0;
	uint32 decode_mcu_num = 0;
	uint32 pingpang_buf_status = 0;
	JPEG_RET_E ret;

	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();
	SCI_ASSERT(jpeg_fw_codec != PNULL);
	SCI_ASSERT(0 == (num_of_rows & 0x7));

	num_slice = (jpeg_fw_codec->c_height) / num_of_rows;
	remained_rows = jpeg_fw_codec->c_height  - (num_slice * num_of_rows);

	//the remained rows must be more than 1 line because the X and Y ID will be cleared to 0 after the decoding is done
	if (num_slice > 0 && remained_rows == 0)
	{
		num_slice -= 1;
		remained_rows = num_of_rows;
	}

	SCI_PASSERT(remained_rows > 0, ("the last slice must be more than 1 "));

	//JPEG_PRINT("[JPEG_HWDecStartMCUSynchro] num_slice =d, remained_rows =d", num_slice, remained_rows);

	ret = JpegDec_CheckRowNumValid(num_of_rows, jpeg_fw_codec->input_mcu_info);

	if(ret != JPEG_SUCCESS)
	{
		return ret;
	}

	ret = JpegDec_CheckRowNumValid(remained_rows, jpeg_fw_codec->input_mcu_info);

	if(ret != JPEG_SUCCESS)
	{
		return ret;
	}

	slice_mcu_num = (uint16)(num_of_rows / (jpeg_fw_codec->mcu_height)) * ((jpeg_fw_codec->mcu_num_x));
	remained_mcu_num = (uint16)(remained_rows / (jpeg_fw_codec->mcu_height)) * ((jpeg_fw_codec->mcu_num_x));

	//VSP_CFG1
	cmd =(jpeg_fw_codec->input_mcu_info<<24) | ((jpeg_fw_codec->mcu_num_y & 0x3ff) << 12) | (jpeg_fw_codec->mcu_num_x & 0x3ff);
	JPG_WRITE_REG(JPG_GLB_REG_BASE+MB_CFG_OFFSET, cmd, "VSP_CFG1: input mcu info, slice mcu number x and y");

	//BSM Module cfg
	pingpang_buf_status = ((jpeg_fw_codec->bsm_buf0_valid<<1) | (jpeg_fw_codec->bsm_buf1_valid));
	cmd = ((uint32)pingpang_buf_status<<30) | ((jpeg_fw_codec->pingpang_buf_len+3) >> 2);
	JPG_WRITE_REG(JPG_BSM_REG_BASE+BSM_CFG0_OFFSET, cmd, "BSM_CFG0: buffer0 for read, and the buffer size");
#if _CMODEL_
	g_bs_pingpang_bfr0 = jpeg_fw_codec->stream_0;
	g_bs_pingpang_bfr1 = jpeg_fw_codec->stream_1;
	init_bsm();
#endif

	//polling bsm status
	JPG_READ_REG_POLL(JPG_BSM_REG_BASE+BSM_STS0_OFFSET, ((uint32)1<<31), ((uint32)0<<31), TIME_OUT_CLK, "BSM_DEBUG: polling bsm status");

	//JPEG_PRINT("[JPEG_HWDecStartMCUSynchro] slice_mcu_num =d, remained_mcu_num =d", slice_mcu_num, remained_mcu_num);

	/*start mbio*/
	cmd = ((jpeg_fw_codec->mbio_bfr1_valid<<1)|(jpeg_fw_codec->mbio_bfr0_valid));
	JPG_WRITE_REG(JPG_MBIO_REG_BASE+BUF_STS_OFFSET, cmd, "DBK_VDB_BUF_ST: set which mbio buffer is valid now.");
	mbio_total_mcu_num = (jpeg_fw_codec->mcu_num_x*jpeg_fw_codec->mcu_num_y);
	cmd = mbio_total_mcu_num & 0xffff;
	JPG_WRITE_REG(JPG_MBIO_REG_BASE+JPEG_TOTAL_MCU_OFFSET, cmd, "VLD_TOTAL_MCU: configure the total mcu number");
	JPG_WRITE_REG(JPG_MBIO_REG_BASE+CTRL_OFFSET, 1, "VLD_CTRL: configure DBK configure finished");

// 	TM_SendTestPointRequest(0x20090409, 0);

	//config VLD
	JPG_WRITE_REG(JPG_VLD_REG_BASE+JPEG_RESTART_MCU_CNT_OFFSET, jpeg_fw_codec->restart_mcu_cnt, "VLD_JPEG_RESTART_MCU_CNT: restart_mcu_cnt");
	JPG_WRITE_REG(JPG_VLD_REG_BASE+JPEG_RESTART_MCU_INTV_OFFSET, jpeg_fw_codec->restart_interval, "VLD_JPEG_RESTART_MCU_INTV: restart_interval");
	//config DC pred for Y, U, V
	JPG_WRITE_REG(JPG_VLD_REG_BASE+JPEG_DC_Y_OFFSET, jpeg_fw_codec->dc_pred_y, "VLD_JPEG_DC_Y: config DC_PRED for Y");
	JPG_WRITE_REG(JPG_VLD_REG_BASE+JPEG_DC_UV_OFFSET, jpeg_fw_codec->dc_pred_uv, "VLD_JPEG_DC_UV: config DC_PRED for U and V");
#if 0
	{
		uint32 y_addr = (uint32)(jpeg_fw_codec->YUV_Info_0.y_data_ptr);
		uint32 u_addr = (uint32)(jpeg_fw_codec->YUV_Info_0.u_data_ptr);


		uint32 endian_sel = 5;
              cmd = u_addr - y_addr;

			//now, for uv_interleaved
		cmd >>= 2; //word unit
		endian_sel = VSP_READ_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, "red endian sel offset");		//add by shan.he
		JPG_WRITE_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, (cmd<<4)|(endian_sel & 0xf), "configure uv offset");
	}
#endif
{
	uint32 y_addr = (unsigned long)(jpeg_fw_codec->YUV_Info_0.y_data_ptr);
	uint32 u_addr = (unsigned long)(jpeg_fw_codec->YUV_Info_0.u_data_ptr);
	uint32 v_addr = (unsigned long)(jpeg_fw_codec->YUV_Info_0.v_data_ptr);

	uint32 cmd = u_addr - y_addr;

	//now, for uv_interleaved
	cmd >>= 2; //word unit
//	VSP_WRITE_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, (cmd), "configure yu offset");

	/*3plane */
//	cmd = (((v_addr - u_addr)>> 2))&0x3fffffff; //word unit
//	VSP_WRITE_REG(VSP_AHBM_REG_BASE+AHBM_V_ADDR_OFFSET, cmd, "configure uv offset");
}

	for(j = 0; j < num_slice; j++)
	{
		//JPEG_PRINT("[JPEG_HWDecStartMCUSynchro] decode one slice,d, mbio status = 0x%x, mcu_num_x =d", j, *(volatile uint32*)0x20200068, jpeg_fw_codec->mcu_num_x);

		//configure the number of mcu vld would process;
		cmd = slice_mcu_num & 0xfffff;
		JPG_WRITE_REG(JPG_VLD_REG_BASE+JPEG_TOTAL_MCU_OFFSET, cmd, "VLD_TOTAL_MCU: configure total mcu number in this slice");
		JPG_WRITE_REG(JPG_VLD_REG_BASE+VLD_CTRL_OFFSET, 1, "VLD_CTRL: Stard VLD Module");

		vsp_time_out_cnt = 0;

		while((decode_mcu_num < (slice_mcu_num * (j+1)))/* || VSP_AXIM_IsBusy*/)
		{
			//SCI_Sleep(3);

			if(JPG_VLD_IsError)
			{
				//JPEG_PRINT("[JPEG_HWDecStartMCUSynchro] decode error, slice =d", j);
				return JPEG_FAILED;
			}

			if (vsp_time_out_cnt > max_time_out_cnt)
			{
				//JPEG_PRINT("[JPEG_HWDecStartMCUSynchro] decode timeout, slice =d", j);
				return JPEG_FAILED;
			}
			vsp_time_out_cnt++;

			decode_mcu_num = JPG_MBIO_MCU_X + JPG_MBIO_MCU_Y * jpeg_fw_codec->mcu_num_x;
		}
	}

	//JPEG_PRINT("[JPEG_HWDecStartMCUSynchro] decoded slices donw, mbio status = 0x%x", j, *(volatile uint32*)0x20200068);

	//check the register (VSP_DBK_REG_BASE + DBK_VDB_BUF_ST_OFF) to makesure the last slice is done
	//never use the decoded mcu number because that register will be cleared once the decoding has done
	//configure the number of mcu vld would process;
	cmd = remained_mcu_num & 0xfffff;
	JPG_WRITE_REG(JPG_VLD_REG_BASE+JPEG_TOTAL_MCU_OFFSET, cmd, "VLD_TOTAL_MCU: configure total mcu number in this slice");
	JPG_WRITE_REG(JPG_VLD_REG_BASE+VLD_CTRL_OFFSET, 1, "VLD_CTRL: Stard VLD Module");

	vsp_time_out_cnt = 0;
	while(!JPG_MBIO_IsJpegDecEnd /*||VSP_AXIM_IsBusy*/)
	{
		//SCI_Sleep(3);

		if(JPG_VLD_IsError)
		{
			//JPEG_PRINT("[JPEG_HWDecStartMCUSynchro] decode error, last slice");
			return JPEG_FAILED;
		}

		if (vsp_time_out_cnt > max_time_out_cnt)
		{
			//JPEG_PRINT("[JPEG_HWDecStartMCUSynchro] decode timeout, last slice");
			return JPEG_FAILED;
		}
		vsp_time_out_cnt++;
	}

//	TM_SendTestPointRequest(0x20090409, 1);

	if(!JPEG_HWWaitingEnd())
	{
		//JPEG_PRINT("[JPEG_HWDecStartMCUSynchro] JPEG_HWWaitingEnd failed");
		return JPEG_FAILED;
	}

#if _CMODEL_
	START_SW_DECODE(jpeg_fw_codec, jpeg_fw_codec->c_height);
#endif//_CMODEL_

	output_para_ptr->output_width = jpeg_fw_codec->c_width;
	output_para_ptr->output_height = jpeg_fw_codec->c_height;

	return JPEG_SUCCESS;
}

PUBLIC void JpegDec_FwClose(JPEG_DEC_INPUT_PARA_T *jpeg_dec_input)
{
#if _CMODEL_
	VSP_Delete_CModel();
#endif

	return;
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
