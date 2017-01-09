# Spreadtrum Communication Inc. 2016

# Append resources checked
ifdef LOCAL_RESOURCE_DIR
# clear the user hooking
LOCAL_VENDOR_RESOURCE_DIR :=

# validate every dir has exists and put them into LOCAL_VENDOR_RESOURCE_DIR
$(foreach v, $(LOCAL_RESOURCE_DIR), \
    $(eval LOCAL_VENDOR_RESOURCE_DIR += $(wildcard $(VENDOR_PLATFORM)/$(v))) \
)

# Put the LOCAL_VENDOR_RESOURCE_DIR ahead of LOCAL_RESOURCE_DIR
LOCAL_RESOURCE_DIR := $(strip $(LOCAL_VENDOR_RESOURCE_DIR)) $(LOCAL_RESOURCE_DIR)

ifneq ($(strip $(LOCAL_VENDOR_RESOURCE_DIR)),)
LOCAL_RESOURCE_DIR := $(strip $(LOCAL_VENDOR_RESOURCE_DIR)) $(LOCAL_RESOURCE_DIR)

endif # Assume the LOCAL_VENDOR_RESOURCE_DIR exists

ifdef localDebug
$(info LOCAL_RESOURCE_DIR := $(LOCAL_RESOURCE_DIR))
endif

else # If LOCAL_RESOURCE_DIR defined or not ->

LOCAL_VENDOR_RESOURCE_DIR := $(wildcard $(LOCAL_VENDOR_PATH)/res)
# NOTICE, LOCAL_VENDOR_RESOURCE_DIR must be put in front of origin one.
LOCAL_RESOURCE_DIR := $(LOCAL_VENDOR_RESOURCE_DIR) $(wildcard $(LOCAL_PATH)/res)

ifdef localDebug
$(info LOCAL_RESOURCE_DIR := $(LOCAL_RESOURCE_DIR))
endif

endif # If LOCAL_RESOURCE_DIR not defined

# We may add --add-auto-overlay flag into AAPT CONFIG to avoid build error
ifneq (,$(strip $(LOCAL_VENDOR_RESOURCE_DIR)))
ifeq ($(filter --auto-add-overlay,$(LOCAL_AAPT_FLAGS)),)
LOCAL_AAPT_FLAGS += --auto-add-overlay
endif # Append --add-auto-overlay flag
endif

ifdef localDebug
$(info LOCAL_AAPT_FLAGS += $(LOCAL_AAPT_FLAGS))
endif
