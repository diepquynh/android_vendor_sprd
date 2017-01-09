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

#define LOG_TAG "isp_app"

#include <sys/types.h>
#include "isp_log.h"
#include "isp_ctrl.h"
#include "isp_app.h"
#include "isp_app_msg.h"

/**---------------------------------------------------------------------------*
**				Micro Define					*
**----------------------------------------------------------------------------*/
#define ISP_APP_EVT_START                (1 << 0)
#define ISP_APP_EVT_STOP                  (1 << 1)
#define ISP_APP_EVT_INIT                    (1 << 2)
#define ISP_APP_EVT_DEINIT                (1 << 3)
#define ISP_APP_EVT_CONTINUE           (1 << 4)
#define ISP_APP_EVT_CONTINUE_STOP (1 << 5)
#define ISP_APP_EVT_SIGNAL               (1 << 6)
#define ISP_APP_EVT_SIGNAL_NEXT     (1 << 7)
#define ISP_APP_EVT_IOCTRL               (1 << 8)
#define ISP_APP_EVT_CAPABILITY         (1 << 9)
#define ISP_APP_EVT_SOF                   (1 << 10)
#define ISP_APP_EVT_CTRL_CALLBAC   (1 << 11)

#define ISP_APP_EVT_MASK                (uint32_t)(ISP_APP_EVT_START | ISP_APP_EVT_STOP | ISP_APP_EVT_INIT \
					| ISP_APP_EVT_DEINIT | ISP_APP_EVT_CONTINUE | ISP_APP_EVT_CONTINUE_STOP \
					| ISP_APP_EVT_SIGNAL | ISP_APP_EVT_SIGNAL_NEXT | ISP_APP_EVT_IOCTRL \
					| ISP_APP_EVT_CAPABILITY | ISP_APP_EVT_SOF | ISP_APP_EVT_CTRL_CALLBAC)

#define ISP_APP_THREAD_QUEUE_NUM 50

#define ISP_APP_TRAC(_x_) ISP_LOG _x_
#define ISP_APP_RETURN_IF_FAIL(exp,warning) do{if(exp) {ISP_APP_TRAC(warning); return exp;}}while(0)
#define ISP_APP_TRACE_IF_FAIL(exp,warning) do{if(exp) {ISP_APP_TRAC(warning);}}while(0)

#define ISP_APP_EB 0x01
#define ISP_APP_UEB 0x00

#define ISP_APP_ZERO 0x00
#define ISP_APP_ONE 0x01
#define ISP_APP_TWO 0x02

#define ISP_APP_MAX_HANDLE_NUM 0x02

#define ISP_APP_INVALID 0xffffffff

/**---------------------------------------------------------------------------*
**				Data Structures 					*
**---------------------------------------------------------------------------*/
enum isp_app_return {
	ISP_APP_SUCCESS=0x00,
	ISP_APP_PARAM_NULL,
	ISP_APP_PARAM_ERROR,
	ISP_APP_CALLBACK_NULL,
	ISP_APP_ALLOC_ERROR,
	ISP_APP_NO_READY,
	ISP_APP_ERROR,
	ISP_APP_RETURN_MAX=0xffffffff
};

enum isp_app_status{
	ISP_APP_CLOSE=0x00,
	ISP_APP_IDLE,
	ISP_APP_RUN,
	ISP_APP_STATE_MAX
};

struct isp_app_context{
	uint32_t ae_stab;
	uint32_t af_flag;
	uint32_t lum_measure_flag;
	uint32_t stop_handle_flag;
	struct isp_af_win af_info;
	enum isp_ae_weight lum_measure_mode;

	proc_callback ctrl_callback;
	cmr_handle oem_handle;
};

struct isp_app_system{
	uint32_t handler_num;
	uint32_t isp_status;
	pthread_t app_thr;
	cmr_handle app_queue;
	uint32_t app_status;
	pthread_mutex_t cond_mutex;

	pthread_cond_t init_cond;
	pthread_cond_t deinit_cond;
	pthread_cond_t continue_cond;
	pthread_cond_t continue_stop_cond;
	pthread_cond_t signal_cond;
	pthread_cond_t ioctrl_cond;
	pthread_cond_t capability_cond;
	pthread_cond_t thread_common_cond;
};

struct isp_app_param{
	struct isp_app_system system;
	struct isp_app_context context[2];
};

struct isp_app_respond
{
	uint32_t rtn;
};

/**---------------------------------------------------------------------------*
**				extend Variables and function			*
**---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
**				Local Variables 					*
**---------------------------------------------------------------------------*/
static struct isp_app_param s_isp_app_context;
static struct isp_app_param* s_isp_app_context_ptr = &s_isp_app_context;
static pthread_mutex_t s_app_mutex={0x00};

/**---------------------------------------------------------------------------*
**					Constant Variables				*
**---------------------------------------------------------------------------*/
static int _isp_app_msg_post(struct isp_app_msg *message);

/**---------------------------------------------------------------------------*
**					Local Function Prototypes			*
**---------------------------------------------------------------------------*/

/* ispAppGetSystem --
*@
*@
*@ return:
*/
static struct isp_app_system* ispAppGetSystem(void)
{
	return &(s_isp_app_context_ptr->system);
}

/* ispAppGetContext --
*@
*@
*@ return:
*/
static struct isp_app_context* ispAppGetContext(uint32_t handler_id)
{
	return &s_isp_app_context_ptr->context[handler_id];
}

/* _isp_AppInitContext --
*@
*@
*@ return:
*/
static uint32_t _isp_AppInitContext(void)
{
	int32_t rtn = ISP_APP_SUCCESS;
	memset((void*)s_isp_app_context_ptr, 0x00, sizeof(struct isp_app_param));

	return rtn;
}

#if 0
/* _isp_AppDeinitContext --
*@
*@
*@ return:
*/
static int32_t _isp_AppDeinitContext(void)
{
	int32_t rtn = ISP_APP_SUCCESS;

	if (NULL != s_isp_app_context_ptr) {
		free(s_isp_app_context_ptr);
		s_isp_app_context_ptr = NULL;
	}
	return rtn;
}
#endif

/* _isp_AppLock --
*@
*@
*@ return:
*/
static int32_t _isp_AppLock(void)
{
	int32_t rtn = ISP_APP_SUCCESS;

	rtn = pthread_mutex_lock(&s_app_mutex);

	return rtn;
}

/* _isp_AppUnlock --
*@
*@
*@ return:
*/
static int32_t _isp_AppUnlock(void)
{
	int32_t rtn=ISP_APP_SUCCESS;

	rtn = pthread_mutex_unlock(&s_app_mutex);

	return rtn;
}

/* _isp_AppCreateHandler --
*@
*@
*@ return:
*/
static uint32_t _isp_AppCreateHandler(struct isp_init_param* ptr)
{
	uint32_t handler_id = ISP_APP_INVALID;

	handler_id = 0x00;

	return handler_id;
}

