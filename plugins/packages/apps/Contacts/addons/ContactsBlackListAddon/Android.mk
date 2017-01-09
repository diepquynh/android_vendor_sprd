LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_MODULE_TAGS := optional


#The Plugin Apk name, suggest the name rules as
#"HostAppName + PluginFunction + Addon".
#eg.FileExplorerDrmAddon, it means that this plugin
#is the Drm function of FileExplorer.
LOCAL_PACKAGE_NAME := ContactsBlackListAddon

#Add the APK libraries ensure that you can use the
#class of the host application
LOCAL_APK_LIBRARIES += Contacts

LOCAL_DEX_PREOPT := false

#Enable Proguard to make Java class file shrinked,
#optimized, obfuscated, and preverified.You also
#can use "LOCAL_PROGUARD_ENABLED := disabled" to
#disable it,the default is enable.
#proguard.flags is used to keep the unused class
#you want to reserve.
LOCAL_PROGUARD_FLAG_FILES := proguard.flags

include $(BUILD_ADDON_PACKAGE)
