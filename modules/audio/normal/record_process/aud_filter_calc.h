/******************************************************************************
** File Name:      filter_calc.h                                            
** Author:         cherry.liu                                              
** DATE:           04/14/2006                                                
** Copyright:      2010 Spreatrum, Incoporated. All Rights Reserved.         
** Description:    This file defines the basic operation interfaces 
**                 of filter paras caculator.                   
******************************************************************************

******************************************************************************
**                        Edit History                                       
**  -----------------------------------------------------------------------  
** DATE           NAME             DESCRIPTION                               
** 04/14/2006       cherry.liu     Create.                                   
******************************************************************************/
  
#ifndef _AUD_FILTER_CALC_H_
#define _AUD_FILTER_CALC_H_




#include "audproc_type.h"
#include <utils/Log.h>
/**---------------------------------------------------------------------------**
**                         Dependencies                                      **
**---------------------------------------------------------------------------**/
#ifdef __cplusplus
    extern   "C"
    {
#endif

/**---------------------------------------------------------------------------**
**                        Debugging Flag                                     **
**---------------------------------------------------------------------------**/

/**---------------------------------------------------------------------------**
**                         Compiler Flag                                      **
**----------------------------------------------------------------------------**/


/**---------------------------------------------------------------------------**
 **                         MACRO Definations                                 **
 **---------------------------------------------------------------------------**/

#define max(a, b) (((a) > (b)) ? (a) : (b))

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
} REC_IIR_FILTER_PARA_T; 


typedef enum//  3  para  types
{   
    REC_FILTER_EQPARA_NORMAL_EQ = 0,
    REC_FILTER_EQPARA_LOW_SHELVE ,
    REC_FILTER_EQPARA_HIGH_SHELVE ,
    REC_FILTER_EQPARA_MAX
} REC_FILTER_EQPARA_TYPE_E;

union REC_Filter_eq_input_para_union
{
    int16_t q;          //FOR noraml eq
    int16_t fo_next;   //for LS          
    int16_t fo_last;   //for HS   
};

typedef struct 
{
    BOOLEAN   isFilterOn;
    REC_FILTER_EQPARA_TYPE_E eEqParaType;	
	int16_t  fo;                     //central freq in Hz      , nv
	union REC_Filter_eq_input_para_union unEqPara; //q OR fo_next OR  fo_last
	int16_t  boostdB;                //range -720:1:180;  (-72dB ~ 18dB ,step:0.1dB)
    int16_t  basegaindB;             //range -720:1:180;  (-72dB ~ 18dB ,step:0.1dB)
} REC_FILTER_EQ_CALC_PARA_T;


typedef enum
{
	REC_FILTER_LCFPARA_F1F1 = 0,            
	REC_FILTER_LCFPARA_BUTTERWORTH,                
	REC_FILTER_LCFPARA_CHEBYSHEV,    
	REC_FILTER_LCFPARA_BESSEL	,         
	REC_FILTER_LCFPARA_ELLIPTICAL,                  
	REC_FILTER_LCFPARA_MAX
} REC_FILTER_LCFPARA_TYPE_E;


typedef struct 
{
    int16_t f1_g0;  
    int16_t f1_g1;  
    int16_t f1_fp;  
    
    int16_t f2_g0;  
    int16_t f2_g1;    
    int16_t f2_fp;  
} REC_FILTER_LCF_F1F1_PARAM_T; 

union REC_Filter_lcf_input_para_union
{
    int16_t fp;        
    REC_FILTER_LCF_F1F1_PARAM_T lcfPara;
};

typedef struct 
{
    BOOLEAN   isFilterOn;
    REC_FILTER_LCFPARA_TYPE_E eLcfParaType;	
	union REC_Filter_lcf_input_para_union unlcfPara;
} REC_FILTER_LCF_CALC_PARA_T;

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
BOOLEAN Rec_Filter_CalcEqCoeffs(
	int16_t  fo,                       //central freq in Hz      , nv
	int16_t  q,                        //fo/df *512 , nv
	int16_t  boostdB,                 //range -720:1:180;  (-72dB ~ 18dB ,step:0.1dB)
    int16_t  basegaindB,              //range -720:1:180;  (-72dB ~ 18dB ,step:0.1dB)
	int32_t  fs,                      //samplerate in Hz
	REC_IIR_FILTER_PARA_T* filterPara  //out ptr,filter paras a,b
);

/*****************************************************************************/
//! \brief  Description:  This function is to caculate f1f1 filter paras.
//! \author Author:  cherry.liu 
/*****************************************************************************/
BOOLEAN Rec_Filter_CalcF1f1Coeffs(
		int16_t  f1_g0_dB,                //range -720:1:180;  (-72dB ~ 18dB ,step:0.1dB)
		int16_t  f1_g1_dB,                //range -720:1:180;  (-72dB ~ 18dB ,step:0.1dB)
		int16_t  f1_fp,                   // in Hz
		int16_t  f2_g0_dB,                //range -720:1:180;  (-72dB ~ 18dB ,step:0.1dB)
		int16_t  f2_g1_dB,                //range -720:1:180;  (-72dB ~ 18dB ,step:0.1dB)
		int16_t  f2_fp,                   // in Hz
		int32_t  fs,                      //samplerate in Hz
		REC_IIR_FILTER_PARA_T* filterPara   //out ptr,filter paras a,b
		
);

