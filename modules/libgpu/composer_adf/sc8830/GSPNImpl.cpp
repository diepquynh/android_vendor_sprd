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
 ** File: GSPNImpl.cpp                DESCRIPTION                             *
 **                                   GSP class define                        *
 ******************************************************************************
 ** Author:         tianci.yin@spreadtrum.com                                 *
 *****************************************************************************/

#include "GSPNImpl.h"
using namespace android;

static void closeFence(int *fd)
{
    if (fd == NULL || *fd < 0) {
        return;
    }

    close(*fd);
    *fd = -1;
}

GSPNImpl::GSPNImpl() : GSPBase(),
    mInitFlag(0),
    mFrameId(0),
    mAsyncFlag(1)
{
    mOutputFormat = GSPN_DST_FMT_ARGB888;
    memset((void*)&mGSPNCap,0,sizeof(mGSPNCap));
}

GSPNImpl::~GSPNImpl()
{
    if(mSclTmpBuffer) {
        GraphicBufferAllocator::get().free((buffer_handle_t)mSclTmpBuffer);
        mSclTmpBuffer = NULL;
    }
    dev_close();
}

int32_t GSPNImpl::init()
{
    if(mDevFd < 0) {
        dev_open();
        if(mDevFd < 0) {
            GSP_ALOGE("%s[%d] opend gsp failed \n", __func__, __LINE__);
            return GSP_HAL_KERNEL_DRIVER_NOT_EXIST;
        }
    }

    GSP_ALOGI_IF(mDebugFlag, "probe gsp device success! fd=%d.\n", mDevFd);
    return getCapability((void*)&mGSPNCap, (uint32_t)sizeof(mGSPNCap));
}

/*
func:getGSPCapability
desc: get gsp process capability from kernel, and according to fb pixel size, adjust it to a proper setting
        kernel driver can read chip id , so it can get GSP HW version, then can evaluate capability.
return: 0:success ; other failed
*/
int32_t GSPNImpl::getCapability(void *pCap, uint32_t size)
{
    int32_t ret = 0;

    if(mGSPNCap.magic == CAPABILITY_MAGIC_NUMBER) {
        goto copy;
    }

    //get capability from kernel
    if(mDevFd < 0) {
        dev_open();
        if(mDevFd < 0) {
            GSP_ALOGE("%s[%d] opend gsp failed!\n", __func__, __LINE__);
            return GSPN_HAL_DRIVER_NOT_EXIST;
        }
    }
    ret = ioctl(mDevFd, GSPN_IO_GET_CAPABILITY, &mGSPNCap);
    if(0 == ret ) {
        GSP_ALOGI_IF(mDebugFlag, "ioctl GET_CAPABILITY ok.\n");

        if(mGSPNCap.magic == GSPN_CAPABILITY_MAGIC_NUMBER) {
            GSP_ALOGI_IF(mDebugFlag, "GET_CAPABILITY success.\n");
        } else {
            GSP_ALOGE("GET_CAPABILITY failed!\n");
            return -1;
        }
    } else {
        GSP_ALOGE("GET_CAPABILITY err:%d.\n",ret);
        return -1;
    }

    GSP_ALOGI_IF(mDebugFlag, "GSP Capability [version:%d, addrType:%d,max_layer:%d,need_copy:%d,scaling_range:%d,blendVideoWithosd:%d,withVideoMaxLayer:%d]",
                 mGSPNCap.version,
                 mGSPNCap.block_alpha_limit,
                 mGSPNCap.scale_updown_sametime,
                 mGSPNCap.seq0_scale_range_up,
                 mGSPNCap.seq0_scale_range_down,
                 mGSPNCap.seq1_scale_range_up,
                 mGSPNCap.seq1_scale_range_down,
                 mGSPNCap.src_yuv_xywh_even_limit,
                 mGSPNCap.max_layer_cnt,
                 mGSPNCap.max_yuvLayer_cnt,
                 mGSPNCap.max_scaleLayer_cnt);
#if 0

    //if it's a perfect chipset, like tshark v2, can process multi-layers, we adjusting it's capability adapt to fb size.
    if(mGSPNCap.max_layer_cnt > 3) {
        int32_t fb_pixel = mFBWidth * mFBHeight;

        GSP_ALOGI_IF(mDebugFlag, "get from kernel,max:%d,max_yuv:%d,max_scale:%d.",
                     mGSPNCap.max_layer_cnt,mGSPNCap.max_yuvLayer_cnt,mGSPNCap.max_scaleLayer_cnt);
        /*
        (          ~ 240*320  ] support 8 layer,      ~76800
        (240*320   ~ 320*480  ] support 6 layer, 76800~153600
        (320*480   ~ 480*800  ] support 5 layer, 153600~384000
        (480*800   ~ 540*960  ] support 4 layer, 384000~518400
        (540*960   ~ 720*1280 ] support 3 layer, 518400~921600
        (720*1280  ~ 1080*1920] support 2 layer, 921600~2073600
        (1080*1920 ~ 1536*2048] support 2 layer, 2073600~3145728
        (1536*2048 ~ 1600*2560] support 2 layer, 3145728~4096000
        */

        if(fb_pixel <= 518400) { //<=540*960, R:480MB W:120MB
            if(mGSPNCap.max_throughput*1024*1024>540*960*60)
                mGSPNCap.max_layer_cnt = 4;

        } else if(fb_pixel <= 921600) { //<=720*1280
            mGSPNCap.max_layer_cnt = 3;
            mGSPNCap.max_layer_cnt_with_video = 3;
        } else if(fb_pixel < 921600) { //<=1080*1920
            mGSPNCap.max_layer_cnt = 2;
            mGSPNCap.max_layer_cnt_with_video = 2;
        } else if(fb_pixel < 921600) { //<=1536*2048
            mGSPNCap.max_layer_cnt = 2;
            mGSPNCap.max_layer_cnt_with_video = 2;
        } else if(fb_pixel < 921600) { //<=1600*2560
            mGSPNCap.max_layer_cnt = 2;
            mGSPNCap.max_layer_cnt_with_video = 2;
        } else { //>1600*2560
            mGSPNCap.max_layer_cnt = 2;
            mGSPNCap.max_layer_cnt_with_video = 2;
        }
        GSP_ALOGI_IF(mDebugFlag, "after adjust,max:%d,maxinvideo:%d.",
                     mGSPNCap.max_layer_cnt,mGSPNCap.max_layer_cnt_with_video);
    }

#endif


copy:
    if(size >= sizeof(mGSPNCap)) {
        memcpy(pCap, (const void*)&mGSPNCap, sizeof(mGSPNCap));
    }

    GSP_ALOGI_IF(mDebugFlag, "Get Capability ok.");

    return ret;
}


int32_t GSPNImpl::clip_range_check(GSPN_RECT_SIZE_T &pitch, GSPN_POSITION_T &clip_start, GSPN_RECT_SIZE_T &clip_size)
{
#define SIZE_IN_HW_RANG_CHECK(iw,ih)\
    if((iw) > mGSPNCap.crop_max.w || (ih) > mGSPNCap.crop_max.h \
       || (iw) < mGSPNCap.crop_min.w || (ih) < mGSPNCap.crop_min.h) { \
        GSP_ALOGE("clip rectangle size beyond hw limit!"); \
        return GSP_HAL_PARAM_CHECK_ERR; \
    }

    SIZE_IN_HW_RANG_CHECK(pitch.w,pitch.h);
    SIZE_IN_HW_RANG_CHECK(clip_size.w,clip_size.h);

    if(clip_start.x+clip_size.w > pitch.w
       ||clip_start.y+clip_size.h > pitch.h) {
        GSP_ALOGE("clip rectangle size beyond src pitch!");
        return GSP_HAL_PARAM_CHECK_ERR;
    }

    return GSPN_NO_ERR;
}

int32_t GSPNImpl::reg_range_check(GSPN_CMD_INFO_T &cmd_info)
{
    if(cmd_info.misc_info.run_mod>1
       ||cmd_info.misc_info.scale_seq>1
       ||cmd_info.misc_info.async_flag>1
       ||cmd_info.misc_info.htap4_en>1
       ||cmd_info.misc_info.pmargb_en>1
       ||cmd_info.misc_info.bg_en>1
       ||cmd_info.misc_info.dither1_en>1
       ||cmd_info.misc_info.dither2_en>1
       ||cmd_info.misc_info.pmargb_en>1
       ||cmd_info.misc_info.pmargb_en>1) {
        GSP_ALOGE("enumerate misc range check failed!");
        return GSP_HAL_PARAM_CHECK_ERR;
    }

    if(cmd_info.des1_info.layer_en>1
       ||cmd_info.des1_info.compress_r8>1
       ||cmd_info.des1_info.r2y_mod>1
       ||cmd_info.des1_info.rot_mod>=GSPN_ROT_MODE_MAX_NUM
       ||cmd_info.des1_info.fmt>=GSPN_DST_FMT_MAX_NUM) {
        GSP_ALOGE("enumerate des1 range check failed!");
        return GSP_HAL_PARAM_CHECK_ERR;
    }

    if(cmd_info.des2_info.layer_en>1
       ||cmd_info.des2_info.compress_r8>1
       ||cmd_info.des2_info.r2y_mod>1
       ||cmd_info.des2_info.rot_mod>=GSPN_ROT_MODE_MAX_NUM
       ||cmd_info.des2_info.fmt>=GSPN_DST_FMT_MAX_NUM) {
        GSP_ALOGE("enumerate des2 range check failed!");
        return GSP_HAL_PARAM_CHECK_ERR;
    }

    if(cmd_info.l0_info.layer_en>1
       ||cmd_info.l0_info.ck_en>1
       ||cmd_info.l0_info.y2r_mod>3
       ||cmd_info.l0_info.pmargb_mod>1
       ||cmd_info.l0_info.z_order>3
       ||cmd_info.l0_info.fmt>=GSPN_LAYER0_FMT_MAX_NUM
       ||cmd_info.l0_info.y2r_param.y_contrast>1023
       ||(cmd_info.l0_info.y2r_param.y_brightness<-256 || 255<cmd_info.l0_info.y2r_param.y_brightness)
       ||cmd_info.l0_info.y2r_param.u_saturation>1023
       ||cmd_info.l0_info.y2r_param.u_offset>255
       ||cmd_info.l0_info.y2r_param.v_saturation>1023
       ||cmd_info.l0_info.y2r_param.v_offset>1023) {
        GSP_ALOGE("enumerate l0 range check failed!");
        return GSP_HAL_PARAM_CHECK_ERR;
    }

    GSPN_LAYER1_INFO_T *osd = &cmd_info.l1_info;
    for(int i=0; i<3; i++) {
        if(osd[i].layer_en>1
           ||osd[i].ck_en>1
           ||osd[i].pallet_en>3
           ||osd[i].pmargb_mod>1
           ||osd[i].fmt>=GSPN_LAYER1_FMT_MAX_NUM) {
            GSP_ALOGE("enumerate osd range check failed!");
            return GSP_HAL_PARAM_CHECK_ERR;
        }
    }

    return GSPN_NO_ERR;
}

