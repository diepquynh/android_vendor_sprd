#include <stdio.h>
#include <sys/types.h>
#include <utils/Log.h>
#include "packet_convert.h"

uint16_t golden_awb[GOLDEN_AWB_SIZE]={
                                       0x0078, /*R*/
                                       0x00c8, /*GR*/
                                       0x00c8, /*GB*/
                                       0x0079, /*B*/
};/*R,GR,GB,B*/
uint16_t golden_lsc[4][GOLDEN_LSC_CHANNAL_SIZE]; /*lsc block struct*/

LOCAL int32_t awb_golden_package(cmr_uint param){
	cmr_uint rtn = SENSOR_SUCCESS;
	uint32_t golden_data_temp;
	char	value[100];
	struct awb_target_packet *awb_dst = (struct awb_target_packet *)param;
	struct awb_source_packet *awb_src = (struct awb_source_packet *)golden_awb;
	if(NULL == awb_dst)
		return SENSOR_FAIL;
	property_get("debug.otp.golden", value, "0");
	sscanf(value,"%x",&golden_data_temp);

	awb_dst->version_id = ISP_GOLDEN_AWB_V001;
	/*if not 0,the camera in test pattern*/
	if(golden_data_temp){
		SENSOR_PRINT("golden: %d",golden_data_temp);
		awb_dst->mean_value_R = golden_data_temp & 0xff;
		awb_dst->mean_value_G = (((golden_data_temp >> 8) & 0xff) + ((golden_data_temp >> 16) & 0xff))/2;
		awb_dst->mean_value_B = golden_data_temp >> 24 & 0xff;
	}else{
		awb_dst->mean_value_R = awb_src->average_R;
		awb_dst->mean_value_G = (awb_src->average_G + awb_src->average_Gb)/2;
		awb_dst->mean_value_B = awb_src->average_B;
	}
	return rtn;
}
LOCAL int32_t lsc_golden_package(cmr_uint param){
	cmr_uint rtn = SENSOR_SUCCESS;
	struct sensor_lsc_golden_data *lsc = (struct sensor_lsc_golden_data *)param;
	if(NULL == lsc)
		return SENSOR_FAIL;

	lsc->header.version = ISP_GOLDEN_LSC_V001;
	lsc->header.length = sizeof(golden_lsc);
	lsc->header.block_num = 6;

	//basic info
	lsc->block[0].id = BLOCK_ID_BASIC;
	lsc->block[0].offset =sizeof(struct sensor_lsc_golden_data) + 6 * sizeof(struct lsc_golden_block_info);
	lsc->block[0].size = sizeof(struct lsc_golden_basic_info);

	struct lsc_golden_basic_info *basic_info = (struct lsc_golden_basic_info *)(lsc->block[0].offset+(uint8_t*)lsc);
	basic_info->base_gain = 1024;
	basic_info->algorithm_version = 1;
	basic_info->compress_flag = 1;
	basic_info->image_width = 3264;
	basic_info->image_height = 2448;
	basic_info->gain_width = 20;
	basic_info->gain_height = 16;
	basic_info->optical_x = 1632;
	basic_info->optical_y = 1224;
	basic_info->grid_width = 96;
	basic_info->grid_height = 96;
	basic_info->percent = 80;
	basic_info->bayer_pattern = 0; /*gr*/

	/*standard gain info*/
	lsc->block[1].id = BLOCK_ID_STD_GAIN;
	lsc->block[1].offset = lsc->block[0].offset + lsc->block[0].size;
	lsc->block[1].size = sizeof(struct lsc_golden_gain_info) + GOLDEN_LSC_CHANNAL_SIZE;
	/*r*/
	lsc->block[2].id = BLOCK_ID_DIFF_GAIN;
	lsc->block[2].offset = lsc->block[1].offset + lsc->block[1].size;
	lsc->block[2].size = sizeof(struct lsc_golden_gain_info) + GOLDEN_LSC_CHANNAL_SIZE;
	memcpy((void*)(lsc->block[2].offset + (uint8_t*)lsc),(void*)golden_lsc[0],GOLDEN_LSC_CHANNAL_SIZE);
	/*gr*/
	lsc->block[3].id = BLOCK_ID_DIFF_GAIN;
	lsc->block[3].offset = lsc->block[2].offset + lsc->block[2].size;
	lsc->block[3].size = sizeof(struct lsc_golden_gain_info) + GOLDEN_LSC_CHANNAL_SIZE;
	memcpy((void*)(lsc->block[3].offset + (uint8_t*)lsc),(void*)golden_lsc[1],GOLDEN_LSC_CHANNAL_SIZE);
	/*gb*/
	lsc->block[4].id = BLOCK_ID_DIFF_GAIN;
	lsc->block[4].offset = lsc->block[3].offset + lsc->block[3].size;
	lsc->block[4].size = sizeof(struct lsc_golden_gain_info) + GOLDEN_LSC_CHANNAL_SIZE;
	memcpy((void*)(lsc->block[4].offset + (uint8_t*)lsc),(void*)golden_lsc[2],GOLDEN_LSC_CHANNAL_SIZE);
	/*b*/
	lsc->block[5].id = BLOCK_ID_DIFF_GAIN;
	lsc->block[5].offset = lsc->block[4].offset + lsc->block[4].size;
	lsc->block[5].size = sizeof(struct lsc_golden_gain_info) + GOLDEN_LSC_CHANNAL_SIZE;
	memcpy((void*)(lsc->block[5].offset + (uint8_t*)lsc),(void*)golden_lsc[3],GOLDEN_LSC_CHANNAL_SIZE);

	return rtn;
}
LOCAL cmr_uint golden_data_init(){
	struct sensor_golden_data *golden_data;
	int awb_size,lsc_size;
	awb_size = sizeof(struct awb_target_packet);
	lsc_size = sizeof(struct sensor_lsc_golden_data) + sizeof(struct lsc_golden_basic_info) + \
               5 * (sizeof(struct lsc_golden_gain_info) + GOLDEN_LSC_CHANNAL_SIZE);

	int need_size = sizeof(struct sensor_golden_data) + awb_size + lsc_size;
	golden_data = (struct sensor_golden_data *)malloc(need_size);
	SENSOR_PRINT("golden_data_init: need_size:%d,ptr:0x%x",need_size,golden_data);

	/*init golden data*/
	memset(golden_data, 0,need_size);
	golden_data->header.block_num = 3;
	golden_data->header.length = need_size;
	golden_data->header.start = 0x53505244;
	golden_data->header.version = ISP_GOLDEN_V001;
	golden_data->header.awb_lsc_enable = AWB_CALIBRATION_ENABLE <<8 | LSC_CALIBRATION_ENABLE;

	golden_data->block[0].id =3;/*awb info*/
	golden_data->block[0].offset = sizeof(struct golden_header) + 3 * sizeof(struct block_info);
	golden_data->block[0].length = awb_size;

	golden_data->block[1].id =2;/*lsc block info*/
	golden_data->block[1].offset = golden_data->block[0].offset + golden_data->block[0].length;
	golden_data->block[1].length = lsc_size;

	golden_data->block[2].id =1;/*module info*/
	golden_data->block[2].offset = golden_data->block[1].offset + golden_data->block[1].length;
	golden_data->block[2].length = 0;

	return (cmr_uint)golden_data;
}

