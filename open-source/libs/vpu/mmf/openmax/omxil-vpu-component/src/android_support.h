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


#ifndef _OMX_VPU_ANDROID_SUPPORT_H
#define _OMX_VPU_ANDROID_SUPPORT_H


#ifdef CNM_SPRD_PLATFORM
#else
#undef LOG_TAG
#define LOG_TAG "VPUOMX"
#endif

#ifdef __cplusplus
extern "C" {
#endif
#include <OMX_Types.h>
#include <OMX_Component.h>
#include <OMX_Core.h>
#include <OMX_Index.h>
#include <OMX_VPU_Index.h>

#ifdef ANDROID
#include <system/window.h>
#endif


typedef enum OMX_VPU_ANDORID_INDEXTYPE {
#define STR_INDEX_PARAM_ENABLE_ANDROID_NATIVE_BUFFER "OMX.google.android.index.enableAndroidNativeBuffers"
		OMX_IndexParamEnableAndroidBuffers_CM = OMX_IndexParamVideoAVS + 1,  /**< reference: Android Native Window */
#define STR_INDEX_PARAM_GET_ANDROID_NATIVE_BUFFER "OMX.google.android.index.getAndroidNativeBufferUsage"
		OMX_IndexParamGetAndroidNativeBuffer_CM,
#define STR_INDEX_PARAM_USE_ANDROID_NATIVE_BUFFER "OMX.google.android.index.useAndroidNativeBuffer"
		OMX_IndexParamUseAndroidNativeBuffer,
		/* for Android Store Metadata Inbuffer */
#define STR_INDEX_PARAM_STORE_METADATA_BUFFER "OMX.google.android.index.storeMetaDataInBuffers"
		OMX_IndexParamStoreMetaDataBuffer_CM,	/**< reference: Android Store Metadata Inbuffer */
#define STR_INDEX_PARAM_THUMBNAIL_MODE "OMX.sprd.index.ThumbnailMode"
		OMX_IndexConfigThumbnailMode_CM,
} OMX_VPU_ANDORID_INDEXTYPE;


typedef struct {
    void *YPhyAddr;                     // [IN/OUT] physical address of Y
    void *CPhyAddr;                     // [IN/OUT] physical address of CbCr
    void *YVirAddr;                     // [IN/OUT] virtual address of Y
    void *CVirAddr;                     // [IN/OUT] virtual address of CbCr
    int YSize;                          // [IN/OUT] input size of Y data
    int CSize;                          // [IN/OUT] input size of CbCr data
} BUFFER_ADDRESS_INFO;


#ifdef CNM_SPRD_PLATFORM
/* Added by h002555928 2014-03-27 for Hisilicon platform adaptation start */
#define GET_VIRTUAL_ADDRESS 0
#define GET_PHYSICAL_ADDRESS 1
/* Added by h002555928 2014-03-27 for Hisilicon platform adaptation end */
#endif //CNM_SPRD_PLATFORM

#ifdef CNM_SPRD_PLATFORM
//#define WORKAROUND_RGB2YUV_CSC_BY_SW	// if this is needed. should add LOCAL_STATIC_LIBRARIES := libyuv_static and LOCAL_C_INCLUDES += $(TOP)/external/libyuv/files/include
#else
//#define WORKAROUND_RGB2YUV_CSC_BY_SW	// if this is needed. should add LOCAL_STATIC_LIBRARIES := libyuv_static and LOCAL_C_INCLUDES += $(TOP)/external/libyuv/files/include
#endif


#define LOCK_MODE_TO_GET_VIRTUAL_ADDRESS 0
#define LOCK_MODE_TO_GET_PHYSICAL_ADDRESS 1
OMX_ERRORTYPE checkEnableAndroidBuffersHeader(OMX_PTR ComponentParameterStructure);
OMX_ERRORTYPE checkEnableAndroidBuffersPort(OMX_PTR ComponentParameterStructure, OMX_U32 *portIndex);
OMX_BOOL enableAndroidBuffer(OMX_PTR ComponentParameterStructure);


OMX_ERRORTYPE checkUseAndroidNativeBufferHeader(OMX_PTR ComponentParameterStructure);
OMX_ERRORTYPE checkUseAndroidNativeBufferPort(OMX_PTR ComponentParameterStructure, OMX_U32 *portIndex);
OMX_ERRORTYPE useAndroidNativeBuffer(OMX_PTR ComponentParameterStructure, OMX_BUFFERHEADERTYPE **pNativeBufHeaderType, OMX_COLOR_FORMATTYPE eColorFormat);


OMX_ERRORTYPE checkGetAndroidNativeBufferHeader(OMX_PTR ComponentParameterStructure);
OMX_ERRORTYPE checkGetAndroidNativeBufferPort(OMX_PTR ComponentParameterStructure, OMX_U32 *portIndex);
OMX_ERRORTYPE GetAndroidNativeBuffer(OMX_PTR ComponentParameterStructure);
OMX_U32 lockAndroidNativeBuffer(OMX_BUFFERHEADERTYPE *pNativeBufHeaderType, int stride, int height, OMX_U32 mode, void *pAddrs[]);
OMX_U32 unlockAndroidNativeBuffer(OMX_BUFFERHEADERTYPE *pNativeBufHeaderType);
OMX_BOOL getAndroidNativeBufferInfo(OMX_BUFFERHEADERTYPE *pNativeBufHeaderType, int *pFormat, int *pStride, int *pWidth, int *pHeight);
int getNativeBufferSize(OMX_COLOR_FORMATTYPE colorFormat, int native_buffer_format, int stride, int height);
/** @@ Modified code by SPRD
 * In order to store and get output buffer index in queue, we add two functions:
 * SetBufferIndex: set the buffer index to buffer header
 * GetBufferIndex: get the buffer index from buffer header
**/
void SetBufferIndex(OMX_BUFFERHEADERTYPE *pBuffer, int index);
int GetBufferIndex(OMX_BUFFERHEADERTYPE *pBuffer);
void SetPicType(OMX_BUFFERHEADERTYPE *pBuffer, int picType);
int GetPicType(OMX_BUFFERHEADERTYPE *pBuffer);
void ClearPicType(OMX_BUFFERHEADERTYPE *pBuffer);
OMX_ERRORTYPE AllocateIONBuffer(int inSize, void **bufHandle, unsigned long *phyAddr, void **virAddr, size_t *bufSize);
OMX_ERRORTYPE FreeIONBuffer(void *bufHandle, unsigned long phyAddr, size_t size);
// Add by Alan Wang
OMX_ERRORTYPE getIOMMUPhyAddr(OMX_BUFFERHEADERTYPE *pNativeBufHeaderType, unsigned long *pAddrs);
OMX_ERRORTYPE freeIOMMUAddr(OMX_BUFFERHEADERTYPE *pNativeBufHeaderType);

OMX_BOOL getAndroidNativeBufferHandleInfo(buffer_handle_t handle, int *pFormat, int *pWidth, int *pHeight, int *pSize);

#ifdef WORKAROUND_RGB2YUV_CSC_BY_SW
OMX_BOOL ConvertRgbToYuvbySW(OMX_BYTE pYuvData, OMX_BYTE pRgbData, OMX_U32 grallocFormat, OMX_U32 grallocWidth,  OMX_U32 grallocHeight);
#endif

OMX_ERRORTYPE checkStoreMetaDataBufferPort(OMX_PTR ComponentParameterStructure, OMX_U32 *portIndex);
OMX_ERRORTYPE checkStoreMetaDataBufferHeader(OMX_PTR ComponentParameterStructure);
OMX_ERRORTYPE StoreMetaDataBuffer(OMX_PTR ComponentParameterStructure, OMX_BOOL *pbEnable);
OMX_U32 lockAndroidBufferHandle(buffer_handle_t handle, int width, int height, OMX_U32 mode, void *pAddrs[]);
OMX_U32 unLockAndroidBufferHandle(buffer_handle_t handle);

#ifdef CNM_SPRD_PLATFORM

/* Modified by h00255928 2014-03-27 for Hisilicon platform adaptation start */
OMX_ERRORTYPE GetAndroidNativeBufferAddr(OMX_BUFFERHEADERTYPE **ppNativeBufHeaderType, OMX_U32 mode, void **pAddrs);
OMX_ERRORTYPE munmapAndroidNativeBuffer(OMX_BUFFERHEADERTYPE *pNativeBufHeaderType, void *pVirAddr);
void setNativeBufferInfo(OMX_BUFFERHEADERTYPE *pNativeBufHeaderType, int original_width, int original_height, int actual_width, int actual_height, int coded_width, int coded_height);
/* Modified by h00255928 2014-03-27 for Hisilicon platform adaptation end */
/*Added by s00216212 2014-6-23 for interface of private_handle_t*/
int getHandlePhys(buffer_handle_t bufHandle, unsigned int* phys);
// start, add by z00183533 y00251056, for cts encodedecotest virtualdisplay wifidisplay
int IsRGBA( buffer_handle_t bufHandle);
int getHandleVirualAddr(buffer_handle_t bufHandle, unsigned int* virt);

// end, add by z00183533 y00251056, for cts encodedecotest virtualdisplay wifidisplay
#endif //CNM_SPRD_PLATFORM



#ifdef __cplusplus
}
#endif

#endif

