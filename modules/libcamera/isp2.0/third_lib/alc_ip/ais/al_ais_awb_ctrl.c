/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "awb_al_ctrl"

#include "awb_ctrl.h"
#include "isp_awb.h"
#include "awb_packet.h"
#include "isp_com.h"
#include "ae_misc.h"
#include <cutils/properties.h>


#if  !defined( CONFIG_USE_ALC_AWB)  ||  !defined( CONFIG_USE_ALC_AE)
#error invalid make target...
#endif



//#define LIB_DYNAMIC_LOAD
//#define CCM_DEBUG_FREQ_CHAMGE
//#define LSC_DEBUG_TABLE

//#define LSC_SIZE	(1280)	//20*16 *4 color
#define LSC_SIZE	(24*19*4)	//front is 24*10 *4 color



#include    <dlfcn.h>
//#include "AlAwbInterface.h"
#include "AlAisInterface.h"


/*------------------------------------------------------------------------------*
*					Compiler Flag				*
*-------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------*
*					Micro Define				*
*-------------------------------------------------------------------------------*/
#define AWB_CTRL_MAGIC_BEGIN		0xe5a55e5a
#define AWB_CTRL_MAGIC_END		0x5e5ae5a5
#define AWB_CTRL_RESOLUTION_NUM 	8
#define AWB_CTRL_MWB_NUM		20	
#define AWB_CTRL_SCENEMODE_NUM	10
#define AWBV_WEIGHT_UNIT 256

#define     UNUSED(param)  (void)(param)

#define AWB_CTRL_TRUE			1
#define AWB_CTRL_FALSE			0
#define AWB_CTRL_WORK_MODE_NUM		8
#define AWB_CTRL_ENABLE 1
#define AWB_CTRL_LOCKMODE 1
#define AWB_CTRL_UNLOCKMODE 0
#define AWB_CTRL_SAFE_FREE(_p) \
	do { \
		if (NULL != (_p)) {\
			free(_p); \
			_p = NULL; \
		} \
	}while(0)

/*------------------------------------------------------------------------------*
*					structures				*
*-------------------------------------------------------------------------------*/
struct awb_ctrl_tuning_param {
	/**/
	uint32_t enable;
	/*window size of statistic image*/
	struct awb_ctrl_size stat_win_size;
	/*start position of statistic area*/
	struct awb_ctrl_pos stat_start_pos;
	/*compensate gain for each resolution*/
	struct awb_ctrl_gain compensate_gain[AWB_CTRL_RESOLUTION_NUM];
	/*gain for each manual white balance*/
	struct awb_ctrl_gain mwb_gain[AWB_CTRL_MWB_NUM];
	/*gain for each scenemode gain*/
	struct awb_ctrl_gain scene_gain[AWB_CTRL_SCENEMODE_NUM];
	/*bv value range for awb*/
	struct awb_ctrl_bv bv_range;
	/*init gain and ct*/
	struct awb_ctrl_gain init_gain;
	uint32_t init_ct;
	/*algorithm param*/
	void *alg_param;
	/*algorithm param size*/
	uint32_t alg_param_size;
};

struct tg_dmy_log
{
	char	head[4+4];
	uint16_t tar[20*16*4];
	uint16_t ref[20*16*4];
	char	foot[4];
};//dmy_log;


struct awb_ctrl_cxt {
	/*must be the first one*/
	uint32_t magic_begin;
	/*awb status lock*/
	pthread_mutex_t status_lock;
	/*initialize parameter*/
	struct awb_ctrl_init_param init_param;
	/*tuning parameter for each work mode*/
	struct awb_ctrl_tuning_param tuning_param[AWB_CTRL_WORK_MODE_NUM];
	/*whether initialized*/
	uint32_t init;
	/*camera id*/
	uint32_t camera_id; /* 0: back camera, 1: front camera */
	void* lsc_otp_random;
	void* lsc_otp_golden;
	uint32_t lsc_otp_width;
	uint32_t lsc_otp_height;
	/*work mode*/
	uint32_t work_mode;   /* 0: preview, 1:capture, 2:video */
	/*white balance mode: auto or manual*/
	enum awb_ctrl_wb_mode wb_mode;
	/*scene mode*/
	enum awb_ctrl_scene_mode scene_mode;
	/*format of statistic image*/
	enum awb_ctrl_stat_img_format stat_img_format;
	/*statistic image size*/
	struct awb_ctrl_size stat_img_size;
	/*previous gain*/
	struct awb_ctrl_gain prv_gain;
	/*flash info*/
	struct awb_flash_info flash_info;
	/*previous ct*/
	uint32_t prv_ct;
	/*current gain*/
	struct awb_ctrl_gain cur_gain;
	/*output gain*/
	struct awb_ctrl_gain output_gain;
	/*output ct*/
	uint32_t output_ct;
	/*recover gain*/
	struct awb_ctrl_gain recover_gain;
	/*recover ct*/
	uint32_t recover_ct;
	/*recover awb mode*/
	enum awb_ctrl_wb_mode recover_mode;
	/*awb lock info */
	struct awb_ctrl_lock_info lock_info;
	/*current ct*/
	uint32_t cur_ct;
	/*whether to update awb gain*/
	uint32_t update;
	/*algorithm handle*/
	void *alg_handle;
	/*statistic image buffer*/
	void *stat_img_buf;
	/*statistic image buffer size*/
	uint32_t stat_img_buf_size;

//Acutelogic modified
	TT_AlAisInterface	*mptAis;

