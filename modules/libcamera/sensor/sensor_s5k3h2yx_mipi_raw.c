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
#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"
#include "sensor_s5k3h2yx_raw_param.c"


#define s5k3h2yx_I2C_ADDR_W        0x10
#define s5k3h2yx_I2C_ADDR_R         0x10

#define DW9714_VCM_SLAVE_ADDR (0x18>>1)

#define s5k3h2yx_MIN_FRAME_LEN_PRV  0x4E0
#define s5k3h2yx_MIN_FRAME_LEN_CAP  0x9B0
#define s5k3h2yx_RAW_PARAM_COM  0x0000
LOCAL uint32_t _s5k3h2yx_GetResolutionTrimTab(uint32_t param);
LOCAL uint32_t _s5k3h2yx_PowerOn(uint32_t power_on);
LOCAL uint32_t _s5k3h2yx_Identify(uint32_t param);
LOCAL uint32_t _s5k3h2yx_BeforeSnapshot(uint32_t param);
LOCAL uint32_t _s5k3h2yx_after_snapshot(uint32_t param);
LOCAL uint32_t _s5k3h2yx_StreamOn(uint32_t param);
LOCAL uint32_t _s5k3h2yx_StreamOff(uint32_t param);
LOCAL uint32_t _s5k3h2yx_write_exposure(uint32_t param);
LOCAL uint32_t _s5k3h2yx_write_gain(uint32_t param);
LOCAL uint32_t _s5k3h2yx_write_af(uint32_t param);
LOCAL uint32_t _s5k3h2yx_ReadGain(uint32_t*gain_ptr);
LOCAL uint32_t _s5k3h2yx_SetEV(uint32_t param);
LOCAL uint32_t _dw9174_SRCInit(uint32_t mode);
LOCAL uint32_t _s5k3h2yx_com_Identify_otp(void* param_ptr);

LOCAL const struct raw_param_info_tab s_s5k3h2yx_raw_param_tab[]={
	{s5k3h2yx_RAW_PARAM_COM, &s_s5k3h2yx_mipi_raw_info, _s5k3h2yx_com_Identify_otp, PNULL},
	{RAW_INFO_END_ID, PNULL, PNULL, PNULL}
};

static uint32_t s_s5k3h2yx_gain = 0;
static uint32_t g_module_id = 0;


LOCAL const SENSOR_REG_T s5k3h2yx_com_mipi_raw[] = {
	//$MIPI[Width:3264,Height:2464,Format:RAW10,Lane:2,ErrorCheck:0,PolarityData:0
						
		//Flip/Mirror Setting				 
		//Address	Data	Comment 			 
	{0x0100, 0x00}, //
	
	{0x0103, 0x01},
	{0x0101, 0x00},
	
	{0x3210, 0x81}, // OTP CLK enable. 
	{0x3200, 0x08},
					
		//MIPI Setting					 
		//Address	Data	Comment 			 
	{0x3065, 0x35}, 	
	{0x310E, 0x00}, 	
	{0x3098, 0xAB}, 	
	{0x30C7, 0x0A}, 	
	{0x309A, 0x01}, 	
	{0x310D, 0xC6}, 	
	{0x30c3, 0x40}, 	
	{0x30BB, 0x02}, 	
	{0x30BC, 0x38}, //According to MCLK, these registers should be changed.
	{0x30BD, 0x40}, 
	{0x3110, 0x70}, 
	{0x3111, 0x80}, 
	{0x3112, 0x7B}, 
	{0x3113, 0xC0}, 
	{0x30C7, 0x1A}, 
		//Manufacture Setting					 
		//Address	Data	Comment 				 
	{0x3000, 0x08}, 	
	{0x3001, 0x05}, 	
	{0x3002, 0x0D}, 	
	{0x3003, 0x21}, 	
	{0x3004, 0x62}, 	
	{0x3005, 0x0B}, 	
	{0x3006, 0x6D}, 	
	{0x3007, 0x02}, 	
	{0x3008, 0x62}, 	
	{0x3009, 0x62}, 	
	{0x300A, 0x41}, 	
	{0x300B, 0x10}, 	
	{0x300C, 0x21}, 	
	{0x300D, 0x04}, 	
	{0x307E, 0x03}, 	
	{0x307F, 0xA5}, 	
	{0x3080, 0x04}, 	
	{0x3081, 0x29}, 	
	{0x3082, 0x03}, 	
	{0x3083, 0x21}, 	
	{0x3011, 0x5F}, 	
	{0x3156, 0xE2}, 	
	{0x3027, 0xBE}, 	//DBR_CLK enable for EMI	
	{0x300f, 0x02}, 	
	{0x3010, 0x10}, 	
	{0x3017, 0x74}, 	
	{0x3018, 0x00}, 	
	{0x3020, 0x02}, 	
	{0x3021, 0x00}, 	//EMI		
	{0x3023, 0x80}, 	
	{0x3024, 0x08}, 	
	{0x3025, 0x08}, 	
	{0x301C, 0xD4}, 	
	{0x315D, 0x00}, 	
	//s3053, 0xCF}, 	//CF for full ,CB for preview/HD/FHD/QVGA120fps move to  size configure 
	{0x3054, 0x00}, 	
	{0x3055, 0x35}, 	
	{0x3062, 0x04}, 	
	{0x3063, 0x38}, 	
	{0x31A4, 0x04}, 	
	{0x3016, 0x54}, 	
	{0x3157, 0x02}, 	
	{0x3158, 0x00}, 	
	{0x315B, 0x02}, 	
	{0x315C, 0x00}, 	
	{0x301B, 0x05}, 	
	{0x3028, 0x41}, 	
	{0x302A, 0x10}, 	//20100503 00	
	{0x3060, 0x00}, 	
	{0x302D, 0x19}, 		
	{0x302B, 0x05}, 	
	{0x3072, 0x13}, 	
	{0x3073, 0x21}, 	
	{0x3074, 0x82}, 	
	{0x3075, 0x20}, 	
	{0x3076, 0xA2}, 	
	{0x3077, 0x02}, 	
	{0x3078, 0x91}, 	
	{0x3079, 0x91}, 	
	{0x307A, 0x61}, 	
	{0x307B, 0x28}, 	
	{0x307C, 0x31}, 	
	
	//black level =64 @ 10bit 
	{0x304E, 0x40}, 	//Pedestal
	{0x304F, 0x01}, 	//Pedestal
	{0x3050, 0x00}, 	//Pedestal
	{0x3088, 0x01}, 	//Pedestal
	{0x3089, 0x00}, 	//Pedestal
	{0x3210, 0x81}, 	//Pedestal  -->OTP enable case
	{0x3211, 0x00}, 	//Pedestal
	{0x308E, 0x01}, 	//110512 update 	 
	{0x308F, 0x8F}, 	
	{0x3064, 0x03}, 	//110323 update 
	{0x31A7, 0x0F}, 	//110323 update
};

