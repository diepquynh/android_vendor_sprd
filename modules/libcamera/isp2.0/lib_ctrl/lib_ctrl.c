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

#include "isp_com.h"
#include "awb_ctrl.h"
#include "ae_sprd_ctrl.h"
#include "af_ctrl.h"
#include "awb_sprd_ctrl.h"
#include "sensor_raw.h"
#include "isp_log.h"
#include "ae_log.h"
#include "af_log.h"
#include "lib_ctrl.h"
#include "sprd_af_ctrl.h"
#include "sp_af_ctrl.h"
#include <dlfcn.h>
#include "af_dummy_ctrl.h"
#include "ae_dummy_ctrl.h"
#include "awb_dummy_ctrl.h"

struct awb_lib_fun awb_lib_fun;
struct ae_lib_fun ae_lib_fun;
struct af_lib_fun af_lib_fun;
extern struct al_awb_thirdlib_fun al_awb_thirdlib_fun;

uint32_t isp_awblib_init(struct sensor_libuse_info* libuse_info, struct awb_lib_fun* awb_lib_fun)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	struct third_lib_info 	awb_lib_info;
	uint32_t awb_producer_id  	= 0;
	uint32_t awb_lib_version  	= 0;
	uint32_t al_awb_lib_version  	= 0;

	ISP_LOGE("E");
	if (libuse_info) {
		awb_lib_info = libuse_info->awb_lib_info;
		awb_producer_id = awb_lib_info.product_id;
		awb_lib_version = awb_lib_info.version_id;
		ISP_LOGE("awb_producer_id= ____0x%x", awb_producer_id);
		ISP_LOGE("awb_lib_version= ____0x%x", awb_lib_version);
	}

	switch (awb_producer_id)
	{
	case SPRD_AWB_LIB:
		switch (awb_lib_version)
		{
		case AWB_LIB_VERSION_0:
			sprd_awb_fun_init();
			break;
		case AWB_LIB_VERSION_1:
		default :
			AWB_CTRL_LOGE("awb invalid lib version = 0x%x", awb_lib_version);
			rtn = AWB_CTRL_ERROR;
		}
		break;

#ifdef TARGET_BOARD_USE_THIRD_AWB_LIB_A
	case AL_AWB_LIB:
		AWB_CTRL_LOGE("in TARGET_BOARD_USE_THIRD_AWB_LIB_A\n");
		al_awb_lib_version = awb_lib_version;
		al_awb_lib_open(al_awb_lib_version);
		alc_awb_fun_init();
		break;
#else
	case AL_AWB_LIB:
		AWB_CTRL_LOGE("out TARGET_BOARD_USE_THIRD_AWB_LIB_A\n");
		dummy_awb_fun_init();
		break;
#endif
	default:
		AWB_CTRL_LOGE("awb invalid lib producer id = 0x%x", awb_producer_id);
		rtn = AWB_CTRL_ERROR;
	}

	return rtn;
}

uint32_t isp_aelib_init(struct sensor_libuse_info* libuse_info, struct ae_lib_fun* ae_lib_fun)
{
	uint32_t rtn = AE_SUCCESS;
	struct third_lib_info 	ae_lib_info;
	uint32_t ae_producer_id  	= 0;
	uint32_t ae_lib_version  	= 0;

	AE_LOGE("E");
	if (libuse_info) {
		ae_lib_info  = libuse_info->ae_lib_info;
		ae_producer_id = ae_lib_info.product_id;
		ae_lib_version = ae_lib_info.version_id;
		AE_LOGE("ae_producer_id= ____0x%x", ae_producer_id);
	}

	ae_lib_fun->product_id = ae_producer_id;
	ae_lib_fun->version_id = ae_lib_version;

	switch (ae_producer_id)
	{
	case SPRD_AE_LIB:
		switch (ae_lib_version)
		{
		case AE_LIB_VERSION_0:
			sprd_ae_fun_init();
			break;
		case AE_LIB_VERSION_1:
		default :
			AE_LOGE("ae invalid lib version = 0x%x", ae_lib_version);
			rtn = AE_ERROR;
		}
		break;
#ifdef TARGET_BOARD_USE_ALC_AE_AWB
	case AL_AE_LIB:
		switch (ae_lib_version)	{
		case AE_LIB_VERSION_0:
			alc_ae_fun_init();
			break;
		default:
			AE_LOGE("ae invalid lib version = 0x%x", ae_lib_version);
			rtn = AE_ERROR;
			break;
		}
		break;
#else	
	case AL_AE_LIB:
		switch (ae_lib_version)	{
		case AE_LIB_VERSION_0:
			dummy_ae_fun_init();
			break;
		default:
			AE_LOGE("ae invalid lib version = 0x%x", ae_lib_version);
			rtn = AE_ERROR;
			break;
		}
		break;
#endif
	default:
		AE_LOGE("ae invalid lib producer id = 0x%x", ae_producer_id);
		rtn = AE_ERROR;
	}

	return rtn;
}

