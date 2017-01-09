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
#ifndef _ISP_DRV_H_
#define _ISP_DRV_H_

#include <sys/types.h>
#include <utils/Log.h>
#include <stdlib.h>
#include <fcntl.h>              /* low-level i/o */
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "isp_drv_kernel.h"
#include "isp_log.h"
/*------------------------------------------------------------------------------*
*					Compiler Flag				*
*-------------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif
/*------------------------------------------------------------------------------*
*					Micro Define				*
*-------------------------------------------------------------------------------*/
#define SC8825_ISP_ID 0x00000000
#define SC8830_ISP_ID 0x00010000
#define SC9630_ISP_ID 0x00020000

#define ISP_EB 0x01
#define ISP_UEB 0x00

#define ISP_ZERO 0x00
#define ISP_ONE 0x01
#define ISP_ALPHA_ONE	1024
#define ISP_ALPHA_ZERO	0
#define ISP_TWO 0x02

#define ISP_MAX_HANDLE_NUM 0x02

#define ISP_TRAC(_x_) ISP_LOG _x_
#define ISP_RETURN_IF_FAIL(exp,warning) do{if(exp) {ISP_TRAC(warning); return exp;}}while(0)
#define ISP_TRACE_IF_FAIL(exp,warning) do{if(exp) {ISP_TRAC(warning);}}while(0)

//#define ISP_USER_DRV_DEBUG	0

/*------------------------------------------------------------------------------*
*					Data Structures				*
*-------------------------------------------------------------------------------*/
typedef enum
{
	ISP_SUCCESS=0x00,
	ISP_PARAM_NULL,
	ISP_PARAM_ERROR,
	ISP_CALLBACK_NULL,
	ISP_ALLOC_ERROR,
	ISP_NO_READY,
	ISP_ERROR,
	ISP_RETURN_MAX=0xffffffff
}ISP_RETURN_VALUE_E;

//int32_t isp_SetIspId(uint32_t isp_id);
int32_t ispRegInit(uint32_t handler_id, uint32_t isp_id);

// FETCH
int32_t ispGetFetchStatus(uint32_t handler_id, uint32_t* status);
int32_t ispGetFetchPreviewStatus(uint32_t handler_id, uint32_t* status);
int32_t ispFetchBypass(uint32_t handler_id, uint8_t bypass);
int32_t ispFetchSubtract(uint32_t handler_id, uint8_t eb);
int32_t ispSetFetchSliceSize(uint32_t handler_id, uint16_t w, uint16_t h);
int32_t ispSetFetchYAddr(uint32_t handler_id, uint32_t y_addr);
int32_t ispSetFetchYPitch(uint32_t handler_id, uint32_t y_pitch);
int32_t ispSetFetchUAddr(uint32_t handler_id, uint32_t u_addr);
int32_t ispSetFetchUPitch(uint32_t handler_id, uint32_t u_pitch);
int32_t ispSetFetchMipiWordInfo(uint32_t handler_id, uint16_t word_num);
int32_t ispSetFetchMipiByteInfo(uint32_t handler_id, uint16_t byte_rel_pos);
int32_t ispSetFetchVAddr(uint32_t handler_id, uint32_t v_addr);
int32_t ispSetFetchVPitch(uint32_t handler_id, uint32_t v_pitch);

//BNLC
int32_t ispGetBNLCStatus(uint32_t handler_id, uint32_t* status);
int32_t ispBlcBypass(uint32_t handler_id, uint8_t bypass);
int32_t ispSetBlcMode(uint32_t handler_id, uint8_t mode);
int32_t ispSetBlcCalibration(uint32_t handler_id, uint16_t r, uint16_t b, uint16_t gr, uint16_t gb);