	struct tg_dmy_log	dmy_log	;
	/*must be the last one*/
	uint32_t magic_end;
};

/*------------------------------------------------------------------------------*
*					local function declaration		*
*-------------------------------------------------------------------------------*/

uint32_t _awb_get_gain(struct awb_ctrl_cxt *cxt, void *param);
/*------------------------------------------------------------------------------*
*					local variable				*
*-------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------*
*					local functions				*
*-------------------------------------------------------------------------------*/

static void fill_lsc_outer(uint16_t* buffer, int w, int h)
{
	int i, j;

	for (j=1; j<h-1; j++)
	{
		// left
		for (i=0; i<4; i++)
		{
			uint16_t a = buffer[j * w * 4 + i + 4];
			uint16_t b = buffer[j * w * 4 + i + 8];
			uint16_t c = buffer[j * w * 4 + i + 12];

			int16_t d = 3 * a - 3 * b + c;
			if (d < 1024) 
			{
				d = a;
			}
			else if (d > 16383) 
			{
				d = 16383;
			}

			buffer[j * w * 4 + i] = d;
		}

		// right
		for (i=4*w-4; i<4*w; i++)
		{
			uint16_t a = buffer[j * w * 4 + i - 4];
			uint16_t b = buffer[j * w * 4 + i - 8];
			uint16_t c = buffer[j * w * 4 + i - 12];

			int16_t d = 3 * a - 3 * b + c;
			if (d < 1024) 
			{
				d = a;
			}
			else if (d > 16383) 
			{
				d = 16383;
			}

			buffer[j * w * 4 + i] = d;
		}
	}

	// top line
	for (i=0; i<4*w; i++)
	{
		uint16_t a = buffer[1 * w * 4 + i];
		uint16_t b = buffer[2 * w * 4 + i];
		uint16_t c = buffer[3 * w * 4 + i];

		int16_t d = 3 * a - 3 * b + c;
		if (d < 1024) 
		{
			d = a;
		}
		else if (d > 16383) 
		{
			d = 16383;
		}

		buffer[i] = d;
	}

	// bottom line
	for (i=0; i<4*w; i++)
	{
		uint16_t a = buffer[(h-2) * w * 4 + i];
		uint16_t b = buffer[(h-3) * w * 4 + i];
		uint16_t c = buffer[(h-4) * w * 4 + i];

		int16_t d = 3 * a - 3 * b + c;
		if (d < 1024) 
		{
			d = a;
		}
		else if (d > 16383) 
		{
			d = 16383;
		}

		buffer[(h-1) * w * 4 + i] = d;
	}
}


static uint32_t _check_handle(awb_ctrl_handle_t handle)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	struct awb_ctrl_cxt *cxt = (struct awb_ctrl_cxt *)handle;

	if (NULL == cxt) {
		AWB_CTRL_LOGE("invalid cxt pointer");
		return AWB_CTRL_ERROR;
	}

	if (AWB_CTRL_MAGIC_BEGIN != cxt->magic_begin
		|| AWB_CTRL_MAGIC_END != cxt->magic_end) {
		AWB_CTRL_LOGE("invalid magic begin = 0x%x, magic end = 0x%x",
					cxt->magic_begin, cxt->magic_end);
		return AWB_CTRL_ERROR;
	}

	return rtn;
}

#if 1
/*===========================================================================*/
UI_16	 ais_cmd_set_awb_presetwb(
		 TT_AlAisInterface*  awb_al_ins	 ,
		 UI_08				 puiIdx  )
