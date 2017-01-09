/******************************************************************************
 ** File Name:      jpegdec_out.c                                            *
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

void JPEGFW_OutMCU444(uint8 *y_coeff, uint8 *uv_coeff, uint16 x, uint16 y, uint8 scale_down_factor)
{
	uint16 i = 0, j = 0;
	uint32 width = 0;
	uint8  output_num_blk = 8>>scale_down_factor ;

	uint8  *srcdata = NULL, *u_srcdata = NULL, *v_srcdata = NULL;
	uint8  *destdata = NULL, *u_destdata = NULL, *v_destdata = NULL;
	uint16 x_offset = 0, y_offset = 0;
	JPEG_PROGRESSIVE_INFO_T *progressive_info = JPEGFW_GetProgInfo();
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();

	//output one Y block 8x8
	{
		width = jpeg_fw_codec->out_width;
		x_offset = x * output_num_blk;
		y_offset = y * output_num_blk;
		destdata = y_coeff + width * y_offset + x_offset;
		srcdata = progressive_info->org_blocks[0];

		//output to frame
		for (i = 0; i < output_num_blk; i++)
		{
			for(j = 0; j < output_num_blk; j++)
			{
				destdata[j] = srcdata[j];
			}
			destdata += width;
			srcdata += output_num_blk;
		}
	}

	//output 8x4 U, V block
	{
		x_offset = x * output_num_blk;
		y_offset = y * output_num_blk;

		u_destdata = uv_coeff + width * y_offset + x_offset;
		v_destdata = uv_coeff + width * y_offset + x_offset+1;
		u_srcdata = progressive_info->org_blocks[1];
		v_srcdata = progressive_info->org_blocks[2];

		//output to frame
		for (i = 0; i < output_num_blk; i++)
		{
			for(j = 0; j < (output_num_blk>>1); j++)
			{
				u_destdata[j*2] = u_srcdata[2*j];

				v_destdata[j*2] = v_srcdata[2*j];
			}

			u_destdata += width;
			v_destdata += width;
			u_srcdata += (output_num_blk/*>>1*/);
			v_srcdata += (output_num_blk/*>>1*/);
		}
	}
}

void JPEGFW_OutMCU420(uint8 *y_coeff, uint8 *uv_coeff, uint16 x, uint16 y, uint8 scale_down_factor)
{
	uint16  block_id, i = 0, j = 0;
	uint32  width;
	uint8   output_num_mb = (16>>scale_down_factor);
	uint8   output_num_blk = (8>>scale_down_factor);

	uint8   *srcdata = NULL, *u_srcdata = NULL, *v_srcdata = NULL;
	uint8   *destdata = NULL, *u_destdata = NULL, *v_destdata = NULL;
	uint16 x_offset = 0, y_offset = 0;
	JPEG_PROGRESSIVE_INFO_T *progressive_info = JPEGFW_GetProgInfo();
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();

	/*output four Y block 8x8*/
	for (block_id = 0; block_id < 4; block_id++)
	{
		width = jpeg_fw_codec->out_width;
		x_offset = x * output_num_mb + (block_id % 2) * output_num_blk;
		y_offset = y * output_num_mb + (block_id >> 1) * output_num_blk;
		destdata = y_coeff + width * y_offset + x_offset;
		srcdata = progressive_info->org_blocks[block_id];

		//output to frame
		for (i = 0; i < output_num_blk; i++)
		{
			for(j = 0; j < output_num_blk; j++)
			{
				destdata[j] = srcdata[j];
			}
			destdata += width;
			srcdata += output_num_blk;
		}
	}
	//output one 8*8 U block and one 8*8 V block
	{
		x_offset = x * output_num_mb;
		y_offset = y * output_num_blk;
		u_destdata = uv_coeff + width * y_offset + x_offset;
		v_destdata = uv_coeff + width * y_offset + x_offset+1;
		u_srcdata = progressive_info->org_blocks[block_id++];
		v_srcdata = progressive_info->org_blocks[block_id];

		for (i = 0; i < output_num_blk; i++)
		{
			for(j = 0; j < output_num_blk; j++)
			{
				u_destdata[j*2] = u_srcdata[j];

				v_destdata[j*2] = v_srcdata[j];
			}
			u_destdata += width;
			v_destdata += width;
			u_srcdata += output_num_blk;
			v_srcdata += output_num_blk;
		}
	}
}

void JPEGFW_OutMCU400(uint8 *y_coeff, uint8 *uv_coeff, uint16 x, uint16 y, uint8 scale_down_factor)
{

}

void JPEGFW_OutMCU411(uint8 *y_coeff, uint8 *uv_coeff, uint16 x, uint16 y, uint8 scale_down_factor)
{

}

void JPEGFW_OutMCU422(uint8 *y_coeff, uint8 *uv_coeff, uint16 x, uint16 y, uint8 scale_down_factor)
{

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
