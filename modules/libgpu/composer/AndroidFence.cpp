
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
 ** File: AndroidFence.h              DESCRIPTION                             *
 **                                   Handle Android Framework fence          *
 ******************************************************************************
 ******************************************************************************
 ** Author:         zhongjun.chen@spreadtrum.com                              *
 *****************************************************************************/

#include <sys/mman.h>
#include <sys/types.h>
#include <hardware/hardware.h>
#include <linux/ion.h>
#include "ion_sprd.h"
#include "AndroidFence.h"
#include <cutils/log.h>
#include "SprdDisplayDevice.h"
#include "SprdTrace.h"

using namespace android;

#define ION_DEVICE "/dev/ion"

static int ion_device_fd = -1;

enum {
    TIMEOUT_NEVER = -1
};

struct HWC_fence_data {
    int release_fence_fd;
    int retired_fence_fd;
};

int sprd_fence_build(enum SPRD_DEVICE_SYNC_TYPE type, struct HWC_fence_data *HWCData)
{
    HWC_TRACE_CALL;
    if (ion_device_fd < 0)
    {
        ALOGE("get ion device failed");
        return -1;
    }

    if (HWCData == NULL)
    {
        ALOGE("sprd_fence_build input para is NULL");
        return -1;
    }

    struct ion_custom_data  custom_data;
    struct ion_fence_data data;

    data.device_type = type;
    data.life_value = 1;
    data.release_fence_fd = -1;
    data.retired_fence_fd = -1;

    custom_data.cmd = ION_SPRD_CUSTOM_FENCE_CREATE;
    custom_data.arg = (unsigned long)&data;

    int ret = ioctl(ion_device_fd, ION_IOC_CUSTOM, &custom_data);
    if (ret < 0)
    {
        ALOGE("sprd_fence_create failed");
        return -1;
    }

    if (data.release_fence_fd < 0 || data.retired_fence_fd < 0)
    {
        ALOGE("sprd_fence_build return data error");
        return -1;
    }

    HWCData->release_fence_fd = data.release_fence_fd;
    HWCData->retired_fence_fd = data.retired_fence_fd;

    return 0;;
}

int sprd_fence_signal(enum SPRD_DEVICE_SYNC_TYPE type)
{
    HWC_TRACE_CALL;
    if (ion_device_fd < 0)
    {
        ALOGE("get ion device failed");
        return -1;
    }

    struct ion_custom_data  custom_data;
    struct ion_fence_data data;

    memset(&data, 0, sizeof(struct ion_fence_data));
    data.device_type = type;

    custom_data.cmd = ION_SPRD_CUSTOM_FENCE_SIGNAL;
    custom_data.arg = (unsigned long)&data;

    int ret = ioctl(ion_device_fd, ION_IOC_CUSTOM, &custom_data);
    if (ret < 0)
    {
        ALOGE("sprd_fence_signal failed");
        return -1;
    }

    return 0;
}

int openSprdFence()
{
    ion_device_fd = open(ION_DEVICE, O_RDWR);

    if (ion_device_fd < 0)
    {
        ALOGE("open ION_DEVICE failed");
        return -1;
    }

    return 0;
}

void closeSprdFence()
{
    if (ion_device_fd >= 0)
    {
        close(ion_device_fd);
    }
}

void closeAcquireFDs(hwc_display_contents_1_t *list)
{
    if (list)
    {
        for(unsigned int i = 0; i < list->numHwLayers; i++)
        {
            hwc_layer_1_t *l = &(list->hwLayers[i]);

            if (l->compositionType == HWC_FRAMEBUFFER)
            {
                continue;
            }

            if (l->acquireFenceFd >= 0)
            {
                close(l->acquireFenceFd);
                l->acquireFenceFd = -1;
            }
        }
    }
}

int FenceWaitForever(const String8& name, int fenceFd)
{
    HWC_TRACE_CALL;
    if (fenceFd < 0)
    {
        return 0;
    }

    unsigned int warningTimeout = 3000;

    int err = sync_wait(fenceFd, warningTimeout);
    if (err < 0)
    {
        ALOGE("Fence: %s FD: %d didn't signal in %u ms", name.string(), fenceFd, warningTimeout);

        err = sync_wait(fenceFd, 6000);
        if (err < 0)
        {
            ALOGE("Fence: %s FD: %d didn't signal in 6000 ms, app do not finish the rendering work",
                  name.string(), fenceFd);
        }
    }

    return err;
}

