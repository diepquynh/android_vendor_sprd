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
#ifdef CONFIG_CAMERA_SNR_UV_DENOISE
#define LOG_TAG "snr_uvdenoise"

#include <sys/types.h>
#include <utils/Log.h>
#include <utils/Timers.h>
#include <time.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <time.h>
#include <fcntl.h>		/* low-level i/o */
#include <unistd.h>
#include <arm_neon.h>
#include <semaphore.h>
#include <cutils/properties.h>
#include "cmr_ipm.h"
//#include "cmr_snr_uvdenoise.h"
#include "isp_app.h"
#include "cmr_common.h"
#include "cmr_msg.h"
#include "cmr_oem.h"
#include "sensor_drv_u.h"
#include "snr_uv_interface.h"
//#include "snr_uvdenoise_params.h"
//#include "sensor_ov5648_cnr.h"



#define CAMERA_CNRDE_MSG_QUEUE_SIZE       5
#define THREAD_CNRDE                      1
#define USE_SENSOR_THR_ARR_CLONE


#define CMR_EVT_CNRDE_BASE                (CMR_EVT_IPM_BASE + 0X400)
#define CMR_EVT_CNRDE_INIT                (CMR_EVT_CNRDE_BASE + 0)
#define CMR_EVT_CNRDE_START               (CMR_EVT_CNRDE_BASE + 1)
#define CMR_EVT_CNRDE_EXIT                (CMR_EVT_CNRDE_BASE + 2)

#define CHECK_HANDLE_VALID(handle) \
	do { \
		if (!handle) { \
			return -CMR_CAMERA_INVALID_PARAM; \
		} \
	} while(0)

typedef int (*proc_func) (void *param);
typedef void (*proc_cb) (cmr_handle class_handle, void *param);

struct class_snr_uvde {
	struct ipm_common common;
	sem_t denoise_sem_lock;
	cmr_handle thread_handles[THREAD_CNRDE];
	cmr_uint is_inited;
};

struct snr_uvde_start_param {
	proc_func func_ptr;
	proc_cb cb;
	cmr_handle caller_handle;
	void *param;
};

struct snr_uv_denoise_handle_param {
	int width;
	int height;
	uint8 *snr_uv_in;
	uint8 *snr_uv_out;
	//int8 *mask;
	cur_thr *curthr;
};

uint32 res_index=0;

static struct class_ops snr_uvde_ops_tab_info;

static cmr_int snr_uvde_thread_proc(struct cmr_msg *message, void *private_data)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct class_snr_uvde *class_handle = (struct class_snr_uvde *)private_data;
	cmr_u32 evt = 0;
	struct snr_uvde_start_param *snr_uvde_start_ptr = NULL;

	if (!message || !class_handle) {
		CMR_LOGE("parameter is fail");
		return CMR_CAMERA_INVALID_PARAM;
	}

	evt = (cmr_u32) message->msg_type;

	switch (evt) {
		case CMR_EVT_CNRDE_INIT:
			CMR_LOGI("cnr_init");
			ret = cnr_init();
			if (ret) {
				CMR_LOGE("failed to init snr_uv lib%ld", ret);
			}

			break;

		case CMR_EVT_CNRDE_START:
			snr_uvde_start_ptr = (struct snr_uvde_start_param *)message->data;
			if (snr_uvde_start_ptr->func_ptr) {
				ret = snr_uvde_start_ptr->func_ptr(snr_uvde_start_ptr->param);
			}

			if (snr_uvde_start_ptr->cb) {
				snr_uvde_start_ptr->cb(class_handle, (void *)snr_uvde_start_ptr->param);
			}
			break;

		case CMR_EVT_CNRDE_EXIT:
			CMR_LOGI("cnr_destroy");
			ret = cnr_destroy();
			if (ret) {
				CMR_LOGE("failed to deinit snr_uv lib%ld", ret);
			}

			break;

		default:
			break;
	}

	return ret;
}

