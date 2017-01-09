/*
 * Copyright (C) 2014 The Android Open Source Project
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
#define LOG_TAG "cmr_focus"

#include "cmr_msg.h"
#include "cmr_oem.h"
#include "cmr_focus.h"

#define CMR_EVT_AF_START                             (CMR_EVT_OEM_BASE + 10)
#define CMR_EVT_AF_EXIT                              (CMR_EVT_OEM_BASE + 11)
#define CMR_EVT_AF_INIT                              (CMR_EVT_OEM_BASE + 12)
#define CMR_EVT_CAF_MOVE_START                       (CMR_EVT_OEM_BASE + 13)
#define CMR_EVT_CAF_MOVE_STOP                        (CMR_EVT_OEM_BASE + 14)
#define CMR_EVT_AF_STOP                              (CMR_EVT_OEM_BASE + 15)

#define CMR_AF_MSG_QUEUE_SIZE                        (5)
#define ISP_PROCESS_SEC_TIMEOUT                      (1)
#define ISP_PROCESS_NSEC_TIMEOUT                     (800000000)
#define CAMERA_FOCUS_RECT_PARAM_LEN                  (200)

struct af_focus_rect {
	cmr_u32                  zone_cnt;
	struct img_rect          zone[FOCUS_ZONE_CNT_MAX];
	struct img_rect          sensor_trim_rect;
} ;

struct img_rect const_sensor_rect = {
	.start_x = 716,
	.start_y = 476,
	.width   = 206,
	.height  = 272,
};
struct af_context {
	cmr_handle               oem_handle;
	cmr_handle               thread_handle;
	af_cb_func_ptr           evt_cb;
	struct af_md_ops         ops;
	pthread_mutex_t          set_af_cancel_mutex;
	pthread_mutex_t          af_sensor_caf_mutex;
	pthread_mutex_t          af_isp_caf_mutex;
	sem_t                    isp_af_sem;
	cmr_u32                  af_cancelled;
	cmr_u32                  caf_move_done;
	cmr_u32                  isp_af_timeout;
	cmr_u32                  isp_af_win_val;
	cmr_u32                  af_busy;
	cmr_u32                  camera_id;
	cmr_u32                  af_mode;
	cmr_u32                  focus_zone_param[CMR_FOCUS_RECT_PARAM_LEN];
	cmr_u32                  focus_need_quit;
	cmr_u32                  af_mode_inflight;
};

struct af_isp_mode_pairs {
	cmr_u32 af_mode;
	cmr_u32 isp_af_mode;
};

/*it must be matching isp focus mode */
struct af_isp_mode_pairs af_isp_focus_mode [CAMERA_FOCUS_MODE_MAX]={
	{CAMERA_FOCUS_MODE_AUTO      , ISP_FOCUS_TRIG      },
	{CAMERA_FOCUS_MODE_AUTO_MULTI, ISP_FOCUS_MULTI_ZONE},
	{CAMERA_FOCUS_MODE_MACRO     , ISP_FOCUS_MACRO     },
	{CAMERA_FOCUS_MODE_INFINITY  , ISP_FOCUS_NONE      },/*need to check */
	{CAMERA_FOCUS_MODE_CAF       , ISP_FOCUS_CONTINUE  },
	{CAMERA_FOCUS_MODE_CAF_VIDEO , ISP_FOCUS_VIDEO      },/*need to check */
};

#define CMR_CHECK_AF_HANDLE \
	do { \
		if (!af_cxt) { \
			CMR_LOGE("Invalid af handle"); \
			return -1; \
		} \
	} while(0)



/*local function declaration start */
static cmr_int af_set_focusmove_flag(cmr_handle af_handle, cmr_u32 is_done);
static cmr_int af_get_focusmove_flag(cmr_handle af_handle);
static cmr_int af_isp_done(cmr_handle af_handle, void *data);
static cmr_int af_set_mode(cmr_handle af_handle, cmr_u32 came_id, cmr_u32 af_mode);
static cmr_int af_init(void);
static cmr_int af_start(cmr_handle af_handle, cmr_u32 camera_id);
static cmr_int af_quit(cmr_handle af_handle, cmr_u32 camera_id);
static cmr_int af_stop(cmr_handle af_handle, cmr_u32 camera_id, cmr_u32 is_need_abort_msg);
static cmr_int af_start_lightly(cmr_handle af_handle, cmr_u32 camera_id);
static cmr_int af_clear_exit_flag(cmr_handle af_handle);
static cmr_int af_need_exit(cmr_handle af_handle, cmr_u32 *is_need_abort_msg);
static cmr_int af_mode_to_isp(cmr_u32 af_mode, cmr_u32 *isp_af_mode);
static cmr_int af_check_area(cmr_handle af_handle,
				struct img_rect *sensor_rect_ptr,
				struct img_rect *rect_ptr,
				cmr_u32 rect_num);
static cmr_int caf_move_start_handle(cmr_handle af_handle);
static cmr_int caf_move_stop_handle(cmr_handle af_handle);
static cmr_int focus_rect_parse(cmr_handle af_handle, SENSOR_EXT_FUN_PARAM_T_PTR p_focus_rect);
static cmr_int focus_rect_param_to_isp(SENSOR_EXT_FUN_PARAM_T focus_rect, struct isp_af_win *p_isp_af_param);
static cmr_int wait_isp_focus_result(cmr_handle af_handle, cmr_u32 camera_id, cmr_u32 is_need_abort_msg);
static cmr_int af_thread_proc(struct cmr_msg *message, void* data);
static void af_try_stop(cmr_handle af_handle, cmr_u32 camera_id);
static cmr_int cmr_focus_clear_sem(cmr_handle af_handle);
/*local function declaration end */


cmr_int cmr_focus_init(struct af_init_param *parm_ptr, cmr_u32 camera_id, cmr_handle *af_handle)
{
	UNUSED(camera_id);

	cmr_int           ret     = CMR_CAMERA_SUCCESS;
	struct af_context *af_cxt = NULL;
	CMR_MSG_INIT(message);

	CMR_LOGI("E");

	if (!af_handle || !parm_ptr) {
		CMR_LOGE("af_handle param or parm_ptr error");
		ret = CMR_CAMERA_INVALID_PARAM;
		goto af_init_end;
	}

	*af_handle = (cmr_handle)0;
	af_cxt = (struct af_context*)malloc(sizeof(*af_cxt));
	if (!af_cxt) {
		CMR_LOGE("No mem");
		ret = CMR_CAMERA_NO_MEM;
		goto af_init_end;
	}
	cmr_bzero(af_cxt, sizeof(*af_cxt));
	pthread_mutex_init(&af_cxt->af_isp_caf_mutex, NULL);
	pthread_mutex_init(&af_cxt->set_af_cancel_mutex, NULL);
	sem_init(&af_cxt->isp_af_sem, 0, 0);

	/*create thread*/
	ret = cmr_thread_create((cmr_handle*)&af_cxt->thread_handle,
				CMR_AF_MSG_QUEUE_SIZE,
				af_thread_proc,
				(void*)af_cxt);

	if (CMR_MSG_SUCCESS != ret) {
		CMR_LOGE("create thread fail");
		goto af_init_end;
	}
	*af_handle = (cmr_handle)af_cxt;

	/*send init msg*/
	message.msg_type   = CMR_EVT_AF_INIT;
	message.sync_flag  = CMR_MSG_SYNC_RECEIVED;
	message.data       = malloc(sizeof(*parm_ptr));
	if (!message.data) {
		CMR_LOGE("no memory");
		ret = CMR_CAMERA_NO_MEM;
		goto af_init_end;
	}

	message.alloc_flag = 1;
	cmr_copy(message.data, (void*)parm_ptr, sizeof(*parm_ptr));

	ret = cmr_thread_msg_send(af_cxt->thread_handle, &message);
	if (ret) {
		CMR_LOGE("Failed to send one msg to af thread");
	}

af_init_end:

	if (ret) {
		if (af_cxt) {
			sem_destroy(&af_cxt->isp_af_sem);
			pthread_mutex_destroy(&af_cxt->set_af_cancel_mutex);
			pthread_mutex_destroy(&af_cxt->af_isp_caf_mutex);
			free(af_cxt);
			af_cxt = NULL;
		}

		if (1 == message.alloc_flag) {
			free(message.data);
			message.data = NULL;
		}
	}

	CMR_LOGI("X ret= %ld", ret);
	return ret;
}

