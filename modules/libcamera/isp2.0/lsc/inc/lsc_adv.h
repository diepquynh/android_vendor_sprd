#ifndef _ISP_LSC_ADV_H_
#define _ISP_LSC_ADV_H_

/*----------------------------------------------------------------------------*
 **				Dependencies				*
 **---------------------------------------------------------------------------*/
#ifdef WIN32
#include "data_type.h"
#else
#include <sys/types.h>
#endif

/**---------------------------------------------------------------------------*
**				Compiler Flag				*
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif

/**---------------------------------------------------------------------------*
**				Micro Define				**
**----------------------------------------------------------------------------*/



/**---------------------------------------------------------------------------*
**				Data Structures 				*
**---------------------------------------------------------------------------*/
typedef void* lsc_adv_handle_t;

#define ISP_1_0 	1
#define ISP_2_0 	2


enum smart_lsc_calc_status {
	SMART_LSC_ALG_LOCK = 1,
	SMART_LSC_ALG_UNLOCK = 0,
};

enum lsc_gain_pattern {
	LSC_GAIN_PATTERN_GRBG = 0,
	LSC_GAIN_PATTERN_RGGB = 1,
	LSC_GAIN_PATTERN_BGGR = 2,
	LSC_GAIN_PATTERN_GBRG = 3,
};

struct statistic_raw_t {
	uint32_t *r;
	uint32_t *gr;
	uint32_t *gb;
	uint32_t *b;
};

struct lsc_size {
	uint32_t w;
	uint32_t h;
};









struct lsc_adv_tune_param {


	uint32_t enable;



	uint32_t alg_id;

	uint32_t debug_level;
	uint32_t restore_open;

	/* alg 0 */
	int strength_level;
	float pa;				//threshold for seg
	float pb;
	uint32_t fft_core_id; 	//fft param ID
	uint32_t con_weight;		//convergence rate
	uint32_t freq;

	/* alg 1 */
	//global
	uint32_t alg_effective_freq;
	double gradient_threshold_rg_coef[5];
	double gradient_threshold_bg_coef[5];
	uint32_t thres_bv;
	double ds_sub_pixel_ratio;
	double es_statistic_credibility;
	uint32_t thres_s1_mi;
	double es_credibility_s3_ma;
	int WindowSize_rg;
	int WindowSize_bg;
	double dSigma_rg_dx;
	double dSigma_rg_dy;
	double dSigma_bg_dx;
	double dSigma_bg_dy;
	double iir_factor;
};


struct alsc_alg0_turn_para{
	float pa;				//threshold for seg
	float pb;
	uint32_t fft_core_id; 	//fft param ID
	uint32_t con_weight;		//convergence rate
	uint32_t bv;
	uint32_t ct;
	uint32_t pre_ct;
	uint32_t pre_bv;
	uint32_t restore_open;
	uint32_t freq;
	float threshold_grad;
};

struct lsc_adv_init_param {
	uint32_t gain_pattern;
	int gain_width;
	int gain_height;
	uint16_t *lum_gain;
	struct lsc_adv_tune_param tune_param;
	uint32_t alg_open;
	uint32_t param_level;
	uint32_t camera_id;
};

struct lsc_adv_calc_param{
	struct statistic_raw_t stat_img;
	struct lsc_size stat_size;

	int gain_width;
	int gain_height;
	uint16_t *lum_gain;

	struct lsc_size block_size;
	uint32_t bv;
	uint32_t pre_bv;
	uint32_t ct;
	uint32_t pre_ct;
	uint32_t camera_id;
	uint32_t ae_stable;
	uint32_t isp_mode;
	uint32_t isp_id;
};

struct lsc_adv_calc_result {
	uint16_t *dst_gain;
};
struct lsc_weight_value {
	int32_t value[2];
	uint32_t weight[2];
};

struct lsc_adv_info {
	uint32_t flat_num;
	uint32_t flat_status1;
	uint32_t flat_status2;
	uint32_t stat_r_var;
	uint32_t stat_b_var;
	uint32_t cali_status;

	uint32_t alg_calc_cnt;
	struct lsc_weight_value cur_index;
	struct lsc_weight_value calc_index;
	uint32_t cur_ct;

	uint32_t alg2_enable;
	uint32_t alg2_seg1_num;
	uint32_t alg2_seg2_num;
	uint32_t alg2_seg_num;
	uint32_t alg2_seg_valid;
	uint32_t alg2_cnt;
	uint32_t center_same_num[4];
};



/**---------------------------------------------------------------------------*
**					Data Prototype				**
**----------------------------------------------------------------------------*/
typedef lsc_adv_handle_t (* fun_lsc_adv_init)(struct lsc_adv_init_param *param);
typedef const char* (* fun_lsc_adv_get_ver_str)(lsc_adv_handle_t handle);
typedef int32_t (* fun_lsc_adv_calculation)(lsc_adv_handle_t handle, struct lsc_adv_calc_param *param, struct lsc_adv_calc_result *result);
typedef int32_t (* fun_lsc_adv_ioctrl)(lsc_adv_handle_t handle, enum smart_lsc_calc_status status);
typedef int32_t (* fun_lsc_adv_deinit)(lsc_adv_handle_t handle);

lsc_adv_handle_t lsc_adv_init(struct lsc_adv_init_param *param);
const char* lsc_adv_get_ver_str(lsc_adv_handle_t handle);
int32_t lsc_adv_calculation(lsc_adv_handle_t handle, struct lsc_adv_calc_param *param, struct lsc_adv_calc_result *result);
int32_t lsc_adv_ioctrl(lsc_adv_handle_t handle, enum smart_lsc_calc_status status);
int32_t lsc_adv_deinit(lsc_adv_handle_t handle);
/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif
// End
