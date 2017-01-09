//--=========================================================================--
//  This file is a part of VPU Reference API project
//-----------------------------------------------------------------------------
//
//       This confidential and proprietary software may be used only
//     as authorized by a licensing agreement from Chips&Media Inc.
//     In the event of publication, the following notice is applicable:
//
//            (C) COPYRIGHT 2006 - 2011  CHIPS&MEDIA INC.
//                      ALL RIGHTS RESERVED
//
//       The entire notice above must be reproduced on all authorized
//       copies.
//
//--=========================================================================--

#include "vpuapifunc.h"
#include "regdefine.h"



#ifndef MIN
#define MIN(a, b)       (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b)       (((a) > (b)) ? (a) : (b))
#endif
/******************************************************************************
    define value
******************************************************************************/

/******************************************************************************
    Codec Instance Slot Management
******************************************************************************/


RetCode InitCodecInstancePool(Uint32 coreIdx)
{
	int i;
	CodecInst * pCodecInst;
	vpu_instance_pool_t *vip;

	vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
	if (!vip)
		return RETCODE_INSUFFICIENT_RESOURCE;

	if (vip->instance_pool_inited==0)
	{
		for( i = 0; i < MAX_NUM_INSTANCE; i++)
		{
			pCodecInst = (CodecInst *)vip->codecInstPool[i];
			pCodecInst->instIndex = i;
			pCodecInst->inUse = 0;
		}
		vip->instance_pool_inited = 1;
	}
	return RETCODE_SUCCESS;
}
 
/*
 * GetCodecInstance() obtains a instance.
 * It stores a pointer to the allocated instance in *ppInst
 * and returns RETCODE_SUCCESS on success.
 * Failure results in 0(null pointer) in *ppInst and RETCODE_FAILURE.
 */

RetCode GetCodecInstance(Uint32 coreIdx, CodecInst ** ppInst)
{
	int i;
	CodecInst * pCodecInst = 0;
	vpu_instance_pool_t *vip;
	vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
	if (!vip)
		return RETCODE_INSUFFICIENT_RESOURCE;

	for (i = 0; i < MAX_NUM_INSTANCE; i++) {
		pCodecInst = (CodecInst *)vip->codecInstPool[i];

		if (!pCodecInst) {
			return RETCODE_FAILURE;
		}

		if (!pCodecInst->inUse) {
			break;
		}
	}

	if (i == MAX_NUM_INSTANCE) {
		*ppInst = 0;
		return RETCODE_INSUFFICIENT_RESOURCE;
	}
	pCodecInst->loggingEnable = 0;
	pCodecInst->inUse = 1;
	pCodecInst->coreIdx = coreIdx;
	pCodecInst->codecModeAux = -1;
	pCodecInst->codecMode = -1;
	osal_memset((void*)&pCodecInst->CodecInfo, 0x00, sizeof(pCodecInst->CodecInfo));
	*ppInst = pCodecInst;

	if (vdi_open_instance(pCodecInst->coreIdx, pCodecInst->instIndex) < 0)
		return RETCODE_FAILURE;

	return RETCODE_SUCCESS;
}

void FreeCodecInstance(CodecInst * pCodecInst)
{

	pCodecInst->inUse = 0;
    pCodecInst->codecMode    = -1;
    pCodecInst->codecModeAux = -1;
   
	vdi_close_instance(pCodecInst->coreIdx, pCodecInst->instIndex);

}

RetCode CheckInstanceValidity(CodecInst * pCodecInst)
{
	int i;
	vpu_instance_pool_t *vip;

	vip = (vpu_instance_pool_t *)vdi_get_instance_pool(pCodecInst->coreIdx);
	if (!vip)
		return RETCODE_INSUFFICIENT_RESOURCE;

	for (i = 0; i < MAX_NUM_INSTANCE; i++) {
		if ((CodecInst *)vip->codecInstPool[i] == pCodecInst)
			return RETCODE_SUCCESS;
	}
	return RETCODE_INVALID_HANDLE;
}



/******************************************************************************
    API Subroutines
******************************************************************************/
RetCode BitLoadFirmware(Uint32 coreIdx, PhysicalAddress codeBase, const Uint16 *codeWord, int codeSize)
{
	int i;
	Uint32 data;


	LoadBitCode(coreIdx, codeBase, codeWord, codeSize);

	VpuWriteReg(coreIdx, BIT_INT_ENABLE, 0);
	VpuWriteReg(coreIdx, BIT_CODE_RUN, 0);

	for (i=0; i<2048; ++i) {
		data = codeWord[i];
		VpuWriteReg(coreIdx, BIT_CODE_DOWN, (i << 16) | data);
	}

	return RETCODE_SUCCESS;
}

void LoadBitCode(Uint32 coreIdx, PhysicalAddress codeBase, const Uint16 *codeWord, int codeSize)
{
	int i;
	BYTE code[8];

	for (i=0; i<codeSize; i+=4) {
		// 2byte little endian variable to 1byte big endian buffer
		code[0] = (BYTE)(codeWord[i+0]>>8);
		code[1] = (BYTE)codeWord[i+0];
		code[2] = (BYTE)(codeWord[i+1]>>8);
		code[3] = (BYTE)codeWord[i+1];
		code[4] = (BYTE)(codeWord[i+2]>>8);
		code[5] = (BYTE)codeWord[i+2];
		code[6] = (BYTE)(codeWord[i+3]>>8);
		code[7] = (BYTE)codeWord[i+3];
		VpuWriteMem(coreIdx, codeBase+i*2, (BYTE *)code, 8, VDI_BIG_ENDIAN);
	}


	vdi_set_bit_firmware_to_pm(coreIdx, codeWord); 
}


void BitIssueCommand(Uint32 coreIdx, CodecInst *inst, int cmd)
{
	int instIdx = 0;
	int cdcMode = 0;
	int auxMode = 0;

	if (inst != NULL) // command is specific to instance
	{
		instIdx = inst->instIndex;
		cdcMode = inst->codecMode;
		auxMode = inst->codecModeAux;
	}



	if (inst) {
		if (inst->codecMode < AVC_ENC)	{
		}
		else {
		}
	}

	VpuWriteReg(coreIdx, BIT_BUSY_FLAG, 1);
	VpuWriteReg(coreIdx, BIT_RUN_INDEX, instIdx);
	VpuWriteReg(coreIdx, BIT_RUN_COD_STD, cdcMode);
	VpuWriteReg(coreIdx, BIT_RUN_AUX_STD, auxMode);
	if (inst && inst->loggingEnable)
		vdi_log(coreIdx, cmd, 1);



	VpuWriteReg(coreIdx, BIT_RUN_COMMAND, cmd);
}

RetCode CheckDecInstanceValidity(DecHandle handle)
{
	CodecInst * pCodecInst;
	RetCode ret;

    if (handle == NULL)
        return RETCODE_INVALID_HANDLE;

	pCodecInst = handle;
	ret = CheckInstanceValidity(pCodecInst);
	if (ret != RETCODE_SUCCESS) {
		return RETCODE_INVALID_HANDLE;
	}
	if (!pCodecInst->inUse) {
		return RETCODE_INVALID_HANDLE;
	}
	if (pCodecInst->codecMode != AVC_DEC && 
		pCodecInst->codecMode != VC1_DEC &&
		pCodecInst->codecMode != MP2_DEC &&
		pCodecInst->codecMode != MP4_DEC &&
		pCodecInst->codecMode != DV3_DEC &&
		pCodecInst->codecMode != RV_DEC &&
		pCodecInst->codecMode != AVS_DEC &&
        pCodecInst->codecMode != VPX_DEC
        ) {


		return RETCODE_INVALID_HANDLE;
	}

	return RETCODE_SUCCESS;
}

RetCode CheckDecOpenParam(DecOpenParam * pop)
{
	if (pop == 0) {
		return RETCODE_INVALID_PARAM;
	}
	if (pop->bitstreamBuffer % 512) {
		return RETCODE_INVALID_PARAM;
	}

	if (pop->bitstreamBufferSize % 1024 || pop->bitstreamBufferSize < 1024) {
		return RETCODE_INVALID_PARAM;
	}

	if (pop->bitstreamFormat != STD_AVC
			&& pop->bitstreamFormat != STD_VC1
			&& pop->bitstreamFormat != STD_MPEG2
			&& pop->bitstreamFormat != STD_H263
			&& pop->bitstreamFormat != STD_MPEG4
			&& pop->bitstreamFormat != STD_DIV3
			&& pop->bitstreamFormat != STD_RV
            && pop->bitstreamFormat != STD_AVS
            && pop->bitstreamFormat != STD_THO
            && pop->bitstreamFormat != STD_VP3
			&& pop->bitstreamFormat != STD_VP8
	) {
		return RETCODE_INVALID_PARAM;
	}

	if( pop->mp4DeblkEnable == 1 && !(pop->bitstreamFormat == STD_MPEG4 || pop->bitstreamFormat == STD_MPEG2 || pop->bitstreamFormat == STD_DIV3)) {
		return RETCODE_INVALID_PARAM;
	}


	if (pop->scalerInfo.enScaler) {

		if (pop->scalerInfo.scaleWidth > 0 || pop->scalerInfo.scaleHeight > 0) { // the parameter intend to scaler down

			if ((pop->scalerInfo.scaleWidth % 16)) {
				return RETCODE_INVALID_PARAM;
			}

			if ((pop->scalerInfo.scaleHeight % 8)) {
				return RETCODE_INVALID_PARAM;
			}
		}

		if (pop->scalerInfo.imageFormat <= YUV_FORMAT_YUYV) {

			if (pop->scalerInfo.imageFormat == YUV_FORMAT_NV12 || pop->scalerInfo.imageFormat == YUV_FORMAT_NV21 ||
				pop->scalerInfo.imageFormat == YUV_FORMAT_NV16 || pop->scalerInfo.imageFormat == YUV_FORMAT_NV61) {
					if (pop->cbcrInterleave == 0) {
						return RETCODE_INVALID_PARAM;
					}
			} else {
				if (pop->cbcrInterleave != 0) {
					return RETCODE_INVALID_PARAM;
				}
			}
		}
	}

	if (pop->coreIdx > MAX_VPU_CORE_NUM) {
		return RETCODE_INVALID_PARAM;
	}

	return RETCODE_SUCCESS;
}

int DecBitstreamBufEmpty(DecInfo * pDecInfo)
{
	return (pDecInfo->streamRdPtr == pDecInfo->streamWrPtr);
}

