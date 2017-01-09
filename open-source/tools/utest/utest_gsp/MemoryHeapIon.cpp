
#include <sys/types.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <sys/mman.h>

#include "infrastructure.h"
#include "ion.h"
#include "MemoryHeapIon.h"
#include "ion_sprd.h"

status_t MemoryHeapIon::mapIonFd(int fd, size_t size, unsigned long memory_type, int uflags)
{
    /* If size is 0, just fail the mmap. There is no way to get the size
     * with ion
     */
    //int map_fd;

    struct ion_allocation_data data;
    struct ion_fd_data fd_data;
    struct ion_handle_data handle_data;
    void *base = NULL;

    data.len = size;
    data.align = getpagesize();
    data.heap_id_mask = memory_type;
    //if cached buffer , force set the lowest two bits 11
    if((memory_type&(1<<31))) {
        data.flags = ((memory_type&(1<<31)) | 3);
    } else {
        data.flags = 0;
    }


    if (ioctl(fd, ION_IOC_ALLOC, &data) < 0) {
        //ALOGE("%s: ION_IOC_ALLOC error:%s!\n",__func__,strerror(errno));
        close(fd);
        return -errno;
    }

    if ((uflags & DONT_MAP_LOCALLY) == 0) {
        int flags = 0;

        fd_data.handle = data.handle;


        if (ioctl(fd, ION_IOC_SHARE, &fd_data) < 0) {
            //ALOGE("%s: ION_IOC_SHARE error:%s!\n",__func__,strerror(errno));
            handle_data.handle = data.handle;
            ioctl(fd, ION_IOC_FREE, &handle_data);
            close(fd);
            return -errno;
        }

        base = (uint8_t*)mmap(0, size,
                              PROT_READ|PROT_WRITE,
                              MAP_SHARED|flags, fd_data.fd, 0);
        if (base == MAP_FAILED) {
            //ALOGE("mmap(fd=%d, size=%u) failed (%s)\n", fd, uint32_t(size), strerror(errno));
            handle_data.handle = data.handle;
            ioctl(fd, ION_IOC_FREE, &handle_data);
            close(fd);
            return -errno;
        } else {
            //ALOGI_GSP_GSP(LOG_INFO,"mmap success, fd=%d, size=%u, addr=%p\n", fd, uint32_t(size),base);
        }
    }
    mIonDevFd = fd;

    /*
     * Call this with NULL now and set device with set_device
     * above for consistency sake with how MemoryHeapPmem works.
     */
    mIonBufferFd = fd_data.fd;
    mIonBufferHandle = data.handle;
    mIonBufferBase = base;
    mIonBufferSize = size;
    mFlags = uflags;
//ALOGE("%s%d: fd:%d hd%d\n",__func__,__LINE__,mIonBufferFd,mIonBufferHandle);
    return NO_ERROR;
}


MemoryHeapIon::MemoryHeapIon(const char* strIonDev, size_t size, uint32_t flags,
                             unsigned long memory_types): mIonBufferHandle(-1),mIonDevFd(-1)
{
    int open_flags = O_RDONLY;
    int fd = 0;

    if (flags & NO_CACHING)
        open_flags |= O_SYNC;

    fd = open(strIonDev, open_flags);
    if (fd >= 0) {
        const size_t pagesize = getpagesize();
        //const size_t pagesize = 4096;
        //ALOGI_GSP_GSP(LOG_INFO,"open ion success\n");
        size = ((size + pagesize-1) & ~(pagesize-1));
        if (mapIonFd(fd, size, memory_types, flags) == NO_ERROR) {
            mIonDevStr = strIonDev;
            //ALOGI_GSP_GSP(LOG_INFO,"alloc ion buffer success\n");
        } else {
            //ALOGE("alloc ion buffer failed\n");
        }
    } else {
        //ALOGE("open ion fail\n");
    }
}


