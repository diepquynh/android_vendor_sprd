// lsc_alg_test.cpp : Defines the entry point for the console application.
//

//#include "stdafx.h"
#include "isp_calibration_lsc.h"
#include <stdlib.h>
#include "data_format.h"
#include "isp_otp_calibration.h"
#include "isp_calibration_lsc_internal.h"
#include <string.h>

#include "otp_pack.h"
#include "basic_type.h"
#include "random_unpack.h"
#include "otp_cali.h"

#define RANDOM_NO 2
#define GOLDEN_NO 2

#ifdef __cplusplus
extern "C"
{
#endif

extern int32_t test_create_calibration_data(uint32_t image_pattern, const char *golden_file,
										const char *random_lsc_file, const char *random_awb_file,
										const char *output_file);

#ifdef __cplusplus
}
#endif

char *get_output_path()
{
	return "output\\";
}


char *get_golden_file_name()
{
	const char file_path[] = "output\\";
	static char file_name[128] = "";

	sprintf(file_name, "%s%d#golden.bin", file_path, GOLDEN_NO);

	return file_name;
}

char *get_random_file_name()
{
	const char file_path[] = "output\\";
	static char file_name[128] = "";

	sprintf(file_name, "%s%d#random.bin", file_path, RANDOM_NO);

	return file_name;
}

char *get_random_lsc_file_name()
{
	const char file_path[] = "output\\";
	static char file_name[128] = "";

	sprintf(file_name, "%s%d#random_lsc.bin", file_path, RANDOM_NO);

	return file_name;
}

char *get_random_awb_file_name()
{
	const char file_path[] = "output\\";
	static char file_name[128] = "";

	sprintf(file_name, "%s%d#random_awb.bin", file_path, RANDOM_NO);

	return file_name;
}

char *get_calibration_file_name()
{
	const char file_path[] = "output\\";
	static char file_name[128] = "";

	sprintf(file_name, "%s%d#calibration.bin", file_path, RANDOM_NO);

	return file_name;
}

char *get_pic_file_path()
{
	static char pic_path[128] = "";

	sprintf(pic_path, "8825\\s%d", GOLDEN_NO);	

	return pic_path;
}

#if 0
void test_scaling()
{
	uint16_t *src_img = PNULL;
	uint16_t *dst_img = PNULL;
	FILE *src_handle = PNULL;
	FILE *dst_handle = PNULL;
	char src_file[] = "data_in\\3264X2448_4.raw";
	char dst_file[] = "data_in\\1296x976.raw";
	uint32_t src_img_size = 0;
	uint32_t dst_img_size = 0;
	struct isp_size src_size = {3264, 2448};
	struct isp_size dst_size = {1296, 976};
	struct isp_size src_chn_size = {3264, 2448};
	struct isp_size dst_chn_size = {1296, 976};	
	uint16_t *src_chn[4] = {PNULL};
	uint16_t *dst_chn[4] = {PNULL};
	uint16_t *src_chn_img = PNULL;
	uint16_t *dst_chn_img = PNULL;
	uint32_t i = 0;

	src_img_size = src_size.w * src_size.h * sizeof(uint16_t);
	dst_img_size = dst_size.w * dst_size.h * sizeof(uint16_t);

	src_chn_size.w = src_size.w / 2;
	src_chn_size.h = src_size.h / 2;

	dst_chn_size.w = dst_size.w / 2;
	dst_chn_size.h = dst_size.h / 2;

	//read golden data
	src_handle = fopen(src_file, "rb");
	if (PNULL == src_handle)
		goto EXIT;

	fseek(src_handle,0,SEEK_SET);
	src_img = (uint16_t *)malloc(src_img_size);
	if (PNULL == src_img)
		goto EXIT;	

	src_chn_img = (uint16_t *)malloc(src_img_size);
	if (PNULL == src_chn_img)
		goto EXIT;	

	for (i=0; i<4; i++) {
		src_chn[i] = src_chn_img + i * src_chn_size.w * src_chn_size.h;
		memset(src_chn[i], i, src_chn_size.w * src_chn_size.h * 2);
	}

	if (src_img_size != fread(src_img, 1, src_img_size, src_handle))
		goto EXIT;	

	split_bayer_raw(src_chn, src_img, src_size.w, src_size.h);

	dst_img = (uint16_t *)malloc(dst_img_size);
	if (PNULL == dst_img)
		goto EXIT;

	dst_chn_img = (uint16_t *)malloc(dst_img_size);
	if (PNULL == dst_chn_img)
		goto EXIT;	

	for (i=0; i<4; i++) {
		dst_chn[i] = dst_chn_img + i * dst_chn_size.w * dst_chn_size.h;
		memset(dst_chn[i], i, dst_chn_size.w * dst_chn_size.h * 2);
	}

	for (i=0; i<1; i++) {
		if (ISP_SUCCESS != isp_scaling_uint16(dst_chn[i], src_chn[i], &dst_chn_size, &src_chn_size))
			goto EXIT;
	}

	merge_bayer_raw(dst_img, dst_chn, dst_size.w, dst_size.h);

	dst_handle = fopen(dst_file, "wb");
	if (PNULL == src_handle)
		goto EXIT;	
	
	fwrite(dst_chn[0], 1, dst_chn_size.w * dst_chn_size.h * sizeof(uint16_t), dst_handle);

EXIT:
	if (PNULL == src_handle) {
		fclose(src_handle);
		src_handle = PNULL;
	}

	if (PNULL == dst_handle) {
		fclose(dst_handle);
		dst_handle = PNULL;
	}

	if (PNULL == src_img) {
		free(src_img);
		src_img = PNULL;
	}

	if (PNULL == dst_img) {
		free(dst_img);
		dst_img = PNULL;
	}

	if (PNULL == dst_chn_img) {
		free(dst_chn_img);
		dst_chn_img = PNULL;
	}

	if (PNULL == src_chn_img) {
		free(src_chn_img);
		src_chn_img = PNULL;
	}
}
#endif



