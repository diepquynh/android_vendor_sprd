#include <sys/types.h>
#include <utils/Log.h>
#include <utils/Timers.h>
#include <time.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <time.h>
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <arm_neon.h>
#include "denoise.h"
#include "isp_stub_proc.h"
#include "denoise_app.h"
#include "isp_app.h"
static sem_t denoise_sem_lock;

#define DISABLE_CPU_HOTPLUG	"1"
#define ENABLE_CPU_HOTPLUG	"0"

#define PIXEL_STRIDE	2		//uv interleave
#define UNUSED(x) (void)(x)

void add_border_uv(uint8_t *dst, uint8_t *src, uint32_t w, uint32_t h, uint32_t border_w, uint32_t border_h);

void isp_uv_denoise(struct isp_denoise_input* uv_denoise_in , uint32_t alg_num)
{
#ifdef CONFIG_CAMERA_UV_DENOISE
	int ret = 0;
	//struct img_addr den_out;
	int8_t *cnr_in = NULL;
	int8_t *cnr_out = NULL;

	int8_t *ext_src = NULL;
	uint32_t ext_w = 0;
	uint32_t ext_h = 0;
	uint32_t line_stride = 0;

	struct uv_denoise uv_denoise_param;
	struct uv_denoise_param0 uv_param1;
	struct uv_denoise_param0 uv_param2;
	struct uv_denoise_param0 uv_param3;
	struct uv_denoise_param0 uv_param4;

	struct uv_denoise_param0 *uv_param1_ptr = &uv_param1;
	struct uv_denoise_param0 *uv_param2_ptr = &uv_param2;
	struct uv_denoise_param0 *uv_param3_ptr = &uv_param3;
	struct uv_denoise_param0 *uv_param4_ptr = &uv_param4;
	uint32_t part0_h = 0;
	uint32_t part1_h = 0;
	uint32_t part2_h = 0;
	uint32_t part3_h = 0;
	uint32_t SrcOffsetOne = 0;
	uint32_t SrcOffsetTwo = 0;
	uint32_t SrcOffsetThr = 0;
	uint32_t SrcOffsetFour = 0;
	uint32_t DstOffsetOne = 0;
	uint32_t DstOffsetTwo = 0;
	uint32_t DstOffsetThr = 0;
	uint32_t DstOffsetFour = 0;
	uint32_t denoise_level[2] = {0};
	uint32_t y_denoise_level = 0;
	uint32_t uv_denoise_level = 0;
	uint32_t max6delta = 0;
	uint32_t max4delta = 0;
	uint32_t max2delta = 0;

	int8_t *task1src = NULL;
	int8_t *task1dst = NULL;
	int8_t *task2src = NULL;
	int8_t *task2dst = NULL;
	int8_t *task3src = NULL;
	int8_t *task3dst = NULL;
	int8_t *task4src = NULL;
	int8_t *task4dst = NULL;

	ret = cpu_hotplug_disable(1);
	if(0 != ret) {
		ALOGE("Failed to disable cpu_hotplug, directly return!");
		return;
	}

	cnr_out = (int8_t *)malloc((uv_denoise_in->InputHeight * uv_denoise_in->InputWidth)>>1);
	if (NULL == cnr_out) {
		ALOGE("failed to allocate memory");
		goto EXIT;
	}

	cnr_in = (int8_t *)uv_denoise_in->InputAddr;

	if(0 == alg_num)
	{
		ext_w = uv_denoise_in->InputWidth / 2 + 24;
		ext_h = uv_denoise_in->InputHeight / 2 + 24;
		line_stride = ext_w * 2;
		ext_src = (int8_t *)malloc(ext_w * ext_h *2);
		if (NULL == ext_src)
		{
			ALOGE("allocate extend buffer failed!");
			goto EXIT;
		}

		add_border_uv((uint8_t *)ext_src, (uint8_t *)cnr_in,uv_denoise_in->InputWidth/2,uv_denoise_in->InputHeight/2,12,12);

	}

	isp_capability(NULL, ISP_DENOISE_INFO, (void*)denoise_level);
	y_denoise_level = denoise_level[0];
	uv_denoise_level = denoise_level[1];

	if (uv_denoise_level > 36) {
		max6delta = uv_denoise_level;
		max4delta = uv_denoise_level;
		max2delta = uv_denoise_level;
	} else {
		if (uv_denoise_level < 9) {
			uv_denoise_level = 9;
		}
		max6delta = uv_denoise_level;
		max4delta = uv_denoise_level*4/6;
		max2delta = uv_denoise_level*2/6;
	}

	ALOGE("isp_uv_denoise, uv_denoise_level=%d (%d, %d, %d)", uv_denoise_level, max6delta, max4delta, max2delta);

	part0_h = uv_denoise_in->InputHeight / 4;
	part1_h = part0_h;
	part2_h = part0_h;
	part3_h = uv_denoise_in->InputHeight - 3 * part0_h;

	if (0 == alg_num) {

		SrcOffsetOne = 0;
		DstOffsetOne = 0;

		SrcOffsetTwo = part0_h / 2 * line_stride * sizeof(int8_t);
		DstOffsetTwo = part0_h / 2 * uv_denoise_in->InputWidth* sizeof(int8_t);

		SrcOffsetThr = part0_h * line_stride * sizeof(int8_t);
		DstOffsetThr = part0_h * uv_denoise_in->InputWidth* sizeof(int8_t);

		SrcOffsetFour = 3 * part0_h /2 * line_stride * sizeof(int8_t);
		DstOffsetFour = 3 * part0_h / 2 * uv_denoise_in->InputWidth* sizeof(int8_t);

			/* uv denoise level*/
		uv_param1.dst_uv_image = cnr_out+DstOffsetOne;
		uv_param1.src_uv_image = ext_src+SrcOffsetOne;
		uv_param1.in_width = line_stride;
		uv_param1.in_height = part0_h+48;
		uv_param1.out_width = 0;
		uv_param1.out_height = 0;
		uv_param1.max_6_delta = max6delta;
		uv_param1.max_4_delta = max4delta;
		uv_param1.max_2_delta = max2delta;
		uv_param1.task_no = 1;

		uv_param2.dst_uv_image = cnr_out+DstOffsetTwo;
		uv_param2.src_uv_image = ext_src+SrcOffsetTwo;
		uv_param2.in_width = line_stride;
		uv_param2.in_height = part1_h+48;
		uv_param2.out_width = 0;
		uv_param2.out_height = 0;
		uv_param2.max_6_delta = max6delta;
		uv_param2.max_4_delta = max4delta;
		uv_param2.max_2_delta = max2delta;
		uv_param2.task_no = 2;

		uv_param3.dst_uv_image = cnr_out+DstOffsetThr;
		uv_param3.src_uv_image = ext_src+SrcOffsetThr;
		uv_param3.in_width = line_stride;
		uv_param3.in_height = part2_h+48;
		uv_param3.out_width = 0;
		uv_param3.out_height = 0;
		uv_param3.max_6_delta = max6delta;
		uv_param3.max_4_delta = max4delta;
		uv_param3.max_2_delta = max2delta;
		uv_param3.task_no = 3;

		uv_param4.dst_uv_image = cnr_out+DstOffsetFour;
		uv_param4.src_uv_image = ext_src+SrcOffsetFour;
		uv_param4.in_width = line_stride;
		uv_param4.in_height = part3_h+48;
		uv_param4.out_width = 0;
		uv_param4.out_height = 0;
		uv_param4.max_6_delta = max6delta;
		uv_param4.max_4_delta = max4delta;
		uv_param4.max_2_delta = max2delta;
		uv_param4.task_no = 4;
		sem_init(&denoise_sem_lock, 0, 0);

	 	isp_stub_process(THREAD_0,
						uv_proc_func_neon0,
						uv_proc_cb,
						0,
						(void*)uv_param1_ptr);

		isp_stub_process(THREAD_1,
						uv_proc_func_neon0,
						uv_proc_cb,
						0,
						(void*)uv_param2_ptr);

		isp_stub_process(THREAD_2,
						uv_proc_func_neon0,
						uv_proc_cb,
						0,
						(void*)uv_param3_ptr);

		isp_stub_process(THREAD_3,
						uv_proc_func_neon0,
						uv_proc_cb,
						0,
						(void*)uv_param4_ptr);

	} else if(1 == alg_num) {

		SrcOffsetOne = 0;
		DstOffsetOne = 0;

		SrcOffsetTwo = (part0_h / 2 - 2) * uv_denoise_in->InputWidth* sizeof(int8_t);
		DstOffsetTwo = part0_h / 2 * uv_denoise_in->InputWidth* sizeof(int8_t);

		SrcOffsetThr = (part0_h - 2) * uv_denoise_in->InputWidth* sizeof(int8_t);
		DstOffsetThr = part0_h * uv_denoise_in->InputWidth* sizeof(int8_t);

		SrcOffsetFour = (3 * part0_h /2 - 2) * uv_denoise_in->InputWidth* sizeof(int8_t);
		DstOffsetFour = 3 * part0_h / 2 * uv_denoise_in->InputWidth* sizeof(int8_t);

			/* uv denoise level*/
		uv_param1.dst_uv_image = cnr_out+DstOffsetOne;
		uv_param1.src_uv_image = cnr_in+SrcOffsetOne;
		uv_param1.in_width = uv_denoise_in->InputWidth;
		uv_param1.in_height = part0_h+4;
		uv_param1.out_width = 0;
		uv_param1.out_height = 0;
		uv_param1.max_6_delta = max6delta;
		uv_param1.max_4_delta = max4delta;
		uv_param1.max_2_delta = max2delta;
		uv_param1.task_no = 1;

		uv_param2.dst_uv_image = cnr_out+DstOffsetTwo;
		uv_param2.src_uv_image = cnr_in+SrcOffsetTwo;
		uv_param2.in_width = uv_denoise_in->InputWidth;
		uv_param2.in_height = part1_h+8;
		uv_param2.out_width = 0;
		uv_param2.out_height = 0;
		uv_param2.max_6_delta = max6delta;
		uv_param2.max_4_delta = max4delta;
		uv_param2.max_2_delta = max2delta;
		uv_param2.task_no = 2;

		uv_param3.dst_uv_image = cnr_out+DstOffsetThr;
		uv_param3.src_uv_image = cnr_in+SrcOffsetThr;
		uv_param3.in_width = uv_denoise_in->InputWidth;
		uv_param3.in_height = part2_h+8;
		uv_param3.out_width = 0;
		uv_param3.out_height = 0;
		uv_param3.max_6_delta = max6delta;
		uv_param3.max_4_delta = max4delta;
		uv_param3.max_2_delta = max2delta;
		uv_param3.task_no = 3;

		uv_param4.dst_uv_image = cnr_out+DstOffsetFour;
		uv_param4.src_uv_image = cnr_in+SrcOffsetFour;
		uv_param4.in_width = uv_denoise_in->InputWidth;
		uv_param4.in_height = part3_h+4;
		uv_param4.out_width = 0;
		uv_param4.out_height = 0;
		uv_param4.max_6_delta = max6delta;
		uv_param4.max_4_delta = max4delta;
		uv_param4.max_2_delta = max2delta;
		uv_param4.task_no = 4;
		sem_init(&denoise_sem_lock, 0, 0);

	 	isp_stub_process(THREAD_0,
						uv_proc_func_neon1,
						uv_proc_cb,
						0,
						(void*)uv_param1_ptr);

		isp_stub_process(THREAD_1,
						uv_proc_func_neon1,
						uv_proc_cb,
						0,
						(void*)uv_param2_ptr);

		isp_stub_process(THREAD_2,
						uv_proc_func_neon1,
						uv_proc_cb,
						0,
						(void*)uv_param3_ptr);

		isp_stub_process(THREAD_3,
						uv_proc_func_neon1,
						uv_proc_cb,
						0,
						(void*)uv_param4_ptr);

	} else if (2 == alg_num) {

		SrcOffsetOne = 0;
		DstOffsetOne = 0;

		SrcOffsetTwo = (part0_h / 2 - 2) * uv_denoise_in->InputWidth* sizeof(int8_t);
		DstOffsetTwo = part0_h / 2 * uv_denoise_in->InputWidth* sizeof(int8_t);

		SrcOffsetThr = (part0_h - 2) * uv_denoise_in->InputWidth* sizeof(int8_t);
		DstOffsetThr = part0_h * uv_denoise_in->InputWidth* sizeof(int8_t);

		SrcOffsetFour = (3 * part0_h /2 - 2) * uv_denoise_in->InputWidth* sizeof(int8_t);
		DstOffsetFour = 3 * part0_h / 2 * uv_denoise_in->InputWidth* sizeof(int8_t);

			/* uv denoise level*/
		uv_param1.dst_uv_image = cnr_out+DstOffsetOne;
		uv_param1.src_uv_image = cnr_in+SrcOffsetOne;
		uv_param1.in_width = uv_denoise_in->InputWidth;
		uv_param1.in_height = part0_h+4;
		uv_param1.out_width = 0;
		uv_param1.out_height = 0;
		uv_param1.max_6_delta = max6delta;
		uv_param1.max_4_delta = max4delta;
		uv_param1.max_2_delta = max2delta;
		uv_param1.task_no = 1;

		uv_param2.dst_uv_image = cnr_out+DstOffsetTwo;
		uv_param2.src_uv_image = cnr_in+SrcOffsetTwo;
		uv_param2.in_width = uv_denoise_in->InputWidth;
		uv_param2.in_height = part1_h+8;
		uv_param2.out_width = 0;
		uv_param2.out_height = 0;
		uv_param2.max_6_delta = max6delta;
		uv_param2.max_4_delta = max4delta;
		uv_param2.max_2_delta = max2delta;
		uv_param2.task_no = 2;

		uv_param3.dst_uv_image = cnr_out+DstOffsetThr;
		uv_param3.src_uv_image = cnr_in+SrcOffsetThr;
		uv_param3.in_width = uv_denoise_in->InputWidth;
		uv_param3.in_height = part2_h+8;
		uv_param3.out_width = 0;
		uv_param3.out_height = 0;
		uv_param3.max_6_delta = max6delta;
		uv_param3.max_4_delta = max4delta;
		uv_param3.max_2_delta = max2delta;
		uv_param3.task_no = 3;

		uv_param4.dst_uv_image = cnr_out+DstOffsetFour;
		uv_param4.src_uv_image = cnr_in+SrcOffsetFour;
		uv_param4.in_width = uv_denoise_in->InputWidth;
		uv_param4.in_height = part3_h+4;
		uv_param4.out_width = 0;
		uv_param4.out_height = 0;
		uv_param4.max_6_delta = max6delta;
		uv_param4.max_4_delta = max4delta;
		uv_param4.max_2_delta = max2delta;
		uv_param4.task_no = 4;
		sem_init(&denoise_sem_lock, 0, 0);

	 	isp_stub_process(THREAD_0,
						uv_proc_func_neon2,
						uv_proc_cb,
						0,
						(void*)uv_param1_ptr);

		isp_stub_process(THREAD_1,
						uv_proc_func_neon2,
						uv_proc_cb,
						0,
						(void*)uv_param2_ptr);

		isp_stub_process(THREAD_2,
						uv_proc_func_neon2,
						uv_proc_cb,
						0,
						(void*)uv_param3_ptr);

		isp_stub_process(THREAD_3,
						uv_proc_func_neon2,
						uv_proc_cb,
						0,
						(void*)uv_param4_ptr);
	} else {
		goto EXIT;
	}

	sem_wait(&denoise_sem_lock);
	sem_wait(&denoise_sem_lock);
	sem_wait(&denoise_sem_lock);
	sem_wait(&denoise_sem_lock);
	isp_stub_process(THREAD_0, NULL, NULL, 1, NULL);
	isp_stub_process(THREAD_1, NULL, NULL, 1, NULL);
	isp_stub_process(THREAD_2, NULL, NULL, 1, NULL);
	isp_stub_process(THREAD_3, NULL, NULL, 1, NULL);

	memcpy((void*)uv_denoise_in->InputAddr, (void*)cnr_out,
				(uv_denoise_in->InputHeight*uv_denoise_in->InputWidth)>>1);

EXIT:
	if(cnr_out != NULL) {
		free(cnr_out);
		cnr_out = NULL;
	}

	if(0 == alg_num) {
		if (NULL != ext_src)
		{
			free(ext_src);
			ext_src = NULL;
		}
		ALOGE("[uv_denoise] uv_denoise_alg0: X!\n");
	}

	cpu_hotplug_disable(0);
#endif
}

