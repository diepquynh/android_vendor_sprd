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

# Basic support option tuning for Android
#
SUPPORT_ANDROID_PLATFORM := 1
SUPPORT_OPENGLES1_V1_ONLY := 1
DONT_USE_SONAMES := 1

# By default, the HAL_VARIANT should match TARGET_DEVICE
#
HAL_VARIANT ?= $(TARGET_DEVICE)

# Always print debugging after 5 seconds of no activity
#
CLIENT_DRIVER_DEFAULT_WAIT_RETRIES := 50

# Android WSEGL is always the same
#
OPK_DEFAULT := libpvrANDROID_WSEGL.so

# srvkm is always built
#
KERNEL_COMPONENTS := srvkm

# Kernel modules are always installed here under Android
#
ifeq ($(wildcard $(TARGET_ROOT)/product/$(TARGET_DEVICE)/vendor),)
PVRSRV_MODULE_BASEDIR := /lib/modules/
APP_DESTDIR := /data/app
BIN_DESTDIR := /system/vendor/bin
FW_DESTDIR := /system/vendor/firmware
else
ifeq ($(is_at_least_nougat),0)
# Platform versions prior to Nougat do not support installing system apps
# without Java code (only natives) to /vendor due to a bug in ClassLoader.
PVR_ANDROID_FORCE_APP_NATIVE_UNPACKED ?= 1
endif
PVRSRV_MODULE_BASEDIR := /vendor/modules/
APP_DESTDIR := /vendor/app
BIN_DESTDIR := /vendor/bin
FW_DESTDIR := /vendor/firmware
endif

# Disable server sync and multi sync features in Services
#
SUPPORT_SERVER_SYNC := 0

# Show GPU activity in systrace
#
SUPPORT_GPUTRACE_EVENTS ?= 1

# Enable source fortification by default
#
FORTIFY ?= 1

# Enable compressed debug sections by default
#
COMPRESS_DEBUG_SECTIONS ?= 1

# If the vncserver is being built, don't rely on libvncserver for now, and use
# the IMG rfblite feature instead
#
REMOTE_WITH_LIBVNCSERVER ?= 0

# Enable stack trace functions by default
#
PVRSRV_NEED_PVR_STACKTRACE ?= 1

# Enable PVR DVFS
#
PVR_DVFS ?= 1

# If the Android tree has the testkey.pk8 file, set it up and enable firmware
# signing. If the kernel source tree has a DER formatted version of the
# testkey.x509.pem file, in-kernel signature verification can also be enabled,
# and the Signer and KeyID fields will be added to the signature header.
#
RGX_FW_X509 ?= $(wildcard $(KERNELDIR)/testkey.x509)
ifeq ($(RGX_FW_X509),)
RGX_FW_X509 := $(wildcard $(KERNELDIR)/source/testkey.x509)
endif
RGX_FW_PK8 ?= $(wildcard $(ANDROID_ROOT)/build/target/product/security/testkey.pk8)
ifneq ($(RGX_FW_PK8),)
RGX_FW_SIGNED ?= 1
endif

##############################################################################
# Unless overridden by the user, assume the RenderScript Compute API level
# matches that of the SDK API_LEVEL.
#
RS_VERSION ?= $(API_LEVEL)
ifneq ($(findstring $(RS_VERSION),21 22),)
RS_VERSION := 20
endif

##############################################################################
# JB MR1 introduces cross-process syncs associated with a fd.
# This requires a new enough kernel version to have the base/sync driver.
#
EGL_EXTENSION_ANDROID_NATIVE_FENCE_SYNC ?= 1

ifeq ($(PDUMP),1)
# PDUMPs won't process if any native synchronization is enabled
override EGL_EXTENSION_ANDROID_NATIVE_FENCE_SYNC := 0
override SUPPORT_NATIVE_FENCE_SYNC := 0
override PVR_ANDROID_DEFER_CLEAR := 0
else
SUPPORT_NATIVE_FENCE_SYNC ?= 1
endif

##############################################################################
# Handle various platform cxxflags and includes
#
SYS_CXXFLAGS := -fuse-cxa-atexit $(SYS_CFLAGS)
ifeq ($(SUPPORT_ARC_PLATFORM),)
SYS_INCLUDES := \
 -isystem $(ANDROID_ROOT)/external/libcxx/include \
 $(SYS_INCLUDES) \
 -isystem $(ANDROID_ROOT)/external/zlib/src \
 -isystem $(ANDROID_ROOT)/libnativehelper/include/nativehelper
SYS_KHRONOS_INCLUDES += \
 -isystem $(ANDROID_ROOT)/frameworks/native/opengl/include
