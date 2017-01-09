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

#if defined (CONFIG_CAMERA_FACE_DETECT) && defined (CONFIG_CAMERA_FACE_DETECT_SPRD)

#define LOG_TAG "cmr_fd_sprd"

#include "cmr_msg.h"
#include "cmr_ipm.h"
#include "cmr_common.h"
#include "SprdOEMCamera.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "sprdfdapi.h"
#include "facealignapi.h"
#include "faceattributeapi.h"

#define FD_MAX_FACE_NUM         10
#define FD_RUN_FAR_INTERVAL     6   /* The frame interval to run FAR. For reducing computation cost */

struct class_faceattr {
	FA_SHAPE        shape;
	FAR_ATTRIBUTE   attr;
	int             face_id;  /* face id gotten from face detection */
};

struct class_faceattr_array {
	int                     count;                   /* face count      */
	int                     frame_idx;               /* The frame when the face attributes are updated */
	struct class_faceattr   face[FD_MAX_FACE_NUM+1]; /* face attricutes */
};

struct class_fd {
	struct ipm_common               common;
	cmr_handle                      thread_handle;
	cmr_uint                        is_busy;
	cmr_uint                        is_inited;
	void                            *alloc_addr;
	cmr_uint                        mem_size;
	cmr_uint                        frame_cnt;
	cmr_uint                        frame_total_num;
	struct ipm_frame_in             frame_in;
	struct ipm_frame_out            frame_out;
	ipm_callback                    frame_cb;
	struct img_size                 fd_img_size;
	struct img_face_area            face_area_prev;/* The faces detected from the previous frame; It is used to make face detection results more stable */
	struct class_faceattr_array     faceattr_arr;  /* face attributes */
	cmr_uint                        curr_frame_idx;
	cmr_uint                        is_get_result;
	FD_DETECTOR_HANDLE              hDT;           /* Face Detection Handle */
	FA_ALIGN_HANDLE                 hFaceAlign;    /* Handle for face alignment */
	FAR_RECOGNIZER_HANDLE           hFAR;          /* Handle for face attribute recognition */
};


struct fd_start_parameter {
	void                            *frame_data;
	ipm_callback                    frame_cb;
	cmr_handle                      caller_handle;
	void                            *private_data;
};

static cmr_int fd_open(cmr_handle ipm_handle, struct ipm_open_in *in, struct ipm_open_out *out,
				cmr_handle *out_class_handle);
static cmr_int fd_close(cmr_handle class_handle);
static cmr_int fd_transfer_frame(cmr_handle class_handle,struct ipm_frame_in *in, struct ipm_frame_out *out);
static cmr_int fd_pre_proc(cmr_handle class_handle);
static cmr_int fd_post_proc(cmr_handle class_handle);
static cmr_int fd_start(cmr_handle class_handle, struct fd_start_parameter *param);
static cmr_uint check_size_data_invalid(struct img_size *fd_img_size);
static cmr_int fd_call_init(struct class_fd *class_handle, const struct img_size *fd_img_size);
static cmr_uint fd_is_busy(struct class_fd *class_handle);
static void fd_set_busy(struct class_fd *class_handle, cmr_uint is_busy);
static cmr_int fd_thread_create(struct class_fd *class_handle);
static cmr_int fd_thread_proc(struct cmr_msg *message, void *private_data);


static struct class_ops fd_ops_tab_info = {
	fd_open,
	fd_close,
	fd_transfer_frame,
	fd_pre_proc,
	fd_post_proc,
};

struct class_tab_t fd_tab_info = {
	&fd_ops_tab_info,
};


#define CMR_EVT_FD_START          (1 << 16)
#define CMR_EVT_FD_EXIT           (1 << 17)
#define CMR_EVT_FD_INIT           (1 << 18)
#define CMR__EVT_FD_MASK_BITS     (cmr_u32)(CMR_EVT_FD_START | \
					CMR_EVT_FD_EXIT | \
					CMR_EVT_FD_INIT)