/*NLC */
int32_t ispGetNLCStatus(uint32_t handler_id, uint32_t* status);
int32_t ispNlcBypass(uint32_t handler_id, uint8_t bypass);
int32_t ispSetNlcRNode(uint32_t handler_id, uint16_t* r_node_ptr);
int32_t ispSetNlcGNode(uint32_t handler_id, uint16_t* g_node_ptr);
int32_t ispSetNlcBNode(uint32_t handler_id, uint16_t* b_node_ptr);
int32_t ispSetNlcLNode(uint32_t handler_id, uint16_t* l_node_ptr);
int32_t ispSetBNLCSliceSize(uint32_t handler_id, uint16_t w, uint16_t h);
int32_t ispSetBNLCSliceInfo(uint32_t handler_id, uint8_t edge_info);

// Lens
int32_t ispGetLensStatus(uint32_t handler_id, uint32_t* status);
int32_t ispLensBypass(uint32_t handler_id, uint8_t bypass);
int32_t ispLensBufSel(uint32_t handler_id, uint8_t buf_mode);
int32_t ispLensParamAddr(uint32_t handler_id, unsigned long param_addr);
int32_t ispSetLensSliceStart(uint32_t handler_id, uint16_t x, uint16_t y);
int32_t ispSetLensLoaderEnable(uint32_t handler_id, unsigned long param_addr);
int32_t ispSetLensGridPitch(uint32_t handler_id, uint16_t pitch);
int32_t ispSetLensGridMode(uint32_t handler_id, uint8_t mode);
int32_t ispSetLensGridSize(uint32_t handler_id, uint16_t w, uint16_t h);
int32_t ispSetLensBuf(uint32_t handler_id, uint8_t buf_sel);
int32_t ispSetLensEndian(uint32_t handler_id, uint8_t endian);
int32_t ispLensSliceSize(uint32_t handler_id, uint16_t w, uint16_t h);

//AWBM
int32_t ispGetAwbmStatus(uint32_t handler_id, uint32_t* status);
int32_t ispAwbmBypass(uint32_t handler_id, uint8_t bypass);
int32_t ispAwbmMode(uint32_t handler_id, uint8_t mode);
int32_t ispAwbmSkip(uint32_t handler_id, uint8_t num);
int32_t ispSetAwbmWinStart(uint32_t handler_id, uint16_t x, uint16_t y);
int32_t ispSetAwbmWinSize(uint32_t handler_id, uint16_t w, uint16_t h);
int32_t ispSetAwbmShift(uint32_t handler_id, uint16_t shift);

//AWBC
int32_t ispGetAwbcStatus(uint32_t handler_id, uint32_t* status);
int32_t ispAwbcBypass(uint32_t handler_id, uint8_t bypass);
int32_t ispSetAwbGain(uint32_t handler_id, uint16_t r_gain, uint16_t g_gain, uint16_t b_gain);
int32_t ispSetAwbGainOffset(uint32_t handler_id, uint16_t r_offset, uint16_t g_offset, uint16_t b_offset);
int32_t ispSetAwbGainThrd(uint32_t handler_id, uint16_t r_thr, uint16_t g_thr, uint16_t b_thr);
void ispGetAWBMStatistic(uint32_t handler_id, uint32_t* r_info, uint32_t* g_info, uint32_t* b_info);

//BPC
int32_t ispGetBpcStatus(uint32_t handler_id, uint32_t* status);
int32_t ispBpcBypass(uint32_t handler_id, uint8_t bypass);
int32_t ispBpcMode(uint32_t handler_id, uint16_t mode);
int32_t ispSetBpcThrd(uint32_t handler_id, uint16_t flat_thr, uint16_t std_thr, uint16_t texture_thr);
int32_t ispBpcMapAddr(uint32_t handler_id, uint32_t map_addr);
int32_t ispBpcPixelNum(uint32_t handler_id, uint32_t pixel_num);

