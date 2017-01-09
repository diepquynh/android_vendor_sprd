/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "isp_app.h"
#include "isp_com.h"
#include "isp_ae.h"
#include "isp_ae_ctrl.h"
#include "isp_flash_ctrl.h"

/**---------------------------------------------------------------------------*
 ** 				Compiler Flag					*
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif


static int32_t isp_ae_flash_eb(uint32_t handler_id, uint32_t eb)
{
 	int32_t rtn=ISP_SUCCESS;
 	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

 	if (!ae_param_ptr) {
 		ISP_LOG("fail");
 		return ISP_PARAM_NULL;
 	}

 	switch (ae_param_ptr->alg_id) {
 		case 0:
 		{
 			rtn = isp_ae_v00_flash(handler_id, eb);
 			break;
 		}
 		default :
 			break;
 	}

 	return rtn;
 }

static int32_t isp_ae_flash_reback_index(uint32_t handler_id)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);
	memset((void*)&ae_param_ptr->write_ctrl, 0x00, sizeof(struct isp_ae_exp_ctrl));
	if (!ae_param_ptr) {
		ISP_LOG("fail");
		return ISP_PARAM_NULL;
	}

	switch (ae_param_ptr->alg_id) {
		case 0:
		{
			rtn = isp_ae_v00_flash_reback_index(handler_id,&ae_param_ptr->cur_index, &ae_param_ptr->cur_lum);
			ae_param_ptr->write_ctrl.valid_index = ae_param_ptr->cur_index;
			isp_ae_set_index(handler_id, ae_param_ptr->cur_index);
			isp_ae_update_expos_gain(handler_id);
			_isp_ae_set_exposure_gain(handler_id);
			rtn = _isp_ae_monitor(handler_id, 0, __FUNCTION__);
			break;
		}
		default :
			break;
	}

	return rtn;
}

static int32_t isp_ae_flash_save_index(uint32_t handler_id, uint32_t cur_index, uint32_t cur_lum)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);

	if (!ae_param_ptr) {
		ISP_LOG("fail");
		return ISP_PARAM_NULL;
	}

	if (ae_param_ptr->is_save_index) {
		ae_param_ptr->is_save_index = 0;
		switch (ae_param_ptr->alg_id) {
			case 0:
			{
				rtn = isp_ae_v00_flash_save_index(handler_id, cur_index, cur_lum);
				break;
			}
			default :
				break;
		}
		ae_param_ptr->callback(handler_id, ISP_CALLBACK_EVT|ISP_FLASH_READY_CALLBACK, NULL, 0);
	}

	return rtn;
}

static uint32_t isp_ae_set_flash_exposure_gain(uint32_t handler_id)
{
	uint32_t rtn=ISP_SUCCESS;
	struct isp_ae_param* ae_param_ptr=ispGetAeContext(handler_id);
	struct isp_flash_param* flash_param_ptr;

	if (!ae_param_ptr) {
		ISP_LOG("fail");
		return ISP_PARAM_NULL;
	}
	flash_param_ptr=&ae_param_ptr->flash;

	ISP_LOG("flash_param_ptr->next_index= %d", flash_param_ptr->next_index);
	flash_param_ptr->set_awb = ISP_EB;
	isp_ae_set_index(handler_id, flash_param_ptr->next_index);
	isp_ae_update_expos_gain(handler_id);
	_isp_ae_set_exposure_gain(handler_id);
	isp_ae_set_index(handler_id, flash_param_ptr->prv_index);

	return rtn;
}

static int32_t _ispAlgIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)(), struct isp_system *isp_system_ptr)
{
	int32_t rtn = ISP_SUCCESS;
	struct isp_context* isp_context_ptr = ispGetAlgContext(handler_id);
	struct isp_alg* alg_ptr = (struct isp_alg*)param_ptr;

	ISP_LOG("--IOCtrl--ALG--:0x%x",*(uint32_t*)param_ptr);

	pthread_mutex_lock(&isp_system_ptr->ctrl_3a_mutex);

	if(ISP_UEB == isp_context_ptr->ae.back_bypass)
	{
		if((ISP_EB==isp_context_ptr->ae.bypass)
			&&(ISP_AE_BYPASS!=alg_ptr->mode))
		{
			isp_context_ptr->ae.monitor_bypass=ISP_UEB;
		}

		isp_context_ptr->ae.bypass=ISP_UEB;
		switch(alg_ptr->mode)
		{
			case ISP_ALG_NORMAL:
			{
				//isp_context_ptr->ae.alg_mode=ISP_ALG_NORMAL;
				isp_ae_set_alg(handler_id, ISP_ALG_NORMAL);
				break;
			}
			case ISP_ALG_FAST:
			{
				uint32_t quick_mode = ISP_EB;
				isp_context_ptr->tune.alg=ISP_EB;
				isp_ae_set_alg(handler_id, ISP_ALG_FAST);
				isp_context_ptr->awb.quick_mode_enable = ISP_EB;
				ISP_LOG("--IOCtrl--ALG--flash_ratio:%d, eb:%d, quick_mode=%d", alg_ptr->flash_ratio, alg_ptr->flash_eb,
						isp_context_ptr->awb.quick_mode_enable);
				//isp_context_ptr->ae.flash.ratio=alg_ptr->flash_ratio;
				isp_ae_flash_eb(handler_id, alg_ptr->flash_eb);
				isp_context_ptr->ae.monitor_bypass = ISP_UEB;
				isp_ae_set_skipnum_ext(handler_id,1);
				break;
			}
			case ISP_AE_BYPASS:
			{
				isp_context_ptr->ae.bypass=ISP_EB;
				isp_context_ptr->ae.ae_bypass=ISP_EB;
				isp_context_ptr->ae.bypass_conter=isp_context_ptr->ae.skip_frame;
				isp_context_ptr->ae.is_save_index = 1;
				isp_ae_flash_save_index(handler_id, isp_context_ptr->ae.cur_index, isp_context_ptr->ae.cur_lum);
				//ispNotifyAe(handler_id);
				break;
			}
			default :
				break;
		}
	} else {
		isp_context_ptr->ae.bypass=ISP_EB;
	}

	if(ISP_UEB == isp_context_ptr->awb.back_bypass)
	{
		if((ISP_EB==isp_context_ptr->awb.bypass)
			&&(ISP_AWB_BYPASS!=alg_ptr->mode))
		{
			isp_context_ptr->awb.monitor_bypass=ISP_UEB;
		}

		switch(alg_ptr->mode)
		{
			case ISP_ALG_NORMAL:
			{
				isp_context_ptr->awb.bypass=ISP_UEB;
				isp_context_ptr->awb.alg_mode=ISP_ALG_NORMAL;
				break;
			}
			case ISP_AWB_ALG_FAST:
			{
				isp_context_ptr->awb.bypass=ISP_UEB;
				isp_context_ptr->awb.alg_mode=ISP_ALG_FAST;
				isp_context_ptr->tune.alg=ISP_EB;
				break;
			}
			case ISP_AWB_FLASH_START_SET:
			{
				/*awb is set to quick mode*/
				if (ISP_AWB_FLASH_START_SET == alg_ptr->mode) {
					uint32_t is_flash_eb = 0;
					if (0 != isp_context_ptr->flash_lnc_index
						|| 0 != isp_context_ptr->flash_cmc_index) {
						if (NULL != param_ptr) {
							is_flash_eb = *(int32_t*)param_ptr;
						}
						isp_context_ptr->is_flash_eb = is_flash_eb;
						ISP_LOG("is_flash_eb=%d", is_flash_eb);
					} else {
						isp_context_ptr->is_flash_eb = 0;
					}
				}
				isp_context_ptr->awb.flash_awb_flag = ISP_ONE;
				isp_context_ptr->awb.prv_gain = isp_context_ptr->awb.cur_gain;
				ISP_LOG("hait: cmd:%d, (%d, %d, %d)\n", alg_ptr->mode, isp_context_ptr->awb.cur_gain.r, isp_context_ptr->awb.cur_gain.g, isp_context_ptr->awb.cur_gain.b);
			}
			break;

			case ISP_AWB_FLASH_STOP_SET:
			{
				/*awb is set to normal mode*/
				isp_ae_set_skipnum_ext(handler_id,1);
				isp_context_ptr->awb.flash_awb_flag = ISP_ZERO;
				isp_context_ptr->awb.cur_gain = isp_context_ptr->awb.prv_gain;
				isp_context_ptr->is_flash_eb = 0;
				isp_ae_flash_reback_index(handler_id);
				break;
			}
			case ISP_AWB_BYPASS:
			{
				isp_context_ptr->awb.bypass=ISP_EB;
				break;
			}
			default :
				break;
		}
	} else {
		isp_context_ptr->awb.bypass = ISP_EB;
	}
	pthread_mutex_unlock(&isp_system_ptr->ctrl_3a_mutex);

	return rtn;
}

