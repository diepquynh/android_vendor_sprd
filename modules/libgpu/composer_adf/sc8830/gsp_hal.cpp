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
 ** 28/02/2015    GSP HAL/GSP class   Used to process HWC accelerate request. *
 **                                   bind with  GSP hw.                      *
 ******************************************************************************
 ** File: gsp_hal.cpp                 DESCRIPTION                             *
 **                                   GSP class define                        *
 ******************************************************************************
 ** Author:         tianci.yin@spreadtrum.com                                 *
 *****************************************************************************/

#include "gsp_hal.h"

gsp_hal_context_t *ctx = NULL;


/*
func:GSP_GetCapability
desc:get GSP capability , such as buffer addr type, layer count..
return:
*/


static int32_t GSPHAL_getCapability(void *pCap, uint32_t size)
{
    int32_t ret = 0;
    int32_t gsp_fd = -1;

    if (ctx == NULL || ctx->base == NULL) {
        ALOGE("%s[%d] ctx is NULL! return.",__func__,__LINE__);
        return GSP_HAL_PARAM_ERR;
    }
    if(pCap == NULL || size == 0) {
        ALOGE("%s[%d]: pGsp_cap is null! return.\n",__func__,__LINE__);
        return GSP_HAL_PARAM_ERR;
    }

    ret = ctx->base->getCapability(pCap, size);
    if(ret) {
        ALOGE("%s[%d]: get capability err! return.\n",__func__,__LINE__);
        return -1;
    }

    return 0;
}

/** if GSP can process this case, return true, else return false */
static int32_t GSPHAL_prepare(SprdHWLayer **layerList, int32_t layerCnt, bool &support)
{
    if (ctx == NULL || ctx->base == NULL) {
        ALOGE("%s[%d] ctx is NULL",__func__,__LINE__);
        return -1;
    }

    return ctx->base->prepare(layerList, layerCnt, support);
}



/** set frame buffer w & h */
static int32_t GSPHAL_setFBInfo(uint32_t w,uint32_t h)
{
    if (ctx == NULL || ctx->base == NULL) {
        ALOGE("%s[%d] ctx is NULL",__func__,__LINE__);
        return -1;
    }

    ctx->base->setFBInfo(w,h);
    ctx->base->init();
    return 0;
}

static int32_t GSPHAL_setOutFormat(uint32_t format)
{
    if (ctx == NULL || ctx->base == NULL) {
        ALOGE("%s[%d] ctx is NULL",__func__,__LINE__);
        return -1;
    }
    ctx->base->updateOutputFormat(format);
    return 0;
}


/** if GSP can process this case, return true, else return false */
static int32_t GSPHAL_set(struct _SprdUtilSource *Source, struct _SprdUtilTarget *Target)
{
    if (ctx == NULL || ctx->base == NULL) {
        ALOGE("%s[%d] ctx is NULL",__func__,__LINE__);
        return -1;
    }

    return ctx->base->composeLayerList(Source, Target);
}

/*
func:GSPHAL_process
desc:all the GSP function can be complete in this function, such as CFC,scaling,blend,rotation and mirror,clipping.
note:1 the source and destination image buffer should be physical-coherent memory space,
       the caller should ensure the in and out buffer size is large enough, or the GSP will access cross the buffer border,
       these two will rise unexpected exception.
     2 this function will be block until GSP process over.
return: 0 success, other err
*/
static int32_t GSPHAL_process(void *pgsp_cfg_info)//(GSP_CONFIG_INFO_T *pgsp_cfg_info)
{
    int32_t ret = 0;
    int32_t gsp_fd = -1;

    if (ctx == NULL || ctx->base == NULL) {
        ALOGE("%s[%d] ctx is NULL",__func__,__LINE__);
        return -1;
    }

    return ctx->base->setCmd(pgsp_cfg_info,1);
}



/** Close the gsp device */
static int32_t GSPHAL_close(hw_device_t *dev)
{
    gsp_hal_context_t* ctx = (gsp_hal_context_t*)dev;
    if (ctx) {
        if(ctx->base) {
            delete ctx->base;
            ctx->base = NULL;
        }
        delete ctx;
        ctx = NULL;
    }
    return 0;
}


/** Open a new instance of a copybit device using name */
static int32_t GSPHAL_open(const struct hw_module_t* module,
                           const char* name,
                           struct hw_device_t** device)
{
    int32_t status = 0;

    ALOGD("%s[%d]111111gsp hal lib name:%s \n",__func__, __LINE__, name);
    ctx = new (gsp_hal_context_t);
    if(ctx) {
        memset(ctx, 0, sizeof(*ctx));
    } else {
        ALOGE("gsp hal open, alloc context failed. Line:%d \n", __LINE__);
        status = GSP_HAL_ALLOC_ERR;
        goto exit;
    }
    ctx->device.common.tag = HARDWARE_DEVICE_TAG;
    ctx->device.common.version = 2;
    ctx->device.common.module = const_cast<hw_module_t*>(module);
    ctx->device.common.close = GSPHAL_close;
    ctx->device.GSP_Proccess = GSPHAL_process;
    ctx->device.GSP_GetCapability = GSPHAL_getCapability;
    ctx->device.GSP_SetFBInfo = GSPHAL_setFBInfo;
    ctx->device.GSP_SetOutFormat = GSPHAL_setOutFormat;
    ctx->device.GSP_Prepare = GSPHAL_prepare;
    ctx->device.GSP_Set = GSPHAL_set;

    ctx->base = new GSPImpl;
    if(ctx->base->init()) {
        ALOGE("%s[%d] gsp dev open failed!\n", __func__, __LINE__);
        delete ctx->base;
        ctx->base = NULL;
        *device = NULL;

        ctx->base = new GSPNImpl;
        if(ctx->base->init()) {
            ALOGE("%s[%d] gspn dev open failed!\n", __func__, __LINE__);
            delete ctx->base;
            ctx->base = NULL;
            *device = NULL;
            return -1;
        }
    }

    *device = &ctx->device.common;
    status = 0;
    ALOGD("%s[%d] gsp hal lib open success. Line:%d \n", __func__, __LINE__);

exit:
    return status;
}




static struct hw_module_methods_t gsp_module_methods = {
open:
    GSPHAL_open
};


/*
 * The COPYBIT Module
 */
gsp_module_t HAL_MODULE_INFO_SYM = {
common:
    {
        tag: HARDWARE_MODULE_TAG,
        version_major: 1,
        version_minor: 0,
        id: GSP_HARDWARE_MODULE_ID,
        name: "SPRD 2D Accelerate Module",
        author: "Google, Inc.",
        methods: &gsp_module_methods,
        dso: 0,
        reserved: {0},
    }
};