int32_t GSPNImpl::src_params_check(GSPN_CMD_INFO_T &cmd_info)
{
    if(cmd_info.l0_info.layer_en == 0
       && cmd_info.l1_info.layer_en == 0
       && cmd_info.l2_info.layer_en == 0
       && cmd_info.l3_info.layer_en == 0) {
        GSP_ALOGE("No any input layer be enabled!");
        return GSP_HAL_PARAM_CHECK_ERR;
    }

    if(mGSPNCap.block_alpha_limit==1) {
#define GSPN_Lx_BLOCK_ALPHA_CHECK(lx_info) \
    if((lx_info.layer_en == 1) && (lx_info.alpha != 255)) { \
        GSP_ALOGE("block alpha is not 255!"); \
        return GSP_HAL_PARAM_CHECK_ERR; \
    }
        //GSPN_Lx_BLOCK_ALPHA_CHECK(cmd_info.l0_info);
        //GSPN_Lx_BLOCK_ALPHA_CHECK(cmd_info.l1_info);
        //GSPN_Lx_BLOCK_ALPHA_CHECK(cmd_info.l2_info);
        //GSPN_Lx_BLOCK_ALPHA_CHECK(cmd_info.l3_info);
    }

    if(cmd_info.l0_info.layer_en == 1 && GSPN_LAYER0_YUV_CHECK(cmd_info.l0_info.fmt)) {
        if((cmd_info.l0_info.pitch.w & 0x1)
           ||(cmd_info.l0_info.pitch.h & 0x1)) {
            GSP_ALOGE("L0 is yuv fmt and pitch w h include odd paramter!");
            return GSP_HAL_PARAM_CHECK_ERR;
        }

        if((cmd_info.l0_info.clip_start.x & 0x1)
           ||(cmd_info.l0_info.clip_start.y & 0x1)
           ||(cmd_info.l0_info.clip_size.w & 0x1)
           ||(cmd_info.l0_info.clip_size.h & 0x1)) {
            GSP_ALOGE("L0 is yuv fmt and xywh include odd paramter!");
            return GSP_HAL_PARAM_CHECK_ERR;
        }
    }

#define GSPN_Lx_CLIP_RANGE_CHECK(lx_info) \
    if((lx_info.layer_en == 1) && \
        clip_range_check(lx_info.pitch, lx_info.clip_start, lx_info.clip_size)) { \
        return GSP_HAL_PARAM_CHECK_ERR; \
    }

    GSPN_Lx_CLIP_RANGE_CHECK(cmd_info.l0_info);
    GSPN_Lx_CLIP_RANGE_CHECK(cmd_info.l1_info);
    GSPN_Lx_CLIP_RANGE_CHECK(cmd_info.l2_info);
    GSPN_Lx_CLIP_RANGE_CHECK(cmd_info.l3_info);

    if(cmd_info.l0_info.layer_en == 1 && GSPN_LAYER0_YUV_CHECK(cmd_info.l0_info.fmt)) {
        if(cmd_info.l0_info.addr.plane_y > 0) {
            if(cmd_info.l0_info.addr.plane_u >= cmd_info.l0_info.addr.plane_y) {
                if((cmd_info.l0_info.addr.plane_u-cmd_info.l0_info.addr.plane_y) < (uint32_t)(cmd_info.l0_info.pitch.w*cmd_info.l0_info.pitch.h)) {
                    GSP_ALOGE("L0 is yuv fmt and uv plane addr err!");
                    return GSP_HAL_PARAM_CHECK_ERR;
                }
            } else {
                if((cmd_info.l0_info.addr.plane_y-cmd_info.l0_info.addr.plane_u) < (uint32_t)(cmd_info.l0_info.pitch.w*cmd_info.l0_info.pitch.h/2)) {
                    GSP_ALOGE("L0 is yuv fmt and y plane addr err!");
                    return GSP_HAL_PARAM_CHECK_ERR;
                }
            }
        } else {
            if(cmd_info.l0_info.addr_info.uv_offset < (uint32_t)(cmd_info.l0_info.pitch.w*cmd_info.l0_info.pitch.h)) {
                GSP_ALOGE("L0 is yuv fmt and uv plane offset err!");
                return GSP_HAL_PARAM_CHECK_ERR;
            }
        }
    }

    return GSPN_NO_ERR;
}

int32_t GSPNImpl::dst_params_check(GSPN_CMD_INFO_T &cmd_info)
{
    uint16_t px = 0,py = 0;
    GSPN_LAYER1_INFO_T *osd = &cmd_info.l1_info;

#define GSPN_RECT_INSIDE_CHECK(desX,desY,desW,desH,srcX,srcY,srcW,srcH) \
    (((desX)<=(srcX)) && ((desY)<=(srcY)) && (((srcX)+(srcW))<=((desX)+(desW))) && (((srcY)+(srcH))<=((desY)+(desH))))
#define GSPN_Lx_DST_RECT_IN_WORK_AREA_CHECK(lx_info) \
    if(lx_info.layer_en == 1 \
        && !GSPN_RECT_INSIDE_CHECK(cmd_info.des1_info.work_src_start.x, \
                                   cmd_info.des1_info.work_src_start.y, \
                                   cmd_info.des1_info.work_src_size.w, \
                                   cmd_info.des1_info.work_src_size.h, \
                                   lx_info.dst_start.x, \
                                   lx_info.dst_start.y, \
                                   lx_info.clip_size.w, \
                                   lx_info.clip_size.h)) { \
        if(cmd_info.misc_info.bg_en == 0) { \
            GSP_ALOGE("Lx dst region is not in work src region!"); \
            return GSP_HAL_PARAM_CHECK_ERR; \
        } else { \
            GSP_ALOGI_IF(mDebugFlag, "Lx dst region is not in work src region!"); \
        } \
    }

    if(cmd_info.misc_info.run_mod == 0) {
        if(cmd_info.misc_info.scale_seq == 0) {
            /*
            dst region of each layer should be inside of work_src region,
            or these region that not in work_src will lose in the final output.
            */
            if(cmd_info.l0_info.layer_en == 1
               && !GSPN_RECT_INSIDE_CHECK(cmd_info.des1_info.work_src_start.x,
                                          cmd_info.des1_info.work_src_start.y,
                                          cmd_info.des1_info.work_src_size.w,
                                          cmd_info.des1_info.work_src_size.h,
                                          cmd_info.l0_info.dst_start.x,
                                          cmd_info.l0_info.dst_start.y,
                                          cmd_info.des1_info.scale_out_size.w,
                                          cmd_info.des1_info.scale_out_size.h)) {
              GSP_ALOGE("des_start_x = %d, des_start_y = %d, des_w = %d, des_h = %d,l0_start_x = %d, l0_start_y = %d, l0_w = %d, l0_h = %d ", 
											cmd_info.des1_info.work_src_start.x, 
                                   		cmd_info.des1_info.work_src_start.y, 
                                   		cmd_info.des1_info.work_src_size.w, 
                                   		cmd_info.des1_info.work_src_size.h, 
                                   		cmd_info.l0_info.dst_start.x, 
                                   		cmd_info.l0_info.dst_start.y, 
                                   		cmd_info.des1_info.scale_out_size.w, 
                                   		cmd_info.des1_info.scale_out_size.h);
                GSP_ALOGE("L0 dst region is not in work src region!");
              //  return GSP_HAL_PARAM_CHECK_ERR;
            }
				//GSP_ALOGE("work_src_start.x = %d, work_src_start.y = %d, work_src_size.w = %d, work_src_size.h = %d,\n",
										//cmd_info.des1_info.work_src_start.x, cmd_info.des1_info.work_src_start.y,
										//cmd_info.des1_info.work_src_size.w,cmd_info.des1_info.work_src_size.h);

				//GSP_ALOGE("l1 des_stat = %d, dst_start.x = %d, dst_start.y = %d,clip_size.w = %d, clip_size.h = %d\n",
										//cmd_info.l1_info.dst_start.x, osd[0].dst_start.x, osd[0].dst_start.y, osd[0].clip_size.w, osd[0].clip_size.h);
				
            for(int i=0; i<3; i++) {
                if(osd[i].layer_en == 1
                   && !GSPN_RECT_INSIDE_CHECK(cmd_info.des1_info.work_src_start.x,
                                              cmd_info.des1_info.work_src_start.y,
                                              cmd_info.des1_info.work_src_size.w,
                                              cmd_info.des1_info.work_src_size.h,
                                              osd[i].dst_start.x,
                                              osd[i].dst_start.y,
                                              osd[i].clip_size.w,
                                              osd[i].clip_size.h)) {
                    GSP_ALOGE("L%d dst region is not in work src region!",i+1);
                    return GSP_HAL_PARAM_CHECK_ERR;
                }
            }

            /*work_des region should be inside of des1_pitch*/
            if(!GSPN_RECT_INSIDE_CHECK(px,
                                       py,
                                       cmd_info.des1_info.pitch.w,
                                       cmd_info.des1_info.pitch.h,
                                       cmd_info.des1_info.work_dst_start.x,
                                       cmd_info.des1_info.work_dst_start.y,
                                       cmd_info.des1_info.work_src_size.w,
                                       cmd_info.des1_info.work_src_size.h)) {
                GSP_ALOGE("dst work region is not in pitch region!");
             //   return GSP_HAL_PARAM_CHECK_ERR;
            }
        } else { //scl seq 1
            /*
            dst region of each layer should be inside of work_src region,
            or these region that not in work_src will lose in the final output.
            */
            GSPN_Lx_DST_RECT_IN_WORK_AREA_CHECK(cmd_info.l0_info);
            GSPN_Lx_DST_RECT_IN_WORK_AREA_CHECK(cmd_info.l1_info);
            GSPN_Lx_DST_RECT_IN_WORK_AREA_CHECK(cmd_info.l2_info);
            GSPN_Lx_DST_RECT_IN_WORK_AREA_CHECK(cmd_info.l3_info);

            /*work_des region should be inside of des1_pitch*/
            if(!GSPN_RECT_INSIDE_CHECK(px,
                                       py,
                                       cmd_info.des1_info.pitch.w,
                                       cmd_info.des1_info.pitch.h,
                                       cmd_info.des1_info.work_dst_start.x,
                                       cmd_info.des1_info.work_dst_start.y,
                                       cmd_info.des1_info.scale_out_size.w,
                                       cmd_info.des1_info.scale_out_size.h)) {
                GSP_ALOGE("dst work region is not in pitch region!");
                return GSP_HAL_PARAM_CHECK_ERR;
            }
        }
    } else {
        /*scaler path*/
        if(cmd_info.l0_info.layer_en == 1
           && !GSPN_RECT_INSIDE_CHECK(px,
                                      py,
                                      cmd_info.des2_info.pitch.w,
                                      cmd_info.des2_info.pitch.h,
                                      cmd_info.l0_info.dst_start.x,
                                      cmd_info.l0_info.dst_start.y,
                                      cmd_info.des2_info.scale_out_size.w,
                                      cmd_info.des2_info.scale_out_size.h)) {
            GSP_ALOGE("scaler path dst region is not in work src region!");
            return GSP_HAL_PARAM_CHECK_ERR;
        }

        /*blender path*/
        GSPN_Lx_DST_RECT_IN_WORK_AREA_CHECK(cmd_info.l1_info);
        GSPN_Lx_DST_RECT_IN_WORK_AREA_CHECK(cmd_info.l2_info);
        GSPN_Lx_DST_RECT_IN_WORK_AREA_CHECK(cmd_info.l3_info);

        if(!GSPN_RECT_INSIDE_CHECK(px,
                                   py,
                                   cmd_info.des1_info.pitch.w,
                                   cmd_info.des1_info.pitch.h,
                                   cmd_info.des1_info.work_dst_start.x,
                                   cmd_info.des1_info.work_dst_start.y,
                                   cmd_info.des1_info.work_src_size.w,
                                   cmd_info.des1_info.work_src_size.h)) {
            GSP_ALOGE("blender path dst work region is not in pitch region!");
            return GSP_HAL_PARAM_CHECK_ERR;
        }
    }

    if(cmd_info.des1_info.layer_en == 1
       && cmd_info.des1_info.compress_r8 == 1
       && (cmd_info.des1_info.fmt != GSPN_DST_FMT_RGB888)) {
        GSP_ALOGE("des1 enable compress, but fmt is not RGB888!");
        return GSP_HAL_PARAM_CHECK_ERR;
    }

    if(cmd_info.des2_info.layer_en == 1
       && cmd_info.des2_info.compress_r8 == 1
       && (cmd_info.des2_info.fmt != GSPN_DST_FMT_RGB888)) {
        GSP_ALOGE("des1 enable compress, but fmt is not RGB888!");
        return GSP_HAL_PARAM_CHECK_ERR;
    }

    return GSPN_NO_ERR;
}


