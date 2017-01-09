/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _ION_SPRD_H
#define _ION_SPRD_H

//#define size_t  unsigned int
//#define uint16_t  (unsigned short)
//#define uint32_t  (unsigned int)
//#define int32_t   (signed int)
#define status_t    int
#define NO_ERROR 0

#define ION_SET_CACHED(__cache)     (__cache | ION_FLAG_CACHED)
#define ION_SET_UNCACHED(__cache)   (__cache & ~ION_FLAG_CACHED)
#define ION_IS_CACHED(__flags)      ((__flags) & ION_FLAG_CACHED)


#define ION_HEAP_ID_SYSTEM	1
#define ION_HEAP_ID_MM		2
#define ION_HEAP_ID_OVERLAY	3

#define ION_HEAP_ID_MASK_SYSTEM 	(1<<ION_HEAP_ID_SYSTEM)
#define ION_HEAP_ID_MASK_MM		(1<<ION_HEAP_ID_MM)
#define ION_HEAP_ID_MASK_OVERLAY	(1<<ION_HEAP_ID_OVERLAY)



//#define getpagesize_gsp()   4096

struct ion_phys_data
{
    int fd_buffer;
    unsigned long phys;
    size_t size;
};

struct ion_mmu_data
{
    int fd_buffer;
    unsigned long iova_addr;
    size_t iova_size;
};

struct ion_msync_data
{
    int fd_buffer;
    void *vaddr;
    void *paddr;
    size_t size;
};



enum
{
    READ_ONLY = 0x00000001,
    // memory won't be mapped locally, but will be mapped in the remote
    // process.
    DONT_MAP_LOCALLY = 0x00000100,
    NO_CACHING = 0x00000200
};

enum ION_SPRD_CUSTOM_CMD
{
    ION_SPRD_CUSTOM_PHYS,
    ION_SPRD_CUSTOM_MSYNC,

    /* to get/free mmu iova */ //added by yfs
    ION_SPRD_CUSTOM_GSP_MAP,
    ION_SPRD_CUSTOM_GSP_UNMAP,
    ION_SPRD_CUSTOM_MM_MAP,
    ION_SPRD_CUSTOM_MM_UNMAP,
    ION_SPRD_CUSTOM_FENCE_CREATE,
    ION_SPRD_CUSTOM_FENCE_SIGNAL,
    ION_SPRD_CUSTOM_FENCE_DUP,
};

#endif /* _ION_SPRD_H */
