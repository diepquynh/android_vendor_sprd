#include "isp_otp_calibration.h"
#include "isp_calibration_lsc.h"
#include "isp_calibration_lsc_internal.h"
#include "isp_log.h"
#include "random_unpack.h"
#include "random_pack.h"

#define ISP_CALI_ERROR	0XFF
#define ISP_CALI_SUCCESS 0
#define ISP_CALI_AWB_ID	0x1
#define ISP_CALI_LSC_ID 0x2
#define ISP_GOLDEN_START 0x53505244
#define ISP_VERSION_FLAG 0x5350
#define ISP_GOLDEN_V001	 ((ISP_VERSION_FLAG << 16) | 1)
#define ISP_GOLDEN_LSC_V001	 ((ISP_VERSION_FLAG << 16) | 0x1001)
#define ISP_GOLDEN_AWB_V001	 ((ISP_VERSION_FLAG << 16) | 0x2001)
#define ISP_AWB_BASE_GAIN 	1024

#define ISP_CALI_MIN_SIZE (200 * 1024)

#define     UNUSED(param)  (void)(param)

struct golden_header {
	uint32_t start;
	uint32_t length;
	uint32_t version;
	uint32_t block_num;
};

struct block_info {
	uint32_t id;
	uint32_t offset;
	uint32_t length;
};

struct isp_cali_data {
	uint32_t version;
	uint32_t length;
	uint32_t block_num;
	struct block_info block[2];
	void *data;
};

static int32_t _golden_parse(struct isp_data_t *golden, struct isp_data_t *lsc, struct isp_data_t *awb)
{
	uint8_t *start = PNULL;
	uint8_t *end = PNULL;
	struct golden_header *header = PNULL;
	struct block_info *block_info = PNULL;
	uint8_t module_index = 0xff;
	uint8_t lsc_index = 0xff;
	uint8_t awb_index = 0xff;
	uint16_t version = 0;
	uint32_t i = 0;

	if (PNULL == golden)
		return ISP_CALI_ERROR;

	if (PNULL == golden->data_ptr || 0 == golden->size)
		return ISP_CALI_ERROR;

	if (PNULL == lsc || PNULL == awb)
		return ISP_CALI_ERROR;

	/*check header*/
	header = (struct golden_header *)golden->data_ptr;
	if (golden->size < sizeof(*header ))
		return ISP_CALI_ERROR;

	if (ISP_GOLDEN_START != header->start)
		return ISP_CALI_ERROR;

	/*just support V001 now*/
	if (ISP_GOLDEN_V001 != header->version)
		return ISP_CALI_ERROR;

	start = (uint8_t *)header;
	end = start + header->length;

	block_info = (struct block_info *)((uint8_t *)header + sizeof(*header));

	for (i=0; i<header->block_num; i++) {

		switch (block_info[i].id) {
		case 1:
			/*module info*/
			module_index = i;
			break;

		case 2:
			/*lsc info*/
			lsc_index = i;
			break;

		case 3:
			/*awb info*/
			awb_index = i;
			break;

		default:
			break;
		}
	}

	if (module_index >= header->block_num || lsc_index >= header->block_num
			|| awb_index >= header->block_num)
			return ISP_CALI_ERROR;

	if (start + block_info[module_index].offset + block_info[module_index].length > end)
			return ISP_CALI_ERROR;

	if (start + block_info[lsc_index].offset + block_info[lsc_index].length > end)
			return ISP_CALI_ERROR;

	if (start + block_info[awb_index].offset + block_info[awb_index].length > end)
			return ISP_CALI_ERROR;

	lsc->data_ptr = (void *)&start[block_info[lsc_index].offset];
	lsc->size = block_info[lsc_index].length;

	awb->data_ptr = (void *)&start[block_info[awb_index].offset];
	awb->size = block_info[awb_index].length;

	return ISP_CALI_SUCCESS;
}

