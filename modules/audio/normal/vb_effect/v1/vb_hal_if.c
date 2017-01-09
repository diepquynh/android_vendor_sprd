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
/**---------------------------------------------------------------------------*
 **                         Dependencies                                      *
 **---------------------------------------------------------------------------*/
#include "vbc_codec.h"
#include "vb_hal.h"
#include "vb_hal_adp.h"
#include "filter_calc.h"
#include "aud_enha.h"

/**---------------------------------------------------------------------------*
 **                         Debugging Flag                                    *
 **---------------------------------------------------------------------------*/
//#define AUDDEV_DRV_DEBUG

#ifdef AUDDEV_DRV_DEBUG
#define AUDDEV_PRINT      ALOGW
#else
#define AUDDEV_PRINT(...)
#endif  // AUDDEV_DRV_DEBUG

/**---------------------------------------------------------------------------*
 **                         Compiler Flag                                     *
 **---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

/**---------------------------------------------------------------------------*
 **                         Macro defination                                  *
 **---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
 **                         Local variables                                   *
 **---------------------------------------------------------------------------*/

LOCAL uint32_t _hpf_is_need_disable = 0;  //ken.kuang add.

/**---------------------------------------------------------------------------*
 **                         Global Variables                                  *
 **---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
 **                         Local Function Definitions                        *
 **---------------------------------------------------------------------------*/



/*****************************************************************************/
// Description :
// Author :         Jeff.Li
// Note :
/*****************************************************************************/
PUBLIC void VB_SwitchHpf (
    BOOLEAN is_enable // 1:open ;0: close
)
{
    AUDDEV_PRINT ("[VB_SwitchHpf]:is_enable:%d \n",is_enable);
	//ken.kuang add.
    _hpf_is_need_disable = 0;
    if (is_enable)
    {
        VB_PHY_HPFSwitch (is_enable);
    }
    else
    {
        // The HPF should not be closed while the state machine is ON, because
        // VB module including HPF is outputing, the closing operation might
        // cause stableless output.

//        if (!CODEC_PHY_IsRuning())
        {
            VB_PHY_HPFSwitch (is_enable);
        }
  //      else
        {
    		_hpf_is_need_disable = 1;
        }
    }
}

/*****************************************************************************/
// Description :
// Author :         Jeff.Li
// Note :
/*****************************************************************************/
PUBLIC void VB_SetHpfMode (
    BOOLEAN is_reset_mode
)
{
    AUDDEV_PRINT ("[VB_SetHpfMode]:is_reset_mode:%d \n",is_reset_mode);
    VB_PHY_SetHPFMode (is_reset_mode);
}

/*****************************************************************************/
// Description :
// Author :         Jeff.Li
// Note :  cherry add
// for 6800h,8800s4,6606l,6610  ,filter index:1,2
// for 8800G  ,                  filter index:1,2,3,4,5,6
/*****************************************************************************/
PUBLIC void VB_SetHpfParas (
    int16_t filter_index,
    int16_t B0,
    int16_t B1,
    int16_t B2,
    int16_t A0,
    int16_t minusA1,
    int16_t minusA2
)
{
    VB_PHY_SetHPFParas (
        filter_index,
        B0,
        B1,
        B2,
        A0,
        minusA1,
        minusA2);
}

/*****************************************************************************/
// Description :
// Author :         Jeff.Li
// Note :  cherry add
// for 6800h,8800s4,6606l,6610  ,gain index:0,1,2
// for 8800G  ,                  gain index:0,1,2,3,4,5,6
/*****************************************************************************/
PUBLIC void VB_SetHpfGain (
    int16_t gain_index,
    int16_t gain_value
)
{
    AUDDEV_PRINT ("[VB_SetHpfGain]:gain_index:%d,gain_value:%d \n",
                  gain_index, gain_value);
    VB_PHY_SetHPFGain (gain_index, gain_value);
}

/*****************************************************************************/
// Description :
// Author :         Jeff.Li
// Note :  cherry add
// for 6800h,8800s4,6606l,6610  ,gain index:0,1,2
// for 8800G  ,                  gain index:0,1,2,3,4,5,6
/*****************************************************************************/
PUBLIC uint32_t VB_GetHpfGain (
    int16_t gain_index
)
{
    uint32_t gain = 0;
    AUDDEV_PRINT ("[VB_GetHpfGain]:gain_index:%d \n",gain_index);
    gain = VB_PHY_GetHPFGain (gain_index);

    return gain;
}

/*****************************************************************************/
// Description :
// Author :         Jeff.Li
// Note :
/*****************************************************************************/
PUBLIC void VB_SetHpfLimit (
    int8_t rLimit //0~127
)
{
    AUDDEV_PRINT ("[VB_SetHpfLimit]:rLimit:%d \n",rLimit);
    VB_PHY_SetHPFLimit (rLimit);
}