int32_t _ispFlashEGIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)(), struct isp_system *isp_system_ptr)
{
	int32_t rtn=ISP_SUCCESS;

	ISP_LOG("--IOCtrl--FLASH_EG--");
	pthread_mutex_lock(&isp_system_ptr->ctrl_3a_mutex);

	isp_ae_set_flash_exposure_gain(handler_id);
	pthread_mutex_unlock(&isp_system_ptr->ctrl_3a_mutex);

	return rtn;
}

int32_t _ispCallAlgIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)(), struct isp_system *isp_system_ptr)
{
	int32_t rtn = ISP_SUCCESS;
	struct isp_context *isp_cxt_ptr = ispGetAlgContext(handler_id);
	lsc_adv_handle_t adv_lsc_handle = NULL;
	struct isp_alg *flash_param = (struct isp_alg*)param_ptr;
	uint32_t work_mode = flash_param->mode;
	adv_lsc_handle = isp_cxt_ptr->handle_lsc_adv;

	if (ISP_AWB_FLASH_START_SET == work_mode) {
		lsc_adv_ioctrl(adv_lsc_handle, SMART_LSC_ALG_LOCK);
	} else if (ISP_AWB_FLASH_STOP_SET == work_mode) {
		lsc_adv_ioctrl(adv_lsc_handle, SMART_LSC_ALG_UNLOCK);
	}

	rtn = _ispAlgIOCtrl(handler_id, param_ptr, call_back, isp_system_ptr);

	return rtn;
}

