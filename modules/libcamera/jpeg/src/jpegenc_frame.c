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

#if defined(JPEG_ENC)
//////////////////////////////////////////////////////////////////////////

#if _CMODEL_
void JPEG_EncodeMCU(void)
{
	mbio_module();
	dct_module();
	vlc_module();
}

extern void MBIO_INT_PROC(void);
void START_SW_ENCODE(uint16 total_mcu_num)
{
	JPEG_CODEC_T *JpegCodec = Get_JPEGEncCodec();
	uint32 raw_height = total_mcu_num * JpegCodec->mcu_height * JpegCodec->mcu_width/JpegCodec->width;
	uint16 x = 0, y = 0;
	uint8 *y_coeff = JpegCodec->YUV_Info_0.y_data_ptr;
	uint8 *u_coeff = JpegCodec->YUV_Info_0.u_data_ptr;
	uint8 *v_coeff = JpegCodec->YUV_Info_0.v_data_ptr;
	uint16 mcu_num_x = JpegCodec->mcu_num_x;
	uint16 mcu_num_y = (uint16)(raw_height / (JpegCodec->mcu_height));

	int32 clock_time = 1000;

//	JPEG_CallBack_Frame_To_MCU CopyMCUToCoeff;

	if(JpegCodec->mbio_bfr1_valid)
	{
		y_coeff = JpegCodec->YUV_Info_1.y_data_ptr;
		u_coeff = JpegCodec->YUV_Info_1.u_data_ptr;
		v_coeff = JpegCodec->YUV_Info_1.v_data_ptr;
	}

//	g_mea_reg_ptr->MEA_CFG2 = 0; //reset!

	switch(JpegCodec->input_mcu_info)
	{
	case JPEG_FW_YUV422:
//		CopyMCUToCoeff = CopyCoeffToMCU422_UV;
		g_block_num_in_one_mcu = 4;
		break;
	case JPEG_FW_YUV420:
//		CopyMCUToCoeff = CopyCoeffToMCU420_UV;
		g_block_num_in_one_mcu = 6;
		break;
	default:
		break;
	}

	/*for every MCU do following*/
	for (y = 0; y < mcu_num_y; y++)
	{
		for (x = 0; x < mcu_num_x; x++)
		{
		//	fprintf (g_pfVlcEvent, "y: %d   x: %d\n", y, x);
			if(x==7 && y == 15)
			{
				PRINTF("");
			}
			/*1, copy coeff data to correspondig blocks*/
//			CopyMCUToCoeff((uint8 *)y_coeff, (uint8 *)u_coeff, (uint8 *)v_coeff, x, y);

			/*quantilization is done in huffman encode*/
			/*2, call EncodeMCU*/
			JPEG_EncodeMCU();
		}
	}

	if((JpegCodec->work_mode == SWITCH_MODE) || ((JpegCodec->is_last_slice)&&(JpegCodec->work_mode == ALONE_MODE)))
	{
		g_int_vlc_done = 1;
		clear_vlc();
// 		clear_bsm_bfr(1, 1);

		g_pre_dc_value[0] = 0;
		g_pre_dc_value[1] = 0;
		g_pre_dc_value[2] = 0;
	}

#if 0//defined(TEST_VECTOR)
// 	clear_vlc_buffer();

	/*byte align*/
	{
		extern uint32 jremain_bit_num;
		int stuffingBis;
		int nBits = 32 - jremain_bit_num;  //left bits in bsm

		nBits = nBits & 7;

		stuffingBis = 8 - nBits;

		//if(stuffingBis < 8)
		{
			outputBits_vrf(0, stuffingBis);
		}
	}
#endif
// jpeg_end:
	if(raw_height < JpegCodec->c_height)
	{
		JPEG_TRACE("Slice encoding end...\n");
	}else
	{
		JPEG_TRACE("JPEG encoding Finished...\n\n");
	}

	MBIO_INT_PROC();
}
#endif //_CMODEL_

PUBLIC void START_HW_ENCODE(uint16 total_mcu_num)
{
	int32 cmd = 0;
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGEncCodec();
	ALOGE("total_mcu_num %x",total_mcu_num);
	//VLC Module cfg
	if(ALONE_MODE == jpeg_fw_codec->work_mode)
	{
		cmd = ((jpeg_fw_codec->mcu_num_y * jpeg_fw_codec->mcu_num_x)  & 0xfffff);
		JPG_WRITE_REG(JPG_VLC_REG_BASE+VLC_TOTAL_MCU_OFFSET, cmd, "VLC_CFG_OFF: total mcu number");
	}else
	{
		cmd = (total_mcu_num & 0xfffff);
		JPG_WRITE_REG(JPG_VLC_REG_BASE+VLC_TOTAL_MCU_OFFSET, cmd, "VLC_CFG_OFF: total mcu number of current slice");
	}

	/*start MEA*/
	JPG_WRITE_REG(JPG_MBIO_REG_BASE+MCU_NUM_OFFSET, total_mcu_num&0xfffff, "MEA_MCU_NUM: total mcu number");
	//hardware will auto switch mbio bfr between 0 and 1. sw can't interfere. but sw should fill the corresponding mbio bfr for hw. xiaowei@20090417
	JPG_WRITE_REG(JPG_MBIO_REG_BASE+BUF_STS_OFFSET, 1, "BUF_STS: buf0 is valid now");
	JPG_WRITE_REG(JPG_MBIO_REG_BASE+CTRL_OFFSET, 1, "MBIO_CTRL:  sw is ready for mea start");

#if _CMODEL_
	START_SW_ENCODE(total_mcu_num);
#endif //_CMODEL_
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
