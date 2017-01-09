/******************************************************************************
** File Name:      JpegDec_init.c                                             *
** Author:         yi.wang													  *
** DATE:           07/12/2007                                                 *
** Copyright:      2007 Spreadtrum, Incoporated. All Rights Reserved.         *
** Description:    Initialize the encoder									  *
** Note:           None                                                       *
*******************************************************************************/
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
#if defined(_VSP_) && defined(SMALL_SYS)

typedef enum
{
	SENSOR_SOF_ID = 0,
	SENSOR_EOF_ID,
	CAP_SOF_ID,
	CAP_EOF_ID,
	ISP_TX_DONE_ID,
	SC_DONE_ID,
	ISP_BUFFER_OVF_ID,
	VSP_BSM_DONE_ID,
	VSP_VLC_DONE_ID,
	VSP_MBIO_DOEN_ID,
	CAP_FIFO_DONE_ID,
	JPEG_BUF_OVF_ID,
	VSP_TIMEOUT_ID,
	VSP_VLD_ERROR_ID,
	ISP_INT_ID_MAX
}DCAM_MODULE_INT_ID_E;
#endif

//#if defined(JPEG_DEC)
//////////////////////////////////////////////////////////////////////////

#if defined(_ARM_) && defined(SMALL_SYS)
extern void BSM_INT_PROC(void);
extern void MBIO_INT_PROC(void);
extern void TIME_OUT_INT_PROC(void);
extern void VLC_DONE_INT_PROC(void);
extern void VLD_ERROR_INT_PROC(void);
#endif
/************************************************************************/
/* JPEG_HW_Cfg, Enable top reg                                          */
/************************************************************************/
PUBLIC void JpegDec_HwTopRegCfg(void)
{
	uint32 cmd = 0;
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();
//	uint32 pTableAddr = (uint32)VSP_MEMO10_ADDR;
	uint32 int_mask = 0;
	uint32 endian_sel = 0;

#if _CMODEL_
	AllocMCUBuf();
	VSP_Init_CModel();
#endif

#if 0
	/*reset vsp*/
	VSP_Reset();
#else
	//backup the INT mask register for the VSP reset will clear it
	int_mask = JPG_READ_REG(JPG_GLB_REG_BASE+GLB_INT_EN_OFFSET, "GLB_INT_EN_OFFSET: read INT bit");

	JPEG_TRACE("[JpegDec_HwTopRegCfg] int mask = 0x%x", int_mask);

	/*reset vsp*/
	JPG_Reset();
#endif

	//DCAM init
	cmd = (1<<0);
	JPG_WRITE_REG(JPG_GLB_REG_BASE+GLB_CTRL_OFFSET, cmd, "DCAM_CFG: DCAM init");

        cmd = (jpeg_fw_codec->c_width & 0x1fff);
        JPG_WRITE_REG(JPG_GLB_REG_BASE+GLB_PITCH_OFFSET, cmd, "configure jpeg pitch, pixel unit");
        cmd =  (1<<3)| (1<<2)|(1<<0);
        //Clear INT
        JPG_WRITE_REG(JPG_GLB_REG_BASE+GLB_INT_CLR_OFFSET, cmd, "GLB_INT_CLR_OFFSET: clear related INT bit");

        //INT Enable
        JPG_WRITE_REG(JPG_GLB_REG_BASE+GLB_INT_EN_OFFSET, cmd, "GLB_INT_EN_OFFSET: enable related INT bit");


#if defined(_VSP_) && defined(SMALL_SYS)
	//enable dcam interrupt
	*(volatile uint32 *)0x71300000 |= (1<<20);	//INTC1 enable
	*(volatile uint32 *)0x71500008 |= (1<<10);	//JPEG interrupt is bit10 of INTC1  42
#endif

	//INT Enable
	if (!(jpeg_fw_codec->is_res_file))
	{
#if 0
		cmd = (1 << 7) | (1 << 9) | (1 << 13);
		JPG_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_MASK_OFF, cmd, "DCAM_INT_MASK: enable related INT bit");
#else
		//restore the INT mask register
		JPG_WRITE_REG(JPG_GLB_REG_BASE+GLB_INT_EN_OFFSET, int_mask, "GLB_INT_EN_OFFSET: enable related INT bit");
		JPEG_TRACE("[JpegDec_HwTopRegCfg] after reset, int mask = 0x%x", JPG_READ_REG(JPG_GLB_REG_BASE+GLB_INT_EN_OFFSET, "DCAM_INT_MASK: read INT bit"));
#endif
	}
	else
	{
		//clear all INTs while decoding SJPG
		int_mask = 0;
		JPG_WRITE_REG(JPG_GLB_REG_BASE+GLB_INT_EN_OFFSET, int_mask, "GLB_INT_EN_OFFSET: enable related INT bit");
	}

	JPEG_TRACE("[JpegDec_HwTopRegCfg] after reset, int mask = 0x%x", JPG_READ_REG(JPG_GLB_REG_BASE+GLB_INT_EN_OFFSET, "DCAM_INT_MASK: read INT bit"));