/*------------------------------------------------------------------------*//**

@return		 void
@note

*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/
	 TT_AlUtilCmd				 attCmd;
	 TT_AlAisCmdSetColorMode2*	 aptCmd = (TT_AlAisCmdSetColorMode2*)&attCmd;
	 aptCmd->muiCmdId			 = ALAIS_SET_COLOR_MODE2;
	 aptCmd->muiMd				 = ALAIS_COLOR_MODE2_MD_PRESET;
	 aptCmd->muiAct 			 = puiIdx;

	 return AlAisIfSendCommand(awb_al_ins  ,&attCmd	 );
}/*--------------------- End of function -----------------------*/

/*===========================================================================*/
UI_16	 ais_cmd_set_awb_scenemode(
		 TT_AlAisInterface*  awb_al_ins	 ,
		 UI_08				 puiIdx  )
/*------------------------------------------------------------------------*//**

@return		 void
@note
*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/
	 TT_AlUtilCmd				 attCmd;
	 TT_AlAisCmdSetColorMode2*	 aptCmd = (TT_AlAisCmdSetColorMode2*)&attCmd;
	 aptCmd->muiCmdId			 = ALAIS_SET_COLOR_MODE2;
	 aptCmd->muiMd				 = ALAIS_COLOR_MODE2_MD_AUTO;
	 aptCmd->muiAct 			 = puiIdx;
	 return AlAisIfSendCommand(awb_al_ins  ,&attCmd	 );
}/*--------------------- End of function -----------------------*/

/*===========================================================================*/
UI_16	 ais_cmd_set_awb_LockMode(
		 TT_AlAisInterface*  awb_al_ins	 ,
		 UI_08				 puiMode	 )		 //0:unlock,1;lock
/*------------------------------------------------------------------------*//**

@return		 void
@note
*//*-------------------------------------------------------------------------*/
{/*--------------------- Start of function ---------------------*/
	 TT_AlUtilCmd				 attCmd;
	 TT_AlAisCmdSetColorLock* aptCmd = (TT_AlAisCmdSetColorLock*)&attCmd;
	 aptCmd->muiCmdId			 = ALAIS_SET_COLOR_LOCK;
	 aptCmd->muiAwblc			 = puiMode;
	 return AlAisIfSendCommand(awb_al_ins  ,&attCmd	 );
}/*--------------------- End of function -----------------------*/
#endif


static uint32_t _deinit(struct awb_ctrl_cxt *cxt)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;

	if (AWB_CTRL_TRUE != cxt->init) {
		AWB_CTRL_LOGE("AWB do not init!");
		return AWB_CTRL_ERROR;
	}
//	AlAwbInterfaceDestroy(&cxt->mttInferface);
	/*clear buffer*/
	memset(cxt, 0, sizeof(*cxt));

	return rtn;
}/*--------------------- End of function -----------------------*/


static void set_result_wbg(struct awb_ctrl_cxt *cxt, float* wbg,SI_32 ct)
{
	cxt->output_gain.r = (uint32_t)(wbg[0] *1024);
	cxt->output_gain.g = (uint32_t)(wbg[1] *1024);
	cxt->output_gain.b = (uint32_t)(wbg[2] *1024);
	cxt->output_ct = ct;
}


static void set_result_ccm(int16_t* pOut, SQ_32* psqccm)
{
	int asiLp;

//CCM outputs
#ifdef CCM_DEBUG_FREQ_CHAMGE
//for Test
	static int i = 0;
	const float ccm_dmy[2][9] ={
	{
		0.0,0.0,1.0,
		0.0,1.0,0.0,
		1.0,0.0,0.0,
	},{
		1.0,0.0,0.0,
		0.0,1.0,0.0,
		0.0,0.0,1.0,
	},
	};
	const float* pccm = ccm_dmy[(i>>1)&0x1];
	AWB_CTRL_LOGE("TEST_CCM:%d",(i>>1)&0x1);
	i++;
#else
	const float* pccm = psqccm;
#endif
	for(asiLp=0;asiLp<9;asiLp++)
	{
		pOut[asiLp] = 
		 ((int16_t)(pccm[asiLp] * 4096.0) >> 2) & 0x3fff;	//ccm coversion :from Shan@SPRD
	}
	//result->use_ccm = 1;
}

