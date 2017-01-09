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
#ifndef _FILTER_CALC_H_
#define _FILTER_CALC_H_
/**---------------------------------------------------------------------------**
**                         Dependencies                                      **
**---------------------------------------------------------------------------**/
//#include "apm_codec.h"
//#include "asm.h"
#include <eng_audio.h>
#include "aud_common.h"
#include <linux/types.h>
#define filter_max(a, b) (((a) > (b)) ? (a) : (b))
/**---------------------------------------------------------------------------**
**                        Debugging Flag                                     **
**---------------------------------------------------------------------------**/

/**---------------------------------------------------------------------------**
**                         Compiler Flag                                      **
**----------------------------------------------------------------------------**/
#ifdef __cplusplus
extern   "C"
{
#endif
/**---------------------------------------------------------------------------**
 **                         MACRO Definations                                 **
 **---------------------------------------------------------------------------**/

/**---------------------------------------------------------------------------**
 **                         Data Structures                                   **
 **---------------------------------------------------------------------------**/
typedef struct
{
    int16_t   B0;
    int16_t   B1;
    int16_t   B2;

    int16_t   A0;
    int16_t   A1;
    int16_t   A2;
}IIR_FILTER_PARA_T;


typedef enum//  3  para  types
{
    FILTER_EQPARA_NORMAL_EQ = 0,
    FILTER_EQPARA_LOW_SHELVE ,
    FILTER_EQPARA_HIGH_SHELVE ,
    FILTER_EQPARA_MAX
}FILTER_EQPARA_TYPE_E;

union Filter_eq_input_para_union
{
    int16_t q;          //FOR noraml eq
    int16_t fo_next;   //for LS
    int16_t fo_last;   //for HS
};

typedef struct
{
    BOOLEAN   isFilterOn;
    FILTER_EQPARA_TYPE_E eEqParaType;
	int16_t  fo;                     //central freq in Hz      , nv
	union Filter_eq_input_para_union unEqPara; //q OR fo_next OR  fo_last
	int16_t  boostdB;                //range -720:1:180;  (-72dB ~ 18dB ,step:0.1dB)
    int16_t  basegaindB;             //range -720:1:180;  (-72dB ~ 18dB ,step:0.1dB)
}FILTER_EQ_CALC_PARA_T;


typedef enum
{
	FILTER_LCFPARA_F1F1 = 0,
	FILTER_LCFPARA_BUTTERWORTH,
	FILTER_LCFPARA_CHEBYSHEV,
	FILTER_LCFPARA_BESSEL	,
	FILTER_LCFPARA_ELLIPTICAL,
	FILTER_LCFPARA_MAX
}FILTER_LCFPARA_TYPE_E;


typedef struct
{
    int16_t f1_g0;
    int16_t f1_g1;
    int16_t f1_fp;

    int16_t f2_g0;
    int16_t f2_g1;
    int16_t f2_fp;
}FILTER_LCF_F1F1_PARAM_T;

union Filter_lcf_input_para_union
{
    int16_t fp;
    FILTER_LCF_F1F1_PARAM_T lcfPara;
};

typedef struct
{
    BOOLEAN   isFilterOn;
    FILTER_LCFPARA_TYPE_E eLcfParaType;
	union Filter_lcf_input_para_union unlcfPara;
}FILTER_LCF_CALC_PARA_T;

/**---------------------------------------------------------------------------**
 **                         Global Variables                                  **
 **---------------------------------------------------------------------------**/


/**---------------------------------------------------------------------------**
 **                         Constant Variables                                **
 **---------------------------------------------------------------------------**/


/**---------------------------------------------------------------------------**
 **                          Function Declare                                 **
 **---------------------------------------------------------------------------**/
/*****************************************************************************/
//! \brief  Description:  This function is to caculate eq paras.
//! \author Author:  cherry.liu
/*****************************************************************************/
BOOLEAN Filter_CalcEqCoeffs(
	int16_t  fo,                       //central freq in Hz      , nv
	int16_t  q,                        //fo/df *512 , nv
	int16_t  boostdB,                 //range -720:1:180;  (-72dB ~ 18dB ,step:0.1dB)
    int16_t  basegaindB,              //range -720:1:180;  (-72dB ~ 18dB ,step:0.1dB)
	int32_t  fs,                      //samplerate in Hz
	IIR_FILTER_PARA_T* filterPara  //out ptr,filter paras a,b
);

/*****************************************************************************/
//! \brief  Description:  This function is to caculate f1f1 filter paras.
//! \author Author:  cherry.liu
/*****************************************************************************/
BOOLEAN Filter_CalcF1f1Coeffs(
		int16_t  f1_g0_dB,                //range -720:1:180;  (-72dB ~ 18dB ,step:0.1dB)
		int16_t  f1_g1_dB,                //range -720:1:180;  (-72dB ~ 18dB ,step:0.1dB)
		int16_t  f1_fp,                   // in Hz
		int16_t  f2_g0_dB,                //range -720:1:180;  (-72dB ~ 18dB ,step:0.1dB)
		int16_t  f2_g1_dB,                //range -720:1:180;  (-72dB ~ 18dB ,step:0.1dB)
		int16_t  f2_fp,                   // in Hz
		int32_t  fs,                      //samplerate in Hz
		IIR_FILTER_PARA_T* filterPara   //out ptr,filter paras a,b

);

/*****************************************************************************/
//! \brief  Description:  This function is to caculate eq paras and output hpf para set.
//! \author Author:  cherry.liu
/*****************************************************************************/
BOOLEAN Filter_CalcEQ(
    FILTER_EQ_CALC_PARA_T* eq_input_para_ptr,//IN
	int32_t  fs,  //IN
    IIR_FILTER_PARA_T* filterPara ,//OUT
    int16_t*   sGain//OUT
);

/*****************************************************************************/
//! \brief  Description:  This function is to caculate f1f1 filter paras and output hpf para set.
//! \author Author:  cherry.liu
/*****************************************************************************/
BOOLEAN Filter_CalcLCF(
    FILTER_LCF_CALC_PARA_T* lcf_input_para_ptr,//IN
    int16_t  lcf_gain,//IN ,scaled by 16384
	int32_t  fs,  //IN
    IIR_FILTER_PARA_T* filterPara ,//OUT
    int16_t*   sGain//OUT
);

/*****************************************************************************/
//! \brief  Description:  This function is to caculate eq paras and output hpf para set.
//! \author Author:  cherry.liu
//! this interface is for tool layer only
/*****************************************************************************/
BOOLEAN Filter_CalcEQ_Para(
    BOOLEAN  band_filter_on,
	int16_t    band_fo,
    int16_t    band_q,
	int16_t    band_boostdB,
    int16_t    band_basegaindB,
	int32_t    fs,
    IIR_FILTER_PARA_T* filterPara ,//OUT
    int16_t*   sGain//OUT
);


/*****************************************************************************/
//! \brief  Description:  This function is to caculate f1f1 filter paras and output hpf para set.
//! \author Author:  cherry.liu
//! this interface is for tool layer only
/*****************************************************************************/
BOOLEAN Filter_CalcLCF_Para(
    FILTER_LCF_CALC_PARA_T* lcf_input_para_ptr,//IN
	int32_t  fs,  //IN
    IIR_FILTER_PARA_T* filterPara ,//OUT
    int16_t*   sGain//OUT
);

/*****************************************************************************/
//! \brief  Description:  This function is to caculate eq a/b paras.
//!    it is an enhancement or extention function for Filter_CalcEqCoeffs
//! \author Author:  cherry.liu
/*****************************************************************************/
BOOLEAN Filter_CalcEqCoeffs_Ex(
	int16_t  fo,                    //central freq in Hz
	int16_t  df,                    //band width in Hz
	int16_t  boostdB,               //range -720:1:180;  (-72dB ~ 18dB ,step:0.1dB)
    int16_t  basegaindB,            //range -840:1:60;  (-84dB ~ 6dB ,step:0.1dB)
	int32_t  Fs,                    //samplerate in Hz
	IIR_FILTER_PARA_T* filterPara,  //out ptr,filter paras a,b
	int16_t*   sGain//OUT
);
/**---------------------------------------------------------------------------**
 **                         Compiler Flag                                     **
 **---------------------------------------------------------------------------**/
#ifdef __cplusplus
}
#endif

#endif //end of _TEST_H

//end of file