//NBPC
int32_t ispGetNBpcStatus(uint32_t handler_id, uint32_t* status);
int32_t ispNBpcBypass(uint32_t handler_id, uint8_t bypass);
int32_t ispNBpcPvdBypass(uint32_t handler_id, uint8_t bypass);
int32_t ispSetNBpcMode(uint32_t handler_id, uint8_t mode);
int32_t ispSetNBpcMaskMode(uint32_t handler_id, uint8_t mode);
int32_t ispSetNBpcKThr(uint32_t handler_id, uint8_t kmix, uint8_t kmax);
int32_t ispSetNBpcThrd(uint32_t handler_id, uint8_t cntr_thr);
int32_t ispSetNBpcHWClrEn(uint32_t handler_id, uint8_t hw_fifo_en);
int32_t ispSetNBpcEstimate14(uint32_t handler_id, uint8_t ktimes);
int32_t ispSetNBpcFifoClr(uint32_t handler_id, uint8_t fifoclr);
int32_t ispSetNBpcDelt34(uint32_t handler_id, uint8_t delt34);
int32_t ispNBpcPixelNum(uint32_t handler_id, uint32_t pixel_num);
int32_t ispNBpcFactor(uint32_t handler_id,  uint8_t flat_fct, uint8_t safe_fct);
int32_t ispNBpCoeff(uint32_t handler_id, uint8_t spike_coeff, uint8_t dead_coeff);
int32_t ispNBpCLutword(uint32_t handler_id, uint16_t *interrept_b, uint16_t *slope_k, uint16_t *lut_level);
int32_t ispNBPCMapDownSel(uint32_t handler_id, uint8_t mapsel);
int32_t ispNBPCSel(uint32_t handler_id, uint8_t select);
int32_t ispNBpcMapAddr(uint32_t handler_id, uint32_t map_addr);
//Wave denoise
int32_t ispGetWDenoiseStatus(uint32_t handler_id, uint32_t* status);
int32_t ispWDenoiseBypass(uint32_t handler_id, uint8_t bypass);
int32_t ispWDenoiseWriteBack(uint32_t handler_id, uint16_t write_back);
int32_t ispWDenoiseThrd(uint32_t handler_id, uint16_t r_thr, uint16_t g_thr, uint16_t b_thr);
int32_t ispWDenoiseSliceSize(uint32_t handler_id, uint16_t w, uint16_t h);
int32_t ispWDenoiseSliceInfo(uint32_t handler_id, uint16_t edge_info);
int32_t ispWDenoiseDiswei(uint32_t handler_id, uint8_t* diswei_ptr);
int32_t ispWDenoiseRanwei(uint32_t handler_id, uint8_t* ranwei_ptr);

//GrGb C
int32_t ispGetGrGbStatus(uint32_t handler_id, uint32_t* status);
int32_t ispGrGbbypass(uint32_t handler_id, uint8_t bypass);
int32_t ispSetGrGbThrd(uint32_t handler_id, uint16_t edge_thr, uint16_t diff_thr);

//CFA
int32_t ispGetCFAStatus(uint32_t handler_id, uint32_t* status);
int32_t ispSetCFAThrd(uint32_t handler_id, uint16_t edge_thr, uint16_t diff_thr);
int32_t ispCFASliceSize(uint32_t handler_id, uint16_t w, uint16_t h);
int32_t ispCFASliceInfo(uint32_t handler_id, uint16_t edge_info);

//CMC
int32_t ispGetCMCStatus(uint32_t handler_id, uint32_t* status);
int32_t ispCMCbypass(uint32_t handler_id, uint8_t bypass);
int32_t ispSetCMCMatrix(uint32_t handler_id, uint16_t* matrix_ptr);

//Gamma
int32_t ispGetGammaStatus(uint32_t handler_id, uint32_t* status);
int32_t ispGammabypass(uint32_t handler_id, uint8_t bypass);

int32_t ispSetGammaXNode(uint32_t handler_id, uint16_t* node);
int32_t ispSetGammaYNode(uint32_t handler_id, uint16_t* node);
int32_t ispSetGammaRNode(uint32_t handler_id, uint16_t* node);
int32_t ispSetGammaGNode(uint32_t handler_id, uint16_t* node);
int32_t ispSetGammaBNode(uint32_t handler_id, uint16_t* node);
int32_t ispSetGammaNodeIndex(uint32_t handler_id, uint8_t* node);
int32_t ispSetGammaXNode_v002(uint32_t handler_id, uint16_t* node);
int32_t ispSetGammaYNode_v002(uint32_t handler_id, uint16_t* k, uint16_t* b);

