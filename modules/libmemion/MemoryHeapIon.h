/*
 * Copyright (C) 2008 The Android Open Source Project
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.
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

#ifndef ANDROID_MEMORY_HEAP_ION_H
#define ANDROID_MEMORY_HEAP_ION_H

#include <stdlib.h>
#include <stdint.h>

//#include <binder/MemoryHeapBase.h>
//#include <binder/IMemory.h>
#include <utils/SortedVector.h>
#include <utils/threads.h>


namespace android {

// ---------------------------------------------------------------------------

class MemoryHeapIon: public RefBase
{
public:
    enum {
        READ_ONLY = 0x00000001,
        // memory won't be mapped locally, but will be mapped in the remote
        // process.
        DONT_MAP_LOCALLY = 0x00000100,
        NO_CACHING = 0x00000200
    };

    MemoryHeapIon();
    MemoryHeapIon(const char*, size_t, uint32_t, long unsigned int);
    ~MemoryHeapIon();
    int         getHeapID() const;
    void*       getBase() const;

    static int Get_phy_addr_from_ion(int fd, unsigned long *phy_addr, size_t *size);/*fd is "fd of the corresponding ion hanlde"*/
    int get_phy_addr_from_ion(unsigned long *phy_addr, size_t *size);
    static int Flush_ion_buffer(int buffer_fd, void *v_addr,void *p_addr, size_t size);
    int flush_ion_buffer(void *v_addr, void *p_addr, size_t size);

    int get_gsp_iova(unsigned long *mmu_addr, size_t *size);
    int free_gsp_iova(unsigned long mmu_addr, size_t size);
    int get_mm_iova(unsigned long *mmu_addr, size_t *size);
    int free_mm_iova(unsigned long mmu_addr, size_t size);
    static int Get_gsp_iova(int buffer_fd, unsigned long *mmu_addr, size_t *size);
    static int Free_gsp_iova(int buffer_fd, unsigned long mmu_addr, size_t size);
    static int Get_mm_iova(int buffer_fd,unsigned long *mmu_addr, size_t *size);
    static int Free_mm_iova(int buffer_fd,unsigned long mmu_addr, size_t size);
    static bool Gsp_iommu_is_enabled(void);
    static bool Mm_iommu_is_enabled(void);

    int get_iova(int master_id, unsigned long *mmu_addr, size_t *size);
    int free_iova(int master_id, unsigned long mmu_addr, size_t size);
    static int Get_iova(int master_id, int buffer_fd,
        unsigned long *mmu_addr, size_t *size);
    static int Free_iova(int master_id, int buffer_fd,
        unsigned long mmu_addr, size_t size);
    int get_kaddr(uint64_t *kaddr, size_t *size);
    int free_kaddr();
    static int Get_kaddr(int buffer_fd,
                    uint64_t *kaddr, size_t *size);
    static int Free_kaddr(int buffer_fd);
    static bool IOMMU_is_enabled(int master_id);

private:
    status_t mapIonFd(int fd, size_t size, unsigned long memory_type, int flags);

    int mIonDeviceFd;  /*fd we get from open("/dev/ion")*/
    int mIonHandle;  /*handle we get from ION_IOC_ALLOC*/ 
    int         mFD;
    size_t      mSize;
    void*       mBase;
};
// ---------------------------------------------------------------------------
}; // namespace android

#endif // ANDROID_MEMORY_HEAP_ION_H
