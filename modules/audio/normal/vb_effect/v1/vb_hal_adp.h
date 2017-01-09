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
#ifndef _AUDIO_DEV_PHY_H_
#define _AUDIO_DEV_PHY_H_

#ifdef __cplusplus
extern   "C"
{
#endif

#include "vb_hal.h"

typedef struct
{
	uint32_t dac_pga_l;
	uint32_t dac_pga_r;
	uint32_t hp_pga_l;
	uint32_t hp_pga_r;
}CODEC_DAC_OUTPUT_PGA_T;
#define DOL_DAC_GOD_SHIFT	0
#define DOL_DAC_GO_SHIFT	4

#define DOL_DAC_GOD_MASK  (0xF << DOL_DAC_GOD_SHIFT)
#define DOL_DAC_GO_MASK  (0x1F << DOL_DAC_GO_SHIFT)


PUBLIC void VB_PHY_HPFSwitch(BOOLEAN is_enable);

PUBLIC void VB_PHY_SetHPFMode(BOOLEAN is_reset);

PUBLIC void VB_PHY_SetHPFGain (
    int16_t gain_index,//gain index:0,1,2,3,4,5,6
    int16_t gain_value
);

PUBLIC uint32_t VB_PHY_GetHPFGain (
    int16_t gain_index
);

PUBLIC void VB_PHY_SetHPFLimit (
    int8_t rLimit //0~127
);

PUBLIC void VB_PHY_SetHPFWidth (
    uint16_t  width //width = 16 or 24 (bits)
);

PUBLIC void VB_PHY_ALCSwitch (
    BOOLEAN is_enable
);

PUBLIC void VB_PHY_SetALCParas (
    VBC_ALC_PARAS_T *ptAlcPara
);

PUBLIC void VB_PHY_SetFMMixMode (
    int16_t da_index,//0,1
    int16_t mix_mode  //0,1,2
);

PUBLIC void VB_PHY_DGSwitch (
    int16_t da_index,//0,1
    BOOLEAN is_enable
);

PUBLIC void VB_PHY_SetDG (
    int16_t da_index,//0,1
    int16_t dg_value
);

PUBLIC void VB_PHY_SetHPFParas (
    int16_t filter_index,
    int16_t B0,
    int16_t B1,
    int16_t B2,
    int16_t A0,
    int16_t minusA1,
    int16_t minusA2
);

PUBLIC void CODEC_PHY_SetDACPGA(uint32_t uichannel,CODEC_DAC_OUTPUT_PGA_T pga);

#endif