int waitAcquireFence(hwc_display_contents_1_t *list)
{
    HWC_TRACE_CALL;

    int ret = -1;

    if (list)
    {
        for(unsigned int i = 0; i < list->numHwLayers; i++)
        {
            hwc_layer_1_t *l = &(list->hwLayers[i]);

            if (l->compositionType != HWC_OVERLAY)
            {
                continue;
            }

            if (l->acquireFenceFd >= 0)
            {
                String8 name;

                name.appendFormat("acquireFence%d", i);

                ret = FenceWaitForever(name, l->acquireFenceFd);
            }
        }
    }

    return ret;
}

int HWCBufferSyncBuild(hwc_display_contents_1_t *list, int display)
{
    HWC_TRACE_CALL;

    static int releaseFenceFd = -1;
    enum SPRD_DEVICE_SYNC_TYPE device_type = SPRD_DEVICE_PRIMARY_SYNC;
    struct HWC_fence_data fenceData;

    if (display == DISPLAY_VIRTUAL)
    {
        device_type = SPRD_DEVICE_VIRTUAL_SYNC;
    }

    if (releaseFenceFd >= 0)
    {
        int ret = -1;

        /*
         *  Display do not need previous buffer any more.
         *  Just release the previous buffer release fence.
         * */
        ret = sprd_fence_signal(device_type);

        if (ret < 0)
        {
            ALOGE("sprd_fence_signal name");
            return -1;
        }

        close(releaseFenceFd);
        releaseFenceFd = -1;
    }

    if (sprd_fence_build(device_type, &fenceData) < 0)
    {
        ALOGE("HWCBufferSyncBuild create fence fd failed");
        return -1;
    }

    releaseFenceFd = dup(fenceData.release_fence_fd);

    /*
     *  Fill fence info for SurfaceFlinger
     * */
    if (list && fenceData.release_fence_fd >= 0)
    {
        for(unsigned int i = 0; i < list->numHwLayers; i++)
        {
            hwc_layer_1_t *l = &(list->hwLayers[i]);

            if (l->compositionType == HWC_FRAMEBUFFER)
            {
                continue;
            }

            if (l->releaseFenceFd < 0)
            {
                l->releaseFenceFd = dup(fenceData.release_fence_fd);
            }
        }
    }

    if (fenceData.release_fence_fd >= 0)
    {
        close(fenceData.release_fence_fd);
    }

    if (list->retireFenceFd < 0)
    {
        list->retireFenceFd = fenceData.retired_fence_fd;
    }

    return 0;
}

/*
 *  Interface for Virtual Display
 * */
static int releaseFenceFdForVirtualDisplay = -1;
int HWCBufferSyncBuildForVirtualDisplay(hwc_display_contents_1_t *list)
{
    HWC_TRACE_CALL;

    enum SPRD_DEVICE_SYNC_TYPE device_type;
    struct HWC_fence_data fenceData;

    device_type = SPRD_DEVICE_VIRTUAL_SYNC;

    if (sprd_fence_build(device_type, &fenceData) < 0)
    {
        ALOGE("HWCBufferSyncBuild create fence fd failed");
        return -1;
    }

    releaseFenceFdForVirtualDisplay = dup(fenceData.release_fence_fd);

    /*
     *  Fill fence info for SurfaceFlinger
     * */
    if (list && fenceData.release_fence_fd >= 0)
    {
        for(unsigned int i = 0; i < list->numHwLayers; i++)
        {
            hwc_layer_1_t *l = &(list->hwLayers[i]);

            if (l->compositionType == HWC_FRAMEBUFFER)
            {
                continue;
            }

            if (l->releaseFenceFd < 0)
            {
                l->releaseFenceFd = dup(fenceData.release_fence_fd);
            }
        }
    }

    if (fenceData.release_fence_fd >= 0)
    {
        close(fenceData.release_fence_fd);
        fenceData.release_fence_fd = -1;
    }

    list->retireFenceFd = fenceData.retired_fence_fd;

    return 0;
}

int HWCBufferSyncReleaseForVirtualDisplay(hwc_display_contents_1_t *list)
{
    HWC_TRACE_CALL;

    if (releaseFenceFdForVirtualDisplay < 0)
    {
        return 0;
    }

    int ret = -1;
    enum SPRD_DEVICE_SYNC_TYPE device_type = SPRD_DEVICE_VIRTUAL_SYNC;

    ret = sprd_fence_signal(device_type);
    close(releaseFenceFdForVirtualDisplay);
    releaseFenceFdForVirtualDisplay = -1;
    if (ret < 0)
    {
        ALOGE("sprd_fence_signal name");
        return -1;
    }

    return 0;
}
