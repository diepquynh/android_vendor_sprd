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
#include "lib_ctrl.h"
#include "isp_log.h"
#include "awb_al_ctrl.h"
#include <cutils/properties.h>


#ifndef CONFIG_USE_ALC_AWB
#error invalid make target...
#endif



//#define LIB_DYNAMIC_LOAD
//#define CCM_DEBUG_FREQ_CHAMGE
//#define LSC_DEBUG_TABLE

//#define LSC_SIZE	(1280)	//20*16 *4 color
//#define LSC_SIZE	(24*19*4)	//front is 24*10 *4 color



#include    <dlfcn.h>
#include "AlAwbInterface.h"

struct al_awb_thirdlib_fun al_awb_thirdlib_fun;
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

extern struct al_awb_thirdlib_fun al_awb_thirdlib_fun;
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
	void*   mpvLib;
	TT_AlAwbInterface	mttInferface;
	TT_AlAwbStats	mttStats;//Todo Support SPRD format
	struct tg_dmy_log	dmy_log	;
#ifdef LIB_DYNAMIC_LOAD
//workaround/  dynamic loadding don't used.
	void	(*AlAwbInterfaceInit)(TT_AlAwbInterface*		pptInterface );
	void	(*AlAwbInterfaceDestroy)(TT_AlAwbInterface*		pptInterface );
	void	(*AlAwbInterfaceReset)(	TT_AlAwbInterface*		pptInterface);
	void	(*AlAwbInterfaceMain)(	TT_AlAwbInterface*		pptInterface);
	UI_16	(*AlAwbInterfaceSendCommand)(TT_AlAwbInterface*		pptInterface	,TT_AlAisCmd*			pptCmd			);
	void	(*AlAwbInterfaceShowVersion)(TT_AlAwbInterface*		pptInterface	);
#endif	//#ifdef LIB_DYNAMIC_LOAD
	/*must be the last one*/
	uint32_t magic_end;
};

/*------------------------------------------------------------------------------*
*					local function declaration		*
*-------------------------------------------------------------------------------*/

static uint32_t _awb_get_gain(struct awb_ctrl_cxt *cxt, void *param);
/*------------------------------------------------------------------------------*
*					local variable				*
*-------------------------------------------------------------------------------*/