static cmr_int snr_uvde_thread_create(struct class_snr_uvde *class_handle)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct class_snr_uvde *snr_uvde_handle = (struct class_snr_uvde *)class_handle;
	cmr_handle *cur_handle_ptr = NULL;
	cmr_int thread_id = 0;

	CHECK_HANDLE_VALID(snr_uvde_handle);
	CMR_MSG_INIT(message);

	if (!class_handle->is_inited) {
		for (thread_id = 0; thread_id < THREAD_CNRDE; thread_id++) {
			cur_handle_ptr = &snr_uvde_handle->thread_handles[thread_id];
			ret = cmr_thread_create(cur_handle_ptr,
					CAMERA_CNRDE_MSG_QUEUE_SIZE, snr_uvde_thread_proc, (void *)class_handle);
			if (ret) {
				CMR_LOGE("send msg failed!");
				ret = CMR_CAMERA_FAIL;
				return ret;
			}
			message.sync_flag = CMR_MSG_SYNC_PROCESSED;
			message.msg_type = CMR_EVT_CNRDE_INIT;
			ret = cmr_thread_msg_send(*cur_handle_ptr, &message);
			if (CMR_CAMERA_SUCCESS != ret) {
				CMR_LOGE("msg send fail");
				ret = CMR_CAMERA_FAIL;
				return ret;
			}

		}

		class_handle->is_inited = 1;
	}

	return ret;
}

static cmr_int snr_uvde_thread_destroy(struct class_snr_uvde *class_handle)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct class_snr_uvde *snr_uvde_handle = (struct class_snr_uvde *)class_handle;
	cmr_handle *cur_handle_ptr = NULL;
	cmr_uint thread_id = 0;

	CHECK_HANDLE_VALID(class_handle);
	CMR_MSG_INIT(message);

	if (snr_uvde_handle->is_inited) {
		for (thread_id = 0; thread_id < THREAD_CNRDE; thread_id++) {

			cur_handle_ptr = &snr_uvde_handle->thread_handles[thread_id];
			message.sync_flag = CMR_MSG_SYNC_PROCESSED;
			message.msg_type = CMR_EVT_CNRDE_EXIT;
			ret = cmr_thread_msg_send(*cur_handle_ptr, &message);
			if (CMR_CAMERA_SUCCESS != ret) {
				CMR_LOGE("msg send fail");
				//ret = CMR_CAMERA_FAIL;
				//return ret;
			}

			if (snr_uvde_handle->thread_handles[thread_id]) {
				ret = cmr_thread_destroy(snr_uvde_handle->thread_handles[thread_id]);
				if (ret) {
					CMR_LOGE("snr_uvde cmr_thread_destroy fail");
					return CMR_CAMERA_FAIL;
				}
				snr_uvde_handle->thread_handles[thread_id] = 0;
			}
		}
		snr_uvde_handle->is_inited = 0;
	}

	return ret;
}

static cmr_int snr_uvde_open(cmr_handle ipm_handle, struct ipm_open_in *in, struct ipm_open_out *out,
		cmr_handle * class_handle)
{
	UNUSED(in);
	UNUSED(out);

	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct class_snr_uvde *snr_uvde_handle = NULL;

	if (!ipm_handle || !class_handle) {
		CMR_LOGE("Invalid Param!");
		return CMR_CAMERA_INVALID_PARAM;
	}

	snr_uvde_handle = (struct class_snr_uvde *)malloc(sizeof(struct class_snr_uvde));
	if (!snr_uvde_handle) {
		CMR_LOGE("No mem!");
		return CMR_CAMERA_NO_MEM;
	}

	cmr_bzero(snr_uvde_handle, sizeof(struct class_snr_uvde));

	snr_uvde_handle->common.ipm_cxt = (struct ipm_context_t *)ipm_handle;
	snr_uvde_handle->common.class_type = IPM_TYPE_SNR_UVDE;
	snr_uvde_handle->common.ops = &snr_uvde_ops_tab_info;
	sem_init(&snr_uvde_handle->denoise_sem_lock, 0, 0);
	ret = snr_uvde_thread_create(snr_uvde_handle);
	if (ret) {
		CMR_LOGE("HDR error: create thread.");
		goto exit;
	}

	*class_handle = (cmr_handle) snr_uvde_handle;
	return ret;

exit:
	if (NULL != snr_uvde_handle)
		free(snr_uvde_handle);
	return CMR_CAMERA_FAIL;

}