void test_print_calibration_data(struct isp_data_t *lsc, struct isp_data_t *awb)
{
	struct isp_cali_awb_info *awb_info = NULL;
	struct isp_cali_lsc_info *lsc_info = NULL;
	struct isp_cali_lsc_map *lsc_map = NULL;
	uint16_t *chn_buf = NULL;
	uint16_t *chn_gain[4] = {NULL};
	uint32_t i = 0;

	awb_info = (struct isp_cali_awb_info *)awb->data_ptr;

	PRINTF("****cali awb info****\n");
	PRINTF("golden avg: %d, %d, %d\n", awb_info->golden_avg[0],
				awb_info->golden_avg[1], awb_info->golden_avg[2]);
	PRINTF("random avg: %d, %d, %d\n", awb_info->ramdon_avg[0],
				awb_info->ramdon_avg[1], awb_info->ramdon_avg[2]);

	PRINTF("****cali lsc info****\n");

	lsc_info = (struct isp_cali_lsc_info *)lsc->data_ptr;

	PRINTF("map num = %d\n", lsc_info->num);

	for (i=0; i<lsc_info->num; i++) {

		uint16_t *data = NULL;
		char file_name[64] = {0};
		uint32_t envi = 0;
		uint32_t ct = 0;
		uint32_t j = 0;

		lsc_map = &lsc_info->map[i];
		envi = lsc_map->ct >> 16;
		ct = lsc_map->ct & 0xffff;

		chn_buf = (uint16_t *)malloc(lsc_map->width * lsc_map->height * sizeof(uint16_t) * 4);
		chn_gain[0] = chn_buf;
		chn_gain[1] = chn_buf + lsc_map->width * lsc_map->height * 1;
		chn_gain[2] = chn_buf + lsc_map->width * lsc_map->height * 2;
		chn_gain[3] = chn_buf + lsc_map->width * lsc_map->height * 3;

		PRINTF("envi=%d, ct=%d\n", envi, ct);
		PRINTF("size: %d X %d\n", lsc_map->width, lsc_map->height);
		PRINTF("grid=%d\n", lsc_map->grid);
		PRINTF("offset=%d, len=%d\n", lsc_map->offset, lsc_map->len);

		data = (uint16_t *)((uint8_t *)&lsc_info->data_area + lsc_map->offset);
		sprintf(file_name, "output\\cali_lsc_%d_[%d_%d].txt", i, envi, ct);
		write_data_uint16_dec(file_name, data, lsc_map->width, lsc_map->len);

		split_bayer_raw(chn_gain, (uint16_t *)data,
										lsc_map->width, lsc_map->height);

		sprintf(file_name, "output\\cali_lsc_%d_chn[%d][%d_%d].txt", i, 0, envi, ct);
		write_data_uint16_dec(file_name, chn_gain[1], lsc_map->width, lsc_map->len / 4);
		sprintf(file_name, "output\\cali_lsc_%d_chn[%d][%d_%d].txt", i, 1, envi, ct);
		write_data_uint16_dec(file_name, chn_gain[0], lsc_map->width, lsc_map->len / 4);
			sprintf(file_name, "output\\cali_lsc_%d_chn[%d][%d_%d].txt", i, 2, envi, ct);
		write_data_uint16_dec(file_name, chn_gain[3], lsc_map->width, lsc_map->len / 4);
			sprintf(file_name, "output\\cali_lsc_%d_chn[%d][%d_%d].txt", i, 3, envi, ct);
		write_data_uint16_dec(file_name, chn_gain[2], lsc_map->width, lsc_map->len / 4);

		free(chn_buf);
	}
}

