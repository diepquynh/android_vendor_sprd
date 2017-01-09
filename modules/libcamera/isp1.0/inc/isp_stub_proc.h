/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef ISP_STUB_H
#define ISP_STUB_H

#ifdef __cplusplus
extern "C"
{
#endif


#include <stdio.h>
#include <string.h>
#include "isp_stub_msg.h"


enum stub_thread_id_e {
     THREAD_0 = 0,
     THREAD_1,
     THREAD_2,
     THREAD_3,
     THREAD_4,
     THREAD_5,
     THREAD_ID_MAX
   };

typedef int (*proc_func)(void* param);
typedef void (*proc_cb)(int evt, void* param);


int isp_stub_process(uint32_t		thread_id,
		     proc_func		func_ptr,
		     proc_cb		cb,
		     uint32_t		is_need_destory,
		     void		*param);



#ifdef __cplusplus
}
#endif

#endif /* ISP_STUB_H */

