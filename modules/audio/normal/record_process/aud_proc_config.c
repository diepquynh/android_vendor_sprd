/******************************************************************************
 ** File Name:      aud_proc_config.c                                                     *
 ** Author:         Cherry.Liu                                                *
 ** DATE:           04/15/2010                                                *
 ** Copyright:      2010 Spreadtrum, Incoporated. All Rights Reserved.        *
 ** Description:    This file serves as audio process module.             * 
 **                                                                           *
 ******************************************************************************

 ******************************************************************************
 **                        Edit History                                       *
 ** ------------------------------------------------------------------------- *
 ** DATE           NAME             DESCRIPTION                               *
 ** 04/15/2010     Cherry.Liu       Create.                                   *
 ******************************************************************************/  
/**---------------------------------------------------------------------------*
 **                         Dependencies                                      *
 **---------------------------------------------------------------------------*/ 
#define LOG_TAG "record_process"
/* #define LOG_NDEBUG 0 */



#include "audio_record_nr.h"
#include "aud_proc.h"
#include "aud_filter_calc.h"


/**---------------------------------------------------------------------------*
 **                         Compiler Flag                                     *
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus 
extern "c"
{
#endif
/**---------------------------------------------------------------------------*
 **                         Macro Definition                                  *
 **---------------------------------------------------------------------------*/
#define EQ_PARA_SET_DELAY_TIME 20 //ms range:25-50
#define EQ_PARA_SET_S2_SET_TIMES 20



/**---------------------------------------------------------------------------*
 **                         Constant Variables                                *
 **---------------------------------------------------------------------------*/
 static  const uint8_t AGC_G75[8] = 
{
    128,117,108,99,90,83,76,70
};
/**---------------------------------------------------------------------------*
 **                         Data Structures                                   *
 **---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
 **                         Global Variables                                  *
 **---------------------------------------------------------------------------*/
//=========global params those are from nv set===================//
//%agc
 BOOLEAN AGC_SW_m    =  TRUE;  // 0 = disable;  1:AGC enable ; 
 BOOLEAN AGC_ZC_SW   =  TRUE; // 1:zeros cross on;0:zeros cross off

 int16_t AGC_input_Gain         = 4*1024;//scaled by 1024 int16_t
 int16_t AGC_input_Gain_start   = 4*1024;//scaled by 1024 int16_t

 int16_t AGC_delay        = 25;   //ms
 int16_t AGC_Hold_HC      = 15;   //ms  15
 int16_t AGC_attack       = 15;   //ms 
 int16_t AGC_release      = 1000 ;//ms 

//%lcf
 int16_t LCF_SW_m = 0; // 0 is off, 1 is LCF first then AGC, 2 is first AGC  then LCF
// int16_t LCF_Fp   = 5000; //Hz
// int16_t LCF_GAIN = 14418;//0.88*16384 scaled by 16384


//=========global varibles those are used in agc ===================//
//max with 48 samples
 int16_t  max_FN = 0;
 int16_t  d48i   = 0;
 uint16_t x_p    = 0;

//max delay
// int16_t  max_DN = 0;
// uint16_t *x_d   = 0;
 uint16_t x_d[26]={0};
 int16_t  x_d_i  = 0;

 uint16_t x_pm  = 0;//uint16_t
// uint16_t x_rp  = 0;//uint16_t
 uint16_t ox_rp = 0;//uint16_t  ori value:-1,same as 0

 int32_t agc_gain_p    = 0;       //scaled by 1024*1024 int32_t
 int32_t agc_gain_m    = 0 ;//scaled by 1024*1024 int32_t
 int32_t Hold_HC       = 0;
 int32_t Hold_H        = 0;


 int16_t agc_gain_l= 0;//scaled by 1024 int16_t
 int16_t agc_gain_r= 0;//scaled by 1024 int16_t
 int16_t o_L = 0;
 int16_t o_R = 0;


 int32_t data_dN  = 0;
 int16_t *d_dl    = 0;
 int16_t *d_dr    = 0;
 int32_t delay_i  = 0;

 int32_t gd_Attack=0;
 int32_t gd_Release=0;

//=========global varibles those are used in lcf ===================//
 int32_t LCF_L_D1 = 0;  
 int32_t LCF_L_D2 = 0;  
 int32_t LCF_R_D1 = 0;  
 int32_t LCF_R_D2 = 0;  
 int16_t s_cur_lcf_para_l[5] = {0, 0, 0, 0, 0};
 int16_t s_cur_lcf_s_gain_l  = 0;//sacled by 4096
 int16_t s_cur_lcf_para_r[5] = {0, 0, 0, 0, 0};
 int16_t s_cur_lcf_s_gain_r  = 0;//sacled by 4096


//================%dp for record 20120217%====================//
 BOOLEAN DP_SW          =  TRUE;  // 0 = disable;  1:AGC enable ; 
 BOOLEAN DP_ZC_SW       =  TRUE; // 1:zeros cross on;0:zeros cross off
 BOOLEAN LCF_SW_dp      =  FALSE; 
 int16_t   DP_input_gain  =  1*1024;//scaled by 1024 int16_t


//max with 48 samples
 int16_t  max_FN_dp = 0;
 int16_t  d48i_dp   = 0;
 uint16_t x_p_dp    = 0;

//max delay
 uint16_t x_d_dp[26]={0};
 int16_t  x_d_i_dp  = 0;

 uint16_t x_pm_dp  = 0;//uint16_t

//init
 int32_t agc_gain_p_dp    = 0;       //scaled by 1024*1024 int32_t
 int32_t agc_gain_m_dp    = 0 ;//scaled by 1024*1024 int32_t

 int32_t Hold_H_dp        = 0;


 int16_t agc_gain_l_dp= 0;//scaled by 1024 int16_t
 int16_t agc_gain_r_dp= 0;//scaled by 1024 int16_t
 int16_t o_L_dp = 0;
 int16_t o_R_dp = 0;


 int32_t data_dN_dp  = 0;
 int16_t *d_dl_dp    = 0;
 int16_t *d_dr_dp    = 0;
 int32_t delay_i_dp  = 0;


 int16_t DP_limit_up    = 0;  //%  uint: cdB
 int16_t DP_limit_down  = 0;  //%  uint: cdB

 int16_t COMPRESSOR_threshold = 0; //%uint: cdB
 int16_t COMPRESSOR_ratio     = 0; 
 int16_t EXPANDER_threshold   = 0; //%uint: cdB  -250
 int16_t EXPANDER_ratio       = 0; //%expander_ratio = ctan(<);scaled by 32768

 int32_t Hold_HC_Expander=0,Hold_HC_Compressor=0;
 int32_t gd_Attack_Compressor=0 , gd_Attack_Expander=0;
 int32_t gd_Release_Compressor=0 , gd_Release_Expander=0;

 BOOLEAN dp_is_expander = TRUE;

 int32_t CUR_LCF_L_D1_dp = 0; //%long
 int32_t CUR_LCF_L_D2_dp = 0; //%long
 int32_t CUR_LCF_R_D1_dp = 0; //%long
 int32_t CUR_LCF_R_D2_dp = 0; //%long

 int16_t s_cur_lcf_para_left_dp[3]   = {0,0,0};
 int16_t s_cur_lcf_para_right_dp[3] = {0,0,0};
 int16_t s_cur_lcf_s_gain_left_dp  = 0;//sacled by 4096
 int16_t s_cur_lcf_s_gain_right_dp  = 0;//sacled by 4096
//================================================//

//================%eq for record 20120309%====================//
 BOOLEAN RECORD_EQ_SW     =  FALSE; 
 BOOLEAN RECORD_EQ_STEREO =  FALSE; 
 BOOLEAN RECORD_EQ_BAND_SW[RECORDEQ_MAX_BAND] =  {0}; 
 int16_t s_cur_recordeq_master_gain = 0; //sacled by 1024

 int16_t s_cur_recordeq_para[RECORDEQ_MAX_BAND][5] = {0};
 int16_t s_cur_recordeq_s_gain[RECORDEQ_MAX_BAND]  = {0};//sacled by 4096
 int32_t CUR_RECORD_EQ_L_D1[RECORDEQ_MAX_BAND] = {0}; //%long
 int32_t CUR_RECORD_EQ_L_D2[RECORDEQ_MAX_BAND] = {0}; //%long
 int32_t CUR_RECORD_EQ_R_D1[RECORDEQ_MAX_BAND] = {0}; //%long
 int32_t CUR_RECORD_EQ_R_D2[RECORDEQ_MAX_BAND] = {0}; //%long
//============================================================//



/**---------------------------------------------------------------------------*
 **                     static Function definitions                             *
 **---------------------------------------------------------------------------*/
static void VB_SetHpfMode(BOOLEAN a)
{
}

static void VB_SetHpfParas(int16_t a0, int16_t a1, int16_t a2, int16_t a3, int16_t a4, int16_t a5, int16_t a6)
{
}

static void VB_SetHpfWidth(uint16_t width)
{
}

static void  VB_SetHpfGain(int16_t gIndex, int16_t gValue)
{
}

static void VB_SwitchHpf(BOOLEAN is_enable)
{
}

static uint32_t VB_GetHpfGain(int16_t g_index)
{
    return 0;		
}

static void VB_SetHpfLimit(int8_t limit)
{
}
			
/*****************************************************************************/
//  Description:    set input gain of agc (iAgcIngainSet)
//  Author:         Cherry.Liu
//  Note:           !attention! you should set params down between frames!  
//****************************************************************************/  

static BOOLEAN AUDPROC_SetAgcIng(
    int16_t input_gain,
    int16_t in_gain_set)
{
    int32_t temp=0;


    SCI_TRACE_LOW("aud_proc_config.c,[AUDPROC_SetAgcIng] AGC_SW_n:%d,input_gain:%d,in_gain_set:%d",
        AGC_SW_m,input_gain,in_gain_set); 

    temp = (input_gain * in_gain_set)>>12;
    if(temp<=32767)
    {
        AGC_input_Gain   = temp;
    }
    else
    {
        AGC_input_Gain   = 32767;
    }    

 
    return TRUE; 
}

