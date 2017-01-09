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
#include "isp_calibration.h"
#include "sensor_raw.h"

//#define CLIP(x, bottom, top) ((x)<bottom ? bottom : (x) > top ? top : (x))

int32_t CLIP(int32_t x, int32_t bottom, int32_t top)
{
	int32_t val = 0;

	if (x < bottom) {
		val = bottom;
	} else if (x > top) {
		val = top;
	} else {
		val = x;
	}

	return val;
}

static uintptr_t ISP_Alloc(uint32_t mem_len)
{
	uintptr_t mem_addr = 0;
	mem_addr = (uintptr_t)malloc(mem_len);
	if (!mem_addr) {
		ISP_CALI_LOG("ISP_Alloc: malloc failed \n");
		mem_addr =  0;
	}
	memset((void*)mem_addr, 0x00, mem_len);

	return mem_addr;
}

static int32_t  ISP_Free(void **mem_ptr)
{
	uint32_t** pointer = (uint32_t**)mem_ptr;
	if (0 == mem_ptr) {
		ISP_CALI_LOG("ISP_Alloc: param pointer is null \n");
		return ISP_CALI_RTN_PT_NULL;
	}

	if (0 == (*mem_ptr)) {
		return ISP_CALI_RTN_SUCCESS;
	}

	free(*mem_ptr);
	*mem_ptr = 0;

	return ISP_CALI_RTN_SUCCESS;
}



uint32_t ISP_Cali_Get_RawRGB_Stat(struct isp_addr_t *img_addr,
					struct isp_rect_t *rect,
					struct isp_size_t *img_size,
					uint32_t byer_pttn,
					struct isp_bayer_ptn_stat_t *stat_param)
{
	uint16_t *src= 0;
	uint32_t i = 0, j = 0;
	uint16_t *src_bits = 0;
	uint32_t stride = 0, inx = 0;
	uint32_t st_x = 0, ed_x = 0;
	uint32_t st_y = 0, ed_y = 0;
	uint64_t sum[4] = {0,0,0,0};
	uint32_t cnts[4] = {0,0,0,0};
	uint32_t img_w = 0, img_h = 0;

	if ((!img_addr)
	      ||(!rect)
	      ||(!img_size)
	      ||(!stat_param)) {

	      ISP_CALI_LOG("ISP_Cali_Get_RawRGB_Stat: params are NULL,addr=0x%p, rect=0x%p, size=0x%p, stat=0x%p\n",
		img_addr, rect, img_size, stat_param);

	      return ISP_CALI_RTN_PT_NULL;
	}

	if (0 == img_addr->y_addr) {
		ISP_CALI_LOG("ISP_Cali_Get_RawRGB_Stat: params are NULL,addr_y=0x%x\n",
		img_addr->y_addr);

		return ISP_CALI_RTN_PT_NULL;
	}

	img_h = rect->height;
	img_w = rect->width;
#if 0
	if ((0 != (img_h & 0x01))
	||(0 != (img_w & 0x01))) {
		ISP_CALI_LOG("ISP_Cali_Get_RawRGB_Stat: w = %d, h = %d\n"
			img_w, img_h);
		return ISP_CALI_RTN_PARAM_INVALID;
	}

#endif

	stride = rect->width;
	st_x = rect->x;
	st_y = rect->y;
	ed_x = rect->x + rect->width-1;
	ed_y = rect->y + rect->height-1;
	src = (uint16_t*)img_addr->y_addr + st_y * stride + st_x;
	for(i = st_y; i<= ed_y; ++i) {
		src_bits = src ;
		for(j = st_x; j <= ed_x; ++j) {
			inx = ((i&0x01)<<1)+(j&0x01);
			sum[inx] += *src_bits;
			cnts[inx]++;
			src_bits++;
		}
		src += stride;
	}

	sum[0] = sum[0] / cnts[0];
	sum[1] = sum[1] / cnts[1];
	sum[2] = sum[2] / cnts[2];
	sum[3] = sum[3] / cnts[3];

	switch(byer_pttn)
	{
		case ISP_BAYER_PTN_B:
			stat_param->b_stat = sum[0];
			stat_param->gb_stat = sum[1];
			stat_param->gr_stat = sum[2];
			stat_param->r_stat = sum[3];
		break;

		case ISP_BAYER_PTN_GB:
			stat_param->gb_stat = sum[0];
			stat_param->b_stat = sum[1];
			stat_param->r_stat = sum[2];
			stat_param->gr_stat= sum[3];
		break;

		case ISP_BAYER_PTN_GR:
			stat_param->gr_stat = sum[0];
			stat_param->r_stat = sum[1];
			stat_param->b_stat = sum[2];
			stat_param->gb_stat = sum[3];
		break;

		case ISP_BAYER_PTN_R:
			stat_param->r_stat = sum[0];
			stat_param->gr_stat = sum[1];
			stat_param->gb_stat = sum[2];
			stat_param->b_stat = sum[3];
		break;
	}

	return ISP_CALI_RTN_SUCCESS;
}

#if 0
int32_t ISP_Cali_Get_LNCTable_Size(struct isp_size_t *img_size, struct isp_size_t *grid_size, struct isp_size_t *lnc_tab_size)
{
	int32_t rtn = ISP_CALI_RTN_SUCCESS;

	uint32_t real_w = 0;
	uint32_t real_h = 0;
	uint32_t tmp1 = 0;
	uint32_t tmp2 = 0;

	if ((!img_size)
	||(!grid_size)
	||(!lnc_tab_size)) {
		ISP_CALI_LOG("ISP_Cali_Get_LNCTable_Size: img addr=0x%x, grid size addr=0x%x, lnc tab size addr=0x%x\n"
			(uint32_t)img_size, (uint32_t)grid_size, (uint32_t)lnc_tab_size);

		rtn = ISP_CALI_RTN_PT_NULL
		return rtn;
	}

	tmp1 = img_size->width/grid_size->width;
	tmp2 = img_size->width - (grid_size->width * tmp1);
	if (tmp2) {
		tmp1 +=1;
	}
	lnc_tab_size->width = tmp1;

	tmp1 = img_size->height/grid_size->height;
	tmp2 = img_size->height - (grid_size->height * tmp1);
	if (tmp2) {
		tmp1 +=1;
	}
	lnc_tab_size->height = tmp1;


	return rtn;

}
#endif

