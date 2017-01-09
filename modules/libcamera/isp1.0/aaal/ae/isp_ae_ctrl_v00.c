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

#include "isp_com.h"
#include "isp_ae_alg_v00.h"

/**---------------------------------------------------------------------------*
 ** 				Compiler Flag					*
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif

/**---------------------------------------------------------------------------*
**				Micro Define					*
**----------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
**				Data Structures					*
**---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
**				extend Variables and function			*
**---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
**				Local Variables					*
**---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
**				Constant Variables					*
**---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
** 				Local Function Prototypes				*
**---------------------------------------------------------------------------*/

/* isp_ae_v00_init_context --
*@
*@
*@ return:
*/
uint32_t isp_ae_v00_init_context(uint32_t handler_id, void* param_ptr)
{
	uint32_t rtn=ISP_SUCCESS;

	rtn = ispSetAeV00Context(handler_id, param_ptr);

	return rtn;
}

/* isp_ae_v00_init --
*@
*@
*@ return:
*/
uint32_t isp_ae_v00_init(uint32_t handler_id, void* param_ptr)
{
	uint32_t rtn=ISP_SUCCESS;
	struct isp_ae_v00_context* ae_context_ptr=ispGetAeV00Context(handler_id);

	param_ptr = param_ptr;
	ae_context_ptr->alg_mode = ISP_ALG_FAST;
	memset((void*)&ae_context_ptr->exp_info, 0x00, sizeof(struct isp_ae_v00_exp_info));

	return rtn;
}

/* isp_ae_v00_deinit --
*@
*@
*@ return:
*/
uint32_t isp_ae_v00_deinit(uint32_t handler_id, void* param_ptr)
{
	uint32_t rtn=ISP_SUCCESS;
	handler_id = handler_id;
	param_ptr = param_ptr;

	return rtn;
}

static uint32_t last_pre_flash_lum = 0;
/* isp_ae_v00_calculation --
*@
*@
*@ return:
*/
uint32_t isp_ae_v00_calculation(uint32_t handler_id, void* in_param_ptr, void* out_param_ptr)
{
	uint32_t rtn=ISP_SUCCESS;
	struct isp_ae_v00_context* ae_context_ptr=ispGetAeV00Context(handler_id);
	struct isp_ae_v00_calc_param* calc_ptr=in_param_ptr;
	struct isp_ae_v00_calc_out_param* calc_out_ptr = (struct isp_ae_v00_calc_out_param*)out_param_ptr;
	//int32_t cur_index=ae_context_ptr->cur_index;
	int32_t prv_index = calc_ptr->valid_index;
	int32_t cur_index = calc_ptr->valid_index;
	int32_t max_index=ae_context_ptr->max_index;
	uint32_t target_lum=ae_context_ptr->target_lum;
	uint32_t wDeadZone=ae_context_ptr->target_zone;
	int32_t EVCompensation=ae_context_ptr->ev;
	uint32_t wYlayer=calc_ptr->cur_lum;
	uint32_t exposure=0x00;
	uint32_t smooth_flag=0x00;
	uint32_t i = 0;
	static uint32_t s_fastae_times = 0;

	calc_out_ptr->fast_end = ISP_EB;
	target_lum+=EVCompensation;

	if((ISP_ALG_FAST==ae_context_ptr->alg_mode)
		&&(0x01==ae_context_ptr->quick_mode)) {
		//ISP_LOG("ISP_ALG_FAST");
		rtn=isp_ae_succesive_fast(handler_id, wYlayer, target_lum, wDeadZone, &cur_index);
		if ( ISP_SUCCESS != rtn) {
			calc_out_ptr->fast_end = ISP_UEB;
		} else {
			ae_context_ptr->ae_fast_callback(handler_id, rtn);
		}
		last_pre_flash_lum = wYlayer;
		rtn=ISP_SUCCESS;
	}
	else if((ISP_EB==ae_context_ptr->flash_eb)
		&&(ISP_ALG_NORMAL==ae_context_ptr->alg_mode)) {
		ISP_LOG("HAIT ISP_ALG_NORMAL target_lum, wDeadZone,wYlayer %d %d %d", target_lum, wDeadZone,wYlayer);
		s_fastae_times++;
		if (((target_lum + wDeadZone) > wYlayer) && (wYlayer > (target_lum - wDeadZone))) {
			ae_context_ptr->flash.target_lum = target_lum;
			ae_context_ptr->flash.target_zone = wDeadZone;
			ae_context_ptr->flash.cur_lum = wYlayer;
			ae_context_ptr->flash.cur_index = prv_index;
			ae_context_ptr->flash_calc(handler_id, &ae_context_ptr->flash);
			ae_context_ptr->ae_fast_callback(handler_id, rtn);
			ae_context_ptr->flash_eb=ISP_UEB;
			s_fastae_times = 0;
		} else {
			ISP_LOG("last_pre_flash_lum %d wYlayer %d", last_pre_flash_lum, wYlayer);
			if ((3 >= s_fastae_times) && (abs(wYlayer - last_pre_flash_lum) >= 2)) {
				rtn=isp_ae_succesive_fast(handler_id, wYlayer, target_lum, wDeadZone, &cur_index);
				last_pre_flash_lum = wYlayer;
				rtn=ISP_SUCCESS;
			} else {
				ae_context_ptr->flash.target_lum = target_lum;
				ae_context_ptr->flash.target_zone = wDeadZone;
				ae_context_ptr->flash.cur_lum = wYlayer;
				ae_context_ptr->flash.cur_index = prv_index;
				ae_context_ptr->flash_calc(handler_id, &ae_context_ptr->flash);
				ae_context_ptr->ae_fast_callback(handler_id, rtn);
				ae_context_ptr->flash_eb=ISP_UEB;
				s_fastae_times = 0;
			}
		}
	} else {
		//ISP_LOG("ISP_ALG_SUCCESIVE");
		isp_ae_succesive_calculation(handler_id, wYlayer, target_lum, wDeadZone, &cur_index);
		smooth_flag = 1;
	}

	//ISP_LOG("AE_TEST: index %d next index:%d, wYlayer %d", ae_context_ptr->cur_index, cur_index, wYlayer);

	isp_ae_index_smooth(handler_id, cur_index, &calc_out_ptr->exp, smooth_flag, wYlayer);

	ae_context_ptr->cur_index = cur_index;
	calc_out_ptr->cur_index = cur_index;
	calc_out_ptr->cur_ev = EVCompensation;

	EXIT:

	return rtn;
}


