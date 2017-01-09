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

#include "GspR1P1PlaneOsd.h"
#include "gralloc_priv.h"
#include "SprdHWLayer.h"
#include "gsp_r1p1_cfg.h"

namespace android {

GspR1P1PlaneOsd::GspR1P1PlaneOsd(bool async, const GspRangeSize &range) {
  mIndex = -1;
  mAttached = false;
  mAsync = async;
  mRangeSize = range;
  mRangeSize.print();
  ALOGI("create GspR1P1PlaneOsd");
}

void GspR1P1PlaneOsd::reset(int flag) {
  mAttached = false;
  mIndex = -1;
  mDebugFlag = flag;
  memset(&mConfig, 0, sizeof(struct gsp_r1p1_osd_layer_user));
}

bool GspR1P1PlaneOsd::adapt(SprdHWLayer *layer, int index) {
  struct sprdRect *srcRect = layer->getSprdSRCRect();
  struct sprdRect *dstRect = layer->getSprdFBRect();
  hwc_layer_1_t *hwcLayer = layer->getAndroidLayer();
  struct private_handle_t *handle =
      (struct private_handle_t *)(hwcLayer->handle);
  mali_gralloc_yuv_info yuv_info = handle->yuv_info;
  enum gsp_addr_type bufType = bufTypeConvert(handle->flags);
  enum gsp_r1p1_src_layer_format format = srcFormatConvert(handle->format);
  enum gsp_rot_angle rot = rotationTypeConvert(hwcLayer->transform);

  ALOGI_IF(mDebugFlag, "osd plane start to adapt");
  if (isAttached() == true) return false;

  if (isVideoLayer(format) == true) return false;

  if (checkRangeSize(srcRect, dstRect, mRangeSize) == false) return false;

  if (needScale(srcRect, dstRect, rot) == true) return false;

  if (checkBlending(hwcLayer) == false) return false;

  attached(index);
  ALOGI_IF(mDebugFlag, "osd plane attached: %d", index);

  return true;
}

void GspR1P1PlaneOsd::configCommon(int wait_fd, int share_fd) {
  struct gsp_layer_user *common = &mConfig.common;

  common->type = GSP_OSD_LAYER;
  common->enable = 1;
  common->wait_fd = mAsync == true ? wait_fd : -1;
  common->share_fd = share_fd;
  ALOGI_IF(mDebugFlag, "config osd plane enable: %d, wait_fd: %d, share_fd: %d",
           common->enable, common->wait_fd, common->share_fd);
}

void GspR1P1PlaneOsd::configRotation(enum gsp_rot_angle rot) {
  struct gsp_r1p1_osd_layer_params *params = &mConfig.params;

  params->rot_angle = rot;
  ALOGI_IF(mDebugFlag, "config osd plane rotation angle: %d", rot);
}

void GspR1P1PlaneOsd::configSize(struct sprdRect *srcRect,
                              struct sprdRect *dstRect, uint32_t pitch) {
  struct gsp_r1p1_osd_layer_params *params = &mConfig.params;

  // configure source clip size
  params->clip_rect.st_x = srcRect->x;
  params->clip_rect.st_y = srcRect->y;
  params->clip_rect.rect_w = srcRect->w;
  params->clip_rect.rect_h = srcRect->h;

  params->des_pos.pt_x = dstRect->x;
  params->des_pos.pt_y = dstRect->y;

  params->pitch = pitch;

