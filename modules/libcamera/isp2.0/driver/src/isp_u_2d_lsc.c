/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "isp_u_2d_lsc"

#include "isp_drv.h"

#define ISP_LSC_BUF_SIZE                             (32 * 1024)

/*use to cal Q value, the code from kernel*/
#define CLIP1(input,top, bottom) {if (input>top) input = top; if (input < bottom) input = bottom;}
#define TABLE_LEN_128	128
#define TABLE_LEN_96	96

typedef struct lnc_bicubic_weight_t_64_tag
{
	int16_t w0;
	int16_t w1;
	int16_t w2;
}LNC_BICUBIC_WEIGHT_TABLE_T;

static LNC_BICUBIC_WEIGHT_TABLE_T lnc_bicubic_weight_t_96_simple[] =
{
	{0,	1024,	0},
	{-5,1024,	6},
	{-10,1023,	12},
	{-15,1022,	18},
	{-20,1020,	25},
	{-24,1017,	32},
	{-28,1014,	40},
	{-32,1011,	48},
	{-36,1007,	56},
	{-39,1003,	65},
	{-43,998,	74},
	{-46,993,	83},
	{-49,987,	93},
	{-52,981,	103},
	{-54,974,	113},
	{-57,967,	124},
	{-59,960,	135},
	{-61,952,	146},
	{-63,944,	158},
	{-65,936,	170},
	{-67,927,	182},
	{-68,918,	194},
	{-70,908,	206},
	{-71,898,	219},
	{-72,888,	232},
	{-73,878,	245},
	{-74,867,	258},
	{-74,856,	272},
	{-75,844,	285},
	{-75,833,	299},
	{-76,821,	313},
	{-76,809,	327},
	{-76,796,	341},
	{-76,784,	356},
	{-76,771,	370},
	{-75,758,	384},
	{-75,745,	399},
	{-75,732,	414},
	{-74,718,	428},
	{-73,704,	443},
	{-73,691,	458},
	{-72,677,	473},
	{-71,663,	487},
	{-70,648,	502},
	{-69,634,	517},
	{-68,620,	532},
	{-67,605,	547},
	{-65,591,	561},
	{-64,576,	576},
};

static LNC_BICUBIC_WEIGHT_TABLE_T lnc_bicubic_weight_t_128_simple[] =
{
	 {0,	1024,	0},
	 {-4,	1024,	4},
	 {-8,	1023,	8},
	 {-11,	1023,	13},
	 {-15,	1022,	18},
	 {-18,	1020,	23},
	 {-22,	1019,	28},
	 {-25,	1017,	34},
	 {-28,	1014,	40},
	 {-31,	1012,	46},
	 {-34,	1009,	52},
	 {-37,	1006,	58},
	 {-39,	1003,	65},
	 {-42,	999,	72},
	 {-44,	995,	78},
	 {-47,	991,	86},
	 {-49,	987,	93},
	 {-51,	982,	101},
	 {-53,	978,	108},
	 {-55,	973,	116},
	 {-57,	967,	124},
	 {-59,	962,	132},
	 {-60,	956,	141},
	 {-62,	950,	149},
	 {-63,	944,	158},
	 {-65,	938,	167},
	 {-66,	931,	176},
	 {-67,	925,	185},
	 {-68,	918,	194},
	 {-69,	910,	203},
	 {-70,	903,	213},
	 {-71,	896,	222},
	 {-72,	888,	232},
	 {-73,	880,	242},
	 {-73,	872,	252},
	 {-74,	864,	262},
	 {-74,	856,	272},
	 {-75,	847,	282},
	 {-75,	839,	292},
	 {-75,	830,	303},
	 {-76,	821,	313},
	 {-76,	812,	324},
	 {-76,	803,	334},
	{-76,	793,	345},
	{-76,	784,	356},
	{-76,	774,	366},
	{-76,	765,	377},
	{-75,	755,	388},
	{-75,	745,	399},
	{-75,	735,	410},
	{-74,	725,	421},
	{-74,	715,	432},
	{-73,	704,	443},
	{-73,	694,	454},
	{-72,	684,	465},
	{-72,	673,	476},
	{-71,	663,	487},
	{-70,	652,	498},
	{-69,	641,	510},
	{-69,	631,	521},
	{-68,	620,	532},
	{-67,	609,	543},
	{-66,	598,	554},
	{-65,	587,	565},
	{-64,	576,	576},
};