cmr_int cmr_focus_deinit_notice(cmr_handle af_handle)
{
	cmr_int           ret     = CMR_CAMERA_SUCCESS;
	struct af_context *af_cxt = (struct af_context *)af_handle;
	CMR_LOGI("cmr_focus_deinit_notice");

	pthread_mutex_lock(&af_cxt->af_isp_caf_mutex);
	af_cxt->focus_need_quit = FOCUS_NEED_QUIT;
	pthread_mutex_unlock(&af_cxt->af_isp_caf_mutex);

	sem_post(&af_cxt->isp_af_sem); // fastly quit af process,--for safty quit post one times

	return ret;
}

cmr_int cmr_focus_deinit(cmr_handle af_handle)
{
	cmr_int           ret     = CMR_CAMERA_SUCCESS;
	struct af_context *af_cxt = (struct af_context *)af_handle;
	CMR_MSG_INIT(message);
	CMR_LOGI("E");

	if (!af_cxt) {
		CMR_LOGE(" handle param invalid ");
		return CMR_CAMERA_INVALID_PARAM;
	}

	/*send exit msg*/
	message.msg_type  = CMR_EVT_AF_EXIT;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	ret = cmr_thread_msg_send(af_cxt->thread_handle, &message);
	if (ret) {
		CMR_LOGE("Failed to send one msg to af thread ret = %ld",ret);
		goto deinit_end;
	}

	/*destroy thread*/
	if (af_cxt->thread_handle) {
		cmr_thread_destroy(af_cxt->thread_handle);
		af_cxt->thread_handle = 0;
	}

	/*local deinit*/
	sem_destroy(&af_cxt->isp_af_sem);
	pthread_mutex_destroy(&af_cxt->set_af_cancel_mutex);
	pthread_mutex_destroy(&af_cxt->af_isp_caf_mutex);

	free(af_cxt);
	af_cxt = NULL;

deinit_end:

	CMR_LOGI("X ret =%ld",ret);
	return ret;
}

cmr_int cmr_af_start_notice_focus(cmr_handle af_handle){
	cmr_int                     ret = 0;
	struct af_context *af_cxt = (struct af_context *)af_handle;
	CMR_LOGI("cmr_af_start_notice_focus");

	pthread_mutex_lock(&af_cxt->af_isp_caf_mutex);
	af_cxt->focus_need_quit = FOCUS_START;
	pthread_mutex_unlock(&af_cxt->af_isp_caf_mutex);

	return ret;
}

cmr_int cmr_af_cancel_notice_focus(cmr_handle af_handle){
	cmr_int                     ret = 0;
	struct af_context *af_cxt = (struct af_context *)af_handle;
	CMR_LOGI("cmr_af_cancel_notice_focus");

	pthread_mutex_lock(&af_cxt->af_isp_caf_mutex);
	af_cxt->focus_need_quit = FOCUS_NEED_QUIT;
	pthread_mutex_unlock(&af_cxt->af_isp_caf_mutex);

	sem_post(&af_cxt->isp_af_sem); // fastly quit af process,--for safty quit post one times

	return ret;
}

cmr_int cmr_transfer_caf_to_af(cmr_handle af_handle){
	cmr_int                         ret                  = CMR_CAMERA_SUCCESS;
	struct af_context               *af_cxt               = (struct af_context *)af_handle;

	pthread_mutex_lock(&af_cxt->af_isp_caf_mutex);
	af_cxt->af_mode_inflight = CAMERA_FOCUS_MODE_CAF;
	pthread_mutex_unlock(&af_cxt->af_isp_caf_mutex);

	return ret;
}

cmr_int cmr_transfer_af_to_caf(cmr_handle af_handle){
	cmr_int                         ret                  = CMR_CAMERA_SUCCESS;
	struct af_context               *af_cxt               = (struct af_context *)af_handle;

	pthread_mutex_lock(&af_cxt->af_isp_caf_mutex);
	af_cxt->af_mode_inflight = CAMERA_FOCUS_MODE_AUTO;
	pthread_mutex_unlock(&af_cxt->af_isp_caf_mutex);

	return ret;
}

/*send message to lunch af*/
cmr_int cmr_focus_start(cmr_handle af_handle, cmr_u32 camera_id)
{
	cmr_int           ret     = CMR_CAMERA_SUCCESS;
	struct af_context *af_cxt = (struct af_context *)af_handle;
	CMR_MSG_INIT(message);
	CMR_LOGI("E");

	if (!af_cxt) {
		CMR_LOGE("invalid param");
		return CMR_CAMERA_INVALID_PARAM;
	}

	af_clear_exit_flag(af_handle);

	message.msg_type  = CMR_EVT_AF_START;
	message.sync_flag = CMR_MSG_SYNC_NONE;
	message.data      = (void*)((unsigned long)camera_id);

	ret = cmr_thread_msg_send(af_cxt->thread_handle, &message);
	if (ret) {
		CMR_LOGE("Failedto send one msg to af thread");
	}

	CMR_LOGI("X ret = %ld", ret);
	return ret;
}

cmr_int cmr_focus_stop(cmr_handle af_handle, cmr_u32 camera_id, cmr_u32 is_need_abort_msg)
{
	cmr_int           ret     = CMR_CAMERA_SUCCESS;
	struct af_context *af_cxt = (struct af_context *)af_handle;
	CMR_MSG_INIT(message);
	CMR_LOGI("E");

	if (!af_cxt) {
		CMR_LOGE("handle param invalid");
		return CMR_CAMERA_INVALID_PARAM;
	}

	pthread_mutex_lock(&af_cxt->set_af_cancel_mutex);
	CMR_LOGI("is_need_abort_msg 0x%x", is_need_abort_msg);
	af_cxt->af_cancelled |= 0x01;
	af_cxt->af_cancelled |= (is_need_abort_msg & 0x1) << 1;
	CMR_LOGI("af_cancelled 0x%x", af_cxt->af_cancelled);
	pthread_mutex_unlock(&af_cxt->set_af_cancel_mutex);

	af_try_stop(af_handle, camera_id);

	message.msg_type     = CMR_EVT_AF_STOP;
	message.sub_msg_type = is_need_abort_msg;
	message.sync_flag    = CMR_MSG_SYNC_NONE;
	message.data         = (void*)((unsigned long)camera_id);

	ret = cmr_thread_msg_send(af_cxt->thread_handle, &message);
	if (ret) {
		CMR_LOGE("Faile to send one msg to af thread");
	}

	CMR_LOGI("ret= %ld X", ret);

	return ret;
}

