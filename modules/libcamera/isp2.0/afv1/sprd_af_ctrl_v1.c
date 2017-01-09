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
#define LOG_TAG "AFv1"

#include <assert.h>
#include <dlfcn.h>
#include <utils/Timers.h>

#include "sprd_af_ctrl_v1.h"
#include "isp_log.h"

#include "AFv1_Common.h"
#include "AFv1_Interface.h"
#include "AFv1_Tune.h"

#include "aft_interface.h"

#include "isp_drv.h"
#include "isp_pm.h"

#include "cmr_msg.h"
#include "lsc_adv.h"

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#define AF_LOGD ISP_LOGD
#define AF_LOGE ISP_LOGE

#define SPLIT_TEST      1
#define AF_WAIT_CAF_FINISH     1
#define AF_RING_BUFFER         0
#define AF_SYS_VERSION "-20161129-15"

int32_t _smart_io_ctrl(isp_ctrl_context* handle, uint32_t cmd);
int32_t lsc_adv_ioctrl(lsc_adv_handle_t handle, enum alsc_io_ctrl_cmd cmd, void *in_param, void *out_param);

enum af_state
{
    STATE_INACTIVE = 0,
    STATE_IDLE,         // isp is ready, waiting for cmd
    STATE_NORMAL_AF,    // single af or touch af
    STATE_CAF,
    STATE_RECORD_CAF,
	STATE_FAF,
};

static const char *state_string[] = 
{
    "inactive",
    "idle",
    "normal_af",
    "caf",
    "record caf",
    "faf",
};

#define STATE_STRING(state)    state_string[state]

enum caf_state
{
    CAF_IDLE,
    CAF_MONITORING,
    CAF_SEARCHING
};

static const char *caf_state_str[] = 
{
    "caf idle",
    "caf monitoring",
    "caf searching"
};

enum dcam_after_vcm
{
    DCAM_AFTER_VCM_NO=0,
    DCAM_AFTER_VCM_YES
};

#define CAF_STATE_STR(state)    caf_state_str[state]

static char AF_MODE[30] = {'\0'};
static char AF_POS[10]={'\0'};


typedef struct _isp_info
{
    uint32_t width;
    uint32_t height;
    uint32_t win_num;
} isp_info_t;


#define MAX_ROI_NUM     32 // arbitrary, more than hardware wins

typedef struct _roi_rgb_y
{
	int num;
	uint32_t Y_sum[MAX_ROI_NUM];
	uint32_t R_sum[MAX_ROI_NUM];
	uint32_t G_sum[MAX_ROI_NUM];
	uint32_t B_sum[MAX_ROI_NUM];
}roi_rgb_y_t;

typedef struct _lens_info
{
    int32_t pos;
} lens_info_t;

typedef struct _ae_info
{
    uint32_t stable;
    uint32_t bv;
    uint32_t exp; // 0.1us
    uint32_t gain;
    uint32_t gain_index;
    uint32_t win_size;
    struct ae_calc_out ae;
} ae_info_t;

typedef struct _awb_info
{
    uint32_t r_gain;
    uint32_t g_gain;
    uint32_t b_gain;
} awb_info_t;

typedef struct _isp_awb_statistic_hist_info{
	uint32_t r_info[1024];
	uint32_t g_info[1024];
	uint32_t b_info[1024];
	uint32_t r_hist[256];
	uint32_t g_hist[256];
	uint32_t b_hist[256];
	uint32_t y_hist[256];
} isp_awb_statistic_hist_info_t;

enum AF_RINGBUFFER
{
    AF_RING_BUFFER_NO,
    AF_RING_BUFFER_YES,
};

enum scene
{
    OUT_SCENE = 0,
    INDOOR_SCENE,
    DARK_SCENE,
    SCENE_NUM
};

enum AF_AE_GAIN{
	GAIN_1x = 0,
	GAIN_2x,
	GAIN_4x,
	GAIN_8x,
	GAIN_16x,
	GAIN_32x,
	GAIN_TOTAL
};

#pragma pack(push,1)
typedef struct _filter_clip
{
    uint32_t spsmd_max;
    uint32_t spsmd_min;
    uint32_t sobel_max;
    uint32_t sobel_min;
} filter_clip_t;

typedef struct _win_coord
{
    uint32_t start_x;
    uint32_t start_y;
    uint32_t end_x;
    uint32_t end_y;
} win_coord_t;

typedef struct _AF_Window_Config{
	uint8_t    valid_win_num;
	uint8_t    win_strategic;
	win_coord_t  win_pos[25];
	uint32_t  win_weight[25];
}AF_Window_Config;

typedef struct _af_tuning_param{
    uint8_t             flag;// Tuning parameter switch, 1 enable tuning parameter, 0 disenable it
    filter_clip_t       filter_clip[SCENE_NUM][GAIN_TOTAL];// AF filter threshold
    int32_t             bv_threshold[SCENE_NUM][SCENE_NUM];//BV threshold
    AF_Window_Config SAF_win;// SAF window config
    AF_Window_Config CAF_win; // CAF window config
    AF_Window_Config VAF_win; // VAF window config
    // default param for indoor/outdoor/dark
    AF_Tuning AF_Tuning_Data[SCENE_NUM];// Algorithm related parameter
    uint8_t soft_landing_dly;
    uint8_t soft_landing_step;
    uint8_t dummy[99];// for 4-bytes alignment issue
}af_tuning_param_t;

#pragma pack(pop)

typedef struct _roi_info
{
    int num;
    win_coord_t win[MAX_ROI_NUM];
} roi_info_t;

enum filter_type
{
    FILTER_SOBEL5 = 0,
    FILTER_SOBEL9,
    FILTER_SPSMD,
    FILTER_NUM
};

#define SOBEL5_BIT  (1 << FILTER_SOBEL5)
#define SOBEL9_BIT  (1 << FILTER_SOBEL9)
#define SPSMD_BIT   (1 << FILTER_SPSMD)

// caf trigger
typedef struct _caf_trigger_ops
{
    aft_proc_handle_t handle;
    struct aft_ae_skip_info ae_skip_info;
    int32_t (*init)(struct aft_tuning_block_param *init_param, aft_proc_handle_t *handle);
    int32_t (*deinit)(aft_proc_handle_t *handle);
    int32_t (*calc)(aft_proc_handle_t *handle, struct aft_proc_calc_param *alg_calc_in,
        struct aft_proc_result *alg_calc_result);
    int32_t (*ioctrl)(aft_proc_handle_t *handle, enum aft_cmd cmd, void *param0, void *param1);
} caf_trigger_ops_t;

#define CAF_TRIGGER_LIB "libspcaftrigger.so"

typedef struct _ae_cali{
    uint32_t            r_avg[9];
    uint32_t            g_avg[9];
    uint32_t            b_avg[9];
    uint32_t            r_avg_all;
    uint32_t            g_avg_all;
    uint32_t            b_avg_all;
}ae_cali_t;

typedef struct _vcm_ops{
	void (*set_pos)(uint16_t pos);
	void (*get_otp)(uint16_t *inf,uint16_t *macro);
	void (*set_motor_bestmode)();
	void (*set_test_vcm_mode)(char* vcm_mode);
	void (*get_test_vcm_mode)();
	void (*get_motor_pos)(uint16_t *pos);
}vcm_ops_t;

typedef struct _prime_face_base_info{
	uint32_t   sx;
	uint32_t   sy;
	uint32_t   ex;
	uint32_t   ey;
	uint32_t   area;
	uint32_t   diff_area_thr;
	uint32_t   diff_cx_thr;
	uint32_t   diff_cy_thr;
	uint16_t   converge_cnt_thr;
	uint16_t   converge_cnt;
	uint16_t   diff_trigger;
}prime_face_base_info_t;

typedef struct _focus_stat{
	unsigned int force_write;
	unsigned int reg_param[10];
}focus_stat_reg_t;

typedef struct _af_ctrl
{
    char af_version[40];
    enum af_state       state;
    enum af_state       pre_state;
    enum caf_state      caf_state;
    enum scene          curr_scene;
    eAF_MODE            algo_mode;
    uint32_t            takePicture_timeout;
    uint32_t            request_mode; // enum isp_focus_mode
    uint32_t		need_re_trigger;
    uint64_t		vcm_timestamp;
    uint64_t		dcam_timestamp;
    uint64_t		takepic_timestamp;
    AF_Data             fv;
    struct afm_thrd_rgb thrd;
    struct isp_face_area face_info;
    uint32_t Y_sum_trigger;
    uint32_t Y_sum_normalize;
    uint64 fv_combine[T_TOTAL_FILTER_TYPE];
    prime_face_base_info_t  face_base;
    isp_ctrl_context    *isp_ctx;
    pthread_mutex_t    af_work_lock;
    pthread_mutex_t    caf_work_lock;
    sem_t                    af_wait_caf;
    af_tuning_param_t   af_tuning_data;
    AF_Window_Config *win_config;
    isp_info_t          isp_info;
    lens_info_t         lens;
    int                 touch;
    int                 flash_on;
    roi_info_t          roi;
    roi_rgb_y_t             roi_RGBY;
    ae_info_t           ae;
    awb_info_t          awb;
    filter_clip_t       filter_clip[SCENE_NUM][GAIN_TOTAL];
    int32_t             bv_threshold[SCENE_NUM][SCENE_NUM];
    uint8_t             pre_scene;
    int                 ae_lock_num;
    int                 awb_lock_num;
    int                 lsc_lock_num;
    void                *trig_lib;
    caf_trigger_ops_t   trig_ops;
    //pthread_mutex_t     caf_lock;
    cmr_handle          af_task;
    uint64_t            last_frame_ts; // timestamp of last frame
    ae_cali_t           ae_cali_data;
    vcm_ops_t vcm_ops;
    uint32_t vcm_stable;
    uint32_t af_statistics[105];// for maximum size in all chip to accomodate af statistics
    int32_t node_type;
    uint64_t k_addr;
    uint64_t u_addr;
	focus_stat_reg_t stat_reg;
	unsigned int defocus;
	uint8_t bypass;
	uint8_t soft_landing_dly;
	uint8_t soft_landing_step;
	uint8_t caf_first_stable;
	unsigned int inited_af_req;
} af_ctrl_t;

typedef struct _test_mode_command{
	char* command;
	uint64 key;
	void (*command_func)(af_ctrl_t* af,char* test_param);
}test_mode_command_t;
/*
typedef struct _af_exif_info
{
    char af_version[40];
    enum af_state       state;
    enum af_state       pre_state;
    enum caf_state      caf_state;
    enum scene          curr_scene;
    eAF_MODE            algo_mode;
    uint32_t            takePicture_timeout;
    uint32_t            request_mode; // enum isp_focus_mode
    uint32_t		need_re_trigger;
    uint64_t		vcm_timestamp;
    uint64_t		dcam_timestamp;
    uint64_t		takepic_timestamp;
    AF_Data             fv;
    struct afm_thrd_rgb thrd;
    struct isp_face_area face_info;
} af_exif_info_t;

static af_exif_info_t af_exif_info;
*/
static ERRCODE if_get_motor_pos(uint16 *motor_pos, void *cookie);
/*
static void load_to_exif_buffer(af_ctrl_t *af){
	memcpy(af_exif_info.af_version,af->af_version,sizeof(af_exif_info.af_version));
	af_exif_info.state = af->state;
	af_exif_info.pre_state = af->pre_state;
	af_exif_info.caf_state = af->caf_state;
	af_exif_info.curr_scene = af->curr_scene;
	af_exif_info.algo_mode = af->algo_mode;
	af_exif_info.takePicture_timeout = af->takePicture_timeout;
	af_exif_info.request_mode = af->request_mode;
	af_exif_info.need_re_trigger = af->need_re_trigger;
	af_exif_info.vcm_timestamp = af->vcm_timestamp;
	af_exif_info.dcam_timestamp = af->dcam_timestamp;
	af_exif_info.takepic_timestamp = af->takepic_timestamp;
	memcpy(&af_exif_info.fv,&af->fv,sizeof(af_exif_info.fv));
	memcpy(&af_exif_info.thrd,&af->thrd,sizeof(af_exif_info.thrd));
	memcpy(&af_exif_info.face_info,&af->face_info,sizeof(af_exif_info.face_info));
}
*/
// afm hardware
static void afm_enable(isp_ctrl_context *isp)
{
    isp_u_raw_afm_bypass(isp->handle_device, 0);
}

static void afm_disable(isp_ctrl_context *isp)
{
    isp_u_raw_afm_bypass(isp->handle_device, 1);
}

static void afm_setup(af_ctrl_t *af)
{
    isp_ctrl_context *isp = af->isp_ctx;
    isp_handle device = isp->handle_device;

    // AF_LOGD("scene = %d, spsmd_max = 0x%x, spsmd_min = 0x%x, sobel_max = 0x%x, sobel_min = 0x%x", 
    //     af->curr_scene,
    //     af->filter_clip[af->curr_scene].spsmd_max, af->filter_clip[af->curr_scene].spsmd_min,
    //     af->filter_clip[af->curr_scene].sobel_max, af->filter_clip[af->curr_scene].sobel_min);

	af->thrd.sobel5_thr_min_red = af->filter_clip[af->curr_scene][af->ae.gain_index].sobel_min;
	af->thrd.sobel5_thr_max_red = af->filter_clip[af->curr_scene][af->ae.gain_index].sobel_max;
	af->thrd.sobel5_thr_min_green = af->filter_clip[af->curr_scene][af->ae.gain_index].sobel_min;
	af->thrd.sobel5_thr_max_green = af->filter_clip[af->curr_scene][af->ae.gain_index].sobel_max;
	af->thrd.sobel5_thr_min_blue = af->filter_clip[af->curr_scene][af->ae.gain_index].sobel_min;
	af->thrd.sobel5_thr_max_blue = af->filter_clip[af->curr_scene][af->ae.gain_index].sobel_max;
	af->thrd.sobel9_thr_min_red = af->filter_clip[af->curr_scene][af->ae.gain_index].sobel_min;
	af->thrd.sobel9_thr_max_red = af->filter_clip[af->curr_scene][af->ae.gain_index].sobel_max;
	af->thrd.sobel9_thr_min_green = af->filter_clip[af->curr_scene][af->ae.gain_index].sobel_min;
	af->thrd.sobel9_thr_max_green = af->filter_clip[af->curr_scene][af->ae.gain_index].sobel_max;
	af->thrd.sobel9_thr_min_blue = af->filter_clip[af->curr_scene][af->ae.gain_index].sobel_min;
	af->thrd.sobel9_thr_max_blue = af->filter_clip[af->curr_scene][af->ae.gain_index].sobel_max;
	af->thrd.spsmd_thr_min_red = af->filter_clip[af->curr_scene][af->ae.gain_index].spsmd_min;
	af->thrd.spsmd_thr_max_red = af->filter_clip[af->curr_scene][af->ae.gain_index].spsmd_max;
	af->thrd.spsmd_thr_min_green = af->filter_clip[af->curr_scene][af->ae.gain_index].spsmd_min;
	af->thrd.spsmd_thr_max_green = af->filter_clip[af->curr_scene][af->ae.gain_index].spsmd_max;
	af->thrd.spsmd_thr_min_blue = af->filter_clip[af->curr_scene][af->ae.gain_index].spsmd_min;
	af->thrd.spsmd_thr_max_blue = af->filter_clip[af->curr_scene][af->ae.gain_index].spsmd_max;
	if(af->stat_reg.force_write){
		af->thrd.spsmd_thr_min_red = af->stat_reg.reg_param[0];
		af->thrd.spsmd_thr_max_red = af->stat_reg.reg_param[1];
		af->thrd.spsmd_thr_min_blue = af->thrd.spsmd_thr_min_green = af->thrd.spsmd_thr_min_red;
		af->thrd.spsmd_thr_max_blue =af->thrd.spsmd_thr_max_green = af->thrd.spsmd_thr_max_red;	
		AF_LOGD("force write reg: %d ~ %d \n"
			,af->stat_reg.reg_param[0]
			,af->stat_reg.reg_param[1]);		
	}
    if (ISP_CHIP_ID_TSHARK3 == isp_dev_get_chip_id(device))
    {
	    isp_u_raw_afm_skip_num_clr(device, 1);
	    isp_u_raw_afm_skip_num_clr(device, 0);
	    isp_u_raw_afm_skip_num(device, 0);
	    isp_u_raw_afm_spsmd_rtgbot_enable(device, 1);
	    isp_u_raw_afm_spsmd_diagonal_enable(device, 1);
	    isp_u_raw_afm_spsmd_square_en(device, 1);
	    isp_u_raw_afm_overflow_protect(device, 0);
	    isp_u_raw_afm_subfilter(device, 0, 1);
	    isp_u_raw_afm_spsmd_touch_mode(device, 0);
	    isp_u_raw_afm_shfit(device, 0, 0, 0);
	    isp_u_raw_afm_threshold_rgb(device, &af->thrd);
	    isp_u_raw_afm_mode(device, 1);
    }
    else
    {
        isp_u_raw_afm_skip_num_clr(device, 1);
        isp_u_raw_afm_skip_num_clr(device, 0);
        isp_u_raw_afm_skip_num(device, 0);
        isp_u_raw_afm_spsmd_rtgbot_enable(device, 1);
        isp_u_raw_afm_spsmd_diagonal_enable(device, 1);
        isp_u_raw_afm_spsmd_cal_mode(device, 1);
        isp_u_raw_afm_sel_filter1(device, 0); // 0 stands for spsmd
        isp_u_raw_afm_sel_filter2(device, 1); // 1 stands for sobel9x9
        isp_u_raw_afm_sobel_type(device, 0);
        isp_u_raw_afm_spsmd_type(device, 2);
        isp_u_raw_afm_sobel_threshold(device, af->thrd.sobel9_thr_min_green, af->thrd.sobel9_thr_max_green);
        isp_u_raw_afm_spsmd_threshold(device, af->thrd.spsmd_thr_min_green, af->thrd.spsmd_thr_max_green);
        isp_u_raw_afm_mode(device, 1);
    }
}