RetCode SetParaSet(DecHandle handle, int paraSetType, DecParamSet * para)
{
	CodecInst * pCodecInst;
	PhysicalAddress paraBuffer;
	int i;
	Uint32 * src;

	pCodecInst = handle;
	src = para->paraSet;

	EnterLock(pCodecInst->coreIdx);
	SetPendingInst(pCodecInst->coreIdx, pCodecInst);
	paraBuffer = VpuReadReg(pCodecInst->coreIdx, BIT_PARA_BUF_ADDR);
	for (i = 0; i < para->size; i += 4) {
		VpuWriteReg(pCodecInst->coreIdx, paraBuffer + i, *src++);
	}
	VpuWriteReg(pCodecInst->coreIdx, CMD_DEC_PARA_SET_TYPE, paraSetType); // 0: SPS, 1: PPS
	VpuWriteReg(pCodecInst->coreIdx, CMD_DEC_PARA_SET_SIZE, para->size);

	BitIssueCommand(pCodecInst->coreIdx, pCodecInst, DEC_PARA_SET);
	if (vdi_wait_vpu_busy(pCodecInst->coreIdx, VPU_BUSY_CHECK_TIMEOUT, BIT_BUSY_FLAG) == -1) {
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, DEC_PARA_SET, 2);
		SetPendingInst(pCodecInst->coreIdx, 0);
		LeaveLock(pCodecInst->coreIdx);
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, DEC_PARA_SET, 0);
	SetPendingInst(pCodecInst->coreIdx, 0);
	LeaveLock(pCodecInst->coreIdx);
	return RETCODE_SUCCESS;
}

void DecSetHostParaAddr(Uint32 coreIdx, PhysicalAddress baseAddr, PhysicalAddress paraBuffer)
{
	BYTE tempBuf[8]={0,};					// 64bit bus & endian
	Uint32 val;

	val = paraBuffer;
	tempBuf[0] = 0;
	tempBuf[1] = 0;
	tempBuf[2] = 0;
	tempBuf[3] = 0;
	tempBuf[4] = (val >> 24) & 0xff;
	tempBuf[5] = (val >> 16) & 0xff;
	tempBuf[6] = (val >> 8) & 0xff;
	tempBuf[7] = (val >> 0) & 0xff;
	VpuWriteMem(coreIdx, baseAddr, (BYTE *)tempBuf, 8, VDI_BIG_ENDIAN);
}


RetCode CheckEncInstanceValidity(EncHandle handle)
{
	CodecInst * pCodecInst;
	RetCode ret;

	pCodecInst = handle;
	ret = CheckInstanceValidity(pCodecInst);
	if (ret != RETCODE_SUCCESS) {
		return RETCODE_INVALID_HANDLE;
	}
	if (!pCodecInst->inUse) {
		return RETCODE_INVALID_HANDLE;
	}

	if (pCodecInst->codecMode != MP4_ENC && 
		pCodecInst->codecMode != AVC_ENC) {
		return RETCODE_INVALID_HANDLE;
	}
	return RETCODE_SUCCESS;
}



RetCode CheckEncOpenParam(EncOpenParam * pop)
{
	int picWidth;
	int picHeight;

	if (pop == 0) {
		return RETCODE_INVALID_PARAM;
	}
	picWidth = pop->picWidth;
	picHeight = pop->picHeight;

	if (pop->bitstreamBuffer % 8) { 
		return RETCODE_INVALID_PARAM;
	}
	if (pop->bitstreamBufferSize % 1024 || pop->bitstreamBufferSize < 1024) {
		return RETCODE_INVALID_PARAM;
	}

	if (pop->bitstreamFormat != STD_MPEG4 &&
			pop->bitstreamFormat != STD_H263 &&
			pop->bitstreamFormat != STD_AVC) {
		return RETCODE_INVALID_PARAM;
	}
	if (pop->frameRateInfo == 0) {
		return RETCODE_INVALID_PARAM;
	}
	{
		if (pop->bitRate > 32767 || pop->bitRate < 0) {
			return RETCODE_INVALID_PARAM;
		}
	}
	if (pop->bitRate !=0 && pop->initialDelay > 32767) {
		return RETCODE_INVALID_PARAM;
	}
	if (pop->bitRate !=0 && pop->initialDelay != 0 && pop->vbvBufferSize < 0) {
		return RETCODE_INVALID_PARAM;
	}
	if (pop->frameSkipDisable != 0 && pop->frameSkipDisable != 1) {
		return RETCODE_INVALID_PARAM;
	}


	if (pop->sliceMode.sliceMode != 0 && pop->sliceMode.sliceMode != 1) {
		return RETCODE_INVALID_PARAM;
	}
	if (pop->sliceMode.sliceMode == 1) {
		if (pop->sliceMode.sliceSizeMode != 0 && pop->sliceMode.sliceSizeMode != 1) {
			return RETCODE_INVALID_PARAM;
		}
		if (pop->sliceMode.sliceSizeMode == 1 && pop->sliceMode.sliceSize == 0 ) {
			return RETCODE_INVALID_PARAM;
		}
	}
	if (pop->intraRefresh < 0) {
		return RETCODE_INVALID_PARAM;
	}
	if (pop->intraCostWeight < 0 || pop->intraCostWeight >= 65535) {
		return RETCODE_INVALID_PARAM;
	}

	if (pop->bitstreamFormat == STD_MPEG4) {
		EncMp4Param * param = &pop->EncStdParam.mp4Param;
		if (param->mp4DataPartitionEnable != 0 && param->mp4DataPartitionEnable != 1) {
			return RETCODE_INVALID_PARAM;
		}
		if (param->mp4DataPartitionEnable == 1) {
			if (param->mp4ReversibleVlcEnable != 0 && param->mp4ReversibleVlcEnable != 1) {
				return RETCODE_INVALID_PARAM;
			}
		}
		if (param->mp4IntraDcVlcThr < 0 || 7 < param->mp4IntraDcVlcThr) {
			return RETCODE_INVALID_PARAM;
		}

		if (picWidth < MIN_ENC_PIC_WIDTH || picWidth > MAX_ENC_PIC_WIDTH ) {
			return RETCODE_INVALID_PARAM;
		}

		if (picHeight < MIN_ENC_PIC_HEIGHT) {
			return RETCODE_INVALID_PARAM;
		}
	}
	else if (pop->bitstreamFormat == STD_H263) {
		EncH263Param * param = &pop->EncStdParam.h263Param;
		Uint32 frameRateInc, frameRateRes;
		if (param->h263AnnexJEnable != 0 && param->h263AnnexJEnable != 1) {
			return RETCODE_INVALID_PARAM;
		}
		if (param->h263AnnexKEnable != 0 && param->h263AnnexKEnable != 1) {
			return RETCODE_INVALID_PARAM;
		}
		if (param->h263AnnexTEnable != 0 && param->h263AnnexTEnable != 1) {
			return RETCODE_INVALID_PARAM;
		}

		if (picWidth < MIN_ENC_PIC_WIDTH || picWidth > MAX_ENC_PIC_WIDTH ) {
			return RETCODE_INVALID_PARAM;
		}
		if (picHeight < MIN_ENC_PIC_HEIGHT) {
			return RETCODE_INVALID_PARAM;
		}
		frameRateInc = ((pop->frameRateInfo>>16) &0xFFFF) + 1;
		frameRateRes = pop->frameRateInfo & 0xFFFF;
		if( (frameRateRes/frameRateInc) <15 )
			return RETCODE_INVALID_PARAM;

	}
	else if (pop->bitstreamFormat == STD_AVC) {
		EncAvcParam * param = &pop->EncStdParam.avcParam;
		if (param->constrainedIntraPredFlag != 0 && param->constrainedIntraPredFlag != 1) {
			return RETCODE_INVALID_PARAM;
		}
		if (param->disableDeblk != 0 && param->disableDeblk != 1 && param->disableDeblk != 2) {
			return RETCODE_INVALID_PARAM;
		}
		if (param->deblkFilterOffsetAlpha < -6 || 6 < param->deblkFilterOffsetAlpha) {
			return RETCODE_INVALID_PARAM;
		}
		if (param->deblkFilterOffsetBeta < -6 || 6 < param->deblkFilterOffsetBeta) {
			return RETCODE_INVALID_PARAM;
		}
		if (param->chromaQpOffset < -12 || 12 < param->chromaQpOffset) {
			return RETCODE_INVALID_PARAM;
		}
		if (param->audEnable != 0 && param->audEnable != 1) {
			return RETCODE_INVALID_PARAM;
		}
		if (param->frameCroppingFlag != 0 &&param->frameCroppingFlag != 1) {
			return RETCODE_INVALID_PARAM;
		}
		if (param->frameCropLeft & 0x01 ||
			param->frameCropRight & 0x01 ||
			param->frameCropTop & 0x01 ||
			param->frameCropBottom & 0x01) {
			return RETCODE_INVALID_PARAM;
		}
		if (picWidth < MIN_ENC_PIC_WIDTH || picWidth > MAX_ENC_PIC_WIDTH ) {
			return RETCODE_INVALID_PARAM;
		}
		if (picHeight < MIN_ENC_PIC_HEIGHT) {
			return RETCODE_INVALID_PARAM;
		}
	}

	if (pop->coreIdx > MAX_VPU_CORE_NUM) {
		return RETCODE_INVALID_PARAM;
	}

	if (pop->prpYuvFormat == YUV_FORMAT_NV12 ||
		pop->prpYuvFormat == YUV_FORMAT_NV21 ||
		pop->prpYuvFormat == YUV_FORMAT_NV16 ||
		pop->prpYuvFormat == YUV_FORMAT_NV61) {
			if (pop->cbcrInterleave == 0) {
				return RETCODE_INVALID_PARAM;
			}
	}
	else {
		if (pop->cbcrInterleave == 1) {
			return RETCODE_INVALID_PARAM;
		}
	}

	if(pop->prpYuvFormat == YUV_FORMAT_NV21 || pop->prpYuvFormat == YUV_FORMAT_NV61) {
		if (pop->nv21 == 0) {
			return RETCODE_INVALID_PARAM;
		}
	}
	else {
		if (pop->nv21 == 1) {
			return RETCODE_INVALID_PARAM;
		}
	}

	return RETCODE_SUCCESS;
}


RetCode CheckEncParam(EncHandle handle, EncParam * param)
{
	CodecInst *pCodecInst;
	EncInfo *pEncInfo;
	int i;

	pCodecInst = handle;
	pEncInfo = &pCodecInst->CodecInfo.encInfo;

	if (param == 0) {
		return RETCODE_INVALID_PARAM;
	}
	if (param->skipPicture != 0 && param->skipPicture != 1) {
		return RETCODE_INVALID_PARAM;
	}
	if (param->skipPicture == 0) {
		if (param->sourceFrame == 0) {
			return RETCODE_INVALID_FRAME_BUFFER;
		}
		if (param->forceIPicture != 0 && param->forceIPicture != 1) {
			return RETCODE_INVALID_PARAM;
		}
	}
	if (pEncInfo->openParam.bitRate == 0) { // no rate control
		if (pCodecInst->codecMode == MP4_ENC) {
			if (param->quantParam < 1 || param->quantParam > 31) {
				return RETCODE_INVALID_PARAM;
			}
		}
		else { // AVC_ENC
			if (param->quantParam < 0 || param->quantParam > 51) {
				return RETCODE_INVALID_PARAM;
			}
		}
	}
	if (pEncInfo->ringBufferEnable == 0) {
		if (param->picStreamBufferAddr % 8 || param->picStreamBufferSize == 0) {
			return RETCODE_INVALID_PARAM;
		}
	}
	for (i=0; i<pEncInfo->numFrameBuffers; i++) {
		if (pEncInfo->frameBufPool[i].myIndex == param->sourceFrame->myIndex) {
			return RETCODE_INVALID_PARAM;
		}
	}
	return RETCODE_SUCCESS;
}


