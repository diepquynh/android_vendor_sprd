/*
 *  set_log_file_size.cpp - set log file's max size.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-11-4 YAN Zhihang
 *  Initial version.
 */

#include "set_log_file_size.h"

SetLogFileSize::SetLogFileSize(unsigned size)
    : m_size{size} {}

int SetLogFileSize::do_request() {
  char buf[64];
  size_t len;

  len = snprintf(buf, sizeof buf, "SET_LOG_FILE_SIZE %u\n", m_size);

  if (send_req(buf, len)) {
    report_error(RE_SEND_CMD_ERROR);
    return -1;
  }

  return wait_simple_response(DEFAULT_RSP_TIME);
}