static cmr_int snr_uvde_close(cmr_handle class_handle)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct class_snr_uvde *snr_uvde_handle = (struct class_snr_uvde *)class_handle;

	if (!snr_uvde_handle) {
		return CMR_CAMERA_INVALID_PARAM;
	}

	ret = snr_uvde_thread_destroy(snr_uvde_handle);

	if (snr_uvde_handle)
		free(snr_uvde_handle);

	return ret;
}

static cmr_int snr_uvde_choose_cnr_param_size_index(cmr_handle class_handle,cur_param *sensor_cnr_param)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct sensor_exp_info sensor_info;
	cmr_uint sensor_id = 0;
	struct img_size         *act_pic_size = NULL;
	struct img_size         *pic_size = NULL;
	cmr_u32                  search_height = 0;
	cmr_u32                   rot_angle = 0;
	cmr_u32                  sensor_output_height = 0;
	char value_mlog[PROPERTY_VALUE_MAX] = {0};
	struct class_snr_uvde *snr_uvde_handle = (struct class_snr_uvde *)class_handle;
	struct camera_context *cxt = (struct camera_context*)(snr_uvde_handle->common.ipm_cxt->init_in.oem_handle);
	struct cnr_param *cnr_param = sensor_cnr_param;

	ret = snr_uvde_handle->common.ipm_cxt->init_in.get_sensor_info(snr_uvde_handle->common.ipm_cxt->init_in.oem_handle,sensor_id,&sensor_info);

	act_pic_size  = &cxt->snp_cxt.post_proc_setting.actual_snp_size;
	CMR_LOGI("--->	act_pic_size = %d x %d", act_pic_size->width,act_pic_size->height);

	pic_size  = &cxt->snp_cxt.post_proc_setting.snp_size;
	CMR_LOGI("--->	pic_size = %d x %d", pic_size->width,pic_size->height);

	search_height = pic_size->height;
	int j =0;
	CMR_LOGI("search_height = %d,  rot_angle = %d", search_height, rot_angle);
	for (j = SENSOR_MODE_PREVIEW_ONE; j < SENSOR_MODE_MAX; j++) {
		if (SENSOR_MODE_MAX != sensor_info.mode_info[j].mode) {
			sensor_output_height = sensor_info.mode_info[j].trim_height;
			CMR_LOGI("sensor_output_height = %d", sensor_output_height);
			//sensor_output_height = CAMERA_ALIGNED_16(sensor_output_height);
			if ((search_height <= sensor_output_height)||(search_height <= CAMERA_ALIGNED_16(sensor_output_height)) ) {
				//target_mode = i;
				//ret = CMR_CAMERA_SUCCESS;
				break;
			} else {
				//last_mode = i;
			}
		}
	}

	if(sensor_output_height == cnr_param[CAMERA_MODE_PREVIEW].res.height)
		res_index = CAMERA_MODE_PREVIEW;
	else if(sensor_output_height == cnr_param[CAMERA_MODE_CAPTURE].res.height)
		res_index = CAMERA_MODE_CAPTURE;
	else{
		CMR_LOGE("not find match height,please check the cnr pararm!sensor_cnr_param[0].res.height =%ld,sensor_cnr_param[1].res.height=%ld",sensor_cnr_param[0].res.height,sensor_cnr_param[1].res.height);
		return CMR_CAMERA_INVALID_PARAM;
	}

	CMR_LOGI("act sensor_output_height = %d", sensor_output_height);
	//CMR_LOGI("--->	res_index = %ld  ,[CAMERA_MODE_PREVIEW].res.width =%d  [CAMERA_MODE_CAPTURE].res.width = %d",res_index,sensor_cnr_param[CAMERA_MODE_PREVIEW].res.width,sensor_cnr_param[CAMERA_MODE_CAPTURE].res.width);
	return CMR_CAMERA_SUCCESS;
}

