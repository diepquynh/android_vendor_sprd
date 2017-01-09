

#include "infrastructure.h"
#include "display_raw_data.h"// display the gsp output

#include <linux/fb.h>// display the gsp output
#include "sprd_fb.h"// display the gsp output



static int alloc_ov_buffer(FrameBufferInfo *fbInfo)
{
    int ret = 0;
    size_t temp_size = 0;

    fbInfo->MemorySize = fbInfo->fb_width*fbInfo->fb_height*1.5;//ARGB888 1frame
    //alloc none cached and buffered memory
    ALOGI(LOG_INFO,"%s[%d] alloc phy buffer \n",__func__,__LINE__);
    fbInfo->MemoryHeap = new MemoryHeapIon("/dev/ion", fbInfo->MemorySize, NO_CACHING, ION_HEAP_ID_MASK_OVERLAY);//GRALLOC_USAGE_OVERLAY_BUFFER ION_HEAP_CARVEOUT_MASK
    if(fbInfo->MemoryHeap == NULL) {
        ALOGE("%s[%d] alloc phy buffer err!\n",__func__,__LINE__);
        return -1;
    }
    ret = fbInfo->MemoryHeap->get_phy_addr_from_ion((unsigned long *)&(fbInfo->pa.addr_y), &temp_size);
    if(ret || fbInfo->pa.addr_y<0x80000000) {
        ALOGE("%s[%d] alloc phy buffer err!\n",__func__,__LINE__);
        return -1;
    }

    fbInfo->pa.addr_v = fbInfo->pa.addr_uv = fbInfo->pa.addr_y + fbInfo->fb_width*fbInfo->fb_height;
    fbInfo->va.addr_y = (uint64_t)fbInfo->MemoryHeap->get_virt_addr_from_ion();
    if(fbInfo->va.addr_y == 0) {
        ALOGE("%s[%d] alloc phy buffer err!\n",__func__,__LINE__);
        return -1;
    }
    fbInfo->va.addr_v = fbInfo->va.addr_uv = fbInfo->va.addr_y + fbInfo->fb_width*fbInfo->fb_height;
    memset_gsp((void*)fbInfo->va.addr_y,0,fbInfo->MemorySize);

    return 0;
}

int free_ov_buffer(FrameBufferInfo *fbInfo)
{
    //alloc none cached and buffered memory
    if(fbInfo->MemoryHeap) {
        delete fbInfo->MemoryHeap;
        fbInfo->MemoryHeap = NULL;
        fbInfo->pa.addr_y = 0;
        fbInfo->va.addr_y = 0;
        fbInfo->fb_size = 0;
    }
    return 0;
}

int loadFrameBufferHAL(FrameBufferInfo *FBInfo)
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
    int ret=0;

    struct fb_fix_screeninfo finfo;
    struct fb_var_screeninfo vinfo;
    size_t fbSize = 0;
    uint32_t bytespp = 0;


    while ((fd == -1) && deviceTemplate[i]) {
        snprintf(name, 64, deviceTemplate[i], 0);
        fd = open(name, O_RDWR, 0);
        i++;
    }

    if (fd < 0) {
        ALOGE("fail to open fb");
        ret = -1;
        return ret;
    }

    if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo) == -1) {
        ALOGE("fail to get FBIOGET_FSCREENINFO");
        close(fd);
        ret = -1;
        return ret;
    }

    if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
        ALOGE("fail to get FBIOGET_VSCREENINFO");
        close(fd);
        ret = -1;
        return ret;
    }

    fbSize = roundUpToPageSize(finfo.line_length * vinfo.yres_virtual);
    vaddr = mmap(0, fbSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (vaddr == MAP_FAILED) {
        ALOGE("Error mapping the framebuffer (%s)", strerror(errno));
        close(fd);
        ret = -1;
        return ret;
    }


    /*
     * Store the FrameBuffer info
     * */
    //FrameBufferInfo *FBInfo = (FrameBufferInfo *)malloc(sizeof(FrameBufferInfo));
    memset_gsp((void*)FBInfo,0,sizeof(*FBInfo));
    FBInfo->fbfd = fd;
    FBInfo->fb_width = vinfo.xres;
    FBInfo->fb_height = vinfo.yres;
    FBInfo->fb_virt_addr = vaddr;
    FBInfo->fb_size = fbSize;
    FBInfo->stride = finfo.line_length / (vinfo.xres / 8);
    FBInfo->xdpi = (vinfo.xres * 25.4f) / vinfo.width;
    FBInfo->ydpi = (vinfo.yres * 25.4f) / vinfo.height;

    switch(vinfo.bits_per_pixel) {
        case 16:
            bytespp = 2;
            //FBInfo->format = HAL_PIXEL_FORMAT_RGB_565;
            break;
        case 24:
            bytespp = 3;
            //FBInfo->format = HAL_PIXEL_FORMAT_RGB_888;
            break;
        case 32:
            bytespp = 4;
            //FBInfo->format = HAL_PIXEL_FORMAT_RGBA_8888;
            break;
        default:
            ALOGE("fail to getFrameBufferInfo not support bits per pixel:%d" , vinfo.bits_per_pixel);
            //free(FBInfo);
            return -1;
    }

    if(vinfo.yoffset == vinfo.yres) {
        //flushing the second buffer.
        FBInfo->pFrontAddr = (char*)((unsigned long)FBInfo->fb_virt_addr + vinfo.xres * vinfo.yres * bytespp);
        FBInfo->pBackAddr  = (char *)(FBInfo->fb_virt_addr);
    } else if(vinfo.yoffset == 0) {
        //flushing the first buffer.
        FBInfo->pFrontAddr = (char *)(FBInfo->fb_virt_addr);
        FBInfo->pBackAddr = (char*)((unsigned long)FBInfo->fb_virt_addr + vinfo.xres * vinfo.yres * bytespp);
    } else {
        ALOGE("fail to getFrameBufferInfo");
    }

    ret = alloc_ov_buffer(FBInfo);
    if(ret) {
        ALOGE("%s[%d] alloc overlay phy buffer err!\n",__func__,__LINE__);
        return ret;
    }
    FBInfo->magic = FBINFO_MAGIC;
    ret = 0;
    return ret;
}