static const uint16_t lsc_ratio_pow15[20*16*4] =
{
	/*Gr*/
	46138,43825,41834,40537,39506,38630,37898,37348,36970,36776,36784,36951,37294,37807,38478,39254,40257,41511,43723,46241,
	44193,42290,40652,39425,38387,37513,36812,36300,35952,35780,35777,35938,36255,36738,37392,38190,39163,40334,42061,44088,
	42602,41041,39649,38450,37402,36524,35849,35364,35037,34882,34871,35026,35329,35793,36431,37245,38210,39336,40700,42253,
	41685,40214,38877,37646,36591,35730,35084,34614,34293,34142,34133,34288,34591,35045,35658,36452,37440,38570,39835,41240,
	41129,39605,38224,36983,35934,35122,34485,34020,33716,33556,33553,33706,34008,34456,35056,35830,36803,37956,39208,40586,
	40679,39141,37746,36503,35475,34670,34043,33580,33278,33133,33134,33285,33581,34026,34620,35381,36341,37497,38750,40126,
	40383,38833,37429,36191,35182,34382,33760,33306,33011,32878,32874,33012,33304,33746,34341,35095,36035,37195,38460,39847,
	40260,38702,37287,36046,35051,34255,33632,33184,32900,32774,32770,32895,33177,33615,34207,34957,35899,37058,38332,39730,
	40290,38731,37319,36071,35075,34278,33659,33210,32920,32783,32781,32911,33192,33637,34227,34973,35913,37068,38350,39764,
	40491,38938,37530,36264,35254,34452,33832,33376,33081,32928,32930,33066,33350,33801,34394,35143,36093,37248,38533,39954,
	40836,39295,37896,36632,35594,34781,34154,33695,33384,33232,33226,33368,33657,34110,34705,35467,36429,37586,38867,40284,
	41289,39788,38421,37159,36094,35261,34623,34158,33848,33691,33683,33830,34115,34561,35161,35945,36933,38090,39354,40747,
	41955,40452,39093,37861,36779,35912,35256,34779,34459,34296,34291,34440,34727,35174,35792,36594,37585,38727,40026,41476,
	42947,41343,39918,38694,37631,36744,36057,35560,35226,35056,35045,35198,35506,35971,36614,37426,38395,39509,40909,42515,
	44502,42580,40917,39634,38591,37697,36979,36456,36110,35924,35906,36066,36382,36875,37531,38340,39319,40504,42048,43835,
	46304,44001,42011,40627,39600,38699,37947,37392,37033,36826,36799,36970,37292,37816,38488,39289,40297,41611,43324,45297,

	/*R*/
	50921,47788,45149,43454,42334,41203,40217,39484,38942,38694,38722,38969,39489,40279,41247,42308,43473,45226,48273,51881,
	48166,45728,43655,42093,40817,39645,38662,37933,37432,37201,37220,37458,37964,38723,39710,40853,42127,43720,45969,48648,
	45925,44052,42365,40847,39450,38241,37260,36535,36063,35842,35854,36089,36582,37320,38319,39526,40898,42412,44097,45966,
	44614,42915,41310,39703,38252,37035,36079,35386,34935,34718,34723,34961,35425,36138,37102,38329,39772,41333,42873,44489,
	43789,42059,40420,38751,37285,36093,35171,34499,34070,33862,33870,34092,34545,35231,36160,37361,38819,40471,41994,43547,
	43237,41437,39748,38048,36589,35424,34521,33870,33454,33260,33270,33490,33908,34571,35483,36654,38108,39803,41360,42960,
	42878,41028,39304,37593,36150,35001,34112,33486,33086,32908,32918,33117,33523,34162,35061,36205,37639,39340,40965,42672,
	42739,40855,39104,37375,35945,34808,33933,33315,32934,32771,32777,32947,33346,33975,34860,35998,37431,39129,40783,42541,
	42762,40880,39134,37413,35985,34841,33968,33346,32961,32789,32793,32973,33365,33994,34881,36025,37448,39155,40799,42535,
	42990,41140,39414,37695,36252,35098,34214,33582,33182,32989,32993,33186,33595,34234,35130,36279,37721,39414,41040,42752,
	43398,41611,39926,38215,36743,35582,34679,34024,33610,33410,33408,33610,34039,34699,35611,36771,38225,39908,41493,43141,
	44029,42302,40662,38991,37521,36315,35384,34707,34273,34057,34059,34278,34719,35398,36333,37532,38990,40625,42148,43707,
	44932,43207,41590,40003,38541,37308,36341,35639,35181,34962,34958,35195,35649,36360,37322,38545,39981,41537,43080,44705,
	46423,44467,42717,41171,39778,38560,37568,36836,36353,36124,36132,36374,36852,37586,38585,39779,41149,42655,44391,46345,
	48645,46068,43894,42313,41047,39879,38910,38163,37672,37425,37433,37684,38180,38912,39895,41031,42248,43838,46003,48567,
	51201,47878,45105,43442,42343,41241,40313,39551,39054,38786,38789,39051,39566,40294,41240,42303,43306,45064,47803,51087,

	/*B*/
	48549,45876,43604,41992,40900,39850,39006,38381,37901,37718,37724,37926,38314,38952,39753,40641,41744,43297,45997,49214,
	46081,44087,42332,40839,39635,38556,37711,37095,36672,36487,36487,36683,37069,37670,38464,39418,40562,41965,43945,46313,
	44123,42641,41218,39772,38490,37396,36554,35950,35566,35380,35377,35570,35952,36528,37316,38309,39488,40815,42282,43908,
	43026,41646,40270,38768,37467,36391,35582,35003,34625,34445,34442,34639,35008,35571,36341,37315,38513,39868,41188,42546,
	42464,40938,39471,37943,36635,35621,34833,34259,33890,33714,33722,33912,34280,34836,35580,36524,37723,39111,40420,41764,
	41976,40393,38887,37338,36062,35062,34288,33718,33355,33192,33198,33387,33752,34305,35035,35971,37147,38548,39866,41223,
	41653,40036,38508,36951,35694,34714,33941,33387,33032,32890,32898,33067,33419,33965,34699,35623,36784,38192,39526,40908,
	41533,39894,38348,36782,35550,34562,33794,33249,32908,32772,32775,32923,33267,33809,34541,35460,36619,38021,39388,40828,
	41579,39933,38383,36808,35577,34594,33837,33283,32936,32786,32790,32945,33288,33830,34565,35482,36638,38051,39410,40831,
	41794,40169,38632,37053,35789,34802,34043,33492,33137,32967,32969,33140,33487,34030,34765,35686,36844,38253,39611,41033,
	42133,40579,39076,37497,36210,35200,34434,33872,33508,33334,33332,33509,33858,34414,35137,36066,37251,38657,40002,41400,
	42481,41180,39722,38153,36839,35801,35015,34450,34081,33897,33894,34073,34429,34964,35704,36653,37859,39249,40560,41904,
	43180,41965,40533,39012,37676,36610,35802,35219,34835,34652,34649,34830,35186,35735,36489,37465,38656,40015,41360,42666,
	44572,43066,41529,40037,38740,37651,36801,36195,35803,35605,35597,35787,36158,36728,37514,38504,39651,40956,42499,44236,
	46645,44452,42559,41034,39837,38733,37870,37246,36835,36630,36606,36807,37199,37768,38570,39522,40648,42076,43815,45817,
	48891,46020,43611,42013,40954,39844,38982,38337,37900,37692,37646,37859,38278,38838,39645,40530,41646,43314,45247,46884,

	/*Gb*/
	45752,43558,41679,40438,39484,38647,37952,37422,37059,36890,36898,37055,37395,37914,38589,39347,40311,41565,43741,46235,
	43825,42025,40479,39315,38350,37518,36848,36348,36008,35846,35845,35999,36321,36812,37463,38248,39200,40362,42045,44022,
	42222,40757,39453,38329,37347,36518,35866,35389,35066,34907,34902,35052,35363,35832,36467,37274,38230,39333,40647,42136,
	41242,39888,38656,37517,36524,35715,35090,34627,34315,34159,34153,34304,34608,35060,35674,36461,37438,38537,39757,41110,
	40614,39252,38010,36866,35879,35102,34483,34025,33722,33566,33567,33713,34020,34465,35067,35832,36783,37899,39108,40437,
	40180,38795,37537,36395,35425,34650,34041,33587,33283,33137,33135,33290,33581,34028,34623,35380,36310,37442,38628,39919,
	39868,38489,37234,36085,35131,34370,33758,33300,33007,32881,32878,33016,33303,33747,34342,35084,36012,37133,38335,39650,
	39750,38357,37092,35943,35005,34238,33629,33182,32898,32773,32772,32898,33175,33615,34204,34947,35869,36987,38203,39538,
	39775,38386,37121,35966,35028,34259,33653,33206,32922,32782,32782,32912,33194,33636,34225,34965,35880,36998,38223,39572,
	39970,38582,37320,36155,35199,34433,33823,33374,33084,32934,32930,33065,33354,33798,34391,35131,36060,37187,38403,39738,
	40308,38935,37681,36510,35529,34759,34148,33689,33390,33241,33235,33371,33662,34112,34706,35459,36404,37528,38745,40083,
	40832,39463,38211,37034,36029,35235,34617,34163,33856,33702,33695,33840,34129,34575,35170,35942,36906,38034,39251,40585,
	41527,40146,38890,37732,36717,35890,35254,34784,34475,34315,34308,34454,34747,35197,35816,36611,37580,38688,39954,41370,
	42577,41077,39744,38589,37579,36737,36070,35585,35253,35088,35081,35237,35544,36011,36654,37461,38416,39506,40891,42488,
	44238,42374,40768,39544,38561,37699,37021,36494,36163,35981,35971,36137,36463,36951,37601,38405,39371,40542,42071,43847,
	46129,43852,41883,40550,39598,38709,38020,37440,37117,36909,36898,37072,37420,37935,38594,39394,40385,41692,43377,45172,
};

