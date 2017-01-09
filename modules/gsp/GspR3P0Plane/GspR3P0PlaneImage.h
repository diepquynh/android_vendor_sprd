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
#ifndef GSPR3P0PLANE_GSPR3P0PLANEIMAGE_H_
#define GSPR3P0PLANE_GSPR3P0PLANEIMAGE_H_

#include "GspR3P0Plane.h"
#include "gralloc_priv.h"
#include "SprdHWLayer.h"
#include "gsp_r3p0_cfg.h"

namespace android {

class GspR3P0PlaneImage : public GspR3P0Plane {
 public:
  GspR3P0PlaneImage(bool async, bool odd, bool support_up_down, bool size,
                    uint16_t up, uint16_t down, const GspRangeSize &range);
  virtual ~GspR3P0PlaneImage() {}

  bool adapt(SprdHWLayer *layer, int index);

  void reset(int flag);

  bool parcel(SprdHWLayer *layer);

  struct gsp_r3p0_img_layer_user &getConfig();

 private:
  bool hasMaxVideoSize();

  bool checkYuvInfo(mali_gralloc_yuv_info info);

  bool checkOddBoundary(struct sprdRect *srcRect,
                        enum gsp_r3p0_img_layer_format format);

  bool checkVideoSize(struct sprdRect *srcRect);

  bool checkImgLayerFormat(enum gsp_r3p0_img_layer_format format);

  bool checkScaleSize(struct sprdRect *srcRect, struct sprdRect *dstRect,
                      enum gsp_rot_angle rot);

  bool needScaleUpDown(struct sprdRect *srcRect, struct sprdRect *dstRect,
                       enum gsp_rot_angle rot);

  bool checkScale(struct sprdRect *srcRect, struct sprdRect *dstRect,
                  enum gsp_rot_angle rot);

  void configCommon(int wait_fd, int share_fd);

  void configScaleEnable(struct sprdRect *srcRect, struct sprdRect *dstRect,
                         enum gsp_rot_angle rot);

  void configFormat(enum gsp_r3p0_img_layer_format format);

  void configPmargbMode(hwc_layer_1_t *layer);

  void configAlpha(uint8_t alpha);

  void configZorder(uint8_t zorder);

  void configCSCMatrix(uint8_t y2r_mod);

  void configSize(struct sprdRect *srcRect, struct sprdRect *dstRect,
                  uint32_t pitch, uint32_t height,
                  enum gsp_r3p0_img_layer_format format,
                  enum gsp_rot_angle rot);

  void configEndian(struct private_handle_t *handle);

  bool mAsync;
  bool mOddSupport;
  bool mVideoMaxSize;
  bool mScale_Updown_Sametime;
  uint16_t mScaleRangeUp;
  uint16_t mScaleRangeDown;

  GspRangeSize mRangeSize;

  struct gsp_r3p0_img_layer_user mConfig;
};
}  // namespace android

#endif  // GSPR3P0PLANE_GSPR3P0PLANEIMAGE_H_
