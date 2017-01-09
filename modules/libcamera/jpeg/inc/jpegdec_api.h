/******************************************************************************
 ** File Name:      jpegdec_api.c                                            *
 ** Author:         xiaozhe.wang													  *
 ** DATE:           02/15/2011                                                *
 ** Copyright:      2011 Spreadtrum, Incoporated. All Rights Reserved.        *
 ** Description:    The interface functions for jpeg encoder.									  *
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
//#include "sc8810_video_header.h"

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/

#ifndef JPEGDEC_API_H
#define JPEGDEC_API_H

#ifdef   __cplusplus
    extern   "C"
    {
#endif

//#if defined(JPEG_DEC)
//////////////////////////////////////////////////////////////////////////
typedef void (*jpegdec_callback)(uint32_t buf_id, uint32_t stream_size, uint32_t is_last_slice);

typedef enum
{
	JPEGDEC_YUV_420 = 0,
	JPEGDEC_YUV_422,
	JPEGDEC_YUV_MAX
}JPEGDEC_FORMAT_E;
typedef enum
{
	JPEGDEC_QUALITY_LOW = 0,				//bad
	JPEGDEC_QUALITY_MIDDLE_LOW,			//poor
	JPEGDEC_QUALITY_MIDDLE,				//good
	JPEGDEC_QUALITY_MIDDLE_HIGH,			//excellent
	JPEGDEC_QUALITY_HIGH,					//oustanding
	JPEGDEC_QUALITY_MAX
}JPEGDEC_QUALITY_E;

typedef struct jpegdec_input_params
{
	JPEGDEC_FORMAT_E format;
	uint32_t 	width;
	uint32_t 	height;
	uint32_t   src_size;
	void *	src_virt_buf;
	uint32_t	src_phy_buf;
         void *	target_virt_buf_Y;
	uint32_t	target_phy_buf_Y;
	void *	target_virt_buf_UV;
	uint32_t	target_phy_buf_UV;
	uint32_t   slice_height;
	uint32_t  crop_x;
	uint32_t  crop_y;
	uint32_t  crop_width;
	uint32_t  crop_height;
	jpegdec_callback write_yuv_callback;
}JPEGDEC_INPUT_PARAMS_T;

typedef struct jpegdec_params
{
	JPEGDEC_FORMAT_E format;
	uint32_t 	width;
	uint32_t 	height;
	uint32_t   set_slice_height;
	void *	yuv_virt_buf;
	uint32_t 	yuv_phy_buf;

	uint32_t  	yuv_phy_buf_size;
	void *	stream_virt_buf[2];
	uint32_t 	stream_phy_buf[2];
	uint32_t 	stream_buf_len;
	uint32_t 	stream_size;
	JPEGDEC_QUALITY_E quality;

	void *	src_buf;
         void *	target_buf_Y;
	void *	target_buf_UV;

	uint32_t	src_phy_buf;
	uint32_t	target_phy_buf_Y;
	uint32_t   target_phy_buf_UV;

	uint32_t  crop_x;
	uint32_t  crop_y;
	uint32_t  crop_width;
	uint32_t  crop_height;

	void * 	temp_buf_addr;
	uint32_t	temp_buf_len;

	uint32_t 	header_len;

	//uint32_t   jpeg_buf_size;

	uint32_t   fw_decode_buf_size;
	void * 	fw_decode_buf;

}JPEGDEC_PARAMS_T;

typedef struct
{
	uint32_t slice_height;
	void *yuv_virt_buf;/*virtual address for Y*/
	uint32_t yuv_phy_buf;/*physical address for Y*/
	void *yuv_u_virt_buf;/*virtual address for U*/
	uint32_t yuv_u_phy_buf;/*physical address for U*/
	void *yuv_v_virt_buf;/*virtual address for V*/
	uint32_t yuv_v_phy_buf;/*physical address for V*/
}JPEGDEC_SLICE_NEXT_T;

typedef struct
{
	uint32_t	total_height;
	uint32_t	is_over;/*0: ongoing; 1: over*/
}JPEGDEC_SLICE_OUT_T;

int JPEGDEC_decode_one_pic(JPEGDEC_PARAMS_T *jpegdec_params, jpegdec_callback callback);

uint32_t get_address_align(uint32_t  ,uint32_t );
int JPEGDEC_Slice_Start(JPEGDEC_PARAMS_T *jpegdec_params,  JPEGDEC_SLICE_OUT_T *out_ptr);
int JPEGDEC_Slice_Next(JPEGDEC_SLICE_NEXT_T *update_params,  JPEGDEC_SLICE_OUT_T *out_ptr);

//////////////////////////////////////////////////////////////////////////
//#endif //JPEG_ENC
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/

#endif
// End
