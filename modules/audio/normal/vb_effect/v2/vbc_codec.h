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
#ifndef __VBC_CODEC_H
#define __VBC_CODEC_H

#define LOG_TAG  "vbc_effect"


#include <cutils/log.h>


#define printk   ALOGW

#define VBC_FIFO_FRAME_NUM	  160
#define VBC_NOSIE_CURRENT_SOUND_HARDWARE_BUG_FIX 1

#define VBC_RATE_8000   (10)
#define VBC_RATE_9600   ( 9)
#define VBC_RATE_11025  ( 8)
#define VBC_RATE_12000  ( 7)
#define VBC_RATE_16000  ( 6)
#define VBC_RATE_22050  ( 5)
#define VBC_RATE_24000  ( 4)
#define VBC_RATE_32000  ( 3)
#define VBC_RATE_44100  ( 2)
#define VBC_RATE_48000  ( 1)
#define VBC_RATE_96000  ( 0)

/* VBADBUFFDTA */
enum {
	VBISADCK_INV = 9,
	VBISDACK_INV,
	VBLSB_EB,
	VBIIS_DLOOP = 13,
	VBPCM_MODE,
	VBIIS_LRCK,
};
/* VBDABUFFDTA */
enum {
	RAMSW_NUMB = 9,
	RAMSW_EN,
	VBAD0DMA_EN,
	VBAD1DMA_EN,
	VBDA0DMA_EN,
	VBDA1DMA_EN,
	VBENABLE,
};
/* VBAICR */
#define VBCAICR_MODE_ADC_I2S	(1 << 0)
#define VBCAICR_MODE_DAC_I2S	(1 << 1)
#define VBCAICR_MODE_ADC_SERIAL	(1 << 2)
#define VBCAICR_MODE_DAC_SERIAL	(1 << 3)
/* VBCR1 */
enum {
	BTL_MUTE = 1,
	BYPASS,
	DACSEL,
	HP_DIS,
	DAC_MUTE,
	MONO,
	SB_MICBIAS,
};
/* VBCR2 */
#define DAC_DATA_WIDTH_16_bit	(0x00)
#define DAC_DATA_WIDTH_18_bit	(0x01)
#define DAC_DATA_WIDTH_20_bit	(0x02)
#define DAC_DATA_WIDTH_24_bit	(0x03)

#define ADC_DATA_WIDTH_16_bit	(0x00)
#define ADC_DATA_WIDTH_18_bit	(0x01)
#define ADC_DATA_WIDTH_20_bit	(0x02)
#define ADC_DATA_WIDTH_24_bit	(0x03)

#define MICROPHONE1				(0)
#define MICROPHONE2				(1)
enum {
	MICSEL = 1,
	ADC_HPF,
	ADC_ADWL,
	DAC_ADWL = 5,
	DAC_DEEMP = 7,
};
/* VBPMR1 */
enum {
	SB_LOUT = 1,
	SB_BTL,
	SB_LIN,
	SB_ADC,
	SB_MIX,
	SB_OUT,
	SB_DAC,
};
/* VBPMR2 */
enum {
	SB_SLEEP = 0,
	SB,
	SB_MC,
	GIM,
	RLGOD,
	LRGOD,
};
/* -------------------------- */
#define SPRD_VBC_ALSA_CTRL2ARM_REG (SPRD_GREG_BASE + GR_BUSCLK)

#define SPRD_VB_BASE 0x0000000   //FIXME

#define ARM_VB_BASE		SPRD_VB_BASE

#define VIRT_VBDA0		(SPRD_VB_BASE + 0x0000) /* 0x0000  Voice band DAC0 data buffer */
#define VIRT_VBDA1		(SPRD_VB_BASE + 0x0004) /* 0x0002  Voice band DAC1 data buffer */
#define VIRT_VBAD0		(SPRD_VB_BASE + 0x0008) /* 0x0004  Voice band ADC0 data buffer */
#define VIRT_VBAD1		(SPRD_VB_BASE + 0x000C) /* 0x0006  Voice band ADC1 data buffer */
#define PHYS_VBDA0		(SPRD_VB_PHYS + 0x0000) /* 0x0000  Voice band DAC0 data buffer */
#define PHYS_VBDA1		(SPRD_VB_PHYS + 0x0004) /* 0x0002  Voice band DAC1 data buffer */
#define PHYS_VBAD0		(SPRD_VB_PHYS + 0x0008) /* 0x0004  Voice band ADC0 data buffer */
#define PHYS_VBAD1		(SPRD_VB_PHYS + 0x000C) /* 0x0006  Voice band ADC1 data buffer */

