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
#ifndef _ISP_AWB_DEBUG_H_
#define _ISP_AWB_DEBUG_H_
/*------------------------------------------------------------------------------*
*				Dependencies					*
*-------------------------------------------------------------------------------*/
#ifdef WIN32
#include <stdlib.h>
#include <stdio.h>
#include "sci_types.h"
#else
#include <linux/types.h>
#include <sys/types.h>
#include <utils/Log.h>
#endif

#include <stdio.h>
/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif
/*------------------------------------------------------------------------------*
*				Micro Define					*
*-------------------------------------------------------------------------------*/
#define AWBL_DEBUG_FILE_BUF_SIZE	4096

/*------------------------------------------------------------------------------*
*				Data Structures					*
*-------------------------------------------------------------------------------*/
typedef void* debug_handle_t;

struct awbl_debug_file
{
	char		file_name[32];
	char		open_mode[4];
	char		file_buffer[AWBL_DEBUG_FILE_BUF_SIZE];
	uint32_t	buffer_size;
	uint32_t	buffer_used_size;
	FILE		*pf;
};
/*------------------------------------------------------------------------------*
*				Data Prototype					*
*-------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------*
*				Functions					*
*-------------------------------------------------------------------------------*/
/*old interface: used by alg1*/
void _debugPrintf(struct awbl_debug_file *debug_file, char *str);
void _debugOpenFile(struct awbl_debug_file *debug_file);
void _debugCloseFile(struct awbl_debug_file *debug_file);
void _initDebugFile(struct awbl_debug_file *debug_file, uint32_t level, uint32_t level_thres,
			const char *file_name, const char *open_mode);
void _deinitDebugFile(struct awbl_debug_file *debug_file, uint32_t level);

/*new interface*/
debug_handle_t debug_file_init(const char file_name[], const char open_mode[]);

void debug_file_deinit(debug_handle_t handle);

uint32_t debug_file_open(debug_handle_t handle, uint32_t debug_level, uint32_t debug_level_thres);

void debug_file_close(debug_handle_t handle);

void debug_file_print(debug_handle_t handle, char *str);
/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/
#endif
// End