/**
 * GetEncHeader() 
 *  1. Generate encoder header bitstream
 * @param handle         : encoder handle
 * @param encHeaderParam : encoder header parameter (buffer, size, type)
 * @return none
 */
RetCode GetEncHeader(EncHandle handle, EncHeaderParam * encHeaderParam)
{

	CodecInst * pCodecInst;
	EncInfo * pEncInfo;
	EncOpenParam *encOP;
	PhysicalAddress rdPtr;
	PhysicalAddress wrPtr;
	int flag=0;
	int val = 0;

	pCodecInst = handle;
	pEncInfo = &pCodecInst->CodecInfo.encInfo;
	encOP = &(pEncInfo->openParam);

	EnterLock(pCodecInst->coreIdx);

	SetPendingInst(pCodecInst->coreIdx, pCodecInst);

	if (pEncInfo->ringBufferEnable == 0) {

		if (pEncInfo->lineBufIntEn)
			val |= (0x1<<6);
		val |= (0x1<<5);
		val |= (0x1<<4);

	} 
	else {
		val |= (0x1<<3);
	}
	val |= pEncInfo->openParam.streamEndian;
	VpuWriteReg(pCodecInst->coreIdx, BIT_BIT_STREAM_CTRL, val);

	if (pEncInfo->ringBufferEnable == 0) {
		VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_HEADER_BB_START, encHeaderParam->buf);
		VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_HEADER_BB_SIZE, encHeaderParam->size/1024);
	}

	if ((encHeaderParam->headerType == SPS_RBSP || encHeaderParam->headerType == SPS_RBSP_MVC) && 
        pEncInfo->openParam.bitstreamFormat == STD_AVC) {
		Uint32 CropV, CropH;
		if (encOP->EncStdParam.avcParam.frameCroppingFlag == 1) {
			flag = 1;
			CropH = encOP->EncStdParam.avcParam.frameCropLeft << 16;
			CropH |= encOP->EncStdParam.avcParam.frameCropRight;
			CropV = encOP->EncStdParam.avcParam.frameCropTop << 16;
			CropV |= encOP->EncStdParam.avcParam.frameCropBottom;
			VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_HEADER_FRAME_CROP_H, CropH);
			VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_HEADER_FRAME_CROP_V, CropV);
		}
	}
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_HEADER_CODE, encHeaderParam->headerType | (flag << 3)); // 0: SPS, 1: PPS

	VpuWriteReg(pCodecInst->coreIdx, pEncInfo->streamRdPtrRegAddr, pEncInfo->streamRdPtr);
	VpuWriteReg(pCodecInst->coreIdx, pEncInfo->streamWrPtrRegAddr, pEncInfo->streamWrPtr);

	BitIssueCommand(pCodecInst->coreIdx, pCodecInst, ENCODE_HEADER);
	if (vdi_wait_vpu_busy(pCodecInst->coreIdx, VPU_BUSY_CHECK_TIMEOUT, BIT_BUSY_FLAG) == -1) {
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, ENCODE_HEADER, 2);
		SetPendingInst(pCodecInst->coreIdx, 0);
		LeaveLock(pCodecInst->coreIdx);
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, ENCODE_HEADER, 0);

	if (pEncInfo->ringBufferEnable == 0) {

		rdPtr = encHeaderParam->buf;
		wrPtr = VpuReadReg(pCodecInst->coreIdx, pEncInfo->streamWrPtrRegAddr);
		{
			encHeaderParam->size = wrPtr - rdPtr;
		}
	}
	else {
		rdPtr = VpuReadReg(pCodecInst->coreIdx, pEncInfo->streamRdPtrRegAddr);
		wrPtr = VpuReadReg(pCodecInst->coreIdx, pEncInfo->streamWrPtrRegAddr);
		encHeaderParam->buf = rdPtr;
		encHeaderParam->size       = wrPtr - rdPtr;
	}

	pEncInfo->streamWrPtr = wrPtr;
	pEncInfo->streamRdPtr = rdPtr;
	SetPendingInst(pCodecInst->coreIdx, 0);
	LeaveLock(pCodecInst->coreIdx);
	return RETCODE_SUCCESS;

}

/**
 * EncParaSet() 
 *  1. Setting encoder header option
 *  2. Get RBSP format header in PARA_BUF
 * @param handle      : encoder handle
 * @param paraSetType : encoder header type >> SPS: 0, PPS: 1, VOS: 1, VO: 2, VOL: 0
 * @return none
 */
RetCode EncParaSet(EncHandle handle, int paraSetType)
{
	CodecInst * pCodecInst;
	EncInfo * pEncInfo;
	int flag = 0;
	int encHeaderCode = paraSetType;
	EncOpenParam *encOP;

	pCodecInst = handle;
	pEncInfo = &pCodecInst->CodecInfo.encInfo;
	encOP = &(pEncInfo->openParam);

	EnterLock(pCodecInst->coreIdx);

	SetPendingInst(pCodecInst->coreIdx, pCodecInst);

	if( paraSetType == 0 && pEncInfo->openParam.bitstreamFormat == STD_AVC) {
		Uint32 CropV, CropH;

		if (encOP->EncStdParam.avcParam.frameCroppingFlag == 1) {
			flag = 1;
			CropH = encOP->EncStdParam.avcParam.frameCropLeft << 16;
			CropH |= encOP->EncStdParam.avcParam.frameCropRight;
			CropV = encOP->EncStdParam.avcParam.frameCropTop << 16;
			CropV |= encOP->EncStdParam.avcParam.frameCropBottom;
			VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_HEADER_FRAME_CROP_H, CropH );
			VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_HEADER_FRAME_CROP_V, CropV );
		}
	}
	encHeaderCode = paraSetType| (flag<<2); //paraSetType>> SPS: 0, PPS: 1, VOS: 1, VO: 2, VOL: 0

	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARA_SET_TYPE, encHeaderCode); 


	BitIssueCommand(pCodecInst->coreIdx, pCodecInst, ENC_PARA_SET);
	if (vdi_wait_vpu_busy(pCodecInst->coreIdx, VPU_BUSY_CHECK_TIMEOUT, BIT_BUSY_FLAG) == -1) {
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, ENC_PARA_SET, 2);
		SetPendingInst(pCodecInst->coreIdx, 0);
		LeaveLock(pCodecInst->coreIdx);
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, ENC_PARA_SET, 0);

	SetPendingInst(pCodecInst->coreIdx, 0);
	LeaveLock(pCodecInst->coreIdx);
	return RETCODE_SUCCESS;
}



RetCode SetGopNumber(EncHandle handle, Uint32 *pGopNumber)
{
	CodecInst * pCodecInst;
	int data =0;
	Uint32 gopNumber = *pGopNumber;

	pCodecInst = handle;
	
	data = 1;

	EnterLock(pCodecInst->coreIdx);

	SetPendingInst(pCodecInst->coreIdx, pCodecInst);
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARAM_CHANGE_ENABLE, data);
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARAM_CHANGE_GOP_NUM, gopNumber);

	BitIssueCommand(pCodecInst->coreIdx, pCodecInst, RC_CHANGE_PARAMETER);
	if (vdi_wait_vpu_busy(pCodecInst->coreIdx, VPU_BUSY_CHECK_TIMEOUT, BIT_BUSY_FLAG) == -1) {
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 2);
		SetPendingInst(pCodecInst->coreIdx, 0);
		LeaveLock(pCodecInst->coreIdx);
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 0);
	SetPendingInst(pCodecInst->coreIdx, 0);
	LeaveLock(pCodecInst->coreIdx);
	return RETCODE_SUCCESS;
}

RetCode SetIntraQp(EncHandle handle, Uint32 *pIntraQp)
{
	CodecInst * pCodecInst;
	int data =0;
	Uint32 intraQp = *pIntraQp;

	pCodecInst = handle;


	data = 1<<1;

	EnterLock(pCodecInst->coreIdx);

	SetPendingInst(pCodecInst->coreIdx, pCodecInst);
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARAM_CHANGE_ENABLE, data);
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARAM_CHANGE_INTRA_QP, intraQp);

	BitIssueCommand(pCodecInst->coreIdx, pCodecInst, RC_CHANGE_PARAMETER);
	if (vdi_wait_vpu_busy(pCodecInst->coreIdx, VPU_BUSY_CHECK_TIMEOUT, BIT_BUSY_FLAG) == -1) {
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 2);
		SetPendingInst(pCodecInst->coreIdx, 0);
		LeaveLock(pCodecInst->coreIdx);
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 0);
	SetPendingInst(pCodecInst->coreIdx, 0);
	LeaveLock(pCodecInst->coreIdx);
	return RETCODE_SUCCESS;
}

RetCode SetBitrate(EncHandle handle, Uint32 *pBitrate)
{
	CodecInst * pCodecInst;
	int data =0;
	Uint32 bitrate = *pBitrate;

	pCodecInst = handle;

	data = 1<<2;

	EnterLock(pCodecInst->coreIdx);

	SetPendingInst(pCodecInst->coreIdx, pCodecInst);
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARAM_CHANGE_ENABLE, data);
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARAM_CHANGE_BITRATE, bitrate);

	BitIssueCommand(pCodecInst->coreIdx, pCodecInst, RC_CHANGE_PARAMETER);
	if (vdi_wait_vpu_busy(pCodecInst->coreIdx, VPU_BUSY_CHECK_TIMEOUT, BIT_BUSY_FLAG) == -1) {
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 2);
		SetPendingInst(pCodecInst->coreIdx, 0);
		LeaveLock(pCodecInst->coreIdx);
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 0);
	SetPendingInst(pCodecInst->coreIdx, 0);
	LeaveLock(pCodecInst->coreIdx);
	return RETCODE_SUCCESS;
}