static int32_t _cali_lsc(struct isp_data_t *golden, struct isp_data_t *otp, uint32_t image_pattern, struct isp_data_t *target_buf, uint32_t *size)
{
	int32_t rtn = ISP_CALI_SUCCESS;
	struct isp_calibration_lsc_golden_info lsc_golden_info = {0};
	struct isp_calibration_lsc_calc_in lsc_calc_param = {0};
	struct isp_calibration_lsc_calc_out lsc_calc_result = {0};
	uint32_t i = 0;
	uint32_t target_buf_size = 0;

	struct isp_cali_lsc_info *lsc_info = PNULL;
	uint32_t header_size = 0;
	uint32_t data_size = 0;

	if (PNULL == golden || PNULL == otp || PNULL == target_buf || PNULL == size)
		return ISP_CALI_ERROR;

	rtn = isp_calibration_lsc_get_golden_info(golden->data_ptr, golden->size, &lsc_golden_info);
	if (ISP_CALI_SUCCESS != rtn)
		return rtn;

	lsc_info = (struct isp_cali_lsc_info *)target_buf->data_ptr;
	if (PNULL == lsc_info)
		return ISP_CALI_ERROR;

	if (target_buf->size < sizeof(struct isp_cali_lsc_info))
		return ISP_CALI_ERROR;

	header_size = (uint8_t *)&lsc_info->data_area - (uint8_t *)lsc_info;
	target_buf_size = target_buf->size - header_size;

	lsc_calc_param.golden_data = golden->data_ptr;
	lsc_calc_param.golden_data_size = golden->size;
	lsc_calc_param.otp_data = otp->data_ptr;
	lsc_calc_param.otp_data_size = otp->size;
	lsc_calc_param.img_width = lsc_golden_info.img_width;
	lsc_calc_param.img_height = lsc_golden_info.img_height;
	lsc_calc_param.target_buf = (void *)&lsc_info->data_area;
	lsc_calc_param.target_buf_size = target_buf_size;

	/*convert image bayer pattern to lsc gain pattern*/
	switch (image_pattern) {
	case 0: /*gr*/
		lsc_calc_param.lsc_pattern = ISP_CALIBRATION_RGGB;
		break;

	case 1: /*r*/
		lsc_calc_param.lsc_pattern = ISP_CALIBRATION_GRBG;
		break;

	case 2: /*b*/
		lsc_calc_param.lsc_pattern = ISP_CALIBRATION_GBRG;
		break;

	case 3: /*gb*/
		lsc_calc_param.lsc_pattern = ISP_CALIBRATION_BGGR;
		break;

	default:
		return ISP_CALI_ERROR;
	}

	rtn = isp_calibration_lsc_calc(&lsc_calc_param,&lsc_calc_result);
	if (ISP_CALI_SUCCESS != rtn)
		return rtn;

	lsc_info->num = lsc_calc_result.lsc_param_num;
	for (i=0; i<lsc_info->num; i++) {
		lsc_info->map[i].ct = lsc_calc_result.lsc_param[i].light_ct;
		lsc_info->map[i].width = lsc_calc_result.width;
		lsc_info->map[i].height = lsc_calc_result.height;
		lsc_info->map[i].grid = lsc_calc_result.grid;
		lsc_info->map[i].offset = data_size;
		lsc_info->map[i].len = lsc_calc_result.lsc_param[i].size;

		data_size += lsc_calc_result.lsc_param[i].size;
	}

	*size = data_size + header_size;

	return rtn;
}

static int32_t _cali_awb(struct isp_data_t *golden, struct isp_data_t *otp, struct isp_data_t *target_buf, uint32_t *size)
{
	int32_t rtn = ISP_CALI_SUCCESS;
	uint32_t version = 0;
	uint8_t *start = 0;
	uint16_t offset = 0;
	struct isp_cali_awb_info *awb_info = PNULL;

	if (PNULL == golden || PNULL == otp || PNULL == target_buf || PNULL == size)
		return ISP_CALI_ERROR;

	awb_info = (struct isp_cali_awb_info *)target_buf->data_ptr;
	if (PNULL == awb_info)
		return ISP_CALI_ERROR;

	if (target_buf->size < sizeof(struct isp_cali_awb_info))
		return ISP_CALI_ERROR;

	start = (uint8_t *)golden->data_ptr;
	version = *(uint32_t *)start;
	start += 4;

	if (ISP_GOLDEN_AWB_V001 != version)
		return ISP_CALI_ERROR;

	awb_info->golden_avg[0] = *(uint16_t *)start;
	start += 2;
	awb_info->golden_avg[1] = *(uint16_t *)start;
	start += 2;
	awb_info->golden_avg[2] = *(uint16_t *)start;
	//start += 2;

	start = (uint8_t *)otp->data_ptr;
	version = *(uint32_t *)start;
	start += 4;
	awb_info->ramdon_avg[0] = *(uint16_t *)start;
	start += 2;
	awb_info->ramdon_avg[1] = *(uint16_t *)start;
	start += 2;
	awb_info->ramdon_avg[2] = *(uint16_t *)start;
	start += 2;

	*size = sizeof(*awb_info);

	return rtn;
}

