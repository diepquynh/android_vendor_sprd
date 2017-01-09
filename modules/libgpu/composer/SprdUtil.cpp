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
 ** File: SprdUtil.cpp                DESCRIPTION                             *
 **                                   Transform or composer Hardware layers   *
 **                                   when display controller cannot deal     *
 **                                   with these function                     *
 ******************************************************************************
 ******************************************************************************
 ** Author:         zhongjun.chen@spreadtrum.com                              *
 *****************************************************************************/


#include <ui/GraphicBufferAllocator.h>
#include "MemoryHeapIon.h"
#include "SprdUtil.h"
#include "dump.h"
#include <ui/GraphicBufferMapper.h>

using namespace android;


#ifdef TRANSFORM_USE_DCAM
OSDTransform::OSDTransform(FrameBufferInfo *fbInfo)
    :  mL(NULL),
       mFBInfo(fbInfo),
       mBuffer(NULL),
       mInitFLag(false),
       mDebugFlag(0)
{
#ifdef _PROC_OSD_WITH_THREAD
    sem_init(&startSem, 0, 0);
    sem_init(&doneSem, 0, 0);
#endif
}

OSDTransform::~OSDTransform()
{
#ifdef _PROC_OSD_WITH_THREAD
    sem_destroy(&startSem);
    sem_destroy(&doneSem);
#endif
}

void OSDTransform::onStart(SprdHWLayer *l, private_handle_t* buffer)
{
    if (l == NULL || buffer == NULL) {
        ALOGE("onOSDTransform, input parameters are NULL");
        return;
    }

    mL = l;
    mBuffer = buffer;

#ifndef _PROC_OSD_WITH_THREAD
    transformOSD();
#else
    sem_post(&startSem);
#endif
}

void OSDTransform::onWait()
{
#ifdef _PROC_OSD_WITH_THREAD
    sem_wait(&doneSem);
#endif
}

#ifdef _PROC_OSD_WITH_THREAD
void OSDTransform::onFirstRef()
{
    run("OSDTransform", PRIORITY_URGENT_DISPLAY);
}

status_t OSDTransform::readyToRun()
{
    return NO_ERROR;
}

bool OSDTransform::threadLoop()
{
    sem_wait(&startSem);

    transformOSD();

    sem_post(&doneSem);

    return true;
}
#endif

int OSDTransform::transformOSD()
{
    if (mL == NULL || mBuffer == NULL) {
        ALOGE("layer == NULL || mBuffer == NULL");
        return -1;
    }
    hwc_layer_1_t *layer = mL;
    struct sprdYUV *srcImg = mL->getSprdSRCYUV();
    struct sprdRect *srcRect = mL->getSprdSRCRect();
    struct sprdRect *FBRect = mL->getSprdFBRect();
    if (layer == NULL || srcImg == NULL ||
        srcRect == NULL || FBRect == NULL) {
        ALOGE("Failed to get OSD SprdHWLayer parameters");
        return -1;
    }

    const native_handle_t *pNativeHandle = layer->handle;
    struct private_handle_t *private_h = (struct private_handle_t *)pNativeHandle;

    queryDebugFlag(&mDebugFlag);

    if (private_h->flags & private_handle_t::PRIV_FLAGS_USES_PHY) {
        if (0 == layer->transform) {
            ALOGI_IF(mDebugFlag, "OSD display with rot copy");

            int ret = camera_roataion_copy_data(mFBInfo->fb_width, mFBInfo->fb_height, private_h->phyaddr, buffer2->phyaddr);
            if (-1 == ret) {
                ALOGE("do OSD rotation copy fail");
            }
        } else {
            ALOGI_IF(mDebugFlag, "OSD display with rot");
            int degree = -1;

            switch (layer->transform) {
                case HAL_TRANSFORM_ROT_90:
                    degree = 90;
                    break;
                case HAL_TRANSFORM_ROT_270:
                    degree = 270;
                default:
                    degree = 180;
                    break;
            }

            int ret = camera_rotation(HW_ROTATION_DATA_RGB888, degree, mFBInfo->fb_width, mFBInfo->fb_height,
                                      private_h->phyaddr, buffer2->phyaddr);
            if (-1 == ret) {
                ALOGE("do OSD rotation fail");
            }
        }
    } else {
        ALOGI_IF(mDebugFlag, "OSD display with dma copy");

        camera_rotation_copy_data_from_virtual(mFBInfo->fb_width, mFBInfo->fb_height, private_h->base, buffer2->phyaddr);
    }

    mL = NULL;
    mBuffer = NULL;

    return 0;
}

#endif


SprdUtil::~SprdUtil()
{
#ifdef TRANSFORM_USE_GPU
    destroy_transform_thread();
#endif
#ifdef TRANSFORM_USE_DCAM
#ifdef SCAL_ROT_TMP_BUF
    GraphicBufferAllocator::get().free((buffer_handle_t)tmpBuffer);
#endif
#endif
#ifdef PROCESS_VIDEO_USE_GSP
    if(copyTempBuffer) {
        GraphicBufferAllocator::get().free((buffer_handle_t)copyTempBuffer);
        copyTempBuffer = NULL;
    }
    if (mGspDev) {
        mGspDev->common.close(&(mGspDev->common));
        mGspDev = NULL;
    }
#endif
}

#ifdef TRANSFORM_USE_DCAM
bool SprdUtil::transformLayer(SprdHWLayer *l1, SprdHWLayer *l2,
                              private_handle_t* buffer1, private_handle_t* buffer2)
{
#ifdef TRANSFORM_USE_DCAM
    if (l2 && buffer2) {
        mOSDTransform->onStart(l2, buffer2);
    }

    if (l1 && buffer1) {
        /*
         * Temporary video buffer info for dcam transform
         **/
        int format = HAL_PIXEL_FORMAT_YCbCr_420_SP;

#ifdef SCAL_ROT_TMP_BUF
        if (tmpDCAMBuffer == NULL) {
            int stride;
            size_t size;

            GraphicBufferAllocator::get().alloc(mFBInfo->fb_width, mFBInfo->fb_height, format, GRALLOC_USAGE_OVERLAY_BUFFER, (buffer_handle_t*)&tmpDCAMBuffer, &stride);

            MemoryHeapIon::Get_phy_addr_from_ion(tmpDCAMBuffer->share_fd, &(tmpDCAMBuffer->phyaddr), &size);
            if (tmpDCAMBuffer == NULL) {
                ALOGE("Cannot alloc the tmpBuffer ION buffer");
                return false;
            }

            Rect bounds(mFBInfo->fb_width, mFBInfo->fb_height);
            GraphicBufferMapper::get().lock((buffer_handle_t)tmpDCAMBuffer, GRALLOC_USAGE_SW_READ_OFTEN, bounds, &tmpDCAMBuffer->base);
        }
#endif

        hwc_layer_1_t *layer = l1->getAndroidLayer();
        struct sprdRect *srcRect = l1->getSprdSRCRect();
        struct sprdRect *FBRect = l1->getSprdFBRect();
        if (layer == NULL || srcImg == NULL ||
            srcRect == NULL || FBRect == NULL) {
            ALOGE("Failed to get Video SprdHWLayer parameters");
            return -1;
        }

        const native_handle_t *pNativeHandle = layer->handle;
        struct private_handle_t *private_h = (struct private_handle_t *)pNativeHandle;

        int dstFormat = -1;
#ifdef VIDEO_LAYER_USE_RGB
        dstFormat = HAL_PIXEL_FORMAT_RGBA_8888;
#else
        dstFormat = HAL_PIXEL_FORMAT_YCbCr_420_SP;
#endif
        int ret = transform_layer(private_h->phyaddr, private_h->base, private_h->format,
                                  layer->transform, srcImg->w, srcImg->h,
                                  buffer1->phyaddr, buffer1->base, dstFormat,
                                  FBRect->w, FBRect->h, srcRect,
                                  tmpDCAMBuffer->phyaddr, tmpDCAMBuffer->base);
        if (ret != 0) {
            ALOGE("DCAM transform video layer failed");
            return false;
        }

    }

    if (l2 && buffer2) {
        mOSDTransform->onWait();
    }

#endif

#ifdef TRANSFORM_USE_GPU
    gpu_transform_info_t transformInfo;

    getTransformInfo(l1, l2, buffer1, buffer2, &transformInfo);

    gpu_transform_layers(&transformInfo);
#endif

    return true;
}

#endif


#ifdef PROCESS_VIDEO_USE_GSP

int64_t SprdUtil::UtilGetSystemTime()
{
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(CLOCK_MONOTONIC, &t);
    //printf("time: %u:%u.\n",t.tv_sec,t.tv_nsec);
    return t.tv_sec*1000000000LL + t.tv_nsec;
}

/*
func:test_gen_white_boundary
desc:draw a white framework boundary in source video layer to check holonomy of video frame
*/
void SprdUtil::test_gen_white_boundary(char* base,uint32_t w,uint32_t h,uint16_t format)
{
    uint32_t i=0,r=0,c0=0,c1=0,c2=0,c3=0,c4=0;
    char* base_walk = base;
    uint32_t first_r=0,second_r=16;
    uint32_t first_c=0,second_c=16;
    if(format == GSP_SRC_FMT_YUV420_2P) {
        memset(base_walk+w*first_r,            0xff,w); // 0
        memset(base_walk+w*second_r,        0xff,w); // 10
        memset(base_walk+(w*h>>1),    0xff,w);
        memset(base_walk+w*(h-1-second_r),    0xff,w);
        memset(base_walk+w*(h-1-first_r),    0xff,w);

        base_walk = base;
        r=0;
        c0=first_c;
        c1=second_c;
        c2=(w>>1);
        c3=w-1-second_c;
        c4=w-1-first_c;
        while(r < h) {
            *(base_walk+c0) = 0xff;
            *(base_walk+c1) = 0xff;
            *(base_walk+c2) = 0xff;
            *(base_walk+c3) = 0xff;
            *(base_walk+c4) = 0xff;
            base_walk += w;
            r++;
        }
    }
}


/*
func:test_gen_color_block
desc:generate a pure color block in a pitch_w*pitch_h image, the block position and width height descripted in "rect"
params:
    base: image buffer address
    format:image format
    rect: region of block
    color: block color setting
    gray:the block color have the gray feature
*/
void SprdUtil::test_gen_color_block(char* base,uint16_t pitch_w,uint16_t pitch_h,uint16_t format, struct sprdRect *rect,GEN_COLOR color,uint16_t gray)
{
    char red = 0;
    char green = 0;
    char blue = 0;
    uint32_t r = 0,c = 0;

    //params check
    if(base == NULL|| (unsigned long)base & 0x3 ||pitch_w < 90||rect == NULL) {
        return;
    }

    switch(color) {
        default:
        case GEN_COLOR_BLACK:
            red = 0;
            green = 0;
            blue = 0;
            break;
        case GEN_COLOR_BLUE:
            red = 0;
            green = 0;
            blue = 0xff;
            break;
        case GEN_COLOR_RED:
            red = 0xff;
            green = 0;
            blue = 0;
            break;
        case GEN_COLOR_MAGENTA:
            red = 0xff;
            green = 0;
            blue = 0xff;
            break;
        case GEN_COLOR_GREEN:
            red = 0;
            green = 0xff;
            blue = 0;
            break;
        case GEN_COLOR_CYAN:
            red = 0;
            green = 0xff;
            blue = 0xff;
            break;
        case GEN_COLOR_YELLOW:
            red = 0xff;
            green = 0xff;
            blue = 0;
            break;
        case GEN_COLOR_WHITE:
            red = 0xff;
            green = 0xff;
            blue = 0xff;
            break;
    }

    if(format == GSP_SRC_FMT_ARGB888 || format == GSP_SRC_FMT_RGB888) {
        uint32_t *addr_walk = (uint32_t*)base;
        uint32_t *row_head = (uint32_t*)base;
        uint32_t pixel_value = 0;

        row_head += pitch_w*rect->y+rect->x;
        //0xABGR is GSP default RGB endian
        if(gray==0) {
            pixel_value=(0xff<<24)|(blue<<16)|(green<<8)|red;

            addr_walk = row_head;
            c = 0;
            while(c<rect->w) {
                *addr_walk = pixel_value;
                addr_walk++;
                c++;
            }

            r = 1;
            while(r<rect->h) {
                memcpy((void*)(row_head+pitch_w),row_head,rect->w<<2);
                row_head += pitch_w;
                r++;
            }
        } else {
            char rt = 0;
            char gt = 0;
            char bt = 0;

            float r_step=256.0/rect->h;
            float c_step=256.0/rect->w;

            r=0;
            while(r<rect->h) {
                addr_walk = row_head;
                c=0;
                while(c<rect->w) {
                    rt=gt=bt=r*r_step+c*c_step;
                    rt&=red;
                    gt&=green;
                    bt&=blue;

                    *addr_walk=(0xff<<24)|(bt<<16)|(gt<<8)|rt;
                    addr_walk++;
                    c++;
                }
                row_head += pitch_w;
                r++;
            }
        }
    } else if(format == GSP_SRC_FMT_ARGB565 || format == GSP_SRC_FMT_RGB565) {
    } else if(format == GSP_SRC_FMT_YUV420_2P) {
        char *row_head_y = (char*)base;
        char *row_head_uv = (char*)base;
        char *addr_walk_y =  (char*)base;
        char *addr_walk_uv = (char*)base;
        char y = 0;
        char cb = 0;
        char cr = 0;
        //0xY3Y2Y1Y0 0xV1U1V0U0 is GSP default YUV endian

        if(pitch_w & 0x1 || pitch_h & 0x1
           ||rect->x & 0x1 ||rect->y & 0x1 ||rect->w & 0x1 ||rect->h & 0x1 ) {
            return;
        }

        y=0.299*red+0.587*green+0.114*blue;
        cb=-0.16874*red-0.33126*green+0.5*blue+128;
        cr=0.5*red-0.41869*green+128;

        row_head_y += pitch_w*rect->y+rect->x;

        if(gray==0) {
            memset(row_head_y,y,rect->w);
            r = 1;
            while(r<rect->h) {
                memcpy((void*)(row_head_y+pitch_w),row_head_y,rect->w);
                row_head_y += pitch_w;
                r++;
            }
        } else {
            float r_step=(235.0-16.0)/rect->h;
            float c_step=(235.0-16.0)/rect->w;

            r=0;
            while(r<rect->h) {
                addr_walk_y = row_head_y;
                c=0;
                while(c<rect->w) {
                    *addr_walk_y = 16+r*r_step+c*c_step;
                    addr_walk_y++;
                    c++;
                }
                row_head_y += pitch_w;
                r++;
            }
        }

        row_head_uv += pitch_w*pitch_h;
        row_head_uv += (pitch_w*rect->y)/2+rect->x;

        addr_walk_uv = row_head_uv;
        c=0;
        while(c<rect->w) {
            *addr_walk_uv = cb;
            addr_walk_uv++;
            *addr_walk_uv = cr;
            addr_walk_uv++;
            c+=2;
        }

        r = 2;
        while(r<rect->h) {
            memcpy((void*)(row_head_uv+pitch_w),row_head_uv,rect->w);
            row_head_uv += pitch_w;
            r+=2;
        }
    } else {
        //not supported
    }
}