LOCAL const SENSOR_REG_T s5k3h2yx_1632X1224_mipi_raw[] = {
	//////////////////////////////////////////////////////////////////
	//////////   S5K3H2Y EVT0 setting MIPI mode	    //////////
	//////////						    //////
	/////////////// 	history     //////////////////////////////
	
	
	/////////////////////////////////////////////////////////////////////////////////////////
	//$MIPI[Width:1632,Height:1224,Format:Raw10,Lane:2ErrorCheck:0,PolarityData:0,PolarityClock:0,Buffer:2]
	
	{0x0100, 0x00},    //streamin off
	
	{0x0103, 0x01},    // sw rstn
	//s010103    //Mirror, Flip
	
	///////////////////////////////////////////////////////////////////
	///////////////      mode selection	   ////////////////////////
	
	{0x3065, 0x35},    //sync_mode[4] 0b:SMIA / 1b:parallel [5] HD mode
	{0x310E, 0x00},    // reg_sel 08h:parallel / 04h: CCP / 00h : MIPI
	
	{0x3098, 0xAB}, // [7]mipi csi2 enable
	{0x30C7, 0x0A}, // [1]mipi dphy enable
	{0x309A, 0x01}, // [0]mipi csi2 packer enable
	{0x310D, 0xc6}, // [5]No Embedded, [6]mask_corrupted en for y_addr_size
	{0x30c3, 0x40}, // mipi size auto cal
	
	// Lane selection
	{0x30BB, 0x02}, // num of lane 01h : 1-lane / 02h : 2-lane
	
	// MIPI EXCLK 24MHz
	{0x30bc, 0x38},
	{0x30bd, 0x40},
	{0x3110, 0x70},
	{0x3111, 0x80},
	{0x3112, 0x7B},
	{0x3113, 0xC0},
	{0x30c7, 0x1A},
	
	
	
	///////////////////////////////////////////////////////////////////
	////////////////       PLL	    ///////////////////////////////
	//PLL setting  ext 24MHz  912Mhz pix_clk =182.4Mhz (raw10)
	{0x308E, 0x01},
	{0x308F, 0x8F},
	
	{0x0305, 0x04}, //P		
	{0x0306, 0x00}, //M[MSB]	
	{0x0307, 0x6c}, //M[LSB]	
				
	{0x0303, 0x01}, //vt_sys_clk_div
	{0x0301, 0x05}, //vt_pix_clk_div
	{0x030b, 0x01}, //op_sys_clk_div
	{0x0309, 0x05}, //op_pix_clk_div
	
	{0x31A1, 0x5A},    // DIV_G CLK 				////////// 3H2 >>>>>>>>>>
	
	{0x30CC, 0xB0},
	//////////////////////////////////////////////////////////////////
	//////////////	      size	   ///////////////////////////////
	
	{0x0344, 0x00},//s034400	  
	{0x0345, 0x08},//s034508	 // x_addr_start (8d)	
	{0x0346, 0x00},//s034601
	{0x0347, 0x08},//s03473a		// y_addr_start (314d)
	{0x0348, 0x0C},//s03480C
	{0x0349, 0xC7},//Cf		// x_addr_end (3279d)
	{0x034A, 0x09},//s034A08
	{0x034B, 0x97},//s034B65		// y_addr_end (2149d)
	
	{0x0381, 0x01},
	{0x0383, 0x03}, 	  //X_odd
	{0x0385, 0x01},
	{0x0387, 0x03}, 	  //Y_odd
	
	{0x0401, 0x00}, // Full scaling
	{0x0405, 0x10}, 	// scaling ratio 27/16
	{0x0700, 0x05}, 	
	{0x0701, 0x30}, 	// fifo size 1486d
	{0x034c, 0x06},      //x_output_size 3264d
	{0x034d, 0x60}, 
	{0x034e, 0x04},      //y_output_size 2448d
	{0x034f, 0xc8}, 
	/////////////////////////////////////////////////////////////////////
	///////Analog initial setting 09.08.26 //////////////////////////////
	/////////////////////////////////////////////////////////////////////
	
	//CDS timing
	//(NOTE!! only optimized for pclk 65MHz or lower frequency)
	{0x3000, 0x08}, // ct_ld_satart 
	{0x3001, 0x05}, // ct_sl_start
	{0x3002, 0x0D}, // ct_rx_start
	{0x3003, 0x21}, // ct_cds_dtart
	{0x3004, 0x62}, // ct_smp_width
	{0x3005, 0x0B}, // ct_az_width 
	{0x3006, 0x6D}, // ct_s1r_width 
	{0x3007, 0x02}, // ct_tx_start
	{0x3008, 0x62}, // ct_tx_width 
	{0x3009, 0x62}, // ct_stx_width 
	{0x300A, 0x41}, // ct_dstx_width
	{0x300B, 0x10}, // ct_rmp_rst_start
	{0x300C, 0x21}, // ct_rmp_sig_start
	{0x300D, 0x04}, // ct_rmp_lat
	{0x307E, 0x03}, // ct_st_diff_nor [15:0]
	{0x307F, 0xA5}, //
	{0x3080, 0x04}, // ct_st_diff_we [15:0]
	{0x3081, 0x29}, //
	{0x3082, 0x03}, // ct_st_diff_wo [15:0]
	{0x3083, 0x21}, //
	{0x3011, 0x5F}, // [6:4] rst_mx, [3:0] sig_mx
	{0x3156, 0xE2}, // [7:5] RST offset, [4:0] SIG offset
	{0x3027, 0xBE}, // reg_option [3:1] --> rst_mx for ramp
	{0x300f, 0x02}, // 1'st amp limiter toggle
	
	//F-HD MODE CDS timing
	//(NOTE!! only optimized for pclk 193.7MHz or lower frequency)
	{0x3065, 0x35}, // [5] hd_mode
	{0x3072, 0x13}, // ct_hd_rx_start	13
	{0x3073, 0x21}, // ct_hd_cds_start	31 *
	{0x3074, 0x82}, // ct_hd_smp_width	92 *
	{0x3075, 0x20}, // ct_hd_az_width	10 *
	{0x3076, 0xA2}, // ct_hd_s1r_width	A2 *
	{0x3077, 0x02}, // ct_hd_tx_start	02
	{0x3078, 0x91}, // ct_hd_tx_width	91
	{0x3079, 0x91}, // ct_hd_stx_width	91
	{0x307A, 0x61}, // ct_hd_dstx_width	61
	{0x307B, 0x28}, // ct_hd_rmp_rst_start	18 *
	{0x307C, 0x31}, // ct_hd_rmp_sig_start	31
	
	//Multiple sampling & Continuous/Sampling mode
	{0x3010, 0x10}, // [7]msoff_en(0:no MS @<X2), [6:4]ms=1, [2]smp_en=0  
	
	//Ramp setting
	{0x3017, 0x74}, // [6:4]RMP_INIT DAC MIN, [3]Ramp reg PD, [2:0]RMP_REG 1.8V
	{0x3018, 0x00}, // rmp option, [7]ramp monit			////////// 3H2 >>>>>>>>>>
	
	//Doubler setting  
	{0x3020, 0x02}, // pd_inrush_ctrl     
	{0x3021, 0x24}, // pump ring oscillator set MSB=CP, LSB=NCP	////////// 3H2 >>>>>>>>>>
	{0x3023, 0x80}, // reg_tune_pix(Vpix voltage 3.16V)		////////// 3H2 >>>>>>>>>>
	{0x3024, 0x08}, // vtg						////////// 3H2 >>>>>>>>>>
	{0x3025, 0x08}, // reg_tune_ntg(ntg voltage)
	
	//clamp
	{0x301C, 0xD4}, // CLP level
	{0x315D, 0x00}, // clp_en_cintr 
	
	//ADLC setting
	{0x3053, 0xCF}, // L-ADLC on(dB) F&L-ADLC ON(dF) OFF(d3)	////////// 3H2 >>>>>>>>>>
	{0x3054, 0x00}, // F-adlc max
	{0x3055, 0x35}, // ADLC BPR threshold 53d 
	{0x3062, 0x04}, // F-ADLC filter A
	{0x3063, 0x38}, // F-ADLC filter B
	
	// Dithered L-ADLC //////////				////////// 3H2 >>>>>>>>>>
	{0x31A4, 0x04}, // [2:0] qec : 1~4 
	
	
	{0x3016, 0x54}, // ADC_SAT(650mV)				////////// 3H2 >>>>>>>>>>
	{0x3157, 0x02}, // ADC offset[12:8]
	{0x3158, 0x00}, // ADC offset[7:0]
	{0x315B, 0x02}, // ADC default[12:8]
	{0x315C, 0x00}, // ADC default[7:0]
	
	{0x301B, 0x05}, // Pixel Current 1.5uA				////////// 3H2 >>>>>>>>>>
		
	{0x301A, 0x77}, // CDS COMP1 BIAS[7:4], COMP2 BIAS[3:0] 	////////// 3H2 >>>>>>>>>>
	{0x3028, 0x41}, // blst on
	{0x302a, 0x00}, // blst_en_cintr
	{0x3060, 0x00}, // F_adlc_tune_total				 ////////// 3H2 >>>>>>>>>>
	
	//H_V_bining 
	{0x300E, 0x2d}, // [2] Horizontal binning enable  CLP On, LD always high , RG low clocking		////////// 3H2_Binning >>>>>>>>>>
	{0x31A3, 0x40}, // [6]: PLA enable				  ////////// 3H2_Binning >>>>>>>>>>
	
	///////////////////////////////////////////////////////////////////////////
	/////////// Digital setting 08.11.17	 //////////////////////////////////
	
	{0x302d, 0x19},   // Active low for simmian BD on mechanical shutter / default 01h(Active high)
	//s310d, 0xe6},   // mask_corrupted en for y_addr_size
	{0x3164, 0x03},  
	{0x31A7, 0x0f},
	
	///////////////////////////////////////////////////////////////////////////
	/////////////Int time & Again	       /////////////////////////
	//66ms
	
	{0x0200, 0x02},    // cintegeration time
	{0x0201, 0x50},
	{0x0202, 0x04},    // cintr //09
	{0x0203, 0x80}, 	    //a8
	{0x0204, 0x00},    // Again
	{0x0205, 0x20},
	
	{0x0342, 0x0d},    // line_length //3470d
	{0x0343, 0x80},
	{0x0340, s5k3h2yx_MIN_FRAME_LEN_PRV>>8},    // frame_length //1792d
	{0x0341, s5k3h2yx_MIN_FRAME_LEN_PRV&0xff},
	
	//{0x0100, 0x01},    // streaming on
};