cmr_int af_thread_proc(struct cmr_msg *message, void* data)
{
	cmr_int                 ret                 = CMR_CAMERA_SUCCESS;
	cmr_u32                 is_need_abort_msg   = 0;
	cmr_u32                 ex_af_cancel_flag   = 0;
	struct af_init_param    *af_parm_ptr        = NULL;
	enum af_cb_type         cb_type             = AF_CB_MAX;
	cmr_u32                 isp_param           = 1;
	struct af_context       *af_cxt             = (struct af_context *)data;
	cmr_handle              af_handle           = (cmr_handle)af_cxt;
	cmr_u32                 camera_id           = CAMERA_ID_MAX;

	if (!af_cxt) {
		CMR_LOGE("handle param invalid");
		return CMR_CAMERA_INVALID_PARAM;
	}

	switch (message->msg_type) {
	case CMR_EVT_AF_INIT:
		af_parm_ptr            = (struct af_init_param *)message->data;
		/*save init param to af_context*/
		af_cxt->oem_handle     = af_parm_ptr->oem_handle;
		af_cxt->ops            = af_parm_ptr->ops;
		af_cxt->evt_cb         = af_parm_ptr->evt_cb;
		break;


	case CMR_EVT_AF_START:
		CMR_PRINT_TIME;
		if (!af_cxt->evt_cb || !af_cxt->ops.get_preview_status) {
			CMR_LOGE("invalid param, 0x%p, 0x%p", af_cxt->evt_cb, af_cxt->ops.get_preview_status);
			break;
		}

		camera_id = (cmr_u32)((unsigned long)message->data);

		if (PREVIEWING != af_cxt->ops.get_preview_status(af_cxt->oem_handle)) {
			CMR_LOGE("preview already stoped");

			af_need_exit(af_handle, &ex_af_cancel_flag);
			if (0x01 == ex_af_cancel_flag) {
				cb_type = AF_CB_ABORT;
			} else {
				cb_type = AF_CB_FAILED;
			}

			af_cxt->evt_cb(cb_type, 0, af_cxt->oem_handle);

			break;
		}

		if (((CAMERA_FOCUS_MODE_CAF == af_cxt->af_mode) ||
			(CAMERA_FOCUS_MODE_CAF_VIDEO== af_cxt->af_mode)) &&
			af_get_focusmove_flag(af_handle)) {
			/*caf move done, return directly*/
			CMR_LOGI("CAF move done already isp_af_win_val=%d ", af_cxt->isp_af_win_val);

			if (NULL != af_cxt->ops.af_pre_proc) {
				af_cxt->ops.af_pre_proc(af_cxt->oem_handle);
			}

			af_cxt->evt_cb(AF_CB_DONE, 0, af_cxt->oem_handle);

			if (NULL != af_cxt->ops.af_post_proc) {
				af_cxt->ops.af_post_proc(af_cxt->oem_handle,0);
			}

			break;
		}

		ret = af_start(af_handle, camera_id);
		CMR_LOGI("af_start ret=%ld",ret);

		af_need_exit(af_handle, &ex_af_cancel_flag);
		if (0x01 == ex_af_cancel_flag) {
			cb_type = AF_CB_ABORT;
		} else if (CMR_MSG_SUCCESS == ret) {
			cb_type = AF_CB_DONE;
		} else {
			cb_type = AF_CB_FAILED;
		}
		CMR_LOGI("cb_type=%d",cb_type);

		af_cxt->evt_cb(cb_type, 0, af_cxt->oem_handle);

		CMR_PRINT_TIME;
		break;


	case CMR_SENSOR_FOCUS_MOVE:
		camera_id = (cmr_u32)((unsigned long)message->data);

		if (!af_cxt->evt_cb || !af_cxt->ops.get_preview_status) {
			CMR_LOGE("invalid param, 0x%p, 0x%p", af_cxt->evt_cb, af_cxt->ops.get_preview_status);
			break;
		}

		if (PREVIEWING == af_cxt->ops.get_preview_status(af_cxt->oem_handle)
		    && ((CAMERA_FOCUS_MODE_CAF == af_cxt->af_mode) ||
				(CAMERA_FOCUS_MODE_CAF_VIDEO == af_cxt->af_mode))) {
			/*YUV sensor caf process, app need move and move done status*/
			CMR_LOGV("CMR_SENSOR_FOCUS_MOVE");

			af_cxt->evt_cb(AF_CB_FOCUS_MOVE, 1, af_cxt->oem_handle);

			af_set_focusmove_flag(af_handle, 0);
			ret = af_start_lightly(af_handle, camera_id);
			af_set_focusmove_flag(af_handle, 1);

			af_cxt->evt_cb(AF_CB_FOCUS_MOVE, 0, af_cxt->oem_handle);
		}
		break;


	case CMR_EVT_CAF_MOVE_START:
		if (!af_cxt->evt_cb || !af_cxt->ops.get_preview_status) {
			CMR_LOGE("invalid param, 0x%p, 0x%p", af_cxt->evt_cb, af_cxt->ops.get_preview_status);
			break;
		}

		if (PREVIEWING == af_cxt->ops.get_preview_status(af_cxt->oem_handle)
		    && ((CAMERA_FOCUS_MODE_CAF == af_cxt->af_mode) ||
				(CAMERA_FOCUS_MODE_CAF_VIDEO == af_cxt->af_mode))) {
			CMR_LOGI("CMR_EVT_CAF_MOVE_START");

			af_cxt->evt_cb(AF_CB_FOCUS_MOVE, 1, af_cxt->oem_handle);
		}
		break;


	case CMR_EVT_CAF_MOVE_STOP:
		if (!af_cxt->evt_cb || !af_cxt->ops.get_preview_status) {
			CMR_LOGE("invalid param, 0x%p, 0x%p", af_cxt->evt_cb, af_cxt->ops.get_preview_status);
			break;
		}

		if (PREVIEWING == af_cxt->ops.get_preview_status(af_cxt->oem_handle)
		    && ((CAMERA_FOCUS_MODE_CAF == af_cxt->af_mode) ||
				(CAMERA_FOCUS_MODE_CAF_VIDEO == af_cxt->af_mode))) {
			CMR_LOGI("CMR_EVT_CAF_MOVE_STOP");

			af_cxt->evt_cb(AF_CB_FOCUS_MOVE, 0, af_cxt->oem_handle);
		}
		break;


	case CMR_EVT_AF_STOP:
		CMR_LOGI("AF stop");
		camera_id         = (cmr_u32)((unsigned long)message->data);
		is_need_abort_msg = message->sub_msg_type;
		CMR_LOGI("camera_id = %d is_need_abort_msg = %d", camera_id, is_need_abort_msg);
		af_stop(af_handle, camera_id, is_need_abort_msg);
		CMR_PRINT_TIME;
		break;


	case CMR_EVT_AF_EXIT:
		CMR_LOGI("AF exit");
		CMR_PRINT_TIME;
		break;


	default:
		CMR_LOGD("unsupported message = 0x%x", message->msg_type);
		break;
	}

	CMR_LOGD("exit.");

	return ret;
}

cmr_int cmr_focus_isp_handle(cmr_handle af_handle, cmr_u32 evt_type, cmr_u32 came_id ,void *data)
{
	UNUSED(came_id);

	cmr_int                     ret       = CMR_MSG_SUCCESS;
	struct af_context           *af_cxt   = (struct af_context *)af_handle;
	cmr_int is_caf_mode;

	if (!af_cxt) {
		CMR_LOGE("handle param invalid");
		return CMR_CAMERA_INVALID_PARAM;
	}

	CMR_LOGI("evt_type = 0x%x", evt_type);

	switch (evt_type) {
	case FOCUS_EVT_ISP_AF_NOTICE:
		CMR_LOGI("af_mode %d, af_busy %d", af_cxt->af_mode, af_cxt->af_busy);
		is_caf_mode = (CAMERA_FOCUS_MODE_CAF == af_cxt->af_mode) ||
					(CAMERA_FOCUS_MODE_CAF_VIDEO == af_cxt->af_mode);
		pthread_mutex_lock(&af_cxt->af_isp_caf_mutex);
		if (!is_caf_mode || af_cxt->af_busy) {
			pthread_mutex_unlock(&af_cxt->af_isp_caf_mutex);
			ret = af_isp_done(af_handle, data);
		 } else if (is_caf_mode) {
		 	struct isp_af_notice *isp_af = (struct isp_af_notice*)data;
			pthread_mutex_unlock(&af_cxt->af_isp_caf_mutex);

			if (ISP_FOCUS_MOVE_START == isp_af->mode) {
				ret = caf_move_start_handle(af_handle);
			} else if(ISP_FOCUS_MOVE_END == isp_af->mode) {
				ret = caf_move_stop_handle(af_handle);
			}
		} else {
			pthread_mutex_unlock(&af_cxt->af_isp_caf_mutex);
		}
		break;

	default:
		CMR_LOGI("unsupported  evt_type = %d", evt_type);
		break;
		}

	CMR_LOGI("X ret = %ld", ret);

	return ret;
}