/*
func:test_gen_color_blocks
desc: generate color blocks to verify GSP and DISPC endian setting, block's color follows the below chart
-------------------------
|Red    |Green  |Blue   |
-------------------------
|Yellow |Magenta|Cyan   |
-------------------------
|Black  |White  |Gray   |
-------------------------
*/
void SprdUtil::test_gen_color_blocks(char* base,uint32_t pitch_w,uint32_t pitch_h,uint16_t format,uint16_t gray)
{
    uint16_t c = 0;
    uint16_t r = 0;
    uint16_t i = 0;

    uint16_t block_w = 0;
    uint16_t block_h = 0;

    uint16_t block_w_end = 0;
    uint16_t block_h_end = 0;

    struct sprdRect rects[9];

    //params check
    if(base == NULL || pitch_w <90 || pitch_h < 90) {
        return;
    }

    block_w = (pitch_w/3 & 0xfffffffe);
    block_w_end = pitch_w - block_w*2;

    block_h = (pitch_h/3 & 0xfffffffe);
    block_h_end = pitch_h - block_h*2;

    memset((void*)&rects,0,sizeof(rects));
    r=0;
    while(r<3) {
        c=0;
        while(c<3) {
            rects[i].x = block_w*c;
            rects[i].y = block_h*r;

            if(c == 2) {
                rects[i].w = block_w_end;
            } else {
                rects[i].w = block_w;
            }

            if(r == 2) {
                rects[i].h = block_h_end;
            } else {
                rects[i].h = block_h;
            }
            test_gen_color_block(base, pitch_w,pitch_h,format, &rects[i],(GEN_COLOR)i,gray);
            i++;
            c++;
        }
        r++;
    }
}

/*
func:formatType_convert
desc: image raw data store format covert from andriod hal type to gsp type
return: gsp type
*/
GSP_LAYER_SRC_DATA_FMT_E SprdUtil::formatType_convert(int format)
{
    switch(format) {
        case HAL_PIXEL_FORMAT_YCbCr_420_SP://0x19
            return GSP_SRC_FMT_YUV420_2P;
        case HAL_PIXEL_FORMAT_YCrCb_420_SP:
            return GSP_SRC_FMT_YUV420_2P;
        case HAL_PIXEL_FORMAT_YV12:
            return GSP_SRC_FMT_YUV420_3P;
        case HAL_PIXEL_FORMAT_RGBA_8888:
            return GSP_SRC_FMT_ARGB888;//?
        case HAL_PIXEL_FORMAT_RGBX_8888:
            return GSP_SRC_FMT_RGB888;//?
        case HAL_PIXEL_FORMAT_RGB_565:
            return GSP_SRC_FMT_RGB565;
        default:
            break;
    }

    ALOGE("util[%04d] err:unknow src format:%d!",__LINE__,format);
    return GSP_SRC_FMT_MAX_NUM;
}

/*
func:test_color
desc: generate a pure color block and write this data back to layer's data buffer.
        designed to check gsp hal endian setting, and compare the gsp accuracy with gpu.
*/
void SprdUtil::test_color(struct private_handle_t *private_h, GSP_LAYER_SRC_DATA_FMT_E img_format)
{
    int IntFlag = 0;
    queryIntFlag("gsp.gen.flag", &IntFlag);
    if(IntFlag>0) {
        if (private_h) {
            Rect bounds(private_h->width, private_h->height);
            void* vaddr = NULL;

            GraphicBufferMapper::get().lock((buffer_handle_t)private_h, GRALLOC_USAGE_SW_WRITE_OFTEN, bounds, &vaddr);

            if(vaddr != NULL) {
                ALOGI_IF(1,"composerLayers[%d],test, set white framework in y plane",__LINE__);
                if(IntFlag&0x1) {
                    //test yuv endian
                    test_gen_color_blocks((char*)vaddr,private_h->width,private_h->height,img_format,IntFlag&0x4);
                }
                if(IntFlag&0x2) {
                    //test holonomy of video frame
                    test_gen_white_boundary((char*)vaddr,private_h->width,private_h->height,img_format);
                }
            }
            GraphicBufferMapper::get().unlock((buffer_handle_t)private_h);
        }
    }
}

/*
func:test_color_for_prepare
desc: generate a pure color block and write this data back to layer's data buffer.
        designed to check gsp hal endian setting, and compare the gsp accuracy with gpu.
        should be called in hwc prepare phase.
*/
void SprdUtil::test_color_for_prepare(hwc_display_contents_1_t *list)
{
    hwc_layer_1_t *l = &(list->hwLayers[0]);
    struct private_handle_t *private_h = (struct private_handle_t *)l->handle;
    GSP_LAYER_SRC_DATA_FMT_E img_format = formatType_convert(private_h->format);
    test_color(private_h, img_format);
}

/*
func:rotationType_convert
desc: rotation angle covert from andriod hal type to gsp type
return: gsp type
*/
GSP_ROT_ANGLE_E SprdUtil::rotationType_convert(int angle)
{
    switch(angle) {
        case 0:
            return GSP_ROT_ANGLE_0;
        case HAL_TRANSFORM_FLIP_H:// 1
            return GSP_ROT_ANGLE_180_M;
        case HAL_TRANSFORM_FLIP_V:// 2
            return GSP_ROT_ANGLE_0_M;
        case HAL_TRANSFORM_ROT_180:// 3
            return GSP_ROT_ANGLE_180;
        case HAL_TRANSFORM_ROT_90:// 4
        default:
            return GSP_ROT_ANGLE_270;
        case (HAL_TRANSFORM_ROT_90|HAL_TRANSFORM_FLIP_H)://5
            return GSP_ROT_ANGLE_270_M;
        case (HAL_TRANSFORM_ROT_90|HAL_TRANSFORM_FLIP_V)://6
            return GSP_ROT_ANGLE_90_M;
        case HAL_TRANSFORM_ROT_270:// 7
            return GSP_ROT_ANGLE_90;
    }

    ALOGE("util[%04d] err:unknow src angle !",__LINE__);
    return GSP_ROT_ANGLE_0;
}


/*
func:need_scaling_check
desc: check the input layer need scaling or not
return: 0: don't need scaling; 1: need scaling; other: err
*/
int SprdUtil::need_scaling_check(SprdHWLayer *layer)
{
    struct sprdRect *srcRect = NULL;
    struct sprdRect *dstRect = NULL;
    hwc_layer_1_t *andriod_layer = NULL;
    GSP_ROT_ANGLE_E rot = GSP_ROT_ANGLE_MAX_NUM;

    if(layer == NULL) {
        return -1;
    }

    srcRect = layer->getSprdSRCRect();
    dstRect = layer->getSprdFBRect();
    andriod_layer = layer->getAndroidLayer();
    rot = rotationType_convert(andriod_layer->transform);
    if(rot & 0x1) {
        if(srcRect->w==dstRect->h && srcRect->h==dstRect->w) {
            //ALOGI_IF(1,"util[%04d] no scaling needed",__LINE__);
            return 0;// no scaling
        }
    } else {
        if(srcRect->w==dstRect->w && srcRect->h==dstRect->h) {
            //ALOGI_IF(1,"util[%04d] no scaling needed",__LINE__);
            return 0;// no scaling
        }
    }
    return 1;
}

/*
func:RegionEqualCheck
desc: compare the layer's dst region with DstRegion.
return: 1: equal; 0: not
*/
int SprdUtil::RegionEqualCheck(SprdHWLayer *pLayer,struct sprdRect *DstRegion)
{
    if(pLayer == NULL) return -1;

    struct sprdRect *Rect = pLayer->getSprdFBRect();
    /*
        ALOGI_IF(1,"util[%04d] region compare [w%d,h%d] vs [w%d,h%d]",__LINE__,
                 Rect->w,Rect->h,
                 DstRegion->w,DstRegion->h);
    */
    if(Rect->w == DstRegion->w // dst width == fb width
       && Rect->h == DstRegion->h) {
        return 1;
    }
    return 0;
}

/*
func:RegionAreaCheck
desc:check sum of the first two layer area is big or equal dst area
warning: there must have more than one layer in list
return: 1: bigger or equal; 0: not
*/
int SprdUtil::compositeAreaCheck(SprdHWLayer **LayerList,struct sprdRect *DstRegion)
{
    struct sprdRect *Rect = NULL;
    uint32_t sum_area = 0;
    uint32_t dst_area = 0;
    int i = 0;

    if(LayerList == NULL|| DstRegion == NULL) return -1;

    while(i<2) {
        Rect = LayerList[i]->getSprdFBRect();
        sum_area += Rect->w * Rect->h;
        i++;
    }
    dst_area = DstRegion->w*DstRegion->h;

    //ALOGI_IF(1,"util[%04d] sum_area:%d, dst_area:%d",__LINE__,sum_area,dst_area);

    if(sum_area >= dst_area) {
        return 1;
    }
    return 0;
}

/*
func:scalingup_twice
desc:gsp can only process 1/16-4 range scaling, if scaling up range beyond this limitation,
        we can scaling up twice to achieve that. this func designed to check the judge condition.
return: 1:need scaling up twice ; other : no necessary
*/
int SprdUtil::scaling_up_twice_check(GSP_CONFIG_INFO_T &gsp_cfg_info)
{
    if(gsp_cfg_info.layer0_info.layer_en == 1) { // layer0 be enabled
        if((gsp_cfg_info.layer0_info.rot_angle & 0x1) == 0) { // 0 180 degree
            if(((gsp_cfg_info.layer0_info.clip_rect.rect_w * 4) < gsp_cfg_info.layer0_info.des_rect.rect_w)
               ||((gsp_cfg_info.layer0_info.clip_rect.rect_h * 4) < gsp_cfg_info.layer0_info.des_rect.rect_h)) {
                return 1;
            }
        } else { // 90 270 degree
            if(((gsp_cfg_info.layer0_info.clip_rect.rect_w * 4) < gsp_cfg_info.layer0_info.des_rect.rect_h)
               || ((gsp_cfg_info.layer0_info.clip_rect.rect_h * 4) < gsp_cfg_info.layer0_info.des_rect.rect_w)) {
                return 1;
            }
        }
    }
    return 0;
}


int SprdUtil::gen_points_from_rect(struct sprdPoint *outPoints,SprdHWLayer *layer)
{
    int i = 0;
    struct sprdRect *Rect = NULL;

    if(layer == NULL || outPoints == NULL) {
        return -1;
    }

    Rect = layer->getSprdFBRect();

    outPoints[i].x = Rect->x;
    outPoints[i].y = Rect->y;

    i++;
    outPoints[i].x = Rect->x+Rect->w;
    outPoints[i].y = Rect->y;

    i++;
    outPoints[i].x = Rect->x+Rect->w;
    outPoints[i].y = Rect->y+Rect->h;

    i++;
    outPoints[i].x = Rect->x;
    outPoints[i].y = Rect->y+Rect->h;

    return 0;
}