static uint16_t ISP_Cubic1D(uint16_t a, uint16_t b, uint16_t c, uint16_t d, uint16_t u, uint16_t box)
{
	int32_t out_value;
	uint16_t out_value_uint16_t;
	int16_t w0, w1, w2, w3;
	uint32_t out_value_tmp;
	int16_t sub_tmp0;
	int16_t sub_tmp1;
	int16_t sub_tmp2;

	if(box ==96)
	{
		//use simple table
		if ( u < (TABLE_LEN_96/2 + 1) )
		{
			w0 = lnc_bicubic_weight_t_96_simple[u].w0 ;
			w1 = lnc_bicubic_weight_t_96_simple[u].w1 ;
			w2 = lnc_bicubic_weight_t_96_simple[u].w2 ;

			sub_tmp0 = a-d;
			sub_tmp1 = b-d;
			sub_tmp2 = c-d;
			out_value_tmp = ((uint32_t)d)<<10;
			out_value = out_value_tmp + sub_tmp0 * w0  + sub_tmp1 * w1 + sub_tmp2 * w2;
		}
		else
		{
			w1 = lnc_bicubic_weight_t_96_simple[TABLE_LEN_96 - u].w2 ;
			w2 = lnc_bicubic_weight_t_96_simple[TABLE_LEN_96 - u].w1 ;
			w3 = lnc_bicubic_weight_t_96_simple[TABLE_LEN_96 - u].w0 ;

			sub_tmp0 = b-a;
			sub_tmp1 = c-a;
			sub_tmp2 = d-a;
			out_value_tmp = ((uint32_t)a)<<10;
			out_value = out_value_tmp + sub_tmp0 * w1  + sub_tmp1 * w2 + sub_tmp2 * w3;
		}

	}
	else
	{
		u = u * (TABLE_LEN_128 / box);

		//use simple table
		if ( u < (TABLE_LEN_128/2 + 1) )
		{
			w0 = lnc_bicubic_weight_t_128_simple[u].w0 ;
			w1 = lnc_bicubic_weight_t_128_simple[u].w1 ;
			w2 = lnc_bicubic_weight_t_128_simple[u].w2 ;

			sub_tmp0 = a-d;
			sub_tmp1 = b-d;
			sub_tmp2 = c-d;
			out_value_tmp = ((uint32_t)d)<<10;
			out_value = out_value_tmp + sub_tmp0 * w0  + sub_tmp1 * w1 + sub_tmp2 * w2;
		}
		else
		{
			w1 = lnc_bicubic_weight_t_128_simple[TABLE_LEN_128 - u].w2 ;
			w2 = lnc_bicubic_weight_t_128_simple[TABLE_LEN_128 - u].w1 ;
			w3 = lnc_bicubic_weight_t_128_simple[TABLE_LEN_128 - u].w0 ;

			sub_tmp0 = b-a;
			sub_tmp1 = c-a;
			sub_tmp2 = d-a;
			out_value_tmp = ((uint32_t)a)<<10;
			out_value = out_value_tmp + sub_tmp0 * w1  + sub_tmp1 * w2 + sub_tmp2 * w3;
		}
	}

	//CLIP(out_value, 4095*1024*2, 1024*1024);	// for LSC gain, 1024 = 1.0, 4095 = 4.0 ; 4095*2 is for boundary extension.
	CLIP1(out_value, 16383*1024, 1024*1024);	// for LSC gain, 1024 = 1.0, 16383 = 16.0 ; 16383 is for boundary extension, 14 bit parameter is used.

	out_value_uint16_t = (uint16_t)((out_value + 512) >> 10);

	return out_value_uint16_t;
}


