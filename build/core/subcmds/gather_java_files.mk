# Spreadtrum Communication Inc. 2016

# A relationship of the $(SPRD_BUILD_SYSTEM)/definitions.mk all-java-files-under
# is redefined, and record the path into LOCAL_JAVA_FILES_PATH
ifdef LOCAL_JAVA_FILES_PATH
# So, add the prefix with relative path:
# Before: in Contacts: src ../ContactsCommon/src
# After add prefix: in Contacts: ../../../vendor/sprd/platform/packages/apps/Contacts/src ../../../vendor/sprd/platform/packages/apps/Contacts/../ContactsCommon/src
LOCAL_JAVA_FILES_PATH := $(addprefix $(LOCAL_VENDOR_RELATIVE_PATH)/, $(LOCAL_JAVA_FILES_PATH))

# clear and disable hooking
exists_path :=

# Ensure the folder is exists
$(foreach v,$(LOCAL_JAVA_FILES_PATH), \
  $(if $(wildcard $(LOCAL_PATH)/$(v)), \
    $(eval exists_path += $(v)),\
  ) \
)

# Using the exists folder to find the java files
ifdef exists_path
LOCAL_SRC_FILES += $(call all-java-files-under, $(exists_path))
endif

ifdef localDebug
$(info LOCAL_SRC_FILES := $(LOCAL_SRC_FILES))
endif

endif # LOCAL_JAVA_FILES_PATH is defined