/*
func:RegionAreaCheck
desc: the src 2 layer's dst region have 8 points, while full region have 4.
        this function used to check the 4 points is in the 8 point or not.
warning: there must have more than one layer in list
return: 1: dst 4 points in src 8 points; other : not
*/
int SprdUtil::compositePointCheck(SprdHWLayer **LayerList,struct sprdRect *DstRegion)
{
    int hit_cnt = 0,ret =0,i=0,j=0;

    struct sprdPoint srcPoints[8];
    struct sprdPoint dstPoints[4];
    memset((void*)srcPoints,0,sizeof(srcPoints));
    memset((void*)dstPoints,0,sizeof(dstPoints));

    ret = gen_points_from_rect(srcPoints,LayerList[0]);
    ret |= gen_points_from_rect(&srcPoints[4],LayerList[1]);
    if(ret) return ret;


    dstPoints[i].x = DstRegion->x;
    dstPoints[i].y = DstRegion->y;

    i++;
    dstPoints[i].x = DstRegion->x+DstRegion->w;
    dstPoints[i].y = DstRegion->y;

    i++;
    dstPoints[i].x = DstRegion->x+DstRegion->w;
    dstPoints[i].y = DstRegion->y+DstRegion->h;

    i++;
    dstPoints[i].x = DstRegion->x;
    dstPoints[i].y = DstRegion->y+DstRegion->h;

    i=0;
    while(i<4) {
        j=0;
        while(j<8) {
            if(dstPoints[i].x == srcPoints[j].x && dstPoints[i].y == srcPoints[j].y) {
                hit_cnt++;
                break;
            }
            j++;
        }
        i++;
    }

    if(hit_cnt == 4) {
        return 1;
    }

    return 0;
}


/*
static int get_layer_color(SprdHWLayer *layer)
{
    hwc_layer_1_t *hwcLayer = layer->getAndroidLayer();

    if (hwcLayer == NULL)
    {
        ALOGE("util[%04d] Failed to get Video SprdHWLayer parameters!",__LINE__);
        return -1;
    }

    struct private_handle_t *private_h = (struct private_handle_t *)(hwcLayer->handle);
    if(private_h == NULL)
    {
        ALOGE("util[%04d] err:private_h == NULL,return",__LINE__);
        return -1;
    }
    return formatType_convert(private_h->format);
}
*/
/*
func:findAnIndependentLayer
desc: find an independent layer, which is not overlap with any other layers
return: if find it, return the index in list, or -1
*/
int SprdUtil::findAnIndependentLayer(SprdHWLayer **LayerList, int cnt)
{
    struct sprdRect *candidateRect = NULL;
    struct sprdRect *otherRect = NULL;
    int i=0,j=0;
    int c=0;// not overlay cnt

    j=1;
    while(j<cnt) {
        candidateRect = LayerList[j]->getSprdFBRect();
        c=0;
        i=0;
        while(i<cnt) {
            if(i != j) {
                otherRect = LayerList[i]->getSprdFBRect();

                if((otherRect->x+otherRect->w <= candidateRect->x)
                   ||(otherRect->y+otherRect->h <= candidateRect->y)
                   ||(otherRect->x >= candidateRect->x+candidateRect->w)
                   ||(otherRect->y >= candidateRect->y+candidateRect->h)) {
                    c++;
                }
            }
            i++;
        }
        if(c==cnt-1) {
            return j;
        }

        j++;
    }
    return -1;
}

/*
func:full_screen_check
desc:checking the head two layers of LayerList to find if their dst region is composite full screen,
       if it's full screen, we can composite them in one gsp process.
*/
int SprdUtil::full_screen_check(SprdHWLayer **LayerList, int cnt, FrameBufferInfo *FBInfo)
{
    struct sprdRect DstRegion= {0,0,0,0};
    int i = 0;

    if(LayerList == NULL || FBInfo == NULL) {
        return -1;
    }

    DstRegion.w = FBInfo->fb_width;
    DstRegion.h = FBInfo->fb_height;

    //compare the head two layer's dst region with FB region to find full screen layer out.
    while(i<2) {
        if(1==RegionEqualCheck(LayerList[i],&DstRegion)) {
            //ALOGI_IF(1,"util[%04d] exist full screen size in first two layers",__LINE__);
            return 1;
        }
        i++;
    }

    // if there is an independent layer, we adjust it to bottom of the list
    if(cnt > 2) {
        i = findAnIndependentLayer(LayerList, cnt);
        if(i != -1 && 0< i && i < cnt) { // find independent layer
            //move it to list[1]
            SprdHWLayer *tempLayer = LayerList[i];
            //ALOGI_IF(1,"util[%04d] find an independent layer, move it from list[%d] to list[1]",__LINE__,i);
            while(i>1) {
                LayerList[i] = LayerList[i-1];
                i--;
            }
            LayerList[i] = tempLayer;
        }
    }

    // to check if the first two layer's destination region can composite out a full screen region
    if(1==compositeAreaCheck(LayerList,&DstRegion)
       && 1==compositePointCheck(LayerList,&DstRegion)) {
        return 1;
    }

    return 0;
}

/*
func:gsp_intermedia_dump
desc: in gsp multi-layer case, dump gsp intermedia output
*/
void SprdUtil::gsp_intermedia_dump(private_handle_t* dst_buffer)
{
    int mDumpFlag = 0;
    if(dst_buffer == NULL) return;

    queryDumpFlag(&mDumpFlag);
    if (HWCOMPOSER_DUMP_MULTI_LAYER_FLAG & mDumpFlag) {
        dumpOverlayImage(dst_buffer, "GSPMULTI");
    }
}

/*
func:gsp_split_pages_check
desc:check whether this gsp call need rot or scaling or not, to decide split_pages flag setting
return: 1: flag need set to 1   0:flag need set to 0
*/
int SprdUtil::gsp_split_pages_check(GSP_CONFIG_INFO_T &gsp_cfg_info)
{
    if(gsp_cfg_info.layer0_info.layer_en == 1 && gsp_cfg_info.layer0_info.pallet_en == 0) {
        //for black line bug chip, if scaling required, split_pages should be set to 0 to avoid hunging up.
        if((gsp_cfg_info.layer0_info.rot_angle & 0x1) == 0
           &&(gsp_cfg_info.layer0_info.clip_rect.rect_w != gsp_cfg_info.layer0_info.des_rect.rect_w
              || gsp_cfg_info.layer0_info.clip_rect.rect_h != gsp_cfg_info.layer0_info.des_rect.rect_h)) {
            return 0;
        }
        if((gsp_cfg_info.layer0_info.rot_angle & 0x1) == 1
           &&(gsp_cfg_info.layer0_info.clip_rect.rect_w != gsp_cfg_info.layer0_info.des_rect.rect_h
              || gsp_cfg_info.layer0_info.clip_rect.rect_h != gsp_cfg_info.layer0_info.des_rect.rect_w)) {
            return 0;
        }

        //for YUV format, rotation also can cause hunging up
        if(gsp_cfg_info.layer0_info.img_format > GSP_SRC_FMT_RGB565 // not RGB
           && gsp_cfg_info.layer0_info.rot_angle != GSP_ROT_ANGLE_0) {
            return 0;
        }
    }

    if(gsp_cfg_info.layer1_info.layer_en == 1 && gsp_cfg_info.layer1_info.pallet_en == 0) {
        if(gsp_cfg_info.layer1_info.img_format > GSP_SRC_FMT_RGB565 // not RGB
           && gsp_cfg_info.layer1_info.rot_angle != GSP_ROT_ANGLE_0) {
            return 0;
        }
    }
    return 1;
}


/*
func:openGSPDevice
desc:load gsp hal so
return: 0:success ; other failed
*/
int SprdUtil::openGSPDevice()
{
    hw_module_t const* pModule;

    if (hw_get_module(GSP_HARDWARE_MODULE_ID, &pModule) == 0) {
        pModule->methods->open(pModule, "gsp", (hw_device_t**)(&mGspDev));
        if (mGspDev == NULL) {
            ALOGE("hwcomposer open GSP lib failed! ");
            return -1;
        }
    } else {
        ALOGE("hwcomposer can't find GSP lib ! ");
        return -1;
    }

    return 0;
}

int SprdUtil:: acquireTmpBuffer(int width, int height, int format, private_handle_t* friendBuffer, int *outBufferPhy, int *outBufferSize)
{
    int GSPOutputFormat = -1;
#ifdef VIDEO_LAYER_USE_RGB
    GSPOutputFormat = HAL_PIXEL_FORMAT_RGBX_8888;
#else
#ifdef GSP_OUTPUT_USE_YUV420
    GSPOutputFormat = HAL_PIXEL_FORMAT_YCbCr_420_SP;
#else
    GSPOutputFormat = HAL_PIXEL_FORMAT_YCbCr_422_SP;
#endif
#endif
    uint32_t stride;

    if (friendBuffer == NULL) {
        ALOGE("util[%04d] err:acquireTmpBuffer: Input parameter is NULL!",__LINE__);
        return -1;
    }

    if (GSPOutputFormat == HAL_PIXEL_FORMAT_YCbCr_420_SP ||
        GSPOutputFormat == HAL_PIXEL_FORMAT_YCbCr_422_SP) {
#ifdef BORROW_PRIMARYPLANE_BUFFER
        if (friendBuffer->format != HAL_PIXEL_FORMAT_RGBA_8888) {
            ALOGE("util[%04d] err:Friend buffer need to be RGBA8888!",__LINE__);
            goto AllocGFXBuffer;
        }

        /*
         *  Borrow buffer memory from PrimaryPlane buffer.
         *  Just use 2.0 --- 2.75 (4 bytes for RGBA8888)
         * */
        int offset = width * height * (1.5 + 0.5);
        *outBufferSize = (int)((float)width * (float)height * 1.5 * 0.5);
        *outBufferPhy = friendBuffer->phyaddr + offset;
#else
        goto AllocGFXBuffer;
#endif
    } else if (GSPOutputFormat == HAL_PIXEL_FORMAT_RGBX_8888) {
        goto AllocGFXBuffer;
    }

    return 0;

AllocGFXBuffer:
	if (tmpBuffer != NULL) {
        ALOGE("util[%04d]  alloc tmpBuffer only once for saving memory, never release!",__LINE__);
        return 0;
    }

    if(mGsp_cap.buf_type_support == GSP_ADDR_TYPE_PHYSICAL) {
        GraphicBufferAllocator::get().alloc(width, height, format, GRALLOC_USAGE_OVERLAY_BUFFER, (buffer_handle_t*)&tmpBuffer, &stride);
    } else if(mGsp_cap.buf_type_support == GSP_ADDR_TYPE_IOVIRTUAL) {
        GraphicBufferAllocator::get().alloc(width, height, format, 0, (buffer_handle_t*)&tmpBuffer, &stride);
    }

    if (tmpBuffer == NULL) {
        ALOGE("util[%04d] err:Cannot alloc the tmpBuffer ION buffer!",__LINE__);
        return -1;
    }

    return 0;
}

/*
func:getGSPCapability
desc: get gsp process capability from kernel, and according to fb pixel size, adjust it to a proper setting
        kernel driver can read chip id , so it can get GSP HW version, then can evaluate capability.
return: 0:success ; other failed
*/
int SprdUtil::getGSPCapability(GSP_CAPABILITY_T *pGsp_cap)
{
    int32_t ret = 0;
    if (mGspDev == 0) {
        int r = openGSPDevice();
        if (r != 0 || mGspDev == 0) {
            ALOGE("util[%04d] err:open GSP device failed!!",__LINE__);
            return -1;
        }
    }
    if(mGspDev && pGsp_cap != NULL) {
        ret = mGspDev->GSP_GetCapability(pGsp_cap);
        if(ret != 0) {
            ALOGE("util[%04d] err:GSP_GetCapability return err!",__LINE__);
            return ret;
        } else {
            ALOGI_IF(1,"util[%04d] GSP_GetCapability [version:%d, addrType:%d,max_layer:%d,need_copy:%d,scaling_range:%d,blendVideoWithosd:%d,withVideoMaxLayer:%d]",__LINE__,
                     pGsp_cap->version,
                     pGsp_cap->buf_type_support,
                     pGsp_cap->max_layer_cnt,
                     pGsp_cap->video_need_copy,
                     pGsp_cap->scale_range_up,
                     pGsp_cap->blend_video_with_OSD,
                     pGsp_cap->max_layer_cnt_with_video);
            if(mGsp_cap.buf_type_support != 0) {
                ALOGI_IF(mDebugFlag,"util[%04d] GSP_GetCapability HWC force buffer addr type to %d.",__LINE__,mGsp_cap.buf_type_support);
                pGsp_cap->buf_type_support = mGsp_cap.buf_type_support;
            }
            //if it's a perfect chipset, like tshark v2, can process multi-layers, we adjusting it's capability adapt to fb size.


            if(pGsp_cap->max_layer_cnt > 3) {
                int fb_pixel = mFBInfo->fb_width * mFBInfo->fb_height;

                ALOGI_IF(mDebugFlag,"util[%04d] get from kernel,max:%d,maxinvideo:%d.",__LINE__,
                         pGsp_cap->max_layer_cnt,pGsp_cap->max_layer_cnt_with_video);
                /*
                (             ~ 240*320] support 8 layer,         ~76800
                (240*320 ~ 320*480] support 6 layer, 76800~153600
                (320*480 ~ 480*800) support 5 layer, 153600~384000
                [480*800 ~ 540*960] support 4 layer, 384000~518400
                (540*960 ~ 720*1280) support 3 layer, 518400~921600
                [720*1280 ~           ] support 2 layer, 518400~921600
                */
                if(fb_pixel <= 76800) {
                    pGsp_cap->max_layer_cnt = 8;
                    pGsp_cap->max_layer_cnt_with_video = 5;
                } else if(fb_pixel <= 153600) {
                    pGsp_cap->max_layer_cnt = 6;
                    pGsp_cap->max_layer_cnt_with_video = 5;
                } else if(fb_pixel < 384000) {
                    pGsp_cap->max_layer_cnt = 5;
                    pGsp_cap->max_layer_cnt_with_video = 5;
                } else if(fb_pixel <= 518400) {
                    pGsp_cap->max_layer_cnt = 4;
                    pGsp_cap->max_layer_cnt_with_video = 4;
                } else if(fb_pixel < 921600) {
                    pGsp_cap->max_layer_cnt = 3;
                    pGsp_cap->max_layer_cnt_with_video = 3;
                } else {
                    pGsp_cap->max_layer_cnt = 2;
                    pGsp_cap->max_layer_cnt_with_video = 2;
                }
                ALOGI_IF(mDebugFlag,"util[%04d] after adjust,max:%d,maxinvideo:%d.",__LINE__,
                         pGsp_cap->max_layer_cnt,pGsp_cap->max_layer_cnt_with_video);
            }
            mGsp_cap = *pGsp_cap;
            ALOGE("util[%04d] GSP_GetCapability ok.",__LINE__);
        }
    }

    return ret;
}