LOCAL const SENSOR_REG_T s5k3h2yx_3264X2448_mipi_raw[] = {
	//////////////////////////////////////////////////////////////////
	//////////   S5K3H2Y EVT0 setting MIPI mode	    //////////
	//////////						    //////
	/////////////// 	history     //////////////////////////////
	
	
	/////////////////////////////////////////////////////////////////////////////////////////
	//$MIPI[Width:3264,Height:2448,Format:Raw10,Lane:2ErrorCheck:0,PolarityData:0,PolarityClock:0,Buffer:2]
	
	{0x0100, 0x00},    //streamin off
	
	{0x0103, 0x01},    // sw rstn
	//s0101, 0x03},    //Mirror, Flip
	
	///////////////////////////////////////////////////////////////////
	///////////////      mode selection	   ////////////////////////
	
	{0x3065, 0x35},    //sync_mode[4] 0b:SMIA / 1b:parallel [5] HD mode
	{0x310E, 0x00},    // reg_sel 08h:parallel / 04h: CCP / 00h : MIPI
	
	{0x3098, 0xAB}, // [7]mipi csi2 enable
	{0x30C7, 0x0A}, // [1]mipi dphy enable
	{0x309A, 0x01}, // [0]mipi csi2 packer enable
	{0x310D, 0xc6}, // [5]No Embedded, [6]mask_corrupted en for y_addr_size
	{0x30c3, 0x40}, // mipi size auto cal
	
	// Lane selection
	{0x30BB, 0x02}, // num of lane 01h : 1-lane / 02h : 2-lane
	
	// MIPI EXCLK 24MHz
	{0x30bc, 0x38},
	{0x30bd, 0x40},
	{0x3110, 0x70},
	{0x3111, 0x80},
	{0x3112, 0x7B},
	{0x3113, 0xC0},
	{0x30c7, 0x1A},
	
	
	
	///////////////////////////////////////////////////////////////////
	////////////////       PLL	    ///////////////////////////////
	//PLL setting  ext 24MHz  912Mhz pix_clk =182.4Mhz (raw10)
	{0x308E, 0x01},
	{0x308F, 0x8F},
	
	{0x0305, 0x04}, //P		
	{0x0306, 0x00}, //M[MSB]	
	{0x0307, 0x6c}, //M[LSB]	
				
	{0x0303, 0x01}, //vt_sys_clk_div
	{0x0301, 0x05}, //vt_pix_clk_div
	{0x030b, 0x01}, //op_sys_clk_div
	{0x0309, 0x05}, //op_pix_clk_div
	
	{0x31A1, 0x5A},    // DIV_G CLK 				////////// 3H2 >>>>>>>>>>
	
	{0x30CC, 0xB0},
	//////////////////////////////////////////////////////////////////
	//////////////	      size	   ///////////////////////////////
	
	{0x0344, 0x00},//s034400	  
	{0x0345, 0x08},//s034508	 // x_addr_start (8d)	
	{0x0346, 0x00},//s034601
	{0x0347, 0x08},//s03473a		// y_addr_start (314d)
	{0x0348, 0x0C},//s03480C
	{0x0349, 0xC7},//Cf		// x_addr_end (3279d)
	{0x034A, 0x09},//s034A08
	{0x034B, 0x97},//s034B65		// y_addr_end (2149d)
	
	{0x0381, 0x01},
	{0x0383, 0x01}, 	  //X_odd
	{0x0385, 0x01},
	{0x0387, 0x01}, 	  //Y_odd
	
	{0x0401, 0x00}, // Full scaling
	{0x0405, 0x1B}, 	// scaling ratio 27/16
	{0x0700, 0x05}, 	
	{0x0701, 0xce}, 	// fifo size 1486d
	{0x034c, 0x0c},      //x_output_size 3264d
	{0x034d, 0xc0}, 
	{0x034e, 0x09},      //y_output_size 2448d
	{0x034f, 0x90}, 
	/////////////////////////////////////////////////////////////////////
	///////Analog initial setting 09.08.26 //////////////////////////////
	/////////////////////////////////////////////////////////////////////
	
	//CDS timing
	//(NOTE!! only optimized for pclk 65MHz or lower frequency)
	{0x3000, 0x08}, // ct_ld_satart 
	{0x3001, 0x05}, // ct_sl_start
	{0x3002, 0x0D}, // ct_rx_start
	{0x3003, 0x21}, // ct_cds_dtart
	{0x3004, 0x62}, // ct_smp_width
	{0x3005, 0x0B}, // ct_az_width 
	{0x3006, 0x6D}, // ct_s1r_width 
	{0x3007, 0x02}, // ct_tx_start
	{0x3008, 0x62}, // ct_tx_width 
	{0x3009, 0x62}, // ct_stx_width 
	{0x300A, 0x41}, // ct_dstx_width
	{0x300B, 0x10}, // ct_rmp_rst_start
	{0x300C, 0x21}, // ct_rmp_sig_start
	{0x300D, 0x04}, // ct_rmp_lat
	{0x307E, 0x03}, // ct_st_diff_nor [15:0]
	{0x307F, 0xA5}, //
	{0x3080, 0x04}, // ct_st_diff_we [15:0]
	{0x3081, 0x29}, //
	{0x3082, 0x03}, // ct_st_diff_wo [15:0]
	{0x3083, 0x21}, //
	{0x3011, 0x5F}, // [6:4] rst_mx, [3:0] sig_mx
	{0x3156, 0xE2}, // [7:5] RST offset, [4:0] SIG offset
	{0x3027, 0xBE}, // reg_option [3:1] --> rst_mx for ramp
	{0x300f, 0x02}, // 1'st amp limiter toggle
	
	//F-HD MODE CDS timing
	//(NOTE!! only optimized for pclk 193.7MHz or lower frequency)
	{0x3065, 0x35}, // [5] hd_mode
	{0x3072, 0x13}, // ct_hd_rx_start	13
	{0x3073, 0x21}, // ct_hd_cds_start	31 *
	{0x3074, 0x82}, // ct_hd_smp_width	92 *
	{0x3075, 0x20}, // ct_hd_az_width	10 *
	{0x3076, 0xA2}, // ct_hd_s1r_width	A2 *
	{0x3077, 0x02}, // ct_hd_tx_start	02
	{0x3078, 0x91}, // ct_hd_tx_width	91
	{0x3079, 0x91}, // ct_hd_stx_width	91
	{0x307A, 0x61}, // ct_hd_dstx_width	61
	{0x307B, 0x28}, // ct_hd_rmp_rst_start	18 *
	{0x307C, 0x31}, // ct_hd_rmp_sig_start	31
	
	//Multiple sampling & Continuous/Sampling mode
	{0x3010, 0x10}, // [7]msoff_en(0:no MS @<X2), [6:4]ms=1, [2]smp_en=0  
	
	//Ramp setting
	{0x3017, 0x74}, // [6:4]RMP_INIT DAC MIN, [3]Ramp reg PD, [2:0]RMP_REG 1.8V
	{0x3018, 0x00}, // rmp option, [7]ramp monit			////////// 3H2 >>>>>>>>>>
	
	//Doubler setting  
	{0x3020, 0x02}, // pd_inrush_ctrl     
	{0x3021, 0x24}, // pump ring oscillator set MSB=CP, LSB=NCP	////////// 3H2 >>>>>>>>>>
	{0x3023, 0x80}, // reg_tune_pix(Vpix voltage 3.16V)		////////// 3H2 >>>>>>>>>>
	{0x3024, 0x08}, // vtg						////////// 3H2 >>>>>>>>>>
	{0x3025, 0x08}, // reg_tune_ntg(ntg voltage)
	
	//clamp
	{0x301C, 0xD4}, // CLP level
	{0x315D, 0x00}, // clp_en_cintr 
	
	//ADLC setting
	{0x3053, 0xCF}, // L-ADLC on(dB) F&L-ADLC ON(dF) OFF(d3)	////////// 3H2 >>>>>>>>>>
	{0x3054, 0x00}, // F-adlc max
	{0x3055, 0x35}, // ADLC BPR threshold 53d 
	{0x3062, 0x04}, // F-ADLC filter A
	{0x3063, 0x38}, // F-ADLC filter B
	
	// Dithered L-ADLC //////////				////////// 3H2 >>>>>>>>>>
	{0x31A4, 0x04}, // [2:0] qec : 1~4 
	
	
	{0x3016, 0x54}, // ADC_SAT(650mV)				////////// 3H2 >>>>>>>>>>
	{0x3157, 0x02}, // ADC offset[12:8]
	{0x3158, 0x00}, // ADC offset[7:0]
	{0x315B, 0x02}, // ADC default[12:8]
	{0x315C, 0x00}, // ADC default[7:0]
	
	{0x301B, 0x05}, // Pixel Current 1.5uA				////////// 3H2 >>>>>>>>>>
		
	{0x301A, 0x77}, // CDS COMP1 BIAS[7:4], COMP2 BIAS[3:0] 	////////// 3H2 >>>>>>>>>>
	{0x3028, 0x41}, // blst on
	{0x302a, 0x00}, // blst_en_cintr
	{0x3060, 0x00}, // F_adlc_tune_total				 ////////// 3H2 >>>>>>>>>>
	
	//H_V_bining 
	{0x300E, 0x29}, // [2] Horizontal binning enable  CLP On, LD always high , RG low clocking		////////// 3H2_Binning >>>>>>>>>>
	{0x31A3, 0x00}, // [6]: PLA enable				  ////////// 3H2_Binning >>>>>>>>>>
	
	///////////////////////////////////////////////////////////////////////////
	/////////// Digital setting 08.11.17	 //////////////////////////////////
	
	{0x302d, 0x19},   // Active low for simmian BD on mechanical shutter / default 01h(Active high)
	//s310de6   // mask_corrupted en for y_addr_size
	{0x3164, 0x03},  
	{0x31A7, 0x0f},
	
	///////////////////////////////////////////////////////////////////////////
	/////////////		Int time & Again	  /////////////////////////
	//66ms
	
	{0x0200, 0x02},    // cintegeration time
	{0x0201, 0x50},
	{0x0202, 0x04},    // cintr //09
	{0x0203, 0xE7}, 	    //a8
	{0x0204, 0x00},    // Again
	{0x0205, 0x20},
	
	{0x0342, 0x0d},    // line_length 3470d
	{0x0343, 0x80},
	{0x0340, s5k3h2yx_MIN_FRAME_LEN_CAP>>8},    // frame_length 2480d
	{0x0341, s5k3h2yx_MIN_FRAME_LEN_CAP&0xff},
	
	//{0x0100, 0x01},    // streaming on
};

LOCAL SENSOR_REG_TAB_INFO_T s_s5k3h2yx_resolution_Tab_RAW[] = {
	{ADDR_AND_LEN_OF_ARRAY(s5k3h2yx_com_mipi_raw), 0, 0, 24, SENSOR_IMAGE_FORMAT_RAW},

	{ADDR_AND_LEN_OF_ARRAY(s5k3h2yx_1632X1224_mipi_raw), 1632, 1224, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(s5k3h2yx_3264X2448_mipi_raw), 3264, 2448, 24, SENSOR_IMAGE_FORMAT_RAW},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}
};

LOCAL SENSOR_TRIM_T s_s5k3h2yx_Resolution_Trim_Tab[] = {
	{0, 0, 0, 0, 0, 0, 0},
	{0, 0, 1632, 1224, 267,129,1248},
	{0, 0, 3264, 2448, 267,129,2480},
	{0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0}
};


struct sensor_raw_info* s_s5k3h2yx_mipi_raw_info_ptr;