/*------------------------------------------------------------------------------*
*					local functions				*
*-------------------------------------------------------------------------------*/
static void split_lsc_otp_random(uint16_t* buffer)
{
	uint16_t tmp[20*16*4];
	int i;

	memcpy((void*)tmp, (void*)buffer, 20*16*4*2);

	uint16_t* pbuf = buffer;
	for (i=0; i<20*16; i++)
	{
		*pbuf++ = tmp[4*i+0];
	}
	for (i=0; i<20*16; i++)
	{
		*pbuf++ = tmp[4*i+1];
	}
	for (i=0; i<20*16; i++)
	{
		*pbuf++ = tmp[4*i+2];
	}
	for (i=0; i<20*16; i++)
	{
		*pbuf++ = tmp[4*i+3];
	}

}

static void correct_lsc_otp_random_16(uint16_t* buffer, uint16_t* ratio)
{
	int i;

	uint16_t* pbuf_a = buffer;
	uint16_t* pbuf_b = ratio;
	for (i=0; i<20*16*4; i++)
	{
		uint16_t a = *pbuf_a;
		uint16_t b = *pbuf_b ++;
		uint32_t m = (a * b) + (1 << 14);
		*pbuf_a ++ = (uint16_t)(m >> 15);
	}
}

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

/*===========================================================================*/
UI_16	 ais_cmd_set_awb_presetwb(
		 TT_AlAwbInterface*  awb_al_ins	 ,
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

	 return al_awb_thirdlib_fun.AlAwbInterfaceSendCommand(awb_al_ins  ,&attCmd	 );
}/*--------------------- End of function -----------------------*/

/*===========================================================================*/
UI_16	 ais_cmd_set_awb_scenemode(
		 TT_AlAwbInterface*  awb_al_ins	 ,
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
	 return al_awb_thirdlib_fun.AlAwbInterfaceSendCommand(awb_al_ins  ,&attCmd	 );
}/*--------------------- End of function -----------------------*/

/*===========================================================================*/
UI_16	 ais_cmd_set_awb_LockMode(
		 TT_AlAwbInterface*  awb_al_ins	 ,
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
	 return al_awb_thirdlib_fun.AlAwbInterfaceSendCommand(awb_al_ins  ,&attCmd	 );
}/*--------------------- End of function -----------------------*/

static uint32_t _deinit(struct awb_ctrl_cxt *cxt)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;

	if (AWB_CTRL_TRUE != cxt->init) {
		AWB_CTRL_LOGE("AWB do not init!");
		return AWB_CTRL_ERROR;
	}


	al_awb_thirdlib_fun.AlAwbInterfaceDestroy(&cxt->mttInferface);
	/*clear buffer*/
	memset(cxt, 0, sizeof(*cxt));

	return rtn;
}

static void set_result_ccm(TT_AlAwbInterface*	aptIns,void *p)
{
	struct awb_ctrl_init_result * result = (struct awb_ctrl_init_result *)p;
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
	const float* pccm = aptIns->mttOut.mflCcMatrix;
#endif
	for(asiLp=0;asiLp<9;asiLp++)
	{
		result->ccm[asiLp] =
		 ((int16_t)(pccm[asiLp] * 4096.0) >> 2) & 0x3fff;	//ccm coversion :from Shan@SPRD
	}
	result->use_ccm = 1;
}