#define CAMERA_FD_MSG_QUEUE_SIZE  5
#define IMAGE_FORMAT              "YVU420_SEMIPLANAR"

#define CHECK_HANDLE_VALID(handle) \
	do { \
		if (!handle) { \
			return CMR_CAMERA_INVALID_PARAM; \
		} \
	} while(0)


static cmr_int fd_open(cmr_handle ipm_handle, struct ipm_open_in *in, struct ipm_open_out *out,
				cmr_handle *out_class_handle)
{
	cmr_int              ret        = CMR_CAMERA_SUCCESS;
	struct class_fd      *fd_handle = NULL;
	struct img_size      *fd_img_size;

	if (!out || !in || !ipm_handle || !out_class_handle) {
		CMR_LOGE("Invalid Param!");
		return CMR_CAMERA_INVALID_PARAM;
	}

	fd_handle = (struct class_fd *)malloc(sizeof(struct class_fd));
	if (!fd_handle) {
		CMR_LOGE("No mem!");
		return CMR_CAMERA_NO_MEM;
	}

	cmr_bzero(fd_handle, sizeof(struct class_fd));

	fd_handle->common.ipm_cxt     = (struct ipm_context_t*)ipm_handle;
	fd_handle->common.class_type  = IPM_TYPE_FD;
	fd_handle->common.ops         = &fd_ops_tab_info;
	fd_handle->frame_cb           = in->reg_cb;
	fd_handle->mem_size           = in->frame_size.height * in->frame_size.width * 3 / 2;
	fd_handle->frame_total_num    = in->frame_cnt;
	fd_handle->frame_cnt          = 0;
	fd_handle->fd_img_size        = in->frame_size;
	fd_handle->face_area_prev.face_count = 0;
	fd_handle->curr_frame_idx     = 0;
	fd_handle->faceattr_arr.count = 0;

	CMR_LOGD("mem_size = 0x%ld", fd_handle->mem_size);
	fd_handle->alloc_addr = malloc(fd_handle->mem_size);
	if (!fd_handle->alloc_addr) {
		CMR_LOGE("mem alloc failed");
		goto free_fd_handle;
	}

	ret = fd_thread_create(fd_handle);
	if (ret) {
		CMR_LOGE("failed to create thread.");
		goto free_fd_handle;
	}

	fd_img_size = &in->frame_size;
	CMR_LOGI("fd_img_size height = %d, width = %d", fd_img_size->height, fd_img_size->width);
	ret = fd_call_init(fd_handle, fd_img_size);
	if (ret) {
		CMR_LOGE("failed to init fd");
		fd_close(fd_handle);
	} else {
		*out_class_handle = (cmr_handle )fd_handle;
	}

	return ret;

free_fd_handle:
	if (fd_handle->alloc_addr) {
		free(fd_handle->alloc_addr);
	}
	free(fd_handle);
	return ret;
}

static cmr_int fd_close(cmr_handle class_handle)
{
	cmr_int              ret         = CMR_CAMERA_SUCCESS;
	struct class_fd      *fd_handle  = (struct class_fd *)class_handle;
	CMR_MSG_INIT(message);

	CHECK_HANDLE_VALID(fd_handle);

	message.msg_type = CMR_EVT_FD_EXIT;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	ret = cmr_thread_msg_send(fd_handle->thread_handle, &message);
	if (ret) {
		CMR_LOGE("send msg fail");
		goto out;
	}

	if (fd_handle->thread_handle) {
		cmr_thread_destroy(fd_handle->thread_handle);
		fd_handle->thread_handle = 0;
		fd_handle->is_inited     = 0;
	}

	if (fd_handle->alloc_addr) {
		free(fd_handle->alloc_addr);
	}

	free(fd_handle);

out:
	return ret;
}

