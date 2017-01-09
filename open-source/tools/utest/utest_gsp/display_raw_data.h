#ifndef _UTEST_GSP_DISPLAY_RAW_DATA_H_
#define _UTEST_GSP_DISPLAY_RAW_DATA_H_


#define FBINFO_MAGIC 0xdeafbeaf

/*
 * FrameBuffer information.
 * */
#include "MemoryHeapIon.h"
 #include <stdio.h>
  //#include <stdlib.h>
   #include <stdint.h>
  #include <errno.h>
 //#include <unistd.h>
 #include <time.h>
  #include <sys/mman.h>
   #include <fcntl.h>
  #include <math.h>
  
typedef struct _FrameBufferInfo
{
    int fbfd;
    int fb_width;
    int fb_height;
    float xdpi;
    float ydpi;
    int stride;
    void *fb_virt_addr;
    int fb_size;

    char *pFrontAddr;
    char *pBackAddr;
    int format;

    class MemoryHeapIon      *MemoryHeap;//overlay physical buffer
    int MemorySize;//overlay physical buffer size
    GSP_DATA_ADDR_T         pa;
    GSP_DATA_ADDR_T_ULONG         va;

    uint32_t magic;//FBINFO_MAGIC
} FrameBufferInfo;



inline unsigned int roundUpToPageSize(unsigned int x)
{
    return (x + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1);
}

extern int loadFrameBufferHAL(FrameBufferInfo *FBInfo);

extern int unloadFrameBufferHAL(FrameBufferInfo *FBInfo);

extern int display_raw_file(GSP_LAYER_INFO_T *pLayer,GSP_MISC_INFO_T *pMisc, GSP_CONFIG_INFO_T *pgsp_cfg_info,FrameBufferInfo *fbInfo);

#endif