int32_t _isp_Cali_LNCResAlloc(void *in_ptr, void *cali_ptr, void* gain_ptr)
{
	struct isp_lnc_cali_in *in_param_ptr = (struct isp_lnc_cali_in*)in_ptr;
	struct isp_lnc_cali_info *cali_param_ptr = (struct isp_lnc_cali_info*)cali_ptr;
	struct isp_lnc_cali_gain *gain_param_ptr = (struct isp_lnc_cali_gain*)gain_ptr;

	uint16_t lnc_grid = 0;
	uint16_t width = 0;
	uint16_t height = 0;
	uint16_t map_width = 0;
	uint16_t map_height = 0;
	uint16_t grid_x = 0;
	uint16_t grid_y = 0;
	uint32_t length = 0;
	if ((0 == in_ptr) || (0 == cali_ptr) || (0 == gain_ptr)) {
		ISP_CALI_LOG("ISP_Cali_LNCResAlloc: input param is null, in =0x%p, cali=0x%p, gain=0x%p\n",
			in_ptr, cali_ptr, gain_ptr);
		return ISP_CALI_RTN_PT_NULL;
	}

	lnc_grid = in_param_ptr->grid;
	width = in_param_ptr->width;
	height = in_param_ptr->height;

	grid_x = (((width>>1)-1) % lnc_grid) ? (((width>>1)-1)/lnc_grid + 2) : (((width>>1)-1)/lnc_grid + 1);
	grid_y = (((height>>1)-1) % lnc_grid) ? (((height>>1)-1)/lnc_grid + 2) : (((height>>1)-1)/lnc_grid + 1);

	map_width = grid_x * (lnc_grid << 0x01);
	map_height = grid_y * (lnc_grid << 0x01);

	cali_param_ptr->grid_x = grid_x;
	cali_param_ptr->grid_y = grid_y;
	cali_param_ptr->num = grid_x * grid_y;
	cali_param_ptr->map_width = map_width;
	cali_param_ptr->map_height = map_height;

	length = (map_width * map_height) << 0x01;
	cali_param_ptr->tmp_buf0 = (uint16_t*)ISP_Alloc(length);
	cali_param_ptr->tmp_buf1= (uint16_t*)ISP_Alloc(length);

	length = cali_param_ptr->num<<2;
	cali_param_ptr->chn_00.stat_ptr = (uint32_t*)ISP_Alloc(length);
	cali_param_ptr->chn_01.stat_ptr = (uint32_t*)ISP_Alloc(length);
	cali_param_ptr->chn_10.stat_ptr = (uint32_t*)ISP_Alloc(length);
	cali_param_ptr->chn_11.stat_ptr = (uint32_t*)ISP_Alloc(length);

	length = cali_param_ptr->num<<1;
	gain_param_ptr->chn_00 = (uint16_t*)ISP_Alloc(length);
	gain_param_ptr->chn_01 = (uint16_t*)ISP_Alloc(length);
	gain_param_ptr->chn_10 = (uint16_t*)ISP_Alloc(length);
	gain_param_ptr->chn_11 = (uint16_t*)ISP_Alloc(length);

	return ISP_CALI_RTN_SUCCESS;

}

void _isp_Cali_LNCResFree(struct isp_lnc_cali_info *param_ptr, struct isp_lnc_cali_gain *gain_ptr)
{
	if (0 == param_ptr) {
		ISP_CALI_LOG("ISP_Cali_LNCResFree: free failed \n");
		return ;
	}

	ISP_Free((void**)&param_ptr->tmp_buf0);
	ISP_Free((void**)&param_ptr->tmp_buf1);

	ISP_Free((void**)&param_ptr->chn_00.stat_ptr);
	ISP_Free((void**)&param_ptr->chn_01.stat_ptr);
	ISP_Free((void**)&param_ptr->chn_10.stat_ptr);
	ISP_Free((void**)&param_ptr->chn_11.stat_ptr);

	ISP_Free((void**)&gain_ptr->chn_00);
	ISP_Free((void**)&gain_ptr->chn_01);
	ISP_Free((void**)&gain_ptr->chn_10);
	ISP_Free((void**)&gain_ptr->chn_11);
	return;

}


