/*
 * Copyright (C) 2010 ARM Limited. All rights reserved.
 *
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * You may not use this file except in compliance with the License.
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

#include <errno.h>
#include <pthread.h>

#include <cutils/log.h>
#include <cutils/atomic.h>
#include <hardware/hardware.h>
#include <hardware/gralloc.h>

#include "gralloc_priv.h"
#include "alloc_device.h"
#include "framebuffer_device.h"

#include "gralloc_module_allocator_specific.h"

#if MALI_AFBC_GRALLOC == 1
#include "gralloc_buffer_priv.h"
#endif

#include "format_chooser.h"

static int Debug = 1;

static pthread_mutex_t s_map_lock = PTHREAD_MUTEX_INITIALIZER;

static int gralloc_device_open(const hw_module_t* module, const char* name, hw_device_t** device)
{
	int status = -EINVAL;

	if (!strncmp(name, GRALLOC_HARDWARE_GPU0, MALI_GRALLOC_HARDWARE_MAX_STR_LEN))
	{
		status = alloc_device_open(module, name, device);
	}
	else if (!strncmp(name, GRALLOC_HARDWARE_FB0, MALI_GRALLOC_HARDWARE_MAX_STR_LEN))
	{
		status = framebuffer_device_open(module, name, device);
	}

	return status;
}

static int gralloc_register_buffer(gralloc_module_t const* module, buffer_handle_t handle)
{
	if (private_handle_t::validate(handle) < 0)
	{
		ALOGE("Registering invalid buffer %p, returning error", handle);
		return -EINVAL;
	}

	// if this handle was created in this process, then we keep it as is.
	private_handle_t* hnd = (private_handle_t*)handle;

	if (hnd->pid == getpid())
	{
		// If the handle is created and registered in the same process this is valid,
		// but it could also be that application is registering twice which is illegal.
		ALOGI_IF(Debug, "Registering handle %p coming from the same process: %d.", hnd, hnd->pid);
	}

	int retval = -EINVAL;

	pthread_mutex_lock(&s_map_lock);

	hnd->pid = getpid();

	if (hnd->flags & private_handle_t::PRIV_FLAGS_FRAMEBUFFER) 
	{
		ALOGE("Can't register buffer %p as it is a framebuffer", handle );
	}
	else if (hnd->flags & (private_handle_t::PRIV_FLAGS_USES_UMP |
	                       private_handle_t::PRIV_FLAGS_USES_ION))
	{
		retval = gralloc_backend_register(hnd);
	}
	else
	{
		ALOGE("registering non-UMP buffer not supported. flags = %d", hnd->flags );
	}

	ALOGI_IF(Debug, "gralloc_register_buffer w:%d, h:%d, f:0x%x, usage:0x%x, ion_hnd: 0x%x, cpu vir: 0x%x, handle:0x%x",
                 hnd->width, hnd->height, hnd->format, hnd->usage, hnd->ion_hnd, hnd->base, hnd);

	pthread_mutex_unlock(&s_map_lock);
	return retval;
}

static int gralloc_unregister_buffer(gralloc_module_t const* module, buffer_handle_t handle)
{
	if (private_handle_t::validate(handle) < 0)
	{
		ALOGE("unregistering invalid buffer %p, returning error", handle);
		return -EINVAL;
	}

	private_handle_t* hnd = (private_handle_t*)handle;

	ALOGI_IF(hnd->lockState & private_handle_t::LOCK_STATE_READ_MASK, "[unregister] handle %p still locked (state=%08x)", hnd, hnd->lockState);

	ALOGI_IF(Debug, "gralloc_unregister_buffer w:%d, h:%d, f:0x%x, usage:0x%x, ion_hnd: 0x%x, cpu vir: 0x%x, handle:0x%x",
                 hnd->width, hnd->height, hnd->format, hnd->usage, hnd->ion_hnd, hnd->base, hnd);

	if (hnd->flags & private_handle_t::PRIV_FLAGS_FRAMEBUFFER)
	{
		ALOGE( "Can't unregister buffer %p as it is a framebuffer", handle );
	}
	else if (hnd->pid == getpid()) // never unmap buffers that were not created in this process
	{
		pthread_mutex_lock(&s_map_lock);

		if (hnd->flags & (private_handle_t::PRIV_FLAGS_USES_UMP |
		                  private_handle_t::PRIV_FLAGS_USES_ION))
		{
			gralloc_backend_unregister(hnd);
		}
		else
		{
			ALOGE("Unregistering unknown buffer is not supported. Flags = %d", hnd->flags);
		}

#if MALI_AFBC_GRALLOC == 1
		/*
		 * Close shared attribute region file descriptor. It might seem strange to "free"
		 * this here since this can happen in a client process, but free here is nothing
		 * but unmapping and closing the duplicated file descriptor. The original ashmem
		 * fd instance is still open until alloc_device_free() is called. Even sharing
		 * of gralloc buffers within the same process should have fds dup:ed.
		 */
		gralloc_buffer_attr_free( hnd );

#endif
		hnd->base = 0;
		hnd->lockState  = 0;
		hnd->writeOwner = 0;

		pthread_mutex_unlock(&s_map_lock);
	}
	else
	{
		ALOGE( "Trying to unregister buffer %p from process %d that was not created in current process: %d", hnd, hnd->pid, getpid());
	}

	return 0;
}

