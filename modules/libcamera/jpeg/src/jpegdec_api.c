/******************************************************************************
 ** File Name:      jpegdec_api.c                                            *
 ** Author:          													  *
 ** DATE:           02/15/2011                                                *
 ** Copyright:      2011 Spreadtrum, Incoporated. All Rights Reserved.        *
 ** Description:    The interface functions for jpeg decoder.									  *
 ** Note:           None                                                      *
******************************************************************************/
/******************************************************************************
 **                        Edit History                                       *
 ** ------------------------------------------------------------------------- *
 ** DATE           NAME             DESCRIPTION                               *
 ** 02/15/2011     xiaozhe.wang	         Create.                                  *
******************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "sc8830_video_header.h"
#include "jpegcodec_bufmgr.h"
#include "jpegdec_api.h"
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C"
    {
#endif

//#if defined(JPEG_DEC)

#include "sprd_jpg.h"

#define SPRD_JPG_DRIVER "/dev/sprd_jpg"

#define SLICE_HEIGHT 1024 //the height for slice mode

static uint32_t g_stream_buf_id = 1;  //record the bsm buf id for switch buf. the init buf is 0.
//uint32_t g_stream_buf_size = 0;

//////////////////////////////////////////////////////////////////////////
int JPG_reset_cb_dec(int fd)
{
//	SCI_TRACE_LOW("JPG_reset_cb\n");
	ioctl(fd,JPG_RESET,NULL);
	return 0;
}


uint32_t get_address_align(uint32_t  address_data ,uint32_t align_data)
{
	if( 256 == align_data  )
	{
		if( 0 != address_data % 0x0ff )
		{
			address_data = (address_data & 0x0ff) + 0x100;
		}
		return address_data;

	}
	else
	{
		return  0;
	}
}


LOCAL void JPEGDEC_init_fw_param(JPEGDEC_PARAMS_T *jpegdec_params,
									JPEG_DEC_INPUT_PARA_T *dec_fw_info_ptr)
{
	SCI_TRACE_LOW("enter [JPEGDEC_init_fw_param]   ");
	uint32_t align_width =(( jpegdec_params->width + 15)/16) * 16;
	uint32_t align_height =(( jpegdec_params->height + 7)/8) * 8;
	uint32_t slice_height = (align_height > 1024) ? 1024 : align_height;
	uint32_t align_y_frame_size =  align_width * align_height;
	uint32_t  YUV_Size_Align =slice_height*align_width;

	if(0 != jpegdec_params->set_slice_height)
	{
		slice_height = jpegdec_params->set_slice_height;
		YUV_Size_Align =slice_height*align_width;
		SCI_TRACE_LOW("[JPEGDEC_init_fw_param] set_slice_height = %d  " ,slice_height);
	}
	//YUV_Size_Align = get_address_align(YUV_Size_Align , 256);
	SCI_TRACE_LOW("[JPEGDEC_init_fw_param] get_address_align = %d  " ,YUV_Size_Align);

	dec_fw_info_ptr->data_type = 1;		//output yuv data
	dec_fw_info_ptr->scaling_down_factor = JPEG_SCALING_DOWN_ZERO;

	dec_fw_info_ptr->pingpong_buf_0_ptr = (uint8_t *)((unsigned long)jpegdec_params->stream_phy_buf[0]);
	dec_fw_info_ptr->pingpong_buf_1_ptr = NULL;//(uint8_t *)jpegdec_params->stream_phy_buf[1];
	dec_fw_info_ptr->pingpong_buf_len = jpegdec_params->stream_size + 256 ;


	//****************************** TONY
	dec_fw_info_ptr->read_bitstream  = NULL;
	dec_fw_info_ptr->bitstream_ptr = (uint8_t *)jpegdec_params->stream_virt_buf[0];

	//******************************
	dec_fw_info_ptr->bitstream_len = jpegdec_params->stream_size;
	dec_fw_info_ptr->read_bitstream = NULL;

/*	if(0 == jpegdec_params->set_slice_height)
	{
		dec_fw_info_ptr->yuv_0_addr.y_data_ptr = jpegdec_params->yuv_phy_buf;
		dec_fw_info_ptr->yuv_0_addr.u_data_ptr = jpegdec_params->target_phy_buf_UV;
		dec_fw_info_ptr->yuv_0_addr.v_data_ptr = NULL;
		dec_fw_info_ptr->yuv_1_addr.y_data_ptr = dec_fw_info_ptr->yuv_0_addr.y_data_ptr  + YUV_Size_Align;
		dec_fw_info_ptr->yuv_1_addr.u_data_ptr = dec_fw_info_ptr->yuv_0_addr.u_data_ptr  + YUV_Size_Align;
		dec_fw_info_ptr->yuv_1_addr.v_data_ptr = NULL;
	}
	else*/
	{
		dec_fw_info_ptr->yuv_0_addr.y_data_ptr = (uint8_t*)((unsigned long)jpegdec_params->yuv_phy_buf);
		dec_fw_info_ptr->yuv_0_addr.u_data_ptr = (uint8_t*)((unsigned long)jpegdec_params->target_phy_buf_UV);
		dec_fw_info_ptr->yuv_0_addr.v_data_ptr = NULL;
		dec_fw_info_ptr->yuv_1_addr.y_data_ptr = dec_fw_info_ptr->yuv_0_addr.y_data_ptr;
		dec_fw_info_ptr->yuv_1_addr.u_data_ptr = dec_fw_info_ptr->yuv_0_addr.u_data_ptr;
		dec_fw_info_ptr->yuv_1_addr.v_data_ptr = NULL;
	}

	dec_fw_info_ptr->yuv_0_addr.input_mcu_info = 0;
	dec_fw_info_ptr->yuv_1_addr.input_mcu_info = 0;