int32_t _ispFlashNoticeProc(uint32_t handler_id, void* param_ptr, int(*call_back)(), struct isp_system *isp_system_ptr)
{
	int32_t rtn = ISP_SUCCESS;
	struct isp_context* isp_context_ptr = ispGetAlgContext(handler_id);
	struct isp_flash_notice *flash_notice = (struct isp_flash_notice*)param_ptr;
	struct isp_alg alg_param;

	if (NULL == flash_notice) {
		ISP_LOG("$LHC:notice %p is NULL error", handler_id, flash_notice);
		return ISP_PARAM_NULL;
	}

	ISP_LOG("$LHC:mode=%d", flash_notice->mode);
	switch (flash_notice->mode) {
	case ISP_FLASH_PRE_BEFORE:
		alg_param.flash_eb = 0;
		alg_param.mode = ISP_AWB_FLASH_START_SET;
		rtn = _ispCallAlgIOCtrl(handler_id, (void *)&alg_param, call_back, isp_system_ptr);
		ISP_LOG("$LHC:ISP_AWB_FLASH_START_SET: rtn=%d", rtn);

		alg_param.flash_eb = 0x01;
		alg_param.mode = ISP_AE_BYPASS;
		rtn = _ispCallAlgIOCtrl(handler_id, (void *)&alg_param, call_back, isp_system_ptr);
		ISP_LOG("$LHC:ISP_AE_BYPASS: rtn=%d", rtn);
		ISP_LOG("hait; before_pref: ae cur_idx: %d, cur_lum: %d", isp_context_ptr->ae.cur_index, isp_context_ptr->ae.cur_lum);
		break;

	case ISP_FLASH_PRE_LIGHTING:
		alg_param.mode = ISP_ALG_FAST;
		alg_param.flash_eb = 0x01;
		/*because hardware issue high equal to low, so use hight div high */
		alg_param.flash_ratio = flash_notice->flash_ratio;
		rtn = _ispCallAlgIOCtrl(handler_id, (void *)&alg_param, call_back, isp_system_ptr);
		ISP_LOG("$LHC:ISP_FLASH_PRE_LIGHTING: rtn=%d", rtn);
		break;
	case ISP_FLASH_PRE_AFTER:
	case ISP_FLASH_MAIN_AFTER:
		{
			alg_param.flash_eb = 0;
			alg_param.mode = ISP_AWB_FLASH_STOP_SET;
			rtn = _ispCallAlgIOCtrl(handler_id, (void *)&alg_param, call_back, isp_system_ptr);
			ISP_LOG("$LHC:ISP_AWB_BYPASS: rtn=%d", rtn);
		}
		break;

	case ISP_FLASH_MAIN_BEFORE:
		rtn = _ispFlashEGIOCtrl(handler_id, 0, call_back, isp_system_ptr);
		ISP_LOG("$LHC:ISP_FLASH_MAIN_BEFORE: rtn=%d", rtn);
		break;

	case ISP_FLASH_MAIN_LIGHTING:
		break;

	default:
		break;
	}

	return rtn;
}

/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/
#ifdef	__cplusplus
}
#endif
/**---------------------------------------------------------------------------*/