/*
func:gsp_process_va_copy2_pa
desc:copy va image buffer to pa buffer,
warning: the layer0 image must yuv420_2p, layer1 must be RGB,
         the pa buffer size is fixed in the function, can't over that size.
*/
int SprdUtil::gsp_process_va_copy2_pa(GSP_CONFIG_INFO_T *pgsp_cfg_info)
{
    uint32_t VIDEO_MAX_WIDTH = 1920;
    uint32_t VIDEO_MAX_HEIGHT = 1088;

    int OSD_MAX_WIDTH = 0;
    int OSD_MAX_HEIGHT = 0;

    uint32_t stride = 0;
    int ret = 0;
    int size = 0;
    //int format = HAL_PIXEL_FORMAT_RGBA_8888;
    GSP_CONFIG_INFO_T cfg_info;
    void* vaddr = NULL;

    if((pgsp_cfg_info == NULL)
       ||((pgsp_cfg_info->layer0_info.layer_en == 1)&&((pgsp_cfg_info->layer1_info.layer_en == 1) && (pgsp_cfg_info->layer1_info.pallet_en == 0)))
       ||((pgsp_cfg_info->layer0_info.layer_en == 0)&&(pgsp_cfg_info->layer1_info.layer_en == 0))) {
        ALOGE("util[%04d] copy:params check err.",__LINE__);
        return -1;
    }
    if(mGsp_cap.max_video_size == 1) { // 720P
        VIDEO_MAX_WIDTH = 1280;
        VIDEO_MAX_HEIGHT = 720;
    }
    cfg_info = *pgsp_cfg_info;

    if(mFBInfo == NULL) {
        ALOGE("util[%04d] copy:mFBInfo==NULL, return.",__LINE__);
        return -1;
    }
    OSD_MAX_WIDTH = mFBInfo->fb_width+2;// sometimes , surfaceflinger sends 856x480 rgb data, which is larger than fb size
    OSD_MAX_HEIGHT = mFBInfo->fb_height+2;


    if((cfg_info.layer0_info.layer_en == 1)
       &&((cfg_info.layer0_info.img_format != GSP_SRC_FMT_YUV420_2P)
          ||(cfg_info.layer0_info.pitch*(cfg_info.layer0_info.clip_rect.st_y+cfg_info.layer0_info.clip_rect.rect_h) > VIDEO_MAX_WIDTH*VIDEO_MAX_HEIGHT))) {
        ALOGE("util[%04d] copy:format:%d is not yuv4202p or video src buffer is larger than the pa buffer!",__LINE__,cfg_info.layer0_info.img_format);
        return -1;
    }

    if(cfg_info.layer1_info.layer_en == 1) {
        if(cfg_info.layer1_info.pallet_en == 0) {
            if(cfg_info.layer1_info.pitch*(cfg_info.layer1_info.clip_rect.st_y+cfg_info.layer1_info.clip_rect.rect_h) > (uint32_t)(OSD_MAX_WIDTH*OSD_MAX_HEIGHT)) {
                ALOGE("util[%04d] copy:osd src buffer is larger than the pa buffer!",__LINE__);
                return -1;
            }
        } else {
            ALOGI_IF(mDebugFlag,"util[%04d] copy:osd is pallet, don't need copy",__LINE__);
            return 0;
        }
    }

    if(copyTempBuffer == NULL) {
        //GraphicBufferAllocator::get().alloc(mFBInfo->fb_width, mFBInfo->fb_height, format, GRALLOC_USAGE_OVERLAY_BUFFER, (buffer_handle_t*)&copyTempBuffer, &stride);
        if(OSD_MAX_WIDTH*OSD_MAX_HEIGHT*4 > VIDEO_MAX_WIDTH*VIDEO_MAX_HEIGHT*1.5) {
            GraphicBufferAllocator::get().alloc(OSD_MAX_WIDTH, OSD_MAX_HEIGHT, HAL_PIXEL_FORMAT_RGBA_8888, GRALLOC_USAGE_OVERLAY_BUFFER, (buffer_handle_t*)&copyTempBuffer, &stride);
        } else {
            GraphicBufferAllocator::get().alloc(VIDEO_MAX_WIDTH, VIDEO_MAX_HEIGHT, HAL_PIXEL_FORMAT_YCbCr_420_SP, GRALLOC_USAGE_OVERLAY_BUFFER, (buffer_handle_t*)&copyTempBuffer, &stride);
        }
        if (copyTempBuffer == NULL) {
            ALOGE("util[%04d] copy:copyTempBuffer==NULL,alloc buffer failed!",__LINE__);
            ret = -1;
            return ret;
        }
    }

    if(cfg_info.layer0_info.layer_en == 1) {
        // video layer : va cpy pa scaling&rotation pa
        // osd layer : disable, do not care
        cfg_info.layer1_info.layer_en = 0;
        cfg_info.layer0_info.clip_rect.rect_w = cfg_info.layer0_info.pitch;
        cfg_info.layer0_info.clip_rect.st_x = 0;
        cfg_info.layer0_info.clip_rect.rect_h += cfg_info.layer0_info.clip_rect.st_y;
        cfg_info.layer0_info.clip_rect.st_y = 0;

        cfg_info.layer_des_info.mem_info.share_fd = copyTempBuffer->share_fd;
        cfg_info.layer_des_info.mem_info.uv_offset = cfg_info.layer0_info.clip_rect.rect_w*cfg_info.layer0_info.clip_rect.rect_h;
        cfg_info.layer_des_info.mem_info.v_offset = cfg_info.layer_des_info.mem_info.uv_offset;

        cfg_info.layer0_info.des_rect = cfg_info.layer0_info.clip_rect;
        cfg_info.layer0_info.rot_angle = GSP_ROT_ANGLE_0;
        cfg_info.layer_des_info.img_format = (GSP_LAYER_DST_DATA_FMT_E)cfg_info.layer0_info.img_format;
        memset(&cfg_info.layer0_info.endian_mode,0,sizeof(cfg_info.layer0_info.endian_mode));
        cfg_info.layer_des_info.endian_mode = cfg_info.layer0_info.endian_mode;

        cfg_info.layer_des_info.pitch = cfg_info.layer0_info.clip_rect.rect_w;
    } else {
        // video layer : disable, do not care
        // osd layer : va cpy pa scaling&rotation pa
        cfg_info.layer0_info.layer_en = 0;
        cfg_info.layer1_info.clip_rect.rect_w = cfg_info.layer1_info.pitch;
        cfg_info.layer1_info.clip_rect.st_x = 0;
        cfg_info.layer1_info.clip_rect.rect_h += cfg_info.layer1_info.clip_rect.st_y;
        cfg_info.layer1_info.clip_rect.st_y = 0;

        cfg_info.layer_des_info.mem_info.share_fd = copyTempBuffer->share_fd;
        cfg_info.layer_des_info.mem_info.uv_offset = cfg_info.layer1_info.clip_rect.rect_w*cfg_info.layer1_info.clip_rect.rect_h;
        cfg_info.layer_des_info.mem_info.v_offset = cfg_info.layer_des_info.mem_info.uv_offset;

        cfg_info.layer1_info.des_pos.pos_pt_x =
            cfg_info.layer1_info.des_pos.pos_pt_y = 0;
        cfg_info.layer1_info.rot_angle = GSP_ROT_ANGLE_0;
        cfg_info.layer_des_info.img_format = (GSP_LAYER_DST_DATA_FMT_E)cfg_info.layer1_info.img_format;
        memset(&cfg_info.layer1_info.endian_mode,0,sizeof(cfg_info.layer1_info.endian_mode));
        cfg_info.layer_des_info.endian_mode = cfg_info.layer1_info.endian_mode;

        cfg_info.layer_des_info.pitch = cfg_info.layer1_info.clip_rect.rect_w;

    }

    cfg_info.misc_info.split_pages = 1;//

    ALOGI_IF(mDebugFlag,"util[%04d] copy:L1==%d yaddr:%08x {p%d,s%d,f%d}[x%d,y%d,w%d,h%d] r%d [x%d,y%d,w%d,h%d]",__LINE__,
             cfg_info.layer0_info.layer_en,
             cfg_info.layer0_info.src_addr.addr_y,
             cfg_info.layer0_info.pitch,
             0,//private_h1->height,
             cfg_info.layer0_info.img_format,
             cfg_info.layer0_info.clip_rect.st_x,
             cfg_info.layer0_info.clip_rect.st_y,
             cfg_info.layer0_info.clip_rect.rect_w,
             cfg_info.layer0_info.clip_rect.rect_h,
             cfg_info.layer0_info.rot_angle,
             cfg_info.layer0_info.des_rect.st_x,
             cfg_info.layer0_info.des_rect.st_y,
             cfg_info.layer0_info.des_rect.rect_w,
             cfg_info.layer0_info.des_rect.rect_h);

    ALOGI_IF(mDebugFlag,"util[%04d] copy:L2==%d yaddr:%08x {p%d,s%d,f%d}[x%d,y%d,w%d,h%d] r%d [x%d,y%d]",__LINE__,
             cfg_info.layer1_info.layer_en,
             cfg_info.layer1_info.src_addr.addr_y,
             cfg_info.layer1_info.pitch,
             0,//private_h2->height,
             cfg_info.layer1_info.img_format,
             cfg_info.layer1_info.clip_rect.st_x,
             cfg_info.layer1_info.clip_rect.st_y,
             cfg_info.layer1_info.clip_rect.rect_w,
             cfg_info.layer1_info.clip_rect.rect_h,
             cfg_info.layer1_info.rot_angle,
             cfg_info.layer1_info.des_pos.pos_pt_x,
             cfg_info.layer1_info.des_pos.pos_pt_y);

    ALOGI_IF(mDebugFlag,"util[%04d] copy:Ld y_addr==%08x size==%08x {p%d,s%d,f%d}!",__LINE__,
             cfg_info.layer_des_info.src_addr.addr_y,
             size,
             cfg_info.layer_des_info.pitch,
             0,
             cfg_info.layer_des_info.img_format);

    ret = mGspDev->GSP_Proccess(&cfg_info);
    if(ret == 0) {
        if(cfg_info.layer0_info.layer_en == 1) {
            pgsp_cfg_info->layer0_info.mem_info = cfg_info.layer_des_info.mem_info;
        } else {
            pgsp_cfg_info->layer1_info.mem_info = cfg_info.layer_des_info.mem_info;
        }
        ALOGI_IF(mDebugFlag,"util[%04d] copy:GSP_Proccess succes!",__LINE__);
        pgsp_cfg_info->misc_info.split_pages = 0;//
    } else {
        ALOGE("util[%04d] copy:GSP_Proccess failed:%d!",__LINE__,ret);
    }
    return ret;
}