/* _isp_AppAfDenoiseRecover --
*@
*@
*@ return:
*/
uint32_t _isp_AppAfDenoiseRecover(uint32_t handler_id)
{
	int32_t rtn=ISP_APP_SUCCESS;
	uint32_t denoise_level=0x00;

	rtn = isp_ctrl_capability(handler_id, ISP_DENOISE_LEVEL, (void*)&denoise_level);

	if (ISP_APP_ZERO != (0x80000000&denoise_level)) {
		denoise_level&=0xff;
		rtn = isp_ctrl_ioctl(handler_id, ISP_CTRL_AF_DENOISE, (void*)&denoise_level);
	} else {
		rtn = isp_ctrl_ioctl(handler_id, ISP_CTRL_DENOISE, (void*)&denoise_level);
	}

	return rtn;
}

/* _isp_AppSetLumMeasureCond --
*@
*@
*@ return:
*/
uint32_t _isp_AppSetLumMeasureCond(uint32_t handler_id)
{
	int32_t rtn = ISP_APP_SUCCESS;
	struct isp_app_context* isp_context_ptr = ispAppGetContext(handler_id);
	uint32_t ae_change = ISP_APP_EB;

	if (ISP_APP_EB == isp_context_ptr->lum_measure_flag) {
		rtn = isp_ctrl_ioctl(handler_id, ISP_CTRL_GET_AE_CHG, (void*)&ae_change);
		isp_context_ptr->lum_measure_flag = ISP_APP_UEB;
	}

	return rtn;
}

/* _isp_AppLumMeasureRecover --
*@
*@
*@ return:
*/
uint32_t _isp_AppLumMeasureRecover(uint32_t handler_id)
{
	int32_t rtn = ISP_APP_SUCCESS;
	struct isp_app_context* isp_context_ptr = ispAppGetContext(handler_id);

	rtn = isp_ctrl_ioctl(handler_id, ISP_CTRL_AE_MEASURE_LUM, (void*)&isp_context_ptr->lum_measure_mode);

	return rtn;
}

/* _isp_AppCtrlCallback --
*@
*@
*@ return:
*/
int32_t _isp_AppCtrlCallback(uint32_t handler_id, int32_t mode, void* param_ptr, uint32_t param_len)
{
	int32_t rtn=ISP_APP_SUCCESS;
	struct isp_app_context* isp_context_ptr=ispAppGetContext(handler_id);
	ISP_APP_MSG_INIT(isp_ctrl_msg);

	if (ISP_APP_ZERO != (ISP_CALLBACK_EVT&mode)) {
		if ((ISP_AF_NOTICE_CALLBACK == (ISP_EVT_MASK&mode))
			|| (ISP_AE_CHG_CALLBACK == (ISP_EVT_MASK&mode))) {
			isp_ctrl_msg.handler_id = handler_id;
			isp_ctrl_msg.msg_type = ISP_APP_EVT_CTRL_CALLBAC;
			isp_ctrl_msg.sub_msg_type = mode;
			isp_ctrl_msg.data_len = param_len;
			if (NULL != param_ptr) {
				isp_ctrl_msg.data = malloc(param_len);
				memcpy(isp_ctrl_msg.data, param_ptr, param_len);
				isp_ctrl_msg.alloc_flag = 0x01;
			} else {
				isp_ctrl_msg.alloc_flag = 0x00;
			}
			isp_ctrl_msg.respond = 0x00;
			rtn = _isp_app_msg_post(&isp_ctrl_msg);
			ISP_APP_RETURN_IF_FAIL(rtn, ("ctrl callback send msg to app thread error"));
		} else {
			//isp_context_ptr->ctrl_callback(handler_id, mode, param_ptr, param_len);
			ISP_LOG("$LHC:0x%x", mode);
			if((ISP_AE_BAPASS_CALLBACK == (ISP_EVT_MASK&mode))
				|| (ISP_FLASH_AE_CALLBACK == (ISP_EVT_MASK&mode))) {
				isp_context_ptr->ctrl_callback(isp_context_ptr->oem_handle, ISP_CALLBACK_EVT|ISP_AE_STAB_CALLBACK, param_ptr, param_len);

			} else {
				isp_context_ptr->ctrl_callback(isp_context_ptr->oem_handle, mode, param_ptr, param_len);
			}
		}
	} else {
		//isp_context_ptr->ctrl_callback(handler_id, mode, param_ptr, param_len);
		isp_context_ptr->ctrl_callback(isp_context_ptr->oem_handle, mode, param_ptr, param_len);
	}

	return rtn;
}

/* _isp_AppCtrlCallbackHandler --
*@
*@
*@ return:
*/
uint32_t _isp_AppCtrlCallbackHandler(uint32_t handler_id, int32_t mode, void* param_ptr, uint32_t param_len)
{
	int32_t rtn=ISP_APP_SUCCESS;
	struct isp_app_context* isp_context_ptr=ispAppGetContext(handler_id);

	if (ISP_APP_ZERO != (ISP_CALLBACK_EVT&mode)) {
		if (ISP_AF_NOTICE_CALLBACK == (ISP_EVT_MASK&mode)) {
			if (1 != isp_context_ptr->stop_handle_flag) {
			//rtn = _isp_AppAfDenoiseRecover(handler_id);
			rtn = _isp_AppSetLumMeasureCond(handler_id);
			}
		}
		if (ISP_AE_CHG_CALLBACK == (ISP_EVT_MASK&mode)) {
			rtn = _isp_AppLumMeasureRecover(handler_id);
		}
	}
	isp_context_ptr->ctrl_callback(isp_context_ptr->oem_handle, mode, param_ptr, param_len);

	//isp_context_ptr->ctrl_callback(handler_id, mode, param_ptr, param_len);

	return rtn;
}

/* _isp_AppStopVideoHandler --
*@
*@
*@ return:
*/
uint32_t _isp_AppStopVideoHandler(uint32_t handler_id)
{
	int32_t rtn=ISP_APP_SUCCESS;
	struct isp_app_context* isp_context_ptr=ispAppGetContext(handler_id);
	struct isp_af_notice af_notice;

	if (ISP_APP_EB == isp_context_ptr->af_flag) {
		ISP_LOG("App Stop ISP_AF_NOTICE_CALLBACK");
		af_notice.mode=ISP_FOCUS_MOVE_END;
		af_notice.valid_win=0x00;
		//isp_context_ptr->ctrl_callback(handler_id, ISP_CALLBACK_EVT|ISP_AF_NOTICE_CALLBACK, (void*)&af_notice, sizeof(struct isp_af_notice));
		isp_context_ptr->ctrl_callback(isp_context_ptr->oem_handle,ISP_CALLBACK_EVT|ISP_AF_NOTICE_CALLBACK, (void*)&af_notice, sizeof(struct isp_af_notice));

		isp_context_ptr->af_flag = ISP_APP_UEB;
		isp_context_ptr->stop_handle_flag = 1;
	}

	return rtn;
}

