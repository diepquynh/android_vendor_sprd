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

#include "GspR1P1Plane.h"

namespace android {

enum gsp_addr_type GspR1P1Plane::bufTypeConvert(int flag) {
  enum gsp_addr_type type = GSP_ADDR_TYPE_MAX;

  switch (flag) {
    case private_handle_t::PRIV_FLAGS_USES_PHY:
      type = GSP_ADDR_TYPE_PHYSICAL;
      break;
    default:
      type = GSP_ADDR_TYPE_IOVIRTUAL;
      ALOGI_IF(mDebugFlag, "default need virtual address");
      break;
  }

  return type;
}

enum gsp_r1p1_src_layer_format GspR1P1Plane::srcFormatConvert(int f) {
  enum gsp_r1p1_src_layer_format format = GSP_SRC_FMT_MAX_NUM;

  ALOGI_IF(mDebugFlag, "source format: %d", f);
  switch (f) {
    case HAL_PIXEL_FORMAT_YCbCr_420_SP:  // 0x19
      format = GSP_SRC_FMT_YUV420_2P;
      break;
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
      format = GSP_SRC_FMT_YUV420_2P;
      break;
    case HAL_PIXEL_FORMAT_YV12:
      format = GSP_SRC_FMT_YUV420_3P;
      break;
    case HAL_PIXEL_FORMAT_RGBA_8888:
      format = GSP_SRC_FMT_ARGB888;
      break;
    case HAL_PIXEL_FORMAT_RGBX_8888:
      format = GSP_SRC_FMT_RGB888;
      break;
    case HAL_PIXEL_FORMAT_RGB_565:
      format = GSP_SRC_FMT_RGB565;
      break;
    default:
      ALOGE("err:unknow src format:%d!", format);
      break;
  }

  return format;
}

enum gsp_rot_angle GspR1P1Plane::rotationTypeConvert(int32_t angle) {
  enum gsp_rot_angle rot;

  ALOGI_IF(mDebugFlag, "source rotation: %d", angle);
  switch (angle) {
    case 0:
      rot = GSP_ROT_ANGLE_0;
      break;
    case HAL_TRANSFORM_FLIP_H:  // 1
      rot = GSP_ROT_ANGLE_180_M;
      break;
    case HAL_TRANSFORM_FLIP_V:  // 2
      rot = GSP_ROT_ANGLE_0_M;
      break;
    case HAL_TRANSFORM_ROT_180:  // 3
      rot = GSP_ROT_ANGLE_180;
      break;
    case HAL_TRANSFORM_ROT_90:  // 4
      rot = GSP_ROT_ANGLE_270;
      break;
    case (HAL_TRANSFORM_ROT_90 | HAL_TRANSFORM_FLIP_H):  // 5
      rot = GSP_ROT_ANGLE_270_M;
      break;
    case (HAL_TRANSFORM_ROT_90 | HAL_TRANSFORM_FLIP_V):  // 6
      rot = GSP_ROT_ANGLE_90_M;
      break;
    case HAL_TRANSFORM_ROT_270:  // 7
      rot = GSP_ROT_ANGLE_90;
      break;
    default:
      rot = GSP_ROT_ANGLE_MAX_NUM;
      ALOGE("err:unknow source angle!");
      break;
  }

  return rot;
}

bool GspR1P1Plane::checkRangeSize(struct sprdRect *srcRect,
                               struct sprdRect *dstRect, const GspRangeSize &range) {
  bool result = true;

  ALOGI_IF(mDebugFlag, "src rect[%d %d %d %d] dst rect[%d %d %d %d]",
           srcRect->x, srcRect->y, srcRect->w, srcRect->h, dstRect->x,
           dstRect->y, dstRect->w, dstRect->h);
  // Source and destination rectangle size check.
  if (srcRect->w < range.crop_min.rect_w ||
      srcRect->h < range.crop_min.rect_h ||
      srcRect->w > range.crop_max.rect_w ||
      srcRect->h > range.crop_max.rect_h || dstRect->w < range.out_min.rect_w ||
      dstRect->h < range.out_min.rect_h || dstRect->w > range.out_max.rect_w ||
      dstRect->h > range.out_max.rect_h) {
    ALOGI_IF(mDebugFlag, "clip or dst rect is not supported.");
    result = false;
  }

  return result;
}

bool GspR1P1Plane::checkBlending(hwc_layer_1_t *layer) {
  bool result = true;

  switch (layer->blending) {
    case HWC_BLENDING_NONE:
    case HWC_BLENDING_PREMULT:
    case HWC_BLENDING_COVERAGE:
      ALOGI_IF(mDebugFlag, "not support blending flag: %d", layer->blending);
      result = true;
      break;
    default:
    ALOGE("not support blending flag: %d", layer->blending);
  }

  return result;
}

bool GspR1P1Plane::isLandScapeTransform(enum gsp_rot_angle rot) {
  bool result = false;

  if (rot == GSP_ROT_ANGLE_90 || rot == GSP_ROT_ANGLE_270 ||
      rot == GSP_ROT_ANGLE_90_M || rot == GSP_ROT_ANGLE_270_M)
    result = true;

  return result;
}

bool GspR1P1Plane::needTransform(enum gsp_rot_angle rot) {
  bool result = false;

  if (rot != GSP_ROT_ANGLE_0)
    result = true;

  return result;
}

bool GspR1P1Plane::needScale(struct sprdRect *srcRect, struct sprdRect *dstRect,
                          enum gsp_rot_angle rot) {
  bool result = false;

  if (isLandScapeTransform(rot) == true) {
    if (srcRect->w != dstRect->h || srcRect->h != dstRect->w) result = true;
  } else {
    if (srcRect->w != dstRect->w || srcRect->h != dstRect->h) result = true;
  }

  return result;
}

bool GspR1P1Plane::isVideoLayer(enum gsp_r1p1_src_layer_format format) {
  bool result = false;

  if (format > GSP_SRC_FMT_RGB565 && format < GSP_SRC_FMT_8BPP) result = true;

  return result;
}

}  // namespace android
