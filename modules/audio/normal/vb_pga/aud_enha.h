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
#ifndef _AUD_ENHA_H
#define _AUD_ENHA_H

/**---------------------------------------------------------------------------*
 **                         Dependencies                                      *
 **---------------------------------------------------------------------------*/
#include "vb_hal.h"
#include "filter_calc.h"
#include <eng_audio.h>
#include "aud_common.h"

//#include "aud_enha_exp.h"

#ifdef __cplusplus
    extern   "C"
    {
#endif

//#include "os_api.h"

/**---------------------------------------------------------------------------*
 **                         MACRO Definations                                     *
 **---------------------------------------------------------------------------*/
#define AUDIO_NO_ERROR  0
#define AUDIO_ERROR  1
#define TOTAL_S_GAIN_NUM  7 //s0-s6
#define HPF_S_GAIN_NUM    6 //s0-s5


/**---------------------------------------------------------------------------*
 **                         Data Structures                                   *
 **---------------------------------------------------------------------------*/
//add for 5-bands eq support(eg.sc8800g)
typedef enum
{
    AUD_ENHA_EQPARA_NULL = 0,
    AUD_ENHA_EQPARA_HEADSET ,// 1
    AUD_ENHA_EQPARA_HEADFREE,   // 2
    AUD_ENHA_EQPARA_HANDSET,   // 3
    AUD_ENHA_EQPARA_HANDSFREE,// 4
    AUD_ENHA_EQPARA_MAX// 5
}AUD_ENHA_EQPARA_SET_E;

typedef enum
{
    AUD_ENHA_TUNABLE_EQPARA_NULL=0,//0
    AUD_ENHA_TUNABLE_EQPARA_COMMON,// 1
    AUD_ENHA_TUNABLE_EQPARA_MAX//  2
}AUD_ENHA_TUNABLE_EQPARA_SET_E;

typedef enum
{
    AUD_ENHA_EQMODE_SEL_OFF = 0,     //0
    AUD_ENHA_EQMODE_SEL_REGULAR ,   // 1
    AUD_ENHA_EQMODE_SEL_CLASSIC,   // 2
    AUD_ENHA_EQMODE_SEL_ODEUM,     // 3
    AUD_ENHA_EQMODE_SEL_JAZZ,      // 4
    AUD_ENHA_EQMODE_SEL_ROCK,      // 5
    AUD_ENHA_EQMODE_SEL_SOFTROCK,// 6
    AUD_ENHA_EQMODE_SEL_MMISET =15,// 15
    AUD_ENHA_EQMODE_SEL_MAX
}AUD_ENHA_EQMODE_SEL_E;

//------------definition for getting params for this plugger ----------------//




//------------definition for setting params for this plugger ----------------//
typedef enum//  3  para  types
{
    AUD_ENHA_PARA_EQ_MODE = 0,
    AUD_ENHA_PARA_DIGI_GAIN ,
    AUD_ENHA_PARA_EQ_SET ,
    AUD_ENHA_PARA_DEV_MODE,
    AUD_ENHA_PARA_BAND_BOOST,
    AUD_ENHA_PARA_SET_DEFAULT,
    AUD_ENHA_PARA_EQ_ALLBANDS_BOOST,
    AUD_ENHA_PARA_TUNABLE_EQ_SET,
    AUD_ENHA_PARA_MAX
}AUD_ENHA_PARA_TYPE_E;


typedef struct
{
    uint32_t eqMode;
    int16_t  bandIndex;
	int16_t  bandBoost;
}AUD_ENHA_EXP_BAND_INFO_T;

typedef struct
{
    uint32_t eqMode;
    int16_t  bandNum;
	int16_t  bandBoost[EQ_BAND_MAX];
}AUD_ENHA_EXP_ALLBANDS_INFO_T;

typedef struct
{
    uint32_t  eqSetIndex;
	uint32_t  eqMode;
}AUD_ENHA_EXP_EQ_MODE_SET_T;

union Aud_enha_para_union
{
    uint32_t eqMode;
    uint32_t digitalGain;
    AUD_ENHA_EXP_EQ_MODE_SET_T eqSetInfo;
    char *devModeName;
    AUD_ENHA_EXP_BAND_INFO_T bandInfo;
    AUD_ENHA_EXP_ALLBANDS_INFO_T modeInfo;
};

typedef struct
{
    AUD_ENHA_PARA_TYPE_E eParaType;
	union Aud_enha_para_union unAudProcPara;
}AUD_ENHA_EXP_PARA_T;

//---------------------------control params in nv ----------------------//
//untunable eq

//typedef struct
//{
//    int16_t   fo ;  /*f0*/
//    int16_t   q;    /*q*/
//    int16_t   boostdB;   /*boost */
//    int16_t   gaindB ;      /*gain*/
//}EQ_BAND_INPUT_PARA_T;

//typedef struct
//{
 //   int16_t   agc_in_gain;  /*agc in gain set*/
 //   int16_t   band_control; /*bit15-bit8 :filter_sw_1~8 ; bit 1: high shelve;bit0:low shelve */
//    EQ_BAND_INPUT_PARA_T  eq_band[EQ_BAND_MAX];
//}EQ_MODE_PARA_T;

//typedef struct //PACKED  272 words
//{
//    uint8_t   para_name[NAME_LEN_MAX];/*struct name*/
//    uint16_t  eq_control;//bit15:8-bands-sw
//    EQ_MODE_PARA_T eq_modes[EQ_MODE_MAX];     /*eq mode para*/
//    int16_t externdArray[59]; /*reserved for future*/
//}AUDIO_ENHA_EQ_STRUCT_T;

//tunable eq
typedef struct
{
    int16_t   agc_in_gain;  /*agc in gain set*/
    int16_t   band_control;
    int16_t   boostdB_default[EQ_BAND_MAX];  /*default boost dB for each band*/
    int16_t   boostdB_current[EQ_BAND_MAX];  /*current boost dB for each band;set by mmi*/
}TUNABLE_EQ_MODE_PARA_T;

typedef struct //PACKED 188 words
{
    uint8_t   para_name[NAME_LEN_MAX];/*struct name*/
    int16_t   eq_control;       /* bit15:8-bands-sw;bit9-bit0:level_step;*/
    int16_t   fo_array[EQ_BAND_MAX];
    int16_t   q_array[EQ_BAND_MAX];
    int16_t   level_n;
    TUNABLE_EQ_MODE_PARA_T eq_modes[EQ_MODE_MAX];     /*eq mode para*/
    int16_t externdArray[54]; /*reserved for future*/
}AUDIO_ENHA_TUNABLE_EQ_STRUCT_T;


typedef enum
{
	HPF_DATA_WIDTH_16 = 16,
	HPF_DATA_WIDTH_24 = 24,
	HPF_DATA_WIDTH_MAX
}HPF_DATA_WIDTH_TYPE_E;

typedef struct
{
    int16_t    left_gain;
    int16_t    right_gain;
}DG_CONTROL_PARAM_T;

typedef struct
{
    VBC_DA_MIX_MODE_E left_fm_mix_mode;
    VBC_DA_MIX_MODE_E right_fm_mix_mode;
}DAPATH_CONTROL_PARAM_T;

typedef struct
{
    BOOLEAN alc_sw;
    int16_t   alc_input_gain;
    int16_t   alc_ingain_Set;
    VBC_ALC_PARAS_T alc_Para;
}ALC_CONTROL_PARAM_T;

typedef struct
{
    int16_t s[HPF_S_GAIN_NUM];  //s0 ~ s5
}HPF_GAIN_PARAM_T;


typedef struct
{
    HPF_DATA_WIDTH_TYPE_E  data_width;
    int16_t    r_limit;
    HPF_GAIN_PARAM_T    s_gain;//s0~s6
    FILTER_LCF_CALC_PARA_T lcf_para;//served for hpf-1
    uint32_t    eq_band_num;//0-8
    BOOLEAN  low_shelve_on;
    BOOLEAN  high_shelve_on;
    BOOLEAN  band_sw[EQ_BAND_MAX];
    EQ_BAND_INPUT_PARA_T  eq_band_para[EQ_BAND_MAX];
}HPF_CONTROL_PARAM_T;



/**---------------------------------------------------------------------------*
 **                         Global Variables                                  *
 **---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
 **                         Constant Variables                                *
 **---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
 **                      public  Function Prototypes                               *
 **---------------------------------------------------------------------------*/

 PUBLIC int	AUDENHA_SetPara(
	 AUDIO_TOTAL_T *audio_param_ptr,
         uint32_t *para_buf_ptr
);


 /*****************************************************************************/
//  Description:    init HPF filters:
//                  the first hpf is for lcf;
//                  the second to sixth hp are for 5-band eq;
//
//  Author:         Cherry.Liu
//  Note:
//****************************************************************************/
PUBLIC int AUDENHA_InitHpf(
    HPF_CONTROL_PARAM_T* hpf_param_ptr,
    int32_t sample_rate

);

/*****************************************************************************/
//  Description:    set eq mode para for audio process modules, including lcf,alc and eq.
//  Author:         Cherry.Liu
//  Note:
//****************************************************************************/
PUBLIC int AUDENHA_SetEqMode(
    ALC_CONTROL_PARAM_T* alc_param_ptr,        //alc
    HPF_CONTROL_PARAM_T* hpf_param_ptr,        //hpf
    int32_t sample_rate                         //fs
);

/*****************************************************************************/
//  Description:    set digital gain
//  Author:         Cherry.Liu
//  Note:           dg gain here is mapped to arm volume[i] bit15 ~ bit9
//****************************************************************************/
PUBLIC BOOLEAN AUDENHA_SetDigiGain(
    DG_CONTROL_PARAM_T* dg_param_ptr
);

/*****************************************************************************/
//  Description:    init audio process modules including lcf,agc and eq.
//  Author:         Cherry.Liu
//  Note:           !attention! you should init it before music start.
//****************************************************************************/
PUBLIC int AUDENHA_Init(
    DG_CONTROL_PARAM_T* dg_param_ptr,          //dg
    DAPATH_CONTROL_PARAM_T * dapath_param_ptr, //da path
    ALC_CONTROL_PARAM_T* alc_param_ptr,        //alc
    HPF_CONTROL_PARAM_T* hpf_param_ptr,        //hpf
    int32_t sample_rate                         //fs
);

/*****************************************************************************/
//  Description:    deinti aud proc plugger
//  Author:         Cherry.Liu
//  Note:
//****************************************************************************/
PUBLIC BOOLEAN AUDENHA_DeInit(
    void
);


/**---------------------------------------------------------------------------*
 **                         Compiler Flag                                     *
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif

#endif  // _AUD_AGC_H

// End of aud_agc.h



