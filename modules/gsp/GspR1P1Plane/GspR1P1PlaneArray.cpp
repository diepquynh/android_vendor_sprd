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

#include "GspR1P1PlaneArray.h"
#include "GspR1P1PlaneImage.h"
#include "GspR1P1PlaneOsd.h"
#include "GspR1P1PlaneDst.h"
#include "../GspDevice.h"
#include "dump.h"
#include "SprdUtil.h"
#include "gsp_r1p1_cfg.h"

#define GSP_2PLANE_TRIGGER                                  \
  GSP_IO_TRIGGER(mAsync, (mConfigIndex + 1), mSplitSupport, \
                 struct gsp_r1p1_cfg_user)

#define GSP_LOG_PATH "/data/gsp.cfg"

namespace android {

GspR1P1PlaneArray::GspR1P1PlaneArray(bool async) { mAsync = async; }

GspR1P1PlaneArray::~GspR1P1PlaneArray() {
  if (mImagePlane) delete mImagePlane;

  if (mOsdPlane) delete mOsdPlane;

  if (mDstPlane) delete mDstPlane;

  if (mConfigs) delete mConfigs;
}

int GspR1P1PlaneArray::init(int fd) {
  int ret = -1;

  ret = ioctl(fd, GSP_IO_GET_CAPABILITY(struct gsp_r1p1_capability),
              &mCapability);
  if (ret < 0) {
    ALOGE("GspR1P1Plane array get capability failed\n");
    return ret;
  }

  if (mCapability.common.max_layer < 1) {
    ALOGE("max layer params error");
    return ret;
  }

  GspRangeSize range(mCapability.common.crop_max, mCapability.common.crop_min,
                     mCapability.common.out_max, mCapability.common.out_min);

  mImagePlane = new GspR1P1PlaneImage(
      mAsync, mCapability.yuv_xywh_even, mCapability.max_video_size,
      mCapability.scale_range_up, mCapability.scale_range_down, range);

  mOsdPlane = new GspR1P1PlaneOsd(mAsync, range);

  mDstPlane = new GspR1P1PlaneDst(mAsync);

  mPlaneNum = 2;

  mConfigs = new gsp_r1p1_cfg_user[mCapability.common.io_cnt];

  mDevice = fd;
  reset();

  return 0;
}

void GspR1P1PlaneArray::reset() {
  mAttachedNum = 0;
  mConfigIndex = 0;

  mImagePlane->reset(mDebugFlag);
  mOsdPlane->reset(mDebugFlag);
  mDstPlane->reset(mDebugFlag);
}

void GspR1P1PlaneArray::queryDebugFlag(int *debugFlag) {
  char value[PROPERTY_VALUE_MAX];
  static int openFileFlag = 0;

  if (debugFlag == NULL) {
    ALOGE("queryDebugFlag, input parameter is NULL");
    return;
  }

  property_get("debug.gsp.info", value, "0");

  if (atoi(value) == 1) {
    *debugFlag = 1;
  }
  if (access(GSP_LOG_PATH, R_OK) != 0) {
    return;
  }

  FILE *fp = NULL;
  char *pch;
  char cfg[100];

  fp = fopen(GSP_LOG_PATH, "r");
  if (fp != NULL) {
    if (openFileFlag == 0) {
      int ret;
      memset(cfg, '\0', 100);
      ret = fread(cfg, 1, 99, fp);
      if (ret < 1) {
        ALOGE("fread return size is wrong %d", ret);
      }
      cfg[sizeof(cfg) - 1] = 0;
      pch = strstr(cfg, "enable");
      if (pch != NULL) {
        *debugFlag = 1;
        openFileFlag = 1;
      }
    } else {
      *debugFlag = 1;
    }
    fclose(fp);
  }
}

bool GspR1P1PlaneArray::checkLayerCount(int count) {
  return count > mPlaneNum ? false : true;
}

bool GspR1P1PlaneArray::isExhausted() {
  return mAttachedNum == mMaxAttachedNum ? true : false;
}

bool GspR1P1PlaneArray::isSupportBufferType(enum gsp_addr_type type) {
  if (type == GSP_ADDR_TYPE_IOVIRTUAL && mAddrType == GSP_ADDR_TYPE_PHYSICAL)
    return false;
  else
    return true;
}

bool GspR1P1PlaneArray::isSupportSplit() { return mSplitSupport; }

void GspR1P1PlaneArray::planeAttached() {
  mAttachedNum++;
  ALOGI_IF(mDebugFlag, "has attached total %d plane", mAttachedNum);
}

bool GspR1P1PlaneArray::isImagePlaneAttached() {
  return mImagePlane->isAttached();
}

bool GspR1P1PlaneArray::isOsdPlaneAttached() { return mOsdPlane->isAttached(); }

bool GspR1P1PlaneArray::imagePlaneAdapt(SprdHWLayer *layer, int index) {
  if (isImagePlaneAttached() == true) return false;

  return mImagePlane->adapt(layer, index);
}

bool GspR1P1PlaneArray::osdPlaneAdapt(SprdHWLayer *layer, int index) {
  if (isOsdPlaneAttached() == true) return false;

  return mOsdPlane->adapt(layer, index);
}

int GspR1P1PlaneArray::adapt(SprdHWLayer **list, int count) {
  int ret = 0;
  bool result = false;

  queryDebugFlag(&mDebugFlag);
  reset();
  if (checkLayerCount(count) == false) {
    ALOGI_IF(mDebugFlag, "layer count: %d  over capability", count);
    return -1;
  }

  for (int i = 0; i < count; i++) {
    ALOGI_IF(mDebugFlag, "start to adapt %d/%d layer", i, count - 1);
    result = imagePlaneAdapt(list[i], i);
    if (result == true) continue;

    result = osdPlaneAdapt(list[i], i);
    if (result == false) {
      ret = -1;
      break;
    }
  }

  if (ret < 0)
    ALOGI_IF(mDebugFlag, "gsp 2plane adapt failed");
  else
    ALOGI_IF(mDebugFlag, "gsp 2plane adapt success");

  return ret;
}

bool GspR1P1PlaneArray::imagePlaneParcel(SprdHWLayer **list) {
  int index = mImagePlane->getIndex();
  bool result = false;

  ALOGI_IF(mDebugFlag, "image plane index: %d", index);
  if (index < 0) {
    ALOGE("image plane index error");
  } else {
    result = mImagePlane->parcel(list[index]);
  }

  return result;
}

bool GspR1P1PlaneArray::osdPlaneParcel(SprdHWLayer **list) {
  int index = mOsdPlane->getIndex();
  bool result = false;

  ALOGI_IF(mDebugFlag, "osd plane index: %d", index);
  if (index > 0) {
    result = mOsdPlane->parcel(list[index]);
  } else {
    result = mOsdPlane->parcel(mOutputWidth, mOutputHeight);
  }

  return result;
}

bool GspR1P1PlaneArray::dstPlaneParcel(struct private_handle_t *handle,
                                       uint32_t w, uint32_t h, int format,
                                       int wait_fd) {
  return mDstPlane->parcel(handle, w, h, format, wait_fd);
}

/* TODO implement this function later for multi-configs */
int GspR1P1PlaneArray::split(SprdHWLayer **list, int count) {
  if (list == NULL || count < 1) {
    ALOGE("split params error");
    return -1;
  }

  return 0;
}

void GspR1P1PlaneArray::miscCfgParcel() {
  struct gsp_r1p1_misc_cfg *cfg = NULL;

  cfg = &mConfigs[mConfigIndex].misc;
  cfg->dither_en = 0;
  cfg->gsp_gap = 0;
  cfg->gsp_clock = 3;
  cfg->ahb_clock = 3;
}

bool GspR1P1PlaneArray::parcel(SprdUtilSource *source, SprdUtilTarget *target) {
  int count = source->LayerCount;
  SprdHWLayer **list = source->LayerList;
  struct private_handle_t *handle = NULL;
  bool result = false;

  if (target->buffer != NULL)
    handle = target->buffer;
  else if (target->buffer2 != NULL)
    handle = target->buffer2;
  else
    ALOGE("parcel buffer params error");

  result = imagePlaneParcel(list);
  if (result == false) return result;

  result = osdPlaneParcel(list);
  if (result == false) return result;

  result = dstPlaneParcel(handle, mOutputWidth, mOutputHeight, target->format,
                          target->releaseFenceFd);
  if (result == false) return result;

  miscCfgParcel();
  return result;
}

int GspR1P1PlaneArray::run() {
  /*  uint32_t value = GSP_2PLANE_TRIGGER; */
  unsigned long value = GSP_2PLANE_TRIGGER;
  int ret = -1;

  return ioctl(mDevice, value, mConfigs);
}

int GspR1P1PlaneArray::acquireSignalFd() {
  return mConfigs[mConfigIndex].ld.common.sig_fd;
}

int GspR1P1PlaneArray::set(SprdUtilSource *source, SprdUtilTarget *target,
                           uint32_t w, uint32_t h) {
  int ret = -1;
  int fd = -1;

  mOutputWidth = w;
  mOutputHeight = h;

  if (parcel(source, target) == false) {
    ALOGE("gsp 2plane parcel failed");
    return ret;
  }

  mConfigs[mConfigIndex].l0 = mImagePlane->getConfig();
  mConfigs[mConfigIndex].l1 = mOsdPlane->getConfig();
  mConfigs[mConfigIndex].ld = mDstPlane->getConfig();

  ret = run();
  if (ret < 0) {
    ALOGE("trigger gsp device failed");
    return ret;
  } else {
    ALOGI_IF(mDebugFlag, "trigger gsp device success");
  }

  if (mAsync == false) {
    target->acquireFenceFd = source->releaseFenceFd = -1;
    ALOGI_IF(mDebugFlag, "sync mode set fd to -1");
    return ret;
  }

  fd = acquireSignalFd();
  if (fd <= 0) {
    ALOGE("acquire signal fence fd: %d error", fd);
    return -1;
  }

  target->acquireFenceFd = fd;
  source->releaseFenceFd = dup(fd);
  ALOGI_IF(mDebugFlag,
           "source release fence fd: %d, target acquire fence fd: %d",
           source->releaseFenceFd, target->acquireFenceFd);

  return ret;
}

}  // namespace android