void parser_calibration_data()
{
	int32_t rtn = 0;
	uint8_t *data_buffer = NULL;
	uint32_t data_length = 0;
	FILE *calibration_handle = NULL;
	struct isp_data_t cali_data = {NULL};
	struct isp_data_t lsc = {NULL};
	struct isp_data_t awb = {0};
	char *calibration_file = get_calibration_file_name();

	//read golden data
	calibration_handle = fopen(calibration_file, "rb");
	if (PNULL == calibration_handle)
		goto EXIT;

	fseek(calibration_handle,0,SEEK_END);
	data_length = ftell(calibration_handle);
	fseek(calibration_handle,0,SEEK_SET);
	data_buffer = (uint8_t *)malloc(data_length);
	if (PNULL == data_buffer)
		goto EXIT;	

	if (data_length != fread(data_buffer, 1, data_length, calibration_handle))
		goto EXIT;

	cali_data.data_ptr = (void *)data_buffer;
	cali_data.size = data_length;

	rtn = isp_parse_calibration_data(&cali_data, &lsc, &awb);
	if (ISP_SUCCESS != rtn)
		goto EXIT;

	test_print_calibration_data(&lsc, &awb);

EXIT:
	if (NULL != data_buffer) {
		free(data_buffer);
		data_buffer = NULL;
	}		
}