int32_t _isp_Cali_LNC_Calc(void *in_ptr, void *rtn_ptr)
{
	int32_t rtn = ISP_CALI_RTN_SUCCESS;
	struct isp_lnc_cali_in *in_param_ptr = (struct isp_lnc_cali_in*)in_ptr;
	struct isp_lnc_cali_info *rtn_param_ptr = (struct isp_lnc_cali_info*)rtn_ptr;

	uint32_t x = 0;
	uint32_t y = 0;
	uint16_t stat_x = 0x00;
	uint16_t stat_y = 0x00;
	uint16_t *src_img_ptr = 0;
	uint16_t *dst_img_ptr = 0;
	uint8_t lnc_grid = 0x00;
	uint16_t grid_x = 0x00;
	uint16_t grid_y = 0x00;
	uint16_t add_right = 0x00;
	uint16_t add_bottom = 0x00;
	uint16_t map_width = 0x00;
	uint16_t map_height = 0x00;
	uint16_t *map_img_ptr = 0x00;
	uint16_t *temp_map_img_ptr = 0x00;
	uint16_t width = in_param_ptr->width;
	uint16_t height = in_param_ptr->height;
	uint16_t grid_num = rtn_param_ptr->num;

	lnc_grid = in_param_ptr->grid;//grid width

	grid_x = rtn_param_ptr->grid_x;
	grid_y = rtn_param_ptr->grid_y;

	map_width = rtn_param_ptr->map_width;
	map_height = rtn_param_ptr->map_height;

	add_right = map_width - (lnc_grid + width);
	add_bottom = map_height - (lnc_grid + height);

	temp_map_img_ptr = (uint16_t*)rtn_param_ptr->tmp_buf0;
	map_img_ptr = (uint16_t*)rtn_param_ptr->tmp_buf1;

	/*add board */
	/*add top bottom*/
	src_img_ptr = (uint16_t*)in_param_ptr->img_ptr;
	dst_img_ptr = (uint16_t*)temp_map_img_ptr;

	memcpy((void*)dst_img_ptr, (void*)src_img_ptr, width * lnc_grid * 2);

	dst_img_ptr += width * lnc_grid;
	memcpy((void*)dst_img_ptr, (void*)src_img_ptr, width * height * 2);

	src_img_ptr += width * (height - add_bottom);
	dst_img_ptr += width * height;
	memcpy((void*)dst_img_ptr, (void*)src_img_ptr, width * add_bottom * 2);

	/* add left right*/
	src_img_ptr = temp_map_img_ptr;
	dst_img_ptr = map_img_ptr;

	for (y = 0x00; y < map_height; y++) {
		memcpy((void*)dst_img_ptr, (void*)src_img_ptr, lnc_grid * 2);

		dst_img_ptr += lnc_grid;
		memcpy((void*)dst_img_ptr, (void*)src_img_ptr, width * 2);

		src_img_ptr += (width -add_right);
		dst_img_ptr += width;
		memcpy((void*)dst_img_ptr, (void*)src_img_ptr, add_right * 2);

		src_img_ptr += add_right;
		dst_img_ptr += add_right;
	}

	/*start stat*/
	src_img_ptr = map_img_ptr;
	lnc_grid <<= 0x01;
	for (y = 0x00; y < map_height; y++) {
		stat_y = y / lnc_grid;
		for (x = 0x00; x < map_width; x++) {
			stat_x = x / lnc_grid;

			if ((0x00 == (0x01 & y)) && (0x00 == (0x01 & x))) {
				rtn_param_ptr->chn_00.stat_ptr[grid_x * stat_y + stat_x] += *src_img_ptr++;
			}
			else if ((0x00 == (0x01 & y)) && (0x01 == (0x01 & x))) {
				rtn_param_ptr->chn_01.stat_ptr[grid_x * stat_y + stat_x] += *src_img_ptr++;
			}
			else if ((0x01 == (0x01 & y)) && (0x00 == (0x01 & x))) {
				rtn_param_ptr->chn_10.stat_ptr[grid_x * stat_y + stat_x] += *src_img_ptr++;
			}
			else if ((0x01 == (0x01 & y)) && (0x01 == (0x01 & x))) {
				rtn_param_ptr->chn_11.stat_ptr[grid_x * stat_y + stat_x] += *src_img_ptr++;
			}
		}
	}

	rtn_param_ptr->chn_00.max_stat = rtn_param_ptr->chn_00.stat_ptr[0];
	rtn_param_ptr->chn_01.max_stat = rtn_param_ptr->chn_01.stat_ptr[0];
	rtn_param_ptr->chn_10.max_stat = rtn_param_ptr->chn_10.stat_ptr[0];
	rtn_param_ptr->chn_11.max_stat = rtn_param_ptr->chn_11.stat_ptr[0];

	for (x = 0x00; x < grid_num; x++) {
		if (rtn_param_ptr->chn_00.max_stat < rtn_param_ptr->chn_00.stat_ptr[x]) {
			rtn_param_ptr->chn_00.max_stat = rtn_param_ptr->chn_00.stat_ptr[x];
		}
		if (rtn_param_ptr->chn_01.max_stat < rtn_param_ptr->chn_01.stat_ptr[x]) {
			rtn_param_ptr->chn_01.max_stat = rtn_param_ptr->chn_01.stat_ptr[x];
		}
		if (rtn_param_ptr->chn_10.max_stat < rtn_param_ptr->chn_10.stat_ptr[x]) {
			rtn_param_ptr->chn_10.max_stat = rtn_param_ptr->chn_10.stat_ptr[x];
		}
		if (rtn_param_ptr->chn_11.max_stat < rtn_param_ptr->chn_11.stat_ptr[x]) {
			rtn_param_ptr->chn_11.max_stat = rtn_param_ptr->chn_11.stat_ptr[x];
		}
	}

	return rtn;
}

int32_t _isp_Cali_LNC_GainCalc(uint32_t base_gain,
				uint32_t shf,
				uint32_t bayer_pattern,
				uint32_t r_prc,
				uint32_t g_prc,
				uint32_t b_prc,
				void* in_ptr,
				void* out_ptr)
{
	int32_t rtn = ISP_CALI_RTN_SUCCESS;
	struct isp_lnc_cali_info *in_param_ptr = (struct isp_lnc_cali_info*)in_ptr;
	struct isp_lnc_cali_gain *rtn_param_ptr = (struct isp_lnc_cali_gain*)out_ptr;
	int32_t std_gain = base_gain;
	uint16_t shf_bit = shf;
	int32_t map_gain = 0x00;
	uint32_t x = 0x00;
	int32_t tmp_val = 0;

	uint32_t prc_00 = 0x00;
	uint32_t prc_01 = 0x00;
	uint32_t prc_10 = 0x00;
	uint32_t prc_11 = 0x00;

	switch (bayer_pattern) {
		case ISP_BAYER_PTN_GR:
		{
			prc_00 = g_prc;
			prc_01 = r_prc;
			prc_10 = b_prc;
			prc_11 = g_prc;
		}
		break;

		case ISP_BAYER_PTN_R:
		{
			prc_00 = r_prc;
			prc_01 = g_prc;
			prc_10 = g_prc;
			prc_11 = b_prc;
		}
		break;

		case ISP_BAYER_PTN_B:
		{
			prc_00 = b_prc;
			prc_01 = g_prc;
			prc_10 = g_prc;
			prc_11 = r_prc;
		}
		break;

		case ISP_BAYER_PTN_GB:
		{
			prc_00 = g_prc;
			prc_01 = b_prc;
			prc_10 = r_prc;
			prc_11 = g_prc;
		}
		break;

		default:
		break;
	}

	for (x = 0x00; x < in_param_ptr->num; x++) {
		map_gain = (in_param_ptr->chn_00.max_stat << shf_bit) / (in_param_ptr->chn_00.stat_ptr[x]+1);
		map_gain -= std_gain;
		map_gain *= prc_00;
		map_gain >>= shf_bit;
		tmp_val = std_gain + map_gain;
		tmp_val = CLIP(tmp_val, 0, 0xfff);
		rtn_param_ptr->chn_00[x] = (uint16_t)tmp_val;

		map_gain = (in_param_ptr->chn_01.max_stat << shf_bit) / (in_param_ptr->chn_01.stat_ptr[x]+1);
		map_gain -= std_gain;
		map_gain *= prc_01;
		map_gain >>= shf_bit;
		tmp_val = std_gain + map_gain;
		tmp_val = CLIP(tmp_val, 0, 0xfff);
		rtn_param_ptr->chn_01[x] = (uint16_t)tmp_val;

		map_gain = (in_param_ptr->chn_10.max_stat << shf_bit) / (in_param_ptr->chn_10.stat_ptr[x]+1);
		map_gain -= std_gain;
		map_gain *= prc_10;
		map_gain >>= shf_bit;
		tmp_val = std_gain + map_gain;
		tmp_val = CLIP(tmp_val, 0, 0xfff);
		rtn_param_ptr->chn_10[x] = (uint16_t)tmp_val;

		map_gain = (in_param_ptr->chn_11.max_stat << shf_bit) / (in_param_ptr->chn_11.stat_ptr[x]+1);
		map_gain -= std_gain;
		map_gain *= prc_11;
		map_gain >>= shf_bit;
		tmp_val = std_gain + map_gain;
		tmp_val = CLIP(tmp_val, 0, 0xfff);
		rtn_param_ptr->chn_11[x] = (uint16_t)tmp_val;
	}

	return rtn;

}

