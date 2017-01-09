# Spreadtrum Communication Inc. 2016

auto_merge_src_path := $(strip $(wildcard $(LOCAL_VENDOR_PATH)/src) $(wildcard $(LOCAL_VENDOR_PATH)/$(LOCAL_MODULE)_src))
ifdef auto_merge_src_path

  ifdef LOCAL_C_FILES_PATH

    ifeq (,$(filter src,$(LOCAL_C_FILES_PATH)))
      LOCAL_C_FILES_PATH += src
    endif # ensure src is not in LOCAL_C_FILES_PATH

    ifeq (,$(filter $(LOCAL_MODULE)_src,$(LOCAL_C_FILES_PATH)))
      LOCAL_C_FILES_PATH += $(LOCAL_MODULE)_src
    endif # ensure $(LOCAL_MODULE)_src is not in LOCAL_C_FILES_PATH

  else # LOCAL_C_FILES_PATH == null

    LOCAL_C_FILES_PATH += src $(LOCAL_MODULE)_src

  endif # LOCAL_C_FILES_PATH end

endif # auto_merge_src_path is defined

# If disable auto merge compiling, just clear LOCAL_C_FILES_PATH to skip
ifeq (true,$(strip $(LOCAL_C_MERGE_SRC_DISABLED)))
LOCAL_C_FILES_PATH := 
endif

# A relationship of the $(SPRD_BUILD_SYSTEM)/definitions.mk all-c-files-under
# is redefined, and record the path into LOCAL_C_FILES_PATH
ifdef LOCAL_C_FILES_PATH
# So, add the prefix with relative path:
# Before: in Contacts: src ../ContactsCommon/src
# After add prefix: in Contacts: ../../../vendor/sprd/platform/packages/apps/Contacts/src ../../../vendor/sprd/platform/packages/apps/Contacts/../ContactsCommon/src
LOCAL_C_FILES_PATH := $(addprefix $(LOCAL_VENDOR_RELATIVE_PATH)/, $(LOCAL_C_FILES_PATH))

# clear and disable hooking
exists_path :=

# Ensure the folder is exists
$(foreach v,$(LOCAL_C_FILES_PATH), \
  $(if $(wildcard $(LOCAL_PATH)/$(v)), \
    $(eval exists_path += $(v)),\
  ) \
)

# Using the exists folder to find the java files
ifdef exists_path
LOCAL_SRC_FILES += $(call all-c-cpp-files-under, $(exists_path))
endif

ifdef localDebug
#$(info LOCAL_C_FILES_PATH := $(LOCAL_C_FILES_PATH))
#$(info exists_path := $(exists_path))
$(info LOCAL_SRC_FILES := $(LOCAL_SRC_FILES))
endif

endif # LOCAL_C_FILES_PATH is defined