static int gralloc_lock(gralloc_module_t const* module, buffer_handle_t handle, int usage, int l, int t, int w, int h, void** vaddr)
{
	if (private_handle_t::validate(handle) < 0)
	{
		AERR("Locking invalid buffer %p, returning error", handle );
		return -EINVAL;
	}

	private_handle_t* hnd = (private_handle_t*)handle;
	if (hnd->format == HAL_PIXEL_FORMAT_YCbCr_420_888)
	{
		AERR("Buffers with format YCbCr_420_888 must be locked using (*lock_ycbcr)" );
		return -EINVAL;
	}

	if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_UMP || hnd->flags & private_handle_t::PRIV_FLAGS_USES_ION)
	{
		hnd->writeOwner = usage & GRALLOC_USAGE_SW_WRITE_MASK;
	}
	if (usage & (GRALLOC_USAGE_SW_READ_MASK | GRALLOC_USAGE_SW_WRITE_MASK))
	{
		*vaddr = (void*)hnd->base;
	}
	return 0;
}

static int gralloc_lock_ycbcr(struct gralloc_module_t const* module,
		buffer_handle_t handle, int usage,
		int l, int t, int w, int h,
		struct android_ycbcr *ycbcr)
{
	MALI_IGNORE(module);

	if (private_handle_t::validate(handle) < 0)
	{
		AERR("Locking invalid buffer 0x%p, returning error", handle );
		return -EINVAL;
	}
	private_handle_t* hnd = (private_handle_t*)handle;
	int ystride;
	int err=0;

	if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_UMP || hnd->flags & private_handle_t::PRIV_FLAGS_USES_ION)
	{
		hnd->writeOwner = usage & GRALLOC_USAGE_SW_WRITE_MASK;
	}

	switch (hnd->format) {
		case HAL_PIXEL_FORMAT_YCrCb_420_SP:
		case HAL_PIXEL_FORMAT_YCbCr_420_888:
		/*HAL_PIXEL_FORMAT_YCbCr_420_888 is a flexible yuv format and sprd treat
		 * it as YCrCb 420 sp(be used in DCAM HAL)*/
			ystride = GRALLOC_ALIGN(hnd->width, 16);
			ycbcr->y  = (void*)hnd->base;
			ycbcr->cr = (void*)((uintptr_t)hnd->base + ystride * hnd->height);
			ycbcr->cb = (void*)((uintptr_t)hnd->base + ystride * hnd->height + 1);
			ycbcr->ystride = ystride;
			ycbcr->cstride = ystride;
			ycbcr->chroma_step = 2;
			memset(ycbcr->reserved, 0, sizeof(ycbcr->reserved));
			break;
		case HAL_PIXEL_FORMAT_YCbCr_420_SP:
			ystride = GRALLOC_ALIGN(hnd->width, 16);
			ycbcr->y  = (void*)hnd->base;
			ycbcr->cb = (void*)((uintptr_t)hnd->base + ystride * hnd->height);
			ycbcr->cr = (void*)((uintptr_t)hnd->base + ystride * hnd->height + 1);
			ycbcr->ystride = ystride;
			ycbcr->cstride = ystride;
			ycbcr->chroma_step = 2;
			memset(ycbcr->reserved, 0, sizeof(ycbcr->reserved));
			break;
		default:
			ALOGD("%s: Invalid format passed: 0x%x", __FUNCTION__,
				  hnd->format);
			err = -EINVAL;
	}
	MALI_IGNORE(usage);
	MALI_IGNORE(l);
	MALI_IGNORE(t);
	MALI_IGNORE(w);
	MALI_IGNORE(h);
	return err;
}

static int gralloc_unlock(gralloc_module_t const* module, buffer_handle_t handle)
{
	if (private_handle_t::validate(handle) < 0)
	{
		AERR( "Unlocking invalid buffer %p, returning error", handle );
		return -EINVAL;
	}

	private_handle_t* hnd = (private_handle_t*)handle;

	if (hnd->flags & (private_handle_t::PRIV_FLAGS_USES_UMP |
	                  private_handle_t::PRIV_FLAGS_USES_ION)
	    && hnd->writeOwner)
	{
		gralloc_backend_sync(hnd);
	}

	return 0;
}

// There is one global instance of the module

static struct hw_module_methods_t gralloc_module_methods =
{
	open: gralloc_device_open
};

private_module_t::private_module_t()
{
#define INIT_ZERO(obj) (memset(&(obj),0,sizeof((obj))))

	base.common.tag = HARDWARE_MODULE_TAG;
	base.common.version_major = 1;
	base.common.version_minor = 0;
	base.common.id = GRALLOC_HARDWARE_MODULE_ID;
	base.common.name = "Graphics Memory Allocator Module";
	base.common.author = "ARM Ltd.";
	base.common.methods = &gralloc_module_methods;
	base.common.dso = NULL;
	INIT_ZERO(base.common.reserved);

	base.registerBuffer = gralloc_register_buffer;
	base.unregisterBuffer = gralloc_unregister_buffer;
	base.lock = gralloc_lock;
	base.lock_ycbcr = gralloc_lock_ycbcr;
	base.unlock = gralloc_unlock;
	base.perform = NULL;
	INIT_ZERO(base.reserved_proc);

	framebuffer = NULL;
	flags = 0;
	numBuffers = 0;
	bufferMask = 0;
	pthread_mutex_init(&(lock), NULL);
	currentBuffer = NULL;
	INIT_ZERO(info);
	INIT_ZERO(finfo);
	xdpi = 0.0f; 
	ydpi = 0.0f; 
	fps = 0.0f;
	swapInterval = 1;

	initialize_blk_conf();

#undef INIT_ZERO
};

/*
 * HAL_MODULE_INFO_SYM will be initialized using the default constructor
 * implemented above
 */ 
struct private_module_t HAL_MODULE_INFO_SYM;

