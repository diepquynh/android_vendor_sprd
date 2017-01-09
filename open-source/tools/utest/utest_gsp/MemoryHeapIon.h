#ifndef MEMORY_HEAP_ION_H
#define MEMORY_HEAP_ION_H

#include "ion_sprd.h"


class MemoryHeapIon
{
public:
    MemoryHeapIon(): mIonBufferHandle(-1),mIonDevFd(-1)
    {
    }
    MemoryHeapIon(const char* device, size_t size,uint32_t flags, unsigned long memory_types);
    ~MemoryHeapIon();
    void* get_virt_addr_from_ion();
    int get_phy_addr_from_ion(unsigned long *phy_addr, size_t *size);
    int get_gsp_iova(unsigned long *mmu_addr, size_t *size);
    int free_gsp_iova(unsigned long mmu_addr, size_t size);
    status_t mapIonFd(int fd, size_t size, unsigned long memory_type, int flags);

    int mIonBufferHandle; //handle of data buffer, get from ION_IOC_ALLOC, one buffer have only one handle
    int mIonBufferFd;// fd of data buffer, one buffer maybe have a few fds, one fd in one process
private:
    const char*		mIonDevStr;// "/dev/ion"
    int			mIonDevFd;//*fd we get from open("/dev/ion")
    size_t			mIonBufferSize;// size of data buffer, page aligned
    void*			mIonBufferBase;// arm virtual addr of data buffer, get from mmap. one buffer can map serveral times, each time can get a virtual memory base
    uint32_t			mFlags;// cache or not ? property of data buffer
};

#endif

