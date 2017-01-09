//------------------------------------------------------------------------------
// File: android_memory_alloc.cpp
//
// Copyright (c) 2015, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------


#include "MemoryHeapIon.h"
#include "ion_sprd.h"
#include "vdi.h"
#include "vdi_osal.h"

using namespace android;

#define SPRD_ION_DEV "/dev/ion"

int alloc_mem_from_mmu(vpu_pool_t* vpu_mem_pool, vpu_buffer_t* vdb)
{

    VLOG(ERR, "%s, %d: \n",__func__, __LINE__);

    MemoryHeapIon* mPmem_extra;
    bool IOMMUEnabled = MemoryHeapIon::IOMMU_is_enabled(ION_MM);

    if (IOMMUEnabled) {
        mPmem_extra = new MemoryHeapIon(SPRD_ION_DEV, vdb->size, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
    } else {
        mPmem_extra = new MemoryHeapIon(SPRD_ION_DEV, vdb->size, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
    }

    int fd = mPmem_extra->getHeapID();
    if(fd >= 0) {
        int ret;
        unsigned long phys_addr;
        size_t buffer_size;

        if (IOMMUEnabled) {
            ret = mPmem_extra->get_iova(ION_MM, &phys_addr, &buffer_size);
        } else {
            ret = mPmem_extra->get_phy_addr_from_ion(&phys_addr, &buffer_size);
        }

        if(ret < 0) {
            VLOG (ERR, "mPmem_extra: get phy addr fail %d \n",ret);
            return -1;
        }

        vdb->phys_addr = phys_addr;
        vdb->virt_addr = (unsigned long)mPmem_extra->getBase();
        //vdb->size = buffer_size;


        VLOG(ERR, "pmem 0x%lx - 0x%lx - %d \n", vdb->phys_addr, vdb->virt_addr, vdb->size);

        for (int i=0; i<MAX_VPU_BUFFER_POOL; i++)
        {
            if (vpu_mem_pool->mNode[i].inuse == 0)
            {
                vpu_mem_pool->mNode[i].mPmem_extra= (void*)mPmem_extra;
                vpu_mem_pool->mNode[i].phys_addr = vdb->phys_addr;
                vpu_mem_pool->mNode[i].virt_addr = vdb->virt_addr;
                vpu_mem_pool->mNode[i].size = buffer_size;

                vpu_mem_pool->count++;
                vpu_mem_pool->mNode[i].inuse = 1;
                break;
            }

            if (i == MAX_VPU_BUFFER_POOL) {
                VLOG(ERR, "%s, %d: mmu buffer count is not enough \n", __func__, __LINE__);
                return -1;
            }
        }

    } else {
        VLOG (ERR, "mPmem_extra: getHeapID fail %d \n", fd);
        return -1;
    }

    return 0;
}


int free_mem_from_mmu(vpu_pool_t* vpu_mem_pool, vpu_buffer_t* vdb)
{
    bool IOMMUEnabled = MemoryHeapIon::IOMMU_is_enabled(ION_MM);

    VLOG(ERR, "%s, %d: free phys_addr = %p \n", __func__, __LINE__, vdb->phys_addr);

    for (int i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        unsigned long phys_addr = vpu_mem_pool->mNode[i].phys_addr;
        size_t size = vpu_mem_pool->mNode[i].size;

        if (phys_addr == vdb->phys_addr)
        {
            MemoryHeapIon* mPmem_extra = (MemoryHeapIon*)vpu_mem_pool->mNode[i].mPmem_extra;

            if (vpu_mem_pool->mNode[i].inuse == 0) {
                VLOG (ERR, "%s, %d: This phy_addr (%p) is not used \n", __func__, __LINE__, vdb->phys_addr);
                return -1;
            }

            if (vpu_mem_pool->mNode[i].virt_addr != NULL) {
                if (IOMMUEnabled) {
                    //(MemoryHeapIon*)(vpu_mem_pool->mNode[i].mPmem_extra)->free_iova(ION_MM, phys_addr, size);
                    mPmem_extra->free_iova(ION_MM, phys_addr, size);
                }

                //delete (MemoryHeapIon*)(vpu_mem_pool->mNode[i].mPmem_extra);
                delete mPmem_extra;
                memset(&vpu_mem_pool->mNode[i], 0, sizeof(mmu_buffer_t));
            }

            vpu_mem_pool->count--;
            if (vpu_mem_pool->count < 0)
            {
                VLOG (ERR, "%s, %d: alloc - free memory not match\n", __func__, __LINE__);
            }
            break;
        }
    }

    VLOG(ERR, "%s, %d: VPU_MEM_POOL.count = %d \n", __func__, __LINE__, vpu_mem_pool->count);

    return 0;
}






