/*
 *  def_config.h - The default parameter configuration.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-2-16 Zhang Ziyi
 *  Initial version.
 */
#ifndef _DEF_CONFIG_H_
#define _DEF_CONFIG_H_

#define CP_LOG_CONFIG_FILE "/system/etc/slog_modem.conf"
#define CP_LOG_TMP_CONFIG_FILE "/data/local/tmp/slogmodem/slog_modem.conf"
#define TMP_SLOG_CONFIG "/data/local/tmp/slog/slog.conf"

#define PERSIST_MODEM_CHAR "persist.modem."

#define MODEM_W_DEVICE_PROPERTY "persist.modem.w.enable"
#define MODEM_TD_DEVICE_PROPERTY "persist.modem.t.enable"
#define MODEM_WCN_DEVICE_PROPERTY "ro.modem.wcn.enable"
#define MODEM_GNSS_DEVICE_PROPERTY "ro.modem.gnss.enable"
#define MODEM_L_DEVICE_PROPERTY "persist.modem.l.enable"
#define MODEM_TL_DEVICE_PROPERTY "persist.modem.tl.enable"
#define MODEM_FL_DEVICE_PROPERTY "persist.modem.lf.enable"
#define MODEM_TYPE_PROPERTY "ro.radio.modemtype"

#define MODEM_W_LOG_PROPERTY "ro.modem.w.log"
#define MODEM_TD_LOG_PROPERTY "ro.modem.t.log"
#define MODEM_WCN_LOG_PROPERTY "ro.modem.wcn.log"
#define MODEM_GNSS_LOG_PROPERTY "ro.modem.gnss.log"
#define MODEM_L_LOG_PROPERTY "ro.modem.l.log"
#define MODEM_TL_LOG_PROPERTY "ro.modem.tl.log"
#define MODEM_FL_LOG_PROPERTY "ro.modem.lf.log"

#define MODEM_W_DIAG_PROPERTY "ro.modem.w.diag"
#define MODEM_TD_DIAG_PROPERTY "ro.modem.t.diag"
#define MODEM_WCN_DIAG_PROPERTY "ro.modem.wcn.diag"
#define MODEM_GNSS_DIAG_PROPERTY "ro.modem.gnss.diag"
#define MODEM_L_DIAG_PROPERTY "ro.modem.l.diag"
#define MODEM_TL_DIAG_PROPERTY "ro.modem.tl.diag"
#define MODEM_FL_DIAG_PROPERTY "ro.modem.lf.diag"

#define MODEM_WCN_DEVICE_RESET "persist.sys.sprd.wcnreset"
#define MODEM_WCN_DUMP_LOG_COMPLETE "persist.sys.sprd.wcnlog.result"

#define SLOG_MODEM_SERVER_SOCK_NAME "slogmodem"

#define MODEM_STATE_SOCKET_NAME "modemd"
#ifdef EXTERNAL_WCN
#define WCN_STATE_SOCKET_NAME "external_wcn_slog"
#else
#define WCN_STATE_SOCKET_NAME "wcnd"
#endif

#define MODEMRESET_PROPERTY "persist.sys.sprd.modemreset"
#define MODEM_SOCKET_NAME "modemd"
#ifdef EXTERNAL_WCN
#define WCN_SOCKET_NAME "external_wcn_slog"
#else
#define WCN_SOCKET_NAME "wcnd"
#endif

#define CP_TIME_SYNC_SERVER_SOCKET "cp_time_sync_server"

#define MODEM_VERSION "gsm.version.baseband"

#ifdef HOST_TEST_
#define DEBUG_SMSG_PATH "/data/local/tmp/smsg"
#define DEBUG_SBUF_PATH "/data/local/tmp/sbuf"
#define DEBUG_SBLOCK_PATH "/data/local/tmp/sblock"
#else
#define DEBUG_SMSG_PATH "/d/sipc/smsg"
#define DEBUG_SBUF_PATH "/d/sipc/sbuf"
#define DEBUG_SBLOCK_PATH "/d/sipc/sblock"
#endif

#define MINI_DUMP_LTE_SRC_FILE "/proc/cptl/mini_dump"
#define MINI_DUMP_WCDMA_SRC_FILE "/proc/cpw/mini_dump"
#define MINI_DUMP_TD_SRC_FILE "/proc/cpt/mini_dump"

#define DEFAULT_EXT_LOG_SIZE_LIMIT (512 * 1024 * 1024)
#define DEFAULT_INT_LOG_SIZE_LIMIT (5 * 1024 * 1024)

#define LOG_FILE_WRITE_BUF_SIZE (1024 * 64)
#define SECURE_BOOT_SIZE 1024
#endif  // !_DEF_CONFIG_H_