static cmr_int fd_transfer_frame(cmr_handle class_handle,struct ipm_frame_in *in, struct ipm_frame_out *out)
{
	cmr_int                   ret         = CMR_CAMERA_SUCCESS;
	struct class_fd           *fd_handle  = (struct class_fd *)class_handle;
	cmr_uint                  frame_cnt;
	cmr_u32                   is_busy     = 0;
	struct fd_start_parameter param;

	if (!in || !class_handle) {
		CMR_LOGE("Invalid Param!");
		return CMR_CAMERA_INVALID_PARAM;
	}

	frame_cnt   = ++fd_handle->frame_cnt;

	if (frame_cnt < fd_handle->frame_total_num) {
		CMR_LOGD("This is fd 0x%ld frame. need the 0x%ld frame,",frame_cnt, fd_handle->frame_total_num);
		return ret;
	}

	fd_handle->curr_frame_idx++;

	// reduce the frame rate, because the current face detection (tracking mode) is too fast!!
	{
		const static cmr_uint DROP_RATE = 2;
		if ((fd_handle->curr_frame_idx % DROP_RATE) != 0) {
			return ret;
		}
	}

	is_busy = fd_is_busy(fd_handle);
	// CMR_LOGI("fd is_busy =%d", is_busy);

	if (!is_busy) {
		fd_handle->frame_cnt = 0;
		fd_handle->frame_in  = *in;

		param.frame_data    = (void *)in->src_frame.addr_phy.addr_y;
		param.frame_cb      = fd_handle->frame_cb;
		param.caller_handle = in->caller_handle;
		param.private_data  = in->private_data;

		memcpy(fd_handle->alloc_addr, (void *)in->src_frame.addr_vir.addr_y, fd_handle->mem_size);

		ret = fd_start(class_handle,&param);
		if (ret) {
			CMR_LOGE("send msg fail");
			goto out;
		}

		if (fd_handle->frame_cb) {
			if (out != NULL) {
				cmr_bzero(out,sizeof(struct ipm_frame_out));
			}
		} else {
			if (out != NULL) {
				out = &fd_handle->frame_out;
			} else {
				CMR_LOGE("sync err,out parm can't NULL.");
			}
		}
	} else if(!fd_handle->is_get_result) {
				/*!!Warning: The following codes are not thread-safe */
				memcpy(&fd_handle->frame_out.face_area, &fd_handle->face_area_prev, sizeof(struct img_face_area));
				fd_handle->frame_out.dst_frame.size.width = fd_handle->frame_in.src_frame.size.width;
				fd_handle->frame_out.dst_frame.size.height = fd_handle->frame_in.src_frame.size.height;

				/*callback*/
				if (fd_handle->frame_cb) {
				fd_handle->frame_out.private_data = in->private_data;
				fd_handle->frame_out.caller_handle = in->caller_handle;
				fd_handle->frame_cb(IPM_TYPE_FD, &fd_handle->frame_out);
			}
			if (fd_handle->frame_cb) {
				if (out != NULL) {
					cmr_bzero(out,sizeof(struct ipm_frame_out));
				}
			} else {
				if (out != NULL) {
					out = &fd_handle->frame_out;
				} else {
					CMR_LOGE("sync err,out parm can't NULL.");
			}
		}
	}

out:
	return ret;
}

static cmr_int fd_pre_proc(cmr_handle class_handle)
{
	cmr_int              ret = CMR_CAMERA_SUCCESS;

	/*no need to do*/
	(void)class_handle;

	return ret;
}

static cmr_int fd_post_proc(cmr_handle class_handle)
{
	cmr_int              ret = CMR_CAMERA_SUCCESS;

	/*no need to do*/
	(void)class_handle;

	return ret;
}

