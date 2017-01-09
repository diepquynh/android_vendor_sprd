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



#include <omxcore.h>
#include "android_support.h"

#include "MemoryHeapIon.h"
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferMapper.h>
#include <ui/Rect.h>
#include <HardwareAPI.h>
#include <hardware/hardware.h>
#include <MetadataBufferType.h>
#include <sys/types.h>


#include <gralloc_priv.h>
#include <system/graphics.h>
#include "ion_sprd.h"

#ifdef CNM_SPRD_PLATFORM
/* Added by h002555928 2014-03-27 for Hisilicon platform adaptation start */
#include <sys/types.h>
#include <sys/mman.h>
#include <cutils/native_handle.h>
/* Added by h002555928 2014-03-27 for Hisilicon platform adaptation end */
#include <PixelFormat.h>

#undef LOG_TAG
#define LOG_TAG "OmxVpuAndroidSupport"
#endif //CNM_SPRD_PLATFORM

#ifdef DEBUG
#undef DEBUG
#define DEBUG(n, fmt, args...) \
do { \
    if (n == DEB_LEV_ERR) \
    { \
        ALOGE("[TID:%d-%s-%d] " fmt, gettid(), __FUNCTION__, __LINE__, ##args); \
    } \
    else if (DEBUG_LEVEL & (n)) \
    { \
        ALOGI("[TID:%d-%s-%d] " fmt, gettid(), __FUNCTION__, __LINE__, ##args); \
    } \
} while (0)
#endif


using namespace android;

#define SPRD_ION_DEV "/dev/ion"

typedef enum {
	PIC_TYPE_I            = 0,
	PIC_TYPE_P            = 1,
	PIC_TYPE_B            = 2,
	PIC_TYPE_VC1_BI       = 2,
	PIC_TYPE_VC1_B        = 3,
	PIC_TYPE_D            = 3,    // D picture in mpeg2, and is only composed of DC codfficients
	PIC_TYPE_S            = 3,    // S picture in mpeg4, and is an acronym of Sprite. and used for GMC
	PIC_TYPE_VC1_P_SKIP   = 4,
	PIC_TYPE_MP4_P_SKIP_NOT_CODED = 4, // Not Coded P Picture at mpeg4 packed mode
    PIC_TYPE_IDR          = 5,    // H.264 IDR frame
    PIC_TYPE_MAX
};

typedef struct BufferPrivateStruct
{
    MemoryHeapIon* pMem;
    int bufferFd;
    unsigned long phyAddr;
    size_t bufferSize;
    int picType;
    int bufferIndex;
} BufferPrivateStruct;

static OMX_ERRORTYPE checkAndroidParamHeader(OMX_PTR header, OMX_U32 size) ;

OMX_ERRORTYPE checkEnableAndroidBuffersHeader(OMX_PTR ComponentParameterStructure)
{
	EnableAndroidNativeBuffersParams * pEnableAndroidNativeBuffersParams;

	pEnableAndroidNativeBuffersParams = (EnableAndroidNativeBuffersParams *)ComponentParameterStructure;

	return checkAndroidParamHeader(pEnableAndroidNativeBuffersParams, sizeof (EnableAndroidNativeBuffersParams));

}

OMX_ERRORTYPE checkEnableAndroidBuffersPort(OMX_PTR ComponentParameterStructure, OMX_U32 *portIndex)
{
	EnableAndroidNativeBuffersParams * pEnableAndroidNativeBuffersParams;

	pEnableAndroidNativeBuffersParams = (EnableAndroidNativeBuffersParams *)ComponentParameterStructure;

	*portIndex = pEnableAndroidNativeBuffersParams->nPortIndex;
	if (pEnableAndroidNativeBuffersParams->nPortIndex != 1)	// output port
	{
		return OMX_ErrorBadPortIndex;
	}

	return OMX_ErrorNone;
}

OMX_BOOL enableAndroidBuffer(OMX_PTR ComponentParameterStructure)
{
	EnableAndroidNativeBuffersParams * pEnableAndroidNativeBuffersParams;

	pEnableAndroidNativeBuffersParams = (EnableAndroidNativeBuffersParams *)ComponentParameterStructure;

	return pEnableAndroidNativeBuffersParams->enable;
}

OMX_ERRORTYPE checkUseAndroidNativeBufferHeader(OMX_PTR ComponentParameterStructure)
{
	UseAndroidNativeBufferParams * pUseAndroidNativeBufferParams;

	pUseAndroidNativeBufferParams = (UseAndroidNativeBufferParams *)ComponentParameterStructure;

	return checkAndroidParamHeader(pUseAndroidNativeBufferParams, sizeof (UseAndroidNativeBufferParams));

}
OMX_ERRORTYPE checkUseAndroidNativeBufferPort(OMX_PTR ComponentParameterStructure, OMX_U32 *portIndex)
{
	UseAndroidNativeBufferParams * pUseAndroidNativeBufferParams;

	pUseAndroidNativeBufferParams = (UseAndroidNativeBufferParams *)ComponentParameterStructure;

	*portIndex = pUseAndroidNativeBufferParams->nPortIndex;
	if (pUseAndroidNativeBufferParams->nPortIndex != 1)	// output port
	{
		return OMX_ErrorBadPortIndex;
	}

	return OMX_ErrorNone;
}
OMX_ERRORTYPE useAndroidNativeBuffer(OMX_PTR ComponentParameterStructure, OMX_BUFFERHEADERTYPE **pNativeBufHeaderType, OMX_COLOR_FORMATTYPE eColorFormat)
{
	UseAndroidNativeBufferParams * pUseAndroidNativeBufferParams;
	android_native_buffer_t *buf;
	OMX_ERRORTYPE err;
	int bufWidth;
	int frameSize;
	int bufHeight;
	OMX_BUFFERHEADERTYPE *temp_bufferHeader = NULL;
    BufferPrivateStruct *pBufPrivStr = NULL;

	pUseAndroidNativeBufferParams = (UseAndroidNativeBufferParams *)ComponentParameterStructure;

	if (!pUseAndroidNativeBufferParams)
		return OMX_ErrorBadParameter;

	buf = pUseAndroidNativeBufferParams->nativeBuffer.get();
	bufWidth = ((buf->width + 15) / 16) * 16;
	bufHeight = ((buf->height + 15) / 16) * 16;
	if (eColorFormat == OMX_COLOR_FormatYUV420Planar ||
		eColorFormat == OMX_COLOR_FormatYUV420PackedPlanar ||
		eColorFormat == OMX_COLOR_FormatYUV420SemiPlanar ||
		eColorFormat == OMX_COLOR_FormatYUV420PackedSemiPlanar)
		frameSize = (bufWidth * bufHeight * 3) / 2;
	else
		frameSize = (bufWidth * bufHeight * 2);

	DEBUG(DEB_LEV_FULL_SEQ, "pNativeBufHeaderType: %p, width=%d, height=%d, framesize=%d", *pNativeBufHeaderType, bufWidth, bufHeight, frameSize);

	temp_bufferHeader = *pNativeBufHeaderType;
	temp_bufferHeader->nSize  = sizeof(OMX_BUFFERHEADERTYPE);
	temp_bufferHeader->nVersion.s.nVersionMajor = SPECVERSIONMAJOR;
	temp_bufferHeader->nVersion.s.nVersionMinor = SPECVERSIONMINOR;
	temp_bufferHeader->nVersion.s.nRevision = SPECREVISION;
	temp_bufferHeader->nVersion.s.nStep = SPECSTEP;
	temp_bufferHeader->pBuffer        = (OMX_U8 *)buf;
	temp_bufferHeader->nAllocLen      = (OMX_U32)frameSize;
	temp_bufferHeader->pAppPrivate    = pUseAndroidNativeBufferParams->pAppPrivate;
	temp_bufferHeader->nOutputPortIndex = pUseAndroidNativeBufferParams->nPortIndex;
    temp_bufferHeader->pOutputPortPrivate = new BufferPrivateStruct;
    if (!temp_bufferHeader->pOutputPortPrivate) {
        DEBUG(DEB_LEV_ERR, "temp_bufferHeader->pOutputPortPrivate is NULL!!!\n");
        return OMX_ErrorBadParameter;
    }
    bool iommu_is_enable = MemoryHeapIon::IOMMU_is_enabled(ION_MM);
    if (iommu_is_enable) {
        pBufPrivStr = (BufferPrivateStruct *)temp_bufferHeader->pOutputPortPrivate;
        unsigned long picPhyAddr = 0;
        size_t bufferSize = 0;
        android_native_buffer_t *buf = (android_native_buffer_t *)(temp_bufferHeader->pBuffer);
        native_handle_t *pNativeHandle = (native_handle_t *)(buf->handle);
        struct private_handle_t *private_h = (struct private_handle_t *)pNativeHandle;
        MemoryHeapIon::Get_iova(ION_MM, private_h->share_fd, &picPhyAddr, &bufferSize);

        pBufPrivStr->pMem = NULL;
        pBufPrivStr->bufferFd = private_h->share_fd;
        pBufPrivStr->phyAddr = picPhyAddr;
        pBufPrivStr->bufferSize = bufferSize;
        pBufPrivStr->picType = PIC_TYPE_MAX;
        pBufPrivStr->bufferIndex = -1;

        DEBUG(DEB_LEV_PARAMS, "bufferFd = %ld phyAddr=%x bufferSize = %lu\n", pBufPrivStr->bufferFd, pBufPrivStr->phyAddr, pBufPrivStr->bufferSize);
    } else {
       pBufPrivStr->pMem = NULL;
       pBufPrivStr->bufferFd = 0;
       pBufPrivStr->phyAddr = 0;
       pBufPrivStr->bufferSize = 0;
       pBufPrivStr->picType = PIC_TYPE_MAX;
       pBufPrivStr->bufferIndex = -1;
    }
	*(pUseAndroidNativeBufferParams->bufferHeader) = temp_bufferHeader;

	return OMX_ErrorNone;
}

OMX_ERRORTYPE checkGetAndroidNativeBufferHeader(OMX_PTR ComponentParameterStructure)
{
	GetAndroidNativeBufferUsageParams * pGetAndroidNativeBufferUsageParams;

	pGetAndroidNativeBufferUsageParams = (GetAndroidNativeBufferUsageParams *)ComponentParameterStructure;

	return checkAndroidParamHeader(pGetAndroidNativeBufferUsageParams, sizeof (GetAndroidNativeBufferUsageParams));
}

OMX_ERRORTYPE checkGetAndroidNativeBufferPort(OMX_PTR ComponentParameterStructure, OMX_U32 *portIndex)
{
	GetAndroidNativeBufferUsageParams * pGetAndroidNativeBufferUsageParams;

	pGetAndroidNativeBufferUsageParams = (GetAndroidNativeBufferUsageParams *)ComponentParameterStructure;
	*portIndex = pGetAndroidNativeBufferUsageParams->nPortIndex;
	if (pGetAndroidNativeBufferUsageParams->nPortIndex != 1)	// output port
	{
		return OMX_ErrorBadPortIndex;
	}

	return OMX_ErrorNone;
}


OMX_ERRORTYPE GetAndroidNativeBuffer(OMX_PTR ComponentParameterStructure)
{
	GetAndroidNativeBufferUsageParams * pGetAndroidNativeBufferUsageParams;
	pGetAndroidNativeBufferUsageParams = (GetAndroidNativeBufferUsageParams *)ComponentParameterStructure;
	pGetAndroidNativeBufferUsageParams->nUsage |= GRALLOC_USAGE_HW_RENDER;
	return OMX_ErrorNone;
}

void SetBufferIndex(OMX_BUFFERHEADERTYPE *pBuffer, int index)
{
    BufferPrivateStruct *pBufPrivStr = NULL;

    if (pBuffer && pBuffer->pOutputPortPrivate)
        pBufPrivStr = (BufferPrivateStruct *)pBuffer->pOutputPortPrivate;
    else
        return;

    pBufPrivStr->bufferIndex = index;
    return;
}

int GetBufferIndex(OMX_BUFFERHEADERTYPE *pBuffer)
{
    BufferPrivateStruct *pBufPrivStr = NULL;

    if (pBuffer && pBuffer->pOutputPortPrivate)
        pBufPrivStr = (BufferPrivateStruct *)pBuffer->pOutputPortPrivate;
    else
        return -1;

    return pBufPrivStr->bufferIndex;
}

void SetPicType(OMX_BUFFERHEADERTYPE *pBuffer, int picType)
{
    BufferPrivateStruct *pBufPrivStr = NULL;

    if (pBuffer->pOutputPortPrivate)
        pBufPrivStr = (BufferPrivateStruct *)pBuffer->pOutputPortPrivate;
    else
        return;

    pBufPrivStr->picType = picType;
    return;
}

int GetPicType(OMX_BUFFERHEADERTYPE *pBuffer)
{
    BufferPrivateStruct *pBufPrivStr = NULL;

    if (pBuffer->pOutputPortPrivate)
        pBufPrivStr = (BufferPrivateStruct *)pBuffer->pOutputPortPrivate;
    else
        return PIC_TYPE_MAX;

    return pBufPrivStr->picType;
}

void ClearPicType(OMX_BUFFERHEADERTYPE *pBuffer)
{
    BufferPrivateStruct *pBufPrivStr = NULL;

    if (pBuffer->pOutputPortPrivate)
        pBufPrivStr = (BufferPrivateStruct *)pBuffer->pOutputPortPrivate;
    else
        return;

    pBufPrivStr->picType = PIC_TYPE_MAX;
    return;
}

OMX_ERRORTYPE AllocateIONBuffer(int inSize, void **bufHandle, unsigned long *phyAddr, void **virAddr, size_t *bufSize)
{
    MemoryHeapIon *pMem;
    unsigned long phy_addr;
    int ret;
    bool iommu_enabled = MemoryHeapIon::IOMMU_is_enabled(ION_MM);

    if (iommu_enabled) {
        pMem = new MemoryHeapIon(SPRD_ION_DEV, inSize, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
    } else {
        pMem = new MemoryHeapIon(SPRD_ION_DEV, inSize, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
    }
    if (pMem->getHeapID() < 0) {
        DEBUG(DEB_LEV_ERR, "Failed to AllocateIONBuffer, getHeapID failed");
        return OMX_ErrorInsufficientResources;
    } else {
        if (iommu_enabled) {
            ret = pMem->get_iova(ION_MM, &phy_addr, bufSize);
        } else {
            ret = pMem->get_phy_addr_from_ion(&phy_addr, bufSize);
        }
        if (ret < 0) {
            DEBUG(DEB_LEV_ERR, "Failed to get_mm_iova, get phy addr failed");
            return OMX_ErrorInsufficientResources;
        } else {
            *bufHandle = (void *)pMem;
            *phyAddr = (unsigned long)phy_addr;
            *virAddr = pMem->getBase();
            DEBUG(DEB_LEV_PARAMS, "AllocateIONBuffer successfully %p - 0x%lx - %p - %zd",
                *bufHandle, phy_addr, *virAddr, bufSize);
        }
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE FreeIONBuffer(void *bufHandle, unsigned long phyAddr, size_t size)
{
    bool iommu_enabled = MemoryHeapIon::IOMMU_is_enabled(ION_MM);
    MemoryHeapIon *pMem = (MemoryHeapIon *)bufHandle;

    DEBUG(DEB_LEV_PARAMS, "FreeIONBuffer, bufHandle: %p, phyAddr: 0x%lx, size: %zd",
            bufHandle, phyAddr, size);

    if (iommu_enabled) {
        pMem->free_iova(ION_MM, phyAddr, size);
    }

    delete pMem;

    return OMX_ErrorNone;
}


OMX_ERRORTYPE getIOMMUPhyAddr(OMX_BUFFERHEADERTYPE *pNativeBufHeaderType, unsigned long *pAddrs)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    OMX_S32 bufferFd = 0;
    OMX_U32 phyAddr = 0;
    OMX_U32 bufferSize = 0;

    BufferPrivateStruct *pBufPrivStr = NULL;
    if (pNativeBufHeaderType->pOutputPortPrivate) {
        pBufPrivStr = (BufferPrivateStruct *)pNativeBufHeaderType->pOutputPortPrivate;
    }

    bufferFd = pBufPrivStr->bufferFd;
    phyAddr = pBufPrivStr->phyAddr;
    bufferSize = pBufPrivStr->bufferSize;

    DEBUG(DEB_LEV_FULL_SEQ, "bufferFd = %d, phyAddr = %x, bufferSize = %lu\n",bufferFd, phyAddr, bufferSize);

    *pAddrs = (unsigned long)phyAddr;
    return ret;
}

OMX_ERRORTYPE freeIOMMUAddr(OMX_BUFFERHEADERTYPE *pNativeBufHeaderType)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    BufferPrivateStruct *pBufPrivStr = NULL;
    if (pNativeBufHeaderType->pOutputPortPrivate) {
        pBufPrivStr = (BufferPrivateStruct *)pNativeBufHeaderType->pOutputPortPrivate;
    }
    //DEBUG(DEB_LEV_ERR, "BufferPrivateStruct: %p, pOutputPortPrivate: %p", pBufPrivStr, pNativeBufHeaderType->pOutputPortPrivate);
    if (MemoryHeapIon::IOMMU_is_enabled(ION_MM)) {
        DEBUG(DEB_LEV_PARAMS, "fd: %d, phyAddr: 0x%lx, size: %zd", pBufPrivStr->bufferFd, pBufPrivStr->phyAddr, pBufPrivStr->bufferSize);
        if(pBufPrivStr->bufferFd > 0) {
            MemoryHeapIon::Free_iova(ION_MM, pBufPrivStr->bufferFd, pBufPrivStr->phyAddr, pBufPrivStr->bufferSize);
        }
        delete (BufferPrivateStruct *)(pNativeBufHeaderType->pOutputPortPrivate);
        pNativeBufHeaderType->pOutputPortPrivate = NULL;
    }

    return ret;
}

OMX_U32 lockAndroidNativeBuffer(OMX_BUFFERHEADERTYPE *pNativeBufHeaderType, int stride, int height, OMX_U32 mode, void *pAddrs[])
{
	OMX_U32 ret = 0;
	android_native_buffer_t *buf;
	GraphicBufferMapper &mapper = GraphicBufferMapper::get();
	Rect bounds(stride, height);

	buf = (android_native_buffer_t *)pNativeBufHeaderType->pBuffer;
	if (!buf) {
		DEBUG(DEB_LEV_ERR, "lockAndroidNativeBuffer : fail to get native buffer handle buf=0x%p, buf->handle=0x%p", buf, (buf)?buf->handle:0);
		return -1;
	}


	DEBUG(DEB_LEV_FULL_SEQ, "lockAndroidNativeBuffer buf:0x%p, buffer_handle_t:0x%p, width=%d, height=%d, bufStride=%d, bufFormat=0x%x, usage=0x%x", 
		buf, buf->handle, buf->width, buf->height, buf->stride, buf->format, buf->usage);


	if (stride > buf->stride || height > buf->height) {
		DEBUG(DEB_LEV_ERR, "lockAndroidNativeBuffer : invalid buffer stride=%d, height=%d\n", stride, height);
		return -1;
	}


	if (mode == LOCK_MODE_TO_GET_VIRTUAL_ADDRESS)
		ret = mapper.lock(buf->handle, GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN, bounds, pAddrs);
	else // HW
		ret = mapper.lock(buf->handle, GRALLOC_USAGE_HW_RENDER, bounds, pAddrs);
	if (ret != 0) {
		DEBUG(DEB_LEV_ERR, "lockAndroidNativeBuffer : mapper.lock error code:%d, buf->handle=0x%p", (int)ret, buf->handle);
	}


	return ret;
}

OMX_U32 unlockAndroidNativeBuffer(OMX_BUFFERHEADERTYPE *pNativeBufHeaderType)
{
	android_native_buffer_t *buf;
	int ret = 0;
	GraphicBufferMapper &mapper = GraphicBufferMapper::get();

	buf = (android_native_buffer_t *)pNativeBufHeaderType->pBuffer;

	//DEBUG(DEB_LEV_FULL_SEQ, "unlockAndroidNativeBuffer android_native_buffer_t:0x%p, buf:0x%p, buf->handle:0x%p", pNativeBufHeaderType->pBuffer, buf, buf->handle);

	ret = mapper.unlock(buf->handle);
	if (ret != 0) {
		DEBUG(DEB_LEV_ERR, "unlockAndroidNativeBuffer : mapper.unlock error code:%d, buf->handle=0x%p", (int)ret, buf->handle);
	}

	return ret;
}



OMX_BOOL getAndroidNativeBufferInfo(OMX_BUFFERHEADERTYPE *pNativeBufHeaderType, int *pFormat, int *pStride, int *pWidth, int *pHeight)
{
	android_native_buffer_t *buf;

	buf = (android_native_buffer_t *)pNativeBufHeaderType->pBuffer;
	if (!buf) {
		DEBUG(DEB_LEV_ERR, "lockAndroidNativeBuffer : fail to get native buffer handle buf=0x%p, buf->handle=0x%p", buf, (buf)?buf->handle:0);
		return OMX_FALSE;
	}

	if (pFormat)
		*pFormat = buf->format;

	*pStride = buf->stride;
	*pWidth = buf->width;
	*pHeight = buf->height;


	DEBUG(DEB_LEV_FULL_SEQ, "getAndroidNativeBufferInfo buf:0x%p, buffer_handle_t:0x%p, width=%d, height=%d, bufStride=%d, bufFormat=0x%x, usage=0x%x", 
		buf, buf->handle, buf->width, buf->height, buf->stride, buf->format, buf->usage);

	return OMX_TRUE;
}


int getNativeBufferSize(OMX_COLOR_FORMATTYPE colorFormat, int native_buffer_format, int stride, int height)
{
	int ret;

	if (native_buffer_format == 0)
	{
		switch (colorFormat)
		{
		case OMX_COLOR_FormatYUV420Planar:
		case OMX_COLOR_FormatYUV420PackedPlanar:
		case OMX_COLOR_FormatYUV420SemiPlanar:
		case OMX_COLOR_FormatYUV420PackedSemiPlanar:
			ret = stride * height * 3 / 2;
			break;
		case OMX_COLOR_FormatYUV422Planar:
		case OMX_COLOR_FormatYUV422PackedPlanar:
		case OMX_COLOR_FormatYUV422SemiPlanar:
		case OMX_COLOR_FormatYUV422PackedSemiPlanar:
		case OMX_COLOR_FormatYCbYCr:
		case OMX_COLOR_FormatCbYCrY:
			ret = stride * height * 2;
		default:
#define MAKE_FOURCC(a,b,c,d) ( ((unsigned char)a) | ((unsigned char)b << 8) | ((unsigned char)c << 16) | ((unsigned char)d << 24) )
			if (colorFormat == (int)MAKE_FOURCC('N', 'V', '1', '2') || colorFormat == (int)MAKE_FOURCC('Y', 'V', '1', '2'))
			{
				ret = stride * height * 3 / 2;
			}
			else if (colorFormat == (int)MAKE_FOURCC('I', '4', '2', '2')
				|| colorFormat == (int)MAKE_FOURCC('N', 'V', '1', '6')
				|| colorFormat == (int)MAKE_FOURCC('Y', 'U', 'Y', 'V')
				|| colorFormat == (int)MAKE_FOURCC('U', 'Y', 'V', 'Y'))
			{
				ret = stride * height * 2;
			}
			else
			{
				ret = stride * height * 3;
			}
			break;
		}
	}
	else if (native_buffer_format == HAL_PIXEL_FORMAT_RGB_888)
	{
		ret = stride * height * 3;
	}
	else if (native_buffer_format == HAL_PIXEL_FORMAT_RGB_565)
	{
		ret = stride * height * 2;
	}
	else
	{
		ret = stride * height * 4;
	}
	return ret;
}


OMX_U32 lockAndroidBufferHandle(buffer_handle_t handle, int width, int height, OMX_U32 mode, void *pAddrs[])
{
	OMX_U32 ret = 0;
	GraphicBufferMapper &mapper = GraphicBufferMapper::get();
	Rect bounds(width, height);

	if (mode == LOCK_MODE_TO_GET_VIRTUAL_ADDRESS)
		ret = mapper.lock(handle, GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN, bounds, pAddrs);
	else // HW
		ret = mapper.lock(handle, GRALLOC_USAGE_HW_RENDER, bounds, pAddrs);
	if (ret != 0) {
		DEBUG(DEB_LEV_ERR, "lockAndroidBufferHandle : mapper.lock error code:%d, buf->handle=0x%p", (int)ret, handle);
	}

	DEBUG(DEB_LEV_FULL_SEQ, "lockAndroidBufferHandle ret=0x%x, pAddrs[0]=%p, pAddrs[1]=%p, pAddrs[2]=%p", (int)ret, pAddrs[0], pAddrs[1], pAddrs[2]);

	return ret;
}

OMX_U32 unLockAndroidBufferHandle(buffer_handle_t handle)
{
	int ret = 0;
	GraphicBufferMapper &mapper = GraphicBufferMapper::get();

	ret = mapper.unlock(handle);
	if (ret != 0) {
		DEBUG(DEB_LEV_ERR, "unlockAndroidNativeBuffer : mapper.unlock error code:%d, buf->handle=0x%p", (int)ret, handle);
	}

	return ret;
}

OMX_BOOL getAndroidNativeBufferHandleInfo(buffer_handle_t handle, int *pFormat, int *pWidth, int *pHeight, int *pSize)
{
	OMX_U32 ret = 0;
	private_handle_t* hnd = private_handle_t::dynamicCast(handle);

	if (hnd == NULL)
		return OMX_FALSE;

	if (pFormat)
		*pFormat = hnd->format;

	if (pWidth)
		*pWidth = hnd->width;

	if (pHeight)
		*pHeight = hnd->height;

	if (pSize)
		*pSize = hnd->size;
	DEBUG(DEB_LEV_FULL_SEQ, "getAndroidNativeBufferHandleInfo : fd=0x%x, flags=0x%x, size=%d0, offset=%d, base=0x%x, format=0x%x, width=%d, height=%d\n", 
	 		(int)hnd->fd, (int)hnd->flags, (int)hnd->size, (int)hnd->offset, (unsigned long)hnd->base,
	 		(int)hnd->format, (int)hnd->width, (int)hnd->height);

	return OMX_TRUE;
}




OMX_ERRORTYPE checkAndroidParamHeader(OMX_PTR header, OMX_U32 size)
{
	OMX_VERSIONTYPE* ver;
	if (header == NULL) {
		DEBUG(DEB_LEV_ERR, "the header is null\n");
		return OMX_ErrorBadParameter;
	}
	ver = (OMX_VERSIONTYPE*)((char*)header + sizeof(OMX_U32));
	if(*((OMX_U32*)header) != size) {
		DEBUG(DEB_LEV_ERR, "the header has a wrong size %i should be %i\n",(int)*((OMX_U32*)header),(int)size);
		return OMX_ErrorBadParameter;
	}
	if(ver->s.nVersionMajor != SPECVERSIONMAJOR ||
		ver->s.nVersionMinor != SPECVERSIONMINOR) {
			DEBUG(DEB_LEV_ERR, "The version does not match\n");
			return OMX_ErrorVersionMismatch;
	}
	return OMX_ErrorNone;
}

OMX_ERRORTYPE checkStoreMetaDataBufferHeader(OMX_PTR ComponentParameterStructure)
{
	StoreMetaDataInBuffersParams *pStoreMetaDataInBuffers = (StoreMetaDataInBuffersParams *) ComponentParameterStructure;

	return checkAndroidParamHeader(pStoreMetaDataInBuffers, sizeof (GetAndroidNativeBufferUsageParams));
}

OMX_ERRORTYPE checkStoreMetaDataBufferPort(OMX_PTR ComponentParameterStructure, OMX_U32 *portIndex)
{
	StoreMetaDataInBuffersParams *pStoreMetaDataInBuffers = (StoreMetaDataInBuffersParams *) ComponentParameterStructure;

	if (pStoreMetaDataInBuffers->nPortIndex > 1)
	{
		return OMX_ErrorBadPortIndex;
	}

	if (portIndex)
		*portIndex = pStoreMetaDataInBuffers->nPortIndex;

	return OMX_ErrorNone;
}


OMX_ERRORTYPE StoreMetaDataBuffer(OMX_PTR ComponentParameterStructure, OMX_BOOL *pbEnable)
{
	StoreMetaDataInBuffersParams *pStoreMetaDataInBuffers = (StoreMetaDataInBuffersParams *) ComponentParameterStructure;
	if (pbEnable)
		*pbEnable = pStoreMetaDataInBuffers->bStoreMetaData;
	return OMX_ErrorNone;
}


#ifdef WORKAROUND_RGB2YUV_CSC_BY_SW
#include <libyuv.h>
OMX_BOOL ConvertRgbToYuvbySW(OMX_BYTE pYuvData, OMX_BYTE pRgbData, OMX_U32 rgbFormat, OMX_U32 width, OMX_U32 height)
{
	OMX_BYTE pY;
	OMX_BYTE pCb;
	OMX_BYTE pCr;
	unsigned int yuvFormat;

	pY = pYuvData;
	pCb = pYuvData + (width*height);
	pCr = pYuvData + (width*height) + (width*height)/4;

	switch (rgbFormat) 
	{
	case HAL_PIXEL_FORMAT_RGBA_8888:
		yuvFormat = libyuv::FOURCC_ABGR;
		break;
	case HAL_PIXEL_FORMAT_RGBX_8888:
		yuvFormat = libyuv::FOURCC_ABGR;
		break;
	case HAL_PIXEL_FORMAT_RGB_888:
		yuvFormat = libyuv::FOURCC_24BG;
		break;
	case HAL_PIXEL_FORMAT_RGB_565:
		yuvFormat = libyuv::FOURCC_RGBP;
		break;
	default:
		return OMX_FALSE;
	}

	if (0 != ConvertToI420(pRgbData, 0, pY, width, pCb, (width + 1)/2, pCr, (width + 1)/2, 0, 0, width, height, width, height, libyuv::kRotate0, yuvFormat))
		return OMX_FALSE;

	return OMX_TRUE;
}


#endif


#ifdef CNM_SPRD_PLATFORM
OMX_ERRORTYPE GetAndroidNativeBufferAddr(OMX_BUFFERHEADERTYPE **ppNativeBufHeaderType, OMX_U32 mode, void **pAddrs)
{
	android_native_buffer_t *buf;
	OMX_BUFFERHEADERTYPE *pNativeBufHeaderType = *ppNativeBufHeaderType;
    private_handle_t *handle = NULL;
    OMX_U8 *viraddr = NULL;


	buf = (android_native_buffer_t *)pNativeBufHeaderType->pBuffer;
	if (!buf)
	{

		DEBUG(DEB_LEV_ERR, "fail to get native buffer handle buf=%p, buf->handle=%p", buf, (buf)?buf->handle:0);
		return OMX_ErrorInsufficientResources;
	}

	handle = (private_handle_t *)buf->handle;

	if (GET_PHYSICAL_ADDRESS == mode)
	{
	   *pAddrs = (void *)handle->phys;
		DEBUG(DEB_LEV_FULL_SEQ, "pNativeBufHeaderTypebuf=%p, buf=%p, phy=%p, handle=%p", *ppNativeBufHeaderType, buf, *pAddrs, handle);
		return OMX_ErrorNone;
	}
	else if (GET_VIRTUAL_ADDRESS == mode)
	{
		/* android native buffers can be used only on Output port, just for debug */
		viraddr = (OMX_U8 *)mmap(0, handle->size, PROT_READ | PROT_WRITE, MAP_SHARED, handle->share_fd, 0);

		if (MAP_FAILED == viraddr)
		{
			DEBUG(DEB_LEV_ERR, "Failed to mmap pmem with share_fd = %d, size = %d", handle->share_fd, handle->size);
			return OMX_ErrorInsufficientResources;
		}

	   *pAddrs = (void *)viraddr;
		DEBUG(DEB_LEV_FULL_SEQ, "pNativeBufHeaderTypebuf=%p, buf=%p, vir=%p, handle=%p", *ppNativeBufHeaderType, buf, *pAddrs, handle);
	}
	else
	{
		DEBUG(DEB_LEV_ERR, "invalid mode =%d", (int)mode);
	}

	return OMX_ErrorNone;
}

OMX_ERRORTYPE munmapAndroidNativeBuffer(OMX_BUFFERHEADERTYPE *pNativeBufHeaderType, void *pVirAddr)
{
    private_handle_t *handle = NULL;
	android_native_buffer_t *buf;

    if (pNativeBufHeaderType == NULL || pVirAddr == NULL)
    {
		DEBUG(DEB_LEV_ERR, "pNativeBufHeaderType=%p, pVirAddr=%p", pNativeBufHeaderType, pVirAddr);
        return OMX_ErrorBadParameter;
    }

	buf = (android_native_buffer_t *)(pNativeBufHeaderType->pBuffer);
	if (!buf)
	{
		DEBUG(DEB_LEV_ERR, "fail to get native buffer handle buf=%p, buf->handle=%p", buf, (buf)?buf->handle:0);
		return OMX_ErrorBadParameter;
	}

	handle = (private_handle_t *)buf->handle;

    DEBUG(DEB_LEV_FULL_SEQ, "pNativeBufHeaderType=%p, buf=%p, viraddr=%p, handle=%p", pNativeBufHeaderType, buf, pVirAddr, handle);

    munmap(pVirAddr, handle->size);

    return OMX_ErrorNone;
}

void setNativeBufferInfo(OMX_BUFFERHEADERTYPE *pNativeBufHeaderType, int original_width, int original_height, int actual_width, int actual_height, int coded_width, int coded_height)
{
	android_native_buffer_t *buf;
    private_handle_t *handle = NULL;

	buf = (android_native_buffer_t *)pNativeBufHeaderType->pBuffer;
    handle = (private_handle_t *)buf->handle;

    buf->width              = original_width;
    buf->height             = original_height;

    handle->original_width  = original_width;
    handle->original_height = original_height;

	handle->actual_width    = actual_width;
	handle->actual_height   = actual_height;

    handle->coded_width	    = coded_width;
    handle->coded_height    = coded_height;

	DEBUG(DEB_LEV_FULL_SEQ, "original_width=%d, original_height=%d, actual_width=%d, actual_height=%d, coded_width=%d, coded_height=%d",
                  original_width, original_height,
                  actual_width, actual_height,
                  coded_width, coded_height);
    return;
}

int getHandlePhys(buffer_handle_t bufHandle, unsigned int* phys)
{
    private_handle_t* priv_handle = (private_handle_t*)(bufHandle );

    if(priv_handle == NULL)
    {
        DEBUG(DEB_LEV_ERR, "Error private_handle_t!");
        return -1;
    }

    #if 0
	/*Write YUV data to file*/
    void* data_viraddr = (void*)(priv_handle->base);
    int size = priv_handle->size;

    if(data_viraddr == NULL || size == 0)
    {
	    DEBUG(DEB_LEV_ERR, "Data_viraddr is null!");
	    return -1;
    }

    FILE* fp_dec = fopen("/data/img/record_enc.yuv", "a+");
    if(fp_dec)
    {
	    fwrite((void*)data_viraddr, 1, size, fp_dec);
	    fclose(fp_dec);
	    fp_dec = NULL;
    }
    else
    {
	    DEBUG(DEB_LEV_ERR, "Failed to open file record_enc!!1");
    }
    #endif

    *phys = priv_handle->phys;
	return 0;
}

int getHandleVirualAddr(buffer_handle_t bufHandle, unsigned int* virt)
{
    private_handle_t* priv_handle = (private_handle_t*)(bufHandle );

    if(priv_handle == NULL)
    {
        DEBUG(DEB_LEV_ERR, "Error private_handle_t!");
        return -1;
    }

    *virt = priv_handle->base;
	return 0;
}


int IsRGBA(buffer_handle_t bufHandle) {

    private_handle_t    *priv   = (private_handle_t*)(bufHandle);
    return (PIXEL_FORMAT_RGBA_8888 == priv->format)? 1 : 0;
}
#endif  //CNM_SPRD_PLATFORM