#if defined(CHIP_ENDIAN_LITTLE)
	dec_fw_info_ptr->yuv_0_addr.input_endian = 1;
	dec_fw_info_ptr->yuv_1_addr.input_endian = dec_fw_info_ptr->yuv_0_addr.input_endian;
#endif

	dec_fw_info_ptr->dec_buf.buf_size = jpegdec_params->fw_decode_buf_size;
	dec_fw_info_ptr->dec_buf.buf_ptr = jpegdec_params->fw_decode_buf;

	dec_fw_info_ptr->bsm_buf0_valid = 1;
	dec_fw_info_ptr->bsm_buf1_valid = 0;

    	dec_fw_info_ptr->mbio_bfr0_valid = 1;
    	dec_fw_info_ptr->mbio_bfr1_valid = 0;

	dec_fw_info_ptr->work_mode = 0;
	dec_fw_info_ptr->is_first_slice = TRUE;
}


LOCAL JPEG_RET_E 	JPEG_Mem_Copy(   JPEGDEC_PARAMS_T *jpegdec_params  , int mem_direct  )
{
	SCI_TRACE_LOW("enter [JPEG_Mem_Copy]   header_len = %d ,bitstream_len = %d",jpegdec_params->header_len , jpegdec_params->stream_size);
	uint32_t data_len = jpegdec_params->stream_size - jpegdec_params->header_len ;


	if ( 0 == mem_direct)
	{
		SCI_MEMCPY( jpegdec_params->temp_buf_addr ,  (uint8_t *)jpegdec_params->stream_virt_buf[0] , jpegdec_params->stream_size  );
		SCI_MEMCPY( (void *)jpegdec_params->stream_virt_buf[0] ,(void*)((unsigned long)jpegdec_params->temp_buf_addr +jpegdec_params->header_len) ,data_len);
	}
	else
	{
		SCI_MEMCPY( (void *)jpegdec_params->stream_virt_buf[0] ,  jpegdec_params->temp_buf_addr , jpegdec_params->stream_size  );
	}

	return 0;

}


