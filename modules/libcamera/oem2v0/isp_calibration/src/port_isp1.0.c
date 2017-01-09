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





#if  !USE_ARITH

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

#if (BUILD_ISPCODE_MODE == 3) || (BUILD_ISPCODE_MODE == 4) || (BUILD_ISPCODE_MODE == 5)

#include "isp_com.h"

uint32_t ISP_Algin(uint32_t pLen , uint16_t algin_blk, uint16_t algin_bits)
{
  return 0;
}

struct isp_context* ispGetAlgContext(uint32_t handler_id)
{
  return ispGetContext(handler_id);
}

int32_t _ispGetLncAddr(struct isp_lnc_param* param_ptr,struct isp_slice_param* isp_ptr, uint16_t src_width,uint32_t isp_id)
{
  return 0;
}

#include "isp_ae.h"

struct isp_ae_param* ispGetAeContext(uint32_t handler_id);
int32_t isp_ae_init_context(uint32_t handler_id, void *cxt)
{
	return 0;
}
uint32_t isp_ae_init(uint32_t handler_id)
{
	return 0;
}
uint32_t isp_ae_deinit(uint32_t handler_id)
{
	return 0;
}
uint32_t isp_ae_calculation(uint32_t handler_id)
{
}
uint32_t isp_ae_update_expos_gain(uint32_t handler_id);
uint32_t isp_ae_set_exposure_gain(uint32_t handler_id)
{
	return 0;
}
int32_t isp_ae_flash_eb(uint32_t handler_id, uint32_t eb);
int32_t isp_ae_set_monitor_size(uint32_t handler_id, void* param_ptr);
int32_t isp_ae_set_alg(uint32_t handler_id, uint32_t mode);
int32_t isp_ae_set_ev(uint32_t handler_id, int32_t ev)
{
	return 0;
}
uint32_t isp_ae_set_flash_exposure_gain(uint32_t handler_id);
uint32_t isp_ae_set_denoise(uint32_t handler_id, uint32_t level);
int32_t isp_ae_set_denosie_level(uint32_t handler_id, uint32_t level)
{
	return 0;
}
int32_t isp_ae_get_denosie_level(uint32_t handler_id, uint32_t* level)
{
	return 0;
}
int32_t isp_ae_save_iso(uint32_t handler_id, uint32_t iso);
uint32_t isp_ae_get_save_iso(uint32_t handler_id)
{
	return 0;
}
int32_t isp_ae_set_iso(uint32_t handler_id, uint32_t iso)
{
	return 0;
}
int32_t isp_ae_get_iso(uint32_t handler_id, uint32_t* iso)
{
	return 0;
}
int32_t isp_ae_set_fast_stab(uint32_t handler_id, uint32_t eb);
int32_t isp_ae_set_stab(uint32_t handler_id, uint32_t eb);
int32_t isp_ae_set_change(uint32_t handler_id, uint32_t eb);
int32_t isp_ae_set_param_index(uint32_t handler_id, uint32_t index)
{
	return 0;
}

int32_t isp_ae_ctrl_set(uint32_t handler_id, void* param_ptr);
int32_t isp_ae_ctrl_get(uint32_t handler_id, void* param_ptr);
int32_t isp_ae_get_ev_lum(uint32_t handler_id)
{
	return 0;
}
int32_t isp_ae_stop_callback_handler(uint32_t handler_id)
{
	return 0;
}
int32_t isp_ae_get_denosie_info(uint32_t handler_id, uint32_t* param_ptr)
{
	return 0;
}

int32_t _isp_ae_set_frame_info(uint32_t handler_id);
uint32_t _isp_ae_set_exposure_gain(uint32_t handler_id);
uint32_t _ispGetLineTime(struct isp_resolution_info* resolution_info_ptr, uint32_t param_index)
{
	return 0;
}
uint32_t _ispGetFrameLine(uint32_t handler_id, uint32_t line_time, uint32_t fps)
{
	return 0;
}
uint32_t _ispSetAeTabMaxIndex(uint32_t handler_id, uint32_t mode, uint32_t iso,uint32_t fps)
{
	return 0;
}

uint32_t _ispSetFixFrameMaxIndex(uint32_t handler_id, uint32_t mode, uint32_t iso,uint32_t fps)
{
	return 0;
}
uint32_t _ispAeInfoSet(uint32_t handler_id)
{
	return 0;
}
int32_t _ispGetAeIndexIsMax(uint32_t handler_id, uint32_t* eb)
{
	return 0;
}
#include "isp_alg.h"

