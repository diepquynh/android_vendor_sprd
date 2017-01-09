/*--------------------------------------------------------


----------------------------------------------------------*/

#include <stdio.h>
#include <sys/types.h>
#include <utils/Log.h>
#include "packet_convert.h"

#define LSC_GAIN_SIZE_PER_CH 560
#define CONVERT_LSC_SUCCESS 0
#define CONVERT_AWB_SUCCESS 0
#define CONVERT_LSC_ERROR 1
#define CONVERT_AWB_ERROR 1
#define LSC_CHN_NUM 4

#define RANDOM_LSC_BLOCK_VERSION 0x53501001

int32_t awb_package_convert(void *src_data, void *target_buf, uint32_t target_buf_size, uint32_t *real_size)
{
	struct awb_source_packet *awb_packet_src = NULL;
	struct awb_target_packet *awb_packet_dst = NULL;
	
	awb_packet_src = (struct awb_source_packet *)src_data;
	awb_packet_dst = (struct awb_target_packet *)target_buf;

	if(target_buf_size < sizeof(struct awb_target_packet)){
		return CONVERT_AWB_ERROR;
	}

	/*Param from source packet*/
	awb_packet_dst->version_id = 1;
	awb_packet_dst->mean_value_R = awb_packet_src->average_R;
	awb_packet_dst->mean_value_G = awb_packet_src->average_G;
	awb_packet_dst->mean_value_B = awb_packet_src->average_B;

	ALOGI("awb_package_convert: r=%d, g=%d, b=%d", awb_packet_src->average_R, awb_packet_src->average_G,
							awb_packet_src->average_B);

	*real_size = sizeof(struct awb_target_packet);

	return CONVERT_AWB_SUCCESS;
}

static void _change_pattern(uint8_t *dst, uint8_t *src, uint32_t chn_size, uint32_t pattern)
{
	uint32_t i = 0;
	uint8_t *src_chn[4] = {NULL};

	switch (pattern) {
	case 0:
		src_chn[0] = src;
		src_chn[1] = src + chn_size * 1;
		src_chn[2] = src + chn_size * 2;
		src_chn[3] = src + chn_size * 3;
		break;

	case 1:
		src_chn[1] = src;
		src_chn[0] = src + chn_size * 1;
		src_chn[3] = src + chn_size * 2;
		src_chn[2] = src + chn_size * 3;
		break;

	case 2:
		src_chn[2] = src;
		src_chn[3] = src + chn_size * 1;
		src_chn[0] = src + chn_size * 2;
		src_chn[1] = src + chn_size * 3;
		break;

	case 3:
		src_chn[3] = src;
		src_chn[2] = src + chn_size * 1;
		src_chn[1] = src + chn_size * 2;
		src_chn[0] = src + chn_size * 3;
		break;
	}

	for(i=0;i<LSC_CHN_NUM;i++){
		memcpy(dst, src_chn[i], chn_size);
		dst += chn_size;
	}

}

int32_t lsc_package_convert(void *src_data, void *target_buf, uint32_t target_buf_size, uint32_t *real_size)
{
#if 1
	int i = 0;
	uint8_t *src_lsc = NULL;
	struct lsc_source_packet *lsc_packet_src = {0};
	struct lsc_target_packet *lsc_packet_dst = {0};
	uint32_t chn_gain_size = 0;
	uint8_t *dst_lsc = NULL;


	*real_size = sizeof(struct lsc_target_packet) + LSC_GAIN_SIZE_PER_CH*LSC_CHN_NUM;

	if(target_buf_size < *real_size){
		return CONVERT_LSC_ERROR;
	}

	lsc_packet_src = (struct lsc_source_packet *)src_data;
	lsc_packet_dst = (struct lsc_target_packet *)target_buf;

	/*Param from source packet*/
	//lsc_packet_dst->center_x = lsc_packet_src->center_x;
	//lsc_packet_dst->center_y = lsc_packet_src->center_y;

	/*Param local defined*/
	lsc_packet_dst->version_id = RANDOM_LSC_BLOCK_VERSION;;
	lsc_packet_dst->algorithm_version = 2;	//for tshark2
	lsc_packet_dst->compress = 1;
	lsc_packet_dst->image_width = 3264;
	lsc_packet_dst->image_height = 2448;
	lsc_packet_dst->gain_width = 20;
	lsc_packet_dst->gain_height = 16;
	lsc_packet_dst->grid_width = 96;
	lsc_packet_dst->grid_height = 96;
	lsc_packet_dst->percent = 80;
	lsc_packet_dst->bayer_pattern = 0;	//GR
	lsc_packet_dst->gain_number = lsc_packet_dst->gain_width * lsc_packet_dst->gain_height * 4;
	lsc_packet_dst->data_legth = sizeof(struct lsc_target_packet) + LSC_GAIN_SIZE_PER_CH*LSC_CHN_NUM - LSC_CHN_NUM*sizeof(unsigned int *);
	/*use the image center*/
	lsc_packet_dst->center_x = lsc_packet_dst->gain_width / 2;
	lsc_packet_dst->center_y = lsc_packet_dst->gain_height / 2;

	src_lsc = (uint8_t *)lsc_packet_src + sizeof(struct lsc_source_packet);
	dst_lsc = (uint8_t *)lsc_packet_dst + sizeof(struct lsc_target_packet);

	/*compressed gain size*/
	chn_gain_size = LSC_GAIN_SIZE_PER_CH;

#if 0
	for(i=0;i<LSC_CHN_NUM;i++){
		memcpy(dst_lsc, src_lsc, chn_gain_size);

		dst_lsc += chn_gain_size;
		src_lsc += chn_gain_size;
	}
#else
	{
#ifdef CONFIG_USE_ALC_AWB
		uint32_t pattern = 0;
#else
		uint32_t pattern = 1;
#endif
		_change_pattern(dst_lsc, src_lsc, chn_gain_size, pattern);
	}
#endif
#else

#endif
	return CONVERT_LSC_SUCCESS;
}


