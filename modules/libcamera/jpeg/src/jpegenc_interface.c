/******************************************************************************
 ** File Name:      JpegEnc_interface.c                                            *
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

#if defined(JPEG_ENC)
//////////////////////////////////////////////////////////////////////////

/************************************************************************/
/* initialization of JPEG firmware                                      */
/************************************************************************/
PUBLIC JPEG_RET_E JPEG_HWEncInit(JPEG_ENC_INPUT_PARA_T *input_para_ptr)
{
	JPEG_RET_E ret = JPEG_SUCCESS;
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGEncCodec();

	SCI_ASSERT(input_para_ptr != PNULL);
	SCI_ASSERT(jpeg_fw_codec != PNULL);

	jpeg_fw_codec->is_first_slice = input_para_ptr->is_first_slice;
	jpeg_fw_codec->is_last_slice = input_para_ptr->is_last_slice;

	//load parameter into jpeg codec
	if(input_para_ptr->is_first_slice)
	{
		JpegEnc_InitMem(&(input_para_ptr->enc_buf));
		ret = JpegEnc_InitParam(input_para_ptr);
		if(ret != JPEG_SUCCESS)
		{
			return ret;
		}
	}
	jpeg_fw_codec->g_stream_buf_ptr = input_para_ptr->stream_buf1;
	jpeg_fw_codec->mbio_bfr0_valid = input_para_ptr->mbio_bfr0_valid;
	jpeg_fw_codec->mbio_bfr1_valid = input_para_ptr->mbio_bfr1_valid;
	jpeg_fw_codec->bsm_buf0_valid = input_para_ptr->bsm_buf0_valid;
	jpeg_fw_codec->bsm_buf1_valid = input_para_ptr->bsm_buf1_valid;

	//initialize VSP hardware module
	JpegEnc_HwTopRegCfg();
	JpegEnc_HwSubModuleCfg();
	JPEGFW_InitHuffTblWithDefaultValue(jpeg_fw_codec);
	configure_huff_tab(g_huff_tab_enc, 162);
	JpegEnc_QTableCfg();

	return ret;
}

PUBLIC void JPEG_HWUpdateMBIOBufInfo(void)
{
#if 0
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGEncCodec();
	uint32 y_addr = (uint32)(jpeg_fw_codec->YUV_Info_1.y_data_ptr);
	uint32 u_addr = (uint32)(jpeg_fw_codec->YUV_Info_1.u_data_ptr);

	uint32 cmd = u_addr - y_addr;
	//uint32 endian_sel = 0;

	//now, for uv_interleaved
	cmd >>= 2; //word unit
	//endian_sel = VSP_READ_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, "red endian sel offset");		//add by shan.he
//	VSP_WRITE_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, cmd, "configure uv offset");
#endif
}

PUBLIC JPEG_RET_E JPEG_HWEncStart(uint32 raw_width, uint32 raw_height, JPEG_ENC_OUTPUT_PARA_T *output_para_ptr)
{
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGEncCodec();
	uint16 total_mcu_num = 0;

	SCI_ASSERT(jpeg_fw_codec != PNULL);

	//if(((raw_width & 0xf) == 0) && ((raw_height & 0x7) == 0)){
		total_mcu_num =(uint16)( jpeg_fw_codec->mcu_num_x*jpeg_fw_codec->mcu_num_y);
//	}else{
//		JPEG_TRACE("Invalid raw_height, must be interger times of 8 and raw_width must be interger times of 16...\n");
//		return JPEG_FAILED;
//	}
#if 0
	{
		uint32 y_addr = (uint32)(jpeg_fw_codec->YUV_Info_0.y_data_ptr);
		uint32 u_addr = (uint32)(jpeg_fw_codec->YUV_Info_0.u_data_ptr);

		uint32 cmd = u_addr - y_addr;
		uint32 endian_sel = 0;

		SCI_TRACE_LOW("uv offset: %x.\n,y addr:%x,uv addr:%x", cmd,y_addr,u_addr);

			//now, for uv_interleaved
		cmd >>= 2; //word unit
		endian_sel = VSP_READ_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, "red endian sel offset");		//add by shan.he
		VSP_WRITE_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, (cmd<<4)|(endian_sel & 0xf), "configure uv offset");
	}
#endif

	START_HW_ENCODE(total_mcu_num);

//	if(READ_REG_POLL(VSP_MEA_REG_BASE+MEA_VDB_BUF_ST_OFF, 0x04, 0x04, TIME_OUT_CLK, "polling mea done status"))
//	{
//		JPEG_TRACE("TIME OUT!\n");
//		return FALSE;
//	}

	//if(VSP_READ_REG_POLL(VSP_AHBM_REG_BASE+AHBM_STS_OFFSET, V_BIT_0, 0, TIME_OUT_CLK, "polling AHB idle status"))