static void set_result_lsc(void* p, uint16_t*	apiTbl,uint16_t tableSize)
{
	struct awb_ctrl_init_result * result = (struct awb_ctrl_init_result *)p;
#if 0 //def LSC_DEBUG_TABLE
	//Just Test
	int asiLp;
	uint16_t w,h; 

	static uint16_t dmy_lsc[LSC_SIZE];

	
	{//rear
		w = 20; h=16;		
	}
	result->use_lsc = 1;
	result->lsc_size = w*h*4;

	//ALL
	for(asiLp=0;asiLp<result->lsc_size;asiLp +=4)
	{								//Main  |  front
		dmy_lsc[asiLp+0] = 0x1 << 10;	// R    |  B
		dmy_lsc[asiLp+1] = 0x1 << 10;	// Gr   |  Gb
		dmy_lsc[asiLp+2] = 0x1 << 10;	// Gx   |  Gr
		dmy_lsc[asiLp+3] = 0x1 << 10;	// B    |  R
	}
#if 0
	asiLp=0;
	int asiLp2;
	uint16_t* pt;

	//LEFT side
	for(asiLp2=0;asiLp2<h;asiLp2++)
	{
		pt = &dmy_lsc[asiLp2*(w*4)];
		for(asiLp=0;asiLp<(w*4/2);asiLp +=4)
		{
			pt [asiLp+0] = 0x1 << 10;	//
			pt [asiLp+1] = 0x1 << 10;	//
			pt [asiLp+2] = 0x1 << 10;	//
			pt [asiLp+3] = 0x1 << 10;	//
		}
	}
#endif
	result->lsc = dmy_lsc;
	result->lsc_size = result->lsc_size*4;
	AWB_CTRL_LOGE("GET LSC TABLE size %d  %d,%d ",w*h*4,w,h);
#else
	result->lsc = apiTbl;
	result->lsc_size = tableSize*4;
	
	AWB_CTRL_LOGE("[LSC] OUT table=%p , size=%d",result->lsc,result->lsc_size  );

#endif
}

static uint32_t _init(struct awb_ctrl_cxt *cxt, struct awb_ctrl_init_param *param,
					struct awb_ctrl_init_result *result)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	uint32_t base_gain = param->base_gain;
	
	memset(cxt, 0, sizeof(*cxt));

	cxt->mptAis	= param->priv_handle;
	TT_AlAisInterface* aptAis =cxt->mptAis;
	
	
//Set Init Value
	//WBG outputs
	set_result_wbg(cxt, aptAis->mflWbg,aptAis->msiCtemp);
	result->gain.r = cxt->output_gain.r;
	result->gain.g = cxt->output_gain.g;
	result->gain.b = cxt->output_gain.b;
	result->ct = cxt->output_ct;

	//CCM outputs
	set_result_ccm(result->ccm,aptAis->mflCcMatrix);
	result->use_ccm = 1;
	//LSC outputs
	set_result_lsc(result,aptAis->mttLscOut.mpiTbl,aptAis->mttLscOut.muiSize);
	result->use_lsc = 1;
	//this for sprd manage ctrl
	cxt->init = AWB_CTRL_TRUE;
	cxt->magic_begin = AWB_CTRL_MAGIC_BEGIN;
	cxt->magic_end = AWB_CTRL_MAGIC_END;
	rtn = AWB_CTRL_SUCCESS;
	cxt->init_param = *param;

	return rtn;
}

static uint32_t _calc(struct awb_ctrl_cxt *cxt, struct awb_ctrl_calc_param *param,
					struct awb_ctrl_calc_result *result)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	void *alg_handle = NULL;
	uint32_t mawb_id = cxt->wb_mode;
	uint32_t scene_mode = 0;
	uint32_t work_mode = cxt->work_mode;
	struct awb_ctrl_weight bv_result = {{0}, {0}};
	struct awb_ctrl_bv bv_param = {0};

	TT_AlAisInterface* aptAis =cxt->mptAis;

	if (AWB_CTRL_TRUE != cxt->init) {
		AWB_CTRL_LOGE("AWB do not init!");
		return AWB_CTRL_ERROR;
	}