int32_t isp_ae_get_real_gain(uint32_t gain);
int32_t isp_ae_smart_adjust(uint32_t handler_id, int32_t cur_ev, uint32_t eb);
int32_t isp_ae_stab_smart_adjust(uint32_t handler_id, int32_t cur_ev);
int32_t isp_get_denoise_tab(uint32_t de_level, uint8_t** diswei, uint8_t** ranwei);
int32_t isp_get_denoise_tab_diswei(uint32_t de_level, uint8_t** diswei)
{
	return 0;
}
int32_t isp_get_denoise_tab_ranwei(uint32_t de_level, uint8_t** ranwei)
{
	return 0;
}
int32_t isp_flash_calculation(uint32_t handler_id, struct isp_ae_v00_flash_alg_param* v00_flash_ptr);
int32_t isp_adjust_cmc(uint32_t handler_id, int32_t cur_ev);
int32_t _ispGetFetchPitch(struct isp_pitch* pitch_ptr, uint16_t width, enum isp_format format);
int32_t _ispGetStorePitch(struct isp_pitch* pitch_ptr, uint16_t width, enum isp_format format);
int32_t _ispGetSliceHeightNum(struct isp_size* src_size_ptr, struct isp_slice_param* slice_ptr)
{
	return 0;
}
int32_t _ispGetSliceWidthNum(struct isp_size* src_size_ptr, struct isp_slice_param* slice_ptr)
{
	return 0;
}
int32_t _ispSetSlicePosInfo(struct isp_slice_param* slice_ptr);
int32_t _ispGetSliceSize(enum isp_process_type proc_type, struct isp_size* src_size_ptr, struct isp_slice_param* slice_ptr)
{
	return 0;
}
int32_t _ispGetSliceEdgeInfo(struct isp_slice_param* slice_ptr)
{
	return 0;
}
int32_t _ispGetLncAddr(struct isp_lnc_param* param_ptr,struct isp_slice_param* isp_ptr, uint16_t src_width,uint32_t isp_id);
int32_t _ispGetLncCurrectParam(void* lnc0_ptr,void* lnc1_ptr, uint32_t lnc_len, uint32_t alpha, void* dst_lnc_ptr)
{
	return 0;
}
int32_t _ispGetFetchAddr(uint32_t handler_id, struct isp_fetch_param* fetch_ptr)
{
	return 0;
}
int32_t _ispGetFetchPitch(struct isp_pitch* pitch_ptr, uint16_t width, enum isp_format format)
{
	return 0;
}
int32_t _ispGetStorePitch(struct isp_pitch* pitch_ptr, uint16_t width, enum isp_format format)
{
	return 0;
}
int32_t _ispSetSlicePosInfo(struct isp_slice_param* slice_ptr)
{
	return 0;
}
int32_t _ispAddSliceBorder(enum isp_slice_type type, enum isp_process_type proc_type, struct isp_slice_param* slice_ptr)
{
	return 0;
}
int32_t _ispGetSliceSize(enum isp_process_type proc_type, struct isp_size* src_size_ptr, struct isp_slice_param* slice_ptr);
int32_t _ispGetSliceEdgeInfo(struct isp_slice_param* slice_ptr);
uint16_t _ispGetLensGridPitch(uint16_t src_width, uint8_t len_grid, uint32_t isp_id)
{
	return 0;
}
int32_t _ispGetFetchAddr(uint32_t handler_id, struct isp_fetch_param* fetch_ptr);
int32_t _ispSetTuneParam(uint32_t handler_id);
//int32_t _ispSetSlice(uint32_t handler_id, struct isp_slice_param* slice_ptr);
int32_t _ispAwbModeIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)())
{
	return 0;
}
int32_t _ispAeMeasureLumIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)());
int32_t _ispEVIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)())
{
	return 0;
}
int32_t _ispFlickerIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)())
{
	return 0;
}
int32_t _ispAlgIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)())
{
	return 0;
}
int32_t _ispAeModeIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)());
int32_t _ispIsoIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)())
{
	return 0;
}
int32_t _ispSpecialEffectIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)());
int32_t _ispBrightnessIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)())
{
	return 0;
}
int32_t _ispContrastIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)())
{
	return 0;
}

int32_t _ispHistIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)())
{
	return 0;
}

int32_t _ispAutoContrastIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)())
{
	return 0;
}

int32_t _ispSaturationIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)())
{
	return 0;
}

int32_t _ispSharpnessIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)())
{
	return 0;
}

int32_t _ispAfIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)());
int32_t _ispCSSIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)())
{
	return 0;
}
int32_t _ispHDRIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)())
{
	return 0;
}

int32_t _ispGlobalGainIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)())
{
	return 0;
}