#if defined(_VSP_) && defined(SMALL_SYS)
	//init int
	DCAMMODULE_Init();

        //INT Enable
        *(volatile uint32 *)0x60b00024 |= (1 << 3) | (1 << 2);

	//register the int fun;
	DCAMMODULE_RegisterIntFunc(VSP_BSM_DONE_ID, BSM_INT_PROC);
	DCAMMODULE_RegisterIntFunc(VSP_MBIO_DOEN_ID, MBIO_INT_PROC);
//	DCAMMODULE_RegisterIntFunc(VSP_TIMEOUT_ID, TIME_OUT_INT_PROC);
//	DCAMMODULE_RegisterIntFunc(VSP_VLD_ERROR_ID, VLD_ERROR_INT_PROC);
#endif

//	open_vsp_iram();
#if _CMODEL_
	JPG_WRITE_REG(JPG_GLB_REG_BASE+GLB_FRM_ADDR0_OFFSET, DEC_FRAME0_Y, "Reconstructed Y0 frame buffer ");
	JPG_WRITE_REG(JPG_GLB_REG_BASE+ GLB_FRM_ADDR1_OFFSET, DEC_FRAME0_UV, "Reconstructed UV0 Frame buffer ");

	JPG_WRITE_REG(JPG_GLB_REG_BASE+GLB_FRM_ADDR2_OFFSET, DEC_FRAME1_Y, "Reconstructed Y1 frame buffer ");
	JPG_WRITE_REG(JPG_GLB_REG_BASE+ GLB_FRM_ADDR3_OFFSET, DEC_FRAME1_UV, "Reconstructed UV1 Frame buffer ");

	JPG_WRITE_REG(JPG_GLB_REG_BASE+ GLB_FRM_ADDR4_OFFSET, BIT_STREAM_DEC_0>>2, "Decoded bit stream buffer0 ");
	JPG_WRITE_REG(JPG_GLB_REG_BASE+ GLB_FRM_ADDR5_OFFSET, BIT_STREAM_DEC_1>>2, "Decoded bit stream buffer1 ");
