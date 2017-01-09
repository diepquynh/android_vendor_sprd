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
 ** File: GSPImpl.cpp                 DESCRIPTION                             *
 **                                   GSP class define                        *
 ******************************************************************************
 ** Author:         tianci.yin@spreadtrum.com                                 *
 *****************************************************************************/



#include "GSPImpl.h"

using namespace android;

int32_t mInitFlag;
struct gsp_capability mGsp_cap;
private_handle_t* mCopyTempBuffer;

GSPImpl::GSPImpl() : GSPBase(),
    mInitFlag(0),
	mAsyncFlag(1),
    mCopyTempBuffer(NULL)
{
    mOutputFormat = GSP_DST_FMT_YUV420_2P;
	mLastScaleWaitFd = -1;
	mLastOverlapWaitFd = -1;
    memset((void*)&mGsp_cap,0,sizeof(mGsp_cap));
	init();
}

GSPImpl::~GSPImpl()
{
    if(mCopyTempBuffer) {
        GraphicBufferAllocator::get().free((buffer_handle_t)mCopyTempBuffer);
        mCopyTempBuffer = NULL;
    }

    if(mSclTmpBuffer) {
        GraphicBufferAllocator::get().free((buffer_handle_t)mSclTmpBuffer);
        mSclTmpBuffer = NULL;
    }
    dev_close();
}

int32_t GSPImpl::init()
{
    if(mDevFd < 0) {
        dev_open();
        if(mDevFd < 0) {
            GSP_ALOGE("%s[%d] opend gsp failed \n", __func__, __LINE__);
            return GSP_HAL_KERNEL_DRIVER_NOT_EXIST;
        }
    }

    GSP_ALOGI_IF(mDebugFlag, "probe gsp device success! fd=%d.\n", mDevFd);
    return getCapability((void*)&mGsp_cap, (uint32_t)sizeof(mGsp_cap));
}

