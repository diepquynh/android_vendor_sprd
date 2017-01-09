#ifndef _ISP_LSC_ADV_H_
#define _ISP_LSC_ADV_H_

/*----------------------------------------------------------------------------*
 **				Dependencies				*
 **---------------------------------------------------------------------------*/
#ifdef WIN32
#include "data_type.h"
#include "win_dummy.h"
#else
#include <sys/types.h>
#include <pthread.h>
#include <utils/Log.h>
#endif
#include "stdio.h"
/**---------------------------------------------------------------------------*
**				Compiler Flag				*
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif

#define max(A,B) (((A) > (B)) ? (A) : (B))
#define min(A,B) (((A) < (B)) ? (A) : (B))

/**---------------------------------------------------------------------------*
**				Micro Define				**
**----------------------------------------------------------------------------*/
#define LSC_ADV_DEBUG_STR       "LSC_ADV: L %d, %s: "
#define LSC_ADV_DEBUG_ARGS    __LINE__,__FUNCTION__

#define LSC_ADV_LOGE(format,...) ALOGE(LSC_ADV_DEBUG_STR format, LSC_ADV_DEBUG_ARGS, ##__VA_ARGS__)
#define LSC_ADV_LOGW(format,...) ALOGW(LSC_ADV_DEBUG_STR, format, LSC_ADV_DEBUG_ARGS, ##__VA_ARGS__)
#define LSC_ADV_LOGI(format,...) ALOGI(LSC_ADV_DEBUG_STR format, LSC_ADV_DEBUG_ARGS, ##__VA_ARGS__)
#define LSC_ADV_LOGD(format,...) ALOGD(LSC_ADV_DEBUG_STR format, LSC_ADV_DEBUG_ARGS, ##__VA_ARGS__)
#define LSC_ADV_LOGV(format,...) ALOGV(LSC_ADV_DEBUG_STR format, LSC_ADV_DEBUG_ARGS, ##__VA_ARGS__)

/**---------------------------------------------------------------------------*
**				Data Structures 				*
**---------------------------------------------------------------------------*/

typedef void* lsc_adv_handle_t;

#define ISP_1_0 	1
#define ISP_2_0 	2

#define ISP_ALSC_SUCCESS 0
#define ISP_ALSC_ERROR -1

#define NUM_ROWS 24
#define NUM_COLS 32

#define MAX_SEGS 8

#define MAX_WIDTH   64
#define MAX_HEIGHT  64




/*RAW RGB BAYER*/
#define SENSOR_IMAGE_PATTERN_RAWRGB_GR                0x00
#define SENSOR_IMAGE_PATTERN_RAWRGB_R                 0x01
#define SENSOR_IMAGE_PATTERN_RAWRGB_B                 0x02
#define SENSOR_IMAGE_PATTERN_RAWRGB_GB                0x03

#define BLOCK_PARAM_CFG(input, param_data, blk_cmd, blk_id, cfg_ptr, cfg_size)\
	do {\
		param_data.cmd = blk_cmd;\
		param_data.id = blk_id;\
		param_data.data_ptr = cfg_ptr;\
		param_data.data_size = cfg_size;\
		input.param_data_ptr = &param_data;\
		input.param_num = 1;} while (0);


enum alsc_io_ctrl_cmd{
	SMART_LSC_ALG_UNLOCK = 0,
	SMART_LSC_ALG_LOCK = 1,
	ALSC_CMD_GET_DEBUG_INFO = 2,
	LSC_INFO_TO_AWB = 3,
	ALSC_GET_VER = 4,
	ALSC_FLASH_ON = 5,
	ALSC_FLASH_OFF = 6,
};


struct tg_alsc_debug_info {
	uint8_t *log;
	uint32_t size;
};


struct alsc_ver_info{
	unsigned int LSC_SPD_VERSION;   // LSC version of Spreadtrum 
};


struct debug_lsc_param{
	char LSC_version[50];
	unsigned short TB_DNP[12];
	unsigned short TB_A[12];
	unsigned short TB_TL84[12];
	unsigned short TB_D65[12];
	unsigned short TB_CWF[12];
	unsigned int STAT_R[5];
	unsigned int STAT_GR[5];
	unsigned int STAT_GB[5];
	unsigned int STAT_B[5];
	unsigned int gain_width;
	unsigned int gain_height;
	unsigned int gain_pattern;
	unsigned int grid;

	//SLSC
	int error_x10000[9];
	int eratio_before_smooth_x10000[9];
	int eratio_after_smooth_x10000[9];
	int final_ratio_x10000;
	int final_index;
	unsigned short smart_lsc_table[1024*4];

	//ALSC
	unsigned short alsc_lsc_table[1024*4];