#define DMA_VB_DA0_BIT		(1 << DMA_VB_DA0)
#define DMA_VB_DA1_BIT		(1 << DMA_VB_DA1)
#define DMA_VB_AD0_BIT		(1 << DMA_VB_AD0)
#define DMA_VB_AD1_BIT		(1 << DMA_VB_AD1)

#define VBDA0			(ARM_VB_BASE + 0x0000) /* 0x0000  Voice band DAC0 data buffer */
#define VBDA1			(ARM_VB_BASE + 0x0004) /* 0x0002  Voice band DAC1 data buffer */
#define VBAD0			(ARM_VB_BASE + 0x0008) /* 0x0004  Voice band ADC0 data buffer */
#define VBAD1			(ARM_VB_BASE + 0x000C) /* 0x0006  Voice band ADC1 data buffer */
#define VBBUFFSIZE		(ARM_VB_BASE + 0x0010) /* 0x0008  Voice band buffer size */
#define VBADBUFFDTA		(ARM_VB_BASE + 0x0014) /* 0x000A  Voice band AD buffer control */
#define VBDABUFFDTA		(ARM_VB_BASE + 0x0018) /* 0x000C  Voice band DA buffer control */
#define VBADCNT			(ARM_VB_BASE + 0x001C) /* 0x000E  Voice band AD buffer counter */
#define VBDACNT			(ARM_VB_BASE + 0x0020) /* 0x0010  Voice band DA buffer counter */
#define VBDAICTL		(ARM_VB_BASE + 0x0024) /* 0x0012  Voice band DAI control */
#define VBDAIIN			(ARM_VB_BASE + 0x0028) /* 0x0014  Voice band DAI input */
#define VBDAIOUT		(ARM_VB_BASE + 0x002C) /* 0x0016  Voice band DAI output */

void* VB_MAP_BASE ;
void* VB_AD_CTL_MAP_BASE;