/*****************************************************************************/
// Description :
// Author :         Jeff.Li
// Note :
/*****************************************************************************/
PUBLIC void VB_SetHpfWidth (
    uint16_t  width //width = 16 or 24 (bits)
)
{
    AUDDEV_PRINT ("[VB_SetHpfWidth]:width:%d \n",width);
    VB_PHY_SetHPFWidth (width);
}

/*****************************************************************************/
// Description :
// Author :         cherry.liu
// Note :
/*****************************************************************************/
PUBLIC void VB_ALCSwitch (
    BOOLEAN is_enable
)
{
    AUDDEV_PRINT ("[VB_ALCSwitch]:is_enable:%d \n",is_enable);
    VB_PHY_ALCSwitch (is_enable);
}

/*****************************************************************************/
// Description :
// Author :         cherry.liu
// Note :
/*****************************************************************************/
PUBLIC void VB_SetALCParas (
    VBC_ALC_PARAS_T *ptAlcPara
)
{
    AUDDEV_PRINT ("[VB_SetALCParas]:hold:%d,rise:%d,fall:%d,limit:%d,threshold:%d,ratio:%d \n",
                  ptAlcPara->hold,ptAlcPara->rise,ptAlcPara->fall,ptAlcPara->limit,ptAlcPara->threshold,ptAlcPara->ratio);
    AUDDEV_PRINT ("[VB_SetALCParas]:cg_var:%d,release_rate:%d,attack_rate:%d,release_rate_ex:%d,attack_rate_ex:%d \n",
                  ptAlcPara->cg_var,ptAlcPara->release_rate,ptAlcPara->attack_rate,ptAlcPara->release_rate_ex,ptAlcPara->attack_rate_ex);


    SCI_ASSERT (SCI_NULL != ptAlcPara);/*assert verified*/
    VB_PHY_SetALCParas (ptAlcPara);
}

/*****************************************************************************/
// Description :
// Author :         cherry.liu
// Note :
/*****************************************************************************/
PUBLIC void VB_SetFMMixMode (
    VBC_DA_CHANNEL_E da_index,
    VBC_DA_MIX_MODE_E mix_mode
)
{
    AUDDEV_PRINT ("[VB_SetFMMixMode]:da_index:%d,mix_mode:%d \n",da_index,mix_mode);
    VB_PHY_SetFMMixMode (da_index, mix_mode);
}

/*****************************************************************************/
// Description :
// Author :         cherry.liu
// Note :
/*****************************************************************************/
PUBLIC void VB_DGSwitch (
    VBC_DA_CHANNEL_E da_index,
    BOOLEAN is_enable
)
{
    AUDDEV_PRINT ("[VB_DGSwitch]:da_index:%d,is_enable:%d \n",da_index,is_enable);
    VB_PHY_DGSwitch (da_index, is_enable);
}

/*****************************************************************************/
// Description :
// Author :         cherry.liu
// Note :
/*****************************************************************************/
PUBLIC void VB_SetDG (
    VBC_DA_CHANNEL_E da_index,
    int16_t dg_value
)
{
    AUDDEV_PRINT ("[VB_SetDG]:da_index:%d,dg_value:%d \n",da_index,dg_value);
    VB_PHY_SetDG (da_index, dg_value);
}

/*****************************************************************************/
//  Description:    Sets the PGA of DAC path.
//  Author:         Jeff.Li
//  Note:
/*****************************************************************************/
PUBLIC void AUDDEV_SetPGA (uint32_t uiChannel, uint32_t uiGain)
{
#if 0
    uint32_t dac_pga = 0;
    uint32_t hp_pga  = 0;
    CODEC_DAC_OUTPUT_PGA_T pga;

    // GODL/R gain
    dac_pga = ( (uiGain & DOL_DAC_GOD_MASK) >> DOL_DAC_GOD_SHIFT);
    // GOL/R gain
    hp_pga = ( (uiGain & DOL_DAC_GO_MASK) >> DOL_DAC_GO_SHIFT);

    // printk ("AUDDEV_SetPGA 0x%x, 0x%x, vol_p1=0x%x, vol_p2=0x%x \n",
    //                uiChannel, uiGain, dac_pga, hp_pga);

    pga.dac_pga_l = dac_pga;
    pga.hp_pga_l = hp_pga;
    pga.dac_pga_r = dac_pga;
    pga.hp_pga_r = hp_pga;

    CODEC_PHY_SetDACPGA (uiChannel, pga);
#endif
}

/**---------------------------------------------------------------------------*
 **                         Compiler Flag                                     *
 **---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}

#endif
