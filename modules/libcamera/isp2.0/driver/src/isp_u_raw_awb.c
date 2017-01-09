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

#define LOG_TAG "isp_u_raw_awb"

#include "isp_drv.h"

isp_s32 isp_u_raw_awbm_statistics(isp_handle handle,
		struct isp_raw_awbm_statistics *awb_info)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_raw_awbm_statistics *awbm_statistics = NULL;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	if (!awb_info) {
		ISP_LOGE("awb_info is null error.");
		return -1;
	}

	awbm_statistics = (struct isp_raw_awbm_statistics *)malloc(sizeof(struct isp_raw_awbm_statistics) * ISP_RAW_AWBM_ITEM);
	if (!awbm_statistics) {
		ISP_LOGE("No mem");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_AWB;
	param.property = ISP_PRO_AWBM_STATISTICS;
	memset(awbm_statistics, 0x00, sizeof(struct isp_raw_awbm_statistics) * ISP_RAW_AWBM_ITEM);
	param.property_param = awbm_statistics;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (0 == ret) {
		memcpy((void *)awb_info, (void *)awbm_statistics, sizeof(struct isp_raw_awbm_statistics) * ISP_RAW_AWBM_ITEM);
	} else {
		ISP_LOGE("copy awbm info error.");
	}

	if (awbm_statistics) {
		free(awbm_statistics);
		awbm_statistics = NULL;
	}

	return ret;
}