/* _isp_AppCallBack --
*@
*@
*@ return:
*/
static int32_t _isp_AppCallBack(uint32_t handler_id, int32_t mode, void* param_ptr, uint32_t param_len)
{
	int32_t rtn = ISP_APP_SUCCESS;
	struct isp_app_context* isp_context_ptr = ispAppGetContext(handler_id);
	ISP_APP_MSG_INIT(isp_ctrl_msg);
	uint32_t cmd = mode&0xffff;

	switch(cmd)
	{
		case ISP_AE_STAB_CALLBACK:
			ISP_LOG("APP ISP_AE_STAB_CALLBACK");
			isp_context_ptr->ae_stab = ISP_APP_EB;
			break;

		case ISP_SOF_CALLBACK:
			if ((ISP_APP_EB == isp_context_ptr->af_flag)
				&&(ISP_APP_EB == isp_context_ptr->ae_stab)) {
				ISP_LOG("APP ISP_SOF_CALLBACK");
				isp_ctrl_msg.handler_id = handler_id;
				isp_ctrl_msg.msg_type = ISP_APP_EVT_SOF;
				isp_ctrl_msg.alloc_flag = 0x00;
				isp_ctrl_msg.respond = 0x00;
				rtn = _isp_app_msg_post(&isp_ctrl_msg);
				ISP_APP_RETURN_IF_FAIL(rtn, ("send msg to app thread error"));
			}
			break;

		default :
			break;
	}

	return rtn;
}

/* _isp_AppSofHandler --
*@
*@
*@ return:
*/
static int32_t _isp_AppSofHandler(uint32_t handler_id)
{
	int32_t rtn = ISP_APP_SUCCESS;
	struct isp_app_context* isp_context_ptr = ispAppGetContext(handler_id);

	if (ISP_APP_EB == isp_context_ptr->af_flag) {
		struct isp_af_win af_param;
		memcpy((void*)&af_param, (void*)&isp_context_ptr->af_info, sizeof(struct isp_af_win));
		rtn = isp_ctrl_ioctl(handler_id, ISP_CTRL_AF, (void*)&af_param);
		isp_context_ptr->af_flag = ISP_APP_UEB;
	}

	return rtn;
}

/* _isp_AppAfIoCtrlHandler --
*@
*@
*@ return:
*/
static int32_t _isp_AppAfIoCtrlHandler(uint32_t handler_id, void* param_ptr)
{
	int32_t rtn = ISP_APP_SUCCESS;
	struct isp_app_context* isp_context_ptr = ispAppGetContext(handler_id);
	struct isp_af_win* af_param_ptr=(struct isp_af_win*)param_ptr;
	uint32_t ae_stab = ISP_APP_EB;
	struct isp_af_notice af_notice;
	uint32_t ae_awb_status[2]={100,100};

	if (ISP_APP_EB == af_param_ptr->ae_touch) {
		rtn = isp_ctrl_ioctl(handler_id, ISP_CTRL_AE_TOUCH, (void*)&af_param_ptr->ae_touch_rect);
		if (ISP_APP_SUCCESS == rtn) {
			isp_context_ptr->lum_measure_flag = ISP_APP_EB;
			isp_context_ptr->ae_stab=ISP_APP_UEB;
			rtn = isp_ctrl_ioctl(handler_id, ISP_CTRL_GET_AE_STAB, (void*)&ae_stab);
		} else {
			isp_context_ptr->lum_measure_flag = ISP_APP_UEB;
			rtn = ISP_APP_SUCCESS;
		}
	}

	if (ISP_APP_EB != isp_context_ptr->ae_stab) {
		memcpy((void*)&isp_context_ptr->af_info, af_param_ptr, sizeof(struct isp_af_win));
		isp_context_ptr->af_flag = ISP_APP_EB;
		rtn = ISP_APP_ERROR;
	}

	if(ISP_APP_EB != isp_context_ptr->ae_stab){  // if ae not stable comes up,immediately call back
	    af_param_ptr->ae_stab = isp_context_ptr->ae_stab;
	    af_notice.valid_win = 1;
	    isp_context_ptr->ctrl_callback(handler_id,ISP_CALLBACK_EVT|ISP_AF_NOTICE_CALLBACK, (void*)&af_notice, sizeof(struct isp_af_notice));
	    isp_context_ptr->af_flag = ISP_APP_UEB;
	    rtn = isp_ctrl_ioctl(handler_id, ISP_CTRL_GET_AEAWB_BYPASS_STATUS, (void*)ae_awb_status);
	    ISP_LOG("ae no stable -- ae_status %d awb_status %d",ae_awb_status[0],ae_awb_status[1]);
	}

	return rtn;
}


/* _isp_AppIoCtrlHandler --
*@
*@
*@ return:
*/
static int32_t _isp_AppIoCtrlHandler(uint32_t handler_id, enum isp_ctrl_cmd io_cmd, void* param_ptr)
{
	int32_t rtn = ISP_APP_SUCCESS;
	struct isp_app_context* isp_context_ptr = ispAppGetContext(handler_id);
	enum isp_ctrl_cmd cmd = io_cmd&0x3fffffff;
	ISP_LOG("cmd =%d",cmd);

	switch (cmd)
	{
		case ISP_CTRL_AF:
		{
			uint32_t denoise_level=0xfe;
			//rtn = isp_ctrl_ioctl(handler_id, ISP_CTRL_AF_DENOISE, (void*)&denoise_level);
			rtn = _isp_AppAfIoCtrlHandler(handler_id, param_ptr);
			break ;
		}
		case ISP_CTRL_AE_MEASURE_LUM:
		{
			isp_context_ptr->lum_measure_mode=*(uint32_t*)param_ptr;
			break;
		}
		default :
			break ;
	}

	return rtn;
}

/* _isp_AppSetStatus --
*@
*@
*@ return:
*/
static int32_t _isp_AppSetStatus(uint32_t status)
{
	int32_t rtn = ISP_APP_SUCCESS;
	struct isp_app_system* isp_system_ptr = ispAppGetSystem();
	ALOGI("system = %p", isp_system_ptr);
	isp_system_ptr->isp_status = status;

	return rtn;
}

/* _isp_AppGetStatus --
*@
*@
*@ return:
*/
static int32_t _isp_AppGetStatus(void)
{
	int32_t rtn = ISP_APP_SUCCESS;
	struct isp_app_system* isp_system_ptr = ispAppGetSystem();

	return isp_system_ptr->isp_status;
}