static cmr_int fd_start(cmr_handle class_handle, struct fd_start_parameter *param)
{
	cmr_int              ret         = CMR_CAMERA_SUCCESS;
	struct class_fd      *fd_handle  = (struct class_fd *)class_handle;
	cmr_u32              is_busy     = 0;
	CMR_MSG_INIT(message);

	if (!class_handle || !param) {
		CMR_LOGE("parameter is NULL. fail");
		return CMR_CAMERA_INVALID_PARAM;
	}

	if (!param->frame_data) {
		CMR_LOGE("frame_data is NULL. fail");
		return CMR_CAMERA_INVALID_PARAM;
	}

	message.data = (void *)malloc(sizeof(struct fd_start_parameter));
	if (NULL == message.data) {
		CMR_LOGE("NO mem, Fail to alloc memory for msg data");
		return CMR_CAMERA_NO_MEM;
	}

	memcpy(message.data, param, sizeof(struct fd_start_parameter));

	message.msg_type = CMR_EVT_FD_START;
	message.alloc_flag = 1;

	if (fd_handle->frame_cb) {
		message.sync_flag = CMR_MSG_SYNC_RECEIVED;
		//message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	} else {
		message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	}

	ret = cmr_thread_msg_send(fd_handle->thread_handle, &message);
	if (ret) {
		CMR_LOGE("send msg fail");
		ret = CMR_CAMERA_FAIL;
	}

	return ret;
}

static cmr_uint check_size_data_invalid(struct img_size *fd_img_size)
{
	cmr_int              ret = -CMR_CAMERA_FAIL;

	if (NULL != fd_img_size) {
		if ((fd_img_size->width) && (fd_img_size->height)){
			ret= CMR_CAMERA_SUCCESS;
		}
	}

	return ret;
}

static cmr_int fd_call_init(struct class_fd *class_handle, const struct img_size *fd_img_size)
{
	cmr_int              ret = CMR_CAMERA_SUCCESS;
	CMR_MSG_INIT(message);

	message.data = malloc(sizeof(struct img_size));
	if (NULL == message.data) {
		CMR_LOGE("NO mem, Fail to alloc memory for msg data");
		ret = CMR_CAMERA_NO_MEM;
		goto out;
	}

	message.alloc_flag = 1;
	memcpy(message.data, fd_img_size, sizeof(struct img_size));

	message.msg_type = CMR_EVT_FD_INIT;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	ret = cmr_thread_msg_send(class_handle->thread_handle, &message);
	if (CMR_CAMERA_SUCCESS != ret) {
		CMR_LOGE("msg send fail");
		ret = CMR_CAMERA_FAIL;
		goto free_all;
	}

	return ret;

free_all:
	free(message.data);
out:
	return ret;
}

static cmr_uint fd_is_busy(struct class_fd *class_handle)
{
	cmr_int              is_busy = 0;

	if (NULL == class_handle) {
		return is_busy;
	}

	is_busy = class_handle->is_busy;

	return is_busy;
}

static void fd_set_busy(struct class_fd *class_handle, cmr_uint is_busy)
{
	if (NULL == class_handle) {
		return;
	}

	class_handle->is_busy = is_busy;
}


static cmr_int fd_thread_create(struct class_fd *class_handle)
{
	cmr_int                 ret = CMR_CAMERA_SUCCESS;
	CMR_MSG_INIT(message);

	CHECK_HANDLE_VALID(class_handle);

	if (!class_handle->is_inited) {
		ret = cmr_thread_create(&class_handle->thread_handle,
					CAMERA_FD_MSG_QUEUE_SIZE,
					fd_thread_proc,
					(void*)class_handle);
		if (ret) {
			CMR_LOGE("send msg failed!");
			ret = CMR_CAMERA_FAIL;
			goto end;
		}

		class_handle->is_inited = 1;
	} else {
		CMR_LOGI("fd is inited already");
	}

end:
	return ret;
}