static int camera_get_system_time(char *datetime)
{
	time_t timep;
	struct tm *p;
	time(&timep);
	p = localtime(&timep);
	sprintf(datetime, "%04d%02d%02d%02d%02d%02d",
			(1900 + p->tm_year),
			(1 + p->tm_mon),
			p->tm_mday,
			p->tm_hour,
			p->tm_min,
			p->tm_sec);

	CMR_LOGI("datatime = %s", datetime);

	return CMR_CAMERA_SUCCESS;
}

static cmr_int cnr_save_yuv_to_file(cmr_u32 gain,  cur_param* sensor_cnr_param,cmr_u32 width, cmr_u32 height, struct img_addr *addr)
{
#define FILE_DIR "/data/misc/media/"
#define FILE_NAME_LEN 200
	cmr_int	ret = CMR_CAMERA_SUCCESS;
	char		file_name[FILE_NAME_LEN + 1] = {0};
	char		tmp_str[50];
	FILE 		*fp = NULL;
	char 	datetime[15] = {0};
	struct cnr_param *cnr_param = sensor_cnr_param;

	camera_get_system_time(datetime);
	if(cnr_param!=NULL)
	{
		CMR_LOGI("gain %d act width %d heght %d, orig %ld x %ld", gain, width, height,cnr_param[res_index].res.width,cnr_param[res_index].res.height);

		strcpy(file_name, FILE_DIR);
		sprintf(tmp_str, "%d", width);
		strcat(file_name, tmp_str);
		strcat(file_name, "X");
		sprintf(tmp_str, "%d", height);
		strcat(file_name, tmp_str);

		strcat(file_name, "_orig_");
		sprintf(tmp_str, "%ld",cnr_param[res_index].res.width);
		strcat(file_name, tmp_str);
		strcat(file_name, "X");
		sprintf(tmp_str, "%ld", cnr_param[res_index].res.height);
		strcat(file_name, tmp_str);

		strcat(file_name, "_");
		sprintf(tmp_str, "%s", datetime);
		strcat(file_name, tmp_str);

		strcat(file_name, "_gain_");
		sprintf(tmp_str, "%d", gain);
		strcat(file_name, tmp_str);

		strcat(file_name, ".y420");
		CMR_LOGI("file name %s", file_name);
		fp = fopen(file_name, "wb");

		if (NULL == fp) {
			CMR_LOGI("can not open file: %s \n", file_name);
			return CMR_CAMERA_INVALID_PARAM;
		}

		fwrite((void*)addr->addr_y, 1, width * height, fp);
		fclose(fp);

		bzero(file_name, FILE_NAME_LEN);
		strcpy(file_name, FILE_DIR);
		sprintf(tmp_str, "%d", width);
		strcat(file_name, tmp_str);
		strcat(file_name, "X");
		sprintf(tmp_str, "%d", height);
		strcat(file_name, tmp_str);

		strcat(file_name, "_orig_");
		sprintf(tmp_str, "%ld",cnr_param[res_index].res.width);
		strcat(file_name, tmp_str);
		strcat(file_name, "X");
		sprintf(tmp_str, "%ld", cnr_param[res_index].res.height);
		strcat(file_name, tmp_str);

		strcat(file_name, "_");
		sprintf(tmp_str, "%s", datetime);
		strcat(file_name, tmp_str);

		strcat(file_name, "_gain_");
		sprintf(tmp_str, "%d", gain);

		strcat(file_name, tmp_str);
		strcat(file_name, ".vu420");
		CMR_LOGI("file name %s", file_name);
		fp = fopen(file_name, "wb");
		if (NULL == fp) {
			CMR_LOGI("can not open file: %s \n", file_name);
			return CMR_CAMERA_INVALID_PARAM;
		}

		fwrite((void*)addr->addr_u, 1, width * height / 2, fp);
		fclose(fp);
	}
	else
	{
		CMR_LOGI("capture init act width %d heght %d", width, height);

		strcpy(file_name, FILE_DIR);
		sprintf(tmp_str, "%d", width);
		strcat(file_name, tmp_str);
		strcat(file_name, "X");
		sprintf(tmp_str, "%d", height);
		strcat(file_name, tmp_str);

		strcat(file_name, "_");
		sprintf(tmp_str, "%s", datetime);
		strcat(file_name, tmp_str);

		strcat(file_name, "_init");

		strcat(file_name, ".y420");
		CMR_LOGI("file name %s", file_name);
		fp = fopen(file_name, "wb");

		if (NULL == fp) {
			CMR_LOGI("can not open file: %s \n", file_name);
			return CMR_CAMERA_INVALID_PARAM;
		}

		fwrite((void*)addr->addr_y, 1, width * height, fp);
		fclose(fp);

		bzero(file_name, FILE_NAME_LEN);
		strcpy(file_name, FILE_DIR);
		sprintf(tmp_str, "%d", width);
		strcat(file_name, tmp_str);
		strcat(file_name, "X");
		sprintf(tmp_str, "%d", height);
		strcat(file_name, tmp_str);

		strcat(file_name, "_");
		sprintf(tmp_str, "%s", datetime);
		strcat(file_name, tmp_str);

		strcat(file_name, "_init");

		strcat(file_name, ".vu420");
		CMR_LOGI("file name %s", file_name);
		fp = fopen(file_name, "wb");
		if (NULL == fp) {
			CMR_LOGI("can not open file: %s \n", file_name);
			return CMR_CAMERA_INVALID_PARAM;
		}

		fwrite((void*)addr->addr_u, 1, width * height / 2, fp);
		fclose(fp);


	}
	return CMR_CAMERA_SUCCESS;
}