LOCAL SENSOR_IOCTL_FUNC_TAB_T s_s5k3h2yx_ioctl_func_tab = {
	PNULL,
	_s5k3h2yx_PowerOn,
	PNULL,
	_s5k3h2yx_Identify,

	PNULL,			// write register
	PNULL,			// read  register
	PNULL,
	_s5k3h2yx_GetResolutionTrimTab,

	// External
	PNULL,
	PNULL,
	PNULL,

	PNULL, //_s5k3h2yx_set_brightness,
	PNULL, // _s5k3h2yx_set_contrast,
	PNULL,
	PNULL,			//_s5k3h2yx_set_saturation,

	PNULL, //_s5k3h2yx_set_work_mode,
	PNULL, //_s5k3h2yx_set_image_effect,

	_s5k3h2yx_BeforeSnapshot,
	_s5k3h2yx_after_snapshot,
	PNULL,//_ov540_flash,
	PNULL,
	_s5k3h2yx_write_exposure,
	PNULL,
	_s5k3h2yx_write_gain,
	PNULL,
	PNULL,
	_s5k3h2yx_write_af,
	PNULL,
	PNULL, //_s5k3h2yx_set_awb,
	PNULL,
	PNULL,
	PNULL, //_s5k3h2yx_set_ev,
	PNULL,
	PNULL,
	PNULL,
	PNULL, //_s5k3h2yx_GetExifInfo,
	PNULL, //_s5k3h2yx_ExtFunc,
	PNULL, //_s5k3h2yx_set_anti_flicker,
	PNULL, //_s5k3h2yx_set_video_mode,
	PNULL, //pick_jpeg_stream
	PNULL,  //meter_mode
	PNULL, //get_status
	_s5k3h2yx_StreamOn,
	_s5k3h2yx_StreamOff,
	PNULL,
};


SENSOR_INFO_T g_s5k3h2yx_mipi_raw_info = {
	s5k3h2yx_I2C_ADDR_W,	// salve i2c write address
	s5k3h2yx_I2C_ADDR_R,	// salve i2c read address

	SENSOR_I2C_REG_16BIT | SENSOR_I2C_VAL_8BIT,	// bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
	// bit1: 0: i2c register addr  is 8 bit, 1: i2c register addr  is 16 bit
	// other bit: reseved
	SENSOR_HW_SIGNAL_PCLK_N | SENSOR_HW_SIGNAL_VSYNC_N | SENSOR_HW_SIGNAL_HSYNC_P,	// bit0: 0:negative; 1:positive -> polarily of pixel clock
	// bit2: 0:negative; 1:positive -> polarily of horizontal synchronization signal
	// bit4: 0:negative; 1:positive -> polarily of vertical synchronization signal
	// other bit: reseved

	// preview mode
	SENSOR_ENVIROMENT_NORMAL | SENSOR_ENVIROMENT_NIGHT,

	// image effect
	SENSOR_IMAGE_EFFECT_NORMAL |
	    SENSOR_IMAGE_EFFECT_BLACKWHITE |
	    SENSOR_IMAGE_EFFECT_RED |
	    SENSOR_IMAGE_EFFECT_GREEN |
	    SENSOR_IMAGE_EFFECT_BLUE |
	    SENSOR_IMAGE_EFFECT_YELLOW |
	    SENSOR_IMAGE_EFFECT_NEGATIVE | SENSOR_IMAGE_EFFECT_CANVAS,

	// while balance mode
	0,

	7,			// bit[0:7]: count of step in brightness, contrast, sharpness, saturation
	// bit[8:31] reseved

	SENSOR_HIGH_PULSE_RESET,	// reset pulse level
	50,			// reset pulse width(ms)

	SENSOR_LOW_LEVEL_PWDN,	// 1: high level valid; 0: low level valid

	1,			// count of identify code
	{{0x00, 0x38},		// supply two code to identify sensor.
	 {0x01, 0x2B}},		// for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,	// voltage of avdd

	3264,			// max width of source image
	2448,			// max height of source image
	"s5k3h2yx",		// name of sensor

	SENSOR_IMAGE_FORMAT_RAW,	// define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_RAWRGB_GR,// pattern of input image form sensor;

	s_s5k3h2yx_resolution_Tab_RAW,	// point to resolution table information structure
	&s_s5k3h2yx_ioctl_func_tab,	// point to ioctl function table
	&s_s5k3h2yx_mipi_raw_info_ptr,		// information and table about Rawrgb sensor
	NULL,			//&g_s5k3h2yx_ext_info,                // extend information about sensor
	SENSOR_AVDD_1800MV,	// iovdd
	SENSOR_AVDD_1200MV,	// dvdd
	3,			// skip frame num before preview
	3,			// skip frame num before capture
	0,			// deci frame num during preview
	0,			// deci frame num during video preview

	0,
	0,
	0,
	0,
	0,
	{SENSOR_INTERFACE_TYPE_CSI2, 2, 10, 0},
	NULL,//s_s5k3h2yx_video_info,
	3,			// skip frame num while change setting
};

LOCAL struct sensor_raw_info* Sensor_GetContext(void)
{
	return s_s5k3h2yx_mipi_raw_info_ptr;
}

