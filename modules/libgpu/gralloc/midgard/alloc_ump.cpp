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

#include <ump/ump.h>
#include <ump/ump_ref_drv.h>

#if GRALLOC_SIMULATE_FAILURES
#include <cutils/properties.h>

/* system property keys for controlling simulated UMP allocation failures */
#define PROP_MALI_TEST_GRALLOC_FAIL_FIRST     "mali.test.gralloc.fail_first"
#define PROP_MALI_TEST_GRALLOC_FAIL_INTERVAL  "mali.test.gralloc.fail_interval"

static int __ump_alloc_should_fail()
{

	static unsigned int call_count  = 0;
	unsigned int        first_fail  = 0;
	int                 fail_period = 0;
	int                 fail        = 0;
	
	++call_count;

	/* read the system properties that control failure simulation */	
	{
		char prop_value[PROPERTY_VALUE_MAX];
		
		if (property_get(PROP_MALI_TEST_GRALLOC_FAIL_FIRST, prop_value, "0") > 0)
		{
			sscanf(prop_value, "%11u", &first_fail);
		}

		if (property_get(PROP_MALI_TEST_GRALLOC_FAIL_INTERVAL, prop_value, "0") > 0)
		{
			sscanf(prop_value, "%11u", &fail_period);
		}
	}

	/* failure simulation is enabled by setting the first_fail property to non-zero */
	if (first_fail > 0)
	{
		LOGI("iteration %u (fail=%u, period=%u)\n", call_count, first_fail, fail_period);
		
		fail = 	(call_count == first_fail) ||
				(call_count > first_fail && fail_period > 0 && 0 == (call_count - first_fail) % fail_period);
		
		if (fail) 
		{
			AERR("failed ump_ref_drv_allocate on iteration #%d\n", call_count);
		}
	}
	return fail;
}
#endif

int alloc_backend_alloc(alloc_device_t* dev, size_t size, int usage, buffer_handle_t* pHandle)
{
	{
		ump_handle ump_mem_handle;
		void *cpu_ptr;
		ump_secure_id ump_id;
		ump_alloc_constraints constraints;

		size = round_up_to_page_size(size);

		if( (usage&GRALLOC_USAGE_SW_READ_MASK) == GRALLOC_USAGE_SW_READ_OFTEN )
		{
			constraints =  UMP_REF_DRV_CONSTRAINT_USE_CACHE;
		}
		else
		{
			constraints = UMP_REF_DRV_CONSTRAINT_NONE;
		}

#ifdef GRALLOC_SIMULATE_FAILURES
		/* if the failure condition matches, fail this iteration */
		if (__ump_alloc_should_fail())
		{
			ump_mem_handle = UMP_INVALID_MEMORY_HANDLE;
		}
		else
#endif
		{
			/* protected memory not supported in UMP */
			if (!(usage & GRALLOC_USAGE_PROTECTED))
			{
				ump_mem_handle = ump_ref_drv_allocate(size, constraints);

				if (UMP_INVALID_MEMORY_HANDLE != ump_mem_handle)
				{
					cpu_ptr = ump_mapped_pointer_get(ump_mem_handle);
					if (NULL != cpu_ptr)
					{
						ump_id = ump_secure_id_get(ump_mem_handle);
						if (UMP_INVALID_SECURE_ID != ump_id)
						{
							private_handle_t* hnd = new private_handle_t(private_handle_t::PRIV_FLAGS_USES_UMP,
													 usage,
													 size,
													 cpu_ptr,
													 private_handle_t::LOCK_STATE_MAPPED,
													 ump_id,
													 ump_mem_handle);

							if (NULL != hnd)
							{
								*pHandle = hnd;
								return 0;
							}
							else
							{
								AERR( "gralloc_alloc_buffer() failed to allocate handle. ump_handle = %p, ump_id = %d", ump_mem_handle, ump_id );
							}
						}
						else
						{
							AERR( "gralloc_alloc_buffer() failed to retrieve valid secure id. ump_handle = %p", ump_mem_handle );
						}

						ump_mapped_pointer_release(ump_mem_handle);
					}
					else
					{
						AERR( "gralloc_alloc_buffer() failed to map UMP memory. ump_handle = %p", ump_mem_handle );
					}

					ump_reference_release(ump_mem_handle);
				}
				else
				{
					AERR( "gralloc_alloc_buffer() failed to allocate UMP memory. size:%d constraints: %d", size, constraints );
				}
			}
			else
			{
				AERR( "gralloc_alloc_buffer() protected UMP memory is not supported.");
			}
		}
		return -1;
	}
}

int alloc_backend_alloc_framebuffer(private_module_t* m, private_handle_t* hnd)
{
	hnd->ump_id = m->framebuffer->ump_id;
	/* create a backing ump memory handle if the framebuffer is exposed as a secure ID */
	if ( (int)UMP_INVALID_SECURE_ID != hnd->ump_id )
	{
		hnd->ump_mem_handle = ump_handle_create_from_secure_id( hnd->ump_id );
		if ( UMP_INVALID_MEMORY_HANDLE == hnd->ump_mem_handle )
		{
			AERR("unable to create UMP handle from secure ID %i\n", hnd->ump_id);
			return -1;
		}
	}

	return 0;
}

void alloc_backend_alloc_free(private_handle_t const* hnd, private_module_t* m)
{
	if (hnd->flags & private_handle_t::PRIV_FLAGS_FRAMEBUFFER)
	{
		if ( UMP_INVALID_MEMORY_HANDLE != hnd->ump_mem_handle )
		{
			ump_reference_release((ump_handle)hnd->ump_mem_handle);
		}
	}
	else if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_UMP)
	{
		/* Buffer might be unregistered so we need to check for invalid ump handle*/
		if ( UMP_INVALID_MEMORY_HANDLE != hnd->ump_mem_handle )
		{
			ump_mapped_pointer_release((ump_handle)hnd->ump_mem_handle);
			ump_reference_release((ump_handle)hnd->ump_mem_handle);
		}
	}
	else if ( hnd->flags & private_handle_t::PRIV_FLAGS_USES_ION )
	{
		AERR( "Can't free dma_buf memory for handle:%p. Not supported.", hnd);
	}
}

int alloc_backend_open(alloc_device_t *dev)
{
	ump_result ump_res = ump_open();
	if (UMP_OK != ump_res)
	{
		AERR( "UMP open failed with %d", ump_res );
		return -1;
	}

	return 0;
}

int alloc_backend_close(struct hw_device_t *device)
{
	alloc_device_t* dev = reinterpret_cast<alloc_device_t*>(device);
	if (dev)
	{
		ump_close(); // Our UMP memory refs will be released automatically here...
	}
	return 0;
}