RetCode SetFramerate(EncHandle handle, Uint32 *pFramerate)
{
	CodecInst * pCodecInst;
	int data =0;
	Uint32 frameRate = *pFramerate;


	pCodecInst = handle;


	data = 1<<3;

	EnterLock(pCodecInst->coreIdx);
	SetPendingInst(pCodecInst->coreIdx, pCodecInst);
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARAM_CHANGE_ENABLE, data);
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARAM_CHANGE_F_RATE, frameRate);

	BitIssueCommand(pCodecInst->coreIdx, pCodecInst, RC_CHANGE_PARAMETER);
	if (vdi_wait_vpu_busy(pCodecInst->coreIdx, VPU_BUSY_CHECK_TIMEOUT, BIT_BUSY_FLAG) == -1) {
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 2);
		SetPendingInst(pCodecInst->coreIdx, 0);
		LeaveLock(pCodecInst->coreIdx);
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 0);
	SetPendingInst(pCodecInst->coreIdx, 0);
	LeaveLock(pCodecInst->coreIdx);
	return RETCODE_SUCCESS;
}

RetCode SetIntraRefreshNum(EncHandle handle, Uint32 *pIntraRefreshNum)
{
	CodecInst * pCodecInst;
	Uint32 intraRefreshNum = *pIntraRefreshNum;
	int data = 0;


	pCodecInst = handle;

	data = 1<<4;

	EnterLock(pCodecInst->coreIdx);
	SetPendingInst(pCodecInst->coreIdx, pCodecInst);
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARAM_CHANGE_ENABLE, data);
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARAM_CHANGE_INTRA_REFRESH, intraRefreshNum);

	BitIssueCommand(pCodecInst->coreIdx, pCodecInst, RC_CHANGE_PARAMETER);
	if (vdi_wait_vpu_busy(pCodecInst->coreIdx, VPU_BUSY_CHECK_TIMEOUT, BIT_BUSY_FLAG) == -1) {
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 2);
		SetPendingInst(pCodecInst->coreIdx, 0);
		LeaveLock(pCodecInst->coreIdx);
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 0);
	SetPendingInst(pCodecInst->coreIdx, 0);
	LeaveLock(pCodecInst->coreIdx);
	return RETCODE_SUCCESS;
}

RetCode SetSliceMode(EncHandle handle, EncSliceMode *pSliceMode)
{
	CodecInst * pCodecInst;
	Uint32 data = 0;
	int data2 = 0;

	pCodecInst = handle;

	EnterLock(pCodecInst->coreIdx);
	SetPendingInst(pCodecInst->coreIdx, pCodecInst);
	data = pSliceMode->sliceSize<<2 | pSliceMode->sliceSizeMode<<1 | pSliceMode->sliceMode;
	data2 = 1<<5;



	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARAM_CHANGE_ENABLE, data2);
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARAM_CHANGE_SLICE_MODE, data);

	BitIssueCommand(pCodecInst->coreIdx, pCodecInst, RC_CHANGE_PARAMETER);
	if (vdi_wait_vpu_busy(pCodecInst->coreIdx, VPU_BUSY_CHECK_TIMEOUT, BIT_BUSY_FLAG) == -1) {
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 2);
		SetPendingInst(pCodecInst->coreIdx, 0);
		LeaveLock(pCodecInst->coreIdx);
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 0);

	SetPendingInst(pCodecInst->coreIdx, 0);
	LeaveLock(pCodecInst->coreIdx);

	return RETCODE_SUCCESS;
}

RetCode SetHecMode(EncHandle handle, int mode)
{
	CodecInst * pCodecInst;
	Uint32 HecMode = mode;
	int data = 0;


	pCodecInst = handle;


	data = 1 << 6;

	EnterLock(pCodecInst->coreIdx);
	SetPendingInst(pCodecInst->coreIdx, pCodecInst);
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARAM_CHANGE_ENABLE, data);
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARAM_CHANGE_HEC_MODE, HecMode);

	BitIssueCommand(pCodecInst->coreIdx, pCodecInst, RC_CHANGE_PARAMETER);
	if (vdi_wait_vpu_busy(pCodecInst->coreIdx, VPU_BUSY_CHECK_TIMEOUT, BIT_BUSY_FLAG) == -1) {
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 2);
		SetPendingInst(pCodecInst->coreIdx, 0);
		LeaveLock(pCodecInst->coreIdx);
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 0);
	SetPendingInst(pCodecInst->coreIdx, 0);
	LeaveLock(pCodecInst->coreIdx);
	return RETCODE_SUCCESS;
}




void EncSetHostParaAddr(Uint32 coreIdx, PhysicalAddress baseAddr, PhysicalAddress paraAddr)
{
	BYTE tempBuf[8]={0,};					// 64bit bus & endian
	Uint32 val;

	val =  paraAddr;
	tempBuf[0] = 0;
	tempBuf[1] = 0;
	tempBuf[2] = 0;
	tempBuf[3] = 0;
	tempBuf[4] = (val >> 24) & 0xff;
	tempBuf[5] = (val >> 16) & 0xff;
	tempBuf[6] = (val >> 8) & 0xff;
	tempBuf[7] = (val >> 0) & 0xff;
	VpuWriteMem(coreIdx, baseAddr, (BYTE *)tempBuf, 8, VDI_BIG_ENDIAN);
}


RetCode EnterDispFlagLock(Uint32 coreIdx)
{
    vdi_disp_lock(coreIdx);
    return RETCODE_SUCCESS;
}

RetCode LeaveDispFlagLock(Uint32 coreIdx)
{
    vdi_disp_unlock(coreIdx);
    return RETCODE_SUCCESS;
}

RetCode EnterLock(Uint32 coreIdx)
{
	vdi_lock(coreIdx);
	SetClockGate(coreIdx, 1);
	return RETCODE_SUCCESS;
}

RetCode LeaveLock(Uint32 coreIdx)
{
	SetClockGate(coreIdx, 0);
	vdi_unlock(coreIdx);
	return RETCODE_SUCCESS;
}

RetCode SetClockGate(Uint32 coreIdx, Uint32 on)
{
	vdi_set_clock_gate(coreIdx, on);

	return RETCODE_SUCCESS;
}

void SetPendingInst(Uint32 coreIdx, CodecInst *inst)
{
	vpu_instance_pool_t *vip;

	vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
	if (!vip)
		return;

	vip->pendingInst = inst;

	if (inst)
		vip->pendingInstIdxPlus1 = (inst->instIndex+1);
	else
		vip->pendingInstIdxPlus1 = 0;
}

void ClearPendingInst(Uint32 coreIdx)
{
	vpu_instance_pool_t *vip;

	vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
	if (!vip)
		return;

	if(vip->pendingInst) {
		vip->pendingInst = 0;
		vip->pendingInstIdxPlus1 = 0;
	}
}

CodecInst *GetPendingInst(Uint32 coreIdx)
{
	vpu_instance_pool_t *vip;
	int pendingInstIdx;

	vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
	if (!vip)
		return NULL;

	pendingInstIdx = vip->pendingInstIdxPlus1-1;
	if (pendingInstIdx < 0)
		return NULL;
	if (pendingInstIdx > MAX_NUM_INSTANCE)
		return NULL;


	return (CodecInst *)vip->codecInstPool[pendingInstIdx];
}