LOCAL int32_t construct_golden_data(cmr_uint param){
	cmr_uint rtn = SENSOR_SUCCESS;
	SENSOR_OTP_PARAM_T* param_ptr = (SENSOR_OTP_PARAM_T*)param;
	struct awb_target_packet awb;
	struct sensor_golden_data *golden_data;

	golden_data = (struct sensor_golden_data *)golden_data_init();
	SENSOR_PRINT("construct_golden_data: 0x%x",golden_data);
	if(NULL == golden_data)
		return SENSOR_FAIL;
	param_ptr->golden.data_ptr = (void*)golden_data;
	param_ptr->golden.size = golden_data->header.length;

#if AWB_CALIBRATION_ENABLE
	awb_golden_package((cmr_uint)(golden_data->block[0].offset + (uint8_t*)golden_data));
#endif
#if LSC_CALIBRATION_ENABLE
	lsc_golden_package(golden_data->block[1].offset + (uint8_t*)golden_data);
#endif

	SENSOR_PRINT("golden: %d", param_ptr->golden.size);
	return rtn;
}


LOCAL int32_t awb_package_convert(void *src_data, void *target_buf, uint32_t target_buf_size, uint32_t *real_size)
{
	struct awb_ctrl_rgb *awb_packet_src = NULL;
	struct awb_target_packet *awb_packet_dst = NULL;

	uint32_t random_data_temp;
	char	value[100];
	property_get("debug.otp.random", value, "0");
	sscanf(value,"%x",&random_data_temp);

	if(!src_data || !target_buf)
		return CONVERT_AWB_ERROR;
	awb_packet_src = (struct awb_ctrl_rgb *)src_data;
	awb_packet_dst = (struct awb_target_packet *)target_buf;

	if(target_buf_size < sizeof(struct awb_target_packet)){
		return CONVERT_AWB_ERROR;
	}

	awb_packet_dst->version_id = 1;
	if(random_data_temp){
		awb_packet_dst->mean_value_R = random_data_temp & 0xff;;
		awb_packet_dst->mean_value_G = (((random_data_temp >> 8) & 0xff) + ((random_data_temp >> 16) & 0xff))/2;
		awb_packet_dst->mean_value_B = random_data_temp >> 24 & 0xff;
	}else{
	/*Param from source packet*/
		awb_packet_dst->mean_value_R = awb_packet_src->r;
		awb_packet_dst->mean_value_G = awb_packet_src->g;
		awb_packet_dst->mean_value_B = awb_packet_src->b;
	}
	SENSOR_PRINT("awb_package_convert: r=%d, g=%d, b=%d",awb_packet_dst->mean_value_R,
                                                         awb_packet_dst->mean_value_G,
                                                         awb_packet_dst->mean_value_B);

	*real_size = sizeof(struct awb_target_packet);

	return CONVERT_AWB_SUCCESS;
}