ifeq ($(is_at_least_nougat),1)
SYS_KHRONOS_INCLUDES += \
 -isystem $(ANDROID_ROOT)/frameworks/native/vulkan/include
endif
endif

##############################################################################
# Android doesn't use these install script variables. They're still in place
# because the Linux install scripts use them.
#
ifeq ($(SUPPORT_ARC_PLATFORM),)
 SHLIB_DESTDIR := not-used
 EGL_DESTDIR := not-used
endif

# Must give our EGL/GLES libraries a globally unique name
#
EGL_BASENAME_SUFFIX := _POWERVR_ROGUE

##############################################################################
# ICS requires that at least one driver EGLConfig advertises the
# EGL_RECORDABLE_ANDROID attribute. The platform requires that surfaces
# rendered with this config can be consumed by an OMX video encoder.
#
EGL_EXTENSION_ANDROID_RECORDABLE := 1

##############################################################################
# ICS added the EGL_ANDROID_blob_cache extension. Enable support for this
# extension in EGL/GLESv2.
#
EGL_EXTENSION_ANDROID_BLOB_CACHE ?= 1

##############################################################################
# Framebuffer target extension is used to find configs compatible with
# the framebuffer
#
EGL_EXTENSION_ANDROID_FRAMEBUFFER_TARGET := 1

##############################################################################
# This is currently broken on KK. Disable until fixed.
#
SUPPORT_ANDROID_APPHINTS := 0

##############################################################################
# KitKat added very provisional/early support for sRGB render targets
#
# (Leaving this optional until the framework makes it mandatory.)
#
PVR_ANDROID_HAS_HAL_PIXEL_FORMAT_sRGB ?= 1

##############################################################################
# Lollipop-mr1 added a libion wrapper for ion ioctls
#
PVR_ANDROID_HAS_LIBION ?= 1

##############################################################################
# Versions of Android prior to Nougat required Java 7 (OpenJDK).
#
ifeq ($(is_at_least_nougat),0)
LEGACY_USE_JAVA7 ?= 1
endif

##############################################################################
# Versions of Android prior to Nougat required .apk files to be processed with
# zipalign. Using this tool on Nougat or greater will corrupt the .apk file,
# as alignment is already done by signapk.jar, so we must disable it.
#
ifeq ($(is_at_least_nougat),0)
LEGACY_USE_ZIPALIGN ?= 1
endif

##############################################################################
# Marshmallow needs --soname turned on
#
ifeq ($(is_at_least_marshmallow),1)
PVR_ANDROID_NEEDS_SONAME ?= 1
endif

##############################################################################
# Marshmallow replaces RAW_SENSOR with RAW10, RAW12 and RAW16
#
ifeq ($(is_at_least_marshmallow),1)
PVR_ANDROID_HAS_HAL_PIXEL_FORMAT_RAWxx := 1
endif

##############################################################################
# Marshmallow has redesigned sRGB support
#
ifeq ($(is_at_least_marshmallow),1)
PVR_ANDROID_HAS_SET_BUFFERS_DATASPACE ?= 1
PVR_ANDROID_HAS_HAL_PIXEL_FORMAT_sRGB := 0
endif

##############################################################################
# Marshmallow has partial updates support
#
ifeq ($(is_at_least_marshmallow),0)
EGL_EXTENSION_PARTIAL_UPDATES := 0
endif

##############################################################################
# fenv was rewritten in Marshmallow
#
ifeq ($(is_at_least_marshmallow),1)
PVR_ANDROID_HAS_WORKING_FESETROUND := 1
endif

##############################################################################
# Marshmallow renderscript support
#
ifeq ($(is_at_least_marshmallow),1)
PVR_ANDROID_HAS_RS_INTERNAL_DEFINES := 1
PVR_ANDROID_HAS_SCRIPTGROUPBASE := 1
PVR_ANDROID_POST_L_HAL := 1
endif

##############################################################################
# Nougat has a substantially redesigned android_dataspace_t enum
#
ifeq ($(is_at_least_nougat),1)
PVR_ANDROID_HAS_SET_BUFFERS_DATASPACE_2 := 1
endif

##############################################################################
# Nougat advertises and utilizes the EGL_KHR_mutable_render_buffer extension
#
ifeq ($(is_at_least_nougat),1)
EGL_EXTENSION_MUTABLE_RENDER_BUFFER := 1
endif

##############################################################################
# Nougat has a new 'set_shared_buffer_mode' perform() function on the
# ANativeWindow object
ifeq ($(is_at_least_nougat),1)
PVR_ANDROID_HAS_SHARED_BUFFER := 1
endif

# Placeholder for future version handling
#
ifeq ($(is_future_version),1)
-include ../common/android/future_version.mk
endif
