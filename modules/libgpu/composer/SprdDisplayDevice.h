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
 ** File: SprdDisplayDevice.h         DESCRIPTION                             *
 **                                   Define some display device structures.  *
 ******************************************************************************
 ******************************************************************************
 ** Author:         zhongjun.chen@spreadtrum.com                              *
 *****************************************************************************/

#ifndef _SPRD_DISPLAY_DEVICE_H_
#define _SPRD_DISPLAY_DEVICE_H_

#include <hardware/hwcomposer.h>

/*
 *  We follow SurfaceFlinger
 * */
#define MAX_NUM_CONFIGS 128

enum DisplayType {
    DISPLAY_ID_INVALIDE = -1,
    DISPLAY_PRIMARY = HWC_DISPLAY_PRIMARY,
    DISPLAY_EXTERNAL = HWC_DISPLAY_EXTERNAL,
    DISPLAY_VIRTUAL = HWC_DISPLAY_VIRTUAL,
    NUM_BUILDIN_DISPLAY_TYPES = HWC_NUM_PHYSICAL_DISPLAY_TYPES,
};

enum DisplayPowerMode {
    POWER_MODE_NORMAL = HWC_POWER_MODE_NORMAL,
    POWER_MODE_DOZE = HWC_POWER_MODE_DOZE,
    POWER_MODE_OFF = HWC_POWER_MODE_OFF,
#ifdef __LP64__
    POWER_MODE_DOZE_SUSPEND = HWC_POWER_MODE_DOZE_SUSPEND,
#endif
};

#define MAX_DISPLAYS HWC_NUM_DISPLAY_TYPES

typedef struct _DisplayAttributesSet
{
    uint32_t vsync_period; //nanos
    uint32_t xres;
    uint32_t yres;
    uint32_t stride;
    float xdpi;
    float ydpi;
} AttributesSet;

typedef struct _DisplayAttributes {
    AttributesSet sets[MAX_NUM_CONFIGS];
    uint32_t configsIndex; // config index
    bool connected;
    unsigned int AcceleratorMode;
} DisplayAttributes;

#endif