void test_golden_pack()
{
	/*const image info*/
	const uint32_t img_w = 3264;
	const uint32_t img_h = 2448;
	char *pic_path = get_pic_file_path();
	char *std_img_name = "dnp.raw";
	char *nonstd_img_name[] = {
		"a.raw",
		"cwf.raw",
		"d65.raw",
		"h.raw",
		"tl84.raw"
//		"outdoor.raw",
//		"lowlight.raw"
	};

	uint32_t std_ct = (0 << 16 | 5000);
	uint32_t nonstd_ct[] = {
		(2 << 16) | 2800,
		(2 << 16) | 4100,
		(2 << 16) | 6500,
//		(3 << 16) | 5500,
//		(1 << 16) | 5500
		(2 << 16) | 2200,
		(2 << 16) | 4000,	
	};

	uint32_t nonstd_img_num = sizeof(nonstd_img_name) / sizeof(char *);
	char *golden_file_name = get_golden_file_name();
	const uint32_t bayer_pattern = ISP_PATTERN_B;
	uint16_t blc_r = 16;
	uint16_t blc_gr = 16;
	uint16_t blc_gb = 16;
	uint16_t blc_b = 16;

	//////////////////////////////////////////////////////////////
	char file_name[64] = {0};
	void *image_buf[10] = {NULL};
	uint32_t image_size = img_w * img_h * sizeof(uint16_t);
	uint32_t i=0;
	int32_t rtn = ISP_SUCCESS;
	struct otp_pack_golden_param proc_param = {0};
	void *golden_buf = NULL;
	uint32_t golden_buf_size = 0;
	uint32_t real_size = 0;

	for (i=0; i<nonstd_img_num+1; i++) {
		image_buf[i] = (void *)malloc(image_size);
		if (NULL == image_buf[i]) {
			goto EXIT;
		}
	}

	/*read image*/
	sprintf(file_name, "%s\\%s", pic_path, std_img_name);
	if (image_size != read_file(file_name, image_buf[0], image_size))
		goto EXIT;

	image_blc((uint16_t *)image_buf[0], (uint16_t *)image_buf[0], img_w,
					img_h, bayer_pattern, blc_gr, blc_r, blc_b, blc_gb);

	for (i=0; i<nonstd_img_num; i++) {
		sprintf(file_name, "%s\\%s", pic_path, nonstd_img_name[i]);
		if (image_size != read_file(file_name, image_buf[1+i], image_size))
			goto EXIT;

		image_blc((uint16_t *)image_buf[i+1], (uint16_t *)image_buf[i+1], img_w,
					img_h, bayer_pattern, blc_gr, blc_r, blc_b, blc_gb);
	}


	rtn = otp_golden_size(&proc_param, &golden_buf_size);
	if (ISP_SUCCESS != rtn)
		goto EXIT;

	golden_buf = (void *)malloc(golden_buf_size);
	if (NULL == golden_buf)
		goto EXIT;

	proc_param.awb.alg_version = 0;
	proc_param.base_gain = 1024;
	proc_param.lsc.alg_version = ISP_LSC_ALG_V2;
	proc_param.lsc.alg_type = ISP_LSC_ALG_1D_DIFF;
	proc_param.lsc.center_type = ISP_OPTICAL_CENTER;
	proc_param.lsc.compress = 0;
	proc_param.lsc.grid_width = 64;
	proc_param.lsc.grid_height = 64;
	proc_param.lsc.percent = 80;

	proc_param.awb.alg_version = 0;
	proc_param.awb.roi_width = 100;
	proc_param.awb.roi_height = 100;
	proc_param.awb.roi_x = (img_w - proc_param.awb.roi_width) / 2;
	proc_param.awb.roi_y = (img_h - proc_param.awb.roi_height) / 2;
	proc_param.awb.roi_x = proc_param.awb.roi_x / 2 * 2;
	proc_param.awb.roi_y = proc_param.awb.roi_y / 2 * 2;

	proc_param.std_img = image_buf[0];
	proc_param.std_ct = std_ct;

	proc_param.image_width = img_w;
	proc_param.image_height = img_h;
	proc_param.image_format = ISP_FORMAT_RAW;
	proc_param.image_pattern = bayer_pattern;

	for (i=0; i<nonstd_img_num; i++) {	
		proc_param.nonstd_ct[i] = nonstd_ct[i];
		proc_param.nonstd_img[i] = image_buf[i+1];
	}

	proc_param.nonstd_num = nonstd_img_num;
	proc_param.target_buf = golden_buf;
	proc_param.target_buf_size = golden_buf_size;

	rtn = otp_pack_golden(&proc_param, &real_size);
	if (ISP_SUCCESS != rtn)
		goto EXIT;

	save_file(golden_file_name, (void *)golden_buf, real_size);

EXIT:
	for (i=0; i<nonstd_img_num+1; i++) {
		if (NULL != image_buf[i]) {
			free(image_buf[i]);
			image_buf[i] = NULL;
		}
	}

	if (NULL != golden_buf) {
		free(golden_buf);
		golden_buf = NULL;
	}
		
	return;
}

