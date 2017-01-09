# If use oem partitions, add plugins into oem
ifdef BOARD_OEMIMAGE_PARTITION_SIZE
LOCAL_OEM_MODULE := true
endif

LOCAL_MULTILIB ?= both

include $(BUILD_PACKAGE)
