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
 ** File: SprdUtil.h                  DESCRIPTION                             *
 **                                   Transform or composer Hardware layers   *
 **                                   when display controller cannot deal     *
 **                                   with these function                     *
 ******************************************************************************
 ******************************************************************************
 ** Author:         zhongjun.chen@spreadtrum.com                              *
 *****************************************************************************/


#ifndef _SPRD_UTIL_H_
#define _SPRD_UTIL_H_

#include <utils/Thread.h>
#include <utils/RefBase.h>
#include <cutils/log.h>
#include <semaphore.h>
#include "MemoryHeapIon.h"
#include <ui/Rect.h>

#include "SprdHWLayer.h"
#include "gralloc_priv.h"
#include "SprdPrimaryDisplayDevice/SprdFrameBufferHAL.h"

#ifdef PROCESS_VIDEO_USE_GSP
#include "sc8830/gsp_hal.h"
#endif

//#ifdef TRANSFORM_USE_DCAM
#include "sc8825/dcam_hal.h"
//#endif

#ifdef TRANSFORM_USE_GPU
#include "sc8810/gpu_transform.h"
#endif

using namespace android;

/*
 *  Accelerator mode
 * */
#define ACCELERATOR_NON              (0x00000000)
#define ACCELERATOR_GSP              (0x00000001)
#define ACCELERATOR_GSP_IOMMU        (0x00000010)
#define ACCELERATOR_OVERLAYCOMPOSER  (0x00000100)
#define ACCELERATOR_DCAM             (0x00001000)

#ifndef ALIGN
#define ALIGN(value, base) (((value) + ((base) - 1)) & ~((base) - 1))
#endif

#ifdef PROCESS_VIDEO_USE_GSP
typedef enum
{
    GEN_COLOR_RED,
    GEN_COLOR_GREEN,
    GEN_COLOR_BLUE,
    GEN_COLOR_YELLOW,
    GEN_COLOR_MAGENTA,
    GEN_COLOR_CYAN,
    GEN_COLOR_BLACK,
    GEN_COLOR_WHITE,
    GEN_COLOR_GRAY
} GEN_COLOR;
#endif

#ifdef TRANSFORM_USE_DCAM
/*
 *  Transform OSD layer.
 * */
class OSDTransform: public Thread
{
public:
    OSDTransform();
    ~OSDTransform();

    void onStart(SprdHWLayer *l, private_handle_t* buffer);
    void onWait();

private:
    SprdHWLayer *mL;
    FrameBufferInfo *mFBInfo;
    private_handle_t* mBuffer;
    bool mInitFLag;
    int mDebugFlag;


    /*
     * OSDTransform thread info.
     * In order to accerate the OSD transform speed,
     * need start a new thread to the transform work,
     * Parallel with video transform work.
     * */
#ifdef _PROC_OSD_WITH_THREAD
    sem_t startSem;
    sem_t doneSem;

    virtual status_t readyToRun();
    virtual void onFirstRef();
    virtual bool threadLoop();
#endif

    int transformOSD();
};
#endif


/*
 *  SprdUtil is responsible for transform or composer HW layers with
 *  hardware devices, such as DCAM, GPU or GSP.
 * */
class SprdUtil
{
public:
    SprdUtil(FrameBufferInfo *fbInfo)
        : mFBInfo(fbInfo),
#ifdef TRANSFORM_USE_DCAM
          tmpDCAMBuffer(NULL),
          mOSDTransform(NULL),
#endif
#ifdef PROCESS_VIDEO_USE_GSP
          tmpBuffer(NULL),
          copyTempBuffer(NULL),
          mGspDev(NULL),
          outBufferPhy(0),
          outBufferSize(0),
#endif
          mOutputFormat(GSP_DST_FMT_YUV420_2P),
          mInitFlag(0),
          mDebugFlag(0)
    {
#ifdef PROCESS_VIDEO_USE_GSP
        memset((void*)&mGsp_cap,0,sizeof(mGsp_cap));
#endif
    }
    ~SprdUtil();

    bool transformLayer(SprdHWLayer *l1, SprdHWLayer *l2,
                        private_handle_t* buffer1, private_handle_t* buffer2);

#ifdef PROCESS_VIDEO_USE_GSP
    int composerLayers(SprdHWLayer *l1, SprdHWLayer *l2, GSP_CONFIG_INFO_T *pgsp_cfg_info, private_handle_t* dst_buffer,GSP_LAYER_DST_DATA_FMT_E dst_format);
    int composerLayerList(SprdHWLayer **videoLayerList, int videoLayerCount,
                          SprdHWLayer **osdLayerList, int osdLayerCount,
                          private_handle_t* buffer1, private_handle_t* buffer2);
    /*
     *  Some device just want to use the specified address type
     * */
    inline void forceUpdateAddrType(int addrType)
    {
        mGsp_cap.buf_type_support = addrType;
    }

