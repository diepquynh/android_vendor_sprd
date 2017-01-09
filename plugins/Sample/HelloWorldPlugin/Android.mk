LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_MODULE_TAGS := optional

#The Plugin Apk name, suggest the name rules as
#"HostAppName + PluginFunction + Addon".
#eg.FileExplorerDrmAddon, it means that this plugin
#is the Drm function of FileExplorer.
LOCAL_PACKAGE_NAME := HelloWorldPlugin

#Add the APK libraries ensure that you can use the
#class of the host application
LOCAL_APK_LIBRARIES += HelloWorld

LOCAL_DEX_PREOPT := false

LOCAL_SDK_VERSION := current

LOCAL_JAVA_LIBRARIES := sprd-support-addon

#Enable Proguard to make Java class file shrinked,
#optimized, obfuscated, and preverified.You also
#can use "LOCAL_PROGUARD_ENABLED := disabled" to
#disable it,the default is enable.
#proguard.flags is used to keep the unused class
#you want to reserve.
LOCAL_PROGUARD_FLAG_FILES := proguard.flags

#Alongside the --auto-add-overlay to ensure that
#new resources in the overlays would be automatically
#added.
#The --extra-packages will allow this module can use the
#extra packages data(include resource and classs)
#LOCAL_AAPT_FLAGS := \
    --auto-add-overlay \
    --extra-packages com.sprd.fileexploreraddon \

#This Demo used BUILD_PACKAGE to build, BUT in your plugin
#module you SHOULD use BUILD_ADDON_PACKAGE to insure that
#your plugin apk will build into the properly directory
include $(BUILD_PACKAGE)