/*****************************************************************************/
//  Description:    set EQ filter paras down
//  Author:         Cherry.Liu
//  Note:           !attention! you should set params down between frames!  
//****************************************************************************/  
static BOOLEAN AUDPROC_Seteq(
    EQMODE_FILTER_PARAM_T* eq_param_ptr,
    int32_t sample_rate)
{
    REC_IIR_FILTER_PARA_T  eq_filter_bass    = {0};
    REC_IIR_FILTER_PARA_T  eq_filter_treble  = {0};
    int16_t s_gain_bass   = 0;
    int16_t s_gain_treble = 0;    
    
    BOOLEAN is_need_fade = FALSE;
    
    uint32_t s0_cur_value = 0,s1_cur_value = 0 ,s2_cur_value = 0;
    uint32_t s0_step_value = 0,s1_step_value = 0 ,s2_step_value = 0;
    uint32_t step_time = 0;
    uint32_t i = 0;

    //calc filter 1 paras
    Rec_Filter_CalcEqCoeffs_Ex(eq_param_ptr->bass_fo,
        eq_param_ptr->bass_df, 
        eq_param_ptr->bass_boostdB, 
        eq_param_ptr->bass_gaindB, 
        sample_rate, 
        &eq_filter_bass, 
        &s_gain_bass);


    //trace filter 1 paras
    SCI_TRACE_LOW("aud_proc_config.c,[AUDPROC_Seteq]-bass-:fo:%d  df:%d  boost:%d  gain:%d",
        eq_param_ptr->bass_fo,eq_param_ptr->bass_df, eq_param_ptr->bass_boostdB,eq_param_ptr->bass_gaindB );
    

    SCI_TRACE_LOW("aud_proc_config.c,[AUDPROC_Seteq] eq bass:\nS:%d\nB:%d %d %d\r\n A:%d %d %d\r\n",s_gain_bass,
        eq_filter_bass.B0,eq_filter_bass.B1,eq_filter_bass.B2,
        eq_filter_bass.A0,eq_filter_bass.A1,eq_filter_bass.A2); 
    

    //calc filter 2 paras
    Rec_Filter_CalcEqCoeffs_Ex(eq_param_ptr->treble_fo,
        eq_param_ptr->treble_df, 
        eq_param_ptr->treble_boostdB, 
        eq_param_ptr->treble_gaindB, 
        sample_rate, 
        &eq_filter_treble, 
        &s_gain_treble);

    //trace filter 2 paras
    SCI_TRACE_LOW("aud_proc_config.c,[AUDPROC_Seteq]-treble-:fo:%d  df:%d  boost:%d  gain:%d",
        eq_param_ptr->treble_fo,eq_param_ptr->treble_df, eq_param_ptr->treble_boostdB,eq_param_ptr->treble_gaindB );
    
    SCI_TRACE_LOW("aud_proc_config.c,[AUDPROC_Seteq] \neq treble:\nS:%d\nB:%d %d %d\r\n A:%d %d %d\r\n",s_gain_treble,
        eq_filter_treble.B0,eq_filter_treble.B1,eq_filter_treble.B2,
        eq_filter_treble.A0,eq_filter_treble.A1,eq_filter_treble.A2);  


    //begin the eq paras setting process
    s2_cur_value = VB_GetHpfGain(2);
    s1_cur_value = VB_GetHpfGain(1);
    s0_cur_value = VB_GetHpfGain(0);
    
    SCI_TRACE_LOW("aud_proc_config.c,[AUDPROC_Seteq]:s2_cur_value:%d,s1_cur_value:%d,s0_cur_value:%d",
        s2_cur_value,s1_cur_value,s0_cur_value);
    
    if(0 != s0_cur_value)//the music is playing
    {
        is_need_fade = TRUE;
    }
    else //the music is stopped.
    {
        is_need_fade = FALSE;
    }

    
    if(is_need_fade)//s0,s1,s2 fade out to 0
    {
        s2_step_value   = s2_cur_value/EQ_PARA_SET_S2_SET_TIMES;
        s1_step_value   = s1_cur_value/EQ_PARA_SET_S2_SET_TIMES;
        s0_step_value   = s0_cur_value/EQ_PARA_SET_S2_SET_TIMES;
        step_time    = EQ_PARA_SET_DELAY_TIME/EQ_PARA_SET_S2_SET_TIMES;
        
        //fade out ,50ms
        for(i=1;i<=EQ_PARA_SET_S2_SET_TIMES;i++)
        {     
            VB_SetHpfGain(2, (s2_cur_value-i*s2_step_value));  
            VB_SetHpfGain(1, (s1_cur_value-i*s1_step_value));  
            VB_SetHpfGain(0, (s0_cur_value-i*s0_step_value)); 
            
            SCI_Sleep(step_time);
        }
        
        VB_SetHpfGain(2,0);
        VB_SetHpfGain(1,0);
        VB_SetHpfGain(0,0);
    }
    

    // clear eq  delay register by vbc
    VB_SetHpfMode(1);         
    SCI_Sleep(1);// the clear process need at least 10us 

    //clean eq delay register by filter itself 
    VB_SetHpfParas(1,0,0,0,0,0,0);
    VB_SetHpfParas(2,0,0,0,0,0,0);
    SCI_Sleep(2);

    //set filter paras in registers
    VB_SetHpfParas(1, eq_filter_bass.B0,  eq_filter_bass.B1, eq_filter_bass.B2,  
        16384,-eq_filter_bass.A1, -eq_filter_bass.A2); 

    VB_SetHpfParas(2, eq_filter_treble.B0,eq_filter_treble.B1,eq_filter_treble.B2, 
       16384, -eq_filter_treble.A1, -eq_filter_treble.A2) ;
        

    
    // finish clear process ;and the fileters begin working nornally
    VB_SetHpfMode(0);         
    
    //s0,s1,s2 fade in to its original value in 50ms
    s0_cur_value = s_gain_bass; 
    s1_cur_value = s_gain_treble; 


    s0_step_value   = s0_cur_value/EQ_PARA_SET_S2_SET_TIMES;
    s1_step_value   = s1_cur_value/EQ_PARA_SET_S2_SET_TIMES;
    s2_step_value   = s2_cur_value/EQ_PARA_SET_S2_SET_TIMES;
    step_time    = EQ_PARA_SET_DELAY_TIME/EQ_PARA_SET_S2_SET_TIMES;
    
    for(i=1;i<=EQ_PARA_SET_S2_SET_TIMES;i++)
    {     
        VB_SetHpfGain(0, (i*s0_step_value)); 
        VB_SetHpfGain(1, (i*s1_step_value));  
        VB_SetHpfGain(2, (i*s2_step_value));  
        
        SCI_Sleep(step_time);
    }
    
    VB_SetHpfGain(0, s0_cur_value);
    VB_SetHpfGain(1, s1_cur_value);
    VB_SetHpfGain(2, s2_cur_value);

    return TRUE;   
}

/*****************************************************************************/
//  Description:    set LCF filter paras down
//  Author:         Cherry.Liu
//  Note:           !attention! you should set params down between frames!  
//****************************************************************************/
static BOOLEAN AUDPROC_Setlcf(
    LCF_NV_PARAM_T * lcf_param_ptr,
    int32_t sample_rate)
{
    REC_FILTER_LCF_CALC_PARA_T lcf_para_set ={0};//in
    REC_IIR_FILTER_PARA_T  lcf_filter_set = {0};//out
    int16_t  sGain = 0;

        
    // calc lcf para 
    lcf_para_set.isFilterOn   = lcf_param_ptr->is_lcf_on;
    lcf_para_set.eLcfParaType = lcf_param_ptr->eLcfParaType;
    
    if(REC_FILTER_LCFPARA_BUTTERWORTH == lcf_para_set.eLcfParaType)
    {
        //left channel calc.
        lcf_para_set.unlcfPara.fp = lcf_param_ptr->fp_l;

        Rec_Filter_CalcLCF(&lcf_para_set, 
                lcf_param_ptr->lcf_gain_l, 
                sample_rate, 
                &lcf_filter_set, 
                &sGain);

        SCI_TRACE_LOW("aud_proc_config.c,[AUDPROC_Initlcf] lcf buttorworth left:S:%d\n, B:%d %d %d\r\n A:%d %d %d\r\n",sGain,
            lcf_filter_set.B0,lcf_filter_set.B1,lcf_filter_set.B2,
            lcf_filter_set.A0,lcf_filter_set.A1,lcf_filter_set.A2);   

        s_cur_lcf_para_l[0] = lcf_filter_set.B0; //BUT_B(1);
        s_cur_lcf_para_l[1] = lcf_filter_set.B1; //BUT_B(2);
        s_cur_lcf_para_l[2] = lcf_filter_set.B2; //BUT_B(3);
        s_cur_lcf_para_l[3] = -lcf_filter_set.A1;//-BUT_A(2);
        s_cur_lcf_para_l[4] = -lcf_filter_set.A2;//-BUT_A(3);

        s_cur_lcf_s_gain_l  = sGain;

        //right channel calc.
        lcf_para_set.unlcfPara.fp = lcf_param_ptr->fp_r;

        Rec_Filter_CalcLCF(&lcf_para_set, 
                lcf_param_ptr->lcf_gain_r, 
                sample_rate, 
                &lcf_filter_set, 
                &sGain);

        SCI_TRACE_LOW("aud_proc_config.c,[AUDPROC_Initlcf] lcf buttorworth right:S:%d\n, B:%d %d %d\r\n A:%d %d %d\r\n",sGain,
            lcf_filter_set.B0,lcf_filter_set.B1,lcf_filter_set.B2,
            lcf_filter_set.A0,lcf_filter_set.A1,lcf_filter_set.A2);   

        s_cur_lcf_para_r[0] = lcf_filter_set.B0; //BUT_B(1);
        s_cur_lcf_para_r[1] = lcf_filter_set.B1; //BUT_B(2);
        s_cur_lcf_para_r[2] = lcf_filter_set.B2; //BUT_B(3);
        s_cur_lcf_para_r[3] = -lcf_filter_set.A1;//-BUT_A(2);
        s_cur_lcf_para_r[4] = -lcf_filter_set.A2;//-BUT_A(3);

        s_cur_lcf_s_gain_r  = sGain;
        
    }
    else if(REC_FILTER_LCFPARA_F1F1 == lcf_para_set.eLcfParaType)
    {
        lcf_para_set.unlcfPara.lcfPara.f1_g0 = lcf_param_ptr->f1_g0;
        lcf_para_set.unlcfPara.lcfPara.f1_g1 = lcf_param_ptr->f1_g1;
        lcf_para_set.unlcfPara.lcfPara.f1_fp = lcf_param_ptr->f1_fp;
        lcf_para_set.unlcfPara.lcfPara.f2_g0 = lcf_param_ptr->f2_g0;
        lcf_para_set.unlcfPara.lcfPara.f2_g1 = lcf_param_ptr->f2_g1;
        lcf_para_set.unlcfPara.lcfPara.f2_fp = lcf_param_ptr->f2_fp;

        Rec_Filter_CalcLCF(&lcf_para_set, 
                16384, 
                sample_rate, 
                &lcf_filter_set, 
                &sGain);

        SCI_TRACE_LOW("aud_proc_config.c,[AUDPROC_Initlcf] lcf f1f1:S:%d\n, B:%d %d %d\r\n A:%d %d %d\r\n",sGain,
            lcf_filter_set.B0,lcf_filter_set.B1,lcf_filter_set.B2,
            lcf_filter_set.A0,lcf_filter_set.A1,lcf_filter_set.A2);   

        s_cur_lcf_para_l[0] = lcf_filter_set.B0; //BUT_B(1);
        s_cur_lcf_para_l[1] = lcf_filter_set.B1; //BUT_B(2);
        s_cur_lcf_para_l[2] = lcf_filter_set.B2; //BUT_B(3);
        s_cur_lcf_para_l[3] = -lcf_filter_set.A1;//-BUT_A(2);
        s_cur_lcf_para_l[4] = -lcf_filter_set.A2;//-BUT_A(3);
        s_cur_lcf_s_gain_l  = sGain;

        s_cur_lcf_para_r[0] = lcf_filter_set.B0; //BUT_B(1);
        s_cur_lcf_para_r[1] = lcf_filter_set.B1; //BUT_B(2);
        s_cur_lcf_para_r[2] = lcf_filter_set.B2; //BUT_B(3);
        s_cur_lcf_para_r[3] = -lcf_filter_set.A1;//-BUT_A(2);
        s_cur_lcf_para_r[4] = -lcf_filter_set.A2;//-BUT_A(3);
        s_cur_lcf_s_gain_r  = sGain;
    }
    else
    {
        SCI_TRACE_LOW("aud_proc_config.c,[AUDPROC_Initlcf] lcf type is %d.Not support!",lcf_para_set.eLcfParaType);

        s_cur_lcf_para_l[0] = 16384; 
        s_cur_lcf_para_l[1] = 0;  
        s_cur_lcf_para_l[2] = 0; 
        s_cur_lcf_para_l[3] = 0; 
        s_cur_lcf_para_l[4] = 0; 
        s_cur_lcf_s_gain_l  = 4096;

        s_cur_lcf_para_r[0] = 16384; 
        s_cur_lcf_para_r[1] = 0; 
        s_cur_lcf_para_r[2] = 0;  
        s_cur_lcf_para_r[3] = 0; 
        s_cur_lcf_para_r[4] = 0; 
        s_cur_lcf_s_gain_r  = 4096;

        return FALSE;
    }


    return TRUE;
}

