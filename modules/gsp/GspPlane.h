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
#ifndef GSPPLANE_H_
#define GSPPLANE_H_

#include "gralloc_priv.h"
#include "format_chooser.h"
#include "SprdHWLayer.h"
#include "gsp_cfg.h"

namespace android {

enum GspPlaneType { IMAGE_PLNAE = 0, OSD_PLANE, DST_PLANE, INVAL_PLANE };

struct GspRangeSize {
  GspRangeSize() {}
  ~GspRangeSize() {}
  GspRangeSize(struct gsp_rect cropMax, struct gsp_rect cropMin,
               struct gsp_rect outMax, struct gsp_rect outMin) {
    crop_max = cropMax;
    crop_min = cropMin;
    out_max = outMax;
    out_min = outMin;
    ALOGI("original crop max[%d %d %d %d], crop min[%d %d %d %d]",
          crop_max.st_x, crop_max.st_y, crop_max.rect_w, crop_max.rect_h,
          crop_min.st_x, crop_min.st_y, crop_min.rect_w, crop_min.rect_h);

    ALOGI("original out_max[%d %d %d %d], out_min[%d %d %d %d]", out_max.st_x,
          out_max.st_y, out_max.rect_w, out_max.rect_h, out_min.st_x,
          out_min.st_y, out_min.rect_w, out_min.rect_h);
  }

  GspRangeSize &operator=(const GspRangeSize &range) {
    crop_max = range.crop_max;
    crop_min = range.crop_min;
    out_max = range.out_max;
    out_min = range.out_min;

    ALOGI("assign crop max[%d %d %d %d], crop min[%d %d %d %d]", crop_max.st_x,
          crop_max.st_y, crop_max.rect_w, crop_max.rect_h, crop_min.st_x,
          crop_min.st_y, crop_min.rect_w, crop_min.rect_h);

    ALOGI("assgin out_max[%d %d %d %d], out_min[%d %d %d %d]", out_max.st_x,
          out_max.st_y, out_max.rect_w, out_max.rect_h, out_min.st_x,
          out_min.st_y, out_min.rect_w, out_min.rect_h);
    return *this;
  }

  void print() {
    ALOGI("crop max[%d %d %d %d], crop min[%d %d %d %d]", crop_max.st_x,
          crop_max.st_y, crop_max.rect_w, crop_max.rect_h, crop_min.st_x,
          crop_min.st_y, crop_min.rect_w, crop_min.rect_h);

    ALOGI("out_max[%d %d %d %d], out_min[%d %d %d %d]", out_max.st_x,
          out_max.st_y, out_max.rect_w, out_max.rect_h, out_min.st_x,
          out_min.st_y, out_min.rect_w, out_min.rect_h);
  }

  struct gsp_rect crop_max;
  struct gsp_rect crop_min;
  struct gsp_rect out_max;
  struct gsp_rect out_min;
};

class GspPlane {
 public:
  GspPlane();

  bool isProtectedVideo(struct private_handle_t *handle);
  bool isAttached();

  void attached(int index);

  int getIndex();

 protected:
  virtual ~GspPlane() {}
  virtual void reset(int flag);
  virtual bool adapt(SprdHWLayer *layer, int index);

  bool mAttached;
  int mIndex;
  bool mAsync;
  int mDebugFlag;
};

}  // namespace android

#endif  // GSPPLANE_H_