//    if(VSP_READ_REG_POLL(DCAM_INT_STS_OFF+VSP_DCAM_REG_BASE,V_BIT_8,1,TIME_OUT_CLK, "polling VSP_VLC_DONE"))
//	{
//		JPEG_TRACE("TIME OUT!\n");
//		return FALSE;
//	}

	return JPEG_SUCCESS;
}

//PUBLIC JPEG_RET_E JPEG_HWWriteHead()
PUBLIC JPEG_RET_E JPEG_HWWriteHead(APP1_T *app1_t)
{
	if(JPG_READ_REG_POLL(JPG_BSM_REG_BASE+BSM_RDY_OFFSET, V_BIT_0, V_BIT_0, TIME_OUT_CLK, "bsm: polling bsm status"))
	{
		return JPEG_FAILED;
	}

	/*put SOI*/
	PutMarker(M_SOI);

	/*put APP*/
	//if(PutAPP1(app1_t) != JPEG_SUCCESS)

	if(PutAPP0() != JPEG_SUCCESS)
	{
		return JPEG_FAILED;
	}

	/*put quant tbl*/
	if(PutQuantTbl() != JPEG_SUCCESS)
	{
		return JPEG_FAILED;
	}

	/*put SOF0*/
	if(PutSOF0() != JPEG_SUCCESS)
	{
		return JPEG_FAILED;
	}
	/*put huffman tbl*/
	if(PutHuffTbl()!= JPEG_SUCCESS)
	{
		return JPEG_FAILED;
	}
	/*put DRI*/
	if(WriteDRI()!= JPEG_SUCCESS)
	{
		return JPEG_FAILED;
	}
	/*put SOS*/
	if(PutSOS()!= JPEG_SUCCESS)
	{
		return JPEG_FAILED;
	}

	return JPEG_SUCCESS;
}

PUBLIC JPEG_RET_E JPEG_HWWriteTail(void)
{
	int ret =0;
	ret =JPG_READ_REG_POLL(JPG_VLC_REG_BASE+VLC_CTRL_OFFSET, V_BIT_31, 0, TIME_OUT_CLK, "VLC_CTRL_OFF: polling the vlc is done!");
	if(0 != ret)
	{
		SCI_TRACE_LOW("JPG_READ_REG_POLL timeout %s %d",__FUNCTION__,__LINE__);
	}
	//clear vlc, Hardware should flush vlc internal buffer and byte align(if the last aligned byte value is 0xff, then 0x00 will be followed).
	JPG_WRITE_REG(JPG_VLC_REG_BASE+VLC_CTRL_OFFSET, 1, "VLC_ST_OFF: clear vlc module");

#if _CMODEL_
	write_nbits(0xffd9, 16, 0);
#endif //_CMODEL_

	ret = JPG_READ_REG_POLL(JPG_BSM_REG_BASE+BSM_RDY_OFFSET, 1, 1, TIME_OUT_CLK, "BSM_READY: polling bsm rfifo ready");
	if(0 != ret)
	{
		SCI_TRACE_LOW("JPG_READ_REG_POLL timeout %s %d",__FUNCTION__,__LINE__);
	}
	JPG_WRITE_REG(JPG_BSM_REG_BASE+BSM_CFG2_OFFSET, (16<<24), "BSM_CFG2: configure 16 bit for writing");
	JPG_WRITE_REG(JPG_BSM_REG_BASE+BSM_WDATA_OFFSET, 0xffd9, "BSM_WDATA: configure the value to be written to bitstream");

	if(JPEG_HWWaitingEnd() != TRUE)
	{
		return JPEG_FAILED;
	}

	return JPEG_SUCCESS;
}

