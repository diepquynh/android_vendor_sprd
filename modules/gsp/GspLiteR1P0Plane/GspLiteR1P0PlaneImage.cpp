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

#include <hardware/hwcomposer_defs.h>
#include "GspLiteR1P0Plane.h"
#include "GspLiteR1P0PlaneArray.h"
#include "gsp_cfg.h"
#include "gsp_lite_r1p0_cfg.h"

namespace android {

GspLiteR1P0PlaneImage::GspLiteR1P0PlaneImage(bool async, bool is_need_even,
                                             bool support_up_down,
                                             bool videoSize, uint16_t up,
                                             uint16_t down,
                                             const GspRangeSize &range) {
  mAttached = false;
  mIndex = -1;
  mAsync = async;
  mOddSupport = is_need_even ? false : true;
  mScale_Updown_Sametime = support_up_down ? true : false;
  mVideoMaxSize = videoSize;
  mRangeSize = range;
  mScaleRangeUp = up;
  mScaleRangeDown = down;
  ALOGI("create GspLiteR1P0PlaneImage");
  mRangeSize.print();
  ALOGI("scale range up: %d, scale range down: %d", mScaleRangeUp,
        mScaleRangeDown);
  ALOGI("odd support: %d", mOddSupport);
  ALOGI("up_down_sametime support: %d", mScale_Updown_Sametime);
  ALOGI("vidoe max size: %d", mVideoMaxSize);
}

bool GspLiteR1P0PlaneImage::checkYuvInfo(mali_gralloc_yuv_info info) {
  bool result = true;

  // source layer YUV range check.
  if (info == MALI_YUV_BT709_NARROW || info == MALI_YUV_BT709_WIDE) {
    ALOGI_IF(mDebugFlag, "only support BT609 convert.");
    result = false;
  }

  return result;
}

bool GspLiteR1P0PlaneImage::checkVideoSize(struct sprdRect *srcRect) {
  bool result = true;

  // source video size check.
  if (mVideoMaxSize == true) {
    if (srcRect->w * srcRect->h > 1280 * 720) {
      ALOGI_IF(mDebugFlag, "not support > 720P video.");
      result = false;
    }
  } else if (srcRect->w * srcRect->h > 1920 * 1080) {
    ALOGI_IF(mDebugFlag, "not support > 1080P video.");
    result = false;
  }

  return result;
}

bool GspLiteR1P0PlaneImage::checkOddBoundary(
    struct sprdRect *srcRect, enum gsp_lite_r1p0_img_layer_format format) {
  bool result = true;

  // if yuv_xywh_even == 1, gsp do not support odd source layer.
  if (isVideoLayer(format) == true && mOddSupport == false) {
    if ((srcRect->x & 0x1) || (srcRect->y & 0x1) || (srcRect->w & 0x1) ||
        (srcRect->h & 0x1)) {
      ALOGD_IF(mDebugFlag, "do not support odd source layer xywh.");
      result = false;
    }
  }

  return result;
}

bool GspLiteR1P0PlaneImage::needScaleUpDown(struct sprdRect *srcRect,
                                            struct sprdRect *dstRect,
                                            enum gsp_rot_angle rot) {
  bool result = false;
  uint32_t srcw = 0;
  uint32_t srch = 0;
  uint32_t dstw = dstRect->w;
  uint32_t dsth = dstRect->h;

  if (isLandScapeTransform(rot) == true) {
    srcw = srcRect->h;
    srch = srcRect->w;
  } else {
    srcw = srcRect->w;
    srch = srcRect->h;
  }

  if ((mScale_Updown_Sametime == false) &&
      ((srcw < dstw && srch > dsth) || (srcw > dstw && srch < dsth))) {
    ALOGI_IF(mDebugFlag,
             "need scale up and down at same time, which not support");
    result = true;
  }

  return result;
}

bool GspLiteR1P0PlaneImage::checkScaleSize(struct sprdRect *srcRect,
                                           struct sprdRect *dstRect,
                                           enum gsp_rot_angle rot) {
  bool result = true;