static void set_result_lsc(TT_AlAwbInterface*	aptIns,void *p)
{
	struct awb_ctrl_init_result * result = (struct awb_ctrl_init_result *)p;
#ifdef LSC_DEBUG_TABLE
	//Just Test
	int asiLp;
	uint16_t w,h;

	static uint16_t dmy_lsc[25*20*4];

	result->use_lsc = 1;

	if( aptIns->muiModuleNo == 1 )
	{//front
//		w = 24; h=19;
		w = 25; h=20;
	}
	else
	{//rear
//		w = 20; h=16;
		w = 25; h=20;
	}
	result->lsc_size = w*h*4;

	//ALL
	for(asiLp=0;asiLp<result->lsc_size;asiLp +=4)
	{								//Main  |  front
		dmy_lsc[asiLp+0] = 0x1 << 10;	// R    |  B
		dmy_lsc[asiLp+1] = 0x1 << 10;	// Gr   |  Gb
		dmy_lsc[asiLp+2] = 0x1 << 10;	// Gx   |  Gr
		dmy_lsc[asiLp+3] = 0x1 << 10;	// B    |  R
	}
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
	result->lsc = dmy_lsc;

	result->lsc_size = aptIns->mttOutLsc.muiSize*4;
	AWB_CTRL_LOGE("GET LSC TABLE size %d  %d,%d ",w*h*4,w,h);

#else

	if( aptIns->mttOutLsc.mpiTbl != 0 )
	{
	#if 0
		result->use_lsc = 0;
		result->lsc = aptIns->mttOutLsc.mpiTbl;
		result->lsc_size = aptIns->mttOutLsc.muiSize*4;
	#else
		static uint16_t dmy_lsc[25*20*4];

		result->use_lsc = 1;
		result->lsc = aptIns->mttOutLsc.mpiTbl;
		result->lsc_size = aptIns->mttOutLsc.muiSize*4;
		AWB_CTRL_LOGE("GET LSC TABLE ADDR 0x%08x",result->lsc);

		memcpy( dmy_lsc, result->lsc, sizeof(dmy_lsc) );
		result->lsc = dmy_lsc;
	#endif
	}
	else
	{
		result->use_lsc = 0;
	}
	AWB_CTRL_LOGE("GET LSC TABLE size %d",result->lsc_size);
#endif
}

static uint32_t _init(struct awb_ctrl_cxt *cxt, struct awb_ctrl_init_param *param,
					struct awb_ctrl_init_result *result)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	uint32_t base_gain = param->base_gain;

	memset(cxt, 0, sizeof(*cxt));

	//temp code
	cxt->work_mode = 0;
	cxt->camera_id = param->camera_id;
	cxt->lsc_otp_random = param->lsc_otp_random; // color order: GrRBGb, GrRBGb, ..., GrRBGb, GrRBGb, GrRBGb
	if (cxt->lsc_otp_random != NULL)
	{
		split_lsc_otp_random(cxt->lsc_otp_random); // now color order: GrGrGr...GrGr, RRR...RR, BBB...BB, GbGbGb...GbGb
		correct_lsc_otp_random_16(cxt->lsc_otp_random, lsc_ratio_pow15);
	}
	cxt->lsc_otp_golden = param->lsc_otp_golden;
	cxt->lsc_otp_width = param->lsc_otp_width;
	cxt->lsc_otp_height = param->lsc_otp_height;
	cxt->tuning_param[cxt->work_mode].stat_win_size = param->stat_win_size;
	AWB_CTRL_LOGE("[otp] r=%p g=%p  w=%d h=%d ",param->lsc_otp_random,param->lsc_otp_golden,param->lsc_otp_width,param->lsc_otp_height);

	TT_AlAwbInterface*	aptIns = &cxt->mttInferface;
	aptIns->muiModuleNo = param->camera_id;	//0:Back/Rear camera ,1:Front camera
	aptIns->muiStructSize = sizeof(TT_AlAwbInterface);

//---
	if(( param->otp_info.gldn_stat_info.g != 0 ) && ( param->otp_info.rdm_stat_info.g != 0 ) )
	{
		aptIns->mttIn.mttAwbLineData.muiMode = 1 | 0x80000000; //On(gldn provide by system)
#ifndef AWB_OTP_NEW
		aptIns->mttIn.mttAwbLineData.mttLo.mflBg = (float)param->otp_info.rdm_stat_info.b / (float)param->otp_info.rdm_stat_info.g;
		aptIns->mttIn.mttAwbLineData.mttLo.mflRg = (float)param->otp_info.rdm_stat_info.r / (float)param->otp_info.rdm_stat_info.g;
		aptIns->mttIn.mttAwbLineData.mttHi.mflBg = (float)param->otp_info.rdm_stat_info.b / (float)param->otp_info.rdm_stat_info.g;
		aptIns->mttIn.mttAwbLineData.mttHi.mflRg = (float)param->otp_info.rdm_stat_info.r / (float)param->otp_info.rdm_stat_info.g;
	//---
		aptIns->mttIn.mttAwbLineData.mttRefLo.mflBg = (float)param->otp_info.gldn_stat_info.b / (float)param->otp_info.gldn_stat_info.g;
		aptIns->mttIn.mttAwbLineData.mttRefLo.mflRg = (float)param->otp_info.gldn_stat_info.r / (float)param->otp_info.gldn_stat_info.g;
		aptIns->mttIn.mttAwbLineData.mttRefHi.mflBg = (float)param->otp_info.gldn_stat_info.b / (float)param->otp_info.gldn_stat_info.g;
		aptIns->mttIn.mttAwbLineData.mttRefHi.mflRg = (float)param->otp_info.gldn_stat_info.r / (float)param->otp_info.gldn_stat_info.g;
#else
		//Do not substract OB here!!
		//Random
		aptIns->mttIn.mttAwbLineData.mttLo.muiR = param->otp_info.rdm_stat_info.r;
		aptIns->mttIn.mttAwbLineData.mttLo.muiG = param->otp_info.rdm_stat_info.g;
		aptIns->mttIn.mttAwbLineData.mttLo.muiB = param->otp_info.rdm_stat_info.b;
		aptIns->mttIn.mttAwbLineData.mttHi.muiR = param->otp_info.rdm_stat_info.r;
		aptIns->mttIn.mttAwbLineData.mttHi.muiG = param->otp_info.rdm_stat_info.g;
		aptIns->mttIn.mttAwbLineData.mttHi.muiB = param->otp_info.rdm_stat_info.b;

		//Golden
		aptIns->mttIn.mttAwbLineData.mttRefLo.muiR = param->otp_info.gldn_stat_info.r;
		aptIns->mttIn.mttAwbLineData.mttRefLo.muiG = param->otp_info.gldn_stat_info.g;
		aptIns->mttIn.mttAwbLineData.mttRefLo.muiB = param->otp_info.gldn_stat_info.b;
		aptIns->mttIn.mttAwbLineData.mttRefHi.muiR = param->otp_info.gldn_stat_info.r;
		aptIns->mttIn.mttAwbLineData.mttRefHi.muiG = param->otp_info.gldn_stat_info.g;
		aptIns->mttIn.mttAwbLineData.mttRefHi.muiB = param->otp_info.gldn_stat_info.b;
#endif
	}
	else
	{//invalid data -> force OFF
		aptIns->mttIn.mttAwbLineData.muiMode = 0;	//off
	}


	aptIns->mttIn.mttLscLineData.mpiOtp 		= param->lsc_otp_random;
	aptIns->mttIn.mttLscLineData.mpiOtpRef	= param->lsc_otp_golden;
	aptIns->mttIn.mttLscLineData.muiWidth		= param->lsc_otp_width;
	aptIns->mttIn.mttLscLineData.muiHight		= param->lsc_otp_height;

	//if( cxt->camera_id == 0 )
	AWB_CTRL_LOGE("lsc random = %p, golden = %p, %d x %d", cxt->lsc_otp_random, cxt->lsc_otp_golden, cxt->lsc_otp_width, cxt->lsc_otp_height);