cmr_int cmr_focus_sensor_handle(cmr_handle af_handle, cmr_u32 evt_type, cmr_u32 camera_id ,void *data)
{
	UNUSED(data);

	cmr_int           ret           = CMR_MSG_SUCCESS;
	struct af_context *af_cxt       = (struct af_context *)af_handle;
	CMR_MSG_INIT(message);

	if (!af_cxt) {
		CMR_LOGE("handle param invalid");
		return CMR_CAMERA_INVALID_PARAM;
	}

	message.msg_type   = evt_type;
	message.sync_flag  = CMR_MSG_SYNC_PROCESSED;
	message.data       = (void*)((unsigned long)camera_id);

	if (CMR_SENSOR_FOCUS_MOVE == evt_type)
		ret = cmr_thread_msg_send(af_cxt->thread_handle, &message);

	return ret;
}

cmr_int cmr_focus_set_param(cmr_handle af_handle, cmr_u32 came_id, enum camera_param_type id, void* param)
{

	cmr_int           ret           = CMR_MSG_SUCCESS;
	struct af_context *af_cxt       = (struct af_context *)af_handle;
	cmr_uint           loop         = 0;

	CMR_LOGV("E");

	if (!af_cxt) {
		CMR_LOGE("handle param invalid");
		return CMR_CAMERA_INVALID_PARAM;
	}
	CMR_LOGV("camera_param_type id=%d param=%p",id,param);
	switch (id){
	case CAMERA_PARAM_FOCUS_RECT:
		if(param != NULL){
			struct cmr_focus_param temp =*(struct cmr_focus_param *)param;

			for(loop = 0; loop < temp.zone_cnt; loop++)
				CMR_LOGE("temp.zone_cnt=%d x=%d ,y=%d, w=%d ,h=%d ",temp.zone_cnt, temp.zone[loop].start_x, temp.zone[loop].start_y, temp.zone[loop].width, temp.zone[loop].height);

			cmr_copy((void*)&af_cxt->focus_zone_param[0], param, CAMERA_FOCUS_RECT_PARAM_LEN);

		}
		break;
	case CAMERA_PARAM_AF_MODE:
		CMR_LOGV("CAMERA_PARAM_AF_MODE");
		ret = af_set_mode(af_handle, came_id, (cmr_u32)((unsigned long)param));
		break;

	default:
		CMR_LOGI("unsupported parmater id= %d", id);
		break;
	}

	CMR_LOGV("ret %ld",  ret);
	return ret;
}

cmr_int caf_move_start_handle(cmr_handle af_handle)
{

	cmr_int           ret           = CMR_CAMERA_SUCCESS;
	CMR_MSG_INIT(message);
	struct af_context *af_cxt       = (struct af_context *)af_handle;

	if (!af_cxt) {
		CMR_LOGE("handle param invalid");
		return CMR_CAMERA_INVALID_PARAM;
	}

	CMR_LOGI("caf start move");

	message.msg_type   = CMR_EVT_CAF_MOVE_START;
	message.sync_flag  = CMR_MSG_SYNC_NONE;
	ret = cmr_thread_msg_send(af_cxt->thread_handle, &message);
	if (ret) {
		CMR_LOGE("Failed to send one msg to camera main thread");
	}

	return ret ;
}

cmr_int caf_move_stop_handle(cmr_handle af_handle)
{
	cmr_int           ret           = CMR_CAMERA_SUCCESS;
	struct af_context *af_cxt       = (struct af_context *)af_handle;
	CMR_MSG_INIT(message);

	CMR_LOGI("E");

	if (!af_cxt) {
		CMR_LOGE("handle param invalid");
		return CMR_CAMERA_INVALID_PARAM;
	}

	message.msg_type   = CMR_EVT_CAF_MOVE_STOP;
	message.sync_flag  = CMR_MSG_SYNC_NONE;
	ret = cmr_thread_msg_send(af_cxt->thread_handle, &message);
	if (ret) {
		CMR_LOGE("Faied to send one msg to camera main thread");
	}

	return ret ;
}


/*local function*/
cmr_int af_set_focusmove_flag(cmr_handle af_handle, cmr_u32 is_done)
{
	struct af_context *af_cxt = (struct af_context *)af_handle;

	if (!af_cxt) {
		CMR_LOGE("handle param invalid" );
		return CMR_CAMERA_INVALID_PARAM;
	}

	af_cxt->caf_move_done = is_done;

	return CMR_CAMERA_SUCCESS;
}

cmr_int af_get_focusmove_flag(cmr_handle af_handle)
{
	cmr_int           is_move_done   = 0;
	struct af_context *af_cxt        = (struct af_context *)af_handle;

	if (!af_cxt) {
		CMR_LOGE("handle param invalid");
		return CMR_CAMERA_INVALID_PARAM;
	}

	is_move_done = af_cxt->caf_move_done;

	CMR_LOGI("move_done %ld", is_move_done);

	return is_move_done;
}

cmr_int af_isp_done(cmr_handle af_handle, void *data)
{
	struct af_context       *af_cxt    = (struct af_context *)af_handle;
	struct isp_af_notice 	*isp_af    = (struct isp_af_notice*)data;

	CMR_LOGI("E");

	if (!af_cxt) {
		CMR_LOGE("handle param invalid");
		return CMR_CAMERA_INVALID_PARAM;
	}

	if (NULL == isp_af) {
	   CMR_LOGE("fail data is NULL");
	   return -1;
	}

	pthread_mutex_lock(&af_cxt->af_isp_caf_mutex);
	CMR_LOGI("AF done, valid_win 0x%x, isp_af_timeout %d",
		isp_af->valid_win, af_cxt->isp_af_timeout);
	af_cxt->isp_af_win_val = isp_af->valid_win;
	if (af_cxt->isp_af_timeout == 0) {
		sem_post(&af_cxt->isp_af_sem);
		CMR_LOGI("sem_post : isp_af_sem");
	}
	pthread_mutex_unlock(&af_cxt->af_isp_caf_mutex);

	CMR_LOGI("X");
	return 0;
}

cmr_int af_init(void)
{
	cmr_int     ret = CMR_CAMERA_SUCCESS;

	CMR_LOGW("no use now");

	return ret;
}

cmr_int af_mode_to_isp(cmr_u32 af_mode, cmr_u32 *isp_af_mode)
{
	cmr_int     ret = CMR_CAMERA_SUCCESS;
	cmr_u32     i   = 0;

	if (!isp_af_mode) {
		CMR_LOGE("isp_af_mode param invalid");
		return CMR_CAMERA_INVALID_PARAM;
	}

	for (i = 0; i < CAMERA_FOCUS_MODE_MAX; i++) {
		if (af_mode == af_isp_focus_mode[i].af_mode) {
			*isp_af_mode = af_isp_focus_mode[i].isp_af_mode;
			break;
		}
	}

	if (CAMERA_FOCUS_MODE_MAX == i) {
		CMR_LOGE("cannt come here!!! pls check af mode, and force to set af");
		*isp_af_mode = af_isp_focus_mode[0].isp_af_mode;
	}

	CMR_LOGW("af_mode %d isp_af_mode %d", af_mode, *isp_af_mode);
	return ret;
}

/*
 *set caf mode to isp ;
 *if to sensor ,first  save af mode to settings,then  get and  set af mode in  af_start
 *
 */