static uint32_t afm_get_win_num(isp_ctrl_context *isp)
{
    uint32_t num;
    isp_u_raw_afm_win_num(isp->handle_device, &num);
    return num;
}

static void afm_set_win(isp_ctrl_context *isp, win_coord_t *win, int num, int hw_num)
{
    int i;

    for (i=num; i<hw_num; ++i)
    {
        // some strange hardware behavior
        win[i].start_x = 0;
        win[i].start_y = 0;
        win[i].end_x = 2;
        win[i].end_y = 2;
    }
    
    isp_u_raw_afm_win(isp->handle_device, win);
}

static int afm_get_fv(isp_ctrl_context *isp, uint64_t *fv, uint32_t filter_mask, int roi_num,int ring_buffer)
{
	int i, num;
	uint64_t *p;
	af_ctrl_t *af = (af_ctrl_t *)isp->handle_af;

	num = 0;
	p = fv;

	if(0==ring_buffer){
		if (ISP_CHIP_ID_TSHARK3 == isp_dev_get_chip_id(isp->handle_device))
		{
			uint32_t data[105];

			isp_u_raw_afm_statistic_r6p9(isp->handle_device, data);

			if (filter_mask & SOBEL5_BIT)
			{
				num++;
				for (i=0; i<roi_num; ++i)
				*p++ = data[1 + i * 3];
			}

			if (filter_mask & SOBEL9_BIT)
			{
				num++;
				for (i=0; i<roi_num; ++i)
				*p++ = data[31 + i * 3];
			}

			if (filter_mask & SPSMD_BIT)
			{
				num++;
				for (i=0; i<roi_num; ++i)
				{
					uint64_t high = data[95 + i / 2];
					high = (i & 0x01) ? ((high & 0x00FF0000) << 16) : ((high & 0x000000FF) << 32);
					*p++ = data[61 + i * 3] + high;
				}
			}
		}
		else
		{
			uint32_t data[25];

			if (filter_mask & SOBEL5_BIT)
			{
				num++;
				for (i=0; i<roi_num; ++i)
				*p++ = 0;
			}

			if (filter_mask & SOBEL9_BIT)
			{
				num++;
				isp_u_raw_afm_type2_statistic(isp->handle_device, data);
				for (i=0; i<roi_num; ++i)
					*p++ = data[i];
			}

			if (filter_mask & SPSMD_BIT)
			{
				num++;
				isp_u_raw_afm_type1_statistic(isp->handle_device, data);
				for (i=0; i<roi_num; ++i)
					*p++ = data[i];
			}
		}
	}else{// ring buffer path
		if (ISP_CHIP_ID_TSHARK3 == isp_dev_get_chip_id(isp->handle_device))
		{
			//uint32_t data[105];

			//isp_u_raw_afm_statistic_r6p9(isp->handle_device, data);

			if (filter_mask & SOBEL5_BIT)
			{
				num++;
				for (i=0; i<roi_num; ++i)
					//*p++ = data[1 + i * 3];
					*p++ = af->af_statistics[1 + i * 3];
			}

			if (filter_mask & SOBEL9_BIT)
			{
				num++;
				for (i=0; i<roi_num; ++i)
					//*p++ = data[31 + i * 3];
					*p++ = af->af_statistics[31 + i * 3];
			}

			if (filter_mask & SPSMD_BIT)
			{
				num++;
				for (i=0; i<roi_num; ++i)
				{
					//uint64_t high = data[95 + i / 2];
					uint64_t high = af->af_statistics[95 + i / 2];
					high = (i & 0x01) ? ((high & 0x00FF0000) << 16) : ((high & 0x000000FF) << 32);
					//*p++ = data[61 + i * 3] + high;
					*p++ = af->af_statistics[61 + i * 3] + high;
				}
			}
		}
		else
		{
			//uint32_t data[25];

			if (filter_mask & SOBEL5_BIT)
			{
				num++;
				for (i=0; i<roi_num; ++i)
					*p++ = 0;
			}

			if (filter_mask & SOBEL9_BIT)
			{
				num++;
				//isp_u_raw_afm_type2_statistic(isp->handle_device, data);
				for (i=0; i<roi_num; ++i)
					//*p++ = data[i];
					*p++ = af->af_statistics[i];
			}

			if (filter_mask & SPSMD_BIT)
			{
				num++;
				//isp_u_raw_afm_type1_statistic(isp->handle_device, data);
				for (i=0; i<roi_num; ++i)
					//*p++ = data[i];
					*p++ = af->af_statistics[i];
			}
		}
	}
	return num;
}

// vcm ops
static void set_vcm_chip_ops(af_ctrl_t *af){

	isp_ctrl_context *isp = af->isp_ctx;

	memset(&af->vcm_ops,0,sizeof(af->vcm_ops));
	af->vcm_ops.set_pos = isp->ioctrl_ptr->set_pos;
	af->vcm_ops.get_otp = isp->ioctrl_ptr->get_otp;
	af->vcm_ops.set_motor_bestmode = isp->ioctrl_ptr->set_motor_bestmode;
	af->vcm_ops.set_test_vcm_mode = isp->ioctrl_ptr->set_test_vcm_mode;
	af->vcm_ops.get_test_vcm_mode = isp->ioctrl_ptr->get_test_vcm_mode;
	af->vcm_ops.get_motor_pos = isp->ioctrl_ptr->get_motor_pos;

}

// len
static void lens_move_to(af_ctrl_t *af, int32_t pos)
{
	// AF_LOGD("pos = %d", pos);
	uint16_t last_pos=0;
	if_get_motor_pos(&last_pos, af);

	if( NULL!=af->vcm_ops.set_pos ){
		///*
		while( af->soft_landing_step && af->soft_landing_dly && last_pos != pos){
			if( pos>=last_pos+af->soft_landing_step )
				last_pos+=af->soft_landing_step;
			else if( last_pos>=af->soft_landing_step && pos<=last_pos-af->soft_landing_step )
				last_pos-=af->soft_landing_step;
			else
				last_pos = pos;
			af->vcm_ops.set_pos(last_pos);// must be provided
			usleep(1000*af->soft_landing_dly);
			AF_LOGD("pos,last_pos,step,dly = %d %d %d %d", pos,last_pos,af->soft_landing_step,af->soft_landing_dly);
		}
		//*/
		if( last_pos != pos )
			af->vcm_ops.set_pos(pos);// must be provided
		af->lens.pos = pos;
	}
}

static int32_t lens_get_pos(af_ctrl_t *af)
{
    // AF_LOGD("pos = %d", af->lens.pos);
    return af->lens.pos;
}

static int32_t lens_move_to_infi(af_ctrl_t *af, uint16 pos)
{
	if( NULL!=af->vcm_ops.set_pos ){
		af->vcm_ops.set_pos(pos);// must be provided
	}
       return 0;
}
// notify
enum notify_event
{
    NOTIFY_START = 0,
    NOTIFY_STOP,
};

typedef struct _notify_result
{
    uint32_t win_num;
} notify_result_t;

static int32_t compare_timestamp(af_ctrl_t *af){

	if(af->dcam_timestamp < af->vcm_timestamp+100000000LL)
		return DCAM_AFTER_VCM_NO;
	else
		return DCAM_AFTER_VCM_YES;
}

static void do_notify(isp_ctrl_context *isp, enum notify_event evt, const notify_result_t *result)
{
    struct isp_system *sys = &isp->system;
    struct isp_af_notice param;

    memset(&param, 0, sizeof(param));
    if (NOTIFY_START == evt)
    {
        param.mode = ISP_FOCUS_MOVE_START;
        param.valid_win = 0;
        AF_LOGD("ISP_FOCUS_MOVE_START");
    }
    else // STOP
    {
        param.mode = ISP_FOCUS_MOVE_END;
        param.valid_win = result->win_num;
        AF_LOGD("ISP_FOCUS_MOVE_END");
    }

    if (!sys->isp_callback_bypass)
        sys->callback(sys->caller_id, ISP_CALLBACK_EVT|ISP_AF_NOTICE_CALLBACK, 
            &param, sizeof(param));
}

static void notify_start(af_ctrl_t *af)
{
    do_notify(af->isp_ctx, NOTIFY_START, NULL);
}

static void notify_stop(af_ctrl_t *af, int win_num)
{
	notify_result_t res;
	res.win_num = win_num;

	//load_to_exif_buffer(af);

	af->inited_af_req = (FALSE);
	
	af->vcm_stable = 1;
	if( DCAM_AFTER_VCM_YES==compare_timestamp(af) ){
		sem_post(&af->af_wait_caf);// should be protected by af_work_lock mutex exclusives
	}

	do_notify(af->isp_ctx, NOTIFY_STOP, &res);
}

static int split_win(const win_coord_t *in, int h_num, int v_num, win_coord_t *out)
{
    int h, v, num;
    uint32_t width, height, starty;

    assert(w_num > 0);
    assert(h_num > 0);

    AF_LOGD("win: start_x = %d, start_y = %d, end_x = %d, end_y = %d",
        in->start_x, in->start_y, in->end_x, in->end_y);

    width = (in->end_x - in->start_x + 1) / h_num;
    height = (in->end_y - in->start_y + 1) / v_num;
    starty = in->start_y;
    num = 0;

    for (v = 0; v < v_num; ++v)
    {
        uint32_t startx = in->start_x;
        uint32_t endy = starty + height - 1;

        for (h = 0; h < h_num; ++h)
        {
            out[num].start_x = startx&0xfffffffe;// make sure coordinations are even
            out[num].end_x = (startx + width - 1)&0xfffffffe;
            out[num].start_y = starty&0xfffffffe;
            out[num].end_y = endy&0xfffffffe;
            AF_LOGD("win %d: start_x = %d, start_y = %d, end_x = %d, end_y = %d",
                num, out[num].start_x, out[num].start_y, out[num].end_x, out[num].end_y);

            num++;
            startx += width;
        }

        starty += height;
    }

    return num;
}

static int split_roi(af_ctrl_t *af)
{
    roi_info_t *r = &af->roi;
    win_coord_t w;

    if (1 != r->num)
        return 0;
/*
    w = r->win[0];
    r->num = split_win(&w, 3, 3, r->win);
*/
	switch( af->state ){
	case STATE_FAF:
		w = r->win[0];
		r->num = split_win(&w, 3, 3, r->win);
	break;
	default:
		r->win[1].start_x = r->win[0].start_x+1.0*(r->win[0].end_x - r->win[0].start_x)/5;
		r->win[1].end_x = r->win[0].end_x-1.0*(r->win[0].end_x - r->win[0].start_x)/5;
		r->win[1].start_y = r->win[0].start_y+1.0*(r->win[0].end_y - r->win[0].start_y)/5;
		r->win[1].end_y = r->win[0].end_y-1.0*(r->win[0].end_y - r->win[0].start_y)/5;

		r->win[1].start_x = r->win[1].start_x&0xfffffffe;// make sure coordinations are even
		r->win[1].end_x = r->win[1].end_x&0xfffffffe;
		r->win[1].start_y = r->win[1].start_y&0xfffffffe;
		r->win[1].end_y = r->win[1].end_y&0xfffffffe;

		r->win[2].start_x = r->win[0].start_x+2.0*(r->win[0].end_x - r->win[0].start_x)/5;
		r->win[2].end_x = r->win[0].end_x-2.0*(r->win[0].end_x - r->win[0].start_x)/5;
		r->win[2].start_y = r->win[0].start_y+2.0*(r->win[0].end_y - r->win[0].start_y)/5;
		r->win[2].end_y = r->win[0].end_y-2.0*(r->win[0].end_y - r->win[0].start_y)/5;

		r->win[2].start_x = r->win[2].start_x&0xfffffffe;// make sure coordinations are even
		r->win[2].end_x = r->win[2].end_x&0xfffffffe;
		r->win[2].start_y = r->win[2].start_y&0xfffffffe;
		r->win[2].end_y = r->win[2].end_y&0xfffffffe;

		r->num = 3;
	break;
	}
    return r->num;
}

static void calc_default_roi(af_ctrl_t *af)
{
    isp_info_t *hw = &af->isp_info;
    roi_info_t *roi = &af->roi;

    uint32_t w = hw->width;
    uint32_t h = hw->height;


	switch( af->state ){
	case STATE_FAF:
		roi->num = 1;
		roi->win[0].start_x = af->face_base.sx&0xfffffffe;// make sure coordinations are even
		roi->win[0].start_y = af->face_base.sy&0xfffffffe;
		roi->win[0].end_x   = af->face_base.ex&0xfffffffe;
		roi->win[0].end_y   = af->face_base.ey&0xfffffffe;
	break;
	default:
		roi->num = 1;
		/* center w/3 * h/3 */
		roi->win[0].start_x = ((((w>>1)-(w/6))>>1)<<1)&0xfffffffe;// make sure coordinations are even
		roi->win[0].start_y = ((((h>>1)-(h/6))>>1)<<1)&0xfffffffe;
		roi->win[0].end_x   = ((((w>>1)+(w/6))>>1)<<1)&0xfffffffe;
		roi->win[0].end_y   = ((((h>>1)+(h/6))>>1)<<1)&0xfffffffe;
	break;
	}
	AF_LOGD("af_state %s win 0: start_x = %d, start_y = %d, end_x = %d, end_y = %d",
		STATE_STRING(af->state),roi->win[0].start_x, roi->win[0].start_y, roi->win[0].end_x, roi->win[0].end_y);
}

static void region_calibration(af_ctrl_t *af,win_coord_t* win){

	if( NULL!=win ){/* center w/3 * h/3 */
		uint32_t center_x = (win->start_x+win->end_x)>>1;
		uint32_t center_y = (win->start_y+win->end_y)>>1;

		center_x = center_x<1.0*af->isp_info.width/6 ? 1.0*af->isp_info.width/6 : center_x;
		center_x = center_x>5.0*af->isp_info.width/6 ? 5.0*af->isp_info.width/6 : center_x;
		center_y = center_y<1.0*af->isp_info.height/6 ? 1.0*af->isp_info.height/6 : center_y;
		center_y = center_y>5.0*af->isp_info.height/6 ? 5.0*af->isp_info.height/6 : center_y;

		win->start_x = center_x - 1.0*af->isp_info.width/6;
		win->end_x = center_x + 1.0*af->isp_info.width/6 - 1;
		win->start_y = center_y - 1.0*af->isp_info.height/6;
		win->end_y = center_y + 1.0*af->isp_info.height/6 - 1;

		win->start_x = win->start_x&0xfffffffe;
		win->end_x = win->end_x&0xfffffffe;
		win->start_y = win->start_y&0xfffffffe;
		win->end_y = win->end_y&0xfffffffe;
		AF_LOGD("sx,sy,ex,ey = (%d,%d,%d,%d),cx,cy = (%d,%d)",win->start_x,win->start_y,win->end_x,win->end_y,center_x,center_y);
	}
}