#else
	JPEG_TRACE("jpeg_fw_codec->YUV_Info_0.y_data_ptr 0x%x YUV_Info_1.y_data_ptr 0x%x",
		jpeg_fw_codec->YUV_Info_0.y_data_ptr,
		jpeg_fw_codec->YUV_Info_1.y_data_ptr);

	JPG_WRITE_REG(JPG_GLB_REG_BASE+GLB_FRM_ADDR0_OFFSET, (unsigned long)(jpeg_fw_codec->YUV_Info_0.y_data_ptr), "Reconstructed Y0 frame buffer ");
	JPG_WRITE_REG(JPG_GLB_REG_BASE+GLB_FRM_ADDR1_OFFSET, (unsigned long)(jpeg_fw_codec->YUV_Info_0.u_data_ptr), "Reconstructed U0 frame buffer ");
	JPG_WRITE_REG(JPG_GLB_REG_BASE+GLB_FRM_ADDR6_OFFSET, (unsigned long)(jpeg_fw_codec->YUV_Info_0.v_data_ptr), "Reconstructed V0 frame buffer ");

	JPG_WRITE_REG(JPG_GLB_REG_BASE+GLB_FRM_ADDR2_OFFSET, (unsigned long)(jpeg_fw_codec->YUV_Info_1.y_data_ptr), "Reconstructed Y1 frame buffer ");
	JPG_WRITE_REG(JPG_GLB_REG_BASE+GLB_FRM_ADDR3_OFFSET, (unsigned long)(jpeg_fw_codec->YUV_Info_1.u_data_ptr), "Reconstructed U1 frame buffer ");
	JPG_WRITE_REG(JPG_GLB_REG_BASE+GLB_FRM_ADDR7_OFFSET, (unsigned long)(jpeg_fw_codec->YUV_Info_1.v_data_ptr), "Reconstructed V1 frame buffer ");

// 	VSP_WRITE_REG (VSP_AHBM_REG_BASE+AHBM_BASE_ADDR_OFFSET, (uint32)jpeg_fw_codec->stream_0>>26, "AHBM_BASE_ADDR: PSRAM base address offset");

	//decoded bitstream addr0 and addr1
	JPG_WRITE_REG(JPG_GLB_REG_BASE+GLB_FRM_ADDR4_OFFSET, (unsigned long)(jpeg_fw_codec->stream_0), "Decoded bit stream buffer0 ");
	JPG_WRITE_REG(JPG_GLB_REG_BASE+GLB_FRM_ADDR5_OFFSET, (unsigned long)(jpeg_fw_codec->stream_1), "Decoded bit stream buffer1 ");//modified by leon @2012.09.27

	JPEG_TRACE("jpeg_fw_codec->YUV_Info_0.y_data_ptr 0x%x YUV_Info_0.u_data_ptr 0x%x",
		jpeg_fw_codec->YUV_Info_0.y_data_ptr,
		jpeg_fw_codec->YUV_Info_0.u_data_ptr);
	JPEG_TRACE("jpeg_fw_codec->stream_0 0x%x ",jpeg_fw_codec->stream_0);
#endif//_CMODEL_

//	close_vsp_iram();

	cmd =(0<<6)| (1 << 4) | (1 << 2)| (0 << 1) | (1 << 0);
	JPG_WRITE_REG(JPG_GLB_REG_BASE+GLB_CTRL_OFFSET, cmd, "GLB_CTRL: enable JPEG decoder");

        jpeg_fw_codec->uv_interleaved = (jpeg_fw_codec->YUV_Info_0.v_data_ptr == NULL) ? 1 : 0;
        cmd = (((!jpeg_fw_codec->uv_interleaved) << 27)) | (jpeg_fw_codec->input_mcu_info << 24) | ((jpeg_fw_codec->mcu_num_y & 0x3ff) << 12) | (jpeg_fw_codec->mcu_num_x & 0x3ff);
        JPG_WRITE_REG(JPG_GLB_REG_BASE+MB_CFG_OFFSET, cmd, "uv_interleaved, input mcu infor, mcu max number x and y");

//	cmd = ((uint32)0xffff << 0) | (0 << 16);
//	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_VSP_TIME_OUT_OFF, cmd, "DCAM_VSP_TIME_OUT: disable hardware timer out");

//set the interval of mcu decoding
//	cmd = (jpeg_fw_codec->YUV_Info_0.v_data_ptr == NULL)?1:0;//1    //1: uv_interleaved, two plane, 0: three plane
//	cmd = (cmd<<8)|0xff;
//	JPG_WRITE_REG(JPG_GLB_REG_BASE+BUS_GAP_OFFSET, cmd, "configure AHB register: BURST_GAP, and frame_mode");
        JPG_WRITE_REG(JPG_GLB_REG_BASE+BUS_GAP_OFFSET, 0, "BUS_GAP: 0");

	return;
}

