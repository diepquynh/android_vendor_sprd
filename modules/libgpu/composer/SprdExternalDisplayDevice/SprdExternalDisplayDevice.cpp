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
 ** File:SprdExternalDisplayDevice.cpp DESCRIPTION                            *
 **                                   Manager External Display device.        *
 ******************************************************************************
 ******************************************************************************
 ** Author:         zhongjun.chen@spreadtrum.com                              *
 *****************************************************************************/

#include "SprdExternalDisplayDevice.h"

using namespace android;

SprdExternalDisplayDevice:: SprdExternalDisplayDevice()
    : mDebugFlag(0),
      mDumpFlag(0)
{

}

SprdExternalDisplayDevice:: ~SprdExternalDisplayDevice()
{

}

int SprdExternalDisplayDevice:: getDisplayAttributes(DisplayAttributes *dpyAttributes)
{
    int index = 0;
    float refreshRate = 60.0;

    if (dpyAttributes == NULL)
    {
        ALOGE("Input parameter is NULL");
        return -1;
    }

    index = dpyAttributes->configsIndex;
    if (index < 0)
    {
        ALOGE("SprdExternalDisplayDevice:: getDisplayAttributes invalid index");
        return -1;
    }

    dpyAttributes->sets[index].vsync_period = 0;
    dpyAttributes->sets[index].xres = 0;
    dpyAttributes->sets[index].yres = 0;
    dpyAttributes->sets[index].stride = 0;
    dpyAttributes->sets[index].xdpi = 0;
    dpyAttributes->sets[index].ydpi = 0;
    dpyAttributes->connected = false;

    return 0;
}

int SprdExternalDisplayDevice:: ActiveConfig(DisplayAttributes *dpyAttributes)
{

    return 0;
}

int SprdExternalDisplayDevice:: setPowerMode(int mode)
{
    int ret = -1;

    switch(mode)
    {
        case POWER_MODE_NORMAL:
            /*
             *  Turn on the display (if it was previously off),
             *  and take it out of low power mode.
             * */

             break;
        case POWER_MODE_DOZE:
            /*
             *  Turn on the display (if it was previously off),
             *  and put the display in a low power mode.
             * */

             break;
        case POWER_MODE_OFF:
            /*
             *  Turn the display off.
             * */

             break;
        default:
            return 0;
    }

    return 0;
}

int SprdExternalDisplayDevice:: setCursorPositionAsync(int x_pos, int y_pos)
{

    return 0;
}

int SprdExternalDisplayDevice:: prepare(hwc_display_contents_1_t *list, unsigned int accelerator)
{
    queryDebugFlag(&mDebugFlag);

    if (list == NULL)
    {
        ALOGI_IF(mDebugFlag, "commit: External Display Device maybe closed");
        return 0;
    }

    return 0;
}

int SprdExternalDisplayDevice:: commit(hwc_display_contents_1_t *list)
{
    hwc_layer_1_t *FBTargetLayer = NULL;

    queryDebugFlag(&mDebugFlag);

    if (list == NULL)
    {
        ALOGI_IF(mDebugFlag, "commit: External Display Device maybe closed");
        return 0;
    }

    waitAcquireFence(list);

    FBTargetLayer = &(list->hwLayers[list->numHwLayers - 1]);
    if (FBTargetLayer == NULL)
    {
        ALOGE("FBTargetLayer is NULL");
        return -1;
    }

    const native_handle_t *pNativeHandle = FBTargetLayer->handle;
    struct private_handle_t *privateH = (struct private_handle_t *)pNativeHandle;

    ALOGI_IF(mDebugFlag, "Start Displaying ExternalDisplay FramebufferTarget layer");

    if (FBTargetLayer->acquireFenceFd >= 0)
    {
        String8 name("HWCFBTExternal::Post");

        FenceWaitForever(name, FBTargetLayer->acquireFenceFd);

        if (FBTargetLayer->acquireFenceFd >= 0)
        {
            close(FBTargetLayer->acquireFenceFd);
            FBTargetLayer->acquireFenceFd = -1;
        }
    }

    HWCBufferSyncBuild(list, DISPLAY_EXTERNAL);

    closeAcquireFDs(list);

    return 0;
}