static void calc_roi(af_ctrl_t *af, const struct isp_af_win *win,eAF_MODE alg_mode)
{
	roi_info_t *roi = &af->roi;

	switch( alg_mode ){
		case SAF:
			af->win_config = &af->af_tuning_data.SAF_win;
		break;
		case CAF:
			af->win_config = &af->af_tuning_data.CAF_win;
		break;
		case VAF:
			af->win_config = &af->af_tuning_data.VAF_win;
		break;
		default:
			af->win_config = &af->af_tuning_data.CAF_win;
		break;
	}

	if( 1!=af->af_tuning_data.flag || 1!=af->win_config->win_strategic ){//default window config
		if (win)
			AF_LOGD("valid_win = %d, mode = %d", win->valid_win, win->mode);
		else
			AF_LOGD("win is NULL, use default roi");

		if (!win || (0 == win->valid_win))
		{
			af->touch = 0;
			calc_default_roi(af);
		}
		else
		{
			uint32_t i;

			af->touch = 1;
			roi->num = win->valid_win;
			for (i=0; i<win->valid_win; ++i)
			{
				roi->win[i].start_x = win->win[i].start_x&0xfffffffe;// make sure coordinations are even
				roi->win[i].start_y = win->win[i].start_y&0xfffffffe;
				roi->win[i].end_x = win->win[i].end_x&0xfffffffe;
				roi->win[i].end_y = win->win[i].end_y&0xfffffffe;
				AF_LOGD("win %d: start_x = %d, start_y = %d, end_x = %d, end_y = %d", i,
				roi->win[i].start_x, roi->win[i].start_y, roi->win[i].end_x, roi->win[i].end_y);
			}
			region_calibration(af,&roi->win[0]);
		}
		#if (SPLIT_TEST)
			split_roi(af);
		#endif
	}else{
		if (!win || (0 == win->valid_win))
		{
			uint32_t i=0;

			af->touch = 0;
			roi->num = af->win_config->valid_win_num;
			for( i=0;i<roi->num;i++ ){// the last window is for caf trigger
				roi->win[i].start_x = af->win_config->win_pos[i].start_x&0xfffffffe;// make sure coordinations are even
				roi->win[i].start_y = af->win_config->win_pos[i].start_y&0xfffffffe;
				roi->win[i].end_x = af->win_config->win_pos[i].end_x&0xfffffffe;
				roi->win[i].end_y = af->win_config->win_pos[i].end_y&0xfffffffe;
			}
		}
		else
		{// TAF
			uint32_t i,taf_w,taf_h;
			isp_info_t *hw = &af->isp_info;

			af->touch = 1;
			roi->num = win->valid_win;
			for (i=0; i<win->valid_win; ++i)
			{
				roi->win[i].start_x = win->win[i].start_x&0xfffffffe;// make sure coordinations are even
				roi->win[i].start_y = win->win[i].start_y&0xfffffffe;
				roi->win[i].end_x = win->win[i].end_x&0xfffffffe;
				roi->win[i].end_y = win->win[i].end_y&0xfffffffe;
				AF_LOGD("win %d: start_x = %d, start_y = %d, end_x = %d, end_y = %d", i,
				roi->win[i].start_x, roi->win[i].start_y, roi->win[i].end_x, roi->win[i].end_y);
			}
			region_calibration(af,&roi->win[0]);

			taf_w = roi->win[0].end_x - roi->win[0].start_x;
			taf_h = roi->win[0].end_y - roi->win[0].start_y;

			roi->num = af->win_config->valid_win_num;
			for( i=1;i<roi->num;i++ ){
				roi->win[i].start_x = 1.0*af->win_config->win_pos[i].start_x/hw->width*taf_w+roi->win[0].start_x;
				roi->win[i].start_y = 1.0*af->win_config->win_pos[i].start_y/hw->height*taf_h+roi->win[0].start_y;
				roi->win[i].end_x = 1.0*af->win_config->win_pos[i].end_x/hw->width*taf_w+roi->win[0].start_x;
				roi->win[i].end_y = 1.0*af->win_config->win_pos[i].end_y/hw->height*taf_h+roi->win[0].start_y;

				roi->win[i].start_x = roi->win[i].start_x&0xfffffffe;// make sure coordinations are even
				roi->win[i].start_y = roi->win[i].start_y&0xfffffffe;
				roi->win[i].end_x = roi->win[i].end_x&0xfffffffe;
				roi->win[i].end_y = roi->win[i].end_y&0xfffffffe;
			}
			roi->win[0].start_x = 1.0*af->win_config->win_pos[0].start_x/hw->width*taf_w+roi->win[0].start_x;
			roi->win[0].start_y = 1.0*af->win_config->win_pos[0].start_y/hw->height*taf_h+roi->win[0].start_y;
			roi->win[0].end_x = 1.0*af->win_config->win_pos[0].end_x/hw->width*taf_w+roi->win[0].start_x;
			roi->win[0].end_y = 1.0*af->win_config->win_pos[0].end_y/hw->height*taf_h+roi->win[0].start_y;

			roi->win[0].start_x = roi->win[0].start_x&0xfffffffe;// make sure coordinations are even
			roi->win[0].start_y = roi->win[0].start_y&0xfffffffe;
			roi->win[0].end_x = roi->win[0].end_x&0xfffffffe;
			roi->win[0].end_y = roi->win[0].end_y&0xfffffffe;
		}
	}
}

static void log_roi(af_ctrl_t *af)
{
    int i;

    roi_info_t *r = &af->roi;
    AF_Win *w = &af->fv.AF_Win_Data;

    w->Set_Zone_Num = r->num;
    for (i=0; i<r->num; ++i)
    {
        w->AF_Win_X[i] = r->win[i].start_x;
        w->AF_Win_Y[i] = r->win[i].start_y;
        w->AF_Win_W[i] = r->win[i].end_x - r->win[i].start_x + 1;
        w->AF_Win_H[i] = r->win[i].end_y - r->win[i].start_y + 1;
    }
}

// start hardware
static int do_start_af(af_ctrl_t *af)
{
    isp_ctrl_context *isp = af->isp_ctx;

    //log_roi(af);
    afm_set_win(isp, af->roi.win, af->roi.num, af->isp_info.win_num);
    afm_setup(af);
    afm_enable(isp);
    return 0;
}

// stop hardware
static int do_stop_af(af_ctrl_t *af)
{
    isp_ctrl_context *isp = af->isp_ctx;
    afm_disable(isp);
    return 0;
}

static int faf_trigger_init(af_ctrl_t *af);
// saf stuffs
static void saf_start(af_ctrl_t *af, struct isp_af_win *win)
{
    af->algo_mode = SAF;
    calc_roi(af, win,af->algo_mode);
    AF_Trigger(&af->fv, af->algo_mode, (1==af->defocus)? DEFOCUS : RF_NORMAL);
    do_start_af(af);
    af->vcm_stable = 0;
    faf_trigger_init(af);
}

static void saf_stop(af_ctrl_t *af)
{
   // do_stop_af(af);
    pthread_mutex_lock(&af->af_work_lock);
    AF_STOP(&af->fv, af->algo_mode);
    AF_Process_Frame(&af->fv);
    AF_LOGD("AF_mode = %d", af->fv.AF_mode);
    pthread_mutex_unlock(&af->af_work_lock);
    // let it finish its job
//    AF_Process_Frame(&af->fv);
//    AF_Process_Frame(&af->fv);
}

static int saf_process_frame(af_ctrl_t *af)
{
    AF_Process_Frame(&af->fv);

    // AF_LOGD("AF_mode = %d", af->fv.AF_mode);
    if (Wait_Trigger == af->fv.AF_mode)
    {
        uint8_t res;

        AF_Get_SAF_Result(&af->fv, &res);
        // AF_LOGD("Normal AF end, result = %d", res);

        notify_stop(af, HAVE_PEAK == res ? 1 : 0);
        return 1;
    }
    else
    {
        return 0;
    }
}

// trigger stuffs
#define LOAD_SYMBOL(handle, sym, name) \
{sym=dlsym(handle, name); if(NULL==sym) {AF_LOGE("dlsym fail: %s", name); return -1;}}

static int load_trigger_symbols(af_ctrl_t *af)
{
    LOAD_SYMBOL(af->trig_lib, af->trig_ops.init, "caf_trigger_init");
    LOAD_SYMBOL(af->trig_lib, af->trig_ops.deinit, "caf_trigger_deinit");
    LOAD_SYMBOL(af->trig_lib, af->trig_ops.calc, "caf_trigger_calculation");
    LOAD_SYMBOL(af->trig_lib, af->trig_ops.ioctrl, "caf_trigger_ioctrl");
	return 0;
}

static int load_trigger_lib(af_ctrl_t *af, const char *name)
{
	af->trig_lib = dlopen(name, RTLD_NOW);

    if (NULL == af->trig_lib)
    {
		AF_LOGE("dlopen failed to load: %s", name);
		return -1;
	}

    if (0 != load_trigger_symbols(af))
    {
        dlclose(af->trig_lib);
        af->trig_lib = NULL;
        return -1;
    }

    return 0;
}

static int unload_trigger_lib(af_ctrl_t *af)
{
    if (af->trig_lib)
    {
        dlclose(af->trig_lib);
        af->trig_lib = NULL;
    }
    return 0;
}

static ERRCODE if_get_sys_time(uint64 *time, void *cookie);
static ERRCODE if_aft_binfile_is_exist(uint8* is_exist, void *cookie);
static ERRCODE if_is_aft_mlog(uint32_t *is_save, void *cookie);
static ERRCODE if_aft_log(uint32_t log_level, const char* format, ...);

static int trigger_init(af_ctrl_t *af, const char *lib_name)
{
	struct aft_tuning_block_param aft_in;

	if (0 != load_trigger_lib(af, lib_name))
		return -1;

	struct isp_pm_ioctl_output aft_pm_output;
	memset((void*)&aft_pm_output, 0, sizeof(aft_pm_output));
	isp_pm_ioctl(af->isp_ctx->handle_pm, ISP_PM_CMD_GET_INIT_AFT, NULL, &aft_pm_output);

	if ( PNULL==aft_pm_output.param_data || PNULL==aft_pm_output.param_data[0].data_ptr || 0 == aft_pm_output.param_data[0].data_size) {
		AF_LOGE("aft tuning param error ");
		aft_in.data_len = 0;
		aft_in.data = NULL;
	} else {
		AF_LOGE("aft tuning param ok ");
		aft_in.data_len = aft_pm_output.param_data[0].data_size;
		aft_in.data = aft_pm_output.param_data[0].data_ptr;
	}
	af->trig_ops.handle.aft_ops.aft_cookie = af;
	af->trig_ops.handle.aft_ops.get_sys_time = if_get_sys_time;
	af->trig_ops.handle.aft_ops.binfile_is_exist = if_aft_binfile_is_exist;
	af->trig_ops.handle.aft_ops.is_aft_mlog = if_is_aft_mlog;
	af->trig_ops.handle.aft_ops.aft_log = if_aft_log;

	pthread_mutex_lock(&af->caf_work_lock);
	af->trig_ops.init(&aft_in, &af->trig_ops.handle);
	af->trig_ops.ioctrl(&af->trig_ops.handle, AFT_CMD_GET_AE_SKIP_INFO, &af->trig_ops.ae_skip_info, NULL);
	pthread_mutex_unlock(&af->caf_work_lock);
	return 0;
}

static int trigger_deinit(af_ctrl_t *af)
{
    pthread_mutex_lock(&af->caf_work_lock);
    af->trig_ops.deinit(&af->trig_ops.handle);
    pthread_mutex_unlock(&af->caf_work_lock);
    unload_trigger_lib(af);
    pthread_mutex_destroy(&af->caf_work_lock);
    return 0;
}

static int trigger_set_mode(af_ctrl_t *af)
{
    int32_t mode;

    mode = STATE_CAF == af->state ? AFT_MODE_CONTINUE : AFT_MODE_VIDEO;
    pthread_mutex_lock(&af->caf_work_lock);
    af->trig_ops.ioctrl(&af->trig_ops.handle, AFT_CMD_SET_AF_MODE, &mode, NULL);
    pthread_mutex_unlock(&af->caf_work_lock);
    return 0;
}

static int trigger_start(af_ctrl_t *af)
{
    pthread_mutex_lock(&af->caf_work_lock);
    af->trig_ops.ioctrl(&af->trig_ops.handle, AFT_CMD_SET_CAF_RESET, NULL, NULL);
    pthread_mutex_unlock(&af->caf_work_lock);
    return 0;
}

static int trigger_stop(af_ctrl_t *af)
{
    pthread_mutex_lock(&af->caf_work_lock);
    af->trig_ops.ioctrl(&af->trig_ops.handle, AFT_CMD_SET_CAF_STOP, NULL, NULL);
    pthread_mutex_unlock(&af->caf_work_lock);
    return 0;
}

static int trigger_calc(af_ctrl_t *af, struct aft_proc_calc_param *prm, struct aft_proc_result *res)
{
    pthread_mutex_lock(&af->caf_work_lock);
    af->trig_ops.calc(&af->trig_ops.handle, prm, res);
    pthread_mutex_unlock(&af->caf_work_lock);
    return 0;
}

static void caf_stop_monitor(af_ctrl_t *af);
// caf stuffs
static void caf_start_search(af_ctrl_t *af, int fast)
{
    AF_Trigger(&af->fv, af->algo_mode, fast ? RF_FAST : RF_NORMAL);
    do_start_af(af);
    if (1 == af->need_re_trigger) {
//   	af->need_re_trigger = 0;
	if( af->request_mode!=ISP_FOCUS_CONTINUE && af->request_mode!=ISP_FOCUS_VIDEO ) {
		caf_stop_monitor(af);
	}
   } else {
   	notify_start(af);
   }
    af->vcm_stable = 0;
}

//static void caf_stop_search(af_ctrl_t *af, int32_t res)
static void caf_stop_search(af_ctrl_t *af)
{
//    do_stop_af(af);

    pthread_mutex_lock(&af->af_work_lock);
    AF_STOP(&af->fv, af->algo_mode);
    AF_Process_Frame(&af->fv);
    AF_LOGD("AF_mode = %d", af->fv.AF_mode);
    pthread_mutex_unlock(&af->af_work_lock);


    // let it finish its job
//    AF_Process_Frame(&af->fv);
//    AF_Process_Frame(&af->fv);

//    if (res >= 0)
//        notify_stop(af, res);
}

static void caf_start_monitor(af_ctrl_t *af)
{
    if (af->request_mode ==ISP_FOCUS_CONTINUE || af->request_mode ==ISP_FOCUS_VIDEO) {
	    trigger_start(af);
    }
    do_start_af(af);
}

static void caf_stop_monitor(af_ctrl_t *af)
{
    //do_stop_af(af);
    trigger_stop(af);
}

static void caf_start(af_ctrl_t *af)
{
    AF_LOGD("state = %s, caf_state = %s", STATE_STRING(af->state), CAF_STATE_STR(af->caf_state));

    if( STATE_RECORD_CAF==af->state )
        af->algo_mode = VAF;
    else
        af->algo_mode = CAF;

        calc_roi(af, NULL,af->algo_mode);

    trigger_set_mode(af);

    af->caf_state = CAF_MONITORING;
    caf_start_monitor(af);    
}

static void caf_stop(af_ctrl_t *af)
{
    AF_LOGD("caf_state = %s", CAF_STATE_STR(af->caf_state));

    switch (af->caf_state)
    {
    case CAF_MONITORING:
        caf_stop_monitor(af);
        break;

    case CAF_SEARCHING:
        //caf_stop_search(af, -1);
        caf_stop_search(af);
        break;

    default:
        break;
    }

    af->caf_state = CAF_IDLE;
    return;
}

static void caf_search_process_af(af_ctrl_t *af)
{
    AF_Process_Frame(&af->fv);
 
    //AF_LOGD("AF_mode = %d", af->fv.AF_mode);
    if (Wait_Trigger == af->fv.AF_mode)
    {
        uint8_t res;

        AF_Get_SAF_Result(&af->fv, &res);
        //AF_LOGD("Normal AF end, result = %d", res);

        //caf_stop_search(af, HAVE_PEAK == res ? 1 : 0);
        //do_stop_af(af);
        if (1 == af->need_re_trigger)
		af->need_re_trigger = 0;
        if( (STATE_CAF == af->state) || (STATE_RECORD_CAF == af->state) ){
		if (res >= 0)
                 notify_stop(af, res);
              af->caf_state = CAF_MONITORING;
              caf_start_monitor(af);//need reset trigger
              //do_start_af(af);
        }
        af->caf_first_stable=0;
    }

}

static void caf_monitor_calc(af_ctrl_t *af, struct aft_proc_calc_param *prm)
{
    struct aft_proc_result res;

    memset(&res, 0, sizeof(res));

    trigger_calc(af, prm, &res);
    //AF_LOGD("is_caf_trig = %d, is_need_rough_search = %d", res.is_caf_trig, res.is_need_rough_search);
    if (res.is_caf_trig || TRUE == af->inited_af_req)
    {
        //caf_stop_monitor(af);
        af->caf_state = CAF_SEARCHING;
        caf_start_search(af, !res.is_need_rough_search);
    }else if (res.is_cancel_caf && af->caf_state == CAF_SEARCHING) {
               pthread_mutex_lock(&af->af_work_lock);
               //notify_stop(af, 0);
               af->need_re_trigger = 1;
               AF_STOP(&af->fv, af->algo_mode);
               AF_Process_Frame(&af->fv);
               AF_LOGD("AF_mode = %d", af->fv.AF_mode);
               af->caf_state = CAF_MONITORING;
               pthread_mutex_unlock(&af->af_work_lock);
               do_start_af(af);
     }
}