uint32 head_byte_len;

/************************************************************************/
/* Enable sub module                                                    */
/************************************************************************/
PUBLIC void JpegDec_HwSubModuleCfg(uint32 header_length)
{
	uint32 cmd = 0;
	uint32 down_sample_setting = DOWN_SAMPLE_DIS;
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();

	//Source Size init
	cmd = (((jpeg_fw_codec->c_height) & 0x01fff)<<16) | ((jpeg_fw_codec->c_width) & 0x01fff);
	JPG_WRITE_REG(JPG_GLB_REG_BASE+GLB_PITCH_OFFSET, cmd, "YUV data storage pitch in memory, it is equal to image width normally");

	///1. enable de-stuffing
	///2. reconfigure current address of source bit stream buffer0, word align(the unit is word)
	cmd = header_length>>2;
	JPG_WRITE_REG(JPG_BSM_REG_BASE+BSM_CFG1_OFFSET, ((uint32)1<<31)|cmd, "BSM_CFG1: bitstream offset address"); /*lint !e569*/
	cmd = ((jpeg_fw_codec->pingpang_buf_len+3) >> 2);
	JPG_WRITE_REG(JPG_BSM_REG_BASE+BSM_CFG0_OFFSET, cmd, "BSM_CFG0: buffer0 for read, and the buffer size");

#if 1
	head_byte_len = header_length;
#endif
	//VLD Module cfg
	cmd = ((jpeg_fw_codec->mcu_num_y * jpeg_fw_codec->mcu_num_x)  & 0xfffff );
	JPG_WRITE_REG(JPG_VLD_REG_BASE+JPEG_TOTAL_MCU_OFFSET, cmd, "VLD_TOTAL_MCU: total mcu number.");

	//DCT Module cfg
	cmd = (((jpeg_fw_codec->input_mcu_info) & 0x7) << 9) | (DCT_QUANT_EN << 8) | (DCT_AUTO_MODE << 1) | (IDCT_MODE);
	JPG_WRITE_REG(JPG_DCT_REG_BASE+DCT_CFG_OFFSET, cmd, "DCT_CONFIG: mcu info, quant enable, auto-mode, idct-mode");
	JPG_WRITE_REG(JPG_DCT_REG_BASE+DCT_CFG_DONE_OFFSET, 1, "DCT_CFG_FINISH: dct config finished");

	//MBC Module cfg
	if(JPEG_SCALING_DOWN_ZERO == jpeg_fw_codec->scale_factor)
	{
		down_sample_setting = DOWN_SAMPLE_DIS;
	}else
	{
		down_sample_setting = DOWN_SAMPLE_EN;
	}
	cmd = 2;
	JPG_WRITE_REG(JPG_MBIO_REG_BASE+CFG_OFFSET, cmd, "MBC_CFG: FREE_RUN_MODE");

	//DBK Module cfg
//	VSP_WRITE_REG(VSP_DBK_REG_BASE+DBK_CFG_OFF, (0<<2)|(DBK_RUN_FREE_MODE), "DBK_CFG: disable post-filter and free_run_mode");
}