int32_t GSPNImpl::scale_range_check(GSPN_CMD_INFO_T &cmd_info)
{
    uint32_t sw = 0,sh = 0,dw = 0,dh = 0;
    GSPN_LAYER1_INFO_T *osd = &cmd_info.l1_info;
    uint16_t px = 0,py = 0;

    if(cmd_info.misc_info.run_mod == 0) {
        if(cmd_info.misc_info.scale_seq == 0) {
            if(cmd_info.l0_info.layer_en == 1) {
                /*1/16~4*/
                sw = cmd_info.l0_info.clip_size.w;
                sh = cmd_info.l0_info.clip_size.h;
                dw = cmd_info.des1_info.scale_out_size.w;
                dh = cmd_info.des1_info.scale_out_size.h;
                if(sw>dw*16 || sw*4<dw
                   || sh>dh*16 || sh*4<dh) {
                    GSP_ALOGE("00 scaling is out of range!");
                    return GSP_HAL_PARAM_CHECK_ERR;
                }
            }
        } else { //scl seq 1
            /*1/4~4*/
            sw = cmd_info.des1_info.work_src_size.w;
            sh = cmd_info.des1_info.work_src_size.h;
            dw = cmd_info.des1_info.scale_out_size.w;
            dh = cmd_info.des1_info.scale_out_size.h;
            if(sw>dw*4 || sw*4<dw
               || sh>dh*4 || sh*4<dh) {
                GSP_ALOGE("01 scaling is out of range!");
                return GSP_HAL_PARAM_CHECK_ERR;
            }
        }
    } else {
        /*1/16~4*/
        if(cmd_info.l0_info.layer_en == 1) {
            sw = cmd_info.l0_info.clip_size.w;
            sh = cmd_info.l0_info.clip_size.h;
            dw = cmd_info.des2_info.scale_out_size.w;
            dh = cmd_info.des2_info.scale_out_size.h;
            if(sw>dw*16 || sw*4<dw
               || sh>dh*16 || sh*4<dh) {
                GSP_ALOGE("00 scaling is out of range!");
                return GSP_HAL_PARAM_CHECK_ERR;
            }
        }
    }

    /*
     *  GSP defect relative check.
     * */
    if(mGSPNCap.version == 0x10/*sharklT8 and whale*/ && dh<sh/*scaling down*/) {
        uint32_t div = 1;

        if(dh*2>sh) { //
            div = 32;
        } else if(dh*4>sh) {
            div = 64;
        } else if(dh*8>sh) {
            div = 128;
        } else if(dh*16>sh) {
            div = 256;
        }

        if(sh/div*div != sh) {
            if((sh/div*div*dh) > (sh*(dh-1)+1)) {
                GSP_ALOGE("T8/whale scaler hw issue! %dx%d->%dx%d",sw,sh,dw,dh);
                return GSP_HAL_PARAM_CHECK_ERR;
            }
        }
    }

    if (mGSPNCap.scale_updown_sametime == 0) {
        if(((sw < dw) && (sh > dh))
           || ((sw > dw) && (sh < dh))) {
            GSP_ALOGI_IF(mDebugFlag, "Don't support one direction scaling down while the other scaling up!");
            return -1;
        }
    }

    return GSPN_NO_ERR;
}

/*
func:gsp_hal_params_check
desc:check gsp config params before config to kernel
return:0 means success,other means failed
*/
int32_t GSPNImpl::params_check(void *cmd_info,int32_t cnt)
{
    GSPN_CMD_INFO_T *cmds = (GSPN_CMD_INFO_T*)cmd_info;
    if(mGSPNCap.magic != GSPN_CAPABILITY_MAGIC_NUMBER) {
        GSP_ALOGE("param check err\n");
        return GSPN_HAL_PARAM_CHECK_ERR;
    }
    for(int i=0; i<cnt; i++) {
        if(reg_range_check(cmds[i])) {
        	GSP_ALOGE("reg range param check err\n");
            return GSPN_HAL_PARAM_CHECK_ERR;
        }
        if(src_params_check(cmds[i])) {
        GSP_ALOGE("src param check err\n");
            return GSPN_HAL_PARAM_CHECK_ERR;
        }
        if(dst_params_check(cmds[i])) {
        GSP_ALOGE("dst param check err\n");
            return GSPN_HAL_PARAM_CHECK_ERR;
        }
        if(scale_range_check(cmds[i])) {
        GSP_ALOGE("scale range param check err\n");
            return GSPN_HAL_PARAM_CHECK_ERR;
        }
    }
	GSP_ALOGI_IF(mDebugFlag, "param check ok\n");
    return GSPN_NO_ERR;
}

/*
func:gsp_hal_open
desc:open GSP device
return: -1 means failed,other success
notes: a thread can't open the device again unless it close first
*/
void GSPNImpl::dev_open(void)
{
	//while(mDevFd < 0) {
	    mDevFd = open("/dev/sprd_gspn", O_RDWR, 0);
	    	GSP_ALOGE("open gsp device, mDevFd = %d \n", mDevFd);
	    if (mDevFd < 0) {
	        GSP_ALOGE("open gsp device failed!\n");
			 //msleep(200);
	    }
	//}
}

/*
func:gsp_hal_config
desc:set GSP device config parameters
return: -1 means failed,0 success
notes:
*/
int32_t GSPNImpl::dev_config(void *cmd_info,int32_t cnt)
{
    int32_t ret = 0;

    if(mDevFd == -1) {
        return GSP_HAL_PARAM_ERR;
    }

    //software params check
    ret = params_check(cmd_info,cnt);
    if(ret) {
        GSP_ALOGE("param check err,exit without config hw.");
        return ret;
    }

    ret = ioctl(mDevFd, GSPN_IO_SET_PARAM(cnt,mAsyncFlag), cmd_info);
    if(0 == ret) { //gsp hw check params err
        //GSP_ALOGI_IF(mDebugFlag, "gsp set params ok, trigger now.\n");
    } else {
        GSP_ALOGE("hwcomposer gsp set params err:%d.\n",ret);
        //ret = -1;
    }
    return ret;
}

/*
func:dev_close
desc:close GSP device
return: -1 means failed,0 success
notes:
*/
void GSPNImpl::dev_close()
{
    if(mDevFd < 0) {
        GSP_ALOGI_IF(mDebugFlag, "dev fd is invalidate! return.\n");
        return;
    }

    if (close(mDevFd)) {
        if (close(mDevFd)) {
            GSP_ALOGE("close dev fd err! \n");
            return;
        }
    }
    mDevFd = -1;
    return;
}

/*
func:GSP_Proccess
desc:all the GSP function can be complete in this function, such as CFC,scaling,blend,rotation and mirror,clipping.
note:1 the source and destination image buffer should be physical-coherent memory space,
       the caller should ensure the in and out buffer size is large enough, or the GSP will access cross the buffer border,
       these two will rise unexpected exception.
     2 this function will be block until GSP process over.
return: 0 success, other err
*/
int32_t GSPNImpl::setCmd(void *pCfg,int32_t cnt)
{
    int32_t ret = 0;

    if (pCfg == NULL) {
        GSP_ALOGE("pCfg is NULL");
        return -1;
    }

    if(mDevFd < 0) {
        dev_open();
        if(mDevFd < 0) {
            GSP_ALOGE("opend gsp failed \n");
            return GSP_HAL_KERNEL_DRIVER_NOT_EXIST;
        }
    }

    ret = dev_config(pCfg,cnt);
    if(ret) {
        GSP_ALOGE("cfg gsp failed \n");
        goto exit;
    }

exit:
    return ret;
}