int GetLowDelayOutput(CodecInst *pCodecInst, DecOutputInfo *info)
{
	Uint32		val  = 0;
	Uint32		val2 = 0;
	Rect        rectInfo;
	DecInfo * pDecInfo;

	pDecInfo = &pCodecInst->CodecInfo.decInfo;

	info->indexFrameDisplay = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_DISPLAY_IDX);
	info->indexFrameDecoded = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_DECODED_IDX);
	if (!pDecInfo->reorderEnable)
		info->indexFrameDisplay = info->indexFrameDecoded;

	val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_SIZE); // decoding picture size
	info->decPicWidth  = (val>>16) & 0xFFFF;
	info->decPicHeight = (val) & 0xFFFF;


	if (info->indexFrameDecoded >= 0 && info->indexFrameDecoded < MAX_GDI_IDX)
	{
		//default value
		rectInfo.left   = 0;
		rectInfo.right  = info->decPicWidth;
		rectInfo.top    = 0;
		rectInfo.bottom = info->decPicHeight;

		if (pCodecInst->codecMode == AVC_DEC || pCodecInst->codecMode == MP2_DEC)
		{
			val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_CROP_LEFT_RIGHT);				// frame crop information(left, right)
			val2 = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_CROP_TOP_BOTTOM);			// frame crop information(top, bottom)

			if (val == (Uint32)-1 || val == 0)
			{
				rectInfo.left   = 0;
				rectInfo.right  = info->decPicWidth;
			} 
			else
			{
				rectInfo.left    = ((val>>16) & 0xFFFF);
				rectInfo.right   = info->decPicWidth - (val&0xFFFF);
			}
			if (val2 == (Uint32)-1 || val2 == 0)
			{
				rectInfo.top    = 0;
				rectInfo.bottom = info->decPicHeight;
			}
			else
			{
				rectInfo.top     = ((val2>>16) & 0xFFFF);
				rectInfo.bottom	= info->decPicHeight - (val2&0xFFFF);
			}
		}

		info->rcDecoded.left   =  pDecInfo->decOutInfo[info->indexFrameDecoded].rcDecoded.left   = rectInfo.left;
		info->rcDecoded.right  =  pDecInfo->decOutInfo[info->indexFrameDecoded].rcDecoded.right  = rectInfo.right;
		info->rcDecoded.top    =  pDecInfo->decOutInfo[info->indexFrameDecoded].rcDecoded.top    = rectInfo.top;   
		info->rcDecoded.bottom =  pDecInfo->decOutInfo[info->indexFrameDecoded].rcDecoded.bottom = rectInfo.bottom;
	}
	else
	{
		info->rcDecoded.left   = 0;  
		info->rcDecoded.right  = info->decPicWidth;
		info->rcDecoded.top    = 0;
		info->rcDecoded.bottom = info->decPicHeight;
	}

	if (info->indexFrameDisplay >= 0 && info->indexFrameDisplay < MAX_GDI_IDX)
	{
		if (pCodecInst->codecMode == VC1_DEC) // vc1 rotates decoded frame buffer region. the other std rotated whole frame buffer region.
		{
			if (pDecInfo->rotationEnable && (pDecInfo->rotationAngle==90 || pDecInfo->rotationAngle==270))
			{
				info->rcDisplay.left   = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.top;
				info->rcDisplay.right  = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.bottom;
				info->rcDisplay.top    = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.left;
				info->rcDisplay.bottom = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.right;
			}
			else
			{
				info->rcDisplay.left   = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.left;
				info->rcDisplay.right  = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.right;
				info->rcDisplay.top    = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.top;
				info->rcDisplay.bottom = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.bottom;
			}
		}
		else
		{
			if (pDecInfo->rotationEnable)
			{
				switch(pDecInfo->rotationAngle)
				{
				case 90:
				case 270:
					info->rcDisplay.left   = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.top;
					info->rcDisplay.right  = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.bottom;
					info->rcDisplay.top    = pDecInfo->rotatorOutput.height - pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.right;
					info->rcDisplay.bottom = pDecInfo->rotatorOutput.height - pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.left;
					break;
				default:
					info->rcDisplay.left   = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.left;
					info->rcDisplay.right  = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.right;
					info->rcDisplay.top    = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.top;
					info->rcDisplay.bottom = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.bottom;
					break;
				}

			}
			else
			{
				info->rcDisplay.left   = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.left;
				info->rcDisplay.right  = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.right;
				info->rcDisplay.top    = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.top;
				info->rcDisplay.bottom = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.bottom;
			}
		}

		if (info->indexFrameDisplay == info->indexFrameDecoded)
		{
			info->dispPicWidth =  info->decPicWidth;
			info->dispPicHeight = info->decPicHeight;
		}
		else
		{
			info->dispPicWidth = pDecInfo->decOutInfo[info->indexFrameDisplay].decPicWidth;
			info->dispPicHeight = pDecInfo->decOutInfo[info->indexFrameDisplay].decPicHeight;
		}
	}
	else
	{
		info->rcDisplay.left   = 0;  
		info->rcDisplay.right  = 0;  
		info->rcDisplay.top    = 0;  
		info->rcDisplay.bottom = 0;

		info->dispPicWidth = 0;
		info->dispPicHeight = 0;
	}

	val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_TYPE);
	info->interlacedFrame	= (val >> 18) & 0x1;
	info->topFieldFirst     = (val >> 21) & 0x0001;	// TopFieldFirst[21]
	if (info->interlacedFrame) {
		info->picTypeFirst = (val & 0x38) >> 3;	  // pic_type of 1st field
		info->picType = val & 0xff;

	}
	else {
		info->picTypeFirst   = PIC_TYPE_MAX;	// no meaning
		info->picType = val & 0xff;
	}

	info->pictureStructure  = (val >> 19) & 0x0003;	// MbAffFlag[17], FieldPicFlag[16]
	info->repeatFirstField  = (val >> 22) & 0x0001;
	info->progressiveFrame  = (val >> 23) & 0x0003;

	if( pCodecInst->codecMode == AVC_DEC)
	{
		info->nalRefIdc = (val >> 7) & 0x03;
		info->decFrameInfo = (val >> 15) & 0x0001;
		info->picStrPresent = (val >> 27) & 0x0001;
		info->picTimingStruct = (val >> 28) & 0x000f;
        info->decFrameInfo  = (val >> 16) & 0x0003;
        if (info->indexFrameDisplay>=0)
        {
            if (info->indexFrameDisplay == info->indexFrameDecoded)
                info->avcNpfFieldInfo = info->decFrameInfo;
            else
                info->avcNpfFieldInfo = pDecInfo->decOutInfo[info->indexFrameDisplay].decFrameInfo;
        }
		val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_HRD_INFO);
		info->avcHrdInfo.cpbMinus1 = val>>2;
		info->avcHrdInfo.vclHrdParamFlag = (val>>1)&1;
		info->avcHrdInfo.nalHrdParamFlag = val&1;

		val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_VUI_INFO);
		info->avcVuiInfo.fixedFrameRateFlag    = val &1;
		info->avcVuiInfo.timingInfoPresent     = (val>>1) & 0x01;
		info->avcVuiInfo.chromaLocBotField     = (val>>2) & 0x07;
		info->avcVuiInfo.chromaLocTopField     = (val>>5) & 0x07;
		info->avcVuiInfo.chromaLocInfoPresent  = (val>>8) & 0x01;
		info->avcVuiInfo.colorPrimaries        = (val>>16) & 0xff;
		info->avcVuiInfo.colorDescPresent      = (val>>24) & 0x01;
		info->avcVuiInfo.isExtSAR              = (val>>25) & 0x01;
		info->avcVuiInfo.vidFullRange          = (val>>26) & 0x01;
		info->avcVuiInfo.vidFormat             = (val>>27) & 0x07;
		info->avcVuiInfo.vidSigTypePresent     = (val>>30) & 0x01;
		info->avcVuiInfo.vuiParamPresent       = (val>>31) & 0x01;
		val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_VUI_PIC_STRUCT);
		info->avcVuiInfo.vuiPicStructPresent = (val & 0x1);
		info->avcVuiInfo.vuiPicStruct = (val>>1);
	}

	info->fRateNumerator    = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_FRATE_NR); //Frame rate, Aspect ratio can be changed frame by frame.
	info->fRateDenominator  = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_FRATE_DR);
	if (pCodecInst->codecMode == AVC_DEC && info->fRateDenominator > 0)
		info->fRateDenominator  *= 2;

	info->aspectRateInfo = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_ASPECT);

	// User Data
	if (pDecInfo->userDataEnable) {
		int userDataNum;
		int userDataSize;
		BYTE tempBuf[8] = {0,};

		VpuReadMem(pCodecInst->coreIdx, pDecInfo->userDataBufAddr + 0, tempBuf, 8,  VPU_USER_DATA_ENDIAN); 

		val =	((tempBuf[0]<<24) & 0xFF000000) |
			((tempBuf[1]<<16) & 0x00FF0000) |
			((tempBuf[2]<< 8) & 0x0000FF00) |
			((tempBuf[3]<< 0) & 0x000000FF);

		userDataNum = (val >> 16) & 0xFFFF;
		userDataSize = (val >> 0) & 0xFFFF;
		if (userDataNum == 0)
			userDataSize = 0;

		info->decOutputExtData.userDataNum = userDataNum;
		info->decOutputExtData.userDataSize = userDataSize;

		val =	((tempBuf[4]<<24) & 0xFF000000) |
			((tempBuf[5]<<16) & 0x00FF0000) |
			((tempBuf[6]<< 8) & 0x0000FF00) |
			((tempBuf[7]<< 0) & 0x000000FF);

		if (userDataNum == 0)
			info->decOutputExtData.userDataBufFull = 0;
		else
			info->decOutputExtData.userDataBufFull = (val >> 16) & 0xFFFF;

		info->decOutputExtData.activeFormat = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_ATSC_USER_DATA_INFO)&0xf;
	}

	info->numOfErrMBs = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_ERR_MB);

	val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_SUCCESS);
	info->decodingSuccess = val;
	info->sequenceChanged = ((val>>20) & 0x1);
	info->streamEndFlag = ((pDecInfo->streamEndflag>>2) & 0x01);


	if (info->indexFrameDisplay >= 0 && info->indexFrameDisplay < pDecInfo->numFrameBuffers) {
		info->dispFrame = pDecInfo->frameBufPool[info->indexFrameDisplay];
	}

	if (pDecInfo->deringEnable || pDecInfo->mirrorEnable || pDecInfo->rotationEnable) {
		info->dispFrame = pDecInfo->rotatorOutput;
	}

	if (pCodecInst->codecMode == AVC_DEC && pCodecInst->codecModeAux == AVC_AUX_MVC)
	{
		int val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_MVC_REPORT);
		info->mvcPicInfo.viewIdxDisplay = (val>>0) & 1;
		info->mvcPicInfo.viewIdxDecoded = (val>>1) & 1;
	}

	if (pCodecInst->codecMode == AVC_DEC)
	{
		val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_AVC_FPA_SEI0);

		if (val == (Uint32)-1) {
			info->avcFpaSei.exist = 0;
		} 
		else {
			info->avcFpaSei.exist = 1;
			info->avcFpaSei.framePackingArrangementId = val;

			val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_AVC_FPA_SEI1);
			info->avcFpaSei.contentInterpretationType               = val&0x3F; // [5:0]
			info->avcFpaSei.framePackingArrangementType             = (val >> 6)&0x7F; // [12:6]
			info->avcFpaSei.framePackingArrangementExtensionFlag    = (val >> 13)&0x01; // [13]
			info->avcFpaSei.frame1SelfContainedFlag                 = (val >> 14)&0x01; // [14]
			info->avcFpaSei.frame0SelfContainedFlag                 = (val >> 15)&0x01; // [15]
			info->avcFpaSei.currentFrameIsFrame0Flag                = (val >> 16)&0x01; // [16]
			info->avcFpaSei.fieldViewsFlag                          = (val >> 17)&0x01; // [17]
			info->avcFpaSei.frame0FlippedFlag                       = (val >> 18)&0x01; // [18]
			info->avcFpaSei.spatialFlippingFlag                     = (val >> 19)&0x01; // [19]
			info->avcFpaSei.quincunxSamplingFlag                    = (val >> 20)&0x01; // [20]
			info->avcFpaSei.framePackingArrangementCancelFlag       = (val >> 21)&0x01; // [21]

			val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_AVC_FPA_SEI2);
			info->avcFpaSei.framePackingArrangementRepetitionPeriod = val&0x7FFF;       // [14:0]
			info->avcFpaSei.frame1GridPositionY                     = (val >> 16)&0x0F; // [19:16]
			info->avcFpaSei.frame1GridPositionX                     = (val >> 20)&0x0F; // [23:20]
			info->avcFpaSei.frame0GridPositionY                     = (val >> 24)&0x0F; // [27:24]
			info->avcFpaSei.frame0GridPositionX                     = (val >> 28)&0x0F; // [31:28]
		}  

		info->avcPocTop = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_POC_TOP);
		info->avcPocBot = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_POC_BOT);

		if (info->interlacedFrame)
		{
			if (info->avcPocTop > info->avcPocBot) {
				info->avcPocPic = info->avcPocBot;
			} else {
				info->avcPocPic = info->avcPocTop;
			}
		}
		else
			info->avcPocPic = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_POC);
	}

	//pDecInfo->streamRdPtr //NA
	pDecInfo->frameDisplayFlag = VpuReadReg(pCodecInst->coreIdx, pDecInfo->frameDisplayFlagRegAddr);
	//info->consumedByte //NA
	//info->notSufficientSliceBuffer; // NA
	//info->notSufficientPsBuffer;  // NA
	//info->bytePosFrameStart //NA
	//info->bytePosFrameEnd   //NA
	//info->rdPtr //NA
	//info->wrPtr //NA
	//info->frameCycle  //NA
	//Vp8ScaleInfo vp8ScaleInfo; //NA
	//Vp8PicInfo vp8PicInfo; //NA
	//MvcPicInfo mvcPicInfo; ////NA
	//info->wprotErrReason; avcVuiInfo
	//PhysicalAddress wprotErrAddress; avcVuiInfo

	// Report Information
	//info->frameDct; //NA
	//info->progressiveSequence; //NA
	//info->mp4TimeIncrement; //NA
	//info->mp4ModuloTimeBase; //NA


	return 1;
}