int32_t _isp_Cali_LNCTabMerge(struct isp_lnc_cali_gain *gain_ptr, uint32_t grid_num, uint16_t *lnc_tab)
{
	uint32_t i = 0;
	uint32_t inx = 0;

	for(i = 0; i < grid_num; i++) {
		lnc_tab[inx++] = gain_ptr->chn_01[i];
		lnc_tab[inx++] = gain_ptr->chn_00[i];
		lnc_tab[inx++] = gain_ptr->chn_11[i];
		lnc_tab[inx++] = gain_ptr->chn_10[i];
	}

	return ISP_CALI_RTN_SUCCESS;
}

void ISP_Cali_GetLNCTabSize(struct isp_size_t img_size, uint32_t grid, uint32_t *tab_size)
{
	uint8_t lnc_grid = grid;
	uint16_t width = img_size.width;
	uint16_t height = img_size.height;
	uint16_t grid_x = 0x00;
	uint16_t grid_y = 0x00;

	grid_x = (((width>>1)-1) % lnc_grid) ? (((width>>1)-1)/lnc_grid + 2) : (((width>>1)-1)/lnc_grid + 1);
	grid_y = (((height>>1)-1) % lnc_grid) ? (((height>>1)-1)/lnc_grid + 2) : (((height>>1)-1)/lnc_grid + 1);

	*tab_size = (grid_x * grid_y)*8;

}

