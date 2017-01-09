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
 
 /* Version = 1.0 */
#ifndef _ISP_DEBUG_INFO_H_
#define _ISP_DEBUG_INFO_H_ 

#define AE_DEBUG_VERSION_ID (1)
#define AF_DEBUG_VERSION_ID (1)
#define AWB_DEBUG_VERSION_ID (1)

 typedef enum {
	EM_AE_MODE,
	EM_AWB_MODE,
	EM_AF_MODE,
 }EM_MODE_INFO;

struct debug_ae_param{
	uint8_t  magic[16];
	uint32_t version;						 /*version No. for this structure*/
	char 	 alg_id[32];
	uint32_t lock_status;		 /*0: unlock; 1: lock*/
	uint32_t FD_AE_status; 	 /*0: FD-AE is on; 1: FD-AE is off*/
	uint32_t cur_lum;				/*the average of image:0 ~255*/
	uint32_t target_lum;		   /*the ae target lum: 0 ~255*/
	uint32_t target_zone;			/*ae target lum stable zone: 0~255*/
	uint32_t target_lum_ori;
	uint32_t target_zone_ori;
	uint32_t max_index;			/*the max index in current ae table: 1~1024*/
	uint32_t cur_index;			/*the current index of ae table in ae now: 1~1024*/
	uint32_t min_index;			/*the mini index in current ae table: 1~1024*/
	uint32_t max_fps;				/*the max fps: 1~120*/
	uint32_t min_fps;				/*the mini fps:1~120*/
	uint32_t exposure_time;		  /*exposure time for current frame*/
	uint32_t cur_fps;				/*current fps:1~120*/
	uint32_t cur_ev;			   /*ev index: 0~15*/
	uint32_t cur_metering_mode;    /*current metering mode*/
	uint32_t cur_exp_line; 	   /*current exposure line: the value is related to the resolution*/
	uint32_t cur_dummy;			/*dummy line: the value is related to the resolution & fps*/
	uint32_t cur_again;			/*current analog gain*/
	uint32_t cur_dgain;			/*current digital gain*/
	uint32_t cur_iso;				/*current iso mode*/
	uint32_t cur_flicker;			/*flicker mode:0: 50HZ;1:60HZ*/
	uint32_t cur_scene;			/*current scene mode*/
	uint32_t cur_bv;			   /*bv parameter*/
	uint32_t flash_effect; 	   /*flash effect ratio*/
	uint32_t is_stab;				/*ae stable status*/
	uint32_t line_time;		/*exposure time for one line*/
	uint32_t frame_line;		 /*frame line length*/
	uint32_t cur_work_mode;		/*current work mode*/
	uint32_t histogram[256];	 /*luma histogram of current frame*/
	uint32_t r_stat_info[1024]; /*r channel of ae statics state data*/
	uint32_t g_stat_info[1024]; /*g channel of ae statics state data*/
	uint32_t b_stat_info[1024]; /*b channel of ae statics state data*/
	uint8_t TC_AE_status;     /*touch ae enable*/
	int8_t TC_target_offset; /*touch target offset*/
	uint16_t TC_cur_lum;       /*touch  lum*/

	int8_t mulaes_target_offset;
	int8_t region_target_offset;
	int8_t flat_target_offset;
	int8_t reserved0;
	int32_t reserved[1014];        /*resurve for future*/
};

 
struct debug_awb_param{
	uint8_t magic[16];
	uint32_t version;

	uint32_t r_gain;
	uint32_t g_gain;
	uint32_t b_gain;
	uint32_t cur_ct;

	uint32_t cur_awb_mode;
	uint32_t cur_work_mode;

	/* awb calibration of golden sensor */
	uint32_t golden_r; 
	uint32_t golden_g;
	uint32_t golden_b;

	/* awb calibration of random sensor */
	uint32_t random_r;
	uint32_t random_g;
	uint32_t random_b;

	int32_t cur_bv;			   /*current bv*/
	int32_t cur_iso;			  /*current iso value*/