/************************************************************************/
/* Decode Initiate                                                      */
/************************************************************************/
PUBLIC int32 JPEGFW_UpdateMiscFields(void)
{
	uint16 width = 0, height = 0;
	uint16 v_ratio_max = 0, h_ratio_max = 0;
	uint16 max_width = 4096, max_height = 0;
	uint16 mcu_width = 0, mcu_height = 0;
	uint32 max_mcu_num = 0x3FFFF;
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();

	width = jpeg_fw_codec->width;
	height = jpeg_fw_codec->height;

	/*get the mcu size*/
	v_ratio_max = JPEG_FW_MAX3(jpeg_fw_codec->ratio[JPEG_FW_Y_ID].v_ratio, jpeg_fw_codec->ratio[JPEG_FW_U_ID].v_ratio, jpeg_fw_codec->ratio[JPEG_FW_V_ID].v_ratio);
	h_ratio_max = JPEG_FW_MAX3(jpeg_fw_codec->ratio[JPEG_FW_Y_ID].h_ratio, jpeg_fw_codec->ratio[JPEG_FW_U_ID].h_ratio, jpeg_fw_codec->ratio[JPEG_FW_V_ID].h_ratio);

	mcu_width = jpeg_fw_codec->mcu_width = 8 * h_ratio_max;
	mcu_height = jpeg_fw_codec->mcu_height = 8 * v_ratio_max;

	SCI_ASSERT(jpeg_fw_codec->mcu_height != 0);
	SCI_ASSERT(jpeg_fw_codec->mcu_width != 0);

	if((jpeg_fw_codec->mcu_height == 0) || (jpeg_fw_codec->mcu_width == 0))
	{
		return JPEG_FAILED;
	}
	jpeg_fw_codec->mcu_num_x = (width + jpeg_fw_codec->mcu_width -1)/jpeg_fw_codec->mcu_width;
	jpeg_fw_codec->mcu_num_y = (height + jpeg_fw_codec->mcu_height -1)/jpeg_fw_codec->mcu_height;

	jpeg_fw_codec->c_width = jpeg_fw_codec->mcu_num_x * jpeg_fw_codec->mcu_width;
	jpeg_fw_codec->c_height = jpeg_fw_codec->mcu_num_y * jpeg_fw_codec->mcu_height;
	jpeg_fw_codec->out_width = ((jpeg_fw_codec->c_width) >> (jpeg_fw_codec->scale_factor));
	jpeg_fw_codec->out_height = ((jpeg_fw_codec->c_height) >> (jpeg_fw_codec->scale_factor));

	jpeg_fw_codec->dc_pred_y = 0;
	jpeg_fw_codec->dc_pred_uv = 0;
	jpeg_fw_codec->restart_mcu_cnt = 0;
	jpeg_fw_codec->bitstream_offset = 0;

	if((jpeg_fw_codec->out_width == 0) || (jpeg_fw_codec->out_height == 0))
	{
		return JPEG_FAILED;
	}

	switch(jpeg_fw_codec->input_mcu_info) //code in 6600L, NEED be modified in 6800H? xwluo,20090401
	{
	case JPEG_FW_YUV444:
	case JPEG_FW_YUV422:
	case JPEG_FW_YUV411:
	case JPEG_FW_YUV400:
		max_height = 8192;
		break;
	case JPEG_FW_YUV422_R:
	case JPEG_FW_YUV420:
		max_height = 8192*2;
		break;
	case JPEG_FW_YUV411_R:
		max_height = 8192*4;
		break;
	default:
		return JPEG_FAILED;
	}

	if((jpeg_fw_codec->c_width > max_width) || (jpeg_fw_codec->c_height > max_height) ||
		((uint32)(jpeg_fw_codec->mcu_num_x*jpeg_fw_codec->mcu_num_y) >= max_mcu_num))
	{
		return JPEG_FAILED;
	}


	return JPEG_SUCCESS;
}