uint32_t isp_aflib_init(struct sensor_libuse_info* libuse_info, struct af_lib_fun* af_lib_fun)
{
	uint32_t rtn = AF_SUCCESS;
	struct third_lib_info af_lib_info;
	uint32_t af_producer_id  	= 0;
	uint32_t af_lib_version  	= 0;

	AF_LOGE("E");
	if (libuse_info) {
		af_lib_info  = libuse_info->af_lib_info;
		af_producer_id = af_lib_info.product_id;
		af_lib_version = af_lib_info.version_id;
		AF_LOGE("af_producer_id= ____0x%x", af_producer_id);
	}
	AF_LOGE("af_producer_id= ____0x%x", af_producer_id);
	memset(af_lib_fun,0,sizeof(struct af_lib_fun));
	switch (af_producer_id)
	{
	case SPRD_AF_LIB:
		switch (af_lib_version)
		{
		case AF_LIB_VERSION_0:
			sprd_af_fun_init();
			break;
		case AF_LIB_VERSION_1:
		default :
			AF_LOGE("af invalid lib version = 0x%x", af_lib_version);
			rtn = AF_ERROR;
		}
		break;
	case SFT_AF_LIB:
		switch (af_lib_version)
		{
		case AF_LIB_VERSION_0:
			sft_af_fun_init();
			break;
		case AF_LIB_VERSION_1:
		default :
			AF_LOGE("af invalid lib version = 0x%x", af_lib_version);
			rtn = AF_ERROR;
		}
		break;
#ifdef TARGET_BOARD_USE_THIRD_AF_LIB_A
	case ALC_AF_LIB:
		switch (af_lib_version)
		{
		case AF_LIB_VERSION_0:
			alc_af_fun_init();
		case AF_LIB_VERSION_1:
		default :
			AF_LOGE("af invalid lib version = 0x%x", af_lib_version);
		}
		break;
#else
	case ALC_AF_LIB:
		switch (af_lib_version)
		{
		case AF_LIB_VERSION_0:
			dummy_af_fun_init();
			break;
		case AF_LIB_VERSION_1:
		default :
			AF_LOGE("af invalid lib version = 0x%x", af_lib_version);
		}
		break;
#endif
	default:
		AF_LOGE("af invalid lib producer id = 0x%x", af_producer_id);
		rtn = AF_ERROR;
	}

	return rtn;
}

uint32_t aaa_lib_init(isp_ctrl_context* handle, struct sensor_libuse_info* libuse_info)
{
	memset(&awb_lib_fun, 0x00, sizeof(awb_lib_fun));
	memset(&ae_lib_fun, 0x00, sizeof(ae_lib_fun));
	memset(&af_lib_fun, 0x00, sizeof(af_lib_fun));

	isp_awblib_init(libuse_info, &awb_lib_fun);
	handle->awb_lib_fun = &awb_lib_fun;
	isp_aelib_init(libuse_info, &ae_lib_fun);
	handle->ae_lib_fun = &ae_lib_fun;
	isp_aflib_init(libuse_info, &af_lib_fun);
	handle->af_lib_fun = &af_lib_fun;

	return 0;
}