  uint16_t scaleUpLimit = mScaleRangeUp / 16;
  uint16_t scaleDownLimit = 16 / mScaleRangeDown;

  uint32_t srcw = 0;
  uint32_t srch = 0;
  uint32_t dstw = dstRect->w;
  uint32_t dsth = dstRect->h;

  if (isLandScapeTransform(rot) == true) {
    srcw = srcRect->h;
    srch = srcRect->w;
  } else {
    srcw = srcRect->w;
    srch = srcRect->h;
  }

  if (scaleUpLimit * srcw < dstw || scaleUpLimit * srch < dsth ||
      scaleDownLimit * dstw < srcw || scaleDownLimit * dsth < srch) {
    // gsp support [1/16-gsp_scaling_up_limit] scaling
    ALOGI_IF(mDebugFlag, "GSP only support 1/16-%d scaling!", scaleUpLimit);
    result = false;
  }

  return result;
}

bool GspLiteR1P0PlaneImage::checkScale(struct sprdRect *srcRect,
                                       struct sprdRect *dstRect,
                                       enum gsp_rot_angle rot) {
  if (needScaleUpDown(srcRect, dstRect, rot) == true) return false;

  if (checkScaleSize(srcRect, dstRect, rot) == false) return false;

  return true;
}

bool GspLiteR1P0PlaneImage::checkYuvType(mali_gralloc_yuv_info yuv_info) {
  bool result = true;

  if (yuv_info == MALI_YUV_BT709_NARROW || yuv_info == MALI_YUV_BT709_WIDE) {
    ALOGI_IF(mDebugFlag, "gsp not support BT609 convert");
    result = false;
  }

  return result;
}

void GspLiteR1P0PlaneImage::reset(int flag) {
  mAttached = false;
  mIndex = -1;
  mDebugFlag = flag;
  memset(&mConfig, 0, sizeof(struct gsp_lite_r1p0_img_layer_user));
}

bool GspLiteR1P0PlaneImage::adapt(SprdHWLayer *layer, int LayerIndex) {
  struct sprdRect *srcRect = layer->getSprdSRCRect();
  struct sprdRect *dstRect = layer->getSprdFBRect();
  hwc_layer_1_t *hwcLayer = layer->getAndroidLayer();
  struct private_handle_t *handle = (struct private_handle_t *)hwcLayer->handle;
  mali_gralloc_yuv_info yuv_info = handle->yuv_info;
  enum gsp_addr_type bufType = bufTypeConvert(handle->flags);
  enum gsp_lite_r1p0_img_layer_format format = imgFormatConvert(handle->format);
  enum gsp_rot_angle rot = rotationTypeConvert(hwcLayer->transform);

  ALOGI_IF(mDebugFlag, "image plane start to adapt");
  if (isAttached() == true) return false;

  if (isProtectedVideo(handle) == true) return false;

  if (checkRangeSize(srcRect, dstRect, mRangeSize) == false) return false;

  // if (checkYuvType(yuv_info) == false) return false;

  // if (checkVideoSize(srcRect) == false) return false;

  if (checkOddBoundary(srcRect, format) == false) return false;

  if (checkScale(srcRect, dstRect, rot) == false) return false;

  if (checkBlending(hwcLayer) == false) return false;

  attached(LayerIndex);
  ALOGI_IF(mDebugFlag, "image plane attached, layer index: %d", LayerIndex);

  return true;
}

void GspLiteR1P0PlaneImage::configCommon(int wait_fd, int share_fd) {
  struct gsp_layer_user *common = &mConfig.common;

  common->type = GSP_IMG_LAYER;
  common->enable = 1;
  common->wait_fd = mAsync == true ? wait_fd : -1;
  common->share_fd = share_fd;
  ALOGI_IF(mDebugFlag,
           "conifg image plane enable: %d, wait_fd: %d, share_fd: %d",
           common->enable, common->wait_fd, common->share_fd);
}

void GspLiteR1P0PlaneImage::configScaleEnable(struct sprdRect *srcRect,
                                              struct sprdRect *dstRect,
                                              enum gsp_rot_angle rot) {
  struct gsp_lite_r1p0_img_layer_params *params = &mConfig.params;