#define CALC_HIST(sum, gain, pixels, awb_gain, max, hist) \
{uint64_t v=((uint64_t)(sum)*(gain))/((uint64_t)(pixels)*(awb_gain)); \
v=v>(max)?(max):v; hist[v]++;}

static void calc_histogram(af_ctrl_t *af, isp_awb_statistic_hist_info_t *stat)
{
    uint32_t gain_r = af->awb.r_gain;
    uint32_t gain_g = af->awb.g_gain;
    uint32_t gain_b = af->awb.b_gain;
    uint32_t pixels = af->ae.win_size;
    uint32_t awb_base_gain = 1024;
    uint32_t max_value = 255;
    int i,j;

    if (pixels < 1)
        return;

    memset(stat->r_hist, 0, sizeof(stat->r_hist));
    memset(stat->g_hist, 0, sizeof(stat->g_hist));
    memset(stat->b_hist, 0, sizeof(stat->b_hist));

    uint32_t ae_skip_line = 0;
    if (af->trig_ops.ae_skip_info.ae_select_support) {
		ae_skip_line = af->trig_ops.ae_skip_info.ae_skip_line;
    }

    for (i=ae_skip_line; i<(32 - ae_skip_line); ++i)
    {
		for (j=ae_skip_line; j<(32 - ae_skip_line); ++j)
		{
			CALC_HIST(stat->r_info[32*i+j], gain_r, pixels, awb_base_gain, max_value, stat->r_hist);
			CALC_HIST(stat->g_info[32*i+j], gain_g, pixels, awb_base_gain, max_value, stat->g_hist);
			CALC_HIST(stat->b_info[32*i+j], gain_b, pixels, awb_base_gain, max_value, stat->b_hist);
		}
    }
}

static struct aft_proc_calc_param prm_ae;
static void caf_monitor_process_ae(af_ctrl_t *af, const struct ae_calc_out *ae, isp_awb_statistic_hist_info_t *stat)
{
    struct aft_proc_calc_param* prm=&prm_ae;

    memset(prm, 0, sizeof(struct aft_proc_calc_param));

    calc_histogram(af, stat);

    prm->active_data_type = AFT_DATA_IMG_BLK;
	prm->img_blk_info.block_w = 32;
	prm->img_blk_info.block_h = 32;
    prm->img_blk_info.pix_per_blk = af->ae.win_size;
	prm->img_blk_info.chn_num = 3;
	prm->img_blk_info.data = (uint32_t *)stat;
	prm->ae_info.exp_time = ae->cur_exp_line * ae->line_time / 10;
	prm->ae_info.gain = ae->cur_again;
	prm->ae_info.cur_lum = ae->cur_lum;
	prm->ae_info.target_lum = 128;
	prm->ae_info.is_stable = ae->is_stab;
	prm->ae_info.bv = af->ae.bv;
	prm->ae_info.y_sum = af->Y_sum_trigger;
	prm->ae_info.cur_scene = af->curr_scene;
	if_get_motor_pos(&prm->ae_info.registor_pos, af);
    // AF_LOGD("exp_time = %d, gain = %d, cur_lum = %d, is_stable = %d",
    //     prm.ae_info.exp_time, prm.ae_info.gain, prm.ae_info.cur_lum, prm.ae_info.is_stable);

    caf_monitor_calc(af, prm);
}

static struct aft_proc_calc_param prm_af;
static void caf_monitor_process_af(af_ctrl_t *af)
{
    uint64_t fv[10];
    struct aft_proc_calc_param*prm=&prm_af;

    memset(fv, 0, sizeof(fv));
    memset(prm, 0, sizeof(struct aft_proc_calc_param));

    //afm_get_fv(af->isp_ctx, fv, SPSMD_BIT, 1);
	if( 1!=af->af_tuning_data.flag || 1!=af->win_config->win_strategic ){//default window config
		#if !(SPLIT_TEST)
		afm_get_fv(af->isp_ctx, fv, SPSMD_BIT, 1, AF_RING_BUFFER);
		#else
		{
/*
			int i;
			uint64_t sum;
*/
			afm_get_fv(af->isp_ctx, fv, SPSMD_BIT, 3, AF_RING_BUFFER);
/*
			sum = 0;
			for (i=0; i<9; ++i)
			sum += fv[i];
			AF_LOGD("spsmd %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld: %lld",
			fv[0], fv[1], fv[2], fv[3], fv[4], fv[5], fv[6],
			fv[7], fv[8], sum);
			fv[0] = sum;
*/
		}
		#endif
	}else{
		afm_get_fv(af->isp_ctx, fv, SPSMD_BIT, af->roi.num, AF_RING_BUFFER);
		AF_LOGD("spsmd %lld",fv[af->roi.num-1]);
		fv[0] = fv[af->roi.num-1];// the fv in last window is for caf trigger
	}

    prm->afm_info.win_cfg.win_cnt = 1;
    prm->afm_info.win_cfg.win_pos[0].sx = af->roi.win[0].start_x;
    prm->afm_info.win_cfg.win_pos[0].sy= af->roi.win[0].start_y;
    prm->afm_info.win_cfg.win_pos[0].ex = af->roi.win[0].end_x;
    prm->afm_info.win_cfg.win_pos[0].ey = af->roi.win[0].end_y;
    prm->afm_info.filter_info.filter_num = 1;
    prm->afm_info.filter_info.filter_data[0].data = fv;
    prm->afm_info.filter_info.filter_data[0].type = 1;
    prm->active_data_type = AFT_DATA_AF;

    caf_monitor_calc(af, prm);
}

static void caf_process_af(af_ctrl_t *af)
{
    //pthread_mutex_lock(&af->caf_lock);

    // AF_LOGD("caf_state = %s", CAF_STATE_STR(af->caf_state));

    switch (af->caf_state)
    {
    case CAF_SEARCHING:
	 pthread_mutex_lock(&af->af_work_lock);
        caf_search_process_af(af);
	 pthread_mutex_unlock(&af->af_work_lock);
	 caf_monitor_process_af(af);
        break;

    case CAF_MONITORING:
        caf_monitor_process_af(af);
        break;

    default:
        break;
    }

    //pthread_mutex_unlock(&af->caf_lock);
}

static void caf_process_ae(af_ctrl_t *af, const struct ae_calc_out *ae, isp_awb_statistic_hist_info_t *stat)
{
    //pthread_mutex_lock(&af->caf_lock);

    // AF_LOGD("caf_state = %s", CAF_STATE_STR(af->caf_state));
    
    //if (CAF_MONITORING == af->caf_state)
    {
        caf_monitor_process_ae(af, ae, stat);
    }

    //pthread_mutex_unlock(&af->caf_lock);
}

static void suspend_caf(af_ctrl_t *af)
{
    AF_LOGD("state = %s, pre_state = %s", STATE_STRING(af->state), STATE_STRING(af->pre_state));
    assert((STATE_CAF == af->state) || (STATE_RECORD_CAF == af->state));

    af->pre_state = af->state;
    af->state = STATE_IDLE;
    caf_stop(af);
}

static void resume_caf(af_ctrl_t *af)
{
    AF_LOGD("state = %s, pre_state = %s", STATE_STRING(af->state), STATE_STRING(af->pre_state));
    assert((STATE_CAF == af->pre_state) || (STATE_RECORD_CAF == af->pre_state));

    af->state = af->pre_state;
    af->pre_state = STATE_IDLE;
    caf_start(af);
}

static int try_suspend_caf(af_ctrl_t *af)
{
    if ((STATE_CAF == af->state) || (STATE_RECORD_CAF == af->state))
    {
        suspend_caf(af);
        return 1;
    }
    else
    {
        return 0;
    }
}

static int try_resume_caf(af_ctrl_t *af)
{
    if (!af->flash_on
        && ((STATE_CAF == af->pre_state) || (STATE_RECORD_CAF == af->pre_state))
        && (STATE_IDLE == af->state))
    {
        resume_caf(af);
        return 1;
    }
    else
    {
        return 0;
    }
}

// faf stuffs
static int faf_process_frame(af_ctrl_t *af)
{
    AF_Process_Frame(&af->fv);

    // AF_LOGD("AF_mode = %d", af->fv.AF_mode);
    if (Wait_Trigger == af->fv.AF_mode)
    {
        uint8_t res;

        AF_Get_SAF_Result(&af->fv, &res);
        // AF_LOGD("Normal AF end, result = %d", res);

        //notify_stop(af, HAVE_PEAK == res ? 1 : 0);
        return 1;
    }
    else
    {
        return 0;
    }
}

static void faf_start(af_ctrl_t *af, struct isp_af_win *win)
{
    af->algo_mode = CAF;
    calc_roi(af, win,af->algo_mode);
    AF_Trigger(&af->fv, af->algo_mode, RF_NORMAL);
    do_start_af(af);
    af->vcm_stable = 0;
}

static int32_t face_dectect_trigger(af_ctrl_t *af){

	uint16_t i=0,max_index=0,trigger=0;
	uint32_t diff_x=0,diff_y=0,diff_area=0;
	uint64_t max_area=0,area=0;
	isp_info_t* isp_size = &af->isp_info;
	struct isp_face_area *face_info = &af->face_info;
	prime_face_base_info_t*  face_base = &af->face_base;
	roi_info_t *roi = &af->roi;

	max_index = face_info->face_num;
	while( i<face_info->face_num ){// pick face of maximum size
		AF_LOGD("face_area%d (sx ex sy ey) = (%d %d %d %d) ",i,face_info->face_info[i].sx,face_info->face_info[i].ex,face_info->face_info[i].sy,face_info->face_info[i].ey);
		area = (face_info->face_info[i].ex - face_info->face_info[i].sx)*(face_info->face_info[i].ey - face_info->face_info[i].sy);
		if( max_area<area ){
			max_index = i;
			max_area = area;
		}
		i++;
	}

	if( max_index==face_info->face_num )
		return 0;

	diff_x = face_base->ex+face_base->sx > face_info->face_info[max_index].ex + face_info->face_info[max_index].sx ?
		face_base->ex + face_base->sx - face_info->face_info[max_index].ex - face_info->face_info[max_index].sx :
		face_info->face_info[max_index].ex + face_info->face_info[max_index].sx - face_base->ex - face_base->sx ;
	diff_y = face_base->ey+face_base->sy > face_info->face_info[max_index].ey + face_info->face_info[max_index].sy ?
		face_base->ey + face_base->sy - face_info->face_info[max_index].ey - face_info->face_info[max_index].sy :
		face_info->face_info[max_index].ey + face_info->face_info[max_index].sy - face_base->ey - face_base->sy ;
	diff_area = face_base->area > max_area ? face_base->area - max_area : max_area - face_base->area ;

	if( (face_base->ex+face_base->sx)*face_base->diff_cx_thr<100*diff_x &&
		(face_base->ey+face_base->sy)*face_base->diff_cy_thr<100*diff_y &&
		face_base->area*face_base->diff_area_thr<100*diff_area ){
		AF_LOGD("diff_cx diff_cy diff_area = %f %f %f",1.0*diff_x/(face_base->ex+face_base->sx),
					1.0*diff_y/(face_base->ey+face_base->sy),
					1.0*diff_area/face_base->area);
		face_base->sx = face_info->face_info[max_index].sx ;// update base face
		face_base->ex = face_info->face_info[max_index].ex ;
		face_base->sy = face_info->face_info[max_index].sy ;
		face_base->ey = face_info->face_info[max_index].ey ;
		face_base->area = max_area;

		face_base->diff_trigger = 1;
		face_base->converge_cnt = 0;
		if( 0==face_base->area )
			face_base->converge_cnt = face_base->converge_cnt_thr+1;
	}else{
		if( 1==face_base->diff_trigger )
			face_base->converge_cnt++;
		else
			face_base->converge_cnt = 0;
	}

	if( 1==face_base->diff_trigger && face_base->converge_cnt>face_base->converge_cnt_thr ){
		face_base->diff_trigger = 0;
		face_base->converge_cnt = 0;
		trigger = 1;
	}

	return trigger;
}

static int faf_trigger_init(af_ctrl_t *af){// all thrs are in percentage unit
	af->face_base.diff_area_thr = 40;
	af->face_base.diff_cx_thr = 80;
	af->face_base.diff_cy_thr = 80;
	af->face_base.converge_cnt_thr = 15;

	af->face_base.sx = 0 ;// update base face
	af->face_base.ex = 0 ;
	af->face_base.sy = 0 ;
	af->face_base.ey = 0 ;
	af->face_base.area = 0;

	af->face_base.diff_trigger = 0;
	af->face_base.converge_cnt = 0;

	return 0;
}


// misc
static uint64_t get_systemtime_ns()
{
	int64_t timestamp = systemTime(CLOCK_MONOTONIC);
    return timestamp;
}

static uint32_t Is_ae_stable(af_ctrl_t *af){

	if( (STATE_CAF==af->state||STATE_RECORD_CAF==af->state) && 
		0==af->caf_first_stable && 1==af->ae.stable ){
		af->caf_first_stable=1;
		return 1;
	}

	if( 1==af->caf_first_stable )
		return 1;

	return af->ae.stable;

}

static void get_isp_size(isp_ctrl_context *isp, uint16_t *widith, uint16_t *height)
{

	*widith = isp->input_size_trim[isp->param_index].width;
	*height = isp->input_size_trim[isp->param_index].height;

}

static void ae_calibration(af_ctrl_t *af ,struct isp_awb_statistic_info* rgb){
	uint32_t i, j, r_sum[9], g_sum[9], b_sum[9];

	memset(r_sum,0,sizeof(r_sum));
	memset(g_sum,0,sizeof(g_sum));
	memset(b_sum,0,sizeof(b_sum));

	for (i = 0; i < 32; i++) {
		for (j = 0; j < 32; j++) {
			r_sum[(i/11)*3 + j/11] += rgb->r_info[i*32 + j]/af->ae.win_size;
			g_sum[(i/11)*3 + j/11] += rgb->g_info[i*32 + j]/af->ae.win_size;
			b_sum[(i/11)*3 + j/11] += rgb->b_info[i*32 + j]/af->ae.win_size;
		}
	}
	af->ae_cali_data.r_avg[0] = r_sum[0]/121; af->ae_cali_data.r_avg_all = af->ae_cali_data.r_avg[0];
	af->ae_cali_data.r_avg[1] = r_sum[1]/121; af->ae_cali_data.r_avg_all += af->ae_cali_data.r_avg[1];
	af->ae_cali_data.r_avg[2] = r_sum[2]/110; af->ae_cali_data.r_avg_all += af->ae_cali_data.r_avg[2];
	af->ae_cali_data.r_avg[3] = r_sum[3]/121; af->ae_cali_data.r_avg_all += af->ae_cali_data.r_avg[3];
	af->ae_cali_data.r_avg[4] = r_sum[4]/121; af->ae_cali_data.r_avg_all += af->ae_cali_data.r_avg[4];
	af->ae_cali_data.r_avg[5] = r_sum[5]/110; af->ae_cali_data.r_avg_all += af->ae_cali_data.r_avg[5];
	af->ae_cali_data.r_avg[6] = r_sum[6]/110; af->ae_cali_data.r_avg_all += af->ae_cali_data.r_avg[6];
	af->ae_cali_data.r_avg[7] = r_sum[7]/110; af->ae_cali_data.r_avg_all += af->ae_cali_data.r_avg[7];
	af->ae_cali_data.r_avg[8] = r_sum[8]/100; af->ae_cali_data.r_avg_all += af->ae_cali_data.r_avg[8];
	af->ae_cali_data.r_avg_all /= 9;

	af->ae_cali_data.g_avg[0] = g_sum[0]/121; af->ae_cali_data.g_avg_all = af->ae_cali_data.g_avg[0];
	af->ae_cali_data.g_avg[1] = g_sum[1]/121; af->ae_cali_data.g_avg_all += af->ae_cali_data.g_avg[1];
	af->ae_cali_data.g_avg[2] = g_sum[2]/110; af->ae_cali_data.g_avg_all += af->ae_cali_data.g_avg[2];
	af->ae_cali_data.g_avg[3] = g_sum[3]/121; af->ae_cali_data.g_avg_all += af->ae_cali_data.g_avg[3];
	af->ae_cali_data.g_avg[4] = g_sum[4]/121; af->ae_cali_data.g_avg_all += af->ae_cali_data.g_avg[4];
	af->ae_cali_data.g_avg[5] = g_sum[5]/110; af->ae_cali_data.g_avg_all += af->ae_cali_data.g_avg[5];
	af->ae_cali_data.g_avg[6] = g_sum[6]/110; af->ae_cali_data.g_avg_all += af->ae_cali_data.g_avg[6];
	af->ae_cali_data.g_avg[7] = g_sum[7]/110; af->ae_cali_data.g_avg_all += af->ae_cali_data.g_avg[7];
	af->ae_cali_data.g_avg[8] = g_sum[8]/100; af->ae_cali_data.g_avg_all += af->ae_cali_data.g_avg[8];
	af->ae_cali_data.g_avg_all /= 9;

	af->ae_cali_data.b_avg[0] = b_sum[0]/121; af->ae_cali_data.b_avg_all = af->ae_cali_data.b_avg[0];
	af->ae_cali_data.b_avg[1] = b_sum[1]/121; af->ae_cali_data.b_avg_all += af->ae_cali_data.b_avg[1];
	af->ae_cali_data.b_avg[2] = b_sum[2]/110; af->ae_cali_data.b_avg_all += af->ae_cali_data.b_avg[2];
	af->ae_cali_data.b_avg[3] = b_sum[3]/121; af->ae_cali_data.b_avg_all += af->ae_cali_data.b_avg[3];
	af->ae_cali_data.b_avg[4] = b_sum[4]/121; af->ae_cali_data.b_avg_all += af->ae_cali_data.b_avg[4];
	af->ae_cali_data.b_avg[5] = b_sum[5]/110; af->ae_cali_data.b_avg_all += af->ae_cali_data.b_avg[5];
	af->ae_cali_data.b_avg[6] = b_sum[6]/110; af->ae_cali_data.b_avg_all += af->ae_cali_data.b_avg[6];
	af->ae_cali_data.b_avg[7] = b_sum[7]/110; af->ae_cali_data.b_avg_all += af->ae_cali_data.b_avg[7];
	af->ae_cali_data.b_avg[8] = b_sum[8]/100; af->ae_cali_data.b_avg_all += af->ae_cali_data.b_avg[8];
	af->ae_cali_data.b_avg_all /= 9;

	ISP_LOGD("(r,g,b) in block4 is (%d,%d,%d)",af->ae_cali_data.r_avg[4],af->ae_cali_data.g_avg[4],af->ae_cali_data.b_avg[4]);
}


