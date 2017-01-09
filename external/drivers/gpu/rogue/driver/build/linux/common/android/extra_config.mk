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

# On Android, functions in libpthread and librt are included in libc. By
# defining $(libpthread_ldflags) to empty, we can prevent the build system
# from passing -lpthread, even if a module wants pthread. See
# moduledefs_libs.mk.
$(eval $(call UserConfigMake,libpthread_ldflags,))
$(eval $(call UserConfigMake,librt_ldflags,))

$(eval $(call UserConfigMake,TARGET_ROOT,$(TARGET_ROOT)))
$(eval $(call UserConfigMake,TARGET_DEVICE,$(TARGET_DEVICE)))

$(eval $(call BothConfigMake,SUPPORT_ANDROID_PLATFORM,1))
$(eval $(call BothConfigMake,SUPPORT_ION,1))
$(call NonTunableOption,SUPPORT_ION)

$(eval $(call BothConfigC,ANDROID,))
$(eval $(call KernelConfigC,SUPPORT_ION,))

$(eval $(call UserConfigC,SUPPORT_ANDROID_PLATFORM,))


# These are set automatically according to the platform version.

# These are user-tunable.

$(eval $(call TunableBothConfigC,PVR_ANDROID_HAS_SW_INCOMPATIBLE_FRAMEBUFFER,,\
Enable this to support running Android$(apos)s software GLES renderer_\
with gralloc from the DDK._\
))





# These are set automatically according to the platform version.

# These are user-tunable.




ifeq ($(NO_HARDWARE),1)
 override PVR_ANDROID_COMPOSERHAL := null
else ifeq ($(PVR_ANDROID_VNC),1)
 override PVR_ANDROID_COMPOSERHAL := vnc
endif




# Support kernels built out-of-tree with O=/other/path
# In those cases, KERNELDIR will be O, not the source tree.
ifneq ($(wildcard $(KERNELDIR)/source),)
KSRCDIR := $(KERNELDIR)/source
else
KSRCDIR := $(KERNELDIR)
endif
ifneq ($(wildcard $(KSRCDIR)/drivers/staging/android/ion/ion.h),)
# The kernel has a more recent version of ion, located in drivers/staging.
# Change the default header paths and the behaviour wrt sg_dma_len.
PVR_ANDROID_ION_HEADER := \"../drivers/staging/android/ion/ion.h\"
PVR_ANDROID_ION_PRIV_HEADER := \"../drivers/staging/android/ion/ion_priv.h\"
PVR_ANDROID_ION_USE_SG_LENGTH := 1
endif
ifneq ($(wildcard $(KSRCDIR)/drivers/staging/android/sync.h),)
# The kernel has a more recent version of the sync driver, located in
# drivers/staging. Change the default header path.
PVR_ANDROID_SYNC_HEADER := \"../drivers/staging/android/sync.h\"
endif
$(eval $(call TunableKernelConfigC,PVR_ANDROID_ION_HEADER,\"linux/ion.h\"))
$(eval $(call TunableKernelConfigC,PVR_ANDROID_ION_PRIV_HEADER,\"../drivers/gpu/ion/ion_priv.h\"))
$(eval $(call TunableKernelConfigC,PVR_ANDROID_ION_USE_SG_LENGTH,))
$(eval $(call TunableKernelConfigC,PVR_ANDROID_SYNC_HEADER,\"linux/sync.h\"))

$(eval $(call TunableKernelConfigC,ADF_FBDEV_NUM_PREFERRED_BUFFERS,))

# Newer Android versions have moved the libdrm includes in an incompatible
# way. We need to compile-time detect the new location, and if it's missing,
# use the old structure.
ifeq ($(wildcard $(ANDROID_ROOT)/external/libdrm/include/drm),)
$(eval $(call UserConfigC,PVR_ANDROID_OLD_LIBDRM_HEADER_PATH,))
$(eval $(call UserConfigMake,PVR_ANDROID_OLD_LIBDRM_HEADER_PATH,1))
endif

# Most development systems will have at least one copy of java, but some may
# have more. If the build system detected that a specific 'forced' version
# of java should be used, and the user didn't override JAVAC, try to set it
# up here. We'll use JAVA_HOME if it's set, falling back to PATH if it is
# not.
ifeq ($(JAVAC),)
 # If JAVA_HOME is unset, implement some assumed paths taken from Android's
 # build/envsetup.sh script (these are intentionally Ubuntu centric).
 ifeq ($(JAVA_HOME),)
  ifeq ($(LEGACY_USE_JAVA7),1)
   JAVA_HOME ?= /usr/lib/jvm/java-7-openjdk-amd64
  else
   JAVA_HOME ?= /usr/lib/jvm/java-8-openjdk-amd64
  endif
  ifeq ($(wildcard $(JAVA_HOME)),)
   JAVA_HOME :=
  endif
 endif

 ifeq ($(JAVA_HOME),)
  JAVA ?= java
  JAVAC ?= javac
 else
  JAVA := $(JAVA_HOME)/bin/java
  JAVAC := $(JAVA_HOME)/bin/javac
  ifeq ($(wildcard $(JAVAC)),)
   $(error JAVA_HOME does not point to a valid java installation)
  endif
 endif

 # Test the configured JDK for validity
 ifeq ($(LEGACY_USE_JAVA7),1)
  ifeq ($(shell $(JAVA) -version 2>&1 | grep -qe 'OpenJDK.*7u' && echo 1 || echo 0),0)
   $(error '$(JAVA) -version' was not for OpenJDK 7)
  endif
 else
  ifeq ($(shell $(JAVA) -version 2>&1 | grep -qe 'OpenJDK.*1\.8\.' && echo 1 || echo 0),0)
   $(error '$(JAVA) -version' was not for OpenJDK 8)
  endif
 endif
 $(eval $(call UserConfigMake,JAVA,$(JAVA)))
 $(eval $(call UserConfigMake,JAVAC,$(JAVAC)))
endif