//	AWB_CTRL_LOGD("[otp]tar,ref :%p, %p ,log: e h %d,%d, ",aptIns->mttIn.mttLscLineData.mpiOtp,aptIns->mttIn.mttLscLineData.mpiOtpRef,aptIns->mttIn.mttLscLineData.muiWidth,aptIns->mttIn.mttLscLineData.muiHight);
	if(( param->lsc_otp_random != 0 ) && ( param->lsc_otp_golden != 0 ) &&
		( param->lsc_otp_height != 0 ) && ( param->lsc_otp_width != 0 ))
	{
		aptIns->mttIn.mttLscLineData.muiMode		= 1;
	}
	else
	{//disable lsc otp
		aptIns->mttIn.mttLscLineData.muiMode		= 0;
	}

	//AWB_CTRL_LOGD("[OTP]gldn r,g,b :%d, %d, %d",param->otp_info.gldn_stat_info.r,param->otp_info.gldn_stat_info.g,param->otp_info.gldn_stat_info.b);
	//AWB_CTRL_LOGD("[OTP]rdm r,g,b :%d, %d, %d",param->otp_info.rdm_stat_info.r,param->otp_info.rdm_stat_info.g,param->otp_info.rdm_stat_info.b);

	aptIns->mttIn.muiCamMode = 0xffffffff; 	//initialize value(in-valid mode)
	aptIns->mttIn.muiOpType = 0;		//Non-ZSL, ZSL, camcoder

	aptIns->mttIn.mptWdAwb = 0;
//	init_param.stat_img_size.w = ctr_init_param->stat_img_size.w;
//	init_param.stat_img_size.h = ctr_init_param->stat_img_size.h;
//	init_param.scalar_factor = (ctr_init_param->stat_win_size.w / 2) * (ctr_init_param->stat_win_size.h / 2);
	aptIns->mttIn.muiBlkH	= 32;		//Statistics block Horizontal number per frame
	aptIns->mttIn.muiBlkV	= 32;		//Statistics block Vertical number per frame
	aptIns->mttIn.muiBlkPixH = param->stat_win_size.w; 	//pixel number Horizontal per block
	aptIns->mttIn.muiBlkPixV = param->stat_win_size.h; 	//pixel number Vertical per block

	al_awb_thirdlib_fun.AlAwbInterfaceInit(aptIns);
//Set Init Value
	//Convert s15.16  -> u4.10
	result->gain.r = (uint32_t)((uint32_t)aptIns->mttOut.muiWbg[0] >> 6);
	result->gain.g = (uint32_t)((uint32_t)aptIns->mttOut.muiWbg[1] >> 6);
	result->gain.b = (uint32_t)((uint32_t)aptIns->mttOut.muiWbg[2] >> 6);
	result->ct = aptIns->mttOut.msiCtemp;

	//CCM outputs
	set_result_ccm(aptIns,result);

	//LSC outputs
	set_result_lsc(aptIns,result);


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

	if (AWB_CTRL_TRUE != cxt->init) {
		AWB_CTRL_LOGE("AWB do not init!");
		return AWB_CTRL_ERROR;
	}

	AWB_CTRL_LOGE("camera_id = %d, work_mode = %d, flash_mode = %d, flash_effect = %d, ev_index = %d", cxt->camera_id, cxt->work_mode, cxt->flash_info.flash_mode, cxt->flash_info.effect, param->ae_info.ev_index);