  if (needScale(srcRect, dstRect, rot) == true) {
    params->scaling_en = 1;
  }
}

void GspLiteR1P0PlaneImage::configSize(
    struct sprdRect *srcRect, struct sprdRect *dstRect, uint32_t pitch,
    uint32_t height, enum gsp_lite_r1p0_img_layer_format format,
    enum gsp_rot_angle rot) {
  struct gsp_lite_r1p0_img_layer_params *params = &mConfig.params;

  // configure source clip size
  params->clip_rect.st_x = srcRect->x;
  params->clip_rect.st_y = srcRect->y;
  params->clip_rect.rect_w = srcRect->w;
  params->clip_rect.rect_h = srcRect->h;

  params->des_rect.st_x = dstRect->x;
  params->des_rect.st_y = dstRect->y;
  params->des_rect.rect_w = dstRect->w;
  params->des_rect.rect_h = dstRect->h;

  params->pitch = pitch;
  params->height = height;

  if (isVideoLayer(format) == true && mOddSupport == false) {
    // image plane not support yuv format osdd size
    if (needScale(srcRect, dstRect, rot) == true) {
      params->clip_rect.st_x &= 0xfffe;
      params->clip_rect.st_y &= 0xfffe;
      params->clip_rect.rect_w &= 0xfffe;
      params->clip_rect.rect_h &= 0xfffe;
    }
  }

  ALOGI_IF(mDebugFlag, "config image plane pitch: %u, height: %d ", pitch,
           height);
  ALOGI_IF(mDebugFlag,
           "config image plane clip rect[%d %d %d %d] dst rect[%d %d %d %d]",
           params->clip_rect.st_x, params->clip_rect.st_y,
           params->clip_rect.rect_w, params->clip_rect.rect_h,
           params->des_rect.st_x, params->des_rect.st_y,
           params->des_rect.rect_w, params->des_rect.rect_h);
}

void GspLiteR1P0PlaneImage::configFormat(
    enum gsp_lite_r1p0_img_layer_format format) {
  struct gsp_lite_r1p0_img_layer_params *params = &mConfig.params;