/*
func:formatType_convert
desc: image raw data store format covert from andriod hal type to gsp type
return: gsp type
*/
GSPN_LAYER0_FMT_E GSPNImpl::imgLayerFormatType_convert(int32_t format)
{
    switch(format) {
        case HAL_PIXEL_FORMAT_YCbCr_420_SP://0x19
            return GSPN_LAYER0_FMT_YUV420_2P;
        case HAL_PIXEL_FORMAT_YCrCb_420_SP:
            return GSPN_LAYER0_FMT_YUV420_2P;
        case HAL_PIXEL_FORMAT_YV12:// not support by gsp and gspn
            return GSPN_LAYER0_FMT_YUV420_3P;
        case HAL_PIXEL_FORMAT_RGBA_8888:
            return GSPN_LAYER0_FMT_ARGB888;
        case HAL_PIXEL_FORMAT_RGBX_8888:
            return GSPN_LAYER0_FMT_RGB888;
        case HAL_PIXEL_FORMAT_RGB_565:
            return GSPN_LAYER0_FMT_RGB565;
        default:
            break;
    }

    GSP_ALOGE("err: not support src format:%d!",format);
    return GSPN_LAYER0_FMT_MAX_NUM;
}

/*
func:formatType_convert
desc: image raw data store format covert from andriod hal type to gsp type
return: gsp type
*/
GSPN_LAYER1_FMT_E GSPNImpl::osdLayerFormatType_convert(int32_t format)
{
    switch(format) {
        case HAL_PIXEL_FORMAT_RGBA_8888:
            return GSPN_LAYER1_FMT_ARGB888;
        case HAL_PIXEL_FORMAT_RGBX_8888:
            return GSPN_LAYER1_FMT_RGB888;
        case HAL_PIXEL_FORMAT_RGB_565:
            return GSPN_LAYER1_FMT_RGB565;
        default:
            break;
    }

    GSP_ALOGE("err: not support src format:%d!",format);
    return GSPN_LAYER1_FMT_MAX_NUM;
}

/*
func:rotationType_convert
desc: rotation angle covert from andriod hal type to gsp type
return: gsp type
*/
GSPN_ROT_MODE_E GSPNImpl::rotationType_convert(int32_t angle)
{
    switch(angle) {
        case 0:
            return GSPN_ROT_MODE_0;
        case HAL_TRANSFORM_FLIP_H:// 1
            return GSPN_ROT_MODE_180_M;
        case HAL_TRANSFORM_FLIP_V:// 2
            return GSPN_ROT_MODE_0_M;
        case HAL_TRANSFORM_ROT_180:// 3
            return GSPN_ROT_MODE_180;
        case HAL_TRANSFORM_ROT_90:// 4
        default:
            return GSPN_ROT_MODE_270;
        case (HAL_TRANSFORM_ROT_90|HAL_TRANSFORM_FLIP_H)://5
            return GSPN_ROT_MODE_270_M;
        case (HAL_TRANSFORM_ROT_90|HAL_TRANSFORM_FLIP_V)://6
            return GSPN_ROT_MODE_90_M;
        case HAL_TRANSFORM_ROT_270:// 7
            return GSPN_ROT_MODE_90;
    }

    GSP_ALOGE("err:unknow src angle!");
    return GSPN_ROT_MODE_0;
}


int32_t GSPNImpl:: acquireTmpBuffer(uint32_t width, uint32_t height, uint32_t format)
{
    uint32_t stride;

    if (mSclTmpBuffer != NULL) {
        GSP_ALOGE("mSclTmpBuffer already allocated!");
        return 0;
    }

    if(mGSPNCap.addr_type_support == GSPN_ADDR_TYPE_PHYSICAL) {
        GraphicBufferAllocator::get().alloc(width, height, format, GRALLOC_USAGE_OVERLAY_BUFFER, (buffer_handle_t*)&mSclTmpBuffer, &stride);
    } else if(mGSPNCap.addr_type_support == GSPN_ADDR_TYPE_IOVIRTUAL) {
        GraphicBufferAllocator::get().alloc(width, height, format, 0, (buffer_handle_t*)&mSclTmpBuffer, &stride);
    }

    if (mSclTmpBuffer == NULL) {
        GSP_ALOGE("err:Cannot alloc the mSclTmpBuffer ION buffer!");
        return -1;
    }

    return 0;
}

/*
func:scalingup_twice
desc:gsp can only process 1/16-4 range scaling, if scaling up range beyond this limitation,
        we can scaling up twice to achieve that. this func designed to check the judge condition.
return: 1:need scaling up twice ; other : no necessary
*/
int32_t GSPNImpl::scaling_up_twice_check(GSPN_CMD_INFO_T &cmd_info)
{
    if(cmd_info.l0_info.layer_en == 1) { // layer0 be enabled
        if((cmd_info.des1_info.rot_mod & 0x1) == 0) { // 0 180 degree
            if(((cmd_info.l0_info.clip_size.w * 4) < cmd_info.des1_info.scale_out_size.w)
               ||((cmd_info.l0_info.clip_size.h * 4) < cmd_info.des1_info.scale_out_size.h)) {
                return -1;
            }
        } else { // 90 270 degree
            if(((cmd_info.l0_info.clip_size.w * 4) < cmd_info.des1_info.scale_out_size.h)
               || ((cmd_info.l0_info.clip_size.h * 4) < cmd_info.des1_info.scale_out_size.w)) {
                return -1;
            }
        }
    }
    return 0;
}



/*
func:scalingup_twice
desc:gsp can only process 1/16-4 range scaling, if scaling up range beyond this limitation,
        we can scaling up twice to achieve that. this func designed to take this job.
return: 0:success ; other failed
*/
int32_t GSPNImpl::scalingup_twice(GSPN_CMD_INFO_T &cmd_info)
{
    int32_t ret = -1;
    GSPN_CMD_INFO_T *cmds = &cmd_info;
    GSPN_LAYER_DST_FMT_E phase1_des_format = GSPN_DST_FMT_ARGB888;

    GSP_ALOGI_IF(mDebugFlag, "scale up twice enter.");
    cmds[0].misc_info.cmd_cnt++;
    cmds[1] = cmds[0];
    if(mSclTmpBuffer == NULL) {
        /*
        considering miracast scenario, we should alloc a larger buffer.
        scale temp buffer size reach half FB size is large enough.
        */
        if(mFBWidth*mFBHeight<1920*1088) {
            ret = acquireTmpBuffer(1920, 1088/2, HAL_PIXEL_FORMAT_RGBA_8888);
        } else {
            ret = acquireTmpBuffer(mFBWidth, mFBHeight/2, HAL_PIXEL_FORMAT_RGBA_8888);
        }
        if (ret != 0) {
            GSP_ALOGE("err:acquireTmpBuffer failed");
            return -1;
        }
    }

    /*phase1*/
    cmds[0].des1_info.fmt = phase1_des_format;
    cmds[0].des1_info.endian.y_word_endn = GSPN_WORD_ENDN_1;
    cmds[0].des1_info.endian.a_swap_mod = GSPN_A_SWAP_RGBA;
    cmds[0].des1_info.addr_info.share_fd = mSclTmpBuffer->share_fd;
    cmds[0].des1_info.addr_info.uv_offset =
        cmds[0].des1_info.addr_info.v_offset = mFBWidth * mFBHeight;
    cmds[0].des1_info.acq_fen_fd = -1;// scale temp buffer is always availabe, because it's used in a frame internal.
    cmds[0].des1_info.addr.plane_y = 0;
    cmds[0].des1_info.addr.plane_u = 0;
    cmds[0].des1_info.addr.plane_v = 0;
    cmds[0].l0_info.dst_start.x = 0;
    cmds[0].l0_info.dst_start.y = 0;
    cmds[0].des1_info.scale_out_size.w = cmds[0].l0_info.clip_size.w;
    cmds[0].des1_info.scale_out_size.h = cmds[0].l0_info.clip_size.h;


    if((cmds[0].des1_info.rot_mod & 0x1) == 0) {
        if((cmds[0].l0_info.clip_size.w * 4) < cmds[1].des1_info.scale_out_size.w) {
            cmds[0].des1_info.scale_out_size.w = ((cmds[1].des1_info.scale_out_size.w + 7)/4 & 0xfffe);
        }
        if((cmds[0].l0_info.clip_size.h * 4) < cmds[1].des1_info.scale_out_size.h) {
            cmds[0].des1_info.scale_out_size.h = ((cmds[1].des1_info.scale_out_size.h + 7)/4 & 0xfffe);
        }
    } else {
        if((cmds[0].l0_info.clip_size.w * 4) < cmds[1].des1_info.scale_out_size.h) {
            cmds[0].des1_info.scale_out_size.w = ((cmds[1].des1_info.scale_out_size.h + 7)/4 & 0xfffe);
        }
        if((cmds[0].l0_info.clip_size.h * 4) < cmds[1].des1_info.scale_out_size.w) {
            cmds[0].des1_info.scale_out_size.h = ((cmds[1].des1_info.scale_out_size.w + 7)/4 & 0xfffe);
        }
    }
    cmds[0].des1_info.pitch = cmds[0].des1_info.scale_out_size;
    cmds[0].des1_info.rot_mod = GSPN_ROT_MODE_0;
    cmds[0].misc_info.cmd_order =
        cmds[0].misc_info.cmd_priority = 0;
    cmds[0].l1_info.layer_en = 0;//disable osd
    cmds[0].l2_info.layer_en = 0;//disable osd
    cmds[0].l3_info.layer_en = 0;//disable osd
    /*
        ret = setCmd(&cmds[0]);
        if(0 == ret)
        {
            GSP_ALOGI_IF(mDebugFlag, "up2:phase 1,setCmd success");
        }
        else
        {
            GSP_ALOGE("up2:phase 1,setCmd failed!! debugenable = 1;");
            mDebugFlag = 1;
            return ret;
        }
    */
    /*phase2*/
    cmds[1].l0_info.fmt = (GSPN_LAYER0_FMT_E)phase1_des_format;
    cmds[1].l0_info.endian = cmds[0].des1_info.endian;
    cmds[1].l0_info.addr_info = cmds[0].des1_info.addr_info;
    cmds[1].l0_info.acq_fen_fd = -1;// cmd_priority will make sure cmds[0] be executed before cmds[1].
    cmds[1].l0_info.addr = cmds[0].des1_info.addr;
    cmds[1].l0_info.pitch = cmds[0].des1_info.pitch;
    cmds[1].l0_info.clip_start.x = 0;
    cmds[1].l0_info.clip_start.y = 0;
    cmds[1].l0_info.clip_size = cmds[0].des1_info.pitch;
    cmds[1].misc_info.cmd_order =
        cmds[1].misc_info.cmd_priority = 1;
    return 0;
}



