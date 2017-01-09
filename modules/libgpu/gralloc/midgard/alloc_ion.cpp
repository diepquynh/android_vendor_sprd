/*
 * Copyright (C) 2013 ARM Limited. All rights reserved.
 *
 * Copyright (C) 2008 The Android Open Source Project
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

#include <string.h>
#include <errno.h>
#include <pthread.h>

#include <cutils/log.h>
#include <cutils/atomic.h>
#include <hardware/hardware.h>
#include <hardware/gralloc.h>

#include <sys/ioctl.h>

#include "alloc_device.h"
#include "gralloc_priv.h"
#include "gralloc_helper.h"
#include "framebuffer_device.h"

#include <linux/ion.h>
#include <ion/ion.h>
#include "ion_sprd.h"

static int mFBIndex=0;
int alloc_backend_alloc(alloc_device_t* dev, size_t size, int usage, buffer_handle_t* pHandle)
{
	private_module_t* m = reinterpret_cast<private_module_t*>(dev->common.module);
	ion_user_handle_t ion_hnd;
	unsigned char *cpu_ptr = NULL;
	int shared_fd;
	int ret;
	unsigned int heap_mask;
	int ion_flags = 0;
	static int support_protected = 1; /* initially, assume we support protected memory */
	int lock_state = 0;

	/* Select heap type based on usage hints */
	if(usage & GRALLOC_USAGE_PROTECTED)
	{
		heap_mask = ION_HEAP_ID_MASK_MM;
	}
	else
	{
		if (usage & (GRALLOC_USAGE_VIDEO_BUFFER|GRALLOC_USAGE_CAMERA_BUFFER))
		{ 
       			heap_mask = ION_HEAP_ID_MASK_MM;
		}
		else if(usage & GRALLOC_USAGE_OVERLAY_BUFFER)
		{
       			heap_mask = ION_HEAP_ID_MASK_OVERLAY;
		}
		else if(usage & GRALLOC_USAGE_HW_FB)
		{
       			heap_mask = ION_HEAP_ID_MASK_FB;
		}
		else
		{
	       		heap_mask = ION_HEAP_ID_MASK_SYSTEM;
		}
	}

	if ( (usage & GRALLOC_USAGE_SW_READ_MASK) == GRALLOC_USAGE_SW_READ_OFTEN )
	{
		ion_flags = ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC;
	}

	ret = ion_alloc(m->ion_client, size, 0, heap_mask,
	                ion_flags, &ion_hnd );

	if ( ret != 0) 
	{
		AERR("Failed to ion_alloc from ion_client:%d", m->ion_client);
		return -1;
	}

	ret = ion_share( m->ion_client, ion_hnd, &shared_fd );
	if ( ret != 0 )
	{
		AERR( "ion_share( %d ) failed", m->ion_client );
		if ( 0 != ion_free( m->ion_client, ion_hnd ) ) AERR( "ion_free( %d ) failed", m->ion_client );		
		return -1;
	}

	if (!(usage & GRALLOC_USAGE_PROTECTED))
	{
		cpu_ptr = (unsigned char*)mmap( NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shared_fd, 0 );

		if ( MAP_FAILED == cpu_ptr )
		{
			AERR( "ion_map( %d ) failed", m->ion_client );
			if ( 0 != ion_free( m->ion_client, ion_hnd ) ) AERR( "ion_free( %d ) failed", m->ion_client );
			close( shared_fd );
			return -1;
		}
		lock_state = private_handle_t::LOCK_STATE_MAPPED;
	}

	private_handle_t *hnd = new private_handle_t( private_handle_t::PRIV_FLAGS_USES_ION, usage, size, cpu_ptr,
	                                              lock_state );

	if ( NULL != hnd )
	{
		hnd->share_fd = shared_fd;
		hnd->ion_hnd = ion_hnd;
		if (ION_HEAP_ID_MASK_FB == heap_mask)
		{
			hnd->buf_idx = mFBIndex;
			mFBIndex++;
		}
		*pHandle = hnd;
		return 0;
	}
	else
	{
		AERR( "Gralloc out of mem for ion_client:%d", m->ion_client );
	}

	close( shared_fd );

	if(!(usage & GRALLOC_USAGE_PROTECTED))
	{
		ret = munmap( cpu_ptr, size );
		if ( 0 != ret ) AERR( "munmap failed for base:%p size: %zd", cpu_ptr, size );
	}

	ret = ion_free( m->ion_client, ion_hnd );
	if ( 0 != ret ) AERR( "ion_free( %d ) failed", m->ion_client );
	return -1;
}

int alloc_backend_alloc_framebuffer(private_module_t* m, private_handle_t* hnd)
{
	struct fb_dmabuf_export fb_dma_buf;
	int res;

#ifdef FBIOGET_DMABUF
	res = ioctl( m->framebuffer->fd, FBIOGET_DMABUF, &fb_dma_buf );
	if(res == 0)
	{
		hnd->share_fd = fb_dma_buf.fd;
		return 0;
	}
	else
#endif/*FBIOGET_DMABUF*/
	{
		AINF("FBIOGET_DMABUF ioctl failed(%d). See gralloc_priv.h and the integration manual for vendor framebuffer integration", res);
#if MALI_ARCHITECTURE_UTGARD
		/* On Utgard we do not have a strict requirement of DMA-BUF integration */
		return 0;
#else
		return -1;
#endif
	}
}

void alloc_backend_alloc_free(private_handle_t const* hnd, private_module_t* m)
{
	if (hnd->flags & private_handle_t::PRIV_FLAGS_FRAMEBUFFER)
	{
		return;
	}
	else if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_UMP)
	{
		AERR( "Can't free ump memory for handle:%p. Not supported.", hnd );
	}
	else if ( hnd->flags & private_handle_t::PRIV_FLAGS_USES_ION )
	{
		/* Buffer might be unregistered already so we need to assure we have a valid handle*/
		if ( 0 != hnd->base )
		{
			if ( 0 != munmap( (void*)hnd->base, hnd->size ) ) AERR( "Failed to munmap handle %p", hnd );
		}
		close( hnd->share_fd );
		if ( 0 != ion_free( m->ion_client, hnd->ion_hnd ) ) AERR( "Failed to ion_free( ion_client: %d ion_hnd: %p )", m->ion_client, hnd->ion_hnd );
		memset( (void*)hnd, 0, sizeof( *hnd ) );
	}
}

int alloc_backend_open(alloc_device_t *dev)
{
	private_module_t *m = reinterpret_cast<private_module_t *>(dev->common.module);
	m->ion_client = ion_open();
	if ( m->ion_client < 0 )
	{
		AERR( "ion_open failed with %s", strerror(errno) );
		return -1;
	}

	return 0;
}

int alloc_backend_close(struct hw_device_t *device)
{
	alloc_device_t* dev = reinterpret_cast<alloc_device_t*>(device);
	if (dev)
	{
		private_module_t *m = reinterpret_cast<private_module_t*>(dev->common.module);
		if ( 0 != ion_close(m->ion_client) ) AERR( "Failed to close ion_client: %d err=%s", m->ion_client , strerror(errno));
		delete dev;
	}
	return 0;
}
