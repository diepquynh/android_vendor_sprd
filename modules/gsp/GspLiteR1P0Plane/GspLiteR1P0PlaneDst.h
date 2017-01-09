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
#ifndef GSPLITER1P0PLANE_GSPLITER1P0PLANEDST_H_
#define GSPLITER1P0PLANE_GSPLITER1P0PLANEDST_H_

#include "GspLiteR1P0Plane.h"
#include "gralloc_priv.h"
#include "SprdHWLayer.h"
#include "gsp_lite_r1p0_cfg.h"

/* MMU size for gsp output buffer > 2560 * 1600 * 4. */
#define LITE_R1P0_DST_GSP_MMU_SIZE (16 * 1024 * 1024)

namespace android {

class GspLiteR1P0PlaneDst : public GspLiteR1P0Plane {
 public:
  explicit GspLiteR1P0PlaneDst(bool async, int max_gspmmu_size,
                               int max_gsp_bandwidth);
  virtual ~GspLiteR1P0PlaneDst() {}

  bool adapt(SprdHWLayer **list, int count);

  struct gsp_lite_r1p0_des_layer_user &getConfig();

  bool parcel(struct private_handle_t *handle, uint32_t w, uint32_t h,
              int format, int wait_fd, int32_t angle);

  void reset(int flag);

 private:
  enum gsp_lite_r1p0_des_layer_format dstFormatConvert(int format);

  bool checkOutputRotation(SprdHWLayer **list, int count);

  void configCommon(int wait_fd, int share_fd);

  void configFormat(int format);

  void configPitch(uint32_t pitch);

  void configHeight(uint32_t height);

  void configEndian(int f, uint32_t w, uint32_t h);

  void configBackGround(struct gsp_background_para bg_para);

  void configOutputRotation(enum gsp_rot_angle rot_angle);

  bool checkGspMMUSize(SprdHWLayer **list, int count);

  bool checkGspBandwidth(SprdHWLayer **list, int count);

  void configDither(bool enable);

  void configCSCMode(uint8_t r2y_mod);

  struct gsp_lite_r1p0_des_layer_user mConfig;

  int maxGspMMUSize;

  int maxGspBandwidth;
};
}  // namespace android

#endif  // GSPLITER1P0PLANE_GSPLITER1P0PLANEDST_H_