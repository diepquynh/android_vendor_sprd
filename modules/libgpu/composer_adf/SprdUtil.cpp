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


//#include "MemoryHeapIon.h"
#include "SprdUtil.h"

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


	SprdUtil::SprdUtil()
        : mFBInfo(NULL),
#ifdef TRANSFORM_USE_DCAM
          tmpDCAMBuffer(NULL),
          mOSDTransform(NULL),
#endif
#ifdef PROCESS_VIDEO_USE_GSP
          mGspDev(NULL),
#endif
          mInitFlag(0),
          mDebugFlag(0)
    {
#ifdef PROCESS_VIDEO_USE_GSP
		openGSPDevice();
#endif
    }
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
/*return value
 * 0: gsp device exits
 * -1: no gsp device*/
int SprdUtil::probeGSPDevice()
{
	int ret = -1;
	if (mGspDev)
		ret = 0;
	else
		ret = -1;
	return ret;
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
            ALOGE("hwcomposer open GSP hal failed! ");
            return -1;
        }
		//mGspDev->GSP_SetFBInfo(mFBInfo->fb_width, mFBInfo->fb_height);
    } else {
        ALOGE("hwcomposer can't find GSP hal ! ");
        return -1;
    }

    return 0;
}

int SprdUtil:: Prepare(SprdHWLayer **LayerList, int LayerCount, bool& Support)
{
    queryDebugFlag(&mDebugFlag);

    if(mGspDev && mGspDev->GSP_Prepare) {
        return mGspDev->GSP_Prepare(LayerList, LayerCount, Support);
    } else {
        Support = false;
        return 0;
    }
}

int SprdUtil::composeLayerList(SprdUtilSource *Source, SprdUtilTarget *Target)
{
	if (Source == NULL || Target == NULL) {
		ALOGE("Source and Target is NUll");
		return -1;
	}
    Source->releaseFenceFd = -1;
    Target->acquireFenceFd = -1;

    if(mGspDev && mGspDev->GSP_Set) {
        return mGspDev->GSP_Set(Source, Target);
    } else {
        return -1;
    }
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