/*
func:scalingup_twice
desc:gsp can only process 1/16-4 range scaling, if scaling up range beyond this limitation,
        we can scaling up twice to achieve that. this func designed to take this job.
return: 0:success ; other failed
*/
int SprdUtil::scalingup_twice(GSP_CONFIG_INFO_T &gsp_cfg_info, private_handle_t* dst_buffer)
{
    int buffersize_layert = 0;//scaling up twice temp
    GSP_CONFIG_INFO_T gsp_cfg_info_phase1 = gsp_cfg_info;
    GSP_LAYER_DST_DATA_FMT_E phase1_des_format = GSP_DST_FMT_YUV420_2P;//GSP_DST_FMT_YUV422_2P; //GSP_DST_FMT_ARGB888
    ALOGI_IF(mDebugFlag,"util[%04d] scale up twice enter. ",__LINE__);

    int ret = -1;
    int format = HAL_PIXEL_FORMAT_YCbCr_420_SP;
    ret = acquireTmpBuffer(mFBInfo->fb_width, mFBInfo->fb_height, format, dst_buffer, &outBufferPhy, &outBufferSize);
    if (ret != 0) {
        ALOGE("util[%04d] err:acquireTmpBuffer failed",__LINE__);
        return -1;
    }

    /*phase1*/
    gsp_cfg_info_phase1.layer_des_info.img_format = phase1_des_format;

    ALOGI_IF(mDebugFlag,"util[%04d] up2:mapped temp iommu addr:%08x,size:%08x",__LINE__,gsp_cfg_info_phase1.layer_des_info.src_addr.addr_y,buffersize_layert);

    if (outBufferPhy != 0 && outBufferSize > 0) {
        gsp_cfg_info_phase1.layer_des_info.src_addr.addr_y = (uint32_t)outBufferPhy;
    } else {
        gsp_cfg_info_phase1.layer_des_info.mem_info.share_fd = tmpBuffer->share_fd;
		gsp_cfg_info_phase1.layer_des_info.src_addr.addr_y = 0;
        gsp_cfg_info_phase1.layer_des_info.mem_info.uv_offset = mFBInfo->fb_width * mFBInfo->fb_height;
        gsp_cfg_info_phase1.layer_des_info.mem_info.v_offset = gsp_cfg_info_phase1.layer_des_info.mem_info.uv_offset;
    }

    gsp_cfg_info_phase1.layer_des_info.src_addr.addr_v =
        gsp_cfg_info_phase1.layer_des_info.src_addr.addr_uv =
            gsp_cfg_info_phase1.layer_des_info.src_addr.addr_y + mFBInfo->fb_width * mFBInfo->fb_height;

    gsp_cfg_info_phase1.layer0_info.des_rect.st_x = 0;
    gsp_cfg_info_phase1.layer0_info.des_rect.st_y = 0;
    gsp_cfg_info_phase1.layer0_info.des_rect.rect_w = gsp_cfg_info_phase1.layer0_info.clip_rect.rect_w;
    gsp_cfg_info_phase1.layer0_info.des_rect.rect_h = gsp_cfg_info_phase1.layer0_info.clip_rect.rect_h;
    if((gsp_cfg_info.layer0_info.rot_angle & 0x1) == 0) {
        if((gsp_cfg_info_phase1.layer0_info.clip_rect.rect_w * 4) < gsp_cfg_info.layer0_info.des_rect.rect_w) {
            gsp_cfg_info_phase1.layer0_info.des_rect.rect_w = ((gsp_cfg_info.layer0_info.des_rect.rect_w + 7)/4 & 0xfffe);
        }
        if((gsp_cfg_info_phase1.layer0_info.clip_rect.rect_h * 4) < gsp_cfg_info.layer0_info.des_rect.rect_h) {
            gsp_cfg_info_phase1.layer0_info.des_rect.rect_h = ((gsp_cfg_info.layer0_info.des_rect.rect_h + 7)/4 & 0xfffe);
        }
    } else {
        if((gsp_cfg_info_phase1.layer0_info.clip_rect.rect_w * 4) < gsp_cfg_info.layer0_info.des_rect.rect_h) {
            gsp_cfg_info_phase1.layer0_info.des_rect.rect_w = ((gsp_cfg_info.layer0_info.des_rect.rect_h + 7)/4 & 0xfffe);
        }
        if((gsp_cfg_info_phase1.layer0_info.clip_rect.rect_h * 4) < gsp_cfg_info.layer0_info.des_rect.rect_w) {
            gsp_cfg_info_phase1.layer0_info.des_rect.rect_h = ((gsp_cfg_info.layer0_info.des_rect.rect_w + 7)/4 & 0xfffe);
        }
    }
    gsp_cfg_info_phase1.layer_des_info.pitch = gsp_cfg_info_phase1.layer0_info.des_rect.rect_w;
    gsp_cfg_info_phase1.layer0_info.rot_angle = GSP_ROT_ANGLE_0;
    gsp_cfg_info_phase1.layer1_info.layer_en = 0;//disable Layer1

    ALOGI_IF(mDebugFlag,"util[%04d] up2:phase 1,src_addr_y:0x%08x,des_addr_y:0x%08x",__LINE__,
             gsp_cfg_info_phase1.layer0_info.src_addr.addr_y,
             gsp_cfg_info_phase1.layer_des_info.src_addr.addr_y);

    ret = mGspDev->GSP_Proccess(&gsp_cfg_info_phase1);
    if(0 == ret) {
        ALOGI_IF(mDebugFlag,"util[%04d] up2:phase 1,GSP_Proccess success",__LINE__);
    } else {
        ALOGE("util[%04d] up2:phase 1,GSP_Proccess failed!! debugenable = 1;",__LINE__);
        mDebugFlag = 1;
        return ret;
    }

    /*phase2*/
    gsp_cfg_info.layer0_info.img_format = (GSP_LAYER_SRC_DATA_FMT_E)phase1_des_format;
    gsp_cfg_info.layer0_info.clip_rect = gsp_cfg_info_phase1.layer0_info.des_rect;
    gsp_cfg_info.layer0_info.pitch = gsp_cfg_info_phase1.layer_des_info.pitch;
    gsp_cfg_info.layer0_info.src_addr = gsp_cfg_info_phase1.layer_des_info.src_addr;
    gsp_cfg_info.layer0_info.endian_mode = gsp_cfg_info_phase1.layer_des_info.endian_mode;
    gsp_cfg_info.layer0_info.mem_info = gsp_cfg_info_phase1.layer_des_info.mem_info;
	if (gsp_cfg_info_phase1.layer_des_info.mem_info.share_fd != 0)
		gsp_cfg_info.layer0_info.src_addr.addr_y = 0;

    ALOGI_IF(mDebugFlag,"util[%04d] up2:phase 2,src_addr_y:0x%08x,des_addr_y:0x%08x",__LINE__,
             gsp_cfg_info.layer0_info.src_addr.addr_y,
             gsp_cfg_info.layer_des_info.src_addr.addr_y);
    return 0;
}


int SprdUtil::gsp_image_layer_config(SprdHWLayer *layer,
                                     GSP_CONFIG_INFO_T &gsp_cfg_info,
                                     GSP_CONFIG_INFO_T *pgsp_cfg_info)
{
    // parameters check
    if(layer == NULL && pgsp_cfg_info == NULL) {
        ALOGE("util[%04d] parameters err,return!!",__LINE__);
        return -1;
    }

    if(layer != NULL) {
        hwc_layer_1_t *hwcLayer = layer->getAndroidLayer();
        struct sprdRect *srcRect = layer->getSprdSRCRect();
        struct sprdRect *dstRect = layer->getSprdFBRect();
        if (hwcLayer == NULL || srcRect == NULL || dstRect == NULL) {
            ALOGE("util[%04d] Failed to get Video SprdHWLayer parameters!",__LINE__);
            return -1;
        }

        struct private_handle_t *private_h = (struct private_handle_t *)(hwcLayer->handle);
        if(private_h == NULL) {
            ALOGE("util[%04d] err:private_h == NULL,return",__LINE__);
            return -1;
        }

        ALOGI_IF(mDebugFlag,"util[%04d] imgLayer src info [f:%x,pm:%d,x%d,y%d,w%d,h%d,p%d,s%d] r%d [x%d,y%d,w%d,h%d]",__LINE__,
                 private_h->format,
                 hwcLayer->blending,
                 srcRect->x, srcRect->y,
                 srcRect->w, srcRect->h,
                 private_h->width, private_h->height,
                 hwcLayer->transform,
                 dstRect->x, dstRect->y,
                 dstRect->w, dstRect->h);

        if((mGsp_cap.buf_type_support == GSP_ADDR_TYPE_IOVIRTUAL)
           ||((mGsp_cap.buf_type_support == GSP_ADDR_TYPE_PHYSICAL) && (private_h->flags & private_handle_t::PRIV_FLAGS_USES_PHY))) {
            //config Video ,use GSP L0
            //uint32_t pixel_cnt = private_h->stride * ALIGN(private_h->height, 16);
            uint32_t pixel_cnt = private_h->stride * private_h->height;
            gsp_cfg_info.layer0_info.mem_info.share_fd = private_h->share_fd;
            gsp_cfg_info.layer0_info.mem_info.v_offset =
                gsp_cfg_info.layer0_info.mem_info.uv_offset = pixel_cnt;

            gsp_cfg_info.layer0_info.img_format = formatType_convert(private_h->format);
            switch(private_h->format) {
                case HAL_PIXEL_FORMAT_RGBA_8888:
                case HAL_PIXEL_FORMAT_RGBX_8888:
                    gsp_cfg_info.layer0_info.endian_mode.y_word_endn = GSP_WORD_ENDN_1;
                    gsp_cfg_info.layer0_info.endian_mode.a_swap_mode = GSP_A_SWAP_RGBA;
                    break;
                case HAL_PIXEL_FORMAT_YV12://YUV420_3P, Y V U
                    gsp_cfg_info.layer0_info.mem_info.uv_offset += (pixel_cnt>>2);
                    break;
                case HAL_PIXEL_FORMAT_YCrCb_420_SP:
                    gsp_cfg_info.layer0_info.endian_mode.uv_word_endn = GSP_WORD_ENDN_2;
                    break;
                case HAL_PIXEL_FORMAT_RGB_565:
                    gsp_cfg_info.layer0_info.endian_mode.rgb_swap_mode = GSP_RGB_SWP_RGB;
                    break;
                default:
                    break;
            }
            if(private_h->yuv_info == MALI_YUV_BT601_NARROW
               ||private_h->yuv_info == MALI_YUV_BT709_NARROW) {
                gsp_cfg_info.misc_info.y2r_opt = 1;
            }

            if(GSP_SRC_FMT_RGB565 < gsp_cfg_info.layer0_info.img_format && gsp_cfg_info.layer0_info.img_format<GSP_SRC_FMT_8BPP ) {
                ALOGI_IF((private_h->stride%16 | private_h->height%16),"util[%04d] warning: buffer stride:%d or height:%d is not 16 aligned!",__LINE__,
                         private_h->stride,
                         private_h->height);
            }

            ALOGI_IF(mDebugFlag,"util[%04d] fd:%d u_offset:%08x,v_offset:%08x,",__LINE__,
                     gsp_cfg_info.layer0_info.mem_info.share_fd,
                     gsp_cfg_info.layer0_info.mem_info.uv_offset,
                     gsp_cfg_info.layer0_info.mem_info.v_offset);
            //gsp_cfg_info.layer0_info.src_addr.addr_v
            //    = gsp_cfg_info.layer0_info.src_addr.addr_uv
            //    = gsp_cfg_info.layer0_info.src_addr.addr_y + private_h1->width * private_h1->height;
            //gsp_cfg_info.layer0_info.src_addr.addr_v = gsp_cfg_info.layer0_info.src_addr.addr_uv = private_h0->phyaddr + context->src_img.w*context->src_img.h;
            gsp_cfg_info.layer0_info.clip_rect.st_x = srcRect->x;
            gsp_cfg_info.layer0_info.clip_rect.st_y = srcRect->y;
            gsp_cfg_info.layer0_info.clip_rect.rect_w = srcRect->w;
            gsp_cfg_info.layer0_info.clip_rect.rect_h = srcRect->h;

            gsp_cfg_info.layer0_info.alpha = hwcLayer->planeAlpha;
            if(hwcLayer->blending == HWC_BLENDING_PREMULT) {
                ALOGI_IF(mDebugFlag,"util[%04d] err:L0 blending flag:%x!",__LINE__,hwcLayer->blending);
            }
            gsp_cfg_info.layer0_info.pmargb_mod = 1;
            gsp_cfg_info.layer0_info.rot_angle = rotationType_convert(hwcLayer->transform);

            gsp_cfg_info.layer0_info.des_rect.st_x = dstRect->x;
            gsp_cfg_info.layer0_info.des_rect.st_y = dstRect->y;
            //if(gsp_cfg_info.layer0_info.img_format < GSP_SRC_FMT_YUV420_2P)//for zhongjun err
            if(0) {
                //we suppose RGB not scaling
                if(gsp_cfg_info.layer0_info.rot_angle&0x1) {
                    gsp_cfg_info.layer0_info.des_rect.rect_w = gsp_cfg_info.layer0_info.clip_rect.rect_h;
                    gsp_cfg_info.layer0_info.des_rect.rect_h = gsp_cfg_info.layer0_info.clip_rect.rect_w;
                } else {
                    gsp_cfg_info.layer0_info.des_rect.rect_w = gsp_cfg_info.layer0_info.clip_rect.rect_w;
                    gsp_cfg_info.layer0_info.des_rect.rect_h = gsp_cfg_info.layer0_info.clip_rect.rect_h;
                }
            } else {
                gsp_cfg_info.layer0_info.des_rect.rect_w = dstRect->w;
                gsp_cfg_info.layer0_info.des_rect.rect_h = dstRect->h;
            }

            if(gsp_cfg_info.layer0_info.img_format < GSP_SRC_FMT_YUV420_2P) { //for check zhongjun err
                if(((gsp_cfg_info.layer0_info.rot_angle&0x1) == 1)
                   &&(gsp_cfg_info.layer0_info.clip_rect.rect_h != dstRect->w
                      ||gsp_cfg_info.layer0_info.clip_rect.rect_w != dstRect->h)) {
                    ALOGI_IF(1,"util[%04d] yintianci warning: RGB scaling!clip[w%d,h%d] rot:%d dstRect[w%d,h%d]",__LINE__,
                             gsp_cfg_info.layer0_info.clip_rect.rect_w,gsp_cfg_info.layer0_info.clip_rect.rect_h,
                             gsp_cfg_info.layer0_info.rot_angle,
                             dstRect->w,dstRect->h);
                } else if(((gsp_cfg_info.layer0_info.rot_angle&0x1) == 0)
                          &&(gsp_cfg_info.layer0_info.clip_rect.rect_h != dstRect->h
                             ||gsp_cfg_info.layer0_info.clip_rect.rect_w != dstRect->w)) {
                    ALOGI_IF(1,"util[%04d] yintianci warning: RGB scaling!clip[w%d,h%d] rot:%d dstRect[w%d,h%d]",__LINE__,
                             gsp_cfg_info.layer0_info.clip_rect.rect_w,gsp_cfg_info.layer0_info.clip_rect.rect_h,
                             gsp_cfg_info.layer0_info.rot_angle,
                             dstRect->w,dstRect->h);
                }
            }

            ALOGI_IF((private_h->width != private_h->stride),"util[%04d] warning: imgLayer width %d, stride %d, not equal!",__LINE__, private_h->width, private_h->stride);
            //gsp_cfg_info.layer0_info.pitch = context->src_img.w;
            gsp_cfg_info.layer0_info.pitch = private_h->stride;
            gsp_cfg_info.layer0_info.layer_en = 1;

#if 0 //test code
            test_color(private_h, gsp_cfg_info.layer0_info.img_format);
#endif

        } else {
            ALOGE("util[%04d] err:layer buffer type is not supported!",__LINE__);
            return -1;
        }
    } else {
        ALOGI_IF(mDebugFlag,"util[%04d] layer == NULL",__LINE__);

        if(pgsp_cfg_info == NULL || pgsp_cfg_info->layer_des_info.pitch == 0) {
            //l2 be processed in GSP layer1, GSP layer0 be used as BG if it's nessary
        } else {
            //GSP layer0 config as former output, layer1 process l2
            ALOGI_IF(mDebugFlag,"util[%04d] use last output as imgLayer input. ",__LINE__);
            gsp_cfg_info.layer0_info.pitch = pgsp_cfg_info->layer_des_info.pitch;

            gsp_cfg_info.layer0_info.clip_rect.st_x = gsp_cfg_info.layer1_info.des_pos.pos_pt_x;
            gsp_cfg_info.layer0_info.clip_rect.st_y = gsp_cfg_info.layer1_info.des_pos.pos_pt_y;
            if(gsp_cfg_info.layer1_info.rot_angle & 0x1) { // 90 270 degree
                gsp_cfg_info.layer0_info.clip_rect.rect_w = gsp_cfg_info.layer1_info.clip_rect.rect_h;
                gsp_cfg_info.layer0_info.clip_rect.rect_h = gsp_cfg_info.layer1_info.clip_rect.rect_w;
            } else { // 0 180 degree
                gsp_cfg_info.layer0_info.clip_rect.rect_w = gsp_cfg_info.layer1_info.clip_rect.rect_w;
                gsp_cfg_info.layer0_info.clip_rect.rect_h = gsp_cfg_info.layer1_info.clip_rect.rect_h;
            }
            gsp_cfg_info.layer0_info.des_rect = gsp_cfg_info.layer0_info.clip_rect;

            gsp_cfg_info.layer0_info.src_addr = pgsp_cfg_info->layer_des_info.src_addr;
            gsp_cfg_info.layer0_info.mem_info = pgsp_cfg_info->layer_des_info.mem_info;
            if(pgsp_cfg_info->layer_des_info.img_format < GSP_DST_FMT_YUV422_2P) {
                gsp_cfg_info.layer0_info.img_format = (GSP_LAYER_SRC_DATA_FMT_E)pgsp_cfg_info->layer_des_info.img_format;
            } else {
                ALOGE("util[%04d] err:color format not supported!",__LINE__);
                return -1;
            }
            gsp_cfg_info.layer0_info.endian_mode = pgsp_cfg_info->layer_des_info.endian_mode;
            gsp_cfg_info.layer0_info.alpha = 0xff;
            gsp_cfg_info.layer0_info.pmargb_mod = 1;
            gsp_cfg_info.layer0_info.rot_angle = GSP_ROT_ANGLE_0;
            gsp_cfg_info.layer0_info.layer_en = 1;
        }
    }

    ALOGI_IF(mDebugFlag,"util[%04d] imgLayer info [f%d,pm:%d,x%d,y%d,w%d,h%d,p%d] r%d [x%d,y%d,w%d,h%d], planeAlpha: %d",__LINE__,
             gsp_cfg_info.layer0_info.img_format,
             gsp_cfg_info.layer0_info.pmargb_mod,
             gsp_cfg_info.layer0_info.clip_rect.st_x,
             gsp_cfg_info.layer0_info.clip_rect.st_y,
             gsp_cfg_info.layer0_info.clip_rect.rect_w,
             gsp_cfg_info.layer0_info.clip_rect.rect_h,
             gsp_cfg_info.layer0_info.pitch,
             gsp_cfg_info.layer0_info.rot_angle,
             gsp_cfg_info.layer0_info.des_rect.st_x,
             gsp_cfg_info.layer0_info.des_rect.st_y,
             gsp_cfg_info.layer0_info.des_rect.rect_w,
             gsp_cfg_info.layer0_info.des_rect.rect_h,
             gsp_cfg_info.layer0_info.alpha);
    return 0;
}