/*****************************************************************************/
//  Description:    set LCF filter paras down
//  Author:         Cherry.Liu
//  Note:           !attention! you should set params down between frames!  
//****************************************************************************/
static BOOLEAN AUDPROC_Initlcf(
    LCF_NV_PARAM_T * lcf_param_ptr,
    int32_t sample_rate)
{
 
    if(lcf_param_ptr->is_lcf_on)
    {
        // init globals use in lcf process          
        LCF_L_D1 = 0; //long
        LCF_L_D2 = 0; //long

        LCF_R_D1 = 0; //long
        LCF_R_D2 = 0; //long
        
        if(lcf_param_ptr->is_lcf_before_agc)
        {
            LCF_SW_m = 1;
        }
        else
        {
            LCF_SW_m = 2;
        }

        return AUDPROC_Setlcf(lcf_param_ptr,sample_rate);
    }
    else
    {
        LCF_SW_m = 0;
        
        return TRUE;
    }
}


/*****************************************************************************/
//  Description:    init  agc module
//  Author:         Cherry.Liu
//  Note:           !attention! you should set params down between frames!  
//****************************************************************************/  
static BOOLEAN AUDPROC_initAgc(
    AGC_NV_PARAM_T *agc_param_ptr,
    int32_t sample_rate)
{
    int32_t Fs = sample_rate;
    int16_t  max_DN = 0;
    int32_t temp=0;

    //basic para setting
    AGC_SW_m             = agc_param_ptr->agc_sw;
    AGC_ZC_SW            = agc_param_ptr->agc_zc_sw;
    
   // AGC_input_Gain       = agc_param_ptr->agc_input_gain;
    temp = (agc_param_ptr->agc_input_gain * agc_param_ptr->agc_ingain_set)>>12;
    if(temp<=32767)
    {
        AGC_input_Gain   = temp;
    }
    else
    {
        AGC_input_Gain   = 32767;
    }   

    AGC_input_Gain_start = agc_param_ptr->agc_input_gain_start;
    AGC_delay            = agc_param_ptr->agc_delay;
    AGC_Hold_HC          = agc_param_ptr->agc_hold_hc;
    AGC_attack           = agc_param_ptr->agc_attack;
    AGC_release          = agc_param_ptr->agc_release;

    //varibles initiation
    max_FN = Fs/1000;
    d48i   = max_FN-1;
    x_p    = 0;

    
    max_DN = 26;//max_DN = (Fs/40)/max_FN+1;
    SCI_MEMSET(&x_d[0],0,max_DN*2);
    x_d_i = 25;

    x_pm  = 0; 
    ox_rp = 0; 


    agc_gain_p    = AGC_input_Gain<<10;       //scaled by 1024*1024 int32_t
    agc_gain_m    = AGC_input_Gain_start<<10 ;//scaled by 1024*1024 int32_t
    if(agc_gain_m>agc_gain_p)
    {
        agc_gain_m = agc_gain_p;
    }
    
    Hold_HC       = AGC_Hold_HC*Fs/1000;
    Hold_H        = 0;


    agc_gain_l    = AGC_input_Gain;//scaled by 1024 int16_t
    agc_gain_r    = AGC_input_Gain;//scaled by 1024 int16_t
    o_L           = 0;
    o_R           = 0;

    //if d_dl&d_dr are allocated already,then free them. 
    if(d_dl)
    {
        SCI_Free(d_dl);  
        d_dl = SCI_NULL;
    }

    if(d_dr)
    {
        SCI_Free(d_dr);
        d_dr = SCI_NULL;
    }
    
    data_dN = AGC_delay * Fs/1000;
    if(data_dN<=0)
    {
        data_dN=1;
    }
    
    d_dl = (int16_t*)SCI_ALLOC(data_dN*2); 
    if(d_dl != SCI_NULL)
    {
        SCI_MEMSET(d_dl,0,data_dN*2);
    }
    else
    {
        return FALSE;
    }
    d_dr = (int16_t*)SCI_ALLOC(data_dN*2);
    if(d_dr != SCI_NULL)
    {
        SCI_MEMSET(d_dr,0,data_dN*2);
    }
    else
    {
        return FALSE;
    }
    delay_i = data_dN-1;
    
    if(AGC_attack<=0)
    {
        AGC_attack = 1;
    }
    if(AGC_release<=0)
    {
        AGC_release = 1;
    }

    if(AGC_input_Gain>1024)
    {
        gd_Attack  = (AGC_input_Gain*1024-1024*1024)/(Fs*AGC_attack/1000);//scaled by 1024*1024 int32_t
        gd_Release = (AGC_input_Gain*1024-1024*1024)/(Fs*AGC_release/1000);//scaled by 1024*1024 int32_t

    }
    else
    {
        AGC_SW_m     =  FALSE; //AGC sw off automatic
        
        gd_Attack  = (AGC_input_Gain*1024)/(Fs*AGC_attack/1000);//scaled by 1024*1024 int32_t
        gd_Release = (AGC_input_Gain*1024)/(Fs*AGC_release/1000);//scaled by 1024*1024 int32_t
    }

    if(gd_Attack<=0)
        gd_Attack = 64;

    if(gd_Release<=0)
        gd_Release = 64;

    return TRUE;

}

/*****************************************************************************/
//  Description:    set EQ filter paras down
//  Author:         Cherry.Liu
//  Note:           !attention! you should set params down between frames!  
//****************************************************************************/ 
static BOOLEAN AUDPROC_Initeq(
    EQMODE_NV_PARAM_T* eq_param_ptr,
    int32_t sample_rate)
{
    BOOLEAN return_value = FALSE;

        
    if(eq_param_ptr->hpf_sw) //eq on :bit11
    {
        SCI_TRACE_LOW("[AUDPROC_Initeq]  eq is on!");

        VB_SetHpfLimit((int8_t)(eq_param_ptr->limit)); //RLimit:bit7~0
        VB_SetHpfWidth(24);       //IIS_Bits_select  
        //if(0 == VB_GetHpfGain(2))//first set when the eq is started
        {
            VB_SwitchHpf(1);          //HPF ENABLE
        }

        return_value = AUDPROC_Seteq(&eq_param_ptr->filter_para,sample_rate);
        if(!return_value)
        {
            SCI_TRACE_LOW("[AUDPROC_Initeq] set eq failed!");
            return FALSE;   
        }
    }
    else
    {
        SCI_TRACE_LOW("[AUDPROC_Initeq]  eq is off!");

        //set s0,s1,s2 to zero
       // VB_SetHpfGain(2, 0);
        VB_SetHpfGain(0, 0);
        VB_SetHpfGain(1, 0);
        
        //clean delay register
        VB_SetHpfMode(1);         // clear hpf delay register 
        SCI_Sleep(1);             // the clear process need at least 10us 
        VB_SetHpfMode(0);         // finish clear process

        //shut down hpf
        VB_SwitchHpf(0); 
    }

    return TRUE;
}



/**---------------------------------------------------------------------------*
 **                          function definitions                       *
 **---------------------------------------------------------------------------*/
/*****************************************************************************/
//  Description:    set digital gain of agc
//  Author:         Cherry.Liu
//  Note:           !attention! you should set params down between frames!  
//****************************************************************************/
 BOOLEAN AUDPROC_SetAgcDg( int16_t digtal_gain)
{
    uint32_t u32_dg        = 0; 
    uint8_t  s_agc_dg_Gi  = 0;
    uint8_t  s_agc_dg_M   = 0;
    int32_t  temp=0;


    SCI_TRACE_LOW("aud_proc_config.c,[AUDPROC_SetAgcDg] digtal_gain:%d", digtal_gain);

    //digtal_gain
    if(digtal_gain > 32)
    {
        digtal_gain = 32;
    }
    
    u32_dg = 32 - digtal_gain;
    
    s_agc_dg_Gi = AGC_G75[u32_dg & 0x7];
    s_agc_dg_M  = (u32_dg >> 3) & 0xF;   

    //set this gain to s2  
    temp = (((int32_t)4096*s_agc_dg_Gi))>>(s_agc_dg_M+3) ; 
     
    if(temp>32767)
    {
        temp = 32767;
    }
    
    //VB_SetHpfGain(2, (int16_t)temp); 


    return TRUE;
    
}