LOCAL uint32_t Sensor_s5k3h2yx_InitRawTuneInfo(void)
{
	uint32_t rtn=0x00;
	struct sensor_raw_info* raw_sensor_ptr=Sensor_GetContext();
	struct sensor_raw_tune_info* sensor_ptr=raw_sensor_ptr->tune_ptr;
#if 0
	//raw_sensor_ptr->version_info->version_id=0x00000000;
	raw_sensor_ptr->version_info->srtuct_size=sizeof(struct sensor_raw_info);

	//bypass
	//sensor_ptr->version_id=0x00000000;
	sensor_ptr->blc_bypass=0x0;
	sensor_ptr->nlc_bypass=0x1;
	sensor_ptr->lnc_bypass=0x0;
	sensor_ptr->ae_bypass=0x0;
	sensor_ptr->awb_bypass=0x0;
	sensor_ptr->bpc_bypass=0x0;
	sensor_ptr->denoise_bypass=0x0;
	sensor_ptr->grgb_bypass=0x1;
	sensor_ptr->cmc_bypass=0x0;
	sensor_ptr->gamma_bypass=0x0;
	sensor_ptr->uvdiv_bypass=0x0;
	sensor_ptr->pref_bypass=0x0;
	sensor_ptr->bright_bypass=0x0;
	sensor_ptr->contrast_bypass=0x0;
	sensor_ptr->hist_bypass=0x1;
	sensor_ptr->auto_contrast_bypass=0x1;
	sensor_ptr->af_bypass=0x0;
	sensor_ptr->edge_bypass=0x0;
	sensor_ptr->fcs_bypass=0x0;
	sensor_ptr->css_bypass=0x1;
	sensor_ptr->saturation_bypass=0x1;
	sensor_ptr->hdr_bypass=0x1;
	sensor_ptr->glb_gain_bypass=0x1;
	sensor_ptr->chn_gain_bypass=0x1;

	//blc
	sensor_ptr->blc.mode=0x0;
	sensor_ptr->blc.offset[0].r=0x40;
	sensor_ptr->blc.offset[0].gr=0x40;
	sensor_ptr->blc.offset[0].gb=0x40;
	sensor_ptr->blc.offset[0].b=0x40;

	sensor_ptr->blc.offset[1].r=0x40;
	sensor_ptr->blc.offset[1].gr=0x40;
	sensor_ptr->blc.offset[1].gb=0x40;
	sensor_ptr->blc.offset[1].b=0x40;
	//nlc
	sensor_ptr->nlc.r_node[0]=0x0;
	sensor_ptr->nlc.r_node[1]=0x10;
	sensor_ptr->nlc.r_node[2]=0x20;
	sensor_ptr->nlc.r_node[3]=0x40;
	sensor_ptr->nlc.r_node[4]=0x60;
	sensor_ptr->nlc.r_node[5]=0x80;
	sensor_ptr->nlc.r_node[6]=0xA0;
	sensor_ptr->nlc.r_node[7]=0xC0;
	sensor_ptr->nlc.r_node[8]=0xE0;
	sensor_ptr->nlc.r_node[9]=0x100;
	sensor_ptr->nlc.r_node[10]=0x120;
	sensor_ptr->nlc.r_node[11]=0x140;
	sensor_ptr->nlc.r_node[12]=0x180;
	sensor_ptr->nlc.r_node[13]=0x1C0;
	sensor_ptr->nlc.r_node[14]=0x200;
	sensor_ptr->nlc.r_node[15]=0x240;
	sensor_ptr->nlc.r_node[16]=0x280;
	sensor_ptr->nlc.r_node[17]=0x2A0;
	sensor_ptr->nlc.r_node[18]=0x2C0;
	sensor_ptr->nlc.r_node[19]=0x2E0;
	sensor_ptr->nlc.r_node[20]=0x300;
	sensor_ptr->nlc.r_node[21]=0x320;
	sensor_ptr->nlc.r_node[22]=0x340;
	sensor_ptr->nlc.r_node[23]=0x360;
	sensor_ptr->nlc.r_node[24]=0x380;
	sensor_ptr->nlc.r_node[25]=0x3A0;
	sensor_ptr->nlc.r_node[26]=0x3C0;
	sensor_ptr->nlc.r_node[27]=0x3E0;
	sensor_ptr->nlc.r_node[28]=0x3FF;

	sensor_ptr->nlc.g_node[0]=0x0;
	sensor_ptr->nlc.g_node[1]=0x10;
	sensor_ptr->nlc.g_node[2]=0x20;
	sensor_ptr->nlc.g_node[3]=0x40;
	sensor_ptr->nlc.g_node[4]=0x60;
	sensor_ptr->nlc.g_node[5]=0x80;
	sensor_ptr->nlc.g_node[6]=0xA0;
	sensor_ptr->nlc.g_node[7]=0xC0;
	sensor_ptr->nlc.g_node[8]=0xE0;
	sensor_ptr->nlc.g_node[9]=0x100;
	sensor_ptr->nlc.g_node[10]=0x120;
	sensor_ptr->nlc.g_node[11]=0x140;
	sensor_ptr->nlc.g_node[12]=0x180;
	sensor_ptr->nlc.g_node[13]=0x1C0;
	sensor_ptr->nlc.g_node[14]=0x200;
	sensor_ptr->nlc.g_node[15]=0x240;
	sensor_ptr->nlc.g_node[16]=0x280;
	sensor_ptr->nlc.g_node[17]=0x2A0;
	sensor_ptr->nlc.g_node[18]=0x2C0;
	sensor_ptr->nlc.g_node[19]=0x2E0;
	sensor_ptr->nlc.g_node[20]=0x300;
	sensor_ptr->nlc.g_node[21]=0x320;
	sensor_ptr->nlc.g_node[22]=0x340;
	sensor_ptr->nlc.g_node[23]=0x360;
	sensor_ptr->nlc.g_node[24]=0x380;
	sensor_ptr->nlc.g_node[25]=0x3A0;
	sensor_ptr->nlc.g_node[26]=0x3C0;
	sensor_ptr->nlc.g_node[27]=0x3E0;
	sensor_ptr->nlc.g_node[28]=0x3FF;

	sensor_ptr->nlc.b_node[0]=0x0;
	sensor_ptr->nlc.b_node[1]=0x10;
	sensor_ptr->nlc.b_node[2]=0x20;
	sensor_ptr->nlc.b_node[3]=0x40;
	sensor_ptr->nlc.b_node[4]=0x60;
	sensor_ptr->nlc.b_node[5]=0x80;
	sensor_ptr->nlc.b_node[6]=0xA0;
	sensor_ptr->nlc.b_node[7]=0xC0;
	sensor_ptr->nlc.b_node[8]=0xE0;
	sensor_ptr->nlc.b_node[9]=0x100;
	sensor_ptr->nlc.b_node[10]=0x120;
	sensor_ptr->nlc.b_node[11]=0x140;
	sensor_ptr->nlc.b_node[12]=0x180;
	sensor_ptr->nlc.b_node[13]=0x1C0;
	sensor_ptr->nlc.b_node[14]=0x200;
	sensor_ptr->nlc.b_node[15]=0x240;
	sensor_ptr->nlc.b_node[16]=0x280;
	sensor_ptr->nlc.b_node[17]=0x2A0;
	sensor_ptr->nlc.b_node[18]=0x2C0;
	sensor_ptr->nlc.b_node[19]=0x2E0;
	sensor_ptr->nlc.b_node[20]=0x300;
	sensor_ptr->nlc.b_node[21]=0x320;
	sensor_ptr->nlc.b_node[22]=0x340;
	sensor_ptr->nlc.b_node[23]=0x360;
	sensor_ptr->nlc.b_node[24]=0x380;
	sensor_ptr->nlc.b_node[25]=0x3A0;
	sensor_ptr->nlc.b_node[26]=0x3C0;
	sensor_ptr->nlc.b_node[27]=0x3E0;
	sensor_ptr->nlc.b_node[28]=0x3FF;

	sensor_ptr->nlc.l_node[0]=0x0;
	sensor_ptr->nlc.l_node[1]=0x10;
	sensor_ptr->nlc.l_node[2]=0x20;
	sensor_ptr->nlc.l_node[3]=0x40;
	sensor_ptr->nlc.l_node[4]=0x60;
	sensor_ptr->nlc.l_node[5]=0x80;
	sensor_ptr->nlc.l_node[6]=0xA0;
	sensor_ptr->nlc.l_node[7]=0xC0;
	sensor_ptr->nlc.l_node[8]=0xE0;
	sensor_ptr->nlc.l_node[9]=0x100;
	sensor_ptr->nlc.l_node[10]=0x120;
	sensor_ptr->nlc.l_node[11]=0x140;
	sensor_ptr->nlc.l_node[12]=0x180;
	sensor_ptr->nlc.l_node[13]=0x1C0;
	sensor_ptr->nlc.l_node[14]=0x200;
	sensor_ptr->nlc.l_node[15]=0x240;
	sensor_ptr->nlc.l_node[16]=0x280;
	sensor_ptr->nlc.l_node[17]=0x2A0;
	sensor_ptr->nlc.l_node[18]=0x2C0;
	sensor_ptr->nlc.l_node[19]=0x2E0;
	sensor_ptr->nlc.l_node[20]=0x300;
	sensor_ptr->nlc.l_node[21]=0x320;
	sensor_ptr->nlc.l_node[22]=0x340;
	sensor_ptr->nlc.l_node[23]=0x360;
	sensor_ptr->nlc.l_node[24]=0x380;
	sensor_ptr->nlc.l_node[25]=0x3A0;
	sensor_ptr->nlc.l_node[26]=0x3C0;
	sensor_ptr->nlc.l_node[27]=0x3E0;
	sensor_ptr->nlc.l_node[28]=0x3FF;

	//ae
	sensor_ptr->ae.skip_frame=0x1;
	sensor_ptr->ae.normal_fix_fps=0;
	sensor_ptr->ae.night_fix_fps=0;
	sensor_ptr->ae.video_fps=0x1E;
	sensor_ptr->ae.target_lum=0x32;
	sensor_ptr->ae.target_zone=0x5;
	sensor_ptr->ae.quick_mode=0x1;
	sensor_ptr->ae.smart=0x0;
	sensor_ptr->ae.smart_rotio=0xFF;
	sensor_ptr->ae.ev[0]=0xE8;
	sensor_ptr->ae.ev[1]=0xF0;
	sensor_ptr->ae.ev[2]=0xF8;
	sensor_ptr->ae.ev[3]=0x0;
	sensor_ptr->ae.ev[4]=0x8;
	sensor_ptr->ae.ev[5]=0x10;
	sensor_ptr->ae.ev[6]=0x18;
	sensor_ptr->ae.ev[7]=0x0;
	sensor_ptr->ae.ev[8]=0x0;
	sensor_ptr->ae.ev[9]=0x0;
	sensor_ptr->ae.ev[10]=0x0;
	sensor_ptr->ae.ev[11]=0x0;
	sensor_ptr->ae.ev[12]=0x0;
	sensor_ptr->ae.ev[13]=0x0;
	sensor_ptr->ae.ev[14]=0x0;
	sensor_ptr->ae.ev[15]=0x0;

	//awb
	sensor_ptr->awb.win_start.x=0x10;
	sensor_ptr->awb.win_start.y=0x4;
	sensor_ptr->awb.win_size.w=0x32;
	sensor_ptr->awb.win_size.h=0x26;
	sensor_ptr->awb.quick_mode = 1;
	sensor_ptr->awb.r_gain[0]=0x1B0;
	sensor_ptr->awb.g_gain[0]=0xFF;
	sensor_ptr->awb.b_gain[0]=0x180;
	sensor_ptr->awb.r_gain[1]=0x100;
	sensor_ptr->awb.g_gain[1]=0xFF;
	sensor_ptr->awb.b_gain[1]=0x210;
	sensor_ptr->awb.r_gain[2]=0xFF;
	sensor_ptr->awb.g_gain[2]=0xFF;
	sensor_ptr->awb.b_gain[2]=0xFF;
	sensor_ptr->awb.r_gain[3]=0xFF;
	sensor_ptr->awb.g_gain[3]=0xFF;
	sensor_ptr->awb.b_gain[3]=0xFF;
	sensor_ptr->awb.r_gain[4]=0x13E;
	sensor_ptr->awb.g_gain[4]=0xFF;
	sensor_ptr->awb.b_gain[4]=0x1B4;
	sensor_ptr->awb.r_gain[5]=0x194;
	sensor_ptr->awb.g_gain[5]=0xFF;
	sensor_ptr->awb.b_gain[5]=0x134;
	sensor_ptr->awb.r_gain[6]=0x240;
	sensor_ptr->awb.g_gain[6]=0xFF;
	sensor_ptr->awb.b_gain[6]=0x12C;
	sensor_ptr->awb.r_gain[7]=0xFF;
	sensor_ptr->awb.g_gain[7]=0xFF;
	sensor_ptr->awb.b_gain[7]=0xFF;
	sensor_ptr->awb.r_gain[8]=0xFF;
	sensor_ptr->awb.g_gain[8]=0xFF;
	sensor_ptr->awb.b_gain[8]=0xFF;
	sensor_ptr->awb.target_zone=0x10;


	/*awb win*/
	sensor_ptr->awb.win[0].x =0x9D;
	sensor_ptr->awb.win[1].x =0x9F;
	sensor_ptr->awb.win[2].x =0xA1;
	sensor_ptr->awb.win[3].x =0xA3;
	sensor_ptr->awb.win[4].x =0xAA;
	sensor_ptr->awb.win[5].x =0xAF;
	sensor_ptr->awb.win[6].x =0xB8;
	sensor_ptr->awb.win[7].x =0xBF;
	sensor_ptr->awb.win[8].x =0xC3;
	sensor_ptr->awb.win[9].x =0xCC;
	sensor_ptr->awb.win[10].x=0xD7;
	sensor_ptr->awb.win[11].x=0xE2;
	sensor_ptr->awb.win[12].x=0xEE;
	sensor_ptr->awb.win[13].x=0xFA;
	sensor_ptr->awb.win[14].x=0x104;
	sensor_ptr->awb.win[15].x=0x10F;
	sensor_ptr->awb.win[16].x=0x119;
	sensor_ptr->awb.win[17].x=0x121;
	sensor_ptr->awb.win[18].x=0x12E;
	sensor_ptr->awb.win[19].x=0x138;

	sensor_ptr->awb.win[0].yb =0xBC;
	sensor_ptr->awb.win[1].yb =0xAC;
	sensor_ptr->awb.win[2].yb =0x9A;
	sensor_ptr->awb.win[3].yb =0x8F;
	sensor_ptr->awb.win[4].yb =0x78;
	sensor_ptr->awb.win[5].yb =0x6B;
	sensor_ptr->awb.win[6].yb =0x67;
	sensor_ptr->awb.win[7].yb =0x67;
	sensor_ptr->awb.win[8].yb =0x67;
	sensor_ptr->awb.win[9].yb =0x70;
	sensor_ptr->awb.win[10].yb=0x76;
	sensor_ptr->awb.win[11].yb=0x5D;
	sensor_ptr->awb.win[12].yb=0x5B;
	sensor_ptr->awb.win[13].yb=0x5D;
	sensor_ptr->awb.win[14].yb=0x5E;
	sensor_ptr->awb.win[15].yb=0x5F;
	sensor_ptr->awb.win[16].yb=0x64;
	sensor_ptr->awb.win[17].yb=0x5E;
	sensor_ptr->awb.win[18].yb=0x54;
	sensor_ptr->awb.win[19].yb=0x54;

	sensor_ptr->awb.win[0].yt =0xD7;
	sensor_ptr->awb.win[1].yt =0xE0;
	sensor_ptr->awb.win[2].yt =0xE1;
	sensor_ptr->awb.win[3].yt =0xE3;
	sensor_ptr->awb.win[4].yt =0xE6;
	sensor_ptr->awb.win[5].yt =0xE4;
	sensor_ptr->awb.win[6].yt =0xE2;
	sensor_ptr->awb.win[7].yt =0xDA;
	sensor_ptr->awb.win[8].yt =0xB3;
	sensor_ptr->awb.win[9].yt =0xB1;
	sensor_ptr->awb.win[10].yt=0x90;
	sensor_ptr->awb.win[11].yt=0x8F;
	sensor_ptr->awb.win[12].yt=0x75;
	sensor_ptr->awb.win[13].yt=0x71;
	sensor_ptr->awb.win[14].yt=0x83;
	sensor_ptr->awb.win[15].yt=0x80;
	sensor_ptr->awb.win[16].yt=0x72;
	sensor_ptr->awb.win[17].yt=0x6D;
	sensor_ptr->awb.win[18].yt=0x73;
	sensor_ptr->awb.win[19].yt=0x72;

	sensor_ptr->awb.gain_convert[0].r=0x100;
	sensor_ptr->awb.gain_convert[0].g=0x100;
	sensor_ptr->awb.gain_convert[0].b=0x100;

	sensor_ptr->awb.gain_convert[1].r=0x100;
	sensor_ptr->awb.gain_convert[1].g=0x100;
	sensor_ptr->awb.gain_convert[1].b=0x100;

	//bpc
	sensor_ptr->bpc.flat_thr=0x50;
	sensor_ptr->bpc.std_thr=0x14;
	sensor_ptr->bpc.texture_thr=0x2;

	// denoise
	sensor_ptr->denoise.write_back=0x2;
	sensor_ptr->denoise.r_thr=0x8;
	sensor_ptr->denoise.g_thr=0x8;
	sensor_ptr->denoise.b_thr=0x8;
	sensor_ptr->denoise.diswei[0]=0xFF;
	sensor_ptr->denoise.diswei[1]=0xF7;
	sensor_ptr->denoise.diswei[2]=0xEF;
	sensor_ptr->denoise.diswei[3]=0xE8;
	sensor_ptr->denoise.diswei[4]=0xE1;
	sensor_ptr->denoise.diswei[5]=0xDA;
	sensor_ptr->denoise.diswei[6]=0xD3;
	sensor_ptr->denoise.diswei[7]=0xCC;
	sensor_ptr->denoise.diswei[8]=0xC6;
	sensor_ptr->denoise.diswei[9]=0xC0;
	sensor_ptr->denoise.diswei[10]=0xBA;
	sensor_ptr->denoise.diswei[11]=0xB4;
	sensor_ptr->denoise.diswei[12]=0xAF;
	sensor_ptr->denoise.diswei[13]=0xA9;
	sensor_ptr->denoise.diswei[14]=0xA4;
	sensor_ptr->denoise.diswei[15]=0x9F;
	sensor_ptr->denoise.diswei[16]=0x9A;
	sensor_ptr->denoise.diswei[17]=0x95;
	sensor_ptr->denoise.diswei[18]=0x91;

	sensor_ptr->denoise.ranwei[0]=0xFF;
	sensor_ptr->denoise.ranwei[1]=0xF7;
	sensor_ptr->denoise.ranwei[2]=0xE1;
	sensor_ptr->denoise.ranwei[3]=0xC0;
	sensor_ptr->denoise.ranwei[4]=0x9A;
	sensor_ptr->denoise.ranwei[5]=0x74;
	sensor_ptr->denoise.ranwei[6]=0x52;
	sensor_ptr->denoise.ranwei[7]=0x37;
	sensor_ptr->denoise.ranwei[8]=0x22;
	sensor_ptr->denoise.ranwei[9]=0x14;
	sensor_ptr->denoise.ranwei[10]=0x13;
	sensor_ptr->denoise.ranwei[11]=0x12;
	sensor_ptr->denoise.ranwei[12]=0x11;
	sensor_ptr->denoise.ranwei[13]=0x10;
	sensor_ptr->denoise.ranwei[14]=0xF;
	sensor_ptr->denoise.ranwei[15]=0xE;
	sensor_ptr->denoise.ranwei[16]=0xD;
	sensor_ptr->denoise.ranwei[17]=0xC;
	sensor_ptr->denoise.ranwei[18]=0xB;
	sensor_ptr->denoise.ranwei[19]=0xA;
	sensor_ptr->denoise.ranwei[20]=0x9;
	sensor_ptr->denoise.ranwei[21]=0x8;
	sensor_ptr->denoise.ranwei[22]=0x7;
	sensor_ptr->denoise.ranwei[23]=0x6;
	sensor_ptr->denoise.ranwei[24]=0x5;
	sensor_ptr->denoise.ranwei[25]=0x4;
	sensor_ptr->denoise.ranwei[26]=0x3;
	sensor_ptr->denoise.ranwei[27]=0x2;
	sensor_ptr->denoise.ranwei[28]=0x1;
	sensor_ptr->denoise.ranwei[29]=0x1;
	sensor_ptr->denoise.ranwei[30]=0x1;

	//GrGb
	sensor_ptr->grgb.edge_thr=0x1A;
	sensor_ptr->grgb.diff_thr=0x50;

	//cfa
	sensor_ptr->cfa.edge_thr=0x1A;
	sensor_ptr->cfa.diff_thr=0x0;

	//cmc
	sensor_ptr->cmc.matrix[0][0]=0x715;
	sensor_ptr->cmc.matrix[0][1]=0x3DFB;
	sensor_ptr->cmc.matrix[0][2]=0x3EF0;
	sensor_ptr->cmc.matrix[0][3]=0x3E37;
	sensor_ptr->cmc.matrix[0][4]=0x6F8;
	sensor_ptr->cmc.matrix[0][5]=0x3ED1;
	sensor_ptr->cmc.matrix[0][6]=0x3FB3;
	sensor_ptr->cmc.matrix[0][7]=0x3D41;
	sensor_ptr->cmc.matrix[0][8]=0x70C;

	//Gamma
	sensor_ptr->gamma.axis[0][0]=0x0;
	sensor_ptr->gamma.axis[0][1]=0x8;
	sensor_ptr->gamma.axis[0][2]=0x10;
	sensor_ptr->gamma.axis[0][3]=0x18;
	sensor_ptr->gamma.axis[0][4]=0x20;
	sensor_ptr->gamma.axis[0][5]=0x30;
	sensor_ptr->gamma.axis[0][6]=0x40;
	sensor_ptr->gamma.axis[0][7]=0x50;
	sensor_ptr->gamma.axis[0][8]=0x60;
	sensor_ptr->gamma.axis[0][9]=0x80;
	sensor_ptr->gamma.axis[0][10]=0xA0;
	sensor_ptr->gamma.axis[0][11]=0xC0;
	sensor_ptr->gamma.axis[0][12]=0xE0;
	sensor_ptr->gamma.axis[0][13]=0x100;
	sensor_ptr->gamma.axis[0][14]=0x120;
	sensor_ptr->gamma.axis[0][15]=0x140;
	sensor_ptr->gamma.axis[0][16]=0x180;
	sensor_ptr->gamma.axis[0][17]=0x1C0;
	sensor_ptr->gamma.axis[0][18]=0x200;
	sensor_ptr->gamma.axis[0][19]=0x240;
	sensor_ptr->gamma.axis[0][20]=0x280;
	sensor_ptr->gamma.axis[0][21]=0x300;
	sensor_ptr->gamma.axis[0][22]=0x340;
	sensor_ptr->gamma.axis[0][23]=0x380;
	sensor_ptr->gamma.axis[0][24]=0x3C0;
	sensor_ptr->gamma.axis[0][25]=0x3FF;

	sensor_ptr->gamma.axis[1][0]=0x0;
	sensor_ptr->gamma.axis[1][1]=0x5;
	sensor_ptr->gamma.axis[1][2]=0xC;
	sensor_ptr->gamma.axis[1][3]=0x12;
	sensor_ptr->gamma.axis[1][4]=0x18;
	sensor_ptr->gamma.axis[1][5]=0x24;
	sensor_ptr->gamma.axis[1][6]=0x35;
	sensor_ptr->gamma.axis[1][7]=0x46;
	sensor_ptr->gamma.axis[1][8]=0x52;
	sensor_ptr->gamma.axis[1][9]=0x65;
	sensor_ptr->gamma.axis[1][10]=0x76;
	sensor_ptr->gamma.axis[1][11]=0x82;
	sensor_ptr->gamma.axis[1][12]=0x8E;
	sensor_ptr->gamma.axis[1][13]=0x96;
	sensor_ptr->gamma.axis[1][14]=0x9F;
	sensor_ptr->gamma.axis[1][15]=0xA6;
	sensor_ptr->gamma.axis[1][16]=0xB4;
	sensor_ptr->gamma.axis[1][17]=0xC1;
	sensor_ptr->gamma.axis[1][18]=0xCB;
	sensor_ptr->gamma.axis[1][19]=0xD4;
	sensor_ptr->gamma.axis[1][20]=0xDC;
	sensor_ptr->gamma.axis[1][21]=0xEA;
	sensor_ptr->gamma.axis[1][22]=0xF0;
	sensor_ptr->gamma.axis[1][23]=0xF6;
	sensor_ptr->gamma.axis[1][24]=0xFC;
	sensor_ptr->gamma.axis[1][25]=0xFF;

	//uv div
	sensor_ptr->uv_div.thrd[0]=0xFC;
	sensor_ptr->uv_div.thrd[1]=0xFA;
	sensor_ptr->uv_div.thrd[2]=0xF8;
	sensor_ptr->uv_div.thrd[3]=0xF6;
	sensor_ptr->uv_div.thrd[4]=0xF4;
	sensor_ptr->uv_div.thrd[5]=0xF2;
	sensor_ptr->uv_div.thrd[6]=0xF0;

	//pref
	sensor_ptr->pref.write_back=0x0;
	sensor_ptr->pref.y_thr=0x4;
	sensor_ptr->pref.u_thr=0x4;
	sensor_ptr->pref.v_thr=0x4;

	//bright
	sensor_ptr->bright.factor[0]=0xD0;
	sensor_ptr->bright.factor[1]=0xE0;
	sensor_ptr->bright.factor[2]=0xF0;
	sensor_ptr->bright.factor[3]=0x0;
	sensor_ptr->bright.factor[4]=0x10;
	sensor_ptr->bright.factor[5]=0x20;
	sensor_ptr->bright.factor[6]=0x30;
	sensor_ptr->bright.factor[7]=0x0;
	sensor_ptr->bright.factor[8]=0x0;
	sensor_ptr->bright.factor[9]=0x0;
	sensor_ptr->bright.factor[10]=0x0;
	sensor_ptr->bright.factor[11]=0x0;
	sensor_ptr->bright.factor[12]=0x0;
	sensor_ptr->bright.factor[13]=0x0;
	sensor_ptr->bright.factor[14]=0x0;
	sensor_ptr->bright.factor[15]=0x0;

	//contrast
	sensor_ptr->contrast.factor[0]=0x10;
	sensor_ptr->contrast.factor[1]=0x20;
	sensor_ptr->contrast.factor[2]=0x30;
	sensor_ptr->contrast.factor[3]=0x40;
	sensor_ptr->contrast.factor[4]=0x50;
	sensor_ptr->contrast.factor[5]=0x60;
	sensor_ptr->contrast.factor[6]=0x70;
	sensor_ptr->contrast.factor[7]=0x40;
	sensor_ptr->contrast.factor[8]=0x40;
	sensor_ptr->contrast.factor[9]=0x40;
	sensor_ptr->contrast.factor[10]=0x40;
	sensor_ptr->contrast.factor[11]=0x40;
	sensor_ptr->contrast.factor[12]=0x40;
	sensor_ptr->contrast.factor[13]=0x40;
	sensor_ptr->contrast.factor[14]=0x40;
	sensor_ptr->contrast.factor[15]=0x40;

	//hist
	sensor_ptr->hist.mode=0x0;
	sensor_ptr->hist.low_ratio=0x0;
	sensor_ptr->hist.high_ratio=0x0;

	//auto contrast
	sensor_ptr->auto_contrast.mode=0x0;

	//saturation
	sensor_ptr->saturation.factor[0]=0x40;
	sensor_ptr->saturation.factor[1]=0x40;
	sensor_ptr->saturation.factor[2]=0x40;
	sensor_ptr->saturation.factor[3]=0x40;
	sensor_ptr->saturation.factor[4]=0x40;
	sensor_ptr->saturation.factor[5]=0x40;
	sensor_ptr->saturation.factor[6]=0x40;
	sensor_ptr->saturation.factor[7]=0x40;
	sensor_ptr->saturation.factor[8]=0x40;
	sensor_ptr->saturation.factor[9]=0x40;
	sensor_ptr->saturation.factor[10]=0x40;
	sensor_ptr->saturation.factor[11]=0x40;
	sensor_ptr->saturation.factor[12]=0x40;
	sensor_ptr->saturation.factor[13]=0x40;
	sensor_ptr->saturation.factor[14]=0x40;
	sensor_ptr->saturation.factor[15]=0x40;

	//af info
	sensor_ptr->af.max_step=0x3ff;
	sensor_ptr->af.stab_period=0x14;

	//edge
	sensor_ptr->edge.info[0].detail_thr=0x3;
	sensor_ptr->edge.info[0].smooth_thr=0x5;
	sensor_ptr->edge.info[0].strength=0xA;
	sensor_ptr->edge.info[1].detail_thr=0x3;
	sensor_ptr->edge.info[1].smooth_thr=0x5;
	sensor_ptr->edge.info[1].strength=0xA;
	sensor_ptr->edge.info[2].detail_thr=0x3;
	sensor_ptr->edge.info[2].smooth_thr=0x5;
	sensor_ptr->edge.info[2].strength=0xA;
	sensor_ptr->edge.info[3].detail_thr=0x3;
	sensor_ptr->edge.info[3].smooth_thr=0x5;
	sensor_ptr->edge.info[3].strength=0xA;
	sensor_ptr->edge.info[4].detail_thr=0x3;
	sensor_ptr->edge.info[4].smooth_thr=0x5;
	sensor_ptr->edge.info[4].strength=0xA;
	sensor_ptr->edge.info[5].detail_thr=0x3;
	sensor_ptr->edge.info[5].smooth_thr=0x5;
	sensor_ptr->edge.info[5].strength=0xA;

	//emboss
	sensor_ptr->emboss.step=0x0;

	//global gain
	sensor_ptr->global.gain=0x0;

	//chn gain
	sensor_ptr->chn.r_gain=0x40;
	sensor_ptr->chn.g_gain=0x40;
	sensor_ptr->chn.b_gain=0x40;
	sensor_ptr->chn.r_offset=0x0;
	sensor_ptr->chn.r_offset=0x0;
	sensor_ptr->chn.r_offset=0x0;
#endif
	return rtn;
}