void test_random_pack()
{
	/*const image info*/
	const uint32_t img_w = 3264;
	const uint32_t img_h = 2448;
	char *pic_path = get_pic_file_path();
	char *std_img_name = "dnp.raw";
	uint32_t std_ct = (0 << 16 | 5000);

	char *random_file_name = get_random_file_name();
	const uint32_t bayer_pattern = ISP_PATTERN_B;
	uint16_t blc_r = 16;
	uint16_t blc_gr = 16;
	uint16_t blc_gb = 16;
	uint16_t blc_b = 16;

	//////////////////////////////////////////////////////////////
	char file_name[64] = {0};
	void *image_buf[10] = {NULL};
	uint32_t image_size = img_w * img_h * sizeof(uint16_t);
	uint32_t i=0;
	int32_t rtn = ISP_SUCCESS;
	struct otp_pack_random_param proc_param = {0};
	void *random_buf = NULL;
	uint32_t random_buf_size = 0;
	uint32_t real_size = 0;

	char random_path[64] = {0};

	image_buf[0] = (void *)malloc(image_size);
	if (NULL == image_buf[0]) {
		goto EXIT;
	}

	/*read image*/
	sprintf(file_name, "%s\\%s", pic_path, std_img_name);
	if (image_size != read_file(file_name, image_buf[0], image_size))
		goto EXIT;

	image_blc((uint16_t *)image_buf[0], (uint16_t *)image_buf[0], img_w,
					img_h, bayer_pattern, blc_gr, blc_r, blc_b, blc_gb);

	rtn = otp_random_size(&proc_param, &random_buf_size);
	if (ISP_SUCCESS != rtn)
		goto EXIT;

	random_buf = (void *)malloc(random_buf_size);
	if (NULL == random_buf)
		goto EXIT;

	proc_param.awb.alg_version = 0;
	proc_param.base_gain = 1024;
	proc_param.lsc.alg_version = ISP_LSC_ALG_V2;
	proc_param.lsc.center_type = ISP_OPTICAL_CENTER;
	proc_param.lsc.compress = 0;
	proc_param.lsc.grid_width = 64;
	proc_param.lsc.grid_height = 64;
	proc_param.lsc.percent = 80;

	proc_param.awb.alg_version = 0;
	proc_param.awb.roi_width = 100;
	proc_param.awb.roi_height = 100;
	proc_param.awb.roi_x = (img_w - proc_param.awb.roi_width) / 2;
	proc_param.awb.roi_y = (img_h - proc_param.awb.roi_height) / 2;
	proc_param.awb.roi_x = proc_param.awb.roi_x / 2 * 2;
	proc_param.awb.roi_y = proc_param.awb.roi_y / 2 * 2;

	proc_param.image_data= image_buf[0];
	proc_param.image_ct = std_ct;

	proc_param.image_width = img_w;
	proc_param.image_height = img_h;
	proc_param.image_format = ISP_FORMAT_RAW;
	proc_param.image_pattern = bayer_pattern;

	proc_param.target_buf = random_buf;
	proc_param.target_buf_size = random_buf_size;

	rtn = otp_pack_random(&proc_param, &real_size);
	if (ISP_SUCCESS != rtn)
		goto EXIT;

	save_file(random_file_name, (void *)random_buf, real_size);

EXIT:

	if (NULL != image_buf[0]) {
		free(image_buf[0]);
		image_buf[0] = NULL;
	}

	if (NULL != random_buf) {
		free(random_buf);
		random_buf = NULL;
	}
		
	return;
}

void test_random_unpack()
{
	struct random_info info = {0};
	void *random_data = NULL;
	uint32_t random_size = 100 * 1024;
	struct random_lsc_info lsc_info = {0};
	int32_t rtn = 0;
	uint32_t i = 0;
	char *random_file = get_random_file_name();
	char *random_lsc_file = get_random_lsc_file_name();
	char *random_awb_file = get_random_awb_file_name();
	char *output_path = get_output_path();
	char file_name[64] = {0};

	random_data = (void *)malloc(random_size);

	random_size = read_file(random_file, random_data, random_size);

	rtn = random_unpack(random_data, random_size, &info);
	if (0 != rtn) {
		goto EXIT;
	}

	save_file(random_awb_file, (uint16_t *)info.awb, info.awb_size);
	save_file(random_lsc_file, (uint16_t *)info.lsc, info.lsc_size);
	rtn = random_lsc_unpack(info.lsc, info.lsc_size, &lsc_info);
	if (0 != rtn) {
		goto EXIT;
	}

	for (i=0; i<4; i++) {
		sprintf(file_name, "%srandom_gain_chn[%d].txt", output_path, i);
		write_data_uint16_dec(file_name, lsc_info.chn_gain[i], lsc_info.gain_width,
					lsc_info.gain_width * lsc_info.gain_height * sizeof(uint16_t));
	}

EXIT:
	if (NULL != random_data) {
		free(random_data);
		random_data = NULL;
	}	
}