static void set_af_RGBY(af_ctrl_t *af ,struct isp_awb_statistic_info* rgb)
{
#define AE_BLOCK_W 32
#define AE_BLOCK_H 32

	uint32_t Y_sx=0,Y_ex=0,Y_sy=0,Y_ey=0,r_sum=0,g_sum=0,b_sum=0,y_sum=0;
	float af_area,ae_area;
	uint16_t width,height,i=0,blockw,blockh,index;

	get_isp_size(af->isp_ctx,&width,&height);
	af->roi_RGBY.num = af->roi.num;

	af->roi_RGBY.Y_sum[af->roi.num] = 0;
	for(i=0; i<af->roi.num; i++){
		Y_sx = af->roi.win[i].start_x / (width/AE_BLOCK_W) ;
		Y_ex = af->roi.win[i].end_x / (width/AE_BLOCK_W) ;
		Y_sy = af->roi.win[i].start_y / (height/AE_BLOCK_H) ;
		Y_ey = af->roi.win[i].end_y / (height/AE_BLOCK_H) ;
		//exception
		if( Y_sx==Y_ex )
			Y_ex = Y_sx+1;
		//exception
		if( Y_sy==Y_ey )
			Y_ey = Y_sy+1;

		r_sum = 0;g_sum = 0;b_sum = 0;
		for(blockw=Y_sx ; blockw<Y_ex ; blockw++)
		{
			for(blockh=Y_sy ; blockh<Y_ey ; blockh++)
			{
				index = blockw * AE_BLOCK_W + blockh ;
				r_sum = r_sum + rgb->r_info[index] ;
				g_sum = g_sum + rgb->g_info[index] ;
				b_sum = b_sum + rgb->b_info[index] ;
			}
		}

		//af_area =  (af->roi.win[i].end_x - af->roi.win[i].start_x + 1) * (af->roi.win[i].end_y - af->roi.win[i].start_y + 1) ;
		ae_area = 1.0 * ((Y_ex - Y_sx) /** (width/AE_BLOCK_W)*/) * ((Y_ey - Y_sy)/* * (height/AE_BLOCK_H)*/) ;
		y_sum = (( (0.299 * r_sum) + (0.587 * g_sum) + (0.114 * b_sum) ) /ae_area) ;
		af->roi_RGBY.Y_sum[i] = y_sum;
		af->roi_RGBY.R_sum[i] = r_sum;
		af->roi_RGBY.G_sum[i] = g_sum;
		af->roi_RGBY.B_sum[i] = b_sum;
		af->roi_RGBY.Y_sum[af->roi.num] += y_sum;
		//AF_LOGD("y_sum[%d] = %d",i,y_sum);
	}

	switch( af->state ){
	case STATE_FAF:
		af->Y_sum_trigger= af->roi_RGBY.Y_sum[af->roi.num];
		af->Y_sum_normalize = af->roi_RGBY.Y_sum[af->roi.num];
	break;
	default:
		af->Y_sum_trigger= af->roi_RGBY.Y_sum[1];
		af->Y_sum_normalize = af->roi_RGBY.Y_sum[1];
	break;
	}

	property_get("af_mode",AF_MODE,"none");
	if( 0!=strcmp(AF_MODE,"none") ){
		ae_calibration(af ,rgb );
	}


}

// i/f to AF model
static ERRCODE if_af_start_notify(eAF_MODE  AF_mode, void *cookie){
    af_ctrl_t *af = cookie;
    log_roi(af);
/*
    struct isp_system *sys = &af->isp_ctx->system;
    struct isp_af_notice param;

    memset(&param, 0, sizeof(param));

    param.mode = ISP_FOCUS_MOVE_START;
    param.valid_win = 0;
    AF_LOGD("ISP_FOCUS_MOVE_START");

    if (!sys->isp_callback_bypass)
        sys->callback(sys->caller_id, ISP_CALLBACK_EVT|ISP_AF_NOTICE_CALLBACK,
            &param, sizeof(param));
*/
    return 0;
}

static ERRCODE if_af_end_notify(eAF_MODE  AF_mode, void *cookie){
/*
    af_ctrl_t *af = cookie;
    struct isp_system *sys = &af->isp_ctx->system;
    struct isp_af_notice param;
    uint8_t res;

    AF_Get_SAF_Result(&af->fv, &res);
    param.mode = ISP_FOCUS_MOVE_END;
    param.valid_win = res;
    AF_LOGD("ISP_FOCUS_MOVE_END");

    if (!sys->isp_callback_bypass)
        sys->callback(sys->caller_id, ISP_CALLBACK_EVT|ISP_AF_NOTICE_CALLBACK,
            &param, sizeof(param));
*/
    return 0;
}

static ERRCODE if_statistics_wait_cal_done(void *cookie)
{
    return 0;
}

static ERRCODE if_statistics_get_data(uint64 fv[T_TOTAL_FILTER_TYPE], void *cookie)
{
    af_ctrl_t *af = cookie;
    isp_ctrl_context *isp = af->isp_ctx;
    uint64_t spsmd[MAX_ROI_NUM];
	
    memset(fv, 0, sizeof(fv[0]) * T_TOTAL_FILTER_TYPE);

	if( 1!=af->af_tuning_data.flag || 1!=af->win_config->win_strategic ){//default window config
		#if !(SPLIT_TEST)
		afm_get_fv(isp, spsmd, SPSMD_BIT, 1, AF_RING_BUFFER);
		fv[T_SPSMD] = spsmd[0]; // TODO only one window now
		// AF_LOGD("spsmd g = %lld", fv[T_SPSMD]);
		#else
		{
			//int i;
			uint64_t sum;
			
			sum = 0;
			memset(&(spsmd[0]),0,sizeof(uint64_t)*MAX_ROI_NUM);
			
			afm_get_fv(isp, spsmd, SPSMD_BIT, af->roi.num, AF_RING_BUFFER);
			
			//sum = 0.2*spsmd[0]+spsmd[1]+3*spsmd[2];
			switch( af->state ){
			case STATE_FAF:
				sum = spsmd[0]+spsmd[1]+spsmd[2]+spsmd[3]+8*spsmd[4]+spsmd[5]+spsmd[6]+spsmd[7]+spsmd[8];
				fv[T_SPSMD] = sum; 	
			break;
			default:
				sum = spsmd[1]+8*spsmd[2];
				fv[T_SPSMD] = sum;
			break;
			}
			AF_LOGD("[%d][%d]spsmd sum %lld",af->state,af->roi.num, sum);
/*
			sum = 0;
			for (i=0; i<4; ++i)
				sum += spsmd[i];

			sum += (spsmd[4]<<3);

			for (i=5; i<9; ++i)
				sum += spsmd[i];
			//        sum = sum>>4;
			fv[T_SPSMD] = sum;
			AF_LOGD("spsmd %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld: %lld",
			spsmd[0], spsmd[1], spsmd[2], spsmd[3], spsmd[4], spsmd[5], spsmd[6],
			spsmd[7], spsmd[8], sum);
*/
		}
		#endif
	}else{
		int i;
		uint64_t sum;

		afm_get_fv(isp, spsmd, SPSMD_BIT, af->roi.num, AF_RING_BUFFER);
		sum = 0;
		for (i=0; i<af->roi.num; ++i)// for caf, the weight in last window is 0
			sum += spsmd[i]*af->win_config->win_weight[i];
		fv[T_SPSMD] = sum;
		AF_LOGD("[%d]spsmd sum %lld",af->roi.num, sum);
	}

	return 0;
}
	
static ERRCODE if_lens_get_pos(uint16 *pos, void *cookie)
{
    af_ctrl_t *af = cookie;
    *pos = lens_get_pos(af);
    return 0;
}

static ERRCODE if_lens_move_to(uint16 pos, void *cookie)
{
    af_ctrl_t *af = cookie;

    lens_move_to(af, pos);
    return 0;
}
	
static ERRCODE if_lens_wait_stop(void *cookie)
{
    return 0;
}

static ERRCODE if_lock_ae(e_LOCK lock, void *cookie)
{
    af_ctrl_t *af = cookie;
    isp_ctrl_context *isp = af->isp_ctx;
    enum ae_io_ctrl_cmd cmd;
    struct ae_calc_out out;

    AF_LOGD("%s, lock_num = %d", LOCK == lock ? "lock" : "unlock", af->ae_lock_num);

    if (LOCK == lock)
    {
        af->ae_lock_num++;
        cmd = AE_SET_PAUSE;
        isp->lock_ae = 1;

    }
    else
    {
        af->ae_lock_num--;
        cmd = AE_SET_RESTORE;
        isp->lock_ae = 0;
    }

    memset(&out, 0, sizeof(out));
    isp->ae_lib_fun->ae_io_ctrl(isp->handle_ae, cmd, NULL, &out);
    return 0;
}

static ERRCODE if_lock_awb(e_LOCK lock, void *cookie)
{
    af_ctrl_t *af = cookie;
    isp_ctrl_context *isp = af->isp_ctx;
    enum awb_ctrl_cmd cmd;

    AF_LOGD("%s, lock_num = %d", LOCK == lock ? "lock" : "unlock", af->awb_lock_num);

    if (LOCK == lock)
    {
        af->awb_lock_num++;
        cmd = AWB_CTRL_CMD_LOCK;
    }
    else
    {
        af->awb_lock_num--;
        cmd = AWB_CTRL_CMD_UNLOCK;
    }

    isp->awb_lib_fun->awb_ctrl_ioctrl(isp->handle_awb, cmd, NULL, NULL);
    return 0;
}

isp_s32 _set_nlm_lock_state(uint8_t state);
static ERRCODE if_lock_nlm(e_LOCK lock, void *cookie)
{
    af_ctrl_t *af = cookie;
    isp_ctrl_context *isp = af->isp_ctx;
    enum awb_ctrl_cmd cmd;

    AF_LOGD("%s, lock_num = %d", LOCK == lock ? "lock" : "unlock", af->lsc_lock_num);

    if (LOCK == lock)
    {
        cmd = 1;
    }
    else
    {
        cmd = 0;
    }


	//lsc_adv_ioctrl(isp->handle_lsc_adv, cmd, NULL, NULL);
	_set_nlm_lock_state(cmd);
    return 0;
}

static ERRCODE if_lock_lsc(e_LOCK lock, void *cookie)
{
    af_ctrl_t *af = cookie;
    isp_ctrl_context *isp = af->isp_ctx;
    enum awb_ctrl_cmd cmd;

    AF_LOGD("%s, lock_num = %d", LOCK == lock ? "lock" : "unlock", af->lsc_lock_num);

    if (LOCK == lock)
    {
        af->lsc_lock_num++;
        cmd = SMART_LSC_ALG_LOCK;
    }
    else
    {
        af->lsc_lock_num--;
        cmd = SMART_LSC_ALG_UNLOCK;
    }


	lsc_adv_ioctrl(isp->handle_lsc_adv, cmd, NULL, NULL);
	//	_smart_io_ctrl(isp,cmd);
    return 0;
}

static ERRCODE if_get_sys_time(uint64 *time, void *cookie)
{
    *time = get_systemtime_ns();
    return 0;
}

static ERRCODE if_sys_sleep_time(uint16 sleep_time,void *cookie)
{
    af_ctrl_t *af = (af_ctrl_t *)cookie;

    af->vcm_timestamp = get_systemtime_ns();
    usleep(sleep_time*1000);
    return 0;
}

static ERRCODE if_get_ae_report(AE_Report *rpt, void *cookie)
{
    af_ctrl_t *af = (af_ctrl_t *)cookie;
    ae_info_t *ae = &af->ae;

    rpt->bAEisConverge = ae->stable;
    rpt->AE_BV         = ae->bv;
    rpt->AE_EXP        = ae->exp / 10000; // 0.1us -> ms
    rpt->AE_Gain       = ae->gain;
    rpt->AE_Pixel_Sum = af->Y_sum_normalize;
    return 0;
}

static ERRCODE if_set_af_exif(const void *data, void *cookie)
{
    // TODO
    return 0;
}

static ERRCODE if_get_otp(AF_OTP_Data *pAF_OTP, void *cookie)
{
	// TODO
	af_ctrl_t *af = cookie;
	isp_ctrl_context *isp = af->isp_ctx;


	// get otp
	if( NULL!=af->vcm_ops.get_otp ){
		af->vcm_ops.get_otp(&pAF_OTP->INF,&pAF_OTP->MACRO);
		af->fv.AF_OTP.bIsExist = T_LENS_BY_OTP;
		AF_LOGD("otp (infi,macro) = (%d,%d)",pAF_OTP->INF,pAF_OTP->MACRO);
	}

	return 0;
}

static ERRCODE if_get_motor_pos(uint16 *motor_pos, void *cookie)
{
	// TODO
	af_ctrl_t *af = cookie;
	isp_ctrl_context *isp = af->isp_ctx;
	// read
	if( NULL!=af->vcm_ops.get_motor_pos ){
		af->vcm_ops.get_motor_pos(motor_pos);
		AF_LOGD("motor pos in register %d",*motor_pos);
	}

	return 0;
}

static ERRCODE if_set_motor_sacmode(void *cookie)
{
	// TODO
	af_ctrl_t *af = cookie;
	isp_ctrl_context *isp = af->isp_ctx;

	if( NULL!=af->vcm_ops.set_motor_bestmode )
		af->vcm_ops.set_motor_bestmode();

	return 0;
}