#if 1
int32_t ISP_Cali_LNCTaleCalc(struct isp_addr_t img_addr, uint32_t bayer_pttn, struct isp_size_t img_size, uint32_t grid, uint16_t *lnc_tab)
{
	int32_t rtn = ISP_CALI_RTN_SUCCESS;
	struct isp_lnc_cali_in cali_in_param;
	struct isp_lnc_cali_info cali_info_param;
	struct isp_lnc_cali_gain cali_gain_param;

	if ((0 == lnc_tab) || (0 == img_addr.y_addr)) {

		ISP_CALI_LOG("ISP_Cali_LNCTaleCalc: raw img addr=0x%lx, lnc tab addr=0x%p\n", img_addr.y_addr, lnc_tab);
		return ISP_CALI_RTN_PT_NULL;
	}

	cali_in_param.grid = grid;
	cali_in_param.height = img_size.height;
	cali_in_param.width = img_size.width;
	cali_in_param.img_ptr = (uint16_t*)img_addr.y_addr;

	_isp_Cali_LNCResAlloc((void*)&cali_in_param, (void*)&cali_info_param, (void*)&cali_gain_param);
	_isp_Cali_LNC_Calc((void*)&cali_in_param, (void*)&cali_info_param);
	_isp_Cali_LNC_GainCalc(1024, 10, bayer_pttn, 1024, 1024, 1024, (void*)&cali_info_param, (void*)&cali_gain_param);
	_isp_Cali_LNCTabMerge(&cali_gain_param, cali_info_param.num, lnc_tab);
	_isp_Cali_LNCResFree(&cali_info_param, &cali_gain_param);

	return rtn;

}
#else
int32_t ISP_Cali_LNC_Calc(void *in_ptr, void *rtn_ptr)
{
	int32_t rtn = ISP_CALI_RTN_SUCCESS;
	struct isp_lnc_cali_in *in_param_ptr = (struct isp_lnc_cali_in*)in_ptr;
	struct isp_lnc_cali_info *rtn_param_ptr = (struct isp_lnc_cali_info*)rtn_ptr;

	uint8_t lnc_grid = in_param_ptr->grid;
	uint16_t width = in_param_ptr->width;
	uint16_t height = in_param_ptr->height;

	uint16_t map_width = 0x00;
	uint16_t map_height = 0x00;
	uint16_t add_right = 0x00;
	uint16_t add_bottom = 0x00;
	uint16_t grid_x = 0x00;
	uint16_t grid_y = 0x00;
	uint16_t grid_num = 0x00;
	uint16_t *temp_map_img_ptr = 0x00;
	uint16_t *map_img_ptr = 0x00;
	uint32_t map_img_len = 0x00;
	uint16_t stat_x = 0x00;
	uint16_t stat_y = 0x00;
	uint32_t x = 0;
	uint32_t y = 0;

	uint16_t *src_img_ptr = 0;
	uint16_t *dst_img_ptr = 0;

	grid_x = (((width>>1)-1) % lnc_grid) ? (((width>>1)-1)/lnc_grid + 2) : (((width>>1)-1)/lnc_grid + 1);
	grid_y = (((height>>1)-1) % lnc_grid) ? (((height>>1)-1)/lnc_grid + 2) : (((height>>1)-1)/lnc_grid + 1);

	rtn_param_ptr->grid_x = grid_x;
	rtn_param_ptr->grid_y = grid_y;

	grid_num = grid_x * grid_y;
	map_width = grid_x * (lnc_grid << 0x01);
	map_height = grid_y * (lnc_grid << 0x01);
	add_right = map_width - (lnc_grid + width);
	add_bottom = map_height - (lnc_grid + height);
	map_img_len = (map_width * map_height) << 0x01;

	temp_map_img_ptr = (uint16_t*)ISP_Alloc(map_img_len);
	if (0x00 == temp_map_img_ptr) {
		ISP_CALI_LOG("ISP_Cali_LNC_Calc:\n");
		rtn = ISP_CALI_RTN_PT_NULL;
		return rtn;
	}

	map_img_ptr = (uint16_t*)ISP_Alloc(map_img_len);
	if (0x00 == map_img_ptr) {
		ISP_CALI_LOG("ISP_Cali_LNC_Calc:\n");
		rtn = ISP_CALI_RTN_PT_NULL;
		return rtn;
	}

	/*add board */
	/*add top bottom*/
	src_img_ptr = in_param_ptr->img_ptr;
	dst_img_ptr = temp_map_img_ptr;

	memcpy((void*)dst_img_ptr, (void*)src_img_ptr, width * lnc_grid * 2);

	dst_img_ptr += width * lnc_grid;
	memcpy((void*)dst_img_ptr, (void*)src_img_ptr, width * height * 2);

	src_img_ptr += width * (height - add_bottom);
	dst_img_ptr += width * height;
	memcpy((void*)dst_img_ptr, (void*)src_img_ptr, width * add_bottom * 2);

	/* add left right*/
	src_img_ptr = temp_map_img_ptr;
	dst_img_ptr = map_img_ptr;

	for (y = 0x00; y < map_height; y++) {
		memcpy((void*)dst_img_ptr, (void*)src_img_ptr, lnc_grid * 2);

		dst_img_ptr += lnc_grid;
		memcpy((void*)dst_img_ptr, (void*)src_img_ptr, width * 2);

		src_img_ptr += (width -add_right);
		dst_img_ptr += width;
		memcpy((void*)dst_img_ptr, (void*)src_img_ptr, add_right * 2);

		src_img_ptr += add_right;
		dst_img_ptr += add_right;
	}

	/*stat rgb*/
	rtn_param_ptr->num = grid_num;

	grid_num <<= 0x02;
	rtn_param_ptr->chn_00.stat_ptr = (uint16_t*)ISP_Alloc(grid_num);
	if (0x00 == rtn_param_ptr->chn_00.stat_ptr) {
		rtn = ISP_CALI_RTN_PT_NULL;
		return rtn;
	}
	memset((void*)rtn_param_ptr->chn_00.stat_ptr, 0x00, grid_num);

	rtn_param_ptr->chn_01.stat_ptr = (uint16_t*)ISP_Alloc(grid_num);
	if (0x00 == rtn_param_ptr->chn_01.stat_ptr) {
		rtn = ISP_CALI_RTN_PT_NULL;
		return rtn;
	}
	memset((void*)rtn_param_ptr->chn_01.stat_ptr, 0x00, grid_num);

	rtn_param_ptr->chn_10.stat_ptr = (uint16_t*)ISP_Alloc(grid_num);
	if (0x00 == rtn_param_ptr->chn_10.stat_ptr) {
		rtn = ISP_CALI_RTN_PT_NULL;
		return rtn;
	}
	memset((void*)rtn_param_ptr->chn_10.stat_ptr, 0x00, grid_num);

	rtn_param_ptr->chn_11.stat_ptr = (uint16_t*)ISP_Alloc(grid_num);
	if (0x00 == rtn_param_ptr->chn_11.stat_ptr) {
		rtn = ISP_CALI_RTN_PT_NULL;
		return rtn;
	}
	memset((void*)rtn_param_ptr->chn_11.stat_ptr, 0x00, grid_num);

	/*start stat*/
	src_img_ptr = map_img_ptr;
	lnc_grid <<= 0x01;
	for (y = 0x00; y < map_height; y++) {
		stat_y = y / lnc_grid;
		for (x = 0x00; x < map_width; x++) {
			stat_x = x / lnc_grid;

			if ((0x00 == (0x01 & y)) && (0x00 == (0x01 & x))) {
				rtn_param_ptr->chn_00.stat_ptr[grid_x * stat_y + stat_x] += *src_img_ptr++;
			}
			else if ((0x00 == (0x01 & y)) && (0x01 == (0x01 & x))) {
				rtn_param_ptr->chn_01.stat_ptr[grid_x * stat_y + stat_x] += *src_img_ptr++;
			}
			else if ((0x01 == (0x01 & y)) && (0x00 == (0x01 & x))) {
				rtn_param_ptr->chn_10.stat_ptr[grid_x * stat_y + stat_x] += *src_img_ptr++;
			}
			else if ((0x01 == (0x01 & y)) && (0x01 == (0x01 && x))) {
				rtn_param_ptr->chn_11.stat_ptr[grid_x * stat_y + stat_x] += *src_img_ptr++;
			}
		}
	}

	rtn_param_ptr->chn_00.max_stat = rtn_param_ptr->chn_00.stat_ptr[0];
	rtn_param_ptr->chn_01.max_stat = rtn_param_ptr->chn_01.stat_ptr[0];
	rtn_param_ptr->chn_10.max_stat = rtn_param_ptr->chn_10.stat_ptr[0];
	rtn_param_ptr->chn_11.max_stat = rtn_param_ptr->chn_11.stat_ptr[0];

	for (x = 0x00; x < rtn_param_ptr->num; x++) {
		if (rtn_param_ptr->chn_00.max_stat < rtn_param_ptr->chn_00.stat_ptr[x]) {
			rtn_param_ptr->chn_00.max_stat = rtn_param_ptr->chn_00.stat_ptr[x];
		}
		if (rtn_param_ptr->chn_01.max_stat < rtn_param_ptr->chn_01.stat_ptr[x]) {
			rtn_param_ptr->chn_01.max_stat = rtn_param_ptr->chn_01.stat_ptr[x];
		}
		if (rtn_param_ptr->chn_10.max_stat < rtn_param_ptr->chn_10.stat_ptr[x]) {
			rtn_param_ptr->chn_10.max_stat = rtn_param_ptr->chn_10.stat_ptr[x];
		}
		if (rtn_param_ptr->chn_11.max_stat < rtn_param_ptr->chn_11.stat_ptr[x]) {
			rtn_param_ptr->chn_11.max_stat = rtn_param_ptr->chn_11.stat_ptr[x];
		}
	}

	ISP_Free(&map_img_ptr);
	ISP_Free(&temp_map_img_ptr);

	return rtn;

}