LOCAL uint32_t _s5k3h2yx_GetResolutionTrimTab(uint32_t param)
{
	SENSOR_PRINT("0x%x", (uint32_t)s_s5k3h2yx_Resolution_Trim_Tab);
	return (uint32_t) s_s5k3h2yx_Resolution_Trim_Tab;
}
LOCAL uint32_t _s5k3h2yx_PowerOn(uint32_t power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_s5k3h2yx_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_s5k3h2yx_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_s5k3h2yx_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_s5k3h2yx_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_s5k3h2yx_mipi_raw_info.reset_pulse_level;
	//uint32_t reset_width=g_s5k3h2yx_yuv_info.reset_pulse_width;

	if (SENSOR_TRUE == power_on) {
		Sensor_PowerDown(power_down);
		// Open power
		Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
		usleep(20*1000);
		//_dw9174_SRCInit(2);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(10*1000);
		Sensor_PowerDown(!power_down);
		// Reset sensor
		Sensor_Reset(reset_level);
	} else {
		Sensor_PowerDown(power_down);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
	}
	SENSOR_PRINT("SENSOR_s5k3h2yx: _s5k3h2yx_Power_On(1:on, 0:off): %d", power_on);
	return SENSOR_SUCCESS;
}

LOCAL uint32_t _s5k3h2yx_cfg_otp(uint32_t  param)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_s5k3h2yx_raw_param_tab;
	uint32_t module_id=g_module_id;

	SENSOR_PRINT("SENSOR_s5k3h2yx: _s5k3h2yx_cfg_otp");

	if(PNULL!=tab_ptr[module_id].cfg_otp){
		tab_ptr[module_id].cfg_otp(0);
		}

	return rtn;
}