//	AWB_CTRL_LOGE("camera_id = %d, work_mode = %d, flash_mode = %d, flash_effect = %d, flash_ratio=[%d, %d, %d]", cxt->camera_id, cxt->work_mode, cxt->flash_info.flash_mode, cxt->flash_info.effect, cxt->flash_info.flash_ratio.r, cxt->flash_info.flash_ratio.g, cxt->flash_info.flash_ratio.b);

	//------------------------------------
	//for UI Setting
	uint32_t lock_mode = 0;
	
	lock_mode = cxt->lock_info.lock_num;
	scene_mode = cxt->scene_mode;
	mawb_id = cxt->wb_mode;

	//scene mode prioriry is high more then MWB
	ais_cmd_set_awb_scenemode(aptAis,scene_mode);
	if((AWB_CTRL_SCENEMODE_AUTO == scene_mode) && (AWB_CTRL_WB_MODE_AUTO != mawb_id))
	{
		ais_cmd_set_awb_presetwb(aptAis,mawb_id);
	}
	ais_cmd_set_awb_LockMode(aptAis,lock_mode);

	//WBG outputs
	set_result_wbg(cxt, aptAis->mflWbg,aptAis->msiCtemp);
	//CCM outputs
	set_result_ccm(result->ccm,aptAis->mflCcMatrix);

	if( cxt->work_mode == 1) 
	{// TODO : support LED outputs
		set_result_wbg(cxt, aptAis->mflWbg,aptAis->msiCtemp);
		set_result_ccm(result->ccm,aptAis->mflCcMatrix);
	}
	result->gain.r = cxt->output_gain.r;
	result->gain.g = cxt->output_gain.g;
	result->gain.b = cxt->output_gain.b;
	result->ct = cxt->output_ct;
	
	result->use_ccm = 1;
	//LSC outputs
	set_result_lsc(result,aptAis->mttLscOut.mpiTbl,aptAis->mttLscOut.muiSize);
	result->use_lsc = 1;
	//Log outputs
#if 0
//not support yet.
	result->log_awb.log = aptIns->mttOut.mpiLog;
	result->log_awb.size = aptIns->mttOut.muiLogSize;

	result->log_lsc.log = aptIns->mttOutLsc.mpiLog;
	result->log_lsc.size = aptIns->mttOutLsc.muiSize;
#else
	result->log_awb.size = 0;
	result->log_lsc.size = 0;
#endif

	return rtn;
}

uint32_t _awb_set_wbmode(struct awb_ctrl_cxt *cxt, void *in_param)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	uint32_t awb_mode = *(uint32_t*)in_param;

	cxt->wb_mode = awb_mode;
	AWB_CTRL_LOGE("fangbing debug wbmode changed!");
	return rtn;
}

uint32_t _awb_set_workmode(struct awb_ctrl_cxt *cxt, void *in_param)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	uint32_t work_mode = *(uint32_t*)in_param;

	cxt->work_mode = work_mode;
	
	return rtn;
}

uint32_t _awb_set_flashratio(struct awb_ctrl_cxt *cxt, void *in_param)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	uint32_t flash_ratio = *(uint32_t*)in_param;

	cxt->flash_info.effect = flash_ratio;

	return rtn;
}

uint32_t _awb_set_scenemode(struct awb_ctrl_cxt *cxt, void *in_param)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	uint32_t scene_mode = *(uint32_t*)in_param;

	cxt->scene_mode = scene_mode;
	return rtn;
}

uint32_t _awb_set_recgain(struct awb_ctrl_cxt *cxt, void *param)
{
	UNUSED(param);
	uint32_t rtn = AWB_CTRL_SUCCESS;
	struct awb_gain awb_gain = {0x0};

	rtn = _awb_get_gain(cxt, (void *)&awb_gain);

	cxt->recover_gain.r = awb_gain.r;
	cxt->recover_gain.g = awb_gain.g;
	cxt->recover_gain.b = awb_gain.b;

	cxt->recover_mode = cxt->wb_mode;
	cxt->recover_ct = cxt->cur_ct;
	
	//cxt->flash_info.flash_mode = AWB_CTRL_FLASH_PRE;
	AWB_CTRL_LOGE("pre flashing mode = %d", cxt->flash_info.flash_mode);

	AWB_CTRL_LOGE("awb flash recover gain = (%d, %d, %d), recover mode = %d", cxt->recover_gain.r, cxt->recover_gain.g, cxt->recover_gain.b, cxt->recover_mode);
	
	return rtn;
	
}

uint32_t _awb_get_gain(struct awb_ctrl_cxt *cxt, void *param)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	uint32_t mawb_id = cxt->wb_mode;
	struct awb_gain *awb_result = (struct awb_gain*)param;

	awb_result->r = cxt->output_gain.r;
	awb_result->g = cxt->output_gain.g;
	awb_result->b = cxt->output_gain.b;

// TODO: Not support LED yet.
#if 0
	if( cxt->work_mode == 1) 
	{
#if 1
		awb_result->r = (uint32_t)((uint32_t)cxt->mttInferface.mttOutLed.muiWbg[0] >> 6);
		awb_result->g = (uint32_t)((uint32_t)cxt->mttInferface.mttOutLed.muiWbg[1] >> 6);
		awb_result->b = (uint32_t)((uint32_t)cxt->mttInferface.mttOutLed.muiWbg[2] >> 6);
#else
		awb_result->r = 1 <<10;
		awb_result->g = 3 <<10;
		awb_result->b = 1 <<10;
#endif
	}
