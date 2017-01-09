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

#if defined (CONFIG_CAMERA_FACE_DETECT) && defined (CONFIG_CAMERA_FACE_DETECT_OMRON)

#define LOG_TAG "cmr_fd_omron"

#include "cmr_msg.h"
#include "cmr_ipm.h"
#include "cmr_common.h"
#include "SprdOEMCamera.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "UdnAPI.h"
#include "CommonDef.h"
#include "UdnDtAPI.h"

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
	struct img_size                 fd_size;
	struct img_face_area            face_area_prev; /* The faces detected from the previous frame; It is used to make face detection results more stable */
	cmr_uint                        curr_frame_idx;
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
static cmr_uint check_size_data_invalid(struct img_size *fd_size);
static cmr_int fd_call_init(struct class_fd *class_handle, const struct img_size *fd_size);
static cmr_uint fd_is_busy(struct class_fd *class_handle);
static void fd_set_busy(struct class_fd *class_handle, cmr_uint is_busy);
static cmr_int fd_thread_create(struct class_fd *class_handle);
static cmr_int fd_thread_proc(struct cmr_msg *message, void *private_data);
static HDETECTION hDT = NULL;          /* Face Detection Handle */
static HDTRESULT hDtResult = NULL;     /* Face Detection Result Handle */

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

#define CMR_FD_LIMIT_SIZE         (320 * 240)
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

/* Set Face Detection parameters */
static cmr_int set_dt_param(HDETECTION hDT)
{
	int32_t   nRet = UDN_NORMAL;
	uint32_t  anAngle[POSE_TYPE_COUNT];
	int32_t   nMotionAngleExtension;

	/* Sets the Face Detection Mode */
	nRet = UDN_SetDtMode(hDT, DT_MODE_MOTION1);
	if (nRet != UDN_NORMAL) {
		CMR_LOGE("UDN_SetDtMode() Error : %d", nRet);
		return nRet;
	}

	/* Sets the Minimum and Maximum face sizes to be detected */
	nRet = UDN_SetDtFaceSizeRange(hDT,40, 8192);
	if (nRet != UDN_NORMAL) {
		CMR_LOGE("UDN_SetDtFaceSizeRange() Error : %d", nRet);
		return nRet;
	}

	/* Sets the Angle settings for face detection */
	anAngle[POSE_FRONT] = ANGLE_ALL;  /* old: ANGLE_ULR15 | ANGLE_6; */
	anAngle[POSE_HALF_PROFILE] =ANGLE_ULR15 | ANGLE_6;
	anAngle[POSE_PROFILE] = ANGLE_NONE;
	//nMotionAngleExtension = ANGLE_ROTATION_EXT1 | ANGLE_POSE_EXT1 | DETECT_HEAD_USE;
	nMotionAngleExtension = ANGLE_ROTATION_EXT1 | ANGLE_POSE_EXT0 | DETECT_HEAD_NOUSE;

	nRet = UDN_SetDtAngle(hDT, anAngle, nMotionAngleExtension);
	if (nRet != UDN_NORMAL) {
		CMR_LOGE("UDN_SetDtAngle() Error : %d", nRet);
		return nRet;
	}

	/* Sets the Face Detection Threshold */
	nRet = UDN_SetDtThreshold(hDT, 600, 600);
	if (nRet != UDN_NORMAL) {
		CMR_LOGE("UDN_SetDtThreshold() Error : %d", nRet);
		return nRet;
	}

	/* Sets the search density coefficient for face detection */
	nRet = UDN_SetDtStep(hDT, 33, 24);
	if (nRet != UDN_NORMAL) {
		CMR_LOGE("UDN_SetDtStep() Error : %d", nRet);
		return nRet;
	}

	/* Sets Motion Face Detection Refresh Count for each Motion mode */
	nRet = UDN_SetDtRefreshCount(hDT, DT_MODE_MOTION1, 5);
	if (nRet != UDN_NORMAL) {
		CMR_LOGE("UDN_SetDtRefreshCount() Error : %d", nRet);
		return nRet;
	}
	/* Sets Motion Face Detection Retry Count, Motion Head Detection Retry Count and Hold Count at lost time */
	nRet = UDN_SetDtLostParam(hDT, 0, 0, 0);
	if (nRet != UDN_NORMAL) {
		CMR_LOGE("UDN_SetDtLostParam() Error : %d", nRet);
		return nRet;
	}
	/* Set  DtModifyMoveRate*/
	nRet = UDN_SetDtModifyMoveRate(hDT, 10);
	if (nRet != UDN_NORMAL) {
		CMR_LOGE("UDN_SetDtModifyMoveRate() Error : %d", nRet);
		return nRet;
	}

	return nRet;
}

