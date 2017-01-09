ifeq ($(strip $(LOCAL_PACKAGE_NAME)),CarrierConfig)
LOCAL_MANIFEST_FILE := $(LOCAL_VENDOR_RELATIVE_PATH)/AndroidManifest.xml
LOCAL_ASSET_DIR := vendor/sprd/platform/packages/apps/CarrierConfig/assets
#SPRD: [Bug475223] Build version code based on seconds since 1970-01-01 00:00:00 UTC.
#In order to make sure carrier config data cache stored in user equipment can be updated after OTA.
LOCAL_AAPT_FLAGS += --version-code $(shell date +%s)
endif