void test_alg_interface()
{
	/*const image info*/
	const uint32_t img_w = 3264;
	const uint32_t img_h = 2448;
	char *pic_path = "8825";
	char *std_img_name = "std_d65.raw";
	uint32_t std_ct = (0 << 16 | 6500);

	char *lsc_file_name = "output\\lsc.txt";
	const uint32_t bayer_pattern = ISP_PATTERN_B;
	uint16_t blc_r = 16;
	uint16_t blc_gr = 16;
	uint16_t blc_gb = 16;
	uint16_t blc_b = 16;

	//////////////////////////////////////////////////////////////
	char file_name[64] = {0};
	void *image_buf[10] = {NULL};
	uint32_t image_size = img_w * img_h * sizeof(uint16_t);
	uint32_t i=0;
	int32_t rtn = ISP_SUCCESS;
	struct otp_pack_random_param proc_param = {0};
	void *target_buf = NULL;
	uint32_t target_buf_size = 0;
	uint32_t real_size = 0;
	uint32_t grid_size = 64;
	uint32_t lsc_alg_id = 0x00020000;
	uint32_t base_gain = 1024;
	uint32_t percent = 80;
	uint16_t *chn_gain[4] = {0};
	uint32_t lsc_version = 0;
	uint32_t data_format = 0;
	uint32_t compress = 0;
	uint32_t chn_size = 0;
	uint32_t idx_gr = 1;
	uint32_t idx_r = 0;
	uint32_t idx_b = 3;
	uint32_t idx_gb = 2;
	uint16_t center_x = 0;
	uint16_t center_y = 0;
	uint32_t center_type = 1;

	image_buf[0] = (void *)malloc(image_size);
	if (NULL == image_buf[0]) {
		goto EXIT;
	}

	/*read image*/
	sprintf(file_name, "%s\\%s", pic_path, std_img_name);
	if (image_size != read_file(file_name, image_buf[0], image_size))
		goto EXIT;

	image_blc((uint16_t *)image_buf[0], (uint16_t *)image_buf[0], img_w,
					img_h, bayer_pattern, blc_gr, blc_r, blc_b, blc_gb);

	rtn = getLSCOneChannelSize(grid_size, img_w, img_h, lsc_alg_id, compress, &chn_size);
	if (ISP_SUCCESS != rtn)
		goto EXIT;

	target_buf_size = chn_size * 4;
	target_buf = (void *)malloc(target_buf_size);
	if (NULL == target_buf)
		goto EXIT;

	chn_gain[idx_r] = (uint16_t *)target_buf;
	chn_gain[idx_gr] = (uint16_t *)((uint8_t *)chn_gain[0] + chn_size);
	chn_gain[idx_gb] = (uint16_t *)((uint8_t *)chn_gain[1] + chn_size);
	chn_gain[idx_b] = (uint16_t *)((uint8_t *)chn_gain[2] + chn_size);

	rtn = calcLSC(image_buf[0], img_w, img_h, bayer_pattern, data_format,
				lsc_alg_id, compress, grid_size, base_gain, percent, 
				chn_gain[idx_r], chn_gain[idx_gr], chn_gain[idx_gb], chn_gain[idx_b], &lsc_version);
	if (ISP_SUCCESS != rtn)
		goto EXIT;

	rtn = calcOpticalCenter(image_buf[0], img_w, img_h, bayer_pattern, data_format, lsc_alg_id, grid_size,
						base_gain, percent, center_type, &center_x, &center_y);
	if (ISP_SUCCESS != rtn)
		goto EXIT;

	//write_data_uint16_interlace(lsc_file_name, (uint16_t *)target_buf, target_buf_size);
	write_data_uint16(lsc_file_name, (uint16_t *)target_buf, target_buf_size);

EXIT:
	if (NULL != target_buf) {
		free(target_buf);
		target_buf = NULL;
	}	
}

int main(int argc, char* argv[])
{	
	/*0: gr, 1:r, 2: b, 3: gb*/
	uint32_t image_pattern = 2;
	char *golden_file = get_golden_file_name();
	char *random_lsc_file = get_random_lsc_file_name();
	char *random_awb_file = get_random_awb_file_name();
	char *calibration_file = get_calibration_file_name();


	//test_alg_interface();

	test_golden_pack();

	test_random_pack();
	test_random_unpack();

	test_create_calibration_data(image_pattern, golden_file, random_lsc_file, random_awb_file, calibration_file);

	parser_calibration_data();
	printf("Hello World!\n");
	return 0;
}

