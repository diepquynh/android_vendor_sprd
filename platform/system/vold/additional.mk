#diasble the auto merge .cpp to LOCAL_SRC_FILES                                                                                                                                   
LOCAL_C_MERGE_SRC_DISABLED := true
USE_VOLD_EX := true
ifeq ($(USE_VOLD_EX),true)
    VOLD_COMMON_CFLAGS = -DVOLD_EX
endif

LOCAL_CFLAGS += $(VOLD_COMMON_CFLAGS)

ifeq ($(BOARD_SUPPORT_UMS_K44),true)
    LOCAL_CFLAGS += -DUMS_K44
endif

vold_or_libvold :=

ifeq (vold,$(LOCAL_MODULE))
	vold_or_libvold := true
else
	ifeq (libvold,$(LOCAL_MODULE))
	    vold_or_libvold := true
    endif
endif

#add complete .cpp files to LOCAL_SRC_FILES
ifeq (true,$(USE_VOLD_EX))
ifdef vold_or_libvold
vold_external_cpp := CommandEx.cpp fs/Exfat.cpp
LOCAL_SRC_FILES += $(addprefix $(LOCAL_VENDOR_RELATIVE_PATH)/src/,$(vold_external_cpp))
endif
endif