cmr_int af_set_mode(cmr_handle af_handle, cmr_u32 came_id, cmr_u32 af_mode)
{
	cmr_int                 ret         = CMR_CAMERA_SUCCESS;
	cmr_u32                 isp_af_mode = 0;
	struct sensor_exp_info  sensor_info;
	struct af_context       *af_cxt     = (struct af_context *)af_handle;
	struct common_isp_cmd_param isp_cmd;

	if (!af_cxt) {
		CMR_LOGE("handle param invalid");
		ret = CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}

	if (!af_cxt->ops.get_sensor_info) {
		CMR_LOGE("ops is null");
		ret = CMR_CAMERA_FAIL;
		goto exit;
	}

	CMR_LOGI("af_mode %d", af_mode);

	if (af_mode < CAMERA_FOCUS_MODE_MAX) {
		af_cxt->af_mode = af_mode;
	} else {
		CMR_LOGE("param invalid, set af mode to AUTO");
		af_cxt->af_mode = CAMERA_FOCUS_MODE_AUTO;
		ret = CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}

	ret = af_cxt->ops.get_sensor_info(af_cxt->oem_handle, came_id, &sensor_info);
	if (ret) {
		CMR_LOGE("failed to get sensor info, ret %ld", ret);
		ret = CMR_CAMERA_FAIL;
		goto exit;
	}

	if (IMG_DATA_TYPE_RAW == sensor_info.image_format) {

		af_mode_to_isp(af_mode, &isp_af_mode);
		isp_cmd.cmd_value = isp_af_mode;

		CMR_LOGE("isp_af_mode %d ",isp_af_mode);
		ret = af_cxt->ops.af_isp_ioctrl(af_cxt->oem_handle, COM_ISP_SET_AF_MODE, &isp_cmd);

	} else {
		CMR_LOGW("sensor not support");
	}

exit:

	CMR_LOGI("ret %ld", ret);
	return ret;
}

cmr_int af_start(cmr_handle af_handle, cmr_u32 camera_id)
{
	cmr_int                         ret                  = CMR_CAMERA_SUCCESS;
	cmr_u32                         *ptr                 = NULL;
	cmr_u32                         i                    = 0;
	cmr_u32                         zone_cnt             = 0;
	cmr_u32                         af_cancel_is_ext     = 0;
	cmr_u32                         focus_stop_prew      = 0;
	SENSOR_EXT_FUN_PARAM_T          af_param;
	struct isp_af_win               isp_af_param;
	struct common_isp_cmd_param     com_isp_af;
	struct sensor_exp_info          sensor_info;
	struct af_context               *af_cxt               = (struct af_context *)af_handle;
	struct common_sn_cmd_param      yuv_sn_param          = {0};

	cmr_bzero(&af_param, sizeof(af_param));
	cmr_bzero(&isp_af_param, sizeof(isp_af_param));
	cmr_bzero(&com_isp_af, sizeof(com_isp_af));

	CMR_LOGI("E");

	if (!af_cxt) {
		CMR_LOGE("handle param invalid");
		return CMR_CAMERA_INVALID_PARAM;
	}

	if (!af_cxt->ops.get_sensor_info) {
		CMR_LOGE("ops is null");
		ret = CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}

	ret = af_cxt->ops.get_sensor_info(af_cxt->oem_handle, camera_id, &sensor_info);
	if (ret) {
		CMR_LOGE("failed to get sensor info, ret= %ld", ret);
		ret = CMR_CAMERA_FAIL;
		goto exit;
	}

	ptr      = (cmr_u32*)&af_cxt->focus_zone_param[0];
	zone_cnt = *ptr++;

	CMR_LOGI("zone_cnt %d, x y w h, %d %d %d %d", zone_cnt, ptr[0], ptr[1], ptr[2], ptr[3]);

	CMR_PRINT_TIME;
	if (af_need_exit(af_handle, &af_cancel_is_ext)) {
		ret = CMR_CAMERA_INVALID_STATE;
		goto exit;
	}

	/*pre_process flash */
	af_cxt->ops.af_pre_proc(af_cxt->oem_handle);

	if (af_need_exit(af_handle, &af_cancel_is_ext)) {
		ret = CMR_CAMERA_INVALID_STATE;
		goto exit;
	}
	/*focuse preprocess for af zone parameter*/
	focus_rect_parse(af_handle, &af_param);
	CMR_LOGD("zone cnt %d x %d ,y %d ,w %d, h %d", af_param.zone_cnt,af_param.zone[0].x,af_param.zone[0].y,af_param.zone[0].w,af_param.zone[0].h);

	pthread_mutex_lock(&af_cxt->af_isp_caf_mutex);
	af_cxt->af_busy = 1;
	pthread_mutex_unlock(&af_cxt->af_isp_caf_mutex);
	if (IMG_DATA_TYPE_RAW == sensor_info.image_format) {
		isp_af_param.mode      = af_param.param;
		isp_af_param.valid_win = af_param.zone_cnt;

		focus_rect_param_to_isp(af_param, &isp_af_param);
		CMR_LOGV("TYPE_RAW");
		CMR_LOGI("af_win num %d, x:%d y:%d e_x:%d e_y:%d",
			isp_af_param.valid_win,
			isp_af_param.win[i].start_x,
			isp_af_param.win[i].start_y,
			isp_af_param.win[i].end_x,
			isp_af_param.win[i].end_y);
		if((isp_af_param.win[0].start_x == 0) &&
			(isp_af_param.win[0].start_y == 0) &&
			(isp_af_param.win[0].end_x == 0) &&
			(isp_af_param.win[0].end_y == 0)) {
			focus_stop_prew = 1;//need stop preview
		}else {
			focus_stop_prew = 0;
		}
		CMR_LOGI("focus_stop_prew %d", focus_stop_prew);

		pthread_mutex_lock(&af_cxt->af_isp_caf_mutex);
		af_cxt->isp_af_timeout = 0;
		pthread_mutex_unlock(&af_cxt->af_isp_caf_mutex);
		com_isp_af.af_param    = isp_af_param;

		pthread_mutex_lock(&af_cxt->af_isp_caf_mutex);
		if( af_cxt->af_mode_inflight == CAMERA_FOCUS_MODE_CAF &&
			1!=af_cxt->ops.get_flash_info(af_cxt->oem_handle,camera_id)/*1 stands for pre-flash turned on*/ ){
			pthread_mutex_unlock(&af_cxt->af_isp_caf_mutex);
		}else{
			pthread_mutex_unlock(&af_cxt->af_isp_caf_mutex);
			ret = af_cxt->ops.af_isp_ioctrl(af_cxt->oem_handle, COM_ISP_SET_AF, &com_isp_af);

			CMR_LOGD("wait for af_isp_ioctrl");
			ret = cmr_focus_clear_sem(af_handle);
			ret = wait_isp_focus_result(af_handle, camera_id, 0);
		}
	} else {
		CMR_LOGV("TYPE_YUV");
		/*yuv sensor process*/
		yuv_sn_param.yuv_sn_af_param.cmd      = af_param.cmd;
		yuv_sn_param.yuv_sn_af_param.param    = af_param.param;
		yuv_sn_param.yuv_sn_af_param.zone_cnt = af_param.zone_cnt;

		for (i = 0; i < zone_cnt; i++) {
			yuv_sn_param.yuv_sn_af_param.zone[i].start_x = af_param.zone[i].x;
			yuv_sn_param.yuv_sn_af_param.zone[i].start_y = af_param.zone[i].y;
			yuv_sn_param.yuv_sn_af_param.zone[i].width   = af_param.zone[i].w;
			yuv_sn_param.yuv_sn_af_param.zone[i].height  = af_param.zone[i].h;
		}
		if((yuv_sn_param.yuv_sn_af_param.zone[0].start_x == 0) &&
			(yuv_sn_param.yuv_sn_af_param.zone[0].start_y == 0) &&
			(yuv_sn_param.yuv_sn_af_param.zone[0].width == 0) &&
			(yuv_sn_param.yuv_sn_af_param.zone[0].height == 0)) {
			focus_stop_prew = 1;//need stop preview
		}else {
			focus_stop_prew = 0;
		}
		ret = af_cxt->ops.af_sensor_ioctrl(af_cxt->oem_handle, COM_SN_SET_FOCUS, &yuv_sn_param);
	}
	pthread_mutex_lock(&af_cxt->af_isp_caf_mutex);
	af_cxt->af_busy = 0;
	pthread_mutex_unlock(&af_cxt->af_isp_caf_mutex);

	CMR_PRINT_TIME;

exit:

	//if (FOCUS_NEED_QUIT != af_cxt->focus_need_quit) {
	af_cxt->ops.af_post_proc(af_cxt->oem_handle, focus_stop_prew);
	//}
	CMR_LOGI("af_cxt->focus_need_quit %d ret %ld", af_cxt->focus_need_quit, ret);
	return ret;
}