  ALOGI_IF(mDebugFlag, "config osd plane pitch: %u", pitch);
  ALOGI_IF(mDebugFlag, "config osd plane clip rect[%d %d %d %d] dst pos[%d %d]",
           params->clip_rect.st_x, params->clip_rect.st_y,
           params->clip_rect.rect_w, params->clip_rect.rect_h,
           params->des_pos.pt_x, params->des_pos.pt_y);
}

void GspR1P1PlaneOsd::configSize(uint32_t w, uint32_t h) {
  struct gsp_r1p1_osd_layer_params *params = &mConfig.params;

  params->clip_rect.st_x = 0;
  params->clip_rect.st_y = 0;
  params->clip_rect.rect_w = w;
  params->clip_rect.rect_h = h;

  params->des_pos.pt_x = 0;
  params->des_pos.pt_y = 0;

  params->pitch = w;

  ALOGI_IF(mDebugFlag, "config osd plane pitch: %u", w);
  ALOGI_IF(mDebugFlag, "config osd plane clip rect[%d %d %d %d] dst pos[%d %d]",
           params->clip_rect.st_x, params->clip_rect.st_y,
           params->clip_rect.rect_w, params->clip_rect.rect_h,
           params->des_pos.pt_x, params->des_pos.pt_y);
}

void GspR1P1PlaneOsd::configFormat(enum gsp_r1p1_src_layer_format format) {
  struct gsp_r1p1_osd_layer_params *params = &mConfig.params;

  params->osd_format = format;
  ALOGI_IF(mDebugFlag, "config osd plane format: %d", format);
}

void GspR1P1PlaneOsd::configPmargbMode(hwc_layer_1_t *layer) {
  struct gsp_r1p1_osd_layer_params *params = &mConfig.params;

  if (layer->blending == HWC_BLENDING_PREMULT)
    params->pmargb_mod = 0;
  else
    params->pmargb_mod = 1;

  ALOGI_IF(mDebugFlag, "config osd plane pmargb mode: %d", params->pmargb_mod);
}

void GspR1P1PlaneOsd::configAlpha(uint8_t alpha) {
  struct gsp_r1p1_osd_layer_params *params = &mConfig.params;

  params->alpha = alpha;
  ALOGI_IF(mDebugFlag, "config osd plane apha: %d", alpha);
}

void GspR1P1PlaneOsd::configEndian(struct private_handle_t *handle) {
  struct gsp_r1p1_osd_layer_params *params = &mConfig.params;
  struct gsp_layer_user *common = &mConfig.common;
  int format = handle->format;
  common->offset.v_offset = common->offset.uv_offset = 0;

  switch (format) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_RGBX_8888:
      params->endian.y_word_endn = GSP_WORD_ENDN_1;
      params->endian.a_swap_mode = GSP_A_SWAP_RGBA;
      break;
    case HAL_PIXEL_FORMAT_RGB_565:
      params->endian.rgb_swap_mode = GSP_RGB_SWP_RGB;
      break;
    default:
      ALOGE("endian from error format");
      break;
  }

  ALOGI_IF(mDebugFlag,
           "osd plane y_word_endn: %d, uv_word_endn: %d, va_word_endn: %d",
           params->endian.y_word_endn, params->endian.uv_word_endn,
           params->endian.va_word_endn);
  ALOGI_IF(mDebugFlag, "osd plane rgb_swap_mode: %d, a_swap_mode: %d",
           params->endian.rgb_swap_mode, params->endian.a_swap_mode);
}

void GspR1P1PlaneOsd::configPallet(int enable) {
  struct gsp_r1p1_osd_layer_params *params = &mConfig.params;

  params->pallet_en = enable;
}

struct gsp_r1p1_osd_layer_user &GspR1P1PlaneOsd::getConfig() {
  return mConfig;
}

bool GspR1P1PlaneOsd::parcel(SprdHWLayer *layer) {
  if (layer == NULL) {
    ALOGE("osd plane parcel params error");
    return false;
  }

  ALOGI_IF(mDebugFlag, "osd plane start to parcel");
  hwc_layer_1_t *hwcLayer = layer->getAndroidLayer();
  enum gsp_rot_angle rot = rotationTypeConvert(hwcLayer->transform);
  // configure rotation
  configRotation(rot);

  struct sprdRect *srcRect = layer->getSprdSRCRect();
  struct sprdRect *dstRect = layer->getSprdFBRect();
  struct private_handle_t *handle =
      (struct private_handle_t *)(hwcLayer->handle);
  uint32_t pitch = handle->stride;
  // configure clip size and dst size
  configSize(srcRect, dstRect, pitch);

  enum gsp_r1p1_src_layer_format format = srcFormatConvert(handle->format);
  // configure format
  configFormat(format);
  // configure pmargb mode
  configPmargbMode(hwcLayer);

  int acquireFenceFd = hwcLayer->acquireFenceFd;
  int share_fd = handle->share_fd;
  // configure acquire fence fd and dma buffer share fd
  configCommon(acquireFenceFd, share_fd);

  uint8_t alpha = hwcLayer->planeAlpha;
  // configure alpha
  configAlpha(alpha);

  configEndian(handle);

  configPallet(0);

  return true;
}

bool GspR1P1PlaneOsd::parcel(uint32_t w, uint32_t h) {
  configPallet(1);

  configCommon(-1, -1);

  configRotation(GSP_ROT_ANGLE_0);

  configSize(w, h);

  return true;
}

}  // namespace android