void uv_proc_cb(int evt, void* param)
{
	UNUSED(evt);
	UNUSED(param);
	ALOGE("[STUB PROC] uv_proc_cb called!");
	sem_post(&denoise_sem_lock);
}

int cpu_hotplug_disable(uint8_t is_disable)
{
	UNUSED(is_disable);
	/*ignore for the core3 can not support hotplug now*/
#if 0
	const char* const hotplug_disable = "/sys/devices/system/cpu/cpufreq/sprdemand/cpu_hotplug_disable";
	const char* cmd_str  = DISABLE_CPU_HOTPLUG;
	uint8_t org_flag = 0;
	//int	ret = 0;

	FILE* fp = fopen(hotplug_disable, "w");

	if (!fp) {
		ALOGE("Failed to open: cpu_hotplug_disable");
		return 7;
	}

	ALOGE("cpu hotplug disable %d", is_disable);
	if(1 == is_disable) {
		cmd_str = DISABLE_CPU_HOTPLUG;
	} else {
		cmd_str = ENABLE_CPU_HOTPLUG;
	}
	fprintf(fp, "%s", cmd_str);
	fclose(fp);
#endif
	return 0;
}

void add_border_uv(uint8_t *dst, uint8_t *src, uint32_t w, uint32_t h, uint32_t border_w, uint32_t border_h)
{
	uint32_t i, j;
	uint8_t *src_ptr;
	uint8_t *dst_ptr;
	uint32_t dst_w = w + border_w * 2;
	uint32_t dst_h = h + border_h * 2;
	uint32_t dst_stride = dst_w * PIXEL_STRIDE;
	uint32_t src_stride = w * PIXEL_STRIDE;

	src_ptr = src;
	dst_ptr = dst + dst_stride * border_h;
	for (i=0; i<h; i++)
	{
		src_ptr += border_w * PIXEL_STRIDE;
		for (j=0; j<border_w; j++)
		{
			*dst_ptr = *src_ptr;				//u
			*(dst_ptr + 1) = *(src_ptr + 1);		//v

			src_ptr -= PIXEL_STRIDE;
			dst_ptr += PIXEL_STRIDE;
		}

		memcpy(dst_ptr, src_ptr, src_stride);
		dst_ptr += src_stride;
		src_ptr += src_stride;

		src_ptr -= 2 * PIXEL_STRIDE;
		for (j=0; j<border_w; j++)
		{
			*dst_ptr = *src_ptr;				//u
			*(dst_ptr + 1) = *(src_ptr + 1);		//v

			src_ptr -= PIXEL_STRIDE;
			dst_ptr += PIXEL_STRIDE;
		}

		src_ptr += (border_w + 2) * PIXEL_STRIDE;
	}

	src_ptr = dst + dst_stride * (border_h + 1);
	dst_ptr = dst + dst_stride * (border_h - 1);
	for (i=0; i<border_h; i++)
	{
		memcpy(dst_ptr, src_ptr, dst_stride);
		src_ptr += dst_stride;
		dst_ptr -= dst_stride;
	}

	src_ptr = dst + dst_stride * (dst_h - border_h - 2);
	dst_ptr = dst + dst_stride * (dst_h - border_h);
	for (i=0; i<border_h; i++)
	{
		memcpy(dst_ptr, src_ptr, dst_stride);
		src_ptr -= dst_stride;
		dst_ptr += dst_stride;
	}
}


