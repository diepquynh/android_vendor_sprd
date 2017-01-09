
# Clear vars
LOCAL_VENDOR_RELATIVE_PATH :=

# Define the relative path of the original Android.mk
v := $(subst /, ,$(LOCAL_PATH))
v := $(patsubst %,..,$(v))
$(foreach st,$(v),\
   $(eval LOCAL_VENDOR_RELATIVE_PATH := ../$(LOCAL_VENDOR_RELATIVE_PATH)) \
)

LOCAL_VENDOR_RELATIVE_PATH := $(LOCAL_VENDOR_RELATIVE_PATH)$(LOCAL_VENDOR_PATH)

# Include additional makefile of the platform dir
include $(wildcard $(LOCAL_VENDOR_PATH)/additional.mk)

local_module_name := $(strip $(LOCAL_MODULE)$(LOCAL_PACKAGE_NAME))

ifdef local_module_name
include $(wildcard $(LOCAL_VENDOR_PATH)/$(local_module_name).mk)
endif