/*****************************************************************************/
//  Description:    set eq mode para for audio process modules, including lcf,agc and eq.
//  Author:         Cherry.Liu
//  Note:           !attention! you should call it between data frames. 
//****************************************************************************/  
 BOOLEAN AUDPROC_Set(
    int32_t sample_rate,
    LCF_NV_PARAM_T *lcf_param_ptr,
    AGC_NV_PARAM_T * agc_param_ptr,
    EQMODE_FILTER_PARAM_T* eq_param_ptr
)
{
    BOOLEAN set_return_value = FALSE;

	if(sample_rate<=0)
		return FALSE;
    
    set_return_value = AUDPROC_Setlcf(lcf_param_ptr,
        sample_rate);

    if(!set_return_value)
    {
        SCI_TRACE_LOW("aud_proc_config.c,[AUDPROC_Set] lcf set failed");
        return FALSE;
    }

    set_return_value = AUDPROC_SetAgcIng(agc_param_ptr->agc_input_gain,
        agc_param_ptr->agc_ingain_set);

    if(!set_return_value)
    {
        SCI_TRACE_LOW("aud_proc_config.c,[AUDPROC_Set] agc set failed");
        return FALSE;
    }

    set_return_value = AUDPROC_Seteq(eq_param_ptr,
        sample_rate);

    if(!set_return_value)
    {
        SCI_TRACE_LOW("aud_proc_config.c,[AUDPROC_Set] eq set failed");
        return FALSE;
    }

    return TRUE;
    
}


/*****************************************************************************/
//  Description:    init audio process modules including lcf,agc and eq.
//  Author:         Cherry.Liu
//  Note:           !attention! you should init it before music start. 
//****************************************************************************/  
 BOOLEAN AUDPROC_Init(
    int32_t sample_rate,
    LCF_NV_PARAM_T * lcf_param_ptr,
    AGC_NV_PARAM_T * agc_param_ptr,
    EQMODE_NV_PARAM_T * eq_param_ptr
)
{

    BOOLEAN init_return_value = FALSE;

	if(sample_rate<=0)
		return FALSE;

    init_return_value = AUDPROC_Initlcf(lcf_param_ptr, 
        sample_rate);

    if(!init_return_value)
    {
        SCI_TRACE_LOW("aud_proc_config.c,[AUDPROC_Init] lcf init failed");
        return FALSE;
    }


    init_return_value = AUDPROC_initAgc(agc_param_ptr,
        sample_rate);
    
    if(!init_return_value)
    {
        SCI_TRACE_LOW("aud_proc_config.c,[AUDPROC_Init] agc init failed");
        return FALSE;
    }


    init_return_value = AUDPROC_Initeq( 
        eq_param_ptr,
        sample_rate);


    if(!init_return_value)
    {
        SCI_TRACE_LOW("aud_proc_config.c,[AUDPROC_Init] eq init failed");
        return FALSE;
    }


    return TRUE;
}

/*****************************************************************************/
//  Description:    deinti aud proc plugger
//  Author:         Cherry.Liu
//  Note:           
//****************************************************************************/  
 BOOLEAN AUDPROC_DeInit(
    void
)
{
    //set s0,s1,s2 to zero
    VB_SetHpfGain(2, 0);
    VB_SetHpfGain(0, 0);
    VB_SetHpfGain(1, 0);
  

    //clean delay register
    VB_SetHpfMode(1);         // clear hpf delay register 
    SCI_Sleep(1);             // the clear process need at least 10us 
    VB_SetHpfMode(0);         // finish clear process

    //shut down hpf
    VB_SwitchHpf(0); 

    if(d_dl)
    {
        SCI_Free(d_dl);  
        d_dl = SCI_NULL;
    }

    if(d_dr)
    {
        SCI_Free(d_dr);
        d_dr = SCI_NULL;
    }

    return TRUE;
}


/*****************************************************************************/
//  Description:    init  dp module
//  Author:         Cherry.Liu
//  Note:           !attention! you should set params down between frames!  
//****************************************************************************/  
 BOOLEAN AUDPROC_initRecordEq(
    RECORDEQ_CONTROL_PARAM_T *recordeq_param_ptr,
    int32_t       Fs)
{
    REC_IIR_FILTER_PARA_T  recordeq_filter_set[RECORDEQ_MAX_BAND] = {0};//out
    int16_t  i=0;

    //RECORD_EQ_SW = recordeq_param_ptr->RECORDEQ_SW;
    //RECORD_EQ_STEREO = recordeq_param_ptr->RECORDEQ_STEREO;
    RECORD_EQ_SW     = ((recordeq_param_ptr->RECORDEQ_sw_switch) & (1 << 15)) ? TRUE : FALSE;//bit 15
    RECORD_EQ_STEREO = ((recordeq_param_ptr->RECORDEQ_sw_switch) & (1 << 14)) ? TRUE : FALSE;//bit 14

	if (!RECORD_EQ_SW && !RECORD_EQ_STEREO)
    {
        SCI_TRACE_LOW("Warning: RECORD_EQ NOT init!!!");
        return FALSE;
    }
	
    if(RECORD_EQ_SW)
    {
        for(i=0;i<RECORDEQ_MAX_BAND;i++)//i :band index
        {

            //RECORD_EQ_BAND_SW[i]  = recordeq_param_ptr->RECORDEQ_BAND_SW[i];
            RECORD_EQ_BAND_SW[i]  = ((recordeq_param_ptr->RECORDEQ_sw_switch) & (1 << (8+i))) ? TRUE : FALSE;//bit (8+i);
            
            Filter_CalcRecordEq(
                RECORD_EQ_BAND_SW[i],                
            	recordeq_param_ptr->RECORDEQ_band_para[i].fo,                      
            	recordeq_param_ptr->RECORDEQ_band_para[i].df,                       
            	recordeq_param_ptr->RECORDEQ_band_para[i].boost,                  
                recordeq_param_ptr->RECORDEQ_band_para[i].gain,               
            	Fs,                      
            	&recordeq_filter_set[i],  
            	&s_cur_recordeq_s_gain[i]);
/*
            SCI_TRACE_LOW("record eq [%d]\n:S:%d\n, B:%d %d %d\r\n A:%d %d %d\r\n",i,s_cur_recordeq_s_gain[i],
                recordeq_filter_set[i].B0,recordeq_filter_set[i].B1,recordeq_filter_set[i].B2,
                recordeq_filter_set[i].A0,recordeq_filter_set[i].A1,recordeq_filter_set[i].A2);   
*/
	  SCI_TRACE_LOW("record eq [%d]\n, mastergain(%d)\n, fo(%d), df(%d), boost(%d), gain(%d)\n",
	  	i,
	  	 recordeq_param_ptr->RECORDEQ_master_gain,
                recordeq_param_ptr->RECORDEQ_band_para[i].fo,
                recordeq_param_ptr->RECORDEQ_band_para[i].df,
               recordeq_param_ptr->RECORDEQ_band_para[i].boost,
               recordeq_param_ptr->RECORDEQ_band_para[i].gain
		);
                  
            s_cur_recordeq_para[i][0] = recordeq_filter_set[i].B0; 
            s_cur_recordeq_para[i][1] = recordeq_filter_set[i].B1;  
            s_cur_recordeq_para[i][2] = recordeq_filter_set[i].B2;  
            s_cur_recordeq_para[i][3] = -recordeq_filter_set[i].A1; 
            s_cur_recordeq_para[i][4] = -recordeq_filter_set[i].A2; 

            CUR_RECORD_EQ_L_D1[i] = 0;  
            CUR_RECORD_EQ_L_D2[i] = 0;  
            CUR_RECORD_EQ_R_D1[i] = 0;  
            CUR_RECORD_EQ_R_D2[i] = 0; 


        }

        s_cur_recordeq_master_gain = recordeq_param_ptr->RECORDEQ_master_gain;

    }


    return TRUE;
}