/* _isp_set_app_init_param --
*@
*@
*@ return:
*/
static int32_t _isp_set_app_init_param(uint32_t handler_id, struct isp_init_param* ptr)
{
	int32_t rtn = ISP_APP_SUCCESS;
	struct isp_app_context* isp_context_ptr = ispAppGetContext(handler_id);

	ptr->self_callback = _isp_AppCallBack;
	isp_context_ptr->ctrl_callback = ptr->ctrl_callback;
	ptr->ctrl_callback = _isp_AppCtrlCallback;
	isp_context_ptr->oem_handle = (cmr_handle)ptr->oem_handle;
	ISP_LOG("isp_context_ptr->oem_handle = %p", isp_context_ptr->oem_handle);

	return rtn;
}

/* _isp_app_init --
*@
*@
*@ return:
*/
static int _isp_app_init(uint32_t handler_id, struct isp_init_param* ptr)
{
	int rtn = ISP_APP_SUCCESS;

	rtn = _isp_set_app_init_param(handler_id, ptr);
	ISP_APP_RETURN_IF_FAIL(rtn, ("set app init param error"));

	rtn = isp_ctrl_init(handler_id, ptr);
	ISP_APP_RETURN_IF_FAIL(rtn, ("ctr init error"));

	return rtn;
}

/* _isp_app_deinit --
*@
*@
*@ return:
*/
static int _isp_app_deinit(uint32_t handler_id)
{
	int rtn = ISP_APP_SUCCESS;

	rtn = isp_ctrl_deinit(handler_id);
	ISP_APP_RETURN_IF_FAIL(rtn, ("ctr deinit error"));

	return rtn;
}


/* _isp_set_app_video_param --
*@
*@
*@ return:
*/
static int32_t _isp_set_app_video_param(uint32_t handler_id, struct isp_video_start* ptr)
{
	int32_t rtn = ISP_APP_SUCCESS;
	struct isp_app_context* isp_context_ptr = ispAppGetContext(handler_id);

	isp_context_ptr->ae_stab = ISP_APP_INVALID;
	isp_context_ptr->af_flag = ISP_APP_INVALID;

	return rtn;
}

/* _isp_app_video_start --
*@
*@
*@ return:
*/
static int _isp_app_video_start(uint32_t handler_id, struct isp_video_start* ptr)
{
	int rtn = ISP_APP_SUCCESS;
	struct isp_app_context* isp_context_ptr=ispAppGetContext(handler_id);

	rtn = _isp_set_app_video_param(handler_id, ptr);
	ISP_APP_RETURN_IF_FAIL(rtn, ("set app video param error"));

	isp_context_ptr->stop_handle_flag = 0;
	rtn = isp_ctrl_video_start(handler_id, ptr);
	ISP_APP_RETURN_IF_FAIL(rtn, ("app video start error"));

	return rtn;
}

/* _isp_msg_queue_create --
*@
*@
*@ return:
*/
static int _isp_msg_queue_create(uint32_t count, cmr_handle *queue_handle)
{
	int rtn = ISP_APP_SUCCESS;

	rtn = isp_app_msg_queue_create( count, queue_handle);

	return rtn;
}

/* _isp_msg_queue_destroy --
*@
*@
*@ return:
*/
static int _isp_msg_queue_destroy(cmr_handle queue_handle)
{
	int rtn = ISP_APP_SUCCESS;

	rtn = isp_app_msg_queue_destroy(queue_handle);

	return rtn;
}

/* _isp_cond_wait --
*@
*@
*@ return:
*/
static int _isp_cond_wait(pthread_cond_t* cond_ptr, pthread_mutex_t* mutex_ptr)
{
	int rtn = ISP_APP_SUCCESS;
	uint32_t handler_id=0x00;

	rtn = pthread_mutex_lock(mutex_ptr);
	ISP_APP_RETURN_IF_FAIL(rtn, ("lock cond mutex %d error", rtn));
	rtn = pthread_cond_wait(cond_ptr, mutex_ptr);
	ISP_APP_RETURN_IF_FAIL(rtn, ("cond wait %d error", rtn));
	rtn = pthread_mutex_unlock(mutex_ptr);
	ISP_APP_RETURN_IF_FAIL(rtn, ("unlock cond mutex %d error", rtn));

	return rtn;
}

/* _isp_cond_signal --
*@
*@
*@ return:
*/
static int _isp_cond_signal(pthread_cond_t* cond_ptr, pthread_mutex_t* mutex_ptr)
{
	int rtn = ISP_APP_SUCCESS;
	uint32_t handler_id=0x00;

	rtn = pthread_mutex_lock(mutex_ptr);
	ISP_APP_RETURN_IF_FAIL(rtn, ("lock cond mutex %d error", rtn));
	rtn=pthread_cond_signal(cond_ptr);
	ISP_APP_RETURN_IF_FAIL(rtn, ("cond signal %d error", rtn));
	rtn = pthread_mutex_unlock(mutex_ptr);
	ISP_APP_RETURN_IF_FAIL(rtn, ("unlock cond mutex %d error", rtn));
	return rtn;
}

/* _isp_app_msg_get --
*@
*@
*@ return:
*/
static int _isp_app_msg_get(struct isp_app_msg *message)
{
	int rtn = ISP_APP_SUCCESS;
	struct isp_app_system* isp_system_ptr = NULL;
	isp_system_ptr = ispAppGetSystem();
	if (!isp_system_ptr) {
		ALOGE("_isp_app_msg_get ptr=NULL");
		message->msg_type = 0;
		return rtn;
	}
	rtn = isp_app_msg_get( isp_system_ptr->app_queue, message);

	return rtn;
}

/* _isp_app_msg_post --
*@
*@
*@ return:
*/
static int _isp_app_msg_post(struct isp_app_msg *message)
{
	int rtn = ISP_APP_SUCCESS;
	struct isp_app_system* isp_system_ptr = ispAppGetSystem();

	rtn = isp_app_msg_post( isp_system_ptr->app_queue, message);

	return rtn;
}