LOCAL JPEG_RET_E JPEGDEC_start_decode(JPEGDEC_PARAMS_T *jpegdec_params)
{
	JPEG_RET_E 				ret_value = JPEG_FAILED;
	JPEG_DEC_OUTPUT_PARA_T jpeg_dec_out_param;
	JPEG_DEC_INPUT_PARA_T 	jpeg_dec_fw_info;
	uint32_t slice_height = SLICE_HEIGHT;

	if(0 != jpegdec_params->set_slice_height)
	{
		slice_height = jpegdec_params->set_slice_height;
	}

	SCI_TRACE_LOW("enter [JPEG_StartDecode] ---1  ");
	SCI_PASSERT(jpegdec_params, ("[JPEG_6600L_StartDecode], context_ptr is NULL"));

	SCI_TRACE_LOW("enter [JPEG_StartDecode] ---2  ");

	SCI_MEMSET(&jpeg_dec_out_param, 0, sizeof(JPEG_DEC_OUTPUT_PARA_T));
	SCI_MEMSET(&jpeg_dec_fw_info, 0, sizeof(JPEG_DEC_INPUT_PARA_T));

	JPEGDEC_init_fw_param(jpegdec_params, &jpeg_dec_fw_info);

	ret_value = JPEG_FWParseHead(&jpeg_dec_fw_info);
	if(JPEG_SUCCESS != ret_value)
	{
		JPEG_PRINT("[JPEG_StartDecode] JPEG_HWParseHead failed = %d", ret_value);
		return ret_value;
	}

	jpegdec_params->header_len = jpeg_dec_fw_info.header_len;
#if 0
	JPEG_Mem_Copy(  jpegdec_params , 0 );
#endif
	SCI_TRACE_LOW("enter [JPEG_FWInitDecInput]   ");
	ret_value = JPEG_FWInitDecInput(&jpeg_dec_fw_info);
	if(JPEG_SUCCESS != ret_value)
	{
		SCI_TRACE_LOW("JPEG_HWDecInit failed = %d", ret_value);
		return ret_value;
	}

	SCI_TRACE_LOW("enter [JPEG_HWDecInit]   ");
	ret_value = JPEG_HWDecInit(&jpeg_dec_fw_info);
	if(JPEG_SUCCESS != ret_value)
	{
		SCI_TRACE_LOW("JPEG_HWWriteHead failed = %d", ret_value);
		return ret_value;
	}

	SCI_TRACE_LOW("[JPEG_6600L_StartDecode] hardware write head done,hear_len=%d",jpeg_dec_fw_info.header_len);

	/*the input width must be mcu aligned width*/
	SCI_TRACE_LOW("enter [JPEG_HWDecStart]   ");
	/*if(jpegdec_params->height > SLICE_HEIGHT){*/
	if(jpegdec_params->set_slice_height != 0){
	//	ret_value = JPEG_HWDecStart(slice_height,  &jpeg_dec_out_param);
	ret_value = JPEG_HWDecStart( jpegdec_params->height,  &jpeg_dec_out_param);
	}
	else{
		ret_value = JPEG_HWDecStart(jpegdec_params->height,&jpeg_dec_out_param);
	}

	SCI_TRACE_LOW("[JPEG_6600L_StartDecode] start dec, src_aligned_width = %d, slice height = %d",
										jpegdec_params->width, jpegdec_params->height);

	return ret_value;
}

PUBLIC JPEG_RET_E JPEGDEC_stop_decode(JPEGDEC_PARAMS_T *jpegdec_params)
{
	JPEG_RET_E	jpeg_ret_value = JPEG_SUCCESS;//JPEG_FAILED;

	SCI_PASSERT(jpegdec_params, ("[JPEG_6600L_StopDecode], context_ptr is NULL"));
	if( JPEG_SUCCESS == jpeg_ret_value)
	{

	}

#if 0
	JPEG_Mem_Copy( jpegdec_params , 1 );
#endif
	SCI_TRACE_LOW("[JPEG_6600L_StopDecode] start dec, stream_size = %d", jpegdec_params->stream_size);

	return jpeg_ret_value;
}

