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
#ifndef _VB_HAL_H_
#define _VB_HAL_H_

#include <stdint.h>
#include <eng_audio.h>
#include "aud_common.h"

/**---------------------------------------------------------------------------*
 **                         Dependencies                                      *
 **---------------------------------------------------------------------------*/
/**---------------------------------------------------------------------------*
 **                         Compiler Flag                                     *
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern   "C"
{
#endif

/**---------------------------------------------------------------------------*
 **                         MACRO Definations                                 *
 **---------------------------------------------------------------------------*/
#define VB_FIFO_SIZE        (160)

#define AUDDEV_MIC_SEL_1    0
#define AUDDEV_MIC_SEL_2    1


/**---------------------------------------------------------------------------*
 **                         Data Structures                                   *
 **---------------------------------------------------------------------------*/
typedef enum
{
    VBC_HOST_ARM = 0,
    VBC_HOST_DSP
}VBC_HOST_E;

#define AUDDEV_FUNC_NUM         (5UL)

#define AUDDEV_FUNC_DATAIN      (0x80000000UL)
#define AUDDEV_FUNC_DATAOUT     (0x40000000UL)
#define AUDDEV_FUNC_DSP_CTL     (0x20000000UL)
#define AUDDEV_FUNC_EXT_IIS     (0x10000000UL)
#define AUDDEV_FUNC_LINE_IN     (0x08000000UL)
#define AUDDEV_FUNC_LINE_IN_REC (0x04000000UL)
#define AUDDEV_FUNC_HP          (0x02000000UL)
#define AUDDEV_FUNC_LOUT        (0x01000000UL)

#define AUDDEV_FUNC_NONE        (0UL)

typedef enum
{
    VBC_DA_RIGHT = 0,//DAC0
    VBC_DA_LEFT,    //DAC1
    VBC_DA_MUX
} VBC_DA_CHANNEL_E;

typedef enum
{
    VBC_DA_MIX_BYPASS = 0,//st or fm is bypassed
    VBC_DA_MIX_ADD,          //st or fm added to da path
    VBC_DA_MIX_SUBTRACT,    //st or fm subtracted from da path
    VBC_DA_MIX_MUX
} VBC_DA_MIX_MODE_E; //VBC_DA_MIX_MODE_E;

typedef struct
{
    int16_t  hold;
    int16_t  rise;
    int16_t  fall;
    int16_t  limit;
    int16_t  threshold;
    int16_t  ratio;
    int16_t  cg_var;
    int16_t  release_rate;
    int16_t  attack_rate;
    int16_t  release_rate_ex;
    int16_t  attack_rate_ex;
} VBC_ALC_PARAS_T;

typedef void (*AUDDEV_OUTPUT_DATA_PFUNC) (void);
typedef void (*AUDDEV_INPUT_DATA_PFUNC) (uint32_t);

typedef void (*AUDDEV_PA_CONTROL_PFUNC) (BOOLEAN bEnable);
typedef void (*AUDDEV_DMA_OUTPUT_CALLBACK_PFUNC) (uint32_t);
typedef void (*AUDDEV_DMA_INPUT_CALLBACK_PFUNC) (uint32_t);

/**---------------------------------------------------------------------------*
**                         Global Variables                                   *
**----------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
 **                         Constant Variables                                *
 **---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
 **                         Function Prototypes                               *
 **---------------------------------------------------------------------------*/
/*****************************************************************************/
// Description :    Initializes the module in system starting up phase.
// Author :         Jeff.Li
// Note :
/*****************************************************************************/
PUBLIC void AUDDEV_Init (void);

/*****************************************************************************/
// Description :    Audio device openning interface called by upper layer. It
//                  opens the audio device with enabling the specific funtion,
//                  if the specific function is openned previously, or the fun-
//                  ction is conflicting with other funtion which is applying,
//                  the interface would return directly.
// Author :         Jeff.Li
// Note :
/*****************************************************************************/
PUBLIC BOOLEAN AUDDEV_Open (
    uint32_t auddev_func,
    BOOLEAN is_turn_on_slowly,
    uint32_t sample_rate
);

/*****************************************************************************/
// Description :    Audio device openning interface called by upper layer. It
//                  opens the audio device with enabling the specific funtion,
//                  if the specific function is openned previously, or the fun-
//                  ction is conflicting with other funtion which is applying,
//                  the interface would return directly.
// Author :         Jeff.Li
// Note :
/*****************************************************************************/
PUBLIC BOOLEAN AUDDEV_Close (
    uint32_t auddev_func,
    BOOLEAN is_shut_down_slowly
);

/*****************************************************************************/
// Description :    Mute or dis-mute the headphone output directly.
// Author :         Jeff.Li
// Note :
/*****************************************************************************/
PUBLIC void AUDDEV_HeadPhoneMute (BOOLEAN is_mute);

