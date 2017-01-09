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
 #ifndef _AE_DEBUG_INFO_PARSER_H_
 #define _AE_DEBUG_INFO_PARSER_H_

#include "ae_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ae_debug_info_packet_in {
	char id[32];
	void* aem_stats;
	void* alg_status;
	void* alg_results;
	void *packet_buf;
};

struct ae_debug_info_packet_out {
	uint32_t size;
};


struct ae_debug_info_unpacket_in {
	char alg_id[32];
	void *packet_buf;
	uint32_t packet_len;
	void* unpacket_buf;
	uint32_t unpacket_len;
};

struct ae_debug_info_unpacket_out {
	uint32_t reserved;
};

int32_t ae_debug_info_packet(void* input, void* output);
int32_t ae_debug_info_unpacket(void* input, void* output);
void* ae_debug_info_get_lib_version(void);
#ifdef __cplusplus
}
#endif

#endif//_AE_DEBUG_INFO_PARSER_H_