LOCAL void change_pattern(uint8_t *dst, uint8_t *src, uint32_t chn_size, uint32_t pattern)
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

static int32_t lsc_package_convert(void *src_data, void *target_buf, uint32_t target_buf_size, uint32_t *real_size)
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
	if(!src_data || !target_buf)
		return CONVERT_LSC_ERROR;

	lsc_packet_src = (struct lsc_source_packet *)src_data;
	lsc_packet_dst = (struct lsc_target_packet *)target_buf;
	SENSOR_PRINT("lsc_packet_src:0x%x,lsc_packet_dst:0x%x", lsc_packet_src,lsc_packet_dst);

	/*Param from source packet*/
	//lsc_packet_dst->center_x = lsc_packet_src->center_x;
	//lsc_packet_dst->center_y = lsc_packet_src->center_y;

	/*Param local defined*/
	lsc_packet_dst->version_id = RANDOM_LSC_BLOCK_VERSION;;
	lsc_packet_dst->algorithm_version = 1;	//for tshark2
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

        /*modifeid dut different otp lsc*/
	//src_lsc = (uint8_t *)lsc_packet_src + sizeof(struct lsc_source_packet);
        src_lsc = (uint8_t *)lsc_packet_src;
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
		change_pattern(dst_lsc, src_lsc, chn_gain_size, pattern);
	}
#endif
#else

#endif
	return CONVERT_LSC_SUCCESS;
}