static void fd_recognize_face_attribute(FD_DETECTOR_HANDLE hDT,
                    FA_ALIGN_HANDLE hFaceAlign,
                    FAR_RECOGNIZER_HANDLE hFAR,
                    struct class_faceattr_array *io_faceattr_arr,
                    const cmr_u8 *i_image_data,
                    struct img_size i_image_size,
                    const cmr_uint i_curr_frame_idx)
{
	cmr_int face_count = 0;
	cmr_int fd_idx = 0;
	cmr_int i = 0;
	struct class_faceattr_array new_attr_array = {0};
	FA_IMAGE img = {0};

	/* Don't update face attribute, if the frame interval is not enough. For reducing computation cost */
	if ( (i_curr_frame_idx - io_faceattr_arr->frame_idx) < FD_RUN_FAR_INTERVAL ) {
		return;
	}

	img.data = (unsigned char *)i_image_data;
	img.width = i_image_size.width;
	img.height = i_image_size.height;
	img.step = img.width;

	face_count = FdGetFaceCount(hDT);

	for (fd_idx = 0; fd_idx < face_count; fd_idx++) {
		struct class_faceattr *fattr = &(new_attr_array.face[fd_idx]);
		FD_FACEINFO info;
		FdGetFaceInfo(hDT, fd_idx, &info);

		/* Assign the same face id with FD */
		fattr->face_id = info.id;
		fattr->attr.smile = 0;
		fattr->attr.eyeClose = 0;

		/* Run face alignment */
		{
			FA_FACEINFO faface;
			faface.x = info.x;
			faface.y = info.y;
			faface.width = info.width;
			faface.height = info.height;
			faface.yawAngle = info.yawAngle;
			faface.rollAngle = info.rollAngle;
			FaFaceAlign(hFaceAlign, &img, &faface, &(fattr->shape));
		}

		/* Run face attribute recognition */
		{
			FAR_OPTION opt;
			FAR_FACEINFO farface;

			/* set option: only do smile detection */
			opt.smileOn = 1;
			opt.eyeOn = 0;
			opt.infantOn = 0;
			opt.genderOn = 0;

			/* Set the eye locations */
			for (i = 0; i < 7; i++) {
				farface.landmarks[i].x = fattr->shape.data[i*2];
				farface.landmarks[i].y = fattr->shape.data[i*2+1];
			}

			{
				int err = FarRecognize(hFAR, (const FAR_IMAGE *)&img, &farface, &opt, &(fattr->attr));
				// CMR_LOGI("FarRecognize: err=%d, smile=%d", err, fattr->attr.smile);
			}
		}
	}

	new_attr_array.count = face_count;
	new_attr_array.frame_idx = i_curr_frame_idx;
	memcpy(io_faceattr_arr, &new_attr_array, sizeof(struct class_faceattr_array));
}


static cmr_int fd_get_face_overlap(const struct face_finder_data *i_face1,
                                   const struct face_finder_data *i_face2)
{
	cmr_int percent = 0;

	/* get the overlapped region */
	cmr_int sx = MAX(i_face1->sx, i_face2->sx);
	cmr_int ex = MIN(i_face1->ex, i_face2->ex);
	cmr_int sy = MAX(i_face1->sy, i_face2->sy);
	cmr_int ey = MIN(i_face1->ey, i_face2->ey);

	if (ex >= sx && ey >= sy) {
		cmr_int overlap_area = (ex - sx + 1) * (ey - sy + 1);
		cmr_int area1 = (i_face1->ex - i_face1->sx + 1) * (i_face1->ey - i_face1->sy + 1);
		cmr_int area2 = (i_face2->ex - i_face2->sx + 1) * (i_face2->ey - i_face2->sy + 1);
		percent = (100 * overlap_area *2) / (area1 + area2);
	}

	return percent;
}


static void fd_smooth_face_rect(const struct img_face_area *i_face_area_prev,
                                struct face_finder_data *io_curr_face)
{
	const cmr_int overlap_thr = 85;
	cmr_uint prevIdx = 0;