RetCode AllocateFrameBufferArray(int coreIdx, FrameBuffer *frambufArray, vpu_buffer_t *pvbFrame, int mapType, int interleave, int framebufFormat, int num, int stride, int memHeight, int gdiIndex, int fbType, PhysicalAddress tiledBaseAddr, DRAMConfig *pDramCfg)
{
	int chr_hscale, chr_vscale;
	int size_dpb_lum, size_dpb_chr, size_dpb_all;
	int alloc_by_user;
	int chr_size_y, chr_size_x;
	int i, width, height;
	PhysicalAddress addrY;
	int size_dpb_lum_4k, size_dpb_chr_4k, size_dpb_all_4k;


	alloc_by_user = (frambufArray[0].bufCb == (PhysicalAddress)-1 && frambufArray[0].bufCr == (PhysicalAddress)-1);

	height = memHeight;
	width = stride;
    
    switch (framebufFormat)
	{
	case FORMAT_420:
		height = (memHeight+1)/2*2;
		width = (stride+1)/2*2;
		break;
	case FORMAT_224:
		height = (memHeight+1)/2*2;
		break;
	case FORMAT_422:
		width = (stride+1)/2*2;
		break;
	case FORMAT_444:
		break;
	case FORMAT_400:
		height = (memHeight+1)/2*2;
		width = (stride+1)/2*2;
		break;
	default:
		return RETCODE_INVALID_PARAM;
	}

	chr_hscale = framebufFormat == FORMAT_420 || framebufFormat == FORMAT_422 ? 2 : 1;
	chr_vscale = framebufFormat == FORMAT_420 || framebufFormat == FORMAT_224 ? 2 : 1;

	if (mapType == LINEAR_FRAME_MAP) { // AllocateFrameBuffer
		chr_size_y = (height/chr_hscale); 
		chr_size_x = (width/chr_vscale);

		size_dpb_lum   = width * height;
		size_dpb_chr   = chr_size_y * chr_size_x;
		size_dpb_all   = size_dpb_lum + (size_dpb_chr*2);

		pvbFrame->size     = size_dpb_all*num;
		if (alloc_by_user) {
		}
		else {
			if (vdi_allocate_dma_memory(coreIdx, pvbFrame)<0)
				return RETCODE_INSUFFICIENT_RESOURCE;
		}

		addrY = pvbFrame->phys_addr;
		for (i=0; i<num; i++)
		{
			if (alloc_by_user)
				addrY = frambufArray[i].bufY;

			frambufArray[i].myIndex = i+gdiIndex;
			frambufArray[i].mapType = mapType;
			frambufArray[i].height = memHeight;
			frambufArray[i].bufY    = addrY;
			frambufArray[i].bufCb   = frambufArray[i].bufY + size_dpb_lum;
			if (!interleave)
				frambufArray[i].bufCr = frambufArray[i].bufY + size_dpb_lum + size_dpb_chr;
            frambufArray[i].stride         = width;
            frambufArray[i].cbcrInterleave = interleave;
			frambufArray[i].sourceLBurstEn = 0;
			frambufArray[i].scalerFlag = SCALER_BUFFER_NOT_USE;
			frambufArray[i].scalerIdxPlus1 = 0;
			addrY += size_dpb_all;
		}

	}
	else if (mapType == TILED_FRAME_MB_RASTER_MAP || mapType == TILED_FIELD_MB_RASTER_MAP) { //AllocateMBRasterTiled
		chr_size_x = width/chr_hscale;
		chr_size_y = height/chr_hscale;
		size_dpb_lum   = width * height;
		size_dpb_chr   = chr_size_y * chr_size_x;

		// aligned to 8192*2 (0x4000) for top/bot field
		// use upper 20bit address only
		size_dpb_lum_4k	=  ((size_dpb_lum + 16383)/16384)*16384;
		size_dpb_chr_4k	=  ((size_dpb_chr + 16383)/16384)*16384;
		size_dpb_all_4k =  size_dpb_lum_4k + 2*size_dpb_chr_4k;

		if (mapType == TILED_FIELD_MB_RASTER_MAP)
		{
			size_dpb_lum_4k = ((size_dpb_lum_4k+(0x8000-1))&~(0x8000-1));
			size_dpb_chr_4k = ((size_dpb_chr_4k+(0x8000-1))&~(0x8000-1));
			size_dpb_all_4k =  size_dpb_lum_4k + 2*size_dpb_chr_4k;
		}

		pvbFrame->size     = size_dpb_all_4k*num;
		if (alloc_by_user) {
		}
		else 
		{
			if (vdi_allocate_dma_memory(coreIdx, pvbFrame)<0)
				return RETCODE_INSUFFICIENT_RESOURCE;
		}

		addrY = ((pvbFrame->phys_addr+(16384-1))&~(16384-1));

		for (i=0; i<num; i++)
		{
			int  lum_top_base;
			int  lum_bot_base;
			int  chr_top_base;
			int  chr_bot_base;
			//-------------------------------------
			// use tiled map
			//-------------------------------------
			if (alloc_by_user)
				addrY = ((frambufArray[i].bufY+(16384-1))&~(16384-1));

			lum_top_base = addrY;
			lum_bot_base = addrY + size_dpb_lum_4k/2;
			chr_top_base = addrY + size_dpb_lum_4k;
			chr_bot_base = addrY + size_dpb_lum_4k + size_dpb_chr_4k; // cbcr is interleaved

			lum_top_base = (lum_top_base>>12) & 0xfffff;
			lum_bot_base = (lum_bot_base>>12) & 0xfffff;
			chr_top_base = (chr_top_base>>12) & 0xfffff;
			chr_bot_base = (chr_bot_base>>12) & 0xfffff;

			frambufArray[i].myIndex = i+gdiIndex;
			frambufArray[i].mapType = mapType;
			frambufArray[i].height = memHeight;
			// {AddrY,AddrCb,AddrCr} = {lum_top_base[31:12],chr_top_base[31:12],lum_bot_base[31:12],chr_bot_base[31:12],16'b0}
			// AddrY  = {lum_top_base[31:12],chr_top_base[31:20]} : 20 + 12 bit
			// AddrCb = {chr_top_base[19:12],lum_bot_base[31:20],chr_bot_base[31:28]} : 8 + 20 + 4 bit
			// AddrCr = {chr_bot_base[27:12],16'b0} : 16 bit
			frambufArray[i].bufY  = ( lum_top_base           << 12) | (chr_top_base >> 8);
			frambufArray[i].bufCb = ((chr_top_base & 0xff  ) << 24) | (lum_bot_base << 4) | (chr_bot_base >> 16);
			frambufArray[i].bufCr = ((chr_bot_base & 0xffff) << 16) ;
            frambufArray[i].stride         = width;
            frambufArray[i].cbcrInterleave = interleave;
			frambufArray[i].sourceLBurstEn = 0;
			addrY += size_dpb_all_4k;
		}

	}
	else {
		PhysicalAddress addrYRas;
		int ChrSizeYField;

		int  VerSizePerRas,HorSizePerRas,RasLowBitsForHor; 
		int  ChrFieldRasSize,ChrFrameRasSize,LumFieldRasSize,LumFrameRasSize;
		int  LumRasTop,LumRasBot,ChrRasTop,ChrRasBot;     

		if (mapType == TILED_FRAME_V_MAP) {
			if (pDramCfg->casBit == 9 && pDramCfg->bankBit == 2 && pDramCfg->rasBit == 13)	// CNN setting 
			{
				VerSizePerRas = 64;
				HorSizePerRas = 256;
				RasLowBitsForHor = 3;
			}
			else if(pDramCfg->casBit == 10 && pDramCfg->bankBit == 3 && pDramCfg->rasBit == 13)
			{
				VerSizePerRas = 64;
				HorSizePerRas = 512;
				RasLowBitsForHor = 2;
			}
			else
				return RETCODE_INVALID_PARAM;
		} else if (mapType == TILED_FRAME_H_MAP) {
			if (pDramCfg->casBit == 9 && pDramCfg->bankBit == 2 && pDramCfg->rasBit == 13)	// CNN setting 
			{
				VerSizePerRas = 64;
				HorSizePerRas = 256;
				RasLowBitsForHor = 3;
			}
			else if(pDramCfg->casBit == 10 && pDramCfg->bankBit == 3 && pDramCfg->rasBit == 13)
			{
				VerSizePerRas = 64;
				HorSizePerRas = 512;
				RasLowBitsForHor = 2;
			}
			else
				return RETCODE_INVALID_PARAM;
		} else if (mapType == TILED_FIELD_V_MAP) {
			if (pDramCfg->casBit == 9 && pDramCfg->bankBit == 2 && pDramCfg->rasBit == 13)	// CNN setting 
			{
				VerSizePerRas = 64;
				HorSizePerRas = 256;
				RasLowBitsForHor = 3;
			}
			else if(pDramCfg->casBit == 10 && pDramCfg->bankBit == 3 && pDramCfg->rasBit == 13)
			{
				VerSizePerRas = 64;
				HorSizePerRas = 512;
				RasLowBitsForHor = 2;
			}
			else
				return RETCODE_INVALID_PARAM;
		} else {
			if (pDramCfg->casBit == 9 && pDramCfg->bankBit == 2 && pDramCfg->rasBit == 13)	// CNN setting 
			{
				VerSizePerRas = 64;
				HorSizePerRas = 256;
				RasLowBitsForHor = 3;
			}
			else if(pDramCfg->casBit == 10 && pDramCfg->bankBit == 3 && pDramCfg->rasBit == 13)
			{
				VerSizePerRas = 64;
				HorSizePerRas = 512;
				RasLowBitsForHor = 2;
			}
			else
				return RETCODE_INVALID_PARAM;
		} 

		UNREFERENCED_PARAMETER(HorSizePerRas);

		chr_size_y = height/chr_hscale;
		ChrSizeYField = ((chr_size_y+1)>>1);
		ChrFieldRasSize = ((ChrSizeYField + (VerSizePerRas-1))/VerSizePerRas) << RasLowBitsForHor;
		ChrFrameRasSize = ChrFieldRasSize *2;
		LumFieldRasSize = ChrFrameRasSize;
		LumFrameRasSize = LumFieldRasSize *2;

		size_dpb_all         = (LumFrameRasSize + ChrFrameRasSize) << (pDramCfg->bankBit+pDramCfg->casBit+pDramCfg->busBit);
		pvbFrame->size     = size_dpb_all*num;
		if (alloc_by_user)
		{
		}
		else 
		{
			if (vdi_allocate_dma_memory(coreIdx, pvbFrame)<0)
				return RETCODE_INSUFFICIENT_RESOURCE;
		}

		if (fbType == FB_TYPE_PPU) {
			addrY = pvbFrame->phys_addr - tiledBaseAddr;
		}
		else {
			addrY = 0;
			tiledBaseAddr = pvbFrame->phys_addr;
		}
	
		for (i=0; i<num; i++)
		{
			if (alloc_by_user) {
				addrY = frambufArray[i].bufY - tiledBaseAddr;
			}

			// align base_addr to RAS boundary
			addrYRas  = (addrY + ((1<<(pDramCfg->bankBit+pDramCfg->casBit+pDramCfg->busBit))-1)) >> (pDramCfg->bankBit+pDramCfg->casBit+pDramCfg->busBit);
			// round up RAS lower 3(or 4) bits
			addrYRas  = ((addrYRas + ((1<<(RasLowBitsForHor))-1)) >> RasLowBitsForHor) << RasLowBitsForHor;

			LumRasTop = addrYRas;
			LumRasBot = LumRasTop  + LumFieldRasSize;
			ChrRasTop = LumRasTop  + LumFrameRasSize;
			ChrRasBot = ChrRasTop  + ChrFieldRasSize;

			frambufArray[i].myIndex = i+gdiIndex;
			frambufArray[i].mapType = mapType;
			frambufArray[i].height = memHeight;
			frambufArray[i].bufY  = (LumRasBot << 16) + LumRasTop;
			frambufArray[i].bufCb = (ChrRasBot << 16) + ChrRasTop;
            frambufArray[i].stride         = width;
            frambufArray[i].cbcrInterleave = interleave;
			frambufArray[i].sourceLBurstEn = 0;
			if (RasLowBitsForHor == 4)
				frambufArray[i].bufCr  = ((((ChrRasBot>>4)<<4) + 8) << 16) + (((ChrRasTop>>4)<<4) + 8);
			else if (RasLowBitsForHor == 3)
				frambufArray[i].bufCr  = ((((ChrRasBot>>3)<<3) + 4) << 16) + (((ChrRasTop>>3)<<3) + 4);
			else if (RasLowBitsForHor == 2)
				frambufArray[i].bufCr  = ((((ChrRasBot>>2)<<2) + 2) << 16) + (((ChrRasTop>>2)<<2) + 2);
			else if (RasLowBitsForHor == 1)
				frambufArray[i].bufCr  = ((((ChrRasBot>>1)<<1) + 1) << 16) + (((ChrRasTop>>1)<<1) + 1);
			else
				return RETCODE_INSUFFICIENT_RESOURCE; // Invalid RasLowBit value

			addrYRas = (addrYRas + LumFrameRasSize + ChrFrameRasSize);
			addrY    = ((addrYRas) << (pDramCfg->bankBit+pDramCfg->casBit+pDramCfg->busBit));
		}
	}



	return RETCODE_SUCCESS;
}




