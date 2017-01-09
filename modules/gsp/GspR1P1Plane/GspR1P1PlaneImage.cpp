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
#include "GspR1P1Plane.h"
#include "GspR1P1PlaneArray.h"
#include "gsp_cfg.h"
#include "gsp_r1p1_cfg.h"

namespace android {

GspR1P1PlaneImage::GspR1P1PlaneImage(bool async, bool oddSupport, bool videoSize,
                               uint16_t up, uint16_t down,
                               const GspRangeSize &range) {
  mAttached = false;
  mIndex = -1;
  mAsync = async;
  mOddSupport = oddSupport;
  mVideoMaxSize = videoSize;
  mRangeSize = range;
  mScaleRangeUp = up;
  mScaleRangeDown = down;
  ALOGI("create GspR1P1PlaneImage");
  mRangeSize.print();
  ALOGI("scale range up: %d, scale range down: %d", mScaleRangeUp,
        mScaleRangeDown);
  ALOGI("odd support: %d", mOddSupport);
  ALOGI("vidoe max size: %d", mVideoMaxSize);
}

bool GspR1P1PlaneImage::checkYuvInfo(mali_gralloc_yuv_info info) {
  bool result = false;

  // source layer YUV range check.
  if (info == MALI_YUV_BT709_NARROW || info == MALI_YUV_BT601_NARROW) {
    ALOGI_IF(mDebugFlag, "need to set y2r opt register.");
    result = true;
  }

  return result;
}

bool GspR1P1PlaneImage::checkVideoSize(struct sprdRect *srcRect) {
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

bool GspR1P1PlaneImage::checkOddBoundary(struct sprdRect *srcRect) {
  bool result = true;

  // if yuv_xywh_even == 1, gsp do not support odd source layer.
  if (mOddSupport == true) {
    if ((srcRect->x & 0x1) || (srcRect->y & 0x1) || (srcRect->w & 0x1) ||
        (srcRect->h & 0x1)) {
      ALOGD_IF(mDebugFlag, "do not support odd source layer xywh.");
      result = false;
    }
  }

  return result;
}

bool GspR1P1PlaneImage::checkScaleDown(struct sprdRect *srcRect,
                                    struct sprdRect *dstRect,
                                    enum gsp_rot_angle rot) {
  bool result = true;
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

  // GSP defect relative check.
  if (dsth < srch) {  // scaling down
    uint32_t div = 1;

    if (dsth * 2 > srch) {
      div = 32;
    } else if (dsth * 4 > srch) {
      div = 64;
    } else if (dsth * 8 > srch) {
      div = 128;
    } else if (dsth * 16 > srch) {
      div = 256;
    }

    if (srch / div * div != srch) {
      if ((srch / div * div * dsth) > (srch * (dsth - 1) + 1)) {
        ALOGI_IF(mDebugFlag, "can't support %dx%d->%dx%d scaling!", srcw, srch,
                 dstw, dsth);
        result = false;
      }
    }
  }

  return result;
}

bool GspR1P1PlaneImage::needScaleUpDown(struct sprdRect *srcRect,
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

  if ((srcw < dstw && srch > dsth) || (srcw > dstw && srch < dsth)) {
    ALOGI_IF(mDebugFlag,
             "need scale up and down at same time, which not support");
    result = true;
  }

  return result;
}

bool GspR1P1PlaneImage::checkScaleSize(struct sprdRect *srcRect,
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

bool GspR1P1PlaneImage::checkScale(struct sprdRect *srcRect,
                                struct sprdRect *dstRect,
                                enum gsp_rot_angle rot) {
  if (needScaleUpDown(srcRect, dstRect, rot) == true) return false;

  if (checkScaleSize(srcRect, dstRect, rot) == false) return false;

  if (checkScaleDown(srcRect, dstRect, rot) == false) return false;

  return true;
}

bool GspR1P1PlaneImage::checkYuvType(mali_gralloc_yuv_info yuv_info) {
  bool result = true;

  if (yuv_info == MALI_YUV_BT709_NARROW || yuv_info == MALI_YUV_BT709_WIDE) {
    ALOGI_IF(mDebugFlag, "gsp not support BT609 convert");
    result = false;
  }

  return result;
}

void GspR1P1PlaneImage::reset(int flag) {
  mAttached = false;
  mIndex = -1;
  mDebugFlag = flag;
  memset(&mConfig, 0, sizeof(struct gsp_r1p1_img_layer_user));
}

bool GspR1P1PlaneImage::adapt(SprdHWLayer *layer, int index) {
  struct sprdRect *srcRect = layer->getSprdSRCRect();
  struct sprdRect *dstRect = layer->getSprdFBRect();
  hwc_layer_1_t *hwcLayer = layer->getAndroidLayer();
  struct private_handle_t *handle = (struct private_handle_t *)hwcLayer->handle;
  mali_gralloc_yuv_info yuv_info = handle->yuv_info;
  enum gsp_addr_type bufType = bufTypeConvert(handle->flags);
  enum gsp_r1p1_src_layer_format format = srcFormatConvert(handle->format);
  enum gsp_rot_angle rot = rotationTypeConvert(hwcLayer->transform);

  ALOGI_IF(mDebugFlag, "image plane start to adapt");
  if (isAttached() == true) return false;

  if (isProtectedVideo(handle) == true) return false;

  if (checkRangeSize(srcRect, dstRect, mRangeSize) == false) return false;

  if (checkYuvType(yuv_info) == false) return false;

  if (checkVideoSize(srcRect) == false) return false;

  if (checkOddBoundary(srcRect) == false) return false;

  if (checkScale(srcRect, dstRect, rot) == false) return false;

  if (checkBlending(hwcLayer) == false) return false;

  attached(index);
  ALOGI_IF(mDebugFlag, "image plane attached: %d", index);

  return true;
}

void GspR1P1PlaneImage::configCommon(int wait_fd, int share_fd) {
  struct gsp_layer_user *common = &mConfig.common;

  common->type = GSP_IMG_LAYER;
  common->enable = 1;
  common->wait_fd = mAsync == true ? wait_fd : -1;
  common->share_fd = share_fd;
  ALOGI_IF(mDebugFlag,
           "conifg image plane enable: %d, wait_fd: %d, share_fd: %d",
           common->enable, common->wait_fd, common->share_fd);
}

void GspR1P1PlaneImage::configScaleEnable(struct sprdRect *srcRect,
                                       struct sprdRect *dstRect,
                                       enum gsp_rot_angle rot) {
  struct gsp_r1p1_img_layer_params *params = &mConfig.params;

  if (needScale(srcRect, dstRect, rot) == true)
     params->scaling_en = 1;
}

void GspR1P1PlaneImage::configRotation(enum gsp_rot_angle rot) {
  struct gsp_r1p1_img_layer_params *params = &mConfig.params;

  params->rot_angle = rot;
  ALOGI_IF(mDebugFlag, "config image plane rotation angle: %d", rot);
}

void GspR1P1PlaneImage::configSize(struct sprdRect *srcRect,
                                struct sprdRect *dstRect, uint32_t pitch,
                                enum gsp_r1p1_src_layer_format format,
                                enum gsp_rot_angle rot) {
  struct gsp_r1p1_img_layer_params *params = &mConfig.params;

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

  if (isVideoLayer(format) == true && mOddSupport == true) {
    // image plane not support yuv format osdd size
    if (needScale(srcRect, dstRect, rot) == true) {
      params->clip_rect.st_x &= 0xfffe;
      params->clip_rect.st_y &= 0xfffe;
      params->clip_rect.rect_w &= 0xfffe;
      params->clip_rect.rect_h &= 0xfffe;
    }
  }

  ALOGI_IF(mDebugFlag, "config image plane pitch: %u", pitch);
  ALOGI_IF(mDebugFlag,
           "config image plane clip rect[%d %d %d %d] dst rect[%d %d %d %d]",
           params->clip_rect.st_x, params->clip_rect.st_y,
           params->clip_rect.rect_w, params->clip_rect.rect_h,
           params->des_rect.st_x, params->des_rect.st_y,
           params->des_rect.rect_w, params->des_rect.rect_h);
}

void GspR1P1PlaneImage::configFormat(enum gsp_r1p1_src_layer_format format) {
  struct gsp_r1p1_img_layer_params *params = &mConfig.params;

  params->img_format = format;
  ALOGI_IF(mDebugFlag, "config image plane format: %d", format);
}

void GspR1P1PlaneImage::configMisc(struct private_handle_t *handle,
                                   enum gsp_r1p1_src_layer_format format,
                                   enum gsp_rot_angle rot) {
  struct gsp_r1p1_img_layer_params *params = &mConfig.params;

  if (checkYuvInfo(handle->yuv_info) == true) {
    params->y2r_opt = 1;
  }

  if (isVideoLayer(format) == true
      && needTransform(rot) == true) {
    params->bnd_bypass = 1;  // means no split
  } else {
    params->bnd_bypass = 0;  // means split
  }
  ALOGI_IF(mDebugFlag, "config y2r_opt: %d, bnd_bypass: %d",
           params->y2r_opt, params->bnd_bypass);
}

void GspR1P1PlaneImage::configPmargbMode(hwc_layer_1_t *layer) {
  struct gsp_r1p1_img_layer_params *params = &mConfig.params;

  if (layer->blending == HWC_BLENDING_PREMULT)
    params->pmargb_mod = 0;
  else
    params->pmargb_mod = 1;

  ALOGI_IF(mDebugFlag, "config image plane pmargb mode: %d",
           params->pmargb_mod);
}

void GspR1P1PlaneImage::configAlpha(uint8_t alpha) {
  struct gsp_r1p1_img_layer_params *params = &mConfig.params;

  params->alpha = alpha;
  ALOGI_IF(mDebugFlag, "config image plane apha: %d", alpha);
}

void GspR1P1PlaneImage::configEndian(struct private_handle_t *handle) {
  struct gsp_r1p1_img_layer_params *params = &mConfig.params;
  struct gsp_layer_user *common = &mConfig.common;
  int format = handle->format;
  uint32_t pixel_cnt = handle->stride * handle->height;
  common->offset.v_offset = common->offset.uv_offset = pixel_cnt;

  switch (format) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_RGBX_8888:
      params->endian.y_word_endn = GSP_WORD_ENDN_1;
      params->endian.a_swap_mode = GSP_A_SWAP_RGBA;
      break;
    case HAL_PIXEL_FORMAT_YV12:  // YUV420_3P, Y V U
      common->offset.uv_offset += (pixel_cnt >> 2);
      break;
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
      params->endian.uv_word_endn = GSP_WORD_ENDN_2;
      break;
    case HAL_PIXEL_FORMAT_RGB_565:
      params->endian.rgb_swap_mode = GSP_RGB_SWP_RGB;
      break;
    default:
      ALOGE("endian from error format");
      break;
  }

  ALOGI_IF(mDebugFlag,
           "image plane y_word_endn: %d, uv_word_endn: %d, va_word_endn: %d",
           params->endian.y_word_endn, params->endian.uv_word_endn,
           params->endian.va_word_endn);
  ALOGI_IF(mDebugFlag, "image plane rgb_swap_mode: %d, a_swap_mode: %d",
           params->endian.rgb_swap_mode, params->endian.a_swap_mode);
}

struct gsp_r1p1_img_layer_user &GspR1P1PlaneImage::getConfig() {
  return mConfig;
}

bool GspR1P1PlaneImage::parcel(SprdHWLayer *layer) {
  if (layer == NULL) {
    ALOGE("image plane parcel params error");
    return false;
  }

  ALOGI_IF(mDebugFlag, "image plane start to parcel");
  hwc_layer_1_t *hwcLayer = layer->getAndroidLayer();
  enum gsp_rot_angle rot = rotationTypeConvert(hwcLayer->transform);
  // configure rotation
  configRotation(rot);

  struct sprdRect *srcRect = layer->getSprdSRCRect();
  struct sprdRect *dstRect = layer->getSprdFBRect();
  struct private_handle_t *handle = (struct private_handle_t *)hwcLayer->handle;
  uint32_t pitch = handle->stride;
  enum gsp_r1p1_src_layer_format format = srcFormatConvert(handle->format);
  // configure clip size and dst size
  configSize(srcRect, dstRect, pitch, format, rot);

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

  configEndian(handle);

  configMisc(handle, format, rot);
  return true;
}

}  // namespace android
