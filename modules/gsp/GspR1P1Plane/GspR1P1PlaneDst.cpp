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

#include "GspR1P1PlaneDst.h"
#include "gralloc_priv.h"

namespace android {

GspR1P1PlaneDst::GspR1P1PlaneDst(bool async) {
  mAsync = async;
  ALOGI("create GspR1P1PlaneDst");
}

enum gsp_r1p1_des_layer_format GspR1P1PlaneDst::dstFormatConvert(int format) {
  enum gsp_r1p1_des_layer_format f = GSP_DST_FMT_MAX_NUM;

  switch (format) {
    case HAL_PIXEL_FORMAT_YCbCr_420_SP:
      f = GSP_DST_FMT_YUV420_2P;
      break;
    case HAL_PIXEL_FORMAT_YCbCr_422_SP:
      f = GSP_DST_FMT_YUV422_2P;
      break;
    case HAL_PIXEL_FORMAT_YCbCr_420_P:
      f = GSP_DST_FMT_YUV420_3P;
      break;
    case HAL_PIXEL_FORMAT_RGBA_8888:
      f = GSP_DST_FMT_ARGB888;
      break;
    case HAL_PIXEL_FORMAT_RGB_565:
      f = GSP_DST_FMT_RGB565;
      break;
    default:
      ALOGE("err:unknow output format:%d!", format);
      break;
  }
  ALOGI_IF(mDebugFlag, "output format: %d", format);

  return f;
}

void GspR1P1PlaneDst::reset(int flag) {
  mDebugFlag = flag;
  memset(&mConfig, 0, sizeof(struct gsp_r1p1_des_layer_user));
}

void GspR1P1PlaneDst::configCommon(int wait_fd, int share_fd) {
  struct gsp_layer_user *common = &mConfig.common;

  common->type = GSP_DES_LAYER;
  common->enable = 1;
  common->wait_fd = mAsync == true ? wait_fd : -1;
  common->share_fd = share_fd;
  ALOGI_IF(mDebugFlag, "conifg dst plane enable: %d, wait_fd: %d, share_fd: %d",
           common->enable, common->wait_fd, common->share_fd);
}

void GspR1P1PlaneDst::configFormat(int format) {
  struct gsp_r1p1_des_layer_params *params = &mConfig.params;

  params->img_format = dstFormatConvert(format);
}

void GspR1P1PlaneDst::configPitch(uint32_t pitch) {
  struct gsp_r1p1_des_layer_params *params = &mConfig.params;

  params->pitch = pitch;
  ALOGI_IF(mDebugFlag, "dst plane pitch: %u", pitch);
}

void GspR1P1PlaneDst::configEndian(int f, uint32_t w, uint32_t h) {
  struct gsp_r1p1_des_layer_params *params = &mConfig.params;
  struct gsp_layer_user *common = &mConfig.common;
  int format = dstFormatConvert(f);

  common->offset.uv_offset = w * h;
  if (GSP_DST_FMT_ARGB888 == format || GSP_DST_FMT_RGB888 == format) {
    params->endian.y_word_endn = GSP_WORD_ENDN_1;
    params->endian.a_swap_mode = GSP_A_SWAP_RGBA;
  }

  params->endian.uv_word_endn = GSP_WORD_ENDN_0;

  ALOGI_IF(mDebugFlag,
           "dst plane y_word_endn: %d, uv_word_endn: %d, va_word_endn: %d",
           params->endian.y_word_endn, params->endian.uv_word_endn,
           params->endian.va_word_endn);
  ALOGI_IF(mDebugFlag, "dst plane rgb_swap_mode: %d, a_swap_mode: %d",
           params->endian.rgb_swap_mode, params->endian.a_swap_mode);
}

struct gsp_r1p1_des_layer_user &GspR1P1PlaneDst::getConfig() {
  return mConfig;
}

bool GspR1P1PlaneDst::parcel(struct private_handle_t *handle, uint32_t w,
                          uint32_t h, int format, int wait_fd) {
  if (handle == NULL) {
    ALOGE("dst plane parcel params error");
    return false;
  }

  configCommon(wait_fd, handle->share_fd);

  configPitch(w);

  configFormat(format);

  configEndian(format, w, h);

  return true;
}

}  // namespace android
