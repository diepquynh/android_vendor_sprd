
#ifndef _GRALLOC_PUBLIC_
#define _GRALLOC_PUBLIC_
#define PARAM_IGNORE(x) (void)x

/*gpu version*/
#define	rogue   0x01
#define	midgard 0x02
#define	soft    0x04
#define	utgard  0x08

#define SPRD_GPU_PLATFORM_ROGUE (TARGET_GPU_PLATFORM & rogue)
#define SPRD_GPU_PLATFORM_MIDGARD (TARGET_GPU_PLATFORM & midgard)
#define SPRD_GPU_PLATFORM_SOFT (TARGET_GPU_PLATFORM & soft)
#define SPRD_GPU_PLATFORM_UTGARD (TARGET_GPU_PLATFORM & utgard)

#if SPRD_GPU_PLATFORM_ROGUE

#define PVR_ANDROID_HAS_SET_BUFFERS_DATASPACE

#include "rogue/img_gralloc_public.h"

static inline int ADP_FORMAT(buffer_handle_t h)
{
	if (h == NULL)
		return 0;

	return HALPixelFormatGetRawFormat(((IMG_native_handle_t*)h)->iFormat);
}

static inline int ADP_WIDTH(buffer_handle_t h)
{
	if (h == NULL)
		return 0;

	return ((IMG_native_handle_t*)h)->iWidth;
}

static inline int ADP_HEIGHT(buffer_handle_t h)
{
	if (h == NULL)
		return 0;

	return ((IMG_native_handle_t*)h)->iHeight;
}

static inline int ADP_STRIDE(buffer_handle_t h)
{
	if (h == NULL)
		return 0;

	return ((IMG_native_handle_t*)h)->aiStride[0];
}

static inline int ADP_VSTRIDE(buffer_handle_t h)
{
	if (h == NULL)
		return 0;

	return ((IMG_native_handle_t*)h)->aiVStride[0];
}

static inline int ADP_BUFFD(buffer_handle_t h)
{
	if (h == NULL)
		return 0;

	return ((IMG_native_handle_t*)h)->fd[0];
}

static inline int ADP_YINFO(buffer_handle_t h)
{
	if (h == NULL)
		return 0;

	//return ((IMG_native_handle_t*)h)->yuv_info;
	return 0;
}


static inline int ADP_USAGE(buffer_handle_t h)
{
	if (h == NULL)
		return 0;

	return ((IMG_native_handle_t*)h)->usage;
}

static inline void* ADP_BASE(buffer_handle_t h)
{
	if (h == NULL)
		return 0;

	if (((IMG_native_handle_t*)h)->vCpuVirtAddr)
		return ((IMG_native_handle_t*)h)->vCpuVirtAddr[0];

	return 0;
}

static inline int ADP_BUFSIZE(buffer_handle_t h)
{
	if (h == NULL)
		return 0;

	if (((IMG_native_handle_t*)h)->sSize)
		return ((IMG_native_handle_t*)h)->sSize[0];

	return 0;
}

static inline int ADP_COMPRESSED(buffer_handle_t h)
{
	if (h == NULL)
		return 0;

	return (((((IMG_native_handle_t*)h)->iFormat >> 4) & 0x7) ==
		HAL_FB_COMPRESSION_DIRECT_16x4);
}

#else

#if SPRD_GPU_PLATFORM_MIDGARD
#include "midgard/include/gralloc_priv.h"
#elif SPRD_GPU_PLATFORM_SOFT
#include "soft/include/gralloc_priv.h"
#else
#include "utgard/include/gralloc_priv.h"
#endif

static inline int ADP_FORMAT(buffer_handle_t h)
{
	if (h == NULL)
		return 0;

	return ((private_handle_t*)h)->format;
}

static inline int ADP_WIDTH(buffer_handle_t h)
{
	if (h == NULL)
		return 0;

	return ((private_handle_t*)h)->width;
}

static inline int ADP_HEIGHT(buffer_handle_t h)
{
	if (h == NULL)
		return 0;

	return ((private_handle_t*)h)->height;
}

static inline int ADP_STRIDE(buffer_handle_t h)
{
	if (h == NULL)
		return 0;

	return ((private_handle_t*)h)->stride;
}

static inline int ADP_BUFFD(buffer_handle_t h)
{
	if (h == NULL)
		return 0;

	return ((private_handle_t*)h)->share_fd;
}

static inline int ADP_YINFO(buffer_handle_t h)
{
	if (h == NULL)
		return 0;

	return ((private_handle_t*)h)->yuv_info;
}

static inline int ADP_USAGE(buffer_handle_t h)
{
	if (h == NULL)
		return 0;

	return ((private_handle_t*)h)->usage;
}

static inline int ADP_FLAGS(buffer_handle_t h)
{
	if (h == NULL)
		return 0;

	return ((private_handle_t*)h)->flags;
}

/*this interface only for sf process */
static inline void* ADP_BASE(buffer_handle_t h)
{
	if (h == NULL)
		return 0;

	return ((private_handle_t*)h)->base;
}

static inline int ADP_BUFSIZE(buffer_handle_t h)
{
	if (h == NULL)
		return 0;

	return ((private_handle_t*)h)->size;
}

static inline int ADP_COMPRESSED(buffer_handle_t h)
{
	if (h == NULL)
		return 0;

	return ((((private_handle_t*)h)->internal_format &
		GRALLOC_ARM_INTFMT_AFBC) == GRALLOC_ARM_INTFMT_AFBC);
}

#define ADP_BUFINDEX(h)	((private_handle_t*)h)->buf_idx
#endif

#endif