/*****************************************************************************/
//  Description:    init  dp module
//  Author:         Cherry.Liu
//  Note:           !attention! you should set params down between frames!  
//****************************************************************************/  
 BOOLEAN AUDPROC_initDp(
    DP_CONTROL_PARAM_T *dp_param_ptr,
    int32_t       Fs)
{
    int16_t  max_DN_dp = 0;
    REC_FILTER_LCF_CALC_PARA_T lcf_para_set ={0};//in
    REC_IIR_FILTER_PARA_T  lcf_filter_set = {0};//out
    int16_t  sGain = 0;//out
    
    int16_t  COMPRESSOR_attack   = dp_param_ptr->COMPRESSOR_attack;  
    int16_t  COMPRESSOR_hold     = dp_param_ptr->COMPRESSOR_hold;  
    int16_t  COMPRESSOR_release  = dp_param_ptr->COMPRESSOR_release ;  

    int16_t  EXPANDER_attack     = dp_param_ptr->EXPANDER_attack;  
    int16_t  EXPANDER_hold       = dp_param_ptr->EXPANDER_hold;  
    int16_t  EXPANDER_release    = dp_param_ptr->EXPANDER_release;  
    int16_t  DP_sdelay           = dp_param_ptr->DP_sdelay;

    //para set by nv
    //DP_SW          =  dp_param_ptr->DP_SW;   
    //DP_ZC_SW       =  dp_param_ptr->DP_ZC_SW;  
    //LCF_SW_dp      =  dp_param_ptr->DP_lcf_sw;
    //> bit[0] of DP_sw_switch stands for  DP_SW,
    //> bit[1] of DP_sw_switch stands for DP_ZC_SW
    //> bit[2] of DP_sw_switch stands for LCF_SW_dp
    DP_SW            =    (dp_param_ptr->DP_sw_switch & 0x01)? TRUE : FALSE;
    DP_ZC_SW      =   (dp_param_ptr->DP_sw_switch & 0x02)? TRUE : FALSE;
    LCF_SW_dp     =   (dp_param_ptr->DP_sw_switch & 0x04)? TRUE : FALSE;
    if (!DP_SW && !DP_ZC_SW && !LCF_SW_dp)
    {
        SCI_TRACE_LOW("Warning: AUDPROC_DP NOT init!!!");
        return FALSE;
    }	
    DP_input_gain  =  dp_param_ptr->DP_input_gain; 

    DP_limit_up    = dp_param_ptr->DP_limit_up;  //%  uint: cdB
    DP_limit_down  = dp_param_ptr->DP_limit_down;  //%  uint: cdB

    COMPRESSOR_threshold = dp_param_ptr->COMPRESSOR_threshold; //%uint: cdB
    COMPRESSOR_ratio     = dp_param_ptr->COMPRESSOR_ratio; 
    EXPANDER_threshold   = dp_param_ptr->EXPANDER_threshold; //%uint: cdB  -250
    EXPANDER_ratio       = dp_param_ptr->EXPANDER_ratio; //%expand
 
    
    //%======================init init init===========================%
    //%max with 48 samples
    max_FN_dp = Fs/1000;
    d48i_dp   = max_FN_dp-1;
    x_p_dp    = 0;

    //%max delay
    max_DN_dp = 26;
    SCI_MEMSET(&x_d_dp[0],0,max_DN_dp*sizeof(uint16_t));
    x_d_i_dp = 25;
    
    x_pm_dp  = 0;

    //%init
    agc_gain_p_dp    = DP_input_gain<<10 ;
    agc_gain_m_dp    = DP_input_gain<<10 ;

    //%compressor & %expander
    Hold_HC_Compressor     = COMPRESSOR_hold*Fs/1000;
    Hold_HC_Expander       = EXPANDER_hold*Fs/1000;
    Hold_H_dp              = 0;
     
    //check para
    if(COMPRESSOR_attack<=0)
    {
        COMPRESSOR_attack = 1;
    }
    if(COMPRESSOR_release<=0)
    {
        COMPRESSOR_release = 1;
    }

    if(EXPANDER_attack<=0)
    {
        EXPANDER_attack = 1;
    }
    if(EXPANDER_release<=0)
    {
        EXPANDER_release = 1;
    }
    
    if(DP_input_gain>1024)
    {
        gd_Attack_Compressor  = ((DP_input_gain*1024-1024*1024)/(Fs*COMPRESSOR_attack/1000)); //%scaled by 1024*1024 int32_t
        gd_Release_Compressor = ((DP_input_gain*1024-1024*1024)/(Fs*COMPRESSOR_release/1000));//%scaled by 1024*1024 int32_t
        
        gd_Attack_Expander  = ((DP_input_gain*1024-1024*1024)/(Fs*EXPANDER_attack/1000)); //%scaled by 1024*1024 int32_t
        gd_Release_Expander = ((DP_input_gain*1024-1024*1024)/(Fs*EXPANDER_release/1000));//%scaled by 1024*1024 int32_t
    }
    else
    {
        gd_Attack_Compressor  = ((DP_input_gain*1024)/(Fs*COMPRESSOR_attack/1000)); //%scaled by 1024*1024 int32_t
        gd_Release_Compressor = ((DP_input_gain*1024)/(Fs*COMPRESSOR_release/1000));//%scaled by 1024*1024 int32_t
        
        gd_Attack_Expander  = ((DP_input_gain*1024)/(Fs*EXPANDER_attack/1000)); //%scaled by 1024*1024 int32_t
        gd_Release_Expander = ((DP_input_gain*1024)/(Fs*EXPANDER_release/1000));//%scaled by 1024*1024 int32_t
    }


    if(gd_Attack_Compressor<=0)
        gd_Attack_Compressor = 64;
    
    if(gd_Release_Compressor<=0)
        gd_Release_Compressor = 64;
    
    if(gd_Attack_Compressor<=0)
        gd_Attack_Compressor = 64;
    
    if(gd_Release_Expander<=0)
        gd_Release_Expander = 64;

    agc_gain_l_dp    = DP_input_gain;
    agc_gain_r_dp    = DP_input_gain;
    o_L_dp = 0;
    o_R_dp = 0;

    //if d_dl&d_dr are allocated already,then free them. 
    if(d_dl_dp)
    {
        SCI_Free(d_dl_dp);  
        d_dl_dp = SCI_NULL;
    }

    if(d_dr_dp)
    {
        SCI_Free(d_dr_dp);
        d_dr_dp = SCI_NULL;
    }
    
    data_dN_dp = DP_sdelay * Fs/1000;
    if(data_dN_dp<=0)
    {
        data_dN_dp=1;
    }
    d_dl_dp = (int16_t*)SCI_ALLOC(data_dN_dp*sizeof(int16_t));
    if(d_dl_dp != SCI_NULL)
    {
        SCI_MEMSET(d_dl_dp,0,data_dN_dp*sizeof(int16_t));
    }
    else
    {
        return FALSE;
    }
    
    d_dr_dp = (int16_t*)SCI_ALLOC(data_dN_dp*sizeof(int16_t));
    if(d_dr_dp != SCI_NULL)
    {
        SCI_MEMSET(d_dr_dp,0,data_dN_dp*sizeof(int16_t));
    }
    else
    {
        return FALSE;
    }
    delay_i_dp = data_dN_dp-1;

    dp_is_expander    =  TRUE;//% 1: expander;  0:compressor  BOOLEAN

    if(LCF_SW_dp)
    {
        lcf_para_set.isFilterOn   = TRUE;
        lcf_para_set.eLcfParaType = REC_FILTER_LCFPARA_BUTTERWORTH;
        
        //left channel calc.
        lcf_para_set.unlcfPara.fp = dp_param_ptr->DP_lcf_fp_l;

        Rec_Filter_CalcLCF(&lcf_para_set, 
                dp_param_ptr->DP_lcf_gain_l, 
                Fs, 
                &lcf_filter_set, 
                &sGain);

        SCI_TRACE_LOW("DP  lcf  left:S:%d\n, B:%d %d %d\r\n A:%d %d %d\r\n",sGain,
            lcf_filter_set.B0,lcf_filter_set.B1,lcf_filter_set.B2,
            lcf_filter_set.A0,lcf_filter_set.A1,lcf_filter_set.A2);   

        s_cur_lcf_para_left_dp[0] = lcf_filter_set.B0; //BUT_B(1);
        s_cur_lcf_para_left_dp[1] = lcf_filter_set.A1; //BUT_A(2);
        s_cur_lcf_para_left_dp[2] = lcf_filter_set.A2; //BUT_A(3);

        s_cur_lcf_s_gain_left_dp  = sGain;

        //right channel calc.
        lcf_para_set.unlcfPara.fp = dp_param_ptr->DP_lcf_fp_r;

        Rec_Filter_CalcLCF(&lcf_para_set, 
                dp_param_ptr->DP_lcf_gain_r,
                Fs, 
                &lcf_filter_set, 
                &sGain);

        SCI_TRACE_LOW("DP  lcf  right:S:%d\n, B:%d %d %d\r\n A:%d %d %d\r\n",sGain,
            lcf_filter_set.B0,lcf_filter_set.B1,lcf_filter_set.B2,
            lcf_filter_set.A0,lcf_filter_set.A1,lcf_filter_set.A2);   

        s_cur_lcf_para_right_dp[0] = lcf_filter_set.B0; //BUT_B(1);
        s_cur_lcf_para_right_dp[1] = lcf_filter_set.A1; //BUT_A(2);
        s_cur_lcf_para_right_dp[2] = lcf_filter_set.A2; //BUT_A(3);

        s_cur_lcf_s_gain_right_dp  = sGain;

        CUR_LCF_L_D1_dp = 0; //%long
        CUR_LCF_L_D2_dp = 0; //%long
        CUR_LCF_R_D1_dp = 0; //%long
        CUR_LCF_R_D2_dp = 0; //%long
        
    }

    
    return TRUE;
}

/*****************************************************************************/
//  Description:    deinit  dp  module
//  Author:         Cherry.Liu
//  Note:             
//****************************************************************************/  
 BOOLEAN AUDPROC_DeInitDp(
    void
)
{
    if(d_dl_dp)
    {
        SCI_Free(d_dl_dp);   
        d_dl_dp = SCI_NULL;
    }

    if(d_dr_dp)
    {
        SCI_Free(d_dr_dp);
        d_dr_dp = SCI_NULL;
    }

    return TRUE;
}

