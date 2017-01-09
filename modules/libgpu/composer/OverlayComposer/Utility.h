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


#ifndef _PUBLIC_RES_H_
#define _PUBLIC_RES_H_

#include <stdint.h>
#include <errno.h>
#include <cutils/log.h>

#include "gralloc_priv.h"


namespace android {


struct LayerRect {
    uint32_t left;
    uint32_t top;
    uint32_t right;
    uint32_t bottom;
};

#define ALIGN(value, base) (((value) + ((base) - 1)) & ~((base) - 1))

extern void getSizeStride(uint32_t width, uint32_t height, uint32_t format, uint32_t &size, uint32_t &stride);

}

#endif
