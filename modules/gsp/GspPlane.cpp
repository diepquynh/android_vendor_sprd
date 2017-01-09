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

#include "GspPlane.h"

namespace android {
GspPlane::GspPlane() { mAttached = false; }

void GspPlane::reset(int flag) { mDebugFlag = flag; }

bool GspPlane::adapt(SprdHWLayer *layer, int index) {
  if (layer == NULL && index < 0) {
    ALOGI_IF(mDebugFlag, "GspPlane adapt params failed.");
    return false;
  }

  return true;
}

bool GspPlane::isAttached() { return mAttached; }

bool GspPlane::isProtectedVideo(struct private_handle_t *handle) {
  return handle->usage & GRALLOC_USAGE_PROTECTED ? true : false;
}

void GspPlane::attached(int index) {
  if (mAttached == true) {
    ALOGI_IF(mDebugFlag, "this plane hase been attached");
  } else {
    mAttached = true;
    mIndex = index;
  }
}

int GspPlane::getIndex() { return mIndex; }

}  // namespace android