/* isp_ae_v00_change_expos_gain --
*@
*@
*@ return:
*/
uint32_t isp_ae_v00_change_expos_gain(uint32_t handler_id, uint32_t shutter, uint32_t dummy, uint32_t again, uint32_t dgain)
{
	uint32_t rtn=ISP_SUCCESS;
	struct isp_ae_v00_context* ae_context_ptr=ispGetAeV00Context(handler_id);

	dgain = dgain;
	ae_context_ptr->cur_gain=again;
	ae_context_ptr->cur_exposure=shutter;
	ae_context_ptr->cur_dummy=dummy;

	return rtn;
}

/* isp_ae_v00_update_expos_gain --
*@
*@
*@ return:
*/
uint32_t isp_ae_v00_update_expos_gain(uint32_t handler_id)
{
	uint32_t rtn=ISP_SUCCESS;

	isp_ae_calc_exposure_gain(handler_id);

	return rtn;
}

/* isp_ae_v00_set_exposure --
*@
*@
*@ return:
*/
int32_t isp_ae_v00_set_exposure(uint32_t handler_id)
{
	uint32_t rtn=ISP_SUCCESS;
	struct isp_ae_v00_context* ae_context_ptr=ispGetAeV00Context(handler_id);
	uint32_t ae_param=0x00;

	if(PNULL!=ae_context_ptr->set_exposure) {
		ae_param=(ae_context_ptr->cur_dummy<<0x10)&0x0fff0000;
		ae_param|=ae_context_ptr->cur_exposure&0x0000ffff;
		ae_context_ptr->set_exposure(handler_id, ae_context_ptr->cur_exposure, ae_context_ptr->cur_dummy);
	} else {
		ISP_LOG("write_exposure_gain_value null error");
	}

	return rtn;
}