#if 0
void get_regs(void)
{
	int i, value;
	for(i = 0; i < 6; i++)
	{
		value = VSP_READ_REG(VSP_GLB_REG_BASE + i * 4,"");
		SCI_TRACE_LOW("GLB 0x%x : 0x%x", VSP_GLB_REG_BASE + i * 4, value);
	}
	for(i = 0; i < 3; i++)
	{
		value = VSP_READ_REG(VSP_AHBM_REG_BASE + i * 4,"");
		SCI_TRACE_LOW("AHBM 0x%x : 0x%x", VSP_AHBM_REG_BASE + i * 4, value);
	}
	for(i = 0; i < 12; i++)
	{
		value = VSP_READ_REG(VSP_BSM_REG_BASE + i * 4,"");
		SCI_TRACE_LOW("BSM 0x%x : 0x%x", VSP_BSM_REG_BASE + i * 4, value);
	}
	for(i = 0; i < 3; i++)
	{
		value = VSP_READ_REG(VSP_VLC_REG_BASE + i * 4,"");
		SCI_TRACE_LOW("VLC 0x%x : 0x%x", VSP_VLC_REG_BASE + i * 4, value);
	}
	for(i = 0; i < 12; i++)
	{
		value = VSP_READ_REG(VSP_DCAM_REG_BASE + i * 4,"");
		SCI_TRACE_LOW("DCAM 0x%x : 0x%x", VSP_DCAM_REG_BASE + i * 4, value);
	}
	for(i = 0; i < 19; i++)
	{
		value = VSP_READ_REG(VSP_MEA_REG_BASE + i * 4,"");
		SCI_TRACE_LOW("MEA 0x%x : 0x%x", VSP_MEA_REG_BASE + i * 4, value);
	}
}
#endif
void JPGEDEC_Clear_INT(uint32_t mask)
{
	uint32_t value;

	value = JPG_READ_REG(JPG_GLB_REG_BASE + GLB_INT_RAW_OFFSET, "");
	SCI_TRACE_LOW("JPEGDEC GLB_INT_RAW_OFFSET: 0x%x.", value);
	value = JPG_READ_REG(JPG_GLB_REG_BASE + GLB_INT_CLR_OFFSET, "read the interrupt clear bits.");
	value |= mask;
	JPG_WRITE_REG(JPG_GLB_REG_BASE + GLB_INT_CLR_OFFSET, value, "clear the VLC interrupt.");
}

void JPEGDEC_Handle_BSM_INT(jpegdec_callback callback)
{
/*	uint32_t tmp = g_stream_buf_id;

	JPGEDEC_Clear_INT(0x80);
	if(0 == g_stream_buf_id){
		g_stream_buf_id = 1;
	}
	else{
		g_stream_buf_id = 0;
	}
	JPEG_HWSet_BSM_Buf_WriteOnly(g_stream_buf_id);
	SCI_TRACE_LOW("JPEGDEC_Poll_MEA_BSM JPEG_HWSet_BSM_Buf_WriteOnly after.g_stream_buf_id: %d.\n", g_stream_buf_id);
	callback(g_stream_buf_id, g_stream_buf_size, 0);
	SCI_TRACE_LOW("JPEGDEC_Poll_MEA_BSM callback after.\n");	*/
}

void poll_ahb_idle(uint32_t time)
{
	uint32_t vsp_time_out_cnt = 0;

	while (1)
	{
		uint32 ahb_idle = 0;
		ahb_idle = ((*(volatile uint32*)(JPG_GLB_REG_BASE+MST_STS_OFFSET-JPG_GLB_REG_BASE+g_jpg_Vaddr_base) & 0x1));

		if (ahb_idle) //busy
		{
			usleep(1000);
			SCI_TRACE_LOW("JPEGDEC_Poll_MEA_BSM X, AHB IS BUSY.\n");
		}else
		{
			break;
		}
		vsp_time_out_cnt++;

		if (vsp_time_out_cnt > time)
		{
			SCI_TRACE_LOW("JPEGDEC_Poll_MEA_BSM X, fail 2.\n");
		}
	}
}

//poll MEA done and BSM done