MemoryHeapIon::~MemoryHeapIon()
{
    struct ion_handle_data data;

    /*
     * Due to the way MemoryHeapBase is set up, munmap will never
     * be called so we need to call it ourselves here.
     */
    if(mIonBufferBase && mIonBufferSize > 0) {
        if (munmap(mIonBufferBase, mIonBufferSize) < 0) {
            //ALOGE("%s:unmap buffer virt addr:%p,size:%d failed, errno:%s\n",__func__,mBase,mSize,strerror(errno));
        } else {
            //ALOGI_GSP_GSP(LOG_INFO,"%s:unmap buffer virt addr:%p,size:%d success.\n",__func__,mBase,mSize);
        }
        mIonBufferBase = 0;
        mIonBufferSize=0;
    }
    if(mIonBufferFd>=0) {
        close(mIonBufferFd);
        mIonBufferFd = -1;
    }
    if (mIonDevFd > 0) {
        data.handle = mIonBufferHandle;
        ioctl(mIonDevFd, ION_IOC_FREE, &data);
        mIonBufferHandle = -1;
        close(mIonDevFd);
        mIonDevFd = -1;
    }
}

int MemoryHeapIon::get_phy_addr_from_ion(unsigned long *phy_addr, size_t *size)
{
    if(mIonDevFd<0 || mIonBufferHandle < 0) {
        //ALOGE("%s:open dev ion error!\n",__func__);
        return -1;
    } else {
        int ret;
        struct ion_phys_data phys_data;
        struct ion_custom_data  custom_data;
        phys_data.fd_buffer = mIonBufferFd;
        custom_data.cmd = ION_SPRD_CUSTOM_PHYS;
        custom_data.arg = (unsigned long)&phys_data;
        ret = ioctl(mIonDevFd,ION_IOC_CUSTOM,&custom_data);
        *phy_addr = phys_data.phys;
        *size = phys_data.size;
        if(ret) {
            //ALOGE("%s: get phy addr error:%d!\n",__func__,ret);
            return -2;
        }
        //ALOGI_GSP_GSP(LOG_INFO,"%s: get phy addr success:0x%08x\n",__func__,(uint32_t)*phy_addr);
    }
    return 0;
}

void* MemoryHeapIon::get_virt_addr_from_ion()
{
    return mIonBufferBase;
}

int MemoryHeapIon::get_gsp_iova(unsigned long *mmu_addr, size_t *size)
{
    if(mIonDevFd<0 || mIonBufferFd<0||mIonBufferHandle<0) {
        return -1;
    } else {
        int ret;
        struct ion_mmu_data mmu_data;
        struct ion_custom_data  custom_data;
        mmu_data.fd_buffer = mIonBufferFd;
        custom_data.cmd = ION_SPRD_CUSTOM_GSP_MAP;
        custom_data.arg = (unsigned long)&mmu_data;

        ret = ioctl(mIonDevFd,ION_IOC_CUSTOM,&custom_data);
        *mmu_addr = mmu_data.iova_addr;
        *size = mmu_data.iova_size;
        if(ret) {
            ALOGE("%s: get gsp iova error!\n",__func__);
            return -2;
        }
        ALOGI(LOG_INFO,"%s: get iova success! 0x%ld!\n",__func__,*mmu_addr);
    }
    return 0;
}

int MemoryHeapIon::free_gsp_iova(unsigned long mmu_addr, size_t size)
{
    if(mIonDevFd<0) {
        //ALOGE("%s:open dev ion error!\n",__func__);
        return -1;
    } else {
        int ret;
        struct ion_mmu_data mmu_data;
        struct ion_custom_data  custom_data;
        mmu_data.fd_buffer = mIonBufferFd;
        mmu_data.iova_addr = mmu_addr;
        mmu_data.iova_size = size;
        custom_data.cmd = ION_SPRD_CUSTOM_GSP_UNMAP;
        custom_data.arg = (unsigned long)&mmu_data;
        ret = ioctl(mIonDevFd,ION_IOC_CUSTOM,&custom_data);
        if(ret) {
            //ALOGE("%s: put gsp iova error!\n",__func__);
            return -2;
        }
    }
    //ALOGI_GSP_GSP(LOG_INFO,"%s[%d]: put gsp iova:0x%ld success!\n",__func__,__LINE__,mmu_addr);
    return 0;
}