/* _isp_app_routine --
*@
*@
*@ return:
*/
static void *_isp_app_routine(void *client_data)
{
	int rtn = ISP_APP_SUCCESS;
	struct isp_app_respond* res_ptr;
	struct isp_app_system* isp_system_ptr = ispAppGetSystem();
	ISP_APP_MSG_INIT(isp_ctrl_msg);
	ISP_APP_MSG_INIT(isp_ctrl_self_msg);
	uint32_t handler_id = ISP_APP_ZERO;
	uint32_t evt = ISP_APP_ZERO;
	uint32_t sub_type = ISP_APP_ZERO;
	void* param_ptr = NULL;
	uint32_t param_len = ISP_APP_ZERO;

	ISP_LOG("enter isp app routine.");

	while (1) {
		rtn = _isp_app_msg_get(&isp_ctrl_msg);
		if (rtn) {
			ISP_LOG("msg queue error");
			break;
		}

		_isp_AppSetStatus(ISP_APP_RUN);
		handler_id = isp_ctrl_msg.handler_id;
		evt = (uint32_t)(isp_ctrl_msg.msg_type & ISP_APP_EVT_MASK);
		sub_type = isp_ctrl_msg.sub_msg_type;
		param_ptr = (void*)isp_ctrl_msg.data;
		param_len = isp_ctrl_msg.data_len;
		res_ptr = (void*)isp_ctrl_msg.respond;
		ISP_LOG("evt=0x%x", evt);
		switch (evt) {
			case ISP_APP_EVT_START:
				//ISP_LOG("ISP_APP_EVT_START");
				_isp_AppSetStatus(ISP_APP_IDLE);
				pthread_mutex_lock(&isp_system_ptr->cond_mutex);
				rtn = pthread_cond_signal(&isp_system_ptr->thread_common_cond);
				pthread_mutex_unlock(&isp_system_ptr->cond_mutex);
				break;

			case ISP_APP_EVT_STOP:
				//ISP_LOG("ISP_APP_EVT_STOP");
				_isp_AppSetStatus(ISP_APP_CLOSE);
				pthread_mutex_lock(&isp_system_ptr->cond_mutex);
				rtn = pthread_cond_signal(&isp_system_ptr->thread_common_cond);
				pthread_mutex_unlock(&isp_system_ptr->cond_mutex);
				break;

			case ISP_APP_EVT_INIT:
				//ISP_LOG("ISP_APP_EVT_INIT");
				rtn = _isp_app_init(handler_id, (struct isp_init_param*)param_ptr);
				pthread_mutex_lock(&isp_system_ptr->cond_mutex);
				rtn = pthread_cond_signal(&isp_system_ptr->init_cond);
				pthread_mutex_unlock(&isp_system_ptr->cond_mutex);
				break;

			case ISP_APP_EVT_DEINIT:
				//ISP_LOG("ISP_APP_EVT_DEINIT");
				rtn = _isp_app_deinit(handler_id);
				pthread_mutex_lock(&isp_system_ptr->cond_mutex);
				rtn = pthread_cond_signal(&isp_system_ptr->deinit_cond);
				pthread_mutex_unlock(&isp_system_ptr->cond_mutex);
				break;

			case ISP_APP_EVT_CONTINUE:
				//ISP_LOG("ISP_APP_EVT_CONTINUE");
				rtn=_isp_app_video_start(handler_id, (struct isp_video_start*)param_ptr);
				res_ptr->rtn = rtn;
				pthread_mutex_lock(&isp_system_ptr->cond_mutex);
				rtn = pthread_cond_signal(&isp_system_ptr->continue_cond);
				pthread_mutex_unlock(&isp_system_ptr->cond_mutex);
				break;

			case ISP_APP_EVT_CONTINUE_STOP:
				//ISP_LOG("ISP_APP_EVT_CONTINUE_STOP");
				rtn = isp_ctrl_video_stop(handler_id);
				if (ISP_APP_SUCCESS == rtn) {
					rtn = _isp_AppStopVideoHandler(handler_id);
				}
				res_ptr->rtn = rtn;
				pthread_mutex_lock(&isp_system_ptr->cond_mutex);
				rtn = pthread_cond_signal(&isp_system_ptr->continue_stop_cond);
				pthread_mutex_unlock(&isp_system_ptr->cond_mutex);
				break;

			case ISP_APP_EVT_SIGNAL:
				//ISP_LOG("ISP_APP_EVT_SIGNAL");
				rtn = isp_ctrl_proc_start(handler_id, (struct ips_in_param*)param_ptr, NULL);
				res_ptr->rtn = rtn;
				break;

			case ISP_APP_EVT_SIGNAL_NEXT:
				//ISP_LOG("ISP_APP_EVT_SIGNAL_NEXT");
				rtn=isp_ctrl_proc_next(handler_id, (struct ipn_in_param*)param_ptr, NULL);
				res_ptr->rtn = rtn;
				break;

			case ISP_APP_EVT_IOCTRL:
				//ISP_LOG("--app_isp_ioctl--cmd:0x%x", sub_type);
				rtn = _isp_AppIoCtrlHandler(handler_id, sub_type, param_ptr);
				ISP_LOG("_isp_AppIoCtrlHandler rtn =%d",rtn);
				if (ISP_APP_SUCCESS == rtn) {
					rtn = isp_ctrl_ioctl(handler_id, sub_type, param_ptr);
				} else {
					ISP_LOG("@@@ _isp_AppIoCtrlHandler error ,and cann`t exec isp_ctrl_ioctl @@@");
				}
				res_ptr->rtn = rtn;
				pthread_mutex_lock(&isp_system_ptr->cond_mutex);
				rtn = pthread_cond_signal(&isp_system_ptr->ioctrl_cond);
				pthread_mutex_unlock(&isp_system_ptr->cond_mutex);
				break;

			case ISP_APP_EVT_CAPABILITY:
				//ISP_LOG("--isp_ctrl_capability--cmd:0x%x", sub_type);
				res_ptr->rtn = isp_ctrl_capability(handler_id, sub_type, param_ptr);
				pthread_mutex_lock(&isp_system_ptr->cond_mutex);
				rtn = pthread_cond_signal(&isp_system_ptr->capability_cond);
				pthread_mutex_unlock(&isp_system_ptr->cond_mutex);
				break;

			case ISP_APP_EVT_SOF:
				rtn = _isp_AppSofHandler(handler_id);
				break;

			case ISP_APP_EVT_CTRL_CALLBAC:
				rtn = _isp_AppCtrlCallbackHandler(handler_id, sub_type, param_ptr, param_len);
				break;

			default:
				ISP_LOG("--default--cmd:0x%x", sub_type);
				break;
		}

		if (0x01==isp_ctrl_msg.alloc_flag) {
			free(isp_ctrl_msg.data);
		}
		if(ISP_APP_CLOSE == _isp_AppGetStatus()) {
			break;
		}
		_isp_AppSetStatus(ISP_APP_IDLE);

	}

	ISP_LOG("exit isp app routine.");

	return NULL;

}