//@yi.wang added for resource pic decode and interleaved dec and enc
PUBLIC JPEG_RET_E JPEG_FWInitDecInput(JPEG_DEC_INPUT_PARA_T *jpeg_dec_input)
{
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();
	JPEG_RET_E ret = JPEG_SUCCESS;

	//Load parameter into JPEG Codec
	jpeg_fw_codec->stream_0					= jpeg_dec_input->pingpong_buf_0_ptr;
	jpeg_fw_codec->stream_1					= jpeg_dec_input->pingpong_buf_1_ptr;
	jpeg_fw_codec->YUV_Info_0.y_data_ptr	= jpeg_dec_input->yuv_0_addr.y_data_ptr;
	jpeg_fw_codec->YUV_Info_0.u_data_ptr	= jpeg_dec_input->yuv_0_addr.u_data_ptr;
	jpeg_fw_codec->YUV_Info_0.v_data_ptr	= jpeg_dec_input->yuv_0_addr.v_data_ptr;
	jpeg_fw_codec->YUV_Info_1.y_data_ptr	= jpeg_dec_input->yuv_1_addr.y_data_ptr;
	jpeg_fw_codec->YUV_Info_1.u_data_ptr	= jpeg_dec_input->yuv_1_addr.u_data_ptr;
	jpeg_fw_codec->YUV_Info_1.v_data_ptr	= jpeg_dec_input->yuv_1_addr.v_data_ptr;
	jpeg_fw_codec->pingpang_buf_len			= jpeg_dec_input->pingpong_buf_len;
	jpeg_fw_codec->scale_factor				= (uint8)jpeg_dec_input->scaling_down_factor;
	jpeg_fw_codec->decoded_stream_len		= jpeg_dec_input->bitstream_len;
	jpeg_fw_codec->out_put_dataType			= (uint8)jpeg_dec_input->data_type;
	jpeg_fw_codec->read						= jpeg_dec_input->read_bitstream;
	//added by xiaowei.luo,20090113
	jpeg_fw_codec->progressive_mode			= jpeg_dec_input->progressive_mode;
	jpeg_fw_codec->width					= (uint16)jpeg_dec_input->input_width;
	jpeg_fw_codec->height					= (uint16)jpeg_dec_input->input_height;
	jpeg_fw_codec->input_mcu_info			= (uint8)jpeg_dec_input->input_mcu_info;
	jpeg_fw_codec->work_mode				= (uint8)jpeg_dec_input->work_mode;
	jpeg_fw_codec->is_first_slice			= jpeg_dec_input->is_first_slice;
	jpeg_fw_codec->mbio_bfr0_valid			= jpeg_dec_input->mbio_bfr0_valid;
	jpeg_fw_codec->mbio_bfr1_valid			= jpeg_dec_input->mbio_bfr1_valid;
	jpeg_fw_codec->bsm_buf0_valid		= jpeg_dec_input->bsm_buf0_valid;
	jpeg_fw_codec->bsm_buf1_valid		= jpeg_dec_input->bsm_buf1_valid;
	jpeg_fw_codec->compress_level			= jpeg_dec_input->quant_level;

	if(jpeg_dec_input->scaling_down_factor > 2)
	{
		JPEG_TRACE("Invalid scaling down factor, which must be 0 ~ 2\n");
	//	return JPEG_FAILED;
	}

	SCI_MEMCPY(&jpeg_fw_codec->YUV_Info_0, &(jpeg_dec_input->yuv_0_addr), (sizeof(YUV_FORMAT_T)));
	SCI_MEMCPY(&jpeg_fw_codec->YUV_Info_1, &(jpeg_dec_input->yuv_1_addr), (sizeof(YUV_FORMAT_T)));

	switch(jpeg_fw_codec->input_mcu_info)
	{
	case JPEG_FW_YUV420:
		jpeg_fw_codec->ratio[JPEG_FW_Y_ID].v_ratio = 2;
		jpeg_fw_codec->ratio[JPEG_FW_Y_ID].h_ratio = 2;
		jpeg_fw_codec->ratio[JPEG_FW_U_ID].v_ratio = 1;
		jpeg_fw_codec->ratio[JPEG_FW_U_ID].h_ratio = 1;
		jpeg_fw_codec->ratio[JPEG_FW_V_ID].v_ratio = 1;
		jpeg_fw_codec->ratio[JPEG_FW_V_ID].h_ratio = 1;
		break;
	case JPEG_FW_YUV411:
		jpeg_fw_codec->ratio[JPEG_FW_Y_ID].v_ratio = 1;
		jpeg_fw_codec->ratio[JPEG_FW_Y_ID].h_ratio = 4;
		jpeg_fw_codec->ratio[JPEG_FW_U_ID].v_ratio = 1;
		jpeg_fw_codec->ratio[JPEG_FW_U_ID].h_ratio = 1;
		jpeg_fw_codec->ratio[JPEG_FW_V_ID].v_ratio = 1;
		jpeg_fw_codec->ratio[JPEG_FW_V_ID].h_ratio = 1;
		break;
	case JPEG_FW_YUV444:
		jpeg_fw_codec->ratio[JPEG_FW_Y_ID].v_ratio = 1;
		jpeg_fw_codec->ratio[JPEG_FW_Y_ID].h_ratio = 1;
		jpeg_fw_codec->ratio[JPEG_FW_U_ID].v_ratio = 1;
		jpeg_fw_codec->ratio[JPEG_FW_U_ID].h_ratio = 1;
		jpeg_fw_codec->ratio[JPEG_FW_V_ID].v_ratio = 1;
		jpeg_fw_codec->ratio[JPEG_FW_V_ID].h_ratio = 1;
		break;
	case JPEG_FW_YUV422:
		jpeg_fw_codec->ratio[JPEG_FW_Y_ID].v_ratio = 1;
		jpeg_fw_codec->ratio[JPEG_FW_Y_ID].h_ratio = 2;
		jpeg_fw_codec->ratio[JPEG_FW_U_ID].v_ratio = 1;
		jpeg_fw_codec->ratio[JPEG_FW_U_ID].h_ratio = 1;
		jpeg_fw_codec->ratio[JPEG_FW_V_ID].v_ratio = 1;
		jpeg_fw_codec->ratio[JPEG_FW_V_ID].h_ratio = 1;
		break;
	case JPEG_FW_YUV400:
		jpeg_fw_codec->ratio[JPEG_FW_Y_ID].v_ratio = 1;
		jpeg_fw_codec->ratio[JPEG_FW_Y_ID].h_ratio = 1;
		jpeg_fw_codec->ratio[JPEG_FW_U_ID].v_ratio = 0;
		jpeg_fw_codec->ratio[JPEG_FW_U_ID].h_ratio = 0;
		jpeg_fw_codec->ratio[JPEG_FW_V_ID].v_ratio = 0;
		jpeg_fw_codec->ratio[JPEG_FW_V_ID].h_ratio = 0;
		break;
	case JPEG_FW_YUV422_R:
		jpeg_fw_codec->ratio[JPEG_FW_Y_ID].v_ratio = 2;
		jpeg_fw_codec->ratio[JPEG_FW_Y_ID].h_ratio = 1;
		jpeg_fw_codec->ratio[JPEG_FW_U_ID].v_ratio = 1;
		jpeg_fw_codec->ratio[JPEG_FW_U_ID].h_ratio = 1;
		jpeg_fw_codec->ratio[JPEG_FW_V_ID].v_ratio = 1;
		jpeg_fw_codec->ratio[JPEG_FW_V_ID].h_ratio = 1;
		break;
	case JPEG_FW_YUV411_R:
		jpeg_fw_codec->ratio[JPEG_FW_Y_ID].v_ratio = 4;
		jpeg_fw_codec->ratio[JPEG_FW_Y_ID].h_ratio = 1;
		jpeg_fw_codec->ratio[JPEG_FW_U_ID].v_ratio = 1;
		jpeg_fw_codec->ratio[JPEG_FW_U_ID].h_ratio = 1;
		jpeg_fw_codec->ratio[JPEG_FW_V_ID].v_ratio = 1;
		jpeg_fw_codec->ratio[JPEG_FW_V_ID].h_ratio = 1;
		break;
	default:
	//	PRINTF("Sample Format is undefined!\n");
		return JPEG_FAILED;
	}

	ret = JPEGFW_UpdateMiscFields();
	if(ret != JPEG_SUCCESS)
	{
		return ret;
	}

	return JPEG_SUCCESS;
}