int32_t GSPNImpl::misc_Info_config(GSPN_CMD_INFO_T &cmd_info)
{
    cmd_info.misc_info.run_mod = 0;
    cmd_info.misc_info.scale_seq = 0;
    cmd_info.misc_info.scale_en = 0;
    cmd_info.misc_info.async_flag = mAsyncFlag;
    cmd_info.misc_info.htap4_en = 0;
    cmd_info.misc_info.pmargb_en = 0;
    cmd_info.misc_info.bg_en = 1;
    cmd_info.misc_info.gap_rb = 0;
    cmd_info.misc_info.gap_wb = 0;
    cmd_info.misc_info.dither1_en = 1;
    cmd_info.misc_info.cmd_en = 0;

    cmd_info.misc_info.frame_id = mFrameId;
    cmd_info.misc_info.cmd_priority = 0;
    cmd_info.misc_info.cmd_order = 0;
    cmd_info.misc_info.cmd_cnt = 1;

    return 0;
}

int32_t GSPNImpl::dst_layer_config(GSPN_CMD_INFO_T &cmd_info, struct _SprdUtilTarget *Target,
		struct private_handle_t *dst_buffer)
{
    //parameter check
    if(dst_buffer->phyaddr == 0 || dst_buffer->size < (int)(mFBWidth * mFBHeight)) {
        GSP_ALOGE("des.y_addr==0x%08x buffersize_layerd==%d FBw:%d FBh:%d!",
                  dst_buffer->phyaddr, dst_buffer->size, mFBWidth, mFBHeight);
        return -1;
    }

    //config output
    cmd_info.des1_info.bg_color.a = 0;
    cmd_info.des1_info.bg_color.r = 0;
    cmd_info.des1_info.bg_color.g = 0;
    cmd_info.des1_info.bg_color.b = 0;

    cmd_info.des1_info.addr.plane_y =  (uint32_t)dst_buffer->phyaddr;
    cmd_info.des1_info.addr.plane_v =
        cmd_info.des1_info.addr.plane_u =
            cmd_info.des1_info.addr.plane_y + mFBWidth * mFBHeight;
    cmd_info.des1_info.pitch.w = mFBWidth;
    cmd_info.des1_info.pitch.h = mFBHeight;
    GSP_ALOGI_IF(mDebugFlag, "des.y_addr:0x%08x, size:%d",cmd_info.des1_info.addr.plane_y,dst_buffer->size);

    if(cmd_info.des1_info.fmt == GSPN_DST_FMT_ARGB888
       ||cmd_info.des1_info.fmt == GSPN_DST_FMT_RGB888) {
        cmd_info.des1_info.endian.y_word_endn = GSPN_WORD_ENDN_0;
       cmd_info.des1_info.endian.a_swap_mod = GSPN_A_SWAP_ARGB;  //hl changed 0420
       //cmd_info.des1_info.endian.a_swap_mod = GSPN_A_SWAP_ARGB;  //hl changed 0420
    } else if(cmd_info.des1_info.fmt == GSPN_DST_FMT_YUV420_2P
              ||cmd_info.des1_info.fmt == GSPN_DST_FMT_YUV422_2P) {
        cmd_info.des1_info.r2y_mod = 1;
        cmd_info.des1_info.endian.uv_word_endn = GSPN_WORD_ENDN_3;
    }
    cmd_info.des1_info.work_src_start.x =
        cmd_info.des1_info.work_src_start.y = 0;
    cmd_info.des1_info.work_dst_start = cmd_info.des1_info.work_src_start;
    cmd_info.des1_info.work_src_size = cmd_info.des1_info.pitch;
    cmd_info.des1_info.acq_fen_fd = (mAsyncFlag)?Target->releaseFenceFd:-1;
    cmd_info.des1_info.rls_fen_fd = -1;
    cmd_info.des1_info.layer_en = 1;
/*
    GSP_ALOGE("des1 info [f%d,p%d*%d]",
                 cmd_info.des1_info.fmt,
                 cmd_info.des1_info.pitch.w,
                 cmd_info.des1_info.pitch.h);
*/
    return 0;
}


int32_t GSPNImpl::img_layer_config(SprdHWLayer **LayerList,
                                   int32_t idex,
                                   GSPN_CMD_INFO_T &cmd_info)
{
    hwc_layer_1_t *hwcLayer = LayerList[idex]->getAndroidLayer();
    struct sprdRect *srcRect = LayerList[idex]->getSprdSRCRect();
    struct sprdRect *dstRect = LayerList[idex]->getSprdFBRect();
    if (hwcLayer == NULL || srcRect == NULL || dstRect == NULL) {
        GSP_ALOGE("Failed to get Video SprdHWLayer parameters!");
        return -1;
    }

    struct private_handle_t *private_h = (struct private_handle_t *)(hwcLayer->handle);
    if(private_h == NULL) {
        GSP_ALOGE("err:private_h == NULL,return");
        return -1;
    }

    
    GSP_ALOGI_IF(mDebugFlag, "imgLayer src info [f:%x,pm:%d,x%d,y%d,w%d,h%d,p%d,s%d] r%d [x%d,y%d,w%d,h%d]",
                 private_h->format,
                 hwcLayer->blending,
                 srcRect->x, srcRect->y,
                 srcRect->w, srcRect->h,
                 private_h->width, private_h->height,
                 hwcLayer->transform,
                 dstRect->x, dstRect->y,
                 dstRect->w, dstRect->h);

    if((mGSPNCap.addr_type_support == GSPN_ADDR_TYPE_IOVIRTUAL)
       ||((mGSPNCap.addr_type_support == GSPN_ADDR_TYPE_PHYSICAL) && (private_h->flags & private_handle_t::PRIV_FLAGS_USES_PHY))) {
        //config Video ,use GSP L0
        uint32_t pixel_cnt = private_h->stride * private_h->height;
        cmd_info.l0_info.addr_info.share_fd = private_h->share_fd;
        cmd_info.l0_info.addr_info.v_offset =
            cmd_info.l0_info.addr_info.uv_offset = pixel_cnt;

		cmd_info.l0_info.fmt = imgLayerFormatType_convert(private_h->format);
		switch(private_h->format) {
			case HAL_PIXEL_FORMAT_RGBA_8888:
			case HAL_PIXEL_FORMAT_RGBX_8888:
				cmd_info.l0_info.endian.y_word_endn = GSPN_WORD_ENDN_0;
				cmd_info.l0_info.endian.a_swap_mod = GSPN_A_SWAP_ARGB;
				break;
			case HAL_PIXEL_FORMAT_YCrCb_420_SP:
				cmd_info.l0_info.endian.uv_word_endn = GSPN_WORD_ENDN_3;
				break;
			case HAL_PIXEL_FORMAT_RGB_565:
				cmd_info.l0_info.endian.rgb_swap_mod = GSPN_RGB_SWP_RGB;
				break;
			case HAL_PIXEL_FORMAT_YCbCr_420_SP:
				cmd_info.l0_info.endian.uv_word_endn = GSPN_WORD_ENDN_1;
				break;
			default:
				break;
		}

        if(private_h->yuv_info == MALI_YUV_BT601_NARROW
           ||private_h->yuv_info == MALI_YUV_BT709_NARROW
           /*||private_h->yuv_info == MALI_YUV_BT2020_NARROW*/) {
            cmd_info.l0_info.y2r_mod = 3;
        } else if(private_h->yuv_info == MALI_YUV_BT601_WIDE
                  ||private_h->yuv_info == MALI_YUV_BT709_WIDE
                  /*||private_h->yuv_info == MALI_YUV_BT2020_WIDE*/) {
            cmd_info.l0_info.y2r_mod = 2;
        } else {
            GSP_ALOGE("not supported std.");
            cmd_info.l0_info.y2r_mod = 3;
        }

        if(GSPN_LAYER0_YUV_CHECK(cmd_info.l0_info.fmt)) {
            ALOGI_IF((private_h->stride%16 | private_h->height%16),
                     "warning: buffer stride:%d or height:%d is not 16 aligned!",
                     private_h->stride,
                     private_h->height);
        }
/*
        GSP_ALOGI_IF(mDebugFlag, "fd:%d u_offset:%08x,v_offset:%08x,",
                     cmd_info.l0_info.addr_info.share_fd,
                     cmd_info.l0_info.addr_info.uv_offset,
                     cmd_info.l0_info.addr_info.v_offset);

        ALOGI_IF((private_h->width != private_h->stride),
                 "%s[%04d] warning: imgLayer width %d, stride %d, not equal!",
                 __func__, __LINE__,private_h->width, private_h->stride);
*/
        cmd_info.l0_info.pitch.w = private_h->stride;
        cmd_info.l0_info.pitch.h = private_h->height;
        cmd_info.l0_info.pmargb_mod = 1;
        cmd_info.l0_info.alpha = hwcLayer->planeAlpha;
        cmd_info.l0_info.clip_start.x = srcRect->x;
        cmd_info.l0_info.clip_start.y = srcRect->y;
        cmd_info.l0_info.clip_size.w = srcRect->w;
        cmd_info.l0_info.clip_size.h = srcRect->h;
        if(idex == 0 && hwcLayer->blending == HWC_BLENDING_PREMULT) {
            GSP_ALOGE("err:L0 blending flag:%x!",hwcLayer->blending);
        }
        cmd_info.l0_info.dst_start.x = dstRect->x;
        cmd_info.l0_info.dst_start.y = dstRect->y;
        cmd_info.des1_info.scale_out_size.w = dstRect->w;
        cmd_info.des1_info.scale_out_size.h = dstRect->h;
        cmd_info.l0_info.z_order = idex;
        cmd_info.des1_info.rot_mod = rotationType_convert(hwcLayer->transform);
        cmd_info.l0_info.acq_fen_fd = (mAsyncFlag)?hwcLayer->acquireFenceFd:-1;
        cmd_info.l0_info.layer_en = 1;

    } else {
        GSP_ALOGE("err:layer buffer type is not supported!");
        return -1;
    }
/*
    GSP_ALOGI_IF(mDebugFlag, "imgLayer info [f%d,pm:%d,x%d,y%d,w%d,h%d,p%d] 2 [x%d,y%d,w%d,h%d], planeAlpha: %d",
                 cmd_info.l0_info.fmt,
                 cmd_info.l0_info.pmargb_mod,
                 cmd_info.l0_info.clip_start.x,
                 cmd_info.l0_info.clip_start.y,
                 cmd_info.l0_info.clip_size.w,
                 cmd_info.l0_info.clip_size.h,
                 cmd_info.l0_info.pitch,
                 cmd_info.l0_info.dst_start.x,
                 cmd_info.l0_info.dst_start.y,
                 cmd_info.des1_info.scale_out_size.w,
                 cmd_info.des1_info.scale_out_size.h,
                 cmd_info.l0_info.alpha);
*/
    return 0;
}


