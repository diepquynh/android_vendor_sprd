# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Warning: this is actually a product definition, to be inherited from

# On space-constrained devices, we include a subset of fonts:
# First, core/required fonts
# Created by SPRD
PRODUCT_COPY_FILES := \
    vendor/sprd/open-source/res/user_fonts/DroidSans.ttf:system/user_fonts/DroidSans.ttf \
    vendor/sprd/open-source/res/user_fonts/DroidSerif-Regular.ttf:system/user_fonts/DroidSerif-Regular.ttf