	uint32_t r_stat_info[1024]; /*r channel of awb statics state data*/
	uint32_t g_stat_info[1024]; /*g channel of awb statics state data*/
	uint32_t b_stat_info[1024]; /*b channel of awb statics state data*/

	uint32_t reserved[256]; 
};

 
struct debug_af_param{
	// System
	uint8_t magic[16];
	uint32_t version;		  /*version No. for this structure*/
	uint32_t af_start_time[50]; 	   /*[50]:AF execute frame; record start time of AF start to execute in each frame*/
	uint32_t af_end_time[50];	  /*[50]:AF execute frame; record end time of AF end to execute in each frame*/
	uint32_t work_time; 		   /*work time = AF finish time - AF start time*/

	// From AE
	uint16_t	bAEisConverge;				  /*flag: check AE is converged or not*/
	uint16_t	AE_BV;						  /*brightness value*/
	uint16_t	AE_EXP; 					/*exposure time (ms)*/
	uint16_t	AE_Gain;					/*128: gain1x = 128*/

	// AF INFO
	uint8_t af_mode;			/*0: manual; 1: auto; 2: macro; 3: INF; 4: hyper*/
	uint8_t af_search_type; 		   /*0: auto; 1: type1; 2: type2; 3: type3; 4: type4; 5: type5*/
	uint8_t af_filter_type[6];			  /*[6]: six filters; af_filter_type[i]:0: auto; 1: type1; 2: type2; 3: type3; 4: type4; 5: type5*/
	uint8_t face_zone[4];		  /*face_zone[0]:X_POS; face_zone[1]:Y_POS; face_zone[2]:Width;face_zone[3]:Height; */
	uint32_t win_num;		  /*number of focus windows: 1~25*/
	uint32_t work_mode; 		   /*0: SAF; 1: CAF; 2: Face-AF; 3: multi-AF; 4: touch AF*/
	uint16_t win[25][4];			/*win[0][0]:zone0_X_POS; win[0][1]:zone0_Y_POS; win[0][2]:zone0_Width; win[0][3]:zone0_Height; */
	uint32_t start_pos; 				   /*focus position before AF work*/

	//Search data
	uint16_t scan_pos[2][50];	  /*[2]:rough/fine search; [50]:AF execute frame; record each scan position during foucsing*/
	uint32_t af_value[2][6][25][50];	/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; record focus value of each frame during foucsing*/
	uint32_t af_pixel_mean[2][4][25][50];  /*[2]:rough/fine search; [4]:color order: 0:R, 1:Gr, 2:Gb, 3:B; [25]:focus zone; [50]:AF execute frame; record pixel mean of each AF zone during foucsing*/
	uint16_t his_cov[2][25][50];		/*[2]:rough/fine search; [25]:focus zone; [50]:AF execute frame; record each covariance of histogram of each AF zone during foucsing*/
	uint8_t MaxIdx[2][6][25][50];		  /*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; index of max statistic value*/
	uint8_t MinIdx[2][6][25][50];		  /*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; index of min statistic value*/
	uint8_t BPMinIdx[2][6][25][50]; 		/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; index of min statistic value before peak*/
	uint8_t APMinIdx[2][6][25][50]; 		/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; index of min statistic value after peak*/
	uint8_t ICBP[2][6][25][50]; 			/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; increase count before peak*/
	uint8_t DCBP[2][6][25][50]; 			/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; decrease count before peak*/
	uint8_t EqBP[2][6][25][50]; 			/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; equal count before peak*/  
	uint8_t ICAP[2][6][25][50]; 			/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; increase count after peak*/
	uint8_t DCAP[2][6][25][50]; 			/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; decrease count after peak*/
	uint8_t EqAP[2][6][25][50]; 			   /*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; equal count after peak*/	   
	int8_t	BPC[2][6][25][50];				  /*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; ICBP - DCBP*/  
	int8_t	APC[2][6][25][50];				  /*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; ICAP - DCAP*/
	uint16_t	BP_R10000[2][4][6][25][50];    /*[2]:rough/fine search; [4]:four type; [6]:six filters; [25]:focus zone; [50]:AF execute frame; FV ratio X 10000 before peak*/	 
	uint16_t	AP_R10000[2][4][6][25][50];    /*[2]:rough/fine search; [4]:four type; [6]:six filters; [25]:focus zone; [50]:AF execute frame; FV ratio X 10000 after peak*/
	int32_t fv_slope_10000[2][4][6][25][50]; /*[2]:rough/fine search; [4]:four type; [6]:six filters; [25]:focus zone; [50]:AF execute frame; record slope of focus value during this foucs*/
	uint32_t	pdaf_confidence[64];		   /*[64]: PD zone; confidence value of phase detection*/
	int16_t pdaf_diff[64];					  /*[64]: PD zone; different value of phase detection*/

