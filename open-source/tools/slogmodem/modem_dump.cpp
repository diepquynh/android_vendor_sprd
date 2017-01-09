/*
 *  modem_dump.cpp - The 3G/4G MODEM dump class.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-6-15 Zhang Ziyi
 *  Initial version.
 */
#include "cp_stor.h"
#include "diag_dev_hdl.h"
#include "modem_dump.h"
#include "multiplexer.h"
#include "parse_utils.h"
#include "wan_modem_log.h"

ModemDumpConsumer::ModemDumpConsumer(const LogString& cp_name,
                                     CpStorage& cp_stor, const struct tm& lt,
                                     const char* dump_path)
    : m_dump_path (dump_path),
      CpDumpConsumer(cp_name, cp_stor, lt), m_timer{0} {}

ModemDumpConsumer::~ModemDumpConsumer() {
  if (m_timer) {
    TimerManager& tmgr = diag_dev()->multiplexer()->timer_mgr();
    tmgr.del_timer(m_timer);
  }
  storage().unsubscribe_ext_stor_umount_evt(this);
}

int ModemDumpConsumer::start() {
  // Send command
  uint8_t dump_cmd[2] = {0x33, 0xa};
  DiagDeviceHandler* diag = diag_dev();
  int ret = -1;

  ssize_t n = ::write(diag->fd(), dump_cmd, 2);
  if (2 == n) {
    if (open_dump_file()) {
      storage().subscribe_ext_stor_umount_evt(this, notify_ext_stor_umount);
      TimerManager& tmgr = diag->multiplexer()->timer_mgr();
      m_timer = tmgr.create_timer(3000, dump_read_timeout, this);
      ret = 0;
    }
  }

  return ret;
}

bool ModemDumpConsumer::process(DeviceFileHandler::DataBuffer& buffer)
//{Debug: test /proc/cpxxx/mem
#if 0
{
  TimerManager& tmgr = diag_dev()->multiplexer()->timer_mgr();

  remove_dump_file();
  tmgr.del_timer(m_timer);
  m_timer = 0;
  notify_client(LPR_FAILURE);
  return true;
}
#else
{
  bool ret = false;
  bool found = false;

  // if the last frame
  found = check_ending(buffer);

  // Destroy the timer
  TimerManager& tmgr = diag_dev()->multiplexer()->timer_mgr();
  tmgr.set_new_due_time(m_timer, 3000);

  LogFile* f = dump_file();
  ssize_t n = f->write(buffer.buffer + buffer.data_start, buffer.data_len);
  if (static_cast<size_t>(n) != buffer.data_len) {
    remove_dump_file();
    tmgr.del_timer(m_timer);
    m_timer = 0;
    ret = true;
    err_log("need to write %lu, write returns %d",
            static_cast<unsigned long>(buffer.data_len), static_cast<int>(n));

    if (save_dump_file()) {
      notify_client(LPR_SUCCESS);
    } else {
      notify_client(LPR_FAILURE);
    }
  } else {
    buffer.data_start = 0;
    buffer.data_len = 0;
    if (found) {
      diag_dev()->del_events(POLLIN);
      f->close();
      tmgr.del_timer(m_timer);
      m_timer = 0;
      notify_client(LPR_SUCCESS);
      info_log("dump complete");
    }
    ret = true;
  }
  return ret;
}
#endif

void ModemDumpConsumer::dump_read_timeout(void* param) {
  ModemDumpConsumer* consumer = static_cast<ModemDumpConsumer*>(param);

  consumer->m_timer = 0;
  consumer->remove_dump_file();
  err_log("read timeout");

  if (consumer->save_dump_file()) {
    consumer->notify_client(LPR_SUCCESS);
  } else {
    consumer->notify_client(LPR_FAILURE);
  }
}

bool ModemDumpConsumer::check_ending(DeviceFileHandler::DataBuffer& buffer) {
  uint8_t* src_ptr = buffer.buffer + buffer.data_start;
  size_t src_len = buffer.data_len;
  uint8_t* dst_ptr;
  size_t dst_len;
  size_t read_len;
  bool has_frame;
  bool ret = false;

  while (src_len) {
    has_frame =
        m_parser.unescape(src_ptr, src_len, &dst_ptr, &dst_len, &read_len);
    src_len -= read_len;
    src_ptr += read_len;
    if (has_frame) {
      uint8_t* data_ptr = m_parser.get_payload(dst_ptr);
      size_t data_len = dst_len - m_parser.get_head_size();
      if (data_len == 21 && !memcmp(data_ptr, "\nmodem_memdump_finish", 21)) {
        ret = true;
      }
    }
  }
  return ret;
}

void ModemDumpConsumer::notify_ext_stor_umount(void *client) {
  ModemDumpConsumer* consumer = static_cast<ModemDumpConsumer*>(client);
  if (consumer->m_timer) {
    TimerManager& tmgr = consumer->diag_dev()->multiplexer()->timer_mgr();
    tmgr.del_timer(consumer->m_timer);
    consumer->m_timer = 0;
  }
  consumer->notify_client(LPR_FAILURE);
}

bool ModemDumpConsumer::save_dump_file() {
  bool ret = false;

  diag_dev()->del_events(POLLIN);
  LogFile* mem_file = open_dump_mem_file();
  if (mem_file) {
    if (mem_file->copy(m_dump_path)) {
      err_log("dump proc memory: %s failed.", m_dump_path);
    } else {
      info_log("dump proc memory: %s successfully.", m_dump_path);
      ret = true;
    }
    mem_file->close();
  } else {
    err_log("create dump mem file failed");
  }

  return ret;
}
