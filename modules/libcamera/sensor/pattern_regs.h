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

#ifndef _REG_PATTERN_TIGER_H_
#define _REG_PATTERN_TIGER_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define SPRD_DCAM_BASE 0x60800000
#define REGS_MM_AHB_BASE 0x60d00000
#define REGS_MM_CLK_BASE 0x60e00000

#define DCAM_CFG (SPRD_DCAM_BASE + 0x0000UL)
#define SHIFT_PATTERN_SEL 8
#define PATTERN_SEL (1<<SHIFT_PATTERN_SEL)
#define DCAM_CONTROL (SPRD_DCAM_BASE + 0x0004UL)
#define SHIFT_PATTERN_START 7
#define SHIFT_PATTERN_CLEAR 6
#define PATTERN_START (1<<SHIFT_PATTERN_START)
#define PATTERN_CLEAR (1<<SHIFT_PATTERN_CLEAR)
#define PATTERN_BASE (SPRD_DCAM_BASE+0x88UL)
#define REG_PATTERN_CFG (PATTERN_BASE + 0)
#define SHIFT_SOLOR_B 24
#define SHIFT_SOLOR_G 16
#define SHIFT_SOLOR_R 8
#define SHIFT_BAYER_MODE 6
#define SHIFT_VSYNC_POL 5
#define SHIFT_HSYNC_POL 4
#define SHIFT_COLOR_MODE 3
#define SHIFT_SEQ_MODE 2
#define SHIFT_PATTERN_MODE 0
#define COLOR_MODE_YUV (1<<SHIFT_COLOR_MODE)
#define COLOR_MODE_RAW (0<<SHIFT_COLOR_MODE)
#define SEQ_MODE_INTER (0<<SHIFT_SEQ_MODE)
#define SEQ_MODE_PAD (1<<SHIFT_SEQ_MODE)
#define SOLOR (0<<SHIFT_PATTERN_MODE)
#define COLORBAR (1<<SHIFT_PATTERN_MODE)
#define COLORBAR2 (2<<SHIFT_PATTERN_MODE)
#define REG_PATTERN_SIZE (PATTERN_BASE + 4)
#define IMAGE_HEIGHT (0xfff<<16)
#define IMAGE_WIDTH (0xfff<<0)
#define REG_PATTERN_VBLANK (PATTERN_BASE + 8)
#define V_END_SET (0xff<<16)
#define V_START_SET (0xff<<8)
#define REG_PATTERN_HBLANK (PATTERN_BASE + 12)

#define SCI_ADDR(_b_, _o_) ((_b_) + (_o_) )
#define BIT(nr) (1UL << (nr))
#define REG_MM_AHB_AHB_EB SCI_ADDR(REGS_MM_AHB_BASE, 0x0000)
#define REG_MM_AHB_AHB_RST SCI_ADDR(REGS_MM_AHB_BASE, 0x0004)
#define REG_MM_AHB_GEN_CKG_CFG SCI_ADDR(REGS_MM_AHB_BASE, 0x0008)

/* bits definitions for register REG_MM_AHB_AHB_EB */
#define BIT_MM_CKG_EB ( BIT(6) )
#define BIT_JPG_EB ( BIT(5) )
#define BIT_CSI_EB ( BIT(4) )
#define BIT_VSP_EB ( BIT(3) )
#define BIT_ISP_EB ( BIT(2) )
#define BIT_CCIR_EB ( BIT(1) )
#define BIT_DCAM_EB ( BIT(0) )

/* bits definitions for register REG_MM_AHB_AHB_RST */
#define BIT_MM_CKG_SOFT_RST ( BIT(13) )
#define BIT_MM_MTX_SOFT_RST ( BIT(12) )
#define BIT_OR1200_SOFT_RST ( BIT(11) )
#define BIT_ROT_SOFT_RST ( BIT(10) )
#define BIT_CAM2_SOFT_RST ( BIT(9) )
#define BIT_CAM1_SOFT_RST ( BIT(8) )
#define BIT_CAM0_SOFT_RST ( BIT(7) )
#define BIT_JPG_SOFT_RST ( BIT(6) )
#define BIT_CSI_SOFT_RST ( BIT(5) )
#define BIT_VSP_SOFT_RST ( BIT(4) )
#define BIT_ISP_CFG_SOFT_RST ( BIT(3) )
#define BIT_ISP_LOG_SOFT_RST ( BIT(2) )
#define BIT_CCIR_SOFT_RST ( BIT(1) )
#define BIT_DCAM_SOFT_RST ( BIT(0) )

/* bits definitions for register REG_MM_AHB_GEN_CKG_CFG */
#define BIT_MM_MTX_AXI_CKG_EN ( BIT(8) )
#define BIT_MM_AXI_CKG_EN ( BIT(7) )
#define BIT_JPG_AXI_CKG_EN ( BIT(6) )
#define BIT_VSP_AXI_CKG_EN ( BIT(5) )
#define BIT_ISP_AXI_CKG_EN ( BIT(4) )
#define BIT_DCAM_AXI_CKG_EN ( BIT(3) )
#define BIT_SENSOR_CKG_EN ( BIT(2) )
#define BIT_MIPI_CSI_CKG_EN ( BIT(1) )
#define BIT_CPHY_CFG_CKG_EN ( BIT(0) )

#define REG_MM_CLK_MM_AHB_CFG SCI_ADDR(REGS_MM_CLK_BASE, 0x0020)
#define REG_MM_CLK_SENSOR_CFG SCI_ADDR(REGS_MM_CLK_BASE, 0x0024)
#define REG_MM_CLK_CCIR_CFG SCI_ADDR(REGS_MM_CLK_BASE, 0x0028)
#define REG_MM_CLK_DCAM_CFG SCI_ADDR(REGS_MM_CLK_BASE, 0x002c)
#define REG_MM_CLK_VSP_CFG SCI_ADDR(REGS_MM_CLK_BASE, 0x0030)
#define REG_MM_CLK_ISP_CFG SCI_ADDR(REGS_MM_CLK_BASE, 0x0034)
#define REG_MM_CLK_JPG_CFG SCI_ADDR(REGS_MM_CLK_BASE, 0x0038)

#ifdef __cplusplus
}
#endif

#endif
