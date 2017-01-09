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
#ifndef GSPLITER1P0PLANE_GSPLITER1P0PLANEOSD_H_
#define GSPLITER1P0PLANE_GSPLITER1P0PLANEOSD_H_

#include "GspLiteR1P0Plane.h"
#include "gralloc_priv.h"
#include "SprdHWLayer.h"
#include "gsp_lite_r1p0_cfg.h"

namespace android {

class GspLiteR1P0PlaneOsd : public GspLiteR1P0Plane {
 public:
  GspLiteR1P0PlaneOsd(bool async, const GspRangeSize &range);
  virtual ~GspLiteR1P0PlaneOsd() {}

  void reset(int flag);

  bool adapt(SprdHWLayer *layer, int index);

  bool parcel(SprdHWLayer *layer);

  bool parcel(uint32_t w, uint32_t h);

  struct gsp_lite_r1p0_osd_layer_user &getConfig();

 private:
  void configCommon(int wait_fd, int share_fd, int enable);

  void configSize(struct sprdRect *srcRect, struct sprdRect *dstRect,
                  uint32_t pitch, uint32_t height);

  void configSize(uint32_t w, uint32_t h);

  void configFormat(enum gsp_lite_r1p0_osd_layer_format format);

  void configAlpha(uint8_t alpha);

  void configZorder(uint8_t zorder);

  void configPmargbMode(hwc_layer_1_t *layer);

  void configEndian(struct private_handle_t *handle);

  void configPallet(int enable);

  GspRangeSize mRangeSize;

  struct gsp_lite_r1p0_osd_layer_user mConfig;
};
}  // namespace android

#endif  // GSPLITER1P0PLANE_GSPLITER1P0PLANEDST_H_