int32_t GSPNImpl::osd_layer_config(SprdHWLayer *layer, GSPN_CMD_INFO_T &cmd_info, int32_t osdIdx)
{
    if(layer) {
        hwc_layer_1_t *hwcLayer = layer->getAndroidLayer();
        struct sprdRect *srcRect = layer->getSprdSRCRect();
        struct sprdRect *dstRect = layer->getSprdFBRect();
        if (hwcLayer == NULL || srcRect == NULL || dstRect == NULL) {
            GSP_ALOGE("Failed to get OSD SprdHWLayer parameters!");
            return -1;
        }

        struct private_handle_t *private_h = (struct private_handle_t *)(hwcLayer->handle);
        if(private_h == NULL) {
            GSP_ALOGE("err:private_h == NULL,return");
            return -1;
        }

        GSP_ALOGI_IF(mDebugFlag, "osdLayer src info [f:%d,pm:%d,x%d,y%d,w%d,h%d,p%d,s%d] r%d [x%d,y%d,w%d,h%d]",
                     private_h->format,
                     hwcLayer->blending,
                     srcRect->x, srcRect->y,
                     srcRect->w, srcRect->h,
                     private_h->width, private_h->height,
                     hwcLayer->transform,
                     dstRect->x, dstRect->y,
                     dstRect->w, dstRect->h);


        if(private_h->flags &&
           ((mGSPNCap.addr_type_support == GSPN_ADDR_TYPE_IOVIRTUAL)
            ||((mGSPNCap.addr_type_support == GSPN_ADDR_TYPE_PHYSICAL)
               && (private_h->flags & private_handle_t::PRIV_FLAGS_USES_PHY)))) {
            GSPN_LAYER1_INFO_T *osd_info = &cmd_info.l1_info;

            //cmd_info.l1_info.rot_angle = rotationType_convert(hwcLayer->transform);

            if(hwcLayer->transform == HAL_TRANSFORM_ROT_90
               ||hwcLayer->transform == HAL_TRANSFORM_ROT_270) {
                if(srcRect->w != dstRect->h || srcRect->h != dstRect->w) {
                    GSP_ALOGE("OSD scaling not supported![%dx%d] [%dx%d]",
                              srcRect->w,srcRect->h,dstRect->w,dstRect->h);
                    return -1;
                }
            } else {
                if(srcRect->w != dstRect->w || srcRect->h != dstRect->h) {
                    GSP_ALOGE("OSD scaling not supported![%dx%d] [%dx%d]",
                              srcRect->w,srcRect->h,dstRect->w,dstRect->h);
                    return -1;
                }
            }
            osd_info[osdIdx].addr_info.share_fd = private_h->share_fd;
            osd_info[osdIdx].addr_info.uv_offset =
                osd_info[osdIdx].addr_info.v_offset = 0;
            osd_info[osdIdx].pitch.w = private_h->stride;
            osd_info[osdIdx].pitch.h = private_h->height;

            osd_info[osdIdx].fmt = osdLayerFormatType_convert(private_h->format);
            switch(private_h->format) {
                case HAL_PIXEL_FORMAT_RGBA_8888:
                case HAL_PIXEL_FORMAT_RGBX_8888:
                    osd_info[osdIdx].endian.y_word_endn = GSPN_WORD_ENDN_0;
                    osd_info[osdIdx].endian.a_swap_mod = GSPN_A_SWAP_ARGB;
                    break;
                case HAL_PIXEL_FORMAT_RGB_565:
                    osd_info[osdIdx].endian.rgb_swap_mod = GSPN_RGB_SWP_BGR;
                    break;
                default:
                    return -1;
                    break;
            }

            osd_info[osdIdx].clip_start.x = srcRect->x;
            osd_info[osdIdx].clip_start.y = srcRect->y;
            osd_info[osdIdx].clip_size.w = srcRect->w;
            osd_info[osdIdx].clip_size.h = srcRect->h;
            osd_info[osdIdx].dst_start.x = dstRect->x;
            osd_info[osdIdx].dst_start.y = dstRect->y;
            osd_info[osdIdx].alpha = hwcLayer->planeAlpha;
            if(hwcLayer->blending == HWC_BLENDING_PREMULT/*have already pre-multiply*/
               ||hwcLayer->blending == HWC_BLENDING_COVERAGE/*coverage the dst layer*/) {
                osd_info[osdIdx].pmargb_mod = 1;
            } else {
                GSP_ALOGE("L%d blending flag:%x!",osdIdx+1,hwcLayer->blending);
            }
            osd_info[osdIdx].acq_fen_fd = (mAsyncFlag)?hwcLayer->acquireFenceFd:-1;
            osd_info[osdIdx].layer_en = 1;

/*
            ALOGI_IF((private_h->width != private_h->stride),
                     "%s[%04d] warning: osdLayer width %d, stride %d, not equal!",
                     __func__, __LINE__,private_h->width, private_h->stride);
            GSP_ALOGI_IF(mDebugFlag, "osdLayer info [f%d,pm:%d,x%d,y%d,w%d,h%d,p%d] r%d [x%d,y%d], planeAlpha: %d",
                         osd_info[osdIdx].fmt,
                         osd_info[osdIdx].pmargb_mod,
                         osd_info[osdIdx].clip_start.x,
                         osd_info[osdIdx].clip_start.y,
                         osd_info[osdIdx].clip_size.w,
                         osd_info[osdIdx].clip_size.h,
                         osd_info[osdIdx].pitch.w,
                         hwcLayer->transform,
                         osd_info[osdIdx].dst_start.x,
                         osd_info[osdIdx].dst_start.y,
                         osd_info[osdIdx].alpha);
 */
        } else {
            GSP_ALOGE("layer buffer type is not supported!");
            return -1;
        }
    }

    return 0;
}



int32_t GSPNImpl::rot_adjust_single(uint16_t &dx, uint16_t &dy, uint16_t &dw, uint16_t &dh, uint32_t transform)
{
    uint32_t x = dx;
    uint32_t y = dy;

    /*first adjust dest x y*/
    if(transform == 0) {
        // no rotation, do nothing.
    } else if(transform == HAL_TRANSFORM_ROT_90/*CLOCKWISE*/) {
        dx = y;
        dy = mFBWidth - x - dw;
    } else if(transform == HAL_TRANSFORM_ROT_270) {
        dx = mFBHeight - y - dh;
        dy = x;
    } else {
        GSP_ALOGE("not supported transform:%d!",transform);
        return -1;
    }

    /*then adjust dest width height*/
    if(transform == HAL_TRANSFORM_ROT_90
       ||transform == HAL_TRANSFORM_ROT_270) {
        swap(dw, dh);
    }

    return 0;
}

/*
func:rot_adjust
desc:the GSPN is not able to rotate before blending,
     so the destination rectangle description should be adjusted to fit the hw.
note:we suppose the destination buffer rectangle is FB size.
     HAL_TRANSFORM_ROT_90 is CLOCKWISE, but GSPN is anti-clockwise.
*/
int32_t GSPNImpl::rot_adjust(GSPN_CMD_INFO_T &cmd_info, SprdHWLayer **LayerList)
{
    int32_t ret = 0;
    uint16_t w = 0;
    uint16_t h = 0;
    uint32_t transform = 0;
    GSPN_LAYER1_INFO_T *osd_info = NULL;
    hwc_layer_1_t *hwcLayer = LayerList[0]->getAndroidLayer();
    if(hwcLayer == NULL) {
        GSP_ALOGE("get hwcLayer failed!");
        return -1;
    }
    transform = hwcLayer->transform;

    if(cmd_info.l0_info.layer_en == 1) {
        ret = rot_adjust_single(cmd_info.l0_info.dst_start.x,
                              cmd_info.l0_info.dst_start.y,
                                cmd_info.des1_info.scale_out_size.w,
                               cmd_info.des1_info.scale_out_size.h,
                               transform);
        if(ret) {
            GSP_ALOGE("rotation adjust failed!");
            return ret;
        }
    }

    osd_info = &cmd_info.l1_info;
    for(int32_t i=0; i<3; i++) {
        if(osd_info[i].layer_en == 1) {
            w = osd_info[i].clip_size.w;
            h = osd_info[i].clip_size.h;
            if(transform == HAL_TRANSFORM_ROT_90
               ||transform == HAL_TRANSFORM_ROT_270) {
                swap(w, h);
            }
            ret = rot_adjust_single(osd_info[i].dst_start.x,
                                    osd_info[i].dst_start.y,
                                    w,h,transform);
            if(ret) {
                GSP_ALOGE("rotation adjust failed!");
                return ret;
            }
        }
    }

    if(transform == HAL_TRANSFORM_ROT_90
       ||transform == HAL_TRANSFORM_ROT_270) {
        swap(cmd_info.des1_info.pitch.w, cmd_info.des1_info.pitch.h);
    }
    return ret;
}


/*
func:composerLayers
desc:if both layers are valid, blend them, but maybe the output image is not correct,
        because these area that are not covered by any layers maybe is random data.
        if only layer1 is valid, we blend it with background.
        if only layer2 is valid, we blend it with former output.
params:
        pcmd_info(in and out): if only layer2 is valid, we get the former output parameters from it,
                                              and after current task over, we assignment the current task config to it to pass back to caller function.

limits: layer1 and layer2 can be any color format, but if both of them need scaling, it is not supported
*/
int32_t GSPNImpl::composerLayers(struct _SprdUtilSource *Source, struct _SprdUtilTarget *Target,
		struct private_handle_t *dst_buffer)
{
    int32_t i = 0;
    int32_t yuv_index = 0;// yuv layer index
    int32_t ret = 0;
    int32_t scl_flg = 0;
    int64_t start_time = 0;
    int64_t end_time = 0;
    hwc_layer_1_t *layer = NULL;
    struct private_handle_t *privateH = NULL;
    SprdHWLayer **LayerList = Source->LayerList;
    int32_t layerCnt = Source->LayerCount;