int SprdUtil::gsp_osd_layer_config(SprdHWLayer *layer, GSP_CONFIG_INFO_T &gsp_cfg_info)
{
    if(layer) {
        hwc_layer_1_t *hwcLayer = layer->getAndroidLayer();
        struct sprdRect *srcRect = layer->getSprdSRCRect();
        struct sprdRect *dstRect = layer->getSprdFBRect();
        if (hwcLayer == NULL || srcRect == NULL || dstRect == NULL) {
            ALOGE("util[%04d] Failed to get OSD SprdHWLayer parameters!",__LINE__);
            return -1;
        }

        struct private_handle_t *private_h = (struct private_handle_t *)(hwcLayer->handle);
        if(private_h == NULL) {
            ALOGE("util[%04d] err:private_h == NULL,return",__LINE__);
            return -1;
        }

        ALOGI_IF(mDebugFlag,"util[%04d] osdLayer src info [f:%d,pm:%d,x%d,y%d,w%d,h%d,p%d,s%d] r%d [x%d,y%d,w%d,h%d]",__LINE__,
                 private_h->format,
                 hwcLayer->blending,
                 srcRect->x, srcRect->y,
                 srcRect->w, srcRect->h,
                 private_h->width, private_h->height,
                 hwcLayer->transform,
                 dstRect->x, dstRect->y,
                 dstRect->w, dstRect->h);


        if(private_h->flags &&
           ((mGsp_cap.buf_type_support == GSP_ADDR_TYPE_IOVIRTUAL)
            ||((mGsp_cap.buf_type_support == GSP_ADDR_TYPE_PHYSICAL) && (private_h->flags & private_handle_t::PRIV_FLAGS_USES_PHY)))) {

            gsp_cfg_info.layer1_info.rot_angle = rotationType_convert(hwcLayer->transform);

            if( gsp_cfg_info.layer1_info.rot_angle&0x1) {
                if(srcRect->w != dstRect->h || srcRect->h != dstRect->w) {
                    ALOGE("util[%04d] OSD scaling not supported![%dx%d] [%dx%d]",__LINE__,
                          srcRect->w,srcRect->h,dstRect->w,dstRect->h);
                    return -1;
                }
            } else {
                if(srcRect->w != dstRect->w || srcRect->h != dstRect->h) {
                    ALOGE("util[%04d] OSD scaling not supported![%dx%d] [%dx%d]",__LINE__,
                          srcRect->w,srcRect->h,dstRect->w,dstRect->h);
                    return -1;
                }
            }

            //config OSD,use GSP L1
            gsp_cfg_info.layer1_info.img_format = formatType_convert(private_h->format);
            switch(private_h->format) {
                case HAL_PIXEL_FORMAT_RGBA_8888:
                case HAL_PIXEL_FORMAT_RGBX_8888:
                    gsp_cfg_info.layer1_info.endian_mode.y_word_endn = GSP_WORD_ENDN_1;
                    gsp_cfg_info.layer1_info.endian_mode.a_swap_mode = GSP_A_SWAP_RGBA;
                    break;
                case HAL_PIXEL_FORMAT_YCrCb_420_SP:
                    gsp_cfg_info.layer1_info.endian_mode.uv_word_endn = GSP_WORD_ENDN_2;
                    break;
                case HAL_PIXEL_FORMAT_RGB_565:
                    gsp_cfg_info.layer1_info.endian_mode.rgb_swap_mode = GSP_RGB_SWP_BGR;
                    break;
                default:
                    break;
            }

            //ALOGI_IF(mDebugFlag," gsp_iommu[%d] mapped L2 iommu addr:%08x,size:%08x",__LINE__,gsp_cfg_info.layer1_info.src_addr.addr_y,buffersize_layer2);

            gsp_cfg_info.layer1_info.mem_info.share_fd = private_h->share_fd;
            gsp_cfg_info.layer1_info.mem_info.uv_offset =
                gsp_cfg_info.layer1_info.mem_info.v_offset = 0;
            gsp_cfg_info.layer1_info.clip_rect.st_x = srcRect->x;
            gsp_cfg_info.layer1_info.clip_rect.st_y = srcRect->y;
            gsp_cfg_info.layer1_info.clip_rect.rect_w = srcRect->w;
            gsp_cfg_info.layer1_info.clip_rect.rect_h = srcRect->h;
            gsp_cfg_info.layer1_info.des_pos.pos_pt_x = dstRect->x;
            gsp_cfg_info.layer1_info.des_pos.pos_pt_y = dstRect->y;


            // dst region check
            if(((gsp_cfg_info.layer1_info.rot_angle&0x1)==0)
               && gsp_cfg_info.layer1_info.clip_rect.rect_h+gsp_cfg_info.layer1_info.des_pos.pos_pt_y > mFBInfo->fb_height) {
                ALOGE("util[%04d] yintianci err: osd dst region beyond dst boundary,clip_h:%d,dst_y:%d,fb_h:%d",__LINE__,
                      gsp_cfg_info.layer1_info.clip_rect.rect_h,
                      gsp_cfg_info.layer1_info.des_pos.pos_pt_y,
                      mFBInfo->fb_height);

                gsp_cfg_info.layer1_info.clip_rect.rect_h = mFBInfo->fb_height-gsp_cfg_info.layer1_info.des_pos.pos_pt_y;
            }
            if(((gsp_cfg_info.layer1_info.rot_angle&0x1)==1)
               && gsp_cfg_info.layer1_info.clip_rect.rect_w+gsp_cfg_info.layer1_info.des_pos.pos_pt_y > mFBInfo->fb_height) {
                ALOGE("util[%04d] yintianci err: osd dst region beyond dst boundary,clip_w:%d,dst_y:%d,fb_h:%d",__LINE__,
                      gsp_cfg_info.layer1_info.clip_rect.rect_w,
                      gsp_cfg_info.layer1_info.des_pos.pos_pt_y,
                      mFBInfo->fb_height);
                gsp_cfg_info.layer1_info.clip_rect.rect_w = mFBInfo->fb_height-gsp_cfg_info.layer1_info.des_pos.pos_pt_y;
            }


            gsp_cfg_info.layer1_info.alpha = hwcLayer->planeAlpha;
            //gsp_cfg_info.layer1_info.pmargb_mod = ((hwcLayer->blending&HWC_BLENDING_PREMULT) == HWC_BLENDING_PREMULT);
            if(hwcLayer->blending == HWC_BLENDING_PREMULT/*have already pre-multiply*/
               ||hwcLayer->blending == HWC_BLENDING_COVERAGE/*coverage the dst layer*/) {
                gsp_cfg_info.layer1_info.pmargb_mod = 1;
            } else {
                ALOGE("util[%04d] err:L1 blending flag:%x!",__LINE__,hwcLayer->blending);
            }

            ALOGI_IF((private_h->width != private_h->stride),"util[%04d] warning: osdLayer width %d, stride %d, not equal!",__LINE__, private_h->width, private_h->stride);
            //gsp_cfg_info.layer1_info.pitch = private_h->width;
            gsp_cfg_info.layer1_info.pitch = private_h->stride;

            //gsp_cfg_info.layer1_info.des_pos.pos_pt_x = gsp_cfg_info.layer1_info.des_pos.pos_pt_y = 0;
            gsp_cfg_info.layer1_info.layer_en = 1;

            ALOGI_IF(mDebugFlag,"util[%04d] osdLayer info [f%d,pm:%d,x%d,y%d,w%d,h%d,p%d] r%d [x%d,y%d], planeAlpha: %d",__LINE__,
                     gsp_cfg_info.layer1_info.img_format,
                     gsp_cfg_info.layer1_info.pmargb_mod,
                     gsp_cfg_info.layer1_info.clip_rect.st_x,
                     gsp_cfg_info.layer1_info.clip_rect.st_y,
                     gsp_cfg_info.layer1_info.clip_rect.rect_w,
                     gsp_cfg_info.layer1_info.clip_rect.rect_h,
                     gsp_cfg_info.layer1_info.pitch,
                     gsp_cfg_info.layer1_info.rot_angle,
                     gsp_cfg_info.layer1_info.des_pos.pos_pt_x,
                     gsp_cfg_info.layer1_info.des_pos.pos_pt_y,
                     gsp_cfg_info.layer1_info.alpha);
        } else {
            ALOGE("util[%04d] err:layer buffer type is not supported!",__LINE__);
            return -1;
        }
    } else {
        /*if((GSP_SRC_FMT_YUV420_2P <= gsp_cfg_info.layer0_info.img_format && gsp_cfg_info.layer0_info.img_format <= GSP_SRC_FMT_YUV422_2P)
            && gsp_cfg_info.layer0_info.layer_en == 1)*/
        if(gsp_cfg_info.layer0_info.layer_en == 1) {
            ALOGI_IF(mDebugFlag,"util[%04d] osdLayer is NULL, use pallet to clean the area imgLayer not covered. ",__LINE__);

            gsp_cfg_info.layer1_info.grey.r_val = 0;
            gsp_cfg_info.layer1_info.grey.g_val = 0;
            gsp_cfg_info.layer1_info.grey.b_val = 0;
            gsp_cfg_info.layer1_info.clip_rect.st_x = 0;
            gsp_cfg_info.layer1_info.clip_rect.st_y = 0;

            gsp_cfg_info.layer1_info.clip_rect.rect_w = mFBInfo->fb_width;
            gsp_cfg_info.layer1_info.clip_rect.rect_h = mFBInfo->fb_height;
            gsp_cfg_info.layer1_info.pitch = mFBInfo->fb_width;

            //the 3-plane addr should not be used by GSP
            gsp_cfg_info.layer1_info.src_addr.addr_y = gsp_cfg_info.layer0_info.src_addr.addr_y;
            gsp_cfg_info.layer1_info.src_addr.addr_uv = gsp_cfg_info.layer0_info.src_addr.addr_uv;
            gsp_cfg_info.layer1_info.src_addr.addr_v = gsp_cfg_info.layer0_info.src_addr.addr_v;

            gsp_cfg_info.layer1_info.pallet_en = 1;
            gsp_cfg_info.layer1_info.alpha = 0x0;

            gsp_cfg_info.layer1_info.rot_angle = GSP_ROT_ANGLE_0;
            gsp_cfg_info.layer1_info.des_pos.pos_pt_x = 0;
            gsp_cfg_info.layer1_info.des_pos.pos_pt_y = 0;
            gsp_cfg_info.layer1_info.layer_en = 1;
        } else {
            ALOGE("util[%04d] err:both GSP layer is invalid, return!",__LINE__);
            return -1;
        }
    }
    return 0;
}



