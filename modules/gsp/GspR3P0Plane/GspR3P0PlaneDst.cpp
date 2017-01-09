/*
 * Copyright (C) 2010 The Android Open Source Project
 * Copyright (C) 2012-2015, The Linux Foundation. All rights reserved.
 *
 * Not a Contribution, Apache license notifications and license are retained
 * for attribution purposes only.
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

#include "GspR3P0PlaneDst.h"
#include "gralloc_priv.h"

namespace android {

GspR3P0PlaneDst::GspR3P0PlaneDst(bool async, int max_gspmmu_size,
                                 int max_gsp_bandwidth) {
  mAsync = async;
  maxGspMMUSize = max_gspmmu_size;
  maxGspBandwidth = max_gsp_bandwidth;

  ALOGI("create GspR3P0PlaneDst");
}

enum gsp_r3p0_des_layer_format GspR3P0PlaneDst::dstFormatConvert(int format) {
  enum gsp_r3p0_des_layer_format f = GSP_R3P0_DST_FMT_MAX_NUM;

  switch (format) {
    case HAL_PIXEL_FORMAT_YCbCr_420_SP:
      f = GSP_R3P0_DST_FMT_YUV420_2P;
      break;
    case HAL_PIXEL_FORMAT_YCbCr_422_SP:
      f = GSP_R3P0_DST_FMT_YUV422_2P;
      break;
    case HAL_PIXEL_FORMAT_YV12:
      f = GSP_R3P0_DST_FMT_YUV420_3P;
      break;
    case HAL_PIXEL_FORMAT_RGBA_8888:
      f = GSP_R3P0_DST_FMT_ARGB888;
      break;
    case HAL_PIXEL_FORMAT_RGB_565:
      f = GSP_R3P0_DST_FMT_RGB565;
      break;
    case HAL_PIXEL_FORMAT_RGBX_8888:
      f = GSP_R3P0_DST_FMT_RGB888;
      break;
    default:
      ALOGE("dstFormatConvert,unsupport output format:%d.", format);
      break;
  }
  ALOGI_IF(mDebugFlag, "output format: %d", format);

  return f;
}

void GspR3P0PlaneDst::reset(int flag) {
  mDebugFlag = flag;
  memset(&mConfig, 0, sizeof(struct gsp_r3p0_des_layer_user));
}

bool GspR3P0PlaneDst::checkOutputRotation(SprdHWLayer **list, int count) {
  bool result = true;
  hwc_layer_1_t *layer = NULL;
  uint32_t transform = 0;

  layer = list[0]->getAndroidLayer();
  transform = layer->transform;
  for (int i = 0; i < count; i++) {
    layer = list[i]->getAndroidLayer();
    if (layer->transform != transform) {
      result = false;
      break;
    }
  }

  return result;
}

bool GspR3P0PlaneDst::checkGspMMUSize(SprdHWLayer **list, int count) {
  bool result = true;
  hwc_layer_1_t *layer = NULL;
  struct private_handle_t *privateH = NULL;
  int i = 0;
  int needGspMmuSize = R3P0_DST_GSP_MMU_SIZE;

  i = 0;
  while (i < count) {
    layer = list[i]->getAndroidLayer();
    privateH = (struct private_handle_t *)(layer->handle);
    needGspMmuSize += privateH->size;
    ALOGI_IF(mDebugFlag, "checkGspMMUSize privateH->size=%d.", privateH->size);
    i++;
  }

  if (needGspMmuSize > maxGspMMUSize) {
    ALOGI_IF(mDebugFlag, "checkGspMMUSize exceed the max GSP_MMU size.");
    result = false;
  }

  return result;
}

bool GspR3P0PlaneDst::checkGspBandwidth(SprdHWLayer **list, int count) {
  bool result = true;
  hwc_layer_1_t *layer = NULL;
  struct private_handle_t *privateH = NULL;
  int i = 0;
  int needGspBandWidth = 0;
  struct sprdRect *srcRect = NULL;

  i = 0;
  while (i < count) {
    srcRect = list[i]->getSprdSRCRect();
    layer = list[i]->getAndroidLayer();
    privateH = (struct private_handle_t *)(layer->handle);

    if ((privateH->format == HAL_PIXEL_FORMAT_YCbCr_420_SP) ||
        (privateH->format == HAL_PIXEL_FORMAT_YCrCb_420_SP)) {
      needGspBandWidth += (srcRect->w * srcRect->h * 3 / 2);
    } else if ((privateH->format == HAL_PIXEL_FORMAT_RGBA_8888) ||
               (privateH->format == HAL_PIXEL_FORMAT_RGBX_8888)) {
      needGspBandWidth += (srcRect->w * srcRect->h * 4);
    } else if (privateH->format == HAL_PIXEL_FORMAT_RGB_565) {
      needGspBandWidth += (srcRect->w * srcRect->h * 2);
    }
    ALOGI_IF(mDebugFlag, "checkGspBandwidth needGspBandWidth=%d.",
             needGspBandWidth);

    i++;
  }

  if (needGspBandWidth > maxGspBandwidth) {
    ALOGI_IF(mDebugFlag,
             "checkGspBandwidth needGspBandWidth=%d, exceed the max gsp "
             "bandwidth size.",
             needGspBandWidth);
    result = false;
  }

  return result;
}

bool GspR3P0PlaneDst::adapt(SprdHWLayer **list, int count) {
  if (checkOutputRotation(list, count) == false) return false;

  //  if (checkGspMMUSize(list, count) == false) return false;

  //  if (checkGspBandwidth(list, count) == false) return false;

  return true;
}

void GspR3P0PlaneDst::configCommon(int wait_fd, int share_fd) {
  struct gsp_layer_user *common = &mConfig.common;

  common->type = GSP_DES_LAYER;
  common->enable = 1;
  common->wait_fd = mAsync == true ? wait_fd : -1;
  common->share_fd = share_fd;
  ALOGI_IF(mDebugFlag, "conifg dst plane enable: %d, wait_fd: %d, share_fd: %d",
           common->enable, common->wait_fd, common->share_fd);
}

void GspR3P0PlaneDst::configFormat(int format) {
  struct gsp_r3p0_des_layer_params *params = &mConfig.params;

  params->img_format = dstFormatConvert(format);
}

void GspR3P0PlaneDst::configPitch(uint32_t pitch) {
  struct gsp_r3p0_des_layer_params *params = &mConfig.params;

  params->pitch = pitch;
  ALOGI_IF(mDebugFlag, "dst plane pitch: %u", pitch);
}

void GspR3P0PlaneDst::configHeight(uint32_t height) {
  struct gsp_r3p0_des_layer_params *params = &mConfig.params;

  params->height = height;
  ALOGI_IF(mDebugFlag, "dst plane height: %u", height);
}

void GspR3P0PlaneDst::configEndian(int f, uint32_t w, uint32_t h) {
  struct gsp_r3p0_des_layer_params *params = &mConfig.params;
  struct gsp_layer_user *common = &mConfig.common;
  int format = dstFormatConvert(f);

  common->offset.uv_offset = w * h;
  common->offset.v_offset = w * h;
  if (GSP_R3P0_DST_FMT_ARGB888 == format || GSP_R3P0_DST_FMT_RGB888 == format) {
    params->endian.y_rgb_word_endn = GSP_R3P0_WORD_ENDN_1;
    params->endian.y_rgb_dword_endn = GSP_R3P0_DWORD_ENDN_0;
    params->endian.a_swap_mode = GSP_R3P0_A_SWAP_RGBA;
  }

  params->endian.uv_word_endn = GSP_R3P0_WORD_ENDN_0;

  ALOGI_IF(mDebugFlag,
           "dst plane y_word_endn: %d, uv_word_endn: %d, va_word_endn: %d",
           params->endian.y_rgb_word_endn, params->endian.uv_word_endn,
           params->endian.va_word_endn);
  ALOGI_IF(mDebugFlag, "dst plane rgb_swap_mode: %d, a_swap_mode: %d",
           params->endian.rgb_swap_mode, params->endian.a_swap_mode);
}

void GspR3P0PlaneDst::configBackGround(struct gsp_background_para bg_para) {
  struct gsp_r3p0_des_layer_params *params = &mConfig.params;
  params->bk_para.bk_enable = bg_para.bk_enable;
  params->bk_para.bk_blend_mod = bg_para.bk_blend_mod;
  params->bk_para.background_rgb = bg_para.background_rgb;
}

void GspR3P0PlaneDst::configOutputRotation(enum gsp_rot_angle rot_angle) {
  struct gsp_r3p0_des_layer_params *params = &mConfig.params;
  params->rot_angle = rot_angle;
}

void GspR3P0PlaneDst::configDither(bool enable) {
  struct gsp_r3p0_des_layer_params *params = &mConfig.params;
  params->dither_en = enable;
}

void GspR3P0PlaneDst::configCSCMode(uint8_t r2y_mod) {
  struct gsp_r3p0_des_layer_params *params = &mConfig.params;
  params->r2y_mod = r2y_mod;
}

struct gsp_r3p0_des_layer_user &GspR3P0PlaneDst::getConfig() {
  return mConfig;
}

bool GspR3P0PlaneDst::parcel(struct private_handle_t *handle, uint32_t w,
                             uint32_t h, int format, int wait_fd,
                             int32_t transform) {
  if (handle == NULL) {
    ALOGE("dst plane parcel params handle=NULL.");
    return false;
  }

  configCommon(wait_fd, handle->share_fd);

  configPitch(handle->stride);

  configHeight(handle->height);

  configFormat(format);

  configEndian(format, handle->stride, handle->height);
  if (w != (uint32_t)handle->stride || h != (uint32_t)handle->height) {
    ALOGI_IF(mDebugFlag, "dst plane cfg stride: %d, width: %d", handle->stride,
             w);
  }

  enum gsp_rot_angle rot = rotationTypeConvert(transform);
  configOutputRotation(rot);

  struct gsp_background_para bg_para;
  bg_para.bk_enable = 1;
  bg_para.bk_blend_mod = 0;
  bg_para.background_rgb.a_val = 0;
  bg_para.background_rgb.r_val = 0;
  bg_para.background_rgb.g_val = 0;
  bg_para.background_rgb.b_val = 0;
  configBackGround(bg_para);

  configCSCMode(3);

  return true;
}

}  // namespace android