/*****************************************************************************/
// Description :    Mute or dis-mute the earphone output directly.
// Author :         Jeff.Li
// Note :
/*****************************************************************************/
PUBLIC void AUDDEV_EarPhoneMute (BOOLEAN is_mute);

/*****************************************************************************/
// Description :    Switches the headphone on or off by state machine when the
//                  codec is opened, and while the codec is openning or closing,
//                  just mute or dismute the headphone.
// Author :         Jeff.Li
// Note :
/*****************************************************************************/
PUBLIC void AUDDEV_HeadPhoneSwitch (BOOLEAN is_on);

/*****************************************************************************/
// Description :    Switches the earphone on or off directly.
// Author :         Jeff.Li
// Note :
/*****************************************************************************/
PUBLIC void AUDDEV_EarPhoneSwitch (BOOLEAN is_on);

/*****************************************************************************/
// Description :    Switches the line-out on or off directly.
// Author :         Jeff.Li
// Note :
/*****************************************************************************/
PUBLIC void AUDDEV_LineOutSwitch (BOOLEAN is_on);

/*****************************************************************************/
// Description :    Selects the MIC channel which inputting to ADC.
// Author :         Jeff.Li
// Note :
/*****************************************************************************/
PUBLIC void AUDDEV_MICMuxSel (uint8_t mic_sel);

/*****************************************************************************/
//  Description:    Registers a data procedure function.
//  Author:         Jeff.Li
//  Note:
//****************************************************************************/
PUBLIC void AUDDEV_RegDataOutProcFunc (AUDDEV_OUTPUT_DATA_PFUNC pProcFunc);

/*****************************************************************************/
//  Description:    Registers a data procedure function.
//  Author:         Jeff.Li
//  Note:
//****************************************************************************/
PUBLIC void AUDDEV_RegDataInProcFunc (AUDDEV_INPUT_DATA_PFUNC pProcFunc);

/*****************************************************************************/
//  Description:    Registers a PA control function.
//  Author:         Jeff.Li
//  Note:
//****************************************************************************/
PUBLIC void AUDDEV_RegPACtlFunc (AUDDEV_PA_CONTROL_PFUNC pControlFunc);

/*****************************************************************************/
//  Description:    This function is used to know which is the controller of
//                  VB, ARM or DSP.
//  Author:         Benjamin.Wang
//  Note:
//****************************************************************************/
PUBLIC BOOLEAN VB_IsControlledbyDSP (void);

/*****************************************************************************/
//  Description:    Sets the PGA of DAC path.
//  Author:         Jeff.Li
//  Note:
/*****************************************************************************/
PUBLIC void AUDDEV_SetPGA (uint32_t uiChannel, uint32_t uiGain);

/*****************************************************************************/
//  Description:    Sets the PGA of line-in path.
//  Author:         Jeff.li
//  Note:
//
/*****************************************************************************/
PUBLIC void AUDDEV_SetLineInPGA (uint32_t uiChannel, uint32_t uiGain);

/*****************************************************************************/
//  Description:    Sets the PGA of ADC path.
//  Author:         Jeff.Li
//  Note:
/*****************************************************************************/
PUBLIC void AUDDEV_SetADPGA (uint32_t uiGain);

/*****************************************************************************/
//  Description:    Sets the PGA of line-in AD path.
//  Author:         ken.kuang
//  Note:
//
/*****************************************************************************/
PUBLIC void AUDDEV_SetLineInADPGA (uint32_t uiGain);

/*****************************************************************************/
//  Description:    This function is used to get the limit to open pa currently.
//  Author:         shujing.dong
//  Note:
//****************************************************************************/
PUBLIC BOOLEAN AUDDEV_GetPAOpenLimit (void);

/*****************************************************************************/
//  Description:    This function is used to open or close PA.
//  Author:         Benjamin.Wang
//  Note:
/*****************************************************************************/
PUBLIC void VB_OpenPA (BOOLEAN is_enable);

/*****************************************************************************/
//  Description:    This function is used to register a vb dma calkback func.
//  Author:
//  Note:
//****************************************************************************/
PUBLIC void AUDDEV_RegDMACallbackFunc (AUDDEV_DMA_OUTPUT_CALLBACK_PFUNC pProcFunc);

/*****************************************************************************/
// Description :    This function is used to register a vb ad dma calkback func.
// Author :         Jeff.Li
// Note :
/*****************************************************************************/
PUBLIC void AUDDEV_RegADDMACallbackFunc (AUDDEV_DMA_INPUT_CALLBACK_PFUNC pProcFunc);