static int32_t ISP_GenerateQValues(uint32_t word_endian, uint32_t q_val[][5], unsigned long param_address, uint16_t grid_w, uint16_t grid_num, uint16_t u)
{
	uint8_t i;
	uint16_t a0 = 0, b0 = 0, c0 = 0, d0 = 0;
	uint16_t a1 = 0, b1 = 0, c1 = 0, d1 = 0;
	uint32_t *addr = (uint32_t *)param_address;

	if (param_address == 0x0 || grid_num == 0x0) {
		ISP_LOGE("ISP_GenerateQValues param_address error addr=0x%lx grid_num=%d \n",param_address, grid_num);
		return -1;
	}


	#if 0
	for (i = 0; i < 5; i++) {
		A0  = (uint16_t)*(addr + i * 2) & 0xFFFF;
		B0  = (uint16_t)*(addr + i * 2 + grid_w * 2) & 0xFFFF;
		C0  = (uint16_t)*(addr + i * 2 + grid_w * 2 * 2) & 0xFFFF;
		D0  = (uint16_t)*(addr + i * 2 + grid_w * 2 * 3) & 0xFFFF;
		A1  = (uint16_t)(*(addr + i * 2) >> 16);
		B1  = (uint16_t)(*(addr + i * 2 + grid_w * 2) >> 16);
		C1  = (uint16_t)(*(addr + i * 2 + grid_w * 2 * 2) >> 16);
		D1  = (uint16_t)(*(addr + i * 2 + grid_w * 2 * 3) >> 16);
		q_val[0][i] = ISP_Cubic1D(A0, B0, C0, D0, u, grid_num);
		q_val[1][i] = ISP_Cubic1D(A1, B1, C1, D1, u, grid_num);
	}
	#endif
	#if 1
	for (i = 0; i < 5; i++) {
		if (1 == word_endian) {	// ABCD = 1 word
			a0  = *(addr + i * 2) >> 16;					// AB
			b0  = *(addr + i * 2 + grid_w * 2) >> 16;       // AB
			c0  = *(addr + i * 2 + grid_w * 2 * 2) >> 16;   // AB
			d0  = *(addr + i * 2 + grid_w * 2 * 3) >> 16;   // AB
			a1  = *(addr + i * 2) & 0xFFFF;					// CD
			b1  = *(addr + i * 2 + grid_w * 2) & 0xFFFF;    // CD
			c1  = *(addr + i * 2 + grid_w * 2 * 2) & 0xFFFF;// CD
			d1  = *(addr + i * 2 + grid_w * 2 * 3) & 0xFFFF;// CD
		} else if (2 == word_endian) { // CDAB = 1 word
			a0  = *(addr + i * 2) & 0xFFFF;
			b0  = *(addr + i * 2 + grid_w * 2) & 0xFFFF;
			c0  = *(addr + i * 2 + grid_w * 2 * 2) & 0xFFFF;
			d0  = *(addr + i * 2 + grid_w * 2 * 3) & 0xFFFF;
			a1  = *(addr + i * 2) >> 16;
			b1  = *(addr + i * 2 + grid_w * 2) >> 16;
			c1  = *(addr + i * 2 + grid_w * 2 * 2) >> 16;
			d1  = *(addr + i * 2 + grid_w * 2 * 3) >> 16;
		} else if (0 == word_endian) { // DCBA = 1 word
			a0  = ((*(addr + i * 2) << 8) & 0x0000FF00) | ((*(addr + i * 2) >> 8) & 0x000000FF);
			b0  = ((*(addr + i * 2 + grid_w * 2) << 8) & 0x0000FF00) | ((*(addr + i * 2 + grid_w * 2) >> 8) & 0x000000FF);
			c0  = ((*(addr + i * 2 + grid_w * 2 * 2) << 8) & 0x0000FF00) | ((*(addr + i * 2 + grid_w * 2 * 2) >> 8) & 0x000000FF);
			d0  = ((*(addr + i * 2 + grid_w * 2 * 3) << 8) & 0x0000FF00) | ((*(addr + i * 2 + grid_w * 2 * 3) >> 8) & 0x000000FF);
			a1  = ((*(addr + i * 2) << 8) & 0xFF000000) | ((*(addr + i * 2) >> 8) & 0x00FF0000);
			b1  = ((*(addr + i * 2 + grid_w * 2) << 8) & 0xFF000000) | ((*(addr + i * 2 + grid_w * 2) >> 8) & 0x00FF0000);
			c1  = ((*(addr + i * 2 + grid_w * 2 * 2) << 8) & 0xFF000000) | ((*(addr + i * 2 + grid_w * 2 * 2) >> 8) & 0x00FF0000);
			d1  = ((*(addr + i * 2 + grid_w * 2 * 3) << 8) & 0xFF000000) | ((*(addr + i * 2 + grid_w * 2 * 3) >> 8) & 0x00FF0000);
		} else if (3 == word_endian) { // BADC = 1 word
			a0  = ((*(addr + i * 2) << 8) & 0xFF000000) | ((*(addr + i * 2) >> 8) & 0x00FF0000);
			b0  = ((*(addr + i * 2 + grid_w * 2) << 8) & 0xFF000000) | ((*(addr + i * 2 + grid_w * 2) >> 8) & 0x00FF0000);
			c0  = ((*(addr + i * 2 + grid_w * 2 * 2) << 8) & 0xFF000000) | ((*(addr + i * 2 + grid_w * 2 * 2) >> 8) & 0x00FF0000);
			d0  = ((*(addr + i * 2 + grid_w * 2 * 3) << 8) & 0xFF000000) | ((*(addr + i * 2 + grid_w * 2 * 3) >> 8) & 0x00FF0000);
			a1  = ((*(addr + i * 2) << 8) & 0x0000FF00) | ((*(addr + i * 2) >> 8) & 0x000000FF);
			b1  = ((*(addr + i * 2 + grid_w * 2) << 8) & 0x0000FF00) | ((*(addr + i * 2 + grid_w * 2) >> 8) & 0x000000FF);
			c1  = ((*(addr + i * 2 + grid_w * 2 * 2) << 8) & 0x0000FF00) | ((*(addr + i * 2 + grid_w * 2 * 2) >> 8) & 0x000000FF);
			d1  = ((*(addr + i * 2 + grid_w * 2 * 3) << 8) & 0x0000FF00) | ((*(addr + i * 2 + grid_w * 2 * 3) >> 8) & 0x000000FF);
		}
		q_val[0][i] = ISP_Cubic1D(a0, b0, c0, d0, u, grid_num);
		q_val[1][i] = ISP_Cubic1D(a1, b1, c1, d1, u, grid_num);
	}
	#endif

	return 0;
}
/*end cal Q value*/