static int snr_uv_proc_func(void* param)
{

	nsecs_t timestamp1 = 0;
	nsecs_t timestamp2 = 0;
	uint32 snr_uv_duration = 0;
	cmr_int ret = -1;

	struct snr_uv_denoise_handle_param *snr_uvde_param = (struct snr_uv_denoise_handle_param *)param;
	if (NULL == param) {
		CMR_LOGE("error param");
		return CMR_CAMERA_INVALID_PARAM;
	}
	CMR_LOGI("snr_uvdenoise begin width = %d, height = %d, snr_uvde_param->curthr =%p", snr_uvde_param->width, snr_uvde_param->height,snr_uvde_param->curthr);

	timestamp1 = systemTime(CLOCK_MONOTONIC);

	//camera_flush();
	//cmr_snapshot_memory_flush(cxt->snp_cxt.snapshot_handle);

	ret = cnr(snr_uvde_param->snr_uv_in, snr_uvde_param->width, snr_uvde_param->height, snr_uvde_param->curthr);

	//camera_flush();
	//cmr_snapshot_memory_flush(cxt->snp_cxt.snapshot_handle);

	timestamp2 = systemTime(CLOCK_MONOTONIC);
	timestamp2 -= timestamp1;
	snr_uv_duration = (uint32_t) timestamp2;
	CMR_LOGI("enc_in_param.size.width = %d, enc_in_param.size.height = %d, snr_uv ret is %ld, duration is %ld", snr_uvde_param->width, snr_uvde_param->height, ret, snr_uv_duration);

exit:
	return 0;
}

static void snr_uv_proc_cb(cmr_handle class_handle, void *param)
{
	UNUSED(param);

	struct class_snr_uvde *snr_uvde_handle = (struct class_snr_uvde *)class_handle;
	CMR_LOGI("%s called! release sem lock", __func__);
	sem_post(&snr_uvde_handle->denoise_sem_lock);
}