	//AF threshold
	uint16_t	UB_Ratio_TH[2][4][6];		  /*[2]:rough/fine search; [4]:four type; [6]:six filters; The Up bound threshold of FV ratio, type[0]:total, type[1]:3 sample, type[2]:5 sample, type[3]:7 sample*/
	uint16_t	LB_Ratio_TH[2][4][6];		  /*[2]:rough/fine search; [4]:four type; [6]:six filters; The low bound threshold of FV ratio, type[0]:total, type[1]:3 sample, type[2]:5 sample, type[3]:7 sample*/

	//AF result
	uint32_t suc_win_num;			 /*number of windows of sucess focus: 0~25*/
	uint8_t result_win[25]; 			   /*[25]: focus zone; result_win[i]: AF result of each zone*/	
	uint8_t reserved0[3];  
	uint32_t cur_pos;		  /*current focus motor position: 0~1023*/
	uint16_t peak_pos[2][6][25];		/*[2]:rough/fine search; [6]:six filters; [25]:focus zone;	peak position: 0~1023*/
	uint16_t choose_filter; 			   /*choose which filter is used to final focus peak*/
	uint16_t choose_zone;				  /*choose which zone is used to final focus peak*/
	uint8_t total_work_frame;			  /*total work frames during AF work*/
	uint8_t rs_total_frame; 				/*total work frames during rough search*/
	uint8_t fs_total_frame; 				/*total work frames during fine search*/
	uint8_t rs_dir; 						/*direction of rough search: 0:Far to near, 1:near to far*/
	uint8_t fs_dir; 						/*direction of fine search: 0:Far to near, 1:near to far*/
	uint8_t reserved1[3];  
	//Reserve
	uint32_t reserved2[256];		   /*resurve for future*/
};

 struct smart_component {
     uint32_t type;  
     uint32_t offset;
     int32_t fix_data[4];
 };
 
 struct smart_block {
     uint32_t block_id;
     uint32_t smart_id;
     uint32_t component_num;
     struct smart_component component[4];
 };
 
 struct debug_smart_param {
 	uint32_t version;
	 struct smart_block block[30];
	 uint32_t enable_num;
	 uint32_t reserved[256];
 };
 
 typedef struct debug_isp_info{
	 struct debug_ae_param ae;
	 struct debug_awb_param awb;
	 struct debug_af_param af;
	 struct debug_smart_param smart;
	 //struct debug_lsc_param lsc;
 }DEBUG_ISP_INFO_T;


