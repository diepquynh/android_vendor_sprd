# Copyright 2008, The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := src/com/sprd/appbackup/service/IAppBackupAgent.aidl \
    src/com/sprd/appbackup/service/IAppBackupRestoreObserver.aidl \
    src/com/sprd/appbackup/service/IAppBackupRepository.aidl \
    src/com/sprd/appbackup/service/IScanAgentAndArchiveListener.aidl \
    src/com/sprd/appbackup/service/Archive.java \
    src/com/sprd/appbackup/service/Agent.java \
    src/com/sprd/appbackup/service/Config.java \
    src/com/sprd/appbackup/service/Category.java \
    src/com/sprd/appbackup/service/Account.java \
    src/com/sprd/appbackup/service/AbstractAppBackupAgent.java \
    src/com/sprd/appbackup/utils/StorageUtil.java \
    src/com/sprd/appbackup/utils/StorageUtilImpl.java \


LIB_FILES := $(LOCAL_SRC_FILES)

LOCAL_MODULE := com.sprd.appbackup.service
include $(BUILD_STATIC_JAVA_LIBRARY)

# Build the Email application itself, along with its tests and the tests for the emailcommon
# static library.  All tests can be run via runtest email

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(call all-java-files-under, src)
LOCAL_SRC_FILES += src/com/sprd/appbackup/service/IAppBackupManager.aidl
LOCAL_SRC_FILES := $(filter-out $(LIB_FILES),$(LOCAL_SRC_FILES))
LOCAL_STATIC_JAVA_LIBRARIES := com.sprd.appbackup.service\
android-support-v4\
android-support-v13\
apachetar\

LOCAL_CERTIFICATE := platform
LOCAL_PACKAGE_NAME := SprdAppBackup

LOCAL_PROGUARD_FLAG_FILES := proguard.flags

include $(BUILD_PACKAGE)

include $(CLEAR_VARS)
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := apachetar:lib/commons-compress-1.4.1.jar
include $(BUILD_MULTI_PREBUILT)