cmr_int af_start_lightly(cmr_handle af_handle, cmr_u32 camera_id)
{
	cmr_int                         ret         = CMR_CAMERA_SUCCESS;
	cmr_int                         i           = 0;
	SENSOR_EXT_FUN_PARAM_T          af_param;
	struct sensor_exp_info          sensor_info;
	struct af_context               *af_cxt      = (struct af_context *)af_handle;
	struct common_sn_cmd_param      yuv_sn_param = {0};

	CMR_LOGI("E");

	if (!af_cxt) {
		CMR_LOGE("handle param invalid");
		return CMR_CAMERA_INVALID_PARAM;
	}

	if (!af_cxt->ops.get_sensor_info) {
		CMR_LOGE("ops is null");
		ret = CMR_CAMERA_FAIL;
		goto exit;
	}

	ret = af_cxt->ops.get_sensor_info(af_cxt->oem_handle, camera_id, &sensor_info);
	if (ret) {
		CMR_LOGE("failed to get sensor info, ret %ld", ret);
		ret = CMR_CAMERA_FAIL;
		goto exit;
	}

	cmr_bzero(&af_param, sizeof(af_param));

	if (IMG_DATA_TYPE_RAW != sensor_info.image_format) {
		af_param.cmd        = SENSOR_EXT_FOCUS_START;
		af_param.param      = SENSOR_EXT_FOCUS_TRIG;
		af_param.zone_cnt   = 0;

		yuv_sn_param.yuv_sn_af_param.cmd      = af_param.cmd;
		yuv_sn_param.yuv_sn_af_param.param    = af_param.param;
		yuv_sn_param.yuv_sn_af_param.zone_cnt = af_param.zone_cnt;

		for (i = 0; i < 1; i++) {
			yuv_sn_param.yuv_sn_af_param.zone[i].start_x = af_param.zone[i].x;
			yuv_sn_param.yuv_sn_af_param.zone[i].start_y = af_param.zone[i].y;
			yuv_sn_param.yuv_sn_af_param.zone[i].width   = af_param.zone[i].w;
			yuv_sn_param.yuv_sn_af_param.zone[i].height  = af_param.zone[i].h;
		}

		ret = af_cxt->ops.af_sensor_ioctrl(af_cxt->oem_handle, COM_SN_SET_FOCUS, &yuv_sn_param);
	}

	if (ret) {
		ret = CMR_CAMERA_FAIL;
		CMR_LOGE("SENSOR_IOCTL_FOCUS error X");
	}

exit:
	CMR_LOGI("ret %ld", ret);
	return ret;
}

cmr_int af_quit(cmr_handle af_handle, cmr_u32 camera_id)
{
	cmr_int                 ret         = CMR_CAMERA_SUCCESS;
	struct sensor_exp_info  sensor_info ;
	struct af_context       *af_cxt     = (struct af_context *)af_handle;

	CMR_LOGI("E");

	if (!af_cxt) {
		CMR_LOGE("handle param invalid");
		return CMR_CAMERA_INVALID_PARAM;
	}

	if (!af_cxt->ops.get_sensor_info) {
		CMR_LOGE("ops is null");
		ret = CMR_CAMERA_FAIL;
		goto exit;
	}

	ret = af_cxt->ops.get_sensor_info(af_cxt->oem_handle, camera_id, &sensor_info);
	if (ret) {
		CMR_LOGE("failed to get sensor info, ret= %ld", ret);
		ret = CMR_CAMERA_FAIL;
		goto exit;
	}

	pthread_mutex_lock(&af_cxt->af_isp_caf_mutex);
	if (!af_cxt->af_busy) {
		pthread_mutex_unlock(&af_cxt->af_isp_caf_mutex);
		CMR_LOGI("autofocus is IDLE direct return!");
		return ret;
	}
	pthread_mutex_unlock(&af_cxt->af_isp_caf_mutex);
	CMR_LOGI("set autofocus quit");
	if (IMG_DATA_TYPE_RAW == sensor_info.image_format) {
		struct isp_af_win isp_af_param;
		cmr_bzero(&isp_af_param,  sizeof(struct isp_af_win));
		ret = af_cxt->ops.af_isp_ioctrl(af_cxt->oem_handle, COM_ISP_SET_AF_STOP, (struct common_isp_cmd_param *)&isp_af_param);
	} else {
		SENSOR_EXT_FUN_PARAM_T af_param;
		cmr_bzero(&af_param, sizeof(af_param));
		af_param.cmd = SENSOR_EXT_FOCUS_QUIT;
		af_cxt->ops.af_sensor_ioctrl(af_cxt->oem_handle, COM_SN_SET_FOCUS, (struct common_sn_cmd_param *)&af_param);
	}

exit:

	CMR_LOGI("ret %ld", ret);
	return ret;
}

cmr_int af_stop(cmr_handle af_handle, cmr_u32 came_id, cmr_u32 is_need_abort_msg)
{
	UNUSED(is_need_abort_msg);

	cmr_int           ret           = CMR_CAMERA_SUCCESS;
	struct af_context *af_cxt       = (struct af_context *)af_handle;

	CMR_LOGI("E");

	if (!af_cxt) {
		CMR_LOGE("handle param invalid");
		return CMR_CAMERA_INVALID_PARAM;
	}

	ret = af_quit(af_handle, came_id);

	CMR_LOGI("X");
	return ret;
}

cmr_int af_clear_exit_flag(cmr_handle af_handle)
{
	cmr_int           ret     = CMR_CAMERA_SUCCESS;
	struct af_context *af_cxt = (struct af_context *)af_handle;

	if (!af_cxt) {
		CMR_LOGE("handle param invalid");
		return CMR_CAMERA_INVALID_PARAM;
	}

	pthread_mutex_lock(&af_cxt->set_af_cancel_mutex);
	af_cxt->af_cancelled = 0x00;
	pthread_mutex_unlock(&af_cxt->set_af_cancel_mutex);

	return ret;
}

cmr_int af_need_exit(cmr_handle af_handle, cmr_u32 *is_need_abort_msg)
{
	cmr_int           ret           = 0;
	struct af_context *af_cxt       = (struct af_context *)af_handle;

	if (!af_cxt) {
		CMR_LOGE("handle param invalid");
		return CMR_CAMERA_INVALID_PARAM;
	}

	/*bit0 is cancel ,bit1 is report to up layer*/
	pthread_mutex_lock(&af_cxt->set_af_cancel_mutex);

	ret = (af_cxt->af_cancelled & 0x01) == 1 ? 1 : 0;
	*is_need_abort_msg = (af_cxt->af_cancelled >> 1) & 0x1;

	pthread_mutex_unlock(&af_cxt->set_af_cancel_mutex);

	if (ret) {
		CMR_LOGI("af_cancelled val: 0x%x, 0x%x", af_cxt->af_cancelled, *is_need_abort_msg);
	}

	CMR_LOGI("ret %ld", ret);
	return ret;
}