	//ALSC_SMOOTH
	unsigned short alsc_smooth_lsc_table[1024*4];

	//OUTPUT
	unsigned short last_lsc_table[1024*4];
	unsigned short output_lsc_table[1024*4];
};


enum lsc_gain_pattern {
	LSC_GAIN_PATTERN_GRBG = 0,
	LSC_GAIN_PATTERN_RGGB = 1,
	LSC_GAIN_PATTERN_BGGR = 2,
	LSC_GAIN_PATTERN_GBRG = 3,
};


struct LSC_info2AWB{
	uint16_t  value[2];//final_index;
	uint16_t  weight[2];// final_ratio;
};





/*
enum interp_table_index{
	LSC_TAB_A = 0,
	LSC_TAB_TL84,
	LSC_TAB_CWF,
	LSC_TAB_D65,
	LSC_TAB_DNP,
};

struct LSC_Segs{
	int table_pair[2];//The index to the shading table pair.
	int max_CT;//The maximum CT that the pair will be used.
	int min_CT;//The minimum CT that the pair will be used.
};

struct LSC_Setting{
	struct LSC_Segs segs[MAX_SEGS];//The shading table pairs.
	int seg_count;//The number of shading table pairs.

	double smooth_ratio;//the smooth ratio for the output weighting
	int proc_inter;//process interval of LSC(unit=frames)

	int num_seg_queue;//The number of elements for segement voting.
	int num_seg_vote_th;//The voting threshold for segmenet voting.
};
*/








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
	uint32_t con_weight;	//convergence rate
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
	uint32_t con_weight;	//convergence rate
	uint32_t bv;
	uint32_t ct;
	uint32_t pre_ct;
	uint32_t pre_bv;
	uint32_t restore_open;
	uint32_t freq;
	float threshold_grad;
};


struct lsc_weight_value {
	int32_t value[2];
	uint32_t weight[2];
};


// smart1.0_log_info
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


struct lsc_adv_context {
	unsigned int LSC_SPD_VERSION;
	uint32_t grid;
	int32_t gain_pattern;
	int gain_width;
    int gain_height;

	int32_t alg_locked;  // lock alsc or not from ioctrl  
	uint32_t alg_open;   // open front camera or not
	
	pthread_mutex_t 	mutex_stat_buf;
	uint32_t * stat_ptr;
	uint32_t* stat_for_alsc;
	
	void* lsc_alg;
    uint16_t *lum_gain;
    uint32_t pbayer_r_sums[NUM_ROWS * NUM_COLS];
    uint32_t pbayer_gr_sums[NUM_ROWS * NUM_COLS];
    uint32_t pbayer_gb_sums[NUM_ROWS * NUM_COLS];
    uint32_t pbayer_b_sums[NUM_ROWS * NUM_COLS];
	float color_gain[NUM_ROWS * NUM_COLS * 4];
    float color_gain_bak[NUM_ROWS * NUM_COLS * 4];
    struct alsc_alg0_turn_para alg0_turn_para;
};


////////////////////////////// HLSC_V2.0 structure //////////////////////////////


struct lsc2_tune_param{     // if modified, please contact to TOOL team
	// system setting
	unsigned int LSC_SPD_VERSION;   // LSC version of Spreadtrum 
	unsigned int number_table;          // number of used original shading tables

	// control_param	
	unsigned int alg_mode;	
	unsigned int table_base_index;
	unsigned int user_mode;
	unsigned int freq;
	unsigned int IIR_weight;	

	// slsc2_param   
	unsigned int num_seg_queue;
	unsigned int num_seg_vote_th;
	unsigned int IIR_smart2;

	// alsc1_param
	int strength;

	// alsc2_param
	unsigned int lambda_r;        // need to add lambda_r , lambda_b and change type to unsigned int
	unsigned int lambda_b;        // need to add lambda_r , lambda_b and change type to unsigned int
	unsigned int weight_r;	 // need to add weight_r , weight_b and change type to unsigned int
	unsigned int weight_b;	 // need to add weight_r , weight_b and change type to unsigned int
};


struct lsc2_context{
	pthread_mutex_t 	mutex_stat_buf;
	unsigned int LSC_SPD_VERSION;   // LSC version of Spreadtrum 
	unsigned int alg_locked;              // lock alsc or not by ioctrl
	unsigned int flash_mode;
	unsigned int alg_open;                 // complie alg0.c or alg2.c
	unsigned int gain_width;
	unsigned int gain_height;
	unsigned int gain_pattern;
	unsigned int grid;