#endif
	AWB_CTRL_LOGE("_awb_get_gain = (%d,%d,%d)",awb_result->r,awb_result->g,awb_result->b);
	return rtn;
	
}

uint32_t _awb_get_stat_size(struct awb_ctrl_cxt *cxt, void *param)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	struct awb_size *stat_size = (struct awb_size*)param;

	stat_size->w = cxt->init_param.stat_img_size.w;
	stat_size->h = cxt->init_param.stat_img_size.h;
	
	AWB_CTRL_LOGE("_awb_get_stat_size = (%d,%d)",stat_size->w, stat_size->h);

	return rtn;	
}

uint32_t _awb_get_winsize(struct awb_ctrl_cxt *cxt, void *param)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	uint32_t workmode = cxt->work_mode;
	struct awb_size *win_size = (struct awb_size*)param;

	win_size->w = cxt->tuning_param[workmode].stat_win_size.w;
	win_size->h = cxt->tuning_param[workmode].stat_win_size.h;
	
	AWB_CTRL_LOGE("_awb_get_winsize = (%d,%d), work_mode=%d",win_size->w, win_size->h, workmode);

	return rtn;	
}

uint32_t _awb_get_ct(struct awb_ctrl_cxt *cxt, void *param)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	uint32_t *ct = (uint32_t *)param;

	*ct = cxt->output_ct;

	AWB_CTRL_LOGE("_awb_get_ct = %d", cxt->output_ct);

	return rtn;
}

uint32_t _awb_get_recgain(struct awb_ctrl_cxt *cxt, void *param)
{
	UNUSED(param);
	uint32_t rtn = AWB_CTRL_SUCCESS;
	struct awb_ctrl_gain awb_gain = {0x0};

	awb_gain.r = cxt->recover_gain.r;
	awb_gain.g = cxt->recover_gain.g;
	awb_gain.b = cxt->recover_gain.b;

	
	cxt->cur_gain.r = awb_gain.r;
	cxt->cur_gain.g = awb_gain.g;
	cxt->cur_gain.b = awb_gain.b;
	cxt->cur_ct = cxt->recover_ct;
	cxt->wb_mode = cxt->recover_mode;

	//cxt->flash_info.flash_mode = AWB_CTRL_FLASH_END;

	AWB_CTRL_LOGE("after flashing mode = %d", cxt->flash_info.flash_mode);

	AWB_CTRL_LOGE("awb flash end  gain = (%d, %d, %d), recover mode = %d", cxt->cur_gain.r, cxt->cur_gain.g, cxt->cur_gain.b, cxt->wb_mode);
	
	return rtn;
	
}

uint32_t _awb_set_flash_gain(struct awb_ctrl_cxt *cxt, void *param)
{
	UNUSED(param);
	uint32_t rtn = AWB_CTRL_SUCCESS;
	struct awb_flash_info*flash_info = (struct awb_flash_info*)param;

	//cxt->flash_info.flash_mode = AWB_CTRL_FLASH_MAIN;
	
	cxt->flash_info.patten = 0;
	cxt->flash_info.effect = flash_info->effect;
	cxt->flash_info.flash_ratio.r = flash_info->flash_ratio.r;
	cxt->flash_info.flash_ratio.g = flash_info->flash_ratio.g;
	cxt->flash_info.flash_ratio.b = flash_info->flash_ratio.b;
	
	AWB_CTRL_LOGE("flashing mode = %d", cxt->flash_info.flash_mode);

	return rtn;
	
}

uint32_t _awb_set_lock(struct awb_ctrl_cxt *cxt, void *param)
{
	UNUSED(param);
	uint32_t rtn = AWB_CTRL_SUCCESS;

	cxt->lock_info.lock_num += 1;

	AWB_CTRL_LOGE("AWB_TEST _awb_set_lock0: luck=%d, mode=%d", cxt->lock_info.lock_num, cxt->lock_info.lock_mode);

	if (0 != cxt->lock_info.lock_num) {

		cxt->lock_info.lock_mode = AWB_CTRL_LOCKMODE;

		cxt->lock_info.lock_gain.r = cxt->output_gain.r;
		cxt->lock_info.lock_gain.g = cxt->output_gain.g;
		cxt->lock_info.lock_gain.b = cxt->output_gain.b;

		cxt->lock_info.lock_ct = cxt->output_ct;
	}
	AWB_CTRL_LOGE("AWB_TEST _awb_set_lock1: luck=%d, mode:%d", cxt->lock_info.lock_num, cxt->lock_info.lock_mode);
	return rtn;

}

