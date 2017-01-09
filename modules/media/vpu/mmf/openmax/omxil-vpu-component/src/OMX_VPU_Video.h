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

#ifndef OMX_VPU_VIDEO_H_
#define OMX_VPU_VIDEO_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <OMX_Video.h>

typedef enum OMX_VPU_VIDEO_CODINGTYPE {
    OMX_VIDEO_CodingMSMPEG = OMX_VIDEO_CodingVendorStartUnused, /**< DIVX3.11 */
#ifdef CNM_SPRD_PLATFORM
    /* note: already defined in /frameworks/native/include/media/openmax/OMX_Video.h */
#else
    OMX_VIDEO_CodingVC1,        /**< VC1 */
    OMX_VIDEO_CodingFLV1,       /**< Sorenson's H.263 */
#endif
    OMX_VIDEO_CodingDIVX,       /**< DIVX */
    //OMX_VIDEO_CodingHEVC,       /**< HEVC */
#ifdef CNM_SPRD_PLATFORM
    /* note: already defined in /frameworks/native/include/media/openmax/OMX_Video.h */
#else
    OMX_VIDEO_CodingFFMPEG,     /**< FFMPEG */
#endif
    OMX_VIDEO_CodingAVS,        /**< AVS */
} OMX_VPU_VIDEO_CODINGTYPE;

typedef enum OMX_VPU_COLORFORMATTYPE {
    OMX_VPU_COLOR_FormatYUV444Planar = OMX_COLOR_FormatMax,
} OMX_VPU_COLORFORMATTYPE;


typedef enum OMX_VIDEO_VPU_WMVFORMATTYPE {
    OMX_VIDEO_WMVFormatVC1 = OMX_VIDEO_WMFFormatVendorStartUnused, /**< VC1 AP format */
} OMX_VIDEO_VPU_WMVFORMATTYPE;

/**
 * MS-MPEG4 Versions
 */
typedef enum OMX_VIDEO_MSMPEGFORMATTYPE {
    OMX_VIDEO_MSMPEGFormatUnused = 0x00,        /**< Microsoft MPEG-4 version 1 or Microsoft MPEG-4 version 2 */ 
    OMX_VIDEO_MSMPEGFormat3,        /**< Microsoft MPEG-4 version 3 */
} OMX_VIDEO_MSMPEGFORMATTYPE;

/**
 * MSMPEG Params
 *
 * STRUCT MEMBERS:
 *  nSize      : Size of the structure in bytes
 *  nVersion   : OMX specification version information
 *  nPortIndex : Port that this structure applies to
 *  eFormat    : Version of MSMPEG stream / data
 */
typedef struct OMX_VIDEO_PARAM_MSMPEGTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_VIDEO_MSMPEGFORMATTYPE eFormat;
} OMX_VIDEO_PARAM_MSMPEGTYPE;

#if ANDROID_PLATFORM_SDK_VERSION >= 19
#else
typedef enum OMX_VIDEO_VP8PROFILETYPE {
	OMX_VIDEO_VP8ProfileMain                = 0x01,        
	OMX_VIDEO_VP8ProfileUnknown             = 0x6EFFFFFF,
	OMX_VIDEO_VP8ProfileKhronosExtensions   = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
	OMX_VIDEO_VP8ProfileVendorStartUnused   = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
	OMX_VIDEO_VP8ProfileMax                 = 0x7FFFFFFF  
} OMX_VIDEO_VP8PROFILETYPE;

typedef enum OMX_VIDEO_VP8LEVELTYPE {
	OMX_VIDEO_VP8Level_Version0             = 0x01,
	OMX_VIDEO_VP8Level_Version1             = 0x02,
	OMX_VIDEO_VP8Level_Version2             = 0x04,
	OMX_VIDEO_VP8Level_Version3             = 0x08,
	OMX_VIDEO_VP8LevelUnknown               = 0x6EFFFFFF,
	OMX_VIDEO_VP8LevelKhronosExtensions     = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
	OMX_VIDEO_VP8LevelVendorStartUnused     = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
	OMX_VIDEO_VP8LevelMax                   = 0x7FFFFFFF  
} OMX_VIDEO_VP8LEVELTYPE;

typedef struct OMX_VIDEO_PARAM_VP8TYPE {
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32 nPortIndex;
	OMX_VIDEO_VP8PROFILETYPE eProfile;
	OMX_VIDEO_VP8LEVELTYPE eLevel;
	OMX_U32 nDCTPartitions;
	OMX_BOOL bErrorResilientMode;
} OMX_VIDEO_PARAM_VP8TYPE;

typedef struct OMX_VIDEO_VP8REFERENCEFRAMETYPE {
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32 nPortIndex;
	OMX_BOOL nPreviousFrameRefresh;
	OMX_BOOL bGoldenFrameRefresh;
	OMX_BOOL bAlternateFrameRefresh;
	OMX_BOOL bUsePreviousFrame;
	OMX_BOOL bUseGoldenFrame;
	OMX_BOOL bUseAlternateFrame;
} OMX_VIDEO_VP8REFERENCEFRAMETYPE ;

typedef struct OMX_VIDEO_VP8REFERENCEFRAMEINFOTYPE {
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32 nPortIndex;
	OMX_BOOL bIsIntraFrame;
	OMX_BOOL bIsGoldenOrAlternateFrame;
} OMX_VIDEO_VP8REFERENCEFRAMEINFOTYPE ;
#endif

#ifdef ANDROID
typedef struct AndroidNativeBuffersParams {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
} AndroidNativeBuffersParams;
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OMX_VPU_VIDEO_H_ */