static cmr_int snr_uvde_start(cmr_handle class_handle, cmr_uint thread_id, proc_func func_ptr, proc_cb cb, void *param)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct class_snr_uvde *snr_uvde_handle = (struct class_snr_uvde *)class_handle;
	cmr_handle *cur_handle_ptr = NULL;
	struct snr_uvde_start_param *snr_uvde_start_ptr = NULL;

	CMR_MSG_INIT(message);

	if (!class_handle) {
		CMR_LOGE("parameter is NULL. fail");
		return -CMR_CAMERA_INVALID_PARAM;
	}

	snr_uvde_start_ptr = (struct snr_uvde_start_param *)malloc(sizeof(struct snr_uvde_start_param));
	if (!snr_uvde_start_ptr) {
		CMR_LOGE("no mem");
		return CMR_CAMERA_NO_MEM;
	}
	memset(snr_uvde_start_ptr, 0, sizeof(struct snr_uvde_start_param));
	cur_handle_ptr = &snr_uvde_handle->thread_handles[thread_id];
	snr_uvde_start_ptr->func_ptr = func_ptr;
	snr_uvde_start_ptr->param = param;
	snr_uvde_start_ptr->cb = cb;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	message.msg_type = CMR_EVT_CNRDE_START;
	message.alloc_flag = 1;
	message.data = snr_uvde_start_ptr;
	ret = cmr_thread_msg_send(*cur_handle_ptr, &message);
	if (ret) {
		CMR_LOGE("send msg fail");
		if (snr_uvde_start_ptr)
			free(snr_uvde_start_ptr);
		ret = CMR_CAMERA_FAIL;
	}

	return ret;
}




cur_thr sensor_thr_array;
static cmr_int snr_uvde_choose_gain_range(/*cmr_uint sensor_id, */float gain,cur_param *sensor_cnr_param, cur_thr **curthr)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	cmr_int i = 0;

	float snr_analog_gain = 0.0;
	uint32_t snr_uv_index = 0;
	struct cnr_param *cnr_param = sensor_cnr_param;


	//cmr_s8* sensor_name = sensor_get_cur_name();
	//snr_uv_analog_gain = get_cnr_analog_gain();

	snr_analog_gain = (float)gain;
	CMR_LOGI("---> snr_analog_gain = %f cnr_param = %p ", snr_analog_gain,cnr_param);

	CMR_LOGI("--->  cnr_param gain_range = %f--%f",  cnr_param[res_index].gain_range[i].gain_lower, cnr_param[res_index].gain_range[i].gain_upper);

	for( i = 0; i < MAX_CNR_GAIN_STEP;  i ++)
	{

		if( snr_analog_gain >= cnr_param[res_index].gain_range[i].gain_lower && snr_analog_gain < cnr_param[res_index].gain_range[i].gain_upper )
		{
			snr_uv_index = i;
			CMR_LOGI("---> find snr_uv_index = %d", snr_uv_index);
			break;
		}
	}
	if( i == MAX_CNR_GAIN_STEP)
	{
		CMR_LOGI("can't find the snr_uv_index");
		return CMR_CAMERA_INVALID_PARAM;
	}


	CMR_LOGI("---> snr_uv_index = %d, i = %ld", snr_uv_index, i);
	sensor_thr_array.udw[0] = cnr_param[res_index].thr[snr_uv_index].udw[0];
	sensor_thr_array.udw[1] = cnr_param[res_index].thr[snr_uv_index].udw[1];
	sensor_thr_array.udw[2] = cnr_param[res_index].thr[snr_uv_index].udw[2];
	sensor_thr_array.vdw[0] = cnr_param[res_index].thr[snr_uv_index].vdw[0];
	sensor_thr_array.vdw[1] = cnr_param[res_index].thr[snr_uv_index].vdw[1];
	sensor_thr_array.vdw[2] = cnr_param[res_index].thr[snr_uv_index].vdw[2];
	sensor_thr_array.urw[0] = cnr_param[res_index].thr[snr_uv_index].urw[0];
	sensor_thr_array.urw[1] = cnr_param[res_index].thr[snr_uv_index].urw[1];
	sensor_thr_array.urw[2] = cnr_param[res_index].thr[snr_uv_index].urw[2];
	sensor_thr_array.vrw[0] = cnr_param[res_index].thr[snr_uv_index].vrw[0];
	sensor_thr_array.vrw[1] = cnr_param[res_index].thr[snr_uv_index].vrw[1];
	sensor_thr_array.vrw[2] = cnr_param[res_index].thr[snr_uv_index].vrw[2];

	*curthr = &sensor_thr_array;

	//curthr = &cnr_param[CAMERA_MODE_CAPTURE].thr[snr_uv_index];
	//*curthr = &ov5648_cnr_param[0].thr[0];