int ConfigSecAXI(Uint32 coreIdx, CodStd codStd, SecAxiInfo *sa, int width, int height, int profile)
{
	vpu_buffer_t vb;
	unsigned int offset;
	int MbNumX = ((width & 0xFFFF) + 15) / 16;
	int MbNumY = ((height & 0xFFFF) + 15) / 16;
	//int MbNumY = ((height & 0xFFFF) + 31) / 32; //field??


	if (vdi_get_sram_memory(coreIdx, &vb) < 0)
		return 0;

	if (!vb.size)
	{
		sa->bufSize = 0;
		sa->useBitEnable = 0;
		sa->useIpEnable = 0;
		sa->useDbkYEnable = 0;
		sa->useDbkCEnable = 0;
		sa->useOvlEnable = 0;
		sa->useMeEnable = 0;
		sa->useScalerEnable = 0;
		return 0;
	}

	offset = 0;
	//BIT
	if (sa->useBitEnable)
	{
		sa->useBitEnable = 1;
		sa->bufBitUse = vb.phys_addr + offset;

		switch (codStd) 
		{
		case STD_AVC:
			offset = offset + MbNumX * 128; 
			break; // AVC
		case STD_RV:
			offset = offset + MbNumX * 128;
			break;
		case STD_VC1:
			{
			int oneBTP;

			oneBTP  = (((MbNumX+15)/16) * MbNumY + 1) * 2;
			oneBTP  = (oneBTP%256) ? ((oneBTP/256)+1)*256 : oneBTP;

			offset = offset + oneBTP * 3;
			}
			offset = offset + MbNumX *  64;
			break;
		case STD_AVS:
			offset = offset + ((MbNumX + 3) & ~3) *  32; 
			break;
		case STD_MPEG2:
			offset = offset + MbNumX * 0; 
			break;
		case STD_VP8:
			offset = offset + MbNumX * 0; 
			break;
		default:
			offset = offset + MbNumX *  16; 
			break; // MPEG-4, Divx3
		}

		if (offset > vb.size)
		{
			sa->bufSize = 0;
			return 0;
		}	

	}

	//Intra Prediction, ACDC
	if (sa->useIpEnable)
	{
		sa->bufIpAcDcUse = vb.phys_addr + offset;
		sa->useIpEnable = 1;

		switch (codStd) 
		{
		case STD_AVC:
			offset = offset + MbNumX * 64; 
			break; // AVC
		case STD_RV:
			offset = offset + MbNumX * 64;
			break;
		case STD_VC1:
			offset = offset + MbNumX * 128;
			break;
		case STD_AVS:
			offset = offset + MbNumX * 64;
			break;
		case STD_MPEG2:
			offset = offset + MbNumX * 0; 
			break;
		case STD_VP8:
			offset = offset + MbNumX * 64; 
			break;
		default:
			offset = offset + MbNumX * 128; 
			break; // MPEG-4, Divx3
		}

		if (offset > vb.size)
		{
			sa->bufSize = 0;
			return 0;
		}
	}

	//Deblock Luma
	if (sa->useDbkYEnable)
	{
		sa->bufDbkYUse = vb.phys_addr + offset;
		sa->useDbkYEnable = 1;

		switch (codStd) 
		{
		case STD_AVC:
			offset = (profile==66/*AVC BP decoder*/ || profile==0/* AVC encoder */)? offset + (MbNumX * 64) : offset + (MbNumX * 128);
			break; // AVC
		case STD_RV:
			offset = offset + MbNumX * 128;
			break;
		case STD_VC1:
			offset = profile==2 ? offset + MbNumX * 256 : offset + MbNumX * 128;
			//#ifdef CODA7L 
			// sram size for Deblock Luma should be aligned 256
			offset = (offset + 255) & (~255);
			//#endif
			break;
		case STD_AVS:
			offset = offset + MbNumX * 64;
			break;
		case STD_MPEG2:
			offset = offset + MbNumX * 64; 
			break;
		case STD_VP8:
			offset = offset + MbNumX * 128; 
			break;
		case STD_H263:
			offset = offset + MbNumX * 64; 
			break;
		case STD_MPEG4:
			offset = offset + MbNumX * 64; 
			break;
		default:
			offset = offset + MbNumX * 128; 
			break;
		}

		if (offset > vb.size)
		{
			sa->bufSize = 0;
			return 0;
		}
	}
	//Deblock Chroma
	if (sa->useDbkCEnable)
	{
		sa->bufDbkCUse = vb.phys_addr + offset;
		sa->useDbkCEnable = 1;
		switch (codStd) 
		{
		case STD_AVC:
			offset = (profile==66/*AVC BP decoder*/ || profile==0/* AVC encoder */) ? offset+ (MbNumX * 64) : offset + (MbNumX * 128);
			break; // AVC
		case STD_RV:
			offset = offset + MbNumX * 128;
			break;
		case STD_VC1:
			offset = profile==2 ? offset + MbNumX * 256 : offset + MbNumX * 128;
			// sram size for Deblock Chroma should be aligned 256
			offset = (offset + 255) & (~255);
			break;
		case STD_AVS:
			offset = offset + MbNumX * 64;
			break;
		case STD_MPEG2:
			offset = offset + MbNumX * 64; 
			break;
		case STD_VP8:
			offset = offset + MbNumX * 128; 
			break;
        case STD_H263:
			offset = offset + MbNumX * 64; 
			break;
		case STD_MPEG4:
			offset = offset + MbNumX * 64; 
			break;
		default:
			offset = offset + MbNumX * 64; 
			break;
		}
		if (offset > vb.size)
		{
			sa->bufSize = 0;
			return 0;
		}
	}


	// check the buffer address which is 256 byte is available.
	if (((offset + 255) & (~255)) > vb.size)
	{
		sa->bufSize = 0;
		return 0;
	}


	//ME search ram
	if (sa->useMeEnable)
	{
		offset = (offset + 255) & (~255);
		sa->bufMeUse = vb.phys_addr + offset;
		sa->useMeEnable = 1;

		offset = offset + MbNumX*16*36+2048;

		if (offset > vb.size)
		{
			sa->bufSize = 0;
			return 0;
		}
	}
	//VC1 Overlap
	if (sa->useOvlEnable)
	{
		if (codStd != STD_VC1)
		{
			sa->useOvlEnable = 0;
		}
		else
		{
			sa->bufOvlUse = vb.phys_addr + offset;
			sa->useOvlEnable = 1;

			offset = offset + MbNumX *  80;

			if (offset > vb.size)
			{
				sa->bufSize = 0;
				return 0;
			}
		}
	}

	if (sa->useScalerEnable)
	{
		static int SecAxiMemScl[3];// = {0x1800,0x0800,0x2000}; 

		sa->bufMinipippenUse = vb.phys_addr + offset;

		SecAxiMemScl[0] = MbNumX * 16 * 3;
		SecAxiMemScl[1] = MbNumX * 16 * 1;
		if(codStd != STD_VC1)
		{
			SecAxiMemScl[2] = MbNumX * 16 * 4;
		}
		else	// VC1 case
		{
			SecAxiMemScl[2] = MbNumX * 16 * 16;
		}

		offset = offset + SecAxiMemScl[0] + SecAxiMemScl[1]+ SecAxiMemScl[2];

		if (offset > vb.size)
		{
			sa->bufSize = 0;
			return 0;
		}
	}

	sa->bufSize = offset;

	return 1;
}