int32_t ISP_Cali_LNC_GainCalc(uint32_t base_gain,
				uint32_t shf,
				uint32_t bayer_pattern,
				uint32_t r_prc,
				uint32_t g_prc,
				uint32_t b_prc,
				void* in_ptr,
				void* out_ptr)
{
	int32_t rtn = ISP_CALI_RTN_SUCCESS;
	struct isp_lnc_cali_info *in_param_ptr = (struct isp_lnc_cali_info*)in_ptr;
	struct isp_lnc_cali_gain *rtn_param_ptr = (struct isp_lnc_cali_gain*)out_ptr;
	uint16_t std_gain = base_gain;
	uint16_t shf_bit = shf;
	uint32_t map_gain = 0x00;
	uint32_t x = 0x00;

	uint32_t prc_00 = 0x00;
	uint32_t prc_01 = 0x00;
	uint32_t prc_10 = 0x00;
	uint32_t prc_11 = 0x00;

	switch (bayer_pattern) {
		case ISP_BAYER_PTN_GR:
		{
			prc_00 = g_prc;
			prc_01 = r_prc;
			prc_10 = b_prc;
			prc_11 = g_prc;
		}
		break;

		case ISP_BAYER_PTN_R:
		{
			prc_00 = r_prc;
			prc_01 = g_prc;
			prc_10 = g_prc;
			prc_11 = b_prc;
		}
		break;

		case ISP_BAYER_PTN_B:
		{
			prc_00 = b_prc;
			prc_01 = g_prc;
			prc_10 = g_prc;
			prc_11 = r_prc;
		}
		break;

		case ISP_BAYER_PTN_GB:
		{
			prc_00 = g_prc;
			prc_01 = b_prc;
			prc_10 = r_prc;
			prc_11 = g_prc;
		}
		break;

		default:
		break;
	}

	for (x = 0x00; x < in_param_ptr->num; x++) {
		map_gain = (in_param_ptr->chn_00.max_stat << shf_bit) / in_param_ptr->chn_00.stat_ptr[x];
		map_gain -= std_gain;
		map_gain *= prc_00;
		map_gain >>= shf_bit;
		rtn_param_ptr->chn_00[x] = std_gain + map_gain;

		map_gain = (in_param_ptr->chn_01.max_stat << shf_bit) / in_param_ptr->chn_01.stat_ptr[x];
		map_gain -= std_gain;
		map_gain *= prc_01;
		map_gain >>= shf_bit;
		rtn_param_ptr->chn_01[x] = std_gain + map_gain;

		map_gain = (in_param_ptr->chn_10.max_stat << shf_bit) / in_param_ptr->chn_10.stat_ptr[x];
		map_gain -= std_gain;
		map_gain *= prc_10;
		map_gain >>= shf_bit;
		rtn_param_ptr->chn_10[x] = std_gain + map_gain;

		map_gain = (in_param_ptr->chn_11.max_stat << shf_bit) / in_param_ptr->chn_11.stat_ptr[x];
		map_gain -= std_gain;
		map_gain *= prc_11;
		map_gain >>= shf_bit;
		rtn_param_ptr->chn_11[x] = std_gain + map_gain;
	}

	return rtn;

}


int32_t ISP_Cali_LNCTaleCalc(struct isp_addr_t img_addr, uint32_t bayer_pttn, struct isp_size_t img_size, uint32_t grid, uint16_t *lnc_tab)
{
	int32_t rtn = ISP_CALI_RTN_SUCCESS;
	struct isp_lnc_cali_in cali_in_param = {0x00};
	struct isp_lnc_cali_info cali_info_param = {0x00};
	struct isp_lnc_cali_gain cali_gain_param = {0x00};

	if ((0 == lnc_tab) || (0 == img_addr.y_addr)) {

		ISP_CALI_LOG("ISP_Cali_LNCTaleCalc: raw img addr=0x%x, lnc tab addr=0x%x\n", img_addr.y_addr, lnc_tab);
		return ISP_CALI_RTN_PT_NULL;
	}

	cali_in_param.grid = grid;
	cali_in_param.height = img_size.height;
	cali_in_param.width = img_size.width;
	cali_in_param.img_ptr = (uint16_t*)img_addr.y_addr;

	ISP_Cali_LNC_Calc(&cali_in_param, &cali_info_param);

	_isp_Cali_LNC_GainCalc(255, 8, bayer_pttn, 256, 256, 256, (void*)&cali_info_param, (void*)&cali_gain_param);
	_isp_Cali_LNCTabMerge(&cali_gain_param, grid, lnc_tab);

	ISP_Free(&cali_info_param.chn_00.stat_ptr);
	ISP_Free(&cali_info_param.chn_01.stat_ptr);
	ISP_Free(&cali_info_param.chn_10.stat_ptr);
	ISP_Free(&cali_info_param.chn_11.stat_ptr);

	return rtn;

}


#endif


int32_t ISP_Cali_Get_Advanced_LensShading(struct isp_addr_t *lnc_tg,
												struct isp_addr_t *lnc_mr,
												struct isp_addr_t *lnc_mg,
												struct isp_addr_t *lnc_final,
												uint32_t size)
{
	uint32_t i = 0;
	uint32_t tmp = 0;
	uint16_t tg_v = 0, mr_v = 0;
	uint16_t mg_v = 0, lnc_v = 0;
	uint16_t *tg_data = 0, *mr_data = 0;
	uint16_t *mg_data = 0, *lnc_data = 0;

	if ((!lnc_tg)||
	(!lnc_mr)
	||(!lnc_mg)
	||(!lnc_final))
	{
		//ISP_CALI_LOG("ISP_Cali_Get_Advanced_LensShading: lnc_tg =0x%x,lnc_mr=0x%x, lnc_mg=0x%x, lnc_final\n",
		//	(uint32_t)lnc_tg, (uint32_t)lnc_mr, (uint32_t)lnc_mg, (uint32_t)lnc_final);
		return ISP_CALI_RTN_PT_NULL;
	}

	if ((0 == lnc_tg->y_addr)
	||(0 == lnc_mr->y_addr)
	||(0 == lnc_mg->y_addr)
	||(0 == lnc_final->y_addr)) {

		ISP_CALI_LOG("ISP_Cali_Get_Advanced_LensShading: tg addr=0x%x, mr addr=0x%x,\
			mg addr=0x%x, final addr =0x%dx\n",\
			lnc_tg->y_addr, lnc_mr->y_addr, lnc_mg->y_addr, lnc_final->y_addr);

		return ISP_CALI_RTN_PT_NULL;
	}

	tg_data = (uint16_t*)lnc_tg->y_addr;
	mr_data = (uint16_t*)lnc_mr->y_addr;
	mg_data = (uint16_t*)lnc_mg->y_addr;
	lnc_data = (uint16_t*)lnc_final->y_addr;
	for ( i = 0; i < (size/2); i++)
	{
		tg_v = *tg_data++;
		mg_v = *mg_data++;
		mr_v = *mr_data++;

		tmp = ((uint32_t)tg_v) * mr_v;
		tmp = (tmp + (mg_v>>1)) / (mg_v+1);
		*lnc_data++ = (uint16_t)tmp;
	}

	return ISP_CALI_RTN_SUCCESS;
}

