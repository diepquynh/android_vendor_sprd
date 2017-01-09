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
** File: gsp_hal.h                 DESCRIPTION                             *
**                                   GSP class define                        *
******************************************************************************
** Author:         tianci.yin@spreadtrum.com                                 *
*****************************************************************************/

#ifndef _GSP_HAL_H_
#define _GSP_HAL_H_

#ifdef   __cplusplus
//extern   "C"
//{
#endif

#include "GSPImpl.h"
#include "GSPNImpl.h"
#include "SprdUtil.h"

using namespace android;

/**
 * The id of this module
 */
#define GSP_HARDWARE_MODULE_ID "sprd_gsp"

/**
 * Every hardware module must have a data structure named HAL_MODULE_INFO_SYM
 * and the fields of this data structure must begin with hw_module_t
 * followed by module specific information.
 */
typedef struct
{
    struct hw_module_t common;
} gsp_module_t;




/**
 * Every device data structure must begin with hw_device_t
 * followed by module specific public methods and attributes.
 */
typedef struct
{
    struct hw_device_t common;

    /*
    func:GSP_Proccess
    desc:all the GSP function can be complete in this function, such as CFC,scaling,blend,rotation and mirror,clipping.
    note:1 the source and destination image buffer should be physical-coherent memory space,
           the caller should ensure the in and out buffer size is large enough, or the GSP will access cross the buffer border,
           these two will rise unexpected exception.
         2 this function will be block until GSP process over.
    */
    int32_t (*GSP_Proccess)(void *pCfg);//GSP_Proccess

    /*
    func:GSP_GetCapability
    desc:get GSP capability, like buffer addr type, scaling range
    return:
    */
    int32_t (*GSP_GetCapability)(void *pGsp_cap, uint32_t size);

    /*
    func:GSP_FBInfoSet
    desc:HWC pass FB info to GSP HAL
    return:
    */
    int32_t (*GSP_SetFBInfo)(uint32_t w, uint32_t h);

    /*
    func:GSP_FBInfoSet
    desc:HWC pass FB info to GSP HAL
    return:
    */
    int32_t (*GSP_SetOutFormat)(uint32_t format);


    /*
    func:GSP_PrepareCheck
    desc:check GSP can process this case or not
    return:
    */
    int32_t (*GSP_Prepare)(SprdHWLayer **layerList, int32_t layerCnt, bool &support);
    int32_t (*GSP_Set)(struct _SprdUtilSource *Source, struct _SprdUtilTarget *Target);

} gsp_device_t;

/** State information for each device instance */
typedef struct
{
    gsp_device_t device;
    GSPBase *base;
    uint16_t mFBWidth;
    uint16_t mFBHeight;
} gsp_hal_context_t;


#ifdef   __cplusplus
//}
#endif
#endif
