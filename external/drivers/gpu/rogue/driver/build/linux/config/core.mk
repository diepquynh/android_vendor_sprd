########################################################################### ###
#@File
#@Title         Root build configuration.
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

# Configuration wrapper for new build system. This file deals with
# configuration of the build. Add to this file anything that deals
# with switching driver options on/off and altering the defines or
# objects the build uses.
#
# At the end of this file is an exhaustive list of all variables
# that are passed between the platform/config stage and the generic
# build. PLEASE refrain from adding more variables than necessary
# to this stage -- almost all options can go through config.h.
#

# Sanity check: Make sure preconfig has been included
ifeq ($(TOP),)
$(error TOP not defined: Was preconfig.mk included in root makefile?)
endif

################################# MACROS ####################################

ALL_TUNABLE_OPTIONS :=

# This records the config option's help text and default value. Note that
# the help text can't contain a literal comma. Use $(comma) instead.
define RegisterOptionHelp
ALL_TUNABLE_OPTIONS += $(1)
ifeq ($(INTERNAL_DESCRIPTION_FOR_$(1)),)
INTERNAL_DESCRIPTION_FOR_$(1) := $(3)
endif
INTERNAL_CONFIG_DEFAULT_FOR_$(1) := $(2)
$(if $(4),\
	$(error Too many arguments in config option '$(1)' (stray comma in help text?)))
endef

