#ifndef _DENOISE_APP_H_
#define _DENOISE_APP_H_

#define  TEST_ENV

#ifdef  TEST_ENV
//#include "isp_data_types.h"
#else
#include <utils/Log.h>
#endif

#ifdef TEST_ENV
#define DENOISE_LOG		printf
#else
#define DENOISE_LOG		ALOGE
#endif

#if 0
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef signed int int32_t;
typedef signed short int16_t;
typedef signed char int8_t;

typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;
typedef signed int int32;
typedef signed short int16;
typedef signed char int8;
#endif

struct uv_denoise {
	int32_t max_6_delat;
	int32_t max_4_delat;
	int32_t max_2_delat;
};

struct isp_denoise_input{
	uint32_t InputHeight;
	uint32_t InputWidth;
	unsigned long InputAddr;
};


struct uv_denoise_param0{
	int8_t *dst_uv_image;
	int8_t *src_uv_image;
	uint32_t in_width;
	uint32_t in_height;
	uint32_t out_width;
	uint32_t out_height;
	int32_t max_6_delta;
	int32_t max_4_delta;
	int32_t max_2_delta;
	int32_t task_no;
};



void uv_proc_cb(int evt, void* param);
int cpu_hotplug_disable(uint8_t is_disable);
void isp_uv_denoise(struct isp_denoise_input *uv_denoise_in , uint32_t alg_num);

#endif