  params->img_format = format;
  ALOGI_IF(mDebugFlag, "config image plane format: %d", format);
}

void GspLiteR1P0PlaneImage::configPmargbMode(hwc_layer_1_t *layer) {
  struct gsp_lite_r1p0_img_layer_params *params = &mConfig.params;

  if (layer->blending == HWC_BLENDING_PREMULT ||
      layer->blending == HWC_BLENDING_COVERAGE)
    params->pmargb_mod = 1;
  else
    params->pmargb_mod = 0;

  ALOGI_IF(mDebugFlag, "config image plane pmargb mode: %d",
           params->pmargb_mod);
}

void GspLiteR1P0PlaneImage::configAlpha(uint8_t alpha) {
  struct gsp_lite_r1p0_img_layer_params *params = &mConfig.params;

  params->alpha = alpha;
  ALOGI_IF(mDebugFlag, "config image plane apha: %d", alpha);
}

void GspLiteR1P0PlaneImage::configCSCMatrix(mali_gralloc_yuv_info info) {
  struct gsp_lite_r1p0_img_layer_params *params = &mConfig.params;
  uint8_t y2r_mod = 0;
  switch (info) {
    case MALI_YUV_BT601_WIDE:
      y2r_mod = 0;
      break;
    case MALI_YUV_BT601_NARROW:
      y2r_mod = 1;
      break;
    case MALI_YUV_BT709_WIDE:
      y2r_mod = 2;
      break;
    case MALI_YUV_BT709_NARROW:
      y2r_mod = 3;
      break;
    default:
      ALOGE("configCSCMatrix, unsupport yuv_csc_matrix %d.", info);
      break;
  }
  params->y2r_mod = y2r_mod;
  ALOGI_IF(mDebugFlag, "config image plane y2r_mod: %d", y2r_mod);
}

void GspLiteR1P0PlaneImage::configZorder(uint8_t zorder) {
  struct gsp_lite_r1p0_img_layer_params *params = &mConfig.params;

  params->zorder = zorder;
  ALOGI_IF(mDebugFlag, "config image zorder: %d", zorder);
}

void GspLiteR1P0PlaneImage::configEndian(struct private_handle_t *handle) {
  struct gsp_lite_r1p0_img_layer_params *params = &mConfig.params;
  struct gsp_layer_user *common = &mConfig.common;
  int format = handle->format;
  uint32_t pixel_cnt = handle->stride * handle->height;
  common->offset.v_offset = common->offset.uv_offset = pixel_cnt;

  switch (format) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_RGBX_8888:
      params->endian.y_rgb_word_endn = GSP_LITE_R1P0_WORD_ENDN_1;
      params->endian.y_rgb_dword_endn = GSP_LITE_R1P0_DWORD_ENDN_0;
      params->endian.a_swap_mode = GSP_LITE_R1P0_A_SWAP_RGBA;
      break;
    case HAL_PIXEL_FORMAT_YV12:  // YUV420_3P, Y V U
      common->offset.uv_offset += (pixel_cnt >> 2);
      break;
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
      params->endian.uv_word_endn = GSP_LITE_R1P0_WORD_ENDN_2;
      break;
    case HAL_PIXEL_FORMAT_YCbCr_420_SP:
      params->endian.uv_word_endn = GSP_LITE_R1P0_WORD_ENDN_0;
      break;
    case HAL_PIXEL_FORMAT_RGB_565:
      params->endian.rgb_swap_mode = GSP_LITE_R1P0_RGB_SWP_RGB;
      break;
    default:
      ALOGE("image configEndian, unsupport format=%d.", format);
      break;
  }

  ALOGI_IF(mDebugFlag, "image plane y_rgb_word_endn: %d, uv_word_endn: %d",
           params->endian.y_rgb_word_endn, params->endian.uv_word_endn);

  ALOGI_IF(mDebugFlag, "image plane rgb_swap_mode: %d, a_swap_mode: %d",
           params->endian.rgb_swap_mode, params->endian.a_swap_mode);
}

struct gsp_lite_r1p0_img_layer_user &GspLiteR1P0PlaneImage::getConfig() {
  return mConfig;
}

bool GspLiteR1P0PlaneImage::parcel(SprdHWLayer *layer) {
  if (layer == NULL) {
    ALOGE("image plane parcel params layer=NULL.");
    return false;
  }

  ALOGI_IF(mDebugFlag, "image plane start to parcel");
  hwc_layer_1_t *hwcLayer = layer->getAndroidLayer();
  enum gsp_rot_angle rot = rotationTypeConvert(hwcLayer->transform);

  struct sprdRect *srcRect = layer->getSprdSRCRect();
  struct sprdRect *dstRect = layer->getSprdFBRect();
  struct private_handle_t *handle = (struct private_handle_t *)hwcLayer->handle;
  uint32_t pitch = handle->stride;
  enum gsp_lite_r1p0_img_layer_format format = imgFormatConvert(handle->format);
  // configure clip size and dst size
  configSize(srcRect, dstRect, pitch, handle->height, format, rot);

  // configure format
  configFormat(format);
  // configure pmargb mode
  configPmargbMode(hwcLayer);

  int acquireFenceFd = hwcLayer->acquireFenceFd;
  int share_fd = handle->share_fd;
  // configure acquire fence fd and dma buffer share fd
  configCommon(acquireFenceFd, share_fd);

  configScaleEnable(srcRect, dstRect, rot);

  uint8_t alpha = hwcLayer->planeAlpha;
  // configure alpha
  configAlpha(alpha);

  configZorder(layer->getLayerIndex());

  configCSCMatrix(handle->yuv_info);

  configEndian(handle);
  return true;
}

}  // namespace android