//CCE
int32_t ispGetCCEStatus(uint32_t handler_id, uint32_t* status);
int32_t ispSetCCEMatrix(uint32_t handler_id, uint16_t* matrix_ptr);
int32_t ispSetCCEShift(uint32_t handler_id, uint16_t y_shift, uint16_t u_shift, uint16_t v_shift);
int32_t ispCCEUVDivBypass(uint32_t handler_id, uint8_t bypass);
int32_t ispSetCCEUVDiv(uint32_t handler_id, uint8_t* div_ptr);
int32_t ispSetCCEUVC(uint32_t handler_id, uint8_t* t_ptr, uint8_t* m_ptr);

//PREF
int32_t ispGetPrefStatus(uint32_t handler_id, uint32_t* status);
int32_t ispPrefBypass(uint32_t handler_id, uint8_t bypass);
int32_t ispPrefWriteBack(uint32_t handler_id, uint16_t write_back);
int32_t ispSetPrefThrd(uint32_t handler_id, uint8_t y_thr, uint8_t u_thr, uint8_t v_thr);
int32_t ispPrefSliceSize(uint32_t handler_id, uint16_t w, uint16_t h);
int32_t ispPrefSliceInfo(uint32_t handler_id, uint16_t edge_info);

//BRIGHT
int32_t ispGetBrightStatus(uint32_t handler_id, uint32_t* status);
int32_t ispBrightBypass(uint32_t handler_id, uint8_t bypass);
int32_t ispSetBrightFactor(uint32_t handler_id, uint8_t factor);
int32_t ispBrightSliceSize(uint32_t handler_id, uint16_t w, uint16_t h);
int32_t ispBrightSliceInfo(uint32_t handler_id, uint16_t edge_info);

//CONTRAST
int32_t ispGetContrastStatus(uint32_t handler_id, uint32_t* status);
int32_t ispContrastbypass(uint32_t handler_id, uint8_t bypass);
int32_t ispSetContrastFactor(uint32_t handler_id, uint8_t factor);

//HIST
int32_t ispGetHistStatus(uint32_t handler_id, uint32_t* status);
int32_t ispHistbypass(uint32_t handler_id, uint8_t bypass);
int32_t ispHistAutoRstDisenable(uint32_t handler_id, uint8_t off);
int32_t ispHistMode(uint32_t handler_id, uint8_t mode);
int32_t ispSetHistRatio(uint32_t handler_id, uint16_t low_ratio, uint16_t high_ratio);
int32_t ispSetHistMaxMin(uint32_t handler_id, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max);
int32_t ispHistClearEn(uint32_t handler_id, uint8_t eb);
int32_t ispGetHISTStatistic(uint32_t handler_id, uint32_t* param_ptr);

//Auto Contrast
int32_t ispGetAutoContrastStatus(uint32_t handler_id, uint32_t* status);
int32_t ispAutoContrastbypass(uint32_t handler_id, uint8_t bypass);
int32_t ispSetAutoContrastMode(uint32_t handler_id, uint16_t mode);
int32_t ispSetAutoContrastMaxMin(uint32_t handler_id, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max);

//AFM
int32_t ispGetAFMStatus(uint32_t handler_id, uint32_t* status);
int32_t ispAFMbypass(uint32_t handler_id, uint8_t bypass);
int32_t ispSetAFMShift(uint32_t handler_id, uint16_t shift);
int32_t ispAFMMode(uint32_t handler_id, uint8_t mode);
int32_t ispAFMSkipNum(uint32_t handler_id, uint8_t num);
int32_t ispAFMSkipClear(uint32_t handler_id, uint8_t eb);
int32_t ispSetAFMWindow(uint32_t handler_id, uint16_t win[][4]);
int32_t ispGetAFMStatistic(uint32_t handler_id, uint32_t* statistic_ptr);