static ERRCODE if_binfile_is_exist(uint8* bisExist, void *cookie){
	// TODO
	af_ctrl_t *af = cookie;

	char* af_tuning_path = "/data/mlog/af_tuning.bin";
	FILE* fp = NULL;

	if( 0==access(af_tuning_path,R_OK) ){//read request successs
		uint32_t len = 0;

		fp = fopen(af_tuning_path,"rb");
		if( NULL==fp ){
			*bisExist=0;
			return 0;
		}

		fseek(fp,0,SEEK_END);
		len = ftell(fp);
		if( sizeof(af->af_tuning_data)!=len ){
			ISP_LOGD("af_tuning.bin len dismatch with af_alg len %d",sizeof(af->af_tuning_data));
			fclose(fp);
			*bisExist=0;
			return 0;
		}

		fseek(fp,0,SEEK_SET);
		fread(&af->af_tuning_data,1,len,fp);
		fclose(fp);

		if( 0==af->af_tuning_data.flag ){
			ISP_LOGD("skip af_tuning");
			*bisExist=0;
		}else{
			af->soft_landing_dly = af->af_tuning_data.soft_landing_dly;
			af->soft_landing_step = af->af_tuning_data.soft_landing_step;
			memcpy(&af->filter_clip[0],&af->af_tuning_data.filter_clip[0],sizeof(af->af_tuning_data.filter_clip));
			memcpy(&af->bv_threshold[0],&af->af_tuning_data.bv_threshold[0],sizeof(af->af_tuning_data.bv_threshold));
			af->fv.AF_OTP.bIsExist = T_LENS_BY_TUNING;
			memcpy(&af->fv.AF_Tuning_Data,&af->af_tuning_data.AF_Tuning_Data[INDOOR_SCENE],sizeof(af->fv.AF_Tuning_Data));
			af->pre_scene = INDOOR_SCENE;
			ISP_LOGD("load af_tuning succeed");
			*bisExist=1;
		}
	}else{
		if( T_LENS_BY_TUNING == af->fv.AF_OTP.bIsExist || 1==af->af_tuning_data.flag ){// tuning form sensor file
			*bisExist=1;
		}else{
			*bisExist=0;
		}
	}

	{	//andrew: how to use OTP data to optimize scan table
		char value[20] = {'\0'};

		property_get("af_set_opt_value", value, "none");//infi~macro
		if( 0!=strcmp(value,"none") ){
			char* p1=value;
			af->fv.AF_OTP.bIsExist = T_LENS_BY_OTP;
			while( *p1!='~'  && *p1!='\0' )
				p1++;
			*p1++ = '\0';
			af->fv.AF_OTP.INF = atoi(value);
			af->fv.AF_OTP.MACRO = atoi(p1);
			ISP_LOGD("adb AF OPT succeed (INFI MACRO)=(%d %d)",af->fv.AF_OTP.INF,af->fv.AF_OTP.MACRO);
		}
	}

	return 0;
}
static char AFlog_buffer[2048]={0};
static ERRCODE if_af_log(const char* format, ...)
{
//	char buffer[2048]={0};
	va_list arg;
	va_start (arg, format);
	vsnprintf(AFlog_buffer, 2048, format, arg);
	va_end (arg);
	AF_LOGE("ISP_AFv1: %s",AFlog_buffer);

	return 0;
}

static ERRCODE if_aft_log(uint32_t log_level,  const char* format, ...)
{
//	char buffer[2048]={0};
	va_list arg;
	va_start (arg, format);
	vsnprintf(AFlog_buffer, 2048, format, arg);
	va_end (arg);
	switch(log_level) {
	case AFT_LOG_VERBOSE:
		ALOGV("%s",AFlog_buffer);
		break;
	case AFT_LOG_DEBUG:
		ALOGD("%s",AFlog_buffer);
		break;
	case AFT_LOG_INFO:
		ALOGI("%s",AFlog_buffer);
		break;
	case AFT_LOG_WARN:
		ALOGW("%s",AFlog_buffer);
		break;
	case AFT_LOG_ERROR:
		ALOGE("%s",AFlog_buffer);
		break;
	default:
		AF_LOGE("default log level not support");
		break;
	}

	return 0;
}

static ERRCODE if_aft_binfile_is_exist(uint8* is_exist, void *cookie){

	af_ctrl_t *af = cookie;
	char* aft_tuning_path = "/data/mlog/aft_tuning_j3.bin";
	FILE* fp = NULL;

	if( 0==access(aft_tuning_path,R_OK) ){//read request successs
		uint16_t len = 0;

		fp = fopen(aft_tuning_path,"rb");
		if( NULL==fp ){
			*is_exist=0;
			return 0;
		}

		fseek(fp,0,SEEK_END);
		len = ftell(fp);
		if( len != af->trig_ops.handle.tuning_param_len){
			AF_LOGE("aft_tuning.bin len dismatch with aft_alg len %d",af->trig_ops.handle.tuning_param_len);
			fclose(fp);
			*is_exist=0;
			return 0;
		}

		fclose(fp);
		*is_exist=1;
	}else{
		*is_exist=0;
	}
	return 0;
}

static ERRCODE if_is_aft_mlog(uint32_t *is_save, void *cookie)
{
#ifndef WIN32
	char value[20] = {'\0'};

	property_get(AF_SAVE_MLOG_STR, value, "no");

	if (!strcmp(value, "save")) {
		*is_save = 1;
	}
#endif
	AF_LOGD("is_save %d",*is_save);
	return 0;
}
/* interface */

/* initialization */
static void load_settings(af_ctrl_t *af,struct isp_pm_ioctl_output* af_pm_output)
{
	// TODO: load from tuning parameters
	//tuning data from common_mode
	uint8_t i=0;
	for( i=0;i<GAIN_TOTAL;i++ ){
		af->filter_clip[OUT_SCENE][i].spsmd_max = 0xFFFF;
		af->filter_clip[OUT_SCENE][i].spsmd_min = 0x0000;
		af->filter_clip[OUT_SCENE][i].sobel_max = 0xFFFF;
		af->filter_clip[OUT_SCENE][i].sobel_min = 0x0000;
		af->filter_clip[INDOOR_SCENE][i].spsmd_max = 0xFFFF;
		af->filter_clip[INDOOR_SCENE][i].spsmd_min = 0x14;
		af->filter_clip[INDOOR_SCENE][i].sobel_max = 0x008F;
		af->filter_clip[INDOOR_SCENE][i].sobel_min = 0x0070;
		af->filter_clip[DARK_SCENE][i].spsmd_max = 0xFFFF;
		af->filter_clip[DARK_SCENE][i].spsmd_min = 0x000C;
		af->filter_clip[DARK_SCENE][i].sobel_max = 0xFFFF;
		af->filter_clip[DARK_SCENE][i].sobel_min = 0x0004;
	}

	af->bv_threshold[OUT_SCENE][OUT_SCENE] = 151-10;
	af->bv_threshold[OUT_SCENE][INDOOR_SCENE] = 55;
	af->bv_threshold[OUT_SCENE][DARK_SCENE] = 0;

	af->bv_threshold[INDOOR_SCENE][OUT_SCENE] = 151+10;
	af->bv_threshold[INDOOR_SCENE][INDOOR_SCENE] = 55-10;
	af->bv_threshold[INDOOR_SCENE][DARK_SCENE] = 0;

	af->bv_threshold[DARK_SCENE][OUT_SCENE] = 151;
	af->bv_threshold[DARK_SCENE][INDOOR_SCENE] = 55+10;
	af->bv_threshold[DARK_SCENE][DARK_SCENE] = 0;

	af->soft_landing_dly = 10;//avoid vcm crash
	af->soft_landing_step = 20;

	if( NULL==af_pm_output->param_data || NULL==af_pm_output->param_data[0].data_ptr || af_pm_output->param_data[0].data_size!=sizeof(af->af_tuning_data) ){

		if( NULL!=af_pm_output->param_data && NULL==af_pm_output->param_data[0].data_ptr )
			AF_LOGD("sensor tuning param doesn't exist");
		else
			AF_LOGD("sensor tuning param size dismatch");

	}else{
		memcpy(&af->af_tuning_data,af_pm_output->param_data[0].data_ptr,sizeof(af->af_tuning_data));
		AF_LOGD("sensor tuning param size match");
		if( 1==af->af_tuning_data.flag ){
			af->soft_landing_dly = af->af_tuning_data.soft_landing_dly;
			af->soft_landing_step = af->af_tuning_data.soft_landing_step;
			memcpy(af->filter_clip,af->af_tuning_data.filter_clip,sizeof(af->filter_clip));
			memcpy(af->bv_threshold,af->af_tuning_data.bv_threshold,sizeof(af->bv_threshold));
			memcpy(&af->fv.AF_Tuning_Data,&af->af_tuning_data.AF_Tuning_Data[INDOOR_SCENE],sizeof(af->fv.AF_Tuning_Data));
			af->fv.AF_OTP.bIsExist = T_LENS_BY_TUNING;
			AF_LOGD("sensor tuning param take effect");
		}else{
			AF_LOGD("sensor tuning param take no effect");
		}
	}
	af->pre_scene = INDOOR_SCENE;

}

static sprd_af_handle_t sprd_afv1_init(isp_ctrl_context *isp)
{
    assert(isp);
	AF_LOGE("sprd_afv1_init E");

	struct isp_pm_ioctl_output af_pm_output;
	//memset((void*)&af_pm_output, 0, sizeof(af_pm_output));
	af_pm_output.param_data = NULL;
	af_pm_output.param_num = 0;
	isp_pm_ioctl(isp->handle_pm, ISP_PM_CMD_GET_INIT_AF_NEW, NULL, &af_pm_output);

    af_ctrl_t *af = (af_ctrl_t *)malloc(sizeof(*af));
    if (NULL == af)
    {
        AF_LOGD("malloc fail");
        return NULL;
    }

    afm_disable(isp);
    //afm_setup(isp); // default settings

    memset(af, 0, sizeof(*af));
    af->isp_ctx = isp;
    af->isp_info.width = isp->input_size_trim[isp->param_index].width;
    af->isp_info.height = isp->input_size_trim[isp->param_index].height;
    af->isp_info.win_num = afm_get_win_num(isp);

    af->fv.AF_Ops.cookie = af;
    af->fv.AF_Ops.statistics_wait_cal_done = if_statistics_wait_cal_done;
	af->fv.AF_Ops.statistics_get_data = if_statistics_get_data;
	af->fv.AF_Ops.lens_get_pos = if_lens_get_pos;
	af->fv.AF_Ops.lens_move_to = if_lens_move_to;
	af->fv.AF_Ops.lens_wait_stop = if_lens_wait_stop;
    af->fv.AF_Ops.lock_ae = if_lock_ae;
    af->fv.AF_Ops.lock_awb = if_lock_awb;
    af->fv.AF_Ops.get_sys_time = if_get_sys_time;
    af->fv.AF_Ops.sys_sleep_time = if_sys_sleep_time;
    af->fv.AF_Ops.get_ae_report = if_get_ae_report;
    af->fv.AF_Ops.set_af_exif = if_set_af_exif;
    af->fv.AF_Ops.get_otp_data = if_get_otp;
    af->fv.AF_Ops.get_motor_pos = if_get_motor_pos;
    af->fv.AF_Ops.set_motor_sacmode = if_set_motor_sacmode;
    af->fv.AF_Ops.binfile_is_exist = if_binfile_is_exist;
    af->fv.AF_Ops.lock_lsc= if_lock_lsc;
    af->fv.AF_Ops.af_log = if_af_log;
    af->fv.AF_Ops.af_start_notify= if_af_start_notify;
    af->fv.AF_Ops.af_end_notify= if_af_end_notify;


    pthread_mutex_init(&af->af_work_lock, NULL);
    pthread_mutex_init(&af->caf_work_lock, NULL);
    sem_init(&af->af_wait_caf, 0, 0);
    af->dcam_timestamp = 0xffffffffffffffff;
    set_vcm_chip_ops(af);
    load_settings(af,&af_pm_output);
    AF_init(&af->fv);
    Get_AF_tuning_Data(&af->fv);

    faf_trigger_init(af);
    if (trigger_init(af, CAF_TRIGGER_LIB) != 0)
    {
        AF_LOGE("trigger_init fail");
        goto fail;
    }

    //pthread_mutex_init(&af->caf_lock, NULL);
    af->af_task = isp->system.thread_af_proc;


    AF_LOGD("width = %d, height = %d, win_num = %d", af->isp_info.width, 
        af->isp_info.height, af->isp_info.win_num);

    isp->handle_af = af;
    // TODO: for debug only
    /*
    isp->log_af = (uint8_t *)&af_exif_info;
    isp->log_af_size = sizeof(af_exif_info);
    */
    isp->log_af = (uint8_t *)af;
    isp->log_af_size = sizeof(*af);
    lens_move_to_infi(af,af->fv.AF_OTP.INF);

	/*
	AF process need to do af once when af init done.
	*/
	af->inited_af_req = TRUE;
    assert(sizeof(af->af_version)>=strlen("AF-")+strlen(af->fv.AF_Version)+strlen(AF_SYS_VERSION));
    memcpy(af->af_version,"AF-",strlen("AF-"));
    memcpy(af->af_version+strlen("AF-"),af->fv.AF_Version,sizeof(af->fv.AF_Version));
    memcpy(af->af_version+strlen("AF-")+strlen(af->fv.AF_Version),AF_SYS_VERSION,strlen(AF_SYS_VERSION));
    property_set("af_mode","none");
/*
	{
		FILE* fp=NULL;
		af_tuning_param_t tuning_data;
		fp = fopen("/data/mlog/af_tuning_default.bin","wb");
		memset(&tuning_data,0,sizeof(tuning_data));
		memcpy(tuning_data.filter_clip,af->filter_clip,sizeof(af->filter_clip));
		memcpy(tuning_data.bv_threshold,af->bv_threshold,sizeof(af->bv_threshold));

		memcpy(&tuning_data.SAF_win,&af->af_tuning_data.SAF_win,sizeof(AF_Window_Config));
		memcpy(&tuning_data.CAF_win,&af->af_tuning_data.CAF_win,sizeof(AF_Window_Config));
		memcpy(&tuning_data.VAF_win,&af->af_tuning_data.VAF_win,sizeof(AF_Window_Config));

		memcpy(&tuning_data.AF_Tuning_Data[OUT_SCENE],&af->fv.AF_Tuning_Data,sizeof(af->fv.AF_Tuning_Data));
		memcpy(&tuning_data.AF_Tuning_Data[INDOOR_SCENE],&af->fv.AF_Tuning_Data,sizeof(af->fv.AF_Tuning_Data));
		memcpy(&tuning_data.AF_Tuning_Data[DARK_SCENE],&af->fv.AF_Tuning_Data,sizeof(af->fv.AF_Tuning_Data));

		fwrite(&tuning_data,1,sizeof(tuning_data),fp);
		fclose(fp);
		AF_LOGD("sizeof(tuning_data) = %d",sizeof(tuning_data));
	}
*/
    return af;

fail:
    free(af);
    return NULL;
}

/*
static void sprd_af_soft_landing(af_ctrl_t *af)
{
	uint16_t cur_pos = 0;

	if_get_motor_pos(&cur_pos, af);
	AF_LOGD("E, INF :%d, cur_pos :%d", af->fv.AF_OTP.INF, cur_pos);

	while (1) {
		if (!cur_pos)
			break;

		if (cur_pos > af->fv.AF_OTP.INF) {
			cur_pos = af->fv.AF_OTP.INF;
		} else if (cur_pos > af->soft_landing_step
				&& cur_pos <= af->fv.AF_OTP.INF) {
			cur_pos -= af->soft_landing_step;
		} else {
			cur_pos = 0;
		}

		if_lens_move_to(cur_pos, af);
		if_sys_sleep_time(af->soft_landing_dly, af);
	}
	AF_LOGD("X");
}
*/
/* de-initialization */
static int32_t sprd_afv1_deinit(isp_handle handle)
{
    isp_ctrl_context *isp = (isp_ctrl_context *)handle;
    AF_LOGD("sprd_afv1_deinit");

    assert(isp);
    af_ctrl_t *af = isp->handle_af;
    assert(af);

    lens_move_to(af,0);
    afm_disable(isp);
    pthread_mutex_destroy(&af->af_work_lock);
    sem_destroy(&af->af_wait_caf);
    AF_deinit(&af->fv);
    trigger_deinit(af);
    //pthread_mutex_destroy(&af->caf_lock);
    property_set("af_mode","none");
    property_set("af_set_pos","none");
    isp->handle_af = NULL;
    free(af);
    
    return 0;
}

static ERRCODE af_wait_caf_finish(af_ctrl_t *af)
{
	int32_t                 rtn;
	struct timespec         ts;

	if (clock_gettime(CLOCK_REALTIME, &ts)) {
		rtn = -1;
	} else {
		ts.tv_sec += AF_WAIT_CAF_FINISH;
/*
		if (ts.tv_nsec + ISP_PROCESS_NSEC_TIMEOUT >= (1000 * 1000 * 1000)) {
			ts.tv_nsec = ts.tv_nsec + ISP_PROCESS_NSEC_TIMEOUT - (1000 * 1000 * 1000);
			ts.tv_sec ++;
		} else {
			ts.tv_nsec += ISP_PROCESS_NSEC_TIMEOUT;
		}
*/
		rtn = sem_timedwait(&af->af_wait_caf, &ts);
		if (rtn) {
			af->takePicture_timeout = 1;
			AF_LOGD("af wait caf timeout");
		} else {
			af->takePicture_timeout = 2;
			AF_LOGD("af wait caf finished");
		}
	}
	return 0;
}