cmr_int af_check_area(cmr_handle  af_handle,
						struct img_rect *sensor_rect_ptr,
						struct img_rect *rect_ptr,
						cmr_u32 rect_num)
{
	UNUSED(sensor_rect_ptr);

	cmr_int                 ret          = CMR_CAMERA_SUCCESS;
	cmr_u32                 sn_work_mode = 0;
	cmr_u32                 i            = 0;
	cmr_uint                sensor_mode  = SENSOR_MODE_MAX;
	cmr_u32                 camera_id    = 0;
	struct sensor_mode_info *sensor_mode_info;
	struct sensor_exp_info  sensor_info;
	struct af_context       *af_cxt      = (struct af_context *)af_handle;
	struct camera_context   *cam_cxt;

	if (!af_cxt) {
		CMR_LOGE("handle param invalid");
		ret = CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	cam_cxt     = (struct camera_context*)af_cxt->oem_handle;

	if (!af_cxt->ops.get_sensor_info) {
		CMR_LOGE("ops is null");
		ret = CMR_CAMERA_FAIL;
		goto exit;
	}

	ret = af_cxt->ops.get_sensor_info(af_cxt->oem_handle, camera_id, &sensor_info);
	if (ret) {
		CMR_LOGE("failed to get sensor info, ret %ld", ret);
		ret = CMR_CAMERA_FAIL;
		goto exit;
	}

	ret = cmr_sensor_get_mode(cam_cxt->sn_cxt.sensor_handle, camera_id, &sensor_mode);

	CMR_LOGI("camera_id %d sensor_mode %ld rect_num %d", camera_id, sensor_mode, rect_num);

	sensor_mode_info = &sensor_info.mode_info[sensor_mode];

	for ( i=0 ; i<rect_num ; i++) {

		CMR_LOGW("rect_ptr[i].width =%d rect_ptr[i].height=%d",rect_ptr[i].width,rect_ptr[i].height);
		CMR_LOGW("rect_ptr->width =%d sensor_mode_info->trim_width=%d",rect_ptr->width,sensor_mode_info->trim_width);
		CMR_LOGW("rect_ptr->height =%d sensor_mode_info->trim_height=%d",rect_ptr->height,sensor_mode_info->trim_height);
		CMR_LOGW("rect_ptr->start_x =%d sensor_mode_info->trim_start_x=%d",rect_ptr->start_x,sensor_mode_info->trim_start_x);
		CMR_LOGW("rect_ptr->start_y =%d sensor_mode_info->trim_start_y=%d",rect_ptr->start_y,sensor_mode_info->trim_start_y);

		if ((0 == rect_ptr[i].width) || (0 == rect_ptr[i].height)
			|| (rect_ptr->width  > sensor_mode_info->trim_width )
			|| (rect_ptr->height > sensor_mode_info->trim_height)
			|| ((rect_ptr->start_x + rect_ptr->width ) > (sensor_mode_info->trim_start_x + sensor_mode_info->trim_width))
			|| ((rect_ptr->start_y + rect_ptr->height) > (sensor_mode_info->trim_start_y + sensor_mode_info->trim_height))) {
			ret = CMR_CAMERA_INVALID_PARAM;
			break;
		}
	}

exit:

	CMR_LOGI("ret %ld, result %s", ret,(ret==CMR_CAMERA_SUCCESS) ? "ok" : "error");
	return ret;
}

cmr_int focus_rect_parse(cmr_handle af_handle, SENSOR_EXT_FUN_PARAM_T_PTR p_focus_rect)
{
	cmr_int                 ret             = CMR_CAMERA_SUCCESS;
	struct af_context       *af_cxt         = (struct af_context *)af_handle;
	uint32_t                *ptr            = NULL;
	uint32_t                zone_cnt        = 0;
	uint32_t                i               = 0;
	SENSOR_EXT_FUN_PARAM_T  af_param;
	struct img_rect         af_rect_zone[FOCUS_ZONE_CNT_MAX];

	if (!af_cxt) {
		CMR_LOGE("handle param invalid");
		ret = CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	CMR_LOGI("E");

	ptr = (uint32_t*)&af_cxt->focus_zone_param[0];
	if(!ptr){
		CMR_LOGE("af_cxt focus_zone_param param invalid");
		ret = CMR_CAMERA_INVALID_PARAM;
		goto exit;

	}
	zone_cnt = *ptr++;
	cmr_bzero(&af_param, sizeof(af_param));

	CMR_LOGI("af_mode %d zone_cnt %d, x y w h, %d %d %d %d", af_cxt->af_mode,zone_cnt, ptr[0], ptr[1], ptr[2], ptr[3]);

	switch (af_cxt->af_mode) {
	case CAMERA_FOCUS_MODE_AUTO:
		af_param.cmd      = SENSOR_EXT_FOCUS_START;
		af_param.param    = SENSOR_EXT_FOCUS_TRIG;
		af_param.zone_cnt = 0;

		/*zone info*/
		if (zone_cnt) {
			af_param.zone_cnt  = 1;
			af_param.zone[0].x = *ptr++;
			af_param.zone[0].y = *ptr++;
			af_param.zone[0].w = *ptr++;
			af_param.zone[0].h = *ptr++;
			CMR_LOGI("x %d y %d ,w %d h %d",af_param.zone[0].x,af_param.zone[0].y,af_param.zone[0].w,af_param.zone[0].h);

			af_rect_zone[0].start_x = af_param.zone[0].x;
			af_rect_zone[0].start_y = af_param.zone[0].y;
			af_rect_zone[0].width   = af_param.zone[0].w;
			af_rect_zone[0].height  = af_param.zone[0].h;

			if (CMR_CAMERA_SUCCESS != af_check_area(af_handle, &const_sensor_rect, (struct img_rect*)&af_rect_zone[0],1)) {
				af_param.zone_cnt = 0;

				CMR_LOGI("af_check_area af_param.zone_cnt=%d",af_param.zone_cnt);
			}
		}
		break;

	case CAMERA_FOCUS_MODE_AUTO_MULTI:
		if ( 0 == zone_cnt) {
			af_param.cmd       = SENSOR_EXT_FOCUS_START;
			af_param.param     = SENSOR_EXT_FOCUS_TRIG;
			af_param.zone_cnt  = 0;

		} else if (1 == zone_cnt) {
			af_param.cmd       = SENSOR_EXT_FOCUS_START;
			af_param.param     = SENSOR_EXT_FOCUS_ZONE;
			af_param.zone_cnt  = 1;

			/*zone info*/
			af_param.zone[0].x = *ptr++;
			af_param.zone[0].y = *ptr++;
			af_param.zone[0].w = *ptr++;
			af_param.zone[0].h = *ptr++;
			CMR_LOGI("x %d y %d ,w %d h %d",af_param.zone[0].x,af_param.zone[0].y,af_param.zone[0].w,af_param.zone[0].h);

			af_rect_zone[0].start_x = af_param.zone[0].x;
			af_rect_zone[0].start_y = af_param.zone[0].y;
			af_rect_zone[0].width   = af_param.zone[0].w;
			af_rect_zone[0].height  = af_param.zone[0].h;

			if (CMR_CAMERA_SUCCESS != af_check_area(af_handle, &const_sensor_rect, (struct img_rect*)&af_rect_zone[0],1)) {
				af_param.cmd      = SENSOR_EXT_FOCUS_START;
				af_param.param    = SENSOR_EXT_FOCUS_TRIG;
				af_param.zone_cnt = 0;
				CMR_LOGI("af_check_area af_param.zone_cnt=%d",af_param.zone_cnt);
			}
		} else if(FOCUS_ZONE_CNT_MAX >= zone_cnt) {
			af_param.cmd      = SENSOR_EXT_FOCUS_START;
			af_param.param    = SENSOR_EXT_FOCUS_MULTI_ZONE;
			af_param.zone_cnt = zone_cnt;

			/*zone info*/
			for (i = 0; i < zone_cnt; i++) {
				af_param.zone[i].x = *ptr++;
				af_param.zone[i].y = *ptr++;
				af_param.zone[i].w = *ptr++;
				af_param.zone[i].h = *ptr++;
			}
			CMR_LOGI("x %d y %d ,w %d h %d",af_param.zone[0].x,af_param.zone[0].y,af_param.zone[0].w,af_param.zone[0].h);

			af_rect_zone[0].start_x = af_param.zone[0].x;
			af_rect_zone[0].start_y = af_param.zone[0].y;
			af_rect_zone[0].width   = af_param.zone[0].w;
			af_rect_zone[0].height  = af_param.zone[0].h;

			if (CMR_CAMERA_SUCCESS != af_check_area(af_handle, &const_sensor_rect, (struct img_rect*)&af_rect_zone[0],1)) {
				af_param.cmd       = SENSOR_EXT_FOCUS_START;
				af_param.param     = SENSOR_EXT_FOCUS_TRIG;
				af_param.zone_cnt  = 0;
				CMR_LOGI("af_check_area af_param.zone_cnt=%d",af_param.zone_cnt);
			}
		}
		break;

	case CAMERA_FOCUS_MODE_MACRO:
		af_param.cmd       = SENSOR_EXT_FOCUS_START;
		af_param.param     = SENSOR_EXT_FOCUS_MACRO;
		af_param.zone_cnt  = 1;

		/*zone info*/
		af_param.zone[0].x = *ptr++;
		af_param.zone[0].y = *ptr++;
		af_param.zone[0].w = *ptr++;
		af_param.zone[0].h = *ptr++;
		CMR_LOGI("x %d y %d ,w %d h %d",af_param.zone[0].x,af_param.zone[0].y,af_param.zone[0].w,af_param.zone[0].h);

		af_rect_zone[0].start_x = af_param.zone[0].x;
		af_rect_zone[0].start_y = af_param.zone[0].y;
		af_rect_zone[0].width   = af_param.zone[0].w;
		af_rect_zone[0].height  = af_param.zone[0].h;

		if (CMR_CAMERA_SUCCESS != af_check_area(af_handle, &const_sensor_rect, (struct img_rect*)&af_rect_zone[0], 1)) {
			af_param.zone_cnt = 0;
			CMR_LOGI("af_check_area af_param.zone_cnt=%d",af_param.zone_cnt);
		}
		break;

	case CAMERA_FOCUS_MODE_CAF:
	case CAMERA_FOCUS_MODE_CAF_VIDEO:
		af_param.cmd      = SENSOR_EXT_FOCUS_START;
		af_param.param    = SENSOR_EXT_FOCUS_CAF;
		af_param.zone_cnt = 0;

		break;

	case CAMERA_FOCUS_MODE_INFINITY:
	default:
		break;

	}

	*p_focus_rect = af_param;

exit:
	if(af_cxt != NULL)
		CMR_LOGI("ret %ld  af_mode %d zone_cnt %d", ret, af_cxt->af_mode, af_param.zone_cnt);
	return ret;
}

cmr_int focus_rect_param_to_isp(SENSOR_EXT_FUN_PARAM_T focus_rect, struct isp_af_win *p_isp_af_param)
{
	cmr_int                 ret         = CMR_CAMERA_SUCCESS;
	cmr_int                 i           = 0;
	struct isp_af_win       isp_af_param;

	cmr_bzero(&isp_af_param, sizeof(struct isp_af_win));
	isp_af_param.mode      = focus_rect.param;
	isp_af_param.valid_win = focus_rect.zone_cnt;
	CMR_LOGI("mode %d valid_win %d",isp_af_param.mode, isp_af_param.valid_win);

	for (i = 0; i < focus_rect.zone_cnt; i++) {
		isp_af_param.win[i].start_x = focus_rect.zone[i].x;
		isp_af_param.win[i].start_y = focus_rect.zone[i].y;
		isp_af_param.win[i].end_x   = focus_rect.zone[i].x + focus_rect.zone[i].w - 1;
		isp_af_param.win[i].end_y   = focus_rect.zone[i].y + focus_rect.zone[i].h - 1;
		CMR_LOGI("af_win num:%d, x:%d y:%d e_x:%d e_y:%d",
			isp_af_param.valid_win,
			isp_af_param.win[i].start_x,
			isp_af_param.win[i].start_y,
			isp_af_param.win[i].end_x,
			isp_af_param.win[i].end_y);
	}

	*p_isp_af_param = isp_af_param;

exit:

	CMR_LOGI("ret %ld", ret);
	return ret;
}

static cmr_int cmr_focus_clear_sem(cmr_handle af_handle)
{
	struct af_context *af_cxt = (struct af_context *)af_handle;
	cmr_s32 tmpVal = 0;

	if (!af_cxt) {
		CMR_LOGE("af_context is null.");
		return -1;
	}

	pthread_mutex_lock(&af_cxt->af_isp_caf_mutex);
	sem_getvalue(&af_cxt->isp_af_sem, &tmpVal);
	while (0 < tmpVal && FOCUS_START==af_cxt->focus_need_quit) {
		sem_wait(&af_cxt->isp_af_sem);
		sem_getvalue(&af_cxt->isp_af_sem, &tmpVal);
	}
	pthread_mutex_unlock(&af_cxt->af_isp_caf_mutex);

	return 0;
}
cmr_int wait_isp_focus_result(cmr_handle af_handle, cmr_u32 camera_id, cmr_u32 is_need_abort_msg)
{
	cmr_int                 ret         = CMR_CAMERA_SUCCESS;
	struct timespec         ts;
	struct af_context       *af_cxt     = (struct af_context *)af_handle;

	if (!af_cxt) {
		CMR_LOGE("handle param invalid");
		ret = CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}

	if (clock_gettime(CLOCK_REALTIME, &ts)) {
		ret = -1;
		CMR_LOGE("get time failed");
		goto exit;
	} else {
		ts.tv_sec += ISP_PROCESS_SEC_TIMEOUT;
		if (ts.tv_nsec + ISP_PROCESS_NSEC_TIMEOUT >= (1000 * 1000 * 1000)) {
			ts.tv_nsec = ts.tv_nsec + ISP_PROCESS_NSEC_TIMEOUT - (1000 * 1000 * 1000);
			ts.tv_sec ++;
		} else {
			ts.tv_nsec += ISP_PROCESS_NSEC_TIMEOUT;
		}
		ret = sem_timedwait(&af_cxt->isp_af_sem, &ts);
		if (ret) {
			CMR_LOGE("isp af timeout");
			pthread_mutex_lock(&af_cxt->af_isp_caf_mutex);
			af_cxt->isp_af_timeout = 1;
			pthread_mutex_unlock(&af_cxt->af_isp_caf_mutex);
			af_stop(af_handle, camera_id, is_need_abort_msg);
		} else {
			pthread_mutex_lock(&af_cxt->af_isp_caf_mutex);
			if (1 == af_cxt->isp_af_timeout) {
				pthread_mutex_unlock(&af_cxt->af_isp_caf_mutex);
				af_stop(af_handle, camera_id, is_need_abort_msg);
				goto exit;
			} else {
				pthread_mutex_unlock(&af_cxt->af_isp_caf_mutex);
			}
			if (0 == af_cxt->isp_af_win_val) {
				CMR_LOGE("isp focus failed");
				ret = -1;
			}
			CMR_LOGI("isp focus ret %ld", ret);
		}
	}

exit:

	CMR_LOGI("ret %ld", ret);
	return ret;
}

void af_try_stop(cmr_handle af_handle, cmr_u32 camera_id)
{
	cmr_int                         ret     = CMR_CAMERA_SUCCESS;
	struct af_context               *af_cxt = (struct af_context *)af_handle;
	struct sensor_exp_info			sensor_info;

	CMR_LOGI("s");
	if (!af_cxt || !af_cxt->ops.get_sensor_info) {
		goto exit;
	}
	ret = af_cxt->ops.get_sensor_info(af_cxt->oem_handle, camera_id, &sensor_info);
	if (ret) {
		CMR_LOGE("failed to get sensor info, ret= %ld", ret);
		ret = CMR_CAMERA_FAIL;
		goto exit;
	}
	if (IMG_DATA_TYPE_RAW != sensor_info.image_format) {
		goto exit;
	}
	pthread_mutex_lock(&af_cxt->af_isp_caf_mutex);
	if (1 == af_cxt->af_busy)  {
		if (0 != sem_trywait(&af_cxt->isp_af_sem)) {
			af_cxt->isp_af_timeout = 1;
			sem_post(&af_cxt->isp_af_sem);
			CMR_LOGI("post isp sem");
		}
	}
	pthread_mutex_unlock(&af_cxt->af_isp_caf_mutex);
exit:
	CMR_LOGI("done");
}