uint32_t JPEGDEC_Poll_DBK_BSM(uint32_t time, uint32_t buf_len,  jpegdec_callback callback, uint32_t slice_num)
{
	uint32_t value;
	uint32_t vsp_time_out_cnt = 0;
	uint32_t buf_id = 1;
/*	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();*/

	SCI_TRACE_LOW("JPEGDEC_Poll_DBK_BSM S, slice_num=%d.\n",slice_num);

	while (1)
	{
		value = JPG_READ_REG(JPG_GLB_REG_BASE + GLB_INT_RAW_OFFSET, "read the interrupt register.");
		if(value & 0x8) //for MBIO done
		{
			slice_num--;

			if (slice_num)
			{
				SCI_TRACE_LOW("JPEGDEC_Poll_DBK_BSM: sencod start , buf_id %d,value 0x%x.", buf_id,value);
				if(NULL != callback)
				{
					poll_ahb_idle(time);
					callback(0,0,0);
				}

				/* clear interrupt */
                	        (*(volatile uint32 *)(JPG_MBIO_REG_BASE+BUF_STS_OFFSET-JPG_CTRL_BASE+g_jpg_Vaddr_base)) |= (1<<2);
	                        (*(volatile uint32 *)(JPG_GLB_REG_BASE+GLB_INT_CLR_OFFSET-JPG_CTRL_BASE+g_jpg_Vaddr_base)) |= (1<<3);
	                        (*(volatile uint32 *)(JPG_GLB_REG_BASE+GLB_INT_CLR_OFFSET-JPG_CTRL_BASE+g_jpg_Vaddr_base)) |= (1<<buf_id);

				buf_id = !buf_id;
			}else
			{
				poll_ahb_idle(time);
				if(NULL != callback)
				{
					callback(1,0,0);
				}
				return 0;
			}
		}
		else if(value & 0x4)//for vsp error
		{
			SCI_TRACE_LOW("JPEGDEC_Poll_DBK_BSM:vsp error 0x%x.",value);
			return 1;
		}

		if (vsp_time_out_cnt > time)
		{
			SCI_TRACE_LOW("JPEGDEC_Poll_DBK_BSM X, fail 1,value 0x%x.\n",value);
			return 1;
		}
		vsp_time_out_cnt++;

		usleep(1000);
	}
}

uint32_t JPEGDEC_Poll_DBK_BSM_FOR_SLICE(uint32_t time, uint32_t *buf_id_ptr,  uint32_t *slice_num_ptr)
{
	uint32_t value;
	uint32_t vsp_time_out_cnt = 0;
	uint32_t buf_id = *buf_id_ptr;
/*	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();*/
	uint32_t slice_num = *slice_num_ptr;
	uint32_t ret = 0;

	SCI_TRACE_LOW("JPEGDEC_Poll_DBK_BSM_FOR_SLICE S, slice_num=%d.\n",slice_num);

	while (1)
	{
		value = JPG_READ_REG(JPG_GLB_REG_BASE + GLB_INT_RAW_OFFSET, "read the interrupt register.");
		if(value & 0x8) //for MBIO done
		{
			slice_num--;

			if (slice_num)
			{
				SCI_TRACE_LOW("JPEGDEC_Poll_DBK_BSM_FOR_SLICE: sencod start , buf_id %d,value 0x%x.", buf_id,value);
				poll_ahb_idle(time);
				*buf_id_ptr = buf_id;
				*slice_num_ptr = slice_num;
				ret = 0;
				return 0;
			}else
			{
				poll_ahb_idle(time);
				*slice_num_ptr = slice_num;
				return 0;
			}
		}
		else if(value & 0x4)//for vsp error
		{
			SCI_TRACE_LOW("JPEGDEC_Poll_DBK_BSM_FOR_SLICE:vsp error 0x%x.",value);
			return 1;
		}

		if (vsp_time_out_cnt > time)
		{
			SCI_TRACE_LOW("JPEGDEC_Poll_DBK_BSM_FOR_SLICE X, fail 1,value 0x%x.\n",value);
			return 1;
		}
		vsp_time_out_cnt++;

		usleep(1000);
	}
	return ret;
}


uint32_t JPEGDEC_Poll_MEA_BSM(uint32_t time, uint32_t buf_len,  jpegdec_callback callback)
{
	uint32_t value;
	uint32_t vsp_time_out_cnt = 0;

	SCI_TRACE_LOW("JPEGDEC_Poll_MEA_BSM E.\n");

	while (1)
	{
		value = JPG_READ_REG(JPG_GLB_REG_BASE + GLB_INT_RAW_OFFSET, "read the interrupt register.");
		if(value & 0x1){ //for BSM done
			JPEGDEC_Handle_BSM_INT(callback);
		}
		if(value & 0x8){ //for MEA done
			 JPEG_HWUpdateMBIOBufInfo();
			JPEG_HWSet_MBIO_Buf_ReadOnly(1);
			JPGEDEC_Clear_INT(0x8);
			SCI_TRACE_LOW("JPEGDEC_Poll_MEA_BSM X.\n");
			return 1;
		}
		if (vsp_time_out_cnt > time)
		{
			SCI_TRACE_LOW("JPEGDEC_Poll_MEA_BSM X, fail..\n");
			return 1;
		}
		vsp_time_out_cnt++;
		usleep(1000);
	}
}

