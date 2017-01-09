ifneq ($(TARGET_SIMULATOR),true)

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_JAVA_LIBRARIES := telephony-common
LOCAL_JAVA_LIBRARIES += com.broadcom.bt
LOCAL_JAVA_LIBRARIES += ims-common
LOCAL_JAVA_LIBRARIES += radio_interactor_common

LOCAL_SRC_FILES := $(call all-subdir-java-files) \
                        src/com/sprd/engineermode/debuglog/slogui/ISlogService.aidl \

LOCAL_PACKAGE_NAME := EngineerMode
LOCAL_STATIC_JAVA_LIBRARIES := httpmime apachemime android-support-v13
LOCAL_STATIC_JAVA_LIBRARIES += com.broadcom.bt
LOCAL_JAVA_LIBRARIES += org.apache.http.legacy

LOCAL_STATIC_JAVA_LIBRARIES += \
        android-support-v7-appcompat \
        android-support-v7-recyclerview \
        android-support-design

LOCAL_RESOURCE_DIR = \
         $(LOCAL_PATH)/res \
        frameworks/support/v7/appcompat/res \
        frameworks/support/v7/recyclerview/res \
        frameworks/support/design/res
LOCAL_AAPT_FLAGS := \
        --auto-add-overlay \
        --extra-packages android.support.v7.appcompat \
        --extra-packages android.support.v7.recyclerview \
        --extra-packages android.support.design

LOCAL_JNI_SHARED_LIBRARIES := libjni_engineermode
#LOCAL_JNI_SHARED_LIBRARIES += libfmjni
LOCAL_JNI_SHARED_LIBRARIES += libatci

ifneq ($(strip $(USE_SPRD_WCN)),true)
        LOCAL_INIT_RC := engineermode.rc
endif

LOCAL_CERTIFICATE := platform

LOCAL_REQUIRED_MODULES += \
    collect_apr_server \
    upload_apr  \
    apr_config

LOCAL_PROGUARD_ENABLED := disabled

include $(BUILD_PACKAGE)
include $(CLEAR_VARS)

LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := httpmime:httpmime-4.0.jar\
                                        apachemime:apache-mime4j-0.6.jar

LOCAL_PREBUILT_EXECUTABLES := \
    tools/collect_apr_server \
    tools/upload_apr \
	tools/apr_config

include $(BUILD_MULTI_PREBUILT)

include $(call all-makefiles-under,$(LOCAL_PATH))

endif
