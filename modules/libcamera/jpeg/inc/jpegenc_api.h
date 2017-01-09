/******************************************************************************
 ** File Name:      jpegenc_api.c                                            *
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

#ifndef JPEG_ENC_API_H
#define JPEG_ENC_API_H

#ifdef   __cplusplus
    extern   "C"
    {
#endif



#include "jpeg_exif_header.h"

//////////////////////////////////////////////////////////////////////////
typedef void (*jpegenc_callback)(uint32_t buf_id, uint32_t stream_size, uint32_t is_last_slice);

typedef enum
{
	JPEGENC_YUV_420 = 0,
	JPEGENC_YUV_422,
	JPEGENC_YUV_MAX
}JPEGENC_FORMAT_E;
typedef enum
{
	JPEGENC_QUALITY_LOW = 0,				//bad
	JPEGENC_QUALITY_MIDDLE_LOW,			//poor
	JPEGENC_QUALITY_MIDDLE,				//good
	JPEGENC_QUALITY_MIDDLE_HIGH,			//excellent
	JPEGENC_QUALITY_HIGH,					//oustanding
	JPEGENC_QUALITY_MAX
}JPEGENC_QUALITY_E;
typedef struct fraction_t
{
	uint32_t numerator;
	uint32_t denominator;
}FRACTION_T;
typedef struct jpegenc_params
{
	JPEGENC_FORMAT_E format;
	uint32_t width;
	uint32_t height;
	uint32_t set_slice_height;
	void *yuv_virt_buf;/*virtual address for Y*/
	uint32_t yuv_phy_buf;/*physical address for Y*/
	void *yuv_u_virt_buf;/*virtual address for U*/
	uint32_t yuv_u_phy_buf;/*physical address for U*/
	void *yuv_v_virt_buf;/*virtual address for V*/
	uint32_t yuv_v_phy_buf;/*physical address for V*/
	void *stream_virt_buf[2];
	uint32_t stream_phy_buf[2];
	uint32_t stream_buf_len;
	uint32_t stream_size;
	JPEGENC_QUALITY_E quality;
	jpegenc_callback read_callback;
	uint32_t y_interleaved;
	uint32_t uv_interleaved;/*0: 3 plane;  1: 2 plane uvuv;  2: 2 plane vuvu*/

#if 0

    //exif information
 	uint32_t thumb_width;
	uint32_t thumb_height;
	uint32_t thumb_quality;
	void *thumb_src_yuv_virt_buf;
	uint32_t thumb_src_yuv_phy_buf;
	FRACTION_T Latitude_dd;
	FRACTION_T Latitude_mm;
	FRACTION_T Latitude_ss;
	uint32_t Latitude_ref; //0: North latitude, 1: South latitude
	FRACTION_T Longitude_dd;
	FRACTION_T Longitude_mm;
	FRACTION_T Longitude_ss;
	uint32_t Longitude_ref; //0: East Longitude, 1: West Longitude
	const char *image_description;
	const char *make;
	const char *model;
	const char *copyright;
	uint32_t orientation;
	const char *datetime;
	const char *gps_date;
	const char *gps_process_method;
	uint32_t gps_hour;
	uint32_t gps_minuter;
	uint32_t gps_second;
	FRACTION_T focal_length;

	JINF_EXIF_INFO_T *dc_exif_info_ptr;

#endif

}JPEGENC_PARAMS_T;

typedef struct
{
	uint32_t slice_height;
	void *yuv_virt_buf;/*virtual address for Y*/
	uint32_t yuv_phy_buf;/*physical address for Y*/
	void *yuv_u_virt_buf;/*virtual address for U*/
	uint32_t yuv_u_phy_buf;/*physical address for U*/
	void *yuv_v_virt_buf;/*virtual address for V*/
	uint32_t yuv_v_phy_buf;/*physical address for V*/
}JPEGENC_SLICE_NEXT_T;


typedef struct
{
	uint32_t	stream_size;
	uint32_t	is_over;/*0: ongoing; 1: over*/
}JPEGENC_SLICE_OUT_T;

uint32_t JPEGENC_encode_one_pic(JPEGENC_PARAMS_T *jpegenc_params, jpegenc_callback callback);

int JPEGENC_Slice_Start(JPEGENC_PARAMS_T *jpegenc_params, JPEGENC_SLICE_OUT_T *out_ptr);

uint32_t JPEGENC_Slice_Next(JPEGENC_SLICE_NEXT_T *update_parm_ptr, JPEGENC_SLICE_OUT_T *out_ptr);
int adjust_jpg_resolution(void* jpg_buf,int jpg_size,int width,int height);
int JPEGCODEC_Open(void);
int JPEGCODEC_Close(void);

//////////////////////////////////////////////////////////////////////////

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/


#endif //JPEG_ENC
// End