	for (prevIdx = 0; prevIdx < i_face_area_prev->face_count; prevIdx++) {
		const struct face_finder_data *prev_face = &(i_face_area_prev->range[prevIdx]);
		{
			cmr_int overlap_percent = fd_get_face_overlap(prev_face, io_curr_face);
			if (overlap_percent >= overlap_thr) {
				io_curr_face->sx = prev_face->sx;
				io_curr_face->sy = prev_face->sy;
				io_curr_face->srx = prev_face->srx;
				io_curr_face->sry = prev_face->sry;
				io_curr_face->elx = prev_face->elx;
				io_curr_face->ely = prev_face->ely;
				io_curr_face->ex = prev_face->ex;
				io_curr_face->ey = prev_face->ey;
				break;
			}
		}
	}
}

static void fd_get_fd_results(FD_DETECTOR_HANDLE hDT,
                              const struct class_faceattr_array *i_faceattr_arr,
															const struct img_face_area *i_face_area_prev,
                              struct img_face_area *o_face_area,
                              struct img_size image_size)
{
	cmr_int               face_num = 0;
	FD_FACEINFO           info;
	cmr_int               face_idx = 0;
	cmr_int               ret = FD_OK;
	cmr_int sx = 0, ex = 0, sy = 0, ey = 0;
	cmr_int valid_count = 0;
	struct face_finder_data *face_ptr = NULL;

	face_num = FdGetFaceCount(hDT);
	for (face_idx = 0; face_idx < face_num; face_idx++) {
		/* Gets the detection result for each face */
		ret = FdGetFaceInfo(hDT, face_idx, &info);
		if (ret != FD_OK) {
			CMR_LOGW("FdGetFaceInfo(%d) Error : %d", face_idx, ret);
			continue;
		}

		sx = info.x;
		sy = info.y;
		ex = info.x + info.width - 1;
		ey = info.y + info.height - 1;

		/* enlarge face size a little.
		!TODO: maybe it is also necessary to adjust the face center
		*/
		{
			cmr_int delta = info.width / 20;
			sx -= delta;
			sy -= delta;
			ex += delta;
			ey += delta;
		}

		/* Ensure the face coordinates are in image region */
		if (sx < 0) sx = 0;
		if (sy < 0) sy = 0;
		if (ex >= (cmr_int)image_size.width)  ex = image_size.width - 1;
		if (ey >= (cmr_int)image_size.height) ey = image_size.height - 1;

		face_ptr = &(o_face_area->range[valid_count]);
		valid_count++;

		face_ptr->sx = sx;
		face_ptr->sy = sy;
		face_ptr->srx = ex;
		face_ptr->sry = sy;
		face_ptr->elx = sx;
		face_ptr->ely = ey;
		face_ptr->ex = ex;
		face_ptr->ey = ey;
		face_ptr->face_id = info.id;
		face_ptr->pose = info.yawAngle;
		face_ptr->angle = info.rollAngle;
		face_ptr->score = info.score / 10; /* Make it in [0,100]. HAL1.0 requires so */
		face_ptr->smile_level = 1;
		face_ptr->blink_level = 0;
		face_ptr->brightness = 128;

		fd_smooth_face_rect(i_face_area_prev, face_ptr);

		/* set smile detection result */
		{
			const cmr_int app_smile_thr = 30;  // smile threshold in APP
			const cmr_int algo_smile_thr = 10; // smile threshold by algorithm; it is a tuning parameter, must be in [1, 50]
			cmr_int i = 0;
			for (i = 0; i < i_faceattr_arr->count; i++) {
				const struct class_faceattr *fattr = &(i_faceattr_arr->face[i]);
				if (fattr->face_id == info.id) {
					/* Note: The original smile score is in [-100, 100].
					   But the Camera APP needs a score in [0, 100], and also the definitions for smile degree are different
					   with the algorithm. So, we need to adjust the smile score to fit the APP.
					*/
					cmr_int smile_score = MAX(0, fattr->attr.smile);
					if (smile_score >= algo_smile_thr)
					{
						/* norm_score is in [0, 70] */
						cmr_int norm_score = ((smile_score - algo_smile_thr) * (100 - app_smile_thr)) / (100 - algo_smile_thr);
						/* scale the smile score to be in [30, 100] */
						smile_score = norm_score + app_smile_thr;
					}
					else
					{
						/* scale the smile score to be in [0, 30) */
						smile_score = (smile_score * app_smile_thr) / algo_smile_thr;
					}

					face_ptr->smile_level = MAX(1, smile_score);
					face_ptr->blink_level = MAX(0, fattr->attr.eyeClose);
					break;
				}
			}
		}
	}

