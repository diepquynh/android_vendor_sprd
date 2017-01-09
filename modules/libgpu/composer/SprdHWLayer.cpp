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

    return true;
}

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

    return true;
}

