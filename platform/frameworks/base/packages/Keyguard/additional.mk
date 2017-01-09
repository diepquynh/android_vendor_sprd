ifeq ($(strip $(LOCAL_MODULE)),Keyguard)
keyguard_resource_overlays := $(strip \
    $(wildcard $(foreach dir, $(FEATURES.PRODUCT_PACKAGE_OVERLAYS), \
      $(addprefix $(dir)/, $(LOCAL_RESOURCE_DIR)))))

LOCAL_RESOURCE_DIR := $(keyguard_resource_overlays) $(LOCAL_RESOURCE_DIR)

ifeq ($(filter --auto-add-overlay,$(LOCAL_AAPT_FLAGS)),)
LOCAL_AAPT_FLAGS += --auto-add-overlay
endif

endif

