//--=========================================================================--
//  This implements some useful common functionalities 
//  for handling the register files used in Bellagio
//-----------------------------------------------------------------------------
//
//       This confidential and proprietary software may be used only
//     as authorized by a licensing agreement from Chips&Media Inc.
//     In the event of publication, the following notice is applicable:
//
//            (C) COPYRIGHT 2006 - 2015  CHIPS&MEDIA INC.
//                      ALL RIGHTS RESERVED
//
//       The entire notice above must be reproduced on all authorized
//       copies.
//
//--=========================================================================--


#ifndef OMX_VPU_INDEX_H_
#define OMX_VPU_INDEX_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <OMX_Index.h>

typedef enum OMX_VPU_INDEXTYPE {
    OMX_IndexParamVideoMSMpeg = OMX_IndexVendorStartUnused, //0x7F000000
	OMX_IndexParamNalStreamFormatSupported, /**< reference: OMX_NALSTREAMFORMATTYPE */
	OMX_IndexParamNalStreamFormat,          /**< reference: OMX_NALSTREAMFORMATTYPE */
	OMX_IndexParamNalStreamFormatSelect,    /**< reference: OMX_NALSTREAMFORMATTYPE */
	OMX_IndexParamVideoVC1,                 /**< reference: OMX_VIDEO_PARAM_VC1TYPE */
	OMX_IndexConfigVideoIntraPeriod,        /**< reference: OMX_VIDEO_INTRAPERIODTYPE */
	OMX_IndexConfigVideoIntraRefresh,       /**< reference: OMX_VIDEO_PARAM_INTRAREFRESHTYPE */
	OMX_IndexParamVideoVp8,                 /**< reference: OMX_VIDEO_PARAM_VP8TYPE */
	OMX_IndexConfigVideoVp8ReferenceFrame,  /**< reference: OMX_VIDEO_VP8REFERENCEFREAMETYPE */
	OMX_IndexConfigVideoVp8ReferenceFrameType,  /**< reference: OMX_VIDEO_VP8REFERENCEFRAMEINFOTYPE */
	OMX_IndexParamVideoAVS,
} OMX_VPU_INDEXTYPE;




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OMX_VPU_INDEX_H_ */
