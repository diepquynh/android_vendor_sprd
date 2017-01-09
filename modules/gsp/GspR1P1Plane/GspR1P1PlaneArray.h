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
#ifndef GSPR1P1PLANE_GSPR1P1PLANEARRAY_H_
#define GSPR1P1PLANE_GSPR1P1PLANEARRAY_H_

#include "GspDevice.h"
#include "GspR1P1PlaneImage.h"
#include "GspR1P1PlaneOsd.h"
#include "GspR1P1PlaneDst.h"
#include "GspPlaneArray.h"
#include "SprdHWLayer.h"
#include "SprdUtil.h"
#include "gsp_r1p1_cfg.h"

namespace android {

class GspR1P1PlaneArray : public GspPlaneArray {
 public:
  explicit GspR1P1PlaneArray(bool async);
  virtual ~GspR1P1PlaneArray();

  int init(int fd);

  int adapt(SprdHWLayer **list, int count);

  bool parcel(SprdHWLayer *list);

  int set(SprdUtilSource *source, SprdUtilTarget *target, uint32_t w,
          uint32_t h);

  void reset();

 private:
  void queryDebugFlag(int *debugFlag);

  bool checkLayerCount(int count);

  bool isExhausted();

  bool isSupportBufferType(enum gsp_addr_type type);

  bool isSupportSplit();

  void planeAttached();

  bool isImagePlaneAttached();

  bool isOsdPlaneAttached();

  bool imagePlaneAdapt(SprdHWLayer *layer, int index);

  bool osdPlaneAdapt(SprdHWLayer *layer, int index);

  bool imagePlaneParcel(SprdHWLayer **list);

  bool osdPlaneParcel(SprdHWLayer **list);

  bool dstPlaneParcel(struct private_handle_t *handle, uint32_t w, uint32_t h,
                      int format, int wait_Fd);

  void miscCfgParcel();

  int split(SprdHWLayer **list, int count);

  bool parcel(SprdUtilSource *source, SprdUtilTarget *target);

  int run();

  int acquireSignalFd();

  int mAttachedNum;
  int mMaxAttachedNum;
  int mPlaneNum;

  enum gsp_addr_type mAddrType;

  GspR1P1PlaneImage *mImagePlane;
  GspR1P1PlaneOsd *mOsdPlane;
  GspR1P1PlaneDst *mDstPlane;

  int mConfigIndex;
  struct gsp_r1p1_cfg_user *mConfigs;

  struct gsp_r1p1_capability mCapability;

  bool mAsync;
  bool mSplitSupport;

  int mDevice;

  uint32_t mOutputWidth;
  uint32_t mOutputHeight;

  int mDebugFlag;
};
}  // namespace android

#endif  // GSPR1P1PLANE_GSPR1P1PLANEARRAY_H_
