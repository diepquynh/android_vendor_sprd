###############################################################################
#
# Copyright (C) u-blox ag
# u-blox ag, Thalwil, Switzerland
#
# All rights reserved.
#
# This  source  file  is  the  sole  property  of  u-blox  AG. Reproduction or
# utilization of this source in whole or part is forbidden without the written 
# consent of u-blox AG.
#
###############################################################################
#
# Project: PE_AND
#
###############################################################################
ifeq ($(BOARD_GPS),ublox)

SUPL_ENABLED := 1

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SHARED_LIBRARIES := \
	liblog \
	libcutils

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/parser

PARSER_SRC_FILES := \
	parser/database.cpp \
	parser/parserbuffer.cpp \
	parser/protocolnmea.cpp \
	parser/protocolubx.cpp \
	parser/protocolunknown.cpp \
	parser/stdafx.cpp

LOCAL_SRC_FILES := \
	$(PARSER_SRC_FILES) \
	ubx_moduleIf.cpp \
	ubx_serial.cpp \
	ubx_udpServer.cpp \
	ubx_localDb.cpp \
	ubx_timer.cpp \
	ubx_debugIf.cpp \
	ubx_rilIf.cpp \
	ubx_niIf.cpp \
	ubx_xtraIf.cpp \
	ubx_agpsIf.cpp \
	ubx_cfg.cpp \
	ubx_log.cpp \
	ubxgpsstate.cpp \
	gps_thread.cpp

LOCAL_CFLAGS := \
	-DPLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION) \
	-DUNIX_API \
	-DANDROID_BUILD \
	-DUDP_SERVER_PORT=46434



# Additions for SUPL
ifeq ($(SUPL_ENABLED),1)
LOCAL_C_INCLUDES += external/openssl/include/
LOCAL_C_INCLUDES += external/openssl/
LOCAL_C_INCLUDES += external/

LOCAL_SHARED_LIBRARIES += libcrypto
LOCAL_SHARED_LIBRARIES += libssl
LOCAL_STATIC_LIBRARIES += libSupl

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/supl \
	$(LOCAL_PATH)/supl/asn1c_header

SUPL_SOURCE_FILES := \
	supl/rrlpmanager.cpp \
	supl/suplSMmanager.cpp \
	supl/upldecod.cpp \
	supl/uplsend.cpp \
	supl/rrlpdecod.cpp

LOCAL_SRC_FILES += $(SUPL_SOURCE_FILES)
LOCAL_CFLAGS += -DSUPL_ENABLED

endif

LOCAL_MODULE := gps.default

LOCAL_MODULE_TAGS := eng

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw

include $(BUILD_SHARED_LIBRARY)

########################
include $(CLEAR_VARS)

LOCAL_MODULE := gps.conf 

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_CLASS := ETC

LOCAL_MODULE_PATH := $(TARGET_OUT)/etc

LOCAL_SRC_FILES := config/gps.conf 
                   
include $(BUILD_PREBUILT)
########################

########################
include $(CLEAR_VARS)

LOCAL_MODULE := u-blox.conf 

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_CLASS := ETC

LOCAL_MODULE_PATH := $(TARGET_OUT)/etc

LOCAL_SRC_FILES := config/u-blox.conf 
                   

include $(BUILD_PREBUILT)
########################

ifeq ($(SUPL_ENABLED),1)
include $(CLEAR_VARS)

MY_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(LOCAL_PATH)/supl/asn1c_header/
include $(LOCAL_PATH)/asn1.mk


LOCAL_C_INCLUDES += \
	$(LOCAL_PATH) 

LOCAL_SRC_FILES := \
	$(ASN_MODULE_SOURCES)

LOCAL_MODULE := libSupl

LOCAL_MODULE_TAGS := eng

LOCAL_CFLAGS :=

LOCAL_PRELINK_MODULE := false


include $(BUILD_STATIC_LIBRARY)
LOCAL_PATH := $(MY_LOCAL_PATH)
endif

# STFU
# FIXME: do it properly.. how?!
ifeq ($(SUPL_ENABLED),1)
NOTICE-TARGET-STATIC_LIBRARIES-libSupl:
	@echo shut up
endif
endif