/*chn_gain[0]: r; chn_gain[1]: gr; chn_gain[2]: gb; chn_gain[3]: b*/
static int32_t _interlace_gain(uint16_t *chn_gain[4], uint16_t gain_num, uint32_t image_pattern, uint16_t *interlaced_gain)
{
	uint32_t i = 0;
	uint32_t chn_idx[4] = {0};
	uint16_t *chn[4] = {NULL};

	chn[0] = chn_gain[0];
	chn[1] = chn_gain[1];
	chn[2] = chn_gain[2];
	chn[3] = chn_gain[3];

	switch (image_pattern) {
	case 0:	//gr
		chn_idx[0] = 0;
		chn_idx[1] = 1;
		chn_idx[2] = 2;
		chn_idx[3] = 3;
		break;

	case 1:	//r
		chn_idx[0] = 1;
		chn_idx[1] = 0;
		chn_idx[2] = 3;
		chn_idx[3] = 2;
		break;

	case 2:	//b
		chn_idx[0] = 2;
		chn_idx[1] = 3;
		chn_idx[2] = 0;
		chn_idx[3] = 1;
		break;

	case 3: //gb
		chn_idx[0] = 3;
		chn_idx[1] = 2;
		chn_idx[2] = 1;
		chn_idx[3] = 0;
		break;

	default:
		return ISP_CALI_ERROR;
	}

	for (i=0; i<gain_num; i++) {
		*interlaced_gain++ = *chn[chn_idx[0]]++;
		*interlaced_gain++ = *chn[chn_idx[1]]++;
		*interlaced_gain++ = *chn[chn_idx[2]]++;
		*interlaced_gain++ = *chn[chn_idx[3]]++;
	}

	return ISP_CALI_SUCCESS;
}

int32_t isp_calibration_get_info(struct isp_data_t *golden_info, struct isp_cali_info_t *cali_info)
{
	if (PNULL == cali_info || PNULL == golden_info)
		return ISP_CALI_ERROR;

	cali_info->size = 200 * 1024;

	return ISP_CALI_SUCCESS;
}


int32_t isp_calibration(struct isp_cali_param *param, struct isp_data_t *result)
{
	int32_t rtn = ISP_CALI_SUCCESS;
	struct isp_data_t golden_lsc = {0};
	struct isp_data_t golden_awb = {0};
	struct isp_data_t tmp = {0};
	struct isp_cali_data *cali_data = PNULL;
	uint8_t *cur = PNULL;
	uint8_t *start = PNULL;
	uint32_t offset = 0;
	uint32_t awb_size = 0;
	uint32_t lsc_size = 0;

	if (PNULL == param || PNULL == result)
		return ISP_CALI_ERROR;

	if (PNULL == param->target_buf.data_ptr || param->target_buf.size < ISP_CALI_MIN_SIZE)
		return ISP_CALI_ERROR;

	rtn = _golden_parse(&param->golden, &golden_lsc, &golden_awb);
	if (ISP_CALI_SUCCESS != rtn)
		return rtn;

	memset(param->target_buf.data_ptr, 0, param->target_buf.size);

	start = (uint8_t *)param->target_buf.data_ptr;
	cali_data = (struct isp_cali_data *)start;
	cur = (uint8_t *)&cali_data->data;

	cali_data->version = 0x1;
	cali_data->block_num = 2;

	/*write AWB*/
	cali_data->block[0].id = ISP_CALI_AWB_ID;
	cali_data->block[0].offset = cur - start;

	tmp.data_ptr = (void *)cur;
	tmp.size = param->target_buf.size - cali_data->block[0].offset;

	rtn = _cali_awb(&golden_awb, &param->awb_otp, &tmp, &awb_size);
	if (ISP_CALI_SUCCESS != rtn)
		return rtn;

	cali_data->block[0].length = awb_size;
	cur += cali_data->block[0].length;

	/*write LSC*/
	cali_data->block[1].id = ISP_CALI_LSC_ID;
	cali_data->block[1].offset = cur - start;

	tmp.data_ptr = (void *)cur;
	tmp.size = param->target_buf.size - cali_data->block[1].offset;

	rtn = _cali_lsc(&golden_lsc, &param->lsc_otp, param->image_pattern, &tmp, &lsc_size);
	if (ISP_CALI_SUCCESS != rtn)
		return rtn;

	cali_data->block[1].length = lsc_size;
	cur += cali_data->block[1].length;
	cali_data->length = cur - start;

	result->data_ptr = param->target_buf.data_ptr;
	result->size = cali_data->length;

	return ISP_CALI_SUCCESS;
}

