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
#ifndef GSPR3P0PLANE_GSPR3P0PLANEARRAY_H_
#define GSPR3P0PLANE_GSPR3P0PLANEARRAY_H_

#include "GspDevice.h"
#include "GspR3P0PlaneImage.h"
#include "GspR3P0PlaneOsd.h"
#include "GspR3P0PlaneDst.h"
#include "GspPlaneArray.h"
#include "SprdHWLayer.h"
#include "SprdUtil.h"
#include "gsp_r3p0_cfg.h"

#define R3P0_MAX_OSD_NUM 3

namespace android {

class GspR3P0PlaneArray : public GspPlaneArray {
 public:
  explicit GspR3P0PlaneArray(bool async);
  virtual ~GspR3P0PlaneArray();

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

  bool isOsdPlaneAttached(int OsdPlaneIndex);

  bool imagePlaneAdapt(SprdHWLayer *layer, int index);

  bool osdPlaneAdapt(SprdHWLayer *layer, int OsdPlaneIndex, int LayerIndex);

  bool dstPlaneAdapt(SprdHWLayer **list, int count);

  bool imagePlaneParcel(SprdHWLayer **list);

  bool osdPlaneParcel(SprdHWLayer **list, int OsdPlaneIndex);

  bool dstPlaneParcel(struct private_handle_t *handle, uint32_t w, uint32_t h,
                      int format, int wait_Fd, int32_t angle);
  bool miscCfgParcel(int mode_type);
  int split(SprdHWLayer **list, int count);

  int rotAdjustSingle(uint16_t *dx, uint16_t *dy, uint16_t *dw, uint16_t *dh,
                      uint32_t pitch, uint32_t transform);
  int rotAdjust(struct gsp_r3p0_cfg_user *cmd_info, SprdHWLayer **LayerList);
  bool parcel(SprdUtilSource *source, SprdUtilTarget *target);

  int run();

  int acquireSignalFd();

  int mAttachedNum;
  int mMaxAttachedNum;
  int mPlaneNum;

  enum gsp_addr_type mAddrType;

  GspR3P0PlaneImage *mImagePlane;
  GspR3P0PlaneOsd *mOsdPlane[R3P0_MAX_OSD_NUM];
  GspR3P0PlaneDst *mDstPlane;

  int mConfigIndex;
  struct gsp_r3p0_cfg_user *mConfigs;

  struct gsp_r3p0_capability mCapability;

  bool mAsync;
  bool mSplitSupport;

  int mDevice;

  uint32_t mOutputWidth;
  uint32_t mOutputHeight;

  int mDebugFlag;
};
}  // namespace android

#endif  // GSPR3P0PLANE_GSPR3P0PLANEARRAY_H_