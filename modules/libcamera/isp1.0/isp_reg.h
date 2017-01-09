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
#ifndef _ISP_REG_H_
#define _ISP_REG_H_

#ifdef   __cplusplus
extern   "C"
{
#endif
/**---------------------------------------------------------------------------*
**                               Micro Define                                **
**----------------------------------------------------------------------------*/
#define ISP_BASE_ADDR             0x00000000
#define ISP_BASE_ADDR_V0000       0x22000000
#define ISP_BASE_ADDR_V0001       0x60a00000
#define ISP_BASE_ADDR_V0002       0x60a00000

// Fetch
#define ISP_FETCH_STATUS               (ISP_BASE_ADDR+0x0000)
#define ISP_FETCH_STATUS_PREVIEW_V0001 (ISP_BASE_ADDR+0x0004)
#define ISP_FETCH_PARAM                (ISP_BASE_ADDR+0x0014)
#define ISP_FETCH_SLICE_SIZE           (ISP_BASE_ADDR+0x0018)
#define ISP_FETCH_SLICE_Y_ADDR         (ISP_BASE_ADDR+0x001c)
#define ISP_FETCH_Y_PITCH              (ISP_BASE_ADDR+0x0020)
#define ISP_FETCH_SLICE_U_ADDR         (ISP_BASE_ADDR+0x0024)
#define ISP_FETCH_U_PITCH              (ISP_BASE_ADDR+0x0028)
#define ISP_FETCH_MIPI_WORD_INFO       (ISP_BASE_ADDR+0x002c)
#define ISP_FETCH_MIPI_BYTE_INFO       (ISP_BASE_ADDR+0x0030)
#define ISP_FETCH_SLICE_V_ADDR         (ISP_BASE_ADDR+0x0034)
#define ISP_FETCH_V_PITCH              (ISP_BASE_ADDR+0x0038)
#define ISP_FETCH_PREVIEW_CNT          (ISP_BASE_ADDR+0x003c)
#define ISP_FETCH_BUF_ADDR_V0002       (ISP_BASE_ADDR+0x0040)

//BLC and NLC
#define ISP_BNLC_STATUS           (ISP_BASE_ADDR+0x0100)
#define ISP_BNLC_PARAM            (ISP_BASE_ADDR+0x0114)
#define ISP_BNLC_B_PARAM_R_B      (ISP_BASE_ADDR+0x0118)
#define ISP_BNLC_B_PARAM_G        (ISP_BASE_ADDR+0x011c)
#define ISP_BNLC_N_PARAM_R0       (ISP_BASE_ADDR+0x0120)
#define ISP_BNLC_N_PARAM_R1       (ISP_BASE_ADDR+0x0124)
#define ISP_BNLC_N_PARAM_R2       (ISP_BASE_ADDR+0x0128)
#define ISP_BNLC_N_PARAM_R3       (ISP_BASE_ADDR+0x012c)
#define ISP_BNLC_N_PARAM_R4       (ISP_BASE_ADDR+0x0130)
#define ISP_BNLC_N_PARAM_R5       (ISP_BASE_ADDR+0x0134)
#define ISP_BNLC_N_PARAM_R6       (ISP_BASE_ADDR+0x0138)
#define ISP_BNLC_N_PARAM_R7       (ISP_BASE_ADDR+0x013c)
#define ISP_BNLC_N_PARAM_R8       (ISP_BASE_ADDR+0x0140)
#define ISP_BNLC_N_PARAM_R9       (ISP_BASE_ADDR+0x0144)
#define ISP_BNLC_N_PARAM_G0       (ISP_BASE_ADDR+0x0148)
#define ISP_BNLC_N_PARAM_G1       (ISP_BASE_ADDR+0x014c)
#define ISP_BNLC_N_PARAM_G2       (ISP_BASE_ADDR+0x0150)
#define ISP_BNLC_N_PARAM_G3       (ISP_BASE_ADDR+0x0154)
#define ISP_BNLC_N_PARAM_G4       (ISP_BASE_ADDR+0x0158)
#define ISP_BNLC_N_PARAM_G5       (ISP_BASE_ADDR+0x015c)
#define ISP_BNLC_N_PARAM_G6       (ISP_BASE_ADDR+0x0160)
#define ISP_BNLC_N_PARAM_G7       (ISP_BASE_ADDR+0x0164)
#define ISP_BNLC_N_PARAM_G8       (ISP_BASE_ADDR+0x0168)
#define ISP_BNLC_N_PARAM_G9       (ISP_BASE_ADDR+0x016c)
#define ISP_BNLC_N_PARAM_B0       (ISP_BASE_ADDR+0x0170)
#define ISP_BNLC_N_PARAM_B1       (ISP_BASE_ADDR+0x0174)
#define ISP_BNLC_N_PARAM_B2       (ISP_BASE_ADDR+0x0178)
#define ISP_BNLC_N_PARAM_B3       (ISP_BASE_ADDR+0x017c)
#define ISP_BNLC_N_PARAM_B4       (ISP_BASE_ADDR+0x0180)
#define ISP_BNLC_N_PARAM_B5       (ISP_BASE_ADDR+0x0184)
#define ISP_BNLC_N_PARAM_B6       (ISP_BASE_ADDR+0x0188)
#define ISP_BNLC_N_PARAM_B7       (ISP_BASE_ADDR+0x018c)
#define ISP_BNLC_N_PARAM_B8       (ISP_BASE_ADDR+0x0190)
#define ISP_BNLC_N_PARAM_B9       (ISP_BASE_ADDR+0x0194)
#define ISP_BNLC_N_PARAM_L0       (ISP_BASE_ADDR+0x0198)
#define ISP_BNLC_N_PARAM_L1       (ISP_BASE_ADDR+0x019c)
#define ISP_BNLC_N_PARAM_L2       (ISP_BASE_ADDR+0x01a0)
#define ISP_BNLC_N_PARAM_L3       (ISP_BASE_ADDR+0x01a4)
#define ISP_BNLC_N_PARAM_L4       (ISP_BASE_ADDR+0x01a8)
#define ISP_BNLC_N_PARAM_L5       (ISP_BASE_ADDR+0x01ac)
#define ISP_BNLC_N_PARAM_L6       (ISP_BASE_ADDR+0x01b0)
#define ISP_BNLC_N_PARAM_L7       (ISP_BASE_ADDR+0x01b4)
#define ISP_BNLC_N_PARAM_L8       (ISP_BASE_ADDR+0x01b8)
#define ISP_BNLC_SLICE_SIZE       (ISP_BASE_ADDR+0x01bc)
#define ISP_BNLC_SLICE_INFO       (ISP_BASE_ADDR+0x01c0)
#define ISP_BLC_SLICE_SIZE_V0001  (ISP_BASE_ADDR+0x0120)
#define ISP_BLC_SLICE_INFO_V0001  (ISP_BASE_ADDR+0x0124)

// NLC
#define ISP_NLC_STATUS_V0001           (ISP_BASE_ADDR+0x1b00)
#define ISP_NLC_PARAM_V0001            (ISP_BASE_ADDR+0x1b14)
#define ISP_NLC_N_PARAM_R0_V0001       (ISP_BASE_ADDR+0x1b18)
#define ISP_NLC_N_PARAM_R1_V0001       (ISP_BASE_ADDR+0x1b1c)
#define ISP_NLC_N_PARAM_R2_V0001       (ISP_BASE_ADDR+0x1b20)
#define ISP_NLC_N_PARAM_R3_V0001       (ISP_BASE_ADDR+0x1b24)
#define ISP_NLC_N_PARAM_R4_V0001       (ISP_BASE_ADDR+0x1b28)
#define ISP_NLC_N_PARAM_R5_V0001       (ISP_BASE_ADDR+0x1b2c)
#define ISP_NLC_N_PARAM_R6_V0001       (ISP_BASE_ADDR+0x1b30)
#define ISP_NLC_N_PARAM_R7_V0001       (ISP_BASE_ADDR+0x1b34)
#define ISP_NLC_N_PARAM_R8_V0001       (ISP_BASE_ADDR+0x1b38)
#define ISP_NLC_N_PARAM_R9_V0001       (ISP_BASE_ADDR+0x1b3c)
#define ISP_NLC_N_PARAM_G0_V0001       (ISP_BASE_ADDR+0x1b40)
#define ISP_NLC_N_PARAM_G1_V0001       (ISP_BASE_ADDR+0x1b44)
#define ISP_NLC_N_PARAM_G2_V0001       (ISP_BASE_ADDR+0x1b48)
#define ISP_NLC_N_PARAM_G3_V0001       (ISP_BASE_ADDR+0x1b4c)
#define ISP_NLC_N_PARAM_G4_V0001       (ISP_BASE_ADDR+0x1b50)
#define ISP_NLC_N_PARAM_G5_V0001       (ISP_BASE_ADDR+0x1b54)
#define ISP_NLC_N_PARAM_G6_V0001       (ISP_BASE_ADDR+0x1b58)
#define ISP_NLC_N_PARAM_G7_V0001       (ISP_BASE_ADDR+0x1b5c)
#define ISP_NLC_N_PARAM_G8_V0001       (ISP_BASE_ADDR+0x1b60)
#define ISP_NLC_N_PARAM_G9_V0001       (ISP_BASE_ADDR+0x1b64)
#define ISP_NLC_N_PARAM_B0_V0001       (ISP_BASE_ADDR+0x1b68)
#define ISP_NLC_N_PARAM_B1_V0001       (ISP_BASE_ADDR+0x1b6c)
#define ISP_NLC_N_PARAM_B2_V0001       (ISP_BASE_ADDR+0x1b70)
#define ISP_NLC_N_PARAM_B3_V0001       (ISP_BASE_ADDR+0x1b74)
#define ISP_NLC_N_PARAM_B4_V0001       (ISP_BASE_ADDR+0x1b78)
#define ISP_NLC_N_PARAM_B5_V0001       (ISP_BASE_ADDR+0x1b7c)
#define ISP_NLC_N_PARAM_B6_V0001       (ISP_BASE_ADDR+0x1b80)
#define ISP_NLC_N_PARAM_B7_V0001       (ISP_BASE_ADDR+0x1b84)
#define ISP_NLC_N_PARAM_B8_V0001       (ISP_BASE_ADDR+0x1b88)
#define ISP_NLC_N_PARAM_B9_V0001       (ISP_BASE_ADDR+0x1b8c)
#define ISP_NLC_N_PARAM_L0_V0001       (ISP_BASE_ADDR+0x1b90)
#define ISP_NLC_N_PARAM_L1_V0001       (ISP_BASE_ADDR+0x1b94)
#define ISP_NLC_N_PARAM_L2_V0001       (ISP_BASE_ADDR+0x1b98)
#define ISP_NLC_N_PARAM_L3_V0001       (ISP_BASE_ADDR+0x1b9c)
#define ISP_NLC_N_PARAM_L4_V0001       (ISP_BASE_ADDR+0x1ba0)
#define ISP_NLC_N_PARAM_L5_V0001       (ISP_BASE_ADDR+0x1ba4)
#define ISP_NLC_N_PARAM_L6_V0001       (ISP_BASE_ADDR+0x1ba8)
#define ISP_NLC_N_PARAM_L7_V0001       (ISP_BASE_ADDR+0x1bac)
#define ISP_NLC_N_PARAM_L8_V0001       (ISP_BASE_ADDR+0x1bb0)

//Lens
#define ISP_LENS_STATUS           (ISP_BASE_ADDR+0x0200)
#define ISP_LENS_PARAM            (ISP_BASE_ADDR+0x0214)
#define ISP_LENS_PARAM_ADDR       (ISP_BASE_ADDR+0x0218)
#define ISP_LENS_SLICE_POS        (ISP_BASE_ADDR+0x021c)
#define ISP_LENS_LOADER_ENABLE    (ISP_BASE_ADDR+0x0220)
#define ISP_LENS_GRID_PITCH       (ISP_BASE_ADDR+0x0224)
#define ISP_LENS_GRID_SIZE        (ISP_BASE_ADDR+0x0228)
#define ISP_LENS_LOAD_BUF         (ISP_BASE_ADDR+0x022c)
#define ISP_LENS_MISC             (ISP_BASE_ADDR+0x0230)
#define ISP_LENS_SLICE_SIZE       (ISP_BASE_ADDR+0x0234)
#define ISP_LENS_SLICE_INFO       (ISP_BASE_ADDR+0x0238)
#define ISP_LENS_GRP_DONE_SEL     (ISP_BASE_ADDR+0x0238) //SharkL

//AWB monitor
#define ISP_AWBM_STATUS           (ISP_BASE_ADDR+0x0300)
#define ISP_AWBM_PARAM            (ISP_BASE_ADDR+0x0314)
#define ISP_AWBM_OFFSET           (ISP_BASE_ADDR+0x0318)
#define ISP_AWBM_BLK_SIZE         (ISP_BASE_ADDR+0x031c)

//AWB correction
#define ISP_AWBC_STATUS           (ISP_BASE_ADDR+0x0400)
#define ISP_AWBC_PARAM            (ISP_BASE_ADDR+0x0414)
#define ISP_AWBC_GAIN0            (ISP_BASE_ADDR+0x0418)
#define ISP_AWBC_GAIN1            (ISP_BASE_ADDR+0x041c)
#define ISP_AWBC_THRD             (ISP_BASE_ADDR+0x0420)
#define ISP_AWBC_OFFSET0_V0001    (ISP_BASE_ADDR+0x0424)
#define ISP_AWBC_OFFSET1_V0001    (ISP_BASE_ADDR+0x0428)

//BPC: bad pixel correction
#define ISP_BPC_STATUS            (ISP_BASE_ADDR+0x0500)
#define ISP_BPC_PARAM             (ISP_BASE_ADDR+0x0514)
#define ISP_BPC_THRD              (ISP_BASE_ADDR+0x0518)
#define ISP_BPC_MAP_ADDR          (ISP_BASE_ADDR+0x051c)
#define ISP_BPC_PIXEL_NUM         (ISP_BASE_ADDR+0x0520)
#define ISP_BPC_DIFF_THRD_V0002   (ISP_BASE_ADDR+0x0524)

//Wave denoise
#define ISP_WAVE_STATUS           (ISP_BASE_ADDR+0x0600)
#define ISP_WAVE_PARAM            (ISP_BASE_ADDR+0x0614)
#define ISP_WAVE_THRD_V0000       (ISP_BASE_ADDR+0x0618)
#define ISP_WAVE_SLICE_SIZE       (ISP_BASE_ADDR+0x061c)
#define ISP_WAVE_SLICE_INFO       (ISP_BASE_ADDR+0x0620)
#define ISP_WAVE_DISWEI_0         (ISP_BASE_ADDR+0x0624)
#define ISP_WAVE_DISWEI_1         (ISP_BASE_ADDR+0x0628)
#define ISP_WAVE_DISWEI_2         (ISP_BASE_ADDR+0x062c)
#define ISP_WAVE_DISWEI_3         (ISP_BASE_ADDR+0x0630)
#define ISP_WAVE_DISWEI_4         (ISP_BASE_ADDR+0x0634)
#define ISP_WAVE_RANWEI_0         (ISP_BASE_ADDR+0x0638)
#define ISP_WAVE_RANWEI_1         (ISP_BASE_ADDR+0x063c)
#define ISP_WAVE_RANWEI_2         (ISP_BASE_ADDR+0x0640)
#define ISP_WAVE_RANWEI_3         (ISP_BASE_ADDR+0x0644)
#define ISP_WAVE_RANWEI_4         (ISP_BASE_ADDR+0x0648)
#define ISP_WAVE_RANWEI_5         (ISP_BASE_ADDR+0x064c)
#define ISP_WAVE_RANWEI_6         (ISP_BASE_ADDR+0x0650)
#define ISP_WAVE_RANWEI_7         (ISP_BASE_ADDR+0x0654)

//GRGB correction
#define ISP_GRGB_STATUS           (ISP_BASE_ADDR+0x0700)
#define ISP_GRGB_PARAM            (ISP_BASE_ADDR+0x0714)

//CFA
#define ISP_CFA_STATUS            (ISP_BASE_ADDR+0x0800)
#define ISP_CFA_PARAM             (ISP_BASE_ADDR+0x0814)
#define ISP_CFA_SLICE_SIZE        (ISP_BASE_ADDR+0x0818)
#define ISP_CFA_SLICE_INFO        (ISP_BASE_ADDR+0x081c)

//CMC: Color matrix correction
#define ISP_CMC_STATUS            (ISP_BASE_ADDR+0x0900)
#define ISP_CMC_PARAM             (ISP_BASE_ADDR+0x0914)
#define ISP_CMC_MATRIX0           (ISP_BASE_ADDR+0x0918)
#define ISP_CMC_MATRIX1           (ISP_BASE_ADDR+0x091c)
#define ISP_CMC_MATRIX2           (ISP_BASE_ADDR+0x0920)
#define ISP_CMC_MATRIX3           (ISP_BASE_ADDR+0x0924)
#define ISP_CMC_MATRIX4           (ISP_BASE_ADDR+0x0928)

//GAMMA correction
#define ISP_GAMMA_STATUS          (ISP_BASE_ADDR+0x0a00)
#define ISP_GAMMA_PARAM           (ISP_BASE_ADDR+0x0a14)
#define ISP_GAMMA_NODE_X0         (ISP_BASE_ADDR+0x0a18)
#define ISP_GAMMA_NODE_X1         (ISP_BASE_ADDR+0x0a1c)
#define ISP_GAMMA_NODE_X2         (ISP_BASE_ADDR+0x0a20)
#define ISP_GAMMA_NODE_X3         (ISP_BASE_ADDR+0x0a24)
#define ISP_GAMMA_NODE_X4         (ISP_BASE_ADDR+0x0a28)
#define ISP_GAMMA_NODE_X5         (ISP_BASE_ADDR+0x0a2c)
#define ISP_GAMMA_NODE_X6         (ISP_BASE_ADDR+0x0a30)
#define ISP_GAMMA_NODE_X7         (ISP_BASE_ADDR+0x0a34)
#define ISP_GAMMA_NODE_Y0         (ISP_BASE_ADDR+0x0a38)
#define ISP_GAMMA_NODE_Y1         (ISP_BASE_ADDR+0x0a3c)
#define ISP_GAMMA_NODE_Y2         (ISP_BASE_ADDR+0x0a40)
#define ISP_GAMMA_NODE_Y3         (ISP_BASE_ADDR+0x0a44)
#define ISP_GAMMA_NODE_Y4         (ISP_BASE_ADDR+0x0a48)
#define ISP_GAMMA_NODE_Y5         (ISP_BASE_ADDR+0x0a4c)
#define ISP_GAMMA_NODE_Y6         (ISP_BASE_ADDR+0x0a50)
#define ISP_GAMMA_NODE_R0_V0001   (ISP_BASE_ADDR+0x0a38)
#define ISP_GAMMA_NODE_R1_V0001   (ISP_BASE_ADDR+0x0a3c)
#define ISP_GAMMA_NODE_R2_V0001   (ISP_BASE_ADDR+0x0a40)
#define ISP_GAMMA_NODE_R3_V0001   (ISP_BASE_ADDR+0x0a44)
#define ISP_GAMMA_NODE_R4_V0001   (ISP_BASE_ADDR+0x0a48)
#define ISP_GAMMA_NODE_R5_V0001   (ISP_BASE_ADDR+0x0a4c)
#define ISP_GAMMA_NODE_R6_V0001   (ISP_BASE_ADDR+0x0a50)
#define ISP_GAMMA_NODE_G0_V0001   (ISP_BASE_ADDR+0x0a54)
#define ISP_GAMMA_NODE_G1_V0001   (ISP_BASE_ADDR+0x0a58)
#define ISP_GAMMA_NODE_G2_V0001   (ISP_BASE_ADDR+0x0a5c)
#define ISP_GAMMA_NODE_G3_V0001   (ISP_BASE_ADDR+0x0a60)
#define ISP_GAMMA_NODE_G4_V0001   (ISP_BASE_ADDR+0x0a64)
#define ISP_GAMMA_NODE_G5_V0001   (ISP_BASE_ADDR+0x0a68)
#define ISP_GAMMA_NODE_G6_V0001   (ISP_BASE_ADDR+0x0a6c)
#define ISP_GAMMA_NODE_B0_V0001   (ISP_BASE_ADDR+0x0a70)
#define ISP_GAMMA_NODE_B1_V0001   (ISP_BASE_ADDR+0x0a74)
#define ISP_GAMMA_NODE_B2_V0001   (ISP_BASE_ADDR+0x0a78)
#define ISP_GAMMA_NODE_B3_V0001   (ISP_BASE_ADDR+0x0a7c)
#define ISP_GAMMA_NODE_B4_V0001   (ISP_BASE_ADDR+0x0a80)
#define ISP_GAMMA_NODE_B5_V0001   (ISP_BASE_ADDR+0x0a84)
#define ISP_GAMMA_NODE_B6_V0001   (ISP_BASE_ADDR+0x0a88)
#define ISP_GAMMA_NODE_IDX0_V0001 (ISP_BASE_ADDR+0x0a8c)
#define ISP_GAMMA_NODE_IDX1_V0001 (ISP_BASE_ADDR+0x0a90)
#define ISP_GAMMA_NODE_IDX2_V0001 (ISP_BASE_ADDR+0x0a94)
#define ISP_GAMMA_NODE_IDX3_V0001 (ISP_BASE_ADDR+0x0a98)

#define ISP_GAMMA_NODE_R0_V0002     (ISP_BASE_ADDR+0x0a18)
#define ISP_GAMMA_NODE_R1_V0002     (ISP_BASE_ADDR+0x0a1c)
#define ISP_GAMMA_NODE_R2_V0002     (ISP_BASE_ADDR+0x0a20)
#define ISP_GAMMA_NODE_R3_V0002     (ISP_BASE_ADDR+0x0a24)
#define ISP_GAMMA_NODE_R4_V0002     (ISP_BASE_ADDR+0x0a28)
#define ISP_GAMMA_NODE_R5_V0002     (ISP_BASE_ADDR+0x0a2c)
#define ISP_GAMMA_NODE_R6_V0002     (ISP_BASE_ADDR+0x0a30)
#define ISP_GAMMA_NODE_R7_V0002     (ISP_BASE_ADDR+0x0a34)
#define ISP_GAMMA_NODE_R8_V0002     (ISP_BASE_ADDR+0x0a38)
#define ISP_GAMMA_NODE_R9_V0002     (ISP_BASE_ADDR+0x0a3c)
#define ISP_GAMMA_NODE_R10_V0002   (ISP_BASE_ADDR+0x0a40)
#define ISP_GAMMA_NODE_R11_V0002   (ISP_BASE_ADDR+0x0a44)
#define ISP_GAMMA_NODE_R12_V0002   (ISP_BASE_ADDR+0x0a48)
#define ISP_GAMMA_NODE_R13_V0002   (ISP_BASE_ADDR+0x0a4c)
#define ISP_GAMMA_NODE_R14_V0002   (ISP_BASE_ADDR+0x0a50)
#define ISP_GAMMA_NODE_R15_V0002   (ISP_BASE_ADDR+0x0a54)
#define ISP_GAMMA_NODE_G0_V0002     (ISP_BASE_ADDR+0x0a58)
#define ISP_GAMMA_NODE_G1_V0002     (ISP_BASE_ADDR+0x0a5c)
#define ISP_GAMMA_NODE_G2_V0002     (ISP_BASE_ADDR+0x0a60)
#define ISP_GAMMA_NODE_G3_V0002     (ISP_BASE_ADDR+0x0a64)
#define ISP_GAMMA_NODE_G4_V0002     (ISP_BASE_ADDR+0x0a68)
#define ISP_GAMMA_NODE_G5_V0002     (ISP_BASE_ADDR+0x0a6c)
#define ISP_GAMMA_NODE_G6_V0002     (ISP_BASE_ADDR+0x0a70)
#define ISP_GAMMA_NODE_G7_V0002     (ISP_BASE_ADDR+0x0a74)
#define ISP_GAMMA_NODE_G8_V0002     (ISP_BASE_ADDR+0x0a78)
#define ISP_GAMMA_NODE_G9_V0002     (ISP_BASE_ADDR+0x0a7c)
#define ISP_GAMMA_NODE_G10_V0002   (ISP_BASE_ADDR+0x0a80)
#define ISP_GAMMA_NODE_G11_V0002   (ISP_BASE_ADDR+0x0a84)
#define ISP_GAMMA_NODE_G12_V0002   (ISP_BASE_ADDR+0x0a88)
#define ISP_GAMMA_NODE_G13_V0002   (ISP_BASE_ADDR+0x0a8c)
#define ISP_GAMMA_NODE_G14_V0002   (ISP_BASE_ADDR+0x0a90)
#define ISP_GAMMA_NODE_G15_V0002   (ISP_BASE_ADDR+0x0a94)
#define ISP_GAMMA_NODE_B0_V0002     (ISP_BASE_ADDR+0x0a98)
#define ISP_GAMMA_NODE_B1_V0002     (ISP_BASE_ADDR+0x0a9c)
#define ISP_GAMMA_NODE_B2_V0002     (ISP_BASE_ADDR+0x0aa0)
#define ISP_GAMMA_NODE_B3_V0002     (ISP_BASE_ADDR+0x0aa4)
#define ISP_GAMMA_NODE_B4_V0002     (ISP_BASE_ADDR+0x0aa8)
#define ISP_GAMMA_NODE_B5_V0002     (ISP_BASE_ADDR+0x0aac)
#define ISP_GAMMA_NODE_B6_V0002     (ISP_BASE_ADDR+0x0ab0)
#define ISP_GAMMA_NODE_B7_V0002     (ISP_BASE_ADDR+0x0ab4)
#define ISP_GAMMA_NODE_B8_V0002     (ISP_BASE_ADDR+0x0ab8)
#define ISP_GAMMA_NODE_B9_V0002     (ISP_BASE_ADDR+0x0abc)
#define ISP_GAMMA_NODE_B10_V0002   (ISP_BASE_ADDR+0x0ac0)
#define ISP_GAMMA_NODE_B11_V0002   (ISP_BASE_ADDR+0x0ac4)
#define ISP_GAMMA_NODE_B12_V0002   (ISP_BASE_ADDR+0x0ac8)
#define ISP_GAMMA_NODE_B13_V0002   (ISP_BASE_ADDR+0x0acc)
#define ISP_GAMMA_NODE_B14_V0002   (ISP_BASE_ADDR+0x0ad0)
#define ISP_GAMMA_NODE_B15_V0002   (ISP_BASE_ADDR+0x0ad4)

//CCE: Color conversion and enhancement
#define ISP_CCE_STATUS            (ISP_BASE_ADDR+0x0b00)
#define ISP_CCE_PARAM             (ISP_BASE_ADDR+0x0b14)
#define ISP_CCE_MATRIX0           (ISP_BASE_ADDR+0x0b18)
#define ISP_CCE_MATRIX1           (ISP_BASE_ADDR+0x0b1c)
#define ISP_CCE_MATRIX2           (ISP_BASE_ADDR+0x0b20)
#define ISP_CCE_MATRIX3           (ISP_BASE_ADDR+0x0b24)
#define ISP_CCE_MATRIX4           (ISP_BASE_ADDR+0x0b28)
#define ISP_CCE_SHIFT             (ISP_BASE_ADDR+0x0b2c)
#define ISP_CCE_UVD_THRD0         (ISP_BASE_ADDR+0x0b30)
#define ISP_CCE_UVD_THRD1         (ISP_BASE_ADDR+0x0b34)
#define ISP_CCE_UVD_PARAM0_V0001  (ISP_BASE_ADDR+0x0b38)
#define ISP_CCE_UVD_PARAM1_V0001  (ISP_BASE_ADDR+0x0b3c)

//PREF: yuv prefilter
#define ISP_PREF_STATUS           (ISP_BASE_ADDR+0x0c00)
#define ISP_PREF_PARAM            (ISP_BASE_ADDR+0x0c14)
#define ISP_PREF_THRD             (ISP_BASE_ADDR+0x0c18)
#define ISP_PREF_SLICE_SIZE       (ISP_BASE_ADDR+0x0c1c)
#define ISP_PREF_SLICE_INFO       (ISP_BASE_ADDR+0x0c20)

//BRIGHT: brightness adjustment
#define ISP_BRIGHT_STATUS         (ISP_BASE_ADDR+0x0d00)
#define ISP_BRIGHT_PARAM          (ISP_BASE_ADDR+0x0d14)
#define ISP_BRIGHT_SLICE_SIZE     (ISP_BASE_ADDR+0x0d18)
#define ISP_BRIGHT_SLICE_INFO     (ISP_BASE_ADDR+0x0d1c)

//CONTRAST: contrast adjustment
#define ISP_CONTRAST_STATUS       (ISP_BASE_ADDR+0x0e00)
#define ISP_CONTRAST_PARAM        (ISP_BASE_ADDR+0x0e14)

//HIST: Histogram statistics
#define ISP_HIST_STATUS           (ISP_BASE_ADDR+0x0f00)
#define ISP_HIST_PARAM            (ISP_BASE_ADDR+0x0f14)
#define ISP_HIST_RATIO            (ISP_BASE_ADDR+0x0f18)
#define ISP_HIST_MAX_MIN          (ISP_BASE_ADDR+0x0f1c)
#define ISP_HIST_CLEAR_ENABLE     (ISP_BASE_ADDR+0x0f20)

//AUTOCONT: auto contrast adjustment
#define ISP_AUTOCONT_STATUS       (ISP_BASE_ADDR+0x1000)
#define ISP_AUTOCONT_MAX_MINSTATUS_V0002    (ISP_BASE_ADDR+0x1004)
#define ISP_AUTOCONT_PARAM        (ISP_BASE_ADDR+0x1014)
#define ISP_AUTOCONT_MAX_MIN      (ISP_BASE_ADDR+0x1018)
#define ISP_AUTOCONT_ADJUST_V0002                  (ISP_BASE_ADDR+0x101c)

//AFM: auto focus monitor
#define ISP_AFM_STATUS            (ISP_BASE_ADDR+0x1100)
#define ISP_AFM_PARAM             (ISP_BASE_ADDR+0x1114)
#define ISP_AFM_WIN_RANGE0        (ISP_BASE_ADDR+0x1118)
#define ISP_AFM_WIN_RANGE1        (ISP_BASE_ADDR+0x111c)
#define ISP_AFM_WIN_RANGE2        (ISP_BASE_ADDR+0x1120)
#define ISP_AFM_WIN_RANGE3        (ISP_BASE_ADDR+0x1124)
#define ISP_AFM_WIN_RANGE4        (ISP_BASE_ADDR+0x1128)
#define ISP_AFM_WIN_RANGE5        (ISP_BASE_ADDR+0x112c)
#define ISP_AFM_WIN_RANGE6        (ISP_BASE_ADDR+0x1130)
#define ISP_AFM_WIN_RANGE7        (ISP_BASE_ADDR+0x1134)
#define ISP_AFM_WIN_RANGE8        (ISP_BASE_ADDR+0x1138)
#define ISP_AFM_WIN_RANGE9        (ISP_BASE_ADDR+0x113c)
#define ISP_AFM_WIN_RANGE10       (ISP_BASE_ADDR+0x1140)
#define ISP_AFM_WIN_RANGE11       (ISP_BASE_ADDR+0x1144)
#define ISP_AFM_WIN_RANGE12       (ISP_BASE_ADDR+0x1148)
#define ISP_AFM_WIN_RANGE13       (ISP_BASE_ADDR+0x114c)
#define ISP_AFM_WIN_RANGE14       (ISP_BASE_ADDR+0x1150)
#define ISP_AFM_WIN_RANGE15       (ISP_BASE_ADDR+0x1154)
#define ISP_AFM_WIN_RANGE16       (ISP_BASE_ADDR+0x1158)
#define ISP_AFM_WIN_RANGE17       (ISP_BASE_ADDR+0x115c)
#define ISP_AFM_STATISTIC0        (ISP_BASE_ADDR+0x1160)
#define ISP_AFM_STATISTIC1        (ISP_BASE_ADDR+0x1164)
#define ISP_AFM_STATISTIC2        (ISP_BASE_ADDR+0x1168)
#define ISP_AFM_STATISTIC3        (ISP_BASE_ADDR+0x116c)
#define ISP_AFM_STATISTIC4        (ISP_BASE_ADDR+0x1170)
#define ISP_AFM_STATISTIC5        (ISP_BASE_ADDR+0x1174)
#define ISP_AFM_STATISTIC6        (ISP_BASE_ADDR+0x1178)
#define ISP_AFM_STATISTIC7        (ISP_BASE_ADDR+0x117c)
#define ISP_AFM_STATISTIC8        (ISP_BASE_ADDR+0x1180)
#define ISP_AFM_STATISTIC0_L_V0001 (ISP_BASE_ADDR+0x1160)
#define ISP_AFM_STATISTIC0_H_V0001 (ISP_BASE_ADDR+0x1164)
#define ISP_AFM_STATISTIC1_L_V0001 (ISP_BASE_ADDR+0x1168)
#define ISP_AFM_STATISTIC1_H_V0001 (ISP_BASE_ADDR+0x116c)
#define ISP_AFM_STATISTIC2_L_V0001 (ISP_BASE_ADDR+0x1170)
#define ISP_AFM_STATISTIC2_H_V0001 (ISP_BASE_ADDR+0x1174)
#define ISP_AFM_STATISTIC3_L_V0001 (ISP_BASE_ADDR+0x1178)
#define ISP_AFM_STATISTIC3_H_V0001 (ISP_BASE_ADDR+0x117c)
#define ISP_AFM_STATISTIC4_L_V0001 (ISP_BASE_ADDR+0x1180)
#define ISP_AFM_STATISTIC4_H_V0001 (ISP_BASE_ADDR+0x1184)
#define ISP_AFM_STATISTIC5_L_V0001 (ISP_BASE_ADDR+0x1188)
#define ISP_AFM_STATISTIC5_H_V0001 (ISP_BASE_ADDR+0x118c)
#define ISP_AFM_STATISTIC6_L_V0001 (ISP_BASE_ADDR+0x1190)
#define ISP_AFM_STATISTIC6_H_V0001 (ISP_BASE_ADDR+0x1194)
#define ISP_AFM_STATISTIC7_L_V0001 (ISP_BASE_ADDR+0x1198)
#define ISP_AFM_STATISTIC7_H_V0001 (ISP_BASE_ADDR+0x119c)
#define ISP_AFM_STATISTIC8_L_V0001 (ISP_BASE_ADDR+0x11a0)
#define ISP_AFM_STATISTIC8_H_V0001 (ISP_BASE_ADDR+0x11a4)

//EE: edge enhancement
#define ISP_EE_STATUS             (ISP_BASE_ADDR+0x1200)
#define ISP_EE_PARAM              (ISP_BASE_ADDR+0x1214)

//EMBOSS
#define ISP_EMBOSS_STATUS         (ISP_BASE_ADDR+0x1300)
#define ISP_EMBOSS_PARAM          (ISP_BASE_ADDR+0x1314)

//FCS: false color suppression
#define ISP_FCS_STATUS            (ISP_BASE_ADDR+0x1400)
#define ISP_FCS_PARAM             (ISP_BASE_ADDR+0x1414)

//CSS: color saturation suppression
#define ISP_CSS_STATUS            (ISP_BASE_ADDR+0x1500)
#define ISP_CSS_PARAM             (ISP_BASE_ADDR+0x1514)
#define ISP_CSS_THRD0             (ISP_BASE_ADDR+0x1518)
#define ISP_CSS_THRD1             (ISP_BASE_ADDR+0x151c)
#define ISP_CSS_THRD2             (ISP_BASE_ADDR+0x1520)
#define ISP_CSS_THRD3             (ISP_BASE_ADDR+0x1524)
#define ISP_CSS_SLICE_SIZE        (ISP_BASE_ADDR+0x1528)
#define ISP_CSS_RATIO            (ISP_BASE_ADDR+0x152c)

//CSA: color saturation adjustment
#define ISP_CSA_STATUS            (ISP_BASE_ADDR+0x1600)
#define ISP_CSA_PARAM             (ISP_BASE_ADDR+0x1614)

//Store
#define ISP_STORE_STATUS               (ISP_BASE_ADDR+0x1700)
#define ISP_STORE_STATUS_PREVIEW_V0001 (ISP_BASE_ADDR+0x1700)
#define ISP_STORE_STATUS_PREVIEW_V0002 (ISP_BASE_ADDR+0x1704)
#define ISP_STORE_PARAM                (ISP_BASE_ADDR+0x1714)
#define ISP_STORE_SLICE_SIZE           (ISP_BASE_ADDR+0x1718)
#define ISP_STORE_SLICE_Y_ADDR         (ISP_BASE_ADDR+0x171c)
#define ISP_STORE_Y_PITCH              (ISP_BASE_ADDR+0x1720)
#define ISP_STORE_SLICE_U_ADDR         (ISP_BASE_ADDR+0x1724)
#define ISP_STORE_U_PITCH              (ISP_BASE_ADDR+0x1728)
#define ISP_STORE_SLICE_V_ADDR         (ISP_BASE_ADDR+0x172c)
#define ISP_STORE_V_PITCH              (ISP_BASE_ADDR+0x1730)
#define ISP_STORE_INT_CTRL             (ISP_BASE_ADDR+0x1734)
#define ISP_STORE_BORDER               (ISP_BASE_ADDR+0x1738)

//Feeder
#define ISP_FEEDER_STATUS         (ISP_BASE_ADDR+0x1800)
#define ISP_FEEDER_PARAM          (ISP_BASE_ADDR+0x1814)
#define ISP_FEEDER_SLICE_SIZE     (ISP_BASE_ADDR+0x1818)

//Arbiter
#define ISP_ARBITER_STATUS             (ISP_BASE_ADDR+0x1900)
#define ISP_AXI_WR_MASTER_STATUS_V0001 (ISP_BASE_ADDR+0x1904)
#define ISP_AXI_RD_MASTER_STATUS_V0001 (ISP_BASE_ADDR+0x1908)
#define ISP_ARBITER_PRERST             (ISP_BASE_ADDR+0x1914)
#define ISP_ARBITER_PAUSE_CYCLE        (ISP_BASE_ADDR+0x1918)
#define ISP_AXI_MASTER_CTRL_V0001      (ISP_BASE_ADDR+0x1908)

//HDR
#define ISP_HDR_STATUS            (ISP_BASE_ADDR+0x1a00)
#define ISP_HDR_PARAM             (ISP_BASE_ADDR+0x1a14)
#define ISP_HDR_INDEX             (ISP_BASE_ADDR+0x1a18)

//NAWB monitor
#define ISP_NAWBM_STATUS           (ISP_BASE_ADDR+0x1c00)
#define ISP_NAWBM_PARAM            (ISP_BASE_ADDR+0x1c14)
#define ISP_NAWBM_OFFSET           (ISP_BASE_ADDR+0x1c18)
#define ISP_NAWBM_BLK_SIZE        (ISP_BASE_ADDR+0x1c1c)
#define ISP_NAWBM_WR_0_S          (ISP_BASE_ADDR+0x1c20)
#define ISP_NAWBM_WR_0_E          (ISP_BASE_ADDR+0x1c24)
#define ISP_NAWBM_WR_1_S          (ISP_BASE_ADDR+0x1c28)
#define ISP_NAWBM_WR_1_E          (ISP_BASE_ADDR+0x1c2c)
#define ISP_NAWBM_WR_2_S          (ISP_BASE_ADDR+0x1c30)
#define ISP_NAWBM_WR_2_E          (ISP_BASE_ADDR+0x1c34)
#define ISP_NAWBM_WR_3_S          (ISP_BASE_ADDR+0x1c38)
#define ISP_NAWBM_WR_3_E          (ISP_BASE_ADDR+0x1c40)
#define ISP_NAWBM_WR_4_S          (ISP_BASE_ADDR+0x1c44)
#define ISP_NAWBM_WR_4_E          (ISP_BASE_ADDR+0x1c48)
#define ISP_NAWBM_ADDR              (ISP_BASE_ADDR+0x1c98)

//PRE wavelet
#define ISP_PRE_WAVELET_STATUS           (ISP_BASE_ADDR+0x1d00)
#define ISP_PRE_WAVELET_PARAM            (ISP_BASE_ADDR+0x1d14)

//PRE binning
#define ISP_BINNING_STATUS           (ISP_BASE_ADDR+0x1e00)
#define ISP_BINNING_PARAM            (ISP_BASE_ADDR+0x1e14)
#define ISP_BINNING_ADDR              (ISP_BASE_ADDR+0x1e18)
#define ISP_BINNING_PITCH             (ISP_BASE_ADDR+0x1e1c)

//Pre global gain
#define ISP_PRE_GLOBAL_GAIN_STATUS     (ISP_BASE_ADDR+0X1f00)
#define ISP_PRE_GLOBAL_GAIN            (ISP_BASE_ADDR+0X1f14)

//Common
#define ISP_COMMON_STATUS              (ISP_BASE_ADDR+0x2000)
#define ISP_COMMON_START               (ISP_BASE_ADDR+0x2014)
#define ISP_COMMON_PARAM               (ISP_BASE_ADDR+0x2018)
#define ISP_COMMON_BURST_SIZE          (ISP_BASE_ADDR+0x201c)
#define ISP_COMMON_MEM_SWITCH_V0000    (ISP_BASE_ADDR+0x2020)// noly v0000
#define ISP_COMMON_SHADOW              (ISP_BASE_ADDR+0x2024)
#define ISP_COMMON_BAYER_MODE          (ISP_BASE_ADDR+0x2028)
#define ISP_COMMON_SHADOW_ALL          (ISP_BASE_ADDR+0x202c)
#define ISP_COMMON_PMU_RAM_MASK        (ISP_BASE_ADDR+0x2038)
#define ISP_COMMON_HW_MASK             (ISP_BASE_ADDR+0x203c)
#define ISP_COMMON_HW_ENABLE           (ISP_BASE_ADDR+0x2040)
#define ISP_COMMON_SW_SWITCH           (ISP_BASE_ADDR+0x2044)
#define ISP_COMMON_SW_TOGGLE           (ISP_BASE_ADDR+0x2048)
#define ISP_COMMON_PREVIEW_STOP        (ISP_BASE_ADDR+0x204c)
#define ISP_COMMON_SHADOW_CNT          (ISP_BASE_ADDR+0x2050)
#define ISP_COMMON_AXI_STOP            (ISP_BASE_ADDR+0x2054)
#define ISP_COMMON_AXI_CTRL_V0002  	   (ISP_BASE_ADDR+0x2054)
#define ISP_COMMON_SLICE_CNT_V0001     (ISP_BASE_ADDR+0x2058)
#define ISP_COMMON_PERFORM_CNT_R_V0001 (ISP_BASE_ADDR+0x205c)
#define ISP_COMMON_PERFORM_CNT_V0001   (ISP_BASE_ADDR+0x2060)
#define ISP_COMMON_ENA_INTERRUPT       (ISP_BASE_ADDR+0x2078)
#define ISP_COMMON_CLR_INTERRUPT       (ISP_BASE_ADDR+0x207c)
#define ISP_COMMON_RAW_INTERRUPT       (ISP_BASE_ADDR+0x2080)
#define ISP_COMMON_INTERRUPT           (ISP_BASE_ADDR+0x2084)
#define ISP_COMMON_ENA_INTERRUPT1      (ISP_BASE_ADDR+0x2088)
#define ISP_COMMON_CLR_INTERRUPT1      (ISP_BASE_ADDR+0x208c)
#define ISP_COMMON_RAW_INTERRUPT1      (ISP_BASE_ADDR+0x2090)
#define ISP_COMMON_INTERRUPT1          (ISP_BASE_ADDR+0x2094)

//glb gain control
#define ISP_GAIN_GLB_STATUS       (ISP_BASE_ADDR+0x2100)
#define ISP_GAIN_GLB_PARAM        (ISP_BASE_ADDR+0x2114)
#define ISP_GAIN_SLICE_SIZE       (ISP_BASE_ADDR+0x2118)

//rgb gain control
#define ISP_GAIN_RGB_STATUS       (ISP_BASE_ADDR+0x2200)
#define ISP_GAIN_RGB_PARAM        (ISP_BASE_ADDR+0x2214)
#define ISP_GAIN_RGB_OFFSET       (ISP_BASE_ADDR+0x2218)

//YIQ
#define ISP_YIQ_STATUS_V0001      (ISP_BASE_ADDR+0x2300)
#define ISP_YIQ_PARAM_V0001       (ISP_BASE_ADDR+0x2314)
#define ISP_YGAMMA_X0_V0001       (ISP_BASE_ADDR+0x2318)
#define ISP_YGAMMA_X1_V0001       (ISP_BASE_ADDR+0x231c)
#define ISP_YGAMMA_Y0_V0001       (ISP_BASE_ADDR+0x2320)
#define ISP_YGAMMA_Y1_V0001       (ISP_BASE_ADDR+0x2324)
#define ISP_YGAMMA_Y2_V0001       (ISP_BASE_ADDR+0x2328)
#define ISP_AF_V_HEIGHT_V0001     (ISP_BASE_ADDR+0x232C)
#define ISP_AF_LINE_COUNTER_V0001 (ISP_BASE_ADDR+0x2330)
#define ISP_AF_LINE_STEP_V0001    (ISP_BASE_ADDR+0x2334)
#define ISP_YGAMMA_NODE_IDX_V0001 (ISP_BASE_ADDR+0x2338)
#define ISP_AF_LINE_START_V0001   (ISP_BASE_ADDR+0x233c)

//HUE
#define ISP_HUE_STATUS_V0001      (ISP_BASE_ADDR+0x2400)
#define ISP_HUE_PARAM_V0001       (ISP_BASE_ADDR+0x2414)

//NBPC
#define ISP_NBPC_STATUS_V0002      (ISP_BASE_ADDR+0x2500)
#define ISP_NBPC_PARAM_V0002       (ISP_BASE_ADDR+0x2514)
#define ISP_NBPC_CFG_V0002           (ISP_BASE_ADDR+0x2518)
#define ISP_NBPC_FACTOR_V0002     (ISP_BASE_ADDR+0x251c)
#define ISP_NBPC_COEFF_V0002       (ISP_BASE_ADDR+0x2520)
#define ISP_NBPC_LUT0_V0002          (ISP_BASE_ADDR+0x2524)
#define ISP_NBPC_LUT1_V0002          (ISP_BASE_ADDR+0x2528)
#define ISP_NBPC_LUT2_V0002          (ISP_BASE_ADDR+0x252c)
#define ISP_NBPC_LUT3_V0002          (ISP_BASE_ADDR+0x2530)
#define ISP_NBPC_LUT4_V0002          (ISP_BASE_ADDR+0x2534)
#define ISP_NBPC_LUT5_V0002          (ISP_BASE_ADDR+0x2538)
#define ISP_NBPC_LUT6_V0002          (ISP_BASE_ADDR+0x253c)
#define ISP_NBPC_LUT7_V0002          (ISP_BASE_ADDR+0x2540)
#define ISP_NBPC_SEL_V0002            (ISP_BASE_ADDR+0x2544)
#define ISP_NBPC_MAP_ADDR_V0002  (ISP_BASE_ADDR+0x2548)

//GAMMA CORE
#define ISP_GAMMA_NODE_RX0_V0002  (ISP_BASE_ADDR+0x2a14)
#define ISP_GAMMA_NODE_RX1_V0002  (ISP_BASE_ADDR+0x2a18)
#define ISP_GAMMA_NODE_RX2_V0002  (ISP_BASE_ADDR+0x2a1c)
#define ISP_GAMMA_NODE_RX3_V0002  (ISP_BASE_ADDR+0x2a20)
#define ISP_GAMMA_NODE_RX4_V0002  (ISP_BASE_ADDR+0x2a24)
#define ISP_GAMMA_NODE_RX5_V0002  (ISP_BASE_ADDR+0x2a28)
#define ISP_GAMMA_NODE_GX0_V0002  (ISP_BASE_ADDR+0x2a2c)
#define ISP_GAMMA_NODE_GX1_V0002  (ISP_BASE_ADDR+0x2a30)
#define ISP_GAMMA_NODE_GX2_V0002  (ISP_BASE_ADDR+0x2a34)
#define ISP_GAMMA_NODE_GX3_V0002  (ISP_BASE_ADDR+0x2a38)
#define ISP_GAMMA_NODE_GX4_V0002  (ISP_BASE_ADDR+0x2a3c)
#define ISP_GAMMA_NODE_GX5_V0002  (ISP_BASE_ADDR+0x2a40)
#define ISP_GAMMA_NODE_BX0_V0002  (ISP_BASE_ADDR+0x2a44)
#define ISP_GAMMA_NODE_BX1_V0002  (ISP_BASE_ADDR+0x2a48)
#define ISP_GAMMA_NODE_BX2_V0002  (ISP_BASE_ADDR+0x2a4c)
#define ISP_GAMMA_NODE_BX3_V0002  (ISP_BASE_ADDR+0x2a50)
#define ISP_GAMMA_NODE_BX4_V0002  (ISP_BASE_ADDR+0x2a54)
#define ISP_GAMMA_NODE_BX5_V0002  (ISP_BASE_ADDR+0x2a58)

//awbm param output
#define ISP_AWBM_OUTPUT           (ISP_BASE_ADDR + 0x5000) //end: 0x7000
#define ISP_AWBM_ITEM             1024

//histogram param output
#define ISP_HIST_OUTPUT           (ISP_BASE_ADDR + 0x7010) //end: 0x7410
#define ISP_HIST_ITEM             256

//hdr comp param output
#define ISP_HDR_COMP_OUTPUT       (ISP_BASE_ADDR + 0x7500) //end: 0x7500
#define ISP_HDR_COMP_ITEM         64

//hdr p2e param output
#define ISP_HDR_P2E_OUTPUT        (ISP_BASE_ADDR + 0x7700) //end: 0x7700
#define ISP_HDR_P2E_ITEM          32

//hdr e2p para output
#define ISP_HDR_E2P_OUTPUT        (ISP_BASE_ADDR + 0x7800) //end: 0x7800
#define ISP_HDR_E2P_ITEM          32

//aem param output
#define ISP_AEM_OUTPUT              (ISP_BASE_ADDR + 0x7900) //end: 0x7900
#define ISP_AEM_ITEM                   1024

/**---------------------------------------------------------------------------*
**                               Data Prototype                              **
**----------------------------------------------------------------------------*/
// mem reg
union _isp_mem_reg_tag {
    struct _isp_mem_reg_map {
        volatile unsigned int value          :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

// fetch reg
union _isp_fetch_status_tag {
    struct _isp_fetch_status_map {
        volatile unsigned int status          :32; // fetch status read only
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_fetch_status_preview_v0001_tag {
    struct _isp_fetch_status_preview_v0001_map {
        volatile unsigned int sof_counter         :8;
        volatile unsigned int eof_counter         :8;
        volatile unsigned int slice_start_counter :4;
        volatile unsigned int fetch_hw_counter    :9;
        volatile unsigned int preview_stop        :1;
        volatile unsigned int fetch_buf_empty     :1;
        volatile unsigned int fetch_buf_full      :1;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_fetch_param_tag {
    struct _isp_fetch_param_map {
        volatile unsigned int bypass          :1;
        volatile unsigned int reserved0       :6;
        volatile unsigned int sub_stract      :1;
        volatile unsigned int afifo_wr_mode   :1;
        volatile unsigned int reserved1       :23;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_fetch_slice_size_tag {
    struct _isp_fetch_slice_size_map {
        volatile unsigned int slice_with      :16;
        volatile unsigned int slice_height    :16;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_fetch_slice_y_addr_tag {
    struct _isp_fetch_slice_y_addr_map {
        volatile unsigned int y_addr          :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_fetch_slice_y_pitch_tag {
    struct _isp_fetch_slice_y_pitch_map {
        volatile unsigned int y_pitch         :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_fetch_slice_u_addr_tag {
    struct _isp_fetch_slice_u_addr_map {
        volatile unsigned int u_addr          :32;
    }mBits ;
    volatile unsigned int dwValue ;
}ISP_FETCH_SLICE_U_ADDR_U;

union _isp_fetch_slice_u_pitch_tag {
    struct _isp_fetch_slice_u_pitch_map {
        volatile unsigned int u_pitch         :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_fetch_mipi_word_info_tag {
    struct _isp_fetch_mipi_word_info_map {
        volatile unsigned int word_num        :16;
        volatile unsigned int reserved        :16;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_fetch_mipi_byte_info_tag {
    struct _isp_fetch_mipi_word_byte_map {
        volatile unsigned int byte_rel_pos    :4;
        volatile unsigned int reserved        :28;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_fetch_slice_v_addr_tag {
    struct _isp_fetch_slice_v_addr_map {
        volatile unsigned int v_addr          :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_fetch_slice_v_pitch_tag {
    struct _isp_fetch_slice_v_pitch_map {
        volatile unsigned int v_pitch         :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_fetch_preview_cnt_tag {
    struct _isp_fetch_preview_cnt_map {
        volatile unsigned int stop_cnt_eb     :1;
        volatile unsigned int reserved0       :7;
        volatile unsigned int stop_cnt_num    :8;
        volatile unsigned int reserved1       :16;
    }mBits ;
    volatile unsigned int dwValue ;
};


union _isp_fetch_buf_addr_v0002_tag {
    struct _isp_fetch_buf_addr_v0002__map {
        volatile unsigned int fetch_buf_waddr   :13;
        volatile unsigned int reserved0             :3;
        volatile unsigned int fetch_buf_raddr    :13;
        volatile unsigned int reserved1             :3;
    }mBits ;
    volatile unsigned int dwValue ;
};

// BLC&NLC
union _isp_bnlc_status_tag {
    struct _isp_bnlc_status_map {
        volatile unsigned int status          :32; // blc and nlc status read only
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_param_tag {
    struct _isp_bnlc_param_map {
        volatile unsigned int blc_bypass      :1;
        volatile unsigned int nlc_bypass      :1;
        volatile unsigned int blc_mode        :1; //0:software 1:camera interface
        volatile unsigned int reserved        :29;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_nlc_param_v0001_tag {
    struct _isp_nlc_param_v0001_map {
        volatile unsigned int bypass      :1;
        volatile unsigned int reserved    :31;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_b_param_r_b_tag {
    struct _isp_bnlc_b_param_r_b_map {
        volatile unsigned int blc_r           :10;
        volatile unsigned int blc_b           :10;
        volatile unsigned int reserved        :12;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_b_param_g_tag {
    struct _isp_bnlc_b_param_g_map {
        volatile unsigned int blc_gr          :10;
        volatile unsigned int blc_gb          :10;
        volatile unsigned int reserved        :12;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_r0_tag {
    struct _isp_bnlc_n_param_r0_map {
        volatile unsigned int nlc_r_node0     :10;
        volatile unsigned int nlc_r_node1     :10;
        volatile unsigned int nlc_r_node2     :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_r1_tag {
    struct _isp_bnlc_n_param_r1_map {
        volatile unsigned int nlc_r_node3     :10;
        volatile unsigned int nlc_r_node4     :10;
        volatile unsigned int nlc_r_node5     :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_r2_tag {
    struct _isp_bnlc_n_param_r2_map {
        volatile unsigned int nlc_r_node6     :10;
        volatile unsigned int nlc_r_node7     :10;
        volatile unsigned int nlc_r_node8     :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_r3_tag {
    struct _isp_bnlc_n_param_r3_map {
        volatile unsigned int nlc_r_node9     :10;
        volatile unsigned int nlc_r_node10    :10;
        volatile unsigned int nlc_r_node11    :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_r4_tag {
    struct _isp_bnlc_n_param_r4_map {
        volatile unsigned int nlc_r_node12    :10;
        volatile unsigned int nlc_r_node13    :10;
        volatile unsigned int nlc_r_node14    :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_r5_tag {
    struct _isp_bnlc_n_param_r5_map {
        volatile unsigned int nlc_r_node15    :10;
        volatile unsigned int nlc_r_node16    :10;
        volatile unsigned int nlc_r_node17    :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_r6_tag {
    struct _isp_bnlc_n_param_r6_map {
        volatile unsigned int nlc_r_node18    :10;
        volatile unsigned int nlc_r_node19    :10;
        volatile unsigned int nlc_r_node20    :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_r7_tag {
    struct _isp_bnlc_n_param_r7_map {
        volatile unsigned int nlc_r_node21    :10;
        volatile unsigned int nlc_r_node22    :10;
        volatile unsigned int nlc_r_node23    :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_r8_tag {
    struct _isp_bnlc_n_param_r8_map {
        volatile unsigned int nlc_r_node24    :10;
        volatile unsigned int nlc_r_node25    :10;
        volatile unsigned int nlc_r_node26    :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_r9_tag {
    struct _isp_bnlc_n_param_r9_map {
        volatile unsigned int reserved0       :10;
        volatile unsigned int nlc_r_node27    :10;
        volatile unsigned int nlc_r_node28    :10;
        volatile unsigned int reserved1       :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_g0_tag {
    struct _isp_bnlc_n_param_g0_map {
        volatile unsigned int nlc_g_node0     :10;
        volatile unsigned int nlc_g_node1     :10;
        volatile unsigned int nlc_g_node2     :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_g1_tag {
    struct _isp_bnlc_n_param_g1_map {
        volatile unsigned int nlc_g_node3     :10;
        volatile unsigned int nlc_g_node4     :10;
        volatile unsigned int nlc_g_node5     :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_g2_tag {
    struct _isp_bnlc_n_param_g2_map {
        volatile unsigned int nlc_g_node6     :10;
        volatile unsigned int nlc_g_node7     :10;
        volatile unsigned int nlc_g_node8     :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_g3_tag {
    struct _isp_bnlc_n_param_g3_map {
        volatile unsigned int nlc_g_node9     :10;
        volatile unsigned int nlc_g_node10    :10;
        volatile unsigned int nlc_g_node11    :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_g4_tag {
    struct _isp_bnlc_n_param_g4_map {
        volatile unsigned int nlc_g_node12    :10;
        volatile unsigned int nlc_g_node13    :10;
        volatile unsigned int nlc_g_node14    :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_g5_tag {
    struct _isp_bnlc_n_param_g5_map {
        volatile unsigned int nlc_g_node15    :10;
        volatile unsigned int nlc_g_node16    :10;
        volatile unsigned int nlc_g_node17    :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_g6_tag {
    struct _isp_bnlc_n_param_g6_map {
        volatile unsigned int nlc_g_node18    :10;
        volatile unsigned int nlc_g_node19    :10;
        volatile unsigned int nlc_g_node20    :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_g7_tag {
    struct _isp_bnlc_n_param_g7_map {
        volatile unsigned int nlc_g_node21    :10;
        volatile unsigned int nlc_g_node22    :10;
        volatile unsigned int nlc_g_node23    :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_g8_tag {
    struct _isp_bnlc_n_param_g8_map {
        volatile unsigned int nlc_g_node24    :10;
        volatile unsigned int nlc_g_node25    :10;
        volatile unsigned int nlc_g_node26    :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_g9_tag {
    struct _isp_bnlc_n_param_g9_map {
        volatile unsigned int reserved0       :10;
        volatile unsigned int nlc_g_node27    :10;
        volatile unsigned int nlc_g_node28    :10;
        volatile unsigned int reserved1       :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_b0_tag {
    struct _isp_bnlc_n_param_b0_map {
        volatile unsigned int nlc_b_node0     :10;
        volatile unsigned int nlc_b_node1     :10;
        volatile unsigned int nlc_b_node2     :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_b1_tag {
    struct _isp_bnlc_n_param_b1_map {
        volatile unsigned int nlc_b_node3     :10;
        volatile unsigned int nlc_b_node4     :10;
        volatile unsigned int nlc_b_node5     :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_b2_tag {
    struct _isp_bnlc_n_param_b2_map {
        volatile unsigned int nlc_b_node6     :10;
        volatile unsigned int nlc_b_node7     :10;
        volatile unsigned int nlc_b_node8     :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_b3_tag {
    struct _isp_bnlc_n_param_b3_map {
        volatile unsigned int nlc_b_node9     :10;
        volatile unsigned int nlc_b_node10    :10;
        volatile unsigned int nlc_b_node11    :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_b4_tag {
    struct _isp_bnlc_n_param_b4_map {
        volatile unsigned int nlc_b_node12    :10;
        volatile unsigned int nlc_b_node13    :10;
        volatile unsigned int nlc_b_node14    :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_b5_tag {
    struct _isp_bnlc_n_param_b5_map {
        volatile unsigned int nlc_b_node15    :10;
        volatile unsigned int nlc_b_node16    :10;
        volatile unsigned int nlc_b_node17    :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_b6_tag {
    struct _isp_bnlc_n_param_b6_map {
        volatile unsigned int nlc_b_node18    :10;
        volatile unsigned int nlc_b_node19    :10;
        volatile unsigned int nlc_b_node20    :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_b7_tag {
    struct _isp_bnlc_n_param_b7_map {
        volatile unsigned int nlc_b_node21    :10;
        volatile unsigned int nlc_b_node22    :10;
        volatile unsigned int nlc_b_node23    :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_b8_tag {
    struct _isp_bnlc_n_param_b8_map {
        volatile unsigned int nlc_b_node24    :10;
        volatile unsigned int nlc_b_node25    :10;
        volatile unsigned int nlc_b_node26    :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_b9_tag {
    struct _isp_bnlc_n_param_b9_map {
        volatile unsigned int reserved0       :10;
        volatile unsigned int nlc_b_node27    :10;
        volatile unsigned int nlc_b_node28    :10;
        volatile unsigned int reserved1       :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_l0_tag {
    struct _isp_bnlc_n_param_l0_map {
        volatile unsigned int nlc_l_node0     :10;
        volatile unsigned int nlc_l_node1     :10;
        volatile unsigned int nlc_l_node2     :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_l1_tag {
    struct _isp_bnlc_n_param_l1_map {
        volatile unsigned int nlc_l_node3     :10;
        volatile unsigned int nlc_l_node4     :10;
        volatile unsigned int nlc_l_node5     :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_l2_tag {
    struct _isp_bnlc_n_param_l2_map {
        volatile unsigned int nlc_l_node6     :10;
        volatile unsigned int nlc_l_node7     :10;
        volatile unsigned int nlc_l_node8     :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_l3_tag {
    struct _isp_bnlc_n_param_l3_map {
        volatile unsigned int nlc_l_node9     :10;
        volatile unsigned int nlc_l_node10    :10;
        volatile unsigned int nlc_l_node11    :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_l4_tag {
    struct _isp_bnlc_n_param_l4_map {
        volatile unsigned int nlc_l_node12    :10;
        volatile unsigned int nlc_l_node13    :10;
        volatile unsigned int nlc_l_node14    :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_l5_tag {
    struct _isp_bnlc_n_param_l5_map {
        volatile unsigned int nlc_l_node15    :10;
        volatile unsigned int nlc_l_node16    :10;
        volatile unsigned int nlc_l_node17    :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_l6_tag {
    struct _isp_bnlc_n_param_l6_map {
        volatile unsigned int nlc_l_node18    :10;
        volatile unsigned int nlc_l_node19    :10;
        volatile unsigned int nlc_l_node20    :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_l7_tag {
    struct _isp_bnlc_n_param_l7_map {
        volatile unsigned int nlc_l_node21    :10;
        volatile unsigned int nlc_l_node22    :10;
        volatile unsigned int nlc_l_node23    :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_n_param_l8_tag {
    struct _isp_bnlc_n_param_l8_map {
        volatile unsigned int nlc_l_node24    :10;
        volatile unsigned int nlc_l_node25    :10;
        volatile unsigned int nlc_l_node26    :10;
        volatile unsigned int reserved        :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_slice_size_tag {
    struct _isp_bnlc_slice_size_map {
        volatile unsigned int slice_width     :16;
        volatile unsigned int slice_height    :16;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bnlc_slice_info_tag {
    struct _isp_bnlc_slice_info_map {
        volatile unsigned int edge_info       :4;
        volatile unsigned int reserved        :28;
    }mBits ;
    volatile unsigned int dwValue ;
};

// Lens C
union _isp_lens_status_tag {
    struct _isp_lens_status_map {
        volatile unsigned int status          :32; // lens shading status read only
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_lens_param_tag {
    struct _isp_lens_param_map {
        volatile unsigned int bypass          :1;
        volatile unsigned int use_buf_sel     :1;
        volatile unsigned int reserved        :30;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_lens_param_addr_tag {
    struct _isp_lens_param_addr_map {
        volatile unsigned int grid_addr       :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_lens_slice_pos_tag {
    struct _isp_lens_slice_pos_map {
        volatile unsigned int offset_x        :7;
        volatile unsigned int reserved0       :1;
        volatile unsigned int offset_y        :7;
        volatile unsigned int reserved1       :1;
        volatile unsigned int reserved        :16;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_lens_loader_eb_tag {
    struct _isp_lens_loader_eb_map {
        volatile unsigned int eb              :1;
        volatile unsigned int reserved        :31;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_lens_grid_pitch_tag {
    struct _isp_lens_grid_pitch_map {
        volatile unsigned int grid_pitch      :9;
        volatile unsigned int reserved0       :7;
        volatile unsigned int grid_mode       :2;
        volatile unsigned int reserved1       :14;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_lens_grid_size_tag {
    struct _isp_lens_grid_size_map {
        volatile unsigned int slice_width     :16;
        volatile unsigned int slice_height    :16;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_lens_load_buf_tag {
    struct _isp_lens_load_buf_map {
        volatile unsigned int load_buf_sel    :1; //0:buf0 1:buf1
        volatile unsigned int reserved        :31;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_lens_misc_tag {
    struct _isp_lens_misc_map {
        volatile unsigned int lens_endian_type :2;
        volatile unsigned int reserved         :30;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_lens_slice_size_tag {
    struct _isp_lens_slice_size_map {
        volatile unsigned int slice_width     :16;
        volatile unsigned int slice_height    :16;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_lens_done_sel_v0002_tag {
    struct _isp_lens_done_sel_v0002_map {
        volatile unsigned int done_sel        :1;
        volatile unsigned int reserved        :31;
    }mBits ;
    volatile unsigned int dwValue ;
};

// AWBM
union _isp_awbm_status_tag {
    struct _isp_awbm_status_map {
        volatile unsigned int status        :32; // awbm status read only
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_awbm_param_tag {
    struct _isp_awbm_param_map {
        volatile unsigned int bypass        :1;
        volatile unsigned int mode          :1;
        volatile unsigned int skip_num      :4;
        volatile unsigned int reserved      :26;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_awbm_offset_tag {
    struct _isp_awbm_offset_map {
        volatile unsigned int offset_x      :16;
        volatile unsigned int offset_y      :16;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_awbm_blk_size_tag {
    struct _isp_awbm_blk_size_map {
        volatile unsigned int blk_width     :7;
        volatile unsigned int blk_height    :7;
        volatile unsigned int awbm_avgshf   :5;
        volatile unsigned int reserved      :13;
    }mBits ;
    volatile unsigned int dwValue ;
};


union _isp_awbm_blk_size_v0002_tag {
    struct _isp_awbm_blk_size_v0002_map {
        volatile unsigned int blk_width     :9;
        volatile unsigned int blk_height    :9;
        volatile unsigned int awbm_avgshf   :5;
        volatile unsigned int reserved      :9;
    }mBits ;
    volatile unsigned int dwValue ;
};

// NAWBM
union _isp_nawbm_status_v0002_tag {
    struct _isp_nawbm_status_v0002_map {
        volatile unsigned int status        :32; // nawbm status read only
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_awbm_param_v0002_tag {
    struct _isp_awbm_param_v0002_map {
        volatile unsigned int bypass                 :1;
        volatile unsigned int mode                   :1;
        volatile unsigned int skip_num             :4;
        volatile unsigned int skip_clr                :1;
        volatile unsigned int reserved0            :17;
        volatile unsigned int o_bypass             :1;
        volatile unsigned int reserved1            :3;
        volatile unsigned int skip_num_status  :4;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_nawbm_offset_v0002_tag {
    struct _isp_nawbm_offset_v0002_map {
        volatile unsigned int offset_x      :16;
        volatile unsigned int offset_y      :16;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_nawbm_blk_size_v0002_tag {
    struct _isp_nawbm_blk_size_v0002_map {
        volatile unsigned int blk_width      :9;
        volatile unsigned int blk_height     :9;
        volatile unsigned int awbm_avgshf :5;
        volatile unsigned int reserved0      :1;
        volatile unsigned int ddr_bypass    :1;
        volatile unsigned int reserved1      :7;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_nawbm_wr_s0_v0002_tag {
    struct _isp_nawbm_wr_s0_v0002_map {
        volatile unsigned int wr_0_sy      :10;
        volatile unsigned int reserved0    :6;
        volatile unsigned int wr_0_sx      :10;
        volatile unsigned int reserved1    :6;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_nawbm_wr_e0_v0002_tag {
    struct _isp_nawbm_wr_e0_v0002_map {
        volatile unsigned int wr_0_ey      :10;
        volatile unsigned int reserved0    :6;
        volatile unsigned int wr_0_ex      :10;
        volatile unsigned int reserved1    :6;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_nawbm_wr_s1_v0002_tag {
    struct _isp_nawbm_wr_s1_v0002_map {
        volatile unsigned int wr_1_sy      :10;
        volatile unsigned int reserved0    :6;
        volatile unsigned int wr_1_sx      :10;
        volatile unsigned int reserved1    :6;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_nawbm_wr_e1_v0002_tag {
    struct _isp_nawbm_wr_e1_v0002_map {
        volatile unsigned int wr_1_ey      :10;
        volatile unsigned int reserved0    :6;
        volatile unsigned int wr_1_ex      :10;
        volatile unsigned int reserved1    :6;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_nawbm_wr_s2_v0002_tag {
    struct _isp_nawbm_wr_s2_v0002_map {
        volatile unsigned int wr_2_sy      :10;
        volatile unsigned int reserved0    :6;
        volatile unsigned int wr_2_sx      :10;
        volatile unsigned int reserved1    :6;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_nawbm_wr_e2_v0002_tag {
    struct _isp_nawbm_wr_e2_v0002_map {
        volatile unsigned int wr_2_ey      :10;
        volatile unsigned int reserved0    :6;
        volatile unsigned int wr_2_ex      :10;
        volatile unsigned int reserved1    :6;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_nawbm_wr_s3_v0002_tag {
    struct _isp_nawbm_wr_s3_v0002_map {
        volatile unsigned int wr_3_sy      :10;
        volatile unsigned int reserved0    :6;
        volatile unsigned int wr_3_sx      :10;
        volatile unsigned int reserved1    :6;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_nawbm_wr_e3_v0002_tag {
    struct _isp_nawbm_wr_e3_v0002_map {
        volatile unsigned int wr_3_ey      :10;
        volatile unsigned int reserved0    :6;
        volatile unsigned int wr_3_ex      :10;
        volatile unsigned int reserved1    :6;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_nawbm_wr_s4_v0002_tag {
    struct _isp_nawbm_wr_s4_v0002_map {
        volatile unsigned int wr_4_sy      :10;
        volatile unsigned int reserved0    :6;
        volatile unsigned int wr_4_sx      :10;
        volatile unsigned int reserved1    :6;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_nawbm_wr_e4_v0002_tag {
    struct _isp_nawbm_wr_e4_v0002_map {
        volatile unsigned int wr_0_ey      :10;
        volatile unsigned int reserved0    :6;
        volatile unsigned int wr_0_ex      :10;
        volatile unsigned int reserved1    :6;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_nawbm_addr_v0002_tag {
    struct _isp_nawbm_addr_v0002_map {
        volatile unsigned int addr      :32;
    }mBits ;
    volatile unsigned int dwValue ;
};


// BINING
union _isp_bining_status_v0002_tag {
    struct _isp_bining_status_v0002_map {
        volatile unsigned int status        :32; // bining status read only
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bining_param_v0002_tag {
    struct _isp_bining_param_v0002_map {
        volatile unsigned int bypass                :1;
        volatile unsigned int endian                :1;
        volatile unsigned int burst_len            :2;
        volatile unsigned int h_bining             :3;
        volatile unsigned int fifo_clr               :1;
        volatile unsigned int v_bining             :3;
        volatile unsigned int reserved            :19;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bining_addr_v0002_tag {
    struct _isp_bining_addr_v0002_map {
        volatile unsigned int addr      :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bining_pitch_v0002_tag {
    struct _isp_bining_pitch_v0002_map {
        volatile unsigned int pitch        :16;
        volatile unsigned int reserved  :16;
    }mBits ;
    volatile unsigned int dwValue ;
};

// AWBC
union _isp_awbc_status_tag {
    struct _isp_awbc_status_map {
        volatile unsigned int status        :32; // awbc status read only
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_awbc_param_tag {
    struct _isp_awbc_param_map {
        volatile unsigned int bypass        :1;
        volatile unsigned int reserved      :31;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_awbc_gain0_tag {
    struct _isp_awbc_gain0_map {
        volatile unsigned int r_gain        :12;
        volatile unsigned int b_gain        :12;
        volatile unsigned int reserved      :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_awbc_gain1_tag {
    struct _isp_awbc_gain1_map {
        volatile unsigned int g_gain        :12;
        volatile unsigned int reserved      :20;

    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_awbc_gain0_v0001_tag {
    struct _isp_awbc_gain0_v0001_map {
        volatile unsigned int r_gain        :16;
        volatile unsigned int b_gain        :16;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_awbc_gain1_v0001_tag {
    struct _isp_awbc_gain1_v0001_map {
        volatile unsigned int gr_gain        :16;
        volatile unsigned int gb_gain      :16;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_awbc_thrd_tag {
    struct _isp_awbc_thrd_map {
        volatile unsigned int r_thr         :10;
        volatile unsigned int g_thr         :10;
        volatile unsigned int b_thr         :10;
        volatile unsigned int reserved      :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_awbc_offset0_v0001_tag {
    struct _isp_awbc_offset0_v0001_map {
        volatile unsigned int r_offset      :16;
        volatile unsigned int b_offset      :16;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_awbc_offset1_v0001_tag {
    struct _isp_awbc_offset1_v0001_map {
        volatile unsigned int gr_offset     :16;
        volatile unsigned int gb_offset     :16;
    }mBits ;
    volatile unsigned int dwValue ;
};

// BPC
union _isp_bpc_status_tag {
    struct _isp_bpc_status_map {
        volatile unsigned int status        :32; // bpc status read only
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bpc_param_tag {
    struct _isp_bpc_param_map {
        volatile unsigned int bypass        :1;
        volatile unsigned int mode          :1; // 0: maping 1:adptive
        volatile unsigned int pattern_type  :6;
        volatile unsigned int reserved0     :2;
        volatile unsigned int maxminThr     :10;
        volatile unsigned int super_badThr  :10;
        volatile unsigned int reserved1     :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bpc_thrd_tag {
    struct _isp_bpc_thrd_map {
        volatile unsigned int flat_thr      :10;
        volatile unsigned int std_thr       :10;
        volatile unsigned int texture_thr   :10;
        volatile unsigned int reserved      :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bpc_map_addr_tag {
    struct _isp_bpc_map_addr_map {
        volatile unsigned int map_addr      :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bpc_pixel_num_tag {
    struct _isp_bpc_pixel_num_map {
        volatile unsigned int pixel_num     :14;
        volatile unsigned int reserved      :18;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bpc_diff_thrd_v0002_tag {
    struct _isp_bpc_diff_thrd_v0002_map {
        volatile unsigned int diff_thr        :10;
        volatile unsigned int reserved      :22;
    }mBits ;
    volatile unsigned int dwValue ;
};

// NBPC
union _isp_nbpc_status_v0002_tag {
    struct _isp_nbpc_status_v0002_map {
        volatile unsigned int status        :32; // bpc status read only
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_nbpc_param_v0002_tag {
    struct _isp_nbpc_param_v0002_map {
        volatile unsigned int bypass        :1;
        volatile unsigned int bypass_pvd        :1;
        volatile unsigned int reserved0        :2;
        volatile unsigned int mode          :2;
        volatile unsigned int reserved1        :2;
        volatile unsigned int mask_mode        :3;
        volatile unsigned int reserved2        :5;
        volatile unsigned int kmin        :3;
        volatile unsigned int reserved3      :1;
        volatile unsigned int kmax        :3;
        volatile unsigned int reserved4        :9;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_nbpc_cfg_v0002_tag {
    struct _isp_nbpc_cfg_v0002_map {
        volatile unsigned int cntr_theshold         :3;
        volatile unsigned int map_hw_fifo_clr_en   :1;
        volatile unsigned int ktimes               :3;
        volatile unsigned int map_fifo_clr         :1;
        volatile unsigned int delt34               :3;
        volatile unsigned int reserved1            :5;
        volatile unsigned int bad_pixel_num        :16;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_nbpc_factor_v0002_tag {
    struct _isp_nbpc_factor_v0002_map {
        volatile unsigned int flat_factor       :3;
        volatile unsigned int reserved0         :1;
        volatile unsigned int safe_factor       :5;
        volatile unsigned int reserved1         :23;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_nbpc_coeff_v0002_tag {
    struct _isp_nbpc_coeff_v0002_map {
        volatile unsigned int spike_coeff       :3;
        volatile unsigned int reserved0         :1;
        volatile unsigned int dead_coeff        :3;
        volatile unsigned int reserved1         :25;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_nbpc_lut0_v0002_tag {
    struct _isp_nbpc_lut0_v0002_map {
        volatile unsigned int intercept_b0      :10;
        volatile unsigned int slope_k0           :10;
        volatile unsigned int lut_level0          :10;
        volatile unsigned int reserved           :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_nbpc_lut1_v0002_tag {
    struct _isp_nbpc_lut1_v0002_map {
        volatile unsigned int intercept_b1      :10;
        volatile unsigned int slope_k1           :10;
        volatile unsigned int lut_level1          :10;
        volatile unsigned int reserved           :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_nbpc_lut2_v0002_tag {
    struct _isp_nbpc_lut2_v0002_map {
        volatile unsigned int intercept_b2      :10;
        volatile unsigned int slope_k2           :10;
        volatile unsigned int lut_level2          :10;
        volatile unsigned int reserved           :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_nbpc_lut3_v0002_tag {
    struct _isp_nbpc_lut3_v0003_map {
        volatile unsigned int intercept_b3      :10;
        volatile unsigned int slope_k3           :10;
        volatile unsigned int lut_level3          :10;
        volatile unsigned int reserved           :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_nbpc_lut4_v0002_tag {
    struct _isp_nbpc_lut4_v0002_map {
        volatile unsigned int intercept_b4      :10;
        volatile unsigned int slope_k4           :10;
        volatile unsigned int lut_level4          :10;
        volatile unsigned int reserved           :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_nbpc_lut5_v0002_tag {
    struct _isp_nbpc_lut5_v0002_map {
        volatile unsigned int intercept_b5      :10;
        volatile unsigned int slope_k5           :10;
        volatile unsigned int lut_level5          :10;
        volatile unsigned int reserved           :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_nbpc_lut6_v0002_tag {
    struct _isp_nbpc_lut6_v0002_map {
        volatile unsigned int intercept_b6      :10;
        volatile unsigned int slope_k6           :10;
        volatile unsigned int lut_level6          :10;
        volatile unsigned int reserved           :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_nbpc_lut7_v0002_tag {
    struct _isp_nbpc_lut7_v0002_map {
        volatile unsigned int intercept_b7      :10;
        volatile unsigned int slope_k7           :10;
        volatile unsigned int lut_level7          :10;
        volatile unsigned int reserved           :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bpc_new_old_sel_tag {
    struct _isp_bpc_new_old_sel_map {
        volatile unsigned int new_old_sel     :1;
	 volatile unsigned int map_down_sel  :1;
	 volatile unsigned int reserved           :30;
    }mBits ;
    volatile unsigned int dwValue ;
};
union _isp_nbpc_map_addr_tag {
    struct _isp_nbpc_map_addr_map {
        volatile unsigned int map_addr      :32;
    }mBits ;
    volatile unsigned int dwValue ;
};
// PRE wavelet
union _isp_pre_wave_status_v0002_tag {
    struct _isp_pre_wave_status_v0002_map {
        volatile unsigned int status        :32; // wave denoise status read only
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_pre_wave_param_v0002_tag {
    struct _isp_pre_wave_param_v0002_map {
        volatile unsigned int bypass        :1;
        volatile unsigned int reserved0    :7;
        volatile unsigned int thrs0           :8;
        volatile unsigned int thrs1           :8;
        volatile unsigned int reserved1    :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

// wave denoise
union _isp_wave_status_tag {
    struct _isp_wave_status_map {
        volatile unsigned int status        :32; // wave denoise status read only
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_wave_param_tag {
    struct _isp_wave_param_map {
        volatile unsigned int bypass        :1;
        volatile unsigned int write_back    :2; // 0:no write back 1:write back 0:adaptive write back
        volatile unsigned int reserved      :29;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_wave_thrd_v0000_tag {
    struct _isp_wave_thrd_v0000_map {
        volatile unsigned int r_thr         :10;
        volatile unsigned int g_thr         :10;
        volatile unsigned int b_thr         :10;
        volatile unsigned int reserved      :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_wave_slice_size_tag {
    struct _isp_wave_slice_size_map {
        volatile unsigned int slice_width   :16;
        volatile unsigned int slice_height  :16;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_wave_slice_info_tag {
    struct _isp_wave_slice_info_map {
        volatile unsigned int edge_info     :4;
        volatile unsigned int reserved      :28;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_wave_diswei_0_tag {
    struct _isp_wave_diswei_0_map {
        volatile unsigned int disweight3_0  :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_wave_diswei_1_tag {
    struct _isp_wave_diswei_1_map {
        volatile unsigned int disweight7_4  :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_wave_diswei_2_tag {
    struct _isp_wave_diswei_2_map {
        volatile unsigned int disweight11_8 :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_wave_diswei_3_tag {
    struct _isp_wave_diswei_3_map {
        volatile unsigned int disweight15_12 :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_wave_diswei_4_tag {
    struct _isp_wave_diswei_4_map {
        volatile unsigned int disweight18_16 :24;
        volatile unsigned int reserved       :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_wave_ranwei_0_tag {
    struct _isp_wave_ranwei_0_map {
        volatile unsigned int ranweight3_0   :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_wave_ranwei_1_tag {
    struct _isp_wave_ranwei_1_map {
        volatile unsigned int ranweight7_4   :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_wave_ranwei_2_tag {
    struct _isp_wave_ranwei_2_map {
        volatile unsigned int ranweight11_8  :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_wave_ranwei_3_tag {
    struct _isp_wave_ranwei_3_map {
        volatile unsigned int ranweight15_12 :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_wave_ranwei_4_tag {
    struct _isp_wave_ranwei_4_map {
        volatile unsigned int ranweight19_16 :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_wave_ranwei_5_tag {
    struct _isp_wave_ranwei_5_map {
        volatile unsigned int ranweight23_20 :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_wave_ranwei_6_tag {
    struct _isp_wave_ranwei_6_map {
        volatile unsigned int ranweight27_24 :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_wave_ranwei_7_tag {
    struct _isp_wave_ranwei_7_map {
        volatile unsigned int ranweight30_28 :24;
        volatile unsigned int reserved       :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

// GrGb C
union _isp_grgb_status_tag {
    struct _isp_grgb_status_map {
        volatile unsigned int status         :32; // grgbc status read only
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_grgb_param_tag {
    struct _isp_grgb_param_map {
        volatile unsigned int bypass         :1;
        volatile unsigned int edge_thr       :6;
        volatile unsigned int diff_thr       :10;
        volatile unsigned int reserved       :15;
    }mBits ;
    volatile unsigned int dwValue ;
};

// CFA
union _isp_cfa_status_tag {
    struct _isp_cfa_status_map {
        volatile unsigned int status         :32; // cfa status read only
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_cfa_param_tag {
    struct _isp_cfa_param_map {
        volatile unsigned int reserved0      :1;
        volatile unsigned int edge_thr       :6;
        volatile unsigned int diff_thr       :2;
        volatile unsigned int reserved1      :23;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_cfa_slice_size_tag {
    struct _isp_cfa_slice_size_map {
        volatile unsigned int slice_width      :16;
        volatile unsigned int slice_height     :16;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_cfa_slice_info_tag {
    struct _isp_cfa_slice_info_map {
        volatile unsigned int edge_info      :4;
        volatile unsigned int reserved       :28;
    }mBits ;
    volatile unsigned int dwValue ;
};

// CMC
union _isp_cmc_status_tag {
    struct _isp_cmc_status_map {
        volatile unsigned int status         :32; // cmc status read only
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_cmc_param_tag {
    struct _isp_cmc_param_map {
        volatile unsigned int bypass         :1;
        volatile unsigned int reserved       :31;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_cmc_matrix0_tag {
    struct _isp_cmc_matrix0_map {
        volatile unsigned int matrix00       :14;
        volatile unsigned int matrix01       :14;
        volatile unsigned int reserved       :4;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_cmc_matrix1_tag {
    struct _isp_cmc_matrix1_map {
        volatile unsigned int matrix02       :14;
        volatile unsigned int matrix10       :14;
        volatile unsigned int reserved       :4;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_cmc_matrix2_tag {
    struct _isp_cmc_matrix2_map {
        volatile unsigned int matrix11       :14;
        volatile unsigned int matrix12       :14;
        volatile unsigned int reserved       :4;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_cmc_matrix3_tag {
    struct _isp_cmc_matrix3_map {
        volatile unsigned int matrix20       :14;
        volatile unsigned int matrix21       :14;
        volatile unsigned int reserved       :4;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_cmc_matrix4_tag {
    struct _isp_cmc_matrix4_map {
        volatile unsigned int matrix22       :14;
        volatile unsigned int reserved       :18;
    }mBits ;
    volatile unsigned int dwValue ;
};

// Gamma Correction
union _isp_gamma_status_tag {
    struct _isp_gamma_status_map {
        volatile unsigned int status         :32; // gamma status read only
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_param_tag {
    struct _isp_gamma_param_map {
        volatile unsigned int bypass         :1;
        volatile unsigned int reserved       :31;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_x0_tag {
    struct _isp_gamma_node_x0_map {
        volatile unsigned int node3          :10;
        volatile unsigned int node2          :10;
        volatile unsigned int node1          :10;
        volatile unsigned int reserved       :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_x1_tag {
    struct _isp_gamma_node_x1_map {
        volatile unsigned int node6          :10;
        volatile unsigned int node5          :10;
        volatile unsigned int node4          :10;
        volatile unsigned int reserved       :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_x2_tag {
    struct _isp_gamma_node_x2_map {
        volatile unsigned int node9          :10;
        volatile unsigned int node8          :10;
        volatile unsigned int node7          :10;
        volatile unsigned int reserved       :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_x3_tag {
    struct _isp_gamma_node_x3_map {
        volatile unsigned int node12         :10;
        volatile unsigned int node11         :10;
        volatile unsigned int node10         :10;
        volatile unsigned int reserved       :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_x4_tag {
    struct _isp_gamma_node_x4_map {
        volatile unsigned int node15         :10;
        volatile unsigned int node14         :10;
        volatile unsigned int node13         :10;
        volatile unsigned int reserved       :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_x5_tag {
    struct _isp_gamma_node_x5_map {
        volatile unsigned int node18         :10;
        volatile unsigned int node17         :10;
        volatile unsigned int node16         :10;
        volatile unsigned int reserved       :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_x6_tag {
    struct _isp_gamma_node_x6_map {
        volatile unsigned int node21         :10;
        volatile unsigned int node20         :10;
        volatile unsigned int node19         :10;
        volatile unsigned int reserved       :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_x7_tag {
    struct _isp_gamma_node_x7_map {
        volatile unsigned int node24        :10;
        volatile unsigned int node23        :10;
        volatile unsigned int node22        :10;
        volatile unsigned int reserved      :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_y0_tag {
    struct _isp_gamma_node_y0_map {
        volatile unsigned int node3         :8;
        volatile unsigned int node2         :8;
        volatile unsigned int node1         :8;
        volatile unsigned int node0         :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_y1_tag {
    struct _isp_gamma_node_y1_map {
        volatile unsigned int node7         :8;
        volatile unsigned int node6         :8;
        volatile unsigned int node5         :8;
        volatile unsigned int node4         :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_y2_tag {
    struct _isp_gamma_node_y2_map {
        volatile unsigned int node11        :8;
        volatile unsigned int node10        :8;
        volatile unsigned int node9         :8;
        volatile unsigned int node8         :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_y3_tag {
    struct _isp_gamma_node_y3_map {
        volatile unsigned int node15        :8;
        volatile unsigned int node14        :8;
        volatile unsigned int node13        :8;
        volatile unsigned int node12        :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_y4_tag {
    struct _isp_gamma_node_y4_map {
        volatile unsigned int node19        :8;
        volatile unsigned int node18        :8;
        volatile unsigned int node17        :8;
        volatile unsigned int node16        :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_y5_tag {
    struct _isp_gamma_node_y5_map {
        volatile unsigned int node23        :8;
        volatile unsigned int node22        :8;
        volatile unsigned int node21        :8;
        volatile unsigned int node20        :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_y6_tag {
    struct _isp_gamma_node_y6_map {
        volatile unsigned int reserved      :16;
        volatile unsigned int node25        :8;
        volatile unsigned int node24        :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_r0_v0001_tag {
    struct _isp_gamma_node_r0_v0001_map {
        volatile unsigned int node3         :8;
        volatile unsigned int node2         :8;
        volatile unsigned int node1         :8;
        volatile unsigned int node0         :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_r1_v0001_tag {
    struct _isp_gamma_node_r1_v0001_map {
        volatile unsigned int node7         :8;
        volatile unsigned int node6         :8;
        volatile unsigned int node5         :8;
        volatile unsigned int node4         :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_r2_v0001_tag {
    struct _isp_gamma_node_r2_v0001_map {
        volatile unsigned int node11        :8;
        volatile unsigned int node10        :8;
        volatile unsigned int node9         :8;
        volatile unsigned int node8         :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_r3_v0001_tag {
    struct _isp_gamma_node_r3_v0001_map {
        volatile unsigned int node15        :8;
        volatile unsigned int node14        :8;
        volatile unsigned int node13        :8;
        volatile unsigned int node12        :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_r4_v0001_tag {
    struct _isp_gamma_node_r4_v0001_map {
        volatile unsigned int node19        :8;
        volatile unsigned int node18        :8;
        volatile unsigned int node17        :8;
        volatile unsigned int node16        :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_r5_v0001_tag {
    struct _isp_gamma_node_r5_v0001_map {
        volatile unsigned int node23        :8;
        volatile unsigned int node22        :8;
        volatile unsigned int node21        :8;
        volatile unsigned int node20        :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_r6_v0001_tag {
    struct _isp_gamma_node_r6_v0001_map {
        volatile unsigned int reserved      :16;
        volatile unsigned int node25        :8;
        volatile unsigned int node24        :8;
    }mBits ;
    volatile unsigned int dwValue ;
};


union _isp_gamma_node_g0_v0001_tag {
    struct _isp_gamma_node_g0_v0001_map {
        volatile unsigned int node3         :8;
        volatile unsigned int node2         :8;
        volatile unsigned int node1         :8;
        volatile unsigned int node0         :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_g1_v0001_tag {
    struct _isp_gamma_node_g1_v0001_map {
        volatile unsigned int node7         :8;
        volatile unsigned int node6         :8;
        volatile unsigned int node5         :8;
        volatile unsigned int node4         :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_g2_v0001_tag {
    struct _isp_gamma_node_g2_v0001_map {
        volatile unsigned int node11        :8;
        volatile unsigned int node10        :8;
        volatile unsigned int node9         :8;
        volatile unsigned int node8         :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_g3_v0001_tag {
    struct _isp_gamma_node_g3_v0001_map {
        volatile unsigned int node15        :8;
        volatile unsigned int node14        :8;
        volatile unsigned int node13        :8;
        volatile unsigned int node12        :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_g4_v0001_tag {
    struct _isp_gamma_node_g4_v0001_map {
        volatile unsigned int node19        :8;
        volatile unsigned int node18        :8;
        volatile unsigned int node17        :8;
        volatile unsigned int node16        :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_g5_v0001_tag {
    struct _isp_gamma_node_g5_v0001_map {
        volatile unsigned int node23        :8;
        volatile unsigned int node22        :8;
        volatile unsigned int node21        :8;
        volatile unsigned int node20        :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_g6_v0001_tag {
    struct _isp_gamma_node_g6_v0001_map {
        volatile unsigned int reserved      :16;
        volatile unsigned int node25        :8;
        volatile unsigned int node24        :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_b0_v0001_tag {
    struct _isp_gamma_node_b0_v0001_map {
        volatile unsigned int node3         :8;
        volatile unsigned int node2         :8;
        volatile unsigned int node1         :8;
        volatile unsigned int node0         :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_b1_v0001_tag {
    struct _isp_gamma_node_b1_v0001_map {
        volatile unsigned int node7         :8;
        volatile unsigned int node6         :8;
        volatile unsigned int node5         :8;
        volatile unsigned int node4         :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_b2_v0001_tag {
    struct _isp_gamma_node_b2_v0001_map {
        volatile unsigned int node11        :8;
        volatile unsigned int node10        :8;
        volatile unsigned int node9         :8;
        volatile unsigned int node8         :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_b3_v0001_tag {
    struct _isp_gamma_node_b3_v0001_map {
        volatile unsigned int node15        :8;
        volatile unsigned int node14        :8;
        volatile unsigned int node13        :8;
        volatile unsigned int node12        :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_b4_v0001_tag {
    struct _isp_gamma_node_b4_v0001_map {
        volatile unsigned int node19        :8;
        volatile unsigned int node18        :8;
        volatile unsigned int node17        :8;
        volatile unsigned int node16        :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_b5_v0001_tag {
    struct _isp_gamma_node_b5_v0001_map {
        volatile unsigned int node23        :8;
        volatile unsigned int node22        :8;
        volatile unsigned int node21        :8;
        volatile unsigned int node20        :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_b6_v0001_tag {
    struct _isp_gamma_node_b6_v0001_map {
        volatile unsigned int reserved      :16;
        volatile unsigned int node25        :8;
        volatile unsigned int node24        :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_idx0_v0001_tag {
    struct _isp_gamma_node_idx0_v0001_map {
        volatile unsigned int index8        :4;
        volatile unsigned int index7        :4;
        volatile unsigned int index6        :4;
        volatile unsigned int index5        :4;
        volatile unsigned int index4        :4;
        volatile unsigned int index3        :4;
        volatile unsigned int index2        :4;
        volatile unsigned int index1        :4;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_idx1_v0001_tag {
    struct _isp_gamma_node_idx1_v0001_map {
        volatile unsigned int index16       :4;
        volatile unsigned int index15       :4;
        volatile unsigned int index14       :4;
        volatile unsigned int index13       :4;
        volatile unsigned int index12       :4;
        volatile unsigned int index11       :4;
        volatile unsigned int index10       :4;
        volatile unsigned int index9        :4;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_idx2_v0001_tag {
    struct _isp_gamma_node_idx2_v0001_map {
        volatile unsigned int index24       :4;
        volatile unsigned int index23       :4;
        volatile unsigned int index22       :4;
        volatile unsigned int index21       :4;
        volatile unsigned int index20       :4;
        volatile unsigned int index19       :4;
        volatile unsigned int index18       :4;
        volatile unsigned int index17       :4;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_idx3_v0001_tag {
    struct _isp_gamma_node_idx3_v0001_map {
        volatile unsigned int reserved      :28;
        volatile unsigned int index25       :4;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_x0_v0002_tag {
    struct _isp_gamma_node_x0_v0002_map {
        volatile unsigned int node2         :10;
        volatile unsigned int node1         :10;
        volatile unsigned int node0         :10;
        volatile unsigned int reserved      :2;
	}mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_x1_v0002_tag {
    struct _isp_gamma_node_x1_v0002_map {
        volatile unsigned int node5         :10;
        volatile unsigned int node4         :10;
        volatile unsigned int node3         :10;
        volatile unsigned int reserved      :2;
	}mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_x2_v0002_tag {
    struct _isp_gamma_node_x2_v0002_map {
        volatile unsigned int node8         :10;
        volatile unsigned int node7         :10;
        volatile unsigned int node6         :10;
        volatile unsigned int reserved      :2;
	}mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_x3_v0002_tag {
    struct _isp_gamma_node_x3_v0002_map {
        volatile unsigned int node11         :10;
        volatile unsigned int node10         :10;
        volatile unsigned int node9         :10;
        volatile unsigned int reserved      :2;
	}mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_x4_v0002_tag {
    struct _isp_gamma_node_x4_v0002_map {
        volatile unsigned int node14         :10;
        volatile unsigned int node13         :10;
        volatile unsigned int node12         :10;
        volatile unsigned int reserved      :2;
	}mBits ;
    volatile unsigned int dwValue ;
};

union _isp_gamma_node_x5_v0002_tag {
    struct _isp_gamma_node_x5_v0002_map {
        volatile unsigned int reserve1       :20;
        volatile unsigned int node15         :10;
        volatile unsigned int reserved       :2;
	}mBits ;
    volatile unsigned int dwValue ;
};


union _isp_gamma_node_y_v0002_tag {
    struct _isp_gamma_node_y_v0002_map {
        volatile unsigned int b         :16;
        volatile unsigned int k         :16;
    }mBits ;
    volatile unsigned int dwValue ;
};

// CCE
union _isp_cce_status_tag {
    struct _isp_cce_status_map {
        volatile unsigned int status        :32; // cce status read only
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_cce_param_tag {
    struct _isp_cce_param_map {
        volatile unsigned int reserved0     :1;
        volatile unsigned int bypass        :1;
        volatile unsigned int reserved1     :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_cce_matrix0_tag {
    struct _isp_cce_matrix0_map {
        volatile unsigned int matrix00      :11;
        volatile unsigned int matrix01      :11;
        volatile unsigned int reserved      :10;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_cce_matrix1_tag {
    struct _isp_cce_matrix1_map {
        volatile unsigned int matrix02      :11;
        volatile unsigned int matrix10      :11;
        volatile unsigned int reserved      :10;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_cce_matrix2_tag {
    struct _isp_cce_matrix2_map {
        volatile unsigned int matrix11      :11;
        volatile unsigned int matrix12      :11;
        volatile unsigned int reserved      :10;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_cce_matrix3_tag {
    struct _isp_cce_matrix3_map {
        volatile unsigned int matrix20      :11;
        volatile unsigned int matrix21      :11;
        volatile unsigned int reserved      :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_cce_matrix4_tag {
    struct _isp_cce_matrix4_map {
        volatile unsigned int matrix22      :11;
        volatile unsigned int reserved      :21;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_cce_shift_tag {
    struct _isp_cce_shift_map {
        volatile unsigned int y_shift       :9;
        volatile unsigned int u_shift       :9;
        volatile unsigned int v_shift       :9;
        volatile unsigned int reserved      :5;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_cce_uvd_thrd0_tag {
    struct _isp_cce_uvd_thrd0_map {
        volatile unsigned int thrd_0        :8;
        volatile unsigned int thrd_1        :8;
        volatile unsigned int thrd_2        :8;
        volatile unsigned int thrd_3        :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_cce_uvd_thrd1_tag {
    struct _isp_cce_uvd_thrd1_map {
        volatile unsigned int thrd_4        :8;
        volatile unsigned int thrd_5        :8;
        volatile unsigned int thrd_6        :8;
        volatile unsigned int reserved      :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_cce_uvc_param0_v0001_tag {
    struct _isp_cce_uvc_param0_v0001_map {
        volatile unsigned int uv_t2         :8;
        volatile unsigned int uv_t1         :8;
        volatile unsigned int reserved      :16;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_cce_uvc_param1_v0001_tag {
    struct _isp_cce_uvc_param1_v0001_map {
        volatile unsigned int uv_m2         :8;
        volatile unsigned int uv_m1         :8;
        volatile unsigned int uv_m          :8;
        volatile unsigned int reserved      :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

// PREF
union _isp_pref_status_tag {
    struct _isp_pref_status_map {
        volatile unsigned int status        :32; // pref read only
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_pref_param_tag {
    struct _isp_pref_param_map {
        volatile unsigned int bypass        :1;
        volatile unsigned int write_back    :2; // 0: no 1: write back
        volatile unsigned int reserved      :29;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_pref_thrd_tag {
    struct _isp_pref_thrd_map {
        volatile unsigned int y_thrd        :8;
        volatile unsigned int u_thrd        :8;
        volatile unsigned int v_thrd        :8;
        volatile unsigned int reserved      :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_pref_slice_size_tag {
    struct _isp_pref_slice_size_map {
        volatile unsigned int slice_width   :16;
        volatile unsigned int slice_height  :16;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_pref_slice_info_tag {
    struct _isp_pref_slice_info_map {
        volatile unsigned int reserved0     :1;
        volatile unsigned int edge_left_info :1;
        volatile unsigned int reserved1     :1;
        volatile unsigned int edge_top_info :1;
        volatile unsigned int reserved2     :28;
    }mBits ;
    volatile unsigned int dwValue ;
};

// BRIGHT
union _isp_bright_status_tag {
    struct _isp_bright_status_map {
        volatile unsigned int status        :32; // bright read only
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bright_param_tag {
    struct _isp_bright_param_map {
        volatile unsigned int bypass        :1;
        volatile unsigned int bright_factor :8;
        volatile unsigned int reserved      :23;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bright_slice_size_tag {
    struct _isp_bright_slice_size_map {
        volatile unsigned int slice_width     :16;
        volatile unsigned int slice_height    :16;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_bright_slice_info_tag {
    struct _isp_bright_slice_info_map {
        volatile unsigned int edge_info     :4;
        volatile unsigned int reserved2     :28;
    }mBits ;
    volatile unsigned int dwValue ;
};

// CONTRAST
union _isp_contrast_status_tag {
    struct _isp_contrast_status_map {
        volatile unsigned int status        :32; // contrast read only
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_contrast_param_tag {
    struct _isp_contrast_param_map {
        volatile unsigned int bypass          :1;
        volatile unsigned int contrast_factor :8;
        volatile unsigned int reserved        :23;
    }mBits ;
    volatile unsigned int dwValue ;
};

// HIST
union _isp_hist_status_tag {
    struct _isp_hist_status_map {
        volatile unsigned int status        :32; // hist read only
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_hist_param_tag {
    struct _isp_hist_param_map {
        volatile unsigned int bypass        :1;
        volatile unsigned int auto_rst_off  :1;
        volatile unsigned int mode          :1;
        volatile unsigned int reserved      :29;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_hist_ratio_tag {
    struct _isp_hist_ratio_map {
        volatile unsigned int low_sum_ratio  :16;
        volatile unsigned int high_sum_ratio :16;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_hist_max_min_tag {
    struct _isp_hist_max_min_map {
        volatile unsigned int in_min        :8;
        volatile unsigned int in_max        :8;
        volatile unsigned int out_min       :8;
        volatile unsigned int out_max       :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_hist_clear_enable_tag {
    struct _isp_hist_clear_enable_map {
        volatile unsigned int clear_eb      :1;
        volatile unsigned int reserved      :31;
    }mBits ;
    volatile unsigned int dwValue ;
};

// AUTO CONTRAST
union _isp_auto_contrast_status_tag {
    struct _isp_auto_contrast_status_map {
        volatile unsigned int status        :32; // auto contrast only
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_auto_contrast_max_min_status_v0002_tag {
    struct _isp_auto_contrast_max_min_status_v0002_map {
        volatile unsigned int in_min        :8;
        volatile unsigned int in_max       :8;
        volatile unsigned int out_min      :8;
        volatile unsigned int out_max      :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_auto_contrast_param_tag {
    struct _isp_auto_contrast_param_map {
        volatile unsigned int bypass        :1;
        volatile unsigned int mode          :1; // 0:firmware 1:hardware
        volatile unsigned int reserved      :30;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_auto_contrast_max_min_tag {
    struct _isp_auto_contrast_max_min_map {
        volatile unsigned int in_min        :8;
        volatile unsigned int in_max        :8;
        volatile unsigned int out_min       :8;
        volatile unsigned int out_max       :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_auto_contrast_adjust_v0002_tag {
    struct _isp_auto_contrast_adjust_v0002_map {
        volatile unsigned int diff_thr        :8;
        volatile unsigned int small_adj    :8;
        volatile unsigned int big_adj        :8;
        volatile unsigned int reserved      :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

// AFM
union _isp_afm_status_tag {
    struct _isp_afm_status_map {
        volatile unsigned int status        :32; // afm status read only
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_param_tag {
    struct _isp_afm_param_map {
        volatile unsigned int bypass        :1;
        volatile unsigned int shift         :5;
        volatile unsigned int mode          :1;
        volatile unsigned int skip_num      :4;
        volatile unsigned int skip_clear    :1;
        volatile unsigned int reserved      :20;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_win_range0_tag {
    struct _isp_afm_win_range0_map {
        volatile unsigned int win0_start_x  :12;
        volatile unsigned int reserved0      :4;
        volatile unsigned int win0_start_y  :12;
        volatile unsigned int reserved1      :4;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_win_range1_tag {
    struct _isp_afm_win_range1_map {
        volatile unsigned int win0_end_x    :12;
        volatile unsigned int reserved0       :4;
        volatile unsigned int win0_end_y    :12;
        volatile unsigned int reserved1       :4;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_win_range2_tag {
    struct _isp_afm_win_range2_map {
        volatile unsigned int win1_start_x  :12;
        volatile unsigned int reserved0      :4;
        volatile unsigned int win1_start_y  :12;
        volatile unsigned int reserved1      :4;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_win_range3_tag {
    struct _isp_afm_win_range3_map {
        volatile unsigned int win1_end_x    :12;
        volatile unsigned int reserved0       :4;
        volatile unsigned int win1_end_y    :12;
        volatile unsigned int reserved1       :4;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_win_range4_tag {
    struct _isp_afm_win_range4_map {
        volatile unsigned int win2_start_x  :12;
        volatile unsigned int reserved0       :4;
        volatile unsigned int win2_start_y  :12;
        volatile unsigned int reserved1      :4;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_win_range5_tag {
    struct _isp_afm_win_range5_map {
        volatile unsigned int win2_end_x    :12;
        volatile unsigned int reserved0       :4;
        volatile unsigned int win2_end_y    :12;
        volatile unsigned int reserved1      :4;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_win_range6_tag {
    struct _isp_afm_win_range6_map {
        volatile unsigned int win3_start_x  :12;
        volatile unsigned int reserved0       :4;
        volatile unsigned int win3_start_y  :12;
        volatile unsigned int reserved1      :4;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_win_range7_tag {
    struct _isp_afm_win_range7_map {
        volatile unsigned int win3_end_x    :12;
        volatile unsigned int reserved0       :4;
        volatile unsigned int win3_end_y    :12;
        volatile unsigned int reserved1      :4;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_win_range8_tag {
    struct _isp_afm_win_range8_map {
        volatile unsigned int win4_start_x  :12;
        volatile unsigned int reserved0       :4;
        volatile unsigned int win4_start_y  :12;
        volatile unsigned int reserved1      :4;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_win_range9_tag {
    struct _isp_afm_win_range9_map {
        volatile unsigned int win4_end_x    :12;
        volatile unsigned int reserved0       :4;
        volatile unsigned int win4_end_y    :12;
        volatile unsigned int reserved1      :4;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_win_range10_tag {
    struct _isp_afm_win_range10_map {
        volatile unsigned int win5_start_x  :12;
        volatile unsigned int reserved0       :4;
        volatile unsigned int win5_start_y  :12;
        volatile unsigned int reserved1      :4;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_win_range11_tag {
    struct _isp_afm_win_range11_map {
        volatile unsigned int win5_end_x    :12;
        volatile unsigned int reserved0       :4;
        volatile unsigned int win5_end_y    :12;
        volatile unsigned int reserved1      :4;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_win_range12_tag {
    struct _isp_afm_win_range12_map {
        volatile unsigned int win6_start_x  :12;
        volatile unsigned int reserved0       :4;
        volatile unsigned int win6_start_y  :12;
        volatile unsigned int reserved1      :4;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_win_range13_tag {
    struct _isp_afm_win_range13_map {
        volatile unsigned int win6_end_x    :12;
        volatile unsigned int reserved0       :4;
        volatile unsigned int win6_end_y    :12;
        volatile unsigned int reserved1      :4;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_win_range14_tag {
    struct _isp_afm_win_range14_map {
        volatile unsigned int win7_start_x  :12;
        volatile unsigned int reserved0       :4;
        volatile unsigned int win7_start_y  :12;
        volatile unsigned int reserved1      :4;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_win_range15_tag {
    struct _isp_afm_win_range15_map {
        volatile unsigned int win7_end_x    :12;
        volatile unsigned int reserved0       :4;
        volatile unsigned int win7_end_y    :12;
        volatile unsigned int reserved1      :4;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_win_range16_tag {
    struct _isp_afm_win_range16_map {
        volatile unsigned int win8_start_x  :12;
        volatile unsigned int reserved0       :4;
        volatile unsigned int win8_start_y  :12;
        volatile unsigned int reserved1      :4;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_win_range17_tag {
    struct _isp_afm_win_range17_map {
        volatile unsigned int win8_end_x    :12;
        volatile unsigned int reserved0       :4;
        volatile unsigned int win8_end_y    :12;
        volatile unsigned int reserved1      :4;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_statistic0_tag {
    struct _isp_afm_statistic0_map {
        volatile unsigned int statistic0    :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_statistic1_tag {
    struct _isp_afm_statistic1_map {
        volatile unsigned int statistic1    :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_statistic2_tag {
    struct _isp_afm_statistic2_map {
        volatile unsigned int statistic2    :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_statistic3_tag {
    struct _isp_afm_statistic3_map {
        volatile unsigned int statistic3    :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_statistic4_tag {
    struct _isp_afm_statistic4_map {
        volatile unsigned int statistic4    :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_statistic5_tag {
    struct _isp_afm_statistic5_map {
        volatile unsigned int statistic5    :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_statistic6_tag {
    struct _isp_afm_statistic6_map {
        volatile unsigned int statistic6    :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_statistic7_tag {
    struct _isp_afm_statistic7_map {
        volatile unsigned int statistic7    :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_statistic8_tag {
    struct _isp_afm_statistic8_map {
        volatile unsigned int statistic8    :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_statistic0_l_v0001_tag {
    struct _isp_afm_statistic0_l_v0001_map {
        volatile unsigned int statistic0l    :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_statistic0_h_v0001_tag {
    struct _isp_afm_statistic0_h_v0001_map {
        volatile unsigned int statistic0h    :1;
        volatile unsigned int reserved       :31;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_statistic1_l_v0001_tag {
    struct _isp_afm_statistic1_l_v0001_map {
        volatile unsigned int statistic0l    :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_statistic1_h_v0001_tag {
    struct _isp_afm_statistic1_h_v0001_map {
        volatile unsigned int statistic0h    :1;
        volatile unsigned int reserved       :31;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_statistic2_l_v0001_tag {
    struct _isp_afm_statistic2_l_v0001_map {
        volatile unsigned int statistic0l    :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_statistic2_h_v0001_tag {
    struct _isp_afm_statisti2_h_v0001_map {
        volatile unsigned int statistic0h    :1;
        volatile unsigned int reserved       :31;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_statistic3_l_v0001_tag {
    struct _isp_afm_statistic3_l_v0001_map {
        volatile unsigned int statistic0l    :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_statistic3_h_v0001_tag {
    struct _isp_afm_statistic3_h_v0001_map {
        volatile unsigned int statistic0h    :1;
        volatile unsigned int reserved       :31;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_statistic4_l_v0001_tag {
    struct _isp_afm_statistic4_l_v0001_map {
        volatile unsigned int statistic0l    :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_statistic4_h_v0001_tag {
    struct _isp_afm_statistic4_h_v0001_map {
        volatile unsigned int statistic0h    :1;
        volatile unsigned int reserved       :31;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_statistic5_l_v0001_tag {
    struct _isp_afm_statistic5_l_v0001_map {
        volatile unsigned int statistic0l    :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_statistic5_h_v0001_tag {
    struct _isp_afm_statistic5_h_v0001_map {
        volatile unsigned int statistic0h    :1;
        volatile unsigned int reserved       :31;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_statistic6_l_v0001_tag {
    struct _isp_afm_statistic6_l_v0001_map {
        volatile unsigned int statistic0l    :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_statistic6_h_v0001_tag {
    struct _isp_afm_statistic6_h_v0001_map {
        volatile unsigned int statistic0h    :1;
        volatile unsigned int reserved       :31;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_statistic7_l_v0001_tag {
    struct _isp_afm_statistic7_l_v0001_map {
        volatile unsigned int statistic0l    :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_statistic7_h_v0001_tag {
    struct _isp_afm_statistic7_h_v0001_map {
        volatile unsigned int statistic0h    :1;
        volatile unsigned int reserved       :31;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_statistic8_l_v0001_tag {
    struct _isp_afm_statistic8_l_v0001_map {
        volatile unsigned int statistic0l    :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_afm_statistic8_h_v0001_tag {
    struct _isp_afm_statistic8_h_v0001_map {
        volatile unsigned int statistic0h    :1;
        volatile unsigned int reserved       :31;
    }mBits ;
    volatile unsigned int dwValue ;
};

// EE
union _isp_ee_status_tag {
    struct _isp_ee_status_map {
        volatile unsigned int status        :32; // edge enhancement status read only
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_ee_param_tag {
    struct _isp_ee_param_map {
        volatile unsigned int bypass        :1;
        volatile unsigned int detail_th     :8;
        volatile unsigned int smooth_th     :8;
        volatile unsigned int strength      :6;
        volatile unsigned int reserved      :9;
    }mBits ;
    volatile unsigned int dwValue ;
};

// EMBOSS
union _isp_emboss_status_tag {
    struct _isp_emboss_status_map {
        volatile unsigned int status       :32; // emboss status read only
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_emboss_param_tag {
    struct _isp_emboss_param_map {
        volatile unsigned int bypass       :1;
        volatile unsigned int step         :6;
        volatile unsigned int reserved     :25;
    }mBits ;
    volatile unsigned int dwValue ;
};

// FCS
union _isp_fcs_status_tag {
    struct _isp_fcs_status_map {
        volatile unsigned int status       :32; // fcs status read only
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_fcs_param_tag {
    struct _isp_fcs_param_map {
        volatile unsigned int bypass       :1;
        volatile unsigned int reserved0     :15;
        volatile unsigned int mode           :1;
        volatile unsigned int reserved     :15;
    }mBits ;
    volatile unsigned int dwValue ;
};

// CSS
union _isp_css_status_tag {
    struct _isp_css_status_map {
        volatile unsigned int status       :32; // css status read only
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_css_param_tag {
    struct _isp_css_param_map {
        volatile unsigned int bypass      :1;
        volatile unsigned int reserved    :31;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_css_thrd0_tag {
    struct _isp_css_thrd0_map {
        volatile unsigned int th_3        :8;
        volatile unsigned int th_2        :8;
        volatile unsigned int th_1        :8;
        volatile unsigned int th_0        :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_css_thrd1_tag {
    struct _isp_css_thrd1_map {
        volatile unsigned int luma_th     :8;
        volatile unsigned int th_6        :8;
        volatile unsigned int th_5        :8;
        volatile unsigned int th_4        :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_css_thrd2_tag {
    struct _isp_css_thrd2_map {
        volatile unsigned int th_3        :8;
        volatile unsigned int th_2        :8;
        volatile unsigned int th_1        :8;
        volatile unsigned int th_0        :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_css_thrd3_tag {
    struct _isp_css_thrd3_map {
        volatile unsigned int chrom_th    :8;
        volatile unsigned int th_6        :8;
        volatile unsigned int th_5        :8;
        volatile unsigned int th_4        :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_css_slice_size_tag {
    struct _isp_css_slice_size_map {
        volatile unsigned int slice_width   :16;
        volatile unsigned int slice_height  :16;
    }mBits ;
    volatile unsigned int dwValue ;
};
union _isp_css_ratio_tag {
    struct _isp_css_ratio_map {
        volatile unsigned int ratio_7   :4;
        volatile unsigned int ratio_6   :4;
        volatile unsigned int ratio_5   :4;
        volatile unsigned int ratio_4   :4;
        volatile unsigned int ratio_3   :4;
        volatile unsigned int ratio_2   :4;
        volatile unsigned int ratio_1   :4;
        volatile unsigned int ratio_0   :4;
    }mBits ;
    volatile unsigned int dwValue ;
};
// CSA
union _isp_csa_status_tag {
    struct _isp_csa_status_map {
        volatile unsigned int status      :32; // csa status read only
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_csa_param_tag {
    struct _isp_csa_param_map {
        volatile unsigned int bypass      :1;
        volatile unsigned int factor      :8;
        volatile unsigned int reserved    :23;
    }mBits ;
    volatile unsigned int dwValue ;
};

// store
union _isp_store_status_tag {
    struct _isp_store_status_map {
        volatile unsigned int status      :32; // store status read only
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_store_status_preview_tag {
    struct _isp_store_status_preview_map {
        volatile unsigned int sof_counter         :8;
        volatile unsigned int eof_counter         :8;
        volatile unsigned int fetch_hw_status     :16;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_store_param_tag {
    struct _isp_store_param_map {
        volatile unsigned int bypass      :1;
        volatile unsigned int reserved0   :4;
        volatile unsigned int sub_stract  :1;
        volatile unsigned int reserved1   :26;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_store_slice_size_tag {
    struct _isp_store_slice_size_map {
        volatile unsigned int slice_with   :16;
        volatile unsigned int slice_height :16;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_store_slice_y_addr_tag {
    struct _isp_store_slice_y_addr_map {
        volatile unsigned int y_addr      :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_store_slice_y_pitch_tag {
    struct _isp_store_slice_y_pitch_map {
        volatile unsigned int y_pitch     :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_store_slice_u_addr_tag {
    struct _isp_store_slice_u_addr_map {
        volatile unsigned int u_addr      :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_store_slice_u_pitch_tag {
    struct _isp_store_slice_u_pitch_map {
        volatile unsigned int u_pitch     :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_store_slice_v_addr_tag {
    struct _isp_store_slice_v_addr_map {
        volatile unsigned int v_addr      :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_store_slice_v_pitch_tag {
    struct _isp_store_slice_v_pitch_map {
        volatile unsigned int v_pitch     :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_store_int_ctrl_tag {
    struct _isp_store_int_ctrl_map {
        volatile unsigned int int_cnt_num :16;
        volatile unsigned int int_eb      :1;
        volatile unsigned int reserved    :15;
    }mBits ;
    volatile unsigned int dwValue ;
};

// feeder
union _isp_feeder_status_tag {
    struct _isp_feeder_status_map {
        volatile unsigned int status      :32; // feeder status read only
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_feeder_param_tag {
    struct _isp_feeder_param_map {
        volatile unsigned int reserved0   :1;
        volatile unsigned int data_type   :3;
        volatile unsigned int reserved1   :28;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_feeder_slice_size_tag {
    struct _isp_feeder_slice_size_map {
        volatile unsigned int slice_with   :16;
        volatile unsigned int slice_height :16;
    }mBits ;
    volatile unsigned int dwValue ;
}ISP_FEEDER_SLICE_SIZE_U;

// Arbiter
union _isp_arbiter_status_tag {
    struct _isp_arbiter_status_map {
        volatile unsigned int status      :32; // arbiter status read only
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_arbiter_wr_master_status_v0001_tag {
    struct _isp_arbiter_wr_master_status_v0001_map {
        volatile unsigned int status      :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_arbiter_rd_master_status_v0001_tag {
    struct _isp_arbiter_rd_master_status_v0001_map {
        volatile unsigned int status      :32;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_arbiter_prerst_tag {
    struct _isp_farbiter_prerst_map {
        volatile unsigned int reset       :1;
        volatile unsigned int reserved    :31;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_arbiter_pause_cycle_tag {
    struct _isp_arbiter_pause_cycle_map {
        volatile unsigned int pause_cycle :8;
        volatile unsigned int reserved    :24;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_arbiter_master_ctrl_v0001_tag {
    struct _isp_arbiter_master_ctrl_v0001_map {
        volatile unsigned int wr_wait_resp :1;
        volatile unsigned int reserved     :31;
    }mBits ;
    volatile unsigned int dwValue ;
};

// hdr
union _isp_hdr_status_tag {
    struct _isp_hdr_status_map {
        volatile unsigned int status      :32; // hdr status read only
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_hdr_param_tag {
    struct _isp_hdr_param_map {
        volatile unsigned int bypass      :1;
        volatile unsigned int level       :1; // 0:slight 1:heavy
        volatile unsigned int reserved    :30;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_hdr_index_tag {
    struct _isp_hdr_index_map {
        volatile unsigned int index_r     :8;
        volatile unsigned int index_g     :8;
        volatile unsigned int index_b     :8;
        volatile unsigned int reserved    :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

// common
union _isp_com_status_tag {
    struct _isp_com_status_map {
        volatile unsigned int isp_end        :1;
        volatile unsigned int store_hist_end :1;
        volatile unsigned int int_occur      :1;
        volatile unsigned int axi_idle       :1;
        volatile unsigned int reserved       :28;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_com_start_tag {
    struct _isp_com_start_map {
        volatile unsigned int start_bit   :1;
        volatile unsigned int reserved    :31;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_com_param_tag {
    struct _isp_com_param_map {
        volatile unsigned int in_mode            :2;
        volatile unsigned int fetch_endian       :2;
        volatile unsigned int fetch_bit_reorder  :1;
        volatile unsigned int reserved0          :1;
        volatile unsigned int bpc_endian         :2;
        volatile unsigned int store_endian       :2;
        volatile unsigned int out_mode           :2;
        volatile unsigned int reserved1          :2;
        volatile unsigned int store_yuv_format   :4;
        volatile unsigned int fetch_color_format :4;
        volatile unsigned int reserved2          :10;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_com_burst_size_tag {
    struct _isp_com_burst_size_map {
        volatile unsigned int burst_size  :3;
        volatile unsigned int reserved    :29;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_com_mem_switch_v0000_tag {
    struct _isp_com_mem_switch_v0000_map {
        volatile unsigned int mem_switch  :1;
        volatile unsigned int reserved    :31;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_com_shadow_tag {
    struct _isp_com_shadow_map {
        volatile unsigned int shadow_bit  :1;
        volatile unsigned int reserved    :31;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_com_bayer_mode_tag {
    struct _isp_com_bayer_mode_map {
        volatile unsigned int bnlc_bayer  :2;
        volatile unsigned int awbc_bayer  :2;
        volatile unsigned int wave_bayer  :2;
        volatile unsigned int cfa_bayer   :2;
        volatile unsigned int gain_bayer  :2;
        volatile unsigned int reserved    :22;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_com_shadow_type_v0002_tag {
    struct _isp_com_shadow_type_v0002_map {
        volatile unsigned int reserved0 :2;
        volatile unsigned int store     :2;
        volatile unsigned int fetch     :2;
        volatile unsigned int css       :2;
        volatile unsigned int fcs       :2;
        volatile unsigned int edge      :2;
        volatile unsigned int bright    :2;
        volatile unsigned int pref      :2;
        volatile unsigned int reserved1 :2;
        volatile unsigned int yiq       :2;
        volatile unsigned int cfa       :2;
        volatile unsigned int lens      :2;
        volatile unsigned int denoise   :2;
        volatile unsigned int bpc       :2;
        volatile unsigned int blc       :2;
        volatile unsigned int pre_wave_denoise :2;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_com_pmu_ram_mask_tag {
    struct _isp_com_pmu_ram_mask_map {
        volatile unsigned int ram_mask    :1;
        volatile unsigned int reserved    :30;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_com_hw_mask_tag {
    struct _isp_com_hw_mask_map {
        volatile unsigned int raw_rgb     :1;
        volatile unsigned int full_rgb    :1;
        volatile unsigned int yuv_logic   :1;
        volatile unsigned int fetch       :1;
        volatile unsigned int lens        :1;
        volatile unsigned int hist        :1;
        volatile unsigned int reserved    :26;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_com_hw_eb_tag {
    struct _isp_com_hw_eb_map {
        volatile unsigned int raw_rgb_eb   :1;
        volatile unsigned int full_rgb_eb  :1;
        volatile unsigned int yuv_logic_eb :1;
        volatile unsigned int fetch_eb     :1;
        volatile unsigned int lens_eb      :1;
        volatile unsigned int hist_eb      :1;
        volatile unsigned int reserved     :26;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_com_sw_switch_tag {
    struct _isp_com_sw_switch_map {
        volatile unsigned int pmu_sel     :1; // 0:hw 1:sw
        volatile unsigned int reserved    :31;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_com_sw_toggle_tag {
    struct _isp_com_sw_toggle_map {
        volatile unsigned int raw_rgb_eb   :1;
        volatile unsigned int full_rgb_eb  :1;
        volatile unsigned int yuv_logic_eb :1;
        volatile unsigned int hlffs_eb     :1;
        volatile unsigned int reserved     :28;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_com_preview_stop_tag {
    struct _isp_com_preview_stop_map {
        volatile unsigned int stop        :1;
        volatile unsigned int reserved    :31;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_com_shadow_cnt_tag {
    struct _isp_com_shadow_cnt_map {
        volatile unsigned int cnt_eb      :1;
        volatile unsigned int cnt_clear   :1;
        volatile unsigned int counter     :8;
        volatile unsigned int reserved    :22;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_com_axi_stop_tag {
    struct _isp_com_axi_stop_map {
        volatile unsigned int stop      :1;
        volatile unsigned int reserved  :30;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_com_axi_ctrl_v0002_tag {
    struct _isp_com_axi_ctrl_v0002_map {
        volatile unsigned int w_outstanding        :4;
        volatile unsigned int r_outstanding        :4;
        volatile unsigned int interval_cycle       :8;
        volatile unsigned int maxlen_8             :1;
        volatile unsigned int wait_bresp           :1;
        volatile unsigned int trans_stop           :1;
        volatile unsigned int reserved             :13;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_com_slice_cnt_v0001_tag {
    struct _isp_com_slice_cnt_v0001_map {
        volatile unsigned int slice_cnt_eb   :1;
        volatile unsigned int pefrorm_cnt_eb :1;
        volatile unsigned int reserved1      :2;
        volatile unsigned int slice_cnt_num  :5;
        volatile unsigned int reserved2      :7;
        volatile unsigned int slice_counter  :5;
        volatile unsigned int reserved3      :11;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_com_perform_cnt_r_v0001_tag {
    struct _isp_com_perform_cnt_r_v0001_map { // read only
        volatile unsigned int pefrorm_cnt_r :28;
        volatile unsigned int slice_cnt_r   :4;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_com_perform_cnt_v0001_tag {
    struct _isp_com_perform_cnt_v0001_map { // read only
        volatile unsigned int pefrorm_cnt :28;
        volatile unsigned int slice_cnt   :4;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_com_int_eb_tag {
    struct _isp_com_int_eb_map {
        volatile unsigned int hist_store_eb      :1;
        volatile unsigned int store_eb           :1;
        volatile unsigned int lens_load_eb       :1;
        volatile unsigned int hist_cal_eb        :1;
        volatile unsigned int hist_rst_eb        :1;
        volatile unsigned int fetch_buf_full_eb  :1;
        volatile unsigned int store_dcam_full_eb :1;
        volatile unsigned int store_err_eb       :1;
        volatile unsigned int shadow_eb          :1;
        volatile unsigned int preview_stop_eb    :1;
        volatile unsigned int awbm_eb            :1;
        volatile unsigned int afm_eb             :1;
        volatile unsigned int reserved           :20;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_com_int_clr_tag {
    struct _isp_com_int_clr_map {
        volatile unsigned int hist_store_eb      :1;
        volatile unsigned int store_eb           :1;
        volatile unsigned int lens_load_eb       :1;
        volatile unsigned int hist_cal_eb        :1;
        volatile unsigned int hist_rst_eb        :1;
        volatile unsigned int fetch_buf_full_eb  :1;
        volatile unsigned int store_dcam_full_eb :1;
        volatile unsigned int store_err_eb       :1;
        volatile unsigned int shadow_eb          :1;
        volatile unsigned int preview_stop_eb    :1;
        volatile unsigned int awbm_eb            :1;
        volatile unsigned int afm_eb             :1;
        volatile unsigned int reserved           :20;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_com_int_raw_tag {
    struct _isp_com_int_raw_map {
        volatile unsigned int hist_store_eb      :1;
        volatile unsigned int store_eb           :1;
        volatile unsigned int lens_load_eb       :1;
        volatile unsigned int hist_cal_eb        :1;
        volatile unsigned int hist_rst_eb        :1;
        volatile unsigned int fetch_buf_full_eb  :1;
        volatile unsigned int store_dcam_full_eb :1;
        volatile unsigned int store_err_eb       :1;
        volatile unsigned int shadow_eb          :1;
        volatile unsigned int preview_stop_eb    :1;
        volatile unsigned int awbm_eb            :1;
        volatile unsigned int afm_eb             :1;
        volatile unsigned int reserved           :20;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_com_int_tag {
    struct _isp_com_int_map {
        volatile unsigned int hist_store_eb      :1;
        volatile unsigned int store_eb           :1;
        volatile unsigned int lens_load_eb       :1;
        volatile unsigned int hist_cal_eb        :1;
        volatile unsigned int hist_rst_eb        :1;
        volatile unsigned int fetch_buf_full_eb  :1;
        volatile unsigned int store_dcam_full_eb :1;
        volatile unsigned int store_err_eb       :1;
        volatile unsigned int shadow_eb          :1;
        volatile unsigned int preview_stop_eb    :1;
        volatile unsigned int awbm_eb            :1;
        volatile unsigned int afm_eb             :1;
        volatile unsigned int reserved           :20;
    }mBits ;
    volatile unsigned int dwValue ;
};

// GAIN GLB
union _isp_glb_gain_status_tag {
    struct _isp_glb_gain_status_map {
        volatile unsigned int status      :32; // hdr status read only
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_glb_gain_param_tag {
    struct _isp_glb_gain_param_map {
        volatile unsigned int gain        :8;
        volatile unsigned int reserved    :23;
		volatile unsigned int bypass      :1;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_glb_gain_slice_size_tag {
    struct _isp_glb_gain_slice_size_map {
        volatile unsigned int slice_with   :16;
        volatile unsigned int slice_height :16;
    }mBits ;
    volatile unsigned int dwValue ;
};

// GAIN RGB
union _isp_rgb_gain_status_tag {
    struct _isp_rgb_gain_status_map {
        volatile unsigned int status      :32; // rgb gain status read only
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_rgb_gain_param_tag {
    struct _isp_rgb_gain_param_map {
        volatile unsigned int b_gain      :8;
        volatile unsigned int g_gain      :8;
        volatile unsigned int r_gain      :8;
        volatile unsigned int reserved    :7;
        volatile unsigned int bypass      :1;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_rgb_gain_offset_tag {
    struct _isp_rgb_gain_offset_map {
        volatile unsigned int b_offset    :10;
        volatile unsigned int g_offset    :10;
        volatile unsigned int r_offset    :10;
        volatile unsigned int reserved    :2;
    }mBits ;
    volatile unsigned int dwValue ;
}ISP_RGB_GAIN_OFFSET_U;

//YIQ
union _isp_yiq_status_v0001_tag {
    struct _isp_yiq_status_v0001_map {
        volatile unsigned int status      :32; // yiq status read only
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_yiq_param_v0001_tag {
    struct _isp_yiq_param_v0001_map {
        volatile unsigned int ygamma_bypass  :1;
        volatile unsigned int ae_bypass      :1;
        volatile unsigned int flicker_bypass :1;
        volatile unsigned int src_sel        :2;
        volatile unsigned int reserved0      :11;
        volatile unsigned int ae_mode        :1;
        volatile unsigned int ae_skip_num    :4;
        volatile unsigned int flicker_mode   :1;
        volatile unsigned int skip_num_clear :1;
        volatile unsigned int reserved1      :9;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_ygamma_node_x0_v0001_tag {
    struct _isp_ygamma_node_x0_v0001_map {
        volatile unsigned int node3  :8;
        volatile unsigned int node2  :8;
        volatile unsigned int node1  :8;
		volatile unsigned int node0  :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_ygamma_node_x1_v0001_tag {
    struct _isp_ygamma_node_x1_v0001_map {
        volatile unsigned int node7  :8;
        volatile unsigned int node6  :8;
        volatile unsigned int node5  :8;
		volatile unsigned int node4  :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_ygamma_node_y0_v0001_tag {
    struct _isp_ygamma_node_y0_v0001_map {
        volatile unsigned int node3  :8;
        volatile unsigned int node2  :8;
        volatile unsigned int node1  :8;
		volatile unsigned int node0  :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_ygamma_node_y1_v0001_tag {
    struct _isp_ygamma_node_y1_v0001_map {
        volatile unsigned int node7  :8;
        volatile unsigned int node6  :8;
        volatile unsigned int node5  :8;
		volatile unsigned int node4  :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_ygamma_node_y2_v0001_tag {
    struct _isp_ygamma_node_y2_v0001_map {
        volatile unsigned int node9    :8;
        volatile unsigned int node8    :8;
        volatile unsigned int reserved :16;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_flicker_v_height_v0001_tag {
    struct _isp_flicker_v_height_v0001_map {
        volatile unsigned int v_height :16;
        volatile unsigned int reserved :16;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_flicker_line_cnt_v0001_tag {
    struct _isp_flicker_line_cnt_v0001_map {
        volatile unsigned int line_cnt :9;
        volatile unsigned int reserved :23;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_flicker_line_step_v0001_tag {
    struct _isp_flicker_line_step_v0001_map {
        volatile unsigned int line_step :4;
        volatile unsigned int reserved  :28;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_ygamma_node_idx_v0001_tag {
    struct _isp_ygamma_node_idx_v0001_map {
        volatile unsigned int index9    :3;
        volatile unsigned int index8    :3;
        volatile unsigned int index7    :3;
        volatile unsigned int index6    :3;
        volatile unsigned int index5    :3;
        volatile unsigned int index4    :3;
        volatile unsigned int index3    :3;
        volatile unsigned int index2    :3;
        volatile unsigned int index1    :3;
        volatile unsigned int reserved  :5;
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_flicker_line_start_v0001_tag {
    struct _isp_flicker_line_start_v0001_map {
        volatile unsigned int line_start :12;
        volatile unsigned int reserved   :20;
    }mBits ;
    volatile unsigned int dwValue ;
};

//HUE
union _isp_hue_status_v0001_tag {
    struct _isp_hue_status_v0001_map {
        volatile unsigned int status      :32; // hue status read only
    }mBits ;
    volatile unsigned int dwValue ;
};

union _isp_hue_param_v0001_tag {
    struct _isp_hue_param_v0001_map {
        volatile unsigned int bypass    :1;
        volatile unsigned int reserved0 :3;
		volatile unsigned int theta     :8;
		volatile unsigned int reserved1 :20;
    }mBits ;
    volatile unsigned int dwValue ;
};

//PRE GLOBAL GAIN
union _isp_pre_global_gain_tag {
    struct _isp_pre_global_gain_map {
        volatile unsigned int bypass      :1;
        volatile unsigned int reserved0   :7;
        volatile unsigned int gain        :16;
        volatile unsigned int reserved1   :8;
    }mBits ;
    volatile unsigned int dwValue ;
};

/**----------------------------------------------------------------------------*
**                         Compiler Flag                                      **
**----------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif
// End

