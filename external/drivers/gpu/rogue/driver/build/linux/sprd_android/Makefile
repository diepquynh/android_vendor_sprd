########################################################################### ###
#@Copyright     Copyright (c) Imagination Technologies Ltd. All Rights Reserved
#@License       Dual MIT/GPLv2
# 
# The contents of this file are subject to the MIT license as set out below.
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# Alternatively, the contents of this file may be used under the terms of
# the GNU General Public License Version 2 ("GPL") in which case the provisions
# of GPL are applicable instead of those above.
# 
# If you wish to allow use of your version of this file only under the terms of
# GPL, and not to allow others to use your version of this file under the terms
# of the MIT license, indicate your decision by deleting the provisions above
# and replace them with the notice and other provisions required by GPL as set
# out in the file called "GPL-COPYING" included in this distribution. If you do
# not delete the provisions above, a recipient may use your version of this file
# under the terms of either the MIT license or GPL.
# 
# This License is also included in this distribution in the file called
# "MIT-COPYING".
# 
# EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
# PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
# BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
# PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
### ###########################################################################
include ../config/preconfig.mk

# $(gettop) can get top dir in shell
# method: make file call bash shell

#defines
ARCH ?= x86_64
BUILD ?= debug
HAL_VARIANT ?= generic
LDM_PLATFORM := 1
RGX_BVNC ?= 8.48.2.39
RGX_NOHW ?= 0
# PDUMP := 1

ANDROID_ROOT ?= $(ANDROID_BUILD_TOP)
TARGET_DEVICE ?= $(notdir $(ANDROID_PRODUCT_OUT))
DISCIMAGE ?= $(ANDROID_PRODUCT_OUT)
KERNELDIR ?= $(ANDROID_PRODUCT_OUT)/obj/KERNEL
KERNEL_CROSS_COMPILE ?= x86_64-linux-android-
CROSS_COMPILE := $(ANDROID_ROOT)/prebuilts/gcc/linux-x86/x86/x86_64-linux-android-4.9/bin/$(KERNEL_CROSS_COMPILE)

ifeq ($(RGX_NOHW),1)
$(warning Warning-----------Now building no hardware user driver!)
PVR_SYSTEM := rgx_nohw
override NO_HARDWARE := 1
else
PVR_SYSTEM := rgx_sprd
override NO_HARDWARE := 0
endif


$(warning ******************checking the settings****************)
$(warning ANDROID_ROOT: $(ANDROID_ROOT) ANDROID_BUILD_TOP: $(ANDROID_BUILD_TOP))
$(warning ANDROID_PRODUCT_OUT: $(ANDROID_PRODUCT_OUT))
$(warning DEBUGMODE: $(BUILD) ARCH=$(ARCH))
$(warning RGX_NOHW=$(RGX_NOHW) PVR_SYSTEM: $(PVR_SYSTEM))
$(warning *******************************************************)


include ../common/android/paths.mk
include ../common/android/arch.mk
include ../common/android/features.mk


PDUMP_STREAMBUF_SIZE_MB := 10
include ../config/core.mk
include ../common/android/extra_config.mk

