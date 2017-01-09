/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _AEM_BINNING_H_
#define _AEM_BINNING_H_

#include "isp_type.h"
#include "isp_log.h"

#ifdef __cplusplus
extern "C"
{
#endif

uint32_t aem_binning(
	uint16_t bayermode,   //bayer mode of the source raw image
	uint16_t width,       //image width of the source raw image
	uint16_t height,      //image height
	uint16_t *raw_in,     //raw image's data pointer
	uint16_t start_x,     // start offset x corr
	uint16_t start_y,     //start offset y corr
	uint16_t blk_width,   //block width of an aem window
	uint16_t blk_height,  //block height
	uint32_t *aem_out     //pointer of the aem output
);


#ifdef __cplusplus
}
#endif

#endif

