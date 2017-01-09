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
#ifndef GSPR1P1PLANE_GSPR1P1PLANEDST_H_
#define GSPR1P1PLANE_GSPR1P1PLANEDST_H_

#include "GspR1P1Plane.h"
#include "gralloc_priv.h"
#include "SprdHWLayer.h"
#include "gsp_r1p1_cfg.h"

namespace android {

class GspR1P1PlaneDst : public GspR1P1Plane {
 public:
  explicit GspR1P1PlaneDst(bool async);
  virtual ~GspR1P1PlaneDst() {}

  bool adapt(SprdHWLayer **list);

  struct gsp_r1p1_des_layer_user &getConfig();

  bool parcel(struct private_handle_t *handle, uint32_t w, uint32_t h,
              int format, int wait_fd);

  void reset(int flag);

 private:
  enum gsp_r1p1_des_layer_format dstFormatConvert(int format);

  void configCommon(int wait_fd, int share_fd);

  void configFormat(int format);

  void configPitch(uint32_t pitch);

  void configEndian(int f, uint32_t w, uint32_t h);

  struct gsp_r1p1_des_layer_user mConfig;
};
}  // namespace android

#endif  // GSPR1P1PLANE_GSPR1P1PLANEDST_H_