//	added by wnaghao @2015-12-23
//	for real-time set or display
 struct debug_ae_display_param{
	 uint8_t magic[16];
	 uint32_t version;						 /*version No. for this structure*/
	 uint32_t lock_status;		 /*0: unlock; 1: lock*/
	 uint32_t FD_AE_status; 	 /*0: FD-AE is on; 1: FD-AE is off*/
	 uint32_t cur_lum;				/*the average of image:0 ~255*/
	 uint32_t target_lum;		   /*the ae target lum: 0 ~255*/
	 uint32_t target_zone;			/*ae target lum stable zone: 0~255*/
	 uint32_t max_index;			/*the max index in current ae table: 1~1024*/
	 uint32_t cur_index;			/*the current index of ae table in ae now: 1~1024*/
	 uint32_t min_index;			/*the mini index in current ae table: 1~1024*/
	 uint32_t max_fps;				/*the max fps: 1~120*/
	 uint32_t min_fps;				/*the mini fps:1~120*/
	 uint32_t exposure_time;		  /*exposure time for current frame*/
	 uint32_t cur_fps;				/*current fps:1~120*/
	 uint32_t cur_ev;			   /*ev index: 0~15*/
	 uint32_t cur_metering_mode;    /*current metering mode*/
	 uint32_t cur_exp_line; 	   /*current exposure line: the value is related to the resolution*/
	 uint32_t cur_dummy;			/*dummy line: the value is related to the resolution & fps*/
	 uint32_t cur_again;			/*current analog gain*/
	 uint32_t cur_dgain;			/*current digital gain*/
	 uint32_t cur_iso;				/*current iso mode*/
	 uint32_t cur_flicker;			/*flicker mode:0: 50HZ;1:60HZ*/
	 uint32_t cur_scene;			/*current scene mode*/
	 uint32_t cur_bv;			   /*bv parameter*/
	 uint32_t flash_effect; 	   /*flash effect ratio*/
	 uint32_t is_stab;				/*ae stable status*/
	 uint32_t line_time;		/*exposure time for one line*/
	 uint32_t frame_line;		 /*frame line length*/
	 uint32_t cur_work_mode;		/*current work mode*/
	 uint32_t histogram[256];	 /*luma histogram of current frame*/
	 uint32_t r_stat_info[1024]; /*r channel of ae statics state data*/
	 uint32_t g_stat_info[1024]; /*g channel of ae statics state data*/
	 uint32_t b_stat_info[1024]; /*b channel of ae statics state data*/
	uint8_t TC_AE_status;     /*touch ae enable*/
	int8_t TC_target_offset; /*touch target offset*/
	uint16_t TC_cur_lum;       /*touch  lum*/

	int8_t mulaes_target_offset;
	int8_t region_target_offset;
	int8_t flat_target_offset;
	int8_t reserved0;
	int32_t reserved[1022];        /*resurve for future*/
 };


struct debug_awb_display_param{
	uint8_t magic[16];
	uint32_t version;

	uint32_t r_gain;
	uint32_t g_gain;
	uint32_t b_gain;
	uint32_t cur_ct;

	uint32_t cur_awb_mode;
	uint32_t cur_work_mode;

	/* awb calibration of golden sensor */
	uint32_t golden_r; 
	uint32_t golden_g;
	uint32_t golden_b;

	/* awb calibration of random sensor */
	uint32_t random_r;
	uint32_t random_g;
	uint32_t random_b;

	int32_t cur_bv;			/*current bv*/
	int32_t cur_iso;			   /*current iso value*/

	uint32_t r_stat_info[1024]; /*r channel of awb statics state data*/
	uint32_t g_stat_info[1024]; /*g channel of awb statics state data*/
	uint32_t b_stat_info[1024]; /*b channel of awb statics state data*/

	uint32_t reserved[256]; 
};

 
struct debug_af_display_param{
	// System
	uint8_t magic[16];
	uint32_t version;		  /*version No. for this structure*/
	uint32_t af_start_time[50];		   /*[50]:AF execute frame; record start time of AF start to execute in each frame*/
	uint32_t af_end_time[50];	  /*[50]:AF execute frame; record end time of AF end to execute in each frame*/
	uint32_t work_time;			   /*work time = AF finish time - AF start time*/

	// From AE
	uint16_t    bAEisConverge;				  /*flag: check AE is converged or not*/
	uint16_t    AE_BV;						  /*brightness value*/
	uint16_t    AE_EXP;						/*exposure time (ms)*/
	uint16_t    AE_Gain; 					/*128: gain1x = 128*/