    if(mDebugFlag) {
        start_time = getSystemTime()/1000;
    }
    if(mGSPNCap.addr_type_support == GSPN_ADDR_TYPE_INVALID) {
        GSP_ALOGE("ERR:can't get GSP address type!");
        return -1;
    }

	//ALOGE("111111, enter GSPNImpl::composerLayers");

    memset(&mGSPNCMDs,0,sizeof(mGSPNCMDs));
	//ALOGE("111111");
    i = 0;
    while(i < layerCnt) {
        layer = LayerList[i]->getAndroidLayer();
        privateH = (struct private_handle_t *)(layer->handle);

        if ((privateH->format == HAL_PIXEL_FORMAT_YCbCr_420_SP)
            ||(privateH->format == HAL_PIXEL_FORMAT_YCrCb_420_SP)) {
            yuv_index = i;
            break;
        }
        i++;
    }
	//ALOGE("111111, i = %d", i);
    misc_Info_config(mGSPNCMDs[0]);
    mGSPNCMDs[0].des1_info.fmt = (GSPN_LAYER_DST_FMT_E)mOutputFormat;
    ALOGI_IF(mDebugFlag,"gspn des buffer format: %d", mGSPNCMDs[0].des1_info.fmt);
    ret = dst_layer_config(mGSPNCMDs[0], Target, dst_buffer);
    if(ret) {
        GSP_ALOGE("dst_layer_config() return err!");
        return -1;
    }
    if(yuv_index) {
        int32_t j = 0;
        ret = img_layer_config(LayerList,yuv_index,mGSPNCMDs[0]);
        if(ret) {
            GSP_ALOGE("img_layer_config() return err!");
            return -1;
        }

        i = 0;
        while(i < layerCnt) {
            if(yuv_index != i) {
                osd_layer_config(LayerList[i], mGSPNCMDs[0], j);
                if(ret) {
                    GSP_ALOGE("osd_layer_config() return err!");
                    return -1;
                }
                j++;
            }
            i++;
        }
    } else {
        i = 0;
        img_layer_config(LayerList,i,mGSPNCMDs[0]);
        if(ret) {
            GSP_ALOGE("img_layer_config() return err!");
            return -1;
        }
        i++;
        while(i < layerCnt) {
            osd_layer_config(LayerList[i], mGSPNCMDs[0], i-1);
            if(ret) {
                GSP_ALOGE("osd_layer_config() return err!");
                return -1;
            }
            i++;
        }
    }

   rot_adjust(mGSPNCMDs[0], LayerList);


    if(mGSPNCap.seq0_scale_range_up == 256) {
        if(scaling_up_twice_check(mGSPNCMDs[0])) {
            ret = scalingup_twice(mGSPNCMDs[0]);
            if(ret) {
                GSP_ALOGE("scalingup_twice() return err!");
                return ret;
            }
            scl_flg++;
        }
    }

    ret = setCmd(&mGSPNCMDs[0],scl_flg+1);
    if(0 == ret) {
        GSP_ALOGI_IF(mDebugFlag, "setCmd success");
    } else {
        GSP_ALOGE("setCmd failed!");
        mDebugFlag = 1;
    }

    Source->releaseFenceFd = -1;
    Target->acquireFenceFd = -1;
    if(mAsyncFlag == 1) {
        if(scl_flg) {
            if(mGSPNCMDs[0].des1_info.rls_fen_fd > -1) {
                closeFence((int*)&(mGSPNCMDs[0].des1_info.rls_fen_fd));
            }
        }
        // pass the fence fd back
        if(mGSPNCMDs[scl_flg].des1_info.rls_fen_fd > -1) {
            Target->acquireFenceFd = mGSPNCMDs[scl_flg].des1_info.rls_fen_fd;
            Source->releaseFenceFd = dup(Target->acquireFenceFd);
        } else {
            Target->acquireFenceFd = -1;
            Source->releaseFenceFd = -1;
		}
        // source fence fd close, leave to hwc, only close target acq fence fd.
        //closeFence((int*)&(Target->releaseFenceFd));
    }

    if(mDebugFlag) {
        end_time = getSystemTime()/1000;
        GSP_ALOGI_IF(mDebugFlag, "single process: start:%lld, end:%lld,cost:%lld us !!\n",
                     start_time,end_time,(end_time-start_time));
    }
    return 0;
}


void GSPNImpl::desFormatConvert(int format)
{
    switch(format) {
		case HAL_PIXEL_FORMAT_YCbCr_420_SP:
            mOutputFormat =  GSPN_DST_FMT_YUV420_2P;
			break;
        case HAL_PIXEL_FORMAT_YCbCr_422_SP:
            mOutputFormat =  GSPN_DST_FMT_YUV422_2P;
			break;
        case HAL_PIXEL_FORMAT_YCbCr_420_P:
            mOutputFormat =  GSPN_DST_FMT_YUV420_3P;
			break;
        case HAL_PIXEL_FORMAT_RGBA_8888:
            mOutputFormat =  GSPN_DST_FMT_ARGB888;
			break;
        case HAL_PIXEL_FORMAT_RGB_565:
			mOutputFormat =  GSPN_DST_FMT_RGB565;
			break;
		default:
			GSP_ALOGE("err:unknow src format:%d!",format);
			break;
    }
}

int32_t GSPNImpl::composeLayerList(struct _SprdUtilSource *Source, struct _SprdUtilTarget *Target)
{
    int32_t ret = 0;
    int64_t start_time = 0;
    int64_t end_time = 0;
	struct private_handle_t *dst_buffer = NULL;
    /*params check*/
    if((mGSPNCap.magic != CAPABILITY_MAGIC_NUMBER)
       || Source == NULL
       || Source->LayerList == NULL
       || Source->LayerCount == 0
       || Source->LayerCount > mGSPNCap.max_layer_cnt
       || Target == NULL) {
		//GSP_ALOGE("mGSPNCap.magic = 0x%x,  Source->LayerList = %p, mGSPNCap.max_layer_cnt = %d, Source->layerCnt = %d, Target->buffer = %p, target->buffer->share_fd = %d,Target->buffer->phyaddr = %p",
								//mGSPNCap.magic, Source->LayerList, mGSPNCap.max_layer_cnt, Source->layerCnt, Target->buffer, Target->buffer->share_fd, Target->buffer->phyaddr);
		//GSP_ALOGE("11111, Target = %p, Target->buffer = %p", Target, Target->buffer);
		GSP_ALOGE("params check err!");
        return -1;
    }

	if (Target->buffer)
	    dst_buffer = Target->buffer;
	else if (Target->buffer2)
	    dst_buffer = Target->buffer2;

	if (dst_buffer == NULL
	|| dst_buffer->share_fd < 0
	&& (dst_buffer->phyaddr == 0 || Target->buffer->size < (int)(mFBWidth*mFBHeight))) {
		GSP_ALOGE("dst buffer error");
		return -1;
	}

	queryDebugFlag(&mDebugFlag);
	mFrameId++;
	desFormatConvert(Target->format);
    if(mDebugFlag) {
        start_time = getSystemTime()/1000;
    }
    GSP_ALOGI_IF(mDebugFlag, "layerlist total:%d.",Source->LayerCount);
    ret = composerLayers(Source, Target, dst_buffer);
    if(ret != 0) {
        GSP_ALOGE("composerLayers() return err!");
    }

exit:
    if(mDebugFlag) {
        end_time = getSystemTime()/1000;
        GSP_ALOGI_IF(mDebugFlag, "%d-layers start:%lld, end:%06lld,cost:%lld us !!\n",
                     Source->LayerCount,start_time,end_time,(end_time-start_time));
    }
    return ret;
}