//poll VLC done and BSM done
uint32_t JPEGDEC_Poll_VLC_BSM(uint32_t time, uint32_t buf_len,  jpegdec_callback callback)
{
	uint32_t value;
	uint32_t vsp_time_out_cnt = 0;

	SCI_TRACE_LOW("JPEGDEC_Poll_VLC_BSM E.\n");
	while (1)
	{
		value = JPG_READ_REG(JPG_GLB_REG_BASE + GLB_INT_RAW_OFFSET, "read the interrupt register.");
		if(value & 0x2){ //for VLC done
			JPGEDEC_Clear_INT(0x2);
			SCI_TRACE_LOW("JPEGDEC_Poll_VLC_BSM X.\n");
			return 1;
		}
		if(value & 0x1){ //for BSM done
			JPEGDEC_Handle_BSM_INT(callback);
		}

		if (vsp_time_out_cnt > time)
		{
			SCI_TRACE_LOW("JPEGDEC_Poll_VLC_BSM X, fail.\n");
			return 1;
		}
		vsp_time_out_cnt++;
		usleep(1000);
	}
}


/********

jpegdec_params->stream_phy_buf[0];
jpegdec_params->stream_virt_buf[0];
 jpegdec_params->stream_size;

jpegdec_params->yuv_phy_buf;
 YUV_Size_Align;

jpegdec_params->temp_buf_addr;
jpegdec_params->temp_buf_len;

jpegdec_params->fw_decode_buf_size;
jpegdec_params->fw_decode_buf;

***/
#if 0
int JPEGDEC_decode_one_pic(JPEGDEC_PARAMS_T *jpegdec_params,  jpegdec_callback callback)
{
	int32_t jpg_fd = -1;
	void *jpg_addr = NULL;
	uint32_t ret = 0;
	uint32 value = 0, int_val = 0, temp = 0,jpg_clk = 0;
	JPEG_DEC_INPUT_PARA_T input_para_ptr;
	uint32 align_height = ((jpegdec_params->height  + 7)/8)*8;
	uint32_t slice_num = (align_height > 1024) ? 2 : 1;
	uint32_t slice_height = SLICE_HEIGHT;

	if(0 != jpegdec_params->set_slice_height) {
		slice_height = jpegdec_params->set_slice_height;
		slice_num = (jpegdec_params->height%slice_height) ? (jpegdec_params->height/slice_height+1):(jpegdec_params->height/slice_height);
		SCI_TRACE_LOW("JPEGDEC set slice_num = %d .",slice_num);
	}

	if(0 ==(jpg_fd = open(SPRD_JPG_DRIVER,O_RDWR))) {
		SCI_TRACE_LOW("JPEGDEC open jpg module error, jpg_fd: %d.\n", jpg_fd);
		return -1;
	} else {
		jpg_addr = mmap(NULL,SPRD_JPG_MAP_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,jpg_fd,0);
		SCI_TRACE_LOW("JPEGDEC jpg addr 0x%x\n",(uint32_t)jpg_addr);
    }
	SCI_TRACE_LOW("JPEGDEC_decode_one_pic --1 ");
	ret =  ioctl(jpg_fd,JPG_ACQUAIRE,NULL);
	if(ret){
		SCI_TRACE_LOW("JPG hardware timeout try again %d\n",ret);
		ret =  ioctl(jpg_fd,JPG_ACQUAIRE,NULL);
		if(ret){
			SCI_TRACE_LOW("JPG hardware timeout give up %d\n",ret);
			ret = -1;
			goto error;
		}
	}
	SCI_TRACE_LOW("JPEGDEC_decode_one_pic --2 ");
	ioctl(jpg_fd,JPG_ENABLE,NULL);
	ioctl(jpg_fd,JPG_RESET,NULL);
	ioctl(jpg_fd,JPG_CONFIG_FREQ,&jpg_clk);
	SCI_TRACE_LOW("JPEGDEC_decode_one_pic --3 ");
	JPG_SetVirtualBaseAddr((uint32)jpg_addr);
	SCI_TRACE_LOW("JPEGDEC_decode_one_pic --4 ");
	JPG_reg_reset_callback(JPG_reset_cb_dec,jpg_fd);
	SCI_TRACE_LOW("JPEGDEC_decode_one_pic ---5 ");
	if(JPEG_SUCCESS != JPEGDEC_start_decode(jpegdec_params)) {
		SCI_TRACE_LOW("JPEGDEC fail to JPEGDEC_start_decode.");
		ret = -1;
		goto error;
	}
	//poll the end of jpeg decoder
	if( JPEG_SUCCESS != JPEGDEC_Poll_DBK_BSM(0xFFF, jpegdec_params->stream_buf_len,callback, slice_num)) {
		SCI_TRACE_LOW("JPEGDEC fail to JPEGDEC decode.");
		ret = -1;
		goto error;
	}
	SCI_TRACE_LOW("JPEGDEC decode   JPEGDEC_Poll_DBK_BSM ---1 end ");

	if(JPEG_SUCCESS != JPEGDEC_stop_decode(jpegdec_params)) {
		SCI_TRACE_LOW("JPEGDEC fail to JPEGDEC_stop_decode.");
		ret = -1;
		goto error;
	}
error:
	munmap(jpg_addr,SPRD_JPG_MAP_SIZE);
	ioctl(jpg_fd,JPG_DISABLE,NULL);
	ioctl(jpg_fd,JPG_RELEASE,NULL);
	if(jpg_fd >= 0){
		close(jpg_fd);
	}
	return ret;
}
#endif
int JPEGDEC_Slice_Start(JPEGDEC_PARAMS_T *jpegdec_params,  JPEGDEC_SLICE_OUT_T *out_ptr)
{
	int jpg_fd = -1;
	void *jpg_addr = NULL;
	uint32_t ret = 0;
	uint32 value = 0, int_val = 0, temp = 0,jpg_clk = 0;
	JPEG_DEC_INPUT_PARA_T input_para_ptr;
	uint32 align_height = ((jpegdec_params->height  + 7)/8)*8;
	uint32_t slice_num = (align_height > 1024) ? 2 : 1;
	uint32_t slice_height = SLICE_HEIGHT;
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();

	if(0 != jpegdec_params->set_slice_height) {
		slice_height = jpegdec_params->set_slice_height;
		slice_num = (jpegdec_params->height%slice_height) ? (jpegdec_params->height/slice_height+1):(jpegdec_params->height/slice_height);
		SCI_TRACE_LOW("JPEGDEC set slice_num = %d .",slice_num);
	}
/*	jpg_fd = open(SPRD_JPG_DRIVER,O_RDWR);
	if(jpg_fd < 0) {
		SCI_TRACE_LOW("JPEGDEC open jpg module error, jpg_fd: %d.\n", jpg_fd);
		return -1;
    } else {
		jpg_addr = mmap(NULL,SPRD_JPG_MAP_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,jpg_fd,0);
		SCI_TRACE_LOW("JPEGDEC  jpg addr 0x%x\n",(uint32_t)jpg_addr);
    }*/
	jpg_fd = jpeg_fw_codec->fd;
	jpg_addr = jpeg_fw_codec->jpg_addr;

	if ((jpg_fd < 0) || (NULL == jpg_addr)) {
		SCI_TRACE_LOW("JPEGDEC_Slice_Start, param err %d", jpg_fd);
		return -1;
	}

	SCI_TRACE_LOW("JPEGDEC_Slice_Start --1 ");
    ret =  ioctl(jpg_fd,JPG_ACQUAIRE,NULL);
	if(ret){
		SCI_TRACE_LOW("JPG hardware timeout try again %d\n",ret);
		ret =  ioctl(jpg_fd,JPG_ACQUAIRE,NULL);
		if(ret){
			SCI_TRACE_LOW("JPG hardware timeout give up %d\n",ret);
			ret = -1;
			goto slice_error;
		}
	}
	SCI_TRACE_LOW("JPEGDEC_Slice_Start --2 ");
	ioctl(jpg_fd,JPG_ENABLE,NULL);
	ioctl(jpg_fd,JPG_RESET,NULL);
	ioctl(jpg_fd,JPG_CONFIG_FREQ,&jpg_clk);
	SCI_TRACE_LOW("JPEGDEC_Slice_Start --3 ");
	JPG_SetVirtualBaseAddr((unsigned long)jpg_addr);
	SCI_TRACE_LOW("JPEGDEC_Slice_Start --4 ");
	JPG_reg_reset_callback(JPG_reset_cb_dec,jpg_fd);
	SCI_TRACE_LOW("JPEGDEC_Slice_Start ---5 ");
	if(JPEG_SUCCESS != JPEGDEC_start_decode(jpegdec_params)) {
		SCI_TRACE_LOW("JPEGDEC fail to JPEGDEC_start_decode.");
		ret = -1;
		goto slice_error;
	}
	jpeg_fw_codec->slice_num = slice_num;
	jpeg_fw_codec->buf_id = 1;
	//poll the end of jpeg decoder
	if( JPEG_SUCCESS != JPEGDEC_Poll_DBK_BSM_FOR_SLICE(0xFFF, &jpeg_fw_codec->buf_id,&jpeg_fw_codec->slice_num)) {
		SCI_TRACE_LOW("JPEGDEC fail to JPEGDEC decode.");
		ret = -1;
		goto slice_error;
	} else {
		goto slice_start_end;
	}
slice_error:
/*	munmap(jpg_addr,SPRD_JPG_MAP_SIZE);*/
	ioctl(jpg_fd,JPG_DISABLE,NULL);
	ioctl(jpg_fd,JPG_RELEASE,NULL);

/*	if(jpg_fd >= 0){
 		close(jpg_fd);
 	}*/
slice_start_end:
/*	jpeg_fw_codec->fd =jpg_fd;
	jpeg_fw_codec->addr = (uint32)jpg_addr;*/
	if(0 == jpeg_fw_codec->slice_num) {
		out_ptr->is_over = 1;

/*		munmap(jpg_addr,SPRD_JPG_MAP_SIZE);*/
		ioctl(jpg_fd,JPG_DISABLE,NULL);
		ioctl(jpg_fd,JPG_RELEASE,NULL);

/*		if(jpg_fd >= 0){
 			close(jpg_fd);
 		}*/

	}
	SCI_TRACE_LOW("JPEGDEC_Slice_start end,slice_num %d.",jpeg_fw_codec->slice_num);
	return ret;
}