/*****************************************************************************/
//  Description: AUDPROC_ProcessDp: (digital gain) + (dynamic processor) +(6-band eq) + (lcf filter)
//  Author:      Cherry.Liu
//****************************************************************************/
void  AUDPROC_ProcessDp(
    int16_t* psSrcLeftData,
    int16_t* psSrcRightData,
    uint32_t uiSrcCount, 
    int16_t* psDestLeftData, 
    int16_t* psDestRightData, 
    uint32_t* puiDestCount
)
{
    int16_t  si  = 0 , i=0;
    int16_t  sl=0,sr=0,d_L=0,d_R=0;
    uint16_t mabslr=0,x_p_del=0;
    uint16_t x_rp_dp  = 0;//uint16_t
    
    //int32_t  xin_L       = 0; 
    int64  xin_L       = 0; //test code cherry
    int16_t  xin_L_H     = 0;//higher word
    int16_t  xin_L_L     =  0;//lower word
    
    //int32_t  xin_R       = 0; 
    int64  xin_R       = 0; 
    int16_t  xin_R_H      =  0;//higher word
    int16_t  xin_R_L     =  0;//lower word
    
    //int32_t  xout_L      = 0; 
    int64  xout_L       = 0; //test code cherry
    int16_t  xout_L_H   =  0;//higher word
    int16_t  xout_L_L   =  0;//lower word
    
    //int32_t  xout_R      = 0; 
    int64  xout_R      = 0; 
    int16_t  xout_R_H   =  0;//higher word
    int16_t  xout_R_L   =  0;//lower word

    int16_t  lcf_gain_l = 0;//scaled by 1024 
    int16_t  lcf_gain_r = 0;  //scaled by 1024
    //int32_t  out_left = 0;
    //int32_t  out_right = 0;   
    int64  out_left = 0;
    int64  out_right = 0; 


    int16_t  power_log = 0,DP_ingain_cdB = 0,DP_in_power = 0;
    int32_t  dp_out_power = 0;
    int32_t  attack_gain = 0,release_gain = 0;
    
    for(si=0;si<uiSrcCount;si++)//AUDIO POST PROCESS
    {
        //get decoder left and right
        sl = psSrcLeftData[si];// s(si,1);
        sr = psSrcRightData[si];// s(si,2);

        //AGC
        if(DP_SW)
        {
            //%max(abs(l,r))
            mabslr = max(abs(sl),abs(sr));
            
            //%peak;
            d48i_dp = d48i_dp -1;
            if(d48i_dp>=0)
            {
                if(mabslr>x_p_dp)
                {
                    x_p_dp = mabslr;
                }
            }
            else
            {
                d48i_dp = max_FN_dp-1;
            
                x_p_del = x_d_dp[x_d_i_dp];
                x_d_dp[x_d_i_dp] = x_p_dp;
                x_d_i_dp = x_d_i_dp -1;
                if(x_d_i_dp < 0)
                {
                    x_d_i_dp = 25;
                }

                if(x_p_del != x_pm_dp)
                {
                    if(x_p_dp>x_pm_dp)
                    {
                        x_pm_dp = x_p_dp;
                    }
                }
                else
                {
                    x_pm_dp  = x_d_dp[25];
                    for(i=24;i>=0;i--)
                    {
                        if(x_d_dp[i]>x_pm_dp)
                        {
                            x_pm_dp = x_d_dp[i];
                        }
                    }
                }

                x_p_dp  = mabslr;
            }

            //%peak
            x_rp_dp = max(x_pm_dp,x_p_dp);

            //%power estimate
            power_log     =  F200log10(x_rp_dp) - 903;   //% int16_t;   
            DP_ingain_cdB =  F200log10(DP_input_gain) - 602;
            DP_in_power   =  (power_log + DP_ingain_cdB); //% cdB

            
            //%gain update  
            if(DP_in_power > COMPRESSOR_threshold)   //%     tc < in     COMPRESSOR
            {
                dp_is_expander = FALSE;
                dp_out_power = COMPRESSOR_threshold + ((DP_in_power - COMPRESSOR_threshold)*COMPRESSOR_ratio>>15);
                if(dp_out_power> DP_limit_up)
                    dp_out_power = DP_limit_up ;
                agc_gain_p_dp = DP_input_gain*F32768power10(DP_in_power - dp_out_power)>>5; //%scaled by 1024*1024
            }
            else if(DP_in_power >= EXPANDER_threshold) //%    te <= in <= tc
            {
                agc_gain_p_dp = DP_input_gain<<10;
            }
            else                                      //%    in < te     EXPANDER
            {
                dp_is_expander = TRUE;
                dp_out_power = EXPANDER_threshold - ((EXPANDER_threshold - DP_in_power)<<15)/EXPANDER_ratio;
                if(dp_out_power >= DP_limit_down)
                {
                    agc_gain_p_dp = DP_input_gain*F32768power10(DP_in_power - dp_out_power)>>5; //%scaled by 1024*1024
                }
                else
                {
                    agc_gain_p_dp = 0;
                }  
            }
        }
        else
        {
            agc_gain_p_dp = DP_input_gain<<10;
        }
        

        //%gain smooth
        if(agc_gain_p_dp < agc_gain_m_dp)
        {
            if(dp_is_expander)
            {
                //%expander attack
                attack_gain = agc_gain_m_dp - gd_Attack_Expander;

                
                if(attack_gain<agc_gain_p_dp)
                    agc_gain_m_dp = agc_gain_p_dp;
                else
                    agc_gain_m_dp = attack_gain;
                

                Hold_H_dp  = Hold_HC_Expander;
            }
            else
            {
                //%compressor attack
                attack_gain = agc_gain_m_dp - gd_Attack_Compressor;
                
                if(attack_gain<agc_gain_p_dp)
                    agc_gain_m_dp = agc_gain_p_dp; 
                else
                    agc_gain_m_dp = attack_gain;
                

                Hold_H_dp  = Hold_HC_Compressor;
            }
        }
        else if(agc_gain_p_dp > agc_gain_m_dp)
        {
            Hold_H_dp = Hold_H_dp -1;//%hold
            if(Hold_H_dp<=0)
            {
                Hold_H_dp  = 0;
                
                //%release
                if(dp_is_expander)
                {
                    release_gain = agc_gain_m_dp + gd_Release_Expander;
                    
                    if(release_gain >agc_gain_p_dp)
                        agc_gain_m_dp = agc_gain_p_dp;
                    else
                        agc_gain_m_dp = release_gain;
                    
                }
                else
                {
                    release_gain = agc_gain_m_dp + gd_Release_Compressor;

                    if(release_gain >agc_gain_p_dp)
                        agc_gain_m_dp = agc_gain_p_dp;
                    else
                        agc_gain_m_dp = release_gain;
                }
            }
        }
    
        //%delay
        d_L = d_dl_dp[delay_i_dp];
        d_dl_dp[delay_i_dp] = sl;
        
        d_R = d_dr_dp[delay_i_dp];
        d_dr_dp[delay_i_dp] = sr;
        
        delay_i_dp = delay_i_dp -1;
        if(delay_i_dp <0)
            delay_i_dp = data_dN_dp-1;

        
        //%zero cross
        if(DP_ZC_SW)
        {

            if(o_L_dp*d_L<=0)//other method to determin?
            {
                agc_gain_l_dp = (agc_gain_m_dp>>10);
            }
            o_L_dp = d_L;

            if(o_R_dp*d_R<=0)
            {
                agc_gain_r_dp = (agc_gain_m_dp>>10);
            }
            o_R_dp = d_R; 
        }
        else
        {
            agc_gain_l_dp = (agc_gain_m_dp>>10);
            agc_gain_r_dp = (agc_gain_m_dp>>10);
        }

        //add alc output gain & shift (LEFT 8 bits)
        //xout_L = (agc_gain_l_dp*d_L>>10)<<8;
        //xout_R = (agc_gain_r_dp*d_R>>10)<<8;
        xout_L = (agc_gain_l_dp*d_L>>2);
        xout_R = (agc_gain_r_dp*d_R>>2);
        
        if(RECORD_EQ_SW)// xout_L xout_R ==>>  out_left & out_right
        {
            for(i=0;i<RECORDEQ_MAX_BAND;i++)//i :band index
            {
                if(RECORD_EQ_BAND_SW[i])
                {
                    xin_L      = xout_L*s_cur_recordeq_s_gain[i]>>12;          
                    xin_L_H    = xin_L >> 15;   
                    xin_L_L    = xin_L-(xin_L_H<<15);

                    //step1 
                    xout_L   =  CUR_RECORD_EQ_L_D1[i] + (xin_L_H*s_cur_recordeq_para[i][0]<<1) +  ((xin_L_L*s_cur_recordeq_para[i][0]+8192)>>14) ; 
                    xout_L_H =  xout_L >> 15;
                    xout_L_L = xout_L - (xout_L_H<<15);
                    
                    //step2
                    CUR_RECORD_EQ_L_D1[i] =  CUR_RECORD_EQ_L_D2[i]  + (xin_L_H*s_cur_recordeq_para[i][1]<<1) + ((xin_L_L*s_cur_recordeq_para[i][1]+8192)>>14)  + (xout_L_H*s_cur_recordeq_para[i][3]<<1) + ((xout_L_L*s_cur_recordeq_para[i][3]+8192)>>14);

                    //step3
                    CUR_RECORD_EQ_L_D2[i] =               (xin_L_H*s_cur_recordeq_para[i][2]<<1) + ((xin_L_L*s_cur_recordeq_para[i][2]+8192)>>14)  + (xout_L_H*s_cur_recordeq_para[i][4]<<1) + ((xout_L_L*s_cur_recordeq_para[i][4]+8192)>>14);
                } 
            }
            out_left = xout_L*s_cur_recordeq_master_gain>>10;

            if(RECORD_EQ_STEREO)
            {
                for(i=0;i<RECORDEQ_MAX_BAND;i++)//i :band index
                {
                    if(RECORD_EQ_BAND_SW[i])
                    {
                        xin_R      = xout_R*s_cur_recordeq_s_gain[i]>>12;
                        xin_R_H    = xin_R >> 15;       
                        xin_R_L    = xin_R - (xin_R_H<<15);

                        //step1
                        xout_R   =  CUR_RECORD_EQ_R_D1[i] + (xin_R_H*s_cur_recordeq_para[i][0]<<1) + ((xin_R_L*s_cur_recordeq_para[i][0]+8192)>>14) ;
                        xout_R_H = xout_R >> 15;
                        xout_R_L = xout_R - (xout_R_H<<15);
                        
                        //step2
                        CUR_RECORD_EQ_R_D1[i] =  CUR_RECORD_EQ_R_D2[i]  + (xin_R_H*s_cur_recordeq_para[i][1]<<1) + ((xin_R_L*s_cur_recordeq_para[i][1]+8192)>>14)  + (xout_R_H*s_cur_recordeq_para[i][3]<<1) + ((xout_R_L*s_cur_recordeq_para[i][3]+8192)>>14);

                        //step3
                        CUR_RECORD_EQ_R_D2[i] =               (xin_R_H*s_cur_recordeq_para[i][2]<<1) + ((xin_R_L*s_cur_recordeq_para[i][2]+8192)>>14)  + (xout_R_H*s_cur_recordeq_para[i][4]<<1) + ((xout_R_L*s_cur_recordeq_para[i][4]+8192)>>14);

                    }
                } 
                out_right = xout_R*s_cur_recordeq_master_gain>>10;
            }
            else
            {
                out_right = out_left;
            }
        }
        else
        {
            out_left  = xout_L;
            out_right = xout_R;
        }
        
        if(LCF_SW_dp) // out_left & out_right ==>>  out_left & out_right
        {
            //lcf process  --- left channel ---  
            xin_L      = (out_left*s_cur_lcf_s_gain_left_dp)>>12;   
            if(xin_L>((1<<30)-1)) 
            {
                xin_L = ((1<<30)-1);
            }
            else if(xin_L<-(1<<30))
            {
                xin_L = -(1<<30);
            }
            xin_L_H   = (xin_L>>15);    
            xin_L_L    = xin_L-(xin_L_H<<15);

            xout_L     =  CUR_LCF_L_D1_dp  + (xin_L_H*s_cur_lcf_para_left_dp[0]<<1) + (( xin_L_L*s_cur_lcf_para_left_dp[0] + 8192)>>14); 
            if(xout_L>((1<<30)-1)) 
            {
                xout_L = ((1<<30)-1);
            }
            else if(xout_L<-(1<<30))
            {
                xout_L = -(1<<30);
            }
            xout_L_H   =  (xout_L>>15);        
            xout_L_L   =  xout_L - (xout_L_H<<15);
            

            CUR_LCF_L_D1_dp =  CUR_LCF_L_D2_dp  + (xin_L_H*(-s_cur_lcf_para_left_dp[0])<<2) + (((xin_L_L*(-s_cur_lcf_para_left_dp[0])<<1)+8192)>>14)  - (xout_L_H*s_cur_lcf_para_left_dp[1]<<1) - ((xout_L_L*s_cur_lcf_para_left_dp[1]+8192)>>14);
            CUR_LCF_L_D2_dp =                  (xin_L_H*s_cur_lcf_para_left_dp[0]<<1) + ((xin_L_L*s_cur_lcf_para_left_dp[0]+8192)>>14)  - (xout_L_H*s_cur_lcf_para_left_dp[2]<<1) - ((xout_L_L*s_cur_lcf_para_left_dp[2]+8192)>>14);

            out_left =  (xout_L>>8);


            if(RECORD_EQ_STEREO)
            {
                //lcf process  --- right channel ---  
                xin_R      = (out_right*s_cur_lcf_s_gain_right_dp)>>12;
                if(xin_R>((1<<30)-1)) 
                {
                    xin_R = ((1<<30)-1);
                }
                else if(xin_R<-(1<<30))
                {
                    xin_R = -(1<<30);
                }
                xin_R_H    = (xin_R>>15);    
                xin_R_L    = xin_R-(xin_R_H<<15);

                xout_R     =  CUR_LCF_R_D1_dp  + (xin_R_H*s_cur_lcf_para_right_dp[0]<<1) + (( xin_R_L*s_cur_lcf_para_right_dp[0]+ 8192)>>14); 
                if(xout_R>((1<<30)-1)) 
                {
                    xout_R = ((1<<30)-1);
                }
                else if(xout_R<-(1<<30))
                {
                    xout_R = -(1<<30);
                }
                xout_R_H   =  (xout_R>>15);        
                xout_R_L   =  xout_R - (xout_R_H<<15);
                

                CUR_LCF_R_D1_dp =  CUR_LCF_R_D2_dp  + (xin_R_H*(-s_cur_lcf_para_right_dp[0])<<2) + (((xin_R_L*(-s_cur_lcf_para_right_dp[0])<<1)+8192)>>14)  - (xout_R_H*s_cur_lcf_para_right_dp[1]<<1) - ((xout_R_L*s_cur_lcf_para_right_dp[1]+8192)>>14);
                CUR_LCF_R_D2_dp =                  (xin_R_H*s_cur_lcf_para_right_dp[0]<<1) + ((xin_R_L*s_cur_lcf_para_right_dp[0]+8192)>>14)  - (xout_R_H*s_cur_lcf_para_right_dp[2]<<1) - ((xout_R_L*s_cur_lcf_para_right_dp[2]+8192)>>14);

                out_right = (xout_R>>8);  
            }
            else
            {
                out_right = out_left;
            }
        }
        else
        {
            out_left  =   out_left>>8;
            out_right =  out_right>>8;
        }

        //limit
        if(out_left>32767)
            out_left = 32767;
        else if(out_left<-32768)
            out_left = -32768;

        if(out_right>32767)
            out_right = 32767;
        else if(out_right<-32768)
            out_right = -32768;
        
        psDestLeftData[si]   =  out_left;
        psDestRightData[si]  =  out_right;

    }

    *puiDestCount = uiSrcCount;
    
    return;
           
}

