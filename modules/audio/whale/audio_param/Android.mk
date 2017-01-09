LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := audio_vbc_eq
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := FAKE
LOCAL_MODULE_SUFFIX := -timestamp

include $(BUILD_SYSTEM)/base_rules.mk

$(LOCAL_BUILT_MODULE): DSP_VBC_PARAM_FILE := /data/local/media/dsp_vbc.bin
$(LOCAL_BUILT_MODULE): NXP_PARAM_FILE := /data/local/media/nxp.bin
$(LOCAL_BUILT_MODULE): AUDIO_STRUCTURE_PARAM_FILE := /data/local/media/audio_structure.bin
ifeq ($(strip $(REALTEK_CODEC)), true)
$(LOCAL_BUILT_MODULE): REALTEK_CODEC_PARAM_FILE := /data/local/media/realtek_codec.bin
endif
$(LOCAL_BUILT_MODULE): DSP_VBC_SYMLINK := $(TARGET_OUT_VENDOR)/firmware/dsp_vbc
$(LOCAL_BUILT_MODULE): NXP_SYMLINK := $(TARGET_OUT_VENDOR)/firmware/nxp
$(LOCAL_BUILT_MODULE): AUDIO_STRUCTURE_SYMLINK := $(TARGET_OUT_VENDOR)/firmware/audio_structure
ifeq ($(strip $(REALTEK_CODEC)), true)
$(LOCAL_BUILT_MODULE): REALTEK_CODEC_SYMLINK := $(TARGET_OUT_VENDOR)/firmware/realtek_codec
endif
$(LOCAL_BUILT_MODULE): $(LOCAL_PATH)/Android.mk
$(LOCAL_BUILT_MODULE):
	$(hide) echo "Symlink: $(DSP_VBC_SYMLINK) -> $(DSP_VBC_PARAM_FILE)"
	$(hide) echo "Symlink: $(NXP_SYMLINK) -> $(NXP_PARAM_FILE)"
	$(hide) echo "Symlink: $(AUDIO_STRUCTURE_SYMLINK) -> $(AUDIO_STRUCTURE_PARAM_FILE)"
ifeq ($(strip $(REALTEK_CODEC)), true)
	$(hide) echo "Symlink: $(REALTEK_CODEC_SYMLINK) -> $(REALTEK_CODEC_PARAM_FILE)"
endif
	$(hide) mkdir -p $(dir $@)
	$(hide) mkdir -p $(dir $(DSP_VBC_SYMLINK))
	$(hide) rm -rf $@
	$(hide) rm -rf $(DSP_VBC_SYMLINK)
	$(hide) rm -rf $(NXP_SYMLINK)
	$(hide) rm -rf $(AUDIO_STRUCTURE_SYMLINK)
ifeq ($(strip $(REALTEK_CODEC)), true)
	$(hide) rm -rf $(REALTEK_CODEC_SYMLINK)
endif
	$(hide) ln -sf $(DSP_VBC_PARAM_FILE) $(DSP_VBC_SYMLINK)
	$(hide) ln -sf $(NXP_PARAM_FILE) $(NXP_SYMLINK)
	$(hide) ln -sf $(AUDIO_STRUCTURE_PARAM_FILE) $(AUDIO_STRUCTURE_SYMLINK)
ifeq ($(strip $(REALTEK_CODEC)), true)
	$(hide) ln -sf $(REALTEK_CODEC_PARAM_FILE) $(REALTEK_CODEC_SYMLINK)
endif
	$(hide) touch $@


include $(CLEAR_VARS)


LOCAL_SRC_FILES:= \
	audio_param_tool.cpp \
	../audio_xml_utils.cpp \
	
LOCAL_32_BIT_ONLY := true

LOCAL_C_INCLUDES += \
	external/tinyalsa/include \
	external/expat/lib \
	system/media/audio_utils/include \
	system/media/audio_effects/include \
	vendor/sprd/modules/audio/whale/record_process \
	vendor/sprd/modules/audio/whale/debug \
	vendor/sprd/modules/audio/whale/record_nr \
	vendor/sprd/modules/audio/whale \
	external/tinyxml \
	external/tinycompress/include

LOCAL_SHARED_LIBRARIES := \
	 liblog libcutils libtinyalsa libaudioutils \
	 libexpat libdl \
	 libmedia libutils \
	 libhardware_legacy libtinyxml

LOCAL_MODULE =	audio_param_tool

include $(BUILD_EXECUTABLE)
