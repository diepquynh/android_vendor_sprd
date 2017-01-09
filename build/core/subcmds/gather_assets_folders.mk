# Spreadtrum Communication 2016. Inc

# Append assets checked
ifndef LOCAL_ASSET_DIR

# Clear user hooking
LOCAL_VENDOR_ASSET_DIR :=

LOCAL_VENDOR_ASSET_DIR := $(wildcard $(vendor_dir)/assets)

ifneq ($(strip $(LOCAL_VENDOR_ASSET_DIR)),)
LOCAL_ASSET_DIR := $(LOCAL_VENDOR_ASSET_DIR) $(wildcard $(LOCAL_PATH)/assets)

endif # Assume the LOCAL_VENDOR_ASSET_DIR exists

ifdef localDebug
$(info LOCAL_ASSET_DIR := $(LOCAL_ASSET_DIR))
endif

else # If LOCAL_ASSET_DIR defined or not ->

# validate every dir has exists and put them into LOCAL_VENDOR_ASSET_DIR
$(foreach v, $(LOCAL_ASSET_DIR), \
    $(eval LOCAL_VENDOR_ASSET_DIR += $(wildcard $(vendor_dir)/$(v))) \
)

ifneq ($(strip $(LOCAL_VENDOR_ASSET_DIR)),)
LOCAL_ASSET_DIR := $(strip $(LOCAL_VENDOR_ASSET_DIR)) $(LOCAL_ASSET_DIR)
endif

ifdef localDebug
$(info LOCAL_ASSET_DIR := $(LOCAL_ASSET_DIR))
endif

endif


