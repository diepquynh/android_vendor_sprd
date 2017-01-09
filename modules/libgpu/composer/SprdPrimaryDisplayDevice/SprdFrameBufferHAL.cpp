
/*
 * Copyright (C) 2010 The Android Open Source Project
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


/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          Module              DESCRIPTION                             *
 ** 22/09/2013    Hardware Composer   Responsible for processing some         *
 **                                   Hardware layers. These layers comply    *
 **                                   with display controller specification,  *
 **                                   can be displayed directly, bypass       *
 **                                   SurfaceFligner composition. It will     *
 **                                   improve system performance.             *
 ******************************************************************************
 ** File: SprdFrameBufferHal.cpp      DESCRIPTION                             *
 **                                   Open FrameBuffer device.                *
 ******************************************************************************
 ******************************************************************************
 ** Author:         zhongjun.chen@spreadtrum.com                              *
 *****************************************************************************/

#include "SprdFrameBufferHAL.h"

using namespace android;


inline unsigned int roundUpToPageSize(unsigned int x)
{
    return (x + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1);
}

int loadFrameBufferHAL(FrameBufferInfo **fbInfo)
{
    char const * const deviceTemplate[] = {
        "/dev/graphics/fb%u",
        "/dev/fb%u",
        NULL
    };

    int fd = -1;
    int i = 0;
    char name[64];
    void *vaddr;

    struct fb_fix_screeninfo finfo;
    struct fb_var_screeninfo vinfo;
    size_t fbSize = 0;
    uint32_t bytespp = 0;

    hw_module_t const* module;
    framebuffer_device_t *fbDev;

    int ret = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
    if (ret != 0)
    {
        ALOGE("%s module not found", GRALLOC_HARDWARE_MODULE_ID);
        return ret;
    }

    ret = framebuffer_open(module, &fbDev);
    if (ret != 0)
    {
        ALOGE("framebuffer_open failed");
        return ret;
    }

    while ((fd == -1) && deviceTemplate[i])
    {
        snprintf(name, 64, deviceTemplate[i], 0);
        fd = open(name, O_RDWR, 0);
        i++;
    }

    if (fd < 0)
    {
        ALOGE("fail to open fb");
        ret = -1;
        return ret;
    }

    if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo) == -1)
    {
        ALOGE("fail to get FBIOGET_FSCREENINFO");
        close(fd);
        ret = -1;
        return ret;
    }

    if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) == -1)
    {
        ALOGE("fail to get FBIOGET_VSCREENINFO");
        close(fd);
        ret = -1;
        return ret;
    }

    fbSize = roundUpToPageSize(finfo.line_length * vinfo.yres_virtual);
    vaddr = mmap(0, fbSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (vaddr == MAP_FAILED)
    {
        ALOGE("Error mapping the framebuffer (%s)", strerror(errno));
        close(fd);
        ret = -1;
        return ret;
    }


    /*
     * Store the FrameBuffer info
     * */
    FrameBufferInfo *FBInfo = (FrameBufferInfo *)malloc(sizeof(FrameBufferInfo));
    if (FBInfo == NULL)
    {
        ALOGE("Cannot malloc the FrameBufferInfo, no MEM");
        munmap(vaddr, fbSize);
        close(fd);
        ret = -1;
        return ret;
    }

    FBInfo->fbfd = fd;
    FBInfo->fb_width = vinfo.xres;
    FBInfo->fb_height = vinfo.yres;
    FBInfo->fb_virt_addr = vaddr;
    FBInfo->stride = finfo.line_length / (vinfo.xres / 8);
    FBInfo->xdpi = (vinfo.xres * 25.4f) / vinfo.width;
    FBInfo->ydpi = (vinfo.yres * 25.4f) / vinfo.height;

    switch(vinfo.bits_per_pixel) {
    case 16:
        bytespp = 2;
        FBInfo->format = HAL_PIXEL_FORMAT_RGB_565;
        break;
    case 24:
        bytespp = 3;
        FBInfo->format = HAL_PIXEL_FORMAT_RGB_888;
        break;
    case 32:
        bytespp = 4;
        FBInfo->format = HAL_PIXEL_FORMAT_RGBA_8888;
        break;
    default:
        ALOGE("fail to getFrameBufferInfo not support bits per pixel:%d" , vinfo.bits_per_pixel);
        free(FBInfo);
        return ret;
    }

    if(vinfo.yoffset == vinfo.yres)
    { //flushing the second buffer.
        FBInfo->pFrontAddr = (char*)((unsigned long)FBInfo->fb_virt_addr + vinfo.xres * vinfo.yres * bytespp);
        FBInfo->pBackAddr  = (char *)(FBInfo->fb_virt_addr);
    }
    else if(vinfo.yoffset == 0)
    { //flushing the first buffer.
        FBInfo->pFrontAddr = (char *)(FBInfo->fb_virt_addr);
        FBInfo->pBackAddr = (char*)((unsigned long)FBInfo->fb_virt_addr + vinfo.xres * vinfo.yres * bytespp);
    }
    else
    {
        ALOGE("fail to getFrameBufferInfo");
    }

    FBInfo->fbDev = fbDev;
    *fbInfo = FBInfo;

    ret = 0;
    return ret;
}

void closeFrameBufferHAL(FrameBufferInfo *fbInfo)
{
    if (fbInfo)
    {
       if (fbInfo->fbDev)
       {
           framebuffer_close(fbInfo->fbDev);
       }
       close(fbInfo->fbfd);
       free(fbInfo);
       fbInfo = NULL;
    }
}