//	AWB_CTRL_LOGE("ev_table[] = [%d, %d, %d, %d, %d, %d, %d, %d, %d]", param->ae_info.ev_table[0], param->ae_info.ev_table[1], param->ae_info.ev_table[2], param->ae_info.ev_table[3], param->ae_info.ev_table[4], param->ae_info.ev_table[5], param->ae_info.ev_table[6], param->ae_info.ev_table[7], param->ae_info.ev_table[8]);

	int asiLp;
	TT_AlAwbInterface*	aptIns = &cxt->mttInferface;

	if( work_mode != aptIns->mttIn.muiCamMode )
	{
		aptIns->mttIn.muiCamMode	= work_mode;	//0:Preview,1:Capture,2:Movie
		aptIns->mttIn.mptWdAwb		= (VP_32)0;
		if( work_mode != 1 )
		{//when capture,skip reset   /_calc() is not right timeing for caputure reset
			al_awb_thirdlib_fun.AlAwbInterfaceReset(aptIns);
		}
	}

	//@TODO support srpd format!
	aptIns->mttIn.mptWdAwb = &cxt->mttStats;
	//for(asiLp=0;asiLp<(aptIns->mttIn.muiBlkH*aptIns->mttIn.muiBlkV);asiLp++)
	for(asiLp=0;asiLp<(32*32);asiLp++)
	{
		cxt->mttStats.muiR[asiLp] = param->stat_img.chn_img.r[asiLp];
		cxt->mttStats.muiG[asiLp] = param->stat_img.chn_img.g[asiLp];
		cxt->mttStats.muiB[asiLp] = param->stat_img.chn_img.b[asiLp];
		cxt->mttStats.muiC[asiLp] = aptIns->mttIn.muiBlkPixH*aptIns->mttIn.muiBlkPixV;
	}
/*
//About AE Infomation
 AV Apertur Value 	AV = LOG2(Fno^2)
 TV Shutter Time Value 	TV = LOG2(1/SEC)
 SV Sensitivity Value 	SV = LOG2(ISO/3.125)  or LOG2(Gain) + BASE SV (-> ISO100 =SV5)
 EV = AV + TV = BV + SV
    => BV = EV -SV = AV + TV - SV
*/
{
#define BASE_SV	(5.0)
	float sv = log2( param->ae_info.gain) + BASE_SV;
	float tv = -log2( param->ae_info.exposure);
	float av = log2( param->ae_info.f_value*param->ae_info.f_value);
	//float bv = tv + av -sv;
	AWB_CTRL_LOGV("[ae_info]bv:%d,exp:%f,gain:%f,fno:%f, stable:%d,",	param->ae_info.bv,param->ae_info.exposure,param->ae_info.gain,param->ae_info.f_value,param->ae_info.stable);
	AWB_CTRL_LOGV("[ae_info]av,tv,sv,bv :%f, %f, %f, %f",av,tv,sv,(tv + av -sv));

	aptIns->mttIn.mflAv = av;
	aptIns->mttIn.mflTv = tv;
	aptIns->mttIn.mflSv = sv;
	aptIns->mttIn.mflEv = av+tv;
	aptIns->mttIn.mflBv = aptIns->mttIn.mflEv-sv;
}

{
	int ev_index  = param->ae_info.ev_index;
	float aaa = (float)param->ae_info.ev_table[ev_index]/(float)param->ae_info.ev_table[4];
	float evb = (float)log2( aaa );
	//SQ_32 evb_ = (SQ_32)(evb*65536);
	aptIns->mttIn.mflBv += evb;
	AWB_CTRL_LOGD("[ae_info]evb bv:%f,%f",evb,aptIns->mttIn.mflBv);

}
	//------------------------------------
	//LED Flash


	aptIns->mttIn.msiFlash_state = 1;
	aptIns->mttIn.mflLedOff 	= 0.0f;
	aptIns->mttIn.mflLedLow 	= 0.0f;
	aptIns->mttIn.mflLedHi		= 0.0f;
	switch(cxt->flash_info.flash_mode)
	{
	case AWB_CTRL_FLASH_PRE:
		aptIns->mttIn.msiFlash_state = 1;
		break;
	case AWB_CTRL_FLASH_MAIN:
		aptIns->mttIn.msiFlash_state = 2;
		aptIns->mttIn.mflLedHi = (float)cxt->flash_info.effect/1024;
		break;
//	case AWB_CTRL_FLASH_END:
	default:
		aptIns->mttIn.msiFlash_state = 0;
		break;
	}

	AWB_CTRL_LOGV("flash_mode:%d FlashRatio %f,",	cxt->flash_info.flash_mode	,(float)cxt->flash_info.effect/1024	);


	//------------------------------------
	//for UI Setting
	uint32_t lock_mode = 0;

	lock_mode = cxt->lock_info.lock_num;
	scene_mode = cxt->scene_mode;
	mawb_id = cxt->wb_mode;
	//scene mode prioriry is high more then MWB
	ais_cmd_set_awb_scenemode(aptIns,scene_mode);
	if((AWB_CTRL_SCENEMODE_AUTO == scene_mode) && (AWB_CTRL_WB_MODE_AUTO != mawb_id))
	{
		ais_cmd_set_awb_presetwb(aptIns,mawb_id);
	}
	ais_cmd_set_awb_LockMode(aptIns,lock_mode)	;

	al_awb_thirdlib_fun.AlAwbInterfaceMain(aptIns);

	//Convert s15.16  -> u4.10
	cxt->output_gain.r = (uint32_t)((uint32_t)aptIns->mttOut.muiWbg[0] >> 6);
	cxt->output_gain.g = (uint32_t)((uint32_t)aptIns->mttOut.muiWbg[1] >> 6);
	cxt->output_gain.b = (uint32_t)((uint32_t)aptIns->mttOut.muiWbg[2] >> 6);
	cxt->output_ct = aptIns->mttOut.msiCtemp;

	if( cxt->work_mode == 1)
	{
#if 1
		cxt->output_gain.r = (uint32_t)((uint32_t)aptIns->mttOutLed.muiWbg[0] >> 6);
		cxt->output_gain.g = (uint32_t)((uint32_t)aptIns->mttOutLed.muiWbg[1] >> 6);
		cxt->output_gain.b = (uint32_t)((uint32_t)aptIns->mttOutLed.muiWbg[2] >> 6);
#else
		cxt->output_gain.r = 1 <<10;
		cxt->output_gain.g = 3 <<10;
		cxt->output_gain.b = 1 <<10;
#endif
	}


	//CCM outputs
	set_result_ccm(aptIns,result);
	//LSC outputs
	set_result_lsc(aptIns,result);

	result->log_awb.log = aptIns->mttOut.mpiLog;
	result->log_awb.size = aptIns->mttOut.muiLogSize;

	result->log_lsc.log = aptIns->mttOutLsc.mpiLog;
	result->log_lsc.size = aptIns->mttOutLsc.muiSize;