static int  save_jpgenc_file2debug()
{
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGEncCodec();
	uint8*				bitstream_ptr = jpeg_fw_codec->g_stream_buf_ptr;
	uint32    				bitstream_len = jpeg_fw_codec->pingpang_buf_len;
	FILE* jpg = fopen("/data/misc/media/enc_failed.jpg","wb");

	if(NULL == jpg)
	{
		SCI_TRACE_LOW("failed to open file /data/misc/media/enc_failed.jpg \n");
		return 0;
	}

	if(NULL == bitstream_ptr || bitstream_len == 0)
	{
		SCI_TRACE_LOW("null enc bitstream \n");
		fclose(jpg);
		return 0;
	}

	fwrite(bitstream_ptr,1,bitstream_len,jpg );
	fclose(jpg);
	return 0;
}
PUBLIC uint32  JPEG_HWGetSize(void)
{
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGEncCodec();
//	SCI_ASSERT(jpeg_fw_codec);
      uint8* buf = jpeg_fw_codec->g_stream_buf_ptr;
	  int i= 0;

	if(ALONE_MODE == jpeg_fw_codec->work_mode)
	{
		//clear vlc, Hardware should flush vlc internal buffer and byte align(if the last aligned byte value is 0xff, then 0x00 will be followed).
	// 	VSP_WRITE_REG(VSP_VLC_REG_BASE+VLC_ST_OFF, 1, "VLC_ST_OFF: clear vlc module");

	//	jpeg_fw_codec->encoded_stream_len = (VSP_READ_REG(VSP_BSM_REG_BASE+BSM_TOTAL_BITS_OFF, "BSM_TOTAL_BITS: Read the total bits") >>3); //byte.
	}else
	{
	#ifdef SMALL_SYS
		while(!g_int_vlc_done)
		{
			;
		}

		JPG_READ_REG_POLL(JPG_GLB_REG_BASE+GLB_INT_STS_OFFSET, 0x2, 0x2, TIME_OUT_CLK, "DCAM_INT_STS: polling the vlc is done!");
	#endif//SMALL_SYS

	        JPG_READ_REG_POLL(JPG_VLC_REG_BASE+VLC_CTRL_OFFSET, V_BIT_31, 0, TIME_OUT_CLK, "VLC_CTRL_OFF: polling the vlc is done!");

		//clear vlc, Hardware should flush vlc internal buffer and byte align(if the last aligned byte value is 0xff, then 0x00 will be followed).
		JPG_WRITE_REG(JPG_VLC_REG_BASE+VLC_CTRL_OFFSET, 1, "VLC_ST_OFF: clear vlc module");

		if(!jpeg_fw_codec->is_last_slice)
		{
			OutPutRstMarker();
		}
	}

	//get the length;
	JPG_WRITE_REG(JPG_BSM_REG_BASE+BSM_CFG2_OFFSET, 2, "BSM_CFG2: Move data remain in FIFO to external memory");
	JPG_READ_REG_POLL(JPG_BSM_REG_BASE+BSM_STS0_OFFSET, V_BIT_31, V_BIT_31, TIME_OUT_CLK, "BSM_DEBUG: polling bsm status");
#if _CMODEL_
	clear_bsm_fifo();
	//clear_bsm_bfr(1, 0);
#endif

	jpeg_fw_codec->encoded_stream_len = (JPG_READ_REG(JPG_BSM_REG_BASE+BSM_TOTAL_BITS_OFFSET, "BSM_TOTAL_BITS: Read the total bits") >>3); //byte.
	SCI_TRACE_LOW("jpeg_fw_codec->encoded_stream_len %d", jpeg_fw_codec->encoded_stream_len);
	if(SWITCH_MODE == jpeg_fw_codec->work_mode) //return the remain bitstream byte length in current ping-pang buffer.
	{
		jpeg_fw_codec->encoded_stream_len %= jpeg_fw_codec->pingpang_buf_len;
		SCI_TRACE_LOW("pingpang_buf_len %d", jpeg_fw_codec->pingpang_buf_len);
	}
	SCI_TRACE_LOW("jpeg_fw_codec->encoded_stream_len %d", jpeg_fw_codec->encoded_stream_len);
#if 0
 	if(!(0xff == buf[jpeg_fw_codec->encoded_stream_len-2]&&0xd9==buf[jpeg_fw_codec->encoded_stream_len-1]))
		jpeg_fw_codec->encoded_stream_len += 4*1024*1024;
	if(!(0xff == buf[jpeg_fw_codec->encoded_stream_len-2]&&0xd9==buf[jpeg_fw_codec->encoded_stream_len-1]))
	{
		save_jpgenc_file2debug();
		jpeg_fw_codec->encoded_stream_len = -1;
		SCI_TRACE_LOW("invalid jpg file");
	}
#endif
	while(!(0xff == buf[jpeg_fw_codec->encoded_stream_len-2]&&0xd9==buf[jpeg_fw_codec->encoded_stream_len-1]))
	{
		if((jpeg_fw_codec->encoded_stream_len + 4*1024*1024) <= jpeg_fw_codec->pingpang_buf_len)
		{
			jpeg_fw_codec->encoded_stream_len += 4*1024*1024;
		}
		else
		{
			SCI_TRACE_LOW("FFD9 not found!");
			break;
		}

	}

	if(!(0xff == buf[jpeg_fw_codec->encoded_stream_len-2]&&0xd9==buf[jpeg_fw_codec->encoded_stream_len-1]))
	{
		save_jpgenc_file2debug();
		jpeg_fw_codec->encoded_stream_len = -1;
		SCI_TRACE_LOW("invalid jpg file");
	}

	return jpeg_fw_codec->encoded_stream_len;

#if _CMODEL_
	VSP_Delete_CModel();
#endif
}

PUBLIC void JpegEnc_FwClose(JPEG_ENC_INPUT_PARA_T *jpeg_enc_input)
{
	return;
}

//////////////////////////////////////////////////////////////////////////
#endif //JPEG_ENC
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End