//for progressive
PUBLIC void JPEGFW_AllocMCUBuf(void)
{
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();
	JPEG_PROGRESSIVE_INFO_T *progressive_info = JPEGFW_GetProgInfo();
	uint16 block_id = 0, block_num = 0;
	uint8 component_id = 0, i = 0;

	progressive_info->block_line[0] = (int16*)JpegDec_ExtraMemAlloc(64*sizeof(int16)*jpeg_fw_codec->mcu_num_x*jpeg_fw_codec->ratio[0].h_ratio*jpeg_fw_codec->ratio[0].v_ratio);
	progressive_info->block_line[1] = (int16*)JpegDec_ExtraMemAlloc(64*sizeof(int16)*jpeg_fw_codec->mcu_num_x);
	progressive_info->block_line[2] = (int16*)JpegDec_ExtraMemAlloc(64*sizeof(int16)*jpeg_fw_codec->mcu_num_x);

	block_id = 0;
	for (component_id = 0; component_id < 3; component_id++)
	{
		block_num = (jpeg_fw_codec->ratio[component_id].h_ratio) * (jpeg_fw_codec->ratio[component_id].v_ratio);
		for (i=0; i<block_num; i++)
		{
			//progressive_info->blocks[block_id] = (int16*)JpegDec_ExtraMemAlloc(64*sizeof(int16)); //g_mcu_buf + block_id * 64;
			progressive_info->org_blocks[block_id] = (uint8*)JpegDec_ExtraMemAlloc(64*sizeof(uint8)); //g_mcu_org_buf +block_id * 64;
			progressive_info->blocks_membership[block_id] = component_id;
			block_id++;
		}
	}
}
PUBLIC void JpegDec_HwTopUpdateYUVAddr(uint32 y_phy_addr,uint32_t u_phy_addr,uint32_t v_phy_addr)
{
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();

	jpeg_fw_codec->YUV_Info_0.y_data_ptr = (unsigned char*)((unsigned long)y_phy_addr);
	jpeg_fw_codec->YUV_Info_1.y_data_ptr = (unsigned char*)((unsigned long)y_phy_addr);
	jpeg_fw_codec->YUV_Info_0.u_data_ptr = (unsigned char*)((unsigned long)u_phy_addr);
	jpeg_fw_codec->YUV_Info_1.u_data_ptr = (unsigned char*)((unsigned long)u_phy_addr);
	jpeg_fw_codec->YUV_Info_0.v_data_ptr = (unsigned char*)((unsigned long)v_phy_addr);
	jpeg_fw_codec->YUV_Info_1.v_data_ptr = (unsigned char*)((unsigned long)v_phy_addr);

	JPG_WRITE_REG(JPG_GLB_REG_BASE+GLB_FRM_ADDR0_OFFSET, (unsigned long)(jpeg_fw_codec->YUV_Info_0.y_data_ptr), "Reconstructed Y0 frame buffer ");
	JPG_WRITE_REG(JPG_GLB_REG_BASE+GLB_FRM_ADDR1_OFFSET, (unsigned long)(jpeg_fw_codec->YUV_Info_0.u_data_ptr), "Reconstructed U0 frame buffer ");
	JPG_WRITE_REG(JPG_GLB_REG_BASE+GLB_FRM_ADDR6_OFFSET, (unsigned long)(jpeg_fw_codec->YUV_Info_0.v_data_ptr), "Reconstructed V0 frame buffer ");

	JPG_WRITE_REG(JPG_GLB_REG_BASE+GLB_FRM_ADDR2_OFFSET, (unsigned long)(jpeg_fw_codec->YUV_Info_1.y_data_ptr), "Reconstructed Y1 frame buffer ");
	JPG_WRITE_REG(JPG_GLB_REG_BASE+GLB_FRM_ADDR3_OFFSET, (unsigned long)(jpeg_fw_codec->YUV_Info_1.u_data_ptr), "Reconstructed U1 frame buffer ");
	JPG_WRITE_REG(JPG_GLB_REG_BASE+GLB_FRM_ADDR7_OFFSET, (unsigned long)(jpeg_fw_codec->YUV_Info_1.v_data_ptr), "Reconstructed V1 frame buffer ");

	SCI_TRACE_LOW("jpeg, update yu addr,0x%x,0x%x.\n",y_phy_addr,u_phy_addr);
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
