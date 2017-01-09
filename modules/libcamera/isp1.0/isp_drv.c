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
#include <sys/types.h>
#include "isp_reg.h"
#include "isp_com.h"
/**---------------------------------------------------------------------------*
**				Compiler Flag					**
**---------------------------------------------------------------------------*/
#ifdef	 __cplusplus
extern	 "C"
{
#endif
/**---------------------------------------------------------------------------*
*				Micro Define					**
**----------------------------------------------------------------------------*/
typedef void (*ISP_ISR_HANDLER)(void);

#define ISP_MAX_WIDTH 3264
#define ISP_MAX_HEIGHT 2448

#define ISP_MAX_SLICE_WIDTH 1632
#define ISP_MAX_SLICE_HEIGHT 2448

#define ISP_AWB_DEFAULT_GAIN 0x100

#define ISP_MAX_WIDTH_V0001 3280
#define ISP_MAX_HEIGHT_V0001 2464

#define ISP_MAX_SLICE_WIDTH_V0001 3280
#define ISP_MAX_SLICE_HEIGHT_V0001 2464

#define ISP_AWB_DEFAULT_GAIN_V0001 0x400

#define ISP_WB_WIN_NUM 32

/**---------------------------------------------------------------------------*
**				Data Structures					**
**---------------------------------------------------------------------------*/
struct isp_reg{

	uint32_t ISP_ID;

	/* fetch reg */
	uint32_t FETCH_STATUS;
	uint32_t FETCH_STATUS_PREVIEW_V0001;
	uint32_t FETCH_PARAM;
	uint32_t FETCH_SLICE_SIZE;
	uint32_t FETCH_SLICE_Y_ADDR;
	uint32_t FETCH_SLICE_Y_PITCH;
	uint32_t FETCH_SLICE_U_ADDR;
	uint32_t FETCH_SLICE_U_PITCH;
	uint32_t FETCH_MIPI_WORD_INFO;
	uint32_t FETCH_MIPI_BYTE_INFO;
	uint32_t FETCH_SLICE_V_ADDR;
	uint32_t FETCH_SLICE_V_PITCH;
	uint32_t FETCH_PREVIEW_CNT;

	/* BLC&NLC */
	uint32_t BNLC_STATUS;
	uint32_t BNLC_PARAM;
	uint32_t BNLC_B_PARAM_R_B;
	uint32_t BNLC_B_PARAM_G;
	uint32_t BNLC_N_PARAM_R0;
	uint32_t BNLC_N_PARAM_R1;
	uint32_t BNLC_N_PARAM_R2;
	uint32_t BNLC_N_PARAM_R3;
	uint32_t BNLC_N_PARAM_R4;
	uint32_t BNLC_N_PARAM_R5;
	uint32_t BNLC_N_PARAM_R6;
	uint32_t BNLC_N_PARAM_R7;
	uint32_t BNLC_N_PARAM_R8;
	uint32_t BNLC_N_PARAM_R9;
	uint32_t BNLC_N_PARAM_G0;
	uint32_t BNLC_N_PARAM_G1;
	uint32_t BNLC_N_PARAM_G2;
	uint32_t BNLC_N_PARAM_G3;
	uint32_t BNLC_N_PARAM_G4;
	uint32_t BNLC_N_PARAM_G5;
	uint32_t BNLC_N_PARAM_G6;
	uint32_t BNLC_N_PARAM_G7;
	uint32_t BNLC_N_PARAM_G8;
	uint32_t BNLC_N_PARAM_G9;
	uint32_t BNLC_N_PARAM_B0;
	uint32_t BNLC_N_PARAM_B1;
	uint32_t BNLC_N_PARAM_B2;
	uint32_t BNLC_N_PARAM_B3;
	uint32_t BNLC_N_PARAM_B4;
	uint32_t BNLC_N_PARAM_B5;
	uint32_t BNLC_N_PARAM_B6;
	uint32_t BNLC_N_PARAM_B7;
	uint32_t BNLC_N_PARAM_B8;
	uint32_t BNLC_N_PARAM_B9;
	uint32_t BNLC_N_PARAM_L0;
	uint32_t BNLC_N_PARAM_L1;
	uint32_t BNLC_N_PARAM_L2;
	uint32_t BNLC_N_PARAM_L3;
	uint32_t BNLC_N_PARAM_L4;
	uint32_t BNLC_N_PARAM_L5;
	uint32_t BNLC_N_PARAM_L6;
	uint32_t BNLC_N_PARAM_L7;
	uint32_t BNLC_N_PARAM_L8;
	uint32_t BNLC_SLICE_SIZE;
	uint32_t BNLC_SLICE_INFO;

	/* NLC */
	uint32_t NLC_STATUS;
	uint32_t NLC_PARAM;

	/* Lens C */
	uint32_t LENS_STATUS;
	uint32_t LENS_PARAM;
	unsigned long LENS_PARAM_ADDR;
	uint32_t LENS_SLICE_POS;
	uint32_t LENS_LOADER_EB;
	uint32_t LENS_GRID_PITCH;
	uint32_t LENS_GRID_SIZE;
	uint32_t LENS_LOAD_BUF;
	uint32_t LENS_MISC;
	uint32_t LENS_SLICE_SIZE;

	/* AWBM */
	uint32_t AWBM_STATUS;
	uint32_t AWBM_PARAM;
	uint32_t AWBM_OFFSET;
	uint32_t AWBM_BLK_SIZE;

	/* AWBC */
	uint32_t AWBC_STATUS;
	uint32_t AWBC_PARAM;
	uint32_t AWBC_GAIN0;
	uint32_t AWBC_GAIN1;
	uint32_t AWBC_THRD;
	uint32_t AWBC_OFFSET0;
	uint32_t AWBC_OFFSET1;

	/* BPC */
	uint32_t BPC_STATUS;
	uint32_t BPC_PARAM;
	uint32_t BPC_THRD;
	uint32_t BPC_MAP_ADDR;
	uint32_t BPC_PIXEL_NUM;

	/* NBPC */
	uint32_t NBPC_STATUS;
	uint32_t NBPC_PARAM;
	uint32_t NBPC_CFG;
	uint32_t NBPC_PIXEL_NUM;
	uint32_t NBPC_FACTOR;
	uint32_t NBPC_COEFF;
	uint32_t NBPC_LUTWORD0;
	uint32_t NBPC_LUTWORD1;
	uint32_t NBPC_LUTWORD2;
	uint32_t NBPC_LUTWORD3;
	uint32_t NBPC_LUTWORD4;
	uint32_t NBPC_LUTWORD5;
	uint32_t NBPC_LUTWORD6;
	uint32_t NBPC_LUTWORD7;
	uint32_t NBPC_NEW_OLD_SEL;
	uint32_t NBPC_MAP_ADDR;
	/* wave denoise */
	uint32_t WAVE_STATUS;
	uint32_t WAVE_PARAM;
	uint32_t WAVE_THRD;
	uint32_t WAVE_SLICE_SIZE;
	uint32_t WAVE_SLICE_INFO;
	uint32_t WAVE_DISWEI_0;
	uint32_t WAVE_DISWEI_1;
	uint32_t WAVE_DISWEI_2;
	uint32_t WAVE_DISWEI_3;
	uint32_t WAVE_DISWEI_4;
	uint32_t WAVE_RANWEI_0;
	uint32_t WAVE_RANWEI_1;
	uint32_t WAVE_RANWEI_2;
	uint32_t WAVE_RANWEI_3;
	uint32_t WAVE_RANWEI_4;
	uint32_t WAVE_RANWEI_5;
	uint32_t WAVE_RANWEI_6;
	uint32_t WAVE_RANWEI_7;

	/* GrGb C */
	uint32_t GRGB_STATUS;
	uint32_t GRGB_PARAM;

	/* CFA */
	uint32_t CFA_STATUS;
	uint32_t CFA_PARAM;
	uint32_t CFA_SLICE_SIZE;
	uint32_t CFA_SLICE_INFO;

	/* CMC */
	uint32_t CMC_STATUS;
	uint32_t CMC_PARAM;
	uint32_t CMC_MATRIX0;
	uint32_t CMC_MATRIX1;
	uint32_t CMC_MATRIX2;
	uint32_t CMC_MATRIX3;
	uint32_t CMC_MATRIX4;

	/* Gamma Correction */
	uint32_t GAMMA_STATUS;
	uint32_t GAMMA_PARAM;
	uint32_t GAMMA_NODE_X0;
	uint32_t GAMMA_NODE_X1;
	uint32_t GAMMA_NODE_X2;
	uint32_t GAMMA_NODE_X3;
	uint32_t GAMMA_NODE_X4;
	uint32_t GAMMA_NODE_X5;
	uint32_t GAMMA_NODE_X6;
	uint32_t GAMMA_NODE_X7;

	uint32_t GAMMA_NODE_Y0;
	uint32_t GAMMA_NODE_Y1;
	uint32_t GAMMA_NODE_Y2;
	uint32_t GAMMA_NODE_Y3;
	uint32_t GAMMA_NODE_Y4;
	uint32_t GAMMA_NODE_Y5;
	uint32_t GAMMA_NODE_Y6;

	uint32_t GAMMA_NODE_R0;
	uint32_t GAMMA_NODE_R1;
	uint32_t GAMMA_NODE_R2;
	uint32_t GAMMA_NODE_R3;
	uint32_t GAMMA_NODE_R4;
	uint32_t GAMMA_NODE_R5;
	uint32_t GAMMA_NODE_R6;
	uint32_t GAMMA_NODE_G0;
	uint32_t GAMMA_NODE_G1;
	uint32_t GAMMA_NODE_G2;
	uint32_t GAMMA_NODE_G3;
	uint32_t GAMMA_NODE_G4;
	uint32_t GAMMA_NODE_G5;
	uint32_t GAMMA_NODE_G6;

	uint32_t GAMMA_NODE_B0;
	uint32_t GAMMA_NODE_B1;
	uint32_t GAMMA_NODE_B2;
	uint32_t GAMMA_NODE_B3;
	uint32_t GAMMA_NODE_B4;
	uint32_t GAMMA_NODE_B5;
	uint32_t GAMMA_NODE_B6;

	uint32_t GAMMA_NODE_IDX0_V0001;
	uint32_t GAMMA_NODE_IDX1_V0001;
	uint32_t GAMMA_NODE_IDX2_V0001;
	uint32_t GAMMA_NODE_IDX3_V0001;

	/* CCE */
	uint32_t CCE_STATUS;
	uint32_t CCE_PARAM;
	uint32_t CCE_MATRIX0;
	uint32_t CCE_MATRIX1;
	uint32_t CCE_MATRIX2;
	uint32_t CCE_MATRIX3;
	uint32_t CCE_MATRIX4;
	uint32_t CCE_SHIFT;
	uint32_t CCE_UVD_THRD0;
	uint32_t CCE_UVD_THRD1;
	uint32_t CCE_UVD_PARAM0_V0001;
	uint32_t CCE_UVD_PARAM1_V0001;

	/* PREF */
	uint32_t PREF_STATUS;
	uint32_t PREF_PARAM;
	uint32_t PREF_THRD;
	uint32_t PREF_SLICE_SIZE;
	uint32_t PREF_SLICE_INFO;

	/* BRIGHT */
	uint32_t BRIGHT_STATUS;
	uint32_t BRIGHT_PARAM;
	uint32_t BRIGHT_SLICE_SIZE;
	uint32_t BRIGHT_SLICE_INFO;

	/* CONTRAST */
	uint32_t CONTRAST_STATUS;
	uint32_t CONTRAST_PARAM;

	/* HIST */
	uint32_t HIST_STATUS;
	uint32_t HIST_PARAM;
	uint32_t HIST_RATION;
	uint32_t HIST_MAX_MIN;
	uint32_t HIST_CLEAR_ENABLE;

	/* AUTO CONTRAST */
	uint32_t AUTO_CONTRAST_STATUS;
	uint32_t AUTO_CONTRAST_PARAM;
	uint32_t AUTO_CONTRAST_MAX_MIN;

	/* AFM */
	uint32_t AFM_STATUS;
	uint32_t AFM_PARAM;
	uint32_t AFM_WIN_RANGE0;
	uint32_t AFM_WIN_RANGE1;
	uint32_t AFM_WIN_RANGE2;
	uint32_t AFM_WIN_RANGE3;
	uint32_t AFM_WIN_RANGE4;
	uint32_t AFM_WIN_RANGE5;
	uint32_t AFM_WIN_RANGE6;
	uint32_t AFM_WIN_RANGE7;
	uint32_t AFM_WIN_RANGE8;
	uint32_t AFM_WIN_RANGE9;
	uint32_t AFM_WIN_RANGE10;
	uint32_t AFM_WIN_RANGE11;
	uint32_t AFM_WIN_RANGE12;
	uint32_t AFM_WIN_RANGE13;
	uint32_t AFM_WIN_RANGE14;
	uint32_t AFM_WIN_RANGE15;
	uint32_t AFM_WIN_RANGE16;
	uint32_t AFM_WIN_RANGE17;
	uint32_t AFM_STATISTIC0;
	uint32_t AFM_STATISTIC1;
	uint32_t AFM_STATISTIC2;
	uint32_t AFM_STATISTIC3;
	uint32_t AFM_STATISTIC4;
	uint32_t AFM_STATISTIC5;
	uint32_t AFM_STATISTIC6;
	uint32_t AFM_STATISTIC7;
	uint32_t AFM_STATISTIC8;

	uint32_t AFM_STATISTIC0_L_V0001;
	uint32_t AFM_STATISTIC0_H_V0001;
	uint32_t AFM_STATISTIC1_L_V0001;
	uint32_t AFM_STATISTIC1_H_V0001;
	uint32_t AFM_STATISTIC2_L_V0001;
	uint32_t AFM_STATISTIC2_H_V0001;
	uint32_t AFM_STATISTIC3_L_V0001;
	uint32_t AFM_STATISTIC3_H_V0001;
	uint32_t AFM_STATISTIC4_L_V0001;
	uint32_t AFM_STATISTIC4_H_V0001;
	uint32_t AFM_STATISTIC5_L_V0001;
	uint32_t AFM_STATISTIC5_H_V0001;
	uint32_t AFM_STATISTIC6_L_V0001;
	uint32_t AFM_STATISTIC6_H_V0001;
	uint32_t AFM_STATISTIC7_L_V0001;
	uint32_t AFM_STATISTIC7_H_V0001;
	uint32_t AFM_STATISTIC8_L_V0001;
	uint32_t AFM_STATISTIC8_H_V0001;

	/* EE */
	uint32_t EE_STATUS;
	uint32_t EE_PARAM;

	/* EMBOSS */
	uint32_t EMBOSS_STATUS;
	uint32_t EMBOSS_PARAM;

	/* FCS */
	uint32_t FCS_STATUS;
	uint32_t FCS_PARAM;

	/* CSS */
	uint32_t CSS_STATUS;
	uint32_t CSS_PARAM;
	uint32_t CSS_THRD0;
	uint32_t CSS_THRD1;
	uint32_t CSS_THRD2;
	uint32_t CSS_THRD3;
	uint32_t CSS_SLICE_SIZE;
	uint32_t CSS_RATIO;

	/* CSA */
	uint32_t CSA_STATUS;
	uint32_t CSA_PARAM;

	/* store */
	uint32_t STORE_STATUS;
	uint32_t STORE_STATUS_PREVIEW;
	uint32_t STORE_PARAM;
	uint32_t STORE_SLICE_SIZE;
	uint32_t STORE_SLICE_Y_ADDR;
	uint32_t STORE_SLICE_Y_PITCH;
	uint32_t STORE_SLICE_U_ADDR;
	uint32_t STORE_SLICE_U_PITCH;
	uint32_t STORE_SLICE_V_ADDR;
	uint32_t STORE_SLICE_V_PITCH;
	uint32_t STORE_INT_CTRL;

	/* feeder */
	uint32_t FEEDER_STATUS;
	uint32_t FEEDER_PARAM;
	uint32_t FEEDER_SLICE_SIZE;

	/* Arbiter */
	uint32_t ARBITER_STATUS;
	uint32_t ARBITER_AXI_WR_MASTER_STATUS_V0001;
	uint32_t ARBITER_AXI_RD_MASTER_STATUS_V0001;
	uint32_t ARBITER_PRERST;
	uint32_t ARBITER_PAUSE_CYCLE;
	uint32_t ARBITER_AXI_MASTER_CTRL_V0001;

	/* hdr */
	uint32_t HDR_STATUS;
	uint32_t HDR_PARAM;
	uint32_t HDR_INDEX;

	/* common */
	uint32_t COM_STATUS;
	uint32_t COM_START;
	uint32_t COM_PARAM;
	uint32_t COM_BURST_SIZE;
	uint32_t COM_MEM_SWITCH_V0000;
	uint32_t COM_SHADOW;
	uint32_t COM_BAYER_MODE;
	uint32_t COM_SHADOW_ALL;
	uint32_t COM_PMU_RAM_MASK;
	uint32_t COM_HW_MASK;
	uint32_t COM_HW_EB;
	uint32_t COM_SW_SWITCH;
	uint32_t COM_SW_TOGGLE;
	uint32_t COM_PREVIEW_STOP;
	uint32_t COM_SHADOW_CNT;
	uint32_t COM_AXI_STOP;
	uint32_t COM_SLICE_CNT;
	uint32_t COM_PERFORM_CNT_R;
	uint32_t COM_PERFORM_CNT;
	uint32_t COM_INT_EB;
	uint32_t COM_INT_CLR;
	uint32_t COM_INT_RAW;
	uint32_t COM_INT;

	/* GAIN GLB */
	uint32_t GLB_GAIN_STATUS;
	uint32_t GLB_GAIN_PARAM;
	uint32_t GLB_GAIN_SLICE_SIZE;

	/* GAIN RGB */
	uint32_t RGB_GAIN_STATUS;
	uint32_t RGB_GAIN_PARAM;
	uint32_t RGB_GAIN_OFFSET;

	//YIQ
	uint32_t YIQ_STATUS_V0001;
	uint32_t YIQ_PARAM_V0001;
	uint32_t YGAMMA_X0_V0001;
	uint32_t YGAMMA_X1_V0001;
	uint32_t YGAMMA_Y0_V0001;
	uint32_t YGAMMA_Y1_V0001;
	uint32_t YGAMMA_Y2_V0001;
	uint32_t AF_V_HEIGHT_V0001;
	uint32_t AF_LINE_COUNTER_V0001;
	uint32_t AF_LINE_STEP_V0001;
	uint32_t YGAMMA_NODE_IDX_V0001;
	uint32_t AF_LINE_START_V0001;

	//HUE
	uint32_t HUE_STATUS_V0001;
	uint32_t HUE_PARAM_V0001;

	/*PRE GAIN GLB */
	//uint32_t PRE_GLB_GAIN_PARAM;
	//Pre Wavelet
	uint32_t PRE_WAVE_STATUS;
	uint32_t PRE_WAVE_PARAM;
};

struct isp_drv_context{
	uint32_t ISP_ID;
	struct isp_reg reg_tab[2];
};

/**---------------------------------------------------------------------------*
 ** 				extend Variables and function			*
 **---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
 ** 				Local Variables					*
 **---------------------------------------------------------------------------*/
static char s_isp_dev_name[50] = "/dev/sprd_isp";
static int	s_isp_fd = -1;

#define ISP_CHECK_FD							\
		do {							\
			if (-1 == s_isp_fd) {				\
				ISP_LOG("device not opened error");		\
				return -1;				\
			}						\
		} while(0)

static ISP_ISR_HANDLER s_isp_isr[12];

static struct isp_drv_context s_isp_drv_context;
static struct isp_drv_context* s_isp_drv_context_ptr=&s_isp_drv_context;

static struct isp_reg* s_isp_reg_tab[2];

//static struct isp_reg s_isp_reg;
//static struct isp_reg* isp_reg_ptr=&s_isp_reg;
/**---------------------------------------------------------------------------*
 ** 				Constant Variables					*
 **---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
 ** 				Local Function Prototypes				*
 **---------------------------------------------------------------------------*/

static int32_t _isp_write(uint32_t *param)
{
#ifdef ISP_USER_DRV_DEBUG
	struct isp_reg_param *reg_param_ptr = (struct isp_reg_param *)param;
	uint32_t i = 0;
	struct isp_reg_bits *ptr = (struct isp_reg_bits*)reg_param_ptr->reg_param;

	for(i=0 ; i<reg_param_ptr->counts ; i++) {
		ptr++;
	}
#endif
	if (-1 == ioctl(s_isp_fd, ISP_IO_WRITE, param)) {
		return ISP_ERROR;
	} else {
		//ISP_LOG("OK to ISP_IO_WRITE");
	}
	return ISP_SUCCESS;
}

static int32_t _isp_read(uint32_t *param)
{
	int32_t ret = ISP_SUCCESS;

	if (-1 == ioctl(s_isp_fd, ISP_IO_READ, param)) {
			ret = -1;
	} else {
		//ISP_LOG("OK to ISP_IO_WRITE");
#ifdef ISP_USER_DRV_DEBUG
		{
		struct isp_reg_param *reg_param_ptr = (struct isp_reg_param *)param;
		uint32_t i = 0;
		struct isp_reg_bits *ptr = (struct isp_reg_bits*)reg_param_ptr->reg_param;

		for(i=0 ; i<reg_param_ptr->counts ; i++) {
			ISP_LOG("ISP read: addr 0x%x,value 0x%x.",ptr->reg_addr,ptr->reg_value);
			ptr++;
		}
		}
#endif
	}
	return ret;
}

/*	--
*@
*@
*/
static struct isp_drv_context*_isp_GetContext(void)
{
	return s_isp_drv_context_ptr;
}

/*	--
*@
*@
*/
static struct isp_reg*_isp_GetRegPtr(uint32_t handler_id)
{
	return s_isp_reg_tab[handler_id];
}

/*	--
*@
*@
*/
static uint32_t _isp_GetIspId(void)
{
	struct isp_drv_context* isp_context_ptr = _isp_GetContext();

	return isp_context_ptr->ISP_ID;
}

/*	--
*@
*@
*/
static int32_t _isp_SetIspId(uint32_t isp_id)
{
	struct isp_drv_context* isp_context_ptr = _isp_GetContext();

	isp_context_ptr->ISP_ID=isp_id;

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispRegInit(uint32_t handler_id, uint32_t isp_id)
{
	struct isp_drv_context* isp_context_ptr = _isp_GetContext();

	memset((void*)isp_context_ptr, ISP_ZERO, sizeof(struct isp_drv_context));

	_isp_SetIspId(isp_id);

	s_isp_reg_tab[0]=&isp_context_ptr->reg_tab[0];
	s_isp_reg_tab[1]=&isp_context_ptr->reg_tab[1];

	return ISP_SUCCESS;
}

/* FETCH */
/*	--
*@
*@
*/
int32_t ispGetFetchStatus(uint32_t handler_id, uint32_t* status)
{
/*	union _isp_fetch_status_tag* reg_ptr=(union _isp_fetch_status_tag*)ISP_FETCH_STATUS;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_fetch_status_tag* reg_s_ptr=(union _isp_fetch_status_tag*)&isp_reg_ptr->FETCH_STATUS;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;

	ISP_CHECK_FD;

/*	reg_s_ptr->dwValue=reg_ptr->dwValue;
	*status=reg_s_ptr->dwValue;
*/
	reg_config.reg_addr = ISP_FETCH_STATUS - ISP_BASE_ADDR;
	read_param.reg_param = (unsigned long)&reg_config;
	read_param.counts = 1;

	if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		reg_s_ptr->dwValue = reg_config.reg_value;
		*status = reg_config.reg_value;
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispGetFetchPreviewStatus(uint32_t handler_id, uint32_t* status)
{
	uint32_t isp_id = _isp_GetIspId();
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);

	if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{

	/*	union _isp_fetch_status_preview_v0001_tag* reg_ptr=(union _isp_fetch_status_preview_v0001_tag*)FETCH_STATUS_PREVIEW_V0001;*/
		union _isp_fetch_status_preview_v0001_tag* reg_s_ptr=(union _isp_fetch_status_preview_v0001_tag*)&isp_reg_ptr->FETCH_STATUS_PREVIEW_V0001;
		struct isp_reg_bits reg_config;
		struct isp_reg_param read_param;

		ISP_CHECK_FD;

	/*	reg_s_ptr->dwValue=reg_ptr->dwValue;
		*status=reg_s_ptr->dwValue;
	*/
		reg_config.reg_addr = ISP_FETCH_STATUS_PREVIEW_V0001 - ISP_BASE_ADDR;
		read_param.reg_param = (unsigned long)&reg_config;
		read_param.counts = 1;

		if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
			reg_s_ptr->dwValue = reg_config.reg_value;
			*status = reg_config.reg_value;
		}
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispFetchBypass(uint32_t handler_id, uint8_t bypass)
{
/*	union _isp_fetch_param_tag* reg_ptr=(union _isp_fetch_param_tag*)ISP_FETCH_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_fetch_param_tag* reg_s_ptr=(union _isp_fetch_param_tag*)&isp_reg_ptr->FETCH_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.bypass=bypass;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_FETCH_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->FETCH_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispFetchSubtract(uint32_t handler_id, uint8_t eb)
{
/*	union _isp_fetch_param_tag* reg_ptr=(union _isp_fetch_param_tag*)ISP_FETCH_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_fetch_param_tag* reg_s_ptr=(union _isp_fetch_param_tag*)&isp_reg_ptr->FETCH_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.sub_stract=eb;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_FETCH_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->FETCH_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispAfifoWrMode(uint32_t handler_id, uint8_t mode)
{
/*	union _isp_fetch_param_tag* reg_ptr=(union _isp_fetch_param_tag*)ISP_FETCH_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_fetch_param_tag* reg_s_ptr=(union _isp_fetch_param_tag*)&isp_reg_ptr->FETCH_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_s_ptr->mBits.afifo_wr_mode=mode;
		/*reg_ptr->dwValue=reg_s_ptr->dwValue;*/
		reg_config.reg_addr = ISP_FETCH_PARAM - ISP_BASE_ADDR;
		reg_config.reg_value = isp_reg_ptr->FETCH_PARAM;
		write_param.reg_param = (unsigned long)&reg_config;
		write_param.counts = 1;

		return _isp_write((uint32_t *)&write_param);
	}
	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispSetFetchSliceSize(uint32_t handler_id, uint16_t w, uint16_t h)
{
/*	union _isp_fetch_slice_size_tag* reg_ptr=(union _isp_fetch_slice_size_tag*)ISP_FETCH_SLICE_SIZE;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_fetch_slice_size_tag* reg_s_ptr=(union _isp_fetch_slice_size_tag*)&isp_reg_ptr->FETCH_SLICE_SIZE;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.slice_with=w;
	reg_s_ptr->mBits.slice_height=h;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_FETCH_SLICE_SIZE - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->FETCH_SLICE_SIZE;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetFetchYAddr(uint32_t handler_id, uint32_t y_addr)
{
/*	union _isp_fetch_slice_y_addr_tag* reg_ptr=(union _isp_fetch_slice_y_addr_tag*)ISP_FETCH_SLICE_Y_ADDR;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_fetch_slice_y_addr_tag* reg_s_ptr=(union _isp_fetch_slice_y_addr_tag*)&isp_reg_ptr->FETCH_SLICE_Y_ADDR;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.y_addr=y_addr;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_FETCH_SLICE_Y_ADDR - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->FETCH_SLICE_Y_ADDR;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetFetchYPitch(uint32_t handler_id, uint32_t y_pitch)
{
/*	union _isp_fetch_slice_y_pitch_tag* reg_ptr=(union _isp_fetch_slice_y_pitch_tag*)ISP_FETCH_Y_PITCH;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_fetch_slice_y_pitch_tag* reg_s_ptr=(union _isp_fetch_slice_y_pitch_tag*)&isp_reg_ptr->FETCH_SLICE_Y_PITCH;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.y_pitch=y_pitch;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_FETCH_Y_PITCH - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->FETCH_SLICE_Y_PITCH;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetFetchUAddr(uint32_t handler_id, uint32_t u_addr)
{
/*	union _isp_fetch_slice_u_addr_tag* reg_ptr=(union _isp_fetch_slice_u_addr_tag*)ISP_FETCH_SLICE_U_ADDR;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_fetch_slice_u_addr_tag* reg_s_ptr=(union _isp_fetch_slice_u_addr_tag*)&isp_reg_ptr->FETCH_SLICE_U_ADDR;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.u_addr=u_addr;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_FETCH_SLICE_U_ADDR - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->FETCH_SLICE_U_ADDR;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetFetchUPitch(uint32_t handler_id, uint32_t u_pitch)
{
/*	union _isp_fetch_slice_u_pitch_tag* reg_ptr=(union _isp_fetch_slice_u_pitch_tag*)ISP_FETCH_U_PITCH;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_fetch_slice_u_pitch_tag* reg_s_ptr=(union _isp_fetch_slice_u_pitch_tag*)&isp_reg_ptr->FETCH_SLICE_U_PITCH;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.u_pitch=u_pitch;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_FETCH_U_PITCH - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->FETCH_SLICE_U_PITCH;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetFetchMipiWordInfo(uint32_t handler_id, uint16_t word_num)
{
/*	union _isp_fetch_mipi_word_info_tag* reg_ptr=(union _isp_fetch_mipi_word_info_tag*)ISP_FETCH_MIPI_WORD_INFO;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_fetch_mipi_word_info_tag* reg_s_ptr=(union _isp_fetch_mipi_word_info_tag*)&isp_reg_ptr->FETCH_MIPI_WORD_INFO;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.word_num=word_num;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_FETCH_MIPI_WORD_INFO - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->FETCH_MIPI_WORD_INFO;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetFetchMipiByteInfo(uint32_t handler_id, uint16_t byte_rel_pos)
{
/*	union _isp_fetch_mipi_byte_info_tag* reg_ptr=(union _isp_fetch_mipi_byte_info_tag*)ISP_FETCH_MIPI_BYTE_INFO;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_fetch_mipi_byte_info_tag* reg_s_ptr=(union _isp_fetch_mipi_byte_info_tag*)&isp_reg_ptr->FETCH_MIPI_BYTE_INFO;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.byte_rel_pos=byte_rel_pos;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_FETCH_MIPI_BYTE_INFO - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->FETCH_MIPI_BYTE_INFO;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetFetchVAddr(uint32_t handler_id, uint32_t v_addr)
{
/*	union _isp_fetch_slice_v_addr_tag* reg_ptr=(union _isp_fetch_slice_v_addr_tag*)ISP_FETCH_SLICE_V_ADDR;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_fetch_slice_v_addr_tag* reg_s_ptr=(union _isp_fetch_slice_v_addr_tag*)&isp_reg_ptr->FETCH_SLICE_V_ADDR;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.v_addr=v_addr;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_FETCH_SLICE_V_ADDR - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->FETCH_SLICE_V_ADDR;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetFetchVPitch(uint32_t handler_id, uint32_t v_pitch)
{
/*	union _isp_fetch_slice_v_pitch_tag* reg_ptr=(union _isp_fetch_slice_v_pitch_tag*)ISP_FETCH_V_PITCH;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_fetch_slice_v_pitch_tag* reg_s_ptr=(union _isp_fetch_slice_v_pitch_tag*)&isp_reg_ptr->FETCH_SLICE_V_PITCH;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.v_pitch=v_pitch;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_FETCH_V_PITCH - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->FETCH_SLICE_V_PITCH;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*BNLC */
/*	--
*@
*@
*/
int32_t ispGetBNLCStatus(uint32_t handler_id, uint32_t* status)
{
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_bnlc_status_tag* reg_s_ptr=(union _isp_bnlc_status_tag*)&isp_reg_ptr->BNLC_STATUS;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;

	ISP_CHECK_FD;

	reg_config.reg_addr = ISP_BNLC_STATUS - ISP_BASE_ADDR;
	read_param.reg_param = (unsigned long)&reg_config;
	read_param.counts = 1;
/*
	reg_s_ptr->dwValue=reg_ptr->dwValue;
	*status=reg_s_ptr->dwValue;
*/
	if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		reg_s_ptr->dwValue = reg_config.reg_value;
		*status = reg_config.reg_value;
	}
	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispBlcBypass(uint32_t handler_id, uint8_t bypass)
{
/*	union _isp_bnlc_param_tag* reg_ptr=(union _isp_bnlc_param_tag*)ISP_BNLC_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_bnlc_param_tag* reg_s_ptr=(union _isp_bnlc_param_tag*)&isp_reg_ptr->BNLC_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.blc_bypass=bypass;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_BNLC_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->BNLC_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetBlcMode(uint32_t handler_id, uint8_t mode)
{
/*	union _isp_bnlc_param_tag* reg_ptr=(union _isp_bnlc_param_tag*)ISP_BNLC_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_bnlc_param_tag* reg_s_ptr=(union _isp_bnlc_param_tag*)&isp_reg_ptr->BNLC_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.blc_mode=mode;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_BNLC_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->BNLC_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetBlcCalibration(uint32_t handler_id, uint16_t r, uint16_t b, uint16_t gr, uint16_t gb)
{
/*	union _isp_bnlc_b_param_r_b_tag* reg0_ptr=(union _isp_bnlc_b_param_r_b_tag*)ISP_BNLC_B_PARAM_R_B;
	union _isp_bnlc_b_param_g_tag* reg1_ptr=(union _isp_bnlc_b_param_g_tag*)ISP_BNLC_B_PARAM_G;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_bnlc_b_param_r_b_tag* reg0_s_ptr=(union _isp_bnlc_b_param_r_b_tag*)&isp_reg_ptr->BNLC_B_PARAM_R_B;
	union _isp_bnlc_b_param_g_tag* reg1_s_ptr=(union _isp_bnlc_b_param_g_tag*)&isp_reg_ptr->BNLC_B_PARAM_G;
	struct isp_reg_bits reg_config[2];
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg0_s_ptr->mBits.blc_r=r;
	reg0_s_ptr->mBits.blc_b=b;
	reg1_s_ptr->mBits.blc_gr=gr;
	reg1_s_ptr->mBits.blc_gb=gb;
/*	reg0_ptr->dwValue=reg0_s_ptr->dwValue;
	reg1_ptr->dwValue=reg1_s_ptr->dwValue;*/
	reg_config[0].reg_addr = ISP_BNLC_B_PARAM_R_B - ISP_BASE_ADDR;
	reg_config[0].reg_value = isp_reg_ptr->BNLC_B_PARAM_R_B;
	reg_config[1].reg_addr = ISP_BNLC_B_PARAM_G - ISP_BASE_ADDR;
	reg_config[1].reg_value = isp_reg_ptr->BNLC_B_PARAM_G;
	write_param.reg_param = (unsigned long)&reg_config[0];
	write_param.counts = 2;

	return _isp_write((uint32_t *)&write_param);
}

/*NLC */
/*	--
*@
*@
*/
int32_t ispGetNLCStatus(uint32_t handler_id, uint32_t* status)
{
	uint32_t isp_id=_isp_GetIspId();
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);

	if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
		/*union _isp_bnlc_status_tag* reg_ptr=(union _isp_bnlc_status_tag*)ISP_NLC_STATUS_V0001;*/
		union _isp_bnlc_status_tag* reg_s_ptr=(union _isp_bnlc_status_tag*)&isp_reg_ptr->NLC_STATUS;
		struct isp_reg_bits reg_config;
		struct isp_reg_param read_param;

		ISP_CHECK_FD;

		reg_config.reg_addr = ISP_NLC_STATUS_V0001 - ISP_BASE_ADDR;
		read_param.reg_param = (unsigned long)&reg_config;
		read_param.counts = 1;
		/*
		reg_s_ptr->dwValue=reg_ptr->dwValue;
		*status=reg_s_ptr->dwValue;
		*/
		if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		reg_s_ptr->dwValue = reg_config.reg_value;
		*status = reg_config.reg_value;
		}
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispNlcBypass(uint32_t handler_id, uint8_t bypass)
{
	uint32_t isp_id=_isp_GetIspId();
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	if(SC8825_ISP_ID==isp_id)
	{
		/*	union _isp_bnlc_param_tag* reg_ptr=(union _isp_bnlc_param_tag*)ISP_BNLC_PARAM;*/
		union _isp_bnlc_param_tag* reg_s_ptr=(union _isp_bnlc_param_tag*)&isp_reg_ptr->BNLC_PARAM;

		reg_s_ptr->mBits.nlc_bypass=bypass;
		/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
		reg_config.reg_addr = ISP_BNLC_PARAM - ISP_BASE_ADDR;
		reg_config.reg_value = isp_reg_ptr->BNLC_PARAM;;
		write_param.reg_param = (unsigned long)&reg_config;
		write_param.counts = 1;
	}
	else if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
		/*	union _isp_nlc_param_tag* reg_ptr=(union _isp_bnlc_param_tag*)ISP_BNLC_PARAM;*/
		union _isp_nlc_param_v0001_tag* reg_s_ptr=(union _isp_nlc_param_v0001_tag*)&isp_reg_ptr->NLC_PARAM;

		reg_s_ptr->mBits.bypass=bypass;
		/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
		reg_config.reg_addr = ISP_NLC_PARAM_V0001 - ISP_BASE_ADDR;
		reg_config.reg_value = isp_reg_ptr->NLC_PARAM;;
		write_param.reg_param = (unsigned long)&reg_config;
		write_param.counts = 1;
	}
	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetNlcRNode(uint32_t handler_id, uint16_t* r_node_ptr)
{
	uint32_t isp_id=_isp_GetIspId();
/*	union _isp_bnlc_n_param_r0_tag* reg0_ptr=(union _isp_bnlc_n_param_r0_tag*)ISP_BNLC_N_PARAM_R0;
	union _isp_bnlc_n_param_r1_tag* reg1_ptr=(union _isp_bnlc_n_param_r1_tag*)ISP_BNLC_N_PARAM_R1;
	union _isp_bnlc_n_param_r2_tag* reg2_ptr=(union _isp_bnlc_n_param_r2_tag*)ISP_BNLC_N_PARAM_R2;
	union _isp_bnlc_n_param_r3_tag* reg3_ptr=(union _isp_bnlc_n_param_r3_tag*)ISP_BNLC_N_PARAM_R3;
	union _isp_bnlc_n_param_r4_tag* reg4_ptr=(union _isp_bnlc_n_param_r4_tag*)ISP_BNLC_N_PARAM_R4;
	union _isp_bnlc_n_param_r5_tag* reg5_ptr=(union _isp_bnlc_n_param_r5_tag*)ISP_BNLC_N_PARAM_R5;
	union _isp_bnlc_n_param_r6_tag* reg6_ptr=(union _isp_bnlc_n_param_r6_tag*)ISP_BNLC_N_PARAM_R6;
	union _isp_bnlc_n_param_r7_tag* reg7_ptr=(union _isp_bnlc_n_param_r7_tag*)ISP_BNLC_N_PARAM_R7;
	union _isp_bnlc_n_param_r8_tag* reg8_ptr=(union _isp_bnlc_n_param_r8_tag*)ISP_BNLC_N_PARAM_R8;
	union _isp_bnlc_n_param_r9_tag* reg9_ptr=(union _isp_bnlc_n_param_r9_tag*)ISP_BNLC_N_PARAM_R9;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_bnlc_n_param_r0_tag* reg0_s_ptr=(union _isp_bnlc_n_param_r0_tag*)&isp_reg_ptr->BNLC_N_PARAM_R0;
	union _isp_bnlc_n_param_r1_tag* reg1_s_ptr=(union _isp_bnlc_n_param_r1_tag*)&isp_reg_ptr->BNLC_N_PARAM_R1;
	union _isp_bnlc_n_param_r2_tag* reg2_s_ptr=(union _isp_bnlc_n_param_r2_tag*)&isp_reg_ptr->BNLC_N_PARAM_R2;
	union _isp_bnlc_n_param_r3_tag* reg3_s_ptr=(union _isp_bnlc_n_param_r3_tag*)&isp_reg_ptr->BNLC_N_PARAM_R3;
	union _isp_bnlc_n_param_r4_tag* reg4_s_ptr=(union _isp_bnlc_n_param_r4_tag*)&isp_reg_ptr->BNLC_N_PARAM_R4;
	union _isp_bnlc_n_param_r5_tag* reg5_s_ptr=(union _isp_bnlc_n_param_r5_tag*)&isp_reg_ptr->BNLC_N_PARAM_R5;
	union _isp_bnlc_n_param_r6_tag* reg6_s_ptr=(union _isp_bnlc_n_param_r6_tag*)&isp_reg_ptr->BNLC_N_PARAM_R6;
	union _isp_bnlc_n_param_r7_tag* reg7_s_ptr=(union _isp_bnlc_n_param_r7_tag*)&isp_reg_ptr->BNLC_N_PARAM_R7;
	union _isp_bnlc_n_param_r8_tag* reg8_s_ptr=(union _isp_bnlc_n_param_r8_tag*)&isp_reg_ptr->BNLC_N_PARAM_R8;
	union _isp_bnlc_n_param_r9_tag* reg9_s_ptr=(union _isp_bnlc_n_param_r9_tag*)&isp_reg_ptr->BNLC_N_PARAM_R9;
	struct isp_reg_bits reg_config[10];
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg0_s_ptr->mBits.nlc_r_node0=r_node_ptr[0];
	reg0_s_ptr->mBits.nlc_r_node1=r_node_ptr[1];
	reg0_s_ptr->mBits.nlc_r_node2=r_node_ptr[2];
	reg1_s_ptr->mBits.nlc_r_node3=r_node_ptr[3];
	reg1_s_ptr->mBits.nlc_r_node4=r_node_ptr[4];
	reg1_s_ptr->mBits.nlc_r_node5=r_node_ptr[5];
	reg2_s_ptr->mBits.nlc_r_node6=r_node_ptr[6];
	reg2_s_ptr->mBits.nlc_r_node7=r_node_ptr[7];
	reg2_s_ptr->mBits.nlc_r_node8=r_node_ptr[8];
	reg3_s_ptr->mBits.nlc_r_node9=r_node_ptr[9];
	reg3_s_ptr->mBits.nlc_r_node10=r_node_ptr[10];
	reg3_s_ptr->mBits.nlc_r_node11=r_node_ptr[11];
	reg4_s_ptr->mBits.nlc_r_node12=r_node_ptr[12];
	reg4_s_ptr->mBits.nlc_r_node13=r_node_ptr[13];
	reg4_s_ptr->mBits.nlc_r_node14=r_node_ptr[14];
	reg5_s_ptr->mBits.nlc_r_node15=r_node_ptr[15];
	reg5_s_ptr->mBits.nlc_r_node16=r_node_ptr[16];
	reg5_s_ptr->mBits.nlc_r_node17=r_node_ptr[17];
	reg6_s_ptr->mBits.nlc_r_node18=r_node_ptr[18];
	reg6_s_ptr->mBits.nlc_r_node19=r_node_ptr[19];
	reg6_s_ptr->mBits.nlc_r_node20=r_node_ptr[20];
	reg7_s_ptr->mBits.nlc_r_node21=r_node_ptr[21];
	reg7_s_ptr->mBits.nlc_r_node22=r_node_ptr[22];
	reg7_s_ptr->mBits.nlc_r_node23=r_node_ptr[23];
	reg8_s_ptr->mBits.nlc_r_node24=r_node_ptr[24];
	reg8_s_ptr->mBits.nlc_r_node25=r_node_ptr[25];
	reg8_s_ptr->mBits.nlc_r_node26=r_node_ptr[26];
	reg9_s_ptr->mBits.nlc_r_node27=r_node_ptr[27];
	reg9_s_ptr->mBits.nlc_r_node28=r_node_ptr[28];

	if(SC8825_ISP_ID==isp_id)
	{
	/*	reg0_ptr->dwValue=reg0_s_ptr->dwValue;
		reg1_ptr->dwValue=reg1_s_ptr->dwValue;
		reg2_ptr->dwValue=reg2_s_ptr->dwValue;
		reg3_ptr->dwValue=reg3_s_ptr->dwValue;
		reg4_ptr->dwValue=reg4_s_ptr->dwValue;
		reg5_ptr->dwValue=reg5_s_ptr->dwValue;
		reg6_ptr->dwValue=reg6_s_ptr->dwValue;
		reg7_ptr->dwValue=reg7_s_ptr->dwValue;
		reg8_ptr->dwValue=reg8_s_ptr->dwValue;
		reg9_ptr->dwValue=reg9_s_ptr->dwValue;*/

		reg_config[0].reg_addr = ISP_BNLC_N_PARAM_R0 - ISP_BASE_ADDR;
		reg_config[0].reg_value = isp_reg_ptr->BNLC_N_PARAM_R0;
		reg_config[1].reg_addr = ISP_BNLC_N_PARAM_R1 - ISP_BASE_ADDR;
		reg_config[1].reg_value = isp_reg_ptr->BNLC_N_PARAM_R1;
		reg_config[2].reg_addr = ISP_BNLC_N_PARAM_R2 - ISP_BASE_ADDR;
		reg_config[2].reg_value = isp_reg_ptr->BNLC_N_PARAM_R2;
		reg_config[3].reg_addr = ISP_BNLC_N_PARAM_R3 - ISP_BASE_ADDR;
		reg_config[3].reg_value = isp_reg_ptr->BNLC_N_PARAM_R3;
		reg_config[4].reg_addr = ISP_BNLC_N_PARAM_R4 - ISP_BASE_ADDR;
		reg_config[4].reg_value = isp_reg_ptr->BNLC_N_PARAM_R4;
		reg_config[5].reg_addr = ISP_BNLC_N_PARAM_R5 - ISP_BASE_ADDR;
		reg_config[5].reg_value = isp_reg_ptr->BNLC_N_PARAM_R5;
		reg_config[6].reg_addr = ISP_BNLC_N_PARAM_R6 - ISP_BASE_ADDR;
		reg_config[6].reg_value = isp_reg_ptr->BNLC_N_PARAM_R6;
		reg_config[7].reg_addr = ISP_BNLC_N_PARAM_R7 - ISP_BASE_ADDR;
		reg_config[7].reg_value = isp_reg_ptr->BNLC_N_PARAM_R7;
		reg_config[8].reg_addr = ISP_BNLC_N_PARAM_R8 - ISP_BASE_ADDR;
		reg_config[8].reg_value = isp_reg_ptr->BNLC_N_PARAM_R8;
		reg_config[9].reg_addr = ISP_BNLC_N_PARAM_R9 - ISP_BASE_ADDR;
		reg_config[9].reg_value = isp_reg_ptr->BNLC_N_PARAM_R9;
	}
	else if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
	/*	reg0_ptr->dwValue=reg0_s_ptr->dwValue;
		reg1_ptr->dwValue=reg1_s_ptr->dwValue;
		reg2_ptr->dwValue=reg2_s_ptr->dwValue;
		reg3_ptr->dwValue=reg3_s_ptr->dwValue;
		reg4_ptr->dwValue=reg4_s_ptr->dwValue;
		reg5_ptr->dwValue=reg5_s_ptr->dwValue;
		reg6_ptr->dwValue=reg6_s_ptr->dwValue;
		reg7_ptr->dwValue=reg7_s_ptr->dwValue;
		reg8_ptr->dwValue=reg8_s_ptr->dwValue;
		reg9_ptr->dwValue=reg9_s_ptr->dwValue;*/

		reg_config[0].reg_addr = ISP_NLC_N_PARAM_R0_V0001 - ISP_BASE_ADDR;
		reg_config[0].reg_value = isp_reg_ptr->BNLC_N_PARAM_R0;
		reg_config[1].reg_addr = ISP_NLC_N_PARAM_R1_V0001 - ISP_BASE_ADDR;
		reg_config[1].reg_value = isp_reg_ptr->BNLC_N_PARAM_R1;
		reg_config[2].reg_addr = ISP_NLC_N_PARAM_R2_V0001 - ISP_BASE_ADDR;
		reg_config[2].reg_value = isp_reg_ptr->BNLC_N_PARAM_R2;
		reg_config[3].reg_addr = ISP_NLC_N_PARAM_R3_V0001 - ISP_BASE_ADDR;
		reg_config[3].reg_value = isp_reg_ptr->BNLC_N_PARAM_R3;
		reg_config[4].reg_addr = ISP_NLC_N_PARAM_R4_V0001 - ISP_BASE_ADDR;
		reg_config[4].reg_value = isp_reg_ptr->BNLC_N_PARAM_R4;
		reg_config[5].reg_addr = ISP_NLC_N_PARAM_R5_V0001 - ISP_BASE_ADDR;
		reg_config[5].reg_value = isp_reg_ptr->BNLC_N_PARAM_R5;
		reg_config[6].reg_addr = ISP_NLC_N_PARAM_R6_V0001 - ISP_BASE_ADDR;
		reg_config[6].reg_value = isp_reg_ptr->BNLC_N_PARAM_R6;
		reg_config[7].reg_addr = ISP_NLC_N_PARAM_R7_V0001 - ISP_BASE_ADDR;
		reg_config[7].reg_value = isp_reg_ptr->BNLC_N_PARAM_R7;
		reg_config[8].reg_addr = ISP_NLC_N_PARAM_R8_V0001 - ISP_BASE_ADDR;
		reg_config[8].reg_value = isp_reg_ptr->BNLC_N_PARAM_R8;
		reg_config[9].reg_addr = ISP_NLC_N_PARAM_R9_V0001 - ISP_BASE_ADDR;
		reg_config[9].reg_value = isp_reg_ptr->BNLC_N_PARAM_R9;
	}

	write_param.reg_param = (unsigned long)&reg_config[0];
	write_param.counts = 10;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetNlcGNode(uint32_t handler_id, uint16_t* g_node_ptr)
{
	uint32_t isp_id=_isp_GetIspId();
/*	union _isp_bnlc_n_param_g0_tag* reg0_ptr=(union _isp_bnlc_n_param_g0_tag*)ISP_BNLC_N_PARAM_G0;
	union _isp_bnlc_n_param_g1_tag* reg1_ptr=(union _isp_bnlc_n_param_g1_tag*)ISP_BNLC_N_PARAM_G1;
	union _isp_bnlc_n_param_g2_tag* reg2_ptr=(union _isp_bnlc_n_param_g2_tag*)ISP_BNLC_N_PARAM_G2;
	union _isp_bnlc_n_param_g3_tag* reg3_ptr=(union _isp_bnlc_n_param_g3_tag*)ISP_BNLC_N_PARAM_G3;
	union _isp_bnlc_n_param_g4_tag* reg4_ptr=(union _isp_bnlc_n_param_g4_tag*)ISP_BNLC_N_PARAM_G4;
	union _isp_bnlc_n_param_g5_tag* reg5_ptr=(union _isp_bnlc_n_param_g5_tag*)ISP_BNLC_N_PARAM_G5;
	union _isp_bnlc_n_param_g6_tag* reg6_ptr=(union _isp_bnlc_n_param_g6_tag*)ISP_BNLC_N_PARAM_G6;
	union _isp_bnlc_n_param_g7_tag* reg7_ptr=(union _isp_bnlc_n_param_g7_tag*)ISP_BNLC_N_PARAM_G7;
	union _isp_bnlc_n_param_g8_tag* reg8_ptr=(union _isp_bnlc_n_param_g8_tag*)ISP_BNLC_N_PARAM_G8;
	union _isp_bnlc_n_param_g9_tag* reg9_ptr=(union _isp_bnlc_n_param_g9_tag*)ISP_BNLC_N_PARAM_G9;
*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_bnlc_n_param_g0_tag* reg0_s_ptr=(union _isp_bnlc_n_param_g0_tag*)&isp_reg_ptr->BNLC_N_PARAM_G0;
	union _isp_bnlc_n_param_g1_tag* reg1_s_ptr=(union _isp_bnlc_n_param_g1_tag*)&isp_reg_ptr->BNLC_N_PARAM_G1;
	union _isp_bnlc_n_param_g2_tag* reg2_s_ptr=(union _isp_bnlc_n_param_g2_tag*)&isp_reg_ptr->BNLC_N_PARAM_G2;
	union _isp_bnlc_n_param_g3_tag* reg3_s_ptr=(union _isp_bnlc_n_param_g3_tag*)&isp_reg_ptr->BNLC_N_PARAM_G3;
	union _isp_bnlc_n_param_g4_tag* reg4_s_ptr=(union _isp_bnlc_n_param_g4_tag*)&isp_reg_ptr->BNLC_N_PARAM_G4;
	union _isp_bnlc_n_param_g5_tag* reg5_s_ptr=(union _isp_bnlc_n_param_g5_tag*)&isp_reg_ptr->BNLC_N_PARAM_G5;
	union _isp_bnlc_n_param_g6_tag* reg6_s_ptr=(union _isp_bnlc_n_param_g6_tag*)&isp_reg_ptr->BNLC_N_PARAM_G6;
	union _isp_bnlc_n_param_g7_tag* reg7_s_ptr=(union _isp_bnlc_n_param_g7_tag*)&isp_reg_ptr->BNLC_N_PARAM_G7;
	union _isp_bnlc_n_param_g8_tag* reg8_s_ptr=(union _isp_bnlc_n_param_g8_tag*)&isp_reg_ptr->BNLC_N_PARAM_G8;
	union _isp_bnlc_n_param_g9_tag* reg9_s_ptr=(union _isp_bnlc_n_param_g9_tag*)&isp_reg_ptr->BNLC_N_PARAM_G9;
	struct isp_reg_bits reg_config[10];
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg0_s_ptr->mBits.nlc_g_node0=g_node_ptr[0];
	reg0_s_ptr->mBits.nlc_g_node1=g_node_ptr[1];
	reg0_s_ptr->mBits.nlc_g_node2=g_node_ptr[2];
	reg1_s_ptr->mBits.nlc_g_node3=g_node_ptr[3];
	reg1_s_ptr->mBits.nlc_g_node4=g_node_ptr[4];
	reg1_s_ptr->mBits.nlc_g_node5=g_node_ptr[5];
	reg2_s_ptr->mBits.nlc_g_node6=g_node_ptr[6];
	reg2_s_ptr->mBits.nlc_g_node7=g_node_ptr[7];
	reg2_s_ptr->mBits.nlc_g_node8=g_node_ptr[8];
	reg3_s_ptr->mBits.nlc_g_node9=g_node_ptr[9];
	reg3_s_ptr->mBits.nlc_g_node10=g_node_ptr[10];
	reg3_s_ptr->mBits.nlc_g_node11=g_node_ptr[11];
	reg4_s_ptr->mBits.nlc_g_node12=g_node_ptr[12];
	reg4_s_ptr->mBits.nlc_g_node13=g_node_ptr[13];
	reg4_s_ptr->mBits.nlc_g_node14=g_node_ptr[14];
	reg5_s_ptr->mBits.nlc_g_node15=g_node_ptr[15];
	reg5_s_ptr->mBits.nlc_g_node16=g_node_ptr[16];
	reg5_s_ptr->mBits.nlc_g_node17=g_node_ptr[17];
	reg6_s_ptr->mBits.nlc_g_node18=g_node_ptr[18];
	reg6_s_ptr->mBits.nlc_g_node19=g_node_ptr[19];
	reg6_s_ptr->mBits.nlc_g_node20=g_node_ptr[20];
	reg7_s_ptr->mBits.nlc_g_node21=g_node_ptr[21];
	reg7_s_ptr->mBits.nlc_g_node22=g_node_ptr[22];
	reg7_s_ptr->mBits.nlc_g_node23=g_node_ptr[23];
	reg8_s_ptr->mBits.nlc_g_node24=g_node_ptr[24];
	reg8_s_ptr->mBits.nlc_g_node25=g_node_ptr[25];
	reg8_s_ptr->mBits.nlc_g_node26=g_node_ptr[26];
	reg9_s_ptr->mBits.nlc_g_node27=g_node_ptr[27];
	reg9_s_ptr->mBits.nlc_g_node28=g_node_ptr[28];

	if(SC8825_ISP_ID==isp_id)
	{
	/*
		reg0_ptr->dwValue=reg0_s_ptr->dwValue;
		reg1_ptr->dwValue=reg1_s_ptr->dwValue;
		reg2_ptr->dwValue=reg2_s_ptr->dwValue;
		reg3_ptr->dwValue=reg3_s_ptr->dwValue;
		reg4_ptr->dwValue=reg4_s_ptr->dwValue;
		reg5_ptr->dwValue=reg5_s_ptr->dwValue;
		reg6_ptr->dwValue=reg6_s_ptr->dwValue;
		reg7_ptr->dwValue=reg7_s_ptr->dwValue;
		reg8_ptr->dwValue=reg8_s_ptr->dwValue;
		reg9_ptr->dwValue=reg9_s_ptr->dwValue;
	*/
		reg_config[0].reg_addr = ISP_BNLC_N_PARAM_G0 - ISP_BASE_ADDR;
		reg_config[0].reg_value = isp_reg_ptr->BNLC_N_PARAM_G0;
		reg_config[1].reg_addr = ISP_BNLC_N_PARAM_G1 - ISP_BASE_ADDR;
		reg_config[1].reg_value = isp_reg_ptr->BNLC_N_PARAM_G1;
		reg_config[2].reg_addr = ISP_BNLC_N_PARAM_G2 - ISP_BASE_ADDR;
		reg_config[2].reg_value = isp_reg_ptr->BNLC_N_PARAM_G2;
		reg_config[3].reg_addr = ISP_BNLC_N_PARAM_G3 - ISP_BASE_ADDR;
		reg_config[3].reg_value = isp_reg_ptr->BNLC_N_PARAM_G3;
		reg_config[4].reg_addr = ISP_BNLC_N_PARAM_G4 - ISP_BASE_ADDR;
		reg_config[4].reg_value = isp_reg_ptr->BNLC_N_PARAM_G4;
		reg_config[5].reg_addr = ISP_BNLC_N_PARAM_G5 - ISP_BASE_ADDR;
		reg_config[5].reg_value = isp_reg_ptr->BNLC_N_PARAM_G5;
		reg_config[6].reg_addr = ISP_BNLC_N_PARAM_G6 - ISP_BASE_ADDR;
		reg_config[6].reg_value = isp_reg_ptr->BNLC_N_PARAM_G6;
		reg_config[7].reg_addr = ISP_BNLC_N_PARAM_G7 - ISP_BASE_ADDR;
		reg_config[7].reg_value = isp_reg_ptr->BNLC_N_PARAM_G7;
		reg_config[8].reg_addr = ISP_BNLC_N_PARAM_G8 - ISP_BASE_ADDR;
		reg_config[8].reg_value = isp_reg_ptr->BNLC_N_PARAM_G8;
		reg_config[9].reg_addr = ISP_BNLC_N_PARAM_G9 - ISP_BASE_ADDR;
		reg_config[9].reg_value = isp_reg_ptr->BNLC_N_PARAM_G9;
	}
	else if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
	/*
		reg0_ptr->dwValue=reg0_s_ptr->dwValue;
		reg1_ptr->dwValue=reg1_s_ptr->dwValue;
		reg2_ptr->dwValue=reg2_s_ptr->dwValue;
		reg3_ptr->dwValue=reg3_s_ptr->dwValue;
		reg4_ptr->dwValue=reg4_s_ptr->dwValue;
		reg5_ptr->dwValue=reg5_s_ptr->dwValue;
		reg6_ptr->dwValue=reg6_s_ptr->dwValue;
		reg7_ptr->dwValue=reg7_s_ptr->dwValue;
		reg8_ptr->dwValue=reg8_s_ptr->dwValue;
		reg9_ptr->dwValue=reg9_s_ptr->dwValue;
	*/
		reg_config[0].reg_addr = ISP_NLC_N_PARAM_G0_V0001 - ISP_BASE_ADDR;
		reg_config[0].reg_value = isp_reg_ptr->BNLC_N_PARAM_G0;
		reg_config[1].reg_addr = ISP_NLC_N_PARAM_G1_V0001 - ISP_BASE_ADDR;
		reg_config[1].reg_value = isp_reg_ptr->BNLC_N_PARAM_G1;
		reg_config[2].reg_addr = ISP_NLC_N_PARAM_G2_V0001 - ISP_BASE_ADDR;
		reg_config[2].reg_value = isp_reg_ptr->BNLC_N_PARAM_G2;
		reg_config[3].reg_addr = ISP_NLC_N_PARAM_G3_V0001 - ISP_BASE_ADDR;
		reg_config[3].reg_value = isp_reg_ptr->BNLC_N_PARAM_G3;
		reg_config[4].reg_addr = ISP_NLC_N_PARAM_G4_V0001 - ISP_BASE_ADDR;
		reg_config[4].reg_value = isp_reg_ptr->BNLC_N_PARAM_G4;
		reg_config[5].reg_addr = ISP_NLC_N_PARAM_G5_V0001 - ISP_BASE_ADDR;
		reg_config[5].reg_value = isp_reg_ptr->BNLC_N_PARAM_G5;
		reg_config[6].reg_addr = ISP_NLC_N_PARAM_G6_V0001 - ISP_BASE_ADDR;
		reg_config[6].reg_value = isp_reg_ptr->BNLC_N_PARAM_G6;
		reg_config[7].reg_addr = ISP_NLC_N_PARAM_G7_V0001 - ISP_BASE_ADDR;
		reg_config[7].reg_value = isp_reg_ptr->BNLC_N_PARAM_G7;
		reg_config[8].reg_addr = ISP_NLC_N_PARAM_G8_V0001 - ISP_BASE_ADDR;
		reg_config[8].reg_value = isp_reg_ptr->BNLC_N_PARAM_G8;
		reg_config[9].reg_addr = ISP_NLC_N_PARAM_G9_V0001 - ISP_BASE_ADDR;
		reg_config[9].reg_value = isp_reg_ptr->BNLC_N_PARAM_G9;
	}

	write_param.reg_param = (unsigned long)&reg_config[0];
	write_param.counts = 10;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetNlcBNode(uint32_t handler_id, uint16_t* b_node_ptr)
{
	uint32_t isp_id=_isp_GetIspId();
/*	union _isp_bnlc_n_param_b0_tag* reg0_ptr=(union _isp_bnlc_n_param_b0_tag*)ISP_BNLC_N_PARAM_B0;
	union _isp_bnlc_n_param_b1_tag* reg1_ptr=(union _isp_bnlc_n_param_b1_tag*)ISP_BNLC_N_PARAM_B1;
	union _isp_bnlc_n_param_b2_tag* reg2_ptr=(union _isp_bnlc_n_param_b2_tag*)ISP_BNLC_N_PARAM_B2;
	union _isp_bnlc_n_param_b3_tag* reg3_ptr=(union _isp_bnlc_n_param_b3_tag*)ISP_BNLC_N_PARAM_B3;
	union _isp_bnlc_n_param_b4_tag* reg4_ptr=(union _isp_bnlc_n_param_b4_tag*)ISP_BNLC_N_PARAM_B4;
	union _isp_bnlc_n_param_b5_tag* reg5_ptr=(union _isp_bnlc_n_param_b5_tag*)ISP_BNLC_N_PARAM_B5;
	union _isp_bnlc_n_param_b6_tag* reg6_ptr=(union _isp_bnlc_n_param_b6_tag*)ISP_BNLC_N_PARAM_B6;
	union _isp_bnlc_n_param_b7_tag* reg7_ptr=(union _isp_bnlc_n_param_b7_tag*)ISP_BNLC_N_PARAM_B7;
	union _isp_bnlc_n_param_b8_tag* reg8_ptr=(union _isp_bnlc_n_param_b8_tag*)ISP_BNLC_N_PARAM_B8;
	union _isp_bnlc_n_param_b9_tag* reg9_ptr=(union _isp_bnlc_n_param_b9_tag*)ISP_BNLC_N_PARAM_B9;
*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_bnlc_n_param_b0_tag* reg0_s_ptr=(union _isp_bnlc_n_param_b0_tag*)&isp_reg_ptr->BNLC_N_PARAM_B0;
	union _isp_bnlc_n_param_b1_tag* reg1_s_ptr=(union _isp_bnlc_n_param_b1_tag*)&isp_reg_ptr->BNLC_N_PARAM_B1;
	union _isp_bnlc_n_param_b2_tag* reg2_s_ptr=(union _isp_bnlc_n_param_b2_tag*)&isp_reg_ptr->BNLC_N_PARAM_B2;
	union _isp_bnlc_n_param_b3_tag* reg3_s_ptr=(union _isp_bnlc_n_param_b3_tag*)&isp_reg_ptr->BNLC_N_PARAM_B3;
	union _isp_bnlc_n_param_b4_tag* reg4_s_ptr=(union _isp_bnlc_n_param_b4_tag*)&isp_reg_ptr->BNLC_N_PARAM_B4;
	union _isp_bnlc_n_param_b5_tag* reg5_s_ptr=(union _isp_bnlc_n_param_b5_tag*)&isp_reg_ptr->BNLC_N_PARAM_B5;
	union _isp_bnlc_n_param_b6_tag* reg6_s_ptr=(union _isp_bnlc_n_param_b6_tag*)&isp_reg_ptr->BNLC_N_PARAM_B6;
	union _isp_bnlc_n_param_b7_tag* reg7_s_ptr=(union _isp_bnlc_n_param_b7_tag*)&isp_reg_ptr->BNLC_N_PARAM_B7;
	union _isp_bnlc_n_param_b8_tag* reg8_s_ptr=(union _isp_bnlc_n_param_b8_tag*)&isp_reg_ptr->BNLC_N_PARAM_B8;
	union _isp_bnlc_n_param_b9_tag* reg9_s_ptr=(union _isp_bnlc_n_param_b9_tag*)&isp_reg_ptr->BNLC_N_PARAM_B9;
	struct isp_reg_bits reg_config[10];
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg0_s_ptr->mBits.nlc_b_node0=b_node_ptr[0];
	reg0_s_ptr->mBits.nlc_b_node1=b_node_ptr[1];
	reg0_s_ptr->mBits.nlc_b_node2=b_node_ptr[2];
	reg1_s_ptr->mBits.nlc_b_node3=b_node_ptr[3];
	reg1_s_ptr->mBits.nlc_b_node4=b_node_ptr[4];
	reg1_s_ptr->mBits.nlc_b_node5=b_node_ptr[5];
	reg2_s_ptr->mBits.nlc_b_node6=b_node_ptr[6];
	reg2_s_ptr->mBits.nlc_b_node7=b_node_ptr[7];
	reg2_s_ptr->mBits.nlc_b_node8=b_node_ptr[8];
	reg3_s_ptr->mBits.nlc_b_node9=b_node_ptr[9];
	reg3_s_ptr->mBits.nlc_b_node10=b_node_ptr[10];
	reg3_s_ptr->mBits.nlc_b_node11=b_node_ptr[11];
	reg4_s_ptr->mBits.nlc_b_node12=b_node_ptr[12];
	reg4_s_ptr->mBits.nlc_b_node13=b_node_ptr[13];
	reg4_s_ptr->mBits.nlc_b_node14=b_node_ptr[14];
	reg5_s_ptr->mBits.nlc_b_node15=b_node_ptr[15];
	reg5_s_ptr->mBits.nlc_b_node16=b_node_ptr[16];
	reg5_s_ptr->mBits.nlc_b_node17=b_node_ptr[17];
	reg6_s_ptr->mBits.nlc_b_node18=b_node_ptr[18];
	reg6_s_ptr->mBits.nlc_b_node19=b_node_ptr[19];
	reg6_s_ptr->mBits.nlc_b_node20=b_node_ptr[20];
	reg7_s_ptr->mBits.nlc_b_node21=b_node_ptr[21];
	reg7_s_ptr->mBits.nlc_b_node22=b_node_ptr[22];
	reg7_s_ptr->mBits.nlc_b_node23=b_node_ptr[23];
	reg8_s_ptr->mBits.nlc_b_node24=b_node_ptr[24];
	reg8_s_ptr->mBits.nlc_b_node25=b_node_ptr[25];
	reg8_s_ptr->mBits.nlc_b_node26=b_node_ptr[26];
	reg9_s_ptr->mBits.nlc_b_node27=b_node_ptr[27];
	reg9_s_ptr->mBits.nlc_b_node28=b_node_ptr[28];

	if(SC8825_ISP_ID==isp_id)
	{
	/*
		reg0_ptr->dwValue=reg0_s_ptr->dwValue;
		reg1_ptr->dwValue=reg1_s_ptr->dwValue;
		reg2_ptr->dwValue=reg2_s_ptr->dwValue;
		reg3_ptr->dwValue=reg3_s_ptr->dwValue;
		reg4_ptr->dwValue=reg4_s_ptr->dwValue;
		reg5_ptr->dwValue=reg5_s_ptr->dwValue;
		reg6_ptr->dwValue=reg6_s_ptr->dwValue;
		reg7_ptr->dwValue=reg7_s_ptr->dwValue;
		reg8_ptr->dwValue=reg8_s_ptr->dwValue;
		reg9_ptr->dwValue=reg9_s_ptr->dwValue;
	*/
		reg_config[0].reg_addr = ISP_BNLC_N_PARAM_B0 - ISP_BASE_ADDR;
		reg_config[0].reg_value = isp_reg_ptr->BNLC_N_PARAM_B0;
		reg_config[1].reg_addr = ISP_BNLC_N_PARAM_B1 - ISP_BASE_ADDR;
		reg_config[1].reg_value = isp_reg_ptr->BNLC_N_PARAM_B1;
		reg_config[2].reg_addr = ISP_BNLC_N_PARAM_B2 - ISP_BASE_ADDR;
		reg_config[2].reg_value = isp_reg_ptr->BNLC_N_PARAM_B2;
		reg_config[3].reg_addr = ISP_BNLC_N_PARAM_B3 - ISP_BASE_ADDR;
		reg_config[3].reg_value = isp_reg_ptr->BNLC_N_PARAM_B3;
		reg_config[4].reg_addr = ISP_BNLC_N_PARAM_B4 - ISP_BASE_ADDR;
		reg_config[4].reg_value = isp_reg_ptr->BNLC_N_PARAM_B4;
		reg_config[5].reg_addr = ISP_BNLC_N_PARAM_B5 - ISP_BASE_ADDR;
		reg_config[5].reg_value = isp_reg_ptr->BNLC_N_PARAM_B5;
		reg_config[6].reg_addr = ISP_BNLC_N_PARAM_B6 - ISP_BASE_ADDR;
		reg_config[6].reg_value = isp_reg_ptr->BNLC_N_PARAM_B6;
		reg_config[7].reg_addr = ISP_BNLC_N_PARAM_B7 - ISP_BASE_ADDR;
		reg_config[7].reg_value = isp_reg_ptr->BNLC_N_PARAM_B7;
		reg_config[8].reg_addr = ISP_BNLC_N_PARAM_B8 - ISP_BASE_ADDR;
		reg_config[8].reg_value = isp_reg_ptr->BNLC_N_PARAM_B8;
		reg_config[9].reg_addr = ISP_BNLC_N_PARAM_B9 - ISP_BASE_ADDR;
		reg_config[9].reg_value = isp_reg_ptr->BNLC_N_PARAM_B9;
	}
	else if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
	/*
		reg0_ptr->dwValue=reg0_s_ptr->dwValue;
		reg1_ptr->dwValue=reg1_s_ptr->dwValue;
		reg2_ptr->dwValue=reg2_s_ptr->dwValue;
		reg3_ptr->dwValue=reg3_s_ptr->dwValue;
		reg4_ptr->dwValue=reg4_s_ptr->dwValue;
		reg5_ptr->dwValue=reg5_s_ptr->dwValue;
		reg6_ptr->dwValue=reg6_s_ptr->dwValue;
		reg7_ptr->dwValue=reg7_s_ptr->dwValue;
		reg8_ptr->dwValue=reg8_s_ptr->dwValue;
		reg9_ptr->dwValue=reg9_s_ptr->dwValue;
	*/
		reg_config[0].reg_addr = ISP_NLC_N_PARAM_B0_V0001 - ISP_BASE_ADDR;
		reg_config[0].reg_value = isp_reg_ptr->BNLC_N_PARAM_B0;
		reg_config[1].reg_addr = ISP_NLC_N_PARAM_B1_V0001 - ISP_BASE_ADDR;
		reg_config[1].reg_value = isp_reg_ptr->BNLC_N_PARAM_B1;
		reg_config[2].reg_addr = ISP_NLC_N_PARAM_B2_V0001 - ISP_BASE_ADDR;
		reg_config[2].reg_value = isp_reg_ptr->BNLC_N_PARAM_B2;
		reg_config[3].reg_addr = ISP_NLC_N_PARAM_B3_V0001 - ISP_BASE_ADDR;
		reg_config[3].reg_value = isp_reg_ptr->BNLC_N_PARAM_B3;
		reg_config[4].reg_addr = ISP_NLC_N_PARAM_B4_V0001 - ISP_BASE_ADDR;
		reg_config[4].reg_value = isp_reg_ptr->BNLC_N_PARAM_B4;
		reg_config[5].reg_addr = ISP_NLC_N_PARAM_B5_V0001 - ISP_BASE_ADDR;
		reg_config[5].reg_value = isp_reg_ptr->BNLC_N_PARAM_B5;
		reg_config[6].reg_addr = ISP_NLC_N_PARAM_B6_V0001 - ISP_BASE_ADDR;
		reg_config[6].reg_value = isp_reg_ptr->BNLC_N_PARAM_B6;
		reg_config[7].reg_addr = ISP_NLC_N_PARAM_B7_V0001 - ISP_BASE_ADDR;
		reg_config[7].reg_value = isp_reg_ptr->BNLC_N_PARAM_B7;
		reg_config[8].reg_addr = ISP_NLC_N_PARAM_B8_V0001 - ISP_BASE_ADDR;
		reg_config[8].reg_value = isp_reg_ptr->BNLC_N_PARAM_B8;
		reg_config[9].reg_addr = ISP_NLC_N_PARAM_B9_V0001 - ISP_BASE_ADDR;
		reg_config[9].reg_value = isp_reg_ptr->BNLC_N_PARAM_B9;
	}

	write_param.reg_param = (unsigned long)&reg_config[0];
	write_param.counts = 10;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetNlcLNode(uint32_t handler_id, uint16_t* l_node_ptr)
{
	uint32_t isp_id=_isp_GetIspId();
/*	union _isp_bnlc_n_param_l0_tag* reg0_ptr=(union _isp_bnlc_n_param_l0_tag*)ISP_BNLC_N_PARAM_L0;
	union _isp_bnlc_n_param_l1_tag* reg1_ptr=(union _isp_bnlc_n_param_l1_tag*)ISP_BNLC_N_PARAM_L1;
	union _isp_bnlc_n_param_l2_tag* reg2_ptr=(union _isp_bnlc_n_param_l2_tag*)ISP_BNLC_N_PARAM_L2;
	union _isp_bnlc_n_param_l3_tag* reg3_ptr=(union _isp_bnlc_n_param_l3_tag*)ISP_BNLC_N_PARAM_L3;
	union _isp_bnlc_n_param_l4_tag* reg4_ptr=(union _isp_bnlc_n_param_l4_tag*)ISP_BNLC_N_PARAM_L4;
	union _isp_bnlc_n_param_l5_tag* reg5_ptr=(union _isp_bnlc_n_param_l5_tag*)ISP_BNLC_N_PARAM_L5;
	union _isp_bnlc_n_param_l6_tag* reg6_ptr=(union _isp_bnlc_n_param_l6_tag*)ISP_BNLC_N_PARAM_L6;
	union _isp_bnlc_n_param_l7_tag* reg7_ptr=(union _isp_bnlc_n_param_l7_tag*)ISP_BNLC_N_PARAM_L7;
	union _isp_bnlc_n_param_l8_tag* reg8_ptr=(union _isp_bnlc_n_param_l8_tag*)ISP_BNLC_N_PARAM_L8;
*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_bnlc_n_param_l0_tag* reg0_s_ptr=(union _isp_bnlc_n_param_l0_tag*)&isp_reg_ptr->BNLC_N_PARAM_L0;
	union _isp_bnlc_n_param_l1_tag* reg1_s_ptr=(union _isp_bnlc_n_param_l1_tag*)&isp_reg_ptr->BNLC_N_PARAM_L1;
	union _isp_bnlc_n_param_l2_tag* reg2_s_ptr=(union _isp_bnlc_n_param_l2_tag*)&isp_reg_ptr->BNLC_N_PARAM_L2;
	union _isp_bnlc_n_param_l3_tag* reg3_s_ptr=(union _isp_bnlc_n_param_l3_tag*)&isp_reg_ptr->BNLC_N_PARAM_L3;
	union _isp_bnlc_n_param_l4_tag* reg4_s_ptr=(union _isp_bnlc_n_param_l4_tag*)&isp_reg_ptr->BNLC_N_PARAM_L4;
	union _isp_bnlc_n_param_l5_tag* reg5_s_ptr=(union _isp_bnlc_n_param_l5_tag*)&isp_reg_ptr->BNLC_N_PARAM_L5;
	union _isp_bnlc_n_param_l6_tag* reg6_s_ptr=(union _isp_bnlc_n_param_l6_tag*)&isp_reg_ptr->BNLC_N_PARAM_L6;
	union _isp_bnlc_n_param_l7_tag* reg7_s_ptr=(union _isp_bnlc_n_param_l7_tag*)&isp_reg_ptr->BNLC_N_PARAM_L7;
	union _isp_bnlc_n_param_l8_tag* reg8_s_ptr=(union _isp_bnlc_n_param_l8_tag*)&isp_reg_ptr->BNLC_N_PARAM_L8;
	struct isp_reg_bits reg_config[9];
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg0_s_ptr->mBits.nlc_l_node0=l_node_ptr[1];
	reg0_s_ptr->mBits.nlc_l_node1=l_node_ptr[2];
	reg0_s_ptr->mBits.nlc_l_node2=l_node_ptr[3];
	reg1_s_ptr->mBits.nlc_l_node3=l_node_ptr[4];
	reg1_s_ptr->mBits.nlc_l_node4=l_node_ptr[5];
	reg1_s_ptr->mBits.nlc_l_node5=l_node_ptr[6];
	reg2_s_ptr->mBits.nlc_l_node6=l_node_ptr[7];
	reg2_s_ptr->mBits.nlc_l_node7=l_node_ptr[8];
	reg2_s_ptr->mBits.nlc_l_node8=l_node_ptr[9];
	reg3_s_ptr->mBits.nlc_l_node9=l_node_ptr[10];
	reg3_s_ptr->mBits.nlc_l_node10=l_node_ptr[11];
	reg3_s_ptr->mBits.nlc_l_node11=l_node_ptr[12];
	reg4_s_ptr->mBits.nlc_l_node12=l_node_ptr[13];
	reg4_s_ptr->mBits.nlc_l_node13=l_node_ptr[14];
	reg4_s_ptr->mBits.nlc_l_node14=l_node_ptr[15];
	reg5_s_ptr->mBits.nlc_l_node15=l_node_ptr[16];
	reg5_s_ptr->mBits.nlc_l_node16=l_node_ptr[17];
	reg5_s_ptr->mBits.nlc_l_node17=l_node_ptr[18];
	reg6_s_ptr->mBits.nlc_l_node18=l_node_ptr[19];
	reg6_s_ptr->mBits.nlc_l_node19=l_node_ptr[20];
	reg6_s_ptr->mBits.nlc_l_node20=l_node_ptr[21];
	reg7_s_ptr->mBits.nlc_l_node21=l_node_ptr[22];
	reg7_s_ptr->mBits.nlc_l_node22=l_node_ptr[23];
	reg7_s_ptr->mBits.nlc_l_node23=l_node_ptr[24];
	reg8_s_ptr->mBits.nlc_l_node24=l_node_ptr[25];
	reg8_s_ptr->mBits.nlc_l_node25=l_node_ptr[26];
	reg8_s_ptr->mBits.nlc_l_node26=l_node_ptr[27];
/*
	reg0_ptr->dwValue=reg0_s_ptr->dwValue;
	reg1_ptr->dwValue=reg1_s_ptr->dwValue;
	reg2_ptr->dwValue=reg2_s_ptr->dwValue;
	reg3_ptr->dwValue=reg3_s_ptr->dwValue;
	reg4_ptr->dwValue=reg4_s_ptr->dwValue;
	reg5_ptr->dwValue=reg5_s_ptr->dwValue;
	reg6_ptr->dwValue=reg6_s_ptr->dwValue;
	reg7_ptr->dwValue=reg7_s_ptr->dwValue;
	reg8_ptr->dwValue=reg8_s_ptr->dwValue;
*/

	if(SC8825_ISP_ID==isp_id)
	{
		reg_config[0].reg_addr = ISP_BNLC_N_PARAM_L0 - ISP_BASE_ADDR;
		reg_config[0].reg_value = isp_reg_ptr->BNLC_N_PARAM_L0;
		reg_config[1].reg_addr = ISP_BNLC_N_PARAM_L1 - ISP_BASE_ADDR;
		reg_config[1].reg_value = isp_reg_ptr->BNLC_N_PARAM_L1;
		reg_config[2].reg_addr = ISP_BNLC_N_PARAM_L2 - ISP_BASE_ADDR;
		reg_config[2].reg_value = isp_reg_ptr->BNLC_N_PARAM_L2;
		reg_config[3].reg_addr = ISP_BNLC_N_PARAM_L3 - ISP_BASE_ADDR;
		reg_config[3].reg_value = isp_reg_ptr->BNLC_N_PARAM_L3;
		reg_config[4].reg_addr = ISP_BNLC_N_PARAM_L4 - ISP_BASE_ADDR;
		reg_config[4].reg_value = isp_reg_ptr->BNLC_N_PARAM_L4;
		reg_config[5].reg_addr = ISP_BNLC_N_PARAM_L5 - ISP_BASE_ADDR;
		reg_config[5].reg_value = isp_reg_ptr->BNLC_N_PARAM_L5;
		reg_config[6].reg_addr = ISP_BNLC_N_PARAM_L6 - ISP_BASE_ADDR;
		reg_config[6].reg_value = isp_reg_ptr->BNLC_N_PARAM_L6;
		reg_config[7].reg_addr = ISP_BNLC_N_PARAM_L7 - ISP_BASE_ADDR;
		reg_config[7].reg_value = isp_reg_ptr->BNLC_N_PARAM_L7;
		reg_config[8].reg_addr = ISP_BNLC_N_PARAM_L8 - ISP_BASE_ADDR;
		reg_config[8].reg_value = isp_reg_ptr->BNLC_N_PARAM_L8;

		write_param.reg_param = (unsigned long)&reg_config[0];
		write_param.counts = 9;
	}
	else if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
	/*
		reg0_ptr->dwValue=reg0_s_ptr->dwValue;
		reg1_ptr->dwValue=reg1_s_ptr->dwValue;
		reg2_ptr->dwValue=reg2_s_ptr->dwValue;
		reg3_ptr->dwValue=reg3_s_ptr->dwValue;
		reg4_ptr->dwValue=reg4_s_ptr->dwValue;
		reg5_ptr->dwValue=reg5_s_ptr->dwValue;
		reg6_ptr->dwValue=reg6_s_ptr->dwValue;
		reg7_ptr->dwValue=reg7_s_ptr->dwValue;
		reg8_ptr->dwValue=reg8_s_ptr->dwValue;
	*/
		reg_config[0].reg_addr = ISP_NLC_N_PARAM_L0_V0001 - ISP_BASE_ADDR;
		reg_config[0].reg_value = isp_reg_ptr->BNLC_N_PARAM_L0;
		reg_config[1].reg_addr = ISP_NLC_N_PARAM_L1_V0001 - ISP_BASE_ADDR;
		reg_config[1].reg_value = isp_reg_ptr->BNLC_N_PARAM_L1;
		reg_config[2].reg_addr = ISP_NLC_N_PARAM_L2_V0001 - ISP_BASE_ADDR;
		reg_config[2].reg_value = isp_reg_ptr->BNLC_N_PARAM_L2;
		reg_config[3].reg_addr = ISP_NLC_N_PARAM_L3_V0001 - ISP_BASE_ADDR;
		reg_config[3].reg_value = isp_reg_ptr->BNLC_N_PARAM_L3;
		reg_config[4].reg_addr = ISP_NLC_N_PARAM_L4_V0001 - ISP_BASE_ADDR;
		reg_config[4].reg_value = isp_reg_ptr->BNLC_N_PARAM_L4;
		reg_config[5].reg_addr = ISP_NLC_N_PARAM_L5_V0001 - ISP_BASE_ADDR;
		reg_config[5].reg_value = isp_reg_ptr->BNLC_N_PARAM_L5;
		reg_config[6].reg_addr = ISP_NLC_N_PARAM_L6_V0001 - ISP_BASE_ADDR;
		reg_config[6].reg_value = isp_reg_ptr->BNLC_N_PARAM_L6;
		reg_config[7].reg_addr = ISP_NLC_N_PARAM_L7_V0001 - ISP_BASE_ADDR;
		reg_config[7].reg_value = isp_reg_ptr->BNLC_N_PARAM_L7;
		reg_config[8].reg_addr = ISP_NLC_N_PARAM_L8_V0001 - ISP_BASE_ADDR;
		reg_config[8].reg_value = isp_reg_ptr->BNLC_N_PARAM_L8;
	}

	write_param.reg_param = (unsigned long)&reg_config[0];
	write_param.counts = 9;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetBNLCSliceSize(uint32_t handler_id, uint16_t w, uint16_t h)
{
/*	union _isp_bnlc_slice_size_tag* reg_ptr=(union _isp_bnlc_slice_size_tag*)ISP_BLC_SLICE_SIZE_V0001;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_bnlc_slice_size_tag* reg_s_ptr=(union _isp_bnlc_slice_size_tag*)&isp_reg_ptr->BNLC_SLICE_SIZE;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	ISP_CHECK_FD;

	reg_s_ptr->mBits.slice_width=w;
	reg_s_ptr->mBits.slice_height=h;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	if(SC8825_ISP_ID==isp_id)
	{
		reg_config.reg_addr = ISP_BNLC_SLICE_SIZE - ISP_BASE_ADDR;
	}
	else if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
		reg_config.reg_addr = ISP_BLC_SLICE_SIZE_V0001 - ISP_BASE_ADDR;
	}
	reg_config.reg_value = isp_reg_ptr->BNLC_SLICE_SIZE;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetBNLCSliceInfo(uint32_t handler_id, uint8_t edge_info)
{
/*	union _isp_bnlc_slice_info_tag* reg_ptr=(union _isp_bnlc_slice_info_tag*)ISP_BLC_SLICE_INFO_V0001;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_bnlc_slice_info_tag* reg_s_ptr=(union _isp_bnlc_slice_info_tag*)&isp_reg_ptr->BNLC_SLICE_INFO;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	ISP_CHECK_FD;

	reg_s_ptr->mBits.edge_info=edge_info;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	if(SC8825_ISP_ID==isp_id)
	{
		reg_config.reg_addr = ISP_BNLC_SLICE_INFO - ISP_BASE_ADDR;
	}
	else if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
		reg_config.reg_addr = ISP_BLC_SLICE_INFO_V0001 - ISP_BASE_ADDR;
	}
	reg_config.reg_value = isp_reg_ptr->BNLC_SLICE_INFO;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/* Lens */
/*	--
*@
*@
*/
int32_t ispGetLensStatus(uint32_t handler_id, uint32_t* status)
{
/*	union _isp_lens_status_tag* reg_ptr=(union _isp_lens_status_tag*)ISP_LENS_STATUS;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_lens_status_tag* reg_s_ptr=(union _isp_lens_status_tag*)&isp_reg_ptr->LENS_STATUS;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;

	ISP_CHECK_FD;

	reg_config.reg_addr = ISP_LENS_STATUS - ISP_BASE_ADDR;
	read_param.reg_param = (unsigned long)&reg_config;
	read_param.counts = 1;
/*	reg_s_ptr->dwValue=reg_ptr->dwValue;*/

	if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		reg_s_ptr->dwValue = reg_config.reg_value;
		*status=reg_s_ptr->dwValue;
	}
	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispLensBypass(uint32_t handler_id, uint8_t bypass)
{
/*	union _isp_lens_param_tag* reg_ptr=(union _isp_lens_param_tag*)ISP_LENS_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_lens_param_tag* reg_s_ptr=(union _isp_lens_param_tag*)&isp_reg_ptr->LENS_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.bypass=bypass;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_LENS_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->LENS_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispLensBufSel(uint32_t handler_id, uint8_t buf_mode)
{
/*	union _isp_lens_param_tag* reg_ptr=(union _isp_lens_param_tag*)ISP_LENS_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_lens_param_tag* reg_s_ptr=(union _isp_lens_param_tag*)&isp_reg_ptr->LENS_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.use_buf_sel=buf_mode;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_LENS_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->LENS_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispLensParamAddr(uint32_t handler_id, unsigned long param_addr)
{
/*	union _isp_lens_param_addr_tag* reg_ptr=(union _isp_lens_param_addr_tag*)ISP_LENS_PARAM_ADDR;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_lens_param_addr_tag* reg_s_ptr=(union _isp_lens_param_addr_tag*)&isp_reg_ptr->LENS_PARAM_ADDR;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.grid_addr=param_addr;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_LENS_PARAM_ADDR - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->LENS_PARAM_ADDR;

	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetLensSliceStart(uint32_t handler_id, uint16_t x, uint16_t y)
{
/*	union _isp_lens_slice_pos_tag* reg_ptr=(union _isp_lens_slice_pos_tag*)ISP_LENS_SLICE_POS;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_lens_slice_pos_tag* reg_s_ptr=(union _isp_lens_slice_pos_tag*)&isp_reg_ptr->LENS_SLICE_POS;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.offset_x=x;
	reg_s_ptr->mBits.offset_y=y;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_LENS_SLICE_POS - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->LENS_SLICE_POS;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetLensLoaderEnable(uint32_t handler_id, unsigned long param_addr)
{
/*	union _isp_lens_loader_eb_tag* reg_ptr=(union _isp_lens_loader_eb_tag*)ISP_LENS_LOADER_ENABLE;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_lens_param_addr_tag* reg0_s_ptr=(union _isp_lens_param_addr_tag*)&isp_reg_ptr->LENS_PARAM_ADDR;
	union _isp_lens_loader_eb_tag* reg1_s_ptr=(union _isp_lens_loader_eb_tag*)&isp_reg_ptr->LENS_LOADER_EB;
	struct isp_reg_bits reg_config[2];
	struct isp_reg_param write_param;
	int32_t ret=ISP_SUCCESS;

	ISP_CHECK_FD;

	reg0_s_ptr->mBits.grid_addr=param_addr;
/*	reg0_ptr->dwValue=reg0_s_ptr->dwValue;*/
	reg_config[0].reg_addr = ISP_LENS_PARAM_ADDR - ISP_BASE_ADDR;
	reg_config[0].reg_value = isp_reg_ptr->LENS_PARAM_ADDR;

	reg1_s_ptr->mBits.eb=ISP_ONE;
/*	reg1_ptr->dwValue=reg1_s_ptr->dwValue;*/
	reg_config[1].reg_addr = ISP_LENS_LOADER_ENABLE - ISP_BASE_ADDR;
	reg_config[1].reg_value = isp_reg_ptr->LENS_LOADER_EB;

	write_param.reg_param = (unsigned long)&reg_config[0];
	write_param.counts = 2;

	if (-1 == ioctl(s_isp_fd, ISP_IO_LNC, (uint32_t *)&write_param)) {
		ret = -1;
	}

	return ret;
}

/*	--
*@
*@
*/
int32_t ispSetLensGridPitch(uint32_t handler_id, uint16_t pitch)
{
/*	union _isp_lens_grid_pitch_tag* reg_ptr=(union _isp_lens_grid_pitch_tag*)ISP_LENS_GRID_PITCH;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_lens_grid_pitch_tag* reg_s_ptr=(union _isp_lens_grid_pitch_tag*)&isp_reg_ptr->LENS_GRID_PITCH;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.grid_pitch=pitch;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_LENS_GRID_PITCH - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->LENS_GRID_PITCH;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetLensGridMode(uint32_t handler_id, uint8_t mode)
{
/*	union _isp_lens_grid_pitch_tag* reg_ptr=(union _isp_lens_grid_pitch_tag*)ISP_LENS_GRID_PITCH;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_lens_grid_pitch_tag* reg_s_ptr=(union _isp_lens_grid_pitch_tag*)&isp_reg_ptr->LENS_GRID_PITCH;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.grid_mode=mode;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_LENS_GRID_PITCH - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->LENS_GRID_PITCH;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetLensGridSize(uint32_t handler_id, uint16_t w, uint16_t h)
{
/*	union _isp_lens_grid_size_tag* reg_ptr=(union _isp_lens_grid_size_tag*)ISP_LENS_GRID_SIZE;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_lens_grid_size_tag* reg_s_ptr=(union _isp_lens_grid_size_tag*)&isp_reg_ptr->LENS_GRID_SIZE;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.slice_width=w;
	reg_s_ptr->mBits.slice_height=h;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_LENS_GRID_SIZE - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->LENS_GRID_SIZE;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetLensBuf(uint32_t handler_id, uint8_t buf_sel)
{
/*	union _isp_lens_load_buf_tag* reg_ptr=(union _isp_lens_load_buf_tag*)ISP_LENS_LOAD_BUF;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_lens_load_buf_tag* reg_s_ptr=(union _isp_lens_load_buf_tag*)&isp_reg_ptr->LENS_LOAD_BUF;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.load_buf_sel=buf_sel;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_LENS_LOAD_BUF - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->LENS_LOAD_BUF;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetLensEndian(uint32_t handler_id, uint8_t endian)
{
/*	union _isp_lens_misc_tag* reg_ptr=(union _isp_lens_misc_tag*)ISP_LENS_MISC;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_lens_misc_tag* reg_s_ptr=(union _isp_lens_misc_tag*)&isp_reg_ptr->LENS_MISC;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.lens_endian_type=endian;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_LENS_MISC - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->LENS_MISC;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispLensSliceSize(uint32_t handler_id, uint16_t w, uint16_t h)
{
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_lens_slice_size_tag* reg_s_ptr=(union _isp_lens_slice_size_tag*)&isp_reg_ptr->LENS_SLICE_SIZE;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_s_ptr->mBits.slice_width=w;
		reg_s_ptr->mBits.slice_height=h;
		reg_config.reg_addr = ISP_LENS_SLICE_SIZE - ISP_BASE_ADDR;
		reg_config.reg_value = isp_reg_ptr->LENS_SLICE_SIZE;
		write_param.reg_param = (unsigned long)&reg_config;
		write_param.counts = 1;

		return _isp_write((uint32_t *)&write_param);
	}
	return ISP_SUCCESS;
}

/*AWBM */
/*	--
*@
*@
*/
int32_t ispGetAwbmStatus(uint32_t handler_id, uint32_t* status)
{
/*	union _isp_awbm_status_tag* reg_ptr=(union _isp_awbm_status_tag*)ISP_AWBM_STATUS;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_awbm_status_tag* reg_s_ptr=(union _isp_awbm_status_tag*)&isp_reg_ptr->AWBM_STATUS;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;

	ISP_CHECK_FD;

	reg_config.reg_addr = ISP_AWBM_STATUS - ISP_BASE_ADDR;
	read_param.reg_param = (unsigned long)&reg_config;
	read_param.counts = 1;

/*	reg_s_ptr->dwValue=reg_ptr->dwValue;*/
	if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		reg_s_ptr->dwValue = reg_config.reg_value;
		*status=reg_s_ptr->dwValue;
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispAwbmBypass(uint32_t handler_id, uint8_t bypass)
{
/*	union _isp_awbm_param_tag* reg_ptr=(union _isp_awbm_param_tag*)ISP_AWBM_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_awbm_param_tag* reg_s_ptr=(union _isp_awbm_param_tag*)&isp_reg_ptr->AWBM_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.bypass=bypass;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_AWBM_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->AWBM_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispAwbmSkip(uint32_t handler_id, uint8_t num)
{
/*	union _isp_awbm_param_tag* reg_ptr=(union _isp_awbm_param_tag*)ISP_AWBM_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_awbm_param_tag* reg_s_ptr=(union _isp_awbm_param_tag*)&isp_reg_ptr->AWBM_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_s_ptr->mBits.skip_num=num;
	/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
		reg_config.reg_addr = ISP_AWBM_PARAM - ISP_BASE_ADDR;
		reg_config.reg_value = reg_s_ptr->dwValue;
		write_param.reg_param = (unsigned long)&reg_config;
		write_param.counts = 1;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispAwbmMode(uint32_t handler_id, uint8_t mode)
{
/*	union _isp_awbm_param_tag* reg_ptr=(union _isp_awbm_param_tag*)ISP_AWBM_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_awbm_param_tag* reg_s_ptr=(union _isp_awbm_param_tag*)&isp_reg_ptr->AWBM_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.mode=mode;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_AWBM_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->AWBM_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetAwbmWinStart(uint32_t handler_id, uint16_t x, uint16_t y)
{
/*	union _isp_awbm_offset_tag* reg_ptr=(union _isp_awbm_offset_tag*)ISP_AWBM_OFFSET;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_awbm_offset_tag* reg_s_ptr=(union _isp_awbm_offset_tag*)&isp_reg_ptr->AWBM_OFFSET;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.offset_x=x;
	reg_s_ptr->mBits.offset_y=y;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_AWBM_OFFSET - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->AWBM_OFFSET;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetAwbmWinSize(uint32_t handler_id, uint16_t w, uint16_t h)
{
/*	union _isp_awbm_blk_size_tag* reg_ptr=(union _isp_awbm_blk_size_tag*)ISP_AWBM_BLK_SIZE;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	ISP_CHECK_FD;
	if(SC8825_ISP_ID==isp_id || SC8830_ISP_ID==isp_id)
	{
		union _isp_awbm_blk_size_tag* reg_s_ptr=(union _isp_awbm_blk_size_tag*)&isp_reg_ptr->AWBM_BLK_SIZE;
		reg_s_ptr->mBits.blk_width=w;
		reg_s_ptr->mBits.blk_height=h;

	}
	else if(SC9630_ISP_ID==isp_id)
	{
		union _isp_awbm_blk_size_v0002_tag* reg_s_ptr=(union _isp_awbm_blk_size_v0002_tag*)&isp_reg_ptr->AWBM_BLK_SIZE;
		reg_s_ptr->mBits.blk_width=w;
		reg_s_ptr->mBits.blk_height=h;
	}
	/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_AWBM_BLK_SIZE - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->AWBM_BLK_SIZE;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);



}

/*	--
*@
*@
*/
int32_t ispSetAwbmShift(uint32_t handler_id, uint16_t shift)
{
/*	union _isp_awbm_blk_size_tag* reg_ptr=(union _isp_awbm_blk_size_tag*)ISP_AWBM_BLK_SIZE;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	uint32_t isp_id=_isp_GetIspId();

	ISP_CHECK_FD;
	if(SC8825_ISP_ID==isp_id || SC8830_ISP_ID==isp_id)
	{
		union _isp_awbm_blk_size_tag* reg_s_ptr=(union _isp_awbm_blk_size_tag*)&isp_reg_ptr->AWBM_BLK_SIZE;
		reg_s_ptr->mBits.awbm_avgshf=shift;

	}
	else if(SC9630_ISP_ID==isp_id)
	{
		union _isp_awbm_blk_size_v0002_tag* reg_s_ptr=(union _isp_awbm_blk_size_v0002_tag*)&isp_reg_ptr->AWBM_BLK_SIZE;
		reg_s_ptr->mBits.awbm_avgshf=shift;
	}

/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_AWBM_BLK_SIZE - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->AWBM_BLK_SIZE;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*AWBC */
/*	--
*@
*@
*/
int32_t ispGetAwbcStatus(uint32_t handler_id, uint32_t* status)
{
/*	union _isp_awbc_status_tag* reg_ptr=(union _isp_awbc_status_tag*)ISP_AWBC_STATUS;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_awbc_status_tag* reg_s_ptr=(union _isp_awbc_status_tag*)&isp_reg_ptr->AWBC_STATUS;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;

	ISP_CHECK_FD;

	reg_config.reg_addr = ISP_AWBC_STATUS - ISP_BASE_ADDR;
	read_param.reg_param = (unsigned long)&reg_config;
	read_param.counts = 1;

/*	reg_s_ptr->dwValue=reg_ptr->dwValue;*/
	if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		reg_s_ptr->dwValue = reg_config.reg_value;
		*status=reg_s_ptr->dwValue;
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispAwbcBypass(uint32_t handler_id, uint8_t bypass)
{
/*	union _isp_awbc_param_tag* reg_ptr=(union _isp_awbc_param_tag*)ISP_AWBC_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_awbc_param_tag* reg_s_ptr=(union _isp_awbc_param_tag*)&isp_reg_ptr->AWBC_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.bypass=bypass;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_AWBC_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->AWBC_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetAwbGain(uint32_t handler_id, uint16_t r_gain, uint16_t g_gain, uint16_t b_gain)
{
/*	union _isp_awbc_gain0_tag* reg0_ptr=(union _isp_awbc_gain0_tag*)ISP_AWBC_GAIN0;
	union _isp_awbc_gain1_tag* reg1_ptr=(union _isp_awbc_gain1_tag*)ISP_AWBC_GAIN1;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	struct isp_reg_bits reg_config[2];
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	ISP_CHECK_FD;

	if(SC8825_ISP_ID==isp_id)
	{
		union _isp_awbc_gain0_tag* reg0_s_ptr=(union _isp_awbc_gain0_tag*)&isp_reg_ptr->AWBC_GAIN0;
		union _isp_awbc_gain1_tag* reg1_s_ptr=(union _isp_awbc_gain1_tag*)&isp_reg_ptr->AWBC_GAIN1;
		reg0_s_ptr->mBits.r_gain=r_gain;
		reg0_s_ptr->mBits.b_gain=b_gain;
		reg1_s_ptr->mBits.g_gain=g_gain;
	}
	else if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
		union _isp_awbc_gain0_v0001_tag* reg0_s_ptr=(union _isp_awbc_gain0_v0001_tag*)&isp_reg_ptr->AWBC_GAIN0;
		union _isp_awbc_gain1_v0001_tag* reg1_s_ptr=(union _isp_awbc_gain1_v0001_tag*)&isp_reg_ptr->AWBC_GAIN1;
		reg0_s_ptr->mBits.r_gain=r_gain;
		reg0_s_ptr->mBits.b_gain=b_gain;
		reg1_s_ptr->mBits.gr_gain=g_gain;
		reg1_s_ptr->mBits.gb_gain=g_gain;
	}
/*	reg0_ptr->dwValue=reg0_s_ptr->dwValue;
	reg1_ptr->dwValue=reg1_s_ptr->dwValue;*/

	reg_config[0].reg_addr = ISP_AWBC_GAIN0 - ISP_BASE_ADDR;
	reg_config[0].reg_value = isp_reg_ptr->AWBC_GAIN0;
	reg_config[1].reg_addr = ISP_AWBC_GAIN1 - ISP_BASE_ADDR;
	reg_config[1].reg_value = isp_reg_ptr->AWBC_GAIN1;

	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 2;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetAwbGainOffset(uint32_t handler_id, uint16_t r_offset, uint16_t g_offset, uint16_t b_offset)
{
/*	union _isp_awbc_offset0_v0001_tag* reg0_ptr=(union _isp_awbc_offset0_v0001_tag*)ISP_AWBC_OFFSET0_V0001;
	union _isp_awbc_offset1_v0001_tag* reg1_ptr=(union _isp_awbc_offset1_v0001_tag*)ISP_AWBC_OFFSET1_V0001;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_awbc_offset0_v0001_tag* reg0_s_ptr=(union _isp_awbc_offset0_v0001_tag*)&isp_reg_ptr->AWBC_OFFSET0;
	union _isp_awbc_offset1_v0001_tag* reg1_s_ptr=(union _isp_awbc_offset1_v0001_tag*)&isp_reg_ptr->AWBC_OFFSET1;
	struct isp_reg_bits reg_config[2];
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg0_s_ptr->mBits.r_offset=r_offset;
	reg0_s_ptr->mBits.b_offset=b_offset;
	reg1_s_ptr->mBits.gr_offset=g_offset;
	reg1_s_ptr->mBits.gb_offset=g_offset;
/*	reg0_ptr->dwValue=reg0_s_ptr->dwValue;
	reg1_ptr->dwValue=reg1_s_ptr->dwValue;*/
	reg_config[0].reg_addr = ISP_AWBC_OFFSET0_V0001 - ISP_BASE_ADDR;
	reg_config[0].reg_value = isp_reg_ptr->AWBC_OFFSET0;
	reg_config[1].reg_addr = ISP_AWBC_OFFSET1_V0001 - ISP_BASE_ADDR;
	reg_config[1].reg_value = isp_reg_ptr->AWBC_OFFSET1;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 2;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetAwbGainThrd(uint32_t handler_id, uint16_t r_thr, uint16_t g_thr, uint16_t b_thr)
{
/*	union _isp_awbc_thrd_tag* reg_ptr=(union _isp_awbc_thrd_tag*)ISP_AWBC_THRD;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_awbc_thrd_tag* reg_s_ptr=(union _isp_awbc_thrd_tag*)&isp_reg_ptr->AWBC_THRD;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.r_thr=r_thr;
	reg_s_ptr->mBits.g_thr=g_thr;
	reg_s_ptr->mBits.b_thr=b_thr;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_AWBC_THRD - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->AWBC_THRD;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
void ispGetAWBMStatistic(uint32_t handler_id, uint32_t* r_info, uint32_t* g_info, uint32_t* b_info)
{
/*	union _isp_mem_reg_tag* reg_ptr=(union _isp_mem_reg_tag*)ISP_AWBM_OUTPUT;*/
	uint32_t i = 0x00;
	uint32_t k = 0x00;
	uint32_t temp32_0 = 0x00;
	uint32_t temp32_1 = 0x00;
	uint32_t offset_addr = ISP_AWBM_OUTPUT - ISP_BASE_ADDR;
	struct isp_reg_bits reg_config[2*ISP_AWBM_ITEM];
	struct isp_reg_param read_param;
	uint32_t isp_id=_isp_GetIspId();

/*	ISP_CHECK_FD;*/

	for(i=0x00; i<2*ISP_AWBM_ITEM; i++) {
		reg_config[i].reg_addr = offset_addr;
		offset_addr += 4;
	}
	read_param.reg_param = (unsigned long)&reg_config;
	read_param.counts = 2*ISP_AWBM_ITEM;

	switch(isp_id)
	{
		case SC8825_ISP_ID:
		{
			if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param))
			{
				for(i=0x00; i<ISP_AWBM_ITEM; i++)
				{
					temp32_0=reg_config[k++].reg_value;
					temp32_1=reg_config[k++].reg_value;
					r_info[i]= (temp32_1>>9) & 0xfffff;
					g_info[i]= temp32_0 & 0x1fffff;
					b_info[i]= ((temp32_1 & 0x1ff)<<11) | ((temp32_0 >>21) & 0x7ff);
				}
			}
			break ;
		}
		case SC8830_ISP_ID:
		case SC9630_ISP_ID:
		{
			if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param))
			{
				for(i=0x00; i<ISP_AWBM_ITEM; i++)
				{
					temp32_0=reg_config[k++].reg_value;
					temp32_1=reg_config[k++].reg_value;
					r_info[i]= (temp32_1>>11) & 0x1fffff;
					g_info[i]= temp32_0 & 0x3fffff;
					b_info[i]= ((temp32_1 & 0x7ff)<<10) | ((temp32_0 >>22) & 0x3ff);
				}
			}
			break ;
		}
		defalult:
		{
			break ;
		}
	}

}

/*BPC */
/*	--
*@
*@
*/
int32_t ispGetBpcStatus(uint32_t handler_id, uint32_t* status)
{
/*	union _isp_bpc_status_tag* reg_ptr=(union _isp_bpc_status_tag*)ISP_BPC_STATUS;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_bpc_status_tag* reg_s_ptr=(union _isp_bpc_status_tag*)&isp_reg_ptr->BPC_STATUS;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;

	ISP_CHECK_FD;

	reg_config.reg_addr = ISP_BPC_STATUS - ISP_BASE_ADDR;
	read_param.reg_param = (unsigned long)&reg_config;
	read_param.counts = 1;

/*	reg_s_ptr->dwValue=reg_ptr->dwValue;*/
	if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		reg_s_ptr->dwValue = reg_config.reg_value;
		*status = reg_s_ptr->dwValue;
	}
	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispBpcBypass(uint32_t handler_id, uint8_t bypass)
{
/*	union _isp_bpc_param_tag* reg_ptr=(union _isp_bpc_param_tag*)ISP_BPC_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_bpc_param_tag* reg_s_ptr=(union _isp_bpc_param_tag*)&isp_reg_ptr->BPC_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.bypass=bypass;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_BPC_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->BPC_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispBpcMode(uint32_t handler_id, uint16_t mode)
{
/*	union _isp_bpc_param_tag* reg_ptr=(union _isp_bpc_param_tag*)ISP_BPC_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_bpc_param_tag* reg_s_ptr=(union _isp_bpc_param_tag*)&isp_reg_ptr->BPC_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.mode=mode;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_BPC_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->BPC_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

int32_t ispBpcPatternThr(uint32_t handler_id, uint16_t pattern_type, uint16_t maxminThr, uint16_t super_badThr)
{
/*	union _isp_bpc_param_tag* reg_ptr=(union _isp_bpc_param_tag*)ISP_BPC_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_bpc_param_tag* reg_s_ptr=(union _isp_bpc_param_tag*)&isp_reg_ptr->BPC_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_s_ptr->mBits.pattern_type = pattern_type;
		reg_s_ptr->mBits.maxminThr = maxminThr;
		reg_s_ptr->mBits.super_badThr = super_badThr;
		/*reg_ptr->dwValue=reg_s_ptr->dwValue;*/
		reg_config.reg_addr = ISP_BPC_PARAM - ISP_BASE_ADDR;
		reg_config.reg_value = isp_reg_ptr->BPC_PARAM;
		write_param.reg_param = (unsigned long)&reg_config;
		write_param.counts = 1;

		return _isp_write((uint32_t *)&write_param);
	}
	return ISP_SUCCESS;


}

/*	--
*@
*@
*/
int32_t ispSetBpcThrd(uint32_t handler_id, uint16_t flat_thr, uint16_t std_thr, uint16_t texture_thr)
{
/*	union _isp_bpc_thrd_tag* reg_ptr=(union _isp_bpc_thrd_tag*)ISP_AWBC_THRD;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_bpc_thrd_tag* reg_s_ptr=(union _isp_bpc_thrd_tag*)&isp_reg_ptr->BPC_THRD;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.flat_thr=flat_thr;
	reg_s_ptr->mBits.std_thr=std_thr;
	reg_s_ptr->mBits.texture_thr=texture_thr;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_BPC_THRD - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->BPC_THRD;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispBpcMapAddr(uint32_t handler_id, uint32_t map_addr)
{
/*	union _isp_bpc_map_addr_tag* reg_ptr=(union _isp_bpc_map_addr_tag*)ISP_BPC_MAP_ADDR;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_bpc_map_addr_tag* reg_s_ptr=(union _isp_bpc_map_addr_tag*)&isp_reg_ptr->BPC_MAP_ADDR;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.map_addr=map_addr;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_BPC_MAP_ADDR - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->BPC_MAP_ADDR;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispBpcPixelNum(uint32_t handler_id, uint32_t pixel_num)
{
/*	union _isp_bpc_pixel_num_tag* reg_ptr=(union _isp_bpc_pixel_num_tag*)ISP_BPC_PIXEL_NUM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_bpc_pixel_num_tag* reg_s_ptr=(union _isp_bpc_pixel_num_tag*)&isp_reg_ptr->BPC_PIXEL_NUM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.pixel_num=pixel_num;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_BPC_PIXEL_NUM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->BPC_PIXEL_NUM;
	write_param.reg_param = (uint32_t)&reg_config;
	write_param.counts = 1;
	return _isp_write((uint32_t *)&write_param);
}
/*NBPC */
/*	--
*@
*@
*/
int32_t ispGetNBpcStatus(uint32_t handler_id, uint32_t* status)
{
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_nbpc_status_v0002_tag* reg_s_ptr=(union _isp_nbpc_status_v0002_tag*)&isp_reg_ptr->NBPC_STATUS;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;
	ISP_CHECK_FD;
	reg_config.reg_addr = ISP_NBPC_STATUS_V0002 - ISP_BASE_ADDR;
	read_param.reg_param = (uint32_t)&reg_config;
	read_param.counts = 1;
	if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		reg_s_ptr->dwValue = reg_config.reg_value;
		*status = reg_s_ptr->dwValue;
	}
	return ISP_SUCCESS;
}
int32_t ispNBpcBypass(uint32_t handler_id, uint8_t bypass)
{
	struct isp_reg* isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_nbpc_param_v0002_tag* reg_s_ptr=(union _isp_nbpc_param_v0002_tag*)&isp_reg_ptr->NBPC_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	ISP_CHECK_FD;
	reg_s_ptr->mBits.bypass = bypass;
	reg_config.reg_addr = ISP_NBPC_PARAM_V0002 - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->NBPC_PARAM;
	write_param.reg_param = (uint32_t)&reg_config;
	write_param.counts = 1;
	return _isp_write((uint32_t *)&write_param);
}
int32_t ispNBpcPvdBypass(uint32_t handler_id, uint8_t bypass)
{
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_nbpc_param_v0002_tag* reg_s_ptr=(union _isp_nbpc_param_v0002_tag*)&isp_reg_ptr->NBPC_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	ISP_CHECK_FD;
	reg_s_ptr->mBits.bypass_pvd = bypass;
	reg_config.reg_addr = ISP_NBPC_PARAM_V0002 - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->NBPC_PARAM;
	write_param.reg_param = (uint32_t)&reg_config;
	write_param.counts = 1;
	return _isp_write((uint32_t *)&write_param);
}
int32_t ispSetNBpcMode(uint32_t handler_id, uint8_t mode)
{
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_nbpc_param_v0002_tag* reg_s_ptr=(union _isp_nbpc_param_v0002_tag*)&isp_reg_ptr->NBPC_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	ISP_CHECK_FD;
	reg_s_ptr->mBits.mode = mode;
	reg_config.reg_addr = ISP_NBPC_PARAM_V0002 - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->NBPC_PARAM;
	write_param.reg_param = (uint32_t)&reg_config;
	write_param.counts = 1;
	return _isp_write((uint32_t *)&write_param);
}
int32_t ispSetNBpcMaskMode(uint32_t handler_id, uint8_t mode)
{
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_nbpc_param_v0002_tag* reg_s_ptr=(union _isp_nbpc_param_v0002_tag*)&isp_reg_ptr->NBPC_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	ISP_CHECK_FD;
	reg_s_ptr->mBits.mask_mode = mode;
	reg_config.reg_addr = ISP_NBPC_PARAM_V0002 - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->NBPC_PARAM;
	write_param.reg_param = (uint32_t)&reg_config;
	write_param.counts = 1;
	return _isp_write((uint32_t *)&write_param);
}
int32_t ispSetNBpcKThr(uint32_t handler_id, uint8_t kmix, uint8_t kmax)
{
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_nbpc_param_v0002_tag* reg_s_ptr=(union _isp_nbpc_param_v0002_tag*)&isp_reg_ptr->NBPC_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	ISP_CHECK_FD;
	reg_s_ptr->mBits.kmin = kmix;
	reg_s_ptr->mBits.kmax = kmax;
	reg_config.reg_addr = ISP_NBPC_PARAM_V0002 - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->NBPC_PARAM;
	write_param.reg_param = (uint32_t)&reg_config;
	write_param.counts = 1;
	return _isp_write((uint32_t *)&write_param);
}
int32_t ispSetNBpcThrd(uint32_t handler_id, uint8_t cntr_thr)
{
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_nbpc_cfg_v0002_tag* reg_s_ptr=(union _isp_nbpc_cfg_v0002_tag*)&isp_reg_ptr->NBPC_CFG;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	ISP_CHECK_FD;
	reg_s_ptr->mBits.cntr_theshold = cntr_thr;
	reg_config.reg_addr = ISP_NBPC_CFG_V0002 - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->NBPC_CFG;
	write_param.reg_param = (uint32_t)&reg_config;
	write_param.counts = 1;
	return _isp_write((uint32_t *)&write_param);
}
int32_t ispSetNBpcHWClrEn(uint32_t handler_id, uint8_t hw_fifo_en)
{
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_nbpc_cfg_v0002_tag* reg_s_ptr=(union _isp_nbpc_cfg_v0002_tag*)&isp_reg_ptr->NBPC_CFG;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	ISP_CHECK_FD;
	reg_s_ptr->mBits.map_hw_fifo_clr_en = hw_fifo_en;
	reg_config.reg_addr = ISP_NBPC_CFG_V0002 - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->NBPC_CFG;
	write_param.reg_param = (uint32_t)&reg_config;
	write_param.counts = 1;
	return _isp_write((uint32_t *)&write_param);
}
int32_t ispSetNBpcEstimate14(uint32_t handler_id, uint8_t ktimes)
{
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_nbpc_cfg_v0002_tag* reg_s_ptr=(union _isp_nbpc_cfg_v0002_tag*)&isp_reg_ptr->NBPC_CFG;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	ISP_CHECK_FD;
	reg_s_ptr->mBits.ktimes = ktimes;
	reg_config.reg_addr = ISP_NBPC_CFG_V0002 - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->NBPC_CFG;
	write_param.reg_param = (uint32_t)&reg_config;
	write_param.counts = 1;
	return _isp_write((uint32_t *)&write_param);
}
int32_t ispSetNBpcFifoClr(uint32_t handler_id, uint8_t fifoclr)
{
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_nbpc_cfg_v0002_tag* reg_s_ptr=(union _isp_nbpc_cfg_v0002_tag*)&isp_reg_ptr->NBPC_CFG;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	ISP_CHECK_FD;
	reg_s_ptr->mBits.map_fifo_clr = fifoclr;
	reg_config.reg_addr = ISP_NBPC_CFG_V0002 - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->NBPC_CFG;
	write_param.reg_param = (uint32_t)&reg_config;
	write_param.counts = 1;
	return _isp_write((uint32_t *)&write_param);
}
int32_t ispSetNBpcDelt34(uint32_t handler_id, uint8_t delt34)
{
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_nbpc_cfg_v0002_tag* reg_s_ptr=(union _isp_nbpc_cfg_v0002_tag*)&isp_reg_ptr->NBPC_CFG;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	ISP_CHECK_FD;
	reg_s_ptr->mBits.delt34 = delt34;
	reg_config.reg_addr = ISP_NBPC_CFG_V0002 - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->NBPC_CFG;
	write_param.reg_param = (uint32_t)&reg_config;
	write_param.counts = 1;
	return _isp_write((uint32_t *)&write_param);
}
int32_t ispNBpcPixelNum(uint32_t handler_id, uint32_t pixel_num)
{
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_nbpc_cfg_v0002_tag* reg_s_ptr = (union _isp_nbpc_cfg_v0002_tag*)&isp_reg_ptr->NBPC_PIXEL_NUM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	ISP_CHECK_FD;
	reg_s_ptr->mBits.bad_pixel_num = pixel_num;
	reg_config.reg_addr = ISP_NBPC_CFG_V0002 - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->NBPC_PIXEL_NUM;
	write_param.reg_param = (uint32_t)&reg_config;
	write_param.counts = 1;
	return _isp_write((uint32_t *)&write_param);
}
int32_t ispNBpcFactor(uint32_t handler_id,  uint8_t flat_fct, uint8_t safe_fct)
{
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_nbpc_factor_v0002_tag* reg_s_ptr = (union _isp_nbpc_factor_v0002_tag*)&isp_reg_ptr->NBPC_FACTOR;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	ISP_CHECK_FD;
	reg_s_ptr->mBits.safe_factor = safe_fct;
	reg_s_ptr->mBits.flat_factor = flat_fct;
	reg_config.reg_addr = ISP_NBPC_FACTOR_V0002 - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->NBPC_FACTOR;
	write_param.reg_param = (uint32_t)&reg_config;
	write_param.counts = 1;
	return _isp_write((uint32_t *)&write_param);
}
int32_t ispNBpCoeff(uint32_t handler_id, uint8_t spike_coeff, uint8_t dead_coeff)
{
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_nbpc_coeff_v0002_tag* reg_s_ptr = (union _isp_nbpc_coeff_v0002_tag*)&isp_reg_ptr->NBPC_COEFF;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	ISP_CHECK_FD;
	reg_s_ptr->mBits.dead_coeff = dead_coeff;
	reg_s_ptr->mBits.spike_coeff = spike_coeff;
	reg_config.reg_addr = ISP_NBPC_COEFF_V0002 - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->NBPC_COEFF;
	write_param.reg_param = (uint32_t)&reg_config;
	write_param.counts = 1;
	return _isp_write((uint32_t *)&write_param);
}
int32_t ispNBpCLutword(uint32_t handler_id, uint16_t *intercept_b, uint16_t *slope_k, uint16_t *lut_level)
{
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_nbpc_lut0_v0002_tag* reg_s_ptr0 = (union _isp_nbpc_lut0_v0002_tag*)&isp_reg_ptr->NBPC_LUTWORD0;
	union _isp_nbpc_lut1_v0002_tag* reg_s_ptr1 = (union _isp_nbpc_lut1_v0002_tag*)&isp_reg_ptr->NBPC_LUTWORD1;
	union _isp_nbpc_lut2_v0002_tag* reg_s_ptr2 = (union _isp_nbpc_lut2_v0002_tag*)&isp_reg_ptr->NBPC_LUTWORD2;
	union _isp_nbpc_lut3_v0002_tag* reg_s_ptr3 = (union _isp_nbpc_lut3_v0002_tag*)&isp_reg_ptr->NBPC_LUTWORD3;
	union _isp_nbpc_lut4_v0002_tag* reg_s_ptr4 = (union _isp_nbpc_lut4_v0002_tag*)&isp_reg_ptr->NBPC_LUTWORD4;
	union _isp_nbpc_lut5_v0002_tag* reg_s_ptr5 = (union _isp_nbpc_lut5_v0002_tag*)&isp_reg_ptr->NBPC_LUTWORD5;
	union _isp_nbpc_lut6_v0002_tag* reg_s_ptr6 = (union _isp_nbpc_lut6_v0002_tag*)&isp_reg_ptr->NBPC_LUTWORD6;
	union _isp_nbpc_lut7_v0002_tag* reg_s_ptr7 = (union _isp_nbpc_lut7_v0002_tag*)&isp_reg_ptr->NBPC_LUTWORD7;
	struct isp_reg_param write_param;
	struct isp_reg_bits reg_config[8];
	ISP_CHECK_FD;
	reg_s_ptr0->mBits.intercept_b0 = intercept_b[0];
	reg_s_ptr0->mBits.slope_k0 = slope_k[0];
	reg_s_ptr0->mBits.lut_level0 = lut_level[0];
	reg_s_ptr1->mBits.intercept_b1 = intercept_b[1];
	reg_s_ptr1->mBits.slope_k1 = slope_k[1];
	reg_s_ptr1->mBits.lut_level1 = lut_level[1];
	reg_s_ptr2->mBits.intercept_b2 = intercept_b[2];
	reg_s_ptr2->mBits.slope_k2 = slope_k[2];
	reg_s_ptr2->mBits.lut_level2 = lut_level[2];
	reg_s_ptr3->mBits.intercept_b3 = intercept_b[3];
	reg_s_ptr3->mBits.slope_k3 = slope_k[3];
	reg_s_ptr3->mBits.lut_level3 = lut_level[3];
	reg_s_ptr4->mBits.intercept_b4 = intercept_b[4];
	reg_s_ptr4->mBits.slope_k4 = slope_k[4];
	reg_s_ptr4->mBits.lut_level4 = lut_level[4];
	reg_s_ptr5->mBits.intercept_b5 = intercept_b[5];
	reg_s_ptr5->mBits.slope_k5 = slope_k[5];
	reg_s_ptr5->mBits.lut_level5 = lut_level[5];
	reg_s_ptr6->mBits.intercept_b6 =intercept_b[6];
	reg_s_ptr6->mBits.slope_k6 = slope_k[6];
	reg_s_ptr6->mBits.lut_level6 = lut_level[6];
	reg_s_ptr7->mBits.intercept_b7 = intercept_b[7];
	reg_s_ptr7->mBits.slope_k7 = slope_k[7];
	reg_s_ptr7->mBits.lut_level7 = lut_level[7];
	reg_config[0].reg_addr = ISP_NBPC_LUT0_V0002 - ISP_BASE_ADDR;
	reg_config[0].reg_value = isp_reg_ptr->NBPC_LUTWORD0;
	reg_config[1].reg_addr = ISP_NBPC_LUT1_V0002 - ISP_BASE_ADDR;
	reg_config[1].reg_value = isp_reg_ptr->NBPC_LUTWORD1;
	reg_config[2].reg_addr = ISP_NBPC_LUT2_V0002 - ISP_BASE_ADDR;
	reg_config[2].reg_value = isp_reg_ptr->NBPC_LUTWORD2;
	reg_config[3].reg_addr = ISP_NBPC_LUT3_V0002 - ISP_BASE_ADDR;
	reg_config[3].reg_value = isp_reg_ptr->NBPC_LUTWORD3;
	reg_config[4].reg_addr = ISP_NBPC_LUT4_V0002 - ISP_BASE_ADDR;
	reg_config[4].reg_value = isp_reg_ptr->NBPC_LUTWORD4;
	reg_config[5].reg_addr = ISP_NBPC_LUT5_V0002 - ISP_BASE_ADDR;
	reg_config[5].reg_value = isp_reg_ptr->NBPC_LUTWORD5;
	reg_config[6].reg_addr = ISP_NBPC_LUT6_V0002 - ISP_BASE_ADDR;
	reg_config[6].reg_value = isp_reg_ptr->NBPC_LUTWORD6;
	reg_config[7].reg_addr = ISP_NBPC_LUT7_V0002 - ISP_BASE_ADDR;
	reg_config[7].reg_value = isp_reg_ptr->NBPC_LUTWORD7;
	write_param.reg_param = (uint32_t)&reg_config;
	write_param.counts = 8;
	return _isp_write((uint32_t *)&write_param);
}
int32_t ispNBPCMapDownSel(uint32_t handler_id, uint8_t mapsel)
{
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_bpc_new_old_sel_tag* reg_s_ptr = (union _isp_bpc_new_old_sel_tag*)&isp_reg_ptr->NBPC_NEW_OLD_SEL;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	ISP_CHECK_FD;
	reg_s_ptr->mBits.map_down_sel = mapsel;
	reg_config.reg_addr = ISP_NBPC_SEL_V0002 - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->NBPC_NEW_OLD_SEL;
	write_param.reg_param = (uint32_t)&reg_config;
	write_param.counts = 1;
	return _isp_write((uint32_t *)&write_param);
}
int32_t ispNBPCSel(uint32_t handler_id, uint8_t select)
{
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_bpc_new_old_sel_tag* reg_s_ptr = (union _isp_bpc_new_old_sel_tag*)&isp_reg_ptr->NBPC_NEW_OLD_SEL;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	ISP_CHECK_FD;
	reg_s_ptr->mBits.new_old_sel = select;
	reg_config.reg_addr = ISP_NBPC_SEL_V0002 - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->NBPC_NEW_OLD_SEL;
	write_param.reg_param = (uint32_t)&reg_config;
	write_param.counts = 1;
	return _isp_write((uint32_t *)&write_param);
}
int32_t ispNBpcMapAddr(uint32_t handler_id, uint32_t map_addr)
{
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_nbpc_map_addr_tag* reg_s_ptr=(union _isp_nbpc_map_addr_tag*)&isp_reg_ptr->NBPC_MAP_ADDR;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	ISP_CHECK_FD;
	reg_s_ptr->mBits.map_addr=map_addr;
	reg_config.reg_addr = ISP_NBPC_MAP_ADDR_V0002 - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->NBPC_MAP_ADDR;
	write_param.reg_param = (uint32_t)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*Wave denoise */
/*	--
*@
*@
*/
int32_t ispGetWDenoiseStatus(uint32_t handler_id, uint32_t* status)
{
/*	union _isp_wave_status_tag* reg_ptr=(union _isp_wave_status_tag*)ISP_WAVE_STATUS;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_wave_status_tag* reg_s_ptr=(union _isp_wave_status_tag*)&isp_reg_ptr->WAVE_STATUS;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;

	ISP_CHECK_FD;

	reg_config.reg_addr = ISP_WAVE_STATUS - ISP_BASE_ADDR;
	read_param.reg_param = (unsigned long)&reg_config;
	read_param.counts = 1;

/*	reg_s_ptr->dwValue=reg_ptr->dwValue;*/
	if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		reg_s_ptr->dwValue = reg_config.reg_value;
		*status = reg_s_ptr->dwValue;
	}
	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispWDenoiseBypass(uint32_t handler_id, uint8_t bypass)
{
/*	union _isp_wave_param_tag* reg_ptr=(union _isp_wave_param_tag*)ISP_WAVE_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_wave_param_tag* reg_s_ptr=(union _isp_wave_param_tag*)&isp_reg_ptr->WAVE_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.bypass=bypass;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_WAVE_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->WAVE_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispWDenoiseWriteBack(uint32_t handler_id, uint16_t write_back)
{
/*	union _isp_wave_param_tag* reg_ptr=(union _isp_wave_param_tag*)ISP_WAVE_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_wave_param_tag* reg_s_ptr=(union _isp_wave_param_tag*)&isp_reg_ptr->WAVE_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8825_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_s_ptr->mBits.write_back=write_back;
		/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
		reg_config.reg_addr = ISP_WAVE_PARAM - ISP_BASE_ADDR;
		reg_config.reg_value = isp_reg_ptr->WAVE_PARAM;
		write_param.reg_param = (unsigned long)&reg_config;
		write_param.counts = 1;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispWDenoiseThrd(uint32_t handler_id, uint16_t r_thr, uint16_t g_thr, uint16_t b_thr)
{
/*	union _isp_wave_thrd_v0000_tag* reg_ptr=(union _isp_wave_thrd_v0000_tag*)ISP_AWBC_THRD;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_wave_thrd_v0000_tag* reg_s_ptr=(union _isp_wave_thrd_v0000_tag*)&isp_reg_ptr->WAVE_THRD;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8825_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_s_ptr->mBits.r_thr=r_thr;
		reg_s_ptr->mBits.g_thr=g_thr;
		reg_s_ptr->mBits.b_thr=b_thr;
		/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
		reg_config.reg_addr = ISP_WAVE_THRD_V0000 - ISP_BASE_ADDR;
		reg_config.reg_value = isp_reg_ptr->WAVE_THRD;
		write_param.reg_param = (unsigned long)&reg_config;
		write_param.counts = 1;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispWDenoiseSliceSize(uint32_t handler_id, uint16_t w, uint16_t h)
{
/*	union _isp_wave_slice_size_tag* reg_ptr=(union _isp_wave_slice_size_tag*)ISP_WAVE_SLICE_SIZE;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_wave_slice_size_tag* reg_s_ptr=(union _isp_wave_slice_size_tag*)&isp_reg_ptr->WAVE_SLICE_SIZE;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.slice_width=w;
	reg_s_ptr->mBits.slice_height=h;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_WAVE_SLICE_SIZE - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->WAVE_SLICE_SIZE;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispWDenoiseSliceInfo(uint32_t handler_id, uint16_t edge_info)
{
/*	union _isp_wave_slice_info_tag* reg_ptr=(union _isp_wave_slice_info_tag*)ISP_WAVE_SLICE_INFO;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_wave_slice_info_tag* reg_s_ptr=(union _isp_wave_slice_info_tag*)&isp_reg_ptr->WAVE_SLICE_INFO;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	ISP_CHECK_FD;

	/* sharkl don't have this register */
	if(SC9630_ISP_ID != isp_id)
	{
		reg_s_ptr->mBits.edge_info=edge_info;
	/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
		reg_config.reg_addr = ISP_WAVE_SLICE_INFO - ISP_BASE_ADDR;
		reg_config.reg_value = isp_reg_ptr->WAVE_SLICE_INFO;
		write_param.reg_param = (unsigned long)&reg_config;
		write_param.counts = 1;

		return _isp_write((uint32_t *)&write_param);
	}
	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispWDenoiseDiswei(uint32_t handler_id, uint8_t* diswei_ptr)
{
/*	union _isp_wave_diswei_0_tag* reg0_ptr=(union _isp_wave_diswei_0_tag*)ISP_WAVE_DISWEI_0;
	union _isp_wave_diswei_1_tag* reg1_ptr=(union _isp_wave_diswei_1_tag*)ISP_WAVE_DISWEI_1;
	union _isp_wave_diswei_2_tag* reg2_ptr=(union _isp_wave_diswei_2_tag*)ISP_WAVE_DISWEI_2;
	union _isp_wave_diswei_3_tag* reg3_ptr=(union _isp_wave_diswei_3_tag*)ISP_WAVE_DISWEI_3;
	union _isp_wave_diswei_4_tag* reg4_ptr=(union _isp_wave_diswei_4_tag*)ISP_WAVE_DISWEI_4;
*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_wave_diswei_0_tag* reg0_s_ptr=(union _isp_wave_diswei_0_tag*)&isp_reg_ptr->WAVE_DISWEI_0;
	union _isp_wave_diswei_1_tag* reg1_s_ptr=(union _isp_wave_diswei_1_tag*)&isp_reg_ptr->WAVE_DISWEI_1;
	union _isp_wave_diswei_2_tag* reg2_s_ptr=(union _isp_wave_diswei_2_tag*)&isp_reg_ptr->WAVE_DISWEI_2;
	union _isp_wave_diswei_3_tag* reg3_s_ptr=(union _isp_wave_diswei_3_tag*)&isp_reg_ptr->WAVE_DISWEI_3;
	union _isp_wave_diswei_4_tag* reg4_s_ptr=(union _isp_wave_diswei_4_tag*)&isp_reg_ptr->WAVE_DISWEI_4;
	struct isp_reg_bits reg_config[5];
	struct isp_reg_param write_param;
	uint32_t reg_val[5] = {0}, i = 0, j = 0;

	ISP_CHECK_FD;

	while(i<16)
	{
		reg_val[j] = (diswei_ptr[i+3]<<24)
			|(diswei_ptr[i+2]<<16)
			|(diswei_ptr[i+1]<<8)
			|(diswei_ptr[i]);
		i+=4;
		++j;
	}

	reg_val[4] = (diswei_ptr[18]<<16)|(diswei_ptr[17]<<8)|(diswei_ptr[16]);

	reg0_s_ptr->mBits.disweight3_0=reg_val[0];
	reg1_s_ptr->mBits.disweight7_4=reg_val[1];
	reg2_s_ptr->mBits.disweight11_8=reg_val[2];
	reg3_s_ptr->mBits.disweight15_12=reg_val[3];
	reg4_s_ptr->mBits.disweight18_16=reg_val[4];
/*
	reg0_ptr->dwValue=reg0_s_ptr->dwValue;
	reg1_ptr->dwValue=reg1_s_ptr->dwValue;
	reg2_ptr->dwValue=reg2_s_ptr->dwValue;
	reg3_ptr->dwValue=reg3_s_ptr->dwValue;
	reg4_ptr->dwValue=reg4_s_ptr->dwValue;
*/
	reg_config[0].reg_addr = ISP_WAVE_DISWEI_0 - ISP_BASE_ADDR;
	reg_config[0].reg_value = isp_reg_ptr->WAVE_DISWEI_0;
	reg_config[1].reg_addr = ISP_WAVE_DISWEI_1 - ISP_BASE_ADDR;
	reg_config[1].reg_value = isp_reg_ptr->WAVE_DISWEI_1;
	reg_config[2].reg_addr = ISP_WAVE_DISWEI_2 - ISP_BASE_ADDR;
	reg_config[2].reg_value = isp_reg_ptr->WAVE_DISWEI_2;
	reg_config[3].reg_addr = ISP_WAVE_DISWEI_3 - ISP_BASE_ADDR;
	reg_config[3].reg_value = isp_reg_ptr->WAVE_DISWEI_3;
	reg_config[4].reg_addr = ISP_WAVE_DISWEI_4 - ISP_BASE_ADDR;
	reg_config[4].reg_value = isp_reg_ptr->WAVE_DISWEI_4;

	write_param.reg_param = (unsigned long)&reg_config[0];
	write_param.counts = 5;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispWDenoiseRanwei(uint32_t handler_id, uint8_t* ranwei_ptr)
{
/*	union _isp_wave_ranwei_0_tag* reg0_ptr=(union _isp_wave_ranwei_0_tag*)ISP_WAVE_RANWEI_0;
	union _isp_wave_ranwei_1_tag* reg1_ptr=(union _isp_wave_ranwei_1_tag*)ISP_WAVE_RANWEI_1;
	union _isp_wave_ranwei_2_tag* reg2_ptr=(union _isp_wave_ranwei_2_tag*)ISP_WAVE_RANWEI_2;
	union _isp_wave_ranwei_3_tag* reg3_ptr=(union _isp_wave_ranwei_3_tag*)ISP_WAVE_RANWEI_3;
	union _isp_wave_ranwei_4_tag* reg4_ptr=(union _isp_wave_ranwei_4_tag*)ISP_WAVE_RANWEI_4;
	union _isp_wave_ranwei_5_tag* reg5_ptr=(union _isp_wave_ranwei_5_tag*)ISP_WAVE_RANWEI_5;
	union _isp_wave_ranwei_6_tag* reg6_ptr=(union _isp_wave_ranwei_6_tag*)ISP_WAVE_RANWEI_6;
	union _isp_wave_ranwei_7_tag* reg7_ptr=(union _isp_wave_ranwei_7_tag*)ISP_WAVE_RANWEI_7;
*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_wave_ranwei_0_tag* reg0_s_ptr=(union _isp_wave_ranwei_0_tag*)&isp_reg_ptr->WAVE_RANWEI_0;
	union _isp_wave_ranwei_1_tag* reg1_s_ptr=(union _isp_wave_ranwei_1_tag*)&isp_reg_ptr->WAVE_RANWEI_1;
	union _isp_wave_ranwei_2_tag* reg2_s_ptr=(union _isp_wave_ranwei_2_tag*)&isp_reg_ptr->WAVE_RANWEI_2;
	union _isp_wave_ranwei_3_tag* reg3_s_ptr=(union _isp_wave_ranwei_3_tag*)&isp_reg_ptr->WAVE_RANWEI_3;
	union _isp_wave_ranwei_4_tag* reg4_s_ptr=(union _isp_wave_ranwei_4_tag*)&isp_reg_ptr->WAVE_RANWEI_4;
	union _isp_wave_ranwei_5_tag* reg5_s_ptr=(union _isp_wave_ranwei_5_tag*)&isp_reg_ptr->WAVE_RANWEI_5;
	union _isp_wave_ranwei_6_tag* reg6_s_ptr=(union _isp_wave_ranwei_6_tag*)&isp_reg_ptr->WAVE_RANWEI_6;
	union _isp_wave_ranwei_7_tag* reg7_s_ptr=(union _isp_wave_ranwei_7_tag*)&isp_reg_ptr->WAVE_RANWEI_7;
	struct isp_reg_bits reg_config[8];
	struct isp_reg_param write_param;
	uint32_t reg_val[8] = {0x00}, i = 0x00, j = 0x00;

	ISP_CHECK_FD;

	while(i<28)
	{
		reg_val[j] =( ranwei_ptr[i+3]<<24)
			|(ranwei_ptr[i+2]<<16)
			|(ranwei_ptr[i+1]<<8)
			|(ranwei_ptr[i]);
		++j;
		i+=4;
	}
	reg_val[7] = (ranwei_ptr[30]<<16)
		|(ranwei_ptr[29]<<8)
		|(ranwei_ptr[28]);

	reg0_s_ptr->mBits.ranweight3_0=reg_val[0];
	reg1_s_ptr->mBits.ranweight7_4=reg_val[1];
	reg2_s_ptr->mBits.ranweight11_8=reg_val[2];
	reg3_s_ptr->mBits.ranweight15_12=reg_val[3];
	reg4_s_ptr->mBits.ranweight19_16=reg_val[4];
	reg5_s_ptr->mBits.ranweight23_20=reg_val[5];
	reg6_s_ptr->mBits.ranweight27_24=reg_val[6];
	reg7_s_ptr->mBits.ranweight30_28=reg_val[7];
/*
	reg0_ptr->dwValue=reg0_s_ptr->dwValue;
	reg1_ptr->dwValue=reg1_s_ptr->dwValue;
	reg2_ptr->dwValue=reg2_s_ptr->dwValue;
	reg3_ptr->dwValue=reg3_s_ptr->dwValue;
	reg4_ptr->dwValue=reg4_s_ptr->dwValue;
	reg5_ptr->dwValue=reg5_s_ptr->dwValue;
	reg6_ptr->dwValue=reg6_s_ptr->dwValue;
	reg7_ptr->dwValue=reg7_s_ptr->dwValue;
*/
	reg_config[0].reg_addr = ISP_WAVE_RANWEI_0 - ISP_BASE_ADDR;
	reg_config[0].reg_value = isp_reg_ptr->WAVE_RANWEI_0;
	reg_config[1].reg_addr = ISP_WAVE_RANWEI_1 - ISP_BASE_ADDR;
	reg_config[1].reg_value = isp_reg_ptr->WAVE_RANWEI_1;
	reg_config[2].reg_addr = ISP_WAVE_RANWEI_2 - ISP_BASE_ADDR;
	reg_config[2].reg_value = isp_reg_ptr->WAVE_RANWEI_2;
	reg_config[3].reg_addr = ISP_WAVE_RANWEI_3 - ISP_BASE_ADDR;
	reg_config[3].reg_value = isp_reg_ptr->WAVE_RANWEI_3;
	reg_config[4].reg_addr = ISP_WAVE_RANWEI_4 - ISP_BASE_ADDR;
	reg_config[4].reg_value = isp_reg_ptr->WAVE_RANWEI_4;
	reg_config[5].reg_addr = ISP_WAVE_RANWEI_5 - ISP_BASE_ADDR;
	reg_config[5].reg_value = isp_reg_ptr->WAVE_RANWEI_5;
	reg_config[6].reg_addr = ISP_WAVE_RANWEI_6 - ISP_BASE_ADDR;
	reg_config[6].reg_value = isp_reg_ptr->WAVE_RANWEI_6;
	reg_config[7].reg_addr = ISP_WAVE_RANWEI_7 - ISP_BASE_ADDR;
	reg_config[7].reg_value = isp_reg_ptr->WAVE_RANWEI_7;

	write_param.reg_param = (unsigned long)&reg_config[0];
	write_param.counts = 8;

	return _isp_write((uint32_t *)&write_param);
}

/*Pre Wave denoise */
/*	--
*@
*@
*/
int32_t ispGetPreWDenoiseStatus(uint32_t handler_id, uint32_t* status)
{
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_pre_wave_status_v0002_tag* reg_s_ptr=(union _isp_pre_wave_status_v0002_tag*)&isp_reg_ptr->PRE_WAVE_STATUS;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_config.reg_addr = ISP_PRE_WAVELET_STATUS - ISP_BASE_ADDR;
		read_param.reg_param = (unsigned long)&reg_config;
		read_param.counts = 1;

		if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
			reg_s_ptr->dwValue = reg_config.reg_value;
			*status = reg_s_ptr->dwValue;
		}
	}
	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispPreWDenoiseBypass(uint32_t handler_id, uint8_t bypass)
{
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_pre_wave_param_v0002_tag* reg_s_ptr=(union _isp_pre_wave_param_v0002_tag*)&isp_reg_ptr->PRE_WAVE_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_s_ptr->mBits.bypass=bypass;
		reg_config.reg_addr = ISP_PRE_WAVELET_PARAM - ISP_BASE_ADDR;
		reg_config.reg_value = isp_reg_ptr->PRE_WAVE_PARAM;

		write_param.reg_param = (unsigned long)&reg_config;
		write_param.counts = 1;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

/*Pre wavelet para */
/*	--
*@
*@
*/
int32_t ispPreWDenoiseThrd(uint32_t handler_id, uint16_t thrs0, uint16_t thrs1)
{
/*	union _isp_wave_thrd_v0000_tag* reg_ptr=(union _isp_wave_thrd_v0000_tag*)ISP_AWBC_THRD;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_pre_wave_param_v0002_tag* reg_s_ptr=(union _isp_pre_wave_param_v0002_tag*)&isp_reg_ptr->PRE_WAVE_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_s_ptr->mBits.thrs0=thrs0;
		reg_s_ptr->mBits.thrs1=thrs1;
		reg_config.reg_addr = ISP_PRE_WAVELET_PARAM - ISP_BASE_ADDR;
		reg_config.reg_value = isp_reg_ptr->PRE_WAVE_PARAM;

		write_param.reg_param = (unsigned long)&reg_config;
		write_param.counts = 1;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

/*GrGb C */
/*	--
*@
*@
*/
int32_t ispGetGrGbStatus(uint32_t handler_id, uint32_t* status)
{
/*	union _isp_grgb_status_tag* reg_ptr=(union _isp_grgb_status_tag*)ISP_GRGB_STATUS;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_grgb_status_tag* reg_s_ptr=(union _isp_grgb_status_tag*)&isp_reg_ptr->GRGB_STATUS;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;

	ISP_CHECK_FD;

	reg_config.reg_addr = ISP_GRGB_STATUS - ISP_BASE_ADDR;
	read_param.reg_param = (unsigned long)&reg_config;
	read_param.counts = 1;

/*	reg_s_ptr->dwValue=reg_ptr->dwValue;*/
	if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		reg_s_ptr->dwValue = reg_config.reg_value;
		*status = reg_s_ptr->dwValue;
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispGrGbbypass(uint32_t handler_id, uint8_t bypass)
{
/*	union _isp_grgb_param_tag* reg_ptr=(union _isp_grgb_param_tag*)ISP_GRGB_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_grgb_param_tag* reg_s_ptr=(union _isp_grgb_param_tag*)&isp_reg_ptr->GRGB_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.bypass=bypass;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_GRGB_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->GRGB_PARAM;

	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetGrGbThrd(uint32_t handler_id, uint16_t edge_thr, uint16_t diff_thr)
{
/*	union _isp_grgb_param_tag* reg_ptr=(union _isp_grgb_param_tag*)ISP_GRGB_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_grgb_param_tag* reg_s_ptr=(union _isp_grgb_param_tag*)&isp_reg_ptr->GRGB_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.edge_thr=edge_thr;
	reg_s_ptr->mBits.diff_thr=diff_thr;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_GRGB_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->GRGB_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*CFA */
/*	--
*@
*@
*/
int32_t ispGetCFAStatus(uint32_t handler_id, uint32_t* status)
{
/*	union _isp_cfa_status_tag* reg_ptr=(union _isp_cfa_status_tag*)ISP_CFA_STATUS;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_cfa_status_tag* reg_s_ptr=(union _isp_cfa_status_tag*)&isp_reg_ptr->CFA_STATUS;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;

	ISP_CHECK_FD;

	reg_config.reg_addr = ISP_CFA_STATUS - ISP_BASE_ADDR;
	read_param.reg_param = (unsigned long)&reg_config;
	read_param.counts = 1;

/*	reg_s_ptr->dwValue=reg_ptr->dwValue;*/
	if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		reg_s_ptr->dwValue = reg_config.reg_value;
		*status = reg_s_ptr->dwValue;
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispSetCFAThrd(uint32_t handler_id, uint16_t edge_thr, uint16_t diff_thr)
{
/*	union _isp_cfa_param_tag* reg_ptr=(union _isp_cfa_param_tag*)ISP_CFA_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_cfa_param_tag* reg_s_ptr=(union _isp_cfa_param_tag*)&isp_reg_ptr->CFA_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.edge_thr=edge_thr;
	reg_s_ptr->mBits.diff_thr=diff_thr;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_CFA_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->CFA_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispCFASliceSize(uint32_t handler_id, uint16_t w, uint16_t h)
{
/*	union _isp_cfa_slice_size_tag* reg_ptr=(union _isp_cfa_slice_size_tag*)ISP_CFA_SLICE_SIZE;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_cfa_slice_size_tag* reg_s_ptr=(union _isp_cfa_slice_size_tag*)&isp_reg_ptr->CFA_SLICE_SIZE;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.slice_width=w;
	reg_s_ptr->mBits.slice_height=h;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_CFA_SLICE_SIZE - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->CFA_SLICE_SIZE;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispCFASliceInfo(uint32_t handler_id, uint16_t edge_info)
{
/*	union _isp_cfa_slice_info_tag* reg_ptr=(union _isp_cfa_slice_info_tag*)ISP_CFA_SLICE_INFO;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_cfa_slice_info_tag* reg_s_ptr=(union _isp_cfa_slice_info_tag*)&isp_reg_ptr->CFA_SLICE_INFO;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.edge_info=edge_info;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_CFA_SLICE_INFO - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->CFA_SLICE_INFO;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*CMC */
/*	--
*@
*@
*/
int32_t ispGetCMCStatus(uint32_t handler_id, uint32_t* status)
{
/*	union _isp_cmc_status_tag* reg_ptr=(union _isp_cmc_status_tag*)ISP_CMC_STATUS;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_cmc_status_tag* reg_s_ptr=(union _isp_cmc_status_tag*)&isp_reg_ptr->CMC_STATUS;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;

	ISP_CHECK_FD;

	reg_config.reg_addr = ISP_CMC_STATUS - ISP_BASE_ADDR;
	read_param.reg_param = (unsigned long)&reg_config;
	read_param.counts = 1;

/*	reg_s_ptr->dwValue=reg_ptr->dwValue;*/
	if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		reg_s_ptr->dwValue = reg_config.reg_value;
		*status = reg_s_ptr->dwValue;
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispCMCbypass(uint32_t handler_id, uint8_t bypass)
{
/*	union _isp_cmc_param_tag* reg_ptr=(union _isp_cmc_param_tag*)ISP_CMC_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_cmc_param_tag* reg_s_ptr=(union _isp_cmc_param_tag*)&isp_reg_ptr->CMC_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.bypass=bypass;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_CMC_PARAM -ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->CMC_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetCMCMatrix(uint32_t handler_id, uint16_t* matrix_ptr)
{
/*	union _isp_cmc_matrix0_tag* reg0_ptr=(union _isp_cmc_matrix0_tag*)ISP_CMC_MATRIX0;
	union _isp_cmc_matrix1_tag* reg1_ptr=(union _isp_cmc_matrix1_tag*)ISP_CMC_MATRIX1;
	union _isp_cmc_matrix2_tag* reg2_ptr=(union _isp_cmc_matrix2_tag*)ISP_CMC_MATRIX2;
	union _isp_cmc_matrix3_tag* reg3_ptr=(union _isp_cmc_matrix3_tag*)ISP_CMC_MATRIX3;
	union _isp_cmc_matrix4_tag* reg4_ptr=(union _isp_cmc_matrix4_tag*)ISP_CMC_MATRIX4;
*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_cmc_matrix0_tag* reg0_s_ptr=(union _isp_cmc_matrix0_tag*)&isp_reg_ptr->CMC_MATRIX0;
	union _isp_cmc_matrix1_tag* reg1_s_ptr=(union _isp_cmc_matrix1_tag*)&isp_reg_ptr->CMC_MATRIX1;
	union _isp_cmc_matrix2_tag* reg2_s_ptr=(union _isp_cmc_matrix2_tag*)&isp_reg_ptr->CMC_MATRIX2;
	union _isp_cmc_matrix3_tag* reg3_s_ptr=(union _isp_cmc_matrix3_tag*)&isp_reg_ptr->CMC_MATRIX3;
	union _isp_cmc_matrix4_tag* reg4_s_ptr=(union _isp_cmc_matrix4_tag*)&isp_reg_ptr->CMC_MATRIX4;
	struct isp_reg_bits reg_config[5];
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg0_s_ptr->mBits.matrix00=matrix_ptr[0];
	reg0_s_ptr->mBits.matrix01=matrix_ptr[1];
	reg1_s_ptr->mBits.matrix02=matrix_ptr[2];
	reg1_s_ptr->mBits.matrix10=matrix_ptr[3];
	reg2_s_ptr->mBits.matrix11=matrix_ptr[4];
	reg2_s_ptr->mBits.matrix12=matrix_ptr[5];
	reg3_s_ptr->mBits.matrix20=matrix_ptr[6];
	reg3_s_ptr->mBits.matrix21=matrix_ptr[7];
	reg4_s_ptr->mBits.matrix22=matrix_ptr[8];
/*
	reg0_ptr->dwValue=reg0_s_ptr->dwValue;
	reg1_ptr->dwValue=reg1_s_ptr->dwValue;
	reg2_ptr->dwValue=reg2_s_ptr->dwValue;
	reg3_ptr->dwValue=reg3_s_ptr->dwValue;
	reg4_ptr->dwValue=reg4_s_ptr->dwValue;
*/
	reg_config[0].reg_addr = ISP_CMC_MATRIX0 - ISP_BASE_ADDR;
	reg_config[0].reg_value = isp_reg_ptr->CMC_MATRIX0;
	reg_config[1].reg_addr = ISP_CMC_MATRIX1 - ISP_BASE_ADDR;
	reg_config[1].reg_value = isp_reg_ptr->CMC_MATRIX1;
	reg_config[2].reg_addr = ISP_CMC_MATRIX2 - ISP_BASE_ADDR;
	reg_config[2].reg_value = isp_reg_ptr->CMC_MATRIX2;
	reg_config[3].reg_addr = ISP_CMC_MATRIX3 - ISP_BASE_ADDR;
	reg_config[3].reg_value = isp_reg_ptr->CMC_MATRIX3;
	reg_config[4].reg_addr = ISP_CMC_MATRIX4 - ISP_BASE_ADDR;
	reg_config[4].reg_value = isp_reg_ptr->CMC_MATRIX4;

	write_param.reg_param = (unsigned long)&reg_config[0];
	write_param.counts = 5;

	return _isp_write((uint32_t *)&write_param);
}

/*Gamma */
/*	--
*@
*@
*/
int32_t ispGetGammaStatus(uint32_t handler_id, uint32_t* status)
{
/*	union _isp_gamma_status_tag* reg_ptr=(union _isp_gamma_status_tag*)ISP_GAMMA_STATUS;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_gamma_status_tag* reg_s_ptr=(union _isp_gamma_status_tag*)&isp_reg_ptr->GAMMA_STATUS;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;

	ISP_CHECK_FD;

	reg_config.reg_addr = ISP_GAMMA_STATUS - ISP_BASE_ADDR;
	read_param.reg_param = (unsigned long)&reg_config;
	read_param.counts = 1;

/*	reg_s_ptr->dwValue=reg_ptr->dwValue;*/
	if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		reg_s_ptr->dwValue = reg_config.reg_value;
		*status = reg_s_ptr->dwValue;
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispGammabypass(uint32_t handler_id, uint8_t bypass)
{
/*	union _isp_gamma_param_tag* reg_ptr=(union _isp_gamma_param_tag*)ISP_GAMMA_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_gamma_param_tag* reg_s_ptr=(union _isp_gamma_param_tag*)&isp_reg_ptr->GAMMA_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.bypass=bypass;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_GAMMA_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->GAMMA_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetGammaXNode(uint32_t handler_id, uint16_t* node)
{
/*	union _isp_gamma_node_x0_tag* reg0_ptr=(union _isp_gamma_node_x0_tag*)ISP_GAMMA_NODE_X0;
	union _isp_gamma_node_x1_tag* reg1_ptr=(union _isp_gamma_node_x1_tag*)ISP_GAMMA_NODE_X1;
	union _isp_gamma_node_x2_tag* reg2_ptr=(union _isp_gamma_node_x2_tag*)ISP_GAMMA_NODE_X2;
	union _isp_gamma_node_x3_tag* reg3_ptr=(union _isp_gamma_node_x3_tag*)ISP_GAMMA_NODE_X3;
	union _isp_gamma_node_x4_tag* reg4_ptr=(union _isp_gamma_node_x4_tag*)ISP_GAMMA_NODE_X4;
	union _isp_gamma_node_x5_tag* reg5_ptr=(union _isp_gamma_node_x5_tag*)ISP_GAMMA_NODE_X5;
	union _isp_gamma_node_x6_tag* reg6_ptr=(union _isp_gamma_node_x6_tag*)ISP_GAMMA_NODE_X6;
	union _isp_gamma_node_x7_tag* reg7_ptr=(union _isp_gamma_node_x7_tag*)ISP_GAMMA_NODE_X7;
*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_gamma_node_x0_tag* reg0_s_ptr=(union _isp_gamma_node_x0_tag*)&isp_reg_ptr->GAMMA_NODE_X0;
	union _isp_gamma_node_x1_tag* reg1_s_ptr=(union _isp_gamma_node_x1_tag*)&isp_reg_ptr->GAMMA_NODE_X1;
	union _isp_gamma_node_x2_tag* reg2_s_ptr=(union _isp_gamma_node_x2_tag*)&isp_reg_ptr->GAMMA_NODE_X2;
	union _isp_gamma_node_x3_tag* reg3_s_ptr=(union _isp_gamma_node_x3_tag*)&isp_reg_ptr->GAMMA_NODE_X3;
	union _isp_gamma_node_x4_tag* reg4_s_ptr=(union _isp_gamma_node_x4_tag*)&isp_reg_ptr->GAMMA_NODE_X4;
	union _isp_gamma_node_x5_tag* reg5_s_ptr=(union _isp_gamma_node_x5_tag*)&isp_reg_ptr->GAMMA_NODE_X5;
	union _isp_gamma_node_x6_tag* reg6_s_ptr=(union _isp_gamma_node_x6_tag*)&isp_reg_ptr->GAMMA_NODE_X6;
	union _isp_gamma_node_x7_tag* reg7_s_ptr=(union _isp_gamma_node_x7_tag*)&isp_reg_ptr->GAMMA_NODE_X7;
	struct isp_reg_bits reg_config[8];
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg0_s_ptr->mBits.node1=node[1];
	reg0_s_ptr->mBits.node2=node[2];
	reg0_s_ptr->mBits.node3=node[3];
	reg1_s_ptr->mBits.node4=node[4];
	reg1_s_ptr->mBits.node5=node[5];
	reg1_s_ptr->mBits.node6=node[6];
	reg2_s_ptr->mBits.node7=node[7];
	reg2_s_ptr->mBits.node8=node[8];
	reg2_s_ptr->mBits.node9=node[9];
	reg3_s_ptr->mBits.node10=node[10];
	reg3_s_ptr->mBits.node11=node[11];
	reg3_s_ptr->mBits.node12=node[12];
	reg4_s_ptr->mBits.node13=node[13];
	reg4_s_ptr->mBits.node14=node[14];
	reg4_s_ptr->mBits.node15=node[15];
	reg5_s_ptr->mBits.node16=node[16];
	reg5_s_ptr->mBits.node17=node[17];
	reg5_s_ptr->mBits.node18=node[18];
	reg6_s_ptr->mBits.node19=node[19];
	reg6_s_ptr->mBits.node20=node[20];
	reg6_s_ptr->mBits.node21=node[21];
	reg7_s_ptr->mBits.node22=node[22];
	reg7_s_ptr->mBits.node23=node[23];
	reg7_s_ptr->mBits.node24=node[24];
/*
	reg0_ptr->dwValue=reg0_s_ptr->dwValue;
	reg1_ptr->dwValue=reg1_s_ptr->dwValue;
	reg2_ptr->dwValue=reg2_s_ptr->dwValue;
	reg3_ptr->dwValue=reg3_s_ptr->dwValue;
	reg4_ptr->dwValue=reg4_s_ptr->dwValue;
	reg5_ptr->dwValue=reg5_s_ptr->dwValue;
	reg6_ptr->dwValue=reg6_s_ptr->dwValue;
	reg7_ptr->dwValue=reg7_s_ptr->dwValue;
*/
	reg_config[0].reg_addr = ISP_GAMMA_NODE_X0 - ISP_BASE_ADDR;
	reg_config[0].reg_value = isp_reg_ptr->GAMMA_NODE_X0;
	reg_config[1].reg_addr = ISP_GAMMA_NODE_X1 - ISP_BASE_ADDR;
	reg_config[1].reg_value = isp_reg_ptr->GAMMA_NODE_X1;
	reg_config[2].reg_addr = ISP_GAMMA_NODE_X2 - ISP_BASE_ADDR;
	reg_config[2].reg_value = isp_reg_ptr->GAMMA_NODE_X2;
	reg_config[3].reg_addr = ISP_GAMMA_NODE_X3 - ISP_BASE_ADDR;
	reg_config[3].reg_value = isp_reg_ptr->GAMMA_NODE_X3;
	reg_config[4].reg_addr = ISP_GAMMA_NODE_X4 - ISP_BASE_ADDR;
	reg_config[4].reg_value = isp_reg_ptr->GAMMA_NODE_X4;
	reg_config[5].reg_addr = ISP_GAMMA_NODE_X5 - ISP_BASE_ADDR;
	reg_config[5].reg_value = isp_reg_ptr->GAMMA_NODE_X5;
	reg_config[6].reg_addr = ISP_GAMMA_NODE_X6 - ISP_BASE_ADDR;
	reg_config[6].reg_value = isp_reg_ptr->GAMMA_NODE_X6;
	reg_config[7].reg_addr = ISP_GAMMA_NODE_X7 - ISP_BASE_ADDR;
	reg_config[7].reg_value = isp_reg_ptr->GAMMA_NODE_X7;

	write_param.reg_param = (unsigned long)&reg_config[0];
	write_param.counts = 8;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetGammaYNode(uint32_t handler_id, uint16_t* node)
{
/*	union _isp_gamma_node_y0_tag* reg1_ptr=(union _isp_gamma_node_y0_tag*)ISP_GAMMA_NODE_Y0;
	union _isp_gamma_node_y1_tag* reg1_ptr=(union _isp_gamma_node_y1_tag*)ISP_GAMMA_NODE_Y1;
	union _isp_gamma_node_y2_tag* reg2_ptr=(union _isp_gamma_node_y2_tag*)ISP_GAMMA_NODE_Y2;
	union _isp_gamma_node_y3_tag* reg3_ptr=(union _isp_gamma_node_y3_tag*)ISP_GAMMA_NODE_Y3;
	union _isp_gamma_node_y4_tag* reg4_ptr=(union _isp_gamma_node_y4_tag*)ISP_GAMMA_NODE_Y4;
	union _isp_gamma_node_y5_tag* reg5_ptr=(union _isp_gamma_node_y5_tag*)ISP_GAMMA_NODE_Y5;
	union _isp_gamma_node_y6_tag* reg6_ptr=(union _isp_gamma_node_y6_tag*)ISP_GAMMA_NODE_Y6;
*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_gamma_node_y0_tag* reg0_s_ptr=(union _isp_gamma_node_y0_tag*)&isp_reg_ptr->GAMMA_NODE_Y0;
	union _isp_gamma_node_y1_tag* reg1_s_ptr=(union _isp_gamma_node_y1_tag*)&isp_reg_ptr->GAMMA_NODE_Y1;
	union _isp_gamma_node_y2_tag* reg2_s_ptr=(union _isp_gamma_node_y2_tag*)&isp_reg_ptr->GAMMA_NODE_Y2;
	union _isp_gamma_node_y3_tag* reg3_s_ptr=(union _isp_gamma_node_y3_tag*)&isp_reg_ptr->GAMMA_NODE_Y3;
	union _isp_gamma_node_y4_tag* reg4_s_ptr=(union _isp_gamma_node_y4_tag*)&isp_reg_ptr->GAMMA_NODE_Y4;
	union _isp_gamma_node_y5_tag* reg5_s_ptr=(union _isp_gamma_node_y5_tag*)&isp_reg_ptr->GAMMA_NODE_Y5;
	union _isp_gamma_node_y6_tag* reg6_s_ptr=(union _isp_gamma_node_y6_tag*)&isp_reg_ptr->GAMMA_NODE_Y6;
	struct isp_reg_bits reg_config[7];
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8825_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg0_s_ptr->mBits.node0=node[0];
		reg0_s_ptr->mBits.node1=node[1];
		reg0_s_ptr->mBits.node2=node[2];
		reg0_s_ptr->mBits.node3=node[3];
		reg1_s_ptr->mBits.node4=node[4];
		reg1_s_ptr->mBits.node5=node[5];
		reg1_s_ptr->mBits.node6=node[6];
		reg1_s_ptr->mBits.node7=node[7];
		reg2_s_ptr->mBits.node8=node[8];
		reg2_s_ptr->mBits.node9=node[9];
		reg2_s_ptr->mBits.node10=node[10];
		reg2_s_ptr->mBits.node11=node[11];
		reg3_s_ptr->mBits.node12=node[12];
		reg3_s_ptr->mBits.node13=node[13];
		reg3_s_ptr->mBits.node14=node[14];
		reg3_s_ptr->mBits.node15=node[15];
		reg4_s_ptr->mBits.node16=node[16];
		reg4_s_ptr->mBits.node17=node[17];
		reg4_s_ptr->mBits.node18=node[18];
		reg4_s_ptr->mBits.node19=node[19];
		reg5_s_ptr->mBits.node20=node[20];
		reg5_s_ptr->mBits.node21=node[21];
		reg5_s_ptr->mBits.node22=node[22];
		reg5_s_ptr->mBits.node23=node[23];
		reg6_s_ptr->mBits.node24=node[24];
		reg6_s_ptr->mBits.node25=node[25];
		/*
		reg0_ptr->dwValue=reg0_s_ptr->dwValue;
		reg1_ptr->dwValue=reg1_s_ptr->dwValue;
		reg2_ptr->dwValue=reg2_s_ptr->dwValue;
		reg3_ptr->dwValue=reg3_s_ptr->dwValue;
		reg4_ptr->dwValue=reg4_s_ptr->dwValue;
		reg5_ptr->dwValue=reg5_s_ptr->dwValue;
		reg6_ptr->dwValue=reg6_s_ptr->dwValue;
		*/
		reg_config[0].reg_addr = ISP_GAMMA_NODE_Y0 - ISP_BASE_ADDR;
		reg_config[0].reg_value = isp_reg_ptr->GAMMA_NODE_Y0;
		reg_config[1].reg_addr = ISP_GAMMA_NODE_Y1 - ISP_BASE_ADDR;
		reg_config[1].reg_value = isp_reg_ptr->GAMMA_NODE_Y1;
		reg_config[2].reg_addr = ISP_GAMMA_NODE_Y2 - ISP_BASE_ADDR;
		reg_config[2].reg_value = isp_reg_ptr->GAMMA_NODE_Y2;
		reg_config[3].reg_addr = ISP_GAMMA_NODE_Y3 - ISP_BASE_ADDR;
		reg_config[3].reg_value = isp_reg_ptr->GAMMA_NODE_Y3;
		reg_config[4].reg_addr = ISP_GAMMA_NODE_Y4 - ISP_BASE_ADDR;
		reg_config[4].reg_value = isp_reg_ptr->GAMMA_NODE_Y4;
		reg_config[5].reg_addr = ISP_GAMMA_NODE_Y5 - ISP_BASE_ADDR;
		reg_config[5].reg_value = isp_reg_ptr->GAMMA_NODE_Y5;
		reg_config[6].reg_addr = ISP_GAMMA_NODE_Y6 - ISP_BASE_ADDR;
		reg_config[6].reg_value = isp_reg_ptr->GAMMA_NODE_Y6;

		write_param.reg_param = (unsigned long)&reg_config[0];
		write_param.counts = 7;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispSetGammaRNode(uint32_t handler_id, uint16_t* node)
{
/*	union _isp_gamma_node_r0_v0001_tag* reg1_ptr=(union _isp_gamma_node_r0_v0001_tag*)ISP_GAMMA_NODE_R0_V0001;
	union _isp_gamma_node_r1_v0001_tag* reg1_ptr=(union _isp_gamma_node_r1_v0001_tag*)ISP_GAMMA_NODE_R1_V0001;
	union _isp_gamma_node_r2_v0001_tag* reg2_ptr=(union _isp_gamma_node_r2_v0001_tag*)ISP_GAMMA_NODE_R2_V0001;
	union _isp_gamma_node_r3_v0001_tag* reg3_ptr=(union _isp_gamma_node_r3_v0001_tag*)ISP_GAMMA_NODE_R3_V0001;
	union _isp_gamma_node_r4_v0001_tag* reg4_ptr=(union _isp_gamma_node_r4_v0001_tag*)ISP_GAMMA_NODE_R4_V0001;
	union _isp_gamma_node_r5_v0001_tag* reg5_ptr=(union _isp_gamma_node_r5_v0001_tag*)ISP_GAMMA_NODE_R5_V0001;
	union _isp_gamma_node_r6_v0001_tag* reg6_ptr=(union _isp_gamma_node_r6_v0001_tag*)ISP_GAMMA_NODE_R6_V0001;
*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_gamma_node_r0_v0001_tag* reg0_s_ptr=(union _isp_gamma_node_r0_v0001_tag*)&isp_reg_ptr->GAMMA_NODE_R0;
	union _isp_gamma_node_r1_v0001_tag* reg1_s_ptr=(union _isp_gamma_node_r1_v0001_tag*)&isp_reg_ptr->GAMMA_NODE_R1;
	union _isp_gamma_node_r2_v0001_tag* reg2_s_ptr=(union _isp_gamma_node_r2_v0001_tag*)&isp_reg_ptr->GAMMA_NODE_R2;
	union _isp_gamma_node_r3_v0001_tag* reg3_s_ptr=(union _isp_gamma_node_r3_v0001_tag*)&isp_reg_ptr->GAMMA_NODE_R3;
	union _isp_gamma_node_r4_v0001_tag* reg4_s_ptr=(union _isp_gamma_node_r4_v0001_tag*)&isp_reg_ptr->GAMMA_NODE_R4;
	union _isp_gamma_node_r5_v0001_tag* reg5_s_ptr=(union _isp_gamma_node_r5_v0001_tag*)&isp_reg_ptr->GAMMA_NODE_R5;
	union _isp_gamma_node_r6_v0001_tag* reg6_s_ptr=(union _isp_gamma_node_r6_v0001_tag*)&isp_reg_ptr->GAMMA_NODE_R6;
	struct isp_reg_bits reg_config[7];
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8830_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg0_s_ptr->mBits.node0=node[0];
		reg0_s_ptr->mBits.node1=node[1];
		reg0_s_ptr->mBits.node2=node[2];
		reg0_s_ptr->mBits.node3=node[3];
		reg1_s_ptr->mBits.node4=node[4];
		reg1_s_ptr->mBits.node5=node[5];
		reg1_s_ptr->mBits.node6=node[6];
		reg1_s_ptr->mBits.node7=node[7];
		reg2_s_ptr->mBits.node8=node[8];
		reg2_s_ptr->mBits.node9=node[9];
		reg2_s_ptr->mBits.node10=node[10];
		reg2_s_ptr->mBits.node11=node[11];
		reg3_s_ptr->mBits.node12=node[12];
		reg3_s_ptr->mBits.node13=node[13];
		reg3_s_ptr->mBits.node14=node[14];
		reg3_s_ptr->mBits.node15=node[15];
		reg4_s_ptr->mBits.node16=node[16];
		reg4_s_ptr->mBits.node17=node[17];
		reg4_s_ptr->mBits.node18=node[18];
		reg4_s_ptr->mBits.node19=node[19];
		reg5_s_ptr->mBits.node20=node[20];
		reg5_s_ptr->mBits.node21=node[21];
		reg5_s_ptr->mBits.node22=node[22];
		reg5_s_ptr->mBits.node23=node[23];
		reg6_s_ptr->mBits.node24=node[24];
		reg6_s_ptr->mBits.node25=node[25];
		/*
		reg0_ptr->dwValue=reg0_s_ptr->dwValue;
		reg1_ptr->dwValue=reg1_s_ptr->dwValue;
		reg2_ptr->dwValue=reg2_s_ptr->dwValue;
		reg3_ptr->dwValue=reg3_s_ptr->dwValue;
		reg4_ptr->dwValue=reg4_s_ptr->dwValue;
		reg5_ptr->dwValue=reg5_s_ptr->dwValue;
		reg6_ptr->dwValue=reg6_s_ptr->dwValue;
		*/
		reg_config[0].reg_addr = ISP_GAMMA_NODE_R0_V0001 - ISP_BASE_ADDR;
		reg_config[0].reg_value = reg0_s_ptr->dwValue;
		reg_config[1].reg_addr = ISP_GAMMA_NODE_R1_V0001 - ISP_BASE_ADDR;
		reg_config[1].reg_value = reg1_s_ptr->dwValue;
		reg_config[2].reg_addr = ISP_GAMMA_NODE_R2_V0001 - ISP_BASE_ADDR;
		reg_config[2].reg_value = reg2_s_ptr->dwValue;
		reg_config[3].reg_addr = ISP_GAMMA_NODE_R3_V0001 - ISP_BASE_ADDR;
		reg_config[3].reg_value = reg3_s_ptr->dwValue;
		reg_config[4].reg_addr = ISP_GAMMA_NODE_R4_V0001 - ISP_BASE_ADDR;
		reg_config[4].reg_value = reg4_s_ptr->dwValue;
		reg_config[5].reg_addr = ISP_GAMMA_NODE_R5_V0001 - ISP_BASE_ADDR;
		reg_config[5].reg_value = reg5_s_ptr->dwValue;
		reg_config[6].reg_addr = ISP_GAMMA_NODE_R6_V0001 - ISP_BASE_ADDR;
		reg_config[6].reg_value = reg6_s_ptr->dwValue;

		write_param.reg_param = (unsigned long)&reg_config[0];
		write_param.counts = 7;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispSetGammaGNode(uint32_t handler_id, uint16_t* node)
{
/*	union _isp_gamma_node_g0_v0001_tag* reg1_ptr=(union _isp_gamma_node_g0_v0001_tag*)ISP_GAMMA_NODE_G0_V0001;
	union _isp_gamma_node_g1_v0001_tag* reg1_ptr=(union _isp_gamma_node_g1_v0001_tag*)ISP_GAMMA_NODE_G1_V0001;
	union _isp_gamma_node_g2_v0001_tag* reg2_ptr=(union _isp_gamma_node_g2_v0001_tag*)ISP_GAMMA_NODE_G2_V0001;
	union _isp_gamma_node_g3_v0001_tag* reg3_ptr=(union _isp_gamma_node_g3_v0001_tag*)ISP_GAMMA_NODE_G3_V0001;
	union _isp_gamma_node_g4_v0001_tag* reg4_ptr=(union _isp_gamma_node_g4_v0001_tag*)ISP_GAMMA_NODE_G4_V0001;
	union _isp_gamma_node_g5_v0001_tag* reg5_ptr=(union _isp_gamma_node_g5_v0001_tag*)ISP_GAMMA_NODE_G5_V0001;
	union _isp_gamma_node_g6_v0001_tag* reg6_ptr=(union _isp_gamma_node_g6_v0001_tag*)ISP_GAMMA_NODE_G6_V0001;
*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_gamma_node_g0_v0001_tag* reg0_s_ptr=(union _isp_gamma_node_g0_v0001_tag*)&isp_reg_ptr->GAMMA_NODE_G0;
	union _isp_gamma_node_g1_v0001_tag* reg1_s_ptr=(union _isp_gamma_node_g1_v0001_tag*)&isp_reg_ptr->GAMMA_NODE_G1;
	union _isp_gamma_node_g2_v0001_tag* reg2_s_ptr=(union _isp_gamma_node_g2_v0001_tag*)&isp_reg_ptr->GAMMA_NODE_G2;
	union _isp_gamma_node_g3_v0001_tag* reg3_s_ptr=(union _isp_gamma_node_g3_v0001_tag*)&isp_reg_ptr->GAMMA_NODE_G3;
	union _isp_gamma_node_g4_v0001_tag* reg4_s_ptr=(union _isp_gamma_node_g4_v0001_tag*)&isp_reg_ptr->GAMMA_NODE_G4;
	union _isp_gamma_node_g5_v0001_tag* reg5_s_ptr=(union _isp_gamma_node_g5_v0001_tag*)&isp_reg_ptr->GAMMA_NODE_G5;
	union _isp_gamma_node_g6_v0001_tag* reg6_s_ptr=(union _isp_gamma_node_g6_v0001_tag*)&isp_reg_ptr->GAMMA_NODE_G6;
	struct isp_reg_bits reg_config[7];
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8830_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg0_s_ptr->mBits.node0=node[0];
		reg0_s_ptr->mBits.node1=node[1];
		reg0_s_ptr->mBits.node2=node[2];
		reg0_s_ptr->mBits.node3=node[3];
		reg1_s_ptr->mBits.node4=node[4];
		reg1_s_ptr->mBits.node5=node[5];
		reg1_s_ptr->mBits.node6=node[6];
		reg1_s_ptr->mBits.node7=node[7];
		reg2_s_ptr->mBits.node8=node[8];
		reg2_s_ptr->mBits.node9=node[9];
		reg2_s_ptr->mBits.node10=node[10];
		reg2_s_ptr->mBits.node11=node[11];
		reg3_s_ptr->mBits.node12=node[12];
		reg3_s_ptr->mBits.node13=node[13];
		reg3_s_ptr->mBits.node14=node[14];
		reg3_s_ptr->mBits.node15=node[15];
		reg4_s_ptr->mBits.node16=node[16];
		reg4_s_ptr->mBits.node17=node[17];
		reg4_s_ptr->mBits.node18=node[18];
		reg4_s_ptr->mBits.node19=node[19];
		reg5_s_ptr->mBits.node20=node[20];
		reg5_s_ptr->mBits.node21=node[21];
		reg5_s_ptr->mBits.node22=node[22];
		reg5_s_ptr->mBits.node23=node[23];
		reg6_s_ptr->mBits.node24=node[24];
		reg6_s_ptr->mBits.node25=node[25];
		/*
		reg0_ptr->dwValue=reg0_s_ptr->dwValue;
		reg1_ptr->dwValue=reg1_s_ptr->dwValue;
		reg2_ptr->dwValue=reg2_s_ptr->dwValue;
		reg3_ptr->dwValue=reg3_s_ptr->dwValue;
		reg4_ptr->dwValue=reg4_s_ptr->dwValue;
		reg5_ptr->dwValue=reg5_s_ptr->dwValue;
		reg6_ptr->dwValue=reg6_s_ptr->dwValue;
		*/
		reg_config[0].reg_addr = ISP_GAMMA_NODE_G0_V0001 - ISP_BASE_ADDR;
		reg_config[0].reg_value = reg0_s_ptr->dwValue;
		reg_config[1].reg_addr = ISP_GAMMA_NODE_G1_V0001 - ISP_BASE_ADDR;
		reg_config[1].reg_value = reg1_s_ptr->dwValue;
		reg_config[2].reg_addr = ISP_GAMMA_NODE_G2_V0001 - ISP_BASE_ADDR;
		reg_config[2].reg_value = reg2_s_ptr->dwValue;
		reg_config[3].reg_addr = ISP_GAMMA_NODE_G3_V0001 - ISP_BASE_ADDR;
		reg_config[3].reg_value = reg3_s_ptr->dwValue;
		reg_config[4].reg_addr = ISP_GAMMA_NODE_G4_V0001 - ISP_BASE_ADDR;
		reg_config[4].reg_value = reg4_s_ptr->dwValue;
		reg_config[5].reg_addr = ISP_GAMMA_NODE_G5_V0001 - ISP_BASE_ADDR;
		reg_config[5].reg_value = reg5_s_ptr->dwValue;
		reg_config[6].reg_addr = ISP_GAMMA_NODE_G6_V0001 - ISP_BASE_ADDR;
		reg_config[6].reg_value = reg6_s_ptr->dwValue;

		write_param.reg_param = (unsigned long)&reg_config[0];
		write_param.counts = 7;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispSetGammaBNode(uint32_t handler_id, uint16_t* node)
{
/*	union _isp_gamma_node_b0_v0001_tag* reg1_ptr=(union _isp_gamma_node_b0_v0001_tag*)ISP_GAMMA_NODE_B0_V0001;
	union _isp_gamma_node_b1_v0001_tag* reg1_ptr=(union _isp_gamma_node_b1_v0001_tag*)ISP_GAMMA_NODE_B1_V0001;
	union _isp_gamma_node_b2_v0001_tag* reg2_ptr=(union _isp_gamma_node_b2_v0001_tag*)ISP_GAMMA_NODE_B2_V0001;
	union _isp_gamma_node_b3_v0001_tag* reg3_ptr=(union _isp_gamma_node_b3_v0001_tag*)ISP_GAMMA_NODE_B3_V0001;
	union _isp_gamma_node_b4_v0001_tag* reg4_ptr=(union _isp_gamma_node_b4_v0001_tag*)ISP_GAMMA_NODE_B4_V0001;
	union _isp_gamma_node_b5_v0001_tag* reg5_ptr=(union _isp_gamma_node_b5_v0001_tag*)ISP_GAMMA_NODE_B5_V0001;
	union _isp_gamma_node_b6_v0001_tag* reg6_ptr=(union _isp_gamma_node_b6_v0001_tag*)ISP_GAMMA_NODE_B6_V0001;
*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_gamma_node_b0_v0001_tag* reg0_s_ptr=(union _isp_gamma_node_b0_v0001_tag*)&isp_reg_ptr->GAMMA_NODE_B0;
	union _isp_gamma_node_b1_v0001_tag* reg1_s_ptr=(union _isp_gamma_node_b1_v0001_tag*)&isp_reg_ptr->GAMMA_NODE_B1;
	union _isp_gamma_node_b2_v0001_tag* reg2_s_ptr=(union _isp_gamma_node_b2_v0001_tag*)&isp_reg_ptr->GAMMA_NODE_B2;
	union _isp_gamma_node_b3_v0001_tag* reg3_s_ptr=(union _isp_gamma_node_b3_v0001_tag*)&isp_reg_ptr->GAMMA_NODE_B3;
	union _isp_gamma_node_b4_v0001_tag* reg4_s_ptr=(union _isp_gamma_node_b4_v0001_tag*)&isp_reg_ptr->GAMMA_NODE_B4;
	union _isp_gamma_node_b5_v0001_tag* reg5_s_ptr=(union _isp_gamma_node_b5_v0001_tag*)&isp_reg_ptr->GAMMA_NODE_B5;
	union _isp_gamma_node_b6_v0001_tag* reg6_s_ptr=(union _isp_gamma_node_b6_v0001_tag*)&isp_reg_ptr->GAMMA_NODE_B6;
	struct isp_reg_bits reg_config[7];
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8830_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg0_s_ptr->mBits.node0=node[0];
		reg0_s_ptr->mBits.node1=node[1];
		reg0_s_ptr->mBits.node2=node[2];
		reg0_s_ptr->mBits.node3=node[3];
		reg1_s_ptr->mBits.node4=node[4];
		reg1_s_ptr->mBits.node5=node[5];
		reg1_s_ptr->mBits.node6=node[6];
		reg1_s_ptr->mBits.node7=node[7];
		reg2_s_ptr->mBits.node8=node[8];
		reg2_s_ptr->mBits.node9=node[9];
		reg2_s_ptr->mBits.node10=node[10];
		reg2_s_ptr->mBits.node11=node[11];
		reg3_s_ptr->mBits.node12=node[12];
		reg3_s_ptr->mBits.node13=node[13];
		reg3_s_ptr->mBits.node14=node[14];
		reg3_s_ptr->mBits.node15=node[15];
		reg4_s_ptr->mBits.node16=node[16];
		reg4_s_ptr->mBits.node17=node[17];
		reg4_s_ptr->mBits.node18=node[18];
		reg4_s_ptr->mBits.node19=node[19];
		reg5_s_ptr->mBits.node20=node[20];
		reg5_s_ptr->mBits.node21=node[21];
		reg5_s_ptr->mBits.node22=node[22];
		reg5_s_ptr->mBits.node23=node[23];
		reg6_s_ptr->mBits.node24=node[24];
		reg6_s_ptr->mBits.node25=node[25];
		/*
		reg0_ptr->dwValue=reg0_s_ptr->dwValue;
		reg1_ptr->dwValue=reg1_s_ptr->dwValue;
		reg2_ptr->dwValue=reg2_s_ptr->dwValue;
		reg3_ptr->dwValue=reg3_s_ptr->dwValue;
		reg4_ptr->dwValue=reg4_s_ptr->dwValue;
		reg5_ptr->dwValue=reg5_s_ptr->dwValue;
		reg6_ptr->dwValue=reg6_s_ptr->dwValue;
		*/
		reg_config[0].reg_addr = ISP_GAMMA_NODE_B0_V0001 - ISP_BASE_ADDR;
		reg_config[0].reg_value = reg0_s_ptr->dwValue;
		reg_config[1].reg_addr = ISP_GAMMA_NODE_B1_V0001 - ISP_BASE_ADDR;
		reg_config[1].reg_value = reg1_s_ptr->dwValue;
		reg_config[2].reg_addr = ISP_GAMMA_NODE_B2_V0001 - ISP_BASE_ADDR;
		reg_config[2].reg_value = reg2_s_ptr->dwValue;
		reg_config[3].reg_addr = ISP_GAMMA_NODE_B3_V0001 - ISP_BASE_ADDR;
		reg_config[3].reg_value = reg3_s_ptr->dwValue;
		reg_config[4].reg_addr = ISP_GAMMA_NODE_B4_V0001 - ISP_BASE_ADDR;
		reg_config[4].reg_value = reg4_s_ptr->dwValue;
		reg_config[5].reg_addr = ISP_GAMMA_NODE_B5_V0001 - ISP_BASE_ADDR;
		reg_config[5].reg_value = reg5_s_ptr->dwValue;
		reg_config[6].reg_addr = ISP_GAMMA_NODE_B6_V0001 - ISP_BASE_ADDR;
		reg_config[6].reg_value = reg6_s_ptr->dwValue;

		write_param.reg_param = (unsigned long)&reg_config[0];
		write_param.counts = 7;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispSetGammaNodeIndex(uint32_t handler_id, uint8_t* node)
{
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_gamma_node_idx0_v0001_tag* reg0_s_ptr=(union _isp_gamma_node_idx0_v0001_tag*)&isp_reg_ptr->GAMMA_NODE_IDX0_V0001;
	union _isp_gamma_node_idx1_v0001_tag* reg1_s_ptr=(union _isp_gamma_node_idx1_v0001_tag*)&isp_reg_ptr->GAMMA_NODE_IDX1_V0001;
	union _isp_gamma_node_idx2_v0001_tag* reg2_s_ptr=(union _isp_gamma_node_idx2_v0001_tag*)&isp_reg_ptr->GAMMA_NODE_IDX2_V0001;
	union _isp_gamma_node_idx3_v0001_tag* reg3_s_ptr=(union _isp_gamma_node_idx3_v0001_tag*)&isp_reg_ptr->GAMMA_NODE_IDX3_V0001;

	struct isp_reg_bits reg_config[4];
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8830_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg0_s_ptr->mBits.index1=node[0];
		reg0_s_ptr->mBits.index2=node[1];
		reg0_s_ptr->mBits.index3=node[2];
		reg0_s_ptr->mBits.index4=node[3];
		reg0_s_ptr->mBits.index5=node[4];
		reg0_s_ptr->mBits.index6=node[5];
		reg0_s_ptr->mBits.index7=node[6];
		reg0_s_ptr->mBits.index8=node[7];
		reg1_s_ptr->mBits.index9=node[8];
		reg1_s_ptr->mBits.index10=node[9];
		reg1_s_ptr->mBits.index11=node[10];
		reg1_s_ptr->mBits.index12=node[11];
		reg1_s_ptr->mBits.index13=node[12];
		reg1_s_ptr->mBits.index14=node[13];
		reg1_s_ptr->mBits.index15=node[14];
		reg1_s_ptr->mBits.index16=node[15];
		reg2_s_ptr->mBits.index17=node[16];
		reg2_s_ptr->mBits.index18=node[17];
		reg2_s_ptr->mBits.index19=node[18];
		reg2_s_ptr->mBits.index20=node[19];
		reg2_s_ptr->mBits.index21=node[20];
		reg2_s_ptr->mBits.index22=node[21];
		reg2_s_ptr->mBits.index23=node[22];
		reg2_s_ptr->mBits.index24=node[23];
		reg3_s_ptr->mBits.index25=node[24];
		/*
		reg0_ptr->dwValue=reg0_s_ptr->dwValue;
		reg1_ptr->dwValue=reg1_s_ptr->dwValue;
		reg2_ptr->dwValue=reg2_s_ptr->dwValue;
		reg3_ptr->dwValue=reg3_s_ptr->dwValue;
		reg4_ptr->dwValue=reg4_s_ptr->dwValue;
		reg5_ptr->dwValue=reg5_s_ptr->dwValue;
		reg6_ptr->dwValue=reg6_s_ptr->dwValue;
		*/
		reg_config[0].reg_addr = ISP_GAMMA_NODE_IDX0_V0001 - ISP_BASE_ADDR;
		reg_config[0].reg_value = reg0_s_ptr->dwValue;
		reg_config[1].reg_addr = ISP_GAMMA_NODE_IDX1_V0001 - ISP_BASE_ADDR;
		reg_config[1].reg_value = reg1_s_ptr->dwValue;
		reg_config[2].reg_addr = ISP_GAMMA_NODE_IDX2_V0001 - ISP_BASE_ADDR;
		reg_config[2].reg_value = reg2_s_ptr->dwValue;
		reg_config[3].reg_addr = ISP_GAMMA_NODE_IDX3_V0001 - ISP_BASE_ADDR;
		reg_config[3].reg_value = reg3_s_ptr->dwValue;

		write_param.reg_param = (unsigned long)&reg_config[0];
		write_param.counts = 4;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

#if 0
int32_t ispSetGammaXNode_v002(uint32_t handler_id, uint16_t* node)
{
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_gamma_node_x0_v0002_tag* reg_rx0_ptr=(union _isp_gamma_node_x0_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_X0;
	union _isp_gamma_node_x1_v0002_tag* reg_rx1_ptr=(union _isp_gamma_node_x1_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_X1;
	union _isp_gamma_node_x2_v0002_tag* reg_rx2_ptr=(union _isp_gamma_node_x2_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_X2;
	union _isp_gamma_node_x3_v0002_tag* reg_rx3_ptr=(union _isp_gamma_node_x3_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_X3;
	union _isp_gamma_node_x4_v0002_tag* reg_rx4_ptr=(union _isp_gamma_node_x4_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_X4;
	union _isp_gamma_node_x5_v0002_tag* reg_rx5_ptr=(union _isp_gamma_node_x5_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_X5;

	union _isp_gamma_node_x0_v0002_tag* reg_gx0_ptr=(union _isp_gamma_node_x0_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_X0;
	union _isp_gamma_node_x1_v0002_tag* reg_gx1_ptr=(union _isp_gamma_node_x1_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_X1;
	union _isp_gamma_node_x2_v0002_tag* reg_gx2_ptr=(union _isp_gamma_node_x2_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_X2;
	union _isp_gamma_node_x3_v0002_tag* reg_gx3_ptr=(union _isp_gamma_node_x3_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_X3;
	union _isp_gamma_node_x4_v0002_tag* reg_gx4_ptr=(union _isp_gamma_node_x4_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_X4;
	union _isp_gamma_node_x5_v0002_tag* reg_gx5_ptr=(union _isp_gamma_node_x5_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_X5;

	union _isp_gamma_node_x0_v0002_tag* reg_bx0_ptr=(union _isp_gamma_node_x0_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_X0;
	union _isp_gamma_node_x1_v0002_tag* reg_bx1_ptr=(union _isp_gamma_node_x1_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_X1;
	union _isp_gamma_node_x2_v0002_tag* reg_bx2_ptr=(union _isp_gamma_node_x2_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_X2;
	union _isp_gamma_node_x3_v0002_tag* reg_bx3_ptr=(union _isp_gamma_node_x3_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_X3;
	union _isp_gamma_node_x4_v0002_tag* reg_bx4_ptr=(union _isp_gamma_node_x4_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_X4;
	union _isp_gamma_node_x5_v0002_tag* reg_bx5_ptr=(union _isp_gamma_node_x5_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_X5;

	struct isp_reg_bits reg_config[18];
	struct isp_reg_param write_param;
	uint32_t i;

	if(SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		i = 0;
		reg_rx0_ptr->mBits.node0=node[i++];
		reg_rx0_ptr->mBits.node1=node[i++];
		reg_rx0_ptr->mBits.node2=node[i++];
		reg_rx1_ptr->mBits.node3=node[i++];
		reg_rx1_ptr->mBits.node4=node[i++];
		reg_rx1_ptr->mBits.node5=node[i++];
		reg_rx2_ptr->mBits.node6=node[i++];
		reg_rx2_ptr->mBits.node7=node[i++];
		reg_rx2_ptr->mBits.node8=node[i++];
		reg_rx3_ptr->mBits.node9=node[i++];
		reg_rx3_ptr->mBits.node10=node[i++];
		reg_rx3_ptr->mBits.node11=node[i++];
		reg_rx4_ptr->mBits.node12=node[i++];
		reg_rx4_ptr->mBits.node13=node[i++];
		reg_rx4_ptr->mBits.node14=node[i++];
		reg_rx5_ptr->mBits.node15=node[i++];
		reg_rx5_ptr->mBits.node16=node[i++];
		reg_rx5_ptr->mBits.node17=node[i++];

		reg_gx0_ptr->mBits.node0=node[i++];
		reg_gx0_ptr->mBits.node1=node[i++];
		reg_gx0_ptr->mBits.node2=node[i++];
		reg_gx1_ptr->mBits.node3=node[i++];
		reg_gx1_ptr->mBits.node4=node[i++];
		reg_gx1_ptr->mBits.node5=node[i++];
		reg_gx2_ptr->mBits.node6=node[i++];
		reg_gx2_ptr->mBits.node7=node[i++];
		reg_gx2_ptr->mBits.node8=node[i++];
		reg_gx3_ptr->mBits.node9=node[i++];
		reg_gx3_ptr->mBits.node10=node[i++];
		reg_gx3_ptr->mBits.node11=node[i++];
		reg_gx4_ptr->mBits.node12=node[i++];
		reg_gx4_ptr->mBits.node13=node[i++];
		reg_gx4_ptr->mBits.node14=node[i++];
		reg_gx5_ptr->mBits.node15=node[i++];
		reg_gx5_ptr->mBits.node16=node[i++];
		reg_gx5_ptr->mBits.node17=node[i++];

		reg_bx0_ptr->mBits.node0=node[i++];
		reg_bx0_ptr->mBits.node1=node[i++];
		reg_bx0_ptr->mBits.node2=node[i++];
		reg_bx1_ptr->mBits.node3=node[i++];
		reg_bx1_ptr->mBits.node4=node[i++];
		reg_bx1_ptr->mBits.node5=node[i++];
		reg_bx2_ptr->mBits.node6=node[i++];
		reg_bx2_ptr->mBits.node7=node[i++];
		reg_bx2_ptr->mBits.node8=node[i++];
		reg_bx3_ptr->mBits.node9=node[i++];
		reg_bx3_ptr->mBits.node10=node[i++];
		reg_bx3_ptr->mBits.node11=node[i++];
		reg_bx4_ptr->mBits.node12=node[i++];
		reg_bx4_ptr->mBits.node13=node[i++];
		reg_bx4_ptr->mBits.node14=node[i++];
		reg_bx5_ptr->mBits.node15=node[i++];
		reg_bx5_ptr->mBits.node16=node[i++];
		reg_bx5_ptr->mBits.node17=node[i++];


		reg_config[0].reg_addr = ISP_GAMMA_NODE_RX0_V0002 - ISP_BASE_ADDR;
		reg_config[0].reg_value = reg_rx0_ptr->dwValue;
		reg_config[1].reg_addr = ISP_GAMMA_NODE_RX1_V0002 - ISP_BASE_ADDR;
		reg_config[1].reg_value = reg_rx1_ptr->dwValue;
		reg_config[2].reg_addr = ISP_GAMMA_NODE_RX2_V0002 - ISP_BASE_ADDR;
		reg_config[2].reg_value = reg_rx2_ptr->dwValue;
		reg_config[3].reg_addr = ISP_GAMMA_NODE_RX3_V0002 - ISP_BASE_ADDR;
		reg_config[3].reg_value = reg_rx3_ptr->dwValue;
		reg_config[4].reg_addr = ISP_GAMMA_NODE_RX4_V0002 - ISP_BASE_ADDR;
		reg_config[4].reg_value = reg_rx4_ptr->dwValue;
		reg_config[5].reg_addr = ISP_GAMMA_NODE_RX5_V0002 - ISP_BASE_ADDR;
		reg_config[5].reg_value = reg_rx5_ptr->dwValue;

		reg_config[6].reg_addr = ISP_GAMMA_NODE_GX0_V0002 - ISP_BASE_ADDR;
		reg_config[6].reg_value = reg_gx0_ptr->dwValue;
		reg_config[7].reg_addr = ISP_GAMMA_NODE_GX1_V0002 - ISP_BASE_ADDR;
		reg_config[7].reg_value = reg_gx1_ptr->dwValue;
		reg_config[8].reg_addr = ISP_GAMMA_NODE_GX2_V0002 - ISP_BASE_ADDR;
		reg_config[8].reg_value = reg_gx2_ptr->dwValue;
		reg_config[9].reg_addr = ISP_GAMMA_NODE_GX3_V0002 - ISP_BASE_ADDR;
		reg_config[9].reg_value = reg_gx3_ptr->dwValue;
		reg_config[10].reg_addr = ISP_GAMMA_NODE_GX4_V0002 - ISP_BASE_ADDR;
		reg_config[10].reg_value = reg_gx4_ptr->dwValue;
		reg_config[11].reg_addr = ISP_GAMMA_NODE_GX5_V0002 - ISP_BASE_ADDR;
		reg_config[11].reg_value = reg_gx5_ptr->dwValue;


		reg_config[12].reg_addr = ISP_GAMMA_NODE_BX0_V0002 - ISP_BASE_ADDR;
		reg_config[12].reg_value = reg_bx0_ptr->dwValue;
		reg_config[13].reg_addr = ISP_GAMMA_NODE_BX1_V0002 - ISP_BASE_ADDR;
		reg_config[13].reg_value = reg_bx1_ptr->dwValue;
		reg_config[14].reg_addr = ISP_GAMMA_NODE_BX2_V0002 - ISP_BASE_ADDR;
		reg_config[14].reg_value = reg_bx2_ptr->dwValue;
		reg_config[15].reg_addr = ISP_GAMMA_NODE_BX3_V0002 - ISP_BASE_ADDR;
		reg_config[15].reg_value = reg_bx3_ptr->dwValue;
		reg_config[16].reg_addr = ISP_GAMMA_NODE_BX4_V0002 - ISP_BASE_ADDR;
		reg_config[16].reg_value = reg_bx4_ptr->dwValue;
		reg_config[17].reg_addr = ISP_GAMMA_NODE_BX5_V0002 - ISP_BASE_ADDR;
		reg_config[17].reg_value = reg_bx5_ptr->dwValue;


		write_param.reg_param = (unsigned long)&reg_config[0];
		write_param.counts = 18;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispSetGammaYNode_v002(uint32_t handler_id, uint32_t isp_id, uint16_t* node)
{
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_gamma_node_y_v0002_tag* reg_r0_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R0;
	union _isp_gamma_node_y_v0002_tag* reg_r1_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R1;
	union _isp_gamma_node_y_v0002_tag* reg_r2_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R2;
	union _isp_gamma_node_y_v0002_tag* reg_r3_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R3;
	union _isp_gamma_node_y_v0002_tag* reg_r4_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R4;
	union _isp_gamma_node_y_v0002_tag* reg_r5_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R5;
	union _isp_gamma_node_y_v0002_tag* reg_r6_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R6;
	union _isp_gamma_node_y_v0002_tag* reg_r7_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R7;
	union _isp_gamma_node_y_v0002_tag* reg_r8_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R8;
	union _isp_gamma_node_y_v0002_tag* reg_r9_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R0;
	union _isp_gamma_node_y_v0002_tag* reg_r10_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R1;
	union _isp_gamma_node_y_v0002_tag* reg_r11_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R2;
	union _isp_gamma_node_y_v0002_tag* reg_r12_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R3;
	union _isp_gamma_node_y_v0002_tag* reg_r13_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R4;
	union _isp_gamma_node_y_v0002_tag* reg_r14_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R5;
	union _isp_gamma_node_y_v0002_tag* reg_r15_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R6;


	union _isp_gamma_node_y_v0002_tag* reg_g0_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R0;
	union _isp_gamma_node_y_v0002_tag* reg_g1_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R1;
	union _isp_gamma_node_y_v0002_tag* reg_g2_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R2;
	union _isp_gamma_node_y_v0002_tag* reg_g3_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R3;
	union _isp_gamma_node_y_v0002_tag* reg_g4_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R4;
	union _isp_gamma_node_y_v0002_tag* reg_g5_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R5;
	union _isp_gamma_node_y_v0002_tag* reg_g6_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R6;
	union _isp_gamma_node_y_v0002_tag* reg_g7_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R7;
	union _isp_gamma_node_y_v0002_tag* reg_g8_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R8;
	union _isp_gamma_node_y_v0002_tag* reg_g9_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R0;
	union _isp_gamma_node_y_v0002_tag* reg_g10_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R1;
	union _isp_gamma_node_y_v0002_tag* reg_g11_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R2;
	union _isp_gamma_node_y_v0002_tag* reg_g12_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R3;
	union _isp_gamma_node_y_v0002_tag* reg_g13_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R4;
	union _isp_gamma_node_y_v0002_tag* reg_g14_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R5;
	union _isp_gamma_node_y_v0002_tag* reg_g15_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R6;

	union _isp_gamma_node_y_v0002_tag* reg_b0_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R0;
	union _isp_gamma_node_y_v0002_tag* reg_b1_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R1;
	union _isp_gamma_node_y_v0002_tag* reg_b2_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R2;
	union _isp_gamma_node_y_v0002_tag* reg_b3_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R3;
	union _isp_gamma_node_y_v0002_tag* reg_b4_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R4;
	union _isp_gamma_node_y_v0002_tag* reg_b5_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R5;
	union _isp_gamma_node_y_v0002_tag* reg_b6_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R6;
	union _isp_gamma_node_y_v0002_tag* reg_b7_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R7;
	union _isp_gamma_node_y_v0002_tag* reg_b8_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R8;
	union _isp_gamma_node_y_v0002_tag* reg_b9_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R0;
	union _isp_gamma_node_y_v0002_tag* reg_b10_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R1;
	union _isp_gamma_node_y_v0002_tag* reg_b11_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R2;
	union _isp_gamma_node_y_v0002_tag* reg_b12_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R3;
	union _isp_gamma_node_y_v0002_tag* reg_b13_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R4;
	union _isp_gamma_node_y_v0002_tag* reg_b14_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R5;
	union _isp_gamma_node_y_v0002_tag* reg_b15_ptr=(union _isp_gamma_node_y_v0002_tag*)&isp_reg_ptr->GAMMA_NODE_R6;



	struct isp_reg_bits reg_config[48];
	struct isp_reg_param write_param;
	uint32_t i;
	//uint32_t isp_id=_isp_GetIspId();

	if(SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		i = 0;
		reg_r0_ptr->mBits.k=node[i++];
		reg_r0_ptr->mBits.b=node[i++];
		reg_r1_ptr->mBits.k=node[i++];
		reg_r1_ptr->mBits.b=node[i++];
		reg_r2_ptr->mBits.k=node[i++];
		reg_r2_ptr->mBits.b=node[i++];
		reg_r3_ptr->mBits.k=node[i++];
		reg_r3_ptr->mBits.b=node[i++];
		reg_r4_ptr->mBits.k=node[i++];
		reg_r4_ptr->mBits.b=node[i++];
		reg_r5_ptr->mBits.k=node[i++];
		reg_r5_ptr->mBits.b=node[i++];
		reg_r6_ptr->mBits.k=node[i++];
		reg_r6_ptr->mBits.b=node[i++];
		reg_r7_ptr->mBits.k=node[i++];
		reg_r7_ptr->mBits.b=node[i++];
		reg_r8_ptr->mBits.k=node[i++];
		reg_r8_ptr->mBits.b=node[i++];
		reg_r9_ptr->mBits.k=node[i++];
		reg_r9_ptr->mBits.b=node[i++];
		reg_r10_ptr->mBits.k=node[i++];
		reg_r10_ptr->mBits.b=node[i++];
		reg_r11_ptr->mBits.k=node[i++];
		reg_r11_ptr->mBits.b=node[i++];
		reg_r12_ptr->mBits.k=node[i++];
		reg_r12_ptr->mBits.b=node[i++];
		reg_r13_ptr->mBits.k=node[i++];
		reg_r13_ptr->mBits.b=node[i++];
		reg_r14_ptr->mBits.k=node[i++];
		reg_r14_ptr->mBits.b=node[i++];
		reg_r15_ptr->mBits.k=node[i++];
		reg_r15_ptr->mBits.b=node[i++];

		reg_g0_ptr->mBits.k=node[i++];
		reg_g0_ptr->mBits.b=node[i++];
		reg_g1_ptr->mBits.k=node[i++];
		reg_g1_ptr->mBits.b=node[i++];
		reg_g2_ptr->mBits.k=node[i++];
		reg_g2_ptr->mBits.b=node[i++];
		reg_g3_ptr->mBits.k=node[i++];
		reg_g3_ptr->mBits.b=node[i++];
		reg_g4_ptr->mBits.k=node[i++];
		reg_g4_ptr->mBits.b=node[i++];
		reg_g5_ptr->mBits.k=node[i++];
		reg_g5_ptr->mBits.b=node[i++];
		reg_g6_ptr->mBits.k=node[i++];
		reg_g6_ptr->mBits.b=node[i++];
		reg_g7_ptr->mBits.k=node[i++];
		reg_g7_ptr->mBits.b=node[i++];
		reg_g8_ptr->mBits.k=node[i++];
		reg_g8_ptr->mBits.b=node[i++];
		reg_g9_ptr->mBits.k=node[i++];
		reg_g9_ptr->mBits.b=node[i++];
		reg_g10_ptr->mBits.k=node[i++];
		reg_g10_ptr->mBits.b=node[i++];
		reg_g11_ptr->mBits.k=node[i++];
		reg_g11_ptr->mBits.b=node[i++];
		reg_g12_ptr->mBits.k=node[i++];
		reg_g12_ptr->mBits.b=node[i++];
		reg_g13_ptr->mBits.k=node[i++];
		reg_g13_ptr->mBits.b=node[i++];
		reg_g14_ptr->mBits.k=node[i++];
		reg_g14_ptr->mBits.b=node[i++];
		reg_g15_ptr->mBits.k=node[i++];
		reg_g15_ptr->mBits.b=node[i++];

		reg_b0_ptr->mBits.k=node[i++];
		reg_b0_ptr->mBits.b=node[i++];
		reg_b1_ptr->mBits.k=node[i++];
		reg_b1_ptr->mBits.b=node[i++];
		reg_b2_ptr->mBits.k=node[i++];
		reg_b2_ptr->mBits.b=node[i++];
		reg_b3_ptr->mBits.k=node[i++];
		reg_b3_ptr->mBits.b=node[i++];
		reg_b4_ptr->mBits.k=node[i++];
		reg_b4_ptr->mBits.b=node[i++];
		reg_b5_ptr->mBits.k=node[i++];
		reg_b5_ptr->mBits.b=node[i++];
		reg_b6_ptr->mBits.k=node[i++];
		reg_b6_ptr->mBits.b=node[i++];
		reg_b7_ptr->mBits.k=node[i++];
		reg_b7_ptr->mBits.b=node[i++];
		reg_b8_ptr->mBits.k=node[i++];
		reg_b8_ptr->mBits.b=node[i++];
		reg_b9_ptr->mBits.k=node[i++];
		reg_b9_ptr->mBits.b=node[i++];
		reg_b10_ptr->mBits.k=node[i++];
		reg_b10_ptr->mBits.b=node[i++];
		reg_b11_ptr->mBits.k=node[i++];
		reg_b11_ptr->mBits.b=node[i++];
		reg_b12_ptr->mBits.k=node[i++];
		reg_b12_ptr->mBits.b=node[i++];
		reg_b13_ptr->mBits.k=node[i++];
		reg_b13_ptr->mBits.b=node[i++];
		reg_b14_ptr->mBits.k=node[i++];
		reg_b14_ptr->mBits.b=node[i++];
		reg_b15_ptr->mBits.k=node[i++];
		reg_b15_ptr->mBits.b=node[i++];

		i = 0;
		reg_config[0].reg_addr = ISP_GAMMA_NODE_R0_V0002 - ISP_BASE_ADDR;
		reg_config[0].reg_value = reg_r0_ptr->dwValue;
		reg_config[1].reg_addr = ISP_GAMMA_NODE_R1_V0002 - ISP_BASE_ADDR;
		reg_config[1].reg_value = reg_r1_ptr->dwValue;
		reg_config[2].reg_addr = ISP_GAMMA_NODE_R2_V0002 - ISP_BASE_ADDR;
		reg_config[2].reg_value = reg_r2_ptr->dwValue;
		reg_config[3].reg_addr = ISP_GAMMA_NODE_R3_V0002 - ISP_BASE_ADDR;
		reg_config[3].reg_value = reg_r3_ptr->dwValue;
		reg_config[4].reg_addr = ISP_GAMMA_NODE_R4_V0002 - ISP_BASE_ADDR;
		reg_config[4].reg_value = reg_r4_ptr->dwValue;
		reg_config[5].reg_addr = ISP_GAMMA_NODE_R5_V0002 - ISP_BASE_ADDR;
		reg_config[5].reg_value = reg_r5_ptr->dwValue;
		reg_config[6].reg_addr = ISP_GAMMA_NODE_R6_V0002 - ISP_BASE_ADDR;
		reg_config[6].reg_value = reg_r6_ptr->dwValue;
		reg_config[7].reg_addr = ISP_GAMMA_NODE_R7_V0002 - ISP_BASE_ADDR;
		reg_config[7].reg_value = reg_r7_ptr->dwValue;
		reg_config[8].reg_addr = ISP_GAMMA_NODE_R8_V0002 - ISP_BASE_ADDR;
		reg_config[8].reg_value = reg_r8_ptr->dwValue;
		reg_config[9].reg_addr = ISP_GAMMA_NODE_R9_V0002 - ISP_BASE_ADDR;
		reg_config[9].reg_value = reg_r9_ptr->dwValue;
		reg_config[10].reg_addr = ISP_GAMMA_NODE_R10_V0002 - ISP_BASE_ADDR;
		reg_config[10].reg_value = reg_r10_ptr->dwValue;
		reg_config[11].reg_addr = ISP_GAMMA_NODE_R11_V0002 - ISP_BASE_ADDR;
		reg_config[11].reg_value = reg_r11_ptr->dwValue;
		reg_config[12].reg_addr = ISP_GAMMA_NODE_R12_V0002 - ISP_BASE_ADDR;
		reg_config[12].reg_value = reg_r12_ptr->dwValue;
		reg_config[13].reg_addr = ISP_GAMMA_NODE_R13_V0002 - ISP_BASE_ADDR;
		reg_config[13].reg_value = reg_r13_ptr->dwValue;
		reg_config[14].reg_addr = ISP_GAMMA_NODE_R14_V0002 - ISP_BASE_ADDR;
		reg_config[14].reg_value = reg_r14_ptr->dwValue;
		reg_config[15].reg_addr = ISP_GAMMA_NODE_R15_V0002 - ISP_BASE_ADDR;
		reg_config[15].reg_value = reg_r15_ptr->dwValue;

		i = 16;
		reg_config[16].reg_addr = ISP_GAMMA_NODE_G0_V0002 - ISP_BASE_ADDR;
		reg_config[16].reg_value = reg_g0_ptr->dwValue;
		reg_config[17].reg_addr = ISP_GAMMA_NODE_G1_V0002 - ISP_BASE_ADDR;
		reg_config[17].reg_value = reg_g1_ptr->dwValue;
		reg_config[18].reg_addr = ISP_GAMMA_NODE_G2_V0002 - ISP_BASE_ADDR;
		reg_config[18].reg_value = reg_g2_ptr->dwValue;
		reg_config[19].reg_addr = ISP_GAMMA_NODE_G3_V0002 - ISP_BASE_ADDR;
		reg_config[19].reg_value = reg_g3_ptr->dwValue;
		reg_config[20].reg_addr = ISP_GAMMA_NODE_G4_V0002 - ISP_BASE_ADDR;
		reg_config[20].reg_value = reg_g4_ptr->dwValue;
		reg_config[21].reg_addr = ISP_GAMMA_NODE_G5_V0002 - ISP_BASE_ADDR;
		reg_config[21].reg_value = reg_g5_ptr->dwValue;
		reg_config[22].reg_addr = ISP_GAMMA_NODE_G6_V0002 - ISP_BASE_ADDR;
		reg_config[22].reg_value = reg_g6_ptr->dwValue;
		reg_config[23].reg_addr = ISP_GAMMA_NODE_G7_V0002 - ISP_BASE_ADDR;
		reg_config[23].reg_value = reg_g7_ptr->dwValue;
		reg_config[24].reg_addr = ISP_GAMMA_NODE_G8_V0002 - ISP_BASE_ADDR;
		reg_config[24].reg_value = reg_g8_ptr->dwValue;
		reg_config[25].reg_addr = ISP_GAMMA_NODE_G9_V0002 - ISP_BASE_ADDR;
		reg_config[25].reg_value = reg_g9_ptr->dwValue;
		reg_config[26].reg_addr = ISP_GAMMA_NODE_G10_V0002 - ISP_BASE_ADDR;
		reg_config[26].reg_value = reg_g10_ptr->dwValue;
		reg_config[27].reg_addr = ISP_GAMMA_NODE_G11_V0002 - ISP_BASE_ADDR;
		reg_config[27].reg_value = reg_g11_ptr->dwValue;
		reg_config[28].reg_addr = ISP_GAMMA_NODE_G12_V0002 - ISP_BASE_ADDR;
		reg_config[28].reg_value = reg_g12_ptr->dwValue;
		reg_config[29].reg_addr = ISP_GAMMA_NODE_G13_V0002 - ISP_BASE_ADDR;
		reg_config[29].reg_value = reg_g13_ptr->dwValue;
		reg_config[30].reg_addr = ISP_GAMMA_NODE_G14_V0002 - ISP_BASE_ADDR;
		reg_config[30].reg_value = reg_g14_ptr->dwValue;
		reg_config[31].reg_addr = ISP_GAMMA_NODE_G15_V0002 - ISP_BASE_ADDR;
		reg_config[31].reg_value = reg_g15_ptr->dwValue;

		reg_config[32].reg_addr = ISP_GAMMA_NODE_B0_V0002 - ISP_BASE_ADDR;
		reg_config[32].reg_value = reg_b0_ptr->dwValue;
		reg_config[33].reg_addr = ISP_GAMMA_NODE_B1_V0002 - ISP_BASE_ADDR;
		reg_config[33].reg_value = reg_b1_ptr->dwValue;
		reg_config[34].reg_addr = ISP_GAMMA_NODE_B2_V0002 - ISP_BASE_ADDR;
		reg_config[34].reg_value = reg_b2_ptr->dwValue;
		reg_config[35].reg_addr = ISP_GAMMA_NODE_B3_V0002 - ISP_BASE_ADDR;
		reg_config[35].reg_value = reg_b3_ptr->dwValue;
		reg_config[36].reg_addr = ISP_GAMMA_NODE_B4_V0002 - ISP_BASE_ADDR;
		reg_config[36].reg_value = reg_b4_ptr->dwValue;
		reg_config[37].reg_addr = ISP_GAMMA_NODE_B5_V0002 - ISP_BASE_ADDR;
		reg_config[37].reg_value = reg_b5_ptr->dwValue;
		reg_config[38].reg_addr = ISP_GAMMA_NODE_B6_V0002 - ISP_BASE_ADDR;
		reg_config[38].reg_value = reg_b6_ptr->dwValue;
		reg_config[39].reg_addr = ISP_GAMMA_NODE_B7_V0002 - ISP_BASE_ADDR;
		reg_config[39].reg_value = reg_b7_ptr->dwValue;
		reg_config[40].reg_addr = ISP_GAMMA_NODE_B8_V0002 - ISP_BASE_ADDR;
		reg_config[40].reg_value = reg_b8_ptr->dwValue;
		reg_config[41].reg_addr = ISP_GAMMA_NODE_B9_V0002 - ISP_BASE_ADDR;
		reg_config[41].reg_value = reg_b9_ptr->dwValue;
		reg_config[42].reg_addr = ISP_GAMMA_NODE_B10_V0002 - ISP_BASE_ADDR;
		reg_config[42].reg_value = reg_b10_ptr->dwValue;
		reg_config[43].reg_addr = ISP_GAMMA_NODE_B11_V0002 - ISP_BASE_ADDR;
		reg_config[43].reg_value = reg_b11_ptr->dwValue;
		reg_config[44].reg_addr = ISP_GAMMA_NODE_B12_V0002 - ISP_BASE_ADDR;
		reg_config[44].reg_value = reg_b12_ptr->dwValue;
		reg_config[45].reg_addr = ISP_GAMMA_NODE_B13_V0002 - ISP_BASE_ADDR;
		reg_config[45].reg_value = reg_b13_ptr->dwValue;
		reg_config[46].reg_addr = ISP_GAMMA_NODE_B14_V0002 - ISP_BASE_ADDR;
		reg_config[46].reg_value = reg_b14_ptr->dwValue;
		reg_config[47].reg_addr = ISP_GAMMA_NODE_B15_V0002 - ISP_BASE_ADDR;
		reg_config[47].reg_value = reg_b15_ptr->dwValue;


		write_param.reg_param = (unsigned long)&reg_config[0];
		write_param.counts = 48;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}
#else

union _isp_gamma_node_x0_v0002_tag reg_rx0;
union _isp_gamma_node_x1_v0002_tag reg_rx1;
union _isp_gamma_node_x2_v0002_tag reg_rx2;
union _isp_gamma_node_x3_v0002_tag reg_rx3;
union _isp_gamma_node_x4_v0002_tag reg_rx4;
union _isp_gamma_node_x5_v0002_tag reg_rx5;

union _isp_gamma_node_x0_v0002_tag reg_gx0;
union _isp_gamma_node_x1_v0002_tag reg_gx1;
union _isp_gamma_node_x2_v0002_tag reg_gx2;
union _isp_gamma_node_x3_v0002_tag reg_gx3;
union _isp_gamma_node_x4_v0002_tag reg_gx4;
union _isp_gamma_node_x5_v0002_tag reg_gx5;

union _isp_gamma_node_x0_v0002_tag reg_bx0;
union _isp_gamma_node_x1_v0002_tag reg_bx1;
union _isp_gamma_node_x2_v0002_tag reg_bx2;
union _isp_gamma_node_x3_v0002_tag reg_bx3;
union _isp_gamma_node_x4_v0002_tag reg_bx4;
union _isp_gamma_node_x5_v0002_tag reg_bx5;



int32_t ispSetGammaXNode_v002(uint32_t handler_id, uint16_t* node)
{
//	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_gamma_node_x0_v0002_tag* reg_rx0_ptr = &reg_rx0;
	union _isp_gamma_node_x1_v0002_tag* reg_rx1_ptr = &reg_rx1;
	union _isp_gamma_node_x2_v0002_tag* reg_rx2_ptr = &reg_rx2;
	union _isp_gamma_node_x3_v0002_tag* reg_rx3_ptr = &reg_rx3;
	union _isp_gamma_node_x4_v0002_tag* reg_rx4_ptr = &reg_rx4;
	union _isp_gamma_node_x5_v0002_tag* reg_rx5_ptr = &reg_rx5;

	union _isp_gamma_node_x0_v0002_tag* reg_gx0_ptr = &reg_gx0;
	union _isp_gamma_node_x1_v0002_tag* reg_gx1_ptr = &reg_gx1;
	union _isp_gamma_node_x2_v0002_tag* reg_gx2_ptr = &reg_gx2;
	union _isp_gamma_node_x3_v0002_tag* reg_gx3_ptr = &reg_gx3;
	union _isp_gamma_node_x4_v0002_tag* reg_gx4_ptr = &reg_gx4;
	union _isp_gamma_node_x5_v0002_tag* reg_gx5_ptr = &reg_gx5;

	union _isp_gamma_node_x0_v0002_tag* reg_bx0_ptr = &reg_bx0;
	union _isp_gamma_node_x1_v0002_tag* reg_bx1_ptr = &reg_bx1;
	union _isp_gamma_node_x2_v0002_tag* reg_bx2_ptr = &reg_bx2;
	union _isp_gamma_node_x3_v0002_tag* reg_bx3_ptr = &reg_bx3;
	union _isp_gamma_node_x4_v0002_tag* reg_bx4_ptr = &reg_bx4;
	union _isp_gamma_node_x5_v0002_tag* reg_bx5_ptr = &reg_bx5;

	struct isp_reg_bits reg_config[18];
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();
	uint32_t i;

	if(SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		if(0==node[0]&&0==node[4]&&0==node[8]&&0==node[12]){
			ALOGI("ispSetGammaXNode_v002 gamma param error, use default gamma para!!!!!!");


		i = 0;
		reg_rx0_ptr->mBits.node0=8;
		reg_rx0_ptr->mBits.node1=16;
		reg_rx0_ptr->mBits.node2=24;
		reg_rx1_ptr->mBits.node3=32;
		reg_rx1_ptr->mBits.node4=48;
		reg_rx1_ptr->mBits.node5=80;
		reg_rx2_ptr->mBits.node6=96;
		reg_rx2_ptr->mBits.node7=160;
		reg_rx2_ptr->mBits.node8=224;
		reg_rx3_ptr->mBits.node9=288;
		reg_rx3_ptr->mBits.node10=384;
		reg_rx3_ptr->mBits.node11=512;
		reg_rx4_ptr->mBits.node12=640;
		reg_rx4_ptr->mBits.node13=768;
		reg_rx4_ptr->mBits.node14=960;
		reg_rx5_ptr->mBits.node15=1023;

		reg_gx0_ptr->mBits.node0=8;
		reg_gx0_ptr->mBits.node1=16;
		reg_gx0_ptr->mBits.node2=24;
		reg_gx1_ptr->mBits.node3=32;
		reg_gx1_ptr->mBits.node4=48;
		reg_gx1_ptr->mBits.node5=80;
		reg_gx2_ptr->mBits.node6=96;
		reg_gx2_ptr->mBits.node7=160;
		reg_gx2_ptr->mBits.node8=224;
		reg_gx3_ptr->mBits.node9=288;
		reg_gx3_ptr->mBits.node10=384;
		reg_gx3_ptr->mBits.node11=512;
		reg_gx4_ptr->mBits.node12=640;
		reg_gx4_ptr->mBits.node13=768;
		reg_gx4_ptr->mBits.node14=960;
		reg_gx5_ptr->mBits.node15=1023;

		reg_bx0_ptr->mBits.node0=8;
		reg_bx0_ptr->mBits.node1=16;
		reg_bx0_ptr->mBits.node2=24;
		reg_bx1_ptr->mBits.node3=32;
		reg_bx1_ptr->mBits.node4=48;
		reg_bx1_ptr->mBits.node5=80;
		reg_bx2_ptr->mBits.node6=96;
		reg_bx2_ptr->mBits.node7=160;
		reg_bx2_ptr->mBits.node8=224;
		reg_bx3_ptr->mBits.node9=288;
		reg_bx3_ptr->mBits.node10=384;
		reg_bx3_ptr->mBits.node11=512;
		reg_bx4_ptr->mBits.node12=640;
		reg_bx4_ptr->mBits.node13=768;
		reg_bx4_ptr->mBits.node14=960;
		reg_bx5_ptr->mBits.node15=1023;
		}else{
			i = 0;
			reg_rx0_ptr->mBits.node0=node[i++];
			reg_rx0_ptr->mBits.node1=node[i++];
			reg_rx0_ptr->mBits.node2=node[i++];
			reg_rx1_ptr->mBits.node3=node[i++];
			reg_rx1_ptr->mBits.node4=node[i++];
			reg_rx1_ptr->mBits.node5=node[i++];
			reg_rx2_ptr->mBits.node6=node[i++];
			reg_rx2_ptr->mBits.node7=node[i++];
			reg_rx2_ptr->mBits.node8=node[i++];
			reg_rx3_ptr->mBits.node9=node[i++];
			reg_rx3_ptr->mBits.node10=node[i++];
			reg_rx3_ptr->mBits.node11=node[i++];
			reg_rx4_ptr->mBits.node12=node[i++];
			reg_rx4_ptr->mBits.node13=node[i++];
			reg_rx4_ptr->mBits.node14=node[i++];
			reg_rx5_ptr->mBits.node15=node[i++];

			i = 0;
			reg_gx0_ptr->mBits.node0=node[i++];
			reg_gx0_ptr->mBits.node1=node[i++];
			reg_gx0_ptr->mBits.node2=node[i++];
			reg_gx1_ptr->mBits.node3=node[i++];
			reg_gx1_ptr->mBits.node4=node[i++];
			reg_gx1_ptr->mBits.node5=node[i++];
			reg_gx2_ptr->mBits.node6=node[i++];
			reg_gx2_ptr->mBits.node7=node[i++];
			reg_gx2_ptr->mBits.node8=node[i++];
			reg_gx3_ptr->mBits.node9=node[i++];
			reg_gx3_ptr->mBits.node10=node[i++];
			reg_gx3_ptr->mBits.node11=node[i++];
			reg_gx4_ptr->mBits.node12=node[i++];
			reg_gx4_ptr->mBits.node13=node[i++];
			reg_gx4_ptr->mBits.node14=node[i++];
			reg_gx5_ptr->mBits.node15=node[i++];
			i = 0;
			reg_bx0_ptr->mBits.node0=node[i++];
			reg_bx0_ptr->mBits.node1=node[i++];
			reg_bx0_ptr->mBits.node2=node[i++];
			reg_bx1_ptr->mBits.node3=node[i++];
			reg_bx1_ptr->mBits.node4=node[i++];
			reg_bx1_ptr->mBits.node5=node[i++];
			reg_bx2_ptr->mBits.node6=node[i++];
			reg_bx2_ptr->mBits.node7=node[i++];
			reg_bx2_ptr->mBits.node8=node[i++];
			reg_bx3_ptr->mBits.node9=node[i++];
			reg_bx3_ptr->mBits.node10=node[i++];
			reg_bx3_ptr->mBits.node11=node[i++];
			reg_bx4_ptr->mBits.node12=node[i++];
			reg_bx4_ptr->mBits.node13=node[i++];
			reg_bx4_ptr->mBits.node14=node[i++];
			reg_bx5_ptr->mBits.node15=node[i++];
		}

		reg_config[0].reg_addr = ISP_GAMMA_NODE_RX0_V0002 - ISP_BASE_ADDR;
		reg_config[0].reg_value = reg_rx0_ptr->dwValue;
		reg_config[1].reg_addr = ISP_GAMMA_NODE_RX1_V0002 - ISP_BASE_ADDR;
		reg_config[1].reg_value = reg_rx1_ptr->dwValue;
		reg_config[2].reg_addr = ISP_GAMMA_NODE_RX2_V0002 - ISP_BASE_ADDR;
		reg_config[2].reg_value = reg_rx2_ptr->dwValue;
		reg_config[3].reg_addr = ISP_GAMMA_NODE_RX3_V0002 - ISP_BASE_ADDR;
		reg_config[3].reg_value = reg_rx3_ptr->dwValue;
		reg_config[4].reg_addr = ISP_GAMMA_NODE_RX4_V0002 - ISP_BASE_ADDR;
		reg_config[4].reg_value = reg_rx4_ptr->dwValue;
		reg_config[5].reg_addr = ISP_GAMMA_NODE_RX5_V0002 - ISP_BASE_ADDR;
		reg_config[5].reg_value = reg_rx5_ptr->dwValue;

		reg_config[6].reg_addr = ISP_GAMMA_NODE_GX0_V0002 - ISP_BASE_ADDR;
		reg_config[6].reg_value = reg_gx0_ptr->dwValue;
		reg_config[7].reg_addr = ISP_GAMMA_NODE_GX1_V0002 - ISP_BASE_ADDR;
		reg_config[7].reg_value = reg_gx1_ptr->dwValue;
		reg_config[8].reg_addr = ISP_GAMMA_NODE_GX2_V0002 - ISP_BASE_ADDR;
		reg_config[8].reg_value = reg_gx2_ptr->dwValue;
		reg_config[9].reg_addr = ISP_GAMMA_NODE_GX3_V0002 - ISP_BASE_ADDR;
		reg_config[9].reg_value = reg_gx3_ptr->dwValue;
		reg_config[10].reg_addr = ISP_GAMMA_NODE_GX4_V0002 - ISP_BASE_ADDR;
		reg_config[10].reg_value = reg_gx4_ptr->dwValue;
		reg_config[11].reg_addr = ISP_GAMMA_NODE_GX5_V0002 - ISP_BASE_ADDR;
		reg_config[11].reg_value = reg_gx5_ptr->dwValue;


		reg_config[12].reg_addr = ISP_GAMMA_NODE_BX0_V0002 - ISP_BASE_ADDR;
		reg_config[12].reg_value = reg_bx0_ptr->dwValue;
		reg_config[13].reg_addr = ISP_GAMMA_NODE_BX1_V0002 - ISP_BASE_ADDR;
		reg_config[13].reg_value = reg_bx1_ptr->dwValue;
		reg_config[14].reg_addr = ISP_GAMMA_NODE_BX2_V0002 - ISP_BASE_ADDR;
		reg_config[14].reg_value = reg_bx2_ptr->dwValue;
		reg_config[15].reg_addr = ISP_GAMMA_NODE_BX3_V0002 - ISP_BASE_ADDR;
		reg_config[15].reg_value = reg_bx3_ptr->dwValue;
		reg_config[16].reg_addr = ISP_GAMMA_NODE_BX4_V0002 - ISP_BASE_ADDR;
		reg_config[16].reg_value = reg_bx4_ptr->dwValue;
		reg_config[17].reg_addr = ISP_GAMMA_NODE_BX5_V0002 - ISP_BASE_ADDR;
		reg_config[17].reg_value = reg_bx5_ptr->dwValue;


		write_param.reg_param = (unsigned long)&reg_config[0];
		write_param.counts = 18;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}


union _isp_gamma_node_y_v0002_tag  reg_r0;
union _isp_gamma_node_y_v0002_tag  reg_r1;
union _isp_gamma_node_y_v0002_tag  reg_r2;
union _isp_gamma_node_y_v0002_tag  reg_r3;
union _isp_gamma_node_y_v0002_tag  reg_r4;
union _isp_gamma_node_y_v0002_tag  reg_r5;
union _isp_gamma_node_y_v0002_tag  reg_r6;
union _isp_gamma_node_y_v0002_tag  reg_r7;
union _isp_gamma_node_y_v0002_tag  reg_r8;
union _isp_gamma_node_y_v0002_tag  reg_r9;
union _isp_gamma_node_y_v0002_tag  reg_r10;
union _isp_gamma_node_y_v0002_tag  reg_r11;
union _isp_gamma_node_y_v0002_tag  reg_r12;
union _isp_gamma_node_y_v0002_tag  reg_r13;
union _isp_gamma_node_y_v0002_tag  reg_r14;
union _isp_gamma_node_y_v0002_tag  reg_r15;


union _isp_gamma_node_y_v0002_tag  reg_g0;
union _isp_gamma_node_y_v0002_tag  reg_g1;
union _isp_gamma_node_y_v0002_tag  reg_g2;
union _isp_gamma_node_y_v0002_tag  reg_g3;
union _isp_gamma_node_y_v0002_tag  reg_g4;
union _isp_gamma_node_y_v0002_tag  reg_g5;
union _isp_gamma_node_y_v0002_tag  reg_g6;
union _isp_gamma_node_y_v0002_tag  reg_g7;
union _isp_gamma_node_y_v0002_tag  reg_g8;
union _isp_gamma_node_y_v0002_tag  reg_g9;
union _isp_gamma_node_y_v0002_tag  reg_g10;
union _isp_gamma_node_y_v0002_tag  reg_g11;
union _isp_gamma_node_y_v0002_tag  reg_g12;
union _isp_gamma_node_y_v0002_tag  reg_g13;
union _isp_gamma_node_y_v0002_tag  reg_g14;
union _isp_gamma_node_y_v0002_tag  reg_g15;

union _isp_gamma_node_y_v0002_tag  reg_b0;
union _isp_gamma_node_y_v0002_tag  reg_b1;
union _isp_gamma_node_y_v0002_tag  reg_b2;
union _isp_gamma_node_y_v0002_tag  reg_b3;
union _isp_gamma_node_y_v0002_tag  reg_b4;
union _isp_gamma_node_y_v0002_tag  reg_b5;
union _isp_gamma_node_y_v0002_tag  reg_b6;
union _isp_gamma_node_y_v0002_tag  reg_b7;
union _isp_gamma_node_y_v0002_tag  reg_b8;
union _isp_gamma_node_y_v0002_tag  reg_b9;
union _isp_gamma_node_y_v0002_tag  reg_b10;
union _isp_gamma_node_y_v0002_tag  reg_b11;
union _isp_gamma_node_y_v0002_tag  reg_b12;
union _isp_gamma_node_y_v0002_tag  reg_b13;
union _isp_gamma_node_y_v0002_tag  reg_b14;
union _isp_gamma_node_y_v0002_tag  reg_b15;


/*	--
*@
*@
*/
int32_t ispSetGammaYNode_v002(uint32_t handler_id, uint16_t* k, uint16_t* b)
{
//	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_gamma_node_y_v0002_tag* reg_r0_ptr  = &reg_r0;
	union _isp_gamma_node_y_v0002_tag* reg_r1_ptr  = &reg_r1;
	union _isp_gamma_node_y_v0002_tag* reg_r2_ptr  = &reg_r2;
	union _isp_gamma_node_y_v0002_tag* reg_r3_ptr  = &reg_r3;
	union _isp_gamma_node_y_v0002_tag* reg_r4_ptr  = &reg_r4;
	union _isp_gamma_node_y_v0002_tag* reg_r5_ptr  = &reg_r5;
	union _isp_gamma_node_y_v0002_tag* reg_r6_ptr  = &reg_r6;
	union _isp_gamma_node_y_v0002_tag* reg_r7_ptr  = &reg_r7;
	union _isp_gamma_node_y_v0002_tag* reg_r8_ptr  = &reg_r8;
	union _isp_gamma_node_y_v0002_tag* reg_r9_ptr  = &reg_r9;
	union _isp_gamma_node_y_v0002_tag* reg_r10_ptr = &reg_r10;
	union _isp_gamma_node_y_v0002_tag* reg_r11_ptr = &reg_r11;
	union _isp_gamma_node_y_v0002_tag* reg_r12_ptr = &reg_r12;
	union _isp_gamma_node_y_v0002_tag* reg_r13_ptr = &reg_r13;
	union _isp_gamma_node_y_v0002_tag* reg_r14_ptr = &reg_r14;
	union _isp_gamma_node_y_v0002_tag* reg_r15_ptr = &reg_r15;


	union _isp_gamma_node_y_v0002_tag* reg_g0_ptr  =&reg_g0;
	union _isp_gamma_node_y_v0002_tag* reg_g1_ptr  =&reg_g1;
	union _isp_gamma_node_y_v0002_tag* reg_g2_ptr  =&reg_g2;
	union _isp_gamma_node_y_v0002_tag* reg_g3_ptr  =&reg_g3;
	union _isp_gamma_node_y_v0002_tag* reg_g4_ptr  =&reg_g4;
	union _isp_gamma_node_y_v0002_tag* reg_g5_ptr  =&reg_g5;
	union _isp_gamma_node_y_v0002_tag* reg_g6_ptr  =&reg_g6;
	union _isp_gamma_node_y_v0002_tag* reg_g7_ptr  =&reg_g7;
	union _isp_gamma_node_y_v0002_tag* reg_g8_ptr  =&reg_g8;
	union _isp_gamma_node_y_v0002_tag* reg_g9_ptr  =&reg_g9;
	union _isp_gamma_node_y_v0002_tag* reg_g10_ptr =&reg_g10;
	union _isp_gamma_node_y_v0002_tag* reg_g11_ptr =&reg_g11;
	union _isp_gamma_node_y_v0002_tag* reg_g12_ptr =&reg_g12;
	union _isp_gamma_node_y_v0002_tag* reg_g13_ptr =&reg_g13;
	union _isp_gamma_node_y_v0002_tag* reg_g14_ptr =&reg_g14;
	union _isp_gamma_node_y_v0002_tag* reg_g15_ptr =&reg_g15;

	union _isp_gamma_node_y_v0002_tag* reg_b0_ptr  =&reg_b0;
	union _isp_gamma_node_y_v0002_tag* reg_b1_ptr  =&reg_b1;
	union _isp_gamma_node_y_v0002_tag* reg_b2_ptr  =&reg_b2;
	union _isp_gamma_node_y_v0002_tag* reg_b3_ptr  =&reg_b3;
	union _isp_gamma_node_y_v0002_tag* reg_b4_ptr  =&reg_b4;
	union _isp_gamma_node_y_v0002_tag* reg_b5_ptr  =&reg_b5;
	union _isp_gamma_node_y_v0002_tag* reg_b6_ptr  =&reg_b6;
	union _isp_gamma_node_y_v0002_tag* reg_b7_ptr  =&reg_b7;
	union _isp_gamma_node_y_v0002_tag* reg_b8_ptr  =&reg_b8;
	union _isp_gamma_node_y_v0002_tag* reg_b9_ptr  =&reg_b9;
	union _isp_gamma_node_y_v0002_tag* reg_b10_ptr =&reg_b10;
	union _isp_gamma_node_y_v0002_tag* reg_b11_ptr =&reg_b11;
	union _isp_gamma_node_y_v0002_tag* reg_b12_ptr =&reg_b12;
	union _isp_gamma_node_y_v0002_tag* reg_b13_ptr =&reg_b13;
	union _isp_gamma_node_y_v0002_tag* reg_b14_ptr =&reg_b14;
	union _isp_gamma_node_y_v0002_tag* reg_b15_ptr =&reg_b15;

	struct isp_reg_bits reg_config[48];
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();
	uint32_t i=0;

	if(SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

#if 0
		i = 0;
		reg_r0_ptr->mBits.k=589;
		reg_r0_ptr->mBits.b=0;
		reg_r1_ptr->mBits.k=589;
		reg_r1_ptr->mBits.b=0;
		reg_r2_ptr->mBits.k=522;
		reg_r2_ptr->mBits.b=5;
		reg_r3_ptr->mBits.k=522;
		reg_r3_ptr->mBits.b=5;
		reg_r4_ptr->mBits.k=432;
		reg_r4_ptr->mBits.b=17;
		reg_r5_ptr->mBits.k=432;
		reg_r5_ptr->mBits.b=17;
		reg_r6_ptr->mBits.k=320;
		reg_r6_ptr->mBits.b=40;
		reg_r7_ptr->mBits.k=320;
		reg_r7_ptr->mBits.b=40;
		reg_r8_ptr->mBits.k=234;
		reg_r8_ptr->mBits.b=67;
		reg_r9_ptr->mBits.k=234;
		reg_r9_ptr->mBits.b=67;
		reg_r10_ptr->mBits.k=171;
		reg_r10_ptr->mBits.b=94;
		reg_r11_ptr->mBits.k=171;
		reg_r11_ptr->mBits.b=94;
		reg_r12_ptr->mBits.k=144;
		reg_r12_ptr->mBits.b=111;
		reg_r13_ptr->mBits.k=144;
		reg_r13_ptr->mBits.b=111;
		reg_r14_ptr->mBits.k=144;
		reg_r14_ptr->mBits.b=111;
		reg_r15_ptr->mBits.k=144;
		reg_r15_ptr->mBits.b=111;


		reg_g0_ptr->mBits.k=589;
		reg_g0_ptr->mBits.b=0;
		reg_g1_ptr->mBits.k=589;
		reg_g1_ptr->mBits.b=0;
		reg_g2_ptr->mBits.k=522;
		reg_g2_ptr->mBits.b=5;
		reg_g3_ptr->mBits.k=522;
		reg_g3_ptr->mBits.b=5;
		reg_g4_ptr->mBits.k=432;
		reg_g4_ptr->mBits.b=17;
		reg_g5_ptr->mBits.k=432;
		reg_g5_ptr->mBits.b=17;
		reg_g6_ptr->mBits.k=320;
		reg_g6_ptr->mBits.b=40;
		reg_g7_ptr->mBits.k=320;
		reg_g7_ptr->mBits.b=40;
		reg_g8_ptr->mBits.k=234;
		reg_g8_ptr->mBits.b=67;
		reg_g9_ptr->mBits.k=234;
		reg_g9_ptr->mBits.b=67;
		reg_g10_ptr->mBits.k=171;
		reg_g10_ptr->mBits.b=94;
		reg_g11_ptr->mBits.k=171;
		reg_g11_ptr->mBits.b=94;
		reg_g12_ptr->mBits.k=144;
		reg_g12_ptr->mBits.b=111;
		reg_g13_ptr->mBits.k=144;
		reg_g13_ptr->mBits.b=111;
		reg_g14_ptr->mBits.k=144;
		reg_g14_ptr->mBits.b=111;
		reg_g15_ptr->mBits.k=144;
		reg_g15_ptr->mBits.b=111;


		reg_b0_ptr->mBits.k=589;
		reg_b0_ptr->mBits.b=0;
		reg_b1_ptr->mBits.k=589;
		reg_b1_ptr->mBits.b=0;
		reg_b2_ptr->mBits.k=522;
		reg_b2_ptr->mBits.b=5;
		reg_b3_ptr->mBits.k=522;
		reg_b3_ptr->mBits.b=5;
		reg_b4_ptr->mBits.k=432;
		reg_b4_ptr->mBits.b=17;
		reg_b5_ptr->mBits.k=432;
		reg_b5_ptr->mBits.b=17;
		reg_b6_ptr->mBits.k=320;
		reg_b6_ptr->mBits.b=40;
		reg_b7_ptr->mBits.k=320;
		reg_b7_ptr->mBits.b=40;
		reg_b8_ptr->mBits.k=234;
		reg_b8_ptr->mBits.b=67;
		reg_b9_ptr->mBits.k=234;
		reg_b9_ptr->mBits.b=67;
		reg_b10_ptr->mBits.k=171;
		reg_b10_ptr->mBits.b=94;
		reg_b11_ptr->mBits.k=171;
		reg_b11_ptr->mBits.b=94;
		reg_b12_ptr->mBits.k=144;
		reg_b12_ptr->mBits.b=111;
		reg_b13_ptr->mBits.k=144;
		reg_b13_ptr->mBits.b=111;
		reg_b14_ptr->mBits.k=144;
		reg_b14_ptr->mBits.b=111;
		reg_b15_ptr->mBits.k=144;
		reg_b15_ptr->mBits.b=111;
#else
		if(0==k[0]&&0==k[4]&&0==k[8]&&0==k[12]){
			ALOGI("ispSetGammaYNode_v002 gamma param error, use default gamma para!!!!!!");


		reg_r0_ptr->mBits.k=0x0480;
		reg_r0_ptr->mBits.b=0x0000;
		reg_r1_ptr->mBits.k=0x0400;
		reg_r1_ptr->mBits.b=0x0001;
		reg_r2_ptr->mBits.k=0x0380;
		reg_r2_ptr->mBits.b=0x0003;
		reg_r3_ptr->mBits.k=0x0380;
		reg_r3_ptr->mBits.b=0x0003;
		reg_r4_ptr->mBits.k=0x0280;
		reg_r4_ptr->mBits.b=0x000b;
		reg_r5_ptr->mBits.k=0x0220;
		reg_r5_ptr->mBits.b=0x0010;
		reg_r6_ptr->mBits.k=0x0280;
		reg_r6_ptr->mBits.b=0x0008;
		reg_r7_ptr->mBits.k=0x0220;
		reg_r7_ptr->mBits.b=0x0011;
		reg_r8_ptr->mBits.k=0x0180;
		reg_r8_ptr->mBits.b=0x002a;
		reg_r9_ptr->mBits.k=0x0110;
		reg_r9_ptr->mBits.b=0x0043;
		reg_r10_ptr->mBits.k=0x00d5;
		reg_r10_ptr->mBits.b=0x0053;
		reg_r11_ptr->mBits.k=0x00b8;
		reg_r11_ptr->mBits.b=0x005e;
		reg_r12_ptr->mBits.k=0x00a0;
		reg_r12_ptr->mBits.b=0x006a;
		reg_r13_ptr->mBits.k=0x0090;
		reg_r13_ptr->mBits.b=0x0074;
		reg_r14_ptr->mBits.k=0x0075;
		reg_r14_ptr->mBits.b=0x0088;
		reg_r15_ptr->mBits.k=0x0092;
		reg_r15_ptr->mBits.b=0x006d;

		reg_g0_ptr->mBits.k=0x0480;
		reg_g0_ptr->mBits.b=0x0000;
		reg_g1_ptr->mBits.k=0x0400;
		reg_g1_ptr->mBits.b=0x0001;
		reg_g2_ptr->mBits.k=0x0380;
		reg_g2_ptr->mBits.b=0x0003;
		reg_g3_ptr->mBits.k=0x0380;
		reg_g3_ptr->mBits.b=0x0003;
		reg_g4_ptr->mBits.k=0x0280;
		reg_g4_ptr->mBits.b=0x000b;
		reg_g5_ptr->mBits.k=0x0220;
		reg_g5_ptr->mBits.b=0x0010;
		reg_g6_ptr->mBits.k=0x0280;
		reg_g6_ptr->mBits.b=0x0008;
		reg_g7_ptr->mBits.k=0x0220;
		reg_g7_ptr->mBits.b=0x0011;
		reg_g8_ptr->mBits.k=0x0180;
		reg_g8_ptr->mBits.b=0x002a;
		reg_g9_ptr->mBits.k=0x0110;
		reg_g9_ptr->mBits.b=0x0043;
		reg_g10_ptr->mBits.k=0x00d5;
		reg_g10_ptr->mBits.b=0x0053;
		reg_g11_ptr->mBits.k=0x00b8;
		reg_g11_ptr->mBits.b=0x005e;
		reg_g12_ptr->mBits.k=0x00a0;
		reg_g12_ptr->mBits.b=0x006a;
		reg_g13_ptr->mBits.k=0x0090;
		reg_g13_ptr->mBits.b=0x0074;
		reg_g14_ptr->mBits.k=0x0075;
		reg_g14_ptr->mBits.b=0x0088;
		reg_g15_ptr->mBits.k=0x0092;
		reg_g15_ptr->mBits.b=0x006d;

		reg_b0_ptr->mBits.k=0x0480;
		reg_b0_ptr->mBits.b=0x0000;
		reg_b1_ptr->mBits.k=0x0400;
		reg_b1_ptr->mBits.b=0x0001;
		reg_b2_ptr->mBits.k=0x0380;
		reg_b2_ptr->mBits.b=0x0003;
		reg_b3_ptr->mBits.k=0x0380;
		reg_b3_ptr->mBits.b=0x0003;
		reg_b4_ptr->mBits.k=0x0280;
		reg_b4_ptr->mBits.b=0x000b;
		reg_b5_ptr->mBits.k=0x0220;
		reg_b5_ptr->mBits.b=0x0010;
		reg_b6_ptr->mBits.k=0x0280;
		reg_b6_ptr->mBits.b=0x0008;
		reg_b7_ptr->mBits.k=0x0220;
		reg_b7_ptr->mBits.b=0x0011;
		reg_b8_ptr->mBits.k=0x0180;
		reg_b8_ptr->mBits.b=0x002a;
		reg_b9_ptr->mBits.k=0x0110;
		reg_b9_ptr->mBits.b=0x0043;
		reg_b10_ptr->mBits.k=0x00d5;
		reg_b10_ptr->mBits.b=0x0053;
		reg_b11_ptr->mBits.k=0x00b8;
		reg_b11_ptr->mBits.b=0x005e;
		reg_b12_ptr->mBits.k=0x00a0;
		reg_b12_ptr->mBits.b=0x006a;
		reg_b13_ptr->mBits.k=0x0090;
		reg_b13_ptr->mBits.b=0x0074;
		reg_b14_ptr->mBits.k=0x0075;
		reg_b14_ptr->mBits.b=0x0088;
		reg_b15_ptr->mBits.k=0x0092;
		reg_b15_ptr->mBits.b=0x006d;
		}else{
			i=0;
			reg_r0_ptr->mBits.k=k[i];
			reg_r0_ptr->mBits.b=b[i++];
			reg_r1_ptr->mBits.k=k[i];
			reg_r1_ptr->mBits.b=b[i++];
			reg_r2_ptr->mBits.k=k[i];
			reg_r2_ptr->mBits.b=b[i++];
			reg_r3_ptr->mBits.k=k[i];
			reg_r3_ptr->mBits.b=b[i++];
			reg_r4_ptr->mBits.k=k[i];
			reg_r4_ptr->mBits.b=b[i++];
			reg_r5_ptr->mBits.k=k[i];
			reg_r5_ptr->mBits.b=b[i++];
			reg_r6_ptr->mBits.k=k[i];
			reg_r6_ptr->mBits.b=b[i++];
			reg_r7_ptr->mBits.k=k[i];
			reg_r7_ptr->mBits.b=b[i++];
			reg_r8_ptr->mBits.k=k[i];
			reg_r8_ptr->mBits.b=b[i++];
			reg_r9_ptr->mBits.k=k[i];
			reg_r9_ptr->mBits.b=b[i++];
			reg_r10_ptr->mBits.k=k[i];
			reg_r10_ptr->mBits.b=b[i++];
			reg_r11_ptr->mBits.k=k[i];
			reg_r11_ptr->mBits.b=b[i++];
			reg_r12_ptr->mBits.k=k[i];
			reg_r12_ptr->mBits.b=b[i++];
			reg_r13_ptr->mBits.k=k[i];
			reg_r13_ptr->mBits.b=b[i++];
			reg_r14_ptr->mBits.k=k[i];
			reg_r14_ptr->mBits.b=b[i++];
			reg_r15_ptr->mBits.k=k[i];
			reg_r15_ptr->mBits.b=b[i++];

			i=0;
			reg_g0_ptr->mBits.k=k[i];
			reg_g0_ptr->mBits.b=b[i++];
			reg_g1_ptr->mBits.k=k[i];
			reg_g1_ptr->mBits.b=b[i++];
			reg_g2_ptr->mBits.k=k[i];
			reg_g2_ptr->mBits.b=b[i++];
			reg_g3_ptr->mBits.k=k[i];
			reg_g3_ptr->mBits.b=b[i++];
			reg_g4_ptr->mBits.k=k[i];
			reg_g4_ptr->mBits.b=b[i++];
			reg_g5_ptr->mBits.k=k[i];
			reg_g5_ptr->mBits.b=b[i++];
			reg_g6_ptr->mBits.k=k[i];
			reg_g6_ptr->mBits.b=b[i++];
			reg_g7_ptr->mBits.k=k[i];
			reg_g7_ptr->mBits.b=b[i++];
			reg_g8_ptr->mBits.k=k[i];
			reg_g8_ptr->mBits.b=b[i++];
			reg_g9_ptr->mBits.k=k[i];
			reg_g9_ptr->mBits.b=b[i++];
			reg_g10_ptr->mBits.k=k[i];
			reg_g10_ptr->mBits.b=b[i++];
			reg_g11_ptr->mBits.k=k[i];
			reg_g11_ptr->mBits.b=b[i++];
			reg_g12_ptr->mBits.k=k[i];
			reg_g12_ptr->mBits.b=b[i++];
			reg_g13_ptr->mBits.k=k[i];
			reg_g13_ptr->mBits.b=b[i++];
			reg_g14_ptr->mBits.k=k[i];
			reg_g14_ptr->mBits.b=b[i++];
			reg_g15_ptr->mBits.k=k[i];
			reg_g15_ptr->mBits.b=b[i++];
			i=0;
			reg_b0_ptr->mBits.k=k[i];
			reg_b0_ptr->mBits.b=b[i++];
			reg_b1_ptr->mBits.k=k[i];
			reg_b1_ptr->mBits.b=b[i++];
			reg_b2_ptr->mBits.k=k[i];
			reg_b2_ptr->mBits.b=b[i++];
			reg_b3_ptr->mBits.k=k[i];
			reg_b3_ptr->mBits.b=b[i++];
			reg_b4_ptr->mBits.k=k[i];
			reg_b4_ptr->mBits.b=b[i++];
			reg_b5_ptr->mBits.k=k[i];
			reg_b5_ptr->mBits.b=b[i++];
			reg_b6_ptr->mBits.k=k[i];
			reg_b6_ptr->mBits.b=b[i++];
			reg_b7_ptr->mBits.k=k[i];
			reg_b7_ptr->mBits.b=b[i++];
			reg_b8_ptr->mBits.k=k[i];
			reg_b8_ptr->mBits.b=b[i++];
			reg_b9_ptr->mBits.k=k[i];
			reg_b9_ptr->mBits.b=b[i++];
			reg_b10_ptr->mBits.k=k[i];
			reg_b10_ptr->mBits.b=b[i++];
			reg_b11_ptr->mBits.k=k[i];
			reg_b11_ptr->mBits.b=b[i++];
			reg_b12_ptr->mBits.k=k[i];
			reg_b12_ptr->mBits.b=b[i++];
			reg_b13_ptr->mBits.k=k[i];
			reg_b13_ptr->mBits.b=b[i++];
			reg_b14_ptr->mBits.k=k[i];
			reg_b14_ptr->mBits.b=b[i++];
			reg_b15_ptr->mBits.k=k[i];
			reg_b15_ptr->mBits.b=b[i++];
		}
#endif
		i = 0;
		reg_config[0].reg_addr = ISP_GAMMA_NODE_R0_V0002 - ISP_BASE_ADDR;
		reg_config[0].reg_value = reg_r0_ptr->dwValue;
		reg_config[1].reg_addr = ISP_GAMMA_NODE_R1_V0002 - ISP_BASE_ADDR;
		reg_config[1].reg_value = reg_r1_ptr->dwValue;
		reg_config[2].reg_addr = ISP_GAMMA_NODE_R2_V0002 - ISP_BASE_ADDR;
		reg_config[2].reg_value = reg_r2_ptr->dwValue;
		reg_config[3].reg_addr = ISP_GAMMA_NODE_R3_V0002 - ISP_BASE_ADDR;
		reg_config[3].reg_value = reg_r3_ptr->dwValue;
		reg_config[4].reg_addr = ISP_GAMMA_NODE_R4_V0002 - ISP_BASE_ADDR;
		reg_config[4].reg_value = reg_r4_ptr->dwValue;
		reg_config[5].reg_addr = ISP_GAMMA_NODE_R5_V0002 - ISP_BASE_ADDR;
		reg_config[5].reg_value = reg_r5_ptr->dwValue;
		reg_config[6].reg_addr = ISP_GAMMA_NODE_R6_V0002 - ISP_BASE_ADDR;
		reg_config[6].reg_value = reg_r6_ptr->dwValue;
		reg_config[7].reg_addr = ISP_GAMMA_NODE_R7_V0002 - ISP_BASE_ADDR;
		reg_config[7].reg_value = reg_r7_ptr->dwValue;
		reg_config[8].reg_addr = ISP_GAMMA_NODE_R8_V0002 - ISP_BASE_ADDR;
		reg_config[8].reg_value = reg_r8_ptr->dwValue;
		reg_config[9].reg_addr = ISP_GAMMA_NODE_R9_V0002 - ISP_BASE_ADDR;
		reg_config[9].reg_value = reg_r9_ptr->dwValue;
		reg_config[10].reg_addr = ISP_GAMMA_NODE_R10_V0002 - ISP_BASE_ADDR;
		reg_config[10].reg_value = reg_r10_ptr->dwValue;
		reg_config[11].reg_addr = ISP_GAMMA_NODE_R11_V0002 - ISP_BASE_ADDR;
		reg_config[11].reg_value = reg_r11_ptr->dwValue;
		reg_config[12].reg_addr = ISP_GAMMA_NODE_R12_V0002 - ISP_BASE_ADDR;
		reg_config[12].reg_value = reg_r12_ptr->dwValue;
		reg_config[13].reg_addr = ISP_GAMMA_NODE_R13_V0002 - ISP_BASE_ADDR;
		reg_config[13].reg_value = reg_r13_ptr->dwValue;
		reg_config[14].reg_addr = ISP_GAMMA_NODE_R14_V0002 - ISP_BASE_ADDR;
		reg_config[14].reg_value = reg_r14_ptr->dwValue;
		reg_config[15].reg_addr = ISP_GAMMA_NODE_R15_V0002 - ISP_BASE_ADDR;
		reg_config[15].reg_value = reg_r15_ptr->dwValue;

		i = 16;
		reg_config[16].reg_addr = ISP_GAMMA_NODE_G0_V0002 - ISP_BASE_ADDR;
		reg_config[16].reg_value = reg_g0_ptr->dwValue;
		reg_config[17].reg_addr = ISP_GAMMA_NODE_G1_V0002 - ISP_BASE_ADDR;
		reg_config[17].reg_value = reg_g1_ptr->dwValue;
		reg_config[18].reg_addr = ISP_GAMMA_NODE_G2_V0002 - ISP_BASE_ADDR;
		reg_config[18].reg_value = reg_g2_ptr->dwValue;
		reg_config[19].reg_addr = ISP_GAMMA_NODE_G3_V0002 - ISP_BASE_ADDR;
		reg_config[19].reg_value = reg_g3_ptr->dwValue;
		reg_config[20].reg_addr = ISP_GAMMA_NODE_G4_V0002 - ISP_BASE_ADDR;
		reg_config[20].reg_value = reg_g4_ptr->dwValue;
		reg_config[21].reg_addr = ISP_GAMMA_NODE_G5_V0002 - ISP_BASE_ADDR;
		reg_config[21].reg_value = reg_g5_ptr->dwValue;
		reg_config[22].reg_addr = ISP_GAMMA_NODE_G6_V0002 - ISP_BASE_ADDR;
		reg_config[22].reg_value = reg_g6_ptr->dwValue;
		reg_config[23].reg_addr = ISP_GAMMA_NODE_G7_V0002 - ISP_BASE_ADDR;
		reg_config[23].reg_value = reg_g7_ptr->dwValue;
		reg_config[24].reg_addr = ISP_GAMMA_NODE_G8_V0002 - ISP_BASE_ADDR;
		reg_config[24].reg_value = reg_g8_ptr->dwValue;
		reg_config[25].reg_addr = ISP_GAMMA_NODE_G9_V0002 - ISP_BASE_ADDR;
		reg_config[25].reg_value = reg_g9_ptr->dwValue;
		reg_config[26].reg_addr = ISP_GAMMA_NODE_G10_V0002 - ISP_BASE_ADDR;
		reg_config[26].reg_value = reg_g10_ptr->dwValue;
		reg_config[27].reg_addr = ISP_GAMMA_NODE_G11_V0002 - ISP_BASE_ADDR;
		reg_config[27].reg_value = reg_g11_ptr->dwValue;
		reg_config[28].reg_addr = ISP_GAMMA_NODE_G12_V0002 - ISP_BASE_ADDR;
		reg_config[28].reg_value = reg_g12_ptr->dwValue;
		reg_config[29].reg_addr = ISP_GAMMA_NODE_G13_V0002 - ISP_BASE_ADDR;
		reg_config[29].reg_value = reg_g13_ptr->dwValue;
		reg_config[30].reg_addr = ISP_GAMMA_NODE_G14_V0002 - ISP_BASE_ADDR;
		reg_config[30].reg_value = reg_g14_ptr->dwValue;
		reg_config[31].reg_addr = ISP_GAMMA_NODE_G15_V0002 - ISP_BASE_ADDR;
		reg_config[31].reg_value = reg_g15_ptr->dwValue;

		reg_config[32].reg_addr = ISP_GAMMA_NODE_B0_V0002 - ISP_BASE_ADDR;
		reg_config[32].reg_value = reg_b0_ptr->dwValue;
		reg_config[33].reg_addr = ISP_GAMMA_NODE_B1_V0002 - ISP_BASE_ADDR;
		reg_config[33].reg_value = reg_b1_ptr->dwValue;
		reg_config[34].reg_addr = ISP_GAMMA_NODE_B2_V0002 - ISP_BASE_ADDR;
		reg_config[34].reg_value = reg_b2_ptr->dwValue;
		reg_config[35].reg_addr = ISP_GAMMA_NODE_B3_V0002 - ISP_BASE_ADDR;
		reg_config[35].reg_value = reg_b3_ptr->dwValue;
		reg_config[36].reg_addr = ISP_GAMMA_NODE_B4_V0002 - ISP_BASE_ADDR;
		reg_config[36].reg_value = reg_b4_ptr->dwValue;
		reg_config[37].reg_addr = ISP_GAMMA_NODE_B5_V0002 - ISP_BASE_ADDR;
		reg_config[37].reg_value = reg_b5_ptr->dwValue;
		reg_config[38].reg_addr = ISP_GAMMA_NODE_B6_V0002 - ISP_BASE_ADDR;
		reg_config[38].reg_value = reg_b6_ptr->dwValue;
		reg_config[39].reg_addr = ISP_GAMMA_NODE_B7_V0002 - ISP_BASE_ADDR;
		reg_config[39].reg_value = reg_b7_ptr->dwValue;
		reg_config[40].reg_addr = ISP_GAMMA_NODE_B8_V0002 - ISP_BASE_ADDR;
		reg_config[40].reg_value = reg_b8_ptr->dwValue;
		reg_config[41].reg_addr = ISP_GAMMA_NODE_B9_V0002 - ISP_BASE_ADDR;
		reg_config[41].reg_value = reg_b9_ptr->dwValue;
		reg_config[42].reg_addr = ISP_GAMMA_NODE_B10_V0002 - ISP_BASE_ADDR;
		reg_config[42].reg_value = reg_b10_ptr->dwValue;
		reg_config[43].reg_addr = ISP_GAMMA_NODE_B11_V0002 - ISP_BASE_ADDR;
		reg_config[43].reg_value = reg_b11_ptr->dwValue;
		reg_config[44].reg_addr = ISP_GAMMA_NODE_B12_V0002 - ISP_BASE_ADDR;
		reg_config[44].reg_value = reg_b12_ptr->dwValue;
		reg_config[45].reg_addr = ISP_GAMMA_NODE_B13_V0002 - ISP_BASE_ADDR;
		reg_config[45].reg_value = reg_b13_ptr->dwValue;
		reg_config[46].reg_addr = ISP_GAMMA_NODE_B14_V0002 - ISP_BASE_ADDR;
		reg_config[46].reg_value = reg_b14_ptr->dwValue;
		reg_config[47].reg_addr = ISP_GAMMA_NODE_B15_V0002 - ISP_BASE_ADDR;
		reg_config[47].reg_value = reg_b15_ptr->dwValue;


		write_param.reg_param = (unsigned long)&reg_config[0];
		write_param.counts = 48;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

#endif
/*CCE */
/*	--
*@
*@
*/
int32_t ispGetCCEStatus(uint32_t handler_id, uint32_t* status)
{
/*	union _isp_cce_status_tag* reg_ptr=(union _isp_cce_status_tag*)ISP_CCE_STATUS;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_cce_status_tag* reg_s_ptr=(union _isp_cce_status_tag*)&isp_reg_ptr->CCE_STATUS;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;

	ISP_CHECK_FD;

	reg_config.reg_addr = ISP_CCE_STATUS - ISP_BASE_ADDR;
	read_param.reg_param = (unsigned long)&reg_config;
	read_param.counts = 1;

/*	reg_s_ptr->dwValue=reg_ptr->dwValue;*/
	if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		reg_s_ptr->dwValue = reg_config.reg_value;
		*status = reg_s_ptr->dwValue;
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispSetCCEMatrix(uint32_t handler_id, uint16_t* matrix_ptr)
{
/*	union _isp_cce_matrix0_tag* reg0_ptr=(union _isp_cce_matrix0_tag*)ISP_CCE_MATRIX0;
	union _isp_cce_matrix1_tag* reg1_ptr=(union _isp_cce_matrix1_tag*)ISP_CCE_MATRIX1;
	union _isp_cce_matrix2_tag* reg2_ptr=(union _isp_cce_matrix2_tag*)ISP_CCE_MATRIX2;
	union _isp_cce_matrix3_tag* reg3_ptr=(union _isp_cce_matrix3_tag*)ISP_CCE_MATRIX3;
	union _isp_cce_matrix4_tag* reg4_ptr=(union _isp_cce_matrix4_tag*)ISP_CCE_MATRIX4;
*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_cce_matrix0_tag* reg0_s_ptr=(union _isp_cce_matrix0_tag*)&isp_reg_ptr->CCE_MATRIX0;
	union _isp_cce_matrix1_tag* reg1_s_ptr=(union _isp_cce_matrix1_tag*)&isp_reg_ptr->CCE_MATRIX1;
	union _isp_cce_matrix2_tag* reg2_s_ptr=(union _isp_cce_matrix2_tag*)&isp_reg_ptr->CCE_MATRIX2;
	union _isp_cce_matrix3_tag* reg3_s_ptr=(union _isp_cce_matrix3_tag*)&isp_reg_ptr->CCE_MATRIX3;
	union _isp_cce_matrix4_tag* reg4_s_ptr=(union _isp_cce_matrix4_tag*)&isp_reg_ptr->CCE_MATRIX4;
	struct isp_reg_bits reg_config[5];
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg0_s_ptr->mBits.matrix00=matrix_ptr[0];
	reg0_s_ptr->mBits.matrix01=matrix_ptr[1];
	reg1_s_ptr->mBits.matrix02=matrix_ptr[2];
	reg1_s_ptr->mBits.matrix10=matrix_ptr[3];
	reg2_s_ptr->mBits.matrix11=matrix_ptr[4];
	reg2_s_ptr->mBits.matrix12=matrix_ptr[5];
	reg3_s_ptr->mBits.matrix20=matrix_ptr[6];
	reg3_s_ptr->mBits.matrix21=matrix_ptr[7];
	reg4_s_ptr->mBits.matrix22=matrix_ptr[8];
/*
	reg0_ptr->dwValue=reg0_s_ptr->dwValue;
	reg1_ptr->dwValue=reg1_s_ptr->dwValue;
	reg2_ptr->dwValue=reg2_s_ptr->dwValue;
	reg3_ptr->dwValue=reg3_s_ptr->dwValue;
	reg4_ptr->dwValue=reg4_s_ptr->dwValue;
*/
	reg_config[0].reg_addr = ISP_CCE_MATRIX0 - ISP_BASE_ADDR;
	reg_config[0].reg_value = isp_reg_ptr->CCE_MATRIX0;
	reg_config[1].reg_addr = ISP_CCE_MATRIX1 - ISP_BASE_ADDR;
	reg_config[1].reg_value = isp_reg_ptr->CCE_MATRIX1;
	reg_config[2].reg_addr = ISP_CCE_MATRIX2 - ISP_BASE_ADDR;
	reg_config[2].reg_value = isp_reg_ptr->CCE_MATRIX2;
	reg_config[3].reg_addr = ISP_CCE_MATRIX3 - ISP_BASE_ADDR;
	reg_config[3].reg_value = isp_reg_ptr->CCE_MATRIX3;
	reg_config[4].reg_addr = ISP_CCE_MATRIX4 - ISP_BASE_ADDR;
	reg_config[4].reg_value = isp_reg_ptr->CCE_MATRIX4;

	write_param.reg_param = (unsigned long)&reg_config[0];
	write_param.counts = 5;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetCCEShift(uint32_t handler_id, uint16_t y_shift, uint16_t u_shift, uint16_t v_shift)
{
/*	union _isp_cce_shift_tag* reg_ptr=(union _isp_cce_shift_tag*)ISP_CCE_SHIFT;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_cce_shift_tag* reg_s_ptr=(union _isp_cce_shift_tag*)&isp_reg_ptr->CCE_SHIFT;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.y_shift=y_shift;
	reg_s_ptr->mBits.u_shift=u_shift;
	reg_s_ptr->mBits.v_shift=v_shift;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_CCE_SHIFT - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->CCE_SHIFT;

	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispCCEUVDivBypass(uint32_t handler_id, uint8_t bypass)
{
/*	union _isp_cce_param_tag* reg_ptr=(union _isp_cce_param_tag*)ISP_CCE_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_cce_param_tag* reg_s_ptr=(union _isp_cce_param_tag*)&isp_reg_ptr->CCE_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.bypass=bypass;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_CCE_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->CCE_PARAM;

	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetCCEUVDiv(uint32_t handler_id, uint8_t* div_ptr)
{
/*	union _isp_cce_uvd_thrd0_tag* reg0_ptr=(union _isp_cce_uvd_thrd0_tag*)ISP_CCE_UVD_THRD0;
	union _isp_cce_uvd_thrd1_tag* reg1_ptr=(union _isp_cce_uvd_thrd1_tag*)ISP_CCE_UVD_THRD1;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_cce_uvd_thrd0_tag* reg0_s_ptr=(union _isp_cce_uvd_thrd0_tag*)&isp_reg_ptr->CCE_UVD_THRD0;
	union _isp_cce_uvd_thrd1_tag* reg1_s_ptr=(union _isp_cce_uvd_thrd1_tag*)&isp_reg_ptr->CCE_UVD_THRD1;
	struct isp_reg_bits reg_config[2];
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg0_s_ptr->mBits.thrd_0=div_ptr[0];
	reg0_s_ptr->mBits.thrd_1=div_ptr[1];
	reg0_s_ptr->mBits.thrd_2=div_ptr[2];
	reg0_s_ptr->mBits.thrd_3=div_ptr[3];
	reg1_s_ptr->mBits.thrd_4=div_ptr[4];
	reg1_s_ptr->mBits.thrd_5=div_ptr[5];
	reg1_s_ptr->mBits.thrd_6=div_ptr[6];
/*	reg0_ptr->dwValue=reg0_s_ptr->dwValue;
	reg1_ptr->dwValue=reg1_s_ptr->dwValue;
*/
	reg_config[0].reg_addr = ISP_CCE_UVD_THRD0 - ISP_BASE_ADDR;
	reg_config[0].reg_value = isp_reg_ptr->CCE_UVD_THRD0;
	reg_config[1].reg_addr = ISP_CCE_UVD_THRD1 - ISP_BASE_ADDR;
	reg_config[1].reg_value = isp_reg_ptr->CCE_UVD_THRD1;
	write_param.reg_param = (unsigned long)&reg_config[0];
	write_param.counts = 2;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetCCEUVC(uint32_t handler_id, uint8_t* t_ptr, uint8_t* m_ptr)
{
/*	union _isp_cce_uvc_param0_v0001_tag* reg0_ptr=(union _isp_cce_uvc_param0_v0001_tag*)ISP_CCE_UVD_PARAM0_V0001;
	union _isp_cce_uvc_param1_v0001_tag* reg1_ptr=(union _isp_cce_uvc_param1_v0001_tag*)ISP_CCE_UVD_PARAM1_V0001;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_cce_uvc_param0_v0001_tag* reg0_s_ptr=(union _isp_cce_uvc_param0_v0001_tag*)&isp_reg_ptr->CCE_UVD_PARAM0_V0001;
	union _isp_cce_uvc_param1_v0001_tag* reg1_s_ptr=(union _isp_cce_uvc_param1_v0001_tag*)&isp_reg_ptr->CCE_UVD_PARAM1_V0001;
	struct isp_reg_bits reg_config[2];
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg0_s_ptr->mBits.uv_t1=t_ptr[0];
		reg0_s_ptr->mBits.uv_t2=t_ptr[1];
		reg1_s_ptr->mBits.uv_m=m_ptr[0];
		reg1_s_ptr->mBits.uv_m1=m_ptr[1];
		reg1_s_ptr->mBits.uv_m2=m_ptr[2];
	/*	reg0_ptr->dwValue=reg0_s_ptr->dwValue;
		reg1_ptr->dwValue=reg1_s_ptr->dwValue;
	*/
		reg_config[0].reg_addr = ISP_CCE_UVD_PARAM0_V0001 - ISP_BASE_ADDR;
		reg_config[0].reg_value = reg0_s_ptr->dwValue;
		reg_config[1].reg_addr = ISP_CCE_UVD_PARAM1_V0001 - ISP_BASE_ADDR;
		reg_config[1].reg_value = reg1_s_ptr->dwValue;
		write_param.reg_param = (unsigned long)&reg_config[0];
		write_param.counts = 2;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

/*PREF */
/*	--
*@
*@
*/
int32_t ispGetPrefStatus(uint32_t handler_id, uint32_t* status)
{
/*	union _isp_pref_status_tag* reg_ptr=(union _isp_pref_status_tag*)ISP_PREF_STATUS;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_pref_status_tag* reg_s_ptr=(union _isp_pref_status_tag*)&isp_reg_ptr->PREF_STATUS;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;

	ISP_CHECK_FD;

	reg_config.reg_addr = ISP_PREF_STATUS - ISP_BASE_ADDR;
	read_param.reg_param = (unsigned long)&reg_config;
	read_param.counts = 1;

/*	reg_s_ptr->dwValue=reg_ptr->dwValue;*/
	if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		reg_s_ptr->dwValue = reg_config.reg_value;
		*status = reg_s_ptr->dwValue;
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispPrefBypass(uint32_t handler_id, uint8_t bypass)
{
/*	union _isp_pref_param_tag* reg_ptr=(union _isp_pref_param_tag*)ISP_PREF_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_pref_param_tag* reg_s_ptr=(union _isp_pref_param_tag*)&isp_reg_ptr->PREF_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.bypass=bypass;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_PREF_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->PREF_PARAM;;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispPrefWriteBack(uint32_t handler_id, uint16_t write_back)
{
/*	union _isp_pref_param_tag* reg_ptr=(union _isp_pref_param_tag*)ISP_PREF_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_pref_param_tag* reg_s_ptr=(union _isp_pref_param_tag*)&isp_reg_ptr->PREF_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.write_back=write_back;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_PREF_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->PREF_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetPrefThrd(uint32_t handler_id, uint8_t y_thr, uint8_t u_thr, uint8_t v_thr)
{
/*	union _isp_pref_thrd_tag* reg_ptr=(union _isp_pref_thrd_tag*)ISP_PREF_THRD;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_pref_thrd_tag* reg_s_ptr=(union _isp_pref_thrd_tag*)&isp_reg_ptr->PREF_THRD;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.y_thrd=y_thr;
	/*must be set to 0 or the image will be a little purplish*/
	reg_s_ptr->mBits.u_thrd=0;//u_thr;
	reg_s_ptr->mBits.v_thrd=0;//v_thr;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_PREF_THRD - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->PREF_THRD;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}


/*	--
*@
*@
*/
int32_t ispPrefSliceSize(uint32_t handler_id, uint16_t w, uint16_t h)
{
/*	union _isp_pref_slice_size_tag* reg_ptr=(union _isp_pref_slice_size_tag*)ISP_PREF_SLICE_SIZE;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_pref_slice_size_tag* reg_s_ptr=(union _isp_pref_slice_size_tag*)&isp_reg_ptr->PREF_SLICE_SIZE;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.slice_width=w;
	reg_s_ptr->mBits.slice_height=h;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_PREF_SLICE_SIZE - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->PREF_SLICE_SIZE;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispPrefSliceInfo(uint32_t handler_id, uint16_t edge_info)
{
/*	union _isp_pref_slice_info_tag* reg_ptr=(union _isp_pref_slice_info_tag*)ISP_PREF_SLICE_INFO;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_pref_slice_info_tag* reg_s_ptr=(union _isp_pref_slice_info_tag*)&isp_reg_ptr->PREF_SLICE_INFO;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	/*reg_s_ptr->mBits.edge_left_info=edge_info;*/
	/*reg_s_ptr->mBits.edge_top_info=edge_info;*/
	reg_s_ptr->dwValue=edge_info;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_PREF_SLICE_INFO - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->PREF_SLICE_INFO;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*BRIGHT */
/*	--
*@
*@
*/
int32_t ispGetBrightStatus(uint32_t handler_id, uint32_t* status)
{
/*	union _isp_bright_status_tag* reg_ptr=(union _isp_bright_status_tag*)ISP_BRIGHT_STATUS;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_bright_status_tag* reg_s_ptr=(union _isp_bright_status_tag*)&isp_reg_ptr->BRIGHT_STATUS;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;

	ISP_CHECK_FD;

	reg_config.reg_addr = ISP_BRIGHT_STATUS - ISP_BASE_ADDR;
	read_param.reg_param = (unsigned long)&reg_config;
	read_param.counts = 1;

/*	reg_s_ptr->dwValue=reg_ptr->dwValue;*/
	if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		reg_s_ptr->dwValue = reg_config.reg_value;
		*status = reg_s_ptr->dwValue;
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispBrightBypass(uint32_t handler_id, uint8_t bypass)
{
/*	union _isp_bright_param_tag* reg_ptr=(union _isp_bright_param_tag*)ISP_BRIGHT_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_bright_param_tag* reg_s_ptr=(union _isp_bright_param_tag*)&isp_reg_ptr->BRIGHT_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.bypass=bypass;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_BRIGHT_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->BRIGHT_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetBrightFactor(uint32_t handler_id, uint8_t factor)
{
/*	union _isp_bright_param_tag* reg_ptr=(union _isp_bright_param_tag*)ISP_BRIGHT_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_bright_param_tag* reg_s_ptr=(union _isp_bright_param_tag*)&isp_reg_ptr->BRIGHT_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.bright_factor=factor;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_BRIGHT_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->BRIGHT_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispBrightSliceSize(uint32_t handler_id, uint16_t w, uint16_t h)
{
/*	union _isp_bright_slice_size_tag* reg_ptr=(union _isp_bright_slice_size_tag*)ISP_BRIGHT_SLICE_SIZE;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_bright_slice_size_tag* reg_s_ptr=(union _isp_bright_slice_size_tag*)&isp_reg_ptr->BRIGHT_SLICE_SIZE;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.slice_width=w;
	reg_s_ptr->mBits.slice_height=h;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_BRIGHT_SLICE_SIZE - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->BRIGHT_SLICE_SIZE;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispBrightSliceInfo(uint32_t handler_id, uint16_t edge_info)
{
/*	union _isp_bright_slice_info_tag* reg_ptr=(union _isp_bright_slice_info_tag*)ISP_BRIGHT_SLICE_INFO;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_bright_slice_info_tag* reg_s_ptr=(union _isp_bright_slice_info_tag*)&isp_reg_ptr->BRIGHT_SLICE_INFO;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.edge_info=edge_info;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_BRIGHT_SLICE_INFO - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->BRIGHT_SLICE_INFO;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*CONTRAST */
/*	--
*@
*@
*/
int32_t ispGetContrastStatus(uint32_t handler_id, uint32_t* status)
{
/*	union _isp_contrast_status_tag* reg_ptr=(union _isp_contrast_status_tag*)ISP_CONTRAST_STATUS;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_contrast_status_tag* reg_s_ptr=(union _isp_contrast_status_tag*)&isp_reg_ptr->CONTRAST_STATUS;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;

	ISP_CHECK_FD;

	reg_config.reg_addr = ISP_CONTRAST_STATUS - ISP_BASE_ADDR;
	read_param.reg_param = (unsigned long)&reg_config;
	read_param.counts = 1;

/*	reg_s_ptr->dwValue=reg_ptr->dwValue;*/
	if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		reg_s_ptr->dwValue = reg_config.reg_value;
		*status = reg_s_ptr->dwValue;
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispContrastbypass(uint32_t handler_id, uint8_t bypass)
{
/*	union _isp_contrast_param_tag* reg_ptr=(union _isp_contrast_param_tag*)ISP_CONTRAST_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_contrast_param_tag* reg_s_ptr=(union _isp_contrast_param_tag*)&isp_reg_ptr->CONTRAST_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.bypass=bypass;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_CONTRAST_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->CONTRAST_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetContrastFactor(uint32_t handler_id, uint8_t factor)
{
/*	union _isp_contrast_param_tag* reg_ptr=(union _isp_contrast_param_tag*)ISP_CONTRAST_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_contrast_param_tag* reg_s_ptr=(union _isp_contrast_param_tag*)&isp_reg_ptr->CONTRAST_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.contrast_factor=factor;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_CONTRAST_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->CONTRAST_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*HIST */
/*	--
*@
*@
*/
int32_t ispGetHistStatus(uint32_t handler_id, uint32_t* status)
{
/*	union _isp_hist_status_tag* reg_ptr=(union _isp_hist_status_tag*)ISP_HIST_STATUS;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_hist_status_tag* reg_s_ptr=(union _isp_hist_status_tag*)&isp_reg_ptr->HIST_STATUS;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;

	ISP_CHECK_FD;

	reg_config.reg_addr = ISP_HIST_STATUS - ISP_BASE_ADDR;
	read_param.reg_param = (unsigned long)&reg_config;
	read_param.counts = 1;

/*	reg_s_ptr->dwValue=reg_ptr->dwValue;*/
	if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		reg_s_ptr->dwValue = reg_config.reg_value;
		*status = reg_s_ptr->dwValue;
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispHistbypass(uint32_t handler_id, uint8_t bypass)
{
/*	union _isp_hist_param_tag* reg_ptr=(union _isp_hist_param_tag*)ISP_HIST_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_hist_param_tag* reg_s_ptr=(union _isp_hist_param_tag*)&isp_reg_ptr->HIST_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.bypass=bypass;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_HIST_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->HIST_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispHistAutoRstDisenable(uint32_t handler_id, uint8_t off)
{
/*	union _isp_hist_param_tag* reg_ptr=(union _isp_hist_param_tag*)ISP_HIST_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_hist_param_tag* reg_s_ptr=(union _isp_hist_param_tag*)&isp_reg_ptr->HIST_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.auto_rst_off=off;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_HIST_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->HIST_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispHistMode(uint32_t handler_id, uint8_t mode)
{
/*	union _isp_hist_param_tag* reg_ptr=(union _isp_hist_param_tag*)ISP_HIST_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_hist_param_tag* reg_s_ptr=(union _isp_hist_param_tag*)&isp_reg_ptr->HIST_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.mode=mode;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_HIST_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->HIST_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetHistRatio(uint32_t handler_id, uint16_t low_ratio, uint16_t high_ratio)
{
/*	union _isp_hist_ratio_tag* reg_ptr=(union _isp_hist_ratio_tag*)ISP_HIST_RATIO;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_hist_ratio_tag* reg_s_ptr=(union _isp_hist_ratio_tag*)&isp_reg_ptr->HIST_RATION;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.low_sum_ratio=low_ratio;
	reg_s_ptr->mBits.high_sum_ratio=high_ratio;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_HIST_RATIO - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->HIST_RATION;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetHistMaxMin(uint32_t handler_id, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max)
{
/*	union _isp_hist_max_min_tag* reg_ptr=(union _isp_hist_max_min_tag*)ISP_HIST_MAX_MIN;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_hist_max_min_tag* reg_s_ptr=(union _isp_hist_max_min_tag*)&isp_reg_ptr->HIST_MAX_MIN;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.in_min=in_min;
	reg_s_ptr->mBits.in_max=in_max;
	reg_s_ptr->mBits.out_min=out_min;
	reg_s_ptr->mBits.out_max=out_max;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_HIST_MAX_MIN - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->HIST_MAX_MIN;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispHistClearEn(uint32_t handler_id, uint8_t eb)
{
/*	union _isp_hist_clear_enable_tag* reg_ptr=(union _isp_hist_clear_enable_tag*)ISP_HIST_CLEAR_ENABLE;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_hist_clear_enable_tag* reg_s_ptr=(union _isp_hist_clear_enable_tag*)&isp_reg_ptr->HIST_CLEAR_ENABLE;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.clear_eb=eb;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_HIST_CLEAR_ENABLE - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->HIST_CLEAR_ENABLE;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispGetHISTStatistic(uint32_t handler_id, uint32_t* param_ptr)
{
/*	union _isp_mem_reg_tag* reg_ptr=(union _isp_mem_reg_tag*)ISP_AWBM_OUTPUT;*/
	uint32_t i=0x00;
	uint32_t offset_addr = ISP_AWBM_OUTPUT - ISP_BASE_ADDR;;
	struct isp_reg_bits reg_config[ISP_HIST_ITEM];
	struct isp_reg_param read_param;

	ISP_CHECK_FD;

	for(i=0x00; i<ISP_HIST_ITEM; i++) {
		reg_config[i].reg_addr = offset_addr;
		offset_addr += 4;
	}
	read_param.reg_param = (unsigned long)&reg_config[0];
	read_param.counts = ISP_HIST_ITEM;

	if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		for(i=0x00; i<ISP_HIST_ITEM; i++) {
			param_ptr[i]=reg_config[i].reg_value;
		}
	}

	return ISP_SUCCESS;
}

/*Auto Contrast */
/*	--
*@
*@
*/
int32_t ispGetAutoContrastStatus(uint32_t handler_id, uint32_t* status)
{
	/*union _isp_auto_contrast_status_tag* reg_ptr=(union _isp_auto_contrast_status_tag*)ISP_AUTOCONT_STATUS;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_auto_contrast_status_tag* reg_s_ptr=(union _isp_auto_contrast_status_tag*)&isp_reg_ptr->AUTO_CONTRAST_STATUS;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;

	ISP_CHECK_FD;

	reg_config.reg_addr = ISP_AUTOCONT_STATUS - ISP_BASE_ADDR;
	read_param.reg_param = (unsigned long)&reg_config;
	read_param.counts = 1;

/*	reg_s_ptr->dwValue=reg_ptr->dwValue;*/
	if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		reg_s_ptr->dwValue = reg_config.reg_value;
		*status = reg_s_ptr->dwValue;
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispAutoContrastbypass(uint32_t handler_id, uint8_t bypass)
{
/*	union _isp_auto_contrast_param_tag* reg_ptr=(union _isp_auto_contrast_param_tag*)ISP_AUTOCONT_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_auto_contrast_param_tag* reg_s_ptr=(union _isp_auto_contrast_param_tag*)&isp_reg_ptr->AUTO_CONTRAST_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.bypass=bypass;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_AUTOCONT_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->AUTO_CONTRAST_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetAutoContrastMode(uint32_t handler_id, uint16_t mode)
{
/*	union _isp_auto_contrast_param_tag* reg_ptr=(union _isp_auto_contrast_param_tag*)ISP_AUTOCONT_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_auto_contrast_param_tag* reg_s_ptr=(union _isp_auto_contrast_param_tag*)&isp_reg_ptr->AUTO_CONTRAST_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.mode=mode;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_AUTOCONT_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->AUTO_CONTRAST_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetAutoContrastMaxMin(uint32_t handler_id, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max)
{
/*	union _isp_auto_contrast_max_min_tag* reg_ptr=(union _isp_auto_contrast_max_min_tag*)ISP_AUTOCONT_MAX_MIN;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_auto_contrast_max_min_tag* reg_s_ptr=(union _isp_auto_contrast_max_min_tag*)&isp_reg_ptr->AUTO_CONTRAST_MAX_MIN;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.in_min=in_min;
	reg_s_ptr->mBits.in_max=in_max;
	reg_s_ptr->mBits.out_min=out_min;
	reg_s_ptr->mBits.out_max=out_max;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_AUTOCONT_MAX_MIN - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->AUTO_CONTRAST_MAX_MIN;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*AFM */
/*	--
*@
*@
*/
int32_t ispGetAFMStatus(uint32_t handler_id, uint32_t* status)
{
/*	union _isp_afm_status_tag* reg_ptr=(union _isp_afm_status_tag*)ISP_AFM_STATUS;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_afm_status_tag* reg_s_ptr=(union _isp_afm_status_tag*)&isp_reg_ptr->AFM_STATUS;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;

	ISP_CHECK_FD;

	reg_config.reg_addr = ISP_AFM_STATUS - ISP_BASE_ADDR;
	read_param.reg_param = (unsigned long)&reg_config;
	read_param.counts = 1;

/*	reg_s_ptr->dwValue=reg_ptr->dwValue;*/
	if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		reg_s_ptr->dwValue = reg_config.reg_value;
		*status = reg_s_ptr->dwValue;
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispAFMbypass(uint32_t handler_id, uint8_t bypass)
{
/*	union _isp_afm_param_tag* reg_ptr=(union _isp_afm_param_tag*)ISP_AFM_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_afm_param_tag* reg_s_ptr=(union _isp_afm_param_tag*)&isp_reg_ptr->AFM_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.bypass=bypass;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_AFM_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->AFM_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetAFMShift(uint32_t handler_id, uint16_t shift)
{
/*	union _isp_afm_param_tag* reg_ptr=(union _isp_afm_param_tag*)ISP_AFM_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_afm_param_tag* reg_s_ptr=(union _isp_afm_param_tag*)&isp_reg_ptr->AFM_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.shift=shift;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_AFM_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->AFM_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispAFMMode(uint32_t handler_id, uint8_t mode)
{
/*	union _isp_afm_param_tag* reg_ptr=(union _isp_afm_param_tag*)ISP_AFM_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_afm_param_tag* reg_s_ptr=(union _isp_afm_param_tag*)&isp_reg_ptr->AFM_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.mode=mode;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_AFM_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->AFM_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispAFMSkipNum(uint32_t handler_id, uint8_t num)
{
/*	union _isp_afm_param_tag* reg_ptr=(union _isp_afm_param_tag*)ISP_AFM_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_afm_param_tag* reg_s_ptr=(union _isp_afm_param_tag*)&isp_reg_ptr->AFM_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_s_ptr->mBits.skip_num=num;
	/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
		reg_config.reg_addr = ISP_AFM_PARAM - ISP_BASE_ADDR;
		reg_config.reg_value = reg_s_ptr->dwValue;
		write_param.reg_param = (unsigned long)&reg_config;
		write_param.counts = 1;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispAFMSkipClear(uint32_t handler_id, uint8_t eb)
{
/*	union _isp_afm_param_tag* reg_ptr=(union _isp_afm_param_tag*)ISP_AFM_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_afm_param_tag* reg_s_ptr=(union _isp_afm_param_tag*)&isp_reg_ptr->AFM_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_s_ptr->mBits.skip_clear=eb;
	/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
		reg_config.reg_addr = ISP_AFM_PARAM - ISP_BASE_ADDR;
		reg_config.reg_value = reg_s_ptr->dwValue;
		write_param.reg_param = (unsigned long)&reg_config;
		write_param.counts = 1;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispSetAFMWindow(uint32_t handler_id, uint16_t win[][4])
{
/*	union _isp_afm_win_range0_tag* reg0_ptr=(union _isp_afm_win_range0_tag*)ISP_AFM_WIN_RANGE0;
	union _isp_afm_win_range1_tag* reg1_ptr=(union _isp_afm_win_range1_tag*)ISP_AFM_WIN_RANGE1;
	union _isp_afm_win_range2_tag* reg2_ptr=(union _isp_afm_win_range2_tag*)ISP_AFM_WIN_RANGE2;
	union _isp_afm_win_range3_tag* reg3_ptr=(union _isp_afm_win_range3_tag*)ISP_AFM_WIN_RANGE3;
	union _isp_afm_win_range4_tag* reg4_ptr=(union _isp_afm_win_range4_tag*)ISP_AFM_WIN_RANGE4;
	union _isp_afm_win_range5_tag* reg5_ptr=(union _isp_afm_win_range5_tag*)ISP_AFM_WIN_RANGE5;
	union _isp_afm_win_range6_tag* reg6_ptr=(union _isp_afm_win_range6_tag*)ISP_AFM_WIN_RANGE6;
	union _isp_afm_win_range7_tag* reg7_ptr=(union _isp_afm_win_range7_tag*)ISP_AFM_WIN_RANGE7;
	union _isp_afm_win_range8_tag* reg8_ptr=(union _isp_afm_win_range8_tag*)ISP_AFM_WIN_RANGE8;
	union _isp_afm_win_range9_tag* reg9_ptr=(union _isp_afm_win_range9_tag*)ISP_AFM_WIN_RANGE9;
	union _isp_afm_win_range10_tag* reg10_ptr=(union _isp_afm_win_range10_tag*)ISP_AFM_WIN_RANGE10;
	union _isp_afm_win_range11_tag* reg11_ptr=(union _isp_afm_win_range11_tag*)ISP_AFM_WIN_RANGE11;
	union _isp_afm_win_range12_tag* reg12_ptr=(union _isp_afm_win_range12_tag*)ISP_AFM_WIN_RANGE12;
	union _isp_afm_win_range13_tag* reg13_ptr=(union _isp_afm_win_range13_tag*)ISP_AFM_WIN_RANGE13;
	union _isp_afm_win_range14_tag* reg14_ptr=(union _isp_afm_win_range14_tag*)ISP_AFM_WIN_RANGE14;
	union _isp_afm_win_range15_tag* reg15_ptr=(union _isp_afm_win_range15_tag*)ISP_AFM_WIN_RANGE15;
	union _isp_afm_win_range16_tag* reg16_ptr=(union _isp_afm_win_range16_tag*)ISP_AFM_WIN_RANGE16;
	union _isp_afm_win_range17_tag* reg17_ptr=(union _isp_afm_win_range17_tag*)ISP_AFM_WIN_RANGE17;
*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_afm_win_range0_tag* reg0_s_ptr=(union _isp_afm_win_range0_tag*)&isp_reg_ptr->AFM_WIN_RANGE0;
	union _isp_afm_win_range1_tag* reg1_s_ptr=(union _isp_afm_win_range1_tag*)&isp_reg_ptr->AFM_WIN_RANGE1;
	union _isp_afm_win_range2_tag* reg2_s_ptr=(union _isp_afm_win_range2_tag*)&isp_reg_ptr->AFM_WIN_RANGE2;
	union _isp_afm_win_range3_tag* reg3_s_ptr=(union _isp_afm_win_range3_tag*)&isp_reg_ptr->AFM_WIN_RANGE3;
	union _isp_afm_win_range4_tag* reg4_s_ptr=(union _isp_afm_win_range4_tag*)&isp_reg_ptr->AFM_WIN_RANGE4;
	union _isp_afm_win_range5_tag* reg5_s_ptr=(union _isp_afm_win_range5_tag*)&isp_reg_ptr->AFM_WIN_RANGE5;
	union _isp_afm_win_range6_tag* reg6_s_ptr=(union _isp_afm_win_range6_tag*)&isp_reg_ptr->AFM_WIN_RANGE6;
	union _isp_afm_win_range7_tag* reg7_s_ptr=(union _isp_afm_win_range7_tag*)&isp_reg_ptr->AFM_WIN_RANGE7;
	union _isp_afm_win_range8_tag* reg8_s_ptr=(union _isp_afm_win_range8_tag*)&isp_reg_ptr->AFM_WIN_RANGE8;
	union _isp_afm_win_range9_tag* reg9_s_ptr=(union _isp_afm_win_range9_tag*)&isp_reg_ptr->AFM_WIN_RANGE9;
	union _isp_afm_win_range10_tag* reg10_s_ptr=(union _isp_afm_win_range10_tag*)&isp_reg_ptr->AFM_WIN_RANGE10;
	union _isp_afm_win_range11_tag* reg11_s_ptr=(union _isp_afm_win_range11_tag*)&isp_reg_ptr->AFM_WIN_RANGE11;
	union _isp_afm_win_range12_tag* reg12_s_ptr=(union _isp_afm_win_range12_tag*)&isp_reg_ptr->AFM_WIN_RANGE12;
	union _isp_afm_win_range13_tag* reg13_s_ptr=(union _isp_afm_win_range13_tag*)&isp_reg_ptr->AFM_WIN_RANGE13;
	union _isp_afm_win_range14_tag* reg14_s_ptr=(union _isp_afm_win_range14_tag*)&isp_reg_ptr->AFM_WIN_RANGE14;
	union _isp_afm_win_range15_tag* reg15_s_ptr=(union _isp_afm_win_range15_tag*)&isp_reg_ptr->AFM_WIN_RANGE15;
	union _isp_afm_win_range16_tag* reg16_s_ptr=(union _isp_afm_win_range16_tag*)&isp_reg_ptr->AFM_WIN_RANGE16;
	union _isp_afm_win_range17_tag* reg17_s_ptr=(union _isp_afm_win_range17_tag*)&isp_reg_ptr->AFM_WIN_RANGE17;
	struct isp_reg_bits reg_config[18];
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg0_s_ptr->mBits.win0_start_x=win[0][0];
	reg0_s_ptr->mBits.win0_start_y=win[0][1];
	reg1_s_ptr->mBits.win0_end_x=win[0][2];
	reg1_s_ptr->mBits.win0_end_y=win[0][3];
	reg2_s_ptr->mBits.win1_start_x=win[1][0];
	reg2_s_ptr->mBits.win1_start_y=win[1][1];
	reg3_s_ptr->mBits.win1_end_x=win[1][2];
	reg3_s_ptr->mBits.win1_end_y=win[1][3];
	reg4_s_ptr->mBits.win2_start_x=win[2][0];
	reg4_s_ptr->mBits.win2_start_y=win[2][1];
	reg5_s_ptr->mBits.win2_end_x=win[2][2];
	reg5_s_ptr->mBits.win2_end_y=win[2][3];
	reg6_s_ptr->mBits.win3_start_x=win[3][0];
	reg6_s_ptr->mBits.win3_start_y=win[3][1];
	reg7_s_ptr->mBits.win3_end_x=win[3][2];
	reg7_s_ptr->mBits.win3_end_y=win[3][3];
	reg8_s_ptr->mBits.win4_start_x=win[4][0];
	reg8_s_ptr->mBits.win4_start_y=win[4][1];
	reg9_s_ptr->mBits.win4_end_x=win[4][2];
	reg9_s_ptr->mBits.win4_end_y=win[4][3];
	reg10_s_ptr->mBits.win5_start_x=win[5][0];
	reg10_s_ptr->mBits.win5_start_y=win[5][1];
	reg11_s_ptr->mBits.win5_end_x=win[5][2];
	reg11_s_ptr->mBits.win5_end_y=win[5][3];
	reg12_s_ptr->mBits.win6_start_x=win[6][0];
	reg12_s_ptr->mBits.win6_start_y=win[6][1];
	reg13_s_ptr->mBits.win6_end_x=win[6][2];
	reg13_s_ptr->mBits.win6_end_y=win[6][3];
	reg14_s_ptr->mBits.win7_start_x=win[7][0];
	reg14_s_ptr->mBits.win7_start_y=win[7][1];
	reg15_s_ptr->mBits.win7_end_x=win[7][2];
	reg15_s_ptr->mBits.win7_end_y=win[7][3];
	reg16_s_ptr->mBits.win8_start_x=win[8][0];
	reg16_s_ptr->mBits.win8_start_y=win[8][1];
	reg17_s_ptr->mBits.win8_end_x=win[8][2];
	reg17_s_ptr->mBits.win8_end_y=win[8][3];
/*
	reg0_ptr->dwValue=reg0_s_ptr->dwValue;
	reg1_ptr->dwValue=reg1_s_ptr->dwValue;
	reg2_ptr->dwValue=reg2_s_ptr->dwValue;
	reg3_ptr->dwValue=reg3_s_ptr->dwValue;
	reg4_ptr->dwValue=reg4_s_ptr->dwValue;
	reg5_ptr->dwValue=reg5_s_ptr->dwValue;
	reg6_ptr->dwValue=reg6_s_ptr->dwValue;
	reg7_ptr->dwValue=reg7_s_ptr->dwValue;
	reg8_ptr->dwValue=reg8_s_ptr->dwValue;
	reg9_ptr->dwValue=reg9_s_ptr->dwValue;
	reg10_ptr->dwValue=reg10_s_ptr->dwValue;
	reg11_ptr->dwValue=reg11_s_ptr->dwValue;
	reg12_ptr->dwValue=reg12_s_ptr->dwValue;
	reg13_ptr->dwValue=reg13_s_ptr->dwValue;
	reg14_ptr->dwValue=reg14_s_ptr->dwValue;
	reg15_ptr->dwValue=reg15_s_ptr->dwValue;
	reg16_ptr->dwValue=reg16_s_ptr->dwValue;
	reg17_ptr->dwValue=reg17_s_ptr->dwValue;
*/
	reg_config[0].reg_addr = ISP_AFM_WIN_RANGE0 - ISP_BASE_ADDR;
	reg_config[0].reg_value = isp_reg_ptr->AFM_WIN_RANGE0;
	reg_config[1].reg_addr = ISP_AFM_WIN_RANGE1 - ISP_BASE_ADDR;
	reg_config[1].reg_value = isp_reg_ptr->AFM_WIN_RANGE1;
	reg_config[2].reg_addr = ISP_AFM_WIN_RANGE2 - ISP_BASE_ADDR;
	reg_config[2].reg_value = isp_reg_ptr->AFM_WIN_RANGE2;
	reg_config[3].reg_addr = ISP_AFM_WIN_RANGE3 - ISP_BASE_ADDR;
	reg_config[3].reg_value = isp_reg_ptr->AFM_WIN_RANGE3;
	reg_config[4].reg_addr = ISP_AFM_WIN_RANGE4 - ISP_BASE_ADDR;
	reg_config[4].reg_value = isp_reg_ptr->AFM_WIN_RANGE4;
	reg_config[5].reg_addr = ISP_AFM_WIN_RANGE5 - ISP_BASE_ADDR;
	reg_config[5].reg_value = isp_reg_ptr->AFM_WIN_RANGE5;
	reg_config[6].reg_addr = ISP_AFM_WIN_RANGE6 - ISP_BASE_ADDR;
	reg_config[6].reg_value = isp_reg_ptr->AFM_WIN_RANGE6;
	reg_config[7].reg_addr = ISP_AFM_WIN_RANGE7 - ISP_BASE_ADDR;
	reg_config[7].reg_value = isp_reg_ptr->AFM_WIN_RANGE7;
	reg_config[8].reg_addr = ISP_AFM_WIN_RANGE8 - ISP_BASE_ADDR;
	reg_config[8].reg_value = isp_reg_ptr->AFM_WIN_RANGE8;
	reg_config[9].reg_addr = ISP_AFM_WIN_RANGE9 - ISP_BASE_ADDR;
	reg_config[9].reg_value = isp_reg_ptr->AFM_WIN_RANGE9;
	reg_config[10].reg_addr = ISP_AFM_WIN_RANGE10 - ISP_BASE_ADDR;
	reg_config[10].reg_value = isp_reg_ptr->AFM_WIN_RANGE10;
	reg_config[11].reg_addr = ISP_AFM_WIN_RANGE11 - ISP_BASE_ADDR;
	reg_config[11].reg_value = isp_reg_ptr->AFM_WIN_RANGE11;
	reg_config[12].reg_addr = ISP_AFM_WIN_RANGE12 - ISP_BASE_ADDR;
	reg_config[12].reg_value = isp_reg_ptr->AFM_WIN_RANGE12;
	reg_config[13].reg_addr = ISP_AFM_WIN_RANGE13 - ISP_BASE_ADDR;
	reg_config[13].reg_value = isp_reg_ptr->AFM_WIN_RANGE13;
	reg_config[14].reg_addr = ISP_AFM_WIN_RANGE14 - ISP_BASE_ADDR;
	reg_config[14].reg_value = isp_reg_ptr->AFM_WIN_RANGE14;
	reg_config[15].reg_addr = ISP_AFM_WIN_RANGE15 - ISP_BASE_ADDR;
	reg_config[15].reg_value = isp_reg_ptr->AFM_WIN_RANGE15;
	reg_config[16].reg_addr = ISP_AFM_WIN_RANGE16 - ISP_BASE_ADDR;
	reg_config[16].reg_value = isp_reg_ptr->AFM_WIN_RANGE16;
	reg_config[17].reg_addr = ISP_AFM_WIN_RANGE17 - ISP_BASE_ADDR;
	reg_config[17].reg_value = isp_reg_ptr->AFM_WIN_RANGE17;

	write_param.reg_param = (unsigned long)&reg_config[0];
	write_param.counts = 18;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispGetAFMStatistic(uint32_t handler_id, uint32_t* statistic_ptr)
{
	uint32_t isp_id=_isp_GetIspId();

	if(SC8825_ISP_ID==isp_id){
	/*	union _isp_afm_statistic0_tag* reg0_ptr=(union _isp_afm_statistic0_tag*)ISP_AFM_STATISTIC0;
		union _isp_afm_statistic1_tag* reg1_ptr=(union _isp_afm_statistic1_tag*)ISP_AFM_STATISTIC1;
		union _isp_afm_statistic2_tag* reg2_ptr=(union _isp_afm_statistic2_tag*)ISP_AFM_STATISTIC2;
		union _isp_afm_statistic3_tag* reg3_ptr=(union _isp_afm_statistic3_tag*)ISP_AFM_STATISTIC3;
		union _isp_afm_statistic4_tag* reg4_ptr=(union _isp_afm_statistic4_tag*)ISP_AFM_STATISTIC4;
		union _isp_afm_statistic5_tag* reg5_ptr=(union _isp_afm_statistic5_tag*)ISP_AFM_STATISTIC5;
		union _isp_afm_statistic6_tag* reg6_ptr=(union _isp_afm_statistic6_tag*)ISP_AFM_STATISTIC6;
		union _isp_afm_statistic7_tag* reg7_ptr=(union _isp_afm_statistic7_tag*)ISP_AFM_STATISTIC7;
		union _isp_afm_statistic8_tag* reg8_ptr=(union _isp_afm_statistic8_tag*)ISP_AFM_STATISTIC8;*/
		struct isp_reg_bits reg_config[9];
		struct isp_reg_param read_param;

		ISP_CHECK_FD;

		reg_config[0].reg_addr = ISP_AFM_STATISTIC0 - ISP_BASE_ADDR;
		reg_config[1].reg_addr = ISP_AFM_STATISTIC1 - ISP_BASE_ADDR;
		reg_config[2].reg_addr = ISP_AFM_STATISTIC2 - ISP_BASE_ADDR;
		reg_config[3].reg_addr = ISP_AFM_STATISTIC3 - ISP_BASE_ADDR;
		reg_config[4].reg_addr = ISP_AFM_STATISTIC4 - ISP_BASE_ADDR;
		reg_config[5].reg_addr = ISP_AFM_STATISTIC5 - ISP_BASE_ADDR;
		reg_config[6].reg_addr = ISP_AFM_STATISTIC6 - ISP_BASE_ADDR;
		reg_config[7].reg_addr = ISP_AFM_STATISTIC7 - ISP_BASE_ADDR;
		reg_config[8].reg_addr = ISP_AFM_STATISTIC8 - ISP_BASE_ADDR;
		read_param.reg_param = (unsigned long)&reg_config[0];
		read_param.counts = 9;

		if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
			statistic_ptr[0] = reg_config[0].reg_value;
			statistic_ptr[1] = reg_config[1].reg_value;
			statistic_ptr[2] = reg_config[2].reg_value;
			statistic_ptr[3] = reg_config[3].reg_value;
			statistic_ptr[4] = reg_config[4].reg_value;
			statistic_ptr[5] = reg_config[5].reg_value;
			statistic_ptr[6] = reg_config[6].reg_value;
			statistic_ptr[7] = reg_config[7].reg_value;
			statistic_ptr[8] = reg_config[8].reg_value;
		}
	}else if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id) {
	/*	union _isp_afm_statistic0_tag* reg0_ptr=(union _isp_afm_statistic0_tag*)ISP_AFM_STATISTIC0_L_V0001;
		union _isp_afm_statistic1_tag* reg1_ptr=(union _isp_afm_statistic1_tag*)ISP_AFM_STATISTIC0_H_V0001;
		union _isp_afm_statistic2_tag* reg2_ptr=(union _isp_afm_statistic2_tag*)ISP_AFM_STATISTIC1_L_V0001;
		union _isp_afm_statistic3_tag* reg3_ptr=(union _isp_afm_statistic3_tag*)ISP_AFM_STATISTIC1_H_V0001;
		union _isp_afm_statistic4_tag* reg4_ptr=(union _isp_afm_statistic4_tag*)ISP_AFM_STATISTIC2_L_V0001;
		union _isp_afm_statistic5_tag* reg5_ptr=(union _isp_afm_statistic5_tag*)ISP_AFM_STATISTIC2_H_V0001;
		union _isp_afm_statistic6_tag* reg6_ptr=(union _isp_afm_statistic6_tag*)ISP_AFM_STATISTIC3_L_V0001;
		union _isp_afm_statistic7_tag* reg7_ptr=(union _isp_afm_statistic7_tag*)ISP_AFM_STATISTIC3_H_V0001;
		union _isp_afm_statistic8_tag* reg8_ptr=(union _isp_afm_statistic8_tag*)ISP_AFM_STATISTIC4_L_V0001;
		union _isp_afm_statistic9_tag* reg8_ptr=(union _isp_afm_statistic8_tag*)ISP_AFM_STATISTIC4_H_V0001;
		union _isp_afm_statistic10_tag* reg0_ptr=(union _isp_afm_statistic0_tag*)ISP_AFM_STATISTIC5_L_V0001;
		union _isp_afm_statistic11_tag* reg1_ptr=(union _isp_afm_statistic1_tag*)ISP_AFM_STATISTIC5_H_V0001;
		union _isp_afm_statistic12_tag* reg2_ptr=(union _isp_afm_statistic2_tag*)ISP_AFM_STATISTIC6_L_V0001;
		union _isp_afm_statistic13_tag* reg3_ptr=(union _isp_afm_statistic3_tag*)ISP_AFM_STATISTIC6_H_V0001;
		union _isp_afm_statistic14_tag* reg4_ptr=(union _isp_afm_statistic4_tag*)ISP_AFM_STATISTIC7_L_V0001;
		union _isp_afm_statistic15_tag* reg5_ptr=(union _isp_afm_statistic5_tag*)ISP_AFM_STATISTIC7_H_V0001;
		union _isp_afm_statistic16_tag* reg6_ptr=(union _isp_afm_statistic6_tag*)ISP_AFM_STATISTIC8_L_V0001;
		union _isp_afm_statistic17_tag* reg7_ptr=(union _isp_afm_statistic7_tag*)ISP_AFM_STATISTIC8_H_V0001;*/

		struct isp_reg_bits reg_config[18];
		struct isp_reg_param read_param;

		ISP_CHECK_FD;

		reg_config[0].reg_addr = ISP_AFM_STATISTIC0_L_V0001 - ISP_BASE_ADDR;
		reg_config[1].reg_addr = ISP_AFM_STATISTIC0_H_V0001 - ISP_BASE_ADDR;
		reg_config[2].reg_addr = ISP_AFM_STATISTIC1_L_V0001 - ISP_BASE_ADDR;
		reg_config[3].reg_addr = ISP_AFM_STATISTIC1_H_V0001 - ISP_BASE_ADDR;
		reg_config[4].reg_addr = ISP_AFM_STATISTIC2_L_V0001 - ISP_BASE_ADDR;
		reg_config[5].reg_addr = ISP_AFM_STATISTIC2_H_V0001 - ISP_BASE_ADDR;
		reg_config[6].reg_addr = ISP_AFM_STATISTIC3_L_V0001 - ISP_BASE_ADDR;
		reg_config[7].reg_addr = ISP_AFM_STATISTIC3_H_V0001 - ISP_BASE_ADDR;
		reg_config[8].reg_addr = ISP_AFM_STATISTIC4_L_V0001 - ISP_BASE_ADDR;
		reg_config[9].reg_addr = ISP_AFM_STATISTIC4_H_V0001 - ISP_BASE_ADDR;
		reg_config[10].reg_addr = ISP_AFM_STATISTIC5_L_V0001 - ISP_BASE_ADDR;
		reg_config[11].reg_addr = ISP_AFM_STATISTIC5_H_V0001 - ISP_BASE_ADDR;
		reg_config[12].reg_addr = ISP_AFM_STATISTIC6_L_V0001 - ISP_BASE_ADDR;
		reg_config[13].reg_addr = ISP_AFM_STATISTIC6_H_V0001 - ISP_BASE_ADDR;
		reg_config[14].reg_addr = ISP_AFM_STATISTIC7_L_V0001 - ISP_BASE_ADDR;
		reg_config[15].reg_addr = ISP_AFM_STATISTIC7_H_V0001 - ISP_BASE_ADDR;
		reg_config[16].reg_addr = ISP_AFM_STATISTIC8_L_V0001 - ISP_BASE_ADDR;
		reg_config[17].reg_addr = ISP_AFM_STATISTIC8_H_V0001 - ISP_BASE_ADDR;

		read_param.reg_param = (unsigned long)&reg_config[0];
		read_param.counts = 18;

		if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
			statistic_ptr[0] = (uint32_t)(((reg_config[1].reg_value<<0x1f)&0x80000000)|((reg_config[0].reg_value>>ISP_ONE)&0x7fffffff));
			statistic_ptr[1] = (uint32_t)(((reg_config[3].reg_value<<0x1f)&0x80000000)|((reg_config[2].reg_value>>ISP_ONE)&0x7fffffff));
			statistic_ptr[2] = (uint32_t)(((reg_config[5].reg_value<<0x1f)&0x80000000)|((reg_config[4].reg_value>>ISP_ONE)&0x7fffffff));
			statistic_ptr[3] = (uint32_t)(((reg_config[7].reg_value<<0x1f)&0x80000000)|((reg_config[6].reg_value>>ISP_ONE)&0x7fffffff));
			statistic_ptr[4] = (uint32_t)(((reg_config[9].reg_value<<0x1f)&0x80000000)|((reg_config[8].reg_value>>ISP_ONE)&0x7fffffff));
			statistic_ptr[5] = (uint32_t)(((reg_config[11].reg_value<<0x1f)&0x80000000)|((reg_config[10].reg_value>>ISP_ONE)&0x7fffffff));
			statistic_ptr[6] = (uint32_t)(((reg_config[13].reg_value<<0x1f)&0x80000000)|((reg_config[12].reg_value>>ISP_ONE)&0x7fffffff));
			statistic_ptr[7] = (uint32_t)(((reg_config[15].reg_value<<0x1f)&0x80000000)|((reg_config[14].reg_value>>ISP_ONE)&0x7fffffff));
			statistic_ptr[8] = (uint32_t)(((reg_config[17].reg_value<<0x1f)&0x80000000)|((reg_config[16].reg_value>>ISP_ONE)&0x7fffffff));
		}

	}

	return ISP_SUCCESS;
}

/*EE */
/*	--
*@
*@
*/
int32_t ispGetEdgeStatus(uint32_t handler_id, uint32_t* status)
{
/*	union _isp_ee_status_tag* reg_ptr=(union _isp_ee_status_tag*)ISP_EE_STATUS;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_ee_status_tag* reg_s_ptr=(union _isp_ee_status_tag*)&isp_reg_ptr->EE_STATUS;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;

	ISP_CHECK_FD;

	reg_config.reg_addr = ISP_EE_STATUS - ISP_BASE_ADDR;
	read_param.reg_param = (unsigned long)&reg_config;
	read_param.counts = 1;

/*	reg_s_ptr->dwValue=reg_ptr->dwValue;*/

	if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		reg_s_ptr->dwValue = reg_config.reg_value;
		*status=reg_s_ptr->dwValue;
	}
	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispEdgeBypass(uint32_t handler_id, uint8_t bypass)
{
/*	union _isp_ee_param_tag* reg_ptr=(union _isp_ee_param_tag*)ISP_EE_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_ee_param_tag* reg_s_ptr=(union _isp_ee_param_tag*)&isp_reg_ptr->EE_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.bypass=bypass;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_EE_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->EE_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetEdgeParam(uint32_t handler_id, uint16_t detail_th, uint16_t smooth_th, uint16_t strength)
{
/*	union _isp_ee_param_tag* reg_ptr=(union _isp_ee_param_tag*)ISP_EE_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_ee_param_tag* reg_s_ptr=(union _isp_ee_param_tag*)&isp_reg_ptr->EE_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.detail_th=detail_th;
	reg_s_ptr->mBits.smooth_th=smooth_th;
	reg_s_ptr->mBits.strength=strength;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_EE_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->EE_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*Emboss */
/*	--
*@
*@
*/
int32_t ispGetEmbossStatus(uint32_t handler_id, uint32_t* status)
{
/*	union _isp_emboss_status_tag* reg_ptr=(union _isp_emboss_status_tag*)ISP_EMBOSS_STATUS;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_emboss_status_tag* reg_s_ptr=(union _isp_emboss_status_tag*)&isp_reg_ptr->EMBOSS_STATUS;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;

	ISP_CHECK_FD;

	reg_config.reg_addr = ISP_EMBOSS_STATUS - ISP_BASE_ADDR;
	read_param.reg_param = (unsigned long)&reg_config;
	read_param.counts = 1;

/*	reg_s_ptr->dwValue=reg_ptr->dwValue;*/
	if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		reg_s_ptr->dwValue = reg_config.reg_value;
		*status=reg_s_ptr->dwValue;
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispEmbossypass(uint32_t handler_id, uint8_t bypass)
{
/*	union _isp_emboss_param_tag* reg_ptr=(union _isp_emboss_param_tag*)ISP_EMBOSS_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_emboss_param_tag* reg_s_ptr=(union _isp_emboss_param_tag*)&isp_reg_ptr->EMBOSS_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.bypass=bypass;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_EMBOSS_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->EMBOSS_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetEmbossParam(uint32_t handler_id, uint16_t step)
{
/*	union _isp_emboss_param_tag* reg_ptr=(union _isp_emboss_param_tag*)ISP_EMBOSS_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_emboss_param_tag* reg_s_ptr=(union _isp_emboss_param_tag*)&isp_reg_ptr->EMBOSS_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.step=step;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_EMBOSS_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->EMBOSS_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*FS */
/*	--
*@
*@
*/
int32_t ispGetFalseColorStatus(uint32_t handler_id, uint32_t* status)
{
/*	union _isp_fcs_status_tag* reg_ptr=(union _isp_fcs_status_tag*)ISP_FCS_STATUS;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_fcs_status_tag* reg_s_ptr=(union _isp_fcs_status_tag*)&isp_reg_ptr->FCS_STATUS;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;

	ISP_CHECK_FD;

	reg_config.reg_addr = ISP_FCS_STATUS - ISP_BASE_ADDR;
	read_param.reg_param = (unsigned long)&reg_config;
	read_param.counts = 1;

/*	reg_s_ptr->dwValue=reg_ptr->dwValue;*/
	if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		reg_s_ptr->dwValue = reg_config.reg_value;
		*status=reg_s_ptr->dwValue;
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispFalseColorBypass(uint32_t handler_id, uint8_t bypass)
{
/*	union _isp_fcs_param_tag* reg_ptr=(union _isp_fcs_param_tag*)ISP_FCS_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_fcs_param_tag* reg_s_ptr=(union _isp_fcs_param_tag*)&isp_reg_ptr->FCS_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.bypass=bypass;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_FCS_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->FCS_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispFalseColorMode(uint32_t handler_id, uint8_t mode)
{
/*	union _isp_fcs_param_tag* reg_ptr=(union _isp_fcs_param_tag*)ISP_FCS_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_fcs_param_tag* reg_s_ptr=(union _isp_fcs_param_tag*)&isp_reg_ptr->FCS_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id){

		ISP_CHECK_FD;

		reg_s_ptr->mBits.mode=mode;
	/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
		reg_config.reg_addr = ISP_FCS_PARAM - ISP_BASE_ADDR;
		reg_config.reg_value = reg_s_ptr->dwValue;
		write_param.reg_param = (unsigned long)&reg_config;
		write_param.counts = 1;

		return _isp_write((uint32_t *)&write_param);
	}
	return ISP_SUCCESS;
}

/*CSS color saturation suppression */
/*	--
*@
*@
*/
int32_t ispGetColorSaturationSuppressStatus(uint32_t handler_id, uint32_t* status)
{
/*	union _isp_css_status_tag* reg_ptr=(union _isp_css_status_tag*)ISP_CSS_STATUS;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_css_status_tag* reg_s_ptr=(union _isp_css_status_tag*)&isp_reg_ptr->CSS_STATUS;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;

	ISP_CHECK_FD;

	reg_config.reg_addr = ISP_CSS_STATUS - ISP_BASE_ADDR;
	read_param.reg_param = (unsigned long)&reg_config;
	read_param.counts = 1;

/*	reg_s_ptr->dwValue=reg_ptr->dwValue;*/
	if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		reg_s_ptr->dwValue = reg_config.reg_value;
		*status=reg_s_ptr->dwValue;
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispColorSaturationSuppressBypass(uint32_t handler_id, uint8_t bypass)
{
/*	union _isp_css_param_tag* reg_ptr=(union _isp_css_param_tag*)ISP_CSS_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_css_param_tag* reg_s_ptr=(union _isp_css_param_tag*)&isp_reg_ptr->CSS_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.bypass=bypass;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_CSS_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->CSS_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetColorSaturationSuppressThrd(uint32_t handler_id, uint8_t* low_thr, uint8_t* low_sum_thr, uint8_t lum_thr, uint chr_thr)
{
/*	union _isp_css_thrd0_tag* reg0_ptr=(union _isp_css_thrd0_tag*)ISP_CSS_THRD0;
	union _isp_css_thrd1_tag* reg1_ptr=(union _isp_css_thrd1_tag*)ISP_CSS_THRD1;
	union _isp_css_thrd2_tag* reg2_ptr=(union _isp_css_thrd2_tag*)ISP_CSS_THRD2;
	union _isp_css_thrd3_tag* reg3_ptr=(union _isp_css_thrd3_tag*)ISP_CSS_THRD3;
*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_css_thrd0_tag* reg0_s_ptr=(union _isp_css_thrd0_tag*)&isp_reg_ptr->CSS_THRD0;
	union _isp_css_thrd1_tag* reg1_s_ptr=(union _isp_css_thrd1_tag*)&isp_reg_ptr->CSS_THRD1;
	union _isp_css_thrd2_tag* reg2_s_ptr=(union _isp_css_thrd2_tag*)&isp_reg_ptr->CSS_THRD2;
	union _isp_css_thrd3_tag* reg3_s_ptr=(union _isp_css_thrd3_tag*)&isp_reg_ptr->CSS_THRD3;
	struct isp_reg_bits reg_config[4];
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg0_s_ptr->mBits.th_0=low_thr[0];
	reg0_s_ptr->mBits.th_1=low_thr[1];
	reg0_s_ptr->mBits.th_2=low_thr[2];
	reg0_s_ptr->mBits.th_3=low_thr[3];
	reg1_s_ptr->mBits.th_4=low_thr[4];
	reg1_s_ptr->mBits.th_5=low_thr[5];
	reg1_s_ptr->mBits.th_6=low_thr[6];
	reg1_s_ptr->mBits.luma_th=lum_thr;
	reg2_s_ptr->mBits.th_0=low_sum_thr[0];
	reg2_s_ptr->mBits.th_1=low_sum_thr[1];
	reg2_s_ptr->mBits.th_2=low_sum_thr[2];
	reg2_s_ptr->mBits.th_3=low_sum_thr[3];
	reg3_s_ptr->mBits.th_4=low_sum_thr[4];
	reg3_s_ptr->mBits.th_5=low_sum_thr[5];
	reg3_s_ptr->mBits.th_6=low_sum_thr[6];
	reg3_s_ptr->mBits.chrom_th=chr_thr;
/*
	reg0_ptr->dwValue=reg0_s_ptr->dwValue;
	reg1_ptr->dwValue=reg1_s_ptr->dwValue;
	reg2_ptr->dwValue=reg2_s_ptr->dwValue;
	reg3_ptr->dwValue=reg3_s_ptr->dwValue;
*/
	reg_config[0].reg_addr = ISP_CSS_THRD0 - ISP_BASE_ADDR;
	reg_config[0].reg_value = isp_reg_ptr->CSS_THRD0;
	reg_config[1].reg_addr = ISP_CSS_THRD1 - ISP_BASE_ADDR;
	reg_config[1].reg_value = isp_reg_ptr->CSS_THRD1;
	reg_config[2].reg_addr = ISP_CSS_THRD2 - ISP_BASE_ADDR;
	reg_config[2].reg_value = isp_reg_ptr->CSS_THRD2;
	reg_config[3].reg_addr = ISP_CSS_THRD3 - ISP_BASE_ADDR;
	reg_config[3].reg_value = isp_reg_ptr->CSS_THRD3;

	write_param.reg_param = (unsigned long)&reg_config[0];
	write_param.counts = 4;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispColorSaturationSuppressSliceSize(uint32_t handler_id, uint16_t w, uint16_t h)
{
/*	union _isp_css_slice_size_tag* reg_ptr=(union _isp_css_slice_size_tag*)ISP_CSS_SLICE_SIZE;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_css_slice_size_tag* reg_s_ptr=(union _isp_css_slice_size_tag*)&isp_reg_ptr->CSS_SLICE_SIZE;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.slice_width=w;
	reg_s_ptr->mBits.slice_height=h;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_CSS_SLICE_SIZE - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->CSS_SLICE_SIZE;

	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetColorSaturationSuppressRatio(uint32_t handler_id, uint8_t* ratio)
{
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_css_ratio_tag* reg0_s_ptr=(union _isp_css_ratio_tag*)&isp_reg_ptr->CSS_RATIO;
	struct isp_reg_bits reg_config[1];
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg0_s_ptr->mBits.ratio_0=ratio[0];
	reg0_s_ptr->mBits.ratio_1=ratio[1];
	reg0_s_ptr->mBits.ratio_2=ratio[2];
	reg0_s_ptr->mBits.ratio_3=ratio[3];
	reg0_s_ptr->mBits.ratio_4=ratio[4];
	reg0_s_ptr->mBits.ratio_5=ratio[5];
	reg0_s_ptr->mBits.ratio_6=ratio[6];
	reg0_s_ptr->mBits.ratio_7=ratio[7];

	reg_config[0].reg_addr = ISP_CSS_RATIO - ISP_BASE_ADDR;
	reg_config[0].reg_value = isp_reg_ptr->CSS_RATIO;

	write_param.reg_param = (unsigned long)&reg_config[0];
	write_param.counts = 1;

	_isp_write((uint32_t *)&write_param);

	return ISP_SUCCESS;
}

/*SATURATION */
/*	--
*@
*@
*/
int32_t ispGetSaturationStatus(uint32_t handler_id, uint32_t* status)
{
/*	union _isp_csa_status_tag* reg_ptr=(union _isp_csa_status_tag*)ISP_CSA_STATUS;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_csa_status_tag* reg_s_ptr=(union _isp_csa_status_tag*)&isp_reg_ptr->CSA_STATUS;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;

	ISP_CHECK_FD;

	reg_config.reg_addr = ISP_CSA_STATUS - ISP_BASE_ADDR;
	read_param.reg_param = (unsigned long)&reg_config;
	read_param.counts = 1;

/*	reg_s_ptr->dwValue=reg_ptr->dwValue;*/
	if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		reg_s_ptr->dwValue = reg_config.reg_value;
		*status=reg_s_ptr->dwValue;
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispSaturationbypass(uint32_t handler_id, uint8_t bypass)
{
/*	union _isp_csa_param_tag* reg_ptr=(union _isp_csa_param_tag*)ISP_CSA_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_csa_param_tag* reg_s_ptr=(union _isp_csa_param_tag*)&isp_reg_ptr->CSA_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.bypass=bypass;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_CSA_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->CSA_PARAM;

	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetSaturationFactor(uint32_t handler_id, uint8_t factor)
{
/*	union _isp_csa_param_tag* reg_ptr=(union _isp_csa_param_tag*)ISP_CSA_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_csa_param_tag* reg_s_ptr=(union _isp_csa_param_tag*)&isp_reg_ptr->CSA_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.factor=factor;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_CSA_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->CSA_PARAM;

	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/* STORE */
/*	--
*@
*@
*/
int32_t ispGetStoreStatus(uint32_t handler_id, uint32_t* status)
{
/*	union _isp_store_status_tag* reg_ptr=(union _isp_store_status_tag*)ISP_STORE_STATUS;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_store_status_tag* reg_s_ptr=(union _isp_store_status_tag*)&isp_reg_ptr->STORE_STATUS;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;

	ISP_CHECK_FD;

	reg_config.reg_addr = ISP_STORE_STATUS - ISP_BASE_ADDR;
	read_param.reg_param = (unsigned long)&reg_config;
	read_param.counts = 1;

/*	reg_s_ptr->dwValue=reg_ptr->dwValue;*/
	if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		reg_s_ptr->dwValue = reg_config.reg_value;
		*status=reg_s_ptr->dwValue;
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispGetStoreStatusPreview(uint32_t handler_id, uint32_t* status)
{
/*	union _isp_store_status_tag* reg_ptr=(union _isp_store_status_tag*)ISP_STORE_STATUS_PREVIEW_V0001;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_store_status_preview_tag* reg_s_ptr=(union _isp_store_status_preview_tag*)&isp_reg_ptr->STORE_STATUS_PREVIEW;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;
	uint32_t isp_id=_isp_GetIspId();

	ISP_CHECK_FD;
	if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
		reg_config.reg_addr = ISP_STORE_STATUS_PREVIEW_V0001 - ISP_BASE_ADDR;
		read_param.reg_param = (unsigned long)&reg_config;
		read_param.counts = 1;

	/*	reg_s_ptr->dwValue=reg_ptr->dwValue;*/
		if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
			reg_s_ptr->dwValue = reg_config.reg_value;
			*status=reg_s_ptr->dwValue;
		}
	}
#if 0
	else if (SC9630_ISP_ID==isp_id)
	{
		reg_config.reg_addr = ISP_STORE_STATUS_PREVIEW_V0002 - ISP_BASE_ADDR;
		read_param.reg_param = (unsigned long)&reg_config;
		read_param.counts = 1;

		/*	reg_s_ptr->dwValue=reg_ptr->dwValue;*/
		if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
			reg_s_ptr->dwValue = reg_config.reg_value;
			*status=reg_s_ptr->dwValue;
		}
	}
#endif
	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispStoreBypass(uint32_t handler_id, uint8_t bypass)
{
/*	union _isp_store_param_tag* reg_ptr=(union _isp_store_param_tag*)ISP_STORE_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_store_param_tag* reg_s_ptr=(union _isp_store_param_tag*)&isp_reg_ptr->STORE_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.bypass=bypass;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_STORE_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->STORE_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispStoreSubtract(uint32_t handler_id, uint8_t subtract)
{
/*	union _isp_store_param_tag* reg_ptr=(union _isp_store_param_tag*)ISP_STORE_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_store_param_tag* reg_s_ptr=(union _isp_store_param_tag*)&isp_reg_ptr->STORE_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.sub_stract=subtract;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_STORE_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->STORE_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetStoreSliceSize(uint32_t handler_id, uint16_t w, uint16_t h)
{
/*	union _isp_store_slice_size_tag* reg_ptr=(union _isp_store_slice_size_tag*)ISP_STORE_SLICE_SIZE;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_store_slice_size_tag* reg_s_ptr=(union _isp_store_slice_size_tag*)&isp_reg_ptr->STORE_SLICE_SIZE;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.slice_with=w;
	reg_s_ptr->mBits.slice_height=h;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_STORE_SLICE_SIZE - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->STORE_SLICE_SIZE;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetStoreYAddr(uint32_t handler_id, uint32_t y_addr)
{
/*	union _isp_store_slice_y_addr_tag* reg_ptr=(union _isp_store_slice_y_addr_tag*)ISP_STORE_SLICE_Y_ADDR;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_store_slice_y_addr_tag* reg_s_ptr=(union _isp_store_slice_y_addr_tag*)&isp_reg_ptr->STORE_SLICE_Y_ADDR;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.y_addr=y_addr;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr= ISP_STORE_SLICE_Y_ADDR - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->STORE_SLICE_Y_ADDR;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetStoreYPitch(uint32_t handler_id, uint32_t y_pitch)
{
/*	union _isp_store_slice_y_pitch_tag* reg_ptr=(union _isp_store_slice_y_pitch_tag*)ISP_STORE_Y_PITCH;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_store_slice_y_pitch_tag* reg_s_ptr=(union _isp_store_slice_y_pitch_tag*)&isp_reg_ptr->STORE_SLICE_Y_PITCH;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.y_pitch=y_pitch;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_STORE_Y_PITCH - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->STORE_SLICE_Y_PITCH;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetStoreUAddr(uint32_t handler_id, uint32_t u_addr)
{
/*	union _isp_store_slice_u_addr_tag* reg_ptr=(union _isp_store_slice_u_addr_tag*)ISP_STORE_SLICE_U_ADDR;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_store_slice_u_addr_tag* reg_s_ptr=(union _isp_store_slice_u_addr_tag*)&isp_reg_ptr->STORE_SLICE_U_ADDR;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.u_addr=u_addr;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_STORE_SLICE_U_ADDR - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->STORE_SLICE_U_ADDR;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetStoreUPitch(uint32_t handler_id, uint32_t u_pitch)
{
/*	union _isp_store_slice_u_pitch_tag* reg_ptr=(union _isp_store_slice_u_pitch_tag*)ISP_STORE_U_PITCH;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_store_slice_u_pitch_tag* reg_s_ptr=(union _isp_store_slice_u_pitch_tag*)&isp_reg_ptr->STORE_SLICE_U_PITCH;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.u_pitch=u_pitch;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_STORE_U_PITCH - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->STORE_SLICE_U_PITCH;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetStoreVAddr(uint32_t handler_id, uint32_t v_addr)
{
/*	union _isp_store_slice_v_addr_tag* reg_ptr=(union _isp_store_slice_v_addr_tag*)ISP_STORE_SLICE_V_ADDR;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_store_slice_v_addr_tag* reg_s_ptr=(union _isp_store_slice_v_addr_tag*)&isp_reg_ptr->STORE_SLICE_V_ADDR;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.v_addr=v_addr;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_STORE_SLICE_V_ADDR - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->STORE_SLICE_V_ADDR;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetStoreVPitch(uint32_t handler_id, uint32_t v_pitch)
{
/*	union _isp_store_slice_v_pitch_tag* reg_ptr=(union _isp_store_slice_v_pitch_tag*)ISP_STORE_V_PITCH;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_store_slice_v_pitch_tag* reg_s_ptr=(union _isp_store_slice_v_pitch_tag*)&isp_reg_ptr->STORE_SLICE_V_PITCH;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.v_pitch=v_pitch;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_STORE_V_PITCH - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->STORE_SLICE_V_PITCH;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetStoreInt(uint32_t handler_id, uint16_t int_cnt_num, uint8_t int_eb)
{
/*	union _isp_store_int_ctrl_tag* reg_ptr=(union _isp_store_int_ctrl_tag*)ISP_STORE_INT_CTRL;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_store_int_ctrl_tag* reg_s_ptr=(union _isp_store_int_ctrl_tag*)&isp_reg_ptr->STORE_INT_CTRL;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.int_cnt_num=int_cnt_num;
	reg_s_ptr->mBits.int_eb=int_eb;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_STORE_INT_CTRL- ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->STORE_INT_CTRL;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

int32_t ispSetStoreBoarder(uint32_t handler_id, uint8_t right, uint8_t left, uint8_t down, uint8_t up)
{
	return ISP_SUCCESS;
}


/* FEEDER */
/*	--
*@
*@
*/
int32_t ispGetFeederStatus(uint32_t handler_id, uint32_t* status)
{
/*	union _isp_feeder_status_tag* reg_ptr=(union _isp_feeder_status_tag*)ISP_FEEDER_STATUS;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_feeder_status_tag* reg_s_ptr=(union _isp_feeder_status_tag*)&isp_reg_ptr->FEEDER_STATUS;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;

	ISP_CHECK_FD;

	reg_config.reg_addr = ISP_FEEDER_STATUS - ISP_BASE_ADDR;
	read_param.reg_param = (unsigned long)&reg_config;
	read_param.counts = 1;

/*	reg_s_ptr->dwValue=reg_ptr->dwValue;*/
	if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		reg_s_ptr->dwValue = reg_config.reg_value;
		*status=reg_s_ptr->dwValue;
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispSetFeederDataType(uint32_t handler_id, uint16_t data_type)
{
/*	union _isp_feeder_param_tag* reg_ptr=(union _isp_feeder_param_tag*)ISP_FEEDER_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_feeder_param_tag* reg_s_ptr=(union _isp_feeder_param_tag*)&isp_reg_ptr->FEEDER_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.data_type=data_type;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_FEEDER_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->FEEDER_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetFeederSliceSize(uint32_t handler_id, uint16_t w, uint16_t h)
{
/*	union _isp_feeder_slice_size_tag* reg_ptr=(union _isp_feeder_slice_size_tag*)ISP_FEEDER_SLICE_SIZE;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_feeder_slice_size_tag* reg_s_ptr=(union _isp_feeder_slice_size_tag*)&isp_reg_ptr->FEEDER_SLICE_SIZE;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.slice_with=w;
	reg_s_ptr->mBits.slice_height=h;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_FEEDER_SLICE_SIZE - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->FEEDER_SLICE_SIZE;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/* ARBITER */
/*	--
*@
*@
*/
int32_t ispGetArbiterStatus(uint32_t handler_id, uint32_t* status)
{
/*	union _isp_arbiter_status_tag* reg_ptr=(union _isp_arbiter_status_tag*)ISP_ARBITER_STATUS;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_arbiter_status_tag* reg_s_ptr=(union _isp_arbiter_status_tag*)&isp_reg_ptr->ARBITER_STATUS;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;

	ISP_CHECK_FD;

	reg_config.reg_addr = ISP_ARBITER_STATUS - ISP_BASE_ADDR;
	read_param.reg_param = (unsigned long)&reg_config;
	read_param.counts = 1;

/*	reg_s_ptr->dwValue=reg_ptr->dwValue;*/
	if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		reg_s_ptr->dwValue = reg_config.reg_value;
		*status=reg_s_ptr->dwValue;
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispGetAxiWrMasterStatus(uint32_t handler_id, uint32_t* status)
{
/*	union _isp_store_status_tag* reg_ptr=(union _isp_store_status_tag*)ISP_AXI_WR_MASTER_STATUS_V0001;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_store_status_tag* reg_s_ptr=(union _isp_store_status_tag*)&isp_reg_ptr->ARBITER_AXI_WR_MASTER_STATUS_V0001;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_config.reg_addr = ISP_AXI_WR_MASTER_STATUS_V0001 - ISP_BASE_ADDR;
		read_param.reg_param = (unsigned long)&reg_config;
		read_param.counts = 1;

	/*	reg_s_ptr->dwValue=reg_ptr->dwValue;*/
		if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
			reg_s_ptr->dwValue = reg_config.reg_value;
			*status=reg_s_ptr->dwValue;
		}
	}
	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispGetAxiRdMasterStatus(uint32_t handler_id, uint32_t* status)
{
/*	union _isp_store_status_tag* reg_ptr=(union _isp_store_status_tag*)ISP_AXI_RD_MASTER_STATUS_V0001;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_store_status_tag* reg_s_ptr=(union _isp_store_status_tag*)&isp_reg_ptr->ARBITER_AXI_RD_MASTER_STATUS_V0001;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_config.reg_addr = ISP_AXI_RD_MASTER_STATUS_V0001 - ISP_BASE_ADDR;
		read_param.reg_param = (unsigned long)&reg_config;
		read_param.counts = 1;

	/*	reg_s_ptr->dwValue=reg_ptr->dwValue;*/
		if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
			reg_s_ptr->dwValue = reg_config.reg_value;
			*status=reg_s_ptr->dwValue;
		}
	}
	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispArbiterReset(uint32_t handler_id, uint8_t rst)
{
/*	union _isp_arbiter_prerst_tag* reg_ptr=(union _isp_arbiter_prerst_tag*)ISP_ARBITER_PRERST;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_arbiter_prerst_tag* reg_s_ptr=(union _isp_arbiter_prerst_tag*)&isp_reg_ptr->ARBITER_PRERST;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.reset=rst;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_ARBITER_PRERST - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->ARBITER_PRERST;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetArbiterPauseCycle(uint32_t handler_id, uint16_t cycle)
{
/*	union _isp_arbiter_pause_cycle_tag* reg_ptr=(union _isp_arbiter_pause_cycle_tag*)ISP_ARBITER_PAUSE_CYCLE;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_arbiter_pause_cycle_tag* reg_s_ptr=(union _isp_arbiter_pause_cycle_tag*)&isp_reg_ptr->ARBITER_PAUSE_CYCLE;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.pause_cycle=cycle;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_ARBITER_PAUSE_CYCLE - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->ARBITER_PAUSE_CYCLE;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispAxiMasterCtrl(uint32_t handler_id, uint8_t wait_resp)
{
/*	union _isp_arbiter_prerst_tag* reg_ptr=(union _isp_arbiter_prerst_tag*)ISP_AXI_MASTER_CTRL_V0001;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_arbiter_prerst_tag* reg_s_ptr=(union _isp_arbiter_prerst_tag*)&isp_reg_ptr->ARBITER_AXI_MASTER_CTRL_V0001;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8830_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_s_ptr->mBits.reset=wait_resp;
		/*reg_ptr->dwValue=reg_s_ptr->dwValue;*/
		reg_config.reg_addr = ISP_AXI_MASTER_CTRL_V0001 - ISP_BASE_ADDR;
		reg_config.reg_value = reg_s_ptr->dwValue;
		write_param.reg_param = (unsigned long)&reg_config;
		write_param.counts = 1;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

/* HDR */
/*	--
*@
*@
*/
int32_t ispGetHDRStatus(uint32_t handler_id, uint32_t* status)
{
/*	union _isp_hdr_status_tag* reg_ptr=(union _isp_hdr_status_tag*)ISP_HDR_STATUS;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_hdr_status_tag* reg_s_ptr=(union _isp_hdr_status_tag*)&isp_reg_ptr->HDR_STATUS;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;

	ISP_CHECK_FD;

	reg_config.reg_addr = ISP_HDR_STATUS - ISP_BASE_ADDR;
	read_param.reg_param = (unsigned long)&reg_config;
	read_param.counts = 1;

/*	reg_s_ptr->dwValue=reg_ptr->dwValue;*/
	if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		reg_s_ptr->dwValue = reg_config.reg_value;
		*status=reg_s_ptr->dwValue;
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispHDRBypass(uint32_t handler_id, uint8_t bypass)
{
/*	union _isp_hdr_param_tag* reg_ptr=(union _isp_hdr_param_tag*)ISP_HDR_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_hdr_param_tag* reg_s_ptr=(union _isp_hdr_param_tag*)&isp_reg_ptr->HDR_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.bypass=bypass;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_HDR_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->HDR_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetHDRLevel(uint32_t handler_id, uint16_t level)
{
/*	union _isp_hdr_param_tag* reg_ptr=(union _isp_hdr_param_tag*)ISP_HDR_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_hdr_param_tag* reg_s_ptr=(union _isp_hdr_param_tag*)&isp_reg_ptr->HDR_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.level=level;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_HDR_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->HDR_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetHDRIndex(uint32_t handler_id, uint8_t r_index, uint8_t g_index, uint8_t b_index)
{
/*	union _isp_hdr_index_tag* reg_ptr=(union _isp_hdr_index_tag*)ISP_HDR_INDEX;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_hdr_index_tag* reg_s_ptr=(union _isp_hdr_index_tag*)&isp_reg_ptr->HDR_INDEX;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.index_r=r_index;
	reg_s_ptr->mBits.index_g=g_index;
	reg_s_ptr->mBits.index_b=b_index;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_HDR_INDEX - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->HDR_INDEX;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetHDRIndexTab(uint32_t handler_id, uint8_t* com_ptr, uint8_t* p2e_ptr, uint8_t* e2p_ptr)
{
	union _isp_mem_reg_tag* reg_com_ptr=(union _isp_mem_reg_tag*)ISP_HDR_COMP_OUTPUT;
	union _isp_mem_reg_tag* reg_p2e_ptr=(union _isp_mem_reg_tag*)ISP_HDR_P2E_OUTPUT;
	union _isp_mem_reg_tag* reg_e2p_ptr=(union _isp_mem_reg_tag*)ISP_HDR_E2P_OUTPUT;
	uint32_t i=0x00;
	uint32_t value=0x00;
	uint32_t k = 0x00;
	uint32_t addr_offset = 0x00;
	struct isp_reg_bits reg_config[ISP_HDR_COMP_ITEM+ISP_HDR_P2E_ITEM+ISP_HDR_E2P_ITEM];
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	/*config the compensation value */
	addr_offset = ISP_HDR_COMP_OUTPUT - ISP_BASE_ADDR;
	for(i=0x00; i<ISP_HDR_COMP_ITEM; i++)
	{
		value=(com_ptr[0]|(com_ptr[1]<<8)|(com_ptr[2]<<16)|(com_ptr[3]<<24));
		/*reg_com_ptr->dwValue=value;*/
		/*reg_com_ptr++;*/
		reg_config[k].reg_addr = addr_offset;
		addr_offset += 4;
		reg_config[k++].reg_value = value;
		com_ptr+=0x04;
	}

	/*config the p2e map */
	addr_offset = ISP_HDR_P2E_OUTPUT - ISP_BASE_ADDR;
	for(i=0x00; i<ISP_HDR_P2E_ITEM; i++)
	{
		value=(p2e_ptr[0]|(p2e_ptr[1]<<8)|(p2e_ptr[2]<<16)|(p2e_ptr[3]<<24));
		/*reg_p2e_ptr->dwValue=value;*/
		/*reg_p2e_ptr++;*/
		reg_config[k].reg_addr = addr_offset;
		addr_offset += 4;
		reg_config[k++].reg_value = value;
		p2e_ptr+=0x04;
	}

	/*config the e2p map */
	addr_offset = ISP_HDR_E2P_OUTPUT - ISP_BASE_ADDR;
	for(i=0x00; i<ISP_HDR_E2P_ITEM; i++)
	{
		value=(e2p_ptr[0]|(e2p_ptr[1]<<8)|(e2p_ptr[2]<<16)|(e2p_ptr[3]<<24));
		/*reg_e2p_ptr->dwValue=value;*/
		/*reg_e2p_ptr++;*/
		reg_config[k].reg_addr = addr_offset;
		addr_offset += 4;
		reg_config[k++].reg_value = value;
		e2p_ptr+=0x04;
	}

	write_param.reg_param = (unsigned long)&reg_config[0];
	write_param.counts = ISP_HDR_COMP_ITEM+ISP_HDR_P2E_ITEM+ISP_HDR_E2P_ITEM;

	return _isp_write((uint32_t *)&write_param);
}

/* COMMON */
/*	--
*@
*@
*/
int32_t ispGetIspStatus(uint32_t handler_id, uint32_t* com_status)
{
/*	union _isp_com_status_tag* reg_ptr=(union _isp_com_status_tag*)ISP_COMMON_STATUS;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_com_status_tag* reg_s_ptr=(union _isp_com_status_tag*)&isp_reg_ptr->COM_STATUS;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;

	ISP_CHECK_FD;

	reg_config.reg_addr = ISP_COMMON_STATUS - ISP_BASE_ADDR;
	read_param.reg_param = (unsigned long)&reg_config;
	read_param.counts = 1;

/*	*com_status=reg_ptr->dwValue;*/
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		reg_s_ptr->dwValue = reg_config.reg_value;
		*com_status=reg_s_ptr->dwValue;
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t isp_Start(uint32_t handler_id, uint8_t start)
{
/*	union _isp_com_start_tag* reg_ptr=(union _isp_com_start_tag*)ISP_COMMON_START;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_com_start_tag* reg_s_ptr=(union _isp_com_start_tag*)&isp_reg_ptr->COM_START;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.start_bit=start;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_COMMON_START - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->COM_START;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispInMode(uint32_t handler_id, uint32_t mode)
{
/*	union _isp_com_param_tag* reg_ptr=(union _isp_com_param_tag*)ISP_COMMON_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_com_param_tag* reg_s_ptr=(union _isp_com_param_tag*)&isp_reg_ptr->COM_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.in_mode=mode;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_COMMON_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->COM_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispOutMode(uint32_t handler_id, uint32_t mode)
{
/*	union _isp_com_param_tag* reg_ptr=(union _isp_com_param_tag*)ISP_COMMON_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_com_param_tag* reg_s_ptr=(union _isp_com_param_tag*)&isp_reg_ptr->COM_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.out_mode=mode;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_COMMON_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->COM_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispFetchEdian(uint32_t handler_id, uint32_t endian, uint32_t bit_reorder)
{
/*	union _isp_com_param_tag* reg_ptr=(union _isp_com_param_tag*)ISP_COMMON_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_com_param_tag* reg_s_ptr=(union _isp_com_param_tag*)&isp_reg_ptr->COM_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.fetch_endian=endian;
	reg_s_ptr->mBits.fetch_bit_reorder=bit_reorder;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_COMMON_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->COM_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispBPCEdian(uint32_t handler_id, uint32_t endian)
{
/*	union _isp_com_param_tag* reg_ptr=(union _isp_com_param_tag*)ISP_COMMON_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_com_param_tag* reg_s_ptr=(union _isp_com_param_tag*)&isp_reg_ptr->COM_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.bpc_endian=endian;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_COMMON_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->COM_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispStoreEdian(uint32_t handler_id, uint32_t endian)
{
/*	union _isp_com_param_tag* reg_ptr=(union _isp_com_param_tag*)ISP_COMMON_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_com_param_tag* reg_s_ptr=(union _isp_com_param_tag*)&isp_reg_ptr->COM_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.store_endian=endian;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_COMMON_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->COM_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}
/*	--
*@
*@
*/
int32_t ispFetchDataFormat(uint32_t handler_id, uint32_t format)
{
/*	union _isp_com_param_tag* reg_ptr=(union _isp_com_param_tag*)ISP_COMMON_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_com_param_tag* reg_s_ptr=(union _isp_com_param_tag*)&isp_reg_ptr->COM_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.fetch_color_format=format;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_COMMON_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->COM_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispStoreFormat(uint32_t handler_id, uint32_t format)
{
/*	union _isp_com_param_tag* reg_ptr=(union _isp_com_param_tag*)ISP_COMMON_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_com_param_tag* reg_s_ptr=(union _isp_com_param_tag*)&isp_reg_ptr->COM_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.store_yuv_format=format;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_COMMON_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->COM_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetBurstSize(uint32_t handler_id, uint16_t burst_size)
{
/*	union _isp_com_burst_size_tag* reg_ptr=(union _isp_com_burst_size_tag*)ISP_COMMON_BURST_SIZE;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_com_burst_size_tag* reg_s_ptr=(union _isp_com_burst_size_tag*)&isp_reg_ptr->COM_BURST_SIZE;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.burst_size=burst_size;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_COMMON_BURST_SIZE - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->COM_BURST_SIZE;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispMemSwitch(uint32_t handler_id, uint8_t mem_switch)
{
/*	union _isp_com_mem_switch_v0000_tag* reg_ptr=(union _isp_com_mem_switch_v0000_tag*)ISP_COMMON_MEM_SWITCH_V0000;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_com_mem_switch_v0000_tag* reg_s_ptr=(union _isp_com_mem_switch_v0000_tag*)&isp_reg_ptr->COM_MEM_SWITCH_V0000;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8825_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_s_ptr->mBits.mem_switch=mem_switch;
		/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
		reg_config.reg_addr = ISP_COMMON_MEM_SWITCH_V0000- ISP_BASE_ADDR;
		reg_config.reg_value = reg_s_ptr->dwValue;
		write_param.reg_param = (unsigned long)&reg_config;
		write_param.counts = 1;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispShadowAll(uint32_t handler_id, uint8_t shadow)
{
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_com_shadow_tag* reg_s_ptr=(union _isp_com_shadow_tag*)&isp_reg_ptr->COM_SHADOW_ALL;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.shadow_bit=shadow;
	reg_config.reg_addr = ISP_COMMON_SHADOW_ALL - ISP_BASE_ADDR;
	reg_config.reg_value = 0x55555555;//isp_reg_ptr->COM_SHADOW;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispShadow(uint32_t handler_id, uint8_t shadow)
{
/*	union _isp_com_shadow_tag* reg_ptr=(union _isp_com_shadow_tag*)ISP_COMMON_SHADOW;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_com_shadow_tag* reg_s_ptr=(union _isp_com_shadow_tag*)&isp_reg_ptr->COM_SHADOW;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.shadow_bit=shadow;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_COMMON_SHADOW - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->COM_SHADOW;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispByerMode(uint32_t handler_id, uint32_t nlc_bayer, uint32_t awbc_bayer, uint32_t wave_bayer, uint32_t cfa_bayer, uint32_t gain_bayer)
{
/*	union _isp_com_bayer_mode_tag* reg_ptr=(union _isp_com_bayer_mode_tag*)ISP_COMMON_BAYER_MODE;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_com_bayer_mode_tag* reg_s_ptr=(union _isp_com_bayer_mode_tag*)&isp_reg_ptr->COM_BAYER_MODE;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.bnlc_bayer=nlc_bayer;
	reg_s_ptr->mBits.awbc_bayer=awbc_bayer;
	reg_s_ptr->mBits.wave_bayer=wave_bayer;
	reg_s_ptr->mBits.cfa_bayer=cfa_bayer;
	reg_s_ptr->mBits.gain_bayer=gain_bayer;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_COMMON_BAYER_MODE - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->COM_BAYER_MODE;;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispPMURamMask(uint32_t handler_id, uint8_t mask)
{
/*	union _isp_com_pmu_ram_mask_tag* reg_ptr=(union _isp_com_pmu_ram_mask_tag*)ISP_COMMON_PMU_RAM_MASK;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_com_pmu_ram_mask_tag* reg_s_ptr=(union _isp_com_pmu_ram_mask_tag*)&isp_reg_ptr->COM_PMU_RAM_MASK;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.ram_mask=mask;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_COMMON_PMU_RAM_MASK - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->COM_PMU_RAM_MASK;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispHwMask(uint32_t handler_id, uint32_t hw_logic)
{
/*	union _isp_com_hw_mask_tag* reg_ptr=(union _isp_com_hw_mask_tag*)ISP_COMMON_HW_MASK;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_com_hw_mask_tag* reg_s_ptr=(union _isp_com_hw_mask_tag*)&isp_reg_ptr->COM_HW_MASK;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->dwValue|=(1<<hw_logic);
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_COMMON_HW_MASK - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->COM_HW_MASK;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispHwEnable(uint32_t handler_id, uint32_t hw_logic)
{
/*	union _isp_com_hw_eb_tag* reg_ptr=(union _isp_com_hw_eb_tag*)ISP_COMMON_HW_ENABLE;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_com_hw_eb_tag* reg_s_ptr=(union _isp_com_hw_eb_tag*)&isp_reg_ptr->COM_HW_EB;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->dwValue|=(1<<hw_logic);
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_COMMON_HW_ENABLE - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->COM_HW_EB;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispPMUSel(uint32_t handler_id, uint8_t sel)
{
/*	union _isp_com_sw_switch_tag* reg_ptr=(union _isp_com_sw_switch_tag*)ISP_COMMON_SW_SWITCH;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_com_sw_switch_tag* reg_s_ptr=(union _isp_com_sw_switch_tag*)&isp_reg_ptr->COM_SW_SWITCH;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.pmu_sel=sel;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_COMMON_SW_SWITCH - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->COM_SW_SWITCH;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSwEnable(uint32_t handler_id, uint32_t sw_logic)
{
/*	union _isp_com_sw_toggle_tag* reg_ptr=(union _isp_com_sw_toggle_tag*)ISP_COMMON_SW_TOGGLE;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_com_sw_toggle_tag* reg_s_ptr=(union _isp_com_sw_toggle_tag*)&isp_reg_ptr->COM_SW_TOGGLE;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->dwValue|=(1<<sw_logic);
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_COMMON_SW_TOGGLE - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->COM_SW_TOGGLE;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispPreviewStop(uint32_t handler_id, uint8_t eb)
{
/*	union _isp_com_preview_stop_tag* reg_ptr=(union _isp_com_preview_stop_tag*)ISP_COMMON_PREVIEW_STOP;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_com_preview_stop_tag* reg_s_ptr=(union _isp_com_preview_stop_tag*)&isp_reg_ptr->COM_PREVIEW_STOP;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.stop=eb;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_COMMON_PREVIEW_STOP - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->COM_PREVIEW_STOP;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetShadowConterEb(uint32_t handler_id, uint8_t eb)
{
/*	union _isp_com_shadow_cnt_tag* reg_ptr=(union _isp_com_shadow_cnt_tag*)ISP_COMMON_SHADOW_CNT;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_com_shadow_cnt_tag* reg_s_ptr=(union _isp_com_shadow_cnt_tag*)&isp_reg_ptr->COM_SHADOW_CNT;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.cnt_eb=eb;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_COMMON_SHADOW_CNT - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->COM_SHADOW_CNT;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispShadowConterClear(uint32_t handler_id, uint8_t eb)
{
/*	union _isp_com_shadow_cnt_tag* reg_ptr=(union _isp_com_shadow_cnt_tag*)ISP_COMMON_SHADOW_CNT;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_com_shadow_cnt_tag* reg_s_ptr=(union _isp_com_shadow_cnt_tag*)&isp_reg_ptr->COM_SHADOW_CNT;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.cnt_clear=eb;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_COMMON_SHADOW_CNT - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->COM_SHADOW_CNT;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetShadowConter(uint32_t handler_id, uint32_t conter)
{
/*	union _isp_com_shadow_cnt_tag* reg_ptr=(union _isp_com_shadow_cnt_tag*)ISP_COMMON_SHADOW_CNT;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_com_shadow_cnt_tag* reg_s_ptr=(union _isp_com_shadow_cnt_tag*)&isp_reg_ptr->COM_SHADOW_CNT;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.counter=conter;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_COMMON_SHADOW_CNT - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->COM_SHADOW_CNT;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispAXIStop(uint32_t handler_id, uint8_t eb)
{
/*	union _isp_com_hw_mask_tag* reg_ptr=(union _isp_com_hw_mask_tag*)ISP_COMMON_HW_MASK;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_com_hw_mask_tag* reg_s_ptr=(union _isp_com_hw_mask_tag*)&isp_reg_ptr->COM_HW_MASK;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.raw_rgb=eb;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_COMMON_HW_MASK - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->COM_HW_MASK;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSliceCntEb(uint32_t handler_id, uint8_t eb)
{
/*	union _isp_com_slice_cnt_v0001_tag* reg_ptr=(union _isp_com_slice_cnt_v0001_tag*)ISP_COMMON_SLICE_CNT_V0001;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_com_slice_cnt_v0001_tag* reg_s_ptr=(union _isp_com_slice_cnt_v0001_tag*)&isp_reg_ptr->COM_SLICE_CNT;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_s_ptr->mBits.slice_cnt_eb=eb;
		/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
		reg_config.reg_addr = ISP_COMMON_SLICE_CNT_V0001- ISP_BASE_ADDR;
		reg_config.reg_value = reg_s_ptr->dwValue;
		write_param.reg_param = (unsigned long)&reg_config;
		write_param.counts = 1;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispPerformCntEb(uint32_t handler_id, uint8_t eb)
{
/*	union _isp_com_slice_cnt_v0001_tag* reg_ptr=(union _isp_com_slice_cnt_v0001_tag*)ISP_COMMON_SLICE_CNT_V0001;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_com_slice_cnt_v0001_tag* reg_s_ptr=(union _isp_com_slice_cnt_v0001_tag*)&isp_reg_ptr->COM_SLICE_CNT;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_s_ptr->mBits.pefrorm_cnt_eb=eb;
		/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
		reg_config.reg_addr = ISP_COMMON_SLICE_CNT_V0001- ISP_BASE_ADDR;
		reg_config.reg_value = reg_s_ptr->dwValue;
		write_param.reg_param = (unsigned long)&reg_config;
		write_param.counts = 1;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispSetSliceNum(uint32_t handler_id, uint8_t num)
{
/*	union _isp_com_slice_cnt_v0001_tag* reg_ptr=(union _isp_com_slice_cnt_v0001_tag*)ISP_COMMON_SLICE_CNT_V0001;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_com_slice_cnt_v0001_tag* reg_s_ptr=(union _isp_com_slice_cnt_v0001_tag*)&isp_reg_ptr->COM_SLICE_CNT;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_s_ptr->mBits.slice_cnt_num=num;
		/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
		reg_config.reg_addr = ISP_COMMON_SLICE_CNT_V0001- ISP_BASE_ADDR;
		reg_config.reg_value = reg_s_ptr->dwValue;
		write_param.reg_param = (unsigned long)&reg_config;
		write_param.counts = 1;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
uint8_t ispGetSliceNum(uint32_t handler_id)
{
/*	union _isp_com_slice_cnt_v0001_tag* reg_ptr=(union _isp_com_slice_cnt_v0001_tag*)ISP_COMMON_SLICE_CNT_V0001;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_com_slice_cnt_v0001_tag* reg_s_ptr=(union _isp_com_slice_cnt_v0001_tag*)&isp_reg_ptr->COM_SLICE_CNT;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;
	uint32_t isp_id=_isp_GetIspId();
	uint8_t num=0x00;

	if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_config.reg_addr = ISP_COMMON_SLICE_CNT_V0001 - ISP_BASE_ADDR;
		read_param.reg_param = (unsigned long)&reg_config;
		read_param.counts = 1;

	/*	*com_status=reg_ptr->dwValue;*/
	/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
		if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
			reg_s_ptr->dwValue = reg_config.reg_value;
			num = reg_s_ptr->mBits.slice_counter;
		}
	}

	return num;
}

/*	--
*@
*@
*/
int32_t ispGetPerformCntRStatus(uint32_t handler_id, uint32_t* cnt)
{
/*	union _isp_com_perform_cnt_r_v0001_tag* reg_ptr=(union _isp_com_perform_cnt_r_v0001_tag*)ISP_COMMON_PERFORM_CNT_R_V0001;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_com_perform_cnt_r_v0001_tag* reg_s_ptr=(union _isp_com_perform_cnt_r_v0001_tag*)&isp_reg_ptr->COM_PERFORM_CNT_R;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_config.reg_addr = ISP_COMMON_PERFORM_CNT_R_V0001 - ISP_BASE_ADDR;
		read_param.reg_param = (unsigned long)&reg_config;
		read_param.counts = 1;

		/*	*com_status=reg_ptr->dwValue;*/
		/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
		if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
			reg_s_ptr->dwValue = reg_config.reg_value;
			*cnt=reg_s_ptr->dwValue;
		}
	}
	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispGetPerformCntStatus(uint32_t handler_id, uint32_t* cnt)
{
/*	union _isp_com_perform_cnt_v0001_tag* reg_ptr=(union _isp_com_perform_cnt_v0001_tag*)ISP_COMMON_PERFORM_CNT_V0001;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_com_perform_cnt_v0001_tag* reg_s_ptr=(union _isp_com_perform_cnt_v0001_tag*)&isp_reg_ptr->COM_PERFORM_CNT;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_config.reg_addr = ISP_COMMON_PERFORM_CNT_R_V0001 - ISP_BASE_ADDR;
		read_param.reg_param = (unsigned long)&reg_config;
		read_param.counts = 1;

		/*	*com_status=reg_ptr->dwValue;*/
		/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
		if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
			reg_s_ptr->dwValue = reg_config.reg_value;
			*cnt=reg_s_ptr->dwValue;
		}
	}
	return ISP_SUCCESS;
}

/* YIQ */
/*	--
*@
*@
*/
int32_t ispGetYiqStatus(uint32_t handler_id, uint32_t* yiq_status)
{
/*	union _isp_yiq_status_v0001_tag* reg_ptr=(union _isp_yiq_status_v0001_tag*)ISP_YIQ_STATUS_V0001;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_yiq_status_v0001_tag* reg_s_ptr=(union _isp_yiq_status_v0001_tag*)&isp_reg_ptr->YIQ_STATUS_V0001;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
	ISP_CHECK_FD;

	reg_config.reg_addr = ISP_YIQ_STATUS_V0001 - ISP_BASE_ADDR;
	read_param.reg_param = (unsigned long)&reg_config;
	read_param.counts = 1;

	/*	*com_status=reg_ptr->dwValue;*/
	/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		reg_s_ptr->dwValue = reg_config.reg_value;
		*yiq_status=reg_s_ptr->dwValue;
		}
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispYGammabypass(uint32_t handler_id, uint8_t bypass)
{
/*	union _isp_yiq_param_v0001_tag* reg_ptr=(union _isp_yiq_param_v0001_tag*)ISP_YIQ_PARAM_V0001;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_yiq_param_v0001_tag* reg_s_ptr=(union _isp_yiq_param_v0001_tag*)&isp_reg_ptr->YIQ_PARAM_V0001;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_s_ptr->mBits.ygamma_bypass=bypass;
		/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
		reg_config.reg_addr = ISP_YIQ_PARAM_V0001 - ISP_BASE_ADDR;
		reg_config.reg_value = reg_s_ptr->dwValue;
		write_param.reg_param = (unsigned long)&reg_config;
		write_param.counts = 1;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispSetYGammaXNode(uint32_t handler_id, uint8_t* node)
{
/*	union _isp_ygamma_node_x0_v0001_tag* reg0_ptr=(union _isp_ygamma_node_x0_v0001_tag*)ISP_YGAMMA_X0_V0001;
	union _isp_ygamma_node_x1_v0001_tag* reg1_ptr=(union _isp_ygamma_node_x1_v0001_tag*)ISP_YGAMMA_X1_V0001;
*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_ygamma_node_x0_v0001_tag* reg0_s_ptr=(union _isp_ygamma_node_x0_v0001_tag*)&isp_reg_ptr->YGAMMA_X0_V0001;
	union _isp_ygamma_node_x1_v0001_tag* reg1_s_ptr=(union _isp_ygamma_node_x1_v0001_tag*)&isp_reg_ptr->YGAMMA_X1_V0001;
	struct isp_reg_bits reg_config[2];
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8830_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg0_s_ptr->mBits.node0=node[0];
		reg0_s_ptr->mBits.node1=node[1];
		reg0_s_ptr->mBits.node2=node[2];
		reg0_s_ptr->mBits.node3=node[3];
		reg1_s_ptr->mBits.node4=node[4];
		reg1_s_ptr->mBits.node5=node[5];
		reg1_s_ptr->mBits.node6=node[6];
		reg1_s_ptr->mBits.node7=node[7];

		/*
		reg0_ptr->dwValue=reg0_s_ptr->dwValue;
		reg1_ptr->dwValue=reg1_s_ptr->dwValue;
		*/
		reg_config[0].reg_addr = ISP_YGAMMA_X0_V0001 - ISP_BASE_ADDR;
		reg_config[0].reg_value = isp_reg_ptr->YGAMMA_X0_V0001;
		reg_config[1].reg_addr = ISP_YGAMMA_X1_V0001 - ISP_BASE_ADDR;
		reg_config[1].reg_value = isp_reg_ptr->YGAMMA_X1_V0001;

		write_param.reg_param = (unsigned long)&reg_config[0];
		write_param.counts = 2;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispSetYGammaYNode(uint32_t handler_id, uint16_t* node)
{
/*	union _isp_ygamma_node_y0_v0001_tag* reg0_ptr=(union _isp_ygamma_node_y0_v0001_tag*)ISP_YGAMMA_Y0_V0001;
	union _isp_ygamma_node_y1_v0001_tag* reg1_ptr=(union _isp_ygamma_node_y1_v0001_tag*)ISP_YGAMMA_Y1_V0001;
	union _isp_ygamma_node_y2_v0001_tag* reg1_ptr=(union _isp_ygamma_node_y1_v0001_tag*)ISP_YGAMMA_Y1_V0001;
*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_ygamma_node_y0_v0001_tag* reg0_s_ptr=(union _isp_ygamma_node_y0_v0001_tag*)&isp_reg_ptr->YGAMMA_Y0_V0001;
	union _isp_ygamma_node_y1_v0001_tag* reg1_s_ptr=(union _isp_ygamma_node_y1_v0001_tag*)&isp_reg_ptr->YGAMMA_Y1_V0001;
	union _isp_ygamma_node_y2_v0001_tag* reg2_s_ptr=(union _isp_ygamma_node_y2_v0001_tag*)&isp_reg_ptr->YGAMMA_Y1_V0001;
	struct isp_reg_bits reg_config[3];
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8830_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg0_s_ptr->mBits.node0=node[0];
		reg0_s_ptr->mBits.node1=node[1];
		reg0_s_ptr->mBits.node2=node[2];
		reg0_s_ptr->mBits.node3=node[3];
		reg1_s_ptr->mBits.node4=node[4];
		reg1_s_ptr->mBits.node5=node[5];
		reg1_s_ptr->mBits.node6=node[6];
		reg1_s_ptr->mBits.node7=node[7];
		reg2_s_ptr->mBits.node8=node[8];
		reg2_s_ptr->mBits.node9=node[9];

		/*
		reg0_ptr->dwValue=reg0_s_ptr->dwValue;
		reg1_ptr->dwValue=reg1_s_ptr->dwValue;
		reg2_ptr->dwValue=reg2_s_ptr->dwValue;
		*/
		reg_config[0].reg_addr = ISP_YGAMMA_Y0_V0001 - ISP_BASE_ADDR;
		reg_config[0].reg_value = reg0_s_ptr->dwValue;
		reg_config[1].reg_addr = ISP_YGAMMA_Y1_V0001 - ISP_BASE_ADDR;
		reg_config[1].reg_value = reg1_s_ptr->dwValue;
		reg_config[1].reg_addr = ISP_YGAMMA_Y1_V0001 - ISP_BASE_ADDR;
		reg_config[1].reg_value = reg2_s_ptr->dwValue;

		write_param.reg_param = (unsigned long)&reg_config[0];
		write_param.counts = 3;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispSetYGammaNodeIdx(uint32_t handler_id, uint16_t* index)
{
/*	union _isp_ygamma_node_idx_v0001_tag* reg_ptr=(union _isp_ygamma_node_idx_v0001_tag*)ISP_YGAMMA_NODE_IDX_V0001;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_ygamma_node_idx_v0001_tag* reg_s_ptr=(union _isp_ygamma_node_idx_v0001_tag*)&isp_reg_ptr->YGAMMA_NODE_IDX_V0001;
	struct isp_reg_bits reg_config[1];
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8830_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_s_ptr->mBits.index1=index[0];
		reg_s_ptr->mBits.index2=index[1];
		reg_s_ptr->mBits.index3=index[2];
		reg_s_ptr->mBits.index4=index[3];
		reg_s_ptr->mBits.index5=index[4];
		reg_s_ptr->mBits.index6=index[5];
		reg_s_ptr->mBits.index7=index[6];
		reg_s_ptr->mBits.index8=index[7];
		reg_s_ptr->mBits.index9=index[8];

		/*
		reg0_ptr->dwValue=reg0_s_ptr->dwValue;
		*/
		reg_config[0].reg_addr = ISP_YGAMMA_NODE_IDX_V0001 - ISP_BASE_ADDR;
		reg_config[0].reg_value = reg_s_ptr->dwValue;

		write_param.reg_param = (unsigned long)&reg_config[0];
		write_param.counts = 1;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispAutoFlickerVHeight(uint32_t handler_id, uint16_t height)
{
/*	union _isp_flicker_v_height_v0001_tag* reg_ptr=(union _isp_flicker_v_height_v0001_tag*)ISP_AF_V_HEIGHT_V0001;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_flicker_v_height_v0001_tag* reg_s_ptr=(union _isp_flicker_v_height_v0001_tag*)&isp_reg_ptr->AF_V_HEIGHT_V0001;

	struct isp_reg_bits reg_config[1];
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_s_ptr->mBits.v_height=height;

		/*
		reg_ptr->dwValue=reg_s_ptr->dwValue;
		*/
		reg_config[0].reg_addr = ISP_AF_V_HEIGHT_V0001 - ISP_BASE_ADDR;
		reg_config[0].reg_value = reg_s_ptr->dwValue;

		write_param.reg_param = (unsigned long)&reg_config[0];
		write_param.counts = 1;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispAutoFlickerLineConter(uint32_t handler_id, uint16_t counter)
{
/*	union _isp_flicker_line_cnt_v0001_tag* reg_ptr=(union _isp_flicker_line_cnt_v0001_tag*)ISP_AF_LINE_COUNTER_V0001;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_flicker_line_cnt_v0001_tag* reg_s_ptr=(union _isp_flicker_line_cnt_v0001_tag*)&isp_reg_ptr->AF_LINE_COUNTER_V0001;

	struct isp_reg_bits reg_config[1];
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_s_ptr->mBits.line_cnt=counter;
		/*
		reg_ptr->dwValue=reg_s_ptr->dwValue;
		*/
		reg_config[0].reg_addr = ISP_AF_LINE_COUNTER_V0001 - ISP_BASE_ADDR;
		reg_config[0].reg_value = reg_s_ptr->dwValue;

		write_param.reg_param = (unsigned long)&reg_config[0];
		write_param.counts = 1;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispAutoFlickerLineStep(uint32_t handler_id, uint8_t step)
{
/*	union _isp_flicker_line_step_v0001_tag* reg_ptr=(union _isp_flicker_line_step_v0001_tag*)ISP_AF_LINE_STEP_V0001;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_flicker_line_step_v0001_tag* reg_s_ptr=(union _isp_flicker_line_step_v0001_tag*)&isp_reg_ptr->AF_LINE_STEP_V0001;

	struct isp_reg_bits reg_config[1];
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_s_ptr->mBits.line_step=step;
		/*
		reg_ptr->dwValue=reg_s_ptr->dwValue;
		*/
		reg_config[0].reg_addr = ISP_AF_LINE_STEP_V0001 - ISP_BASE_ADDR;
		reg_config[0].reg_value = reg_s_ptr->dwValue;

		write_param.reg_param = (unsigned long)&reg_config[0];
		write_param.counts = 1;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispAutoFlickerLineStart(uint32_t handler_id, uint16_t start_line)
{
/*	union _isp_flicker_line_start_v0001_tag* reg_ptr=(union _isp_flicker_line_start_v0001_tag*)ISP_AF_LINE_START_V0001;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_flicker_line_start_v0001_tag* reg_s_ptr=(union _isp_flicker_line_start_v0001_tag*)&isp_reg_ptr->AF_LINE_STEP_V0001;

	struct isp_reg_bits reg_config[1];
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_s_ptr->mBits.line_start=start_line;
		/*
		reg_ptr->dwValue=reg_s_ptr->dwValue;
		*/
		reg_config[0].reg_addr = ISP_AF_LINE_START_V0001 - ISP_BASE_ADDR;
		reg_config[0].reg_value = reg_s_ptr->dwValue;

		write_param.reg_param = (unsigned long)&reg_config[0];
		write_param.counts = 1;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispAembypass(uint32_t handler_id, uint8_t bypass)
{
/*	union _isp_yiq_param_v0001_tag* reg_ptr=(union _isp_yiq_param_v0001_tag*)ISP_YIQ_PARAM_V0001;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_yiq_param_v0001_tag* reg_s_ptr=(union _isp_yiq_param_v0001_tag*)&isp_reg_ptr->YIQ_PARAM_V0001;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_s_ptr->mBits.ae_bypass=bypass;
		/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
		reg_config.reg_addr = ISP_YIQ_PARAM_V0001 - ISP_BASE_ADDR;
		reg_config.reg_value = reg_s_ptr->dwValue;
		write_param.reg_param = (unsigned long)&reg_config;
		write_param.counts = 1;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispAemMode(uint32_t handler_id, uint8_t mode)
{
/*	union _isp_yiq_param_v0001_tag* reg_ptr=(union _isp_yiq_param_v0001_tag*)ISP_YIQ_PARAM_V0001;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_yiq_param_v0001_tag* reg_s_ptr=(union _isp_yiq_param_v0001_tag*)&isp_reg_ptr->YIQ_PARAM_V0001;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_s_ptr->mBits.ae_mode=mode;
		/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
		reg_config.reg_addr = ISP_YIQ_PARAM_V0001 - ISP_BASE_ADDR;
		reg_config.reg_value = reg_s_ptr->dwValue;
		write_param.reg_param = (unsigned long)&reg_config;
		write_param.counts = 1;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispAemSkipNum(uint32_t handler_id, uint8_t num)
{
/*	union _isp_yiq_param_v0001_tag* reg_ptr=(union _isp_yiq_param_v0001_tag*)ISP_YIQ_PARAM_V0001;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_yiq_param_v0001_tag* reg_s_ptr=(union _isp_yiq_param_v0001_tag*)&isp_reg_ptr->YIQ_PARAM_V0001;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_s_ptr->mBits.ae_skip_num=num;
		/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
		reg_config.reg_addr = ISP_YIQ_PARAM_V0001 - ISP_BASE_ADDR;
		reg_config.reg_value = reg_s_ptr->dwValue;
		write_param.reg_param = (unsigned long)&reg_config;
		write_param.counts = 1;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispAemSourceSel(uint32_t handler_id, uint8_t src_sel)
{
/*	union _isp_yiq_param_v0001_tag* reg_ptr=(union _isp_yiq_param_v0001_tag*)ISP_YIQ_PARAM_V0001;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_yiq_param_v0001_tag* reg_s_ptr=(union _isp_yiq_param_v0001_tag*)&isp_reg_ptr->YIQ_PARAM_V0001;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_s_ptr->mBits.src_sel=src_sel;
		/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
		reg_config.reg_addr = ISP_YIQ_PARAM_V0001 - ISP_BASE_ADDR;
		reg_config.reg_value = reg_s_ptr->dwValue;
		write_param.reg_param = (unsigned long)&reg_config;
		write_param.counts = 1;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispGetAEMStatistic(uint32_t handler_id, uint32_t* y_info)
{
/*	union _isp_mem_reg_tag* reg_ptr=(union _isp_mem_reg_tag*)ISP_AWBM_OUTPUT;*/
	uint32_t i = 0x00;
	uint32_t k = 0x00;
	uint32_t temp32_0 = 0x00;
	uint32_t temp32_1 = 0x00;
	uint32_t offset_addr = ISP_AEM_OUTPUT - ISP_BASE_ADDR;
	struct isp_reg_bits reg_config[ISP_AEM_ITEM];
	struct isp_reg_param read_param;
	uint32_t isp_id=_isp_GetIspId();

/*	ISP_CHECK_FD;*/

	for(i=0x00; i<ISP_AEM_ITEM; i++) {
		reg_config[i].reg_addr = offset_addr;
		offset_addr += 4;
	}
	read_param.reg_param = (unsigned long)&reg_config;
	read_param.counts = ISP_AEM_ITEM;

	switch(isp_id)
	{
		case SC8830_ISP_ID:
		case SC9630_ISP_ID:
		{
			if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param))
			{
				for(i=0x00; i<ISP_AEM_ITEM; i++)
				{
					y_info[i]=reg_config[i].reg_value & 0x3fffff;
				}
			}
			break ;
		}
		defalult:
		{
			break ;
		}
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispAutoFlickerbypass(uint32_t handler_id, uint8_t bypass)
{
/*	union _isp_yiq_param_v0001_tag* reg_ptr=(union _isp_yiq_param_v0001_tag*)ISP_YIQ_PARAM_V0001;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_yiq_param_v0001_tag* reg_s_ptr=(union _isp_yiq_param_v0001_tag*)&isp_reg_ptr->YIQ_PARAM_V0001;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_s_ptr->mBits.flicker_bypass=bypass;
		/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
		reg_config.reg_addr = ISP_YIQ_PARAM_V0001 - ISP_BASE_ADDR;
		reg_config.reg_value = reg_s_ptr->dwValue;
		write_param.reg_param = (unsigned long)&reg_config;
		write_param.counts = 1;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispAutoFlickerMode(uint32_t handler_id, uint8_t mode)
{
/*	union _isp_yiq_param_v0001_tag* reg_ptr=(union _isp_yiq_param_v0001_tag*)ISP_YIQ_PARAM_V0001;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_yiq_param_v0001_tag* reg_s_ptr=(union _isp_yiq_param_v0001_tag*)&isp_reg_ptr->YIQ_PARAM_V0001;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_s_ptr->mBits.flicker_mode=mode;
		/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
		reg_config.reg_addr = ISP_YIQ_PARAM_V0001 - ISP_BASE_ADDR;
		reg_config.reg_value = reg_s_ptr->dwValue;
		write_param.reg_param = (unsigned long)&reg_config;
		write_param.counts = 1;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispIntRegister(uint32_t handler_id, uint32_t int_num, void(*fun)())
{
	union _isp_com_int_eb_tag* reg_ptr=(union _isp_com_int_eb_tag*)ISP_COMMON_ENA_INTERRUPT;

	reg_ptr->dwValue|=(1<<int_num);
	s_isp_isr[int_num]=fun;

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispIntClear(uint32_t handler_id, uint32_t int_num)
{
	union _isp_com_int_clr_tag* reg_ptr=(union _isp_com_int_clr_tag*)ISP_COMMON_CLR_INTERRUPT;

	reg_ptr->dwValue|=(1<<int_num);

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispGetIntRaw(uint32_t handler_id, uint32_t* raw)
{
	union _isp_com_int_raw_tag* reg_ptr=(union _isp_com_int_raw_tag*)ISP_COMMON_RAW_INTERRUPT;

	*raw=reg_ptr->dwValue;

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispGetIntCom(uint32_t handler_id, uint32_t* raw)
{
	union _isp_com_int_tag* reg_ptr=(union _isp_com_int_tag*)ISP_COMMON_INTERRUPT;

	*raw=reg_ptr->dwValue;

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispPreGlbGainBypass(uint32_t handler_id, uint8_t bypass)
{
#if 0
/*	union _isp_glb_gain_param_tag* reg_ptr=(union _isp_glb_gain_param_tag*)ISP_PRE_GLOBAL_GAIN;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_pre_global_gain_tag* reg_s_ptr=(union _isp_pre_global_gain_tag*)&isp_reg_ptr->PRE_GLB_GAIN_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.bypass=bypass;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_PRE_GLOBAL_GAIN - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->PRE_GLB_GAIN_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	_isp_write((uint32_t *)&write_param);
#endif
	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispPreSetGlbGain(uint32_t handler_id, uint32_t gain)
{
#if 0
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_pre_global_gain_tag* reg_s_ptr=(union _isp_pre_global_gain_tag*)&isp_reg_ptr->PRE_GLB_GAIN_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.gain=gain;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_PRE_GLOBAL_GAIN - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->PRE_GLB_GAIN_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	_isp_write((uint32_t *)&write_param);
#endif
	return ISP_SUCCESS;
}


/*Glb gain */
/*	--
*@
*@
*/
int32_t ispGetGlbGainStatus(uint32_t handler_id, uint32_t* status)
{
/*	union _isp_glb_gain_status_tag* reg_ptr=(union _isp_glb_gain_status_tag*)ISP_GAIN_GLB_STATUS;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_glb_gain_status_tag* reg_s_ptr=(union _isp_glb_gain_status_tag*)&isp_reg_ptr->GLB_GAIN_STATUS;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;

	ISP_CHECK_FD;

	reg_config.reg_addr = ISP_GAIN_GLB_STATUS - ISP_BASE_ADDR;
	read_param.reg_param = (unsigned long)&reg_config;
	read_param.counts = 1;

/*	reg_s_ptr->dwValue=reg_ptr->dwValue;*/
	if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		reg_s_ptr->dwValue = reg_config.reg_value;
		*status=reg_s_ptr->dwValue;
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispGlbGainBypass(uint32_t handler_id, uint8_t bypass)
{
/*	union _isp_glb_gain_param_tag* reg_ptr=(union _isp_glb_gain_param_tag*)ISP_GAIN_GLB_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_glb_gain_param_tag* reg_s_ptr=(union _isp_glb_gain_param_tag*)&isp_reg_ptr->GLB_GAIN_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.bypass=bypass;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_GAIN_GLB_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->GLB_GAIN_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetGlbGain(uint32_t handler_id, uint32_t gain)
{
/*	union _isp_glb_gain_param_tag* reg_ptr=(union _isp_glb_gain_param_tag*)ISP_GAIN_GLB_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_glb_gain_param_tag* reg_s_ptr=(union _isp_glb_gain_param_tag*)&isp_reg_ptr->GLB_GAIN_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.gain=gain;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_GAIN_GLB_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->GLB_GAIN_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispGlbGainSliceSize(uint32_t handler_id, uint16_t w, uint16_t h)
{
/*	union _isp_glb_gain_slice_size_tag* reg_ptr=(union _isp_glb_gain_slice_size_tag*)ISP_GAIN_SLICE_SIZE;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_glb_gain_slice_size_tag* reg_s_ptr=(union _isp_glb_gain_slice_size_tag*)&isp_reg_ptr->GLB_GAIN_SLICE_SIZE;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.slice_with=w;
	reg_s_ptr->mBits.slice_height=h;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_GAIN_SLICE_SIZE - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->GLB_GAIN_SLICE_SIZE;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*RGB gain */
/*	--
*@
*@
*/
int32_t ispGetChnGainStatus(uint32_t handler_id, uint32_t* status)
{
/*	union _isp_rgb_gain_status_tag* reg_ptr=(union _isp_rgb_gain_status_tag*)ISP_GAIN_RGB_STATUS;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_rgb_gain_status_tag* reg_s_ptr=(union _isp_rgb_gain_status_tag*)&isp_reg_ptr->RGB_GAIN_STATUS;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;

	ISP_CHECK_FD;

	reg_config.reg_addr = ISP_GAIN_RGB_STATUS - ISP_BASE_ADDR;
	read_param.reg_param = (unsigned long)&reg_config;
	read_param.counts = 1;

/*	reg_s_ptr->dwValue=reg_ptr->dwValue;*/
	if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		reg_s_ptr->dwValue = reg_config.reg_value;
		*status=reg_s_ptr->dwValue;
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispChnGainBypass(uint32_t handler_id, uint8_t bypass)
{
/*	union _isp_rgb_gain_param_tag* reg_ptr=(union _isp_rgb_gain_param_tag*)ISP_GAIN_RGB_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_rgb_gain_param_tag* reg_s_ptr=(union _isp_rgb_gain_param_tag*)&isp_reg_ptr->RGB_GAIN_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.bypass=bypass;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_GAIN_RGB_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->RGB_GAIN_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetChnGain(uint32_t handler_id, uint16_t r_gain, uint16_t g_gain, uint16_t b_gain)
{
/*	union _isp_rgb_gain_param_tag* reg_ptr=(union _isp_rgb_gain_param_tag*)ISP_GAIN_RGB_PARAM;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_rgb_gain_param_tag* reg_s_ptr=(union _isp_rgb_gain_param_tag*)&isp_reg_ptr->RGB_GAIN_PARAM;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.b_gain=b_gain;
	reg_s_ptr->mBits.g_gain=g_gain;
	reg_s_ptr->mBits.r_gain=r_gain;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_GAIN_RGB_PARAM - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->RGB_GAIN_PARAM;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispSetChnGainOffset(uint32_t handler_id, uint16_t r_offset, uint16_t g_offset, uint16_t b_offset)
{
/*	union _isp_rgb_gain_offset_tag* reg_ptr=(union _isp_rgb_gain_offset_tag*)ISP_GAIN_RGB_OFFSET;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_rgb_gain_offset_tag* reg_s_ptr=(union _isp_rgb_gain_offset_tag*)&isp_reg_ptr->RGB_GAIN_OFFSET;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	reg_s_ptr->mBits.b_offset=b_offset;
	reg_s_ptr->mBits.g_offset=g_offset;
	reg_s_ptr->mBits.r_offset=r_offset;
/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
	reg_config.reg_addr = ISP_GAIN_RGB_OFFSET - ISP_BASE_ADDR;
	reg_config.reg_value = isp_reg_ptr->RGB_GAIN_OFFSET;
	write_param.reg_param = (unsigned long)&reg_config;
	write_param.counts = 1;

	return _isp_write((uint32_t *)&write_param);
}

/* HUE */
/*	--
*@
*@
*/
int32_t ispGetHueStatus(uint32_t handler_id, uint32_t* status)
{
/*	union _isp_hue_status_v0001_tag* reg_ptr=(union _isp_hue_status_v0001_tag*)ISP_HUE_STATUS_V0001;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_hue_status_v0001_tag* reg_s_ptr=(union _isp_hue_status_v0001_tag*)&isp_reg_ptr->HUE_STATUS_V0001;
	struct isp_reg_bits reg_config;
	struct isp_reg_param read_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_config.reg_addr = ISP_HUE_STATUS_V0001 - ISP_BASE_ADDR;
		read_param.reg_param = (unsigned long)&reg_config;
		read_param.counts = 1;

		/*	*com_status=reg_ptr->dwValue;*/
		/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
		if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		reg_s_ptr->dwValue = reg_config.reg_value;
		*status=reg_s_ptr->dwValue;
		}
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispHuebypass(uint32_t handler_id, uint8_t bypass)
{
/*	union _isp_hue_param_v0001_tag* reg_ptr=(union _isp_hue_param_v0001_tag*)ISP_HUE_PARAM_V0001;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_hue_param_v0001_tag* reg_s_ptr=(union _isp_hue_param_v0001_tag*)&isp_reg_ptr->HUE_PARAM_V0001;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_s_ptr->mBits.bypass=bypass;
		/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
		reg_config.reg_addr = ISP_HUE_PARAM_V0001 - ISP_BASE_ADDR;
		reg_config.reg_value = reg_s_ptr->dwValue;
		write_param.reg_param = (unsigned long)&reg_config;
		write_param.counts = 1;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispSetHueFactor(uint32_t handler_id, uint8_t factor)
{
/*	union _isp_hue_param_v0001_tag* reg_ptr=(union _isp_hue_param_v0001_tag*)ISP_HUE_PARAM_V0001;*/
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	union _isp_hue_param_v0001_tag* reg_s_ptr=(union _isp_hue_param_v0001_tag*)&isp_reg_ptr->HUE_PARAM_V0001;
	struct isp_reg_bits reg_config;
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	if(SC8830_ISP_ID==isp_id || SC9630_ISP_ID==isp_id)
	{
		ISP_CHECK_FD;

		reg_s_ptr->mBits.theta=factor;
		/*	reg_ptr->dwValue=reg_s_ptr->dwValue;*/
		reg_config.reg_addr = ISP_HUE_PARAM_V0001 - ISP_BASE_ADDR;
		reg_config.reg_value = reg_s_ptr->dwValue;
		write_param.reg_param = (unsigned long)&reg_config;
		write_param.counts = 1;

		return _isp_write((uint32_t *)&write_param);
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispGetContinueSize(uint32_t handler_id, uint16_t* width, uint16_t* height)
{
	uint32_t isp_id=_isp_GetIspId();

	switch(isp_id)
	{
		case SC8825_ISP_ID:
		{
			*width=ISP_MAX_SLICE_WIDTH;
			*height=ISP_MAX_SLICE_HEIGHT;
			break ;
		}
		case SC8830_ISP_ID:
		{
			*width=ISP_MAX_SLICE_WIDTH_V0001;
			*height=ISP_MAX_SLICE_HEIGHT_V0001;
			break ;
		}
		case SC9630_ISP_ID:
		{
			*width=ISP_MAX_SLICE_WIDTH_V0001;
			*height=ISP_MAX_SLICE_HEIGHT_V0001;
			break ;
		}
		defalult:
		{
			break ;
		}
	}

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispGetSignalSize(uint32_t handler_id, uint16_t* width, uint16_t* height)
{
	uint32_t isp_id=_isp_GetIspId();

	switch(isp_id)
	{
		case SC8825_ISP_ID:
		{
			*width=ISP_MAX_WIDTH;
			*height=ISP_MAX_HEIGHT;
			break ;
		}
		case SC8830_ISP_ID:
		{
			*width=ISP_MAX_WIDTH_V0001;
			*height=ISP_MAX_HEIGHT_V0001;
			break ;
		}
		case SC9630_ISP_ID:
		{
			*width=ISP_MAX_WIDTH_V0001;
			*height=ISP_MAX_HEIGHT_V0001;
			break ;
		}
		defalult:
		{
			break ;
		}
	}

	ISP_LOG("capture max size: width:%d, height:%d",*width, *height);

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispGetAwbWinNum(uint32_t handler_id, uint16_t* width, uint16_t* height)
{
	*width=ISP_WB_WIN_NUM;
	*height=ISP_WB_WIN_NUM;

	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
uint32_t ispGetAwbDefaultGain(uint32_t handler_id)
{
	uint32_t gain=ISP_ZERO;
	uint32_t isp_id=_isp_GetIspId();

	switch(isp_id)
	{
		case SC8825_ISP_ID:
		{
			gain=ISP_AWB_DEFAULT_GAIN;
			break ;
		}
		case SC8830_ISP_ID:
		{
			gain=ISP_AWB_DEFAULT_GAIN_V0001;
			break ;
		}
		case SC9630_ISP_ID:
		{
			gain=ISP_AWB_DEFAULT_GAIN_V0001;
			break ;
		}
		defalult:
		{
			break ;
		}
	}

	return gain;
}

/*	--
*@
*@
*/
uint32_t ispGetAfMaxNum(uint32_t handler_id)
{
	return 0x09;
}

/*	--
*@
*@
*/
int32_t ispOpenClk(uint32_t handler_id, uint32_t clk_on)
{
	int32_t rtn = ISP_SUCCESS;

	return rtn;
}

int32_t ispOpenDev(uint32_t handler_id, uint32_t clk_on)
{
	if(ISP_ONE == clk_on) {
		if(-1 != s_isp_fd) {
			goto open_end;
		}
		s_isp_fd = open(s_isp_dev_name, O_RDWR, 0);
		if (-1 == s_isp_fd) {
			fprintf(stderr, "cannot open '%s': %d, %s\n", s_isp_dev_name, errno,  strerror(errno));
			exit(EXIT_FAILURE);
		} else {
		}
	} else {
		if(-1 != s_isp_fd) {
			if (-1 == close(s_isp_fd)) {
				fprintf(stderr, "error %d, %s\n", errno, strerror(errno));
				exit(EXIT_FAILURE);
			}
			s_isp_fd = -1;
		} else {
		}
	}
open_end:
	return ISP_SUCCESS;
}

/*	--
*@
*@
*/
int32_t ispReset(uint32_t handler_id, uint32_t rst)
{
	int32_t ret = ISP_SUCCESS;
	uint32_t param;

	ISP_CHECK_FD;

	if (-1 == ioctl(s_isp_fd, ISP_IO_RST, &param)) {
		ret = -1;
	}

	return ret;
}

/*	--
*@
*@
*/
int32_t ispGetIRQ(uint32_t *evt_ptr)
{
	int32_t ret = ISP_SUCCESS;
	uint32_t handler_id = ISP_ZERO;
	struct isp_irq_param*ptr = (struct isp_irq_param*)evt_ptr;

	ISP_CHECK_FD;

	while (1) {
		ret = ioctl(s_isp_fd, ISP_IO_IRQ, ptr);
		if (0 == ret) {
			break;
		} else {
			if (-EINTR == ptr->ret_val) {
				usleep(5000);
				ISP_LOG("continue.");
				continue;
			}
			isp_perror("ispGetIRQ failed");
			ISP_LOG("ret_val=%d", ptr->ret_val);
			break;
		}
	}

	return ret;
}

/*	--
*@
*@
*/
int32_t ispRegIRQ(uint32_t handler_id, uint32_t int_num)
{
	int32_t ret = ISP_SUCCESS;
	uint32_t interrupt_num = int_num;

	ISP_CHECK_FD;

	if (-1 == ioctl(s_isp_fd, ISP_IO_INT, (void*)&interrupt_num)) {
		ret = -1;
	}

	return ret;
}

/*	--
*@
*@
*/
int32_t ispCfgDcamIRQ(uint32_t handler_id, uint32_t param)
{
	int32_t ret = ISP_SUCCESS;
	uint32_t irq_param = param;

	ISP_CHECK_FD;

	if (-1 == ioctl(s_isp_fd, ISP_IO_DCAM_INT, (void*)&irq_param)) {
		ret = -1;
	}

	return ret;
}

/*	--
*@
*@
*/
int32_t ispSetLncParam(uint32_t handler_id, unsigned long addr, uint32_t len)
{
	int32_t ret = ISP_SUCCESS;
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	write_param.reg_param = addr;
	write_param.counts = len;
	if (-1 == ioctl(s_isp_fd, ISP_IO_LNC_PARAM, (uint32_t *)&write_param)) {
		ret = -1;
		isp_perror("ispSetLncParam failed");
		ISP_LOG("kernal alloc lnc param buf error");
	}

	return ret;
}

/*	--
*@
*@
*/
uint32_t ispAlloc(uint32_t handler_id, uint32_t len)
{
	struct isp_reg_param write_param;

	ISP_CHECK_FD;

	write_param.reg_param = 0x00;
	write_param.counts = len;
//	ISP_LOG("ispAlloc: len=%d\n", len);
	if(len==0)
	{
		return 0;
	}
	if (-1 == ioctl(s_isp_fd, ISP_IO_ALLOC, (uint32_t *)&write_param)) {
		write_param.reg_param = 0x00;
	}

	return write_param.reg_param;
}

/*	--
*@
*@
*/
int32_t ispStop(uint32_t handler_id)
{
	int32_t ret = ISP_SUCCESS;
	uint32_t param;

	ISP_CHECK_FD;

	if (-1 == ioctl(s_isp_fd, ISP_IO_STOP, &param)) {
		ret = -1;
	}

	return ret;
}

/*	--
*@
*@
*/
int32_t ispRegWrite(uint32_t handler_id, uint32_t num, void* param_ptr)
{
	struct isp_reg_bits reg_config[20];
	struct isp_reg_param write_param;
	uint32_t i=0x00;
	uint32_t reg_num=num;
	uint32_t* reg_ptr=(uint32_t*)param_ptr;
	uint32_t isp_id=_isp_GetIspId();

	ISP_CHECK_FD;

	if (20<reg_num) {
		reg_num=20;
	}

	for (i=0x00; i<reg_num; i++) {
		reg_config[i].reg_addr = (*reg_ptr++)&0xffff;
		reg_config[i].reg_value = *reg_ptr++;
	}

	write_param.reg_param = (unsigned long)&reg_config[0];
	write_param.counts = reg_num;

	return _isp_write((uint32_t *)&write_param);
}

/*	--
*@
*@
*/
int32_t ispRegRead(uint32_t handler_id, uint32_t num, void* param_ptr)
{
	struct isp_reg_bits reg_config[20];
	struct isp_reg_param read_param;
	uint32_t i=0x00;
	uint32_t reg_num=num;
	uint32_t* reg_ptr=(uint32_t*)param_ptr;
	uint32_t reg_addr=reg_ptr[0]&0xffff;
	uint32_t isp_id=_isp_GetIspId();

	ISP_CHECK_FD;

	if (20<reg_num) {
		reg_num=20;
	}

	for (i=0x00; i<reg_num; i++) {
		reg_config[i].reg_addr = reg_addr + i*0x04;
	}

	read_param.reg_param = (unsigned long)&reg_config[0];
	read_param.counts = reg_num;

	if(ISP_SUCCESS == _isp_read((uint32_t *)&read_param)) {
		reg_addr=reg_ptr[0];
		for (i=0x00; i<reg_num; i++) {
			*reg_ptr++ = reg_addr + i*0x04;
			*reg_ptr++ = reg_config[i].reg_value;
		}
	}

	return ISP_SUCCESS;
}

int ispGetRegVal(uint32_t handler_id, uint32_t base_offset, uint32_t *buf, uint32_t len)
{
/*	union _isp_mem_reg_tag* reg_ptr=(union _isp_mem_reg_tag*)ISP_AWBM_OUTPUT;*/
	int32_t ret = ISP_SUCCESS;
	uint32_t i = 0x00;
	uint32_t offset_addr = base_offset;
	struct isp_reg_bits *reg_config =(struct isp_reg_bits *)buf;
	struct isp_reg_param read_param;
	uint32_t isp_id=_isp_GetIspId();

/*	ISP_CHECK_FD;*/

	for(i=0x00; i<len; i++) {
		reg_config[i].reg_addr = offset_addr;
		offset_addr += 4;
	}
	read_param.reg_param = (unsigned long)reg_config;
	read_param.counts = len;

	return _isp_read((uint32_t *)&read_param);
}

int32_t ispBypassNewFeature(uint32_t handler_id)
{
	struct isp_reg*isp_reg_ptr = _isp_GetRegPtr(handler_id);
	struct isp_reg_bits reg_config[32];
	struct isp_reg_param write_param;
	uint32_t isp_id=_isp_GetIspId();

	ISP_CHECK_FD;

	if (isp_id == SC9630_ISP_ID)
	{
		reg_config[0].reg_addr = 0x1C14;  //nawbm, new feature
		reg_config[0].reg_value = 0x01;
		reg_config[1].reg_addr = 0x1D14; //wavelet, new feature
		reg_config[1].reg_value = 0x01;
		reg_config[2].reg_addr = 0x1E14; //binning4awb, new feature
		reg_config[2].reg_value = 0x01;
		reg_config[3].reg_addr = 0x1F14; //pre gain, new feature
		reg_config[3].reg_value = 0x01;

		write_param.reg_param = (unsigned long)&reg_config[0];
		write_param.counts = 4;

		_isp_write((uint32_t *)&write_param);
	}
	return ISP_SUCCESS;
}

void isp_perror(const char *prefix)
{
	char   buff[100];
	uint32_t handler_id = 0;


	strerror_r(errno, buff, sizeof(buff));
	if (prefix) {
		ISP_LOG("%s:errno=%d, %s", prefix, errno, buff);
	} else {
		ISP_LOG("errno=%d, %s", errno, buff);
	}
}

/**----------------------------------------------------------------------------*
*			Compiler Flag						*
**----------------------------------------------------------------------------*/
#ifdef	 __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/