LOCAL uint32_t _s5k3h2yx_com_Identify_otp(void* param_ptr)
{
	uint32_t rtn=SENSOR_FAIL;
	uint32_t param_id;

	SENSOR_PRINT("SENSOR_s5k3h2yx: _s5k3h2yx_com_Identify_otp");

	/*read param id from sensor omap*/
	param_id=s5k3h2yx_RAW_PARAM_COM;

	if(s5k3h2yx_RAW_PARAM_COM==param_id){
		rtn=SENSOR_SUCCESS;
	}

	return rtn;
}

LOCAL uint32_t _s5k3h2yx_GetRawInof(void)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_s5k3h2yx_raw_param_tab;
	uint32_t param_id;
	uint32_t i=0x00;

	/*read param id from sensor omap*/
	param_id=s5k3h2yx_RAW_PARAM_COM;

	for(i=0x00; ; i++)
	{
		g_module_id = i;
		if(RAW_INFO_END_ID==tab_ptr[i].param_id){
			if(NULL==s_s5k3h2yx_mipi_raw_info_ptr){
				SENSOR_PRINT("SENSOR_s5k3h2yx: ov5647_GetRawInof no param error");
				rtn=SENSOR_FAIL;
			}
			SENSOR_PRINT("SENSOR_s5k3h2yx: s5k3h2yx_GetRawInof end");
			break;
		}
		else if(PNULL!=tab_ptr[i].identify_otp){
			if(SENSOR_SUCCESS==tab_ptr[i].identify_otp(0))
			{
				s_s5k3h2yx_mipi_raw_info_ptr = tab_ptr[i].info_ptr;
				SENSOR_PRINT("SENSOR_s5k3h2yx: s5k3h2yx_GetRawInof success");
				break;
			}
		}
	}

	return rtn;
}