/*****************************************************************************/
//! \brief  Description:  This function is to caculate eq paras and output hpf para set.
//! \author Author:  cherry.liu 
/*****************************************************************************/
BOOLEAN Rec_Filter_CalcEQ(
    REC_FILTER_EQ_CALC_PARA_T* eq_input_para_ptr,//IN
	int32_t  fs,  //IN
    REC_IIR_FILTER_PARA_T* filterPara ,//OUT
    int16_t*   sGain//OUT
);

/*****************************************************************************/
//! \brief  Description:  This function is to caculate f1f1 filter paras and output hpf para set.
//! \author Author:  cherry.liu 
/*****************************************************************************/
BOOLEAN Rec_Filter_CalcLCF(
    REC_FILTER_LCF_CALC_PARA_T* lcf_input_para_ptr,//IN
    int16_t  lcf_gain,//IN ,scaled by 16384
	int32_t  fs,  //IN
    REC_IIR_FILTER_PARA_T* filterPara ,//OUT
    int16_t*   sGain//OUT
);

/*****************************************************************************/
//! \brief  Description:  This function is to caculate eq paras and output hpf para set.
//! \author Author:  cherry.liu 
//! this interface is for tool layer only
/*****************************************************************************/
BOOLEAN Rec_Filter_CalcEQ_Para(
    BOOLEAN  band_filter_on,
	int16_t    band_fo,                
    int16_t    band_q, 
	int16_t    band_boostdB,  
    int16_t    band_basegaindB, 
	int32_t    fs,              
    REC_IIR_FILTER_PARA_T* filterPara ,//OUT
    int16_t*   sGain//OUT
);


/*****************************************************************************/
//! \brief  Description:  This function is to caculate f1f1 filter paras and output hpf para set.
//! \author Author:  cherry.liu 
//! this interface is for tool layer only
/*****************************************************************************/
BOOLEAN Rec_Filter_CalcLCF_Para(
    REC_FILTER_LCF_CALC_PARA_T* lcf_input_para_ptr,//IN
	int32_t  fs,  //IN
    REC_IIR_FILTER_PARA_T* filterPara ,//OUT
    int16_t*   sGain//OUT
);

/*****************************************************************************/
//! \brief  Description:  This function is to caculate eq a/b paras.
//!    it is an enhancement or extention function for Rec_Filter_CalcEqCoeffs
//! \author Author:  cherry.liu 
/*****************************************************************************/
BOOLEAN Rec_Filter_CalcEqCoeffs_Ex(
	int16_t  fo,                    //central freq in Hz  
	int16_t  df,                    //band width in Hz  
	int16_t  boostdB,               //range -720:1:180;  (-72dB ~ 18dB ,step:0.1dB)
    int16_t  basegaindB,            //range -840:1:60;  (-84dB ~ 6dB ,step:0.1dB)
	int32_t  Fs,                    //samplerate in Hz
	REC_IIR_FILTER_PARA_T* filterPara,  //out ptr,filter paras a,b
	int16_t*   sGain//OUT
);

/*****************************************************************************/
//! \brief  Description:  This function is to caculate 200*log10(x)
//! \range of x [1,32767]
//! \author Author:  cherry.liu 
/*****************************************************************************/
int16_t F200log10(int32_t input);

/*****************************************************************************/
//! \brief  Description:  This function is to caculate 32768*10^(-x/200)
//! \range of x [0,1023]
//! \author Author:  cherry.liu 
/*****************************************************************************/
int16_t F32768power10(int32_t input);

/*****************************************************************************/
//! \brief  Description:  This function is to caculate eq a/b paras.
//!    This function is used for record eq.
//! \author Author:  cherry.liu 
/*****************************************************************************/
BOOLEAN Filter_CalcRecordEq(
    BOOLEAN  bandSW,                //SWITCH of current band
	int16_t  fo,                      //central freq in Hz  
	int16_t  df,                      //band width in Hz  
	int16_t  boostdB,                 //range -720:1:180;  (-72dB ~ 18dB ,step:0.1dB)
    int16_t  basegaindB,              //range -720:1:180;  (-72dB ~ 18dB ,step:0.1dB)
	int32_t  Fs,                      //samplerate in Hz
	REC_IIR_FILTER_PARA_T* filterPara,  //out ptr,filter paras a,b
	int16_t*   sGain                  //OUT  scaled by 4096
);
/**---------------------------------------------------------------------------**
 **                         Compiler Flag                                     **
 **---------------------------------------------------------------------------**/ 

#ifdef __cplusplus
}
#endif

#endif







