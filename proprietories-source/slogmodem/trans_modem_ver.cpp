/*
 *  trans_modem_ver.cpp - The CP SleepLog class.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-6-8 Zhang Ziyi
 *  Initial version.
 */

#include <unistd.h>

#include "diag_dev_hdl.h"
#include "multiplexer.h"
#include "trans_modem_ver.h"

WanModemVerQuery::WanModemVerQuery(const LogString& cp_name,
                                   CpStorage& cp_stor)
    :DataConsumer(cp_name, cp_stor),
     m_max_times{3},
     m_failed_times{0},
     m_state{QS_NOT_BEGIN},
     m_timer{nullptr},
     m_ver_len{0},
     m_modem_ver{nullptr} {}

WanModemVerQuery::~WanModemVerQuery() {
  if (m_timer) {
    DiagDeviceHandler* dev = diag_dev();
    TimerManager& tmgr = dev->multiplexer()->timer_mgr();
    tmgr.del_timer(m_timer);
    m_timer = 0;
  }
  delete [] m_modem_ver;
}

int WanModemVerQuery::send_query() {
  static uint8_t s_ver_req[] = { 0x7e, 0, 0, 0, 0, 8, 0, 0, 0, 0x7e };
  DiagDeviceHandler* dev = diag_dev();
  ssize_t nwr = write(dev->fd(), s_ver_req, sizeof s_ver_req);

  return nwr == sizeof s_ver_req ? 0 : -1;
}

int WanModemVerQuery::start_query() {
  // Send version query command
  if (send_query()) {
    err_log("send MODEM version query error");
    return -1;
  }

  info_log("querying MODEM version ...");

  // Start timer
  DiagDeviceHandler* dev = diag_dev();
  TimerManager& tmgr = dev->multiplexer()->timer_mgr();

  m_timer = tmgr.create_timer(2000, ver_query_timeout, this);

  m_state = QS_WAIT_RESULT;

  return 0;
}

int WanModemVerQuery::start() {
  return start_query();
}

void WanModemVerQuery::stop() {
  if (QS_WAIT_RESULT == m_state) {
    DiagDeviceHandler* dev = diag_dev();
    TimerManager& tmgr = dev->multiplexer()->timer_mgr();
    tmgr.del_timer(m_timer);
    m_timer = 0;

    m_parser.reset();

    m_state = QS_FINISHED;
  }
}

bool WanModemVerQuery::process(DataBuffer& buffer) {
  if (QS_WAIT_RESULT != m_state) {
    buffer.data_start = 0;
    buffer.data_len = 0;
    return false;
  }

  // Parse the data
  bool ret = false;

  while (buffer.data_len) {
    uint8_t* frame_ptr;
    size_t frame_len;
    size_t parsed_len;
    bool has_frame = m_parser.unescape(buffer.buffer + buffer.data_start,
                                       buffer.data_len,
                                       &frame_ptr, &frame_len,
                                       &parsed_len);
    if (has_frame && frame_len > 8 && !frame_ptr[6] && !frame_ptr[7]) {  // Version result
      DiagDeviceHandler* dev = diag_dev();
      TimerManager& tmgr = dev->multiplexer()->timer_mgr();
      tmgr.del_timer(m_timer);
      m_timer = 0;

      m_ver_len = frame_len - 8;
      m_modem_ver = new char[m_ver_len];
      memcpy(m_modem_ver, frame_ptr + 8, m_ver_len);

      buffer.data_len = 0;
      m_state = QS_FINISHED;
      break;
    }

    buffer.data_start += parsed_len;
    buffer.data_len -= parsed_len;
  }

  if (buffer.data_len) {
    if (buffer.data_start) {
      memmove(buffer.buffer, buffer.buffer + buffer.data_start, buffer.data_len);
      buffer.data_start = 0;
    }
  } else {
    buffer.data_start = 0;
  }

  if (QS_FINISHED == m_state) {
    notify_client(LPR_SUCCESS);
    ret = true;
  }
  return ret;
}

void WanModemVerQuery::ver_query_timeout(void* param) {
  err_log("MODEM version query timeout");

  WanModemVerQuery* query = static_cast<WanModemVerQuery*>(param);

  query->m_timer = nullptr;

  ++query->m_failed_times;
  if (query->m_failed_times >= query->m_max_times) {  // Query failed
    query->m_state = QS_FINISHED;
    query->notify_client(LPR_FAILURE);
    return;
  }

  // Reset the parser state
  query->m_parser.reset();

  if (query->start_query()) {
    query->m_state = QS_FINISHED;
    query->notify_client(LPR_FAILURE);
  }
}