uint32_t ISP_Cali_LNCCorrection(struct isp_addr_t * src_data, struct isp_addr_t * dst_data, struct isp_size_t img_size, uint8_t grid, uint16_t *lnc_tab)
{
	// u/v in Q15
	uint32_t i, j;
	uint32_t ii, jj;
	uint32_t ii_step;
	uint32_t jj_step;
	uint16_t iWidth, iHeight;
	uint16_t cor_ratio;
	uint16_t u, v;
	uint8_t LNC_GRID;
	uint16_t GridX, GridY;//total grid number
	int32_t temp1,temp2,temp3,temp4;
	uint16_t *r0c0_ptr;
	uint16_t *r0c1_ptr;
	uint16_t *r1c0_ptr;
	uint16_t *r1c1_ptr;
	uint16_t *src_ptr;
	uint16_t *dst_ptr;
	uint16_t row,col;// the actual row and col of slice in_data_ptr
	uint16_t GridCurrentX,GridCurrentY;//use to index Lnc_cor_para_grid array

	uint16_t RelativeX,RelativeY;//the relative position of current pixel inside the grid
	uint16_t SliceX,SliceY;//use to index slice in_data_ptr

	uint16_t startCol;
	uint16_t startRow;
	uint16_t iSliceWidth;
	uint16_t iSliceHeight;

	if ((0 == src_data)
		||(0 == dst_data)) {
		ISP_CALI_LOG("The lens_correction input data pointer is NULL\n");
		return -1;
	}

	if ((0 == src_data->y_addr)
		||(0 == dst_data->y_addr)) {
		ISP_CALI_LOG("The len_correction output data pointer is NULL\n");
		return -1;
	}

	LNC_GRID = grid;

	src_ptr = (uint16_t*)src_data->y_addr;
	dst_ptr = (uint16_t*)dst_data->y_addr;

	iWidth = img_size.width;
	iHeight = img_size.height;

	startCol = 0;
	startRow = 0;
	iSliceWidth = iWidth;
	iSliceHeight = iHeight;

	r0c1_ptr = (uint16_t*)lnc_tab;
	r0c0_ptr = (uint16_t*)lnc_tab + 1;
	r1c1_ptr = (uint16_t*)lnc_tab + 2;
	r1c0_ptr = (uint16_t*)lnc_tab + 3 ;

	GridX = ((iWidth / 2 - 1) % LNC_GRID) ? ((iWidth / 2 -1) / LNC_GRID + 2) : ((iWidth / 2 -1) / LNC_GRID + 1);
	GridY = ((iHeight / 2 - 1) % LNC_GRID) ? ((iHeight / 2 -1) / LNC_GRID + 2) : ((iHeight / 2 -1) / LNC_GRID + 1);

	for ( row = startRow, SliceY = 0; row < startRow + iSliceHeight; row++, SliceY++) {
		for (col = startCol, SliceX = 0; col < startCol + iSliceWidth; col++, SliceX++) {

			GridCurrentX = (col / 2) / LNC_GRID;
			GridCurrentY = (row / 2) / LNC_GRID;
			RelativeX = (col / 2) % LNC_GRID;
			RelativeY = (row / 2) % LNC_GRID;

			if (((iWidth/2) % LNC_GRID ==1) && ((col / 2) == iWidth / 2 -1) ) {
				GridCurrentX--;
				RelativeX++;
			}

			if (((iHeight / 2) % LNC_GRID == 1) && ((row / 2) == iHeight / 2 -1)) {
				GridCurrentY--;
				RelativeY++;
			}

			ii = RelativeY;
			jj = RelativeX;
			i = GridCurrentY;
			j = GridCurrentX;
			ii_step = 32768 / LNC_GRID;
			jj_step = 32768 / LNC_GRID;

			u = (uint16_t)(ii * ii_step);//Q15
			v = (uint16_t)(jj * jj_step);
			temp1 = ((32768 - u) * (32768 - v))>>15;//Q15
			temp2 = (u * (32768 - v))>>15;
			temp3 = ((32768 - u) * v)>>15;
			temp4 = (u * v)>>15;

			if ((row & 1) && (col & 1)) {
				cor_ratio = (uint16_t) ((r1c1_ptr[(i * GridX + j)* 4] * temp1
					+r1c1_ptr[((i + 1) * GridX + j) * 4] * temp2
					+ r1c1_ptr[(i * GridX + j +1) * 4] * temp3
					+ r1c1_ptr[((i + 1) * GridX + j + 1)* 4] * temp4)>>15);
			}

			if (!(row & 1) && !(col & 1)) {
				cor_ratio = (uint16_t) ((r0c0_ptr[(i * GridX + j) * 4] * temp1
					+ r0c0_ptr[((i + 1) * GridX + j)* 4] * temp2
					+ r0c0_ptr[(i * GridX + j +1 ) * 4] * temp3
					+ r0c0_ptr[((i + 1) * GridX + j + 1)* 4] * temp4)>>15);
			}

			if ((row & 1) && !(col & 1)) {
				cor_ratio = (uint16_t) ((r1c0_ptr[(i * GridX + j)* 4] * temp1
					+ r1c0_ptr[((i + 1) * GridX + j)* 4] * temp2
					+ r1c0_ptr[(i * GridX + j +1)* 4] * temp3
					+ r1c0_ptr[((i + 1) * GridX + j + 1) * 4] * temp4)>>15);
			}

			if (!(row & 1) && (col & 1)) {
				cor_ratio = (uint16_t) ((r0c1_ptr[(i * GridX + j) * 4] * temp1
					+ r0c1_ptr[((i + 1) * GridX + j)* 4] * temp2
					+ r0c1_ptr[(i * GridX + j +1 )* 4] * temp3
					+ r0c1_ptr[((i + 1) * GridX +j + 1) * 4] * temp4)>>15);
			}

			temp1 = (uint16_t)((src_ptr[SliceY * iSliceWidth + SliceX] * (uint32_t)cor_ratio + 512)>>10);

			temp1 = CLIP(temp1, 0, 1023);

			dst_ptr[SliceY * iSliceWidth + SliceX] = (uint16_t)temp1;

		}
	}

	return 0;
}

