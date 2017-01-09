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
 ** File: gspbase.h                   DESCRIPTION                             *
 **                                   GSP class define                        *
 ******************************************************************************
 ** Author:         tianci.yin@spreadtrum.com                                 *
 *****************************************************************************/


#ifndef _GSP_BASE_H_
#define _GSP_BASE_H_

#include <ui/GraphicBufferAllocator.h>
#include <ui/GraphicBufferMapper.h>

#include <ui/Rect.h>
#include "SprdHWLayer.h"
#include "dump.h"
#include "SprdUtil.h"

using namespace android;

#ifndef BIT
#define BIT(nr)         (1UL << (nr))
#endif


#ifndef ALIGN
#define ALIGN(value, base) (((value) + ((base) - 1)) & ~((base) - 1))
#endif

#ifndef GSP_ALOGI_IF
#define GSP_ALOGI_IF(flag, fmt, ...) \
	do { \
		if ((flag)) \
			((void)ALOG(LOG_INFO, LOG_TAG, " %s[%d] " fmt,__func__,__LINE__,##__VA_ARGS__)); \
	} while(0)
#endif

#ifndef GSP_ALOGE
#define GSP_ALOGE(fmt, ...) \
    ((void)ALOG(LOG_ERROR, LOG_TAG, " %s[%d] " fmt,__func__,__LINE__,##__VA_ARGS__))
#endif

template <typename T>
static inline void swap(T& a, T& b)
{
    T t(a);
    a = b;
    b = t;
}


typedef enum {
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

/*
 *  SprdUtil is responsible for transform or composer HW layers with
 *  hardware devices, such as DCAM, GPU or GSP.
 * */
class GSPBase
{
public:
    GSPBase();
    virtual ~GSPBase();
    int64_t getSystemTime();
    //virtual int acquireTmpBuffer(int width, int height, int format, private_handle_t* friendBuffer, int *outBufferPhy, int *outBufferSize);
    void setFBInfo(uint32_t w,uint32_t h);
    void printLayerInfo(hwc_layer_1_t const* l);
    void updateOutputFormat(uint32_t format);

    void test_gen_white_boundary(char* base,uint32_t w,uint32_t h,uint32_t format);
    void test_gen_color_block(char* base,uint32_t pitch_w,uint32_t pitch_h,uint32_t format, struct sprdRect *rect,GEN_COLOR color,uint32_t gray);
    void test_gen_color_blocks(char* base,uint32_t pitch_w,uint32_t pitch_h,uint32_t format,uint32_t gray);
    void test_color(struct private_handle_t *private_h, uint32_t img_format);
    void test_color_for_prepare(hwc_display_contents_1_t *list);

    virtual int32_t init()=0;// probe dev/sprd_gsp exist? and then get capability, then close
    virtual int32_t getCapability(void *pCap, uint32_t size)=0;
    virtual int32_t setCmd(void *pCfg,int32_t cnt)=0;
    virtual int32_t prepare(SprdHWLayer **layerList, int32_t layerCnt, bool &support)=0;
    virtual int32_t composeLayerList(struct _SprdUtilSource *Source, struct _SprdUtilTarget *Target)=0;

public:
    int32_t mDevFd;//"dev/sprd_gsp"
    int32_t mDebugFlag;
    uint32_t mOutputFormat;
    uint32_t mFBWidth;
    uint32_t mFBHeight;
    private_handle_t* mSclTmpBuffer;//scaling up twice

};


#endif