	// AF INFO
	uint8_t af_mode; 			/*0: manual; 1: auto; 2: macro; 3: INF; 4: hyper*/
	uint8_t af_search_type;			   /*0: auto; 1: type1; 2: type2; 3: type3; 4: type4; 5: type5*/
	uint8_t af_filter_type[6];			  /*[6]: six filters; af_filter_type[i]:0: auto; 1: type1; 2: type2; 3: type3; 4: type4; 5: type5*/
	uint8_t face_zone[4];		  /*face_zone[0]:X_POS; face_zone[1]:Y_POS; face_zone[2]:Width;face_zone[3]:Height; */
	uint32_t win_num;		  /*number of focus windows: 1~25*/
	uint32_t work_mode;			   /*0: SAF; 1: CAF; 2: Face-AF; 3: multi-AF; 4: touch AF*/
	uint16_t win[25][4]; 			/*win[0][0]:zone0_X_POS; win[0][1]:zone0_Y_POS; win[0][2]:zone0_Width; win[0][3]:zone0_Height; */
	uint32_t start_pos;					   /*focus position before AF work*/

	//Search data
	uint16_t scan_pos[2][50];	  /*[2]:rough/fine search; [50]:AF execute frame; record each scan position during foucsing*/
	uint32_t af_value[2][6][25][50]; 	/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; record focus value of each frame during foucsing*/
	uint32_t af_pixel_mean[2][4][25][50];  /*[2]:rough/fine search; [4]:color order: 0:R, 1:Gr, 2:Gb, 3:B; [25]:focus zone; [50]:AF execute frame; record pixel mean of each AF zone during foucsing*/
	uint16_t his_cov[2][25][50]; 		/*[2]:rough/fine search; [25]:focus zone; [50]:AF execute frame; record each covariance of histogram of each AF zone during foucsing*/
	uint8_t	MaxIdx[2][6][25][50];		  /*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; index of max statistic value*/
	uint8_t	MinIdx[2][6][25][50];		  /*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; index of min statistic value*/
	uint8_t	BPMinIdx[2][6][25][50]; 		/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; index of min statistic value before peak*/
	uint8_t	APMinIdx[2][6][25][50]; 		/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; index of min statistic value after peak*/
	uint8_t	ICBP[2][6][25][50]; 			/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; increase count before peak*/
	uint8_t	DCBP[2][6][25][50]; 			/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; decrease count before peak*/
	uint8_t	EqBP[2][6][25][50]; 			/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; equal count before peak*/  
	uint8_t	ICAP[2][6][25][50]; 			/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; increase count after peak*/
	uint8_t	DCAP[2][6][25][50]; 			/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; decrease count after peak*/
	uint8_t	EqAP[2][6][25][50]; 			   /*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; equal count after peak*/	   
	int8_t	BPC[2][6][25][50];				  /*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; ICBP - DCBP*/  
	int8_t	APC[2][6][25][50];				  /*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; ICAP - DCAP*/
	uint16_t    BP_R10000[2][4][6][25][50];	   /*[2]:rough/fine search; [4]:four type; [6]:six filters; [25]:focus zone; [50]:AF execute frame; FV ratio X 10000 before peak*/	 
	uint16_t    AP_R10000[2][4][6][25][50];	   /*[2]:rough/fine search; [4]:four type; [6]:six filters; [25]:focus zone; [50]:AF execute frame; FV ratio X 10000 after peak*/
	int32_t	fv_slope_10000[2][4][6][25][50]; /*[2]:rough/fine search; [4]:four type; [6]:six filters; [25]:focus zone; [50]:AF execute frame; record slope of focus value during this foucs*/
	uint32_t    pdaf_confidence[64]; 		   /*[64]: PD zone; confidence value of phase detection*/
	int16_t	pdaf_diff[64];					  /*[64]: PD zone; different value of phase detection*/