/*****************************************************************************/
//  Description: AUDPROC_ProcessDpEx: 
//  (lcf filter)+ (nr)+ (digital gain) + (dynamic processor) + (eq)
//  Author:      Cherry.Liu
//  attention:   1. fs should be 48000 
//               2. the uiSrcCount should be 480;
//****************************************************************************/
 void  AUDPROC_ProcessDpEx(
    int16_t* psSrcLeftData,
    int16_t* psSrcRightData,
    uint32_t uiSrcCount, 
    int16_t* psDestLeftData, 
    int16_t* psDestRightData, 
    uint32_t* puiDestCount
)
{
    int16_t  si  = 0 , i=0;
    int16_t  sl=0,sr=0,d_L=0,d_R=0;
    uint16_t mabslr=0,x_p_del=0;
    uint16_t x_rp_dp  = 0;//uint16_t
    
    //int32_t  xin_L       = 0; 
    int64  xin_L       = 0; //test code cherry
    int16_t  xin_L_H     = 0;//higher word
    int16_t  xin_L_L     =  0;//lower word
    
    //int32_t  xin_R       = 0; 
    int64  xin_R       = 0; 
    int16_t  xin_R_H      =  0;//higher word
    int16_t  xin_R_L     =  0;//lower word
    
    //int32_t  xout_L      = 0; 
    int64  xout_L       = 0; //test code cherry
    int16_t  xout_L_H   =  0;//higher word
    int16_t  xout_L_L   =  0;//lower word
    
    //int32_t  xout_R      = 0; 
    int64  xout_R      = 0; 
    int16_t  xout_R_H   =  0;//higher word
    int16_t  xout_R_L   =  0;//lower word

    int16_t  lcf_gain_l = 0;//scaled by 1024 
    int16_t  lcf_gain_r = 0;  //scaled by 1024
    //int32_t  out_left = 0;
    //int32_t  out_right = 0;   
    int64  out_left = 0;
    int64  out_right = 0; 


    int16_t  power_log = 0,DP_ingain_cdB = 0,DP_in_power = 0;
    int32_t  dp_out_power = 0;
    int32_t  attack_gain = 0,release_gain = 0;

	//lcf    (psSrcLeftData -> psSrcLeftData)
	for(si=0;si<uiSrcCount;si++)
	{
		if(LCF_SW_dp) // out_left & out_right ==>> out_left & out_right
		{
			out_left  = (int32_t)psSrcLeftData[si]<<8; 
			out_right = (int32_t)psSrcRightData[si]<<8; 

            //lcf process  --- left channel ---  
            xin_L      = (out_left*s_cur_lcf_s_gain_left_dp)>>12;   
            if(xin_L>((1<<30)-1)) 
            {
                xin_L = ((1<<30)-1);
            }
            else if(xin_L<-(1<<30))
            {
                xin_L = -(1<<30);
            }
            xin_L_H   = (xin_L>>15);    
            xin_L_L    = xin_L-(xin_L_H<<15);

            xout_L     =  CUR_LCF_L_D1_dp  + (xin_L_H*s_cur_lcf_para_left_dp[0]<<1) + (( xin_L_L*s_cur_lcf_para_left_dp[0] + 8192)>>14); 
            if(xout_L>((1<<30)-1)) 
            {
                xout_L = ((1<<30)-1);
            }
            else if(xout_L<-(1<<30))
            {
                xout_L = -(1<<30);
            }
            xout_L_H   =  (xout_L>>15);        
            xout_L_L   =  xout_L - (xout_L_H<<15);
            

            CUR_LCF_L_D1_dp =  CUR_LCF_L_D2_dp  + (xin_L_H*(-s_cur_lcf_para_left_dp[0])<<2) + (((xin_L_L*(-s_cur_lcf_para_left_dp[0])<<1)+8192)>>14)  - (xout_L_H*s_cur_lcf_para_left_dp[1]<<1) - ((xout_L_L*s_cur_lcf_para_left_dp[1]+8192)>>14);
            CUR_LCF_L_D2_dp =                  (xin_L_H*s_cur_lcf_para_left_dp[0]<<1) + ((xin_L_L*s_cur_lcf_para_left_dp[0]+8192)>>14)  - (xout_L_H*s_cur_lcf_para_left_dp[2]<<1) - ((xout_L_L*s_cur_lcf_para_left_dp[2]+8192)>>14);

            out_left =  (xout_L>>8);


            if(RECORD_EQ_STEREO)
            {
                //lcf process  --- right channel ---  
                xin_R      = (out_right*s_cur_lcf_s_gain_right_dp)>>12;
                if(xin_R>((1<<30)-1)) 
                {
                    xin_R = ((1<<30)-1);
                }
                else if(xin_R<-(1<<30))
                {
                    xin_R = -(1<<30);
                }
                xin_R_H    = (xin_R>>15);    
                xin_R_L    = xin_R-(xin_R_H<<15);

                xout_R     =  CUR_LCF_R_D1_dp  + (xin_R_H*s_cur_lcf_para_right_dp[0]<<1) + (( xin_R_L*s_cur_lcf_para_right_dp[0]+ 8192)>>14); 
                if(xout_R>((1<<30)-1)) 
                {
                    xout_R = ((1<<30)-1);
                }
                else if(xout_R<-(1<<30))
                {
                    xout_R = -(1<<30);
                }
                xout_R_H   =  (xout_R>>15);        
                xout_R_L   =  xout_R - (xout_R_H<<15);
                

                CUR_LCF_R_D1_dp =  CUR_LCF_R_D2_dp  + (xin_R_H*(-s_cur_lcf_para_right_dp[0])<<2) + (((xin_R_L*(-s_cur_lcf_para_right_dp[0])<<1)+8192)>>14)  - (xout_R_H*s_cur_lcf_para_right_dp[1]<<1) - ((xout_R_L*s_cur_lcf_para_right_dp[1]+8192)>>14);
                CUR_LCF_R_D2_dp =                  (xin_R_H*s_cur_lcf_para_right_dp[0]<<1) + ((xin_R_L*s_cur_lcf_para_right_dp[0]+8192)>>14)  - (xout_R_H*s_cur_lcf_para_right_dp[2]<<1) - ((xout_R_L*s_cur_lcf_para_right_dp[2]+8192)>>14);

                out_right = (xout_R>>8);  
            }
            else
            {
                out_right = out_left;
            }
			
	        //limit
	        if(out_left>32767)
	            out_left = 32767;
	        else if(out_left<-32768)
	            out_left = -32768;

	        if(out_right>32767)
	            out_right = 32767;
	        else if(out_right<-32768)
	            out_right = -32768;
	        
	        psSrcLeftData[si]   =  out_left;
	        psSrcRightData[si]  =  out_right;
		}
	}

#if 1
	//nr
	if(!RECORD_EQ_STEREO)
	{
		audio_record_nr(psSrcLeftData);
		memcpy(psSrcRightData,psSrcLeftData,sizeof(int16_t)*uiSrcCount);
	}
	else
	{
		audio_record_nr_stereo(psSrcLeftData,psSrcRightData);
	}
#endif
	//dg + alc + eq
    for(si=0;si<uiSrcCount;si++)//AUDIO POST PROCESS
    {
        //get decoder left and right
        sl = psSrcLeftData[si];// s(si,1);
        sr = psSrcRightData[si];// s(si,2);

        //AGC
        if(DP_SW)
        {
            //%max(abs(l,r))
            mabslr = max(abs(sl),abs(sr));
            
            //%peak;
            d48i_dp = d48i_dp -1;
            if(d48i_dp>=0)
            {
                if(mabslr>x_p_dp)
                {
                    x_p_dp = mabslr;
                }
            }
            else
            {
                d48i_dp = max_FN_dp-1;
            
                x_p_del = x_d_dp[x_d_i_dp];
                x_d_dp[x_d_i_dp] = x_p_dp;
                x_d_i_dp = x_d_i_dp -1;
                if(x_d_i_dp < 0)
                {
                    x_d_i_dp = 25;
                }

                if(x_p_del != x_pm_dp)
                {
                    if(x_p_dp>x_pm_dp)
                    {
                        x_pm_dp = x_p_dp;
                    }
                }
                else
                {
                    x_pm_dp  = x_d_dp[25];
                    for(i=24;i>=0;i--)
                    {
                        if(x_d_dp[i]>x_pm_dp)
                        {
                            x_pm_dp = x_d_dp[i];
                        }
                    }
                }

                x_p_dp  = mabslr;
            }

            //%peak
            x_rp_dp = max(x_pm_dp,x_p_dp);

            //%power estimate
            power_log     =  F200log10(x_rp_dp) - 903;   //% int16_t;   
            DP_ingain_cdB =  F200log10(DP_input_gain) - 602;
            DP_in_power   =  (power_log + DP_ingain_cdB); //% cdB

            
            //%gain update  
            if(DP_in_power > COMPRESSOR_threshold)   //%     tc < in     COMPRESSOR
            {
                dp_is_expander = FALSE;
                dp_out_power = COMPRESSOR_threshold + ((DP_in_power - COMPRESSOR_threshold)*COMPRESSOR_ratio>>15);
                if(dp_out_power> DP_limit_up)
                    dp_out_power = DP_limit_up ;
                agc_gain_p_dp = DP_input_gain*F32768power10(DP_in_power - dp_out_power)>>5; //%scaled by 1024*1024
            }
            else if(DP_in_power >= EXPANDER_threshold) //%    te <= in <= tc
            {
                agc_gain_p_dp = DP_input_gain<<10;
            }
            else                                      //%    in < te     EXPANDER
            {
                dp_is_expander = TRUE;
                dp_out_power = EXPANDER_threshold - ((EXPANDER_threshold - DP_in_power)<<15)/EXPANDER_ratio;
                if(dp_out_power >= DP_limit_down)
                {
                    agc_gain_p_dp = DP_input_gain*F32768power10(DP_in_power - dp_out_power)>>5; //%scaled by 1024*1024
                }
                else
                {
                    agc_gain_p_dp = 0;
                }  
            }
        }
        else
        {
            agc_gain_p_dp = DP_input_gain<<10;
        }
        

        //%gain smooth
        if(agc_gain_p_dp < agc_gain_m_dp)
        {
            if(dp_is_expander)
            {
                //%expander attack
                attack_gain = agc_gain_m_dp - gd_Attack_Expander;

                
                if(attack_gain<agc_gain_p_dp)
                    agc_gain_m_dp = agc_gain_p_dp;
                else
                    agc_gain_m_dp = attack_gain;
                

                Hold_H_dp  = Hold_HC_Expander;
            }
            else
            {
                //%compressor attack
                attack_gain = agc_gain_m_dp - gd_Attack_Compressor;
                
                if(attack_gain<agc_gain_p_dp)
                    agc_gain_m_dp = agc_gain_p_dp; 
                else
                    agc_gain_m_dp = attack_gain;
                

                Hold_H_dp  = Hold_HC_Compressor;
            }
        }
        else if(agc_gain_p_dp > agc_gain_m_dp)
        {
            Hold_H_dp = Hold_H_dp -1;//%hold
            if(Hold_H_dp<=0)
            {
                Hold_H_dp  = 0;
                
                //%release
                if(dp_is_expander)
                {
                    release_gain = agc_gain_m_dp + gd_Release_Expander;
                    
                    if(release_gain >agc_gain_p_dp)
                        agc_gain_m_dp = agc_gain_p_dp;
                    else
                        agc_gain_m_dp = release_gain;
                    
                }
                else
                {
                    release_gain = agc_gain_m_dp + gd_Release_Compressor;

                    if(release_gain >agc_gain_p_dp)
                        agc_gain_m_dp = agc_gain_p_dp;
                    else
                        agc_gain_m_dp = release_gain;
                }
            }
        }
    
        //%delay
        d_L = d_dl_dp[delay_i_dp];
        d_dl_dp[delay_i_dp] = sl;
        
        d_R = d_dr_dp[delay_i_dp];
        d_dr_dp[delay_i_dp] = sr;
        
        delay_i_dp = delay_i_dp -1;
        if(delay_i_dp <0)
            delay_i_dp = data_dN_dp-1;

        
        //%zero cross
        if(DP_ZC_SW)
        {

            if(o_L_dp*d_L<=0)//other method to determin?
            {
                agc_gain_l_dp = (agc_gain_m_dp>>10);
            }
            o_L_dp = d_L;

            if(o_R_dp*d_R<=0)
            {
                agc_gain_r_dp = (agc_gain_m_dp>>10);
            }
            o_R_dp = d_R; 
        }
        else
        {
            agc_gain_l_dp = (agc_gain_m_dp>>10);
            agc_gain_r_dp = (agc_gain_m_dp>>10);
        }

        //add alc output gain & shift (LEFT 8 bits)
        //xout_L = (agc_gain_l_dp*d_L>>10)<<8;
        //xout_R = (agc_gain_r_dp*d_R>>10)<<8;
        xout_L = (agc_gain_l_dp*d_L>>2);
        xout_R = (agc_gain_r_dp*d_R>>2);
        
        if(RECORD_EQ_SW)// xout_L xout_R ==>>  out_left & out_right
        {
            for(i=0;i<RECORDEQ_MAX_BAND;i++)//i :band index
            {
                if(RECORD_EQ_BAND_SW[i])
                {
                    xin_L      = xout_L*s_cur_recordeq_s_gain[i]>>12;          
                    xin_L_H    = xin_L >> 15;   
                    xin_L_L    = xin_L-(xin_L_H<<15);

                    //step1 
                    xout_L   =  CUR_RECORD_EQ_L_D1[i] + (xin_L_H*s_cur_recordeq_para[i][0]<<1) +  ((xin_L_L*s_cur_recordeq_para[i][0]+8192)>>14) ; 
                    xout_L_H =  xout_L >> 15;
                    xout_L_L = xout_L - (xout_L_H<<15);
                    
                    //step2
                    CUR_RECORD_EQ_L_D1[i] =  CUR_RECORD_EQ_L_D2[i]  + (xin_L_H*s_cur_recordeq_para[i][1]<<1) + ((xin_L_L*s_cur_recordeq_para[i][1]+8192)>>14)  + (xout_L_H*s_cur_recordeq_para[i][3]<<1) + ((xout_L_L*s_cur_recordeq_para[i][3]+8192)>>14);

                    //step3
                    CUR_RECORD_EQ_L_D2[i] =               (xin_L_H*s_cur_recordeq_para[i][2]<<1) + ((xin_L_L*s_cur_recordeq_para[i][2]+8192)>>14)  + (xout_L_H*s_cur_recordeq_para[i][4]<<1) + ((xout_L_L*s_cur_recordeq_para[i][4]+8192)>>14);
                } 
            }
            out_left = xout_L*s_cur_recordeq_master_gain>>10;

            if(RECORD_EQ_STEREO)
            {
                for(i=0;i<RECORDEQ_MAX_BAND;i++)//i :band index
                {
                    if(RECORD_EQ_BAND_SW[i])
                    {
                        xin_R      = xout_R*s_cur_recordeq_s_gain[i]>>12;
                        xin_R_H    = xin_R >> 15;       
                        xin_R_L    = xin_R - (xin_R_H<<15);

                        //step1
                        xout_R   =  CUR_RECORD_EQ_R_D1[i] + (xin_R_H*s_cur_recordeq_para[i][0]<<1) + ((xin_R_L*s_cur_recordeq_para[i][0]+8192)>>14) ;
                        xout_R_H = xout_R >> 15;
                        xout_R_L = xout_R - (xout_R_H<<15);
                        
                        //step2
                        CUR_RECORD_EQ_R_D1[i] =  CUR_RECORD_EQ_R_D2[i]  + (xin_R_H*s_cur_recordeq_para[i][1]<<1) + ((xin_R_L*s_cur_recordeq_para[i][1]+8192)>>14)  + (xout_R_H*s_cur_recordeq_para[i][3]<<1) + ((xout_R_L*s_cur_recordeq_para[i][3]+8192)>>14);

                        //step3
                        CUR_RECORD_EQ_R_D2[i] =               (xin_R_H*s_cur_recordeq_para[i][2]<<1) + ((xin_R_L*s_cur_recordeq_para[i][2]+8192)>>14)  + (xout_R_H*s_cur_recordeq_para[i][4]<<1) + ((xout_R_L*s_cur_recordeq_para[i][4]+8192)>>14);

                    }
                } 
                out_right = xout_R*s_cur_recordeq_master_gain>>10;
            }
            else
            {
                out_right = out_left;
            }
        }
        else
        {
            out_left  = xout_L;
            out_right = xout_R;
        }
        
        out_left  =   out_left>>8;
        out_right =  out_right>>8;
      

        //limit
        if(out_left>32767)
            out_left = 32767;
        else if(out_left<-32768)
            out_left = -32768;

        if(out_right>32767)
            out_right = 32767;
        else if(out_right<-32768)
            out_right = -32768;
        
        psDestLeftData[si]   =  out_left;
        psDestRightData[si]  =  out_right;

    }

    *puiDestCount = uiSrcCount;
    
    return;
           
}

/**---------------------------------------------------------------------------*
 **                         Compiler Flag                                     *
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus 
}
#endif