//EE
int32_t ispGetEdgeStatus(uint32_t handler_id, uint32_t* status);
int32_t ispEdgeBypass(uint32_t handler_id, uint8_t bypass);
int32_t ispSetEdgeParam(uint32_t handler_id, uint16_t detail_th, uint16_t smooth_th, uint16_t strength);

//Emboss
int32_t ispGetEmbossStatus(uint32_t handler_id, uint32_t* status);
int32_t ispEmbossypass(uint32_t handler_id, uint8_t bypass);
int32_t ispSetEmbossParam(uint32_t handler_id, uint16_t step);

//FS
int32_t ispGetFalseColorStatus(uint32_t handler_id, uint32_t* status);
int32_t ispFalseColorBypass(uint32_t handler_id, uint8_t bypass);
int32_t ispFalseColorMode(uint32_t handler_id, uint8_t mode);

//CSS color saturation suppression
int32_t ispGetColorSaturationSuppressStatus(uint32_t handler_id, uint32_t* status);
int32_t ispColorSaturationSuppressBypass(uint32_t handler_id, uint8_t bypass);
int32_t ispSetColorSaturationSuppressThrd(uint32_t handler_id, uint8_t* low_thr, uint8_t* low_sum_thr, uint8_t lum_thr, uint chr_thr);
int32_t ispColorSaturationSuppressSliceSize(uint32_t handler_id, uint16_t w, uint16_t h);
int32_t ispSetColorSaturationSuppressRatio(uint32_t handler_id, uint8_t* ratio);


//SATURATION
int32_t ispGetSaturationStatus(uint32_t handler_id, uint32_t* status);
int32_t ispSaturationbypass(uint32_t handler_id, uint8_t uint8_t);
int32_t ispSetSaturationFactor(uint32_t handler_id, uint8_t factor);

// STORE
int32_t ispGetStoreStatus(uint32_t handler_id, uint32_t* status);
int32_t ispGetStoreStatusPreview(uint32_t handler_id, uint32_t* status);
int32_t ispStoreBypass(uint32_t handler_id, uint8_t bypass);
int32_t ispStoreSubtract(uint32_t handler_id, uint8_t subtract);
int32_t ispSetStoreSliceSize(uint32_t handler_id, uint16_t w, uint16_t h);
int32_t ispSetStoreYAddr(uint32_t handler_id, uint32_t y_addr);
int32_t ispSetStoreYPitch(uint32_t handler_id, uint32_t y_pitch);
int32_t ispSetStoreUAddr(uint32_t handler_id, uint32_t u_addr);
int32_t ispSetStoreUPitch(uint32_t handler_id, uint32_t u_pitch);
int32_t ispSetStoreVAddr(uint32_t handler_id, uint32_t v_addr);
int32_t ispSetStoreVPitch(uint32_t handler_id, uint32_t v_pitch);
int32_t ispSetStoreInt(uint32_t handler_id, uint16_t int_cnt_num, uint8_t int_eb);

// FEEDER
int32_t ispGetFeederStatus(uint32_t handler_id, uint32_t* status);
int32_t ispSetFeederDataType(uint32_t handler_id, uint16_t data_type);
int32_t ispSetFeederSliceSize(uint32_t handler_id, uint16_t w, uint16_t h);

// ARBITER
int32_t ispGetArbiterStatus(uint32_t handler_id, uint32_t* status);
int32_t ispGetAxiWrMasterStatus(uint32_t handler_id, uint32_t* status);
int32_t ispGetAxiRdMasterStatus(uint32_t handler_id, uint32_t* status);
int32_t ispArbiterReset(uint32_t handler_id, uint8_t rst);
int32_t ispSetArbiterPauseCycle(uint32_t handler_id, uint16_t cycle);
int32_t ispAxiMasterCtrl(uint32_t handler_id, uint8_t wait_resp);