	//AF threshold
	uint16_t 	UB_Ratio_TH[2][4][6];		  /*[2]:rough/fine search; [4]:four type; [6]:six filters; The Up bound threshold of FV ratio, type[0]:total, type[1]:3 sample, type[2]:5 sample, type[3]:7 sample*/
	uint16_t 	LB_Ratio_TH[2][4][6];		  /*[2]:rough/fine search; [4]:four type; [6]:six filters; The low bound threshold of FV ratio, type[0]:total, type[1]:3 sample, type[2]:5 sample, type[3]:7 sample*/

	//AF result
	uint32_t suc_win_num;			 /*number of windows of sucess focus: 0~25*/
	uint8_t result_win[25];				   /*[25]: focus zone; result_win[i]: AF result of each zone*/	
	uint8_t reserved0[3];  
	uint32_t cur_pos;		  /*current focus motor position: 0~1023*/
	uint16_t peak_pos[2][6][25]; 		/*[2]:rough/fine search; [6]:six filters; [25]:focus zone;	peak position: 0~1023*/
	uint16_t choose_filter;				   /*choose which filter is used to final focus peak*/
	uint16_t choose_zone;				  /*choose which zone is used to final focus peak*/
	uint8_t total_work_frame;			  /*total work frames during AF work*/
	uint8_t	rs_total_frame; 				/*total work frames during rough search*/
	uint8_t	fs_total_frame; 				/*total work frames during fine search*/
	uint8_t	rs_dir; 						/*direction of rough search: 0:Far to near, 1:near to far*/
	uint8_t	fs_dir; 						/*direction of fine search: 0:Far to near, 1:near to far*/
	uint8_t reserved1[3];  
	//Reserve
	uint32_t reserved2[256]; 		   /*resurve for future*/
};



  typedef struct debug_isp_display_info{
	 struct debug_ae_display_param ae;
	 struct debug_awb_display_param awb;
	 struct debug_af_display_param af;
 }DEBUG_ISP_DISPLAY_INFO_T;



struct em_ae_param_set{
	uint16_t ver;
	uint8_t lock_ae;    /* 0:unloc; 1:lock */
	uint32_t exposure;    /* unit: us */
	uint32_t gain;        /* 128 means 1 time gain , user can set any value to it */
	int32_t min_fps;    /* e.g. 2000 means 20.00fps , 0 means Auto*/
	int32_t max_fps;
	int8_t iso;
	int8_t ev_index;    /* not real value , just index !! */
	int8_t flicker;
	int8_t FD_AE;        /* 0:off; 1:on */
	int8_t metering_mode;
	int8_t scene_mode;
	int8_t reserve_case;
	int16_t reserve_len;                    /*len for reserve*/
	uint8_t reserve_info[2048];    /* reserve for future */
 };


struct em_af_param_set{
	uint16_t ver;
//	......
	int8_t reserve_case;
	int16_t reserve_len;                    /*len for reserve*/
	uint8_t reserve_info[2048];    /* reserve for future */
};


struct em_awb_param_set{
	uint16_t ver;
//	......
	int8_t reserve_case;
	int16_t reserve_len;                    /*len for reserve*/
	uint8_t reserve_info[2048];    /* reserve for future */
};


typedef struct em_3A_param_set{
	struct em_ae_param_set ae;
	struct em_af_param_set af;
	struct em_awb_param_set awb;
} EM_3A_PARAM_SET;

struct em_param {
	EM_MODE_INFO em_mode;
	void *param_ptr;
};


//	from Shengzhu.Shi
struct sprd_isp_debug_info{
	uint32_t debug_startflag;  //0x65786966
	uint32_t debug_len;
	uint32_t version_id;          //0x00000001
	uint8_t data[0];
	//uint32_t debug_endflag;  // 0x637a6768
};
 
#define SPRD_DEBUG_START_FLAG   0x65786966
#define SPRD_DEBUG_END_FLAG     0x637a6768


#endif
