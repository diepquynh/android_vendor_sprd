/******************************************************************************
 ** File Name:      aud_proc.h                                                *
 ** Author:         Cherry.Liu                                             *
 ** DATE:           04/14/2010                                               *
 ** Copyright:      2010 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:
 ******************************************************************************

 ******************************************************************************
 **                        Edit History                                       *
 ** ------------------------------------------------------------------------- *
 ** DATE           NAME             DESCRIPTION                               *
 ** 04/14/2010       Cherry.Liu       Create.                                  *
 ******************************************************************************/

#ifndef _AUD_PROC_H
#define _AUD_PROC_H

/** ---------------------------------------------------------------------------*
 **                         Dependencies                                      *
 **---------------------------------------------------------------------------*/

#ifdef __cplusplus
extern   "C"
{
#endif



#include "aud_filter_calc.h"
/** ---------------------------------------------------------------------------*
 **                         MACRO Definations                                     *
 **---------------------------------------------------------------------------*/

/** ---------------------------------------------------------------------------*
 **                         Data Structures                                   *
 **---------------------------------------------------------------------------*/
typedef enum {
    AUD_PROC_INPUT_LEFT = 0,// 0:  L
    AUD_PROC_INPUT_RIGHT ,  // 1:  R
    AUD_PROC_INPUT_AVER,    // 2 : (L+R)/2
    AUD_PROC_INPUT_DIFF,   // 3:  (L-R)/2
    AUD_PROC_INPUT_ZERO,   // 4:  zero
    AUD_PROC_INPUT_MAX
} AUD_PROC_INPUT_TYPE_E;

typedef struct {
    AUD_PROC_INPUT_TYPE_E input_type;
    BOOLEAN   reverse;
} AUD_PROC_INPUT_SET_T;


//mixer nv
typedef struct {
    AUD_PROC_INPUT_SET_T left_control;
    AUD_PROC_INPUT_SET_T right_control;
} MIXER_NV_PARAM_T;


//lcf nv
typedef struct {
    BOOLEAN is_lcf_on;        //1:lcf on;0:lcf off;
    BOOLEAN is_lcf_before_agc;//1: LCF first then AGC; 0 : first AGC  then LCF
    REC_FILTER_LCFPARA_TYPE_E eLcfParaType;

    int16_t f1_g0;
    int16_t f1_g1;
    int16_t f1_fp;
    int16_t f2_g0;
    int16_t f2_g1;
    int16_t f2_fp;

    int16_t fp_l;
    int16_t fp_r;

    int16_t lcf_gain_l;          //scaled by 16384
    int16_t lcf_gain_r;          //scaled by 16384
} LCF_NV_PARAM_T;



//agc nv
typedef struct {
    BOOLEAN agc_sw;
    BOOLEAN agc_zc_sw  ;
    int16_t   agc_input_gain;
    int16_t   agc_ingain_set; //from each eq mode
    int16_t   agc_input_gain_start;
    int16_t   agc_delay;
    int16_t   agc_hold_hc;
    int16_t   agc_attack;
    int16_t   agc_release;
} AGC_NV_PARAM_T;


//eq nv
typedef struct {
    int16_t bass_fo;
    int16_t bass_df;
    int16_t bass_boostdB;  //[-72,+18] dB
    int16_t bass_gaindB;   //[-84,+6]  dB

    int16_t treble_fo;
    int16_t treble_df;
    int16_t treble_boostdB;  //[-72,+18] dB
    int16_t treble_gaindB;   //[-84,+6]  dB
} EQMODE_FILTER_PARAM_T;

typedef struct {
    BOOLEAN hpf_sw;
    int8_t    limit;//scaled by 127
    EQMODE_FILTER_PARAM_T filter_para;
} EQMODE_NV_PARAM_T;

//nr nv
typedef struct {
    int16_t   nr_switch;
    int16_t   modu1_switch;
    int16_t   modu2_switch;
    int16_t   min_psne;
    int16_t   max_psne;
    int16_t   nr_dgain;
    int16_t   ns_limit;
    int16_t   ns_factor;
} NR_CONTROL_PARAM_T;

//DP nv
typedef struct {
    //    BOOLEAN     DP_SW          ;
    //    BOOLEAN     DP_ZC_SW       ;  //% zeros cross switch
    //    BOOLEAN     DP_lcf_sw      ;
    int16_t
    DP_sw_switch;                //%  zeros cross switch for DP_sw, DP_ZC_sw, DP_lcf_sw.
    int16_t       DP_input_gain  ;  //% linear gain ; scaled by 1024;
    int16_t       DP_sdelay      ;  //% ms
    int16_t       DP_limit_up    ;  //%  uint: cdB
    int16_t       DP_limit_down  ;  //%  uint: cdB

    int16_t       COMPRESSOR_threshold ; //%uint: cdB
    int16_t       COMPRESSOR_ratio
    ; //%COMPRESSOR_ratio = tan(<) ;scaled by 32768
    int16_t       COMPRESSOR_attack    ; //%uint:ms  30
    int16_t       COMPRESSOR_hold      ; //%uint:ms  30
    int16_t       COMPRESSOR_release   ; //%uint:ms  1000

    int16_t       EXPANDER_threshold  ; //%uint: cdB  -250
    int16_t       EXPANDER_ratio      ; //%expander_ratio = ctan(<);scaled by 32768
    int16_t       EXPANDER_attack     ; //%uint:ms    30
    int16_t       EXPANDER_hold       ; //%uint:ms    30
    int16_t       EXPANDER_release    ; //%uint:ms    1000

    int16_t       DP_lcf_fp_l;
    int16_t       DP_lcf_fp_r;
    int16_t       DP_lcf_gain_l;          //scaled by 16384
    int16_t       DP_lcf_gain_r;          //scaled by 16384
} DP_CONTROL_PARAM_T;

#define RECORDEQ_MAX_BAND 6

typedef struct {
    int16_t       fo;
    int16_t       df;
    int16_t       boost;
    int16_t       gain;
} RECORDEQ_BAND_INPUT_PARAM_T;


//RECORDEQ nv
typedef struct {
    //BOOLEAN     RECORDEQ_SW;      //0:record eq bypass; 1:record eq is on;
    //BOOLEAN     RECORDEQ_STEREO;  //0:mono;1:stereo
    //BOOLEAN     RECORDEQ_BAND_SW[RECORDEQ_MAX_BAND]; //0:band eq is off;1:band eq is on
    //int16_t       RECORDEQ_fo[RECORDEQ_MAX_BAND];      //Hz
    //int16_t       RECORDEQ_df[RECORDEQ_MAX_BAND];      //Hz
    //int16_t       RECORDEQ_boost[RECORDEQ_MAX_BAND]  ; //range -720:1:180;
    //int16_t       RECORDEQ_gain[RECORDEQ_MAX_BAND]  ;  //range -720:1:180;
    int16_t       RECORDEQ_sw_switch;
    int16_t       RECORDEQ_master_gain;                //scaled by 1024
    RECORDEQ_BAND_INPUT_PARAM_T RECORDEQ_band_para[RECORDEQ_MAX_BAND];
} RECORDEQ_CONTROL_PARAM_T;


/** ---------------------------------------------------------------------------*
 **                         Global Variables                                  *
 **---------------------------------------------------------------------------*/

/** ---------------------------------------------------------------------------*
 **                         Constant Variables                                *
 **---------------------------------------------------------------------------*/

/** ---------------------------------------------------------------------------*
 **                        Function Prototypes                               *
 **---------------------------------------------------------------------------*/

/*****************************************************************************/
//  Description:    set digital gain of agc
//  Author:         Cherry.Liu
//  Note:           !attention! you should set params down between frames!
//****************************************************************************/
BOOLEAN AUDPROC_SetAgcDg( int16_t digtal_gain);

/*****************************************************************************/
//  Description:    set eq mode para for audio process modules, including lcf,agc and eq.
//  Author:         Cherry.Liu
//  Note:           !attention! you should call it between data frames.
//****************************************************************************/
BOOLEAN AUDPROC_Set(
    int32_t sample_rate,
    LCF_NV_PARAM_T *lcf_param_ptr,
    AGC_NV_PARAM_T *agc_param_ptr,
    EQMODE_FILTER_PARAM_T *eq_param_ptr
);

/*****************************************************************************/
//  Description:    init audio process modules including lcf,agc and eq.
//  Author:         Cherry.Liu
//  Note:           !attention! you should init it before music start.
//****************************************************************************/
BOOLEAN AUDPROC_Init(
    int32_t sample_rate,
    LCF_NV_PARAM_T *lcf_param_ptr,
    AGC_NV_PARAM_T *agc_param_ptr,
    EQMODE_NV_PARAM_T *eq_param_ptr
);

/*****************************************************************************/
//  Description:    deinti aud proc plugger
//  Author:         Cherry.Liu
//  Note:
//****************************************************************************/
BOOLEAN AUDPROC_DeInit(
    void
);

/*****************************************************************************/
//  Description:    init  dp module
//  Author:         Cherry.Liu
//  Note:           !attention! you should set params down between frames!
//****************************************************************************/
BOOLEAN AUDPROC_initRecordEq(
    RECORDEQ_CONTROL_PARAM_T *recordeq_param_ptr,
    int32_t       Fs);

/*****************************************************************************/
//  Description:    init  dp module
//  Author:         Cherry.Liu
//  Note:           !attention! you should set params down between frames!
//****************************************************************************/
BOOLEAN AUDPROC_initDp(
    DP_CONTROL_PARAM_T *dp_param_ptr,
    int32_t       Fs);

/*****************************************************************************/
//  Description:    deinit  dp  module
//  Author:         Cherry.Liu
//  Note:
//****************************************************************************/
BOOLEAN AUDPROC_DeInitDp(
    void
);

/*****************************************************************************/
//  Description: AUDPROC_ProcessDp: (digital gain) + (dynamic processor) + (lcf filter)
//  Author:      Cherry.Liu
//****************************************************************************/
void  AUDPROC_ProcessDp(
    int16_t *psSrcLeftData,
    int16_t *psSrcRightData,
    uint32_t uiSrcCount,
    int16_t *psDestLeftData,
    int16_t *psDestRightData,
    uint32_t *puiDestCount
);

/*****************************************************************************/
//  Description: AUDPROC_ProcessDpEx:
//  (lcf filter)+ (nr)+ (digital gain) + (dynamic processor) + (eq)
//  Author:      Cherry.Liu
//  attention:   1. fs should be 48000
//               2. the uiSrcCount should be 480;
//****************************************************************************/
void  AUDPROC_ProcessDpEx(
    int16_t *psSrcLeftData,
    int16_t *psSrcRightData,
    uint32_t uiSrcCount,
    int16_t *psDestLeftData,
    int16_t *psDestRightData,
    uint32_t *puiDestCount
);
/** ---------------------------------------------------------------------------*
 **                         Compiler Flag                                     *
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif

#endif  // _AUD_AGC_H

// End of aud_agc.h