// HDR
int32_t ispGetHDRStatus(uint32_t handler_id, uint32_t* status);
int32_t ispHDRBypass(uint32_t handler_id, uint8_t bypass);
int32_t ispSetHDRLevel(uint32_t handler_id, uint16_t level);
int32_t ispSetHDRIndex(uint32_t handler_id, uint8_t r_index, uint8_t g_index, uint8_t b_index);
int32_t ispSetHDRIndexTab(uint32_t handler_id, uint8_t* com_ptr, uint8_t* p2e_ptr, uint8_t* e2p_ptr);

// COMMON
int32_t ispSetIspStatus(uint32_t handler_id, uint8_t end_status, uint8_t end_int);
int32_t isp_Start(uint32_t handler_id, uint8_t start);
int32_t ispInMode(uint32_t handler_id, uint32_t mode);
int32_t ispOutMode(uint32_t handler_id, uint32_t mode);
int32_t ispFetchEdian(uint32_t handler_id, uint32_t endian, uint32_t bit_reorder);
int32_t ispBPCEdian(uint32_t handler_id, uint32_t endian);
int32_t ispStoreEdian(uint32_t handler_id, uint32_t endian);
int32_t ispFetchDataFormat(uint32_t handler_id, uint32_t format);
int32_t ispStoreFormat(uint32_t handler_id, uint32_t format);
int32_t ispSetBurstSize(uint32_t handler_id, uint16_t burst_size);
int32_t ispMemSwitch(uint32_t handler_id, uint8_t mem_switch);
int32_t ispShadow(uint32_t handler_id, uint8_t shadow);
int32_t ispShadowAll(uint32_t handler_id, uint8_t shadow);
int32_t ispByerMode(uint32_t handler_id, uint32_t nlc_bayer, uint32_t awbc_bayer, uint32_t wave_bayer, uint32_t cfa_bayer, uint32_t gain_bayer);
int32_t ispIntRegister(uint32_t handler_id, uint32_t int_num, void(*fun)());
int32_t ispIntClear(uint32_t handler_id, uint32_t int_num);
int32_t ispGetIntRaw(uint32_t handler_id, uint32_t* raw);
int32_t ispPMURamMask(uint32_t handler_id, uint8_t mask);
int32_t ispHwMask(uint32_t handler_id, uint32_t hw_logic);
int32_t ispHwEnable(uint32_t handler_id, uint32_t hw_logic);
int32_t ispPMUSel(uint32_t handler_id, uint8_t sel);
int32_t ispSwEnable(uint32_t handler_id, uint32_t sw_logic);
int32_t ispPreviewStop(uint32_t handler_id, uint8_t eb);
int32_t ispSetShadowConter(uint32_t handler_id, uint32_t conter);
int32_t ispShadowConterClear(uint32_t handler_id, uint8_t eb);
int32_t ispAXIStop(uint32_t handler_id, uint8_t eb);
int32_t ispSliceCntEb(uint32_t handler_id, uint8_t eb);
int32_t ispPerformCntEb(uint32_t handler_id, uint8_t eb);
int32_t ispSetSliceNum(uint32_t handler_id, uint8_t num);
uint8_t ispGetSliceNum(uint32_t handler_id);
int32_t ispGetPerformCntRStatus(uint32_t handler_id, uint32_t* cnt);
int32_t ispGetPerformCntStatus(uint32_t handler_id, uint32_t* cnt);

//Pre Glb Gain
int32_t ispPreGlbGainBypass(uint32_t handler_id, uint8_t bypass);
int32_t ispPreSetGlbGain(uint32_t handler_id, uint32_t gain);

//Glb gain
int32_t ispGetGlbGainStatus(uint32_t handler_id, uint32_t* status);
int32_t ispGlbGainBypass(uint32_t handler_id, uint8_t bypass);
int32_t ispSetGlbGain(uint32_t handler_id, uint32_t gain);
int32_t ispGlbGainSliceSize(uint32_t handler_id, uint16_t w, uint16_t h);

