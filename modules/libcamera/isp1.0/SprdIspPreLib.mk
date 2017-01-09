# add lib

ifeq ($(strip $(ISP_HW_VER)),1.0)

include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := \
	$(ISP_LIB_PATH)/lib/libae.so \
	$(ISP_LIB_PATH)/lib/libawb.so \
	$(ISP_LIB_PATH)/lib/libaf.so \
	$(ISP_LIB_PATH)/lib/liblsc.so \
	$(ISP_LIB_PATH)/lib/libev.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)


endif