int SprdUtil::gsp_dst_layer_config(GSP_CONFIG_INFO_T &gsp_cfg_info,
                                   private_handle_t* dst_buffer)
{

    if(dst_buffer == NULL) return -1;

    //config output
    //current_overlay_vaddr = (unsigned int)dst_buffer->base;
    gsp_cfg_info.layer_des_info.src_addr.addr_y =  (uint32_t)dst_buffer->phyaddr;
    gsp_cfg_info.layer_des_info.src_addr.addr_v =
        gsp_cfg_info.layer_des_info.src_addr.addr_uv =
            gsp_cfg_info.layer_des_info.src_addr.addr_y + mFBInfo->fb_width * mFBInfo->fb_height;
    gsp_cfg_info.layer_des_info.pitch = mFBInfo->fb_width;
    if(gsp_cfg_info.layer_des_info.src_addr.addr_y == 0) {
        ALOGE("util[%04d] des.y_addr==0x%08x buffersize_layerd==%d!",__LINE__,gsp_cfg_info.layer_des_info.src_addr.addr_y,dst_buffer->size);
        return -1;
    }
    ALOGI_IF(mDebugFlag,"util[%04d] des.y_addr:0x%08x, size:%d",__LINE__,gsp_cfg_info.layer_des_info.src_addr.addr_y,dst_buffer->size);

#if 1
    //when scaling need, all xywh should be even to make sure GSP won't busy hung up
    if((GSP_SRC_FMT_YUV420_2P<=gsp_cfg_info.layer0_info.img_format &&  gsp_cfg_info.layer0_info.img_format<=GSP_SRC_FMT_YUV422_2P)
       && ((((gsp_cfg_info.layer0_info.rot_angle&0x1) == 1)
            &&(gsp_cfg_info.layer0_info.clip_rect.rect_h != gsp_cfg_info.layer0_info.des_rect.rect_w
               ||gsp_cfg_info.layer0_info.clip_rect.rect_w != gsp_cfg_info.layer0_info.des_rect.rect_h))
           ||(((gsp_cfg_info.layer0_info.rot_angle&0x1) == 0)
              &&(gsp_cfg_info.layer0_info.clip_rect.rect_h != gsp_cfg_info.layer0_info.des_rect.rect_h
                 ||gsp_cfg_info.layer0_info.clip_rect.rect_w != gsp_cfg_info.layer0_info.des_rect.rect_w)))) {
        gsp_cfg_info.layer0_info.clip_rect.st_x &= 0xfffe;
        gsp_cfg_info.layer0_info.clip_rect.st_y &= 0xfffe;
        gsp_cfg_info.layer0_info.clip_rect.rect_w &= 0xfffe;
        gsp_cfg_info.layer0_info.clip_rect.rect_h &= 0xfffe;
        /*
        if(gsp_cfg_info.layer0_info.des_rect.st_y & 0x1) {
            gsp_cfg_info.layer0_info.des_rect.st_y += 1;
            gsp_cfg_info.layer0_info.des_rect.rect_h -= 1;
        }
        gsp_cfg_info.layer0_info.des_rect.st_x &= 0xfffe;
        gsp_cfg_info.layer0_info.des_rect.rect_w &= 0xfffe;
        gsp_cfg_info.layer0_info.des_rect.rect_h &= 0xfffe;
        */
    }
#endif

    //for YUV format data, GSP don't support odd width/height and x/y
    if((GSP_SRC_FMT_RGB565 < gsp_cfg_info.layer0_info.img_format)
       && (gsp_cfg_info.layer0_info.img_format < GSP_SRC_FMT_8BPP)
       && mGsp_cap.yuv_xywh_even==1) { //YUV format

        gsp_cfg_info.layer0_info.clip_rect.st_x &= 0xfffe;
        gsp_cfg_info.layer0_info.clip_rect.st_y &= 0xfffe;
        gsp_cfg_info.layer0_info.clip_rect.rect_w &= 0xfffe;
        gsp_cfg_info.layer0_info.clip_rect.rect_h &= 0xfffe;
#if 0
        if(gsp_cfg_info.layer0_info.des_rect.st_y & 0x1) {
            gsp_cfg_info.layer0_info.des_rect.st_y += 1;
            gsp_cfg_info.layer0_info.des_rect.rect_h -= 1;
        }
        gsp_cfg_info.layer0_info.des_rect.st_x &= 0xfffe;
        gsp_cfg_info.layer0_info.des_rect.rect_w &= 0xfffe;
        gsp_cfg_info.layer0_info.des_rect.rect_h &= 0xfffe;
#endif
    }

    if((GSP_SRC_FMT_RGB565 < gsp_cfg_info.layer1_info.img_format )
       && (gsp_cfg_info.layer1_info.img_format < GSP_SRC_FMT_8BPP)
       && mGsp_cap.yuv_xywh_even==1) { //YUV format
        gsp_cfg_info.layer1_info.clip_rect.st_x &= 0xfffe;
        gsp_cfg_info.layer1_info.clip_rect.st_y &= 0xfffe;
        gsp_cfg_info.layer1_info.clip_rect.rect_w &= 0xfffe;
        gsp_cfg_info.layer1_info.clip_rect.rect_h &= 0xfffe;
        /*
        gsp_cfg_info.layer1_info.des_pos.pos_pt_x &= 0xfffe;
        gsp_cfg_info.layer1_info.des_pos.pos_pt_y &= 0xfffe;
        */
    }

    if(mGsp_cap.buf_type_support == GSP_ADDR_TYPE_IOVIRTUAL) {
        gsp_cfg_info.misc_info.split_pages = 1;
    } else {
        gsp_cfg_info.misc_info.split_pages = 0;
    }

    if(gsp_cfg_info.layer_des_info.img_format == GSP_DST_FMT_ARGB888
       ||gsp_cfg_info.layer_des_info.img_format == GSP_DST_FMT_RGB888) {
        gsp_cfg_info.layer_des_info.endian_mode.y_word_endn = GSP_WORD_ENDN_1;
        gsp_cfg_info.layer_des_info.endian_mode.a_swap_mode = GSP_A_SWAP_RGBA;
    }
    if(gsp_cfg_info.layer1_info.pallet_en == 1) {
        gsp_cfg_info.layer0_info.pmargb_mod = 1;
    }
    gsp_cfg_info.layer_des_info.endian_mode.uv_word_endn = GSP_WORD_ENDN_0;

    ALOGI_IF(mDebugFlag,"util[%04d] dstLayer info [f%d,p%d] split%d",__LINE__,
             gsp_cfg_info.layer_des_info.img_format,
             gsp_cfg_info.layer_des_info.pitch,
             gsp_cfg_info.misc_info.split_pages);
    return 0;
}



/*
func:composerLayers
desc:if both layers are valid, blend them, but maybe the output image is not correct,
        because these area that are not covered by any layers maybe is random data.
        if only layer1 is valid, we blend it with background.
        if only layer2 is valid, we blend it with former output.
params:
        pgsp_cfg_info(in and out): if only layer2 is valid, we get the former output parameters from it,
                                              and after current task over, we assignment the current task config to it to pass back to caller function.

limits: layer1 and layer2 can be any color format, but if both of them need scaling, it is not supported
*/
int SprdUtil::composerLayers(SprdHWLayer *l1,
                             SprdHWLayer *l2,
                             GSP_CONFIG_INFO_T *pgsp_cfg_info,
                             private_handle_t* dst_buffer,
                             GSP_LAYER_DST_DATA_FMT_E dst_format)
{
    int32_t ret = 0;
    int64_t start_time = 0;
    int64_t end_time = 0;

    GSP_CONFIG_INFO_T gsp_cfg_info;
    if(mDebugFlag) {
        start_time = UtilGetSystemTime()/1000;
    }
    queryDebugFlag(&mDebugFlag);
    if(mGsp_cap.buf_type_support == GSP_ADDR_TYPE_INVALUE) {
        getGSPCapability(&mGsp_cap);
        if(mGsp_cap.buf_type_support == GSP_ADDR_TYPE_INVALUE) {
            ALOGE("util[%04d] ERR:can't get GSP address type!",__LINE__);
            return -1;
        }
    }


    //layer1 must valid, layer2 can be invalid
    if (dst_buffer == NULL || (l1 == NULL && l2 == NULL)) {
        ALOGE("util[%04d] ERR:The output buffer is NULL or src layer is NULL!", __LINE__);
        return -1;
    }

    if (mOutputFormat == -1) {
        mOutputFormat = GSP_DST_FMT_YUV420_2P;
        ALOGI_IF(mDebugFlag, "util[%04d] mOutputFormat is invalid, force output format to GSP_DST_FMT_YUV420_2P",__LINE__);
    }

    memset(&gsp_cfg_info,0,sizeof(gsp_cfg_info));

    //in botton to top order, for iterate scenario, we need l2 dst region, so we can get l1 clip region
    if(l2 != NULL && l1 == NULL && pgsp_cfg_info != NULL && pgsp_cfg_info->layer_des_info.pitch > 0) {
        ret = gsp_osd_layer_config(l2, gsp_cfg_info);
        if(ret) {
            ALOGE("util[%04d] gsp_osd_layer_config() return err!",__LINE__);
            return -1;
        }
    }
    ret = gsp_image_layer_config(l1,gsp_cfg_info, pgsp_cfg_info);
    if(ret) {
        ALOGE("util[%04d] gsp_image_layer_config() return err!",__LINE__);
        return -1;
    }

    // if layer1 config before, skip config it again
    if(gsp_cfg_info.layer1_info.pitch == 0) {
        ret = gsp_osd_layer_config(l2, gsp_cfg_info);
        if(ret) {
            ALOGE("util[%04d] gsp_osd_layer_config() return err!",__LINE__);
            return -1;
        }
    }

    if (gsp_cfg_info.layer0_info.layer_en == 1 || gsp_cfg_info.layer1_info.layer_en == 1) {
        if(dst_format != GSP_DST_FMT_MAX_NUM) {
            gsp_cfg_info.layer_des_info.img_format = dst_format;
        } else {
            if (l1 != NULL) {
                gsp_cfg_info.layer_des_info.img_format = mOutputFormat;
            } else if (l2 != NULL) {
#ifndef PRIMARYPLANE_USE_RGB565
                gsp_cfg_info.layer_des_info.img_format = mOutputFormat;
#else
                gsp_cfg_info.layer_des_info.img_format = GSP_DST_FMT_RGB565;
#endif
            }
        }

        ret = gsp_dst_layer_config(gsp_cfg_info, dst_buffer);
        if(ret) {
            ALOGE("util[%04d] gsp_dst_layer_config() return err!",__LINE__);
            return -1;
        }


        if(mGsp_cap.video_need_copy == 1) {
            if((mGsp_cap.buf_type_support == GSP_ADDR_TYPE_IOVIRTUAL
                && (gsp_cfg_info.layer0_info.layer_en == 1) && (gsp_cfg_info.layer0_info.img_format == GSP_SRC_FMT_YUV420_2P))
               /*&&(!(private_h1->flags & private_handle_t::PRIV_FLAGS_USES_PHY))
               &&((gsp_cfg_info.layer0_info.layer_en == 1 && gsp_cfg_info.layer0_info.rot_angle != GSP_ROT_ANGLE_0)
                   || (gsp_cfg_info.layer1_info.layer_en == 1 && gsp_cfg_info.layer1_info.rot_angle != GSP_ROT_ANGLE_0))*/) {
                ret = gsp_process_va_copy2_pa(&gsp_cfg_info);
                if(ret) {
                    ALOGE("util[%04d] gsp_process_va_copy2_pa() return err!",__LINE__);
                    return ret;
                }
            } else {
                ALOGI_IF(mDebugFlag,"util[%04d] don't need copy,GSPAddrType%d,layer0_en%d,layer0format%d",__LINE__,
                         mGsp_cap.buf_type_support,
                         gsp_cfg_info.layer0_info.layer_en,
                         gsp_cfg_info.layer0_info.img_format);
            }
        }

        if(mGsp_cap.scale_range_up == 256) {
            if(scaling_up_twice_check(gsp_cfg_info)) {
                ret = scalingup_twice(gsp_cfg_info, dst_buffer);
                if(ret) {
                    ALOGE("util[%04d] scalingup_twice() return err!",__LINE__);
                    return ret;
                }
            }
        }
		//ALOGI_IF(mDebugFlag,"util[%04d] ",__LINE__);

        gsp_cfg_info.misc_info.split_pages = 0;//set to 0, to make sure system won't crash
        if(mGsp_cap.buf_type_support == GSP_ADDR_TYPE_IOVIRTUAL) {
            if(mGsp_cap.video_need_copy == 0//don't need copying means black line issue be fixed, perfect chip
               ||1 == gsp_split_pages_check(gsp_cfg_info)) {
                gsp_cfg_info.misc_info.split_pages = 1;
            }
        }

        ALOGI_IF(mDebugFlag,"util[%04d] split%d",__LINE__,gsp_cfg_info.misc_info.split_pages);

        if(mGspDev) {
            ret = mGspDev->GSP_Proccess(&gsp_cfg_info);
            if(pgsp_cfg_info != NULL) {
                *pgsp_cfg_info=gsp_cfg_info;
            }
            if(0 == ret) {
                ALOGI_IF(mDebugFlag,"util[%04d] GSP_Proccess success",__LINE__);
            } else {
                ALOGE("util[%04d] GSP_Proccess failed!",__LINE__);
                mDebugFlag = 1;
                return ret;
            }
        }
    } else {
        ALOGE("util[%04d] err: both GSP layer are not well configured!",__LINE__);
        return -1;
    }
    if(mDebugFlag) {
        end_time = UtilGetSystemTime()/1000;
        ALOGI_IF(mDebugFlag,"util[%04d] single process: start:%lld, end:%lld,cost:%lld us !!\n",__LINE__,
                 start_time,end_time,(end_time-start_time));
    }
    return 0;
}