#if 0	//OTP TEST
{
	struct tg_dmy_log*	dmy_log = &cxt->dmy_log;

	memcpy(dmy_log->head,"ALCSIIF6",8);
	memcpy(dmy_log->foot,"ALCE",4);


	if( aptIns->mttIn.mttLscLineData.mpiOtp != 0 )
	{
		AWB_CTRL_LOGD("[otp]tar,ref :%p, %p ,log: e h %d,%d, ",aptIns->mttIn.mttLscLineData.mpiOtp,aptIns->mttIn.mttLscLineData.mpiOtpRef,aptIns->mttIn.mttLscLineData.muiWidth,aptIns->mttIn.mttLscLineData.muiHight);
//		memcpy(dmy_log.tar,aptIns->mttIn.mttLscLineData.mpiOtp		,aptIns->mttIn.mttLscLineData.muiWidth * aptIns->mttIn.mttLscLineData.muiHight	*2 );
//		memcpy(dmy_log.ref,aptIns->mttIn.mttLscLineData.mpiOtpRef	,aptIns->mttIn.mttLscLineData.muiWidth * aptIns->mttIn.mttLscLineData.muiHight	*2 );
		memcpy(dmy_log->tar,aptIns->mttIn.mttLscLineData.mpiOtp		,20*16*4*2 );
		memcpy(dmy_log->ref,aptIns->mttIn.mttLscLineData.mpiOtpRef	,20*16*4*2 );
	}
	//disable AWB log
	//result->log_lsc.size = 0;//strlen(result->log_lsc.log);
	result->log_awb.log = (uint8_t*)dmy_log;
	result->log_awb.size = sizeof(struct tg_dmy_log);
}
#endif	//OTP TEST
#if 0
	if(cxt->work_mode == 1)
	{
		FILE*	fp;
		fp = fopen("/data/misc/media/otp_tar.bin","wb");
		fwrite(aptIns->mttIn.mttLscLineData.mpiOtp,20*16*4*2,1,fp);
		fclose(fp);
		fp = fopen("/data/misc/media/otp_ref.bin","wb");
		fwrite(aptIns->mttIn.mttLscLineData.mpiOtpRef,20*16*4*2,1,fp);
		fclose(fp);

	}
#endif
	result->gain.r = cxt->output_gain.r;
	result->gain.g = cxt->output_gain.g;
	result->gain.b = cxt->output_gain.b;
	result->ct = cxt->output_ct;

	return rtn;
}

static uint32_t _awb_set_wbmode(struct awb_ctrl_cxt *cxt, void *in_param)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	uint32_t awb_mode = *(uint32_t*)in_param;

	cxt->wb_mode = awb_mode;
	AWB_CTRL_LOGE("debug wbmode changed!");
	return rtn;
}

static uint32_t _awb_set_workmode(struct awb_ctrl_cxt *cxt, void *in_param)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	uint32_t work_mode = *(uint32_t*)in_param;

	cxt->work_mode = work_mode;

	return rtn;
}

static uint32_t _awb_set_flashratio(struct awb_ctrl_cxt *cxt, void *in_param)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	uint32_t flash_ratio = *(uint32_t*)in_param;

	cxt->flash_info.effect = flash_ratio;

	return rtn;
}

static uint32_t _awb_set_scenemode(struct awb_ctrl_cxt *cxt, void *in_param)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	uint32_t scene_mode = *(uint32_t*)in_param;

	cxt->scene_mode = scene_mode;
	return rtn;
}

static uint32_t _awb_set_recgain(struct awb_ctrl_cxt *cxt, void *param)
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

static uint32_t _awb_get_gain(struct awb_ctrl_cxt *cxt, void *param)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	uint32_t mawb_id = cxt->wb_mode;
	struct awb_gain *awb_result = (struct awb_gain*)param;

	awb_result->r = cxt->output_gain.r;
	awb_result->g = cxt->output_gain.g;
	awb_result->b = cxt->output_gain.b;

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

	AWB_CTRL_LOGV("_awb_get_gain = (%d,%d,%d)",awb_result->r,awb_result->g,awb_result->b);


	return rtn;

}

