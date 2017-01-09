#if 0
#include "port.h"

#include <cutils/atomic-arm.h>


#if 0 // USE_LOG

int log_level;

void ALOG(int level, char file, va_list __arg)
{
	if(level <= log_level)
	{
	   printf(file);
	   vprintf(__arg);
	 }
}
#endif





#ifdef USE_ARITH

#include "arithmetic/inc/FaceFinder.h"

int FaceFinder_Init(int width, int height, MallocFun Mfp, FreeFun Ffp)
{
  return 0;
}

int FaceFinder_Function(unsigned char *src, FaceFinder_Data** ppDstFaces, int *pDstFaceNum ,int skip)
{
  return 0;
}

int FaceFinder_Finalize(FreeFun Ffp)
{
  return 0;
}

#include "arithmetic/inc/FaceSolid.h"

int FaceSolid_Init(int width, int height, MallocFun Mfp, FreeFun Ffp)
{
  return 0;
}

int FaceSolid_Function(unsigned char *src, ACCESS_FaceRect ** ppDstFaces, int *pDstFaceNum ,int skip)
{
  return 0;
}

int FaceSolid_Finalize(FreeFun Ffp)
{
  return 0;
}

#include "arithmetic/inc/HDR2.h"

int HDR_Function(BYTE *Y0, BYTE *Y1, BYTE *Y2, BYTE* output, int height, int width, char *format)
{
  return 0;
}

#endif

#if (BUILD_ISPCODE_MODE == 3) || (BUILD_ISPCODE_MODE == 4)
void* ae_init(struct ae_init_in *in_param, struct ae_init_out *out_param)
{
}
int32_t ae_deinit(void* handler, void *in_param, void *out_param)
{
	return 0;
}
int32_t ae_calculation(void* handler, struct ae_calc_in *in_param, struct ae_calc_out *out_param)
{
	return 0;
}
#if  0
int32_t ae_io_ctrl(void* handler, enum ae_io_ctrl_cmd cmd, void *in_param, void *out_param)
{
	return 0;
}
#endif


#endif

#if 0

#include "ae.h"
#include "ae_misc.h"
#include "isp_awb.h"

void* ae_misc_init(struct ae_misc_init_in *in_param, struct ae_misc_init_out *out_param)
{
}
int32_t ae_misc_deinit(void* handle, void *in_param, void *out_param)
{
	return 0;
}
int32_t ae_misc_calculation(void* handle, struct ae_misc_calc_in *in_param, struct ae_misc_calc_out *out_param)
{
	return 0;
}
int32_t ae_misc_io_ctrl(void* handle, enum ae_misc_io_cmd cmd, void *in_param, void *out_param)
{
	return 0;
}

awb_handle_t awb_init(struct awb_init_param *in_param, void *out_param)
{
	return 1;
}

uint32_t awb_deinit(awb_handle_t handle, void *in_param, void *out_param)
{
	return 0;
}

uint32_t awb_calculation(awb_handle_t handle, struct awb_calc_param *param,
	struct awb_calc_result *result)
{
	return 0;
}

int32_t awb_param_unpack(void *pack_data, uint32_t data_size, struct awb_param_tuning *tuning_param)
{
	return 0;
}
#endif


size_t strlcpy(register char * dst,
				  register const char * src,
				  size_t n)
{
	const char *src0 = src;
	char dummy[1];

	if (!n) {
		dst = dummy;
	} else {
		--n;
	}

	while ((*dst = *src) != 0) {
		if (n) {
			--n;
			++dst;
		}
		++src;
	}

	return src - src0;
}

int uv_proc_func_neon58(void* param)
{
	return 0;
}
int uv_proc_func_neon0(void* param_uv_in)
{
	return 0;
}
int uv_proc_func_neon1(void* param_uv_in)
{
	return 0;
}
int uv_proc_func_neon2(void* param_uv_in)
{
	return 0;
}
int uv_proc_func_neon730(void* param_uv_in)
{
	return 0;
}


size_t __strlen_chk(const char *s, size_t s_len)
{
    size_t ret = strlen(s);
    return ret;
}

#if(MINICAMERA == 1)
int property_get(const char *key, char *value, const char *default_value)
{
	strcpy(value,default_value);
}
#endif


static pthread_attr_t attr[100];
int   threadcnt = 0;

void pthread_hook (pthread_t __th)
{
	if(!pthread_getattr_np(__th, &attr[threadcnt])) {
		threadcnt++;
	} else {
		ALOGW("lookat: pthread_getattr_np %d fail\n", threadcnt);
	}
}

void pthread_stackshow()
{
	int i,j;
	unsigned int *pstack;
	size_t size;

	ALOGW("lookat: thread total %d\n",threadcnt);
	for(i = threadcnt-1; i > 0 ; i-- ){
		ALOGW("lookat: thread %d\n",i);
		if(!pthread_attr_getstack(&attr[i], &pstack, &size) && pstack) {
			ALOGW("lookat: statckinfo: 0x%x, %p\n",size, pstack);
			for (j = 0; j < size/sizeof(unsigned int *) ; j++) {
				ALOGW("lookat: statck %d, %p\n",j, pstack[j]);
			}
		}
	}
}

#endif