    inline void UpdateFBInfo(FrameBufferInfo *FBInfo)
    {
        mFBInfo = FBInfo;
    }

    inline void UpdateOutputFormat(GSP_LAYER_DST_DATA_FMT_E format)
    {
        mOutputFormat = format;
    }
    int getGSPCapability(GSP_CAPABILITY_T *pGsp_cap);

    static GSP_LAYER_SRC_DATA_FMT_E formatType_convert(int format);
    static void test_gen_white_boundary(char* base,uint32_t w,uint32_t h,uint16_t format);
    static void test_gen_color_block(char* base,uint16_t pitch_w,uint16_t pitch_h,uint16_t format, struct sprdRect *rect,GEN_COLOR color,uint16_t gray);
    static void test_gen_color_blocks(char* base,uint32_t pitch_w,uint32_t pitch_h,uint16_t format,uint16_t gray);
    static void test_color(struct private_handle_t *private_h, GSP_LAYER_SRC_DATA_FMT_E img_format);
    static void test_color_for_prepare(hwc_display_contents_1_t *list);
#endif

private:
    FrameBufferInfo *mFBInfo;
#ifdef TRANSFORM_USE_DCAM
    private_handle_t* tmpDCAMBuffer;
    sp<OSDTransform>  mOSDTransform;
#endif
#ifdef PROCESS_VIDEO_USE_GSP
    private_handle_t* tmpBuffer;
    private_handle_t* copyTempBuffer;
    gsp_device_t *mGspDev;
    int outBufferPhy;
    int outBufferSize;
    GSP_LAYER_DST_DATA_FMT_E mOutputFormat;
    GSP_CAPABILITY_T mGsp_cap;
#endif
    int mInitFlag;
    int mDebugFlag;

#ifdef TRANSFORM_USE_GPU
    int getTransformInfo(SprdHWLayer *l1, SprdHWLayer *l2,
                         private_handle_t* buffer1, private_handle_t* buffer2,
                         gpu_transform_info_t *transformInfo);
#endif
#ifdef PROCESS_VIDEO_USE_GSP
    int openGSPDevice();
    int acquireTmpBuffer(int width, int height, int format, private_handle_t* friendBuffer, int *outBufferPhy, int *outBufferSize);

    int gsp_osd_layer_config(SprdHWLayer *layer, GSP_CONFIG_INFO_T &gsp_cfg_info);
    int gsp_dst_layer_config(GSP_CONFIG_INFO_T &gsp_cfg_info, private_handle_t* dst_buffer);
    int gsp_image_layer_config(SprdHWLayer *l1, GSP_CONFIG_INFO_T &gsp_cfg_info, GSP_CONFIG_INFO_T *pgsp_cfg_info);

    int64_t UtilGetSystemTime();
    int need_scaling_check(SprdHWLayer *layer);
    GSP_ROT_ANGLE_E rotationType_convert(int angle);
    void gsp_intermedia_dump(private_handle_t* dst_buffer);
    int findAnIndependentLayer(SprdHWLayer **LayerList, int cnt);
    int scaling_up_twice_check(GSP_CONFIG_INFO_T &gsp_cfg_info);
    int gsp_split_pages_check(GSP_CONFIG_INFO_T &gsp_cfg_info);
    int RegionEqualCheck(SprdHWLayer *pLayer,struct sprdRect *DstRegion);
    int compositeAreaCheck(SprdHWLayer **LayerList,struct sprdRect *DstRegion);
    int gen_points_from_rect(struct sprdPoint *outPoints,SprdHWLayer *layer);
    int compositePointCheck(SprdHWLayer **LayerList,struct sprdRect *DstRegion);
    int full_screen_check(SprdHWLayer **LayerList, int cnt, FrameBufferInfo *FBInfo);
    int gsp_process_va_copy2_pa(GSP_CONFIG_INFO_T *pgsp_cfg_info);
    int scalingup_twice(GSP_CONFIG_INFO_T &gsp_cfg_info, private_handle_t* dst_buffer);
#endif
};


#endif