uint32_t _awb_get_unlock(struct awb_ctrl_cxt *cxt, void *param)
{
	UNUSED(param);
	uint32_t rtn = AWB_CTRL_SUCCESS;
	AWB_CTRL_LOGE("AWB_TEST _awb_get_unlock0: lock_num=%d, mode:=%d", cxt->lock_info.lock_num, cxt->lock_info.lock_mode);
	if (0 != cxt->lock_info.lock_num) {
		cxt->lock_info.lock_num -= 1;
	}

	if(0 == cxt->lock_info.lock_num) {
		cxt->lock_info.lock_mode = AWB_CTRL_UNLOCKMODE;
	}

	AWB_CTRL_LOGE("AWB_TEST _awb_get_unlock1: lock_num=%d, mode:=%d", cxt->lock_info.lock_num, cxt->lock_info.lock_mode);

	return rtn;
}

/*------------------------------------------------------------------------------*
*					public functions			*
*-------------------------------------------------------------------------------*/
/* awb_ctrl_init--
*@ handle: instance
*@ param: input param
*@ result: output result
*@ return: 
*@           AWB_CTRL_INVALID_HANDLE: failed
*@	     others: awb ctrl handle
*/
awb_ctrl_handle_t awb_al_ctrl_init(struct awb_ctrl_init_param *param,
				struct awb_ctrl_init_result *result)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	struct awb_ctrl_cxt *cxt = NULL;

	if (NULL == param || NULL == result) {
		AWB_CTRL_LOGE("invalid param: param=%p, result=%p", param, result);
		goto ERROR_EXIT;
	}

	cxt = (struct awb_ctrl_cxt *)malloc(sizeof(struct awb_ctrl_cxt));
	if (NULL == cxt) {
		AWB_CTRL_LOGE("malloc awb_ctrl_cxt failed");
		goto ERROR_EXIT;
	}


	rtn = _init(cxt, param, result);
	if (AWB_CTRL_SUCCESS != rtn) {
		AWB_CTRL_LOGE("_init failed");
		goto ERROR_EXIT;
	}

	fill_lsc_outer(result->lsc, cxt->lsc_otp_width, cxt->lsc_otp_height);

	
	cxt->cur_gain.r = result->gain.r;
	cxt->cur_gain.g = result->gain.g;
	cxt->cur_gain.b = result->gain.b;
	cxt->cur_ct = result->ct;

	pthread_mutex_init(&cxt->status_lock, NULL);
	return (awb_ctrl_handle_t)cxt;
	
ERROR_EXIT:	
	AWB_CTRL_SAFE_FREE(cxt);

	return AWB_CTRL_INVALID_HANDLE;
}

/* awb_ctrl_deinit--
*@ handle: instance
*@ param: input param
*@ result: output result
*@ return: 
*@           0: successful
*@	     others: failed
*/
uint32_t awb_al_ctrl_deinit(awb_ctrl_handle_t handle, void *param, void *result)
{
	UNUSED(param);
	UNUSED(result);
	uint32_t rtn = AWB_CTRL_SUCCESS;
	struct awb_ctrl_cxt *cxt = (struct awb_ctrl_cxt *)handle;

	rtn = _check_handle(handle);
	
	if (AWB_CTRL_SUCCESS != rtn) {
		AWB_CTRL_LOGE("_check_cxt failed");
		return AWB_CTRL_ERROR;
	}

	pthread_mutex_destroy(&cxt->status_lock);
	rtn = _deinit(cxt);
	if (AWB_CTRL_SUCCESS != rtn) {
		AWB_CTRL_LOGE("_deinit failed");
		return AWB_CTRL_ERROR;
	}

	AWB_CTRL_SAFE_FREE(cxt);

	return rtn;
}

