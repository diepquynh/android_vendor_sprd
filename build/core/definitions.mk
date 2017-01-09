# Nothing yet, but will be more infuture
SPRD_BUILD_SYSTEM := vendor/sprd/build/core

include $(wildcard $(SPRD_BUILD_SYSTEM)/config.mk)
 
define all-java-files-under
$(strip \
  $(patsubst ./%,%, \
    $(shell cd $(LOCAL_PATH) ; \
            find -L $(1) -name "*.java" -and -not -name ".*") \
  ) \
  $(eval LOCAL_JAVA_FILES_PATH := $(1) $(LOCAL_JAVA_FILES_PATH)) \
)
endef

define find-other-java-files
$(strip \
        $(call find-subdir-files,$(1) -name "*.java" -and -not -name ".*") \
        $(eval LOCAL_JAVA_FILES_PATH := $(1) $(LOCAL_JAVA_FILES_PATH)) \
)
endef

define all-c-files-under
$(strip \
  $(call all-named-files-under,*.c,$(1)) \
  $(eval LOCAL_C_FILES_PATH := $(1) $(LOCAL_C_FILES_PATH )) \
)
endef

define all-cpp-files-under
$(strip \
$(sort $(patsubst ./%,%, \
  $(shell cd $(LOCAL_PATH) ; \
          find -L $(1) -name "*$(or $(LOCAL_CPP_EXTENSION),.cpp)" -and -not -name ".*") \
 )) \
  $(strip $(eval LOCAL_C_FILES_PATH := $(1) $(LOCAL_C_FILES_PATH)))
)
endef

define all-c-cpp-files-under
$(strip \
 $(sort $(patsubst ./%,%, \
   $(shell cd $(LOCAL_PATH) ; \
           find -L $(1) -name "*$(or $(LOCAL_CPP_EXTENSION),.cpp)" -and -not -name ".*") \
   $(strip $(call all-named-files-under,*.c, $(1))) \
  )) \
)
endef
