/* Copyright (c) 2012-2013, The Linux Foundataion. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef __SPRDCAMERA3HWI_MEM_H__
#define __SPRDCAMERA3HWI_MEM_H__
#include <hardware/camera3.h>
#include <utils/Mutex.h>

extern "C" {
#include <sys/types.h>
}

namespace sprdcamera {

typedef struct {
	int fd;
	size_t size;
	void *addr_phy;
	void *addr_vir;
#ifdef CONFIG_SPRD_PRIVATE_ZSL
	uint32_t valid;
#endif
}hal_mem_info_t;

// Base class for all memory types. Abstract.
class SprdCamera3Memory {

public:
    	int cleanCache(int index) {return cacheOps(index, 0);}
    	int invalidateCache(int index) {return cacheOps(index, 0);}
    	int cleanInvalidateCache(int index) {return cacheOps(index, 0);}

    	virtual int cacheOps(int index, unsigned int cmd) = 0;

    	SprdCamera3Memory();
    	virtual ~SprdCamera3Memory();

	virtual int map(buffer_handle_t *buffer_handle ,hal_mem_info_t *mem_info) = 0;
	virtual int unmap(buffer_handle_t *buffer_handle ,hal_mem_info_t *mem_info) = 0;

public:
	static int getUsage(int stream_type, cmr_uint &usage);
protected:
    	static bool mIsIOMMU;
protected:

    int cacheOpsInternal(int index, unsigned int cmd, void *vaddr);

};

// Internal heap memory is used for memories used internally
// They are allocated from /dev/ion. Examples are: capabilities,
// parameters, metadata, and internal YUV data for jpeg encoding.
class SprdCamera3HeapMemory : public SprdCamera3Memory {
public:
    	SprdCamera3HeapMemory();
    	virtual ~SprdCamera3HeapMemory();

    	virtual int cacheOps(int index, unsigned int cmd);
	virtual int map(buffer_handle_t *buffer_handle ,hal_mem_info_t *mem_info) {return 0;};
	virtual int unmap(buffer_handle_t *buffer_handle ,hal_mem_info_t *mem_info) {return 0;};

private:
};

// Gralloc Memory shared with frameworks
class SprdCamera3GrallocMemory : public SprdCamera3Memory {
public:
    	SprdCamera3GrallocMemory();
    	virtual ~SprdCamera3GrallocMemory();

	virtual int cacheOps(int index, unsigned int cmd);
	virtual int map(buffer_handle_t *buffer_handle ,hal_mem_info_t *mem_info);
	// memory optimization, CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT dont need iommu addr
	int map2(buffer_handle_t *buffer_handle ,hal_mem_info_t *mem_info);
	virtual int unmap(buffer_handle_t *buffer_handle, hal_mem_info_t *mem_info);

};

};
#endif