int JPEGDEC_Slice_Next(JPEGDEC_SLICE_NEXT_T *update_params,  JPEGDEC_SLICE_OUT_T *out_ptr)
{
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();
	int ret =JPEG_SUCCESS;

	JpegDec_HwTopUpdateYUVAddr(update_params->yuv_phy_buf,update_params->yuv_u_phy_buf,update_params->yuv_v_phy_buf);
	JPEGDEC_HWUpdateMBIOBufInfo();

	/* clear interrupt */
	(*(volatile uint32 *)(JPG_MBIO_REG_BASE+BUF_STS_OFFSET-JPG_CTRL_BASE+g_jpg_Vaddr_base)) |= (1<<2);
	(*(volatile uint32 *)(JPG_GLB_REG_BASE+GLB_INT_CLR_OFFSET-JPG_CTRL_BASE+g_jpg_Vaddr_base)) |= (1<<3);
	(*(volatile uint32 *)(JPG_GLB_REG_BASE+GLB_INT_CLR_OFFSET-JPG_CTRL_BASE+g_jpg_Vaddr_base)) |= (1<<jpeg_fw_codec->buf_id);

	jpeg_fw_codec->buf_id = !jpeg_fw_codec->buf_id;
	SCI_TRACE_LOW("buf id %d.",jpeg_fw_codec->buf_id);
	ret = JPEGDEC_Poll_DBK_BSM_FOR_SLICE(0xFFF,&jpeg_fw_codec->buf_id,&jpeg_fw_codec->slice_num);

	if((JPEG_SUCCESS != ret) || (0 == jpeg_fw_codec->slice_num)) {
		out_ptr->is_over = 1;
		SCI_TRACE_LOW("dec finish.");
		munmap((void*)jpeg_fw_codec->addr,SPRD_JPG_MAP_SIZE);
		ioctl(jpeg_fw_codec->fd,JPG_DISABLE,NULL);
		ioctl(jpeg_fw_codec->fd,JPG_RELEASE,NULL);
		if(jpeg_fw_codec->fd >= 0){
			close(jpeg_fw_codec->fd);
		}
		return ret;
	}

	SCI_TRACE_LOW("JPEGDEC_Slice_Next,slice_num %d.",jpeg_fw_codec->slice_num);
	return ret;
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

