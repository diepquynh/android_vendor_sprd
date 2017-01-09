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
 ** 16/08/2013    Hardware Composer   Add a new feature to Harware composer,  *
 **                                   verlayComposer use GPU to do the        *
 **                                   Hardware layer blending on Overlay      *
 **                                   buffer, and then post the OVerlay       *
 **                                   buffer to Display                       *
 ******************************************************************************
 ** Author:         zhongjun.chen@spreadtrum.com                              *
 *****************************************************************************/


#include "Utility.h"

namespace android {

void getSizeStride(uint32_t width, uint32_t height, uint32_t format, uint32_t &size, uint32_t &stride)
{
    switch (format)
    {
    case HAL_PIXEL_FORMAT_YCbCr_420_SP:
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
    case HAL_PIXEL_FORMAT_YV12:
        stride = ALIGN(width , 16);
        size = height * (stride + ALIGN(stride/2, 16));
        size = ALIGN(size, 4096);
        break;
    default:
        {
            int bpp = 0;
            switch (format)
            {
            case HAL_PIXEL_FORMAT_RGBA_8888:
            case HAL_PIXEL_FORMAT_RGBX_8888:
            case HAL_PIXEL_FORMAT_BGRA_8888:
                bpp = 4;
                break;
            case HAL_PIXEL_FORMAT_RGB_888:
                bpp = 3;
                break;
            case HAL_PIXEL_FORMAT_RGB_565:
#if 0
            case HAL_PIXEL_FORMAT_RGBA_5551:
            case HAL_PIXEL_FORMAT_RGBA_4444:
#endif
                bpp = 2;
                break;
            default:
                return;
            }
            uint32_t bpr = ALIGN(width*bpp, 8);
            size = bpr * height;
            stride = bpr / bpp;
            size = ALIGN(size, 4096);
        }
    }
}


}
