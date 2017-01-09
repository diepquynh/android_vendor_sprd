
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CERTIFICATE := platform
LOCAL_MODULE_TAGS := optional

LOCAL_STATIC_JAVA_LIBRARIES := asmack
LOCAL_JAVA_LIBRARIES += org.apache.http.legacy

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := SystemUpdate

#LOCAL_OVERRIDES_PACKAGES :=Note

include $(BUILD_PACKAGE)
##################################################
include $(CLEAR_VARS)

LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := asmack:libs/asmack-jse.jar

include $(BUILD_MULTI_PREBUILT)

include $(call all-makefiles-under,$(LOCAL_PATH))



#include $(BUILD_PREBUILT)

# Use the following include to make our test apk.

#include $(call all-makefiles-under,$(LOCAL_PATH))
