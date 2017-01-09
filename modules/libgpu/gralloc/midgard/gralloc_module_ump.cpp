/*
 * Copyright (C) 2013 ARM Limited. All rights reserved.
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

#include <ump/ump_ref_drv.h>
static int s_ump_is_open = 0;

int gralloc_backend_register(private_handle_t* hnd)
{
	int retval = -EINVAL;

	switch (hnd->flags & (private_handle_t::PRIV_FLAGS_USES_UMP |
	                      private_handle_t::PRIV_FLAGS_USES_ION))
	{
	case private_handle_t::PRIV_FLAGS_USES_UMP:
		if (!s_ump_is_open)
		{
			ump_result res = ump_open(); // MJOLL-4012: UMP implementation needs a ump_close() for each ump_open
			if (res != UMP_OK)
			{
				AERR("Failed to open UMP library with res=%d", res);
			}
			s_ump_is_open = 1;
		}

		if (s_ump_is_open)
		{
			hnd->ump_mem_handle = ump_handle_create_from_secure_id(hnd->ump_id);
			if (UMP_INVALID_MEMORY_HANDLE != (ump_handle)hnd->ump_mem_handle)
			{
				hnd->base = ump_mapped_pointer_get(hnd->ump_mem_handle);
				if (0 != hnd->base)
				{
					hnd->lockState = private_handle_t::LOCK_STATE_MAPPED;
					hnd->writeOwner = 0;
					hnd->lockState = 0;

					return 0;
				}
				else
				{
					AERR("Failed to map UMP handle %p", hnd->ump_mem_handle );
				}

				ump_reference_release((ump_handle)hnd->ump_mem_handle);
			}
			else
			{
				AERR("Failed to create UMP handle %p", hnd->ump_mem_handle );
			}
		}
		break;
	case private_handle_t::PRIV_FLAGS_USES_ION:
		AERR("Gralloc does not support DMA_BUF. Unable to map memory for handle %p", hnd );
		break;
	}

	return retval;
}

void gralloc_backend_unregister(private_handle_t* hnd)
{
	switch (hnd->flags & (private_handle_t::PRIV_FLAGS_USES_UMP |
	                      private_handle_t::PRIV_FLAGS_USES_ION))
	{
	case private_handle_t::PRIV_FLAGS_USES_UMP:
		ump_mapped_pointer_release((ump_handle)hnd->ump_mem_handle);
		hnd->base = 0;
		ump_reference_release((ump_handle)hnd->ump_mem_handle);
		hnd->ump_mem_handle = UMP_INVALID_MEMORY_HANDLE;
		break;
	case private_handle_t::PRIV_FLAGS_USES_ION:
		AERR( "Can't unregister DMA_BUF buffer for hnd %p. Not supported", hnd );
		break;
	}
}

void gralloc_backend_sync(private_handle_t* hnd)
{
	switch (hnd->flags & (private_handle_t::PRIV_FLAGS_USES_UMP |
	                      private_handle_t::PRIV_FLAGS_USES_ION))
	{
	case private_handle_t::PRIV_FLAGS_USES_UMP:
		ump_cpu_msync_now((ump_handle)hnd->ump_mem_handle, UMP_MSYNC_CLEAN_AND_INVALIDATE, (void*)hnd->base, hnd->size);
		break;
	case private_handle_t::PRIV_FLAGS_USES_ION:
		AERR( "Buffer %p is DMA_BUF type but it is not supported", hnd );
		break;
	}
}