LOCAL uint32_t _s5k3h2yx_GetMaxFrameLine(uint32_t index)
{
	uint32_t max_line=0x00;
	SENSOR_TRIM_T_PTR trim_ptr=s_s5k3h2yx_Resolution_Trim_Tab;

	max_line=trim_ptr[index].frame_line;

	return max_line;
}
LOCAL uint32_t _s5k3h2yx_Identify(uint32_t param)
{
#define s5k3h2yx_PID_VALUE    0x38
#define s5k3h2yx_PID_ADDR     0x0000
#define s5k3h2yx_VER_VALUE    0x2B
#define s5k3h2yx_VER_ADDR     0x0001

	uint8_t pid_value = 0x00;
	uint8_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT("SENSOR_s5k3h2yx: mipi raw identify\n");

	pid_value = Sensor_ReadReg(s5k3h2yx_PID_ADDR);

	if (s5k3h2yx_PID_VALUE == pid_value) {
		ver_value = Sensor_ReadReg(s5k3h2yx_VER_ADDR);
		SENSOR_PRINT("SENSOR_s5k3h2yx: Identify: PID = %x, VER = %x", pid_value, ver_value);
		if (s5k3h2yx_VER_VALUE == ver_value) {
			SENSOR_PRINT_HIGH("SENSOR_s5k3h2yx: this is s5k3h2yx sensor !");
			ret_value=_s5k3h2yx_GetRawInof();
			if(SENSOR_SUCCESS != ret_value)
			{
				SENSOR_PRINT_ERR("SENSOR_s5k3h2yx: the module is unknow error !");
			}
			Sensor_s5k3h2yx_InitRawTuneInfo();
		} else {
			SENSOR_PRINT_HIGH("SENSOR_s5k3h2yx: identify fail,ver_value=%d", ver_value);
		}
	} else {
		SENSOR_PRINT_ERR("SENSOR_s5k3h2yx: identify fail,pid_value=%d", pid_value);
	}
	
	return ret_value;
}

LOCAL uint32_t _s5k3h2yx_write_exposure(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t expsure_line = 0x00;
	uint16_t dummy_line = 0x00;
	uint16_t frame_len_cur = 0x00;
	uint16_t frame_len = 0x00;
	uint16_t size_index=0x00;
	uint16_t max_frame_len=0x00;

	expsure_line=param&0xffff;
	dummy_line=(param>>0x10)&0x0fff;
	size_index=(param>>0x1c)&0x0f;
	SENSOR_PRINT("SENSOR_s5k3h2yx: write_exposure line:%d, dummy:%d, size_index:%d", expsure_line, dummy_line, size_index);
	max_frame_len=_s5k3h2yx_GetMaxFrameLine(size_index);
	if(expsure_line < 3){
		expsure_line = 3;
	}

	frame_len = expsure_line + dummy_line;
	frame_len = frame_len > (expsure_line + 8) ? frame_len : (expsure_line + 8);
	frame_len = (frame_len > max_frame_len) ? frame_len : max_frame_len;
	if(0x00!=(0x01&frame_len))
	{
		frame_len+=0x01;
	}


	frame_len_cur = (Sensor_ReadReg(0x0341))&0xff;
	frame_len_cur |= (Sensor_ReadReg(0x0340)<<0x08)&0xff00;


	ret_value = Sensor_WriteReg(0x104, 0x01);
	if(frame_len_cur < frame_len){
		ret_value = Sensor_WriteReg(0x0341, frame_len & 0xff);
		ret_value = Sensor_WriteReg(0x0340, (frame_len >> 0x08) & 0xff);
	}
	
	ret_value = Sensor_WriteReg(0x203, expsure_line & 0xff);
	ret_value = Sensor_WriteReg(0x202, (expsure_line >> 0x08) & 0xff);
	
	if(frame_len_cur > frame_len){
		ret_value = Sensor_WriteReg(0x0341, frame_len & 0xff);
		ret_value = Sensor_WriteReg(0x0340, (frame_len >> 0x08) & 0xff);
	}
	ret_value = Sensor_WriteReg(0x104, 0x00);

	return ret_value;
}

LOCAL uint32_t _s5k3h2yx_write_gain(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t value=0x00;
	uint32_t real_gain = 0;

	real_gain = ((param&0xf)+16)*(((param>>4)&0x01)+1)*(((param>>5)&0x01)+1);
	real_gain = real_gain*(((param>>6)&0x01)+1)*(((param>>7)&0x01)+1)*(((param>>8)&0x01)+1);

	real_gain = real_gain<<1;
	SENSOR_PRINT("SENSOR_s5k3h2yx: real_gain:0x%x, param: 0x%x", real_gain, param);

	ret_value = Sensor_WriteReg(0x104, 0x01);
	value = real_gain>>0x08;
	ret_value = Sensor_WriteReg(0x204, value);
	value = real_gain&0xff;
	ret_value = Sensor_WriteReg(0x205, value);

	ret_value = Sensor_WriteReg(0x104, 0x00);

	return ret_value;
}

LOCAL uint32_t _s5k3h2yx_write_af(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint8_t cmd_val[2] = {0x00};
	uint16_t  slave_addr = 0;
	uint16_t cmd_len = 0;

	SENSOR_PRINT("SENSOR_s5k3h2yx: _write_af %d", param);

	//for direct mode
	slave_addr = DW9714_VCM_SLAVE_ADDR;
	cmd_val[0] = (param&0xfff0)>>4;
	cmd_val[1] = ((param&0x0f)<<4)|0x0C;
	cmd_len = 2;	
	ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
	SENSOR_PRINT("SENSOR_s5k3h2yx: _write_af, ret =  %d, MSL:%x, LSL:%x\n", ret_value, cmd_val[0], cmd_val[1]);

	return ret_value;
}

LOCAL uint32_t _s5k3h2yx_ReadGain(uint32_t*  gain_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
	uint32_t gain = 0;
	uint16_t value=0x00;

	value = Sensor_ReadReg(0x205);/*0-7*/
	gain = value&0xff;
	value = Sensor_ReadReg(0x204);/*8*/
	gain |= (value<<0x08)&0xff00;

	s_s5k3h2yx_gain=gain;

	if (gain_ptr) {
		*gain_ptr = gain;
	}

	SENSOR_PRINT("SENSOR_s5k3h2yx: _ReadGain gain: 0x%x", s_s5k3h2yx_gain);

	return rtn;
}



LOCAL uint32_t _s5k3h2yx_BeforeSnapshot(uint32_t param)
{
	uint16_t ret;
	uint32_t cap_mode = (param>>CAP_MODE_BITS);
	uint32_t capture_exposure;
	uint32_t preview_exposure;
	uint32_t prv_linetime=s_s5k3h2yx_Resolution_Trim_Tab[SENSOR_MODE_PREVIEW_ONE].line_time;
	uint32_t cap_linetime;
	uint32_t frame_len = 0x00;
	uint32_t gain = 0;
	uint8_t ret_l, ret_m, ret_h;

	param = param&0xffff;
	SENSOR_PRINT("SENSOR_s5k3h2yx:cap_mode = %d,param = %d.",cap_mode,param);
	cap_linetime = s_s5k3h2yx_Resolution_Trim_Tab[param].line_time;


	SENSOR_PRINT("SENSOR_s5k3h2yx: BeforeSnapshot moe: %d",param);

	if (SENSOR_MODE_PREVIEW_ONE >= param){
		SENSOR_PRINT("SENSOR_s5k3h2yx: prvmode equal to capmode");
		return SENSOR_SUCCESS;
	}

	ret_h = (uint8_t) Sensor_ReadReg(0x202);
	ret_l = (uint8_t) Sensor_ReadReg(0x203);
	preview_exposure = (ret_h << 8) + (ret_l);

	_s5k3h2yx_ReadGain(&gain);

	Sensor_SetMode(param);
	Sensor_SetMode_WaitDone();

	capture_exposure = preview_exposure * prv_linetime / cap_linetime;

	ret_h = (uint8_t) Sensor_ReadReg(0x340);
	ret_l = (uint8_t) Sensor_ReadReg(0x341);
	frame_len = (ret_h << 8) + (ret_l);
	
	while(gain >= 0x40){
		capture_exposure = capture_exposure * 2;
		gain=gain / 2;
		if(capture_exposure > frame_len*2)
			break;
	}
	SENSOR_PRINT("SENSOR_s5k3h2yx: cap moe: %d,FL: %x,exp=%d,g=%x",param,frame_len,capture_exposure,gain);

	if(capture_exposure >= (frame_len - 8)){
		frame_len = capture_exposure+8;
		frame_len = ((frame_len+1)>>1)<<1;
		Sensor_WriteReg(0x0341, frame_len & 0xff);
		Sensor_WriteReg(0x0340, (frame_len >> 0x08) & 0xff);
	}
	Sensor_WriteReg(0x203, capture_exposure & 0xff);
	Sensor_WriteReg(0x202, (capture_exposure >> 0x08) & 0xff);
	Sensor_WriteReg(0x204, gain>>0x08);
	Sensor_WriteReg(0x205, gain&0xff);

	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, capture_exposure);

	return SENSOR_SUCCESS;
}

LOCAL uint32_t _s5k3h2yx_after_snapshot(uint32_t param)
{
	SENSOR_PRINT("SENSOR_s5k3h2yx: after_snapshot mode:%d", param);
	Sensor_SetMode(param);
	return SENSOR_SUCCESS;
}

LOCAL uint32_t _s5k3h2yx_StreamOn(uint32_t param)
{
	SENSOR_PRINT("SENSOR_s5k3h2yx: StreamOn");

	Sensor_WriteReg(0x0100, 0x01);

	return 0;
}

LOCAL uint32_t _s5k3h2yx_StreamOff(uint32_t param)
{
	SENSOR_PRINT("SENSOR_s5k3h2yx: StreamOff");

	Sensor_WriteReg(0x0100, 0x00);
	usleep(40*1000);

	return 0;
}

LOCAL uint32_t _dw9174_SRCInit(uint32_t mode)
{
	uint8_t cmd_val[6] = {0x00};
	uint16_t  slave_addr = 0;
	uint16_t cmd_len = 0;
	uint32_t ret_value = SENSOR_SUCCESS;	
	
	slave_addr = DW9714_VCM_SLAVE_ADDR;
	
	switch (mode) {
		case 1:
		break;
		
		case 2:
		{			
			cmd_val[0] = 0xec;
			cmd_val[1] = 0xa3;
			cmd_len = 2;
			Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);			
			usleep(20*1000);
			cmd_val[0] = 0xf2;
			cmd_val[1] = 0x00;
			cmd_len = 2;
			Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);			
			usleep(20*1000);
			cmd_val[0] = 0xdc;
			cmd_val[1] = 0x51;			
			cmd_len = 2;
			Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);			
		}
		break;
		
		case 3:
		break;
		
	}

	return ret_value;
}
