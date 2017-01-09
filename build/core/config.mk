# SPRD_BUILD_SYSTEM := vendor/sprd/build/core

# ensuring use sprd feature control
ifneq ($(wildcard vendor/sprd/feature_configs),)
APPLY_PRODUCT_REVISION := $(SPRD_BUILD_SYSTEM)/apply_product_revision.mk
endif

# Gathering product revision
include $(APPLY_PRODUCT_REVISION)

VENDOR_PLATFORM := vendor/sprd/platform
CLEAR_VARS := $(SPRD_BUILD_SYSTEM)/clear_vars.mk $(CLEAR_VARS)
BUILD_ADDON_PACKAGE := $(SPRD_BUILD_SYSTEM)/addon_package.mk
BUILD_PACKAGE := $(wildcard $(SPRD_BUILD_SYSTEM)/package.mk) $(BUILD_PACKAGE)
BUILD_JAVA_LIBRARY := $(wildcard $(SPRD_BUILD_SYSTEM)/java_library.mk) $(BUILD_JAVA_LIBRARY)
BUILD_STATIC_JAVA_LIBRARY := $(wildcard $(SPRD_BUILD_SYSTEM)/java_library.mk) $(BUILD_STATIC_JAVA_LIBRARY)
BUILD_SHARED_LIBRARY := $(wildcard $(SPRD_BUILD_SYSTEM)/shared_library.mk) $(BUILD_SHARED_LIBRARY)
BUILD_EXECUTABLE := $(wildcard $(SPRD_BUILD_SYSTEM)/executable.mk) $(BUILD_EXECUTABLE)
BUILD_STATIC_LIBRARY := $(wildcard $(SPRD_BUILD_SYSTEM)/static_library.mk) $(BUILD_STATIC_LIBRARY)
