/******************************************************************************
 ** File Name:       jpeg_fw_def.h                                         	  *
 ** Author:          Frank.Yang		                                          *
 ** DATE:            06/25/2008                                               *
 ** Copyright:      2008 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:                                                              *
 ******************************************************************************

 ******************************************************************************
 **                        Edit History                                       *
 ** ------------------------------------------------------------------------- *
 ** DATE              NAME             DESCRIPTION                            *
 ******************************************************************************/

#ifndef _JPEG_FW_DEF_H_
#define _JPEG_FW_DEF_H_

#ifdef   __cplusplus
    extern   "C"
    {
#endif

#include "sci_types.h"
#include "jpeg_common.h"
//#include "jpeg_exif_header.h"

//#if !defined(_SIMULATION_)
//#include "os_api.h"
//#endif

typedef BOOLEAN (*JINF_READ_FILE_FUNC)(uint8 *buf, uint32 offset, uint32 bytes_to_read, uint32 *bytes_read_ptr);

/*function of write JPEG stream data or RGB data.*/
typedef BOOLEAN (*JINF_WRITE_FILE_FUNC)(uint8 *buf_ptr, uint32 file_offset, uint32 bytes_to_write, uint32 *written_bytes_ptr);


typedef struct yuv_format_tag
{
	uint8 	*y_data_ptr;
	uint8 	*u_data_ptr;
	uint8 	*v_data_ptr;
	uint32 	input_mcu_info;
	uint32  out_mcu_info;
	uint32	input_endian; //0: big, 1: little.
}YUV_FORMAT_T;

typedef struct mem_structure_tag
{
	void 	*buf_ptr;
	uint32 	 buf_size;
}JPEG_MEMORY_T;

typedef struct enc_input_para_tag
{
	uint32				width;
	uint32				height;
	uint8* 	 			stream_buf0;
	uint8*				stream_buf1;
	uint32 	 			bitstream_buf_len;
	YUV_FORMAT_T		yuv_0_info;
	YUV_FORMAT_T		yuv_1_info;
	JPEG_MEMORY_T		enc_buf;
	JPEG_QUALITY_E		quant_level;

	uint8				work_mode; //0: only encoder, 1: switch decoder and encoder.
	BOOLEAN				mbio_bfr0_valid; //0: invalid, 1: valid
	BOOLEAN				mbio_bfr1_valid; //0: invalid, 1: valid
	BOOLEAN 			bsm_buf0_valid;//0: invalid, 1: valid
	BOOLEAN				bsm_buf1_valid;//0: invalid, 1: valid
	BOOLEAN				is_first_slice;
	BOOLEAN				is_last_slice;
	uint32				restart_interval;
	int32 				y_interleaved;
	int32 				uv_interleaved;
} JPEG_ENC_INPUT_PARA_T;

typedef struct enc_output_para_tag
{
	uint32   output_bitstream_size;
} JPEG_ENC_OUTPUT_PARA_T;

typedef struct jpeg_dec_input_param_tag
{
	uint16				data_type;				//0:RGB  1:YUV
	uint16	 			scaling_down_factor;
	uint8* 	 			pingpong_buf_0_ptr;
	uint8* 	 			pingpong_buf_1_ptr;
	uint32    				pingpong_buf_len;
	YUV_FORMAT_T		yuv_0_addr;			// yuv data address
	YUV_FORMAT_T		yuv_1_addr;			// yuv data address
	JPEG_MEMORY_T 		dec_buf;
	JPEG_QUALITY_E		quant_level;
	JINF_READ_FILE_FUNC  read_bitstream;
	BOOLEAN				progressive_mode;	//added by xiaowei.luo,20090113

	uint32				input_width;
	uint32				input_height;
	uint32				input_mcu_info;

	uint8				work_mode; //0: only decoder, 1: switch decoder and encoder.
	BOOLEAN				mbio_bfr0_valid; //0: invalid, 1: valid
	BOOLEAN				mbio_bfr1_valid; //0: invalid, 1: valid
	BOOLEAN 			bsm_buf0_valid;//0: invalid, 1: valid
	BOOLEAN				bsm_buf1_valid;//0: invalid, 1: valid

	BOOLEAN				is_first_slice;
	uint8*				bitstream_ptr;	//bitstream start address, contain the header.
	uint32    				bitstream_len;
	uint32				header_len;		//header length, return after parsing header.
}JPEG_DEC_INPUT_PARA_T;

typedef struct jpeg_dec_outputparam_tag
{
   	uint32 				output_width;		// width of output image
	uint32 				output_height;		// height of output image

}JPEG_DEC_OUTPUT_PARA_T;
typedef struct fraction_type
{
	uint32 numerator;
	uint32 denominator;
}FRACTION_TYPE_T;
typedef struct app1_t{
	uint32 thumb_width;
	uint32 thumb_height;
	uint32 thumbnail_len;
	void *thumbnail_virt_addr;
	FRACTION_TYPE_T Latitude_dd;
	FRACTION_TYPE_T Latitude_mm;
	FRACTION_TYPE_T Latitude_ss;
	uint32 Latitude_ref; //0: North latitude, 1: South latitude
	FRACTION_TYPE_T Longitude_dd;
	FRACTION_TYPE_T Longitude_mm;
	FRACTION_TYPE_T Longitude_ss;
	uint32 Longitude_ref; //0: East Longitude, 1: West Longitude
	const char *image_description;
	const char *make;
	const char *model;
	const char *copyright;
	uint32 orientation;
	const char *datetime;
	const char *gps_date;
	const char *gps_process_method;
	uint32 gps_hour;
	uint32 gps_minuter;
	uint32 gps_second;
	uint32 image_width;
	uint32 image_height;
	FRACTION_TYPE_T focal_length;

	uint32 	stream_buf_len;
//	JINF_EXIF_INFO_T *dc_exif_info_ptr;
}APP1_T;

///////////////////////////////////////////////////////////////////////////
//								Common									///
///////////////////////////////////////////////////////////////////////////

/*waiting the AHB master is idle*/
PUBLIC BOOLEAN	  JPEG_HWWaitingEnd(void);
/*reset the vsp*/
PUBLIC void		  JPEG_HWResetVSP(void);
/*set the bsm pingpang buffer can be read by hardware, for JPEG decoding*/
PUBLIC void		  JPEG_HWSet_BSM_Buf_ReadOnly(uint8 buf_id);
/*set the bsm pingpang buffer can be write by hardware, for JPEG encoding*/
PUBLIC void		  JPEG_HWSet_BSM_Buf_WriteOnly(uint8 buf_id);
/*set the mbio pingpang buffer can be read by hardware, for JPEG encoding*/
PUBLIC void		  JPEG_HWSet_MBIO_Buf_ReadOnly(uint8 buf_id);
/*set the mbio pingpang buffer can be write by hardware, for JPEG decoding*/
PUBLIC void		  JPEG_HWSet_MBIO_Buf_WriteOnly(uint8 buf_id);

///////////////////////////////////////////////////////////////////////////
//								Decode									///
///////////////////////////////////////////////////////////////////////////

/*head parsing by firmware*/
PUBLIC JPEG_RET_E JPEG_FWParseHead(JPEG_DEC_INPUT_PARA_T *input_para_ptr);

/*initialization of firmware when normal jpeg picture decoding*/
PUBLIC JPEG_RET_E JPEG_HWDecInit(JPEG_DEC_INPUT_PARA_T *input_para_ptr);

/*jpeg decoding by hardware*/
PUBLIC JPEG_RET_E JPEG_HWDecStart(uint32 num_of_rows, JPEG_DEC_OUTPUT_PARA_T *output_para_ptr);

/*decode defined number of MCU according to synchronism mode by hardware*/
PUBLIC JPEG_RET_E JPEG_HWDecStartMCUSynchro(uint32 num_of_rows, JPEG_DEC_OUTPUT_PARA_T *output_para_ptr);

/*read out bitstream information when current slice decoding is finished. only used in switch mode*/
PUBLIC JPEG_RET_E JpegDec_FwReadoutDecInfo(uint32 *bitstrm_byte_offset_ptr, uint32 bitstrm_bfr_switch_cnt);

/*close decoder and release resource*/
PUBLIC void JpegDec_FwClose(JPEG_DEC_INPUT_PARA_T *jpeg_dec_input);

PUBLIC void JpegDec_BackupLast100ByteBs(uint8 *bitstrm_ptr, uint32 pingpang_buf_len)	;

PUBLIC JPEG_RET_E JpegDec_SetFlushWordFlag(int32 flag);

///////////////////////////////////////////////////////////////////////////
//								Encode									///
///////////////////////////////////////////////////////////////////////////

/*initialization of encoding by hardware*/
PUBLIC JPEG_RET_E JPEG_HWEncInit(JPEG_ENC_INPUT_PARA_T *input_para_ptr);

/*head writting by hardware*/
PUBLIC JPEG_RET_E JPEG_HWWriteHead(APP1_T *app1_t);
//PUBLIC JPEG_RET_E JPEG_HWWriteHead(void);
PUBLIC JPEG_RET_E JPEG_HWWriteHeadForThumbnail(void);

/*tail writting by hardware*/
PUBLIC JPEG_RET_E JPEG_HWWriteTail(void);
PUBLIC JPEG_RET_E JPEG_HWWriteTailForThumbnail(void);

/*start the hardware to encode the jpeg*/
PUBLIC JPEG_RET_E JPEG_HWEncStart(uint32 width, uint32 height, JPEG_ENC_OUTPUT_PARA_T *output_para_ptr);

/*obtain the encoded size of bitstream*/
PUBLIC uint32	JPEG_HWGetSize(void);

/*close encoder and release resource*/
PUBLIC void JpegEnc_FwClose(JPEG_ENC_INPUT_PARA_T *jpeg_enc_input);

PUBLIC void JPEG_HWUpdateMBIOBufInfo(void);
PUBLIC void JPEGDEC_HWUpdateMBIOBufInfo(void);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End
#endif
