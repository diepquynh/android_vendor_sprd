/*
 *  data_proc_hdl.cpp - The socket data handler class implementation.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-2-16 Zhang Ziyi
 *  Initial version.
 */
#include <unistd.h>
#include <errno.h>
#include "cp_log_cmn.h"
#include "data_proc_hdl.h"

DataProcessHandler::DataProcessHandler(int sock, LogController* ctrl,
                                       Multiplexer* multi, size_t buf_len)
    : FdHandler{sock, ctrl, multi},
      m_buffer{buf_len, new uint8_t[buf_len], 0, 0} {}

DataProcessHandler::~DataProcessHandler() { delete[] m_buffer.buffer; }

void DataProcessHandler::process(int /*events*/) {
  // Client socket connection readable

  size_t free_size = m_buffer.buf_size - m_buffer.data_start;
  ssize_t n = read(m_fd, m_buffer.buffer + m_buffer.data_start, free_size);

  if (!n) {
    // The connection is closed by the peer
    process_conn_closed();
  } else if (-1 == n) {
    if (EAGAIN != errno && EINTR != errno) {
      process_conn_error(errno);
    }
  } else {
    // Data read
    m_buffer.data_len += n;
    process_data();
  }
}
