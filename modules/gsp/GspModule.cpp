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

#include <cutils/log.h>
#include <errno.h>
#include <fcntl.h>
#include <hardware/hardware.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <unistd.h>
#include "GspDevice.h"
#include "SprdHWLayer.h"

#define GSP_HEADER_VERSION 1

#define GSP_DEVICE_API_VERSION_1_0 \
  HARDWARE_DEVICE_API_VERSION_2(1, 0, GSP_HEADER_VERSION)

#define GET_GSP_RETURN_X_IF_NULL(X, dev)          \
  GspDevice *gsp = static_cast<GspDevice *>(dev); \
  do {                                            \
    if (!gsp) {                                   \
      ALOGE("invalid GSP device.");               \
      return X;                                   \
    }                                             \
  } while (0)

#define GET_GSP_RETURN_ERROR_IF_NULL(dev) GET_GSP_RETURN_X_IF_NULL(-EINVAL, dev)
#define GET_GSP_RETURN_VOID_IF_NULL(dev) GET_GSP_RETURN_X_IF_NULL()

typedef struct gsp_module { struct hw_module_t common; } gsp_module_t;

namespace android {

static int gsp_device_prepare(gsp_device_1_t *dev, SprdHWLayer **list, int cnt,
                              bool *support) {
  GET_GSP_RETURN_ERROR_IF_NULL(dev);

  if (cnt < 1) {
    ALOGE("prepare layer count params error");
    return -1;
  }

  return gsp->prepare(list, cnt, support);
}

static int gsp_device_set(gsp_device_1_t *dev, SprdUtilSource *source,
                          SprdUtilTarget *target) {
  GET_GSP_RETURN_ERROR_IF_NULL(dev);

  return gsp->set(source, target);
}

static int gsp_device_updateOutputSize(gsp_device_1_t *dev, uint32_t w,
                                       uint32_t h) {
  GET_GSP_RETURN_ERROR_IF_NULL(dev);

  if (w < 4 || h < 4) {
    ALOGE("update output size params error");
    return -1;
  }

  return gsp->updateOutputSize(w, h);
}

static int gsp_device_close(struct hw_device_t *dev) {
  ALOGI("close gsp device");

  gsp_device_1_t *gsp = (gsp_device_1_t *)dev;

  GspDevice *gspDevice = static_cast<GspDevice *>(gsp);
  delete gspDevice;

  return 0;
}

static int gsp_device_open(const struct hw_module_t *module, const char *name,
                           struct hw_device_t **device) {
  if (strcmp(name, GSP_HARDWARE_MODULE_ID)) return -EINVAL;

  ALOGI("open gsp device module");

  GspDevice *gsp = new GspDevice();

  // setup GSP methods
  gsp->gsp_device_1_t::common.tag = HARDWARE_DEVICE_TAG;
  gsp->gsp_device_1_t::common.version = GSP_DEVICE_API_VERSION_1_0;
  gsp->gsp_device_1_t::common.module = const_cast<hw_module_t *>(module);
  gsp->gsp_device_1_t::common.close = gsp_device_close;
  gsp->gsp_device_1_t::prepare = gsp_device_prepare;
  gsp->gsp_device_1_t::commit = gsp_device_set;
  gsp->gsp_device_1_t::updateOutputSize = gsp_device_updateOutputSize;

  if (gsp->init() == false) {
    ALOGE("initialized failed");
    return -EINVAL;
  }

  *device = &gsp->gsp_device_1_t::common;

  return 0;
}

}  // namespace android

static hw_module_methods_t gsp_module_methods = {open : gsp_device_open};

gsp_module_t HAL_MODULE_INFO_SYM = {
  common : {
    tag : HARDWARE_MODULE_TAG,
    version_major : 1,
    version_minor : 0,
    id : GSP_HARDWARE_MODULE_ID,
    name : "Spreadtrum Graphic Signal Proccessor module",
    author : "Spreadtrum Communications, Inc",
    methods : &gsp_module_methods,
    dso : 0,
    reserved : {0},
  }
};
