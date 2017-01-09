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

#ifndef GSPPLANEARRAY_H_
#define GSPPLANEARRAY_H_

#include "GspDevice.h"
#include "GspPlane.h"
#include "SprdHWLayer.h"
#include "SprdUtil.h"

namespace android {

class GspPlaneArray {
 public:
  virtual ~GspPlaneArray() {}

  virtual int adapt(SprdHWLayer **list, int count);

  virtual int set(SprdUtilSource *source, SprdUtilTarget *target, uint32_t w,
                  uint32_t h);

  virtual int init(int fd);
};
}  // namespace android

#endif  // GSPPLANEARRAY_H_