int SprdUtil::composerLayerList(SprdHWLayer **videoLayerList, int videoLayerCount,
                                SprdHWLayer **osdLayerList, int osdLayerCount,
                                private_handle_t* buffer1, private_handle_t* buffer2)
{
    GSP_LAYER_DST_DATA_FMT_E dst_format = GSP_DST_FMT_MAX_NUM;
    private_handle_t* dst_buffer = NULL;
    SprdHWLayer *LayerList[8]= {0};
    GSP_CONFIG_INFO_T gsp_cfg_info;
    int head_2layer_consumed = 0;
    int i = 0,ret = 0,layer_total = 0;
    int process_order = 0;// 0: from bottom to top; 1:from top to bottom

    int64_t start_time = 0;
    int64_t end_time = 0;


    layer_total = videoLayerCount+osdLayerCount;
    /*params check*/
    if((mGsp_cap.magic != CAPABILITY_MAGIC_NUMBER)
       ||(layer_total == 0)
       ||(videoLayerCount==0 && layer_total>mGsp_cap.max_layer_cnt)
       ||(videoLayerCount>0 && layer_total>mGsp_cap.max_layer_cnt_with_video)
       ||(videoLayerCount > mGsp_cap.max_videoLayer_cnt)
       ||((videoLayerCount>0 && osdLayerCount>0) && (mGsp_cap.blend_video_with_OSD==0))
       ||(videoLayerCount>0 && (videoLayerList==NULL || buffer1 == NULL))
       ||(osdLayerCount>0 && osdLayerList==NULL)
       ||(videoLayerCount == 0 && osdLayerCount > 0 && buffer2 == NULL)
       ||(buffer1 == NULL && buffer2 == NULL)) {
        //ALOGE("err: composerLayers() layer total count(%d+%d) > 2, GSP cant process,return!",videoLayerCount,osdLayerCount);
        ALOGE("util[%04d] composerLayerList params check err!",__LINE__);
        return -1;
    }


    if(mDebugFlag) {
        start_time = UtilGetSystemTime()/1000;
    }
    memset((void*)&gsp_cfg_info,0,sizeof(gsp_cfg_info));

    /*dst buffer select, video yuv buffer or osd ARGB888 buffer*/
    if (videoLayerCount > 0) {
        dst_buffer = buffer1;
    } else if (videoLayerCount == 0 && osdLayerCount > 0) {
        dst_buffer = buffer2;
    }

    // build LayerList array
    i=0;
    while(i<videoLayerCount) {
        LayerList[i]=videoLayerList[i];
        i++;
    }
    i=0;
    while(i<osdLayerCount) {
        LayerList[videoLayerCount+i]=osdLayerList[i];
        i++;
    }
    ALOGI_IF(mDebugFlag,"util[%04d] layerlist total:%d",__LINE__,layer_total);


    //check whether the OSD layers need scaling or not
    i=1;
    while(i<layer_total) {
        if(need_scaling_check(LayerList[i]) == 1) {
            process_order = 1;
            break;
        }
        i++;
    }
    if(mGsp_cap.OSD_scaling == 0 && process_order == 1) {
        ALOGE("util[%04d] err:find osd layer need scaling, but GSP not support this! ",__LINE__);
        goto exit;
    }

    if(process_order == 0) { // bottom to top
        if(layer_total == 1) {
            dst_format = mOutputFormat;
            ALOGI_IF(mDebugFlag,"util[%04d] process 0/%d layer, dst_format:%d",__LINE__,layer_total,dst_format);
            //layer0 process src image, layer1 as BG, because layer0 is capable of scaling
            ret = composerLayers(LayerList[0], NULL, &gsp_cfg_info, dst_buffer,dst_format);
            if(ret != 0) {
                ALOGE("util[%04d] composerLayers() return err!",__LINE__);
            }
            goto exit;
        } else {
            int full = full_screen_check(LayerList,layer_total,mFBInfo);
            int scale = need_scaling_check(LayerList[1]);

            //top layer don't need scaling && one of the two layers is dst full screen
            if((0==scale) && (1==full)) {
                //ALOGI_IF(mDebugFlag,"util[%04d] the first two-layers of %d-layers can blend in once",__LINE__,layer_total);
                ALOGI_IF(mDebugFlag,"util[%04d] process 1-2/%d layer, dst_format:%d",__LINE__,layer_total,dst_format);
                //blend first two layers,layer0 process bottom layer, layer1 process top layer
                if(layer_total == 2) {
                    dst_format = mOutputFormat;
                    ret = composerLayers(LayerList[0], LayerList[1], &gsp_cfg_info, dst_buffer,dst_format);
                    if(ret != 0) {
                        ALOGE("util[%04d] composerLayers() return err!",__LINE__);
                    }
                    goto exit;
                } else {
                    //dst_format = GSP_DST_FMT_ARGB888;
                    dst_format = mOutputFormat;
                    ret = composerLayers(LayerList[0], LayerList[1], &gsp_cfg_info, dst_buffer,dst_format);
                    if(ret != 0) {
                        ALOGE("util[%04d] composerLayers() return err!",__LINE__);

                        goto exit;
                    }
                    gsp_intermedia_dump(dst_buffer);
                }
                head_2layer_consumed=1;
            } else {
                ALOGI_IF(mDebugFlag,"util[%04d] the first two-layers of %d-layers can't blend  for scale:%d,full:%d",__LINE__,layer_total,scale,full);
            }
        }


        i=0;
        if(head_2layer_consumed == 1) {
            i+=2;
        }

        //layer[0] blend with BG => R0; R0 blend with layer[1] => R1; R1 blend with layer[2] => R2; ....
        while(i<layer_total) {
            if(i==layer_total-1) {
                dst_format = mOutputFormat;
            } else {
                //dst_format = GSP_DST_FMT_ARGB888;
                dst_format = mOutputFormat;
            }

            ALOGI_IF(mDebugFlag,"util[%04d] process %d/%d layer, dst_format:%d",__LINE__,i,layer_total,dst_format);

            if(i == 0) {
                //layer0 process src image, layer1 as BG
                ret = composerLayers(LayerList[i], NULL, &gsp_cfg_info, dst_buffer,dst_format);
                if(ret != 0) {
                    ALOGE("util[%04d] composerLayers() return err!layerlist[%d]",__LINE__,i);
                    goto exit;
                }
            } else {
                //layer0 process last output, layer1 process new src layer
                ret = composerLayers(NULL, LayerList[i], &gsp_cfg_info, dst_buffer,dst_format);
                if(ret != 0) {
                    ALOGE("util[%04d] composerLayers() return err!layerlist[%d]",__LINE__,i);
                    goto exit;
                }
            }
            if(i < layer_total-1) {
                gsp_intermedia_dump(dst_buffer);
            }
            i++;
        }
    } else { // top to bottom
        ALOGE("util[%04d] anti-order not supported",__LINE__);
    }

exit:
    if(mDebugFlag) {
        end_time = UtilGetSystemTime()/1000;
        ALOGI_IF(mDebugFlag,"util[%04d] %d-layers start:%lld, end:%06lld,cost:%lld us !!\n",__LINE__,
                 layer_total,start_time,end_time,(end_time-start_time));
    }
    return ret;
}


#endif

#ifdef TRANSFORM_USE_GPU
int SprdUtil::getTransformInfo(SprdHWLayer *l1, SprdHWLayer *l2,
                               private_handle_t* buffer1, private_handle_t* buffer2,
                               gpu_transform_info_t *transformInfo)
{
    memset(transformInfo , 0 , sizeof(gpu_transform_info_t));

    /*
     * Init parameters for Video transform
     * */
    if(l1 && buffer1) {
        hwc_layer_1_t *layer = l1->getAndroidLayer();
        struct sprdYUV *srcImg = l1->getSprdSRCYUV();
        struct sprdRect *srcRect = l1->getSprdSRCRect();
        struct sprdRect *FBRect = l1->getSprdFBRect();
        if (layer == NULL || srcImg == NULL ||
            srcRect == NULL || FBRect == NULL) {
            ALOGE("Failed to get Video SprdHWLayer parameters");
            return -1;
        }

        const native_handle_t *pNativeHandle = layer->handle;
        struct private_handle_t *private_h = (struct private_handle_t *)pNativeHandle;

        transformInfo->flag |= VIDEO_LAYER_EXIST;
        transformInfo->video.srcPhy = private_h->phyaddr;
        transformInfo->video.srcVirt =  private_h->base;
        transformInfo->video.srcFormat = private_h->format;
        transformInfo->video.transform = layer->transform;
        transformInfo->video.srcWidth = srcImg->w;
        transformInfo->video.srcHeight = srcImg->h;
        transformInfo->video.dstPhy = buffer1->phyaddr;
        transformInfo->video.dstVirt = (uint32_t)buffer1->base;
        transformInfo->video.dstFormat = HAL_PIXEL_FORMAT_RGBX_8888;
        transformInfo->video.dstWidth = FBRect->w;
        transformInfo->video.dstHeight = FBRect->h;

        transformInfo->video.tmp_phy_addr = 0;
        transformInfo->video.tmp_vir_addr = 0;
        transformInfo->video.trim_rect.x  = srcRect->x;
        transformInfo->video.trim_rect.y  = srcRect->y;
        transformInfo->video.trim_rect.w  = srcRect->w;
        transformInfo->video.trim_rect.h  = srcRect->h;
    }

    /*
     * Init parameters for OSD transform
     * */
    if(l2 && buffer2) {
        hwc_layer_1_t *layer = l2->getAndroidLayer();
        struct sprdYUV *srcImg = l2->getSprdSRCYUV();
        struct sprdRect *srcRect = l2->getSprdSRCRect();
        struct sprdRect *FBRect = l2->getSprdFBRect();
        if (layer == NULL || srcImg == NULL ||
            srcRect == NULL || FBRect == NULL) {
            ALOGE("Failed to get OSD SprdHWLayer parameters");
            return -1;
        }

        const native_handle_t *pNativeHandle = layer->handle;
        struct private_handle_t *private_h = (struct private_handle_t *)pNativeHandle;

        transformInfo->flag |= OSD_LAYER_EXIST;
        transformInfo->osd.srcPhy = private_h->phyaddr;
        transformInfo->osd.srcVirt = private_h->base;
        transformInfo->osd.srcFormat = HAL_PIXEL_FORMAT_RGBA_8888;
        transformInfo->osd.transform = layer->transform;
        transformInfo->osd.srcWidth = private_h->width;
        transformInfo->osd.srcHeight = private_h->height;
        transformInfo->osd.dstPhy = buffer2->phyaddr;
        transformInfo->osd.dstVirt = (uint32_t)buffer2->base;
        transformInfo->osd.dstFormat = HAL_PIXEL_FORMAT_RGBA_8888;
        transformInfo->osd.dstWidth = FBRect->w;
        transformInfo->osd.dstHeight = FBRect->h;
        transformInfo->osd.tmp_phy_addr = 0;
        transformInfo->osd.tmp_vir_addr = 0;
        transformInfo->osd.trim_rect.x  = 0;
        transformInfo->osd.trim_rect.y  = 0;
        transformInfo->osd.trim_rect.w  = private_h->width; // osd overlay must be full screen
        transformInfo->osd.trim_rect.h  = private_h->height; // osd overlay must be full screen
    }

    return 0;
}
#endif