static cmr_int fd_open(cmr_handle ipm_handle, struct ipm_open_in *in, struct ipm_open_out *out,
				cmr_handle *out_class_handle)
{
	cmr_int              ret        = CMR_CAMERA_SUCCESS;
	struct class_fd      *fd_handle = NULL;
	struct img_size      *fd_size;

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
	fd_handle->fd_size            = in->frame_size;
	fd_handle->face_area_prev.face_count = 0;
	fd_handle->curr_frame_idx     = 0;

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

	fd_size = &in->frame_size;
	CMR_LOGI("fd_size height = %d, width = %d", fd_size->height, fd_size->width);
	ret = fd_call_init(fd_handle, fd_size);
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
	cmr_u32                    is_busy    = 0;
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

	// reduce the frame rate, because the current face detection (tracking mode) is too fast!!
	{
		const static cmr_uint DROP_RATE = 2;
		fd_handle->curr_frame_idx++;
		if ((fd_handle->curr_frame_idx % DROP_RATE) != 0) {
			return ret;
		}
	}

	is_busy = fd_is_busy(fd_handle);
	CMR_LOGI("fd is_busy =%d", is_busy);

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

static cmr_uint check_size_data_invalid(struct img_size *fd_size)
{
	cmr_int              ret = -CMR_CAMERA_FAIL;

	if (NULL != fd_size) {
		if ((fd_size->width) && (fd_size->height)){
			ret= CMR_CAMERA_SUCCESS;
		}
	}

	return ret;
}

static cmr_int fd_call_init(struct class_fd *class_handle, const struct img_size *fd_size)
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
	memcpy(message.data, fd_size, sizeof(struct img_size));

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


static cmr_int fd_get_face_overlap(const struct face_finder_data *i_face1,
										const struct face_finder_data *i_face2)
{
	cmr_int percent = 0;

	/* get the bounding box of the two faces */
	cmr_int f1_sx = MIN(MIN(i_face1->sx, i_face1->ex), MIN(i_face1->srx, i_face1->elx));
	cmr_int f1_ex = MAX(MAX(i_face1->sx, i_face1->ex), MAX(i_face1->srx, i_face1->elx));
	cmr_int f1_sy = MIN(MIN(i_face1->sy, i_face1->ey), MIN(i_face1->sry, i_face1->ely));
	cmr_int f1_ey = MAX(MAX(i_face1->sy, i_face1->ey), MAX(i_face1->sry, i_face1->ely));
	cmr_int f2_sx = MIN(MIN(i_face2->sx, i_face2->ex), MIN(i_face2->srx, i_face2->elx));
	cmr_int f2_ex = MAX(MAX(i_face2->sx, i_face2->ex), MAX(i_face2->srx, i_face2->elx));
	cmr_int f2_sy = MIN(MIN(i_face2->sy, i_face2->ey), MIN(i_face2->sry, i_face2->ely));
	cmr_int f2_ey = MAX(MAX(i_face2->sy, i_face2->ey), MAX(i_face2->sry, i_face2->ely));

	/* get the overlapped region */
	cmr_int sx = MAX(f1_sx, f2_sx);
	cmr_int ex = MIN(f1_ex, f2_ex);
	cmr_int sy = MAX(f1_sy, f2_sy);
	cmr_int ey = MIN(f1_ey, f2_ey);

	if (ex >= sx && ey >= sy) {
		cmr_int overlap_area = (ex - sx + 1) * (ey - sy + 1);
		cmr_int area1 = (f1_ex - f1_sx + 1) * (f1_ey - f1_sy + 1);
		cmr_int area2 = (f2_ex - f2_sx + 1) * (f2_ey - f2_sy + 1);
		percent = (100 * overlap_area *2) / (area1 + area2);
	}

	return percent;
}

static void fd_smooth_fd_results(const struct img_face_area *i_face_area_prev,
									const struct img_face_area *i_face_area_curr,
									struct img_face_area *o_face_area)
{
	const cmr_int overlap_thr = 90;
	cmr_uint prevIdx = 0;
	cmr_uint currIdx = 0;
	cmr_int trueCount = 0;

	for (currIdx = 0; currIdx < i_face_area_curr->face_count; currIdx++) {
		cmr_int overlap_percent = 0;
		cmr_int overlap_prevIdx = -1;
		for (prevIdx = 0; prevIdx < i_face_area_prev->face_count; prevIdx++) {
			overlap_percent = fd_get_face_overlap(&(i_face_area_curr->range[currIdx]),
												  &(i_face_area_prev->range[prevIdx]));
			if (overlap_percent >= overlap_thr) {
				overlap_prevIdx = prevIdx;
				break;
			}
		}

		/* when two faces have large overlaps, they are considered as "true" faces */
		if (overlap_prevIdx >= 0) {
			const struct face_finder_data *f1 = &(i_face_area_curr->range[currIdx]);
			const struct face_finder_data *f2 = &(i_face_area_prev->range[overlap_prevIdx]);
			/* output the average of the two faces */
			struct face_finder_data *outf = &(o_face_area->range[trueCount]);
			outf->face_id = trueCount;
			outf->sx = f2->sx;
			outf->sy = f2->sy;
			outf->srx = f2->srx;
			outf->sry = f2->sry;
			outf->ex = f2->ex;
			outf->ey = f2->ey;
			outf->elx = f2->elx;
			outf->ely = f2->ely;
			outf->brightness = f1->brightness;
			outf->angle = f1->angle;
			outf->smile_level = f1->smile_level;
			outf->blink_level = f1->blink_level;

			memcpy(f1,outf,sizeof(struct face_finder_data));
			trueCount++;
		}
	}

	o_face_area->face_count = trueCount;
}

static void fd_get_fd_results(HDTRESULT i_hDtResult,
								struct img_face_area *io_face_area_prev,
								struct img_face_area *o_face_area)
{
	cmr_int               valid_count = 0;
	int32_t               face_num = 0;
	int32_t               nIndex = 0;
	int32_t               ret = UDN_NORMAL;
	FACEINFO              info;	/* Detection Result */
	struct img_face_area  curr_face_area;
	const uint8_t         is_do_smooth = 1;

	/* Gets the number of detected face */
	ret = UDN_GetDtFaceCount(i_hDtResult, (INT32*)&face_num);
	if (ret != UDN_NORMAL) {
		CMR_LOGW("UDN_GetDtFaceCount() Error : %d", ret);
		face_num = 0;
	}
	CMR_LOGI("UDN_GetDtFaceCount() OK : nFaceCount = %d", face_num);

	for (nIndex = 0; nIndex < face_num; nIndex++) {   /*** Face Loop ***/
		/* Gets the detection result for each face */
		ret = UDN_GetDtFaceInfo(i_hDtResult, nIndex, &info);
		if (ret != UDN_NORMAL) {
			CMR_LOGW("UDN_GetDtFaceInfo(%d) Error : %d", nIndex, ret);
			continue;
		}

		/*
		CMR_LOGI("<NO.%d>  (Confidence=%d, [%d,%d][%d,%d][%d,%d][%d,%d], ID=%d)",
			nIndex, info.nConfidence,
			info.ptLeftTop.x, info.ptLeftTop.y,
			info.ptRightTop.x, info.ptRightTop.y,
			info.ptLeftBottom.x, info.ptLeftBottom.y,
			info.ptRightBottom.x, info.ptRightBottom.y,
			info.nID);

		if ((info.ptLeftTop.x < 0 || info.ptLeftTop.y < 0) ||
			(info.ptRightTop.x < 0 || info.ptRightTop.y < 0) ||
			(info.ptLeftBottom.x < 0 || info.ptLeftBottom.y < 0) ||
			(info.ptRightBottom.x < 0 || info.ptRightBottom.y < 0)) {
				continue;
		}
		*/

		/* convert rotated rectangle to upright rectangle, because HAL1.0 only support the upright one */
		{
			cmr_int cx = (info.ptLeftTop.x + info.ptRightBottom.x) / 2;
			cmr_int cy = (info.ptLeftTop.y + info.ptRightBottom.y) / 2;
			cmr_int diff_x = info.ptRightTop.x - info.ptLeftTop.x;
			cmr_int diff_y = info.ptRightTop.y - info.ptLeftTop.y;
			cmr_int face_size = sqrt(diff_x*diff_x + diff_y*diff_y);
			cmr_int half_face_size = face_size / 2;
			cmr_int sx = cx - half_face_size;
			cmr_int ex = cx + half_face_size;
			cmr_int sy = cy - half_face_size;
			cmr_int ey = cy + half_face_size;

			/*
			CMR_LOGI("<NO.%d>  (Confidence=%d, [%d,%d,%d,%d], size=%d, ID=%d)",
			nIndex, info.nConfidence, sx, sy, ex, ey, face_size, info.nID);
			*/

			curr_face_area.range[valid_count].sx = sx;
			curr_face_area.range[valid_count].sy = sy;
			curr_face_area.range[valid_count].srx = ex;
			curr_face_area.range[valid_count].sry = sy;
			curr_face_area.range[valid_count].elx = sx;
			curr_face_area.range[valid_count].ely = ey;
			curr_face_area.range[valid_count].ex = ex;
			curr_face_area.range[valid_count].ey = ey;
		}

		curr_face_area.range[valid_count].face_id = valid_count;
		curr_face_area.range[valid_count].smile_level = info.nConfidence / 10; /* Make it in [0,100] */
		curr_face_area.range[valid_count].blink_level = 0;
		curr_face_area.range[valid_count].angle = info.nPose;
		curr_face_area.range[valid_count].brightness = 128;

		valid_count++;

	}

	curr_face_area.face_count = valid_count;

	if (is_do_smooth) {
		fd_smooth_fd_results(io_face_area_prev, &curr_face_area, o_face_area);
	} else {
		memcpy(o_face_area, &curr_face_area, sizeof(struct img_face_area));
	}
	memcpy(io_face_area_prev, &curr_face_area, sizeof(struct img_face_area));

}

static cmr_int fd_thread_proc(struct cmr_msg *message, void *private_data)
{
	cmr_int                   ret           = CMR_CAMERA_SUCCESS;
	struct class_fd           *class_handle = (struct class_fd *)private_data;
	cmr_int                   evt           = 0;
	cmr_uint                  mem_size      = 0;
	void                      *addr         = 0;
	cmr_int                   facesolid_ret = 0;
	cmr_int                   nIndex;  /* Index of face detection */
	FACEINFO                  info;  /* Detection Result */
	cmr_int                   face_num = 0;
	struct fd_start_parameter *start_param;
	uint8_t                   pbyMajor, pbyMinor, pbyMajor1, pbyMinor1;

	if (!message || !class_handle) {
		CMR_LOGE("parameter is fail");
		return CMR_CAMERA_INVALID_PARAM;
	}

	evt = (cmr_u32)(message->msg_type & CMR__EVT_FD_MASK_BITS);

	switch (evt) {
	case CMR_EVT_FD_INIT:
		/* Creates Face Detection handle */
		hDT = UDN_CreateDetection();
		if ( hDT == NULL ) {
			CMR_LOGE("UDN_CreateDetection() Error");
			break;
		}
		/* Creates Face Detection result handle */
		hDtResult = UDN_CreateDtResult(10, 10);
		if ( hDtResult == NULL ) {
			CMR_LOGE("UDN_CreateDtResult() Error");
			break;
		}

		/* Sets Face Detection parameters */
		if (set_dt_param(hDT) != TRUE) {
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

		/* Make the detection speed independent of image size */
		{
			int minFaceSize = MIN(class_handle->fd_size.width, class_handle->fd_size.height) / 10;
			minFaceSize = MAX(minFaceSize, 40);
			UDN_SetDtFaceSizeRange(hDT, minFaceSize, 8192);
		}

		/* Executes Face Detection */
		ret = UDN_Detection(hDT, (cmr_u8*)class_handle->alloc_addr, class_handle->fd_size.width, class_handle->fd_size.height, ACCURACY_HIGH_TR, hDtResult);
		if (ret != UDN_NORMAL) {
			CMR_LOGE("UDN_Detection() Error : %ld", ret);
			fd_set_busy(class_handle, 0);
			break;
		}

		/* extract face detection results */
		fd_get_fd_results(hDtResult, &(class_handle->face_area_prev), &(class_handle->frame_out.face_area));

		class_handle->frame_out.dst_frame.size.width = class_handle->frame_in.src_frame.size.width;
		class_handle->frame_out.dst_frame.size.height = class_handle->frame_in.src_frame.size.height;

		/*callback*/
		if (class_handle->frame_cb) {
			class_handle->frame_out.private_data  = start_param->private_data;
			class_handle->frame_out.caller_handle = start_param->caller_handle;
			class_handle->frame_cb(IPM_TYPE_FD, &class_handle->frame_out);
		}

		fd_set_busy(class_handle, 0);
		break;

	case CMR_EVT_FD_EXIT:
		/* Deletes Face Detection handle */
		if ( hDT != NULL ) {
			UDN_DeleteDetection(hDT);
			hDT = NULL;
		}
		/* Deletes Face Detection result handle */
		if ( hDtResult != NULL ) {
			UDN_DeleteDtResult(hDtResult);
			hDtResult = NULL;
		}
		break;

	default:
		break;
	}

	return ret;
}
#endif