int32_t ISP_Cali_BLCorrecton(struct isp_addr_t *in_img_addr,
								struct isp_addr_t *out_img_addr,
								struct isp_rect_t *rect,
								struct isp_size_t *img_size,
								uint32_t  bayer_pttn,
								struct isp_bayer_ptn_stat_t *stat_param
								)
{
	uint32_t i, j;
	int32_t tmp;
	uint16_t blc_gr;
	uint16_t blc_gb;
	uint16_t blc_r;
	uint16_t blc_b;
	uint16_t st_x, ed_x;
	uint16_t st_y, ed_y;
	uint16_t imagewidth;
	uint16_t *in_data_ptr = PNULL;
	uint16_t *out_data_ptr = PNULL;
	uint32_t bayer_mode;

	if ((PNULL == in_img_addr)
		||(PNULL == out_img_addr)
		||(PNULL == stat_param)
		||(PNULL == rect)
		||(PNULL == img_size)) {

		ISP_CALI_LOG("The ISP_Cali_Blc pointer is NULL, in_ptr: 0x%p, out_ptr: 0x%p, stat_ptr: 0x%p, rect:0x%p, img_size:0x%p\n",
			in_img_addr, out_img_addr, stat_param, rect, img_size);

		return -1;
	}

	in_data_ptr = (uint16_t*)in_img_addr->y_addr;
	out_data_ptr = (uint16_t*)out_img_addr->y_addr;
	if ((PNULL == in_data_ptr)
		||(PNULL == out_data_ptr)) {
		ISP_CALI_LOG("The ISP_Cali_Blc data address is NULL, in data addr: 0x%p, out data addr: 0x%p\n",
			in_data_ptr, out_data_ptr);

		return -1;
	}

	bayer_mode = bayer_pttn;
	imagewidth = img_size->width;

	blc_gr = stat_param->gr_stat;
	blc_gb = stat_param->gb_stat;
	blc_r = stat_param->r_stat;
	blc_b = stat_param->b_stat;

	st_x = rect->x;
	st_y = rect->y;
	ed_x = rect->x + rect->width -1;
	ed_y = rect->y + rect->height -1;

	for (i = st_y; i <= ed_y; i++) {
		for (j = st_x; j <= ed_x; j++) {
			if ((i &1)&&(j &1)) {
				if (0 == bayer_mode) {
					tmp = in_data_ptr[i * imagewidth + j] - blc_gb;
					tmp = CLIP(tmp, 0, 1023);
					*(out_data_ptr++) = (uint16_t)tmp;
				} else if (1 == bayer_mode) {
					tmp = in_data_ptr[i * imagewidth + j] - blc_b;
					tmp = CLIP(tmp, 0, 1023);
					*(out_data_ptr++) = (uint16_t)tmp;
				} else if (2 == bayer_mode) {
					tmp = in_data_ptr[i * imagewidth + j] - blc_r;
					tmp = CLIP(tmp, 0, 1023);
					*(out_data_ptr++) = (uint16_t)tmp;
				} else if (3 == bayer_mode) {
					tmp = in_data_ptr[i * imagewidth + j] - blc_gr;
					tmp = CLIP(tmp, 0, 1023);
					*(out_data_ptr++) = (uint16_t)tmp;
				}
			} else if ((i &1)&&(!(j &1))) {
				if (0 == bayer_mode) {
					tmp = in_data_ptr[i * imagewidth + j] - blc_b;
					tmp = CLIP(tmp, 0, 1023);
					*(out_data_ptr++) = (uint16_t)tmp;
				} else if (1 == bayer_mode) {
					tmp = in_data_ptr[i * imagewidth + j] - blc_gb;
					tmp = CLIP(tmp, 0, 1023);
					*(out_data_ptr++) = (uint16_t)tmp;
				} else if (2 == bayer_mode) {
					tmp = in_data_ptr[i * imagewidth + j] -blc_gr;
					tmp = CLIP(tmp, 0, 1023);
					*(out_data_ptr++) = (uint16_t)tmp;
				} else if (3 == bayer_mode) {
					tmp = in_data_ptr[i * imagewidth + j] - blc_r;
					tmp = CLIP(tmp, 0, 1023);
					*(out_data_ptr++) = (uint16_t)tmp;
				}
			}else if ((!(i &1))&&(j &1)) {
				if (0 == bayer_mode) {
					tmp = in_data_ptr[i * imagewidth + j] - blc_r;
					tmp = CLIP(tmp, 0, 1023);
					*(out_data_ptr++) = (uint16_t)tmp;
				} else if (1 == bayer_mode) {
					tmp = in_data_ptr[i * imagewidth + j] - blc_gr;
					tmp = CLIP(tmp, 0, 1023);
					*(out_data_ptr++) = (uint16_t)tmp;
				} else if (2 == bayer_mode) {
					tmp = in_data_ptr[i * imagewidth + j] - blc_gb;
					tmp = CLIP(tmp, 0, 1023);
					*(out_data_ptr++) = (uint16_t)tmp;
				} else if (3 == bayer_mode) {
					tmp = in_data_ptr[i * imagewidth + j] - blc_b;
					tmp = CLIP(tmp, 0, 1023);
					*(out_data_ptr++) = (uint16_t)tmp;
				}
			}else if ((!(i &1))&&(!(j &1))) {
				if (0 == bayer_mode) {
					tmp = in_data_ptr[i * imagewidth + j] - blc_gr;
					tmp = CLIP(tmp, 0, 1023);
					*(out_data_ptr++) = (uint16_t)tmp;
				} else if (1 == bayer_mode) {
					tmp = in_data_ptr[i * imagewidth + j] - blc_r;
					tmp = CLIP(tmp, 0, 1023);
					*(out_data_ptr++) = (uint16_t)tmp;
				} else if (2 == bayer_mode) {
					tmp = in_data_ptr[i * imagewidth + j] - blc_b;
					tmp = CLIP(tmp, 0, 1023);
					*(out_data_ptr++) = (uint16_t)tmp;
				} else if (3 == bayer_mode) {
					tmp = in_data_ptr[i * imagewidth + j] - blc_gb;
					tmp = CLIP(tmp, 0, 1023);
					*(out_data_ptr++) = (uint16_t)tmp;
				}
			}
		}
	}

	return 0;

}