//RGB gain
int32_t ispGetChnGainStatus(uint32_t handler_id, uint32_t* status);
int32_t ispChnGainBypass(uint32_t handler_id, uint8_t bypass);
int32_t ispSetChnGain(uint32_t handler_id, uint16_t r_gain, uint16_t g_gain, uint16_t b_gain);
int32_t ispSetChnGainOffset(uint32_t handler_id, uint16_t r_offset, uint16_t g_offset, uint16_t b_offset);

//YIQ
int32_t ispGetYiqStatus(uint32_t handler_id, uint32_t* yiq_status);
int32_t ispYGammabypass(uint32_t handler_id, uint8_t bypass);
int32_t ispSetYGammaXNode(uint32_t handler_id, uint8_t* node);
int32_t ispSetYGammaYNode(uint32_t handler_id, uint16_t* node);
int32_t ispSetYGammaNodeIdx(uint32_t handler_id, uint16_t* node);

// Auto exposure
int32_t ispAembypass(uint32_t handler_id, uint8_t bypass);
int32_t ispAemMode(uint32_t handler_id, uint8_t mode);
int32_t ispAemSkipNum(uint32_t handler_id, uint8_t num);
int32_t ispAemSourceSel(uint32_t handler_id, uint8_t src_sel);
int32_t ispGetAEMStatistic(uint32_t handler_id, uint32_t* y_info);

// Auto Flicker
int32_t ispAutoFlickerVHeight(uint32_t handler_id, uint16_t height);
int32_t ispAutoFlickerLineConter(uint32_t handler_id, uint16_t counter);
int32_t ispAutoFlickerLineStep(uint32_t handler_id, uint8_t step);
int32_t ispAutoFlickerLineStart(uint32_t handler_id, uint16_t start_line);
int32_t ispAutoFlickerbypass(uint32_t handler_id, uint8_t bypass);
int32_t ispAutoFlickerMode(uint32_t handler_id, uint8_t mode);

//Hue
int32_t ispGetHueStatus(uint32_t handler_id, uint32_t* status);
int32_t ispHuebypass(uint32_t handler_id, uint8_t bypass);
int32_t ispSetHueFactor(uint32_t handler_id, uint8_t factor);

int32_t ispGetSignalSize(uint32_t handler_id, uint16_t* width, uint16_t* height);
int32_t ispGetContinueSize(uint32_t handler_id, uint16_t* width, uint16_t* height);
int32_t ispGetAwbWinNum(uint32_t handler_id, uint16_t* width, uint16_t* height);
uint32_t ispGetAwbDefaultGain(uint32_t handler_id);
uint32_t ispGetAfMaxNum(uint32_t handler_id);
int32_t ispOpenDev(uint32_t handler_id, uint32_t clk_on);
int32_t ispOpenClk(uint32_t handler_id, uint32_t clk_on);
int32_t ispReset(uint32_t handler_id, uint32_t rst);
int32_t ispGetIRQ(uint32_t *evt_ptr);
int32_t ispRegIRQ(uint32_t handler_id, uint32_t int_num);
int32_t ispCfgDcamIRQ(uint32_t handler_id, uint32_t param);
int32_t ispSetLncParam(uint32_t handler_id, unsigned long addr, uint32_t len);
uint32_t ispAlloc(uint32_t handler_id, uint32_t len);
int32_t ispStop(uint32_t handler_id);
int32_t ispRegWrite(uint32_t handler_id, uint32_t num, void* param_ptr);
int32_t ispRegRead(uint32_t handler_id, uint32_t num, void* param_ptr);

int ispGetRegVal(uint32_t handler_id, uint32_t base_offset, uint32_t *buf, uint32_t len);
int32_t ispBypassNewFeature(uint32_t handler_id);

/*------------------------------------------------------------------------------*
*					Compiler Flag				*
*-------------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/*-----------------------------------------------------------------------------*/
#endif
// End