isp_s32 isp_u_2d_lsc_block(isp_handle handle, void *block_info)
{
	isp_s32 ret = 0, i = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle || !block_info) {
		ISP_LOGE("handle is null error: 0x%lx 0x%lx",
				(isp_uint)handle, (isp_uint)block_info);
		return -1;
	}

	file = (struct isp_file *)(handle);

	struct isp_dev_2d_lsc_info *lens_info = (struct isp_dev_2d_lsc_info *)(block_info);
	unsigned long buf_addr;

#if __WORDSIZE == 64
	buf_addr = ((unsigned long)lens_info->buf_addr[1] << 32) | lens_info->buf_addr[0];
#else
	buf_addr = lens_info->buf_addr[0];
#endif

	if (0 == file->reserved || 0 == buf_addr ||  ISP_LSC_BUF_SIZE < lens_info->buf_len) {
		ISP_LOGE("lsc memory error: %x %x %x", file->reserved, buf_addr, lens_info->buf_len);
		return ret;
	} else {
		memcpy((void*)file->reserved, (void*)buf_addr, lens_info->buf_len);
	}

	ret = ISP_GenerateQValues(1, lens_info->q_value, buf_addr,
			(uint16_t)lens_info->grid_pitch, (lens_info->grid_width & 0xFF), (lens_info->relative_y & 0xFF)>>1);
	if (0 != ret) {
		ISP_LOGE("ISP_GenerateQValues is error");
		return ret;
	}

	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_2D_LSC;
	param.property = ISP_PRO_2D_LSC_BLOCK;
	param.property_param = lens_info;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_2d_lsc_bypass(isp_handle handle, isp_u32 bypass)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_2D_LSC;
	param.property = ISP_PRO_2D_LSC_BYPASS;
	param.property_param = &bypass;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_2d_lsc_param_update(isp_handle handle)
{
	isp_s32 ret = 0;
	isp_u32 upsdate_param = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_2D_LSC;
	param.property = ISP_PRO_2D_LSC_PARAM_UPDATE;
	param.property_param = &upsdate_param;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_2d_lsc_pos(isp_handle handle, isp_u32 x, isp_u32 y)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_img_offset offset;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_2D_LSC;
	param.property = ISP_PRO_2D_LSC_POS;
	offset.x = x;
	offset.y = y;
	param.property_param = &offset;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_2d_lsc_grid_size(isp_handle handle, isp_u32 w, isp_u32 h)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_img_size size;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_2D_LSC;
	param.property = ISP_PRO_2D_LSC_GRID_SIZE;
	size.width = w;
	size.height = h;
	param.property_param = &size;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_2d_lsc_slice_size(isp_handle handle, isp_u32 w, isp_u32 h)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_img_size size;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_2D_LSC;
	param.property = ISP_PRO_2D_LSC_SLICE_SIZE;
	size.width = w;
	size.height = h;
	param.property_param = &size;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_2d_lsc_transaddr(isp_handle handle, isp_u32 phys_addr)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_lsc_addr lsc_addr;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_2D_LSC;
	param.property = ISP_PRO_2D_LSC_TRANSADDR;
	lsc_addr.phys_addr = phys_addr;
	param.property_param = &lsc_addr;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}
