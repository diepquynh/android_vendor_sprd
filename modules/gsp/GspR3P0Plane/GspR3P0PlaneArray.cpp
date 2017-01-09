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

#include "GspR3P0PlaneArray.h"
#include <algorithm>
#include "GspR3P0PlaneImage.h"
#include "GspR3P0PlaneOsd.h"
#include "GspR3P0PlaneDst.h"
#include "../GspDevice.h"
#include "dump.h"
#include "SprdUtil.h"
#include "gsp_r3p0_cfg.h"

#define GSP_R3P0_PLANE_TRIGGER                              \
  GSP_IO_TRIGGER(mAsync, (mConfigIndex + 1), mSplitSupport, \
                 struct gsp_r3p0_cfg_user)

#define GSP_LOG_PATH "/data/gsp.cfg"

namespace android {

GspR3P0PlaneArray::GspR3P0PlaneArray(bool async) {
  mAsync = async;
  mSplitSupport = 0;
  mDebugFlag = 0;
}

GspR3P0PlaneArray::~GspR3P0PlaneArray() {
  if (mImagePlane) delete mImagePlane;

  for (int i = 0; i < R3P0_MAX_OSD_NUM; i++) {
    if (mOsdPlane[i]) delete mOsdPlane[i];
  }

  if (mDstPlane) delete mDstPlane;

  if (mConfigs) delete mConfigs;
}
int GspR3P0PlaneArray::init(int fd) {
  int ret = -1;

  ret = ioctl(fd, GSP_IO_GET_CAPABILITY(struct gsp_r3p0_capability),
              &mCapability);
  if (ret < 0) {
    ALOGE("gspr3p0plane array get capability failed, ret=%d.\n", ret);
    return ret;
  }

  if (mCapability.common.max_layer < 1) {
    ALOGE("max layer params error");
    return -1;
  }

  GspRangeSize range(mCapability.common.crop_max, mCapability.common.crop_min,
                     mCapability.common.out_max, mCapability.common.out_min);

  mImagePlane = new GspR3P0PlaneImage(
      mAsync, mCapability.yuv_xywh_even, mCapability.scale_updown_sametime,
      mCapability.max_video_size, mCapability.scale_range_up,
      mCapability.scale_range_down, range);
  if (mImagePlane == NULL) {
    ALOGE("R3P0 ImgPlane alloc fail!");
    return -1;
  }

  for (int i = 0; i < R3P0_MAX_OSD_NUM; i++) {
    mOsdPlane[i] = new GspR3P0PlaneOsd(mAsync, range);
    if (mOsdPlane[i] == NULL) {
      ALOGE("R3P0 OsdPlane[%d] alloc fail!", i);
      return -1;
    }
  }

  mDstPlane = new GspR3P0PlaneDst(mAsync, mCapability.max_gspmmu_size,
                                  mCapability.max_gsp_bandwidth);
  if (mDstPlane == NULL) {
    ALOGE("R3P0 DstPlane alloc fail!");
    return -1;
  }

  mPlaneNum = 4;

  mConfigs = new gsp_r3p0_cfg_user[mCapability.common.io_cnt];
  if (mConfigs == NULL) {
    ALOGE("R3P0 mConfigs alloc fail, io_cnt:%d !", mCapability.common.io_cnt);
    return -1;
  }

  mDevice = fd;
  reset();

  return 0;
}

void GspR3P0PlaneArray::reset() {
  mAttachedNum = 0;
  mConfigIndex = 0;

  mImagePlane->reset(mDebugFlag);
  for (int i = 0; i < R3P0_MAX_OSD_NUM; i++) {
    mOsdPlane[i]->reset(mDebugFlag);
  }
  mDstPlane->reset(mDebugFlag);
}

void GspR3P0PlaneArray::queryDebugFlag(int *debugFlag) {
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
        ALOGI_IF(mDebugFlag, "fread return size is wrong %d", ret);
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

bool GspR3P0PlaneArray::checkLayerCount(int count) {
  return count > mPlaneNum ? false : true;
}

bool GspR3P0PlaneArray::isExhausted() {
  return mAttachedNum == mMaxAttachedNum ? true : false;
}

bool GspR3P0PlaneArray::isSupportBufferType(enum gsp_addr_type type) {
  if (type == GSP_ADDR_TYPE_IOVIRTUAL && mAddrType == GSP_ADDR_TYPE_PHYSICAL)
    return false;
  else
    return true;
}

bool GspR3P0PlaneArray::isSupportSplit() { return mSplitSupport; }

void GspR3P0PlaneArray::planeAttached() {
  mAttachedNum++;
  ALOGI_IF(mDebugFlag, "has attached total %d plane", mAttachedNum);
}

bool GspR3P0PlaneArray::isImagePlaneAttached() {
  return mImagePlane->isAttached();
}

bool GspR3P0PlaneArray::isOsdPlaneAttached(int OsdPlaneIndex) {
  return mOsdPlane[OsdPlaneIndex]->isAttached();
}

bool GspR3P0PlaneArray::imagePlaneAdapt(SprdHWLayer *layer, int LayerIndex) {
  if (isImagePlaneAttached() == true) return false;

  return mImagePlane->adapt(layer, LayerIndex);
}

bool GspR3P0PlaneArray::osdPlaneAdapt(SprdHWLayer *layer, int OsdPlaneIndex,
                                      int LayerIndex) {
  if (isOsdPlaneAttached(OsdPlaneIndex) == true) return false;

  return mOsdPlane[OsdPlaneIndex]->adapt(layer, LayerIndex);
}

bool GspR3P0PlaneArray::dstPlaneAdapt(SprdHWLayer **list, int count) {
  return mDstPlane->adapt(list, count);
}

int GspR3P0PlaneArray::adapt(SprdHWLayer **list, int count) {
  int ret = 0;
  bool result = false;
  hwc_layer_1_t *layer = NULL;
  struct private_handle_t *privateH = NULL;
  int32_t yuv_index = 0;  // yuv layer index
  int32_t osd_index = 0;

  queryDebugFlag(&mDebugFlag);
  reset();
  if (checkLayerCount(count) == false) {
    ALOGI_IF(mDebugFlag, "layer count: %d  over capability", count);
    return -1;
  }

  int j = 0;
  while (j < count) {
    layer = list[j]->getAndroidLayer();
    privateH = (struct private_handle_t *)(layer->handle);

    if (privateH->internal_format & GRALLOC_ARM_INTFMT_AFBC) {
      ALOGI_IF(mDebugFlag, "GSP do not support AFBC switch to GPU handle.");
      return -1;
    }
    j++;
  }

  int i = 0;
  while (i < count) {
    layer = list[i]->getAndroidLayer();
    privateH = (struct private_handle_t *)(layer->handle);

    if ((privateH->format == HAL_PIXEL_FORMAT_YCbCr_420_SP) ||
        (privateH->format == HAL_PIXEL_FORMAT_YCrCb_420_SP)) {
      yuv_index = i;
      break;
    }
    i++;
  }

  /*  imglayer and osdlayer configure */
  if (yuv_index) {
    result = imagePlaneAdapt(list[yuv_index], yuv_index);
    if (result == false) {
      ALOGI_IF(mDebugFlag, "imagePlaneAdapt, unsupport case (YUV format).");
      return -1;
    }

    i = 0;
    while (i < count) {
      if (yuv_index != i) {
        result = osdPlaneAdapt(list[i], osd_index, i);
        if (result == false) {
          ALOGI_IF(mDebugFlag, "osdPlaneAdapt, unsupport case LayerIndex: %d.",
                   i);
          return -1;
        }
        osd_index++;
      }
      i++;
    }
  } else {
    i = 0;
    result = imagePlaneAdapt(list[i], i);
    if (result == false) {
      ALOGI_IF(mDebugFlag, "imagePlaneAdapt, unsupport case (RGB format).");
      return -1;
    }

    i++;
    osd_index = 0;
    while (i < count) {
      result = osdPlaneAdapt(list[i], osd_index, i);
      if (result == false) {
        ALOGI_IF(mDebugFlag, "osdPlaneAdapt, unsupport case LayerIndex: %d ",
                 i);
        return -1;
      }
      osd_index++;
      i++;
    }
  }

  result = dstPlaneAdapt(list, count);
  if (result == false) {
    ALOGI_IF(mDebugFlag, "dstPlaneAdapt unsupport case.");
    return -1;
  }

  ALOGI_IF(mDebugFlag, "gsp r3p0plane adapt success");

  return ret;
}

bool GspR3P0PlaneArray::imagePlaneParcel(SprdHWLayer **list) {
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

bool GspR3P0PlaneArray::osdPlaneParcel(SprdHWLayer **list, int OsdPlaneIndex) {
  int index = mOsdPlane[OsdPlaneIndex]->getIndex();
  bool result = false;

  ALOGI_IF(mDebugFlag, "osd plane index: %d", index);
  if (index >= 0) {
    result = mOsdPlane[OsdPlaneIndex]->parcel(list[index]);
  } else {
    result = mOsdPlane[OsdPlaneIndex]->parcel(mOutputWidth, mOutputHeight);
  }

  return result;
}

bool GspR3P0PlaneArray::dstPlaneParcel(struct private_handle_t *handle,
                                       uint32_t w, uint32_t h, int format,
                                       int wait_fd, int32_t angle) {
  return mDstPlane->parcel(handle, w, h, format, wait_fd, angle);
}

bool GspR3P0PlaneArray::miscCfgParcel(int mode_type) {
  bool status = false;
  struct gsp_r3p0_misc_cfg_user &r3p0_misc_cfg = mConfigs[mConfigIndex].misc;
  const struct gsp_r3p0_img_layer_user r3p0_l0_cfg = mImagePlane->getConfig();
  switch (mode_type) {
    case 0: {
      /* run_mod = 0, scale_seq = 0 */
      r3p0_misc_cfg.run_mod = 0;
      r3p0_misc_cfg.scale_seq = 0;
      r3p0_misc_cfg.scale_para.htap_mod = 4;
      r3p0_misc_cfg.scale_para.vtap_mod = 4;
      if (r3p0_l0_cfg.params.scaling_en) {
        r3p0_misc_cfg.scale_para.scale_en = 1;
      } else {
        r3p0_misc_cfg.scale_para.scale_en = 0;
      }
      r3p0_misc_cfg.scale_para.scale_rect_in = r3p0_l0_cfg.params.clip_rect;
      r3p0_misc_cfg.scale_para.scale_rect_out = r3p0_l0_cfg.params.des_rect;

      r3p0_misc_cfg.workarea_src_rect.st_x = 0;
      r3p0_misc_cfg.workarea_src_rect.st_y = 0;
      r3p0_misc_cfg.workarea_src_rect.rect_w = mOutputWidth;
      r3p0_misc_cfg.workarea_src_rect.rect_h = mOutputHeight;
      r3p0_misc_cfg.workarea_dst_rect.st_x = 0;
      r3p0_misc_cfg.workarea_dst_rect.st_y = 0;
      r3p0_misc_cfg.workarea_dst_rect.rect_w = mOutputWidth;
      r3p0_misc_cfg.workarea_dst_rect.rect_h = mOutputHeight;
    }
      status = true;
      break;
    default:
      ALOGE("gsp r3p0 not implement other mode(%d) yet! ", mode_type);
      break;
  }

  return status;
}

/* TODO implement this function later for multi-configs */
int GspR3P0PlaneArray::split(SprdHWLayer **list, int count) {
  if (list == NULL || count < 1) {
    ALOGE("split params error");
    return -1;
  }

  return 0;
}

int GspR3P0PlaneArray::rotAdjustSingle(uint16_t *dx, uint16_t *dy, uint16_t *dw,
                                       uint16_t *dh, uint32_t pitch,
                                       uint32_t transform) {
  uint32_t x = *dx;
  uint32_t y = *dy;

  /*first adjust dest x y*/
  switch (transform) {
    case 0:
      break;
    case HAL_TRANSFORM_FLIP_H:  // 1
      *dx = pitch - x - *dw;
      break;
    case HAL_TRANSFORM_FLIP_V:  // 2
      *dy = mOutputHeight - y - *dh;
      break;
    case HAL_TRANSFORM_ROT_180:  // 3
      *dx = pitch - x - *dw;
      *dy = mOutputHeight - y - *dh;
      break;
    case HAL_TRANSFORM_ROT_90:  // 4
      *dx = y;
      *dy = pitch - x - *dw;
      break;
    case (HAL_TRANSFORM_ROT_90 | HAL_TRANSFORM_FLIP_H):  // 5
      *dx = mOutputHeight - y - *dh;
      *dy = pitch - x - *dw;
      break;
    case (HAL_TRANSFORM_ROT_90 | HAL_TRANSFORM_FLIP_V):  // 6
      *dx = y;
      *dy = x;
      break;
    case HAL_TRANSFORM_ROT_270:  // 7
      *dx = mOutputHeight - y - *dh;
      *dy = x;
      break;
    default:
      ALOGE("rotAdjustSingle, unsupport angle=%d.", transform);
      break;
  }
  /*then adjust dest width height*/
  if (transform & HAL_TRANSFORM_ROT_90) {
    std::swap(*dw, *dh);
  }

  return 0;
}

int GspR3P0PlaneArray::rotAdjust(struct gsp_r3p0_cfg_user *cmd_info,
                                 SprdHWLayer **LayerList) {
  int32_t ret = 0;
  uint16_t w = 0;
  uint16_t h = 0;
  uint32_t transform = 0;
  struct gsp_r3p0_osd_layer_user *osd_info = NULL;
  hwc_layer_1_t *hwcLayer = LayerList[0]->getAndroidLayer();
  if (hwcLayer == NULL) {
    ALOGE("rotAdjust : get hwcLayer failed!");
    return -1;
  }
  transform = hwcLayer->transform;

  if (cmd_info->l0.common.enable == 1) {
    ret = rotAdjustSingle(&cmd_info->l0.params.des_rect.st_x,
                          &cmd_info->l0.params.des_rect.st_y,
                          &cmd_info->misc.scale_para.scale_rect_out.rect_w,
                          &cmd_info->misc.scale_para.scale_rect_out.rect_h,
                          cmd_info->ld1.params.pitch, transform);
    if (ret) {
      ALOGE("rotAdjust l0 layer rotation adjust failed, ret=%d.", ret);
      return ret;
    }
  }

  osd_info = &(cmd_info->l1);
  for (int32_t i = 0; i < 3; i++) {
    if (osd_info[i].common.enable == 1) {
      w = osd_info[i].params.clip_rect.rect_w;
      h = osd_info[i].params.clip_rect.rect_h;
      if (transform & HAL_TRANSFORM_ROT_90) {
        std::swap(w, h);
      }
      ret = rotAdjustSingle(&osd_info[i].params.des_pos.pt_x,
                            &osd_info[i].params.des_pos.pt_y, &w, &h,
                            cmd_info->ld1.params.pitch, transform);
      if (ret) {
        ALOGE("rotAdjust OSD[%d] rotation adjust failed, ret=%d.", i, ret);
        return ret;
      }
    }
  }

  if (transform & HAL_TRANSFORM_ROT_90) {
    std::swap(cmd_info->ld1.params.pitch, cmd_info->ld1.params.height);
    std::swap(cmd_info->misc.workarea_src_rect.rect_w,
              cmd_info->misc.workarea_src_rect.rect_h);
  }
  return ret;
}

bool GspR3P0PlaneArray::parcel(SprdUtilSource *source, SprdUtilTarget *target) {
  int count = source->LayerCount;
  SprdHWLayer **list = source->LayerList;
  struct private_handle_t *handle = NULL;
  bool result = false;
  hwc_layer_1_t *hwcLayer = list[0]->getAndroidLayer();

  if (target->buffer != NULL)
    handle = target->buffer;
  else if (target->buffer2 != NULL)
    handle = target->buffer2;
  else
    ALOGE("parcel buffer params error");

  result = imagePlaneParcel(list);
  if (result == false) return result;

  for (int i = 0; i < R3P0_MAX_OSD_NUM; i++) {
    result = osdPlaneParcel(list, i);
    if (result == false) return result;
  }
  result = dstPlaneParcel(handle, mOutputWidth, mOutputHeight, target->format,
                          target->releaseFenceFd, hwcLayer->transform);
  if (result == false) return result;

  result = miscCfgParcel(0);

  return result;
}

int GspR3P0PlaneArray::run() {
  /*  uint32_t value = GSP_R3P0_PLANE_TRIGGER; */
  unsigned long value = GSP_R3P0_PLANE_TRIGGER;
  int ret = -1;

  return ioctl(mDevice, value, mConfigs);
}

int GspR3P0PlaneArray::acquireSignalFd() {
  return mConfigs[mConfigIndex].ld1.common.sig_fd;
}

int GspR3P0PlaneArray::set(SprdUtilSource *source, SprdUtilTarget *target,
                           uint32_t w, uint32_t h) {
  int ret = -1;
  int fd = -1;

  mOutputWidth = w;
  mOutputHeight = h;

  if (parcel(source, target) == false) {
    ALOGE("gsp r3p0plane parcel failed");
    return ret;
  }

  mConfigs[mConfigIndex].l0 = mImagePlane->getConfig();
  mConfigs[mConfigIndex].l1 = mOsdPlane[0]->getConfig();
  mConfigs[mConfigIndex].l2 = mOsdPlane[1]->getConfig();
  mConfigs[mConfigIndex].l3 = mOsdPlane[2]->getConfig();
  mConfigs[mConfigIndex].ld1 = mDstPlane->getConfig();

  ret = rotAdjust(&(mConfigs[mConfigIndex]), source->LayerList);
  if (ret) {
    ALOGE("gsp rotAdjust fail ret=%d.", ret);
    return ret;
  }

  ret = run();
  if (ret < 0) {
    ALOGE("trigger gsp device failed ret=%d.", ret);
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
