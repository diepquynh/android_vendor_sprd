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
 #ifndef _ISP_SMART_LSC_H_
 #define _ISP_SMART_LSC_H_
#include <sys/types.h>
#ifdef __cplusplus
extern "C"
{
#endif

 #define SMART_LSC_EB                     1
 #define SMART_LSC_ALG_ID              0
 #define SMART_LSC_DEBUG_LEVEL    1
 #define SMART_LSC_PARAM_LEVEL   5
int32_t isp_smart_lsc_init(uint32_t handler_id);
int32_t isp_smart_lsc_calc(uint32_t handler_id);
int32_t isp_smart_lsc_reload(uint32_t handler_id);
int32_t isp_smart_lsc_deinit(uint32_t  handler_id);

#ifdef __cplusplus
}
#endif
 #endif