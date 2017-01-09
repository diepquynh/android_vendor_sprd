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

#ifndef GSPDEVICE_H_
#define GSPDEVICE_H_

#include "GspPlaneArray.h"
#include "SprdHWLayer.h"
#include "SprdUtil.h"
#include "gsp_cfg.h"

#define GSP_HARDWARE_MODULE_ID "gsp"

namespace android {

typedef struct gsp_device_1 {
  struct hw_device_t common;

  int (*prepare)(struct gsp_device_1 *device, SprdHWLayer **list, int count,
                 bool *support);

  int (*commit)(struct gsp_device_1 *device, SprdUtilSource *source,
                SprdUtilTarget *target);

  int (*updateOutputSize)(struct gsp_device_1 *device, uint32_t w, uint32_t h);
} gsp_device_1_t;

class GspDevice : public gsp_device_1_t {
 public:
  GspDevice();
  ~GspDevice();

  int prepare(SprdHWLayer **list, int count, bool *support);

  int set(SprdUtilSource *source, SprdUtilTarget *target);

  int updateOutputSize(uint32_t w, uint32_t h);

  bool isUpdate();

  bool init();

 private:
  uint32_t mOutputWidth;
  uint32_t mOutputHeight;
  bool mUpdate;

  GspPlaneArray *mPlaneArray;

  int mDevice;
  struct gsp_capability mCapability;
};

}  // namespace android

#endif  // GSPDEVICE_H_