void* DAPATCHCTL;
void* DADGCTL   ;
void* DAHPCTL   ;
void* DAALCCTL0 ;
void* DAALCCTL1 ;
void* DAALCCTL2 ;
void* DAALCCTL3 ;
void* DAALCCTL4 ;
void* DAALCCTL5 ;
void* DAALCCTL6 ;
void* DAALCCTL7 ;
void* DAALCCTL8 ;
void* DAALCCTL9 ;
void* DAALCCTL10;
void* STCTL0	   ;
void* STCTL1	   ;
void* DACSRCCTL ;
void* MIXERCTL ;
void* VBNGCVTHD ;
void* VBNGCTTHD ;
void* VBNGCTL ;
void* ADPATCHCTL;
void* ADDG01CTL   ;
void* ADDG23CTL   ; 
void* ADHPCTL   ; 
void* HPCOEF0_H   ;            
void* HPCOEF0_L  ;      
void* HPCOEF1_H   ;            
void* HPCOEF1_L  ;      
void* HPCOEF2_H   ;            
void* HPCOEF2_L  ;      
void* HPCOEF3_H   ;            
void* HPCOEF3_L  ;      
void* HPCOEF4_H   ;            
void* HPCOEF4_L  ;      
void* HPCOEF5_H   ;            
void* HPCOEF5_L  ;      
void* HPCOEF6_H   ;            
void* HPCOEF6_L  ;      
void* HPCOEF7_H   ;            
void* HPCOEF7_L  ;      
void* HPCOEF8_H   ;            
void* HPCOEF8_L  ;      
void* HPCOEF9_H   ;            
void* HPCOEF9_L  ;      
void* HPCOEF10_H ;             
void* HPCOEF10_L ;       
void* HPCOEF11_H ;             
void* HPCOEF11_L ;       
void* HPCOEF12_H ;             
void* HPCOEF12_L ;       
void* HPCOEF13_H ;             
void* HPCOEF13_L ;       
void* HPCOEF14_H ;             
void* HPCOEF14_L ;       
void* HPCOEF15_H ;             
void* HPCOEF15_L ;       
void* HPCOEF16_H ;             
void* HPCOEF16_L ;       
void* HPCOEF17_H ;             
void* HPCOEF17_L ;       
void* HPCOEF18_H ;             
void* HPCOEF18_L ;       
void* HPCOEF19_H ;             
void* HPCOEF19_L ;       
void* HPCOEF20_H ;             
void* HPCOEF20_L ;       
void* HPCOEF21_H ;             
void* HPCOEF21_L ;       
void* HPCOEF22_H ;             
void* HPCOEF22_L ;       
void* HPCOEF23_H ;             
void* HPCOEF23_L ;       
void* HPCOEF24_H ;             
void* HPCOEF24_L ;       
void* HPCOEF25_H ;             
void* HPCOEF25_L ;       
void* HPCOEF26_H ;             
void* HPCOEF26_L ;       
void* HPCOEF27_H ;             
void* HPCOEF27_L ;       
void* HPCOEF28_H ;             
void* HPCOEF28_L ;       
void* HPCOEF29_H ;             
void* HPCOEF29_L ;       
void* HPCOEF30_H ;             
void* HPCOEF30_L ;       
void* HPCOEF31_H ;             
void* HPCOEF31_L ;       
void* HPCOEF32_H ;             
void* HPCOEF32_L ;       
void* HPCOEF33_H ;             
void* HPCOEF33_L ;       
void* HPCOEF34_H ;             
void* HPCOEF34_L ;       
void* HPCOEF35_H ;             
void* HPCOEF35_L ;       
void* HPCOEF36_H ;             
void* HPCOEF36_L ;       
void* HPCOEF37_H ;             
void* HPCOEF37_L ;       
void* HPCOEF38_H ;             
void* HPCOEF38_L ;       
void* HPCOEF39_H ;             
void* HPCOEF39_L ;       
void* HPCOEF40_H ;             
void* HPCOEF40_L ;       
void* HPCOEF41_H ;            
void* HPCOEF41_L ;       
void* HPCOEF42_H ;             
void* HPCOEF42_L ;       
void* HPCOEF43_H ;             
void* HPCOEF43_L ;       
void* HPCOEF44_H ;             
void* HPCOEF44_L ;       
void* HPCOEF45_H ;             
void* HPCOEF45_L ;       
void* HPCOEF46_H ;             
void* HPCOEF46_L ;       
void* HPCOEF47_H ;             
void* HPCOEF47_L ;       
void* HPCOEF48_H ;             
void* HPCOEF48_L ;       
void* HPCOEF49_H ;             
void* HPCOEF49_L ;       
void* HPCOEF50_H ;             
void* HPCOEF50_L ;       
void* HPCOEF51_H ;             
void* HPCOEF51_L ;       
void* HPCOEF52_H ;             
void* HPCOEF52_L ;       
void* HPCOEF53_H ;             
void* HPCOEF53_L ;       
void* HPCOEF54_H ;             
void* HPCOEF54_L ;       
void* HPCOEF55_H ;             
void* HPCOEF55_L ;       
void* HPCOEF56_H ;             
void* HPCOEF56_L ;       
void* HPCOEF57_H ;             
void* HPCOEF57_L ;       
void* HPCOEF58_H ;             
void* HPCOEF58_L ;       
void* HPCOEF59_H ;             
void* HPCOEF59_L ;       
void* HPCOEF60_H ;             
void* HPCOEF60_L ;       
void* HPCOEF61_H ;             
void* HPCOEF61_L ;       
void* HPCOEF62_H ;             
void* HPCOEF62_L ;       
void* HPCOEF63_H ;             
void* HPCOEF63_L ;       
void* HPCOEF64_H ;             
void* HPCOEF64_L ;       
void* HPCOEF65_H ;             
void* HPCOEF65_L ;       
void* HPCOEF66_H ;             
void* HPCOEF66_L ;       
void* HPCOEF67_H ;             
void* HPCOEF67_L ;       
void* HPCOEF68_H ;             
void* HPCOEF68_L ;       
void* HPCOEF69_H ;             
void* HPCOEF69_L ;       
void* HPCOEF70_H ;             
void* HPCOEF70_L ;       
void* HPCOEF71_H ;             
void* HPCOEF71_L ;       
void* ADC01_HPCOEF0_H   ;      
void* ADC01_HPCOEF0_L   ;
void* ADC01_HPCOEF1_H   ;      
void* ADC01_HPCOEF1_L   ;
void* ADC01_HPCOEF2_H   ;      
void* ADC01_HPCOEF2_L   ;
void* ADC01_HPCOEF3_H   ;      
void* ADC01_HPCOEF3_L   ;
void* ADC01_HPCOEF4_H   ;      
void* ADC01_HPCOEF4_L   ;
void* ADC01_HPCOEF5_H   ;      
void* ADC01_HPCOEF5_L   ;
void* ADC01_HPCOEF6_H   ;      
void* ADC01_HPCOEF6_L   ;
void* ADC01_HPCOEF7_H   ;      
void* ADC01_HPCOEF7_L   ;
void* ADC01_HPCOEF8_H   ;      
void* ADC01_HPCOEF8_L   ;
void* ADC01_HPCOEF9_H   ;      
void* ADC01_HPCOEF9_L   ;
void* ADC01_HPCOEF10_H ;       
void* ADC01_HPCOEF10_L ; 
void* ADC01_HPCOEF11_H ;       
void* ADC01_HPCOEF11_L ; 
void* ADC01_HPCOEF12_H ;       
void* ADC01_HPCOEF12_L ; 
void* ADC01_HPCOEF13_H ;       
void* ADC01_HPCOEF13_L ; 
void* ADC01_HPCOEF14_H ;       
void* ADC01_HPCOEF14_L ; 
void* ADC01_HPCOEF15_H ;       
void* ADC01_HPCOEF15_L ; 
void* ADC01_HPCOEF16_H ;       
void* ADC01_HPCOEF16_L ; 
void* ADC01_HPCOEF17_H ;       
void* ADC01_HPCOEF17_L ; 
void* ADC01_HPCOEF18_H ;       
void* ADC01_HPCOEF18_L ; 
void* ADC01_HPCOEF19_H ;       
void* ADC01_HPCOEF19_L ; 
void* ADC01_HPCOEF20_H ;       
void* ADC01_HPCOEF20_L ; 
void* ADC01_HPCOEF21_H ;       
void* ADC01_HPCOEF21_L ; 
void* ADC01_HPCOEF22_H ;       
void* ADC01_HPCOEF22_L ; 
void* ADC01_HPCOEF23_H ;       
void* ADC01_HPCOEF23_L ; 
void* ADC01_HPCOEF24_H ;       
void* ADC01_HPCOEF24_L ; 
void* ADC01_HPCOEF25_H ;       
void* ADC01_HPCOEF25_L ; 
void* ADC01_HPCOEF26_H ;       
void* ADC01_HPCOEF26_L ; 
void* ADC01_HPCOEF27_H ;       
void* ADC01_HPCOEF27_L ; 
void* ADC01_HPCOEF28_H ;       
void* ADC01_HPCOEF28_L ; 
void* ADC01_HPCOEF29_H ;       
void* ADC01_HPCOEF29_L ; 
void* ADC01_HPCOEF30_H ;       
void* ADC01_HPCOEF30_L ; 
void* ADC01_HPCOEF31_H ;       
void* ADC01_HPCOEF31_L ; 
void* ADC01_HPCOEF32_H ;       
void* ADC01_HPCOEF32_L ; 
void* ADC01_HPCOEF33_H ;       
void* ADC01_HPCOEF33_L ; 
void* ADC01_HPCOEF34_H ;       
void* ADC01_HPCOEF34_L ; 
void* ADC01_HPCOEF35_H ;       
void* ADC01_HPCOEF35_L ; 
void* ADC01_HPCOEF36_H ;       
void* ADC01_HPCOEF36_L ; 
void* ADC01_HPCOEF37_H ;       
void* ADC01_HPCOEF37_L ; 
void* ADC01_HPCOEF38_H ;       
void* ADC01_HPCOEF38_L ; 
void* ADC01_HPCOEF39_H ;       
void* ADC01_HPCOEF39_L ; 
void* ADC01_HPCOEF40_H ;       
void* ADC01_HPCOEF40_L ; 
void* ADC01_HPCOEF41_H ;       
void* ADC01_HPCOEF41_L ; 
void* ADC01_HPCOEF42_H ;       
void* ADC01_HPCOEF42_L ; 
void* ADC23_HPCOEF0_H   ;      
void* ADC23_HPCOEF0_L   ;
void* ADC23_HPCOEF1_H   ;      
void* ADC23_HPCOEF1_L   ;
void* ADC23_HPCOEF2_H   ;      
void* ADC23_HPCOEF2_L   ;
void* ADC23_HPCOEF3_H   ;      
void* ADC23_HPCOEF3_L   ;
void* ADC23_HPCOEF4_H   ;      
void* ADC23_HPCOEF4_L   ;
void* ADC23_HPCOEF5_H   ;      
void* ADC23_HPCOEF5_L   ;
void* ADC23_HPCOEF6_H   ;      
void* ADC23_HPCOEF6_L   ;
void* ADC23_HPCOEF7_H   ;      
void* ADC23_HPCOEF7_L   ;
void* ADC23_HPCOEF8_H   ;      
void* ADC23_HPCOEF8_L   ;
void* ADC23_HPCOEF9_H   ;      
void* ADC23_HPCOEF9_L   ;
void* ADC23_HPCOEF10_H ;       
void* ADC23_HPCOEF10_L ; 
void* ADC23_HPCOEF11_H ;       
void* ADC23_HPCOEF11_L ; 
void* ADC23_HPCOEF12_H ;       
void* ADC23_HPCOEF12_L ; 
void* ADC23_HPCOEF13_H ;       
void* ADC23_HPCOEF13_L ; 
void* ADC23_HPCOEF14_H ;       
void* ADC23_HPCOEF14_L ; 
void* ADC23_HPCOEF15_H ;       
void* ADC23_HPCOEF15_L ; 
void* ADC23_HPCOEF16_H ;       
void* ADC23_HPCOEF16_L ; 
void* ADC23_HPCOEF17_H ;       
void* ADC23_HPCOEF17_L ; 
void* ADC23_HPCOEF18_H ;       
void* ADC23_HPCOEF18_L ; 
void* ADC23_HPCOEF19_H ;       
void* ADC23_HPCOEF19_L ; 
void* ADC23_HPCOEF20_H ;       
void* ADC23_HPCOEF20_L ; 
void* ADC23_HPCOEF21_H ;       
void* ADC23_HPCOEF21_L ; 
void* ADC23_HPCOEF22_H ;       
void* ADC23_HPCOEF22_L ; 
void* ADC23_HPCOEF23_H ;       
void* ADC23_HPCOEF23_L ; 
void* ADC23_HPCOEF24_H ;       
void* ADC23_HPCOEF24_L ; 
void* ADC23_HPCOEF25_H ;       
void* ADC23_HPCOEF25_L ; 
void* ADC23_HPCOEF26_H ;       
void* ADC23_HPCOEF26_L ; 
void* ADC23_HPCOEF27_H ;       
void* ADC23_HPCOEF27_L ; 
void* ADC23_HPCOEF28_H ;       
void* ADC23_HPCOEF28_L ; 
void* ADC23_HPCOEF29_H ;       
void* ADC23_HPCOEF29_L ; 
void* ADC23_HPCOEF30_H ;       
void* ADC23_HPCOEF30_L ; 
void* ADC23_HPCOEF31_H ;       
void* ADC23_HPCOEF31_L ; 
void* ADC23_HPCOEF32_H ;       
void* ADC23_HPCOEF32_L ; 
void* ADC23_HPCOEF33_H ;       
void* ADC23_HPCOEF33_L ; 
void* ADC23_HPCOEF34_H ;       
void* ADC23_HPCOEF34_L ; 
void* ADC23_HPCOEF35_H ;       
void* ADC23_HPCOEF35_L ; 
void* ADC23_HPCOEF36_H ;       
void* ADC23_HPCOEF36_L ; 
void* ADC23_HPCOEF37_H ;       
void* ADC23_HPCOEF37_L ; 
void* ADC23_HPCOEF38_H ;       
void* ADC23_HPCOEF38_L ; 
void* ADC23_HPCOEF39_H ;       
void* ADC23_HPCOEF39_L ; 
void* ADC23_HPCOEF40_H ;       
void* ADC23_HPCOEF40_L ; 
void* ADC23_HPCOEF41_H ;       
void* ADC23_HPCOEF41_L ; 
void* ADC23_HPCOEF42_H ;       
void* ADC23_HPCOEF42_L ; 
/* sc8810 ldo register */
#define	LDO_REG_BASE		(SPRD_MISC_BASE + 0x600)
#define	ANA_LDO_PD_CTL0		(LDO_REG_BASE  + 0x10)
#define ANA_AUDIO_CTRL		(LDO_REG_BASE  + 0x74)
#define	ANA_AUDIO_PA_CTRL0	(LDO_REG_BASE  + 0x78)
#define	ANA_AUDIO_PA_CTRL1	(LDO_REG_BASE  + 0x7C)

