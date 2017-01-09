# CP log
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := slogmodem
LOCAL_INIT_RC := slogmodem.rc
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := agdsp_dump.cpp \
                   agdsp_log.cpp \
                   agdsp_pcm.cpp \
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
                   data_buf.cpp \
                   data_consumer.cpp \
                   data_proc_hdl.cpp \
                   dev_file_hdl.cpp \
                   dev_file_open.cpp \
                   diag_dev_hdl.cpp \
                   diag_stream_parser.cpp \
                   dump_stor_notifier.cpp \
                   ext_gnss_log.cpp \
                   ext_wcn_dump.cpp \
                   ext_storage_monitor.cpp \
                   fd_hdl.cpp \
                   file_watcher.cpp \
                   io_chan.cpp \
                   io_sched.cpp \
                   log_config.cpp \
                   log_config_with_modemtype.cpp \
                   log_ctrl.cpp \
                   log_file.cpp \
                   log_pipe_dev.cpp \
                   log_pipe_hdl.cpp \
                   major_minor_num_a6.cpp \
                   media_stor.cpp \
                   modem_stat_hdl.cpp \
                   multiplexer.cpp \
                   parse_utils.cpp \
                   pm_modem_dump.cpp \
                   pm_sensorhub_log.cpp \
                   slog_config.cpp \
                   stor_mgr.cpp \
                   timer_mgr.cpp \
                   trans_modem_ver.cpp \
                   wan_modem_log.cpp \
                   wan_modem_time_sync_hdl.cpp \
                   wcdma_iq_mgr.cpp \
                   wcdma_iq_notifier.cpp \
                   wcnd_stat_hdl.cpp

ifeq ($(strip $(SPRD_CP_LOG_WCN)), TSHARK)
    LOCAL_SRC_FILES += int_wcn_log.cpp
    LOCAL_CFLAGS += -DSUPPORT_WCN \
                    -DWCN_ASSERT_MESSAGE=\"WCN-CP2-EXCEPTION\" \
                    -DWCN_STAT_SOCK_NAME=\"wcnd\"
else ifeq ($(strip $(SPRD_CP_LOG_WCN)), MARLIN)
    LOCAL_SRC_FILES += ext_wcn_log.cpp
    LOCAL_CFLAGS += -DSUPPORT_WCN \
                    -DEXTERNAL_WCN \
                    -DWCN_ASSERT_MESSAGE=\"WCN-EXTERNAL-DUMP\" \
                    -DWCN_STAT_SOCK_NAME=\"external_wcn_slog\"
else ifeq ($(strip $(SPRD_CP_LOG_WCN)), MARLIN2)
    LOCAL_SRC_FILES += ext_wcn_log.cpp
    LOCAL_CFLAGS += -DSUPPORT_WCN \
                    -DEXTERNAL_WCN \
                    -DWCN_ASSERT_MESSAGE=\"WCN-CP2-EXCEPTION\" \
                    -DWCN_STAT_SOCK_NAME=\"wcnd\"
endif

ifeq ($(strip $(BOARD_SECURE_BOOT_ENABLE)), true)
    LOCAL_CFLAGS += -DSECURE_BOOT_ENABLE
endif

LOCAL_SHARED_LIBRARIES := libc \
                          libc++ \
                          libcutils \
                          liblog
LOCAL_CFLAGS += -DLOG_TAG=\"SLOGCP\" -DUSE_STD_CPP_LIB_ \
                -DTMP_CONFIG_PATH=\"/data/local/slogmodem\" \
                -D_REENTRANT

# slogmodem run time MACROs
# that need to be defined according to requirements:

# 1. EXT_STORAGE_PATH: Specify the mount point of the external SD.
#                      If not defined, slogmodem will use the first
#                      vfat or exfat file system in /proc/mounts.
#    LOCAL_CFILES += -DEXT_STORAGE_PATH=\"path of external storage\"

# 2. AP_LOG_TYPE: Save CP log under ap log directory(slog or ylog).
#                 By default, CP log is saved to /data/modem_log
#                 or /modem_log on external storage.
ifeq ($(strip $(CP_LOG_DIR_IN_AP)), ylog)
    LOCAL_CFLAGS += -DAP_LOG_TYPE=\"ylog\"
else ifeq ($(strip $(CP_LOG_DIR_IN_AP)), slog)
    LOCAL_CFLAGS += -DAP_LOG_TYPE=\"slog\"
endif

LOCAL_CPPFLAGS += -std=c++11
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := flush_slog_modem
LOCAL_SRC_FILES := utility/flush_slog_modem.cpp
LOCAL_SHARED_LIBRARIES := libc \
                          libc++ \
                          libcutils \
                          liblog
LOCAL_CFLAGS += -DLOG_TAG=\"CPLOG_FLUSH\" -DUSE_STD_CPP_LIB_
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := cplogctl
LOCAL_SRC_FILES := client_req.cpp \
                   parse_utils.cpp \
                   utility/clear_req.cpp \
                   utility/cplogctl.cpp \
                   utility/cplogctl_cmn.cpp \
                   utility/flush_req.cpp \
                   utility/get_log_file_size.cpp \
                   utility/get_storage_capacity.cpp \
                   utility/on_off_req.cpp \
                   utility/overwrite_req.cpp \
                   utility/query_state_req.cpp \
                   utility/set_ag_log_dest.cpp \
                   utility/set_ag_pcm.cpp \
                   utility/set_log_file_size.cpp \
                   utility/set_storage_capacity.cpp \
                   utility/slogm_req.cpp
LOCAL_SHARED_LIBRARIES := libc \
                          libcutils \
                          liblog \
                          libutils
LOCAL_CFLAGS += -DLOG_TAG=\"CPLOG_CTL\"
include $(BUILD_EXECUTABLE)

CUSTOM_MODULES += slogmodem flush_slog_modem cplogctl