int32_t _ispChnGainIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)())
{
	return 0;
}

int32_t _ispWbTrimIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)());
int32_t _ispParamUpdateIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)());
int32_t _ispFlashEGIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)())
{
	return 0;
}

int32_t _ispVideoModeIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)())
{
	return 0;
}

int32_t _ispAfModeIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)());
int32_t _ispAeTouchIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)());
int32_t _ispAeInfoIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)());
int32_t _ispGetFastAeStabIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)())
{
	return 0;
}

int32_t _ispGetAeStabIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)())
{
	return 0;
}

int32_t _ispGetAeChangeIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)())
{
	return 0;
}

int32_t _ispGetAwbStatIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)())
{
	return 0;
}

int32_t _ispGetAfStatIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)())
{
	return 0;
}

int32_t _ispGammaIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)());
int32_t _ispDenoiseIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)());
int32_t _ispSmartAeIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)());
int32_t _ispContinueAfIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)())
{
	return 0;
}

int32_t _ispAfDenoiseIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)())
{
	return 0;
}

int32_t _ispAeIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)())
{
	return 0;
}

int32_t _ispAfInfoIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)())
{
	return 0;
}

int32_t _ispRegIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)());
int32_t _ispHueIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)());
int32_t isp_GetChipVersion(void)
{
	return 0;
}
int32_t isp_InterplateCMC(uint32_t handler_id, uint16_t *out, uint16_t *src[2], uint16_t alpha)
{
	return 0;
}
int32_t isp_SetCMC_By_Reduce(uint32_t handler_id, uint16_t *cmc_out, uint16_t *cmc_in, int32_t percent, uint8_t *is_update)
{
	return 0;
}
void isp_interpolate_lsc(uint16_t *out, uint16_t *src[2], uint16_t weight[2], uint32_t size);
int32_t isp_InterplateCCE(uint32_t handler_id, uint16_t dst[9], uint16_t src[9], uint16_t coef[3], uint16_t base_gain)
{
	return 0;
}

#include "isp_af.h"
uint32_t isp_af_init(uint32_t handler_id)
{
	return 0;
}
uint32_t isp_af_deinit(uint32_t handler_id)
{
	return 0;
}
uint32_t isp_af_calculation(uint32_t handler_id)
{
	return 0;
}
uint32_t isp_af_end(uint32_t handler_id, uint8_t stop_mode)
{
	return 0;
}
uint32_t isp_continue_af_calc(uint32_t handler_id)
{
	return 0;
}
uint32_t isp_af_get_mode(uint32_t handler_id, uint32_t *mode);
uint32_t isp_af_set_mode(uint32_t handler_id, uint32_t mode)
{
	return 0;
}

uint32_t isp_af_set_postion(uint32_t handler_id, uint32_t step)
{
	return 0;
}
uint32_t isp_af_get_stat_value(uint32_t handler_id, void* param_ptr)
{
	return 0;
}
uint32_t isp_af_pos_reset(uint32_t handler_id, uint32_t mode)
{
	return 0;
}

#include "isp_awb.h"

uint32_t isp_awb_init(uint32_t handler_id, void *in_param, void *out_param)
{
	return 0;
}
uint32_t isp_awb_deinit(uint32_t handler_id, void *in_param, void *out_param)
{
	return 0;
}

uint32_t isp_awb_calculation(uint32_t handler_id, void *in_param, void *out_param)
{
	return 0;
}

int32_t smart_light_init(uint32_t handler_id, void *in_param, void *out_param)
{
	return 0;
}
int32_t smart_light_calculation(uint32_t handler_id, void *in_param, void *out_param)
{
	return 0;
}
int32_t smart_light_deinit(uint32_t handler_id, void *in_param, void *out_param)
{
	return 0;
}

int32_t auto_adjust_init(uint32_t handler_id, void* in_param_ptr, void* out_param_ptr)
{
	return 0;
}
int32_t auto_adjust_calc(uint32_t handler_id, void* in_param_ptr, void* out_param_ptr);
int32_t auto_adjust_ioctrl(uint32_t handler_id, enum auto_adjust_ioctrl_cmd cmd, void* in_param_ptr, void* out_param_ptr);
int32_t auto_adjust_deinit(uint32_t handler_id, void* in_param_ptr, void* out_param_ptr)
{
	return 0;
}

#include "isp_lsc_proc.h"
int32_t isp_lsc_dec_gain(struct isp_lsc_dec_gain_param *param)
{
	return 0;
}


#else
int32_t _ispAfInfoIOCtrl(uint32_t handler_id, void* param_ptr, int(*call_back)())
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