	unsigned short* lsc_tab_address[9];  // the copy of table in parameter file
	unsigned short* lsc_table_ptr_r;    // storage to save Rfirst table
	unsigned short* tabptr[9]; // address of origianl shading table will be used to interperlation in slsc2
	unsigned short* tabptrPlane[9]; // address R-first shading table ( lsc_table_ptr )	

	void* control_param;
	void* slsc2_param;
	void* alsc1_param;
	void* alsc2_param;
	void* lsc1d_param;

	// tmp storage
	unsigned short* color_gain_r;
	unsigned short* color_gain_gr;
	unsigned short* color_gain_gb;
	unsigned short* color_gain_b;	
	
	// debug info output address
	void* lsc_debug_info_ptr;

	// AEM storage address
	unsigned int* stat_for_lsc;
	
	// Copy the dst_gain addredd
	uint16_t* dst_gain;
	
};


////////////////////////////// calculation dependent //////////////////////////////


struct lsc_adv_init_param {
	unsigned int alg_open;   // complie alg0.c or alg2.c
	unsigned int gain_width;
	unsigned int gain_height;
	unsigned int gain_pattern;
	unsigned int grid;

	/* added parameters */
	void* tune_param_ptr;
	unsigned short* lsc_tab_address[9];  // the copy of table in parameter file	
	struct lsc2_tune_param lsc2_tune_param;  // HLSC_V2.0 tuning structure

	/* no use in lsc_adv2 */	
	uint32_t param_level;	
	uint16_t *lum_gain;  // space to save pre_table from smart1.0
	struct lsc_adv_tune_param tune_param;
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


struct lsc_adv_calc_param{
	struct statistic_raw_t stat_img;    // statistic value of 4 channels
	struct lsc_size stat_size;          // size of statistic value matrix
	int gain_width;                     // width  of shading table
	int gain_height;	                // height of shading table
	uint32_t ct;                        // ct from AWB calc
	int r_gain;                         // r_gain from AWB calc
	int b_gain;                         // b_gain from AWB calc
	uint32_t bv;                        // bv from AE calc	
	uint32_t isp_mode;                  // about the mode of interperlation of shading table
	uint32_t isp_id;                    // 0. alg0.c ,  2. alg2.c
	uint32_t camera_id;                 // 0. back camera  ,  1. front camera
	struct lsc_size img_size;           // raw size

	// no use in HLSC_V2.0
	struct lsc_size block_size;
	uint16_t *lum_gain;                 // pre_table from smart1.0
	uint32_t ae_stable;                 // ae stable info from AE calc
	int awb_pg_flag;
	
	unsigned short* lsc_tab_address[9]; // lsc_tab_address
	uint32_t lsc_tab_size;

	// not fount in isp_app.c
	uint32_t pre_bv;	
	uint32_t pre_ct;	
};


struct lsc_adv_calc_result {
	uint16_t *dst_gain;
};


/**---------------------------------------------------------------------------*
**					Data Prototype				**
**----------------------------------------------------------------------------*/
typedef lsc_adv_handle_t (* fun_lsc_adv_init)(struct lsc_adv_init_param *param);
typedef const char* (* fun_lsc_adv_get_ver_str)(lsc_adv_handle_t handle);
typedef int32_t (* fun_lsc_adv_calculation)(lsc_adv_handle_t handle, struct lsc_adv_calc_param *param, struct lsc_adv_calc_result *result);
typedef int32_t (* fun_lsc_adv_ioctrl)(lsc_adv_handle_t handle, enum alsc_io_ctrl_cmd cmd, void *in_param, void *out_param);
typedef int32_t (* fun_lsc_adv_deinit)(lsc_adv_handle_t handle);

lsc_adv_handle_t lsc_adv_init(struct lsc_adv_init_param *param);
const char* lsc_adv_get_ver_str(lsc_adv_handle_t handle);
int32_t lsc_adv_calculation(lsc_adv_handle_t handle, struct lsc_adv_calc_param *param, struct lsc_adv_calc_result *result);
int32_t lsc_adv_ioctrl(lsc_adv_handle_t handle, enum alsc_io_ctrl_cmd cmd, void *in_param, void *out_param);
int32_t lsc_adv_deinit(lsc_adv_handle_t handle);
int32_t is_print_lsc_log(void);

int otp_lsc_mod(unsigned short* otpLscTabGolden, unsigned short* otpLSCTabRandom,  //T1, T2
				    unsigned short* otpLscTabGoldenRef, //Ts1
				    unsigned int* otpAWBMeanGolden, unsigned int* otpAWBMeanRandom,
				    unsigned short* otpLscTabGoldenMod, //output: Td2
				    unsigned int gainWidth, unsigned int gainHeight,
				    int bayerPattern
				    );
/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif
// End
