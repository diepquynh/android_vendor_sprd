/*
 *  cp_sleep_log.cpp - The CP SleepLog class.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-6-13 Zhang Ziyi
 *  Initial version.
 */

#include <time.h>
#include "cp_dir.h"
#include "cp_sleep_log.h"
#include "cp_stor.h"
#include "diag_dev_hdl.h"
#include "multiplexer.h"

CpSleepLogConsumer::CpSleepLogConsumer(const LogString& cp_name,
                                       CpStorage& cp_stor)
    : DataConsumer(cp_name, cp_stor), m_timer{0}, m_file{0} {}

CpSleepLogConsumer::~CpSleepLogConsumer() {
  if (m_timer) {
    DiagDeviceHandler* dev = diag_dev();
    TimerManager& tmgr = dev->multiplexer()->timer_mgr();
    tmgr.del_timer(m_timer);
    m_timer = 0;
  }
}

LogFile* CpSleepLogConsumer::open_file() {
  char log_name[64];
  time_t t;
  struct tm lt;

  t = time(0);
  if (static_cast<time_t>(-1) == t || !localtime_r(&t, &lt)) {
    return NULL;
  }

  snprintf(log_name, sizeof log_name,
           "_sleeplog_%04d-%02d-%02d_%02d-%02d-%02d.log", lt.tm_year + 1900,
           lt.tm_mon + 1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);
  LogString file_name = cp_name() + log_name;
  LogFile* f = storage().create_file(file_name, LogFile::LT_SLEEPLOG);
  if (!f) {
    err_log("open sleep log file %s failed", ls2cstring(file_name));
  }
  return f;
}

void CpSleepLogConsumer::close_file() {
  m_file->close();
  m_file = 0;
}

void CpSleepLogConsumer::close_rm_file() {
  m_file->close();
  m_file->dir()->remove(m_file);
  m_file = 0;
}

int CpSleepLogConsumer::start() {
  m_file = open_file();
  if (!m_file) {
    return -1;
  }

  uint8_t* buf;
  size_t len;
  parser.frame(DIAG_READ_SLEEP_LOG, DIAG_REQ_SLEEP_LOG, NULL, 0, &buf, &len);
  DiagDeviceHandler* dev = diag_dev();
  ssize_t n = write(dev->fd(), buf, len);
  delete[] buf;
  if (static_cast<size_t>(n) != len) {
    return -1;
  }
  // Set read timeout
  TimerManager& tmgr = dev->multiplexer()->timer_mgr();
  m_timer = tmgr.create_timer(3000, sleep_log_read_timeout, this);
  return 0;
}

void CpSleepLogConsumer::sleep_log_read_timeout(void* param) {
  CpSleepLogConsumer* consumer = static_cast<CpSleepLogConsumer*>(param);

  consumer->m_timer = 0;
  consumer->close_file();
  err_log("read sleep log timeout");
  consumer->notify_client(LPR_FAILURE);
}

bool CpSleepLogConsumer::process(DeviceFileHandler::DataBuffer& buffer) {
  uint8_t* src_ptr = buffer.buffer + buffer.data_start;
  size_t src_len = buffer.data_len;
  uint8_t* dst_ptr;
  size_t dst_len;
  size_t read_len;
  bool has_frame;
  bool ret = false;

  // Reset timer
  TimerManager& tmgr = diag_dev()->multiplexer()->timer_mgr();
  tmgr.set_new_due_time(m_timer, 3000);

  while (src_len) {
    has_frame =
        parser.unescape(src_ptr, src_len, &dst_ptr, &dst_len, &read_len);
    src_len -= read_len;
    src_ptr += read_len;
    if (has_frame) {
      if (parser.get_type(dst_ptr) == DIAG_READ_SLEEP_LOG) {
        uint8_t subtype = parser.get_subytpe(dst_ptr);

        if (subtype == DIAG_RSP_SLEEP_LOG_OK) {
          tmgr.del_timer(m_timer);
          m_timer = 0;
          close_file();
          notify_client(LPR_SUCCESS);
          ret = true;
          break;
        } else if (subtype == DIAG_DATA_SLEEP_LOG) {
          uint8_t* data_ptr = parser.get_payload(dst_ptr);
          size_t data_len = dst_len - parser.get_head_size();
          ssize_t n = m_file->write(data_ptr, data_len);
          if (static_cast<size_t>(n) != data_len) {
            tmgr.del_timer(m_timer);
            m_timer = 0;
            close_rm_file();
            notify_client(LPR_FAILURE);
            ret = true;
            break;
          }
        } else if (subtype == DIAG_REQ_SLEEP_LOG) {
          tmgr.del_timer(m_timer);
          m_timer = 0;
          close_rm_file();
          notify_client(LPR_SLEEPLOG_NO_SUPPORTED);
          ret = true;
          break;
        } else {
          tmgr.del_timer(m_timer);
          m_timer = 0;
          close_rm_file();
          notify_client(LPR_FAILURE);
          ret = true;
          break;
        }
      }
    }
  }
  if (!ret) {
    buffer.data_start = 0;
    buffer.data_len = 0;
  }
  return ret;
}