/* awb_ctrl_calculation--
*@ handle: instance
*@ param: input param
*@ result: output result
*@ return: 
*@           0: successful
*@	     others: failed
*/
uint32_t awb_al_ctrl_calculation(awb_ctrl_handle_t handle, 
				struct awb_ctrl_calc_param *param,
				struct awb_ctrl_calc_result *result)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	struct awb_ctrl_cxt *cxt = (struct awb_ctrl_cxt *)handle;
	struct awb_gain awb_get_gain = {0};
	uint32_t scene_mode = 0;

	if (NULL == param || NULL == result) {
		AWB_CTRL_LOGE("invalid param: param=%p, result=%p", param, result);
		return AWB_CTRL_ERROR;
	}

	rtn = _check_handle(handle);
	if (AWB_CTRL_SUCCESS != rtn) {
		AWB_CTRL_LOGE("_check_cxt failed");
		return AWB_CTRL_ERROR;
	}

	pthread_mutex_lock(&cxt->status_lock);
	if (AWB_CTRL_ENABLE == cxt->init_param.awb_enable) {
		rtn = _calc(cxt, param, result);
		if (AWB_CTRL_SUCCESS != rtn) {
			AWB_CTRL_LOGE("_calc failed");
			rtn = AWB_CTRL_ERROR;
			goto EXIT;
		}

		fill_lsc_outer(result->lsc, cxt->lsc_otp_width, cxt->lsc_otp_height);
	}

EXIT:
	pthread_mutex_unlock(&cxt->status_lock);
	return rtn;
}

/* awb_ctrl_ioctrl--
*@ handle: instance
*@ param: input param
*@ result: output result
*@ return: 
*@           0: successful
*@	     others: failed
*/
uint32_t awb_al_ctrl_ioctrl(awb_ctrl_handle_t handle, enum awb_ctrl_cmd cmd,
				void *param0, void *param1)
{
	UNUSED(param1);
	uint32_t rtn = AWB_CTRL_SUCCESS;
	struct awb_ctrl_cxt *cxt = (struct awb_ctrl_cxt *)handle;

	rtn = _check_handle(handle);
	if (AWB_CTRL_SUCCESS != rtn) {
		AWB_CTRL_LOGE("_check_cxt failed");
		return AWB_CTRL_ERROR;
	}

	pthread_mutex_lock(&cxt->status_lock);
	switch (cmd) {
	case AWB_CTRL_CMD_SET_WB_MODE:
		rtn = _awb_set_wbmode(cxt, param0);
		break;

	case AWB_CTRL_CMD_SET_WORK_MODE:
		rtn = _awb_set_workmode(cxt, param0);
		break;

	case AWB_CTRL_CMD_SET_SCENE_MODE:
		rtn = _awb_set_scenemode(cxt, param0);
		break;

	case AWB_CTRL_CMD_GET_GAIN:
		rtn = _awb_get_gain(cxt, param0);
		break;

	case AWB_CTRL_CMD_FLASHING:
		rtn = _awb_set_flash_gain(cxt, param0);
		break;

	case AWB_CTRL_CMD_FLASH_OPEN_M:
		AWB_CTRL_LOGE("FLASH_TAG AWB_CTRL_CMD_FLASH_OPEN_M");
		cxt->flash_info.flash_mode = AWB_CTRL_FLASH_MAIN;
//		rtn = _awb_set_recgain(cxt, param0);
		break;

	case AWB_CTRL_CMD_FLASH_OPEN_P:
		AWB_CTRL_LOGE("FLASH_TAG AWB_CTRL_CMD_FLASH_OPEN_P");
		cxt->flash_info.flash_mode = AWB_CTRL_FLASH_PRE;
//		rtn = _awb_set_recgain(cxt, param0);
		break;

	case AWB_CTRL_CMD_FLASH_CLOSE:
		cxt->flash_info.flash_mode = AWB_CTRL_FLASH_END;
		rtn = _awb_get_recgain(cxt, param0);
		break;

	case AWB_CTRL_CMD_FLASH_BEFORE_P:
		AWB_CTRL_LOGE("FLASH_TAG: AWB_CTRL_CMD_FLASH_BEFORE_P");
		rtn = _awb_set_recgain(cxt, param0);
		break;

	case AWB_CTRL_CMD_LOCK:
		rtn = _awb_set_lock(cxt, param0);
		break;

	case AWB_CTRL_CMD_UNLOCK:
		rtn = _awb_get_unlock(cxt, param0);
		break;
		
	case AWB_CTRL_CMD_GET_STAT_SIZE:
		rtn = _awb_get_stat_size(cxt,param0);
		break;

	case AWB_CTRL_CMD_GET_WIN_SIZE:
		rtn = _awb_get_winsize(cxt,param0);
		break;

	case AWB_CTRL_CMD_GET_CT:
		rtn = _awb_get_ct(cxt,param0);
		break;

	default:
		AWB_CTRL_LOGE("invalid cmd = 0x%x", cmd);
		rtn = AWB_CTRL_ERROR;
		break;
	}

	pthread_mutex_unlock(&cxt->status_lock);
	return rtn;
}