/*****************************************************************************/
// Description :    Enables the DMA of DA channel.
// Author :         Jeff.Li
// Note :
/*****************************************************************************/
PUBLIC void AUDDEV_DMA_EN (
    int16_t *pcm_buf_left,
    int16_t *pcm_buf_right,
    uint32_t byte_to_write
);

/*****************************************************************************/
// Description :    Enables the DMA of DA channel.
// Author :         Jeff.Li
// Note :
/*****************************************************************************/
PUBLIC void AUDDEV_AD_DMA_EN (int16_t *pcm_buf, uint32_t byte_to_read);

/*****************************************************************************/
//  Description:    This function returns whether LineIn is turned on or off.
//  Author:
//  Note:
//****************************************************************************/
PUBLIC BOOLEAN VB_GetLineinStatus (void);

/*****************************************************************************/
//  Description:    This function set VB sound device
//  Author:         Jimmy.Jia
//  Note:           Now we provide this function in l1_midi.c for convenience
/*****************************************************************************/
PUBLIC void VB_ARM_SetSoundDevice (
    BOOLEAN  b_aux_mic,         // TRUE: use mic2,    FALSE: use mic1
    BOOLEAN  b_aux_speaker,     // TRUE: use speaker, FALSE: use receiver
    uint16_t   mode
);

/*****************************************************************************/
// Description :
// Author :         Jeff.Li
// Note :
/*****************************************************************************/
PUBLIC void VB_Hold_by_ARM (uint32_t sample_rate);

/*****************************************************************************/
// Description :
// Author :         Jeff.Li
// Note :
/*****************************************************************************/
PUBLIC void VB_Hold_by_DSP (uint32_t sample_rate);

/*****************************************************************************/
//  Description:    This function set VB CPG
//  Author:         Jimmy.Jia
//  Note:           speaker volume level need TBD
//                  Now we provide this function in l1_midi.c for convenience
/*****************************************************************************/
PUBLIC void VB_ARM_SetVolume (uint32_t  speaker_vol);

/*****************************************************************************/
//  Description:    This function is used to add some operation with VB when
//                  headset has been detectted in/out.
//  Author:         Jeff.Li
//  input:          status:  1--Headset detectted in
//                           0--Headset detectted out
//  Note:
//****************************************************************************/
PUBLIC void AUDDEV_HPDetectRoutine (BOOLEAN status);

/*****************************************************************************/
//  Description:    This function returns the sample rate value which is supp-
//                  orted by codec according to the value input.
//  Author:         Jeff.Li
//  Note:
/*****************************************************************************/
PUBLIC uint32_t AUDDEV_GetSupportedSampleRate (uint32_t sample_rate);

PUBLIC void VB_SwitchHpf (
    BOOLEAN is_enable // 1:open ;0: close
);

PUBLIC void VB_SetHpfMode (
    BOOLEAN is_reset_mode
);
PUBLIC void VB_SetHpfParas (
    int16_t filter_index,//filter index:1,2
    int16_t B0,
    int16_t B1,
    int16_t B2,
    int16_t A0,
    int16_t minusA1,
    int16_t minusA2
);
PUBLIC void VB_SetHpfGain (
    int16_t gain_index,//gain index:0,1,2
    int16_t gain_value
);
PUBLIC uint32_t VB_GetHpfGain (
    int16_t gain_index//gain index:0,1,2
);
PUBLIC void VB_SetHpfLimit (
    int8_t rLimit //0~127
);
PUBLIC void VB_SetHpfWidth (
    uint16_t  width //width = 16 or 24 (bits)
);
/*****************************************************************************/
// Description :
// Author :         cherry.liu
// Note :
/*****************************************************************************/
PUBLIC void VB_ALCSwitch (
    BOOLEAN is_enable
);
/*****************************************************************************/
// Description :
// Author :         cherry.liu
// Note :
/*****************************************************************************/
PUBLIC void VB_SetALCParas (
    VBC_ALC_PARAS_T *ptAlcPara
);
/*****************************************************************************/
// Description :
// Author :         cherry.liu
// Note :
/*****************************************************************************/
PUBLIC void VB_SetFMMixMode (
    VBC_DA_CHANNEL_E da_index,
    VBC_DA_MIX_MODE_E mix_mode
);

/*****************************************************************************/
// Description :
// Author :         cherry.liu
// Note :
/*****************************************************************************/
PUBLIC void VB_DGSwitch (
    VBC_DA_CHANNEL_E da_index,
    BOOLEAN is_enable
);

/*****************************************************************************/
// Description :
// Author :         cherry.liu
// Note :
/*****************************************************************************/
PUBLIC void VB_SetDG (
    VBC_DA_CHANNEL_E da_index,
    int16_t dg_value
);
/**---------------------------------------------------------------------------*
 **                         Compiler Flag                                     *
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif

#endif  // _VB_HAL_H_