	o_face_area->face_count = valid_count;
}

static cmr_int fd_create_detector(FD_DETECTOR_HANDLE *hDT,
                                  const struct img_size *fd_img_size)
{
	FD_OPTION opt;

	FdInitOption(&opt);
	CMR_LOGI("SPRD FD version: %s .",FdGetVersion());
	opt.workMode        = FD_WORKMODE_MOVIE;
	opt.maxFaceNum      = FACE_DETECT_NUM;
	opt.minFaceSize     = MIN(fd_img_size->width, fd_img_size->height) / 10;
	opt.directions      = FD_DIRECTION_ALL;
	opt.angleFrontal    = FD_ANGLE_RANGE_90;
	opt.angleHalfProfile= FD_ANGLE_RANGE_30;
	opt.angleFullProfile= FD_ANGLE_NONE;
	opt.detectDensity   = 5;
	opt.scoreThreshold  = 0;
	opt.initFrames      = 2;
	opt.detectFrames    = 1;
	opt.detectInterval  = 3;
	opt.trackDensity    = 5;
	opt.lostRetryCount  = 4;
	opt.lostHoldCount   = 3;
	opt.holdPositionRate = 10;
	opt.holdSizeRate    = 6;
	opt.swapFaceRate    = 200;
	opt.guessFaceDirection = 1;

  /* For tuning FD parameter: read parameter from file */
	/*
	{
		FILE *fp = fopen("/data/sprd_fd_param.txt", "rt");
		if (!fp) {
			CMR_LOGI("failed to open /data/sprd_fd_param.txt");
		} else {
			CMR_LOGI("read /data/sprd_fd_param.txt");
		}

		if (fp) {
			unsigned int v[17];
			int i = 0;
			for (i = 0; i < 17; i++){
				fscanf(fp, "%d ", &(v[i]));
				CMR_LOGI("sprd_fd_param: v[%d]=%d", i, v[i]);
			}
			fclose(fp);

			opt.minFaceSize     = MIN(fd_img_size->width, fd_img_size->height) / v[0];
			opt.directions      = v[1];
			opt.angleFrontal    = v[2];
			opt.angleHalfProfile= v[3];
			opt.angleFullProfile= v[4];
			opt.detectDensity   = v[5];
			opt.scoreThreshold  = v[6];
			opt.initFrames      = v[7];
			opt.detectFrames    = v[8];
			opt.detectInterval  = v[9];
			opt.trackDensity    = v[10];
			opt.lostRetryCount  = v[11];
			opt.lostHoldCount   = v[12];
			opt.holdPositionRate = v[13];
			opt.holdSizeRate    = v[14];
			opt.swapFaceRate    = v[15];
			opt.guessFaceDirection = v[16];
		}
	}
	*/
	return FdCreateDetector(hDT, &opt);
}

