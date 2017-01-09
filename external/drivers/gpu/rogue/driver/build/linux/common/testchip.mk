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

ifeq ($(RGX_BVNC),1.82.4.5)
 $(eval $(call KernelConfigC,TC_APOLLO_ES2,))
else ifeq ($(RGX_BVNC),4.31.4.55)
 $(eval $(call KernelConfigC,TC_APOLLO_BONNIE,))
else ifeq ($(RGX_BVNC),12.4.1.48)
 $(eval $(call KernelConfigC,TC_APOLLO_TCF5,))
 $(eval $(call KernelConfigC,TC_APOLLO_TCF5_12_4_1_48,))
else ifeq ($(RGX_BVNC),14.8p.1.20)
 $(eval $(call KernelConfigC,TC_APOLLO_TCF5,))
 $(eval $(call KernelConfigC,TC_APOLLO_TCF5_14_8p_1_20,))
else ifeq ($(RGX_BVNC),22.18.22.22)
 $(eval $(call KernelConfigC,TC_APOLLO_TCF5,))
 $(eval $(call KernelConfigC,TC_APOLLO_TCF5_22_18_22_22,))
else ifeq ($(RGX_BVNC),22.19.54.24)
 $(eval $(call KernelConfigC,TC_APOLLO_TCF5,))
 $(eval $(call KernelConfigC,TC_APOLLO_TCF5_22_19_54_24,))
else ifeq ($(RGX_BVNC),22.23p.22.26)
 $(eval $(call KernelConfigC,TC_APOLLO_TCF5,))
 $(eval $(call KernelConfigC,TC_APOLLO_TCF5_22_23p_22_26,))
else ifeq ($(RGX_BVNC),22.25.54.24)
 $(eval $(call KernelConfigC,TC_APOLLO_TCF5,))
 $(eval $(call KernelConfigC,TC_APOLLO_TCF5_22_25_54_24,))
else ifeq ($(RGX_BVNC),22.26.54.24)
 $(eval $(call KernelConfigC,TC_APOLLO_TCF5,))
 $(eval $(call KernelConfigC,TC_APOLLO_TCF5_22_26_54_24,))
else ifeq ($(RGX_BVNC),22.30.54.25)
 $(eval $(call KernelConfigC,TC_APOLLO_TCF5,))
 $(eval $(call KernelConfigC,TC_APOLLO_TCF5_22_30_54_25,))
else ifeq ($(RGX_BVNC),22.32.54.328)
 $(eval $(call KernelConfigC,TC_APOLLO_TCF5,))
 $(eval $(call KernelConfigC,TC_APOLLO_TCF5_22_32_54_328,))
else ifeq ($(RGX_BVNC),22.33.21.11)
 $(eval $(call KernelConfigC,TC_APOLLO_TCF5,))
 $(eval $(call KernelConfigC,TC_APOLLO_TCF5_22_33_21_11,))
else ifeq ($(RGX_BVNC),22.34.22.23)
 $(eval $(call KernelConfigC,TC_APOLLO_TCF5,))
 $(eval $(call KernelConfigC,TC_APOLLO_TCF5_22_34_22_23,))
else ifeq ($(RGX_BVNC),22.36.54.28)
 $(eval $(call KernelConfigC,TC_APOLLO_TCF5,))
 $(eval $(call KernelConfigC,TC_APOLLO_TCF5_22_36_54_28,))
endif

ifeq ($(PVR_SYSTEM),rgx_linux_apollo)
 # The new rgx_linux_apollo system code only supports LMA
 override TC_MEMORY_CONFIG := TC_MEMORY_LOCAL
endif

ifneq ($(PVR_SYSTEM),rgx_nohw)
 ifeq ($(TC_MEMORY_CONFIG),)
  $(error TC_MEMORY_CONFIG must be defined)
 endif
endif

$(eval $(call TunableBothConfigC,TC_MEMORY_CONFIG,$(TC_MEMORY_CONFIG),\
Selects the memory configuration to be used. The choices are:_\
* TC_MEMORY_LOCAL (Rogue and the display controller use local card memory)_\
* TC_MEMORY_HOST (Rogue and the display controller use system memory)_\
* TC_MEMORY_HYBRID (Rogue can use both system and local memory and the display controller uses local card memory)))

ifeq ($(APOLLO_FAKE_INTERRUPTS),1)
$(eval $(call KernelConfigC,APOLLO_FAKE_INTERRUPTS,))
endif

ifeq ($(TC_MEMORY_CONFIG),TC_MEMORY_LOCAL)
 ifeq ($(TC_DISPLAY_MEM_SIZE),)
  $(error TC_DISPLAY_MEM_SIZE must be set in $(PVR_BUILD_DIR)/Makefile)
 endif
 $(eval $(call KernelConfigC,TC_DISPLAY_MEM_SIZE,$(TC_DISPLAY_MEM_SIZE)))
 $(eval $(call BothConfigC,LMA,))
 $(eval $(call KernelConfigMake,LMA,1))
else ifeq ($(TC_MEMORY_CONFIG),TC_MEMORY_HYBRID)
 ifeq ($(TC_DISPLAY_MEM_SIZE),)
  $(error TC_DISPLAY_MEM_SIZE must be set in $(PVR_BUILD_DIR)/Makefile)
 endif
 $(eval $(call KernelConfigC,TC_DISPLAY_MEM_SIZE,$(TC_DISPLAY_MEM_SIZE)))
endif

$(eval $(call TunableKernelConfigC,SUPPORT_APOLLO_FPGA,))