static ERRCODE af_clear_sem(af_ctrl_t *af)
{
	int32_t tmpVal = 0;

	pthread_mutex_lock(&af->af_work_lock);
	sem_getvalue(&af->af_wait_caf, &tmpVal);
	while ( 0 < tmpVal ) {
		sem_wait(&af->af_wait_caf);
		sem_getvalue(&af->af_wait_caf, &tmpVal);
	}
	pthread_mutex_unlock(&af->af_work_lock);

	return 0;
}

static uint16_t get_vcm_registor_pos(af_ctrl_t *af){

	uint16_t pos = 0;

	if( NULL!=af->vcm_ops.get_motor_pos ){
		af->vcm_ops.get_motor_pos(&pos);
		af->fv.vcm_register = pos;
		/*
		af_exif_info.fv.vcm_register = pos;
		af_exif_info.need_re_trigger = af->need_re_trigger;
		*/
		AF_LOGD("VCM registor pos :%d",af->fv.vcm_register);
	}

	return pos;
}

typedef enum _lock_block{
	LOCK_AE = 0x01,
	LOCK_LSC = 0x02,
	LOCK_NLM = 0x04
}lock_block_t;

struct sensor_ex_exposure {
	uint32_t exposure;
	uint32_t dummy;
	uint32_t size_index;
};

static void get_vcm_mode(af_ctrl_t *af){

	if( NULL!=af->vcm_ops.get_test_vcm_mode )
		af->vcm_ops.get_test_vcm_mode();

	//return 0;
}

static void set_vcm_mode(af_ctrl_t *af,char* vcm_mode){

	if( NULL!=af->vcm_ops.set_test_vcm_mode )
		af->vcm_ops.set_test_vcm_mode(vcm_mode);

	//return 0;
}

static void lock_block(af_ctrl_t *af,char* block){
	uint32_t lock=0;

	lock = atoi(block);

	if( lock&LOCK_AE )
		if_lock_ae(LOCK,af);
	if( lock&LOCK_LSC )
		if_lock_lsc(LOCK,af);
	if( lock&LOCK_NLM )
		if_lock_nlm(LOCK,af);

	//return 0;
}

static void set_ae_gain_exp(af_ctrl_t *af,char* ae_param){
	char* p1=ae_param;
	isp_ctrl_context *isp = (isp_ctrl_context *)af->isp_ctx;
	struct sensor_ex_exposure ex_exposure;

	while( *p1!='~'  && *p1!='\0' )
		p1++;
	*p1++ = '\0';

	isp->ioctrl_ptr->set_gain(atoi(ae_param));
	ae_param = p1;
	ex_exposure.exposure = atoi(ae_param);
	ex_exposure.dummy = 0;
	ex_exposure.size_index = 0;
	isp->ioctrl_ptr->ex_set_exposure((uint32_t)&ex_exposure);

	//return 0;
}

static void set_manual(af_ctrl_t* af,char* test_param){
		af->state = STATE_IDLE;
		af->caf_state = CAF_IDLE;
		//property_set("af_set_pos","0");// to fix lens to position 0
		trigger_stop(af);

		ISP_LOGD("Now is in ISP_FOCUS_MANUAL mode");
		ISP_LOGD("pls adb shell setprop \"af_set_pos\" 0~1023 to fix lens position");
}

static void trigger_caf(af_ctrl_t* af,char* test_param){
		af->request_mode = ISP_FOCUS_TRIG;//not need trigger to work when caf_start_monitor
		af->state = STATE_CAF;
		af->caf_state = CAF_SEARCHING;
		af->algo_mode = CAF;
		ISP_LOGD("_eAF_Triger_Type = %d",atoi(test_param));
		trigger_stop(af);
		AF_Trigger(&af->fv, af->algo_mode, atoi(test_param));//test_param is in _eAF_Triger_Type,     RF_NORMAL = 0,        //noraml R/F search for AFT RF_FAST = 3,              //Fast R/F search for AFT
		do_start_af(af);
}

static void trigger_saf(af_ctrl_t* af,char* test_param){
		af->request_mode = ISP_FOCUS_TRIG;
		af->state = STATE_NORMAL_AF;
		af->caf_state = CAF_IDLE;
		af->defocus = (1 == atoi(test_param))? (1):(af->defocus);
		saf_start(af, NULL);//SAF, win is NULL using default
}

static void calibration_ae_mean(af_ctrl_t* af,char* test_param){
		FILE* fp = fopen("/data/mlog/calibration_ae_mean.txt","ab");
		uint8_t i=0;

		if_lock_lsc(LOCK,af);
		if_lock_ae(LOCK,af);
		if_statistics_get_data(af->fv_combine,af);
		for( i=0;i<9;i++ ){
			ISP_LOGD("pos %d AE_MEAN_WIN_%d R %d G %d B %d r_avg_all %d g_avg_all %d b_avg_all %d FV %lld\n",get_vcm_registor_pos(af),i,af->ae_cali_data.r_avg[i],
				af->ae_cali_data.g_avg[i],af->ae_cali_data.b_avg[i],af->ae_cali_data.r_avg_all,af->ae_cali_data.g_avg_all,af->ae_cali_data.b_avg_all,af->fv_combine[T_SPSMD]);
			fprintf(fp,"pos %d AE_MEAN_WIN_%d R %d G %d B %d r_avg_all %d g_avg_all %d b_avg_all %d FV %lld\n",get_vcm_registor_pos(af),i,af->ae_cali_data.r_avg[i],
				af->ae_cali_data.g_avg[i],af->ae_cali_data.b_avg[i],af->ae_cali_data.r_avg_all,af->ae_cali_data.g_avg_all,af->ae_cali_data.b_avg_all,af->fv_combine[T_SPSMD]);
		}
		fclose(fp);

}

static void trigger_defocus(af_ctrl_t* af,char* test_param)
{
	char* p1=test_param;

	while( *p1!='~'  && *p1!='\0' )
		p1++;
	*p1++ = '\0';

	af->defocus = atoi(test_param);
	AF_LOGD("af->defocus : %d \n",af->defocus);

	return;
}

static void dump_focus_log(af_ctrl_t* af,char* test_param)
{
	char* p1=test_param;

	while( *p1!='~'  && *p1!='\0' )
		p1++;
	*p1++ = '\0';

	af->fv.dump_log = atoi(test_param);

	AF_LOGD("af->fv.dump_log : %d \n",af->fv.dump_log);
	return;
}

static void set_focus_stat_reg(af_ctrl_t* af,char* test_param)
{
	char* p1=test_param;
	char *p2;
	while( *p1!='~'  && *p1!='\0' )
		p1++;
	*p1++ = '\0';

	p2 = p1;
	while( *p2!='~'  && *p2!='\0' )
		p2++;
	*p2++ = '\0';

	
	memset(&(af->stat_reg),0,sizeof(focus_stat_reg_t));
	af->stat_reg.force_write = (1 == atoi(test_param))? (1):(0);
	af->stat_reg.reg_param[0] = atoi(p1);
	af->stat_reg.reg_param[1] = atoi(p2);	
	AF_LOGD("%s - fw:%d p0:%d p1:%d \n"
		,__FUNCTION__
		,af->stat_reg.force_write
		,af->stat_reg.reg_param[0]
		,af->stat_reg.reg_param[1]);
	
	return;
}

static test_mode_command_t test_mode_set[] = {
	{"ISP_FOCUS_MANUAL",0,&set_manual},
	{"ISP_FOCUS_CAF",0,&trigger_caf},
	{"ISP_FOCUS_SAF",0,&trigger_saf},
	{"ISP_FOCUS_CALIBRATION_AE_MEAN",0,&calibration_ae_mean},
	{"ISP_FOCUS_VCM_SET_MODE",0,&set_vcm_mode},
	{"ISP_FOCUS_VCM_GET_MODE",0,&get_vcm_mode},
	{"ISP_FOCUS_LOCK_BLOCK",0,&lock_block},
	{"ISP_FOCUS_SET_AE_GAIN_EXPOSURE",0,&set_ae_gain_exp},
	{"ISP_FOCUS_DEFOCUS",0,&trigger_defocus},
	{"ISP_FOCUS_DUMP_LOG",0,&dump_focus_log},	
	{"ISP_FOCUS_STAT_REG",0,&set_focus_stat_reg},	
	{"ISP_DEFAULT",0,NULL},
};

static void set_af_test_mode(af_ctrl_t *af,char* af_mode)
{
#define CALCULATE_KEY(string,string_const) key=1; \
		while( *string!='~' && *string!='\0' ){ \
			key=key+*string; \
			string++; \
		} \
		if( 0==string_const ) \
			*string = '\0';

	char* p1=af_mode;
	uint64 key = 0,i=0;

	CALCULATE_KEY(p1,0);

	while(i<sizeof(test_mode_set)/sizeof(test_mode_set[0])){
		if( key==test_mode_set[i].key )
			break;
		i++;
	}

	if(sizeof(test_mode_set)/sizeof(test_mode_set[0])<=i){// out of range in test mode,so initialize its ops
		AF_LOGD("AF test mode Command is undefined,start af test mode initialization");
		i=0;
		while(i<sizeof(test_mode_set)/sizeof(test_mode_set[0])){
			p1 = test_mode_set[i].command;
			CALCULATE_KEY(p1,1);
			test_mode_set[i].key = key;
			i++;
		}
		set_manual(af,NULL);
		return;
	}

	if( NULL!=test_mode_set[i].command_func )
		test_mode_set[i].command_func(af,p1+1);
}

static int32_t sprd_afv1_set_mode(isp_handle handle, void *param, int (*callback)())
{
    assert(handle);
    assert(param);

    isp_ctrl_context *isp = (isp_ctrl_context *)handle;
    af_ctrl_t *af = isp->handle_af;
    assert(af);
    uint32_t mode = *(uint32_t *)param;

    property_get("af_mode",AF_MODE,"none");
    if( 0==strcmp(AF_MODE,"none") ){
       AF_LOGD("state = %s, mode = %d", STATE_STRING(af->state), mode);
       /*if (STATE_INACTIVE == af->state)
          return 0;*/

       switch(mode)
      {
          case ISP_FOCUS_CONTINUE:
          case ISP_FOCUS_VIDEO:
//            if (STATE_IDLE == af->state)
            if( STATE_FAF==af->state )
                 return 0;

            if (CAF_SEARCHING != af->caf_state)
            {
                af->request_mode = mode;
                af->state = ISP_FOCUS_CONTINUE == mode ? STATE_CAF : STATE_RECORD_CAF;
                caf_start(af);
            }
            break;

		case ISP_FOCUS_TAKE_PICTURE:
			if (af->need_re_trigger != 1)
				caf_stop_monitor(af);
			AF_LOGD("AF_mode = %d, SAF_Search_Process = %d, need_re_trigger :%d", af->fv.AF_mode, af->fv.sAF_Data.sAFInfo.SAF_Search_Process, af->need_re_trigger);

			af->takePicture_timeout = 0;
			if(Wait_Trigger != af->fv.AF_mode
				|| (SAF_Search_DONE!=af->fv.sAF_Data.sAFInfo.SAF_Search_Process
				&& SAF_Search_INIT!=af->fv.sAF_Data.sAFInfo.SAF_Search_Process)
				|| af->need_re_trigger || DCAM_AFTER_VCM_NO==compare_timestamp(af) ){
				af_clear_sem(af);
				af_wait_caf_finish(af);
				af->state = STATE_CAF ;
				caf_start(af);
			};
			AF_LOGD("dcam_timestamp-vcm_timestamp = %lld ms",((int64_t)af->dcam_timestamp-(int64_t)af->vcm_timestamp)/1000000);
			get_vcm_registor_pos(af);
            break;

           default:
           if ((STATE_CAF == af->state) || (STATE_RECORD_CAF == af->state))
           {
//                af->state = STATE_IDLE;
                //caf_stop(af);
                if (!af->need_re_trigger)
                {
			caf_stop_monitor(af);
		   }
           }
            af->request_mode = mode;
            break;
       }
    }else{
        //set_af_test_mode(af,AF_MODE);// only one thread could call it
        get_vcm_registor_pos(af);// get final vcm pos when in test mode
    }
    return 0;
}

static int32_t sprd_afv1_get_mode(isp_handle handle, void *param, int (*callback)())
{
    isp_ctrl_context *isp = (isp_ctrl_context *)handle;
    assert(isp);
    af_ctrl_t *af = isp->handle_af;
    assert(af);

    assert(param);
    *(uint32_t *)param = af->request_mode;
    return 0;
}

static int32_t sprd_afv1_start(isp_handle handle, void *param, int (*callback)())
{
    af_ctrl_t *af;
    struct isp_af_win *win;
    
    assert(handle);
    assert(param);
    property_set("af_mode","none");
    af = ((isp_ctrl_context *)handle)->handle_af;
    assert(af);
    win = (struct isp_af_win *)param;

    AF_LOGD("state = %s, win = %d", STATE_STRING(af->state), win->valid_win);
    if (STATE_INACTIVE == af->state)
        return 0;

    if (STATE_IDLE != af->state)
    {
        if (((STATE_CAF == af->state) || (STATE_RECORD_CAF == af->state)) 
            /*&& (0 != win->valid_win)*/)
        {
            suspend_caf(af);
        }
        else
        {
            notify_stop(af, 0);
            return 0;
        }
    }
    af->caf_first_stable=0;
    af->state = STATE_NORMAL_AF;
    saf_start(af, win);
    return 0;
}

static int32_t sprd_afv1_stop(isp_handle handle, void *param, int (*callback)())
{
    isp_ctrl_context *isp = (isp_ctrl_context *)handle;
    assert(isp);
    af_ctrl_t *af = (af_ctrl_t *)isp->handle_af;
    assert(af);

    AF_LOGD("state = %s", STATE_STRING(af->state));
    if (STATE_NORMAL_AF == af->state)
    {
/*
        af->state = STATE_IDLE;
        saf_stop(af);

        if ((STATE_CAF == af->pre_state) || (STATE_RECORD_CAF == af->pre_state))
        {
            resume_caf(af);
        }
*/
    }
    else
    {
        // TODO
    }
    return 0;
}

/* called each frame */
static int32_t af_test_lens(af_ctrl_t *af){
         pthread_mutex_lock(&af->af_work_lock);
         AF_STOP(&af->fv, af->algo_mode);
         AF_Process_Frame(&af->fv);
         AF_LOGD("AF_mode = %d", af->fv.AF_mode);
         pthread_mutex_unlock(&af->af_work_lock);

         AF_LOGD("af_pos_set3 %s",AF_POS);
         lens_move_to(af,atoi(AF_POS));
         AF_LOGD("af_pos_set4 %s",AF_POS);
         return 0;
}

#if AF_RING_BUFFER
static int32_t af_dequeue_in_ringbuffer(af_ctrl_t *af){

	isp_ctrl_context *isp = af->isp_ctx;
	int32_t rtn;

	if (ISP_CHIP_ID_TSHARK3 == isp_dev_get_chip_id(isp->handle_device)){
		af->node_type = ISP_NODE_TYPE_RAWAFM;
		rtn = isp_u_bq_dequeue_buf(isp->handle_device, &af->k_addr, &af->u_addr, af->node_type);
		AF_LOGD("tshark3 chip dequeue Finished. rtn=%d, k_addr = 0x%x, u_addr = 0x%x", rtn, af->k_addr,af->u_addr);
		if (rtn || (0==af->u_addr) || (0==af->k_addr)) {
			ISP_LOGE("k_addr or u_addr is 0, %d", rtn);
			return AF_RING_BUFFER_NO;
		}

		memcpy(af->af_statistics, af->u_addr, 105*4);

		return AF_RING_BUFFER_YES;
	}else{
		af->node_type = ISP_NODE_TYPE_RAWAFM;
		rtn = isp_u_bq_dequeue_buf(isp->handle_device, &af->k_addr, &af->u_addr, af->node_type);
		AF_LOGD("dequeue Finished. rtn=%d, k_addr = 0x%x, u_addr = 0x%x", rtn, af->k_addr, af->u_addr);
		if (rtn || (0==af->u_addr) || (0==af->k_addr)) {
			ISP_LOGE("k_addr or u_addr is 0, %d", rtn);
			return AF_RING_BUFFER_NO;
		}

		memcpy(af->af_statistics, af->u_addr, 25*4);// will be 25*4*2,waiting for kernel compelement

		return AF_RING_BUFFER_YES;
	}
}