int32_t isp_parse_calibration_data(struct isp_data_t *cali_data, struct  isp_data_t *lsc, struct isp_data_t *awb)
{
	int32_t rtn = ISP_CALI_SUCCESS;
	struct isp_cali_data *header = PNULL;
	uint8_t *start = PNULL;
	uint8_t *end = PNULL;
	uint8_t lsc_index = 0xff;
	uint8_t awb_index = 0xff;
	uint32_t i;

	if (NULL == cali_data || NULL == lsc || NULL == awb)
		return ISP_CALI_ERROR;

	header = (struct isp_cali_data *)cali_data->data_ptr;
	start = (uint8_t *)header;

	if (PNULL == header || cali_data->size < sizeof(*header))
		return ISP_CALI_ERROR;

	end = start + header->length;

	if (2 != header->block_num)
		return ISP_CALI_ERROR;

	for (i=0; i<header->block_num; i++) {

		switch (header->block[i].id) {
		case ISP_CALI_AWB_ID:
			awb_index = i;
			break;

		case ISP_CALI_LSC_ID:
			lsc_index = i;
			break;
		}
	}

	if (awb_index >= header->block_num || lsc_index >= header->block_num)
		return ISP_CALI_ERROR;

	if (start + header->block[awb_index].offset + header->block[awb_index].length > end)
		return ISP_CALI_ERROR;

	if (start + header->block[lsc_index].offset + header->block[lsc_index].length > end)
		return ISP_CALI_ERROR;

	if (PNULL != lsc) {
		lsc->data_ptr = (void *)(start + header->block[lsc_index].offset);
		lsc->size = header->block[lsc_index].length;
	}

	if (PNULL != awb) {
		awb->data_ptr = (void *)(start + header->block[awb_index].offset);
		awb->size = header->block[awb_index].length;
	}

	return rtn;
}