int unloadFrameBufferHAL(FrameBufferInfo *FBInfo)
{
    int ret=0;
    if(FBInfo->magic == FBINFO_MAGIC) {
        FBInfo->magic = 0;
//        ALOGI(LOG_INFO,"%s[%d]:unmap buffer virt addr:%p\n",__func__,__LINE__,FBInfo->fb_virt_addr);
        munmap(FBInfo->fb_virt_addr,FBInfo->fb_size);
        free_ov_buffer(FBInfo);
    }
    return ret;
}

static int convert_to_fb_size(GSP_LAYER_INFO_T *pLayer,GSP_MISC_INFO_T *pMisc,GSP_CONFIG_INFO_T *pgsp_cfg_info,FrameBufferInfo *fbInfo)
{
    int  ret = 0;
    GSP_CONFIG_INFO_T gsp_cfg_info;

    memset(&gsp_cfg_info, 0, sizeof(GSP_CONFIG_INFO_T));
    gsp_cfg_info.layer0_info.pitch = pgsp_cfg_info->layer_des_info.pitch;
    gsp_cfg_info.layer0_info.img_format = (GSP_LAYER_SRC_DATA_FMT_E)pgsp_cfg_info->layer_des_info.img_format;
    gsp_cfg_info.layer0_info.clip_rect.st_x =
        gsp_cfg_info.layer0_info.clip_rect.st_y = 0;
    gsp_cfg_info.layer0_info.clip_rect.rect_w = pgsp_cfg_info->layer_des_info.pitch;
    gsp_cfg_info.layer0_info.clip_rect.rect_h = pLayer->pitch.h;
    gsp_cfg_info.layer0_info.des_rect.st_x =
        gsp_cfg_info.layer0_info.des_rect.st_y = 0;
    if((pLayer->pitch.w>pLayer->pitch.h
        &&fbInfo->fb_width>fbInfo->fb_height)
       ||(pLayer->pitch.w<pLayer->pitch.h
          &&fbInfo->fb_width<fbInfo->fb_height)) {
        gsp_cfg_info.layer0_info.rot_angle = GSP_ROT_ANGLE_0;
    } else {
        gsp_cfg_info.layer0_info.rot_angle = GSP_ROT_ANGLE_90;
    }
    gsp_cfg_info.layer0_info.des_rect.rect_w = fbInfo->fb_width;
    gsp_cfg_info.layer0_info.des_rect.rect_h = fbInfo->fb_height;
    gsp_cfg_info.layer0_info.src_addr = pgsp_cfg_info->layer_des_info.src_addr;
    gsp_cfg_info.layer0_info.mem_info = pgsp_cfg_info->layer_des_info.mem_info;
    gsp_cfg_info.layer0_info.layer_en = 1;
    gsp_cfg_info.layer_des_info.img_format = GSP_DST_FMT_YUV420_2P;
    gsp_cfg_info.layer_des_info.pitch = fbInfo->fb_width;
    gsp_cfg_info.layer_des_info.src_addr.addr_y = fbInfo->pa.addr_y;
    gsp_cfg_info.layer_des_info.src_addr.addr_v =
        gsp_cfg_info.layer_des_info.src_addr.addr_uv = fbInfo->pa.addr_y+fbInfo->fb_width*fbInfo->fb_height;
    if(pMisc->gsp_process) {
        ret = (*pMisc->gsp_process)(&gsp_cfg_info);
        if(0 == ret) {
            ALOGI(LOG_INFO,"%s[%d] GSP_Proccess ok\n",__func__,__LINE__);
        } else {
            ALOGE("%s[%d] GSP_Proccess err!\n",__func__,__LINE__);
        }
    } else {
        ALOGE("%s[%d] gsp_process is null!\n",__func__,__LINE__);
    }

    return ret;
}