#if 0
	//cur_thr curthr = sensor_cnr_param[0].thr[0];
	int j = 0;
	for (j = 0; j < 4; j++)
	{
		CMR_LOGE("before cnr_param[%d].thr[%d].uthr[%d]= %x", res_index,snr_uv_index,j, cnr_param[res_index].thr[snr_uv_index].uthr[i]);
		CMR_LOGE("before cnr_param[%d].thr[%d].vthr[%d]= %x", res_index,snr_uv_index,j, cnr_param[res_index].thr[snr_uv_index].vthr[i]);
	}
	for (j = 0; j < 25; j++)
	{
		CMR_LOGE("before cnr_param[%d]].curthr[%d]->udw[0][%d] = %x", res_index,snr_uv_index,j, cnr_param[res_index].thr[snr_uv_index].udw[0][j]);
		CMR_LOGE("before cnr_param[%d]].curthr[%d]->udw[1][%d] = %x", res_index,snr_uv_index,j, cnr_param[res_index].thr[snr_uv_index].udw[1][j]);
		CMR_LOGE("before cnr_param[%d]].curthr[%d]->udw[2][%d] = %x", res_index,snr_uv_index,j, cnr_param[res_index].thr[snr_uv_index].udw[2][j]);
		CMR_LOGE("before cnr_param[%d]].curthr[%d]->vdw[0][%d] = %x", res_index,snr_uv_index,j, cnr_param[res_index].thr[snr_uv_index].vdw[0][j]);
		CMR_LOGE("before cnr_param[%d]].curthr[%d]->vdw[1][%d] = %x", res_index,snr_uv_index,j, cnr_param[res_index].thr[snr_uv_index].vdw[1][j]);
		CMR_LOGE("before cnr_param[%d]].curthr[%d]->vdw[2][%d] = %x", res_index,snr_uv_index,i, cnr_param[res_index].thr[snr_uv_index].vdw[2][j]);
		CMR_LOGE("before cnr_param[%d]].curthr[%d]->urw[0][%d] = %x", res_index,snr_uv_index,j, cnr_param[res_index].thr[snr_uv_index].urw[0][j]);
		CMR_LOGE("before cnr_param[%d]].curthr[%d]->urw[1][%d] = %x", res_index,snr_uv_index,j, cnr_param[res_index].thr[snr_uv_index].urw[1][j]);
		CMR_LOGE("before cnr_param[%d]].curthr[%d]->urw[2][%d] = %x", res_index,snr_uv_index,j, cnr_param[res_index].thr[snr_uv_index].urw[2][j]);
		CMR_LOGE("before cnr_param[%d]].curthr[%d]->vrw[0][%d] = %x", res_index,snr_uv_index,j, cnr_param[res_index].thr[snr_uv_index].vrw[0][j]);
		CMR_LOGE("before cnr_param[%d]].curthr[%d]->vrw[1][%d] = %x", res_index,snr_uv_index,j, cnr_param[res_index].thr[snr_uv_index].vrw[1][j]);
		CMR_LOGE("before cnr_param[%d]].curthr[%d]->vrw[2][%d] = %x", res_index,snr_uv_index,j, cnr_param[res_index].thr[snr_uv_index].vrw[2][j]);
	}
#endif

	CMR_LOGI("*curthr = %p ", *curthr);

	//CMR_LOGI("---> curthr = %p ,*curthr.uthr[0] =%d, curthr.udw[0][0]= %d,udw[0][0] = %d", *curthr,curthr.uthr[0], cnr_param[CAMERA_MODE_CAPTURE].thr[snr_uv_index].udw[0][0],ov5648_cnr_param[CAMERA_MODE_CAPTURE].thr[snr_uv_index].udw[0][0]);

	ret = CMR_CAMERA_SUCCESS;
exit:
	return ret;
}