#ifndef SPRD_ADI_BASE
#define SPRD_ADI_BASE           SPRD_MISC_BASE
#endif
#ifndef SPRD_ADI_PHYS
#define SPRD_ADI_PHYS           SPRD_MISC_PHYS
#endif

#define ANA_REG_ADDR_START      (SPRD_MISC_BASE + 0x40)  /* 0x82000040 */
#define ANA_REG_ADDR_END        (SPRD_MISC_BASE + 0x780) /* 0x82000780 */

#define ARM_VB_BASE2		SPRD_MISC_BASE

#define VBAICR			(ARM_VB_BASE2 + 0x0100) /* 0x0080 Voice band Codec AICR */
#define VBCR1			(ARM_VB_BASE2 + 0x0104) /* 0x0082 Voice band Codec CR1 */
#define VBCR2			(ARM_VB_BASE2 + 0x0108) /* 0x0084 Voice band Codec CR2 */
#define VBCCR1			(ARM_VB_BASE2 + 0x010C) /* 0x0086 Voice band Codec CCR1 */
#define VBCCR2			(ARM_VB_BASE2 + 0x0110) /* 0x0088 Voice band Codec CCR2 */
#define VBPMR1			(ARM_VB_BASE2 + 0x0114) /* 0x008A Voice band Codec PMR1 */
#define VBPMR2			(ARM_VB_BASE2 + 0x0118) /* 0x008C Voice band Codec PMR2 */
#define VBCRR			(ARM_VB_BASE2 + 0x011C) /* 0x008E Voice band Codec CRR */
#define VBICR			(ARM_VB_BASE2 + 0x0120) /* 0x0090 Voice band Codec ICR */
#define VBIFR			(ARM_VB_BASE2 + 0x0124) /* 0x0092 Voice band Codec IFR */
#define VBCGR1			(ARM_VB_BASE2 + 0x0128) /* 0x0094 Voice band Codec CGR1 */
#define VBCGR2			(ARM_VB_BASE2 + 0x012C) /* 0x0096 Voice band Codec CGR2 */
#define VBCGR3			(ARM_VB_BASE2 + 0x0130) /* 0x0098 Voice band Codec CGR3 */
#define VBCGR8			(ARM_VB_BASE2 + 0x0144) /* 0x00A2 Voice band Codec CGR8 */
#define VBCGR9			(ARM_VB_BASE2 + 0x0148) /* 0x00A4 Voice band Codec CGR9 */
#define VBCGR10			(ARM_VB_BASE2 + 0x014C) /* 0x00A6 Voice band Codec CGR10 */
#define VBTR1			(ARM_VB_BASE2 + 0x0150) /* 0x00A8 Voice band Codec TR1 */
#define VBTR2			(ARM_VB_BASE2 + 0x0154) /* 0x00AA Voice band Codec TR2 */


/* -------------------------- */
extern uint32_t vbc_reg_write(uint32_t reg, uint8_t shift, uint32_t val, uint32_t mask);
extern uint32_t vbc_reg_read(uint32_t reg, uint8_t shift, uint32_t mask);

#endif
