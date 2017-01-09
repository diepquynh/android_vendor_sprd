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


#ifndef _GSPN_IMPL_H_
#define _GSPN_IMPL_H_

#include "GSPBase.h"
#include "gsp_types_shark_adf.h"
#include "gspn_user_cfg_adf.h"
#include "SprdUtil.h"


using namespace android;

//#define ALL_EVEN

/*
 *  SprdUtil is responsible for transform or composer HW layers with
 *  hardware devices, such as DCAM, GPU or GSP.
 * */
class GSPNImpl : public GSPBase
{
public:
    GSPNImpl();
    ~GSPNImpl();

    virtual int32_t init();// probe dev/sprd_gsp exist? and then get capability, then close
    virtual int32_t getCapability(void *pCap, uint32_t size);
    virtual int32_t setCmd(void *pCfg,int32_t cnt);
    virtual int32_t prepare(SprdHWLayer **layerList, int32_t layerCnt, bool &support);
    virtual int32_t composeLayerList(struct _SprdUtilSource *Source, struct _SprdUtilTarget *Target);

    void dev_open(void);
    void dev_close(void);
    int32_t dev_config(void *cmd_info,int32_t cnt);

    int32_t prepareSingleLayer(struct sprdRect *srcRect,
                               struct sprdRect *dstRect,
                               hwc_layer_1_t *layer,
                               struct private_handle_t *privateH,
                               int32_t &scaleFlag,
                               int32_t &yuvLayerCnt,
                               uint32_t &transform,
                               uint32_t &readByteCnt);
    int32_t composerLayers(struct _SprdUtilSource *Source, struct _SprdUtilTarget *Target,
			struct private_handle_t *dst_buffer);
    int32_t clip_range_check(GSPN_RECT_SIZE_T &pitch, GSPN_POSITION_T &clip_start, GSPN_RECT_SIZE_T &clip_size);
    int32_t reg_range_check(GSPN_CMD_INFO_T &cmd_info);
    int32_t dst_params_check(GSPN_CMD_INFO_T &cmd_info);
    int32_t src_params_check(GSPN_CMD_INFO_T &cmd_info);
    int32_t scale_range_check(GSPN_CMD_INFO_T &cmd_info);
	void desFormatConvert(int format);
    int32_t params_check(void *cmd_info,int32_t cnt);

    /*
     *  Some device just want to use the specified address type
     * */
    inline void forceUpdateAddrType(int32_t addrType)
    {
        mGSPNCap.addr_type_support = addrType;
    }

    GSPN_LAYER0_FMT_E imgLayerFormatType_convert(int32_t format);
    GSPN_LAYER1_FMT_E osdLayerFormatType_convert(int32_t format);
    GSPN_ROT_MODE_E rotationType_convert(int32_t angle);

    int32_t misc_Info_config(GSPN_CMD_INFO_T &cmd_info);
    int32_t dst_layer_config(GSPN_CMD_INFO_T &cmd_info, struct _SprdUtilTarget *Target,
			struct private_handle_t *dst_buffer);
    int32_t img_layer_config(SprdHWLayer **LayerList,int32_t idex,GSPN_CMD_INFO_T &cmd_info);
    int32_t osd_layer_config(SprdHWLayer *layer, GSPN_CMD_INFO_T &cmd_info, int32_t osdIdx);
    int32_t rot_adjust_single(uint16_t &dx, uint16_t &dy, uint16_t &dw, uint16_t &dh, uint32_t transform);
    int32_t rot_adjust(GSPN_CMD_INFO_T &cmd_info, SprdHWLayer **LayerList);

    int32_t acquireTmpBuffer(uint32_t width, uint32_t height, uint32_t format);
    int32_t scaling_up_twice_check(GSPN_CMD_INFO_T &cmd_info);
    int32_t scalingup_twice(GSPN_CMD_INFO_T &cmd_info);

private:
    int32_t mInitFlag;
    GSPN_CAPABILITY_T mGSPNCap;
    GSPN_CMD_INFO_T mGSPNCMDs[4];
    uint32_t mFrameId;// indicate a few cmds belong to a same frame, these cmds in memory should be like GSPN_CMD_INFO_T cmds[n] array form.
    uint32_t mAsyncFlag;// 0 sync; 1 async
};

#define GSPN_LAYER0_YUV_CHECK(fmt)  (GSPN_LAYER0_FMT_RGB888 < (fmt) && (fmt) < GSPN_LAYER0_FMT_RGB565 )

#endif