# Write out a kernel GNU make option.
#
define KernelConfigMake
$$(shell echo "override $(1) := $(2)" >>$(CONFIG_KERNEL_MK).new)
$(if $(filter config,$(D)),$(info KernelConfigMake $(1) := $(2)	# $(if $($(1)),$(origin $(1)),default)))
endef

# Write out a GNU make option for both user & kernel
#
define BothConfigMake
$$(eval $$(call KernelConfigMake,$(1),$(2)))
$$(eval $$(call UserConfigMake,$(1),$(2)))
endef

# Conditionally write out a kernel GNU make option
#
define _TunableKernelConfigMake
ifneq ($$($(1)),)
ifneq ($$($(1)),0)
$$(eval $$(call KernelConfigMake,$(1),$$($(1))))
endif
else
ifneq ($(2),)
$$(eval $$(call KernelConfigMake,$(1),$(2)))
endif
endif
endef

define TunableKernelConfigMake
$$(eval $$(call _TunableKernelConfigMake,$(1),$(2)))
$(call RegisterOptionHelp,$(1),$(2),$(3),$(4))
endef

# Conditionally write out a GNU make option for both user & kernel
#
define TunableBothConfigMake
$$(eval $$(call _TunableKernelConfigMake,$(1),$(2)))
$$(eval $$(call _TunableUserConfigMake,$(1),$(2)))
$(call RegisterOptionHelp,$(1),$(2),$(3),$(4))
endef

# Write out a kernel-only option
#
define KernelConfigC
$$(shell echo "#define $(1) $(2)" >>$(CONFIG_KERNEL_H).new)
$(if $(filter config,$(D)),$(info KernelConfigC    #define $(1) $(2)	/* $(if $($(1)),$(origin $(1)),default) */),)
endef

# Write out an option for both user & kernel
#
define BothConfigC
$$(eval $$(call KernelConfigC,$(1),$(2)))
$$(eval $$(call UserConfigC,$(1),$(2)))
endef

# AppHint defaults are KM and UM if SUPPORT_KERNEL_SRVINIT is not defined
#
define AppHintConfigC
ifneq ($$($(1)),)
$$(eval $$(call BothConfigC,$(1),$$($(1))))
else
$$(eval $$(call BothConfigC,$(1),$(2)))
endif
$(call RegisterOptionHelp,$(1),$(2),$(3),$(4))
endef

# Conditionally write out a kernel-only option
#
define _TunableKernelConfigC
ifneq ($$($(1)),)
ifneq ($$($(1)),0)
ifeq ($$($(1)),1)
$$(eval $$(call KernelConfigC,$(1),))
else
$$(eval $$(call KernelConfigC,$(1),$$($(1))))
endif
endif
else
ifneq ($(2),)
ifeq ($(2),1)
$$(eval $$(call KernelConfigC,$(1),))
else
$$(eval $$(call KernelConfigC,$(1),$(2)))
endif
endif
endif
endef

define TunableKernelConfigC
$$(eval $$(call _TunableKernelConfigC,$(1),$(2)))
$(call RegisterOptionHelp,$(1),$(2),$(3),$(4))
endef

# Conditionally write out an option for both user & kernel
#
define TunableBothConfigC
$$(eval $$(call _TunableKernelConfigC,$(1),$(2)))
$$(eval $$(call _TunableUserConfigC,$(1),$(2)))
$(call RegisterOptionHelp,$(1),$(2),$(3),$(4))
endef

# Use this to mark config options which have to exist, but aren't
# user-tunable. Warn if an attempt is made to change it.
#
define NonTunableOption
$(if $(filter command line environment,$(origin $(1))),\
	$(error Changing '$(1)' is not supported))
endef

############################### END MACROS ##################################

# Check we have a new enough version of GNU make.
#
need := 3.81
ifeq ($(filter $(need),$(firstword $(sort $(MAKE_VERSION) $(need)))),)
$(error A version of GNU make >= $(need) is required - this is version $(MAKE_VERSION))
endif

include ../defs.mk

# Infer PVR_BUILD_DIR from the directory configuration is launched from.
# Check anyway that such a directory exists.
#
PVR_BUILD_DIR := $(notdir $(abspath .))
$(call directory-must-exist,$(TOP)/build/linux/$(PVR_BUILD_DIR))

# Output directory for configuration, object code,
# final programs/libraries, and install/rc scripts.
#
BUILD        ?= release
ifneq ($(filter $(WINDOW_SYSTEM),xorg wayland nullws nulldrmws screen surfaceless gigacluster_ws lws-generic),)
OUT          ?= $(TOP)/binary_$(PVR_BUILD_DIR)_$(WINDOW_SYSTEM)_$(BUILD)
else
OUT          ?= $(TOP)/binary_$(PVR_BUILD_DIR)_$(BUILD)
endif

# Set correct output subdirectory for host or guest virtualisation builds
#
ifeq ($(SUPPORT_PVRSRV_GPUVIRT),1)
ifeq ($(PVRSRV_GPUVIRT_GUESTDRV),)
  GPUVIRT_HOST_DIR ?= host
  OUT := $(OUT)/$(GPUVIRT_HOST_DIR)
else
  GPUVIRT_GUEST_DIR ?= guest
  OUT := $(OUT)/$(GPUVIRT_GUEST_DIR)
endif
endif

override OUT := $(if $(filter /%,$(OUT)),$(OUT),$(TOP)/$(OUT))

# CONFIG_MK			:= $(OUT)/config.mk
CONFIG_H			:= $(OUT)/config.h
CONFIG_KERNEL_MK	:= $(OUT)/config_kernel.mk
CONFIG_KERNEL_H		:= $(OUT)/config_kernel.h

# Convert commas to spaces in $(D). This is so you can say "make
# D=config-changes,freeze-config" and have $(filter config-changes,$(D))
# still work.
override D := $(subst $(comma),$(space),$(D))

# Create the OUT directory
#
$(shell mkdir -p $(OUT))

# Some targets don't need information about any modules. If we only specify
# these targets on the make command line, set INTERNAL_CLOBBER_ONLY to
# indicate that toplevel.mk shouldn't read any makefiles
CLOBBER_ONLY_TARGETS := clean clobber help install
INTERNAL_CLOBBER_ONLY :=
ifneq ($(strip $(MAKECMDGOALS)),)
INTERNAL_CLOBBER_ONLY := \
$(if \
 $(strip $(foreach _cmdgoal,$(MAKECMDGOALS),\
          $(if $(filter $(_cmdgoal),$(CLOBBER_ONLY_TARGETS)),,x))),,true)
endif

# For a clobber-only build, we shouldn't regenerate any config files
ifneq ($(INTERNAL_CLOBBER_ONLY),true)

-include ../config/user-defs.mk

#
# Core handling


# delete any previous intermediary files
$(shell \
	for file in $(CONFIG_KERNEL_H).new $(CONFIG_KERNEL_MK).new ; do \
		rm -f $$file; \
	done)

# Extract the BNC config name
RGX_BNC_SPLIT := $(subst .,$(space) ,$(RGX_BVNC))
RGX_BNC := $(word 1,$(RGX_BNC_SPLIT)).V.$(word 3,$(RGX_BNC_SPLIT)).$(word 4,$(RGX_BNC_SPLIT))

# Check BVNC core version
ALL_KM_BVNCS := \
 $(patsubst rgxcore_km_%.h,%,\
   $(notdir $(shell ls $(TOP)/hwdefs/km/cores/rgxcore_km_*.h)))
ifeq ($(filter $(RGX_BVNC),$(ALL_KM_BVNCS)),)
$(error Error: Invalid Kernel core RGX_BVNC=$(RGX_BVNC). \
   Valid Kernel core BVNCs: $(subst $(space),$(comma)$(space),$(ALL_KM_BVNCS)))
endif

# Check if BVNC core file exist
RGX_BVNC_CORE_KM := $(TOP)/hwdefs/km/cores/rgxcore_km_$(RGX_BVNC).h
RGX_BVNC_CORE_KM_HEADER := \"cores/rgxcore_km_$(RGX_BVNC).h\"
# "rgxcore_km_$(RGX_BVNC).h"
ifeq ($(wildcard $(RGX_BVNC_CORE_KM)),)
$(error The file $(RGX_BVNC_CORE_KM) does not exist. \
   Valid BVNCs: $(ALL_KM_BVNCS))
endif

# Check BNC config version
ALL_KM_BNCS := \
 $(patsubst rgxconfig_km_%.h,%,\
   $(notdir $(shell ls $(TOP)/hwdefs/km/configs/rgxconfig_km_*.h)))
ifeq ($(filter $(RGX_BNC),$(ALL_KM_BNCS)),)
$(error Error: Invalid Kernel config RGX_BNC=$(RGX_BNC). \
   Valid Kernel config BNCs: $(subst $(space),$(comma)$(space),$(ALL_KM_BNCS)))
endif

# Check if BNC config file exist
RGX_BNC_CONFIG_KM := $(TOP)/hwdefs/km/configs/rgxconfig_km_$(RGX_BNC).h
RGX_BNC_CONFIG_KM_HEADER := \"configs/rgxconfig_km_$(RGX_BNC).h\"
#"rgxcore_km_$(RGX_BNC).h"
ifeq ($(wildcard $(RGX_BNC_CONFIG_KM)),)
$(error The file $(RGX_BNC_CONFIG_KM) does not exist. \
   Valid BNCs: $(ALL_KM_BNCS))
endif

# Enforced dependencies. Move this to an include.
#
SUPPORT_LINUX_USING_WORKQUEUES ?= 1
ifeq ($(SUPPORT_LINUX_USING_WORKQUEUES),1)
override PVR_LINUX_USING_WORKQUEUES := 1
override PVR_LINUX_MISR_USING_PRIVATE_WORKQUEUE := 1
override PVR_LINUX_TIMERS_USING_WORKQUEUES := 1
else ifeq ($(SUPPORT_LINUX_USING_SHARED_WORKQUEUES),1)
override PVR_LINUX_USING_WORKQUEUES := 1
override PVR_LINUX_MISR_USING_WORKQUEUE := 1
override PVR_LINUX_TIMERS_USING_SHARED_WORKQUEUE := 1
endif

# Linux based platforms initialize and load the firmware from the 
# Services Server component which runs in a kernel module.
# Use of user-mode/client (pvrsrvctl) initialization is deprecated for Linux
# platforms but may be used in DDK ports for non-Linux OS. In these cases
# SUPPORT_KERNEL_SRVINIT=0 must be defined on the build make line.
#
SUPPORT_KERNEL_SRVINIT ?= 1

# Rather than requiring the user to have to define two variables (one quoted,
# one not), make PVRSRV_MODNAME a non-tunable and give it an overridable
# default here.
#
ifeq ($(PVRSRV_GPUVIRT_MULTIDRV_MODEL),)
PVRSRV_MODNAME := pvrsrvkm
else
# On a multi-driver model, append OSID to module name
PVRSRV_MODNAME := pvrsrvkm$(PVRSRV_GPUVIRT_GUESTDRV)
endif

# Normally builds don't touch these, but we use them to influence the
# components list. Make sure these are defined early enough to make this
# possible.
#
SUPPORT_RAY_TRACING := \
 $(shell grep -qw RGX_FEATURE_RAY_TRACING $(RGX_BNC_CONFIG_KM) && echo 1)

SUPPORT_META_DMA :=\
 $(shell grep -qw RGX_FEATURE_META_DMA $(RGX_BNC_CONFIG_KM) && echo 1)

SUPPORT_META_COREMEM :=\
 $(shell grep -qe 'RGX_FEATURE_META_COREMEM_SIZE ([123456789][1234567890]*)' $(RGX_BNC_CONFIG_KM) && echo 1)

SUPPORT_COMPUTE := \
 $(shell grep -qw RGX_FEATURE_COMPUTE $(RGX_BNC_CONFIG_KM) && echo 1)

SUPPORT_OPENCL_2_X ?= \
 $(shell grep -qw "RGX_FEATURE_CDM_CONTROL_STREAM_FORMAT (2)" $(RGX_BNC_CONFIG_KM) && echo 1)

SUPPORT_MIPS_FIRMWARE :=\
 $(shell grep -qw RGX_FEATURE_MIPS $(RGX_BNC_CONFIG_KM) && echo 1)

SUPPORT_TLA :=\
 $(shell grep -qw RGX_FEATURE_TLA $(RGX_BNC_CONFIG_KM) && echo 1)

SUPPORT_FASTRENDER_DM :=\
 $(shell grep -qw RGX_FEATURE_FASTRENDER_DM $(RGX_BNC_CONFIG_KM) && echo 1)

SUPPORT_SIGNAL_FILTER := \
 $(shell grep -qw RGX_FEATURE_SIGNAL_SNOOPING $(RGX_BNC_CONFIG_KM) && echo 1)

ifneq ($(wildcard $(RGX_BNC_CONFIG_H)),)
 SUPPORT_ES32 :=\
  $(shell grep -qw RGX_FEATURE_ASTC $(RGX_BNC_CONFIG_H) && grep -qw RGX_FEATURE_GS_RTA_SUPPORT $(RGX_BNC_CONFIG_KM) && echo 1)
endif

# Default place for binaries and shared libraries
BIN_DESTDIR ?= /usr/local/bin
SHARE_DESTDIR ?= /usr/local/share
SHLIB_DESTDIR ?= /usr/lib
FW_DESTDIR ?= /lib/firmware

# Build's selected list of components.
# - components.mk is a per-build file that specifies the components that are
#   to be built
-include components.mk

# Set up the host and target compiler.
include ../config/compiler.mk

# PDUMP needs extra components
#
ifeq ($(PDUMP),1)
ifneq ($(COMPONENTS),)
COMPONENTS += pdump
ifeq ($(PDUMP_DEBUG_OUTFILES),1)
ifneq ($(COMPONENTS),)
COMPONENTS += pdump_check
endif
endif
endif
EXTRA_PVRSRVKM_COMPONENTS += dbgdrv
endif

# HWPerf KM Interface example
#
ifeq ($(SUPPORT_KERNEL_HWPERF_TEST),1)
KERNEL_COMPONENTS += rgxhwpdrv
endif

# PVRGDB needs extra components
#
ifeq ($(PVRGDB),1)
ifneq ($(COMPONENTS),)
COMPONENTS += pvrdebugger pvrgdb pvrdebugipc pvrdebugipc_header
ifneq ($(filter opencl,$(COMPONENTS)),)
COMPONENTS += gdb_ocl_test
endif
endif
endif


ifneq ($(SUPPORT_BUILD_LWS),)
 ifneq ($(SYSROOT),)
  $(info WARNING: You have specified a SYSROOT (or are using a buildroot compiler) and enabled SUPPORT_BUILD_LWS.)
  $(info          We will ignore the sysroot and will build all required LWS components.)
  $(info          Unset SUPPORT_BUILD_LWS if this is not what you want.)
 endif
 override SYSROOT:=
endif


ifneq ($(strip $(LWS_PREFIX)),)
 $(eval $(call UserConfigMake,LWS_PREFIX,$(LWS_PREFIX)))
endif

ifeq ($(SUPPORT_BUILD_LWS),1)
 COMPONENTS += ${LWS_COMPONENTS}
endif

# RenderScript Replay needs extra components
ifeq ($(RSCREPLAY),1)
ifneq ($(COMPONENTS),)
COMPONENTS += libufwriter rscompiler renderscript renderscript_sha1 rscreplay replay_rsdriver
endif
endif

# At present it's necessary to specify a physical heap on which to import
# DMA-BUFs. On most platforms there will typically be a single heap so
# instead of having to specify this in every Makefile set a default heap ID
# here.
DMABUF_IMPORT_PHYSHEAP_ID ?= 0

$(if $(filter config,$(D)),$(info Build configuration:))

################################# CONFIG ####################################

ifneq ($(SUPPORT_NEUTRINO_PLATFORM), 1)

# If KERNELDIR is set, write it out to the config.mk, with
# KERNEL_COMPONENTS and KERNEL_ID
#
ifneq ($(strip $(KERNELDIR)),)
PVRSRV_MODULE_BASEDIR ?= /lib/modules/$(KERNEL_ID)/extra/
$(eval $(call BothConfigMake,KERNELDIR,$(KERNELDIR)))
$(eval $(call BothConfigMake,KERNEL_ID,$(KERNEL_ID)))
$(eval $(call BothConfigMake,PVRSRV_MODULE_BASEDIR,$(PVRSRV_MODULE_BASEDIR)))
$(eval $(call BothConfigMake,KERNEL_COMPONENTS,$(KERNEL_COMPONENTS)))
$(eval $(call TunableKernelConfigMake,EXTRA_PVRSRVKM_COMPONENTS,,\
List of components that should be built in to pvrsrvkm.ko$(comma) rather than_\
forming separate kernel modules._\
))

# If KERNEL_CROSS_COMPILE is set to "undef", this is magically
# equivalent to being unset. If it is unset, we use CROSS_COMPILE
# (which might also be unset). If it is set, use it directly.
ifneq ($(KERNEL_CROSS_COMPILE),undef)
KERNEL_CROSS_COMPILE ?= $(CROSS_COMPILE)
$(eval $(call TunableBothConfigMake,KERNEL_CROSS_COMPILE,))
endif

# Alternatively, allow the CC used for kbuild to be overridden
# exactly, bypassing any KERNEL_CROSS_COMPILE configuration.
$(eval $(call TunableBothConfigMake,KERNEL_CC,))

# Check the KERNELDIR has a kernel built and also check that it is
# not 64-bit, which we do not support.
KERNEL_AUTOCONF := \
 $(strip $(wildcard $(KERNELDIR)/include/linux/autoconf.h) \
         $(wildcard $(KERNELDIR)/include/generated/autoconf.h))
ifeq ($(KERNEL_AUTOCONF),)
$(warning autoconf.h not found in $$(KERNELDIR)/include/linux \
or $$(KERNELDIR)/include/generated. Check your $$(KERNELDIR) variable \
and kernel configuration.)
endif
else
$(if $(KERNEL_COMPONENTS),$(warning KERNELDIR is not set. Kernel components cannot be built))
endif

endif # !Neutrino

$(eval $(call UserConfigC,PVRSRV_MODULE_BASEDIR,\"$(PVRSRV_MODULE_BASEDIR)\"))

# Ideally configured by platform Makefiles, as necessary
#
SHADER_DESTDIR := $(SHARE_DESTDIR)/pvr/shaders/
$(eval $(call UserConfigMake,SHADER_DESTDIR,"$(SHADER_DESTDIR)"))
$(eval $(call UserConfigC,SHADER_DIR,"\"$(SHADER_DESTDIR)\""))

$(eval $(call TunableBothConfigC,RGX_FW_HEAP_SHIFT, 25,\
Firmware physical heap log2 size per OSID (minimum 4MiB, default 32MiB)._\
))

ifeq ($(RGX_FW_SIGNED),1)
ifeq ($(RGX_FW_PK8),)
$(error RGX_FW_PK8 must be set for RGX_FW_SIGNED=1.)
endif # !RGX_FW_PK8
$(eval $(call UserConfigMake,RGX_FW_SIGNED,1))
$(eval $(call UserConfigMake,RGX_FW_FILENAME,rgx.fw.signed))
$(eval $(call TunableBothConfigC,RGX_FW_PKCS1_PSS_PADDING,))
else  # RGX_FW_SIGNED
$(eval $(call UserConfigMake,RGX_FW_FILENAME,rgx.fw))
endif # RGX_FW_SIGNED

ifeq ($(RGX_FW_SIGNED),1)
$(eval $(call KernelConfigC,RGX_FW_FILENAME,"\"rgx.fw.signed\""))
ifneq ($(RGX_FW_X509),)
$(eval $(call KernelConfigC,RGX_FW_SIGNED,1))
endif # RGX_FW_X509
else  # RGX_FW_SIGNED
$(eval $(call KernelConfigC,RGX_FW_FILENAME,"\"rgx.fw\""))
endif # RGX_FW_SIGNED

ifneq ($(SUPPORT_ANDROID_PLATFORM),1)
  LLVM_STATICALLY_LINKED ?= 1
else
 ifneq ($(LLVM_STATICALLY_LINKED),)
  $(error LLVM_STATICALLY_LINKED set but Android only supports dynamic linking of LLVM.)
 endif
endif


LLVM_BUILD_TYPE ?= Release+Asserts

ifneq ($(LLVM_STATICALLY_LINKED),)
 ifeq ($(wildcard ${TOP}/build/linux/tools/prepare-llvm.sh),)
  # No facility for using LLVM in this package.
 else ifeq ($(LLVM_BUILD_DIR),)
  $(warning LLVM_BUILD_DIR is not set. Components that use it (e.g., OpenCL, Vulkan) cannot be built)
 else
  override LLVM_BUILD_DIR := $(abspath $(LLVM_BUILD_DIR))
  LLVM_MESSAGE=$(shell ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} ${TOP}/build/linux/tools/prepare-llvm.sh -c $(LLVM_BUILD_DIR))

  ifneq ($(filter Error:,$(firstword $(LLVM_MESSAGE))),)
   $(info  *** prepare-llvm.sh: $(LLVM_MESSAGE))
   $(error *** LLVM_BUILD_DIR $(LLVM_BUILD_DIR) is not suitable)
  endif

  ifneq ($(filter Warning:,$(firstword $(LLVM_MESSAGE))),)
   $(info  *** prepare-llvm.sh: $(LLVM_MESSAGE))
  endif

  # Because we need to handle MULTIARCH builds, we can't work out the
  # architecture to use in the paths until compile-time.  So leave
  # _LLVM_ARCH_ as a placeholder that will be replaced in the
  # moduledef.
  LLVM_INCLUDE_PATH := $(LLVM_BUILD_DIR)/llvm.src/include \
                       $(LLVM_BUILD_DIR)/llvm._LLVM_ARCH_/include

  CLANG_INCLUDE_PATH := $(LLVM_INCLUDE_PATH) \
                        $(LLVM_BUILD_DIR)/llvm.src/tools/clang/include \
                        $(LLVM_BUILD_DIR)/llvm._LLVM_ARCH_/tools/clang/include

  LLVM_INCLUDE_PATH_HOST := $(LLVM_BUILD_DIR)/llvm.src/include/ \
                            $(LLVM_BUILD_DIR)/llvm._LLVM_ARCH_/include/ \

  CLANG_INCLUDE_PATH_HOST := $(LLVM_INCLUDE_PATH_HOST) \
                             $(LLVM_BUILD_DIR)/llvm.src/tools/clang/include \
                             $(LLVM_BUILD_DIR)/llvm._LLVM_ARCH_/tools/clang/include

  LLVM_LIB_PATH := $(LLVM_BUILD_DIR)/llvm._LLVM_ARCH_/$(LLVM_BUILD_TYPE)/lib/
 endif
endif






$(if $(USE_CCACHE),$(if $(USE_DISTCC),$(error\
Enabling both USE_CCACHE and USE_DISTCC at the same time is not supported)))


# Invariant options for Linux
#
$(eval $(call BothConfigC,LINUX,))

$(eval $(call BothConfigC,PVR_BUILD_DIR,"\"$(PVR_BUILD_DIR)\""))
$(eval $(call BothConfigC,PVR_BUILD_TYPE,"\"$(BUILD)\""))
$(eval $(call BothConfigC,PVRSRV_MODNAME,"\"$(PVRSRV_MODNAME)\""))
$(eval $(call BothConfigMake,PVRSRV_MODNAME,$(PVRSRV_MODNAME)))
$(eval $(call BothConfigMake,PVR_BUILD_DIR,$(PVR_BUILD_DIR)))
$(eval $(call BothConfigMake,PVR_BUILD_TYPE,$(BUILD)))

SUPPORT_RGX ?= 1
ifeq ($(SUPPORT_RGX),1)
$(eval $(call BothConfigC,SUPPORT_RGX,1))
$(eval $(call UserConfigMake,SUPPORT_RGX,1))
endif

# Some of the definitions in stdint.h aren't exposed by default in C++ mode,
# unless these macros are defined. To make sure we get these definitions
# regardless of which files include stdint.h, define them here.
$(eval $(call UserConfigC,__STDC_CONSTANT_MACROS,))
$(eval $(call UserConfigC,__STDC_FORMAT_MACROS,))
$(eval $(call UserConfigC,__STDC_LIMIT_MACROS,))

# FIXME: We can't use GCC __thread reliably with clang.
ifeq ($(_CLANG),true)
$(eval $(call UserConfigC,PVR_TLS_USE_PTHREAD_xETSPECIFIC,))
else
$(eval $(call UserConfigC,PVR_TLS_USE_GCC__thread_KEYWORD,))
endif

ifneq ($(DISPLAY_CONTROLLER),)
$(eval $(call BothConfigC,DISPLAY_CONTROLLER,$(DISPLAY_CONTROLLER)))
$(eval $(call BothConfigMake,DISPLAY_CONTROLLER,$(DISPLAY_CONTROLLER)))
endif

$(eval $(call UserConfigC,OPK_DEFAULT,"\"$(OPK_DEFAULT)\""))
$(eval $(call UserConfigC,OPK_FALLBACK,"\"$(OPK_FALLBACK)\""))

$(eval $(call BothConfigMake,PVR_SYSTEM,$(PVR_SYSTEM)))
$(eval $(call KernelConfigMake,PVR_LOADER,$(PVR_LOADER)))

$(eval $(call KernelConfigC,DMABUF_IMPORT_PHYSHEAP_ID, $(DMABUF_IMPORT_PHYSHEAP_ID)))

ifeq ($(MESA_EGL),1)
$(eval $(call UserConfigMake,LIB_IMG_EGL,pvr_dri_support))
$(eval $(call UserConfigC,LIB_IMG_EGL_NAME,\"libpvr_dri_support.so\"))
$(eval $(call UserConfigC,LIB_IMG_OGL_NAME,\"libPVROGL_MESA.so\"))
else
$(eval $(call UserConfigMake,LIB_IMG_EGL,IMGegl))
$(eval $(call UserConfigC,LIB_IMG_EGL_NAME,\"libIMGegl.so\"))
$(eval $(call UserConfigC,LIB_IMG_OGL_NAME,\"libGL.so\"))
endif

$(eval $(call TunableBothConfigMake,SUPPORT_USER_REGISTER_CONFIGURATION,,\
Internal use only._\
))
$(eval $(call TunableBothConfigC,SUPPORT_USER_REGISTER_CONFIGURATION,,\
Internal use only._\
))

# Build-type dependent options
#
$(eval $(call BothConfigMake,BUILD,$(BUILD)))

# Prevent excluding regconfig bridge when the build level macro defined,
# regconfig functions are used in openrl and pvrdebug.
#
ifeq ($(SUPPORT_USER_REGISTER_CONFIGURATION),1)
ifeq ($(EXCLUDE_REGCONFIG_BRIDGE),1)
override EXCLUDE_REGCONFIG_BRIDGE := 0
endif
endif

# Include validation bridge when build level macro is defined.
#
ifeq ($(SUPPORT_VALIDATION),1)
override SUPPORT_VALIDATION_BRIDGE := 1
endif

ifeq ($(BUILD),debug)
$(eval $(call TunableBothConfigMake,PVR_SERVICES_DEBUG,,\
Enable additional services debug options._\
This needs to be enabled for both the UM and KM builds_\
so that compatibility between them is achieved.\
))
ifeq ($(PVR_SERVICES_DEBUG),1)
PVR_RI_DEBUG ?= 1
SUPPORT_PAGE_FAULT_DEBUG ?= 1
PVRSRV_DEBUG_HANDLE_LOCK ?= 1
endif
# Client CCB usage statistics enabled by default in debug builds
PVRSRV_ENABLE_CCCB_UTILISATION_DEBUG ?= 1
# bridge debug and statistics enabled by default in debug builds
DEBUG_BRIDGE_KM ?= 1
$(eval $(call BothConfigC,DEBUG,))
$(eval $(call UserConfigMake,DEBUG,1))
$(eval $(call KernelConfigC,DEBUG_LINUX_MEMORY_ALLOCATIONS,))
$(eval $(call KernelConfigC,DEBUG_LINUX_MEM_AREAS,))
$(eval $(call KernelConfigC,DEBUG_LINUX_MMAP_AREAS,))
$(eval $(call KernelConfigC,DEBUG_HANDLEALLOC_KM,))
$(eval $(call UserConfigC,DLL_METRIC,1))
$(eval $(call TunableBothConfigC,RGXFW_ALIGNCHECKS,1,\
Enable extra runtime alignment checks at Firmware boot time._\
))
$(eval $(call TunableBothConfigC,PVRSRV_DEBUG_CCB_MAX,))
else ifeq ($(BUILD),release)
$(eval $(call BothConfigC,RELEASE,))
$(eval $(call TunableBothConfigMake,DEBUGLINK,1))
$(eval $(call TunableBothConfigC,RGXFW_ALIGNCHECKS,,\
Enable extra runtime alignment checks at Firmware boot time._\
))
else ifeq ($(BUILD),timing)
$(eval $(call BothConfigC,TIMING,))
$(eval $(call UserConfigMake,TIMING,1))
$(eval $(call UserConfigC,DLL_METRIC,1))
$(eval $(call TunableBothConfigMake,DEBUGLINK,1))
else
$(error BUILD= must be either debug, release or timing)
endif

ifeq ($(SUPPORT_PAGE_FAULT_DEBUG),1)
override SUPPORT_DEVICEMEMHISTORY_BRIDGE := 1
endif


$(eval $(call TunableBothConfigMake,COMPRESS_DEBUG_SECTIONS,,\
Enable compression on debug sections (.zdebug)_\
May have tool compatibility issues.))



# User-configurable options
#
$(eval $(call TunableBothConfigC,RGX_BVNC_CORE_KM_HEADER,))
$(eval $(call TunableBothConfigC,RGX_BVNC_CORE_HEADER,))
$(eval $(call TunableBothConfigC,RGX_BNC_CONFIG_KM_HEADER,))
$(eval $(call TunableBothConfigC,RGX_BNC_CONFIG_HEADER,))

$(eval $(call TunableBothConfigC,PVRSRV_DEBUG_HANDLE_LOCK,,\
Enable checking that the handle lock is held when a handle reference_\
count is modified))

$(eval $(call TunableBothConfigC,PVRSRV_FORCE_UNLOAD_IF_BAD_STATE,,\
Make sure the driver unloads even when the FW is stuck. To unload_\
kill all applications still connected to the driver and then set_\
the driver in a bad state via: echo "k" > /sys/kernel/debug/pvr/status.\
))

$(eval $(call TunableBothConfigC,SUPPORT_DBGDRV_EVENT_OBJECTS,1))
$(eval $(call TunableBothConfigC,PVR_DBG_BREAK_ASSERT_FAIL,,\
Enable this to treat PVR_DBG_BREAK as PVR_ASSERT(0)._\
Otherwise it is ignored._\
))
$(eval $(call TunableBothConfigC,PDUMP,,\
Enable parameter dumping in the driver._\
This adds code to record the parameters being sent to the hardware for_\
later analysis._\
))
PDUMP_STREAMBUF_SIZE_MB ?= 16
$(eval $(call TunableBothConfigC,PDUMP_STREAMBUF_MAX_SIZE_MB,$(PDUMP_STREAMBUF_SIZE_MB),))
$(eval $(call TunableBothConfigC,NO_HARDWARE,,\
Disable hardware interactions (e.g. register writes) that the driver would_\
normally perform. A driver built with this option can$(apos)t drive hardware$(comma)_\
but with PDUMP enabled$(comma) it can capture parameters to be played back later._\
))
$(eval $(call TunableBothConfigC,PDUMP_DEBUG_OUTFILES,,\
Add debug information to the pdump script (out2.txt) as it is generated._\
This includes line numbers$(comma) process names and also enables checksumming_\
of the binary data dumped to out2.prm which can be verified offline._\
))
$(eval $(call TunableBothConfigC,PVRSRV_NEED_PVR_DPF,,\
Enable this to turn on PVR_DPF in release builds._\
))
$(eval $(call TunableBothConfigC,PVRSRV_NEED_PVR_ASSERT,,\
Enable this to turn on PVR_ASSERT in release builds._\
))
$(eval $(call TunableBothConfigC,PVRSRV_NEED_PVR_TRACE,,\
Enable this to turn on PVR_TRACE in release builds._\
))
$(eval $(call TunableBothConfigC,PVRSRV_NEED_PVR_STACKTRACE,,\
Enable this to turn on stack trace functions in release builds._\
))
$(eval $(call TunableBothConfigC,REFCOUNT_DEBUG,))
$(eval $(call TunableBothConfigC,DC_DEBUG,,\
Enable debug tracing in the DC (display class) server code))
$(eval $(call TunableBothConfigC,SCP_DEBUG,,\
Enable debug tracing in the SCP (software command processor)_\
which is used by the DC.))
$(eval $(call TunableBothConfigC,SUPPORT_INSECURE_EXPORT,))
$(eval $(call TunableBothConfigC,SUPPORT_SECURE_EXPORT,,\
Enable support for secure device memory and sync export._\
This replaces export handles with file descriptors$(comma) which can be passed_\
between processes to share memory._\
))
$(eval $(call TunableKernelConfigMake,SUPPORT_GPUTRACE_EVENTS,))
$(eval $(call TunableBothConfigC,SUPPORT_GPUTRACE_EVENTS,,\
Linux only. This builds support into the kernel driver for_\
generating FTrace events for GPU work submission and scheduling. The_\
DebugFS pvr/gpu_tracing_on option must be set at run-time to enable.\
))
$(eval $(call TunableBothConfigC,SUPPORT_DISPLAY_CLASS,,\
Enable DC (display class) support. Disable if not using a DC display driver.))
$(eval $(call TunableBothConfigC,PVRSRV_DEBUG_CCB_MAX,))
$(eval $(call TunableBothConfigC,SUPPORT_TRUSTED_DEVICE,,\
Enable a build mode targeting an REE._\
))

$(eval $(call TunableBothConfigC,METRICS_USE_ARM_COUNTERS,,\
Enable usage of hardware performance counters for metrics on ARM platforms._\
))

# Gigacluster flags
ifeq ($(SUPPORT_GIGACLUSTER),1)
endif

#
# GPU virtualization validation
#
$(eval $(call TunableBothConfigC,SUPPORT_GPUVIRT_VALIDATION,,\
Enable validation mode for GPU Virtualisation in which processes inside_\
an OS are given independent OSIDs._\
))
$(eval $(call TunableBothConfigC,GPUVIRT_VALIDATION_NUM_OS,8))

#
# GPU virtualization support
#
$(eval $(call TunableBothConfigC,SUPPORT_PVRSRV_GPUVIRT,,\
Enable GPU virtualization support._\
))
$(eval $(call TunableBothConfigMake,SUPPORT_PVRSRV_GPUVIRT,))
$(eval $(call TunableBothConfigC,PVRSRV_GPUVIRT_GUESTDRV,,\
Enable guest driver build._\
))
$(eval $(call TunableBothConfigMake,PVRSRV_GPUVIRT_GUESTDRV,))
$(eval $(call TunableBothConfigC,PVRSRV_GPUVIRT_NUM_OSID,2,\
Number of firmware supported OSIDs._\
))
$(eval $(call TunableBothConfigMake,PVRSRV_GPUVIRT_NUM_OSID,))
$(eval $(call TunableBothConfigC,PVRSRV_GPUVIRT_MULTIDRV_MODEL,,\
Enable multiple-driver-model support._\
))
$(eval $(call TunableBothConfigMake,PVRSRV_GPUVIRT_MULTIDRV_MODEL,,\
Enable multiple driver model for GPU virtualization in which multiple drivers_\
loaded in the same kernel instance are given independent OSIDs._\
))
ifneq ($(SUPPORT_PVRSRV_GPUVIRT),)
ifeq ($(PVRSRV_GPUVIRT_GUESTDRV),)
$(eval $(call TunableBothConfigC,PVRSRV_GPUVIRT_OSID,"0"))
else
$(eval $(call TunableBothConfigC,PVRSRV_GPUVIRT_OSID,"$(PVRSRV_GPUVIRT_GUESTDRV)"))
endif
endif

# Set to 1 to enable A/B buffer state for VDM context storing
$(eval $(call TunableBothConfigC,SUPPORT_VDM_CONTEXT_STORE_BUFFER_AB,1,\
Internal use only._\
))

$(eval $(call TunableBothConfigC,SUPPORT_VALIDATION,))
$(eval $(call TunableBothConfigC,FIX_DUSTS_POW_ON_INIT,,\
Enable WA for power controllers that power up dusts by default._\
The Firmware powers down the dusts after booting._\
))
$(eval $(call TunableKernelConfigMake,PVR_DVFS,))
$(eval $(call TunableKernelConfigC,PVR_DVFS,,\
Enables PVR DVFS implementation to actively change frequency / voltage depending_\
on current GPU load. Currently only supported on Linux._\
))
$(eval $(call TunableBothConfigC,PVR_POWER_ACTOR,,\
Enables PVR power actor implementation for registration with a kernel configured_\
with IPA. Enables power counter measurement timer in the FW which is periodically_\
read by the host DVFS in order to operate within a governor set power envelope._\
))
$(eval $(call TunableBothConfigC,PVR_POWER_ACTOR_SCALING,,\
Scaling factor for the dynamic power coefficients._\
))
$(eval $(call TunableKernelConfigC,DEBUG_HANDLEALLOC_INFO_KM,))
$(eval $(call TunableKernelConfigC,SUPPORT_LINUX_X86_WRITECOMBINE,1))
$(eval $(call TunableKernelConfigC,SUPPORT_LINUX_X86_PAT,1))
$(eval $(call TunableKernelConfigC,PVRSRV_RESET_ON_HWTIMEOUT,))
$(eval $(call TunableKernelConfigC,PVR_LINUX_USING_WORKQUEUES,))
$(eval $(call TunableKernelConfigC,PVR_LINUX_MISR_USING_WORKQUEUE,))
$(eval $(call TunableKernelConfigC,PVR_LINUX_MISR_USING_PRIVATE_WORKQUEUE,))
$(eval $(call TunableKernelConfigC,PVR_LINUX_TIMERS_USING_WORKQUEUES,))
$(eval $(call TunableKernelConfigC,PVR_LINUX_TIMERS_USING_SHARED_WORKQUEUE,))
$(eval $(call TunableKernelConfigC,PVR_LDM_PLATFORM_PRE_REGISTERED,))
$(eval $(call TunableKernelConfigC,PVR_LDM_DRIVER_REGISTRATION_NAME,"\"$(PVRSRV_MODNAME)\""))
$(eval $(call TunableBothConfigC,PVRSRV_ENABLE_FULL_SYNC_TRACKING,,\
Track and annotate all syncs used in the driver and output this information_\
in the Debug Dump data._\
))
$(eval $(call TunableBothConfigC,PVRSRV_FULL_SYNC_TRACKING_HISTORY_LEN,256))
$(eval $(call TunableKernelConfigC,PVRSRV_ENABLE_FULL_CCB_DUMP,,\
Output the full contents of CCBs in Debug Dump data$(comma) not just items at the head.))
$(eval $(call TunableKernelConfigC,SYNC_DEBUG,))
$(eval $(call TunableKernelConfigC,SUPPORT_DUMP_CLIENT_CCB_COMMANDS,))
$(eval $(call TunableKernelConfigC,PVR_LINUX_DONT_USE_RANGE_BASED_INVALIDATE,))
$(eval $(call TunableKernelConfigC,SUPPORT_MMU_PAGESIZECONFIG_REFCOUNT,))
$(eval $(call TunableKernelConfigC,SUPPORT_DC_COMPLETE_TIMEOUT_DEBUG,))

$(eval $(call TunableKernelConfigC,PVR_DUMMY_PAGE_INIT_VALUE,0x00,\
Define that need to be used to initialise the dummy page._\
When this macro is not defined$(comma) no initialisation of the dummy_\
memory page used for sparse memory allocations is performed._\
))

$(eval $(call TunableBothConfigC,SUPPORT_PVR_VALGRIND,))



$(eval $(call TunableBothConfigC,PVRSRV_DEVMEM_TEST_SAFE_MEMSETCPY,,\
Enable this to force the use of PVRSRVMemSet/Copy in the client driver _\
instead of the built-in libc functions. These implemenations are device _\
memory safe and are used by default on AARCH64 platform._\
))

$(eval $(call TunableBothConfigC,PVRSRV_BRIDGE_LOGGING,,\
If enabled$(comma) provides a debugfs entry which logs the number of calls_\
made to each bridge function._\
))

# If we are building against a ChromeOS kernel, set this.
$(eval $(call TunableBothConfigC,CHROMIUMOS_WORKAROUNDS_KERNEL318,))




ifneq ($(SUPPORT_ANDROID_PLATFORM),1)
   ifeq ($(ENABLE_LINUX_BLOB_CACHE),1)
   endif
endif



$(eval $(call TunableBothConfigC,CACHEFLUSH_UM_TYPE,,\
Specify services UM cache maintenance type_\
(i.e. CACHEFLUSH_UM_[X86,ARM64,GENERIC])._\
))

$(eval $(call TunableBothConfigC,CACHEFLUSH_KM_TYPE,,\
Specify services KM cache maintenance type_\
(i.e. CACHEFLUSH_KM_[GLOBAL,RANGEBASED,RANGEBASED_DEFERRED])._\
))






ifneq ($(SUPPORT_ANDROID_PLATFORM),1)
endif

ifeq ($(SUPPORT_VK_TRACING_EXT),1)
endif

$(eval $(call TunableBothConfigMake,PDUMP,))
$(eval $(call TunableBothConfigMake,SUPPORT_INSECURE_EXPORT,))
$(eval $(call TunableBothConfigMake,SUPPORT_SECURE_EXPORT,))
$(eval $(call TunableBothConfigMake,SUPPORT_DISPLAY_CLASS,))
$(eval $(call TunableBothConfigMake,SUPPORT_RAY_TRACING,,\
Enable support for ray tracing feature in the DDK._\
))
$(eval $(call TunableBothConfigMake,SUPPORT_COMPUTE,,\
Enable support for compute data master in the DDK. Only applicable on cores_\
supporting CDM feature._\
))
$(eval $(call TunableBothConfigC,SUPPORT_OPENCL_2_X,,\
Enable support for OpenCL 2.x features on any core._\
))
$(eval $(call TunableBothConfigMake,SUPPORT_TLA,,\
Enable support for TLA in the DDK. Only applicable on cores supporting TLA_\
feature._\
))
$(eval $(call TunableBothConfigMake,SUPPORT_MIPS_FIRMWARE,,\
Internal use only._\
))
$(eval $(call TunableBothConfigMake,SUPPORT_ES32,))
$(eval $(call TunableBothConfigMake,SUPPORT_SIGNAL_FILTER,))
$(eval $(call TunableBothConfigC,SUPPORT_SIGNAL_FILTER,))
$(eval $(call TunableBothConfigC,FORCE_DM_OVERLAP,))
$(eval $(call TunableBothConfigC,SUPPORT_EXTRA_METASP_DEBUG,,\
Enable extra debug information using the META Slave Port._\
Checks the validity of the Firmware code and dumps sync values_\
using the GPU memory subsystem via the META Slave Port._\
))
$(eval $(call TunableBothConfigC,PVRSRV_UNMAP_ON_SPARSE_CHANGE,1,\
Temporary define to unmap the CPU map of sparse memory when changed and remap_\
it._\
))

$(eval $(call TunableBothConfigMake,OPTIM,,\
Specify the optimisation flags passed to the compiler. Normally this_\
is autoconfigured based on the build type._\
))
$(eval $(call TunableBothConfigC,SUPPORT_PERCONTEXT_FREELIST,1,Internal use only))
$(eval $(call TunableBothConfigC,SUPPORT_MMU_FREELIST,,Internal use only))
$(eval $(call TunableBothConfigC,SUPPORT_VFP,,Internal use only))

$(eval $(call UserConfigC,EGL_BASENAME_SUFFIX,\"$(EGL_BASENAME_SUFFIX)\"))


OCL_USE_KERNEL_BLOB_CACHE ?= 1

OCL_ONLINE_COMPILATION ?= 1


# HWR is enabled by default
HWR_DEFAULT_ENABLED ?= 1
$(eval $(call TunableBothConfigC,HWR_DEFAULT_ENABLED,))

# Build-only AppHint configuration values
$(eval $(call AppHintConfigC,PVRSRV_APPHINT_HWRDEBUGDUMPLIMIT,APPHNT_BLDVAR_DBGDUMPLIMIT,\
Limit for the number of HWR debug dumps produced))
$(eval $(call AppHintConfigC,PVRSRV_APPHINT_ENABLETRUSTEDDEVICEACECONFIG,IMG_FALSE,\
Enable trusted device ACE config))
$(eval $(call AppHintConfigC,PVRSRV_APPHINT_HTBUFFERSIZE,0x1000,\
Buffer size in bytes for Host Trace log data))

# PDUMP AppHint defaults
$(eval $(call AppHintConfigC,PVRSRV_APPHINT_ENABLESIGNATURECHECKS,APPHNT_BLDVAR_ENABLESIGNATURECHECKS,\
Buffer size in bytes for storing signature check data))
$(eval $(call AppHintConfigC,PVRSRV_APPHINT_SIGNATURECHECKSBUFSIZE,RGXFW_SIG_BUFFER_SIZE_MIN,\
Buffer size in bytes for storing signature check data))

# Validation AppHint defaults
$(eval $(call AppHintConfigC,PVRSRV_APPHINT_DISABLECLOCKGATING,0,\
Disable GPU clock gating))
$(eval $(call AppHintConfigC,PVRSRV_APPHINT_DISABLEDMOVERLAP,0,\
Disable GPU data master overlapping))

$(eval $(call AppHintConfigC,PVRSRV_APPHINT_ENABLECDMKILLINGRANDMODE,0,\
Enable random killing of the compute data master))
$(eval $(call AppHintConfigC,PVRSRV_APPHINT_ENABLEFWCONTEXTSWITCH,RGXFWIF_INICFG_CTXSWITCH_DM_ALL,\
Enable firmware context switching))
$(eval $(call AppHintConfigC,PVRSRV_APPHINT_ENABLERDPOWERISLAND,RGX_RD_POWER_ISLAND_DEFAULT,\
Enable RD power island))

$(eval $(call AppHintConfigC,PVRSRV_APPHINT_FIRMWAREPERF,FW_PERF_CONF_NONE,\
Force the initial Firmware Performance Configuration to the specified value))
$(eval $(call AppHintConfigC,PVRSRV_APPHINT_FWCONTEXTSWITCHPROFILE,RGXFWIF_CTXSWITCH_PROFILE_MEDIUM_EN,\
Firmware context switch profile))
$(eval $(call AppHintConfigC,PVRSRV_APPHINT_HWPERFDISABLECUSTOMCOUNTERFILTER,0,\
Force the initial HW Performance Custom Counter Filter value))
$(eval $(call AppHintConfigC,PVRSRV_APPHINT_HWPERFFWBUFSIZEINKB,RGXFW_HWPERF_L1_SIZE_DEFAULT,\
Buffer size in KB of the hardware performance GPU buffer))
$(eval $(call AppHintConfigC,PVRSRV_APPHINT_HWPERFHOSTBUFSIZEINKB,HWPERF_HOST_TL_STREAM_SIZE_DEFAULT,\
Buffer size in KB of the hardware performance host buffer))

$(eval $(call AppHintConfigC,PVRSRV_APPHINT_JONESDISABLEMASK,0,\
Disable Jones))
$(eval $(call AppHintConfigC,PVRSRV_APPHINT_NEWFILTERINGMODE,APPHNT_BLDVAR_NEWFLTMODE,\
Enable new TPU filtering mode))
$(eval $(call AppHintConfigC,PVRSRV_APPHINT_TRUNCATEMODE,0,\
Truncate mode))
$(eval $(call AppHintConfigC,PVRSRV_APPHINT_USEMETAT1,RGX_META_T1_OFF,\
Enable to use the second Meta thread))

# Build-only AppHint configuration values
$(eval $(call AppHintConfigC,PVRSRV_APPHINT_ENABLETRUSTEDDEVICEACECONFIG,IMG_FALSE,\
Enable trusted device ACE config))
$(eval $(call AppHintConfigC,PVRSRV_APPHINT_CLEANUPTHREADPRIORITY,0,\
Set the priority of the cleanup thread))
$(eval $(call AppHintConfigC,PVRSRV_APPHINT_CLEANUPTHREADWEIGHT,0,\
Set the weight of the cleanup thread))
$(eval $(call AppHintConfigC,PVRSRV_APPHINT_WATCHDOGTHREADPRIORITY,0,\
Set the priority of the watchdog thread))
$(eval $(call AppHintConfigC,PVRSRV_APPHINT_WATCHDOGTHREADWEIGHT,0,\
Set the weight of the watchdog thread))


# Debugfs AppHint configuration values
$(eval $(call AppHintConfigC,PVRSRV_APPHINT_ASSERTONHWRTRIGGER,IMG_FALSE,\
Enable firmware assert when an HWR event is triggered))
$(eval $(call AppHintConfigC,PVRSRV_APPHINT_ASSERTOUTOFMEMORY,IMG_FALSE,\
Enable firmware assert when the TA raises out-of-memory))
$(eval $(call AppHintConfigC,PVRSRV_APPHINT_CHECKMLIST,APPHNT_BLDVAR_DEBUG,\
Enable firmware MLIST consistency checker))
$(eval $(call AppHintConfigC,PVRSRV_APPHINT_DISABLEFEDLOGGING,IMG_FALSE,\
Disable fatal error detection debug dumps))

$(eval $(call AppHintConfigC,PVRSRV_APPHINT_ENABLEAPM,RGX_ACTIVEPM_DEFAULT,\
Force the initial driver APM configuration to the specified value))
$(eval $(call AppHintConfigC,PVRSRV_APPHINT_ENABLEHTBLOGGROUP,0,\
Enable host trace log groups))
$(eval $(call AppHintConfigC,PVRSRV_APPHINT_ENABLELOGGROUP,0,\
Enable firmware trace log groups))
$(eval $(call AppHintConfigC,PVRSRV_APPHINT_FIRMWARELOGTYPE,0,\
Specify output mechanism for firmware log data))

$(eval $(call AppHintConfigC,PVRSRV_APPHINT_HTBOPERATIONMODE,HTB_OPMODE_DROPLATEST,\
Configure host trace buffer behaviour))
$(eval $(call AppHintConfigC,PVRSRV_APPHINT_HWPERFFWFILTER,0,\
Mask used to select GPU events to log for performance))
$(eval $(call AppHintConfigC,PVRSRV_APPHINT_HWPERFHOSTFILTER,0,\
Mask used to select host events to log for performance))

$(eval $(call AppHintConfigC,PVRSRV_APPHINT_ZEROFREELIST,IMG_FALSE,\
Zero freelist memory during freelist reconstruction as part of HWR))

$(eval $(call AppHintConfigC,PVRSRV_APPHINT_DUSTREQUESTINJECT,IMG_FALSE,\
Inject dust requests))
$(eval $(call AppHintConfigC,PVRSRV_APPHINT_DISABLEPDUMPPANIC,IMG_FALSE,\
Disable PDUMP panic))

# GLSL compiler options
ifeq ($(BUILD),debug)
DUMP_LOGFILES ?= 1
endif
$(eval $(call TunableBothConfigC,ENABLE_DUMP_DISASSEMBLY,))
# end of GLSL compiler options


$(eval $(call TunableBothConfigMake,PVR_TESTING_UTILS,))
$(eval $(call TunableBothConfigC,PVR_TESTING_UTILS,,\
Enable this to build in extra support for testing the PVR Services Server._\
))

$(eval $(call TunableBothConfigC,SUPPORT_AXI_ACE_TEST,,\
Enable this to add extra FW code for the AXI ACE unittest._\
))


TQ_CAPTURE_PARAMS ?= 1

$(eval $(call TunableBothConfigC,PVR_DPF_ADHOC_DEBUG_ON,))
$(eval $(call TunableBothConfigC,RGXFW_DEBUG_LOG_GROUP,,\
Enable the usage of DEBUG log group in the Firmware logs._\
))

ifeq ($(ENABLE_PDVFS_GENERIC),1)
SUPPORT_PDVFS ?= 1
SUPPORT_WORKLOAD_ESTIMATION ?= 1
endif

ifeq ($(ENABLE_PDVFS_GPIO_THREAD1),1)
SUPPORT_WORKLOAD_ESTIMATION ?= 1
SUPPORT_PDVFS ?= 1
SUPPORT_PDVFS_GPIO ?= 1
SUPPORT_RGX_GPIO_GENERAL ?= 1
SUPPORT_RGX_GPIO_AP ?= 1
endif

# Proactive DVFS (PDVFS)
ifeq ($(SUPPORT_PDVFS),1)
SUPPORT_WORKLOAD_ESTIMATION ?= 1
endif
$(eval $(call TunableBothConfigMake,SUPPORT_PDVFS,,\
Enabling this feature enables proactive dvfs in the meta firmware._\
))
$(eval $(call TunableBothConfigC,SUPPORT_PDVFS,,\
Enabling this feature enables proactive dvfs in the meta firmware._\
))
$(eval $(call TunableBothConfigC,SUPPORT_PDVFS_GPIO,,\
Enabling this feature sends OPP values over the gpio to the PMIC for DVFS._\
))

# Workload Estimation
$(eval $(call TunableBothConfigMake,SUPPORT_WORKLOAD_ESTIMATION,,\
Enabling this feature enables workload intensity estimation from a workloads_\
characteristics and assigning a deadline to it._\
))
$(eval $(call TunableBothConfigC,SUPPORT_WORKLOAD_ESTIMATION,,\
Enabling this feature enables workload intensity estimation from a workloads_\
characteristics and assigning a deadline to it._\
))

# The PVR_GPIO_MODE variable specifies how the RGX GPIO ports
# are going to be used by the firmware. 
# By default, the firmware implements the normal RGX GPIO protocol.   
ifeq ($(PVR_GPIO_MODE),GPIO_MODE_POWMON_GPIO_PIN)
$(eval $(call UserConfigC,SUPPORT_POWMON_GPIO_PIN,1,\
Enable Power Monitoring using GPIO to communicate with Power Controller._\
))
POW_MON_OVER_GPIO := 1
else ifeq ($(PVR_GPIO_MODE),GPIO_MODE_POWMON_WO_GPIO_PIN)
$(eval $(call UserConfigC,SUPPORT_POWMON_WO_GPIO_PIN,1,\
Enable Power Monitoring using special register handshake to communicate with Power Controller._\
))
POW_MON_OVER_GPIO := 1
else ifeq ($(PVR_GPIO_MODE),)
$(eval $(call UserConfigC,SUPPORT_RGX_GPIO_GENERAL,1,\
Enables support for basic send and receive GPIO functionality in the firmware.\
))
$(eval $(call UserConfigMake,SUPPORT_RGX_GPIO_GENERAL,1,\
Enables support for basic send and receive GPIO functionality in the firmware.\
))
else
$(error PVR_GPIO_MODE not valid)
endif

$(eval $(call TunableBothConfigC,SUPPORT_GPIO_VALIDATION,,\
Internal use only._\
))

# GPIO Address Protocol
$(eval $(call TunableBothConfigMake,SUPPORT_RGX_GPIO_AP,,\
This enables the address protocol for the GPIO._\
))
$(eval $(call TunableBothConfigC,SUPPORT_RGX_GPIO_AP,,\
This enables the address protocol for the GPIO._\
))

# Make sure PVR_DVFS=1 is not used in conjunction with 
# PVR_GPIO_MODE=GPIO_MODE_POWMON_GPIO_PIN or 
# PVR_GPIO_MODE=GPIO_MODE_POWMON_WO_GPIO_PIN
# as they are not compatible.
ifeq ($(PVR_DVFS),1)
ifeq ($(POW_MON_OVER_GPIO),1)
$(error PVR_DVFS=1 cannot be used together with PVR_GPIO_MODE=GPIO_MODE_POWMON_[WO_]GPIO_PIN)
endif
endif

# Make sure SUPPORT_PDVFS_GPIO=1 is not used in conjunction with 
# PVR_GPIO_MODE=GPIO_MODE_POWMON_GPIO_PIN or 
# PVR_GPIO_MODE=GPIO_MODE_POWMON_WO_GPIO_PIN
# as they are not compatible.
ifeq ($(SUPPORT_PDVFS_GPIO),1)
ifeq ($(POW_MON_OVER_GPIO),1)
$(error SUPPORT_PDVFS_GPIO=1 cannot be used together with PVR_GPIO_MODE=GPIO_MODE_POWMON_[WO_]GPIO_PIN)
endif
endif

$(eval $(call TunableKernelConfigMake,PVR_HANDLE_BACKEND,idr,\
Specifies the back-end that should be used$(comma) by the Services kernel handle_\
interface$(comma) to allocate handles. The available backends are:_\
* generic (OS agnostic)_\
* idr (Uses the Linux IDR interface)_\
))


$(eval $(call TunableBothConfigC,PVRSRV_ENABLE_PROCESS_STATS,1,\
Enable Process Statistics via DebugFS._\
))

$(eval $(call TunableBothConfigC,PVRSRV_DEBUG_LINUX_MEMORY_STATS,,\
Present Process Statistics memory stats in a more detailed manner to_\
assist with debugging and finding memory leaks (under Linux only)._\
))

$(eval $(call TunableBothConfigC,SUPPORT_SHARED_SLC,,\
When the SLC is shared the SLC reset is performed by the System layer when_\
calling RGXInitSLC and not the GPU driver. Define this for system layer_\
SLC handling. \
))

# EXCLUDE_BREAKPOINT_BRIDGE is set to exclude the breakpoint.brg bridge in
# the Kernel This is disabled by default for release builds.
#
$(eval $(call TunableBothConfigMake,EXCLUDE_BREAKPOINT_BRIDGE,))
$(eval $(call TunableBothConfigC,EXCLUDE_BREAKPOINT_BRIDGE,,Disables the breakpoint bridge))

# EXCLUDE_CMM_BRIDGE is set to exclude the cmm.brg bridge in
# the Kernel This is disabled by default for release builds.
#
$(eval $(call TunableBothConfigMake,EXCLUDE_CMM_BRIDGE,))
$(eval $(call TunableBothConfigC,EXCLUDE_CMM_BRIDGE,,Disables the cmm bridge))

# SUPPORT_DEVICEMEMHISTORY_BRIDGE is set to include the devicememhistory.brg bridge
# in the Kernel This is disabled by default for release builds.
#
$(eval $(call TunableBothConfigMake,SUPPORT_DEVICEMEMHISTORY_BRIDGE,))
$(eval $(call TunableBothConfigC,SUPPORT_DEVICEMEMHISTORY_BRIDGE,,Enables the devicememhistory bridge))

# EXCLUDE_HTBUFFER_BRIDGE is set to exclude the htbuffer.brg bridge in
# the Kernel This is disabled by default for release builds.
#
$(eval $(call TunableBothConfigMake,EXCLUDE_HTBUFFER_BRIDGE,))
$(eval $(call TunableBothConfigC,EXCLUDE_HTBUFFER_BRIDGE,,Disables the htbuffer bridge))

# EXCLUDE_REGCONFIG_BRIDGE is set to exclude the regconfig.brg bridge in
# the Kernel This is disabled by default for release builds.
#
$(eval $(call TunableBothConfigMake,EXCLUDE_REGCONFIG_BRIDGE,))
$(eval $(call TunableBothConfigC,EXCLUDE_REGCONFIG_BRIDGE,,Disables the regconfig bridge))

# SUPPORT_VALIDATION_BRIDGE is set to include the validation.brg bridge in
# the Kernel This is disabled by default for release builds.
#
$(eval $(call TunableBothConfigC,SUPPORT_VALIDATION_BRIDGE,,Enables the validation bridge))

# PVR_RI_DEBUG is set to enable RI annotation of devmem allocations
# This is enabled by default for debug builds.
#
$(eval $(call TunableBothConfigMake,PVR_RI_DEBUG,))
$(eval $(call TunableBothConfigC,PVR_RI_DEBUG,,\
Enable Resource Information (RI) debug. This logs details of_\
resource allocations with annotation to help indicate their use._\
))

$(eval $(call TunableBothConfigMake,SUPPORT_PAGE_FAULT_DEBUG,))
$(eval $(call TunableBothConfigC,SUPPORT_PAGE_FAULT_DEBUG,,\
Collect information about allocations such as descriptive strings_\
and timing data for more detailed page fault analysis._\
))

$(eval $(call TunableKernelConfigC,DEBUG_BRIDGE_KM,,\
Enable Services bridge debugging and bridge statistics output_\
))

$(eval $(call TunableKernelConfigC,PVRSRV_ENABLE_CCCB_UTILISATION_DEBUG,,\
Calculate high watermarks of all the client CCBs and print a warning if the_\
watermarks touched a certain threshold value (90%) of the cCCB allocation size._\
))

$(eval $(call TunableKernelConfigC,PVR_DISABLE_KMALLOC_MEMSTATS,,\
Set to avoid gathering statistical information about kmalloc and vmalloc_\
allocations._\
))

$(eval $(call TunableBothConfigC,PVRSRV_ENABLE_MEMORY_STATS,,\
Enable Memory allocations to be recorded and published via Process Statistics._\
))

$(eval $(call TunableBothConfigC,PVRSRV_STRICT_COMPAT_CHECK,,\
Enable strict mode of checking all the build options between um & km._\
The driver may fail to load if there is any mismatch in the options._\
))

$(eval $(call TunableBothConfigC,PVR_LINUX_PHYSMEM_MAX_POOL_PAGES,10240,\
Defines how many pages the page cache can hold.))

$(eval $(call TunableBothConfigC,PVR_LINUX_PHYSMEM_MAX_EXCESS_POOL_PAGES,32768,\
Defines how many pages the page cache is allowed to store temporarily above the_\
PVR_LINUX_PHYSMEM_MAX_POOL_PAGES limit._\
These pages are only held until the deferred cleanup thread is emptying the excess._\
The signal to wake up the cleanup thread is sent immediately after the pages_\
have been moved to the pool._\
))

$(eval $(call TunableBothConfigC,PVR_MMAP_USE_VM_INSERT,,\
If enabled Linux will always use vm_insert_page for CPU mappings._\
vm_insert_page was found to be slower than remap_pfn_range on ARM kernels_\
but guarantees full memory accounting for the process that mapped the memory._\
The slowdown in vm_insert_page is caused by a dcache flush_\
that is only implemented for ARM and a few other architectures._\
This tunable can be enabled to debug memory issues. On x86 platforms_\
we always use vm_insert_page independent of this tunable._\
))

$(eval $(call TunableBothConfigC,PVR_DIRTY_BYTES_FLUSH_THRESHOLD,1048576,\
When allocating uncached or write-combine memory we need to invalidate the_\
CPU cache before we can use the acquired pages; also when using cached memory_\
we need to clean/flush the CPU cache before we transfer ownership of the_\
memory to the device. This threshold defines at which number of pages expressed_\
in bytes we want to do a full cache flush instead of invalidating pages one by one._\
Default value is 1048576 bytes or 256 pages; ideal value depends on SoC cache size._\
))

$(eval $(call TunableBothConfigC,PVR_LINUX_HIGHORDER_ALLOCATION_THRESHOLD,256,\
Allocate OS pages in 2^(order) chunks if more than this threshold were requested_\
))


# Allocate OS pages in 2^n chunks to help reduce duration of large allocations
# Below defaults request 4 pages per request, only for allocs >= 256 pages
PVR_LINUX_PHYSMEM_MAX_ALLOC_ORDER ?= 2
$(eval $(call TunableBothConfigC,PVR_LINUX_HIGHORDER_ALLOCATION_THRESHOLD, 256 ))
$(eval $(call TunableBothConfigC,PVR_LINUX_PHYSMEM_MAX_ALLOC_ORDER_NUM, $(PVR_LINUX_PHYSMEM_MAX_ALLOC_ORDER) ))

# Choose the threshold at which allocation size we want to use vmalloc instead of
# kmalloc. On highly fragmented systems large kmallocs can fail because it requests
# physically contiguous pages. All allocations bigger than this define use vmalloc.
$(eval $(call TunableBothConfigC,PVR_LINUX_KMALLOC_ALLOCATION_THRESHOLD, 16384 ))

# Tunable RGX_MAX_TA_SYNCS / RGX_MAX_3D_SYNCS to increase the size of sync array in the DDK
# If defined, these macros take up the values as defined in the environment,
# Else, the default value is taken up as defined in include/rgxapi.h
#

$(eval $(call TunableBothConfigMake,SUPPORT_KERNEL_SRVINIT,))
$(eval $(call TunableBothConfigC,SUPPORT_KERNEL_SRVINIT,))

$(eval $(call TunableBothConfigC,SUPPORT_SERVER_SYNC,1))
$(eval $(call TunableBothConfigMake,SUPPORT_SERVER_SYNC,1))

$(eval $(call TunableBothConfigMake,SUPPORT_NATIVE_FENCE_SYNC,))
$(eval $(call TunableBothConfigC,SUPPORT_NATIVE_FENCE_SYNC,))

ifeq ($(PVRSRV_GPUVIRT_MULTIDRV_MODEL),)
PVR_DRM_NAME := pvr
else
PVR_DRM_NAME := pvr$(PVRSRV_GPUVIRT_GUESTDRV)
endif
$(eval $(call BothConfigC,PVR_DRM_NAME,"\"$(PVR_DRM_NAME)\""))



$(eval $(call TunableKernelConfigC,PVRSRV_FORCE_SLOWER_VMAP_ON_64BIT_BUILDS,,\
If enabled$(comma) all kernel mappings will use vmap/vunmap._\
vmap/vunmap is slower than vm_map_ram/vm_unmap_ram and can_\
even have bad peaks taking up to 100x longer than vm_map_ram._\
The disadvantage of vm_map_ram is that it can lead to vmalloc space_\
fragmentation that can lead to vmalloc space exhaustion on 32 bit Linux systems._\
This flag only affects 64 bit Linux builds$(comma) on 32 bit we always default_\
to use vmap because of the described fragmentation problem._\
))

$(eval $(call TunableBothConfigC,DEVICE_MEMSETCPY_ALIGN_IN_BYTES,8,\
Sets pointer alignment (in bytes) needed by PVRSRVDeviceMemSet/Copy_\
for arm64 arch._\
This value should reflect memory bus width e.g. if the bus is 64 bits_\
wide this value should be set to 8 bytes._\
))



$(eval $(call TunableKernelConfigC,PVRSRV_DEBUG_LISR_EXECUTION,,\
Collect information about the last execution of the LISR in order to_\
debug interrupt handling timeouts._\
))






endif # INTERNAL_CLOBBER_ONLY

export INTERNAL_CLOBBER_ONLY
export TOP
export OUT

MAKE_ETC := -Rr --no-print-directory -C $(TOP) TOP=$(TOP) OUT=$(OUT) \
	        -f build/linux/toplevel.mk

# This must match the default value of MAKECMDGOALS below, and the default
# goal in toplevel.mk
.DEFAULT_GOAL := build

ifeq ($(MAKECMDGOALS),)
MAKECMDGOALS := build
else
# We can't pass autogen to toplevel.mk
MAKECMDGOALS := $(filter-out autogen,$(MAKECMDGOALS))
endif

.PHONY: autogen
autogen:
ifeq ($(INTERNAL_CLOBBER_ONLY),)
	@$(MAKE) -s --no-print-directory -C $(TOP) \
		-f build/linux/prepare_tree.mk
else
	@:
endif

include ../config/help.mk

# This deletes built-in suffix rules. Otherwise the submake isn't run when
# saying e.g. "make thingy.a"
.SUFFIXES:

# Because we have a match-anything rule below, we'll run the main build when
# we're actually trying to remake various makefiles after they're read in.
# These rules try to prevent that
%.mk: ;
Makefile%: ;
Makefile: ;

.PHONY: build kbuild install
build kbuild install: autogen
	@$(if $(MAKECMDGOALS),$(MAKE) $(MAKE_ETC) $(MAKECMDGOALS) $(eval MAKECMDGOALS :=),:)

%: autogen
	@$(if $(MAKECMDGOALS),$(MAKE) $(MAKE_ETC) $(MAKECMDGOALS) $(eval MAKECMDGOALS :=),:)