int display_raw_file(GSP_LAYER_INFO_T *pLayer,GSP_MISC_INFO_T *pMisc,GSP_CONFIG_INFO_T *pgsp_cfg_info,FrameBufferInfo *fbInfo)
{
    //memcpy(fbInfo->fb_virt_addr,pLayer->va.addr_y,pLayer->size_all);

    struct overlay_setting BaseContextBody;
    struct overlay_setting *BaseContext = &BaseContextBody;
    struct overlay_display_setting displayContext;
    if(fbInfo->magic != FBINFO_MAGIC) {
        ALOGE("%s[%d] fbInfo->magic is not match!\n",__func__,__LINE__);
        return -1;
    }

    convert_to_fb_size(pLayer,pMisc, pgsp_cfg_info,fbInfo);
    // switch(pgsp_cfg_info->layer_des_info.img_format)
    switch(4) {
        case GSP_DST_FMT_ARGB888:
        case GSP_DST_FMT_RGB888: {
            BaseContext->data_type = SPRD_DATA_FORMAT_RGB888;
            BaseContext->y_endian = SPRD_DATA_ENDIAN_B0B1B2B3;
            BaseContext->uv_endian = SPRD_DATA_ENDIAN_B0B1B2B3;
            BaseContext->rb_switch = 0;
            BaseContext->layer_index = SPRD_LAYERS_OSD;
            displayContext.layer_index = SPRD_LAYERS_OSD;
        }
        break;

        case GSP_DST_FMT_ARGB565:
        case GSP_DST_FMT_RGB565:
            return -1;
            break;

        case GSP_DST_FMT_YUV420_2P: {
            BaseContext->data_type = SPRD_DATA_FORMAT_YUV420;
            BaseContext->y_endian = SPRD_DATA_ENDIAN_B3B2B1B0;
            BaseContext->rb_switch = 0;
            BaseContext->uv_endian = SPRD_DATA_ENDIAN_B3B2B1B0;
            BaseContext->layer_index = SPRD_LAYERS_IMG;
            displayContext.layer_index = SPRD_LAYERS_IMG;

        }
        break;

        case GSP_DST_FMT_YUV422_2P: {
            BaseContext->data_type = SPRD_DATA_FORMAT_YUV422;
            BaseContext->y_endian = SPRD_DATA_ENDIAN_B3B2B1B0;
            //BaseContext->uv_endian = SPRD_DATA_ENDIAN_B3B2B1B0;
            BaseContext->uv_endian = SPRD_DATA_ENDIAN_B0B1B2B3;
            BaseContext->rb_switch = 0;
            BaseContext->layer_index = SPRD_LAYERS_IMG;
            displayContext.layer_index = SPRD_LAYERS_IMG;
        }
        break;

        case GSP_DST_FMT_YUV420_3P:
        default:
            return -1;
    }


    BaseContext->rect.x = 0;
    BaseContext->rect.y = 0;
    BaseContext->rect.w = fbInfo->fb_width;
    BaseContext->rect.h = fbInfo->fb_height;


    //BaseContext->buffer = (unsigned char *)(pLayer->pa.addr_y);
    BaseContext->buffer = (unsigned char *)(long)fbInfo->pa.addr_y;

    ALOGI(LOG_INFO,"SprdOverlayPlane::flush SET_OVERLAY parameter datatype = %d, x = %d, y = %d, w = %d, h = %d, buffer = %p\n",
          BaseContext->data_type,
          BaseContext->rect.x,
          BaseContext->rect.y,
          BaseContext->rect.w,
          BaseContext->rect.h,
          BaseContext->buffer);

    if (ioctl(fbInfo->fbfd, SPRD_FB_SET_OVERLAY, BaseContext) == -1) {
        ALOGE("fail video SPRD_FB_SET_OVERLAY\n");
        ioctl(fbInfo->fbfd, SPRD_FB_SET_OVERLAY, BaseContext);//Fix ME later
    }

    displayContext.display_mode = SPRD_DISPLAY_OVERLAY_ASYNC;
    //displayContext.display_mode = SPRD_DISPLAY_OVERLAY_SYNC;

    displayContext.rect.x = 0;
    displayContext.rect.y = 0;
    displayContext.rect.w = fbInfo->fb_width;
    displayContext.rect.h = fbInfo->fb_height;

    ALOGI(LOG_INFO,"SPRD_FB_DISPLAY_OVERLAY %d\n", displayContext.layer_index);

    ioctl(fbInfo->fbfd, SPRD_FB_DISPLAY_OVERLAY, &displayContext);

    return 0;
}