int32_t GSPNImpl::prepareSingleLayer(struct sprdRect *srcRect,
                                     struct sprdRect *dstRect,
                                     hwc_layer_1_t *layer,
                                     struct private_handle_t *privateH,
                                     int32_t &scaleFlag,
                                     int32_t &yuvLayerCnt,
                                     uint32_t &transform,
                                     uint32_t &readByteCnt)
{
    uint32_t srcWidth;
    uint32_t srcHeight;
    uint32_t destWidth;
    uint32_t destHeight;
    int32_t gxp_scaling_up_limit;
    int32_t gxp_scaling_down_limit;
    if (srcRect == NULL || dstRect == NULL || layer == NULL || privateH == NULL) {
        GSP_ALOGE("input parameters are NULL.");
        return -1;
    }
    if(mGSPNCap.block_alpha_limit == 1
       && layer->planeAlpha != 255) {
        //GSP_ALOGI_IF(mDebugFlag, "block alpha not 255.");
        //return -1;  hl changed 0416
    }


    /*
     *  Source and destination rectangle size check.
     * */
    if(srcRect->w < (uint32_t)(mGSPNCap.crop_min.w)
       || srcRect->h < (uint32_t)(mGSPNCap.crop_min.h)
       || srcRect->w > (uint32_t)(mGSPNCap.crop_max.w)
       || srcRect->h > (uint32_t)(mGSPNCap.crop_max.h)
       || dstRect->w < (uint32_t)(mGSPNCap.out_min.w)
       || dstRect->h < (uint32_t)(mGSPNCap.out_min.h)
       || dstRect->w > (uint32_t)(mGSPNCap.out_max.w)
       || dstRect->h > (uint32_t)(mGSPNCap.out_max.h)
       || dstRect->w > mFBWidth // when HWC do blending by GSP, the output can't larger than LCD width and height
       || dstRect->h > mFBHeight) {
        GSP_ALOGI_IF(mDebugFlag, "clip or dst rect is not supported.");
        return -1;
    }

    /*
    *  Source layer YUV range check. MALI_YUV_BT601_NARROW
    * */
    if(!(mGSPNCap.std_support_in & BIT(privateH->yuv_info-1))) {
        GSP_ALOGI_IF(mDebugFlag, "GSPN don't support this std.cap_std:%x in_std:%x",mGSPNCap.std_support_in,privateH->yuv_info);
        return -1;
    }

    if ((privateH->format == HAL_PIXEL_FORMAT_YCbCr_420_SP)
        ||(privateH->format == HAL_PIXEL_FORMAT_YCrCb_420_SP)) {
        readByteCnt += srcRect->w*srcRect->h*2;
        if (privateH->usage & GRALLOC_USAGE_PROTECTED ) {
            GSP_ALOGI_IF(mDebugFlag, "find protected video.");
        }

        /*
        *  If yuv_xywh_even == 1, GXP do not support odd source layer.
        * */
        if (mGSPNCap.src_yuv_xywh_even_limit == 1) {
            if ((srcRect->x & 0x1)
                || (srcRect->y & 0x1)
                || (srcRect->w & 0x1)
                || (srcRect->h & 0x1)) {
                GSP_ALOGI_IF(mDebugFlag, "GSP do not support odd source layer xywh.");
                return -1;
            }
        }

        /*
        *  Source video size check.
        * */
        switch(mGSPNCap.max_video_size) {
            case 0:
                if(srcRect->w * srcRect->h > 1280 * 720) {
                    GSP_ALOGI_IF(mDebugFlag, "GSP not support > 720P video.");
                    return -1;
                }
                break;
            case 1:
                if(srcRect->w * srcRect->h > 1920 * 1080) {
                    GSP_ALOGI_IF(mDebugFlag, "GXP not support > 1080P video.");
                    return -1;
                }
                break;
            case 2:
                if(srcRect->w * srcRect->h > 3840 * 2160) {
                    GSP_ALOGI_IF(mDebugFlag, "GXP not support > 2160P video.");
                    return -1;
                }
                break;
            default:
                break;
        }

        yuvLayerCnt++;
    } else if(privateH->format == HAL_PIXEL_FORMAT_RGBA_8888
              ||privateH->format == HAL_PIXEL_FORMAT_RGBX_8888
              ||privateH->format == HAL_PIXEL_FORMAT_BGRA_8888) {
        readByteCnt += srcRect->w*srcRect->h*4;
    } else if(privateH->format == HAL_PIXEL_FORMAT_RGB_565) {
        readByteCnt += srcRect->w*srcRect->h*2;
    } else {
        GSP_ALOGI_IF(mDebugFlag, "the format is not supported,format:%x.",privateH->format);
        return -1;
    }


    //take the rotation in consideration
    if(transform != layer->transform) {
        GSP_ALOGI_IF(mDebugFlag, "GSPN not support rotation before blending.");
        return -1;
    }

    destWidth = dstRect->w;
    destHeight = dstRect->h;
    srcWidth = srcRect->w;
    srcHeight = srcRect->h;

    if(srcWidth == destWidth && srcHeight == destHeight) {
        scaleFlag = 0;
    } else {
        scaleFlag = 1;

        /*
         *  The GSP do not support scailing up and down at the same time.
         * */
        if (mGSPNCap.scale_updown_sametime == 0) {
            if(((srcWidth < destWidth) && (srcHeight > destHeight))
               || ((srcWidth > destWidth) && (srcHeight < destHeight))) {
                GSP_ALOGI_IF(mDebugFlag, "GSP not support one direction scaling down while the other scaling up!");
                return -1;
            }
        }

        /*
         *  Scaling range check.
         * */
        gxp_scaling_up_limit = mGSPNCap.seq0_scale_range_up / 16;
        gxp_scaling_down_limit = 16 / mGSPNCap.seq0_scale_range_down;

        if(gxp_scaling_up_limit * srcWidth < destWidth
           || gxp_scaling_up_limit * srcHeight < destHeight
           || gxp_scaling_down_limit * destWidth < srcWidth
           || gxp_scaling_down_limit * destHeight < srcHeight) {
            //gsp support [1/16-gsp_scaling_up_limit] scaling
            GSP_ALOGI_IF(mDebugFlag, "GSP only support 1/16-%d scaling!",gxp_scaling_up_limit);
            return -1;
        }
    }


    /*
     *  GSP defect relative check.
     * */
    if(destHeight<srcHeight) { //scaling down
        uint32_t div = 1;

        if(destHeight*2>srcHeight) { //
            div = 32;
        } else if(destHeight*4>srcHeight) {
            div = 64;
        } else if(destHeight*8>srcHeight) {
            div = 128;
        } else if(destHeight*16>srcHeight) {
            div = 256;
        }

        if(srcHeight/div*div != srcHeight) {
            if((srcHeight/div*div*destHeight) > (srcHeight*(destHeight-1)+1)) {
                GSP_ALOGI_IF(mDebugFlag, "GSP can't support %dx%d->%dx%d scaling!",
                             srcWidth,srcHeight,destWidth,destHeight);
                return -1;
            }
        }
    }

    return 0;
}


int32_t GSPNImpl::prepare(SprdHWLayer **layerList, int32_t layerCnt, bool &support)
{
    int32_t i = 0;
    int32_t ret = 0;
    int32_t scaleFlag = 0;
    int32_t yuvLayerCnt = 0;
    int32_t overlayLayerCnt = 0;
    int32_t scaleLayerCnt = 0;
    uint32_t transform = 0;
    uint32_t readByteCnt = 0;
    uint32_t writeByteCnt = 0;

    uint32_t DDRBandwidth = 0;
    uint32_t DDRWriteBandwidth = 0;

    struct sprdRect *srcRect;
    struct sprdRect *dstRect;
    hwc_layer_1_t *layer = NULL;
    struct private_handle_t *privateH = NULL;

    support = false;
    if (mGSPNCap.magic != CAPABILITY_MAGIC_NUMBER || layerList == NULL || layerCnt <= 0) {
        GSP_ALOGE("input parameter is invalid.");
        return -1;
    }
    queryDebugFlag(&mDebugFlag);
	
    for (i = 0; i < layerCnt; i++) {
        layer = layerList[i]->getAndroidLayer();
        srcRect = layerList[i]->getSprdSRCRect();
        dstRect  = layerList[i]->getSprdFBRect();

        GSP_ALOGI_IF(mDebugFlag, "process LayerList[%d/%d]",i,layerCnt);
        printLayerInfo(layer);

        if (layer->flags & HWC_SKIP_LAYER) {
            GSP_ALOGI_IF(mDebugFlag, "is not HWC layer");
            continue;
        }
        if (layer->compositionType == HWC_FRAMEBUFFER_TARGET) {
            GSP_ALOGI_IF(mDebugFlag, "HWC_FRAMEBUFFER_TARGET layer, ignore it");
            continue;
        }
        if((layer->blending == HWC_BLENDING_NONE && i != 0)
           ||(layer->blending != HWC_BLENDING_NONE && i == 0)) {
            GSP_ALOGI_IF(mDebugFlag, "blend:0x%08x.", layer->blending);
            return -1;
        }

        privateH = (struct private_handle_t *)(layer->handle);
        if (privateH == NULL) {
            GSP_ALOGI_IF(mDebugFlag, "layer handle is NULL");
            return -1;
        }


        GSP_ALOGI_IF(mDebugFlag, "displayFrame[l%d,t%d,r%d,b%d] mFBWidth:%d mFBHeight:%d dstRect[x%d,y%d,w%d,h%d] ",
                     layer->displayFrame.left,layer->displayFrame.top,layer->displayFrame.right,layer->displayFrame.bottom,
                     mFBWidth, mFBHeight,
                     dstRect->x,dstRect->y,dstRect->w,dstRect->h);

        scaleFlag = 0;
		if (i == 0) {
			transform = layer->transform;
		}
        ret = prepareSingleLayer(srcRect, dstRect, layer, privateH, scaleFlag, yuvLayerCnt,transform,readByteCnt);

        if(ret) {
            return ret;
        }

        if(scaleFlag) {
            scaleLayerCnt++;
        }
        overlayLayerCnt++;
    }

    if(yuvLayerCnt > mGSPNCap.max_yuvLayer_cnt) {
        GSP_ALOGI_IF(mDebugFlag, "GSPN only support one YUV layer, yuvLayerCnt:%d",yuvLayerCnt);
        return -1;
    }
    if(scaleLayerCnt > mGSPNCap.max_scaleLayer_cnt) {
        GSP_ALOGI_IF(mDebugFlag, "only support one layer scaling, scaleLayerCnt:%d",scaleLayerCnt);
        return -1;
    }
    if(overlayLayerCnt > mGSPNCap.max_layer_cnt) {
        GSP_ALOGI_IF(mDebugFlag, "total layer cnt > max_layer_cnt, total:%d",overlayLayerCnt);
        return -1;
    }

    /*bandwidth & performance check*/
    if(mOutputFormat == GSPN_DST_FMT_ARGB888
       ||mOutputFormat == GSPN_DST_FMT_RGB888) {
        writeByteCnt = mFBWidth*mFBHeight*4;
    } else {
        writeByteCnt = mFBWidth*mFBHeight*2;
    }
    if(mGSPNCap.version == 0x10) { //T8 support DDR3-1333
        /*DDR3-1333, single channel theory bandwidth 1333*64/8=10664MB/s,
          but the most common freq is 400Mhz,so theory bandwidth is 800*64/8=6400MB/s,
          DDR effective bandwidth is almost half cut, because so many master rand access.
          and DISPC/CPU/MODEM/DSP/ISP/DCAM/VSP also need some bandwidth,we can use half most.*/
        DDRBandwidth = 1677721600;//=6400*1024*1024/2/2 Byte;
    }
    if(yuvLayerCnt > 0) { // 30FPS
        /*bandwidth check*/
        if((readByteCnt+writeByteCnt)*30 > DDRBandwidth*20/33) { // GSPN execute time limit is smaller than 20ms
            GSP_ALOGI_IF(mDebugFlag, "warning: bandwidth risk, r:%d + w:%d > ddr:%d",readByteCnt,writeByteCnt,DDRBandwidth);
        }
        /*performance check*/
        if(mGSPNCap.max_throughput*20/33 < mFBWidth*mFBHeight*30) {
            GSP_ALOGI_IF(mDebugFlag, "warning: performance risk, hw:%d < need:%d",mGSPNCap.max_throughput,mFBWidth*mFBHeight);
        }
    } else { // 60FPS
        /*bandwidth check*/
        if((readByteCnt+writeByteCnt)*60 > DDRBandwidth*12/17) { // GSPN execute time limit is smaller than 12ms
            GSP_ALOGI_IF(mDebugFlag, "warning: bandwidth risk, r:%d + w:%d > ddr:%d",readByteCnt,writeByteCnt,DDRBandwidth);
        }
        /*performance check*/
        if(mGSPNCap.max_throughput*12/17 < mFBWidth*mFBHeight*60) {
            GSP_ALOGI_IF(mDebugFlag, "warning: performance risk, hw:%d < need:%d",mGSPNCap.max_throughput,mFBWidth*mFBHeight);
        }
    }

    support = true;
    return 0;
}