/* isp_ae_v00_set_gain --
*@
*@
*@ return:
*/
int32_t isp_ae_v00_set_gain(uint32_t handler_id)
{
	uint32_t rtn=ISP_SUCCESS;
	struct isp_ae_v00_context* ae_context_ptr=ispGetAeV00Context(handler_id);

	if(PNULL!=ae_context_ptr->set_gain) {
		ae_context_ptr->set_gain(handler_id, ae_context_ptr->cur_gain);
	} else {
		ISP_LOG("write_exposure_gain_value null error");
	}

	return rtn;
}

/* isp_ae_v00_flash --
*@
*@
*@ return:
*/
int32_t isp_ae_v00_flash(uint32_t handler_id, uint32_t eb)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_v00_context* ae_context_ptr=ispGetAeV00Context(handler_id);

	ae_context_ptr->flash_eb = eb;

	return rtn;
}

int32_t isp_ae_v00_flash_reback_index(uint32_t handler_id, uint32_t *cur_index, uint32_t *cur_lum)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_v00_context* ae_context_ptr=ispGetAeV00Context(handler_id);

	*cur_index = ae_context_ptr->flash.prv_index;
	*cur_lum = ae_context_ptr->flash.prv_lum;
	ISP_LOG("cur_index=%d, cur_lum=%d", *cur_index, *cur_lum);

	return rtn;
}

int32_t isp_ae_v00_flash_save_index(uint32_t handler_id, uint32_t cur_index, uint32_t cur_lum)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_v00_context* ae_context_ptr=ispGetAeV00Context(handler_id);
	uint32_t exp = 0;
	uint32_t gain = 0;

	ae_context_ptr->flash.prv_index = cur_index;
	ae_context_ptr->flash.prv_lum = cur_lum;
	ISP_LOG("prv_index=%d, prv_lum=%d", ae_context_ptr->flash.prv_index, ae_context_ptr->flash.prv_lum);

	exp = ae_context_ptr->e_ptr->tab[ae_context_ptr->flash.prv_index].exp;
	gain = ae_context_ptr->e_ptr->tab[ae_context_ptr->flash.prv_index].gain;

	return rtn;
}

/* isp_ae_v00_set_alg --
*@
*@
*@ return:
*/
int32_t isp_ae_v00_set_alg(uint32_t handler_id, uint32_t mode)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_v00_context* ae_context_ptr=ispGetAeV00Context(handler_id);

	ae_context_ptr->alg_mode= mode;

	return rtn;
}

/* isp_ae_v00_set_ev --
*@
*@
*@ return:
*/
int32_t isp_ae_v00_set_ev(uint32_t handler_id, int32_t ev)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_v00_context* ae_context_ptr=ispGetAeV00Context(handler_id);

	ae_context_ptr->ev = ev;

	return rtn;
}

/* isp_ae_v00_set_index --
*@
*@
*@ return:
*/
int32_t isp_ae_v00_set_index(uint32_t handler_id, uint32_t index)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_v00_context* ae_context_ptr=ispGetAeV00Context(handler_id);

	ae_context_ptr->cur_index = index;

	return rtn;
}

/* isp_ae_v00_set_frame_info --
*@
*@
*@ return:
*/
int32_t isp_ae_v00_set_frame_info(uint32_t handler_id, struct isp_ae_v00_frame_info* param_ptr)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_ae_v00_context* ae_context_ptr=ispGetAeV00Context(handler_id);

	ae_context_ptr->weight_ptr = param_ptr->weight_ptr;
	ae_context_ptr->fix_fps = param_ptr->fix_fps;
	ae_context_ptr->frame_line = param_ptr->frame_line;
	ae_context_ptr->line_time = param_ptr->line_time;
	ae_context_ptr->e_ptr = param_ptr->e_ptr;
	ae_context_ptr->g_ptr = param_ptr->g_ptr;
	ae_context_ptr->max_index = param_ptr->max_index;
	ae_context_ptr->min_index = param_ptr->min_index;
	ae_context_ptr->min_frame_line = param_ptr->min_frame_line;
	ae_context_ptr->max_frame_line = param_ptr->max_frame_line;

	return rtn;
}

/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/
#ifdef	__cplusplus
}
#endif
/**---------------------------------------------------------------------------*/