static cmr_int fd_thread_proc(struct cmr_msg *message, void *private_data)
{
	cmr_int                   ret           = CMR_CAMERA_SUCCESS;
	struct class_fd           *class_handle = (struct class_fd *)private_data;
	cmr_int                   evt           = 0;
	struct fd_start_parameter *start_param = NULL;
	struct img_size *fd_img_size = NULL;
	FD_IMAGE fd_img = {0};
	clock_t start_time, end_time;
	int duration;

	if (!message || !class_handle) {
		CMR_LOGE("parameter is fail");
		return CMR_CAMERA_INVALID_PARAM;
	}

	evt = (cmr_u32)(message->msg_type & CMR__EVT_FD_MASK_BITS);

	switch (evt) {
	case CMR_EVT_FD_INIT:
		/* Create face alignment and face attribute recognition handle */
		if (FA_OK != FaCreateAlignHandle(&(class_handle->hFaceAlign))) {
			CMR_LOGE("FaCreateAlignHandle() Error");
			break;
		}
		if (FAR_OK != FarCreateRecognizerHandle(&(class_handle->hFAR))) {
			CMR_LOGE("FarCreateRecognizerHandle() Error");
			break;
		}

		/* Creates Face Detection handle */
		fd_img_size = (struct img_size *)message->data;
		ret = fd_create_detector(&(class_handle->hDT), fd_img_size);
		if (ret != FD_OK) {
			CMR_LOGE("fd_create_detector() Error");
			break;
		}

		break;

	case CMR_EVT_FD_START:
		start_param = (struct fd_start_parameter *)message->data;

		if (NULL == start_param) {
			CMR_LOGE("parameter fail");
			break;
		}

		fd_set_busy(class_handle, 1);

		/* Executes Face Detection */
		fd_img.data = (unsigned char *)class_handle->alloc_addr;
		fd_img.width = class_handle->fd_img_size.width;
		fd_img.height = class_handle->fd_img_size.height;
		fd_img.step = fd_img.width;

		start_time = clock();
		ret = FdDetectFace(class_handle->hDT, &fd_img);
		end_time = clock();

		if (ret != FD_OK) {
			CMR_LOGE("FdDetectFace() Error : %ld", ret);
			fd_set_busy(class_handle, 0);
			break;
		}

		/* recognize face attribute (smile detection) */
		fd_recognize_face_attribute(class_handle->hDT,
                 class_handle->hFaceAlign,
                 class_handle->hFAR,
                 &(class_handle->faceattr_arr),
                 (cmr_u8*)class_handle->alloc_addr,
                 class_handle->fd_img_size,
                 class_handle->curr_frame_idx);

		class_handle->is_get_result = 1;
		/* extract face detection results */
		fd_get_fd_results(class_handle->hDT, &(class_handle->faceattr_arr), &(class_handle->face_area_prev),
		                  &(class_handle->frame_out.face_area), class_handle->fd_img_size);
		/* save a copy for next frame */
		memcpy(&(class_handle->face_area_prev), &(class_handle->frame_out.face_area), sizeof(struct img_face_area));

		class_handle->frame_out.dst_frame.size.width = class_handle->frame_in.src_frame.size.width;
		class_handle->frame_out.dst_frame.size.height = class_handle->frame_in.src_frame.size.height;

		duration = (end_time - start_time) * 1000 / CLOCKS_PER_SEC;
		CMR_LOGI("SPRD_FD: frame(%dx%d), face_num=%d, time=%d ms", class_handle->frame_in.src_frame.size.width,
				class_handle->frame_in.src_frame.size.height, class_handle->frame_out.face_area.face_count, duration);
		/*callback*/
		if (class_handle->frame_cb) {
			class_handle->frame_out.private_data  = start_param->private_data;
			class_handle->frame_out.caller_handle = start_param->caller_handle;
			class_handle->frame_cb(IPM_TYPE_FD, &class_handle->frame_out);
		}

		fd_set_busy(class_handle, 0);
		class_handle->is_get_result = 0;
		break;

	case CMR_EVT_FD_EXIT:
		/* Deletes Face Detection handle */
		FaDeleteAlignHandle(&(class_handle->hFaceAlign));
		FarDeleteRecognizerHandle(&(class_handle->hFAR));
		FdDeleteDetector(&(class_handle->hDT));
		break;

	default:
		break;
	}

	return ret;
}

#endif