/* _isp_create_app_thread --
*@
*@
*@ return:
*/
static int _isp_create_app_thread(void)
{
	int rtn = ISP_APP_SUCCESS;
	uint32_t handler_id=0x00;
	struct isp_app_system* isp_system_ptr = ispAppGetSystem();
	pthread_attr_t attr;
	ISP_APP_MSG_INIT(isp_main_msg);

	rtn = _isp_msg_queue_create(ISP_APP_THREAD_QUEUE_NUM, &isp_system_ptr->app_queue);
	ISP_APP_RETURN_IF_FAIL(rtn, ("careate ctrl queue error"));

	pthread_attr_init (&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	rtn = pthread_create(&isp_system_ptr->app_thr, &attr, _isp_app_routine, NULL);
	ISP_APP_RETURN_IF_FAIL(rtn, ("careate ctrl thread error"));
	pthread_attr_destroy(&attr);

	isp_main_msg.handler_id = ISP_APP_ZERO;
	isp_main_msg.msg_type = ISP_APP_EVT_START;

	pthread_mutex_lock(&isp_system_ptr->cond_mutex);
	rtn = _isp_app_msg_post(&isp_main_msg);
	ISP_APP_RETURN_IF_FAIL(rtn, ("send msg to ctrl thread error"));
	rtn = pthread_cond_wait(&isp_system_ptr->thread_common_cond, &isp_system_ptr->cond_mutex);
	pthread_mutex_unlock(&isp_system_ptr->cond_mutex);

	return rtn;
}

/* _isp_destory_app_thread --
*@
*@
*@ return:
*/
static int _isp_destory_app_thread(void)
{
	int rtn = ISP_APP_SUCCESS;
	uint32_t handler_id=0x00;
	struct isp_app_system* isp_system_ptr = ispAppGetSystem();
	ISP_APP_MSG_INIT(isp_main_msg);

	isp_main_msg.handler_id = ISP_APP_ZERO;
	isp_main_msg.msg_type = ISP_APP_EVT_STOP;

	pthread_mutex_lock(&isp_system_ptr->cond_mutex);
	rtn = _isp_app_msg_post(&isp_main_msg);
	rtn = pthread_cond_wait(&isp_system_ptr->thread_common_cond,&isp_system_ptr->cond_mutex);
	pthread_mutex_unlock(&isp_system_ptr->cond_mutex);
	rtn = _isp_msg_queue_destroy(isp_system_ptr->app_queue);
	ISP_APP_RETURN_IF_FAIL(rtn, ("destroy ctrl queue error"));

	return rtn;
}

/* _isp_app_create_Resource --
*@
*@
*@ return:
*/
static int _isp_app_create_Resource(void)
{
	int rtn=ISP_APP_SUCCESS;
	struct isp_app_system* isp_system_ptr = ispAppGetSystem();

	pthread_mutex_init (&isp_system_ptr->cond_mutex, NULL);

	pthread_cond_init(&isp_system_ptr->init_cond, NULL);
	pthread_cond_init(&isp_system_ptr->deinit_cond, NULL);
	pthread_cond_init(&isp_system_ptr->continue_cond, NULL);
	pthread_cond_init(&isp_system_ptr->continue_stop_cond, NULL);
	pthread_cond_init(&isp_system_ptr->signal_cond, NULL);
	pthread_cond_init(&isp_system_ptr->ioctrl_cond, NULL);
	pthread_cond_init(&isp_system_ptr->capability_cond, NULL);
	pthread_cond_init(&isp_system_ptr->thread_common_cond, NULL);

	_isp_create_app_thread();

	return rtn;
}

/* _isp_DeInitResource --
*@
*@
*@ return:
*/
static int _isp_app_release_resource(void)
{
	int rtn = ISP_APP_SUCCESS;
	struct isp_app_system* isp_system_ptr = ispAppGetSystem();

	_isp_destory_app_thread();

	pthread_mutex_destroy(&isp_system_ptr->cond_mutex);

	pthread_cond_destroy(&isp_system_ptr->init_cond);
	pthread_cond_destroy(&isp_system_ptr->deinit_cond);
	pthread_cond_destroy(&isp_system_ptr->continue_cond);
	pthread_cond_destroy(&isp_system_ptr->continue_stop_cond);
	pthread_cond_destroy(&isp_system_ptr->signal_cond);
	pthread_cond_destroy(&isp_system_ptr->ioctrl_cond);
	pthread_cond_destroy(&isp_system_ptr->capability_cond);
	pthread_cond_destroy(&isp_system_ptr->thread_common_cond);

	return rtn;
}

// public

/* isp_init --
*@
*@
*@ return:
*/
int isp_init(struct isp_init_param* ptr, isp_handle* isp_handler)

{
	int rtn = ISP_APP_SUCCESS;
	uint32_t handler_id=0x00;
	struct isp_app_respond respond;
	struct isp_app_system* isp_system_ptr = NULL;
	ISP_APP_MSG_INIT(isp_main_msg);
	uint32_t hadler_id = ISP_APP_ZERO;

	ISP_LOG("---isp_app_init-- start");
	pthread_mutex_init(&s_app_mutex, NULL);
	rtn = _isp_AppLock();
	ISP_APP_RETURN_IF_FAIL(rtn, ("app lock error"));

	rtn = _isp_AppInitContext();
	ISP_APP_RETURN_IF_FAIL(rtn, ("init isp app context error"));

	isp_system_ptr = ispAppGetSystem();
	if(ISP_APP_MAX_HANDLE_NUM <= isp_system_ptr->handler_num) {
		ISP_LOG("handler num: 0x%x error", isp_system_ptr->handler_num);
		rtn = ISP_APP_ERROR;
		goto EXIT;
	}

	hadler_id = _isp_AppCreateHandler(ptr);

	if(ISP_APP_MAX_HANDLE_NUM <= hadler_id) {
		ISP_LOG("handler : 0x%x error",hadler_id);
		rtn = ISP_APP_ERROR;
		goto EXIT;
	}

	isp_system_ptr->handler_num++;

	if(ISP_APP_ONE == isp_system_ptr->handler_num) {
		rtn = _isp_app_create_Resource();
		ISP_APP_RETURN_IF_FAIL(rtn, ("create app resource error"));
	}

	respond.rtn = ISP_APP_SUCCESS;
	isp_main_msg.data = malloc(sizeof(struct isp_init_param));
	memcpy(isp_main_msg.data, ptr, sizeof(struct isp_init_param));
	isp_main_msg.alloc_flag = 1;
	isp_main_msg.handler_id = ISP_APP_ZERO;
	isp_main_msg.msg_type = ISP_APP_EVT_INIT;
	isp_main_msg.sub_msg_type = ISP_APP_ZERO;
	isp_main_msg.respond=(void*)&respond;

	pthread_mutex_lock(&isp_system_ptr->cond_mutex);

	rtn = _isp_app_msg_post(&isp_main_msg);
	ISP_APP_RETURN_IF_FAIL(rtn, ("send msg to app thread error"));

	rtn = pthread_cond_wait(&isp_system_ptr->init_cond, &isp_system_ptr->cond_mutex);
	pthread_mutex_unlock(&isp_system_ptr->cond_mutex);
	ISP_APP_RETURN_IF_FAIL(rtn, ("pthread_cond_wait error"));

	EXIT:

	rtn = _isp_AppUnlock();
	ISP_APP_RETURN_IF_FAIL(rtn, ("app unlock error"));

	ISP_LOG("---isp_app_init-- end");

	return rtn;
}

/* isp_deinit --
*@
*@
*@ return:
*/
int isp_deinit(isp_handle isp_handler)
{
	int rtn = ISP_APP_SUCCESS;
	uint32_t handler_id=0x00;
	struct isp_app_respond respond;
	struct isp_app_system* isp_system_ptr = NULL;
	ISP_APP_MSG_INIT(isp_main_msg);

	ISP_LOG("--isp_app_deinit--");

	// get mutex
	rtn = _isp_AppLock();
	ISP_APP_RETURN_IF_FAIL(rtn, ("app lock error"));
	isp_system_ptr = ispAppGetSystem();
	if (ISP_APP_ONE > isp_system_ptr->handler_num) {
		ISP_LOG("handler num: 0x%x error", isp_system_ptr->handler_num);
		rtn = ISP_APP_ERROR;
		goto EXIT;
	}

	isp_system_ptr->handler_num--;

	respond.rtn = ISP_APP_SUCCESS;
	isp_main_msg.handler_id = ISP_APP_ZERO;
	isp_main_msg.msg_type = ISP_APP_EVT_DEINIT;
	isp_main_msg.sub_msg_type = ISP_APP_ZERO;
	isp_main_msg.alloc_flag = 0x00;
	isp_main_msg.respond = (void*)&respond;

	// close hw isp
	pthread_mutex_lock(&isp_system_ptr->cond_mutex);
	rtn = _isp_app_msg_post(&isp_main_msg);
	ISP_APP_RETURN_IF_FAIL(rtn, ("send msg to ctrl thread error"));

	rtn = pthread_cond_wait(&isp_system_ptr->deinit_cond, &isp_system_ptr->cond_mutex);
	pthread_mutex_unlock(&isp_system_ptr->cond_mutex);
	ISP_APP_RETURN_IF_FAIL(rtn, ("pthread_cond_wait error"));

	if (ISP_APP_ZERO == isp_system_ptr->handler_num) {
		rtn = _isp_app_release_resource();
		ISP_APP_RETURN_IF_FAIL(rtn, ("_isp_app_release_resource error"));
	}

EXIT:

	rtn = _isp_AppUnlock();
	pthread_mutex_destroy(&s_app_mutex);
	ISP_APP_RETURN_IF_FAIL(rtn, ("app unlock error"));

	ISP_LOG("--isp_app_deinit-- end");

	return rtn;
}

/* isp_capability --
*@
*@
*@ return:
*/
int isp_capability(isp_handle isp_handler, enum isp_capbility_cmd cmd, void* param_ptr)
{
	int rtn = ISP_APP_SUCCESS;
	uint32_t handler_id=0x00;
	struct isp_app_respond respond;
	struct isp_app_system* isp_system_ptr = NULL;
	ISP_APP_MSG_INIT(isp_main_msg);

	rtn = _isp_AppLock();
	ISP_APP_RETURN_IF_FAIL(rtn, ("app lock error"));
	isp_system_ptr = ispAppGetSystem();
	respond.rtn = ISP_APP_SUCCESS;
	isp_main_msg.handler_id = ISP_APP_ZERO;
	isp_main_msg.msg_type = ISP_APP_EVT_CAPABILITY;
	isp_main_msg.sub_msg_type = cmd;
	isp_main_msg.data = (void*)param_ptr;
	isp_main_msg.alloc_flag = 0x00;
	isp_main_msg.respond = (void*)&respond;

	pthread_mutex_lock(&isp_system_ptr->cond_mutex);
	rtn = _isp_app_msg_post(&isp_main_msg);
	ISP_APP_RETURN_IF_FAIL(rtn, ("send msg to ctrl thread error"));

	rtn = pthread_cond_wait(&isp_system_ptr->capability_cond, &isp_system_ptr->cond_mutex);
	pthread_mutex_unlock(&isp_system_ptr->cond_mutex);
	ISP_APP_RETURN_IF_FAIL(rtn, ("pthread_cond_wait error"));

	rtn = _isp_AppUnlock();
	ISP_APP_RETURN_IF_FAIL(rtn, ("app unlock error"));

	ISP_APP_RETURN_IF_FAIL(respond.rtn, ("isp_capability error"));

	return rtn;
}

/* isp_ioctl --
*@
*@
*@ return:
*/
int isp_ioctl(isp_handle isp_handler, enum isp_ctrl_cmd cmd, void* param_ptr)
{
	int rtn = ISP_APP_SUCCESS;
	uint32_t handler_id=0x00;
	struct isp_app_respond respond;
	struct isp_app_system* isp_system_ptr = NULL;
	ISP_APP_MSG_INIT(isp_main_msg);

	rtn = _isp_AppLock();
	ISP_APP_RETURN_IF_FAIL(rtn, ("app lock error"));
	isp_system_ptr = ispAppGetSystem();
	respond.rtn = ISP_APP_SUCCESS;
	isp_main_msg.handler_id = ISP_APP_ZERO;
	isp_main_msg.msg_type = ISP_APP_EVT_IOCTRL;
	isp_main_msg.sub_msg_type = cmd;
	isp_main_msg.data = (void*)param_ptr;
	isp_main_msg.alloc_flag = 0x00;
	isp_main_msg.respond = (void*)&respond;

	pthread_mutex_lock(&isp_system_ptr->cond_mutex);
	rtn = _isp_app_msg_post(&isp_main_msg);
	ISP_APP_RETURN_IF_FAIL(rtn, ("send msg to ctrl thread error"));

	rtn = pthread_cond_wait(&isp_system_ptr->ioctrl_cond, &isp_system_ptr->cond_mutex);
	pthread_mutex_unlock(&isp_system_ptr->cond_mutex);
	ISP_APP_RETURN_IF_FAIL(rtn, ("pthread_cond_wait error"));

	rtn = _isp_AppUnlock();
	ISP_APP_RETURN_IF_FAIL(rtn, ("app unlock error"));

	ISP_APP_RETURN_IF_FAIL(respond.rtn, ("isp_ioctl error, cmr 0x%x, rtn 0x%x", cmd, respond.rtn));

	return rtn;
}

/* isp_video_start --
*@
*@
*@ return:
*/
int isp_video_start(isp_handle isp_handler, struct isp_video_start* param_ptr)
{
	int rtn = ISP_APP_SUCCESS;
	uint32_t handler_id=0x00;
	struct isp_app_respond respond;
	struct isp_app_system* isp_system_ptr = NULL;
	ISP_APP_MSG_INIT(isp_main_msg);

	ISP_LOG("--isp_app_video_start--");

	rtn = _isp_AppLock();
	ISP_APP_RETURN_IF_FAIL(rtn, ("app lock error"));
	isp_system_ptr = ispAppGetSystem();
	respond.rtn = ISP_APP_SUCCESS;
	isp_main_msg.data = malloc(sizeof(struct isp_video_start));
	memcpy(isp_main_msg.data, param_ptr, sizeof(struct isp_video_start));
	isp_main_msg.alloc_flag = 0x01;
	isp_main_msg.handler_id = ISP_APP_ZERO;
	isp_main_msg.msg_type = ISP_APP_EVT_CONTINUE;
	isp_main_msg.sub_msg_type = ISP_APP_ZERO;
	isp_main_msg.respond = (void*)&respond;

	pthread_mutex_lock(&isp_system_ptr->cond_mutex);

	rtn = _isp_app_msg_post(&isp_main_msg);
	ISP_APP_RETURN_IF_FAIL(rtn, ("send msg to ctrl thread error"));

	rtn = pthread_cond_wait(&isp_system_ptr->continue_cond, &isp_system_ptr->cond_mutex);
	pthread_mutex_unlock(&isp_system_ptr->cond_mutex);
	ISP_APP_RETURN_IF_FAIL(rtn, ("pthread_cond_wait error"));

	rtn = _isp_AppUnlock();
	ISP_APP_RETURN_IF_FAIL(rtn, ("app unlock error"));

	ISP_APP_RETURN_IF_FAIL(respond.rtn, ("isp_video_start error"));

	ISP_LOG("--isp_app_video_start-- end");

	return rtn;
}

/* isp_video_start --
*@
*@
*@ return:
*/
int isp_video_stop(isp_handle isp_handler)
{
	int rtn=ISP_APP_SUCCESS;
	uint32_t handler_id=0x00;
	struct isp_app_respond respond;
	struct isp_app_system* isp_system_ptr = NULL;
	ISP_APP_MSG_INIT(isp_main_msg);

	ISP_LOG("--isp_app_video_stop--");

	isp_system_ptr = ispAppGetSystem();
	if (isp_system_ptr->handler_num == ISP_APP_ZERO) { // for cts
		return rtn;
	}

	rtn = _isp_AppLock();
	ISP_APP_RETURN_IF_FAIL(rtn, ("app lock error"));
	isp_system_ptr = ispAppGetSystem();
	if (isp_system_ptr->handler_num == ISP_APP_ZERO) {
		_isp_AppUnlock();
		return rtn;
	}
	respond.rtn = ISP_APP_SUCCESS;
	isp_main_msg.handler_id = ISP_APP_ZERO;
	isp_main_msg.msg_type = ISP_APP_EVT_CONTINUE_STOP;
	isp_main_msg.sub_msg_type = ISP_APP_ZERO;
	isp_main_msg.data = NULL;
	isp_main_msg.alloc_flag = 0x00;
	isp_main_msg.respond = (void*)&respond;

	pthread_mutex_lock(&isp_system_ptr->cond_mutex);
	rtn = _isp_app_msg_post(&isp_main_msg);
	ISP_APP_RETURN_IF_FAIL(rtn, ("send msg to ctrl thread error"));

	rtn = pthread_cond_wait(&isp_system_ptr->continue_stop_cond, &isp_system_ptr->cond_mutex);
	pthread_mutex_unlock(&isp_system_ptr->cond_mutex);
	ISP_APP_RETURN_IF_FAIL(rtn, ("pthread_cond_wait error"));

	rtn = _isp_AppUnlock();
	ISP_APP_RETURN_IF_FAIL(rtn, ("app unlock error"));

	ISP_APP_RETURN_IF_FAIL(respond.rtn, ("isp_video_stop error"));

	ISP_LOG("--isp_app_video_stop--end");

	return rtn;
}

/* isp_proc_start --
*@
*@
*@ return:
*/
int isp_proc_start(isp_handle isp_handler, struct ips_in_param* in_param_ptr, struct ips_out_param* out_param_ptr)
{
	int rtn=ISP_APP_SUCCESS;
	uint32_t handler_id=0x00;
	struct isp_app_respond respond;
	ISP_APP_MSG_INIT(isp_main_msg);

	ISP_LOG("--isp_app_proc_start--");

	rtn = _isp_AppLock();
	ISP_APP_RETURN_IF_FAIL(rtn, ("app lock error"));

	respond.rtn = ISP_APP_SUCCESS;
	isp_main_msg.handler_id = ISP_APP_ZERO;
	isp_main_msg.msg_type = ISP_APP_EVT_SIGNAL;
	isp_main_msg.sub_msg_type = ISP_APP_ZERO;
	isp_main_msg.data = malloc(sizeof(struct ips_in_param));
	memcpy(isp_main_msg.data, in_param_ptr, sizeof(struct ips_in_param));
	isp_main_msg.alloc_flag = 0x01;
	isp_main_msg.respond = (void*)&respond;

	rtn = _isp_app_msg_post(&isp_main_msg);
	ISP_APP_RETURN_IF_FAIL(rtn, ("send msg to app thread error"));

	rtn = _isp_AppUnlock();
	ISP_APP_RETURN_IF_FAIL(rtn, ("app unlock error"));

	ISP_APP_RETURN_IF_FAIL(respond.rtn, ("isp_proc_start error"));

	ISP_LOG("--isp_app_proc_start--end");

	return rtn;
}

/* isp_proc_next --
*@
*@
*@ return:
*/
int isp_proc_next(isp_handle isp_handler, struct ipn_in_param* in_ptr, struct ips_out_param *out_ptr)
{
	int rtn = ISP_APP_SUCCESS;
	uint32_t handler_id=0x00;
	struct isp_app_respond respond;
	ISP_APP_MSG_INIT(isp_main_msg);

	ISP_LOG("--isp_app_proc_next--");

	rtn = _isp_AppLock();
	ISP_APP_RETURN_IF_FAIL(rtn, ("app lock error"));

	respond.rtn = ISP_APP_SUCCESS;
	isp_main_msg.handler_id = ISP_APP_ZERO;
	isp_main_msg.msg_type = ISP_APP_EVT_SIGNAL_NEXT;
	isp_main_msg.sub_msg_type = ISP_APP_ZERO;
	isp_main_msg.data = malloc(sizeof(struct ipn_in_param));
	memcpy(isp_main_msg.data, in_ptr, sizeof(struct ipn_in_param));
	isp_main_msg.alloc_flag = 0x01;
	isp_main_msg.respond = (void*)&respond;

	rtn = _isp_app_msg_post(&isp_main_msg);
	ISP_APP_RETURN_IF_FAIL(rtn, ("send msg to ctrl thread error"));

	rtn = _isp_AppUnlock();
	ISP_APP_RETURN_IF_FAIL(rtn, ("app unlock error"));

	ISP_APP_RETURN_IF_FAIL(respond.rtn, ("isp_proc_next error"));

	ISP_LOG("--isp_app_proc_next--end");

	return rtn;
}

/**---------------------------------------------------------------------------*/

