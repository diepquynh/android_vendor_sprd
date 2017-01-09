# CP log
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := slogmodem
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := agdsp_log.cpp \
                   client_hdl.cpp \
                   client_mgr.cpp \
                   client_req.cpp \
                   cp_dir.cpp \
                   cp_dump.cpp \
                   cp_log_cmn.cpp \
                   cp_log.cpp \
                   cp_ringbuf.cpp \
                   cp_set_dir.cpp \
                   cp_sleep_log.cpp \
                   cp_stat_hdl.cpp \
                   cp_stor.cpp \
                   data_consumer.cpp \
                   data_proc_hdl.cpp \
                   dev_file_hdl.cpp \
                   dev_file_open.cpp \
                   diag_dev_hdl.cpp \
                   diag_stream_parser.cpp \
                   ext_gnss_log.cpp \
                   ext_wcn_dump.cpp \
                   ext_storage_monitor.cpp \
                   fd_hdl.cpp \
                   file_notifier.cpp \
                   file_watcher.cpp \
                   log_config.cpp \
                   log_config_with_modemtype.cpp \
                   log_ctrl.cpp \
                   log_file.cpp \
                   log_pipe_dev.cpp \
                   log_pipe_hdl.cpp \
                   major_minor_num_a6.cpp \
                   media_stor.cpp \
                   modem_dump.cpp \
                   modem_stat_hdl.cpp \
                   multiplexer.cpp \
                   parse_utils.cpp \
                   slog_config.cpp \
                   stor_mgr.cpp \
                   timer_mgr.cpp \
                   wan_modem_log.cpp \
                   wcdma_iq_mgr.cpp \
                   wcdma_iq_notifier.cpp

ifeq ($(strip $(SPRD_EXTERNAL_WCN)), true)
    LOCAL_CFLAGS += -DEXTERNAL_WCN
    LOCAL_SRC_FILES += ext_wcn_log.cpp ext_wcn_stat_hdl.cpp
else
    LOCAL_SRC_FILES += int_wcn_log.cpp int_wcn_stat_hdl.cpp
endif

ifeq ($(strip $(BOARD_SECURE_BOOT_ENABLE)), true)
    LOCAL_CFLAGS += -DSECURE_BOOT_ENABLE
endif

LOCAL_SHARED_LIBRARIES := libc \
              libcutils \
              liblog \
              libutils
LOCAL_CFLAGS += -DLOG_TAG=\"SLOGCP\"
LOCAL_CPPFLAGS += -std=c++11
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := flush_slog_modem
LOCAL_SRC_FILES := flush_slog_modem.cpp
LOCAL_SHARED_LIBRARIES := libc \
              libcutils \
              liblog \
              libutils
LOCAL_CFLAGS += -DLOG_TAG=\"CPLOG_FLUSH\"
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := cplogctl
LOCAL_SRC_FILES := ctrl_modem_log.cpp
LOCAL_SHARED_LIBRARIES := libc \
              libcutils \
              liblog \
              libutils
LOCAL_CFLAGS += -DLOG_TAG=\"CPLOG_CTL\"
include $(BUILD_EXECUTABLE)

CUSTOM_MODULES += slogmodem flush_slog_modem cplogctl
