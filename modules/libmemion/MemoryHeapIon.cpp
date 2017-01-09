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

#define LOG_TAG "MemoryHeapIon"

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <cutils/log.h>

#include <linux/ion.h>

#include "ion_sprd.h"
#include "MemoryHeapIon.h"

namespace android {

MemoryHeapIon::MemoryHeapIon() : mIonDeviceFd(-1), mIonHandle(-1)
{
}

MemoryHeapIon::MemoryHeapIon(const char* device, size_t size,
    uint32_t flags, unsigned long memory_types)
    : mIonDeviceFd(-1),
      mIonHandle(-1),
      mFD(-1),
      mSize(0),
      mBase(NULL) {
    int open_flags = O_RDONLY;
    if (flags & NO_CACHING)
         open_flags |= O_SYNC;

    int fd = open(device, open_flags);
    if (fd >= 0) {
            const size_t pagesize = getpagesize();
            size = ((size + pagesize-1) & ~(pagesize-1));
            if (mapIonFd(fd, size, memory_types, flags) == NO_ERROR) {
                //MemoryHeapBase::setDevice(device);
            }
    }else {
        ALOGE("%s, open ion fail, %d(%s)", __func__, -errno, strerror(errno));
    }
}

status_t MemoryHeapIon::mapIonFd(int fd, size_t size, unsigned long memory_type, int uflags)
{
    /* If size is 0, just fail the mmap. There is no way to get the size
     * with ion
     */
    int map_fd;

    struct ion_allocation_data data;
    struct ion_fd_data fd_data;
    struct ion_handle_data handle_data;
    void *base = NULL;

    data.len = size;
    data.align = getpagesize();
#if (ION_DRIVER_VERSION == 1)
    data.heap_id_mask = memory_type;
    //if cached buffer , force set the lowest two bits 11
    if((memory_type&(1<<31)))
    {
        data.flags = ((memory_type&(1<<31)) | 3);
    }
    else
    {
        data.flags = 0;
    }
#else
    data.flags = memory_type;
#endif

    if (ioctl(fd, ION_IOC_ALLOC, &data) < 0) {
        ALOGE("%s: ION_IOC_ALLOC error: %d (%s)", __func__, -errno, strerror(errno));
        close(fd);
        return -errno;
    }

    if ((uflags & DONT_MAP_LOCALLY) == 0) {
        int flags = 0;

        fd_data.handle = data.handle;

        if (ioctl(fd, ION_IOC_SHARE, &fd_data) < 0) {
            ALOGE("%s: ION_IOC_SHARE error: %d (%s)", __func__, -errno, strerror(errno));
            handle_data.handle = data.handle;
            ioctl(fd, ION_IOC_FREE, &handle_data);
            close(fd);
            return -errno;
        }

        base = (uint8_t*)mmap(0, size,
                PROT_READ|PROT_WRITE, MAP_SHARED|flags, fd_data.fd, 0);
        if (base == MAP_FAILED) {
            ALOGE("mmap(fd=%d, size=%u) failed (%s)",
                    fd, uint32_t(size), strerror(errno));
            handle_data.handle = data.handle;
            ioctl(fd, ION_IOC_FREE, &handle_data);
            close(fd);
            return -errno;
        }
    }
    mIonHandle = data.handle;
    mIonDeviceFd = fd;
    mFD = fd_data.fd;
    mSize = size;
    mBase = base;

    return NO_ERROR;
}

MemoryHeapIon::~MemoryHeapIon()
{
    struct ion_handle_data data;

    data.handle = mIonHandle;

    munmap(mBase, mSize);
    if (mIonDeviceFd > 0) {
        if (ioctl(mIonDeviceFd, ION_IOC_FREE, &data) < 0) {
            ALOGE("%s: ION_IOC_FREE error: %d (%s)", __func__, -errno, strerror(errno));
        }
        close(mIonDeviceFd);
    }

    int fd = android_atomic_or(-1, &mFD);
    if (fd >= 0) {
        mSize = 0;
        mBase = NULL;
        close(fd);
    }
}

int MemoryHeapIon::getHeapID() const {
    return mFD;
}

void* MemoryHeapIon::getBase() const {
    return mBase;
}

int  MemoryHeapIon::Get_phy_addr_from_ion(int buffer_fd,
                                        unsigned long *phy_addr, size_t *size){
    int fd = open("/dev/ion", O_SYNC);
    if(fd<0){
        ALOGE("%s:open dev ion error!",__func__);
        return -1;
    }else{
        int ret;
        struct ion_phys_data phys_data;
        struct ion_custom_data  custom_data;
        phys_data.fd_buffer = buffer_fd;
        custom_data.cmd = ION_SPRD_CUSTOM_PHYS;
        custom_data.arg = (unsigned long)&phys_data;
        ret = ioctl(fd,ION_IOC_CUSTOM,&custom_data);
        *phy_addr = phys_data.phys;
        *size = phys_data.size;
        close(fd);
        if(ret)
        {
            ALOGE("%s: Getphyaddr error: %d", __func__, ret);
            return -2;
        }
    }
    return 0;
}

int MemoryHeapIon::get_phy_addr_from_ion(unsigned long *phy_addr, size_t *size){
    if(mIonDeviceFd<0){
        ALOGE("%s:open dev ion error!",__func__);
        return -1;
    }else{
        int ret;
        struct ion_phys_data phys_data;
        struct ion_custom_data  custom_data;
        phys_data.fd_buffer = mFD;
        custom_data.cmd = ION_SPRD_CUSTOM_PHYS;
        custom_data.arg = (unsigned long)&phys_data;
        ret = ioctl(mIonDeviceFd,ION_IOC_CUSTOM,&custom_data);
        *phy_addr = phys_data.phys;
        *size = phys_data.size;
        if(ret)
        {
            ALOGE("%s: getphyaddr error: %d", __func__, ret);
            return -2;
        }
     }
    return 0;
}

int  MemoryHeapIon::Flush_ion_buffer(int buffer_fd, void *v_addr, void *p_addr, size_t size){
       int fd = open("/dev/ion", O_SYNC);
    if(fd<0){
        ALOGE("%s:open dev ion error!",__func__);
        return -1;
    }else{
        int ret;
        struct ion_msync_data msync_data;
        struct ion_custom_data  custom_data;

        msync_data.fd_buffer = buffer_fd;
        msync_data.vaddr = v_addr;
        msync_data.paddr = p_addr;
        msync_data.size = size;
        custom_data.cmd = ION_SPRD_CUSTOM_MSYNC;
        custom_data.arg = (unsigned long)&msync_data;
        ret = ioctl(fd,ION_IOC_CUSTOM,&custom_data);
        close(fd);
        if(ret)
        {
            ALOGE("%s: return error: %d", __func__, ret);
            return -2;
        }
    }
    return 0;
}

int MemoryHeapIon::flush_ion_buffer(void *v_addr, void *p_addr, size_t size){
    if(mIonDeviceFd<0){
        return -1;
    }else{
        int ret;
        struct ion_msync_data msync_data;
        struct ion_custom_data  custom_data;

        if (((unsigned char *)v_addr < (unsigned char *)mBase)  ||
			((unsigned char *)v_addr + size > (unsigned char *)mBase + mSize)){
             ALOGE("flush_ion_buffer error  mBase=0x%lx,mSize=0x%zx",
                    (unsigned long)mBase, mSize);
             ALOGE("flush_ion_buffer error  v_addr=0x%lx,p_addr=0x%lx,size=0x%zx",
                    (unsigned long)v_addr,(unsigned long)p_addr,size);

             return -3;
        }
        msync_data.fd_buffer = mFD;
        msync_data.vaddr = v_addr;
        msync_data.paddr = p_addr;
        msync_data.size = size;
        custom_data.cmd = ION_SPRD_CUSTOM_MSYNC;
        custom_data.arg = (unsigned long)&msync_data;
        ret = ioctl(mIonDeviceFd,ION_IOC_CUSTOM,&custom_data);
        if(ret)
        {
            ALOGE("%s: return error: %d", __func__, ret);
            return -2;
        }
     }
    return 0;
}

int MemoryHeapIon::get_gsp_iova(unsigned long *mmu_addr, size_t *size){
    if(mIonDeviceFd<0){
        ALOGE("%s:open dev ion error!",__func__);
        return -1;
    }else{
        int ret;
        struct ion_mmu_data mmu_data;
        struct ion_custom_data  custom_data;

        mmu_data.master_id = ION_GSP;
        mmu_data.fd_buffer = mFD;
        custom_data.cmd = ION_SPRD_CUSTOM_GSP_MAP;
        custom_data.arg = (unsigned long)&mmu_data;
        ret = ioctl(mIonDeviceFd,ION_IOC_CUSTOM,&custom_data);
        *mmu_addr = mmu_data.iova_addr;
        *size = mmu_data.iova_size;
        if(ret)
        {
            ALOGE("%s: return error: %d", __func__, ret);
            return -2;
        }
     }
    return 0;
}

int MemoryHeapIon::free_gsp_iova(unsigned long mmu_addr, size_t size){
    if(mIonDeviceFd<0){
        ALOGE("%s:open dev ion error!",__func__);
        return -1;
    }else{
        int ret;
        struct ion_mmu_data mmu_data;
        struct ion_custom_data  custom_data;

        mmu_data.master_id = ION_GSP;
        mmu_data.fd_buffer = mFD;
        mmu_data.iova_addr = mmu_addr;
        mmu_data.iova_size = size;
        custom_data.cmd = ION_SPRD_CUSTOM_GSP_UNMAP;
        custom_data.arg = (unsigned long)&mmu_data;
        ret = ioctl(mIonDeviceFd,ION_IOC_CUSTOM,&custom_data);
        if(ret)
        {
            ALOGE("%s: return error: %d", __func__, ret);
            return -2;
        }
     }
    return 0;
}

int MemoryHeapIon::get_mm_iova(unsigned long *mmu_addr, size_t *size){
    if(mIonDeviceFd<0){
        ALOGE("%s:open dev ion error!",__func__);
        return -1;
    }else{
        int ret;
        struct ion_mmu_data mmu_data;
        struct ion_custom_data  custom_data;

        mmu_data.master_id = ION_MM;
        mmu_data.fd_buffer = mFD;
        custom_data.cmd = ION_SPRD_CUSTOM_MM_MAP;
        custom_data.arg = (unsigned long)&mmu_data;
        ret = ioctl(mIonDeviceFd,ION_IOC_CUSTOM,&custom_data);
        *mmu_addr = mmu_data.iova_addr;
        *size = mmu_data.iova_size;
        if(ret)
        {
            ALOGE("%s: return error: %d", __func__, ret);
            return -2;
        }
     }
    return 0;
}

int MemoryHeapIon::free_mm_iova(unsigned long mmu_addr, size_t size){
    if(mIonDeviceFd<0){
        ALOGE("%s:open dev ion error!",__func__);
        return -1;
    }else{
        int ret;
        struct ion_mmu_data mmu_data;
        struct ion_custom_data  custom_data;

        mmu_data.master_id = ION_MM;
        mmu_data.fd_buffer = mFD;
        mmu_data.iova_addr = mmu_addr;
        mmu_data.iova_size = size;
        custom_data.cmd = ION_SPRD_CUSTOM_MM_UNMAP;
        custom_data.arg = (unsigned long)&mmu_data;
        ret = ioctl(mIonDeviceFd,ION_IOC_CUSTOM,&custom_data);
        if(ret)
        {
            ALOGE("%s: return error: %d", __func__, ret);
            return -2;
        }
     }
    return 0;
}
int MemoryHeapIon::Get_gsp_iova(int buffer_fd,unsigned long *mmu_addr, size_t *size){
    int fd = open("/dev/ion", O_SYNC);
    if(fd<0){
        ALOGE("%s:open dev ion error!",__func__);
        return -1;
    }else{
        int ret;
        struct ion_mmu_data mmu_data;
        struct ion_custom_data  custom_data;

        mmu_data.master_id = ION_GSP;
        mmu_data.fd_buffer = buffer_fd;
        custom_data.cmd = ION_SPRD_CUSTOM_GSP_MAP;
        custom_data.arg = (unsigned long)&mmu_data;
        ret = ioctl(fd,ION_IOC_CUSTOM,&custom_data);
        *mmu_addr = mmu_data.iova_addr;
        *size = mmu_data.iova_size;
        close(fd);
        if(ret)
        {
            ALOGE("%s: return error: %d", __func__, ret);
            return -2;
        }
    }
    return 0;
}
int MemoryHeapIon::Get_mm_iova(int buffer_fd,unsigned long *mmu_addr, size_t *size){
    int fd = open("/dev/ion", O_SYNC);
    if(fd<0){
        ALOGE("%s:open dev ion error!",__func__);
        return -1;
    }else{
        int ret;
        struct ion_mmu_data mmu_data;
        struct ion_custom_data  custom_data;

        mmu_data.master_id = ION_MM;
        mmu_data.fd_buffer =  buffer_fd;
        custom_data.cmd = ION_SPRD_CUSTOM_MM_MAP;
        custom_data.arg = (unsigned long)&mmu_data;
        ret = ioctl(fd,ION_IOC_CUSTOM,&custom_data);
        *mmu_addr = mmu_data.iova_addr;
        *size = mmu_data.iova_size;
        close(fd);
        if(ret)
        {
            ALOGE("%s: return error: %d", __func__, ret);
            return -2;
        }
    }
    return 0;
}

int MemoryHeapIon::Free_gsp_iova(int buffer_fd,unsigned long mmu_addr, size_t size){
    int fd = open("/dev/ion", O_SYNC);
    if(fd<0){
        ALOGE("%s:open dev ion error!",__func__);
        return -1;
    }else{
        int ret;
        struct ion_mmu_data mmu_data;
        struct ion_custom_data  custom_data;

        mmu_data.master_id = ION_GSP;
        mmu_data.fd_buffer = buffer_fd;
        mmu_data.iova_addr = mmu_addr;
        mmu_data.iova_size = size;
        custom_data.cmd = ION_SPRD_CUSTOM_GSP_UNMAP;
        custom_data.arg = (unsigned long)&mmu_data;
        ret = ioctl(fd,ION_IOC_CUSTOM,&custom_data);
        close(fd);
        if(ret)
        {
            ALOGE("%s: return error: %d", __func__, ret);
            return -2;
        }
    }
    return 0;
}
int MemoryHeapIon::Free_mm_iova(int buffer_fd,unsigned long mmu_addr, size_t size){
    int fd = open("/dev/ion", O_SYNC);
    if(fd<0){
        ALOGE("%s:open dev ion error!",__func__);
        return -1;
    }else{
        int ret;
        struct ion_mmu_data mmu_data;
        struct ion_custom_data  custom_data;

        mmu_data.master_id = ION_MM;
        mmu_data.fd_buffer = buffer_fd;
        mmu_data.iova_addr = mmu_addr;
        mmu_data.iova_size = size;
        custom_data.cmd = ION_SPRD_CUSTOM_MM_UNMAP;
        custom_data.arg = (unsigned long)&mmu_data;
        ret = ioctl(fd,ION_IOC_CUSTOM,&custom_data);
        close(fd);
        if(ret)
        {
            ALOGE("%s: return error: %d", __func__, ret);
            return -2;
        }
    }
    return 0;
}

bool MemoryHeapIon::Gsp_iommu_is_enabled(void)
{
	if(access("/dev/sprd_iommu_gsp",F_OK)<0)
	{
		return false;
	}
	return true;
}

bool MemoryHeapIon::Mm_iommu_is_enabled(void)
{
	if(access("/dev/sprd_iommu_mm",F_OK)<0)
	{
		return false;
	}
	return true;
}

int MemoryHeapIon::get_iova(int master_id, unsigned long *mmu_addr, size_t *size) {
    if(mIonDeviceFd<0){
        ALOGE("%s:open dev ion error!",__func__);
        return -1;
    }else{
        int ret;
        struct ion_mmu_data mmu_data;
        struct ion_custom_data  custom_data;

        mmu_data.master_id = master_id;
        mmu_data.fd_buffer = mFD;
        custom_data.cmd = ION_SPRD_CUSTOM_MAP;
        custom_data.arg = (unsigned long)&mmu_data;
        ret = ioctl(mIonDeviceFd,ION_IOC_CUSTOM,&custom_data);
        *mmu_addr = mmu_data.iova_addr;
        *size = mmu_data.iova_size;
        if(ret)
        {
            ALOGE("%s: return error: %d", __func__, ret);
            return -2;
        }
     }

    return 0;
}

int MemoryHeapIon::free_iova(int master_id, unsigned long mmu_addr, size_t size) {
    if(mIonDeviceFd<0){
        ALOGE("%s:open dev ion error!",__func__);
        return -1;
    }else{
        int ret;
        struct ion_mmu_data mmu_data;
        struct ion_custom_data  custom_data;

        mmu_data.master_id = master_id;
        mmu_data.fd_buffer = mFD;
        mmu_data.iova_addr = mmu_addr;
        mmu_data.iova_size = size;
        custom_data.cmd = ION_SPRD_CUSTOM_UNMAP;
        custom_data.arg = (unsigned long)&mmu_data;
        ret = ioctl(mIonDeviceFd,ION_IOC_CUSTOM,&custom_data);
        if(ret)
        {
            ALOGE("%s: return error: %d", __func__, ret);
            return -2;
        }
     }

    return 0;
}

int MemoryHeapIon::Get_iova(int master_id, int buffer_fd,
        unsigned long *mmu_addr, size_t *size) {
    int fd = open("/dev/ion", O_SYNC);

    if(fd<0){
        ALOGE("%s:open dev ion error!",__func__);
        return -1;
    }else{
        int ret;
        struct ion_mmu_data mmu_data;
        struct ion_custom_data  custom_data;

        mmu_data.master_id = master_id;
        mmu_data.fd_buffer =  buffer_fd;
        custom_data.cmd = ION_SPRD_CUSTOM_MAP;
        custom_data.arg = (unsigned long)&mmu_data;
        ret = ioctl(fd,ION_IOC_CUSTOM,&custom_data);
        *mmu_addr = mmu_data.iova_addr;
        *size = mmu_data.iova_size;
        close(fd);
        if(ret)
        {
            ALOGE("%s: return error: %d", __func__, ret);
            return -2;
        }
    }

    return 0;
}

int MemoryHeapIon::Free_iova(int master_id, int buffer_fd,
        unsigned long mmu_addr, size_t size) {
    int fd = open("/dev/ion", O_SYNC);

    if(fd<0){
        ALOGE("%s:open dev ion error!",__func__);
        return -1;
    }else{
        int ret;
        struct ion_mmu_data mmu_data;
        struct ion_custom_data  custom_data;

        mmu_data.master_id = master_id;
        mmu_data.fd_buffer = buffer_fd;
        mmu_data.iova_addr = mmu_addr;
        mmu_data.iova_size = size;
        custom_data.cmd = ION_SPRD_CUSTOM_UNMAP;
        custom_data.arg = (unsigned long)&mmu_data;
        ret = ioctl(fd,ION_IOC_CUSTOM,&custom_data);
        close(fd);
        if(ret)
        {
            ALOGE("%s: return error: %d", __func__, ret);
            return -2;
        }
    }

    return 0;
}

int MemoryHeapIon::get_kaddr(uint64_t *kaddr, size_t *size) {
    if (mIonDeviceFd < 0) {
        ALOGE("%s:open dev ion error!",__func__);
        return -1;
    } else {
        int ret;
        struct ion_kmap_data kmap_data;
        struct ion_custom_data  custom_data;

        kmap_data.fd_buffer = mFD;
        custom_data.cmd = ION_SPRD_CUSTOM_MAP_KERNEL;
        custom_data.arg = (unsigned long)&kmap_data;
        ret = ioctl(mIonDeviceFd, ION_IOC_CUSTOM, &custom_data);
        *kaddr = kmap_data.kaddr;
        *size = kmap_data.size;
        if (ret) {
            ALOGE("%s: return error: %d", __func__, ret);
            return -2;
        }
     }

    return 0;
}

int MemoryHeapIon::free_kaddr() {
    if (mIonDeviceFd < 0) {
        ALOGE("%s:open dev ion error!", __func__);
        return -1;
    } else {
        int ret;
        struct ion_kunmap_data kunmap_data;
        struct ion_custom_data  custom_data;

        kunmap_data.fd_buffer = mFD;
        custom_data.cmd = ION_SPRD_CUSTOM_UNMAP_KERNEL;
        custom_data.arg = (unsigned long)&kunmap_data;
        ret = ioctl(mIonDeviceFd, ION_IOC_CUSTOM, &custom_data);
        if (ret) {
            ALOGE("%s: return error: %d", __func__, ret);
            return -2;
        }
     }

    return 0;
}

int MemoryHeapIon::Get_kaddr(int buffer_fd,
                uint64_t *kaddr, size_t *size) {
    int fd = open("/dev/ion", O_SYNC);

    if (fd < 0) {
        ALOGE("%s:open dev ion error!", __func__);
        return -1;
    } else {
        int ret;
        struct ion_kmap_data kmap_data;
        struct ion_custom_data  custom_data;

        kmap_data.fd_buffer =  buffer_fd;
        custom_data.cmd = ION_SPRD_CUSTOM_MAP_KERNEL;
        custom_data.arg = (unsigned long)&kmap_data;
        ret = ioctl(fd,ION_IOC_CUSTOM,&custom_data);
        *kaddr = kmap_data.kaddr;
        *size = kmap_data.size;
        close(fd);
        if (ret) {
            ALOGE("%s: return error: %d", __func__, ret);
            return -2;
        }
    }

    return 0;
}

int MemoryHeapIon::Free_kaddr(int buffer_fd) {
    int fd = open("/dev/ion", O_SYNC);

    if (fd < 0) {
        ALOGE("%s:open dev ion error!", __func__);
        return -1;
    } else {
        int ret;
        struct ion_kunmap_data kunmap_data;
        struct ion_custom_data  custom_data;

        kunmap_data.fd_buffer =  buffer_fd;
        custom_data.cmd = ION_SPRD_CUSTOM_UNMAP_KERNEL;
        custom_data.arg = (unsigned long)&kunmap_data;
        ret = ioctl(fd,ION_IOC_CUSTOM,&custom_data);
        close(fd);
        if (ret) {
            ALOGE("%s: return error: %d", __func__, ret);
            return -2;
        }
    }

    return 0;
}

bool MemoryHeapIon::IOMMU_is_enabled(int master_id)
{
    int ret;

    switch(master_id) {
        case ION_GSP:
            ret = access("/dev/sprd_iommu_gsp",F_OK);
            break;
        case ION_MM:
            ret = access("/dev/sprd_iommu_mm",F_OK);
            break;
        case ION_VSP:
            ret = access("/dev/sprd_iommu_vsp",F_OK);
            break;
        case ION_DCAM:
            ret = access("/dev/sprd_iommu_dcam",F_OK);
            break;
        case ION_DISPC:
            ret = access("/dev/sprd_iommu_dispc",F_OK);
            break;
        case ION_GSP0:
            ret = access("/dev/sprd_iommu_gsp0",F_OK);
            break;
        case ION_GSP1:
            ret = access("/dev/sprd_iommu_gsp1",F_OK);
            break;
        case ION_VPP:
            ret = access("/dev/sprd_iommu_vpp",F_OK);
            break;
        default:
            ret = -1;
            break;
    }

    if(ret < 0)
        return false;
    else
        return true;
}

// ---------------------------------------------------------------------------
}; // namespace android