static int32_t af_enqueue_in_ringbuffer(af_ctrl_t *af){

	isp_ctrl_context *isp = af->isp_ctx;
	int32_t rtn;

	if (ISP_CHIP_ID_TSHARK3 == isp_dev_get_chip_id(isp->handle_device)){
		af->node_type = ISP_NODE_TYPE_RAWAFM;

		isp->isp_smart_eb = 1;
		isp_u_bq_enqueue_buf(isp->handle_device, af->k_addr, af->u_addr, af->node_type);
		AF_LOGD("isp_u_af_transaddr: Enqueue! k_addr = 0x%x, u_addr = 0x%x", rtn, af->k_addr, af->u_addr);
	}else{
		af->node_type = ISP_NODE_TYPE_RAWAFM;

		isp->isp_smart_eb = 1;
		isp_u_bq_enqueue_buf(isp->handle_device, af->k_addr, af->u_addr, af->node_type);
		AF_LOGD("isp_u_af_transaddr: Enqueue! k_addr = 0x%x, u_addr = 0x%x", rtn, af->k_addr, af->u_addr);
	}

	return 0;
}
#endif

static int32_t sprd_afv1_calc(isp_ctrl_context *isp)
{
    af_ctrl_t *af = isp->handle_af;
    nsecs_t system_time0 = 0;
    nsecs_t system_time1 = 0;

    if( 1==af->bypass )
             return 0;

    property_get("af_mode",AF_MODE,"none");
    if( 0!=strcmp(AF_MODE,"none") ){
         set_af_test_mode(af,AF_MODE);
         property_set("af_mode","ISP_DEFAULT");
         property_get("af_set_pos",AF_POS,"none");
         if( 0!=strcmp(AF_POS,"none") ){
             af_test_lens(af);
             property_set("af_set_pos","none");
             return 0;
         }
    }

    // AF_LOGD("state = %s, pre_state = %s", STATE_STRING(af->state), STATE_STRING(af->pre_state));
    if( 1!=Is_ae_stable(af) ){
		AF_LOGD("ae not stable in non caf mode");
            return 0;
    }

    //AF ring buffer
#if (AF_RING_BUFFER)
    if( AF_RING_BUFFER_YES!=af_dequeue_in_ringbuffer(af) ){
            return 0;
    }
#endif

    system_time0 = systemTime(CLOCK_MONOTONIC)/1000000LL;

    switch (af->state)
    {
    case STATE_NORMAL_AF:
	  pthread_mutex_lock(&af->af_work_lock);
        if (saf_process_frame(af))
        {
            af->state = STATE_IDLE;
          //  saf_stop(af);
           //do_stop_af(af);

            /*if ((STATE_CAF == af->pre_state) || (STATE_RECORD_CAF == af->pre_state))
            {
                resume_caf(af);
            }*/
        }
	 pthread_mutex_unlock(&af->af_work_lock);
        break;
    case STATE_CAF:
    case STATE_RECORD_CAF:
        caf_process_af(af);
        break;
    case STATE_FAF:
	pthread_mutex_lock(&af->af_work_lock);
	if( faf_process_frame(af) ){
		af->state = STATE_CAF ;
		caf_start(af);
	}
	pthread_mutex_unlock(&af->af_work_lock);
        break;
    default:
	 pthread_mutex_lock(&af->af_work_lock);
        AF_Process_Frame(&af->fv);
        AF_LOGD("AF_mode = %d", af->fv.AF_mode);
	 pthread_mutex_unlock(&af->af_work_lock);
        break;
    }

    system_time1 = systemTime(CLOCK_MONOTONIC)/1000000LL;
    AF_LOGD("SYSTEM_TEST-af:%lldms", system_time1-system_time0);

#if (AF_RING_BUFFER)
    af_enqueue_in_ringbuffer(af);
#endif

    return 0;
}

static uint32_t calc_gain_index(uint32_t cur_gain){
	uint32_t gain = cur_gain>>7,i=0;// 128 is 1 ae gain unit

	if( gain>(1<<GAIN_32x) )
		gain = (1<<GAIN_32x);

	i = 0;
	while( gain=gain>>1 ){
		i++;
	}
	return i;
}

static void set_ae_info(af_ctrl_t *af, const struct ae_calc_out *ae, int32_t bv)
{
    ae_info_t *p = &af->ae;

    // AF_LOGD("state = %s, bv = %d", STATE_STRING(af->state), bv);

    p->stable = ae->is_stab;
    p->bv     = bv;
    p->exp    = ae->cur_exp_line * ae->line_time;
    p->gain   = ae->cur_again;
    p->gain_index = calc_gain_index(ae->cur_again);

    if (bv >= af->bv_threshold[af->pre_scene][OUT_SCENE])
    {
        af->curr_scene = OUT_SCENE;
    }
    else if (bv >= af->bv_threshold[af->pre_scene][INDOOR_SCENE])
    {
        af->curr_scene = INDOOR_SCENE;
    }
    else
    {
        af->curr_scene = DARK_SCENE;
    }

    af->pre_scene = af->curr_scene;

    if( 1==af->af_tuning_data.flag ){// dynamicly adjust gain in different scene
        memcpy(&af->fv.AF_Tuning_Data,&af->af_tuning_data.AF_Tuning_Data[ af->curr_scene],sizeof(af->fv.AF_Tuning_Data));
    }

    if ((STATE_CAF == af->state) || (STATE_RECORD_CAF == af->state))
    {
        CMR_MSG_INIT(msg);

        p->ae = *ae;

        msg.msg_type = ISP_PROC_AF_IMG_DATA_UPDATE;
        cmr_thread_msg_send(af->af_task, &msg);
/*
        struct isp_pm_ioctl_input in;
        struct isp_pm_ioctl_output out;
        struct isp_pm_param_data param;

        memset(&in, 0, sizeof(in));
        memset(&out, 0, sizeof(out));

        param.cmd = ISP_PM_BLK_AEM_STATISTIC;
        param.id = ISP_BLK_AE_V1;
        param.data_ptr = NULL;
        param.data_size = 0;
        in.param_data_ptr = &param;
        in.param_num = 1;

        isp_pm_ioctl(af->isp_ctx->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, &in, &out);
        if (out.param_data)
        {
            struct isp_awb_statistic_info *stat = out.param_data->data_ptr;
            caf_process_ae(af, ae, stat);
	    }
*/
    }
}

isp_awb_statistic_hist_info_t af_tmp;
static int32_t image_data_update(isp_ctrl_context *isp)
{
    af_ctrl_t *af = isp->handle_af;

    struct isp_pm_ioctl_input in;
    struct isp_pm_ioctl_output out;
    struct isp_pm_param_data param;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    param.cmd = ISP_PM_BLK_AEM_STATISTIC;
    param.id = ISP_BLK_AE_V1;
    param.data_ptr = NULL;
    param.data_size = 0;
    in.param_data_ptr = &param;
    in.param_num = 1;

    isp_pm_ioctl(af->isp_ctx->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, &in, &out);
    if (out.param_data)
    {
		struct isp_awb_statistic_info *ae_stat = out.param_data->data_ptr;
		memcpy(af_tmp.r_info, ae_stat->r_info, sizeof(af_tmp.r_info));
		memcpy(af_tmp.g_info, ae_stat->g_info, sizeof(af_tmp.g_info));
		memcpy(af_tmp.b_info, ae_stat->b_info, sizeof(af_tmp.b_info));

		isp_awb_statistic_hist_info_t *stat = &af_tmp;
//	 	struct isp_awb_statistic_info *stat = &isp->aem_statistic;
        caf_process_ae(af, &af->ae.ae, stat);
    }

    return 0;
}

static void set_awb_info(af_ctrl_t *af, const struct awb_ctrl_calc_result *awb, const struct awb_gain *gain)
{
    af->awb.r_gain = gain->r;
    af->awb.g_gain = gain->g;
    af->awb.b_gain = gain->b;
}

static int32_t sprd_afv1_set_ae_awb_info(isp_ctrl_context *isp, void *ae, void *awb,
    void *bv, void *rgb)
{
    struct awb_gain gain;

    isp->ae_lib_fun->ae_io_ctrl(isp->handle_ae, AE_GET_BV_BY_LUM_NEW, NULL, bv);
    isp->awb_lib_fun->awb_ctrl_ioctrl(isp->handle_awb, AWB_CTRL_CMD_GET_GAIN, &gain, NULL);

    set_af_RGBY(isp->handle_af,rgb);
    set_ae_info(isp->handle_af, ae, *(int32_t *)bv);
    set_awb_info(isp->handle_af, awb, &gain);
    return 0;
}

static void ae_calc_win_size(af_ctrl_t *af, struct isp_video_start *param)
{
    if (param->size.w && param->size.h)
    {
        uint32_t w, h;
        w = ((param->size.w / 32) >> 1) << 1;
        h = ((param->size.h / 32) >> 1) << 1;
        af->ae.win_size = w * h;
    }
    else
    {
        af->ae.win_size = 1;
    }
}

static int32_t sprd_afv1_isp_start(isp_handle handle, struct isp_video_start *param)
{
	af_ctrl_t *af = ((isp_ctrl_context *)handle)->handle_af;

	AF_LOGD("state = %s", STATE_STRING(af->state));
	//af->state = af->pre_state = STATE_IDLE;
	property_get("af_mode",AF_MODE,"none");
	if( 0==strcmp(AF_MODE,"none") ){
		af->state = af->pre_state = STATE_CAF;
		caf_start(af);
		ae_calc_win_size(af, param);
	}

    return 0;
}

static int32_t sprd_afv1_isp_stop(isp_handle handle)
{
    af_ctrl_t *af = ((isp_ctrl_context *)handle)->handle_af;
    AF_LOGD("state = %s", STATE_STRING(af->state));

    if (STATE_IDLE != af->state)
        do_stop_af(af);


    //af->state = af->pre_state = STATE_INACTIVE;

    return 0;
}

static int32_t set_flash_status(isp_handle handle, void *param, int (*callback)())
{
    af_ctrl_t *af = ((isp_ctrl_context *)handle)->handle_af;
    struct isp_flash_notice *flash = (struct isp_flash_notice *)param;

    AF_LOGD("state = %s, pre_state = %s, flash_on = %d, flash mode = %d",
        STATE_STRING(af->state), STATE_STRING(af->pre_state), af->flash_on, flash->mode);

    switch (flash->mode)
    {
        case ISP_FLASH_PRE_BEFORE:
		case ISP_FLASH_PRE_LIGHTING:
		case ISP_FLASH_MAIN_BEFORE:
		case ISP_FLASH_MAIN_LIGHTING:
            if (!af->flash_on)
            {
                af->flash_on = 1;
                // TODO: suspend caf
            }
            break;

        case ISP_FLASH_PRE_AFTER:
		case ISP_FLASH_MAIN_AFTER:
            if (af->flash_on)
            {
                af->flash_on = 0;
                // TODO: resume caf
            }
            break;

        default:
            break;
    }
    return 0;
}

static int32_t set_face_detect(isp_handle handle, void *param, int (*callback)())
{
	af_ctrl_t *af = ((isp_ctrl_context *)handle)->handle_af;
	struct isp_face_area *face = (struct isp_face_area *)param;

	AF_LOGD("state = %s", STATE_STRING(af->state));
	if (STATE_INACTIVE == af->state)
		return 0;

	AF_LOGD("type = %d, face_num = %d", face->type, face->face_num);

	if( STATE_NORMAL_AF == af->state ){
		return 0;
	}else if (STATE_IDLE != af->state){
		memcpy(&af->face_info,face,sizeof(struct isp_face_area));
		if( face_dectect_trigger(af) ){
			if( STATE_CAF == af->state || STATE_RECORD_CAF == af->state ){
				suspend_caf(af);
			}else if( STATE_FAF == af->state ){
				pthread_mutex_lock(&af->af_work_lock);
				AF_STOP(&af->fv, af->algo_mode);
				AF_Process_Frame(&af->fv);
				AF_LOGD("AF_mode = %d", af->fv.AF_mode);
				pthread_mutex_unlock(&af->af_work_lock);
			}
			af->state = STATE_FAF;
			faf_start(af, NULL);
			AF_LOGD("FAF Trigger");
		}
	}

	return 0;
}

static int32_t sprd_afv1_ioctrl(sprd_af_handle_t handle, int cmd, void *param0, 
    void *param1)
{
    AF_LOGD("sprd_afv1_ioctrl");
    return 0;
}

static sprd_af_handle_t sprd_afv1_lib_version(isp_ctrl_context* handle,char* version,int len){

	af_ctrl_t *af = ((isp_ctrl_context *)handle)->handle_af;

	memset(version,'\0',len);
	memcpy(version,"AF-",3);
	if( len-3>=sizeof(af->fv.AF_Version) ){
		uint8 i=0;
		memcpy(version+3,af->fv.AF_Version,sizeof(af->fv.AF_Version));
		i = strlen(af->fv.AF_Version)+3;
		memcpy(version+i,"-20160927-15",12);
	}

	return 0;
}

static int32_t sprd_afv1_set_dcam_timestamp(isp_handle handle, void* param, int capture){
	af_ctrl_t *af;
	af = ((isp_ctrl_context *)handle)->handle_af;

	if( 0==capture ){
		af->dcam_timestamp = *(int64_t*)param;
		//af_exif_info.dcam_timestamp = af->dcam_timestamp;
		if( DCAM_AFTER_VCM_YES==compare_timestamp(af) && 1==af->vcm_stable){
			sem_post(&af->af_wait_caf);
		}
	}else if(1==capture){
		af->takepic_timestamp = *(int64_t*)param;
		//af_exif_info.takepic_timestamp = af->takepic_timestamp;
		AF_LOGD("takepic_timestamp - vcm_timestamp =%lld ms",((int64_t)af->takepic_timestamp-(int64_t)af->vcm_timestamp)/1000000);
	}

	return 0;
}

static int32_t sprd_afv1_ioctrl_set_af_bypass(isp_handle isp_handler,void* param_ptr, int(*call_back)()){
	af_ctrl_t *af;
	struct isp_pm_ioctl_output af_pm_output;
	af_tuning_param_t* af_tuning_data = NULL;

	af = ((isp_ctrl_context *)isp_handler)->handle_af;

	memset((void*)&af_pm_output, 0, sizeof(af_pm_output));
	isp_pm_ioctl(((isp_ctrl_context *)isp_handler)->handle_pm, ISP_PM_CMD_GET_INIT_AF_NEW, NULL, &af_pm_output);

	if( PNULL!=af_pm_output.param_data && PNULL!=af_pm_output.param_data->data_ptr ){
		af->bypass = af_pm_output.param_data->user_data[0];//af bypass flag

		af_tuning_data = (af_tuning_param_t*)af_pm_output.param_data->data_ptr;

		af->soft_landing_dly = af_tuning_data->soft_landing_dly;
		af->soft_landing_step = af_tuning_data->soft_landing_step;
		memcpy(af->filter_clip,af_tuning_data->filter_clip,sizeof(af->filter_clip));
		memcpy(af->bv_threshold,af_tuning_data->bv_threshold,sizeof(af->bv_threshold));
		memcpy(&af->fv.AF_Tuning_Data,&af_tuning_data->AF_Tuning_Data[INDOOR_SCENE],sizeof(af->fv.AF_Tuning_Data));
	}

	return 0;
}


int32_t sprd_afv1_ioctrl_set_af_pos(isp_handle isp_handler,void* param_ptr, int(*call_back)())
{
	af_ctrl_t *af = ((isp_ctrl_context *)isp_handler)->handle_af;
	uint32_t pos = *(uint32_t*)param_ptr;

	if( NULL!=af->vcm_ops.set_pos )
		af->vcm_ops.set_pos((uint16_t)pos);

	return 0;
}

int32_t sprd_afv1_module_init(struct af_lib_fun *ops)
{
    ops->af_init_interface            = sprd_afv1_init;
    ops->af_deinit_interface          = sprd_afv1_deinit;
    ops->af_ioctrl_set_af_mode        = sprd_afv1_set_mode;
    ops->af_ioctrl_get_af_mode        = sprd_afv1_get_mode;
    ops->af_ioctrl_af_start	          = sprd_afv1_start;
    ops->af_ioctrl_set_af_stop        = sprd_afv1_stop;
    ops->af_calc_interface            = sprd_afv1_calc;
    ops->af_ioctrl_set_ae_awb_info    = sprd_afv1_set_ae_awb_info;
    ops->af_image_data_update         = image_data_update;
    ops->af_ioctrl_set_isp_start_info = sprd_afv1_isp_start;
    ops->af_ioctrl_set_isp_stop_info  = sprd_afv1_isp_stop;
    ops->af_ioctrl_set_flash_notice   = set_flash_status;
    ops->af_ioctrl_set_fd_update      = set_face_detect;
    ops->af_ioctrl_interface          = sprd_afv1_ioctrl;
    ops->af_get_lib_version         = sprd_afv1_lib_version;
    ops->af_ioctrl_set_dcam_timestamp     = sprd_afv1_set_dcam_timestamp;
    ops->af_ioctrl_set_af_bypass	= sprd_afv1_ioctrl_set_af_bypass;
    ops->af_ioctrl_set_af_pos	= sprd_afv1_ioctrl_set_af_pos;
    return 0;
}


