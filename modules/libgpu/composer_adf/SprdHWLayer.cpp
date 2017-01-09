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
 ** File: SprdHWLayer.cpp             DESCRIPTION                             *
 **                                   Mainly responsible for filtering HWLayer*
 **                                   list, find layers that meet OverlayPlane*
 **                                   and PrimaryPlane specifications and then*
 **                                   mark them as HWC_OVERLAY.               *
 ******************************************************************************
 ******************************************************************************
 ** Author:         zhongjun.chen@spreadtrum.com                              *
 *****************************************************************************/

#include "SprdHWLayer.h"

using namespace android;

/*
 *  SprdHWLayer
 *  constract from a hwc_layer_1_t that composited in SF and can be directly show.
 * */
SprdHWLayer:: SprdHWLayer(hwc_layer_1_t *androidLayer, int format)
    : mInit(false),
      mAndroidLayer(androidLayer),
      mLayerType(LAYER_INVALIDE),
      mPrivateH(NULL),
      mFormat(format),
      mLayerIndex(-1),
      mSprdLayerIndex(-1),
      mAccelerator(-1),
      mProtectedFlag(false),
      mPlaneAlpha(255),
      mBlending(HWC_BLENDING_NONE),
      mTransform(0x0),
      mAcquireFenceFd(-1),
      mDebugFlag(0)
{
      if (checkRGBLayerFormat())
      {
          setLayerType(LAYER_OSD);
      }
      else if (checkYUVLayerFormat())
      {
          setLayerType(LAYER_OVERLAY);
      }
}

/*
 *  SprdHWLayer
 *  constract from a overlay buffer that used to send to dispc.
 * */
SprdHWLayer:: SprdHWLayer(struct private_handle_t *handle, int format, int32_t planeAlpha,
                          int32_t blending, int32_t transform, int32_t fenceFd)
    : mInit(false),
      mAndroidLayer(NULL),
      mLayerType(LAYER_INVALIDE),
      mPrivateH(handle),
      mFormat(format),
      mLayerIndex(-1),
      mSprdLayerIndex(-1),
      mAccelerator(-1),
      mProtectedFlag(false),
      mPlaneAlpha(planeAlpha),
      mBlending(blending),
      mTransform(transform),
      mAcquireFenceFd(fenceFd),
      mDebugFlag(0)
{
    if (handle)
    {

        if (mFormat == HAL_PIXEL_FORMAT_RGBA_8888 ||
            mFormat == HAL_PIXEL_FORMAT_RGBX_8888 ||
            mFormat == HAL_PIXEL_FORMAT_RGB_565)
        {
            setLayerType(LAYER_OSD);
        }
        else if (mFormat == HAL_PIXEL_FORMAT_YCbCr_420_SP ||
                 mFormat == HAL_PIXEL_FORMAT_YCrCb_420_SP ||
                 mFormat == HAL_PIXEL_FORMAT_YV12)
       {
           setLayerType(LAYER_OVERLAY);
       }
        mInit = true;
    }
}

/*
 *  checkRGBLayerFormat
 *  if it's rgb format,init SprdHWLayer from hwc_layer_1_t.
 * */
bool SprdHWLayer:: checkRGBLayerFormat()
{
    hwc_layer_1_t *layer = mAndroidLayer;
    if (layer == NULL)
    {
        return false;
    }

    const native_handle_t *pNativeHandle = layer->handle;
    struct private_handle_t *privateH = (struct private_handle_t *)pNativeHandle;

    if (pNativeHandle == NULL || privateH == NULL)
    {
        return false;
    }

    if ((privateH->format != HAL_PIXEL_FORMAT_RGBA_8888) &&
        (privateH->format != HAL_PIXEL_FORMAT_RGBX_8888) &&
        (privateH->format != HAL_PIXEL_FORMAT_RGB_565))
    {
        return false;
    }

    if (!mInit)
    {
        mPrivateH = privateH;
        setPlaneAlpha(layer->planeAlpha);
        setBlending(layer->blending);
        setTransform(layer->transform);
        setAcquireFenceFd(layer->acquireFenceFd);

        mInit = true;
    }

    return true;
}

/*
 *  checkYUVLayerFormat
 *  if it's yuv format,init SprdHWLayer from hwc_layer_1_t.
 * */
bool SprdHWLayer:: checkYUVLayerFormat()
{
    hwc_layer_1_t *layer = mAndroidLayer;
    if (layer == NULL)
    {
        return false;
    }

    const native_handle_t *pNativeHandle = layer->handle;
    struct private_handle_t *privateH = (struct private_handle_t *)pNativeHandle;

    if (pNativeHandle == NULL || privateH == NULL)
    {
        return false;
    }

    if ((privateH->format != HAL_PIXEL_FORMAT_YCbCr_420_SP) &&
        (privateH->format != HAL_PIXEL_FORMAT_YCrCb_420_SP) &&
        (privateH->format != HAL_PIXEL_FORMAT_YV12))
    {
        return false;
    }

    if (!mInit)
    {
        mPrivateH = privateH;
        setPlaneAlpha(layer->planeAlpha);
        setBlending(layer->blending);
        setTransform(layer->transform);
        setAcquireFenceFd(layer->acquireFenceFd);

        mInit = true;
    }

    return true;
}