static uint32_t _awb_get_stat_size(struct awb_ctrl_cxt *cxt, void *param)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	struct awb_size *stat_size = (struct awb_size*)param;

	stat_size->w = cxt->init_param.stat_img_size.w;
	stat_size->h = cxt->init_param.stat_img_size.h;

	AWB_CTRL_LOGV("_awb_get_stat_size = (%d,%d)",stat_size->w, stat_size->h);

	return rtn;
}

static uint32_t _awb_get_winsize(struct awb_ctrl_cxt *cxt, void *param)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	uint32_t workmode = cxt->work_mode;
	struct awb_size *win_size = (struct awb_size*)param;

	win_size->w = cxt->tuning_param[workmode].stat_win_size.w;
	win_size->h = cxt->tuning_param[workmode].stat_win_size.h;

	AWB_CTRL_LOGV("_awb_get_winsize = (%d,%d), work_mode=%d",win_size->w, win_size->h, workmode);

	return rtn;
}

static uint32_t _awb_get_ct(struct awb_ctrl_cxt *cxt, void *param)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	uint32_t *ct = (uint32_t *)param;

	*ct = cxt->output_ct;

	AWB_CTRL_LOGE("_awb_get_ct = %d", cxt->output_ct);

	return rtn;
}

static uint32_t _awb_get_recgain(struct awb_ctrl_cxt *cxt, void *param)
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

static uint32_t _awb_set_flash_gain(struct awb_ctrl_cxt *cxt, void *param)
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

static uint32_t _awb_set_lock(struct awb_ctrl_cxt *cxt, void *param)
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

static uint32_t _awb_get_unlock(struct awb_ctrl_cxt *cxt, void *param)
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

static uint32_t _awb_set_flash_status(struct awb_ctrl_cxt *cxt, void *param)
{
	UNUSED(param);
	uint32_t rtn = AWB_CTRL_SUCCESS;
	enum awb_ctrl_flash_status *flash_status = (enum awb_ctrl_flash_status*)param;

	cxt->flash_info.flash_status = *flash_status;

	AWB_CTRL_LOGE("flashing status = %d", cxt->flash_info.flash_status);

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
		if ((AWB_CTRL_FLASH_PRE == cxt->flash_info.flash_mode) || (AWB_CTRL_FLASH_MAIN == cxt->flash_info.flash_mode)) {
			rtn = _awb_get_recgain(cxt, param0);
		}
		cxt->flash_info.flash_mode = AWB_CTRL_FLASH_END;
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

	case AWB_CTRL_CMD_SET_FLASH_STATUS:
		rtn = _awb_set_flash_status(cxt,param0);
		break;
		
	default:
		AWB_CTRL_LOGE("invalid cmd = 0x%x", cmd);
		rtn = AWB_CTRL_ERROR;
		break;
	}

	pthread_mutex_unlock(&cxt->status_lock);
	return rtn;
}

uint32_t al_awb_lib_open(uint32_t version_id)
{
	void *handle;
	char *AWB_LIB;
	ISP_LOGE("E");
	AWB_LIB = al_libversion_choice(version_id);
	handle = dlopen(AWB_LIB, RTLD_NOW);
	if(NULL == handle) {
		ISP_LOGE("dlopen get handle error\n");
		return ISP_ERROR;
	}
	al_awb_thirdlib_fun.AlAwbInterfaceInit = (void *)dlsym(handle, "AlAwbInterfaceInit");
	al_awb_thirdlib_fun.AlAwbInterfaceDestroy =
		(void *)dlsym(handle, "AlAwbInterfaceDestroy");
	al_awb_thirdlib_fun.AlAwbInterfaceReset = (void *)dlsym(handle, "AlAwbInterfaceReset");
	al_awb_thirdlib_fun.AlAwbInterfaceMain =
		(void *)dlsym(handle, "AlAwbInterfaceMain");
	al_awb_thirdlib_fun.AlAwbInterfaceSendCommand =
		(void *)dlsym(handle, "AlAwbInterfaceSendCommand");
	al_awb_thirdlib_fun.AlAwbInterfaceShowVersion =
		(void *)dlsym(handle, "AlAwbInterfaceShowVersion");
	ISP_LOGE("awb_thirdlib_fun.AlAwbInterfaceInit= %p", al_awb_thirdlib_fun.AlAwbInterfaceInit);
	ISP_LOGE("AWB_LIB= %s", AWB_LIB);

	return 0;
}

char* al_libversion_choice(uint32_t version_id)
{
	ISP_LOGE("E");
	switch (version_id)
	{
		case AL_AWB_LIB_VERSION_0:
			return "/system/lib/libAl_Awb_ov13850r2a.so";
		case AL_AWB_LIB_VERSION_1:
			return "/system/lib/libAl_Awb_t4kb3.so";
	}

	return NULL;
}

extern struct awb_lib_fun awb_lib_fun;
void alc_awb_fun_init()
{
	awb_lib_fun.awb_ctrl_init 		= awb_al_ctrl_init;
	awb_lib_fun.awb_ctrl_deinit		= awb_al_ctrl_deinit;
	awb_lib_fun.awb_ctrl_calculation	= awb_al_ctrl_calculation;
	awb_lib_fun.awb_ctrl_ioctrl		= awb_al_ctrl_ioctrl;

	return;
}
