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
#include "aud_enha.h"
#include "filter_calc.h"
#include <stdlib.h>
#include <string.h>
#include "vbc_codec.h"
#include <eng_audio.h>
#include "aud_common.h"

 /**---------------------------------------------------------------------------**
 ** 						Compiler Flag									   **
 **----------------------------------------------------------------------------**/
#ifdef __cplusplus
 extern   "C"
 {
#endif


 /**---------------------------------------------------------------------------**
  **						 MACRO Definations								   **
  **---------------------------------------------------------------------------**/
#define AUD_PARAS_BUF_LEN    (100 << 2)
 /**---------------------------------------------------------------------------**
  **						 Data Structures								   **
  **---------------------------------------------------------------------------**/
 typedef struct
 {
 //    AUDIO_ENHA_EQ_STRUCT_T eq_para_set[AUD_ENHA_EQPARA_MAX-1];
	 AUDIO_ENHA_EQ_STRUCT_T eq_para_set;

	 AUDIO_ENHA_TUNABLE_EQ_STRUCT_T tunable_eq_para_set[AUD_ENHA_TUNABLE_EQPARA_MAX-1];
 }AUDIO_ENHA_EQ_NV_PARA_T;

 /**---------------------------------------------------------------------------**
  **						 const data 								  **
  **---------------------------------------------------------------------------**/

 /**---------------------------------------------------------------------------*
 **                         Macro Definition                                  *
 **---------------------------------------------------------------------------*/
// #define AUD_EHA_DEBUG

#define DEFAULT_OUT_SAMPLING_RATE 44100

#define HPF_FADE_OUT_TOTAL_TIME 10      //ms  range:25-50
#define HPF_FADE_OUT_GAIN_SET_TIMES 20
#define HPF_FADE_IN_TOTAL_TIME 10      //ms  range:25-50
#define HPF_FADE_IN_GAIN_SET_TIMES 20

#define BAND_FADE_OUT_TOTAL_TIME 8      //ms  range:25-50
#define BAND_FADE_OUT_GAIN_SET_TIMES 8
#define BAND_FADE_IN_TOTAL_TIME 8      //ms  range:25-50
#define BAND_FADE_IN_GAIN_SET_TIMES 8
 /**---------------------------------------------------------------------------**
  **						 Global Variables								   **
  **---------------------------------------------------------------------------**/
 LOCAL AUDIO_ENHA_EQ_NV_PARA_T s_enha_eq_para_nv ={0};
 LOCAL DG_CONTROL_PARAM_T	   s_cur_dg_param	  = {0};   //dg
 LOCAL DAPATH_CONTROL_PARAM_T  s_cur_dapath_param = {0};   //da path
 LOCAL ALC_CONTROL_PARAM_T	   s_cur_alc_param	  = {0};   //alc
 LOCAL HPF_CONTROL_PARAM_T	   s_cur_hpf_param	  = {0};   //hpf

 LOCAL uint32_t  s_cur_sample_rate		   = 44100;
 LOCAL uint32_t  s_cur_eq_para_set_index  = 0;
 LOCAL uint32_t  s_cur_tunable_eq_para_set_index  = 1;

 LOCAL uint32_t  s_cur_music_type  = 0; //music type:mp3
 LOCAL AUD_ENHA_EQMODE_SEL_E  s_cur_eq_mode_sel 	  =  AUD_ENHA_EQMODE_SEL_OFF;

 /**---------------------------------------------------------------------------**
  **						local functions definition								 **
  **---------------------------------------------------------------------------**/
LOCAL int32_t get_cur_sample_rate(void)
{
    return DEFAULT_OUT_SAMPLING_RATE;
}

 LOCAL void AUDENHA_FadeOut(
	 HPF_GAIN_PARAM_T* ori_gain_ptr,
	 int32_t fade_out_total_time,//unit:ms
	 int32_t fade_out_set_times
 )
 {
	 int32_t i = 0,j = 0;
	 int32_t step_value[HPF_S_GAIN_NUM] = {0} ,step_time = 0;

	 //prepare step value and step time for each filter
	 for(i=0;i<HPF_S_GAIN_NUM;i++)//S0	~ S5
	 {
		 step_value[i]	 = ori_gain_ptr->s[i]/fade_out_set_times;
	 }
	 step_time	  = fade_out_total_time*1000/fade_out_set_times;  //us

	 //fade out step by step
	 for(j=1;j<=fade_out_set_times;j++)
	 {
		 for(i=0;i<HPF_S_GAIN_NUM;i++)//S0	~ S5
		 {
			 VB_SetHpfGain(i, (ori_gain_ptr->s[i]-j*step_value[i]));
		 }

		 //udelay(step_time);  //us
	 }

	 //set all gain to 0 finally
	 for(i=0;i<HPF_S_GAIN_NUM;i++)//S0	~ S5
	 {
		 VB_SetHpfGain(i,0);
	 }

	 return ;
 }

 /*****************************************************************************/
 //  Description:	 fade in hpf filters from zeros
 //  Author:		 Cherry.Liu
 //  Note:
 //****************************************************************************/
 LOCAL void AUDENHA_FadeIn(
	 HPF_GAIN_PARAM_T* dest_gain_ptr,
	 int32_t fade_in_total_time,//unit:ms
	 int32_t fade_in_set_times
 )
 {
	 int32_t i = 0,j = 0;
	 int32_t step_value[HPF_S_GAIN_NUM] = {0} ,step_time = 0;

	 //prepare step value and step time for each filter
	 for(i=0;i<HPF_S_GAIN_NUM;i++)//S0	~ S5
	 {
		 step_value[i]	 = dest_gain_ptr->s[i]/fade_in_set_times;
	 }
	 step_time	  = fade_in_total_time*1000/fade_in_set_times;	 //us

	 for(j=1;j<=fade_in_set_times;j++)
	 {
		 for(i=0;i<HPF_S_GAIN_NUM;i++)//S0	~ S5
		 {
			 VB_SetHpfGain(i, (j*step_value[i]));
		 }
		 //udelay(step_time); 	 //us
	 }

	 //set all gain to dest  gain finally
	 for(i=0;i<HPF_S_GAIN_NUM;i++)//S0	~ S5
	 {
		 VB_SetHpfGain(i,dest_gain_ptr->s[i]);
	 }

	 return ;
 }

 /*****************************************************************************/
 //  Description:	 set HPF filter A/B paras & gains down when the hpf is enabled:
 // 				 the first hpf is for lcf;
 // 				 the second to sixth hp are for 5-band eq;
 //
 //  Author:		 Cherry.Liu
 //  Note:
 //****************************************************************************/
 LOCAL BOOLEAN AUDENHA_SetHpf(
	 HPF_CONTROL_PARAM_T* hpf_param_ptr,
	 int32_t sample_rate
 )
 {
	 uint32_t i  = 0;
	 FILTER_LCF_CALC_PARA_T* lcf_param_ptr = &hpf_param_ptr->lcf_para;//in
	 HPF_GAIN_PARAM_T*		 hpf_gain_ptr  = &hpf_param_ptr->s_gain;
	 FILTER_EQ_CALC_PARA_T	eq_input_para = {0};//in
	 IIR_FILTER_PARA_T	lcf_filter_set = {0};//out
	 IIR_FILTER_PARA_T	eq_filter_set[EQ_BAND_MAX] = {0};//out
	 BOOLEAN  return_value = SCI_TRUE;
	 HPF_GAIN_PARAM_T ori_hpf_gain = {0};


	 //check input paras
	 SCI_ASSERT(lcf_param_ptr != SCI_NULL);/*assert verified*/
	 //SCI_ASSERT(eq_param_ptr != SCI_NULL);
	 SCI_ASSERT(sample_rate  != 0);/*assert verified*/

	 //the first hpf is for lcf;
	 return_value = Filter_CalcLCF(lcf_param_ptr, 16384,
		 sample_rate,
		 &lcf_filter_set,
		 &hpf_gain_ptr->s[0]);

	 if(!return_value)
	 {
		 // printk("vbc_codec_eq.c,[AUDENHA_SetHpfs] encounters error when caculating filter para! \n");

		 //if return error,this filter is set to all-pass filter
		 hpf_gain_ptr->s[0]= 4096;
		 lcf_filter_set.B0 = 16384;
		 lcf_filter_set.B1 = 0;
		 lcf_filter_set.B2 = 0;
		 lcf_filter_set.A0 = 16384;
		 lcf_filter_set.A1 = 0;
		 lcf_filter_set.A2 = 0;
	 }

#ifdef AUD_EHA_DEBUG
	 //trace filter paras
	 printk("vbc_codec_eq.c,[AUDENHA_SetHpfs]lcf:lcf_sw:%d  filter_type:%d\n",
		 lcf_param_ptr->isFilterOn,lcf_param_ptr->eLcfParaType);
	 printk("vbc_codec_eq.c,[AUDENHA_SetHpfs]lcf:f1_g0:%d  f1_g1:%d  f1_fp:%d  f2_g0:%d f2_g1:%d f2_fp:%d\n",
		 lcf_param_ptr->unlcfPara.lcfPara.f1_g0,lcf_param_ptr->unlcfPara.lcfPara.f1_g1,lcf_param_ptr->unlcfPara.lcfPara.f1_fp,lcf_param_ptr->unlcfPara.lcfPara.f2_g0,lcf_param_ptr->unlcfPara.lcfPara.f2_g1,lcf_param_ptr->unlcfPara.lcfPara.f2_fp);
	 printk("vbc_codec_eq.c,[AUDENHA_SetHpfs]lcf:S:%d,B0:%d B1:%d B2:%d; A0:%d A1:%d A2:%d;samplerate:%d\n",
		 hpf_gain_ptr->s[0],lcf_filter_set.B0,lcf_filter_set.B1,lcf_filter_set.B2,
		 lcf_filter_set.A0,(lcf_filter_set.A1),(lcf_filter_set.A2),sample_rate);
#endif


	 //calc the a/b parameters of each eq band
	 for(i=0;i<hpf_param_ptr->eq_band_num;i++)//i :band index
	 {
		 eq_input_para.isFilterOn = hpf_param_ptr->band_sw[i];

		 if((i==0)&&(hpf_param_ptr->low_shelve_on))//the first band
		 {
			 eq_input_para.eEqParaType		= FILTER_EQPARA_LOW_SHELVE;
			 eq_input_para.unEqPara.fo_next = hpf_param_ptr->eq_band_para[i+1].fo;
		 }
		 else if((i==(hpf_param_ptr->eq_band_num-1))&&(hpf_param_ptr->high_shelve_on) && (hpf_param_ptr->eq_band_num >= 2))
		 {
			 eq_input_para.eEqParaType		= FILTER_EQPARA_HIGH_SHELVE;
			 eq_input_para.unEqPara.fo_last = hpf_param_ptr->eq_band_para[hpf_param_ptr->eq_band_num-2].fo;

		 }
		 else
		 {
			 eq_input_para.eEqParaType	= FILTER_EQPARA_NORMAL_EQ;
			 eq_input_para.unEqPara.q	= hpf_param_ptr->eq_band_para[i].q;
		 }
		 eq_input_para.fo		  = hpf_param_ptr->eq_band_para[i].fo;
		 eq_input_para.boostdB	  = hpf_param_ptr->eq_band_para[i].boostdB;
		 eq_input_para.basegaindB = hpf_param_ptr->eq_band_para[i].gaindB;

		 return_value = Filter_CalcEQ(&eq_input_para,
			 sample_rate,
			 &eq_filter_set[i],
			 &hpf_gain_ptr->s[i+1]);

		 if(!return_value)
		 {
			 // printk("vbc_codec_eq.c,[AUDENHA_SetHpfs]band:%d encounters error when caculating filter para! \n",i);

			 //if return error,this filter is set to all-pass filter
			 hpf_gain_ptr->s[i+1]= 4096;
			 eq_filter_set[i].B0 = 16384;
			 eq_filter_set[i].B1 = 0;
			 eq_filter_set[i].B2 = 0;
			 eq_filter_set[i].A0 = 16384;
			 eq_filter_set[i].A1 = 0;
			 eq_filter_set[i].A2 = 0;
		 }

#ifdef AUD_EHA_DEBUG
		 printk("vbc_codec_eq.c,[AUDENHA_SetHpfs]band %d:sw:%d,fo:%d	q:%d  boost:%d	gain:%d\n",i,
			 eq_input_para.isFilterOn,eq_input_para.fo,eq_input_para.unEqPara.q, eq_input_para.boostdB,eq_input_para.basegaindB );

		 printk("vbc_codec_eq.c,[AUDPROC_Seteq]band %d:S:%d,B0:%d B1:%d B2:%d, A0:%d A1:%d A2:%d, samplerate:%d\n",i,
			 hpf_gain_ptr->s[i+1],eq_filter_set[i].B0,eq_filter_set[i].B1,eq_filter_set[i].B2,
			 eq_filter_set[i].A0,(eq_filter_set[i].A1),(eq_filter_set[i].A2),sample_rate);
#endif

	 }

	 //****For future*****here if we add smooth algorithm,we need put the gain from smooth method to the S gain of HPF//

	//begin the eq paras setting process
	 //fading out------
	 for(i=0;i<HPF_S_GAIN_NUM;i++)//S0	~ S5
	 {
		 ori_hpf_gain.s[i]= VB_GetHpfGain(i);
	 }

	 if(0 != ori_hpf_gain.s[HPF_S_GAIN_NUM-1])//S5
	 {
		 //the music is playing;the gain register is non-zero
		 AUDENHA_FadeOut(&ori_hpf_gain,HPF_FADE_OUT_TOTAL_TIME,HPF_FADE_OUT_GAIN_SET_TIMES);
	 }

#ifdef  AUD_EHA_DEBUG
	 printk("vbc_codec_eq.c,[AUDENHA_SetHpf]:s0-s6 registers:%d  %d  %d  %d  %d  %d \n",
		 ori_hpf_gain.s[0],ori_hpf_gain.s[1],ori_hpf_gain.s[2],ori_hpf_gain.s[3],ori_hpf_gain.s[4],ori_hpf_gain.s[5]);
#endif

	 // clear eq  delay register by vbc
	 VB_SetHpfMode(1);

	 //clean eq delay register by filter itself
	 for(i=1;i<=HPF_S_GAIN_NUM;i++)//hpf_1 ~ hpf_6
	 {
		 VB_SetHpfParas(i,0,0,0,0,0,0);
	 }
	 //usleep(2*1000);

	 //set filter hpf_1 --- lcf
	 VB_SetHpfParas(1,lcf_filter_set.B0, lcf_filter_set.B1, lcf_filter_set.B2,
		 16384, -lcf_filter_set.A1, -lcf_filter_set.A2);


	 //set filter hpf_2 ~ hpf_6 --- 5-band eq
	 for(i=0;i<hpf_param_ptr->eq_band_num;i++)
	 {
		 VB_SetHpfParas(i+2, eq_filter_set[i].B0,eq_filter_set[i].B1, eq_filter_set[i].B2,
			 16384,-eq_filter_set[i].A1, -eq_filter_set[i].A2);
	 }

	 // finish clear process ;and the fileters begin working normally
	 VB_SetHpfMode(0);

	 //fading in------
	 AUDENHA_FadeIn(hpf_gain_ptr,HPF_FADE_IN_TOTAL_TIME,HPF_FADE_IN_GAIN_SET_TIMES);

	 return SCI_TRUE;
 }

 /*****************************************************************************/
 //  Description:	 set input gain of alc (alc_ingain_Set)
 //  Author:		 Cherry.Liu
 //  Note:			 /*do we need fade out&in operation? cherry needs check here*/
 //****************************************************************************/
 LOCAL BOOLEAN AUDENHA_SetAlcIngain(
	 ALC_CONTROL_PARAM_T* alc_param_ptr 	   //alc
 )
 {
	 int32_t  temp = 0;
	 int16_t  input_gain  = alc_param_ptr->alc_input_gain;
	 int16_t  in_gain_set = alc_param_ptr->alc_ingain_Set;

	 //input gain
	 temp = (input_gain * in_gain_set)>>12;
	 if(temp<=32767)
	 {
		 input_gain   = temp;
	 }
	 else
	 {
		 input_gain   = 32767;
	 }

	 //s6
	 VB_SetHpfGain(6, input_gain);

	 return SCI_TRUE;
 }

 /*****************************************************************************/
 //  Description:	 enable DG Module
 //  Author:		 Cherry.Liu
 //  Note:
 //****************************************************************************/
 LOCAL int AUDENHA_initDigiGain(
	 DG_CONTROL_PARAM_T* dg_param_ptr
	 )
 {
	 //DG gain set
	 VB_SetDG (VBC_DA_LEFT,  dg_param_ptr->left_gain);
	 VB_SetDG (VBC_DA_RIGHT,  dg_param_ptr->right_gain);


	 //DG switch
	 VB_DGSwitch(VBC_DA_LEFT, SCI_TRUE);
	 VB_DGSwitch(VBC_DA_RIGHT, SCI_TRUE);
	 // printk(KERN_DEBUG "CONFIG: AUDENHA_initDigiGain VBC_DA_LEFT=TRUE VBC_DA_RIGHT=SCI_TRUE \n");


	 return AUDIO_NO_ERROR;
 }

 /*****************************************************************************/
 //  Description:	 init da path
 //  Author:		 Cherry.Liu
 //  Note:
 //****************************************************************************/
 LOCAL BOOLEAN AUDENHA_initDAPath(
	 DAPATH_CONTROL_PARAM_T* dapath_param_ptr
	 )
 {

	 VB_SetFMMixMode(VBC_DA_LEFT, dapath_param_ptr->left_fm_mix_mode);
	 VB_SetFMMixMode(VBC_DA_RIGHT, dapath_param_ptr->right_fm_mix_mode);

	 return SCI_TRUE;
 }


 /*****************************************************************************/
 //  Description:	 int alc module and enable it
 //  Author:		 Cherry.Liu
 //  Note:
 //****************************************************************************/
 LOCAL int AUDENHA_initAlc(
	 ALC_CONTROL_PARAM_T* alc_param_ptr
 )
 {
	 int32_t   temp = 0;
	 int16_t   input_gain = 0; //4096 sacled
	 BOOLEAN alc_sw = SCI_FALSE;

	 //alc sw
	 alc_sw 	 = alc_param_ptr->alc_sw;

	 //calc input gain
	 temp = (alc_param_ptr->alc_input_gain * alc_param_ptr->alc_ingain_Set)>>12;
	 if(temp<=32767)
	 {
		 input_gain   = temp;
	 }
	 else
	 {
		 input_gain   = 32767;
	 }

	 //set the input gain to s6
	 VB_SetHpfGain((TOTAL_S_GAIN_NUM-1), input_gain);


	 if(alc_sw)
	 {
		 //alc para
		 VB_SetALCParas(&alc_param_ptr->alc_Para);

		 //enable alc
		 VB_ALCSwitch(SCI_TRUE);
	 }
	 else
	 {
		 //disenable alc
		 VB_ALCSwitch(SCI_FALSE);
	 }

	 return AUDIO_NO_ERROR;
 }


 /*****************************************************************************/
 //  Description:	 init HPF filters:
 // 				 the first hpf is for lcf;
 // 				 the second to sixth hp are for 5-band eq;
 //
 //  Author:		 Cherry.Liu
 //  Note:
 //****************************************************************************/
 PUBLIC int AUDENHA_InitHpf(
	 HPF_CONTROL_PARAM_T* hpf_param_ptr,
	 int32_t sample_rate
 )
 {
	 BOOLEAN return_value = SCI_FALSE;

	 VB_SetHpfLimit((int8_t)(hpf_param_ptr->r_limit)); //RLimit:bit7~0
	 VB_SetHpfWidth(hpf_param_ptr->data_width); 	  //IIS_Bits_select
	 VB_SwitchHpf(SCI_TRUE);		  //HPF ENABLE


	 return_value = AUDENHA_SetHpf(hpf_param_ptr,
		 sample_rate);

	 if(!return_value)
	 {
		 printk("vbc_codec_eq.c,[AUDENHA_InitHpf]	set eq failed! \n");
		 return AUDIO_ERROR;
	 }

	 return AUDIO_NO_ERROR;
 }

 /**---------------------------------------------------------------------------*
  **						 Public function definitions					   *
  **---------------------------------------------------------------------------*/
 /*****************************************************************************/
 //  Description:	 set eq mode para for audio process modules, including lcf,alc and eq.
 //  Author:		 Cherry.Liu
 //  Note:
 //****************************************************************************/
 PUBLIC int AUDENHA_SetEqMode(
	 ALC_CONTROL_PARAM_T* alc_param_ptr,		//alc
	 HPF_CONTROL_PARAM_T* hpf_param_ptr,		//hpf
	 int32_t sample_rate						 //fs
 )
 {
	 BOOLEAN set_return_value = SCI_FALSE;

	 //set agc in gain set
	 AUDENHA_SetAlcIngain(alc_param_ptr);

	 //set lcf & eq (hpf_1 ~hpf_6)
	 set_return_value = AUDENHA_SetHpf(hpf_param_ptr,
		 sample_rate);

	 if(!set_return_value)
	 {
		 // printk("vbc_codec_eq.c,[AUDENHA_SetEqMode]  set hpf failed! \n");
		 return AUDIO_ERROR;
	 }

	 return AUDIO_NO_ERROR;

 }

 /*****************************************************************************/
 //  Description:	 set digital gain
 //  Author:		 Cherry.Liu
 //  Note:			 dg gain here is mapped to arm volume[i] bit15 ~ bit9
 //****************************************************************************/
 PUBLIC BOOLEAN AUDENHA_SetDigiGain(
	 DG_CONTROL_PARAM_T* dg_param_ptr
 )
 {

	 VB_SetDG (VBC_DA_LEFT,dg_param_ptr->left_gain);
	 VB_SetDG (VBC_DA_RIGHT,dg_param_ptr->right_gain);

	 return SCI_TRUE;
 }

 /*****************************************************************************/
 //  Description:	 init audio process modules including lcf,agc and eq.
 //  Author:		 Cherry.Liu
 //  Note:			 !attention! you should init it before music start.
 //****************************************************************************/
 PUBLIC int AUDENHA_Init(
	 DG_CONTROL_PARAM_T* dg_param_ptr,			//dg
	 DAPATH_CONTROL_PARAM_T * dapath_param_ptr, //da path
	 ALC_CONTROL_PARAM_T* alc_param_ptr,		//alc
	 HPF_CONTROL_PARAM_T* hpf_param_ptr,		//hpf
	 int32_t sample_rate						//fs
 )
 {
	 BOOLEAN init_return_value = SCI_FALSE;

	 //digital gain
	 AUDENHA_initDigiGain(dg_param_ptr);
	 // printk("AUDENHA_Init:AUDENHA_initDigiGain left_gain=%d right_gain=%d \n",dg_param_ptr->left_gain,dg_param_ptr->right_gain);
	 //da path control
	 AUDENHA_initDAPath(dapath_param_ptr);
	 // printk("AUDENHA_Init:AUDENHA_initDAPath left_fm_mix_mode=%d right_fm_mix_mode=%d \n",dapath_param_ptr->left_fm_mix_mode,dapath_param_ptr->right_fm_mix_mode);

	 //alc init
	 AUDENHA_initAlc(alc_param_ptr);
 //  printk("AUDENHA_Init:AUDENHA_initAlc \n",alc_param_ptr->alc_sw,alc_param_ptr->alc_input_gain,alc_param_ptr->alc_ingain_Set,alc_param_ptr->alc_Para.);

	 //hpf init
	 init_return_value = AUDENHA_InitHpf(hpf_param_ptr,sample_rate);
	 if(AUDIO_NO_ERROR != init_return_value)
	 {
		 // printk("vbc_codec_eq.c,[AUDENHA_Init] Hpf init failed! \n");
		 return AUDIO_ERROR;
	 }

	 return AUDIO_NO_ERROR;
 }

 /*****************************************************************************/
 //  Description:	 deinti aud proc plugger
 //  Author:		 Cherry.Liu
 //  Note:
 //****************************************************************************/
 PUBLIC BOOLEAN AUDENHA_DeInit(
	 void
 )
 {

	 int32_t i=0;

	 // printk("vbc_codec_eq.c,[AUDENHA_InitHpf]	eq is off! \n");

	 //set s0  ~ s6 to zero
	 for(i=0;i<TOTAL_S_GAIN_NUM;i++)
	 {
		 VB_SetHpfGain(i, 0);
	 }

	 //clean delay register
	 VB_SetHpfMode(SCI_TRUE);		  // clear hpf delay register
	 //msleep(SCI_TRUE);			   // the clear process need at least 10us
	 VB_SetHpfMode(SCI_FALSE);		   // finish clear process

	 //shut down hpf(including lcf eq alc)
	 VB_SwitchHpf(SCI_FALSE);
	 VB_ALCSwitch(SCI_FALSE);
	 VB_DGSwitch(VBC_DA_LEFT, SCI_FALSE);
	 VB_DGSwitch(VBC_DA_RIGHT, SCI_FALSE);

	 return SCI_TRUE;

 }



 /*****************************************************************************/
 //  Description:	get paras from eq para set
 //
 //  Author:		 Cherry.Liu
 //  Note:
 //****************************************************************************/
 LOCAL int	AUDENHA_GetParaFromEqset(
	 BOOLEAN is_eq_para_tunable,
	 uint32_t eq_para_set_index,
	 uint32_t tunable_eq_para_set_index
	 )
 {
	 uint16_t eq_mode_index = 0;
	 uint16_t i =0;

	 // printk("aud_enha_exp.c,[AUDENHA_GetParaFromEqset]is_eq_para_tunable:%d,eq_para_set_index:%d,tunable_eq_para_set_index:%d,s_cur_eq_mode_sel:%d  \n",
	 //    is_eq_para_tunable,eq_para_set_index,tunable_eq_para_set_index,s_cur_eq_mode_sel);
 //  s_cur_eq_mode_sel=AUD_ENHA_EQMODE_SEL_REGULAR;   //test
	 //no eq
	 if(AUD_ENHA_EQMODE_SEL_OFF == s_cur_eq_mode_sel)
		 {
		 //(no eq effect)five band eqs are all-pass filters;dg on;alc on
		 //s_cur_alc_param
		 s_cur_hpf_param.eq_band_num	= HPF_S_GAIN_NUM-1;
		 s_cur_alc_param.alc_ingain_Set = 4096;

		 //s_cur_hpf_param
		 for(i=0;i<EQ_BAND_MAX;i++)
		 {
			 s_cur_hpf_param.band_sw[i] =  SCI_FALSE;
		 }
		  // printk("aud_enha_exp.c,[AUDENHA_GetParaFromEqset] no eq ! \n");
		  return AUDIO_NO_ERROR;
		 }

	 // eq on
	 if(0 == eq_para_set_index)
	 {
		 // printk("aud_enha_exp.c,[AUDENHA_GetParaFromEqset] eq_para_set_index == 0! \n");
		 return AUDIO_ERROR;
	 }
	 if(AUD_ENHA_EQMODE_SEL_MMISET == s_cur_eq_mode_sel)
	 {
		 eq_mode_index = 0;
	 }
	 else
	 {
		 eq_mode_index = s_cur_eq_mode_sel-1; // mode set by nv
	 }

	 if(!is_eq_para_tunable)
	 {
		 AUDIO_ENHA_EQ_STRUCT_T* eq_para_ptr = SCI_NULL;
		 eq_para_ptr = &s_enha_eq_para_nv.eq_para_set;

		 //s_cur_alc_param
		 s_cur_alc_param.alc_ingain_Set = eq_para_ptr->eq_modes[eq_mode_index].agc_in_gain;

		 //s_cur_hpf_param
		 if(eq_para_ptr->eq_control&0x8000)
		 {
			 s_cur_hpf_param.eq_band_num = 8;
		 }
		 else
		 {
			 s_cur_hpf_param.eq_band_num = 5;
		 }
		 s_cur_hpf_param.low_shelve_on	= eq_para_ptr->eq_modes[eq_mode_index].band_control &0x1;//bit0
		 s_cur_hpf_param.high_shelve_on = eq_para_ptr->eq_modes[eq_mode_index].band_control &0x2;//bit1

		 for(i=0;i<s_cur_hpf_param.eq_band_num;i++)
		 {
			 s_cur_hpf_param.band_sw[i] = ((eq_para_ptr->eq_modes[eq_mode_index].band_control)&(1<<(15-i))) ? SCI_TRUE : SCI_FALSE;
		 }

		 memcpy(&s_cur_hpf_param.eq_band_para[0],&eq_para_ptr->eq_modes[eq_mode_index].eq_band[0],sizeof(EQ_BAND_INPUT_PARA_T)*s_cur_hpf_param.eq_band_num);
	 }
	 else
	 {
		 AUDIO_ENHA_TUNABLE_EQ_STRUCT_T* tunable_eq_para_ptr = SCI_NULL;

		 if(tunable_eq_para_set_index > AUD_ENHA_TUNABLE_EQPARA_COMMON)
		 {
			 tunable_eq_para_set_index = AUD_ENHA_TUNABLE_EQPARA_COMMON;
		 }

		 tunable_eq_para_ptr = &s_enha_eq_para_nv.tunable_eq_para_set[tunable_eq_para_set_index-1];

		 //s_cur_alc_param
		 s_cur_alc_param.alc_ingain_Set = tunable_eq_para_ptr->eq_modes[eq_mode_index].agc_in_gain;

		 //s_cur_hpf_param
		 if(tunable_eq_para_ptr->eq_control&0x8000)
		 {
			 s_cur_hpf_param.eq_band_num = 8;
		 }
		 else
		 {
			 s_cur_hpf_param.eq_band_num = 5;
		 }
		 s_cur_hpf_param.low_shelve_on	= tunable_eq_para_ptr->eq_modes[eq_mode_index].band_control &0x1;//bit0
		 s_cur_hpf_param.high_shelve_on = tunable_eq_para_ptr->eq_modes[eq_mode_index].band_control &0x2;//bit1

		 for(i=0;i<s_cur_hpf_param.eq_band_num;i++)
		 {
			 s_cur_hpf_param.band_sw[i] 			 =	SCI_TRUE ;
			 s_cur_hpf_param.eq_band_para[i].fo 	 = tunable_eq_para_ptr->fo_array[i];
			 s_cur_hpf_param.eq_band_para[i].q		 = tunable_eq_para_ptr->q_array[i];
			 s_cur_hpf_param.eq_band_para[i].boostdB = tunable_eq_para_ptr->eq_modes[eq_mode_index].boostdB_current[i]*(tunable_eq_para_ptr->eq_control&0x3ff);
			 s_cur_hpf_param.eq_band_para[i].gaindB  = 0;
		 }

		 // printk("aud_enha_exp.c,[AUDENHA_GetParaFromEqset] tunable eq !current boost array:%d,%d,%d,%d,%d  \n",tunable_eq_para_ptr->eq_modes[eq_mode_index].boostdB_current[0],
		 // 	tunable_eq_para_ptr->eq_modes[eq_mode_index].boostdB_current[1],
		 // 	tunable_eq_para_ptr->eq_modes[eq_mode_index].boostdB_current[2],
		 // 	tunable_eq_para_ptr->eq_modes[eq_mode_index].boostdB_current[3],
		 // 	tunable_eq_para_ptr->eq_modes[eq_mode_index].boostdB_current[4]);
	 }

	 return AUDIO_NO_ERROR;

 }


 /*****************************************************************************/
 //  Description:	get para from audio mode dev info
 //
 //  Author:		 Cherry.Liu
 //  Note:
 //****************************************************************************/
 LOCAL int	AUDENHA_GetParaFromAudMode(
	 AUDIO_NV_ARM_MODE_INFO_T* armAudioInfo
 )
 {
	 uint16_t aud_proc_control[2];

	 //aud_proc_control
	 aud_proc_control[0] = armAudioInfo->tAudioNvArmModeStruct.app_config_info_set.aud_proc_exp_control[0];//bit7:defined;bit3-bit0:defined;bit4-bit6:defined
	 aud_proc_control[1] = armAudioInfo->tAudioNvArmModeStruct.app_config_info_set.aud_proc_exp_control[1];//bit15-bit8:agc sw;bit7-bit0:lcf sw


	 // printk("aud_enha_exp.c,[AUDENHA_GetParaFromAudMode] aud_proc_control[0]:0x%x;aud_proc_control[1]:0x%x  \n", aud_proc_control[0],aud_proc_control[1]);


	 //s_cur_alc_param
	 s_cur_alc_param.alc_sw  = (aud_proc_control[1] & (1 << 8)) ? SCI_TRUE : SCI_FALSE;//bit 8
	 s_cur_alc_param.alc_input_gain  =	armAudioInfo->tAudioNvArmModeStruct.app_config_info_set.app_config_info[s_cur_music_type].agc_input_gain[s_cur_music_type]; //s_cur_music_type=0:multimedia play(mp3)
	 memcpy(&s_cur_alc_param.alc_Para,&armAudioInfo->tAudioNvArmModeStruct.reserve[7],sizeof(VBC_ALC_PARAS_T));

	 // printk("aud_enha_exp.c,[AUDENHA_GetParaFromAudMode] s_cur_alc_param.alc_sw:%d;s_cur_alc_param.alc_input_gain:%d  \n",s_cur_alc_param.alc_sw,s_cur_alc_param.alc_input_gain);

	 //s_cur_eq_mode_sel
	 s_cur_eq_mode_sel = (AUD_ENHA_EQMODE_SEL_E)(armAudioInfo->tAudioNvArmModeStruct.app_config_info_set.app_config_info[s_cur_music_type].eq_switch&0xF);//BIT 3-BIT 0

	 //s_cur_hpf_param
	 s_cur_hpf_param.lcf_para.isFilterOn = (aud_proc_control[1] & 0x1) ? SCI_TRUE : SCI_FALSE;//bit 0
	 s_cur_hpf_param.lcf_para.eLcfParaType = (FILTER_LCFPARA_TYPE_E)(armAudioInfo->tAudioNvArmModeStruct.reserve[0]&0x0700);   //BIT10	- BIT8

	 if(FILTER_LCFPARA_F1F1 == s_cur_hpf_param.lcf_para.eLcfParaType)
	 {
		 s_cur_hpf_param.lcf_para.unlcfPara.lcfPara.f1_g0 = armAudioInfo->tAudioNvArmModeStruct.reserve[1];
		 s_cur_hpf_param.lcf_para.unlcfPara.lcfPara.f1_g1 = armAudioInfo->tAudioNvArmModeStruct.reserve[2];
		 s_cur_hpf_param.lcf_para.unlcfPara.lcfPara.f1_fp = armAudioInfo->tAudioNvArmModeStruct.reserve[3];
		 s_cur_hpf_param.lcf_para.unlcfPara.lcfPara.f2_g0 = armAudioInfo->tAudioNvArmModeStruct.reserve[4];
		 s_cur_hpf_param.lcf_para.unlcfPara.lcfPara.f2_g1 = armAudioInfo->tAudioNvArmModeStruct.reserve[5];
		 s_cur_hpf_param.lcf_para.unlcfPara.lcfPara.f2_fp = armAudioInfo->tAudioNvArmModeStruct.reserve[6];
	 }
	 else
	 {
		 s_cur_hpf_param.lcf_para.unlcfPara.fp	= armAudioInfo->tAudioNvArmModeStruct.reserve[1];//fp
	 }


	 s_cur_hpf_param.r_limit	= (armAudioInfo->tAudioNvArmModeStruct.reserve[0] &0xff);



	 //s_cur_dapath_param
	 s_cur_dapath_param.left_fm_mix_mode  = (armAudioInfo->tAudioNvArmModeStruct.reserve[0])&0x3000;//BIT13-BIT12
	 s_cur_dapath_param.right_fm_mix_mode = (armAudioInfo->tAudioNvArmModeStruct.reserve[0])&0xC000;//BIT15-BIT14
	 // printk("aud_enha_exp.c,[AUDENHA_GetParaFromAudMode] lcf_sw:%d;filter_type:%d r_limit:%d \n",s_cur_hpf_param.lcf_para.isFilterOn,s_cur_hpf_param.lcf_para.eLcfParaType,s_cur_hpf_param.r_limit);

	 return AUDIO_NO_ERROR;
 }

 /*****************************************************************************/
 //  Description:	 参数获取函数
 //  Author:		 Cherry.Liu
 //  Note:
 //****************************************************************************/
 LOCAL int	AUDENHA_GetPara(
	 AUDIO_TOTAL_T *audio_param_ptr
	 )
 {
	 AUDIO_NV_ARM_MODE_INFO_T *armAudioInfo = NULL;
	 char *mode_name = NULL;
	 BOOLEAN eq_para_tunable = SCI_FALSE;
	 int32_t vol_index = 0;
	 int32_t arm_vol = 0;

	 s_cur_sample_rate=get_cur_sample_rate();
	 vol_index = audio_param_ptr->audio_nv_arm_mode_info.tAudioNvArmModeStruct.app_config_info_set.app_config_info[s_cur_music_type].valid_volume_level_count;

	 arm_vol=audio_param_ptr->audio_nv_arm_mode_info.tAudioNvArmModeStruct.app_config_info_set.app_config_info[s_cur_music_type].arm_volume[vol_index];
	 s_cur_dg_param.left_gain = ((arm_vol & 0xffff0000) >> 16);				 //get s_cur_dg_param
	 s_cur_dg_param.right_gain = ((arm_vol & 0xffff0000) >> 16);

	 armAudioInfo = &audio_param_ptr->audio_nv_arm_mode_info;
	 mode_name = (char *)armAudioInfo->ucModeName;
	 if(NULL==mode_name)
	 {
		 // printk("aud_enha_exp.c AUDENHA_GetPara mode_name:%s.\n", mode_name);
		 return AUDIO_ERROR;
	 }

	 AUDENHA_GetParaFromAudMode(armAudioInfo);
	 s_enha_eq_para_nv.eq_para_set= audio_param_ptr->audio_enha_eq;


	 //s_cur_tunable_eq_para_set_index & s_cur_eq_para_set_index
	 if(0 == memcmp((void*)mode_name, "Headset",7))
	 {
		 s_cur_eq_para_set_index = 1;
	 }
	 else if(0 == memcmp((void*)mode_name, "Headfree",8))
	 {
		 s_cur_eq_para_set_index = 2;
	 }
	 else if(0 == memcmp((void*)mode_name, "Handset",7))
	 {
		 s_cur_eq_para_set_index = 3;
	 }
	 else
	 {
		 s_cur_eq_para_set_index = 4;
	 }

	 // printk("AUDENHA_GetPara mode_name:%s , s_cur_eq_para_set_index :%d \n",mode_name,s_cur_eq_para_set_index);
	if(armAudioInfo->tAudioNvArmModeStruct.reserve[0]&0x0800)
	 {
		 //EQ  Tunable
		 eq_para_tunable = SCI_TRUE;
	 }
	 else
	 {
		 //EQ  unTunable
		 eq_para_tunable = SCI_FALSE;
	 }

	 if( AUDIO_NO_ERROR != AUDENHA_GetParaFromEqset(SCI_FALSE,s_cur_eq_para_set_index,s_cur_tunable_eq_para_set_index) ) //pass eq
	 {
		 // printk("aud_enha_exp.c AUDENHA_GetPara param error!\n");
		 return AUDIO_ERROR;
	 }

 //s_cur_hpf_param.s_gain.s[0]~[6]
	 s_cur_hpf_param.s_gain.s[0] = 4096;
	 s_cur_hpf_param.s_gain.s[1] = 4096;
	 s_cur_hpf_param.s_gain.s[2] = 4096;
	 s_cur_hpf_param.s_gain.s[3] = 4096;
	 s_cur_hpf_param.s_gain.s[4] = 4096;
	 s_cur_hpf_param.s_gain.s[5] = 4096;
	 s_cur_hpf_param.data_width = HPF_DATA_WIDTH_24;

	 return AUDIO_NO_ERROR;
 }

LOCAL void init_regs()
{
     DAPATCHCTL	=	(VB_MAP_BASE + 0x0000);
     DADGCTL	=	(VB_MAP_BASE + 0x0004);
     DAHPCTL	=	(VB_MAP_BASE + 0x0008);
     DAALCCTL0	=	(VB_MAP_BASE + 0x000C);
     DAALCCTL1	=	(VB_MAP_BASE + 0x0010);
     DAALCCTL2	=	(VB_MAP_BASE + 0x0014);
     DAALCCTL3	=	(VB_MAP_BASE + 0x0018);
     DAALCCTL4	=	(VB_MAP_BASE + 0x001C);
     DAALCCTL5	=	(VB_MAP_BASE + 0x0020);
     DAALCCTL6	=	(VB_MAP_BASE + 0x0024);
     DAALCCTL7	=	(VB_MAP_BASE + 0x0028);
     DAALCCTL8	=	(VB_MAP_BASE + 0x002C);
     DAALCCTL9	=	(VB_MAP_BASE + 0x0030);
     DAALCCTL10	=	(VB_MAP_BASE + 0x0034);
     STCTL0	=	(VB_MAP_BASE + 0x0038);
     STCTL1	=	(VB_MAP_BASE + 0x003C);
     ADPATCHCTL	=	(VB_MAP_BASE + 0x0040);
     ADDGCTL	=	(VB_MAP_BASE + 0x0044);
     HPCOEF0	=	(VB_MAP_BASE + 0x0048);
     HPCOEF1	=	(VB_MAP_BASE + 0x004C);
     HPCOEF2	=	(VB_MAP_BASE + 0x0050);
     HPCOEF3	=	(VB_MAP_BASE + 0x0054);
     HPCOEF4	=	(VB_MAP_BASE + 0x0058);
     HPCOEF5	=	(VB_MAP_BASE + 0x005C);
     HPCOEF6	=	(VB_MAP_BASE + 0x0060);
     HPCOEF7	=	(VB_MAP_BASE + 0x0064);
     HPCOEF8	=	(VB_MAP_BASE + 0x0068);
     HPCOEF9	=	(VB_MAP_BASE + 0x006C);
     HPCOEF10	=	(VB_MAP_BASE + 0x0070);
     HPCOEF11	=	(VB_MAP_BASE + 0x0074);
     HPCOEF12	=	(VB_MAP_BASE + 0x0078);
     HPCOEF13	=	(VB_MAP_BASE + 0x007C);
     HPCOEF14	=	(VB_MAP_BASE + 0x0080);
     HPCOEF15	=	(VB_MAP_BASE + 0x0084);
     HPCOEF16	=	(VB_MAP_BASE + 0x0088);
     HPCOEF17	=	(VB_MAP_BASE + 0x008C);
     HPCOEF18	=	(VB_MAP_BASE + 0x0090);
     HPCOEF19	=	(VB_MAP_BASE + 0x0094);
     HPCOEF20	=	(VB_MAP_BASE + 0x0098);
     HPCOEF21	=	(VB_MAP_BASE + 0x009C);
     HPCOEF22	=	(VB_MAP_BASE + 0x00A0);
     HPCOEF23	=	(VB_MAP_BASE + 0x00A4);
     HPCOEF24	=	(VB_MAP_BASE + 0x00A8);
     HPCOEF25	=	(VB_MAP_BASE + 0x00AC);
     HPCOEF26	=	(VB_MAP_BASE + 0x00B0);
     HPCOEF27	=	(VB_MAP_BASE + 0x00B4);
     HPCOEF28	=	(VB_MAP_BASE + 0x00B8);
     HPCOEF29	=	(VB_MAP_BASE + 0x00BC);
     HPCOEF30	=	(VB_MAP_BASE + 0x00C0);
     HPCOEF31	=	(VB_MAP_BASE + 0x00C4);
     HPCOEF32	=	(VB_MAP_BASE + 0x00C8);
     HPCOEF33	=	(VB_MAP_BASE + 0x00CC);
     HPCOEF34	=	(VB_MAP_BASE + 0x00D0);
     HPCOEF35	=	(VB_MAP_BASE + 0x00D4);
     HPCOEF36	=	(VB_MAP_BASE + 0x00D8);
     HPCOEF37	=	(VB_MAP_BASE + 0x00DC);
     HPCOEF38	=	(VB_MAP_BASE + 0x00E0);
     HPCOEF39	=	(VB_MAP_BASE + 0x00E4);
     HPCOEF40	=	(VB_MAP_BASE + 0x00E8);
     HPCOEF41	=	(VB_MAP_BASE + 0x00EC);
     HPCOEF42	=	(VB_MAP_BASE + 0x00F0);
}

 /*****************************************************************************/
 //  Description:	 参数设置函数
 //  Author:		 Cherry.Liu
 //  Note:
 //****************************************************************************/
 PUBLIC int  AUDENHA_SetPara(
	 AUDIO_TOTAL_T *audio_param_ptr,
	uint32_t *para_buf_ptr
	 )
 {
	if (para_buf_ptr == NULL) {
		printk("error: para_buf_ptr is NULL");
		return -1;
	}

	VB_MAP_BASE = (uint32_t)para_buf_ptr;
	init_regs();
	 if(AUDIO_NO_ERROR != AUDENHA_GetPara(audio_param_ptr))
	 {
		 // printk("aud_enha_exp.c AUDENHA_GetPara error!\n");
		 return AUDIO_ERROR;
	 }

	 if(AUDIO_NO_ERROR != AUDENHA_Init(&s_cur_dg_param,   //dg
			 &s_cur_dapath_param,	   //da path
			 &s_cur_alc_param,		   //alc
			 &s_cur_hpf_param,		   //hpf
			 s_cur_sample_rate	//fs
			 ))
	 {
		 // printk("aud_enha_exp.c AUDENHA_Init error! \n");
		 return AUDIO_ERROR;
	 }
	 // printk("aud_enha_exp.c AUDENHA_SetPara success!\n");
	 return AUDIO_NO_ERROR;
 }



 /**---------------------------------------------------------------------------**
  **						 Compiler Flag									   **
  **---------------------------------------------------------------------------**/
#ifdef __cplusplus
 }
#endif //end of file