const unsigned short cosnt_code[1024] = {
    0x0f1e, 0x261e, 0x0f00, 0x0b1d, 0x2820, 0x0d03, 0x0a1c, 0x2821,
    0x0e03, 0x091b, 0x2822, 0x0f03, 0x081a, 0x2623, 0x1104, 0x0719,
    0x2723, 0x1204, 0x0618, 0x2724, 0x1304, 0x0616, 0x2625, 0x1405,
    0x0515, 0x2626, 0x1505, 0x0c1f, 0x2920, 0x0c00, 0x0a1e, 0x2820,
    0x0d03, 0x091c, 0x2921, 0x0e03, 0x081b, 0x2922, 0x0f03, 0x081a,
    0x2823, 0x1003, 0x0719, 0x2724, 0x1104, 0x0617, 0x2726, 0x1204,
    0x0516, 0x2726, 0x1404, 0x0515, 0x2626, 0x1505, 0x0b1f, 0x2b20,
    0x0b00, 0x0817, 0x211a, 0x091d, 0x0819, 0x251d, 0x0b12, 0x0719,
    0x2721, 0x0d0b, 0x0719, 0x2822, 0x0e08, 0x0618, 0x2824, 0x1006,
    0x0617, 0x2825, 0x1105, 0x0515, 0x2826, 0x1305, 0x0514, 0x2727,
    0x1405, 0x0c1e, 0x2b1f, 0x0c00, 0x0b1b, 0x281e, 0x0c08, 0x0a1a,
    0x281f, 0x0d08, 0x0a19, 0x2721, 0x0e07, 0x0918, 0x2722, 0x0f07,
    0x0917, 0x2722, 0x1007, 0x0816, 0x2624, 0x1107, 0x0814, 0x2625,
    0x1207, 0x0813, 0x2525, 0x1308, 0x0b1f, 0x2c1f, 0x0b00, 0x091d,
    0x2b20, 0x0b04, 0x091c, 0x2a21, 0x0c04, 0x081a, 0x2b22, 0x0d04,
    0x0719, 0x2a24, 0x0e04, 0x0717, 0x2a25, 0x0f04, 0x0616, 0x2926,
    0x1104, 0x0615, 0x2826, 0x1205, 0x0513, 0x2828, 0x1305, 0x0820,
    0x3020, 0x0800, 0x071d, 0x3021, 0x0902, 0x061c, 0x2f23, 0x0a02,
    0x061a, 0x2f24, 0x0b02, 0x0518, 0x2f26, 0x0c02, 0x0417, 0x2e27,
    0x0e02, 0x0415, 0x2d29, 0x0f02, 0x0313, 0x2c2b, 0x1003, 0x0312,
    0x2b2b, 0x1203, 0x0520, 0x3620, 0x0500, 0x041e, 0x3622, 0x0600,
    0x031b, 0x3625, 0x0700, 0x0319, 0x3527, 0x0800, 0x0217, 0x3529,
    0x0900, 0x0215, 0x332a, 0x0b01, 0x0213, 0x322c, 0x0c01, 0x0111,
    0x312e, 0x0e01, 0x010f, 0x3030, 0x0f01, 0x001c, 0x481c, 0x0000,
    0x0018, 0x491f, 0x0000, 0x0015, 0x4723, 0x0100, 0x0012, 0x4627,
    0x0100, 0x000f, 0x442b, 0x0200, 0x000c, 0x422f, 0x0300, 0x000a,
    0x3f33, 0x0400, 0x0008, 0x3d36, 0x0500, 0x0006, 0x3a3a, 0x0600,
    0xff1a, 0x4e1a, 0xff00, 0xff16, 0x4e1e, 0xff00, 0xff12, 0x4d23,
    0xff00, 0xff0f, 0x4b27, 0x0000, 0xff0c, 0x492c, 0x0000, 0xff0a,
    0x472f, 0x0100, 0x0008, 0x4333, 0x0200, 0x0006, 0x3f38, 0x0300,
    0x0004, 0x3c3c, 0x0400, 0xff17, 0x5417, 0xff00, 0xff13, 0x541c,
    0xfe00, 0xff10, 0x5221, 0xfe00, 0xff0c, 0x5125, 0xff00, 0xff09,
    0x4e2b, 0xff00, 0xff07, 0x4b30, 0xff00, 0xff05, 0x4735, 0x0000,
    0x0003, 0x423a, 0x0100, 0x0002, 0x3e3e, 0x0200, 0xfe15, 0x5a15,
    0xfe00, 0xfe10, 0x5a1a, 0xfe00, 0xff0c, 0x581f, 0xfe00, 0xff09,
    0x5624, 0xfe00, 0xff06, 0x532b, 0xfd00, 0xff04, 0x4f30, 0xfe00,
    0x0002, 0x4a36, 0xfe00, 0x0000, 0x463c, 0xfe00, 0x00ff, 0x4141,
    0xff00, 0x2a2c, 0x2a00, 0x1d3b, 0x2305, 0x1a3b, 0x2605, 0x173b,
    0x2806, 0x153a, 0x2b06, 0x1239, 0x2d08, 0x1037, 0x3009, 0x0e36,
    0x320a, 0x0c34, 0x340c, 0x233a, 0x2300, 0x1d3b, 0x2305, 0x1a3c,
    0x2505, 0x173c, 0x2805, 0x153b, 0x2a06, 0x123a, 0x2d07, 0x1038,
    0x2f09, 0x0e36, 0x320a, 0x0c34, 0x340c, 0x213e, 0x2100, 0x1d3d,
    0x2204, 0x1a3d, 0x2504, 0x173d, 0x2705, 0x143c, 0x2a06, 0x123a,
    0x2d07, 0x1038, 0x3008, 0x0e36, 0x320a, 0x0c34, 0x340c, 0x2040,
    0x2000, 0x1c3e, 0x2204, 0x193e, 0x2504, 0x163d, 0x2805, 0x143c,
    0x2b05, 0x113b, 0x2d07, 0x0f39, 0x3008, 0x0d37, 0x3309, 0x0b35,
    0x350b, 0x1f42, 0x1f00, 0x1634, 0x1b1b, 0x1537, 0x2014, 0x1438,
    0x2410, 0x1238, 0x280e, 0x1038, 0x2c0c, 0x0f37, 0x2e0c, 0x0e35,
    0x310c, 0x0d33, 0x330d, 0x2040, 0x2000, 0x1a3a, 0x1f0d, 0x183a,
    0x220c, 0x163a, 0x240c, 0x1438, 0x280c, 0x1237, 0x2b0c, 0x1135,
    0x2d0d, 0x1034, 0x2f0d, 0x0e32, 0x320e, 0x1f41, 0x2000, 0x1b3d,
    0x2008, 0x183d, 0x2308, 0x163c, 0x2608, 0x143a, 0x2909, 0x1239,
    0x2c09, 0x1038, 0x2e0a, 0x0e36, 0x310b, 0x0d33, 0x330d, 0x115d,
    0x1200, 0x0e5c, 0x1600, 0x0a5b, 0x1b00, 0x0858, 0x2000, 0x0655,
    0x2500, 0x0450, 0x2c00, 0x034a, 0x3201, 0x0245, 0x3801, 0x013f,
    0x3f01, 0x1060, 0x1000, 0x0c60, 0x1400, 0x095e, 0x1900, 0x075b,
    0x1e00, 0x0557, 0x2400, 0x0352, 0x2b00, 0x024c, 0x3200, 0x0146,
    0x3900, 0x013f, 0x3f01, 0x0e64, 0x0e00, 0x0a64, 0x1200, 0x0762,
    0x1700, 0x055e, 0x1d00, 0x035a, 0x2300, 0x0254, 0x2a00, 0x014e,
    0x3100, 0x0147, 0x3800, 0x0040, 0x4000, 0x0c68, 0x0c00, 0x0868,
    0x1000, 0x0665, 0x1500, 0x0461, 0x1b00, 0x025d, 0x2100, 0x0157,
    0x2800, 0x0050, 0x3000, 0x0048, 0x3800, 0x0040, 0x4000, 0x7f01,
    0x7808, 0x7010, 0x6818, 0x6020, 0x5828, 0x5030, 0x4838, 0x4040,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
    0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190, 0xe190,
};

static unsigned char ScalerCoeff_c[(8+144)+8];


int  ScalerCoefDownload(Uint32 coreIdx, PhysicalAddress paraBuffer, unsigned int xratio, unsigned int yratio)
{
	int xindex      ;
	int yindex      ;
	int line_start  ;
	int line        ;
	int scaler_pos;
	int i;
	unsigned short ScalerCoeff[(8+144)/2];
	unsigned char * p_ScalerCoeff_c;

	// x
	if (xratio >= 16*256*256/3)
		xindex = 0;
	else if (xratio >= 16*256*256/4)
		xindex = 3-2;
	else if (xratio >= 16*256*256/5)
		xindex = 4-2;
	else if (xratio >= 16*256*256/6)
		xindex = 5-2;
	else if (xratio >= 16*256*256/7)
		xindex = 6-2;
	else if (xratio >= 16*256*256/8)
		xindex = 7-2;
	else if (xratio >= 16*256*256/9)
		xindex = 8-2;
	else if (xratio >= 16*256*256/10)
		xindex = 9-2;
	else if (xratio >= 16*256*256/11)
		xindex = 10-2;
	else
		xindex = 11-2;

	// y
	if (yratio >= 16*256*256/3)
		yindex = 2-2;
	else if (yratio >= 16*256*256/4)
		yindex = 3-2;
	else if (yratio >= 16*256*256/5)
		yindex = 4-2;
	else if (yratio >= 16*256*256/6)
		yindex = 5-2;
	else if (yratio >= 16*256*256/7)
		yindex = 6-2;
	else if (yratio >= 16*256*256/8)
		yindex = 7-2;
	else if (yratio >= 16*256*256/9)
		yindex = 8-2;
	else if (yratio >= 16*256*256/10)
		yindex = 9-2;
	else if (yratio >= 16*256*256/11)
		yindex = 10-2;
	else
		yindex = 11-2;

	ScalerCoeff[0] = xratio&0xffff;
	ScalerCoeff[1] = xratio>>16;
	ScalerCoeff[2] = yratio&0xffff;
	ScalerCoeff[3] = yratio>>16;


	scaler_pos  = 4;
	// luma hor 6-ta p
	line_start  = xindex*9*3 ; // 9*6 bytes each
	for (line = line_start ; line<line_start + 9*3; line=line+1) 
	{
		ScalerCoeff[scaler_pos] = cosnt_code[line];

		scaler_pos = scaler_pos + 1;
	}

	// luma ver 4-tap
	line_start  = 11*9*3 + yindex*9*2 ; // 9*4 bytes each
	for (line = line_start ; line<line_start + 9*2; line=line+1) 
	{
		ScalerCoeff[scaler_pos] = cosnt_code[line];

		scaler_pos = scaler_pos + 1;
	}


	// chroma hor 4-tap
	line_start  = 11*9*3 + xindex*9*2 ; // 9*4 bytes each
	for (line = line_start ; line<line_start + 9*2; line=line+1) 
	{
		ScalerCoeff[scaler_pos] = cosnt_code[line];

		scaler_pos = scaler_pos + 1;
	}


	// chroma ver 2-tap
	line_start  = 11*9*3 + 11*9*2 ; // 9*2 bytes each
	for (line = line_start ; line<line_start + 9; line=line+1) 
	{
		ScalerCoeff[scaler_pos] = cosnt_code[line];

		scaler_pos = scaler_pos + 1;
	}

	p_ScalerCoeff_c = (unsigned char*)(((intptr_t)ScalerCoeff_c+8)&~(intptr_t)0x7);

	for (i=0; i<(int)sizeof(ScalerCoeff); i+=2)
	{
		p_ScalerCoeff_c[i] = ScalerCoeff[i/2]>>8;
		p_ScalerCoeff_c[i+1]	= ScalerCoeff[i/2]&0xff;
	}

	/** put scaler coeff at the botton of working buffer */
	vdi_write_memory(coreIdx, paraBuffer + 0x1000, p_ScalerCoeff_c, sizeof(ScalerCoeff_c), VDI_BIG_ENDIAN);

	return 1;
}