/*
func:getGSPCapability
desc: get gsp process capability from kernel, and according to fb pixel size, adjust it to a proper setting
        kernel driver can read chip id , so it can get GSP HW version, then can evaluate capability.
return: 0:success ; other failed
*/
int32_t GSPImpl::getCapability(void *pCap, uint32_t size)
{
    int32_t ret = 0;

    if(mGsp_cap.magic == CAPABILITY_MAGIC_NUMBER) {
        goto copy;
    }

    //get capability from kernel
    if(mDevFd < 0) {
        dev_open();
        if(mDevFd < 0) {
            GSP_ALOGE("%s[%d] opend gsp failed!\n", __func__, __LINE__);
            return GSP_HAL_KERNEL_DRIVER_NOT_EXIST;
        }
    }
    ret = ioctl(mDevFd, GSP_IO_GET_CAPABILITY, &mGsp_cap);
    if(0 == ret ) {
        GSP_ALOGI_IF(mDebugFlag, "ioctl GET_CAPABILITY ok.\n");

        if(mGsp_cap.magic == CAPABILITY_MAGIC_NUMBER) {
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
                 mGsp_cap.version,
                 mGsp_cap.buf_type_support,
                 mGsp_cap.max_layer_cnt,
                 mGsp_cap.video_need_copy,
                 mGsp_cap.scale_range_up,
                 mGsp_cap.blend_video_with_OSD,
                 mGsp_cap.max_layer_cnt_with_video);


    //if it's a perfect chipset, like tshark v2, can process multi-layers, we adjusting it's capability adapt to fb size.
    if(mGsp_cap.max_layer_cnt > 3) {
        int32_t fb_pixel = mFBWidth * mFBHeight;

        GSP_ALOGI_IF(mDebugFlag, "get from kernel,max:%d,maxinvideo:%d.",
                     mGsp_cap.max_layer_cnt,mGsp_cap.max_layer_cnt_with_video);
        /*
        (             ~ 240*320] support 8 layer,         ~76800
        (240*320 ~ 320*480] support 6 layer, 76800~153600
        (320*480 ~ 480*800) support 5 layer, 153600~384000
        [480*800 ~ 540*960] support 4 layer, 384000~518400
        (540*960 ~ 720*1280) support 3 layer, 518400~921600
        [720*1280 ~           ] support 2 layer, 518400~921600
        */
        if(fb_pixel <= 76800) {
            mGsp_cap.max_layer_cnt = 8;
            mGsp_cap.max_layer_cnt_with_video = 5;
        } else if(fb_pixel <= 153600) {
            mGsp_cap.max_layer_cnt = 6;
            mGsp_cap.max_layer_cnt_with_video = 5;
        } else if(fb_pixel < 384000) {
            mGsp_cap.max_layer_cnt = 5;
            mGsp_cap.max_layer_cnt_with_video = 5;
        } else if(fb_pixel <= 518400) {
            mGsp_cap.max_layer_cnt = 4;
            mGsp_cap.max_layer_cnt_with_video = 4;
        } else if(fb_pixel < 921600) {
            mGsp_cap.max_layer_cnt = 3;
            mGsp_cap.max_layer_cnt_with_video = 3;
        } else {
            mGsp_cap.max_layer_cnt = 2;
            mGsp_cap.max_layer_cnt_with_video = 2;
        }
        GSP_ALOGI_IF(mDebugFlag, "after adjust,max:%d,maxinvideo:%d.",
                     mGsp_cap.max_layer_cnt,mGsp_cap.max_layer_cnt_with_video);
    }

    //sharkl
    if(mGsp_cap.blend_video_with_OSD) {
        mOutputFormat = GSP_DST_FMT_ARGB888;
    }

copy:
    if(size >= sizeof(mGsp_cap)) {
        memcpy(pCap, (const void*)&mGsp_cap, sizeof(mGsp_cap));
    }

    GSP_ALOGI_IF(mDebugFlag, "Get Capability ok.");

    return ret;
}

int32_t GSPImpl::layer0_params_check (struct gsp_layer0_cfg_info *layer0_info)
{
    float coef_factor_w = 0.0;
    float coef_factor_h = 0.0;
    uint32_t pixel_cnt = 0x1000000;//max 16M pixel

    if(layer0_info->layer_en == 0) {
        return GSP_NO_ERR;
    }

#ifndef ALL_EVEN
    if((GSP_SRC_FMT_RGB565 < layer0_info->img_format && layer0_info->img_format<GSP_SRC_FMT_8BPP)
       && (layer0_info->clip_rect.st_x & 0x1 || layer0_info->clip_rect.st_y & 0x1
           ||layer0_info->clip_rect.rect_w & 0x1 || layer0_info->clip_rect.rect_h & 0x1)) {
        GSP_ALOGE("param check err\n");
        return GSP_HAL_PARAM_CHECK_ERR;
    }
#else
    if(layer0_info->clip_rect.st_x & 0x1
       ||layer0_info->clip_rect.st_y & 0x1
       ||layer0_info->clip_rect.rect_w & 0x1
       ||layer0_info->clip_rect.rect_h & 0x1
       ||layer0_info->des_rect.st_x & 0x1
       ||layer0_info->des_rect.st_y & 0x1
       ||layer0_info->des_rect.rect_w & 0x1
       ||layer0_info->des_rect.rect_h & 0x1) {
        GSP_ALOGE("param check err\n");
        return GSP_HAL_PARAM_CHECK_ERR;
    }
#endif

    //source check
    if((layer0_info->pitch & 0xfffff000UL)// pitch > 4095
       ||((layer0_info->clip_rect.st_x + layer0_info->clip_rect.rect_w) > layer0_info->pitch) //
       ||((layer0_info->clip_rect.st_y + layer0_info->clip_rect.rect_h) & 0xfffff000UL) // > 4095
      ) {
        GSP_ALOGE("param check err\n");
        return GSP_HAL_PARAM_CHECK_ERR;
    }
    //destination check
    if(((layer0_info->des_rect.st_x + layer0_info->des_rect.rect_w) & 0xfffff000UL) // > 4095
       ||((layer0_info->des_rect.st_y + layer0_info->des_rect.rect_h) & 0xfffff000UL) // > 4095
      ) {

        GSP_ALOGE("param check err\n");
        return GSP_HAL_PARAM_CHECK_ERR;
    }

    if(layer0_info->rot_angle >= GSP_ROT_ANGLE_MAX_NUM) {

        GSP_ALOGE("param check err\n");
        return GSP_HAL_PARAM_CHECK_ERR;
    }

    //scaling range check
    if(layer0_info->rot_angle == GSP_ROT_ANGLE_90
       ||layer0_info->rot_angle == GSP_ROT_ANGLE_270
       ||layer0_info->rot_angle == GSP_ROT_ANGLE_90_M
       ||layer0_info->rot_angle == GSP_ROT_ANGLE_270_M) {
        coef_factor_w = layer0_info->clip_rect.rect_h*1.0/layer0_info->des_rect.rect_w;
        coef_factor_h = layer0_info->clip_rect.rect_w*1.0/layer0_info->des_rect.rect_h;
        //coef_factor_w = CEIL(layer0_info->clip_rect.rect_h*100,layer0_info->des_rect.rect_w);
        //coef_factor_h = CEIL(layer0_info->clip_rect.rect_w*100,layer0_info->des_rect.rect_h);
    } else {
        coef_factor_w = layer0_info->clip_rect.rect_w*1.0/layer0_info->des_rect.rect_w;
        coef_factor_h = layer0_info->clip_rect.rect_h*1.0/layer0_info->des_rect.rect_h;
        //coef_factor_w = CEIL(layer0_info->clip_rect.rect_w*100,layer0_info->des_rect.rect_w);
        //coef_factor_h = CEIL(layer0_info->clip_rect.rect_h*100,layer0_info->des_rect.rect_h);
    }
    if(coef_factor_w < 0.25 //larger than 4 times
       ||coef_factor_h < 0.25 //larger than 4 times
       ||coef_factor_w > 16.0 //smaller than 1/16
       ||coef_factor_h > 16.0 //smaller than 1/16
       ||(coef_factor_w > 1.0 && coef_factor_h < 1.0) //one direction scaling down, the other scaling up
       ||(coef_factor_h > 1.0 && coef_factor_w < 1.0) //one direction scaling down, the other scaling up
      ) {
        GSP_ALOGE("param check err: (%dx%d)-Rot:%d->(%dx%d)\n",
                  layer0_info->clip_rect.rect_w,
                  layer0_info->clip_rect.rect_h,
                  layer0_info->rot_angle,
                  layer0_info->des_rect.rect_w,
                  layer0_info->des_rect.rect_h);

        GSP_ALOGE("param check err: coef_factor_w:%f,coef_factor_h:%f\n",coef_factor_w,coef_factor_h);
        return GSP_HAL_PARAM_CHECK_ERR;
    }
	
    if(layer0_info->clip_rect.rect_w  != layer0_info->des_rect.rect_w
            ||layer0_info->clip_rect.rect_h  !=  layer0_info->des_rect.rect_h)
    {
        if((layer0_info->clip_rect.rect_w <= 4) || (layer0_info->clip_rect.rect_h <= 4))
        {
            GSP_ALOGE("param check err: gsp scale need intput > 4x4.   Line:%d\n", __LINE__);
            return GSP_HAL_PARAM_CHECK_ERR;
        }
    }

    if((layer0_info->clip_rect.rect_h < 32) || (layer0_info->des_rect.rect_h < 32))
    {
        GSP_ALOGE("param check err: gsp blend need image layer rect height >= 32  Line:%d\n", __LINE__);
        return GSP_HAL_PARAM_CHECK_ERR;
    }
    return GSP_NO_ERR;
}


int32_t GSPImpl::layer1_params_check(struct gsp_layer1_cfg_info *layer1_info)
{
    uint32_t pixel_cnt = 0x1000000;//max 16M pixel

    if(layer1_info->layer_en == 0) {
        return GSP_NO_ERR;
    }

#ifndef ALL_EVEN
    if((GSP_SRC_FMT_RGB565 < layer1_info->img_format && layer1_info->img_format<GSP_SRC_FMT_8BPP)
       && (layer1_info->clip_rect.st_x & 0x1 || layer1_info->clip_rect.st_y & 0x1
           ||layer1_info->clip_rect.rect_w & 0x1 || layer1_info->clip_rect.rect_h & 0x1)) {
        GSP_ALOGE("param check err\n");
        return GSP_HAL_PARAM_CHECK_ERR;
    }
#else
    if(layer1_info->clip_rect.st_x & 0x1
       ||layer1_info->clip_rect.st_y & 0x1
       ||layer1_info->clip_rect.rect_w & 0x1
       ||layer1_info->clip_rect.rect_h & 0x1
       ||layer1_info->des_pos.pos_pt_x & 0x1
       ||layer1_info->des_pos.pos_pt_y & 0x1) {
        GSP_ALOGE("param check err\n");
        return GSP_HAL_PARAM_CHECK_ERR;
    }
#endif

    //source check
    if( (layer1_info->pitch & 0xf000UL)// pitch > 4095
        ||((layer1_info->clip_rect.st_x + layer1_info->clip_rect.rect_w) > layer1_info->pitch) //
        ||((layer1_info->clip_rect.st_y + layer1_info->clip_rect.rect_h) & 0xfffff000UL) // > 4095
      ) {

        GSP_ALOGE("param check err\n");
        return GSP_HAL_PARAM_CHECK_ERR;
    }

    if(layer1_info->rot_angle >= GSP_ROT_ANGLE_MAX_NUM) {

        GSP_ALOGE("param check err\n");
        return GSP_HAL_PARAM_CHECK_ERR;
    }

    return GSP_NO_ERR;
}

int32_t GSPImpl::misc_params_check(struct gsp_cfg_info *gsp_cfg_info)
{
    if((gsp_cfg_info->misc_info.gsp_clock & (~3))
       ||(gsp_cfg_info->misc_info.ahb_clock & (~3))) {
        GSP_ALOGE("param check err: gsp_clock or ahb_clock larger than 3! \n");
        return GSP_HAL_PARAM_CHECK_ERR;
    }
    if(gsp_cfg_info->layer0_info.layer_en == 1 && gsp_cfg_info->layer0_info.pallet_en == 1
       && gsp_cfg_info->layer1_info.layer_en == 1 && gsp_cfg_info->layer1_info.pallet_en == 1) {
        GSP_ALOGE("param check err: both layer pallet are enable! \n");
        return GSP_HAL_PARAM_CHECK_ERR;
    }
    return GSP_NO_ERR;
}

int32_t GSPImpl::layerdes_params_check(struct gsp_cfg_info *gsp_cfg_info)
{
    uint32_t pixel_cnt = 0x1000000;//max 16M pixel
    uint32_t max_h0 = 4096;//max 4k
    uint32_t max_h1 = 4096;//max 4k
    uint32_t max_h = 4096;//max 4k

    struct gsp_layer0_cfg_info    *layer0_info = &gsp_cfg_info->layer0_info;
    struct gsp_layer1_cfg_info    *layer1_info = &gsp_cfg_info->layer1_info;
    struct gsp_layerd_cfg_info *layer_des_info = &gsp_cfg_info->layer_des_info;

    if((layer0_info->layer_en == 0) && (layer1_info->layer_en == 0)) {

        GSP_ALOGE("param check err\n");
        return GSP_HAL_PARAM_CHECK_ERR;
    }

    if((layer_des_info->pitch & 0xfffff000UL)) { // des pitch > 4095

        GSP_ALOGE("param check err\n");
        return GSP_HAL_PARAM_CHECK_ERR;
    }
    if(layer0_info->layer_en == 1) {
        if((layer0_info->des_rect.st_x + layer0_info->des_rect.rect_w) > layer_des_info->pitch) {

            GSP_ALOGE("param check err\n");
            return GSP_HAL_PARAM_CHECK_ERR;
        }
    }

    if(layer1_info->layer_en == 1) {
        if((layer1_info->des_pos.pos_pt_x + layer1_info->clip_rect.rect_w > layer_des_info->pitch)
           &&(layer1_info->rot_angle == GSP_ROT_ANGLE_0
              ||layer1_info->rot_angle == GSP_ROT_ANGLE_180
              ||layer1_info->rot_angle == GSP_ROT_ANGLE_0_M
              ||layer1_info->rot_angle == GSP_ROT_ANGLE_180_M)) {

            GSP_ALOGE("param check err\n");
            return GSP_HAL_PARAM_CHECK_ERR;
        } else if((layer1_info->des_pos.pos_pt_x + layer1_info->clip_rect.rect_h > layer_des_info->pitch)
                  &&(layer1_info->rot_angle == GSP_ROT_ANGLE_90
                     ||layer1_info->rot_angle == GSP_ROT_ANGLE_270
                     ||layer1_info->rot_angle == GSP_ROT_ANGLE_90_M
                     ||layer1_info->rot_angle == GSP_ROT_ANGLE_270_M)) {

            GSP_ALOGE("param check err\n");
            return GSP_HAL_PARAM_CHECK_ERR;
        }
    }

    if((GSP_DST_FMT_YUV420_2P <= layer_des_info->img_format) && (layer_des_info->img_format <= GSP_DST_FMT_YUV422_2P)) { //des color is yuv
#ifdef ALL_EVEN
        if((layer0_info->des_rect.st_x & 0x01)
           ||(layer0_info->des_rect.st_y & 0x01)
           ||(layer1_info->des_pos.pos_pt_x & 0x01)
           ||(layer1_info->des_pos.pos_pt_y & 0x01)) { //des start point at odd address

            GSP_ALOGE("param check err\n");
            return GSP_HAL_PARAM_CHECK_ERR;
        }
#endif
    }

    if(layer_des_info->compress_r8_en == 1
       && layer_des_info->img_format != GSP_DST_FMT_RGB888) {

        GSP_ALOGE("param check err\n");
        return GSP_HAL_PARAM_CHECK_ERR;
    }
    return GSP_NO_ERR;
}

/*
func:gsp_hal_params_check
desc:check gsp config params before config to kernel
return:0 means success,other means failed
*/
int32_t GSPImpl::params_check(struct gsp_cfg_info *gsp_cfg_info)
{
    if(layer0_params_check(&gsp_cfg_info->layer0_info)
       ||layer1_params_check(&gsp_cfg_info->layer1_info)
       ||misc_params_check(gsp_cfg_info)
       ||layerdes_params_check(gsp_cfg_info)) {
        return GSP_HAL_PARAM_CHECK_ERR;
    }
    return GSP_NO_ERR;
}


/*
func:gsp_hal_open
desc:open GSP device
return: -1 means failed,other success
notes: a thread can't open the device again unless it close first
*/
void GSPImpl::dev_open(void)
{
    mDevFd = open("/dev/sprd_gsp", O_RDWR, 0);
    if (mDevFd < 0) {
        GSP_ALOGE("open gsp device failed!\n");
    }
}

/*
func:gsp_hal_config
desc:set GSP device config parameters
return: -1 means failed,0 success
notes:
*/
int32_t GSPImpl::dev_config(struct gsp_cfg_info *gsp_cfg_info)
{
    int32_t ret = 0;

    if(mDevFd == -1) {
        return GSP_HAL_PARAM_ERR;
    }

    //software params check
    ret = params_check(gsp_cfg_info);
    if(ret) {
        GSP_ALOGE("gsp param check err,exit without config gsp reg.\n");
        return ret;
    }

    ret = ioctl(mDevFd, GSP_IO_SET_PARAM(mAsyncFlag), gsp_cfg_info);
    if(0 == ret) { //gsp hw check params err
        GSP_ALOGI_IF(mDebugFlag, "gsp set params ok, trigger now.\n");
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
void GSPImpl::dev_close()
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
int32_t GSPImpl::setCmd(void *pCfg,int32_t cnt)
{
    int32_t ret = 0;
	struct gsp_cfg_info *cfg = NULL;
    if (pCfg == NULL || cnt == 0) {
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
	cfg = (struct gsp_cfg_info *)pCfg;
    ret = dev_config(cfg);
    if(ret) {
        GSP_ALOGE("cfg gsp failed \n");
        goto exit;
    }
	if (mAsyncFlag) {
		if (mLastOverlapWaitFd > -1) {
			close(mLastOverlapWaitFd);
			mLastOverlapWaitFd = -1;
		}
		if (mLastScaleWaitFd > -1) {
			close(mLastScaleWaitFd);
			mLastScaleWaitFd = -1;
		}

	}

exit:
    return ret;
}


/*
func:formatType_convert
desc: image raw data store format covert from andriod hal type to gsp type
return: gsp type
*/
GSP_LAYER_SRC_DATA_FMT_E GSPImpl::formatType_convert(int32_t format)
{
    switch(format) {
        case HAL_PIXEL_FORMAT_YCbCr_420_SP://0x19
            return GSP_SRC_FMT_YUV420_2P;
        case HAL_PIXEL_FORMAT_YCrCb_420_SP:
            return GSP_SRC_FMT_YUV420_2P;
        case HAL_PIXEL_FORMAT_YV12:
            return GSP_SRC_FMT_YUV420_3P;
        case HAL_PIXEL_FORMAT_RGBA_8888:
            return GSP_SRC_FMT_ARGB888;//?
        case HAL_PIXEL_FORMAT_RGBX_8888:
            return GSP_SRC_FMT_RGB888;//?
        case HAL_PIXEL_FORMAT_RGB_565:
            return GSP_SRC_FMT_RGB565;
        default:
			GSP_ALOGE("err:unknow src format:%d!",format);
            break;
    }

    return GSP_SRC_FMT_MAX_NUM;
}


/*
func:rotationType_convert
desc: rotation angle covert from andriod hal type to gsp type
return: gsp type
*/
GSP_ROT_ANGLE_E GSPImpl::rotationType_convert(int32_t angle)
{
    switch(angle) {
        case 0:
            return GSP_ROT_ANGLE_0;
        case HAL_TRANSFORM_FLIP_H:// 1
            return GSP_ROT_ANGLE_180_M;
        case HAL_TRANSFORM_FLIP_V:// 2
            return GSP_ROT_ANGLE_0_M;
        case HAL_TRANSFORM_ROT_180:// 3
            return GSP_ROT_ANGLE_180;
        case HAL_TRANSFORM_ROT_90:// 4
        default:
            return GSP_ROT_ANGLE_270;
        case (HAL_TRANSFORM_ROT_90|HAL_TRANSFORM_FLIP_H)://5
            return GSP_ROT_ANGLE_270_M;
        case (HAL_TRANSFORM_ROT_90|HAL_TRANSFORM_FLIP_V)://6
            return GSP_ROT_ANGLE_90_M;
        case HAL_TRANSFORM_ROT_270:// 7
            return GSP_ROT_ANGLE_90;
    }

    GSP_ALOGE("err:unknow src angle!");
    return GSP_ROT_ANGLE_0;
}


/*
func:need_scaling_check
desc: check the input layer need scaling or not
return: 0: don't need scaling; 1: need scaling; other: err
*/
int32_t GSPImpl::need_scaling_check(SprdHWLayer *layer)
{
    struct sprdRect *srcRect = NULL;
    struct sprdRect *dstRect = NULL;
    hwc_layer_1_t *andriod_layer = NULL;
    GSP_ROT_ANGLE_E rot = GSP_ROT_ANGLE_MAX_NUM;

    if(layer == NULL) {
        return -1;
    }

    srcRect = layer->getSprdSRCRect();
    dstRect = layer->getSprdFBRect();
    andriod_layer = layer->getAndroidLayer();
    rot = rotationType_convert(andriod_layer->transform);
    if(rot & 0x1) {
        if(srcRect->w==dstRect->h && srcRect->h==dstRect->w) {
            //ALOGI_IF(1,"util[%04d] no scaling needed",__LINE__);
            return 0;// no scaling
        }
    } else {
        if(srcRect->w==dstRect->w && srcRect->h==dstRect->h) {
            //ALOGI_IF(1,"util[%04d] no scaling needed",__LINE__);
            return 0;// no scaling
        }
    }
    return 1;
}

/*
func:RegionEqualCheck
desc: compare the layer's dst region with DstRegion.
return: 1: equal; 0: not
*/
int32_t GSPImpl::RegionEqualCheck(SprdHWLayer *pLayer,struct sprdRect *DstRegion)
{
    if(pLayer == NULL) return -1;

    struct sprdRect *Rect = pLayer->getSprdFBRect();
    /*
        GSP_ALOGI_IF(mDebugFlag, 1,"util[%04d] region compare [w%d,h%d] vs [w%d,h%d]",__LINE__,
                 Rect->w,Rect->h,
                 DstRegion->w,DstRegion->h);
    */
    if(Rect->w == DstRegion->w // dst width == fb width
       && Rect->h == DstRegion->h) {
        return 1;
    }
    return 0;
}

/*
func:RegionAreaCheck
desc:check sum of the first two layer area is big or equal dst area
warning: there must have more than one layer in list
return: 1: bigger or equal; 0: not
*/
int32_t GSPImpl::compositeAreaCheck(SprdHWLayer **LayerList,struct sprdRect *DstRegion)
{
    struct sprdRect *Rect = NULL;
    uint32_t sum_area = 0;
    uint32_t dst_area = 0;
    int32_t i = 0;

    if(LayerList == NULL|| DstRegion == NULL) return -1;

    while(i<2) {
        Rect = LayerList[i]->getSprdFBRect();
        sum_area += Rect->w * Rect->h;
        i++;
    }
    dst_area = DstRegion->w*DstRegion->h;

    //ALOGI_IF(1,"util[%04d] sum_area:%d, dst_area:%d",__LINE__,sum_area,dst_area);

    if(sum_area >= dst_area) {
        return 1;
    }
    return 0;
}

/*
func:scalingup_twice
desc:gsp can only process 1/16-4 range scaling, if scaling up range beyond this limitation,
        we can scaling up twice to achieve that. this func designed to check the judge condition.
return: 1:need scaling up twice ; other : no necessary
*/
int32_t GSPImpl::scaling_up_twice_check(struct gsp_cfg_info &gsp_cfg_info)
{
    if(gsp_cfg_info.layer0_info.layer_en == 1) { // layer0 be enabled
        if((gsp_cfg_info.layer0_info.rot_angle & 0x1) == 0) { // 0 180 degree
            if(((gsp_cfg_info.layer0_info.clip_rect.rect_w * 4) < gsp_cfg_info.layer0_info.des_rect.rect_w)
               ||((gsp_cfg_info.layer0_info.clip_rect.rect_h * 4) < gsp_cfg_info.layer0_info.des_rect.rect_h)) {
                return 1;
            }
        } else { // 90 270 degree
            if(((gsp_cfg_info.layer0_info.clip_rect.rect_w * 4) < gsp_cfg_info.layer0_info.des_rect.rect_h)
               || ((gsp_cfg_info.layer0_info.clip_rect.rect_h * 4) < gsp_cfg_info.layer0_info.des_rect.rect_w)) {
                return 1;
            }
        }
    }
    return 0;
}


int32_t GSPImpl::gen_points_from_rect(struct sprdPoint *outPoints,SprdHWLayer *layer)
{
    int32_t i = 0;
    struct sprdRect *Rect = NULL;

    if(layer == NULL || outPoints == NULL) {
        return -1;
    }

    Rect = layer->getSprdFBRect();

    outPoints[i].x = Rect->x;
    outPoints[i].y = Rect->y;

    i++;
    outPoints[i].x = Rect->x+Rect->w;
    outPoints[i].y = Rect->y;

    i++;
    outPoints[i].x = Rect->x+Rect->w;
    outPoints[i].y = Rect->y+Rect->h;

    i++;
    outPoints[i].x = Rect->x;
    outPoints[i].y = Rect->y+Rect->h;

    return 0;
}

/*
func:RegionAreaCheck
desc: the src 2 layer's dst region have 8 points, while full region have 4.
        this function used to check the 4 points is in the 8 point or not.
warning: there must have more than one layer in list
return: 1: dst 4 points in src 8 points; other : not
*/
int32_t GSPImpl::compositePointCheck(SprdHWLayer **LayerList,struct sprdRect *DstRegion)
{
    int32_t hit_cnt = 0,ret =0,i=0,j=0;

    struct sprdPoint srcPoints[8];
    struct sprdPoint dstPoints[4];
    memset((void*)srcPoints,0,sizeof(srcPoints));
    memset((void*)dstPoints,0,sizeof(dstPoints));

    ret = gen_points_from_rect(srcPoints,LayerList[0]);
    ret |= gen_points_from_rect(&srcPoints[4],LayerList[1]);
    if(ret) return ret;


    dstPoints[i].x = DstRegion->x;
    dstPoints[i].y = DstRegion->y;

    i++;
    dstPoints[i].x = DstRegion->x+DstRegion->w;
    dstPoints[i].y = DstRegion->y;

    i++;
    dstPoints[i].x = DstRegion->x+DstRegion->w;
    dstPoints[i].y = DstRegion->y+DstRegion->h;

    i++;
    dstPoints[i].x = DstRegion->x;
    dstPoints[i].y = DstRegion->y+DstRegion->h;

    i=0;
    while(i<4) {
        j=0;
        while(j<8) {
            if(dstPoints[i].x == srcPoints[j].x && dstPoints[i].y == srcPoints[j].y) {
                hit_cnt++;
                break;
            }
            j++;
        }
        i++;
    }

    if(hit_cnt == 4) {
        return 1;
    }

    return 0;
}


/*
func:findAnIndependentLayer
desc: find an independent layer, which is not overlap with any other layers
return: if find it, return the index in list, or -1
*/
int32_t GSPImpl::findAnIndependentLayer(SprdHWLayer **LayerList, int32_t cnt)
{
    struct sprdRect *candidateRect = NULL;
    struct sprdRect *otherRect = NULL;
    int32_t i=0,j=0;
    int32_t c=0;// not overlay cnt

    j=1;
    while(j<cnt) {
        candidateRect = LayerList[j]->getSprdFBRect();
        c=0;
        i=0;
        while(i<cnt) {
            if(i != j) {
                otherRect = LayerList[i]->getSprdFBRect();

                if((otherRect->x+otherRect->w <= candidateRect->x)
                   ||(otherRect->y+otherRect->h <= candidateRect->y)
                   ||(otherRect->x >= candidateRect->x+candidateRect->w)
                   ||(otherRect->y >= candidateRect->y+candidateRect->h)) {
                    c++;
                }
            }
            i++;
        }
        if(c==cnt-1) {
            return j;
        }

        j++;
    }
    return -1;
}

/*
func:full_screen_check
desc:checking the head two layers of LayerList to find if their dst region is composite full screen,
       if it's full screen, we can composite them in one gsp process.
*/
int32_t GSPImpl::full_screen_check(SprdHWLayer **LayerList, int32_t cnt)
{
    struct sprdRect DstRegion= {0,0,0,0};
    int32_t i = 0;

    if(LayerList == NULL) {
        return -1;
    }

    DstRegion.w = mFBWidth;
    DstRegion.h = mFBHeight;

    //compare the head two layer's dst region with FB region to find full screen layer out.
    while(i<2) {
        if(1==RegionEqualCheck(LayerList[i],&DstRegion)) {
            //ALOGI_IF(1,"util[%04d] exist full screen size in first two layers",__LINE__);
            return 1;
        }
        i++;
    }

    // if there is an independent layer, we adjust it to bottom of the list
    if(cnt > 2) {
        i = findAnIndependentLayer(LayerList, cnt);
        if(i != -1 && 0< i && i < cnt) { // find independent layer
            //move it to list[1]
            SprdHWLayer *tempLayer = LayerList[i];
            //ALOGI_IF(1,"util[%04d] find an independent layer, move it from list[%d] to list[1]",__LINE__,i);
            while(i>1) {
                LayerList[i] = LayerList[i-1];
                i--;
            }
            LayerList[i] = tempLayer;
        }
    }

    // to check if the first two layer's destination region can composite out a full screen region
    if(1==compositeAreaCheck(LayerList,&DstRegion)
       && 1==compositePointCheck(LayerList,&DstRegion)) {
        return 1;
    }

    return 0;
}

/*
func:gsp_intermedia_dump
desc: in gsp multi-layer case, dump gsp intermedia output
*/
void GSPImpl::gsp_intermedia_dump(private_handle_t* dst_buffer)
{
    int32_t mDumpFlag = 0;
    if(dst_buffer == NULL) return;

    queryDumpFlag(&mDumpFlag);
    if (HWCOMPOSER_DUMP_MULTI_LAYER_FLAG & mDumpFlag) {
        dumpOverlayImage(dst_buffer, "GSPMULTI");
    }
}

/*
func:gsp_split_pages_check
desc:check whether this gsp call need rot or scaling or not, to decide split_pages flag setting
return: 1: flag need set to 1   0:flag need set to 0
*/
int32_t GSPImpl::gsp_split_pages_check(struct gsp_cfg_info &gsp_cfg_info)
{
    if(gsp_cfg_info.layer0_info.layer_en == 1 && gsp_cfg_info.layer0_info.pallet_en == 0) {
        //for black line bug chip, if scaling required, split_pages should be set to 0 to avoid hunging up.
        if((gsp_cfg_info.layer0_info.rot_angle & 0x1) == 0
           &&(gsp_cfg_info.layer0_info.clip_rect.rect_w != gsp_cfg_info.layer0_info.des_rect.rect_w
              || gsp_cfg_info.layer0_info.clip_rect.rect_h != gsp_cfg_info.layer0_info.des_rect.rect_h)) {
            return 0;
        }
        if((gsp_cfg_info.layer0_info.rot_angle & 0x1) == 1
           &&(gsp_cfg_info.layer0_info.clip_rect.rect_w != gsp_cfg_info.layer0_info.des_rect.rect_h
              || gsp_cfg_info.layer0_info.clip_rect.rect_h != gsp_cfg_info.layer0_info.des_rect.rect_w)) {
            return 0;
        }

        //for YUV format, rotation also can cause hunging up
        if(gsp_cfg_info.layer0_info.img_format > GSP_SRC_FMT_RGB565 // not RGB
           && gsp_cfg_info.layer0_info.rot_angle != GSP_ROT_ANGLE_0) {
            return 0;
        }
    }

    if(gsp_cfg_info.layer1_info.layer_en == 1 && gsp_cfg_info.layer1_info.pallet_en == 0) {
        if(gsp_cfg_info.layer1_info.img_format > GSP_SRC_FMT_RGB565 // not RGB
           && gsp_cfg_info.layer1_info.rot_angle != GSP_ROT_ANGLE_0) {
            return 0;
        }
    }
    return 1;
}



int32_t GSPImpl:: acquireTmpBuffer(uint32_t width, uint32_t height, uint32_t format)
{
    uint32_t stride;
    int32_t GSPOutputFormat = -1;

    if (mSclTmpBuffer != NULL) {
        GSP_ALOGE("mSclTmpBuffer already allocated!");
        return 0;
    }

#ifdef VIDEO_LAYER_USE_RGB
    GSPOutputFormat = HAL_PIXEL_FORMAT_RGBX_8888;
#else
#ifdef GSP_OUTPUT_USE_YUV420
    GSPOutputFormat = HAL_PIXEL_FORMAT_YCbCr_420_SP;
#else
    GSPOutputFormat = HAL_PIXEL_FORMAT_YCbCr_422_SP;
#endif
#endif

	GSP_ALOGI_IF(mDebugFlag, "buffer type support: %d", mGsp_cap.buf_type_support);
    if(mGsp_cap.buf_type_support == GSP_ADDR_TYPE_PHYSICAL) {
        GraphicBufferAllocator::get().alloc(width, height, format, GRALLOC_USAGE_OVERLAY_BUFFER, (buffer_handle_t*)&mSclTmpBuffer, &stride);
    } else if(mGsp_cap.buf_type_support == GSP_ADDR_TYPE_IOVIRTUAL) {
        GraphicBufferAllocator::get().alloc(width, height, format, 0, (buffer_handle_t*)&mSclTmpBuffer, &stride);
    }

    if (mSclTmpBuffer == NULL) {
        GSP_ALOGE("err:Cannot alloc the mSclTmpBuffer ION buffer!");
        return -1;
    }

    return 0;
}


/*
func:gsp_process_va_copy2_pa
desc:copy va image buffer to pa buffer,
warning: the layer0 image must yuv420_2p, layer1 must be RGB,
         the pa buffer size is fixed in the function, can't over that size.
*/
int32_t GSPImpl::gsp_process_va_copy2_pa(struct gsp_cfg_info *pgsp_cfg_info)
{
    uint32_t VIDEO_MAX_WIDTH = 1920;
    uint32_t VIDEO_MAX_HEIGHT = 1088;

    int32_t OSD_MAX_WIDTH = 0;
    int32_t OSD_MAX_HEIGHT = 0;

    uint32_t stride = 0;
    int32_t ret = 0;
    int32_t size = 0;
    struct gsp_cfg_info cfg_info;
    void* vaddr = NULL;

    if((pgsp_cfg_info == NULL)
       ||((pgsp_cfg_info->layer0_info.layer_en == 1)&&((pgsp_cfg_info->layer1_info.layer_en == 1) && (pgsp_cfg_info->layer1_info.pallet_en == 0)))
       ||((pgsp_cfg_info->layer0_info.layer_en == 0)&&(pgsp_cfg_info->layer1_info.layer_en == 0))) {
        GSP_ALOGE("copy:params check err.");
        return -1;
    }
    if(mGsp_cap.max_video_size == 1) { // 720P
        VIDEO_MAX_WIDTH = 1280;
        VIDEO_MAX_HEIGHT = 720;
    }
    cfg_info = *pgsp_cfg_info;

    if(mFBWidth == 0 || mFBHeight == 0) {
        GSP_ALOGE("copy:mFBWidth == 0 or mFBHeight == 0, return.");
        return -1;
    }
    OSD_MAX_WIDTH = mFBWidth+2;// sometimes , surfaceflinger sends 856x480 rgb data, which is larger than fb size
    OSD_MAX_HEIGHT = mFBHeight+2;


    if((cfg_info.layer0_info.layer_en == 1)
       &&((cfg_info.layer0_info.img_format != GSP_SRC_FMT_YUV420_2P)
          ||(cfg_info.layer0_info.pitch*(cfg_info.layer0_info.clip_rect.st_y+cfg_info.layer0_info.clip_rect.rect_h) > VIDEO_MAX_WIDTH*VIDEO_MAX_HEIGHT))) {
        GSP_ALOGE("copy:format:%d is not yuv4202p or video src buffer is larger than the pa buffer!",cfg_info.layer0_info.img_format);
        return -1;
    }

    if(cfg_info.layer1_info.layer_en == 1) {
        if(cfg_info.layer1_info.pallet_en == 0) {
            if(cfg_info.layer1_info.pitch*(cfg_info.layer1_info.clip_rect.st_y+cfg_info.layer1_info.clip_rect.rect_h) > (uint32_t)(OSD_MAX_WIDTH*OSD_MAX_HEIGHT)) {
                GSP_ALOGE("copy:osd src buffer is larger than the pa buffer!");
                return -1;
            }
        } else {
            GSP_ALOGI_IF(mDebugFlag, "copy:osd is pallet, don't need copy");
            return 0;
        }
    }

    if(mCopyTempBuffer == NULL) {
        //GraphicBufferAllocator::get().alloc(mFBWidth, mFBHeight, format, GRALLOC_USAGE_OVERLAY_BUFFER, (buffer_handle_t*)&copyTempBuffer, &stride);
        if(OSD_MAX_WIDTH*OSD_MAX_HEIGHT*4 > VIDEO_MAX_WIDTH*VIDEO_MAX_HEIGHT*1.5) {
            GraphicBufferAllocator::get().alloc(OSD_MAX_WIDTH, OSD_MAX_HEIGHT, HAL_PIXEL_FORMAT_RGBA_8888, GRALLOC_USAGE_OVERLAY_BUFFER, (buffer_handle_t*)&mCopyTempBuffer, &stride);
        } else {
            GraphicBufferAllocator::get().alloc(VIDEO_MAX_WIDTH, VIDEO_MAX_HEIGHT, HAL_PIXEL_FORMAT_YCbCr_420_SP, GRALLOC_USAGE_OVERLAY_BUFFER, (buffer_handle_t*)&mCopyTempBuffer, &stride);
        }
        if (mCopyTempBuffer == NULL) {
            GSP_ALOGE("copy:mCopyTempBuffer == NULL,alloc buffer failed!");
            ret = -1;
            return ret;
        }
    }

    if(cfg_info.layer0_info.layer_en == 1) {
        // video layer : va cpy pa scaling&rotation pa
        // osd layer : disable, do not care
        cfg_info.layer1_info.layer_en = 0;
        cfg_info.layer0_info.clip_rect.rect_w = cfg_info.layer0_info.pitch;
        cfg_info.layer0_info.clip_rect.st_x = 0;
        cfg_info.layer0_info.clip_rect.rect_h += cfg_info.layer0_info.clip_rect.st_y;
        cfg_info.layer0_info.clip_rect.st_y = 0;

        cfg_info.layer_des_info.mem_info.share_fd = mCopyTempBuffer->share_fd;
        cfg_info.layer_des_info.mem_info.uv_offset = cfg_info.layer0_info.clip_rect.rect_w*cfg_info.layer0_info.clip_rect.rect_h;
        cfg_info.layer_des_info.mem_info.v_offset = cfg_info.layer_des_info.mem_info.uv_offset;

        cfg_info.layer0_info.des_rect = cfg_info.layer0_info.clip_rect;
        cfg_info.layer0_info.rot_angle = GSP_ROT_ANGLE_0;
        cfg_info.layer_des_info.img_format = (GSP_LAYER_DST_DATA_FMT_E)cfg_info.layer0_info.img_format;
        memset(&cfg_info.layer0_info.endian_mode,0,sizeof(cfg_info.layer0_info.endian_mode));
        cfg_info.layer_des_info.endian_mode = cfg_info.layer0_info.endian_mode;

        cfg_info.layer_des_info.pitch = cfg_info.layer0_info.clip_rect.rect_w;
    } else {
        // video layer : disable, do not care
        // osd layer : va cpy pa scaling&rotation pa
        cfg_info.layer0_info.layer_en = 0;
        cfg_info.layer1_info.clip_rect.rect_w = cfg_info.layer1_info.pitch;
        cfg_info.layer1_info.clip_rect.st_x = 0;
        cfg_info.layer1_info.clip_rect.rect_h += cfg_info.layer1_info.clip_rect.st_y;
        cfg_info.layer1_info.clip_rect.st_y = 0;

        cfg_info.layer_des_info.mem_info.share_fd = mCopyTempBuffer->share_fd;
        cfg_info.layer_des_info.mem_info.uv_offset = cfg_info.layer1_info.clip_rect.rect_w*cfg_info.layer1_info.clip_rect.rect_h;
        cfg_info.layer_des_info.mem_info.v_offset = cfg_info.layer_des_info.mem_info.uv_offset;

        cfg_info.layer1_info.des_pos.pos_pt_x =
            cfg_info.layer1_info.des_pos.pos_pt_y = 0;
        cfg_info.layer1_info.rot_angle = GSP_ROT_ANGLE_0;
        cfg_info.layer_des_info.img_format = (GSP_LAYER_DST_DATA_FMT_E)cfg_info.layer1_info.img_format;
        memset(&cfg_info.layer1_info.endian_mode,0,sizeof(cfg_info.layer1_info.endian_mode));
        cfg_info.layer_des_info.endian_mode = cfg_info.layer1_info.endian_mode;

        cfg_info.layer_des_info.pitch = cfg_info.layer1_info.clip_rect.rect_w;

    }

    cfg_info.misc_info.split_pages = 1;//

    GSP_ALOGI_IF(mDebugFlag, "copy:L1==%d yaddr:%08x {p%d,s%d,f%d}[x%d,y%d,w%d,h%d] r%d [x%d,y%d,w%d,h%d]",
                 cfg_info.layer0_info.layer_en,
                 cfg_info.layer0_info.src_addr.addr_y,
                 cfg_info.layer0_info.pitch,
                 0,//private_h1->height,
                 cfg_info.layer0_info.img_format,
                 cfg_info.layer0_info.clip_rect.st_x,
                 cfg_info.layer0_info.clip_rect.st_y,
                 cfg_info.layer0_info.clip_rect.rect_w,
                 cfg_info.layer0_info.clip_rect.rect_h,
                 cfg_info.layer0_info.rot_angle,
                 cfg_info.layer0_info.des_rect.st_x,
                 cfg_info.layer0_info.des_rect.st_y,
                 cfg_info.layer0_info.des_rect.rect_w,
                 cfg_info.layer0_info.des_rect.rect_h);

    GSP_ALOGI_IF(mDebugFlag, "copy:L2==%d yaddr:%08x {p%d,s%d,f%d}[x%d,y%d,w%d,h%d] r%d [x%d,y%d]",
                 cfg_info.layer1_info.layer_en,
                 cfg_info.layer1_info.src_addr.addr_y,
                 cfg_info.layer1_info.pitch,
                 0,//private_h2->height,
                 cfg_info.layer1_info.img_format,
                 cfg_info.layer1_info.clip_rect.st_x,
                 cfg_info.layer1_info.clip_rect.st_y,
                 cfg_info.layer1_info.clip_rect.rect_w,
                 cfg_info.layer1_info.clip_rect.rect_h,
                 cfg_info.layer1_info.rot_angle,
                 cfg_info.layer1_info.des_pos.pos_pt_x,
                 cfg_info.layer1_info.des_pos.pos_pt_y);

    GSP_ALOGI_IF(mDebugFlag, "copy:Ld y_addr==%08x size==%08x {p%d,s%d,f%d}!",
                 cfg_info.layer_des_info.src_addr.addr_y,
                 size,
                 cfg_info.layer_des_info.pitch,
                 0,
                 cfg_info.layer_des_info.img_format);

    ret = setCmd(&cfg_info,1);
    if(ret == 0) {
        if(cfg_info.layer0_info.layer_en == 1) {
            pgsp_cfg_info->layer0_info.mem_info = cfg_info.layer_des_info.mem_info;
        } else {
            pgsp_cfg_info->layer1_info.mem_info = cfg_info.layer_des_info.mem_info;
        }
        GSP_ALOGI_IF(mDebugFlag, "copy:setCmd succes!");
        pgsp_cfg_info->misc_info.split_pages = 0;//
    } else {
        GSP_ALOGE("copy:setCmd failed:%d!",ret);
    }
    return ret;
}

/*
func:scalingup_twice
desc:gsp can only process 1/16-4 range scaling, if scaling up range beyond this limitation,
        we can scaling up twice to achieve that. this func designed to take this job.
return: 0:success ; other failed
*/
int32_t GSPImpl::scalingup_twice(struct gsp_cfg_info &gsp_cfg_info, struct _SprdUtilTarget *Target)
{
    int32_t buffersize_layert = 0;//scaling up twice temp
    struct gsp_cfg_info gsp_cfg_info_phase1 = gsp_cfg_info;
    GSP_LAYER_DST_DATA_FMT_E phase1_des_format = GSP_DST_FMT_YUV420_2P;//GSP_DST_FMT_YUV422_2P; //GSP_DST_FMT_ARGB888
    GSP_ALOGI_IF(mDebugFlag, "scale up twice enter.");

    int32_t ret = -1;
    int32_t format = HAL_PIXEL_FORMAT_YCbCr_420_SP;
    if(mSclTmpBuffer == NULL) {
        ret = acquireTmpBuffer(mFBWidth, mFBHeight, format);
        if (ret != 0) {
            GSP_ALOGE("err:acquireTmpBuffer failed");
            return -1;
        }
    }

    /*phase1*/
    gsp_cfg_info_phase1.layer_des_info.img_format = phase1_des_format;

    GSP_ALOGI_IF(mDebugFlag, "up2:mapped temp iommu fd:%08x", mSclTmpBuffer->share_fd);

    gsp_cfg_info_phase1.layer_des_info.mem_info.share_fd = mSclTmpBuffer->share_fd;
    gsp_cfg_info_phase1.layer_des_info.mem_info.uv_offset = mFBWidth * mFBHeight;
    gsp_cfg_info_phase1.layer_des_info.mem_info.v_offset = gsp_cfg_info_phase1.layer_des_info.mem_info.uv_offset;

    gsp_cfg_info_phase1.layer_des_info.src_addr.addr_y = 0;
    gsp_cfg_info_phase1.layer_des_info.src_addr.addr_v =
        gsp_cfg_info_phase1.layer_des_info.src_addr.addr_uv =
            gsp_cfg_info_phase1.layer_des_info.src_addr.addr_y + mFBWidth * mFBHeight;

    gsp_cfg_info_phase1.layer0_info.des_rect.st_x = 0;
    gsp_cfg_info_phase1.layer0_info.des_rect.st_y = 0;
    gsp_cfg_info_phase1.layer0_info.des_rect.rect_w = gsp_cfg_info_phase1.layer0_info.clip_rect.rect_w;
    gsp_cfg_info_phase1.layer0_info.des_rect.rect_h = gsp_cfg_info_phase1.layer0_info.clip_rect.rect_h;
    if((gsp_cfg_info.layer0_info.rot_angle & 0x1) == 0) {
        if((gsp_cfg_info_phase1.layer0_info.clip_rect.rect_w * 4) < gsp_cfg_info.layer0_info.des_rect.rect_w) {
            gsp_cfg_info_phase1.layer0_info.des_rect.rect_w = ((gsp_cfg_info.layer0_info.des_rect.rect_w + 7)/4 & 0xfffe);
        }
        if((gsp_cfg_info_phase1.layer0_info.clip_rect.rect_h * 4) < gsp_cfg_info.layer0_info.des_rect.rect_h) {
            gsp_cfg_info_phase1.layer0_info.des_rect.rect_h = ((gsp_cfg_info.layer0_info.des_rect.rect_h + 7)/4 & 0xfffe);
        }
    } else {
        if((gsp_cfg_info_phase1.layer0_info.clip_rect.rect_w * 4) < gsp_cfg_info.layer0_info.des_rect.rect_h) {
            gsp_cfg_info_phase1.layer0_info.des_rect.rect_w = ((gsp_cfg_info.layer0_info.des_rect.rect_h + 7)/4 & 0xfffe);
        }
        if((gsp_cfg_info_phase1.layer0_info.clip_rect.rect_h * 4) < gsp_cfg_info.layer0_info.des_rect.rect_w) {
            gsp_cfg_info_phase1.layer0_info.des_rect.rect_h = ((gsp_cfg_info.layer0_info.des_rect.rect_w + 7)/4 & 0xfffe);
        }
    }
    gsp_cfg_info_phase1.layer_des_info.pitch = gsp_cfg_info_phase1.layer0_info.des_rect.rect_w;
    gsp_cfg_info_phase1.layer_des_info.wait_fd = (mAsyncFlag)?Target->releaseFenceFd:-1;
    gsp_cfg_info_phase1.layer_des_info.sig_fd = -1;
    gsp_cfg_info_phase1.layer0_info.rot_angle = GSP_ROT_ANGLE_0;
    gsp_cfg_info_phase1.layer1_info.layer_en = 0;//disable Layer1

    GSP_ALOGI_IF(mDebugFlag, "up2:phase 1,src_addr_y:0x%08x,des_addr_y:0x%08x",
                 gsp_cfg_info_phase1.layer0_info.src_addr.addr_y,
                 gsp_cfg_info_phase1.layer_des_info.src_addr.addr_y);

    ret = setCmd(&gsp_cfg_info_phase1,1);
    GSP_ALOGI_IF(mDebugFlag, "up2:,phase 1 after setCmd src_addr_y:0x%08x,des_addr_y:0x%08x",
                 gsp_cfg_info_phase1.layer0_info.src_addr.addr_y,
                 gsp_cfg_info_phase1.layer_des_info.src_addr.addr_y);

    if(0 == ret) {
        GSP_ALOGI_IF(mDebugFlag, "up2:phase 1,setCmd success");
    } else {
        GSP_ALOGE("up2:phase 1,setCmd failed!! debugenable = 1;");
        mDebugFlag = 1;
        return ret;
    }

    /*phase2*/
    gsp_cfg_info.layer0_info.img_format = (GSP_LAYER_SRC_DATA_FMT_E)phase1_des_format;
    gsp_cfg_info.layer0_info.clip_rect = gsp_cfg_info_phase1.layer0_info.des_rect;
    gsp_cfg_info.layer0_info.pitch = gsp_cfg_info_phase1.layer_des_info.pitch;
    gsp_cfg_info.layer0_info.src_addr = gsp_cfg_info_phase1.layer_des_info.src_addr;
    gsp_cfg_info.layer0_info.endian_mode = gsp_cfg_info_phase1.layer_des_info.endian_mode;
    gsp_cfg_info.layer0_info.mem_info = gsp_cfg_info_phase1.layer_des_info.mem_info;
    gsp_cfg_info.layer0_info.wait_fd = (mAsyncFlag)?gsp_cfg_info_phase1.layer_des_info.sig_fd:-1;
    mLastScaleWaitFd = gsp_cfg_info.layer0_info.wait_fd;
    gsp_cfg_info.layer_des_info.wait_fd = (mAsyncFlag)?gsp_cfg_info_phase1.layer_des_info.sig_fd:-1;
    gsp_cfg_info.layer_des_info.sig_fd = -1;

    GSP_ALOGI_IF(mDebugFlag, "up2:phase 2,src_addr_y:0x%08x,des_addr_y:0x%08x",
                 gsp_cfg_info.layer0_info.src_addr.addr_y,
                 gsp_cfg_info.layer_des_info.src_addr.addr_y);
    return 0;
}


int32_t GSPImpl::gsp_image_layer_config(SprdHWLayer *layer,
                                        struct gsp_cfg_info &gsp_cfg_info,
                                        struct gsp_cfg_info *pgsp_cfg_info)
{
    // parameters check
    if(layer == NULL && pgsp_cfg_info == NULL) {
        GSP_ALOGE("parameters err,return!!");
        return -1;
    }

    if(layer != NULL) {
        hwc_layer_1_t *hwcLayer = layer->getAndroidLayer();
        struct sprdRect *srcRect = layer->getSprdSRCRect();
        struct sprdRect *dstRect = layer->getSprdFBRect();
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

        if((mGsp_cap.buf_type_support == GSP_ADDR_TYPE_IOVIRTUAL)
           ||((mGsp_cap.buf_type_support == GSP_ADDR_TYPE_PHYSICAL) && (private_h->flags & private_handle_t::PRIV_FLAGS_USES_PHY))) {
            //config Video ,use GSP L0
            uint32_t pixel_cnt = private_h->stride * private_h->height;
            gsp_cfg_info.layer0_info.mem_info.share_fd = private_h->share_fd;
            gsp_cfg_info.layer0_info.mem_info.v_offset =
                gsp_cfg_info.layer0_info.mem_info.uv_offset = pixel_cnt;

            gsp_cfg_info.layer0_info.img_format = formatType_convert(private_h->format);
            switch(private_h->format) {
                case HAL_PIXEL_FORMAT_RGBA_8888:
                case HAL_PIXEL_FORMAT_RGBX_8888:
                    gsp_cfg_info.layer0_info.endian_mode.y_word_endn = GSP_WORD_ENDN_1;
                    gsp_cfg_info.layer0_info.endian_mode.a_swap_mode = GSP_A_SWAP_RGBA;
                    break;
                case HAL_PIXEL_FORMAT_YV12://YUV420_3P, Y V U
                    gsp_cfg_info.layer0_info.mem_info.uv_offset += (pixel_cnt>>2);
                    break;
                case HAL_PIXEL_FORMAT_YCrCb_420_SP:
                    gsp_cfg_info.layer0_info.endian_mode.uv_word_endn = GSP_WORD_ENDN_2;
                    break;
                case HAL_PIXEL_FORMAT_RGB_565:
                    gsp_cfg_info.layer0_info.endian_mode.rgb_swap_mode = GSP_RGB_SWP_RGB;
                    break;
                default:
                    break;
            }
            if(private_h->yuv_info == MALI_YUV_BT601_NARROW
               ||private_h->yuv_info == MALI_YUV_BT709_NARROW) {
                gsp_cfg_info.misc_info.y2r_opt = 1;
            }

            if(GSP_SRC_FMT_RGB565 < gsp_cfg_info.layer0_info.img_format && gsp_cfg_info.layer0_info.img_format<GSP_SRC_FMT_8BPP ) {
                ALOGI_IF((private_h->stride%16 | private_h->height%16),"%s[%04d] warning: buffer stride:%d or height:%d is not 16 aligned!",__func__,__LINE__,
                         private_h->stride,
                         private_h->height);
            }

            GSP_ALOGI_IF(mDebugFlag, "fd:%d u_offset:%08x,v_offset:%08x,",
                         gsp_cfg_info.layer0_info.mem_info.share_fd,
                         gsp_cfg_info.layer0_info.mem_info.uv_offset,
                         gsp_cfg_info.layer0_info.mem_info.v_offset);

            gsp_cfg_info.layer0_info.clip_rect.st_x = srcRect->x;
            gsp_cfg_info.layer0_info.clip_rect.st_y = srcRect->y;
            gsp_cfg_info.layer0_info.clip_rect.rect_w = srcRect->w;
            gsp_cfg_info.layer0_info.clip_rect.rect_h = srcRect->h;

            gsp_cfg_info.layer0_info.alpha = hwcLayer->planeAlpha;
            if(hwcLayer->blending == HWC_BLENDING_PREMULT) {
                GSP_ALOGI_IF(mDebugFlag, "err:L0 blending flag:%x!",hwcLayer->blending);
            }
            gsp_cfg_info.layer0_info.pmargb_mod = 1;
            gsp_cfg_info.layer0_info.rot_angle = rotationType_convert(hwcLayer->transform);

            gsp_cfg_info.layer0_info.des_rect.st_x = dstRect->x;
            gsp_cfg_info.layer0_info.des_rect.st_y = dstRect->y;
            //if(gsp_cfg_info.layer0_info.img_format < GSP_SRC_FMT_YUV420_2P)//for zhongjun err
            if(0) {
                //we suppose RGB not scaling
                if(gsp_cfg_info.layer0_info.rot_angle&0x1) {
                    gsp_cfg_info.layer0_info.des_rect.rect_w = gsp_cfg_info.layer0_info.clip_rect.rect_h;
                    gsp_cfg_info.layer0_info.des_rect.rect_h = gsp_cfg_info.layer0_info.clip_rect.rect_w;
                } else {
                    gsp_cfg_info.layer0_info.des_rect.rect_w = gsp_cfg_info.layer0_info.clip_rect.rect_w;
                    gsp_cfg_info.layer0_info.des_rect.rect_h = gsp_cfg_info.layer0_info.clip_rect.rect_h;
                }
            } else {
                gsp_cfg_info.layer0_info.des_rect.rect_w = dstRect->w;
                gsp_cfg_info.layer0_info.des_rect.rect_h = dstRect->h;
            }

            if(gsp_cfg_info.layer0_info.img_format < GSP_SRC_FMT_YUV420_2P) { //for check zhongjun err
                if(((gsp_cfg_info.layer0_info.rot_angle&0x1) == 1)
                   &&(gsp_cfg_info.layer0_info.clip_rect.rect_h != dstRect->w
                      ||gsp_cfg_info.layer0_info.clip_rect.rect_w != dstRect->h)) {
                    GSP_ALOGI_IF(mDebugFlag, "warning: RGB scaling!clip[w%d,h%d] rot:%d dstRect[w%d,h%d]",
                                 gsp_cfg_info.layer0_info.clip_rect.rect_w,gsp_cfg_info.layer0_info.clip_rect.rect_h,
                                 gsp_cfg_info.layer0_info.rot_angle,
                                 dstRect->w,dstRect->h);
                } else if(((gsp_cfg_info.layer0_info.rot_angle&0x1) == 0)
                          &&(gsp_cfg_info.layer0_info.clip_rect.rect_h != dstRect->h
                             ||gsp_cfg_info.layer0_info.clip_rect.rect_w != dstRect->w)) {
                    GSP_ALOGI_IF(mDebugFlag, "warning: RGB scaling!clip[w%d,h%d] rot:%d dstRect[w%d,h%d]",
                                 gsp_cfg_info.layer0_info.clip_rect.rect_w,gsp_cfg_info.layer0_info.clip_rect.rect_h,
                                 gsp_cfg_info.layer0_info.rot_angle,
                                 dstRect->w,dstRect->h);
                }
            }

            ALOGI_IF((private_h->width != private_h->stride),"%s[%04d] warning: imgLayer width %d, stride %d, not equal!",__func__, __LINE__,private_h->width, private_h->stride);
            //gsp_cfg_info.layer0_info.pitch = context->src_img.w;
            gsp_cfg_info.layer0_info.pitch = private_h->stride;
            gsp_cfg_info.layer0_info.layer_en = 1;
            /*add the fence synchronization*/
            gsp_cfg_info.layer0_info.wait_fd = (mAsyncFlag)?hwcLayer->acquireFenceFd:-1;
        } else {
            GSP_ALOGE("err:layer buffer type is not supported!");
            return -1;
        }
    } else {
        GSP_ALOGI_IF(mDebugFlag, "layer == NULL");

        if(pgsp_cfg_info == NULL || pgsp_cfg_info->layer_des_info.pitch == 0) {
            //l2 be processed in GSP layer1, GSP layer0 be used as BG if it's nessary
        } else {
            //GSP layer0 config as former output, layer1 process l2
            GSP_ALOGI_IF(mDebugFlag, "use last output as imgLayer input.");
            gsp_cfg_info.layer0_info.pitch = pgsp_cfg_info->layer_des_info.pitch;

            gsp_cfg_info.layer0_info.clip_rect.st_x = gsp_cfg_info.layer1_info.des_pos.pos_pt_x;
            gsp_cfg_info.layer0_info.clip_rect.st_y = gsp_cfg_info.layer1_info.des_pos.pos_pt_y;
            if(gsp_cfg_info.layer1_info.rot_angle & 0x1) { // 90 270 degree
                gsp_cfg_info.layer0_info.clip_rect.rect_w = gsp_cfg_info.layer1_info.clip_rect.rect_h;
                gsp_cfg_info.layer0_info.clip_rect.rect_h = gsp_cfg_info.layer1_info.clip_rect.rect_w;
            } else { // 0 180 degree
                gsp_cfg_info.layer0_info.clip_rect.rect_w = gsp_cfg_info.layer1_info.clip_rect.rect_w;
                gsp_cfg_info.layer0_info.clip_rect.rect_h = gsp_cfg_info.layer1_info.clip_rect.rect_h;
            }
            gsp_cfg_info.layer0_info.des_rect = gsp_cfg_info.layer0_info.clip_rect;

            gsp_cfg_info.layer0_info.src_addr = pgsp_cfg_info->layer_des_info.src_addr;
            gsp_cfg_info.layer0_info.mem_info = pgsp_cfg_info->layer_des_info.mem_info;
            if(pgsp_cfg_info->layer_des_info.img_format < GSP_DST_FMT_YUV422_2P) {
                gsp_cfg_info.layer0_info.img_format = (GSP_LAYER_SRC_DATA_FMT_E)pgsp_cfg_info->layer_des_info.img_format;
            } else {
                GSP_ALOGE("err:color format not supported!");
                return -1;
            }
            gsp_cfg_info.layer0_info.endian_mode = pgsp_cfg_info->layer_des_info.endian_mode;
            gsp_cfg_info.layer0_info.alpha = 0xff;
            gsp_cfg_info.layer0_info.pmargb_mod = 1;
            gsp_cfg_info.layer0_info.rot_angle = GSP_ROT_ANGLE_0;
            gsp_cfg_info.layer0_info.layer_en = 1;
            /*add the fence synchronization*/
            gsp_cfg_info.layer0_info.wait_fd = pgsp_cfg_info->layer_des_info.sig_fd;
			/*store the last overlap wait fd*/
			mLastOverlapWaitFd = gsp_cfg_info.layer0_info.wait_fd;
        }
    }

    GSP_ALOGI_IF(mDebugFlag, "imgLayer info [f%d,pm:%d,x%d,y%d,w%d,h%d,p%d] r%d [x%d,y%d,w%d,h%d], planeAlpha: %d",
                 gsp_cfg_info.layer0_info.img_format,
                 gsp_cfg_info.layer0_info.pmargb_mod,
                 gsp_cfg_info.layer0_info.clip_rect.st_x,
                 gsp_cfg_info.layer0_info.clip_rect.st_y,
                 gsp_cfg_info.layer0_info.clip_rect.rect_w,
                 gsp_cfg_info.layer0_info.clip_rect.rect_h,
                 gsp_cfg_info.layer0_info.pitch,
                 gsp_cfg_info.layer0_info.rot_angle,
                 gsp_cfg_info.layer0_info.des_rect.st_x,
                 gsp_cfg_info.layer0_info.des_rect.st_y,
                 gsp_cfg_info.layer0_info.des_rect.rect_w,
                 gsp_cfg_info.layer0_info.des_rect.rect_h,
                 gsp_cfg_info.layer0_info.alpha);
    return 0;
}


int32_t GSPImpl::gsp_osd_layer_config(SprdHWLayer *layer, struct gsp_cfg_info &gsp_cfg_info)
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
           ((mGsp_cap.buf_type_support == GSP_ADDR_TYPE_IOVIRTUAL)
            ||((mGsp_cap.buf_type_support == GSP_ADDR_TYPE_PHYSICAL) && (private_h->flags & private_handle_t::PRIV_FLAGS_USES_PHY)))) {

            gsp_cfg_info.layer1_info.rot_angle = rotationType_convert(hwcLayer->transform);

            if( gsp_cfg_info.layer1_info.rot_angle&0x1) {
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

            //config OSD,use GSP L1
            gsp_cfg_info.layer1_info.img_format = formatType_convert(private_h->format);
            switch(private_h->format) {
                case HAL_PIXEL_FORMAT_RGBA_8888:
                case HAL_PIXEL_FORMAT_RGBX_8888:
                    gsp_cfg_info.layer1_info.endian_mode.y_word_endn = GSP_WORD_ENDN_1;
                    gsp_cfg_info.layer1_info.endian_mode.a_swap_mode = GSP_A_SWAP_RGBA;
                    break;
                case HAL_PIXEL_FORMAT_YCrCb_420_SP:
                    gsp_cfg_info.layer1_info.endian_mode.uv_word_endn = GSP_WORD_ENDN_2;
                    break;
                case HAL_PIXEL_FORMAT_RGB_565:
                    gsp_cfg_info.layer1_info.endian_mode.rgb_swap_mode = GSP_RGB_SWP_BGR;
                    break;
                default:
                    break;
            }

            //ALOGI_IF(mDebugFlag," gsp_iommu[%d] mapped L2 iommu addr:%08x,size:%08x",__LINE__,gsp_cfg_info.layer1_info.src_addr.addr_y,buffersize_layer2);

            gsp_cfg_info.layer1_info.mem_info.share_fd = private_h->share_fd;
            gsp_cfg_info.layer1_info.mem_info.uv_offset =
                gsp_cfg_info.layer1_info.mem_info.v_offset = 0;
            gsp_cfg_info.layer1_info.clip_rect.st_x = srcRect->x;
            gsp_cfg_info.layer1_info.clip_rect.st_y = srcRect->y;
            gsp_cfg_info.layer1_info.clip_rect.rect_w = srcRect->w;
            gsp_cfg_info.layer1_info.clip_rect.rect_h = srcRect->h;
            gsp_cfg_info.layer1_info.des_pos.pos_pt_x = dstRect->x;
            gsp_cfg_info.layer1_info.des_pos.pos_pt_y = dstRect->y;


            // dst region check
            if(((gsp_cfg_info.layer1_info.rot_angle&0x1)==0)
               && gsp_cfg_info.layer1_info.clip_rect.rect_h+gsp_cfg_info.layer1_info.des_pos.pos_pt_y > mFBHeight) {
                GSP_ALOGE("err: osd dst region beyond dst boundary,clip_h:%d,dst_y:%d,fb_h:%d",
                          gsp_cfg_info.layer1_info.clip_rect.rect_h,
                          gsp_cfg_info.layer1_info.des_pos.pos_pt_y,
                          mFBHeight);

                gsp_cfg_info.layer1_info.clip_rect.rect_h = mFBHeight-gsp_cfg_info.layer1_info.des_pos.pos_pt_y;
            }
            if(((gsp_cfg_info.layer1_info.rot_angle&0x1)==1)
               && gsp_cfg_info.layer1_info.clip_rect.rect_w+gsp_cfg_info.layer1_info.des_pos.pos_pt_y > mFBHeight) {
                GSP_ALOGE("err: osd dst region beyond dst boundary,clip_w:%d,dst_y:%d,fb_h:%d",
                          gsp_cfg_info.layer1_info.clip_rect.rect_w,
                          gsp_cfg_info.layer1_info.des_pos.pos_pt_y,
                          mFBHeight);
                gsp_cfg_info.layer1_info.clip_rect.rect_w = mFBHeight-gsp_cfg_info.layer1_info.des_pos.pos_pt_y;
            }


            gsp_cfg_info.layer1_info.alpha = hwcLayer->planeAlpha;
            //gsp_cfg_info.layer1_info.pmargb_mod = ((hwcLayer->blending&HWC_BLENDING_PREMULT) == HWC_BLENDING_PREMULT);
            if(hwcLayer->blending == HWC_BLENDING_PREMULT/*have already pre-multiply*/
               ||hwcLayer->blending == HWC_BLENDING_COVERAGE/*coverage the dst layer*/) {
                gsp_cfg_info.layer1_info.pmargb_mod = 1;
            } else {
                GSP_ALOGE("err:L1 blending flag:%x!",hwcLayer->blending);
            }

            ALOGI_IF((private_h->width != private_h->stride),"%s[%04d] warning: osdLayer width %d, stride %d, not equal!",__func__, __LINE__,private_h->width, private_h->stride);
            //gsp_cfg_info.layer1_info.pitch = private_h->width;
            gsp_cfg_info.layer1_info.pitch = private_h->stride;

            //gsp_cfg_info.layer1_info.des_pos.pos_pt_x = gsp_cfg_info.layer1_info.des_pos.pos_pt_y = 0;
            gsp_cfg_info.layer1_info.layer_en = 1;
            gsp_cfg_info.layer1_info.wait_fd = (mAsyncFlag)?hwcLayer->acquireFenceFd:-1;

            GSP_ALOGI_IF(mDebugFlag, "osdLayer info [f%d,pm:%d,x%d,y%d,w%d,h%d,p%d] r%d [x%d,y%d], planeAlpha: %d",
                         gsp_cfg_info.layer1_info.img_format,
                         gsp_cfg_info.layer1_info.pmargb_mod,
                         gsp_cfg_info.layer1_info.clip_rect.st_x,
                         gsp_cfg_info.layer1_info.clip_rect.st_y,
                         gsp_cfg_info.layer1_info.clip_rect.rect_w,
                         gsp_cfg_info.layer1_info.clip_rect.rect_h,
                         gsp_cfg_info.layer1_info.pitch,
                         gsp_cfg_info.layer1_info.rot_angle,
                         gsp_cfg_info.layer1_info.des_pos.pos_pt_x,
                         gsp_cfg_info.layer1_info.des_pos.pos_pt_y,
                         gsp_cfg_info.layer1_info.alpha);
        } else {
            GSP_ALOGE("err:layer buffer type is not supported!");
            return -1;
        }
    } else {
        if(gsp_cfg_info.layer0_info.layer_en == 1) {
            GSP_ALOGI_IF(mDebugFlag, "osdLayer is NULL, use pallet to clean the area imgLayer not covered. ");

            gsp_cfg_info.layer1_info.grey.r_val = 0;
            gsp_cfg_info.layer1_info.grey.g_val = 0;
            gsp_cfg_info.layer1_info.grey.b_val = 0;
            gsp_cfg_info.layer1_info.clip_rect.st_x = 0;
            gsp_cfg_info.layer1_info.clip_rect.st_y = 0;

            gsp_cfg_info.layer1_info.clip_rect.rect_w = mFBWidth;
            gsp_cfg_info.layer1_info.clip_rect.rect_h = mFBHeight;
            gsp_cfg_info.layer1_info.pitch = mFBWidth;

            //the 3-plane addr should not be used by GSP
            gsp_cfg_info.layer1_info.src_addr.addr_y = gsp_cfg_info.layer0_info.src_addr.addr_y;
            gsp_cfg_info.layer1_info.src_addr.addr_uv = gsp_cfg_info.layer0_info.src_addr.addr_uv;
            gsp_cfg_info.layer1_info.src_addr.addr_v = gsp_cfg_info.layer0_info.src_addr.addr_v;

            gsp_cfg_info.layer1_info.pallet_en = 1;
            gsp_cfg_info.layer1_info.alpha = 0x0;

            gsp_cfg_info.layer1_info.rot_angle = GSP_ROT_ANGLE_0;
            gsp_cfg_info.layer1_info.des_pos.pos_pt_x = 0;
            gsp_cfg_info.layer1_info.des_pos.pos_pt_y = 0;
            gsp_cfg_info.layer1_info.layer_en = 1;
            gsp_cfg_info.layer1_info.mem_info.share_fd = -1;
            gsp_cfg_info.layer1_info.wait_fd = -1;
        } else {
            GSP_ALOGE("err:both GSP layer is invalid, return!");
            return -1;
        }
    }
    return 0;
}



int32_t GSPImpl::gsp_dst_layer_config(struct gsp_cfg_info &gsp_cfg_info,
                                      private_handle_t* dst_buffer,
                                      bool middleComposition,
                                      struct _SprdUtilTarget *Target,
                                      struct gsp_cfg_info *pgsp_cfg_info)
{

    if(dst_buffer == NULL) return -1;

    //config output
    gsp_cfg_info.layer_des_info.src_addr.addr_y =  (uint32_t)dst_buffer->phyaddr;
    gsp_cfg_info.layer_des_info.src_addr.addr_v =
        gsp_cfg_info.layer_des_info.src_addr.addr_uv =
            gsp_cfg_info.layer_des_info.src_addr.addr_y + mFBWidth * mFBHeight;
    gsp_cfg_info.layer_des_info.pitch = mFBWidth;
    if(gsp_cfg_info.layer_des_info.src_addr.addr_y == 0) {
        GSP_ALOGE("des.y_addr==0x%08x buffersize_layerd==%d!",gsp_cfg_info.layer_des_info.src_addr.addr_y,dst_buffer->size);
        return -1;
    }
    GSP_ALOGI_IF(mDebugFlag, "des.y_addr:0x%08x, size:%d",gsp_cfg_info.layer_des_info.src_addr.addr_y,dst_buffer->size);

    //when scaling need, all xywh should be even number to make sure GSP won't busy hung up
    if((GSP_SRC_FMT_YUV420_2P<=gsp_cfg_info.layer0_info.img_format &&  gsp_cfg_info.layer0_info.img_format<=GSP_SRC_FMT_YUV422_2P)
       && ((((gsp_cfg_info.layer0_info.rot_angle&0x1) == 1)
            &&(gsp_cfg_info.layer0_info.clip_rect.rect_h != gsp_cfg_info.layer0_info.des_rect.rect_w
               ||gsp_cfg_info.layer0_info.clip_rect.rect_w != gsp_cfg_info.layer0_info.des_rect.rect_h))
           ||(((gsp_cfg_info.layer0_info.rot_angle&0x1) == 0)
              &&(gsp_cfg_info.layer0_info.clip_rect.rect_h != gsp_cfg_info.layer0_info.des_rect.rect_h
                 ||gsp_cfg_info.layer0_info.clip_rect.rect_w != gsp_cfg_info.layer0_info.des_rect.rect_w)))) {
        gsp_cfg_info.layer0_info.clip_rect.st_x &= 0xfffe;
        gsp_cfg_info.layer0_info.clip_rect.st_y &= 0xfffe;
        gsp_cfg_info.layer0_info.clip_rect.rect_w &= 0xfffe;
        gsp_cfg_info.layer0_info.clip_rect.rect_h &= 0xfffe;
    }

    //for YUV format data, GSP don't support odd width/height and x/y
    if((GSP_SRC_FMT_RGB565 < gsp_cfg_info.layer0_info.img_format)
       && (gsp_cfg_info.layer0_info.img_format < GSP_SRC_FMT_8BPP)
       && mGsp_cap.yuv_xywh_even==1) { //YUV format
        gsp_cfg_info.layer0_info.clip_rect.st_x &= 0xfffe;
        gsp_cfg_info.layer0_info.clip_rect.st_y &= 0xfffe;
        gsp_cfg_info.layer0_info.clip_rect.rect_w &= 0xfffe;
        gsp_cfg_info.layer0_info.clip_rect.rect_h &= 0xfffe;
    }

    if((GSP_SRC_FMT_RGB565 < gsp_cfg_info.layer1_info.img_format )
       && (gsp_cfg_info.layer1_info.img_format < GSP_SRC_FMT_8BPP)
       && mGsp_cap.yuv_xywh_even==1) { //YUV format
        gsp_cfg_info.layer1_info.clip_rect.st_x &= 0xfffe;
        gsp_cfg_info.layer1_info.clip_rect.st_y &= 0xfffe;
        gsp_cfg_info.layer1_info.clip_rect.rect_w &= 0xfffe;
        gsp_cfg_info.layer1_info.clip_rect.rect_h &= 0xfffe;
    }

    if(mGsp_cap.buf_type_support == GSP_ADDR_TYPE_IOVIRTUAL) {
        gsp_cfg_info.misc_info.split_pages = 1;
    } else {
        gsp_cfg_info.misc_info.split_pages = 0;
    }

    if(gsp_cfg_info.layer_des_info.img_format == GSP_DST_FMT_ARGB888
       ||gsp_cfg_info.layer_des_info.img_format == GSP_DST_FMT_RGB888) {
        gsp_cfg_info.layer_des_info.endian_mode.y_word_endn = GSP_WORD_ENDN_1;
        gsp_cfg_info.layer_des_info.endian_mode.a_swap_mode = GSP_A_SWAP_RGBA;
    }
	if(gsp_cfg_info.layer1_info.pallet_en == 1) {
		gsp_cfg_info.layer0_info.pmargb_mod = 1;
	}
	gsp_cfg_info.layer_des_info.endian_mode.uv_word_endn = GSP_WORD_ENDN_0;
	if (middleComposition) {
		if (pgsp_cfg_info->layer_des_info.sig_fd > -1)
			gsp_cfg_info.layer_des_info.wait_fd = (mAsyncFlag)?pgsp_cfg_info->layer_des_info.sig_fd:-1;
		else
			gsp_cfg_info.layer_des_info.wait_fd = (mAsyncFlag)?Target->releaseFenceFd:-1;
	}
	else
		gsp_cfg_info.layer_des_info.wait_fd = (mAsyncFlag)?Target->releaseFenceFd:-1;
	gsp_cfg_info.layer_des_info.sig_fd = -1;
	GSP_ALOGI_IF(mDebugFlag, "gsp des buffer wait fd: %d", gsp_cfg_info.layer_des_info.wait_fd);
	if (middleComposition)
		GSP_ALOGI_IF(mDebugFlag, "gsp des buffer sig fd at middle composition: %d",
				pgsp_cfg_info->layer_des_info.sig_fd);
	gsp_cfg_info.layer_des_info.mem_info.share_fd = -1;
	gsp_cfg_info.layer_des_info.layer_en = 1;
	gsp_cfg_info.misc_info.async_flag = mAsyncFlag;

	GSP_ALOGI_IF(mDebugFlag, "dstLayer info [f%d,p%d] split%d",
			gsp_cfg_info.layer_des_info.img_format,
                 gsp_cfg_info.layer_des_info.pitch,
                 gsp_cfg_info.misc_info.split_pages);
    return 0;
}



/*
func:composerLayers
desc:if both layers are valid, blend them, but maybe the output image is not correct,
        because these area that are not covered by any layers maybe is random data.
        if only layer1 is valid, we blend it with background.
        if only layer2 is valid, we blend it with former output.
params:
        pgsp_cfg_info(in and out): if only layer2 is valid, we get the former output parameters from it,
                                              and after current task over, we assignment the current task config to it to pass back to caller function.

limits: layer1 and layer2 can be any color format, but if both of them need scaling, it is not supported
*/
int32_t GSPImpl::composerLayers(SprdHWLayer *l1,
                                SprdHWLayer *l2,
                                struct gsp_cfg_info *pgsp_cfg_info,
                                private_handle_t* dst_buffer,
                                GSP_LAYER_DST_DATA_FMT_E dst_format,
								struct _SprdUtilTarget *Target,
								bool middleComposition)
{
    int32_t ret = 0;
    int64_t start_time = 0;
    int64_t end_time = 0;
	hwc_layer_1_t *hwcLayer = NULL;

    struct gsp_cfg_info gsp_cfg_info;
    if(mDebugFlag) {
        start_time = getSystemTime()/1000;
    }
    if(mGsp_cap.buf_type_support == GSP_ADDR_TYPE_INVALUE) {
        GSP_ALOGE("ERR:can't get GSP address type!");
        return -1;
    }

    //layer1 must valid, layer2 can be invalid
    if (dst_buffer == NULL || (l1 == NULL && l2 == NULL)) {
        GSP_ALOGE("ERR:The output buffer is NULL or src layer is NULL!");
        return -1;
    }

    memset(&gsp_cfg_info,0,sizeof(gsp_cfg_info));

    //in botton to top order, for iterate scenario, we need l2 dst region, so we can get l1 clip region
    if(l2 != NULL && l1 == NULL && pgsp_cfg_info != NULL && pgsp_cfg_info->layer_des_info.pitch > 0) {
        ret = gsp_osd_layer_config(l2, gsp_cfg_info);
        if(ret) {
            GSP_ALOGE("gsp_osd_layer_config() return err!");
            return -1;
        }
    }
    ret = gsp_image_layer_config(l1,gsp_cfg_info, pgsp_cfg_info);
    if(ret) {
        GSP_ALOGE("gsp_image_layer_config() return err!");
        return -1;
    }

    // if layer1 config before, skip config it again
    if(gsp_cfg_info.layer1_info.pitch == 0) {
        ret = gsp_osd_layer_config(l2, gsp_cfg_info);
        if(ret) {
            GSP_ALOGE("gsp_osd_layer_config() return err!");
            return -1;
        }
    }

    if (gsp_cfg_info.layer0_info.layer_en == 1 || gsp_cfg_info.layer1_info.layer_en == 1) {
        if(dst_format != GSP_DST_FMT_MAX_NUM) {
            gsp_cfg_info.layer_des_info.img_format = dst_format;
        } else {
            if (l1 != NULL) {
                gsp_cfg_info.layer_des_info.img_format = (GSP_LAYER_DST_DATA_FMT_E)mOutputFormat;
            } else if (l2 != NULL) {
#ifndef PRIMARYPLANE_USE_RGB565
                gsp_cfg_info.layer_des_info.img_format = (GSP_LAYER_DST_DATA_FMT_E)mOutputFormat;
#else
                gsp_cfg_info.layer_des_info.img_format = GSP_DST_FMT_RGB565;
#endif
            }
        }

        ret = gsp_dst_layer_config(gsp_cfg_info, dst_buffer,middleComposition,Target,pgsp_cfg_info);
        if(ret) {
            GSP_ALOGE("gsp_dst_layer_config() return err!");
            return -1;
        }

        if(mGsp_cap.video_need_copy == 1) {
            if(mGsp_cap.buf_type_support == GSP_ADDR_TYPE_IOVIRTUAL
               && (gsp_cfg_info.layer0_info.layer_en == 1) && (gsp_cfg_info.layer0_info.img_format == GSP_SRC_FMT_YUV420_2P)) {
                ret = gsp_process_va_copy2_pa(&gsp_cfg_info);
                if(ret) {
                    GSP_ALOGE("gsp_process_va_copy2_pa() return err!");
                    return ret;
                }
            } else {
                GSP_ALOGI_IF(mDebugFlag, "don't need copy,GSPAddrType%d,layer0_en%d,layer0format%d",
                             mGsp_cap.buf_type_support,
                             gsp_cfg_info.layer0_info.layer_en,
                             gsp_cfg_info.layer0_info.img_format);
            }
        }

        if(mGsp_cap.scale_range_up == 256) {
            if(scaling_up_twice_check(gsp_cfg_info)) {
                ret = scalingup_twice(gsp_cfg_info, Target);
                if(ret) {
                    GSP_ALOGE("scalingup_twice() return err!");
                    return ret;
                }
            }
        }
        //ALOGI_IF(mDebugFlag,"util[%04d] ",__LINE__);

        gsp_cfg_info.misc_info.split_pages = 0;//set to 0, to make sure system won't crash
        if(mGsp_cap.buf_type_support == GSP_ADDR_TYPE_IOVIRTUAL) {
            if(mGsp_cap.video_need_copy == 0//don't need copying means black line issue be fixed, perfect chip
               ||1 == gsp_split_pages_check(gsp_cfg_info)) {
                gsp_cfg_info.misc_info.split_pages = 1;
            }
        }

        GSP_ALOGI_IF(mDebugFlag, "split%d",gsp_cfg_info.misc_info.split_pages);

        ret = setCmd(&gsp_cfg_info,1);
        if(pgsp_cfg_info != NULL) {
            *pgsp_cfg_info=gsp_cfg_info;
        }
        if(0 == ret) {
            GSP_ALOGI_IF(mDebugFlag, "setCmd success");
        } else {
            GSP_ALOGE("setCmd failed!");
            mDebugFlag = 1;
        }

    } else {
        GSP_ALOGE("err: both GSP layer are not well configured!");
        return -1;
    }
    if(mDebugFlag) {
        end_time = getSystemTime()/1000;
        GSP_ALOGI_IF(mDebugFlag, "single process: start:%lld, end:%lld,cost:%lld us !!\n",
                     start_time,end_time,(end_time-start_time));
    }

    return ret;
}


void GSPImpl::desFormatConvert(int format)
{
    switch(format) {
		case HAL_PIXEL_FORMAT_YCbCr_420_SP:
            mOutputFormat =  GSP_DST_FMT_YUV420_2P;
			break;
        case HAL_PIXEL_FORMAT_YCbCr_422_SP:
            mOutputFormat =  GSP_DST_FMT_YUV422_2P;
			break;
        case HAL_PIXEL_FORMAT_YCbCr_420_P:
            mOutputFormat =  GSP_DST_FMT_YUV420_3P;
			break;
        case HAL_PIXEL_FORMAT_RGBA_8888:
            mOutputFormat =  GSP_DST_FMT_ARGB888;
			break;
        case HAL_PIXEL_FORMAT_RGB_565:
			mOutputFormat =  GSP_DST_FMT_RGB565;
			break;
		default:
			GSP_ALOGE("err:unknow src format:%d!",format);
			break;
    }
	GSP_ALOGI_IF(mDebugFlag, "mOutputFormat: %d", mOutputFormat);
}

int32_t GSPImpl::composeLayerList(struct _SprdUtilSource *Source, struct _SprdUtilTarget *Target)
{
    GSP_LAYER_DST_DATA_FMT_E dst_format = GSP_DST_FMT_MAX_NUM;
    private_handle_t* dst_buffer = NULL;
    struct gsp_cfg_info gsp_cfg_info;
    int32_t head_2layer_consumed = 0;
    int32_t i = 0,ret = 0;
    int32_t process_order = 0;// 0: from bottom to top; 1:from top to bottom
    int64_t start_time = 0;
    int64_t end_time = 0;
    SprdHWLayer **LayerList = Source->LayerList;
	int32_t layerCnt = Source->LayerCount;
    /*params check*/
    if((mGsp_cap.magic != CAPABILITY_MAGIC_NUMBER)
       ||(Source == NULL || LayerList == NULL || layerCnt == 0 || Target == NULL)) {
        GSP_ALOGE("params check err!");
        return -1;
    }
    queryDebugFlag(&mDebugFlag);
    if(mDebugFlag) {
        start_time = getSystemTime()/1000;
    }
    memset((void*)&gsp_cfg_info,0,sizeof(gsp_cfg_info));
	if (Target->buffer)
	    dst_buffer = Target->buffer;
	else if (Target->buffer2)
	    dst_buffer = Target->buffer2;

    GSP_ALOGI_IF(mDebugFlag, "layerlist total:%d.",layerCnt);

    //check whether the OSD layers need scaling or not
    i=1;
    while (i < layerCnt) {
        if(need_scaling_check(LayerList[i]) == 1) {
            process_order = 1;
            break;
        }
        i++;
    }
    if(mGsp_cap.OSD_scaling == 0 && process_order == 1) {
        GSP_ALOGE("find osd layer need scaling, but GSP not support this! ");
        goto exit;
    }
	
	desFormatConvert(Target->format);

	if(process_order == 0) { // bottom to top
		if(layerCnt == 1) {
			dst_format = (GSP_LAYER_DST_DATA_FMT_E)mOutputFormat;
			GSP_ALOGI_IF(mDebugFlag, "process 0/%d layer, dst_format:%d",layerCnt,dst_format);
			//layer0 process src image, layer1 as BG, because layer0 is capable of scaling
			ret = composerLayers(LayerList[0], NULL, &gsp_cfg_info,
					dst_buffer,dst_format,Target,false);
			if(ret != 0) {
				GSP_ALOGE("composerLayers() return err!");
			}
			goto exit;
		} else {
			int32_t full = full_screen_check(LayerList,layerCnt);
			int32_t scale = need_scaling_check(LayerList[1]);

			//top layer don't need scaling && one of the two layers is dst full screen
			if((0==scale) && (1==full)) {
				GSP_ALOGI_IF(mDebugFlag, "process 1-2/%d layer, dst_format:%d",layerCnt,dst_format);
				//blend first two layers,layer0 process bottom layer, layer1 process top layer
				if(layerCnt == 2) {
					dst_format = (GSP_LAYER_DST_DATA_FMT_E)mOutputFormat;
					ret = composerLayers(LayerList[0], LayerList[1], &gsp_cfg_info,
							dst_buffer, dst_format, Target, false);
					if(ret != 0) {
						GSP_ALOGE("composerLayers() return err!");
					}
					goto exit;
				} else {
					//dst_format = GSP_DST_FMT_ARGB888;
					dst_format = (GSP_LAYER_DST_DATA_FMT_E)mOutputFormat;
					ret = composerLayers(LayerList[0], LayerList[1], &gsp_cfg_info,
							dst_buffer,dst_format,Target,true);
					if(ret != 0) {
						GSP_ALOGE("composerLayers() return err!");
						goto exit;
					}
					gsp_intermedia_dump(dst_buffer);
				}
				head_2layer_consumed=1;
			} else {
				GSP_ALOGI_IF(mDebugFlag, "the first two-layers of %d-layers can't blend  for scale:%d,full:%d",
											layerCnt, scale, full);
			}
		}


		i=0;
		if(head_2layer_consumed == 1) {
			i+=2;
		}

		//layer[0] blend with BG => R0; R0 blend with layer[1] => R1; R1 blend with layer[2] => R2; ....
		while(i<layerCnt) {
			if(i==layerCnt-1) {
				dst_format = (GSP_LAYER_DST_DATA_FMT_E)mOutputFormat;
			} else {
				//dst_format = GSP_DST_FMT_ARGB888;
				dst_format = (GSP_LAYER_DST_DATA_FMT_E)mOutputFormat;
			}

			GSP_ALOGI_IF(mDebugFlag, "process %d/%d layer, dst_format:%d",i,layerCnt,dst_format);

			if(i == 0) {
				//layer0 process src image, layer1 as BG
				ret = composerLayers(LayerList[i], NULL, &gsp_cfg_info,
						dst_buffer,dst_format,Target,false);
				if(ret != 0) {
					GSP_ALOGE("util[%04d] composerLayers() return err!layerlist[%d]",__LINE__,i);
					goto exit;
				}
			} else {
				//layer0 process last output, layer1 process new src layer
				ret = composerLayers(NULL, LayerList[i], &gsp_cfg_info,
						dst_buffer,dst_format,Target,true);
				if(ret != 0) {
					GSP_ALOGE("composerLayers() return err!layerlist[%d]",i);
					goto exit;
				}
			}
			if(i < layerCnt-1) {
				gsp_intermedia_dump(dst_buffer);
			}
			i++;
		}
	} else { // top to bottom
		GSP_ALOGE("anti-order not supported.");
	}

exit:
	if(mDebugFlag) {
		end_time = getSystemTime()/1000;
        GSP_ALOGI_IF(mDebugFlag, "%d-layers start:%lld, end:%06lld,cost:%lld us !!\n",
                     layerCnt,start_time,end_time,(end_time-start_time));
	}
	if (mAsyncFlag) {
		/*as the view of bufferqueue, target fence fd is the gsp signal fence fd*/
		if (gsp_cfg_info.layer_des_info.sig_fd > -1) {
			Target->acquireFenceFd = gsp_cfg_info.layer_des_info.sig_fd;
			Source->releaseFenceFd = dup(Target->acquireFenceFd);
		} else {
			GSP_ALOGE("des buffer signal fd loss");
			Target->acquireFenceFd = -1;
			Source->releaseFenceFd = -1;
		}
	} else {
		Source->releaseFenceFd = -1;
		Target->acquireFenceFd = -1;
	}
	GSP_ALOGI_IF(mDebugFlag, "source release fence fd: %d", Source->releaseFenceFd);
	GSP_ALOGI_IF(mDebugFlag, "target acquire fence fd: %d", Target->acquireFenceFd);
	return ret;
}


int32_t GSPImpl::prepareSingleLayer(struct sprdRect *srcRect,
                                    struct sprdRect *dstRect,
                                    hwc_layer_1_t *layer,
                                    struct private_handle_t *privateH,
                                    int32_t &scaleFlag,
                                    int32_t &yuvLayerCnt)
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

    /*
     *  Buffer address type check.
     * */
    if(mGsp_cap.buf_type_support == GSP_ADDR_TYPE_PHYSICAL
       && !(privateH->flags & private_handle_t::PRIV_FLAGS_USES_PHY)) {
        GSP_ALOGI_IF(mDebugFlag, "buf addr type is not supported.");
        return -1;
    }

    /*
     *  Source and destination rectangle size check.
     * */
    if(srcRect->w < (uint32_t)(mGsp_cap.crop_min.w)
       || srcRect->h < (uint32_t)(mGsp_cap.crop_min.h)
       || srcRect->w > (uint32_t)(mGsp_cap.crop_max.w)
       || srcRect->h > (uint32_t)(mGsp_cap.crop_max.h)
       || dstRect->w < (uint32_t)(mGsp_cap.out_min.w)
       || dstRect->h < (uint32_t)(mGsp_cap.out_min.h)
       || dstRect->w > (uint32_t)(mGsp_cap.out_max.w)
       || dstRect->h > (uint32_t)(mGsp_cap.out_max.h)
       || dstRect->w > mFBWidth // when HWC do blending by GSP, the output can't larger than LCD width and height
       || dstRect->h > mFBHeight) {
        GSP_ALOGI_IF(mDebugFlag, "clip or dst rect is not supported.");
        return -1;
    }

    /*
     *  Source layer format check.
     * */
    if (privateH->format == HAL_PIXEL_FORMAT_YV12) {
        GSP_ALOGI_IF(mDebugFlag, "YV12 is not supported.");
        return -1;
    }

    /*
    *  Source layer YUV range check.
    * */
    if (privateH->yuv_info == MALI_YUV_BT709_NARROW
        || privateH->yuv_info == MALI_YUV_BT709_WIDE) {
        GSP_ALOGI_IF(mDebugFlag, "GSP only support BT609 convert.");
        return -1;
    }


    if ((privateH->format == HAL_PIXEL_FORMAT_YCbCr_420_SP)
        ||(privateH->format == HAL_PIXEL_FORMAT_YCrCb_420_SP)) {
        if (privateH->usage & GRALLOC_USAGE_PROTECTED ) {
            GSP_ALOGI_IF(mDebugFlag, "find protected video.");
        }

        /*
        *  If yuv_xywh_even == 1, GXP do not support odd source layer.
        * */
        if (mGsp_cap.yuv_xywh_even == 1) {
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
        if (mGsp_cap.max_video_size == 1) {
            if(srcRect->w * srcRect->h > 1280 * 720) {
                GSP_ALOGI_IF(mDebugFlag, "GSP not support > 720P video.");
                return -1;
            }
        } else if(srcRect->w * srcRect->h > 1920 * 1080) {
            GSP_ALOGI_IF(mDebugFlag, "GXP not support > 1080P video.");
            return -1;
        }

        yuvLayerCnt++;
    }

    //take the rotation in consideration
    destWidth = dstRect->w;
    destHeight = dstRect->h;
    if (((layer->transform&HAL_TRANSFORM_ROT_90) == HAL_TRANSFORM_ROT_90)
        || ((layer->transform&HAL_TRANSFORM_ROT_270) == HAL_TRANSFORM_ROT_270)) {
        srcWidth = srcRect->h;
        srcHeight = srcRect->w;
    } else {
        srcWidth = srcRect->w;
        srcHeight = srcRect->h;
    }

    if(srcWidth == destWidth && srcHeight == destHeight) {
        scaleFlag = 0;
    } else {
        scaleFlag = 1;

        /*
         *  The GSP do not support scailing up and down at the same time.
         * */
        if (mGsp_cap.scale_updown_sametime == 0) {
            if(((srcWidth < destWidth) && (srcHeight > destHeight))
               || ((srcWidth > destWidth) && (srcHeight < destHeight))) {
                GSP_ALOGI_IF(mDebugFlag, "GSP not support one direction scaling down while the other scaling up!");
                return -1;
            }
        }

        /*
         *  Scaling range check.
         * */
        gxp_scaling_up_limit = mGsp_cap.scale_range_up / 16;
        gxp_scaling_down_limit = 16 / mGsp_cap.scale_range_down;

        if(gxp_scaling_up_limit * srcWidth < destWidth
           || gxp_scaling_up_limit * srcHeight < destHeight
           || gxp_scaling_down_limit * destWidth < srcWidth
           || gxp_scaling_down_limit * destHeight < srcHeight) {
            //gsp support [1/16-gsp_scaling_up_limit] scaling
            GSP_ALOGI_IF(mDebugFlag, "GSP only support 1/16-%d scaling!",gxp_scaling_up_limit);
            return -1;
        }

    }

    if((destHeight != srcHeight) || (destWidth != srcWidth))
    {
        //for scaler input > 4x4
        if((srcWidth <= 4) || (srcHeight <= 4))
        {
            GSP_ALOGI_IF(mDebugFlag,"prepareForGXP[%d], GXP do not support scaling input <= 4x4 ! %d",__LINE__);
            return -1;
        }
    }

    //for blend require the rect's height >= 32
    if((destHeight < 32) || (srcHeight < 32))
    {
        GSP_ALOGI_IF(mDebugFlag,"prepareForGXP[%d], GXP do not support blend with rect height < 32 ! %d",__LINE__);
        return -1;
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


int32_t GSPImpl::prepare(SprdHWLayer **layerList, int32_t layerCnt, bool &support)
{
    int32_t i = 0;
    int32_t ret = 0;
    int32_t scaleFlag = 0;
    int32_t yuvLayerCnt = 0;
    int32_t overlayLayerCnt = 0;
    int32_t scaleLayerCnt = 0;
    struct sprdRect *srcRect;
    struct sprdRect *dstRect;
    hwc_layer_1_t *layer = NULL;
    struct private_handle_t *privateH = NULL;

    support = false;
    if (mGsp_cap.magic != CAPABILITY_MAGIC_NUMBER || layerList == NULL || layerCnt <= 0) {
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
        ret = prepareSingleLayer(srcRect, dstRect, layer, privateH, scaleFlag, yuvLayerCnt);
        if(ret) {
            return ret;
        }

        /*
         *  Only the bottom layer of GSP support scaling.
         * */
        if((mGsp_cap.OSD_scaling == 0) && (scaleFlag == 1) && (i != 0)) {
            GSP_ALOGI_IF(mDebugFlag, "only bottom layer support scaling, i:%d",i);
            return -1;
        }
        if(scaleFlag) {
            scaleLayerCnt++;
        }
        overlayLayerCnt++;
    }

    if(yuvLayerCnt == 0) {
        if(overlayLayerCnt > mGsp_cap.max_layer_cnt) {
            GSP_ALOGI_IF(mDebugFlag, "total layer cnt > max_layer_cnt, total:%d",overlayLayerCnt);
            return -1;
        }
    } else {
        if(overlayLayerCnt > mGsp_cap.max_layer_cnt_with_video) {
            GSP_ALOGI_IF(mDebugFlag, "total layer cnt > max_layer_cnt_with_video, total:%d",overlayLayerCnt);
            return -1;
        }
    }

    if(scaleLayerCnt > mGsp_cap.max_videoLayer_cnt) {
        GSP_ALOGI_IF(mDebugFlag, "only bottom layer support scaling, scaleLayerCnt:%d",scaleLayerCnt);
        return -1;
    }

    if(((overlayLayerCnt - yuvLayerCnt) > 0)//have OSD layer
       && (yuvLayerCnt > 0)
       && (mGsp_cap.blend_video_with_OSD == 0)) {
        GSP_ALOGI_IF(mDebugFlag, "don't support video blend OSD.");
        return -1;
    }


    if(((overlayLayerCnt - yuvLayerCnt) > 0)//have OSD layer
       && (yuvLayerCnt > 0)
       && (full_screen_check(layerList, overlayLayerCnt) != 1)) {
        GSP_ALOGI_IF(mDebugFlag, "don't support video blend OSD without full screen.");
        return -1;
    }

    support = true;
    return 0;
}