static cmr_int snr_uvde_transfer_frame(cmr_handle class_handle, struct ipm_frame_in *in, struct ipm_frame_out *out)
{

	UNUSED(out);

	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct class_snr_uvde *snr_uvde_handle = (struct class_snr_uvde *)class_handle;
	struct snr_uv_denoise_handle_param snr_uvde_param = {0x00};
	struct camera_context *cxt = (struct camera_context*)(snr_uvde_handle->common.ipm_cxt->init_in.oem_handle);
	cmr_uint sensor_id = 0;
	cmr_uint thread_id = 0;
	cmr_uint out_height = 0;
	float gain = 0;
	struct cnr_param* sensor_cnr_param;
	char value[PROPERTY_VALUE_MAX] = {0};
	property_get("persist.sys.camera.cnr.yuv", value, "0");
	if (!strcmp(value, "2"))
	{
		cnr_save_yuv_to_file(gain * 100, NULL,in->src_frame.size.width, in->src_frame.size.height, &in->src_frame.addr_vir);
	}

	property_get("persist.sys.camera.cnr.disable", value, "0");
	if (!strcmp(value, "1"))
		goto snr_uvdenoise_exit;

	if (!in || !class_handle) {
		CMR_LOGE("Invalid Param!");
		return CMR_CAMERA_INVALID_PARAM;
	}
	sensor_id = snr_uvde_handle->common.ipm_cxt->init_in.sensor_id;
	snr_uvde_param.height = in->src_frame.size.height;
	snr_uvde_param.width = in->src_frame.size.width;
	snr_uvde_param.snr_uv_in = (uint8 *)in->src_frame.addr_vir.addr_u;

	ret = cmr_sensor_get_real_gain(cxt->sn_cxt.sensor_handle, sensor_id, &gain);
	if(ret != CMR_CAMERA_SUCCESS)
		goto snr_uvdenoise_exit;
	CMR_LOGI("--->	gain = %f, ret = %ld", gain,ret);
	//cnr_save_gain_to_file(gain * 10);

	ret = cmr_sensor_get_cnr_param(cxt->sn_cxt.sensor_handle, cxt->camera_id, &sensor_cnr_param);
	if(ret != CMR_CAMERA_SUCCESS)
		goto snr_uvdenoise_exit;
	CMR_LOGI("--->	sensor_cnr_param = %p", sensor_cnr_param);

	if (0 != snr_uvde_choose_cnr_param_size_index(snr_uvde_handle,sensor_cnr_param))
		goto snr_uvdenoise_exit;

	property_get("persist.sys.camera.cnr.yuv", value, "0");
	if (!strcmp(value, "1"))
	{
		cnr_save_yuv_to_file(gain * 100, sensor_cnr_param,in->src_frame.size.width, in->src_frame.size.height, &in->src_frame.addr_vir);
	}

	if (0 != snr_uvde_choose_gain_range(/*sensor_id, */gain,sensor_cnr_param,&snr_uvde_param.curthr))
		goto snr_uvdenoise_exit;

	snr_uvde_start(snr_uvde_handle, thread_id, snr_uv_proc_func, snr_uv_proc_cb, &snr_uvde_param);
	sem_wait(&snr_uvde_handle->denoise_sem_lock);

snr_uvdenoise_exit:
	ret = CMR_CAMERA_SUCCESS;
	CMR_LOGI("--->	EXIT");

	return ret;

}

static cmr_int snr_uvde_pre_proc(cmr_handle class_handle)
{
	UNUSED(class_handle);

	cmr_int ret = CMR_CAMERA_SUCCESS;

	/*no need to do */

	return ret;
}

static cmr_int snr_uvde_post_proc(cmr_handle class_handle)
{
	UNUSED(class_handle);

	cmr_int ret = CMR_CAMERA_SUCCESS;

	/*no need to do */

	return ret;
}

static struct class_ops snr_uvde_ops_tab_info = {
	.open = snr_uvde_open,
	.close = snr_uvde_close,
	.transfer_frame = snr_uvde_transfer_frame,
	.pre_proc = snr_uvde_pre_proc,
	.post_proc = snr_uvde_post_proc,
};

struct class_tab_t snr_uvde_tab_info = {
	&snr_uvde_ops_tab_info,
};
#endif
