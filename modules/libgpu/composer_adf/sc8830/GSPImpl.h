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
 ** File: gspimpl                     DESCRIPTION                             *
 **                                   GSP class define                        *
 ******************************************************************************
 ** Author:         tianci.yin@spreadtrum.com                                 *
 *****************************************************************************/


#ifndef _GSP_IMPL_H_
#define _GSP_IMPL_H_

#include <ui/GraphicBufferAllocator.h>
#include <ui/GraphicBufferMapper.h>
#include "SprdUtil.h"
#include "GSPBase.h"
#include "gsp_types_shark_adf.h"

using namespace android;

//#define ALL_EVEN

/*
 *  SprdUtil is responsible for transform or composer HW layers with
 *  hardware devices, such as DCAM, GPU or GSP.
 * */
class GSPImpl : public GSPBase
{
public:
    GSPImpl();
    ~GSPImpl();

    virtual int32_t init();// probe dev/sprd_gsp exist? and then get capability, then close
    virtual int32_t getCapability(void *pCap, uint32_t size);
    virtual int32_t setCmd(void *pCfg,int32_t cnt);
    virtual int32_t prepare(SprdHWLayer **layerList, int32_t layerCnt, bool &support);
    virtual int32_t composeLayerList(struct _SprdUtilSource *Source, struct _SprdUtilTarget *Target);

    int32_t prepareSingleLayer(struct sprdRect *srcRect,
                               struct sprdRect *dstRect,
                               hwc_layer_1_t *layer,
                               struct private_handle_t *privateH,
                               int32_t &scaleFlag,
                               int32_t &yuvLayerCnt);
    int32_t layer0_params_check (struct gsp_layer0_cfg_info *layer0_info);
    int32_t layer1_params_check(struct gsp_layer1_cfg_info *layer1_info);
    int32_t misc_params_check(struct gsp_cfg_info *gsp_cfg_info);
    int32_t layerdes_params_check(struct gsp_cfg_info *gsp_cfg_info);
    int32_t params_check(struct gsp_cfg_info *gsp_cfg_info);
    void dev_open(void);
    void dev_close(void);
    int32_t dev_config(struct gsp_cfg_info *gsp_cfg_info);
	void desFormatConvert(int format);

    int32_t composerLayers(SprdHWLayer *l1, SprdHWLayer *l2, 
								struct gsp_cfg_info *pgsp_cfg_info, 
								private_handle_t* dst_buffer,
								GSP_LAYER_DST_DATA_FMT_E dst_format, 
								struct _SprdUtilTarget *Target,
								bool middleComposition);

    /*
     *  Some device just want to use the specified address type
     * */
    inline void forceUpdateAddrType(int32_t addrType)
    {
        mGsp_cap.buf_type_support = addrType;
    }


    GSP_LAYER_SRC_DATA_FMT_E formatType_convert(int32_t format);
    int32_t acquireTmpBuffer(uint32_t width, uint32_t height, uint32_t format);
    int32_t gsp_osd_layer_config(SprdHWLayer *layer, struct gsp_cfg_info &gsp_cfg_info);
    int32_t gsp_dst_layer_config(struct gsp_cfg_info &gsp_cfg_info, private_handle_t* dst_buffer,bool middleComposition,
			struct _SprdUtilTarget *Target,struct gsp_cfg_info *pgsp_cfg_info);
    int32_t gsp_image_layer_config(SprdHWLayer *l1, struct gsp_cfg_info &gsp_cfg_info, struct gsp_cfg_info *pgsp_cfg_info);
    int32_t need_scaling_check(SprdHWLayer *layer);
    GSP_ROT_ANGLE_E rotationType_convert(int32_t angle);
    void gsp_intermedia_dump(private_handle_t* dst_buffer);
    int32_t findAnIndependentLayer(SprdHWLayer **LayerList, int32_t cnt);
    int32_t scaling_up_twice_check(struct gsp_cfg_info &gsp_cfg_info);
    int32_t gsp_split_pages_check(struct gsp_cfg_info &gsp_cfg_info);
    int32_t RegionEqualCheck(SprdHWLayer *pLayer,struct sprdRect *DstRegion);
    int32_t compositeAreaCheck(SprdHWLayer **LayerList,struct sprdRect *DstRegion);
    int32_t gen_points_from_rect(struct sprdPoint *outPoints,SprdHWLayer *layer);
    int32_t compositePointCheck(SprdHWLayer **LayerList,struct sprdRect *DstRegion);
    int32_t full_screen_check(SprdHWLayer **LayerList, int32_t cnt);
    int32_t gsp_process_va_copy2_pa(struct gsp_cfg_info *pgsp_cfg_info);
    int32_t scalingup_twice(struct gsp_cfg_info &gsp_cfg_info, struct _SprdUtilTarget *Target);

private:
    int32_t mInitFlag;
	int mAsyncFlag;
	int mLastOverlapWaitFd;
	int mLastScaleWaitFd;
    struct gsp_capability mGsp_cap;
    private_handle_t* mCopyTempBuffer;
};


#endif
