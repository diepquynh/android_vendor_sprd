#ifndef _DENOISE_APP_H_
#define _DENOISE_APP_H_

struct uv_denoise {
	cmr_s32 max_6_delat;
	cmr_s32 max_4_delat;
	cmr_s32 max_2_delat;
};

struct isp_denoise_input {
	cmr_u32 InputHeight;
	cmr_u32 InputWidth;
	cmr_u32 InputAddr;
};

struct uv_denoise_param0 {
	cmr_s8 *dst_uv_image;
	cmr_s8 *src_uv_image;
	cmr_u32 in_width;
	cmr_u32 in_height;
	cmr_u32 out_width;
	cmr_u32 out_height;
	cmr_s32 max_6_delta;
	cmr_s32 max_4_delta;
	cmr_s32 max_2_delta;
	cmr_s32 task_no;
};

int uv_proc_func_neon0(void* param_uv_in);
int uv_proc_func_neon1(void* param_uv_in);
int uv_proc_func_neon2(void* param_uv_in);
int uv_proc_func_neon730(void* param_uv_in);
int cpu_hotplug_disable(cmr_u8 is_disable);

#endif