int32_t isp_parse_flash_data(struct isp_data_t *flash_data, void *lsc_buf, uint32_t lsc_buf_size, uint32_t image_pattern,
					uint32_t gain_width, uint32_t gain_height, struct isp_cali_awb_gain *awb_gain)
{
	int32_t rtn = ISP_CALI_SUCCESS;
	UNUSED(flash_data);
	UNUSED(lsc_buf);
	UNUSED(lsc_buf_size);
	UNUSED(image_pattern);
	UNUSED(gain_width);
	UNUSED(gain_height);
	UNUSED(awb_gain);
#if 0
	struct random_info rdm_info = {0};
	struct random_lsc_info lsc_info = {0};
	struct random_awb_info awb_info = {0};
	uint32_t max_value = 0;
	uint32_t lsc_gain_num = 0;

	if (NULL == flash_data || NULL == lsc_buf || NULL == awb_gain)
		return ISP_CALI_ERROR;

	rtn = random_unpack(flash_data->data_ptr, flash_data->size, &rdm_info);
	if (ISP_CALI_SUCCESS != rtn)
		return rtn;

	rtn = random_lsc_unpack(rdm_info.lsc, rdm_info.lsc_size, &lsc_info);
	if (ISP_CALI_SUCCESS != rtn)
		return rtn;

	lsc_gain_num = lsc_info.gain_width * lsc_info.gain_height;

	if (NULL == lsc_buf || lsc_buf_size < lsc_gain_num * 4 * sizeof(uint16_t))
		return ISP_CALI_ERROR;

	rtn = _interlace_gain(lsc_buf, lsc_gain_num, image_pattern, lsc_buf);
	if (ISP_CALI_SUCCESS != rtn)
		return rtn;

	rtn = random_awb_unpack(rdm_info.awb, rdm_info.awb_size, &awb_info);
	if (ISP_CALI_SUCCESS != rtn)
		return rtn;

	if (0 == awb_info.avg_r || 0 == awb_info.avg_g || 0 == awb_info.avg_b)
		return ISP_CALI_ERROR;

	max_value = (awb_info.avg_r > awb_info.avg_b) ? awb_info.avg_r : awb_info.avg_b;
	max_value = (max_value > awb_info.avg_g) ? max_value : awb_info.avg_g;

	awb_gain->r = max_value * ISP_AWB_BASE_GAIN / awb_info.avg_r;
	awb_gain->g = max_value * ISP_AWB_BASE_GAIN / awb_info.avg_g;
	awb_gain->b = max_value * ISP_AWB_BASE_GAIN / awb_info.avg_b;
#endif
	return rtn;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*test code*/
#ifdef ISP_CALI_DEBUG
void test_print_golden(struct isp_data_t *golden)
{
	int32_t rtn = 0;
	uint32_t i = 0;
	struct isp_calibration_lsc_golden_cali_info golden_info = {0};
	struct isp_calibration_lsc_flags flag = {0};
	struct isp_data_t golden_lsc = {0};
	struct isp_data_t golden_awb = {0};
	char file_name[64] = {0};
	uint32_t envi = 0;
	uint32_t ct = 0;

	if (NULL == golden) {
		PRINTF("invalid param");
		return;
	}

	rtn = _golden_parse(golden, &golden_lsc, &golden_awb);
	if (ISP_CALI_SUCCESS != rtn)
		return rtn;

	rtn = isp_calibration_lsc_golden_parse(golden_lsc.data_ptr, golden_lsc.size, &flag, &golden_info);
	if (ISP_CALIBRATION_LSC_SUCCESS != rtn) {
		PRINTF("golden parse error!");
		return;
	}

	PRINTF("\n****golden info****\n");
	PRINTF("version: %d\n", flag.version);
	PRINTF("algorithm type: %d, algorithm version: %d\n", flag.alg_type, flag.alg_version);
	PRINTF("compress: %d\n", flag.compress_flag);
	PRINTF("percent: %d\n", flag.percent);
	PRINTF("image size: %d X %d, grid size: %d\n", golden_info.img_width,
									golden_info.img_height,golden_info.grid_size);
	PRINTF("gain size: %d X %d, center: %d X %d\n", golden_info.grid_width, golden_info.grid_height,
									golden_info.center_x, golden_info.center_y);

	envi = golden_info.gain[0].light_ct >> 16;
	ct = golden_info.gain[0].light_ct & 0xffff;
	PRINTF("std gain: envi=%d, ct=%d\n", envi, ct);
	PRINTF("std gain: size=%d\n", golden_info.gain[0].size);
	sprintf(file_name, "output\\golden_std_%d_[%d_%d].txt", 0, envi, ct);
	write_data_uint16(file_name, (uint16_t *)golden_info.gain[0].param, golden_info.gain[0].size);

	PRINTF("diff num: %d\n", golden_info.diff_num);
	for (i=1; i<golden_info.diff_num+1; i++) {
		envi = golden_info.gain[i].light_ct >> 16;
		ct = golden_info.gain[i].light_ct & 0xffff;
		PRINTF("diff gain: envi=%d, ct=%d\n", envi, ct);
		PRINTF("diff gain: size=%d\n", golden_info.gain[i].size);
		sprintf(file_name, "output\\golden_diff_%d_[%d_%d].txt", i, envi, ct);
		write_data_uint16(file_name, (uint16_t *)golden_info.gain[i].param, golden_info.gain[i].size * 4);

	}
}

void test_print_random_lsc(struct isp_data_t *lsc)
{
	struct lsc_random_info *random_info = (struct lsc_random_info *)lsc->data_ptr;

	PRINTF("\n****random LSC****\n", random_info->version);
	PRINTF("version: 0x%x\n", random_info->version);
	PRINTF("length: %d\n", random_info->data_length);
	PRINTF("algorithm version: 0x%x\n", random_info->algorithm_version);
	PRINTF("compress flag: %d\n", random_info->compress_flag);
	PRINTF("bayer pattern: %d\n", random_info->bayer_pattern);
	PRINTF("percent: %d\n", random_info->percent);
	PRINTF("image size: %d X %d, grid size: %d X %d\n", random_info->image_width,
				random_info->image_height,random_info->grid_width, random_info->grid_height);
	PRINTF("gain size: %d X %d, center: %d X %d\n", random_info->gain_width, random_info->gain_height,
								random_info->optical_x, random_info->optical_y);

	PRINTF("gain num: %d\n", random_info->gain_num);

	write_data_uint16("output\\random_lsc.txt", (uint16_t *)&random_info->gain, random_info->gain_width * random_info->gain_height * sizeof(uint16_t) * 4);
}


int32_t test_create_calibration_data(uint32_t image_pattern, const char *golden_file,
										const char *random_lsc_file, const char *random_awb_file,
										const char *calibration_file)
{
#if 0
#ifdef WIN32
	const char golden_file[64] = "output\\1#golden.bin";
	const char random_lsc_file[64] = "output\\1#random_lsc.bin";
	const char random_awb_file[64] = "output\\1#random_awb.bin";
	const char calibration_file[64] = "output\\1#calibration.bin";
#else
	const char golden_file[] = "/data/golden.bin";
	const char random_lsc_file[] = "/data/random_lsc.bin";
	const char random_awb_file[] = "/data/random_awb.bin";
	const char calibration_file[] = "/data/calibration_phone.bin";
#endif
#endif
	int32_t rtn = 0;
	struct isp_data_t golden = {NULL};
	struct isp_data_t target_buf = {NULL};
	struct isp_data_t lsc_otp = {NULL};
	struct isp_data_t awb_otp = {NULL};
	struct isp_cali_info_t cali_info = {0};
	struct isp_cali_param cali_param = {0};
	struct isp_data_t cali_result = {0};
	FILE *golden_handle = NULL;
	FILE *lsc_otp_handle = NULL;
	FILE *awb_otp_handle = NULL;
	FILE *calibration_handle = NULL;

	//read golden data
	golden_handle = fopen(golden_file, "rb");
	if (NULL == golden_handle) {
		ISP_LOGE("open golden file failed");
		goto EXIT;
	}

	fseek(golden_handle,0,SEEK_END);
	golden.size = ftell(golden_handle);
	fseek(golden_handle,0,SEEK_SET);
	golden.data_ptr = malloc(golden.size);
	if (NULL == golden.data_ptr){
		ISP_LOGE("malloc golden memory failed");
		goto EXIT;
	}

	ISP_LOGI("golden file size=%d, buf=%p", golden.size, golden.data_ptr);

	if (golden.size != fread(golden.data_ptr, 1, golden.size, golden_handle)){
		ISP_LOGE("read golden file failed");
		goto EXIT;
	}

	//read otp lsc data
	lsc_otp_handle = fopen(random_lsc_file, "rb");
	if (NULL == lsc_otp_handle){
		ISP_LOGE("open random lsc file failed");
		goto EXIT;
	}

	fseek(lsc_otp_handle,0,SEEK_END);
	lsc_otp.size = ftell(lsc_otp_handle);
	fseek(lsc_otp_handle,0,SEEK_SET);
	lsc_otp.data_ptr = malloc(lsc_otp.size);
	if (NULL == lsc_otp.data_ptr){
		ISP_LOGE("malloc random lsc file failed");
		goto EXIT;
	}

	ISP_LOGI("random lsc file size=%d, buf=%p", lsc_otp.size, lsc_otp.data_ptr);

	if (lsc_otp.size != fread(lsc_otp.data_ptr, 1, lsc_otp.size, lsc_otp_handle)) {
		ISP_LOGE("read random lsc file failed");
		goto EXIT;
	}

	//read otp awb data
	awb_otp_handle = fopen(random_awb_file, "rb");
	if (NULL == awb_otp_handle){
		ISP_LOGE("open random awb file failed");
		goto EXIT;
	}

	fseek(awb_otp_handle,0,SEEK_END);
	awb_otp.size = ftell(awb_otp_handle);
	fseek(awb_otp_handle,0,SEEK_SET);
	awb_otp.data_ptr = malloc(awb_otp.size);
	if (NULL == awb_otp.data_ptr){
		ISP_LOGE("malloc random awb file failed");
		goto EXIT;
	}

	ISP_LOGI("random awb file size=%d, buf=%p", awb_otp.size, awb_otp.data_ptr);

	if (awb_otp.size != fread(awb_otp.data_ptr, 1, awb_otp.size, awb_otp_handle)) {
		ISP_LOGE("read random awb file failed");
		goto EXIT;
	}

	//get the target buffer size
	rtn = isp_calibration_get_info(&golden, &cali_info);
	if (0 != rtn) {
		ISP_LOGE("isp_calibration_get_info failed");
		goto EXIT;
	}

	ISP_LOGI("get calibration info: %d", cali_info.size);

	target_buf.size = cali_info.size;
	target_buf.data_ptr = malloc(target_buf.size);
	if (NULL == target_buf.data_ptr) {
		ISP_LOGE("malloc target buffer failed");
		goto EXIT;
	}

	//get the calibration data, the real size of data will be write to cali_result.size
	cali_param.golden = golden;
	cali_param.awb_otp = awb_otp;
	cali_param.lsc_otp = lsc_otp;
	cali_param.target_buf = target_buf;
	cali_param.image_pattern = image_pattern;

	rtn = isp_calibration(&cali_param, &cali_result);
	if (0 != rtn) {
		ISP_LOGE("isp_calibration failed");
		goto EXIT;
	}

	ISP_LOGI("calibration data: addr=%p, size = %d", cali_result.data_ptr, cali_result.size);

	//TODO: save the calibration data
	calibration_handle = fopen(calibration_file, "wb");
	if (NULL == calibration_handle) {
		ISP_LOGE("open calibration file failed");
		goto EXIT;
	}

	fwrite(cali_result.data_ptr, 1, cali_result.size, calibration_handle);

	test_print_golden(&golden);
	test_print_random_lsc(&lsc_otp);

EXIT:

	if (NULL != golden.data_ptr) {
		free(golden.data_ptr);
		golden.data_ptr = NULL;
	}

	if (NULL != awb_otp.data_ptr) {
		free(awb_otp.data_ptr);
		awb_otp.data_ptr = NULL;
	}

	if (NULL != lsc_otp.data_ptr) {
		free(lsc_otp.data_ptr);
		lsc_otp.data_ptr = NULL;
	}

	if (NULL != target_buf.data_ptr) {
		free(target_buf.data_ptr);
		target_buf.data_ptr = NULL;
	}

	if (NULL != golden_handle) {
		fclose(golden_handle);
		golden_handle = NULL;
	}

	if (NULL != lsc_otp_handle) {
		fclose(lsc_otp_handle);
		lsc_otp_handle = NULL;
	}

	if (NULL != awb_otp_handle) {
		fclose(awb_otp_handle);
		awb_otp_handle = NULL;
	}

	if (NULL != calibration_handle) {
		fclose(calibration_handle);
		calibration_handle = NULL;
	}

	return ISP_CALI_SUCCESS;
}

int32_t test_init_calibration_data(struct isp_data_t *calibration_param)
{
	const char calibration_file[] = "/data/calibration_phone.bin";
	FILE *calibration_handle = NULL;

	if (NULL == calibration_param)
		return ISP_CALI_ERROR;

	//read golden data
	calibration_handle = fopen(calibration_file, "rb");
	if (NULL == calibration_handle) {
		ISP_LOGE("open golden file failed");
		goto EXIT;
	}

	fseek(calibration_handle,0,SEEK_END);
	calibration_param->size = ftell(calibration_handle);
	fseek(calibration_handle,0,SEEK_SET);
	calibration_param->data_ptr = malloc(calibration_param->size);
	if (NULL == calibration_param->data_ptr){
		ISP_LOGE("malloc golden memory failed");
		goto EXIT;
	}

	ISP_LOGI("calibration file size=%d, buf=%p", calibration_param->size, calibration_param->data_ptr);

	if (calibration_param->size != fread(calibration_param->data_ptr, 1, calibration_param->size, calibration_handle)){
		ISP_LOGE("read golden file failed");
		goto EXIT;
	}

	if (NULL != calibration_handle) {
		fclose(calibration_handle);
		calibration_handle = NULL;
	}

	return ISP_CALI_SUCCESS;

EXIT:
	if (NULL != calibration_param->data_ptr) {
		free(calibration_param->data_ptr );
		calibration_param->data_ptr  = NULL;
	}

	if (NULL != calibration_handle) {
		fclose(calibration_handle);
		calibration_handle = NULL;
	}

	return ISP_CALI_ERROR;
}

void test_deinit_calibration_data(struct isp_data_t *calibration_param)
{
	if (NULL == calibration_param)
		return;

	if (NULL != calibration_param->data_ptr)  {
		free(calibration_param->data_ptr );
		calibration_param->data_ptr = NULL;
	}
}
#endif