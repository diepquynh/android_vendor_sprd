/*
 *  wcdma_iq_mgr.cpp - The WCDMA I/Q data manager.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-7-23 Zhang Ziyi
 *  Initial version.
 */

#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#include "cp_dir.h"
#include "cp_stor.h"
#include "log_pipe_hdl.h"
#include "wcdma_iq_mgr.h"
#include "wiq_drv_api.h"

WcdmaIqManager::WcdmaIqManager(LogPipeHandler* cp)
    : FdHandler(-1, cp->controller(), cp->multiplexer()),
      m_cp{cp},
      m_cp_stor{0},
      m_state{IQS_IDLE},
      m_max_iq_segs{60},
      m_iq_buffer{0},
      m_cur_seg_file{0},
      m_iq_stor_notifier{0},
      m_iq_stor_thread{0},
      m_stor_thread_sock{-1},
      m_saved_size{0} {}

WcdmaIqManager::~WcdmaIqManager() { stop(); }

int WcdmaIqManager::start(CpStorage* cp_stor) {
  if (IQS_IDLE != m_state) {
    return -1;
  }

  if (prepare_iq_mem_map()) {
    return -1;
  }

  m_cp_stor = cp_stor;

  // Start saving thread
  if (start_saving_thread()) {
    goto startThreadFail;
  }

  add_events(POLLIN);

  m_state = IQS_WAITING;
  m_cp_stor->subscribe_ext_stor_umount_evt(this, notify_ext_stor_umount);
  return 0;

startThreadFail:
  close();
  m_cp_stor = 0;
  return -1;
}

int WcdmaIqManager::stop() {
  if (IQS_IDLE != m_state) {
    m_cp_stor->unsubscribe_ext_stor_umount_evt(this);
    m_iq_stor_notifier->send_command(SaveIqNotifier::IQST_EXIT);

    // Wait for the thread to quit
    pthread_join(m_iq_stor_thread, 0);

    delete m_iq_stor_notifier;
    m_iq_stor_notifier = 0;
    ::close(m_stor_thread_sock);
    m_stor_thread_sock = -1;

    // m_cur_set_file is already in m_iq_files
    if (m_cur_seg_file) {
      m_cur_seg_file->count_size(m_saved_size);
      m_cur_seg_file->close();
      m_cur_seg_file = 0;
    }

    m_cp_stor = 0;

    // Set /dev/iq_mem mode back to USB port
    int mode = IQ_USB_MODE;
    ioctl(m_fd, CMD_SET_IQ_CH_TYPE, mode);
    munmap(m_iq_buffer, WCDMA_IQ_BUF_SIZE);
    close();

    m_state = IQS_IDLE;
  }

  return 0;
}

int WcdmaIqManager::resume() {
  int ret = 0;

  if (IQS_IDLE == m_state) {
    ret = -1;
  } else if (IQS_PAUSED == m_state) {
    add_events(POLLIN);
    ioctl(fd(), CMD_SET_IQ_RD_FINISHED);
    m_state = IQS_WAITING;
  }

  return ret;
}

int WcdmaIqManager::pause() {
  int ret = 0;

  if (IQS_SAVING == m_state) {
    del_events(POLLIN);

    // Send pause command
    m_iq_stor_notifier->send_command(SaveIqNotifier::IQST_PAUSE);
    while (true) {
      int resp = m_iq_stor_notifier->wait_response();
      if (SaveIqNotifier::IQST_PAUSED == resp) {
        break;
      }
    }

    // Update size
    m_cur_seg_file->count_size(m_saved_size);
    m_cur_seg_file->close();
    m_cur_seg_file = 0;

    m_state = IQS_PAUSED;
  } else if (IQS_WAITING == m_state) {
    del_events(POLLIN);
    m_state = IQS_PAUSED;
  } else if (IQS_IDLE == m_state) {
    ret = -1;
  }

  return ret;
}

void WcdmaIqManager::pre_clear() { m_iq_files.clear(); }

int WcdmaIqManager::prepare_iq_mem_map() {
  m_fd = open("/dev/iq_mem", O_RDONLY);
  if (-1 == m_fd) {
    err_log("can not open /dev/iq_mem");
    return -1;
  }

  int mode = IQ_SLOG_MODE;
  int err = ioctl(m_fd, CMD_SET_IQ_CH_TYPE, mode);
  if (-1 == err) {
    close();
    err_log("ioctl CMD_SET_IQ_CH_TYPE error");
    return -1;
  }

  // Map the buffer
  m_iq_buffer = static_cast<uint8_t*>(
      mmap(0, WCDMA_IQ_BUF_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, m_fd, 0));
  if (MAP_FAILED == m_iq_buffer) {
    close();
    m_iq_buffer = 0;
    err_log("mmap error");
    return -1;
  }

  info_log("/dev/iq_mem mapped to %p", m_iq_buffer);

  return 0;
}

int WcdmaIqManager::gen_iq_file_name(LogString& file_name) const {
  time_t t;
  struct tm lt;

  t = time(0);
  if (static_cast<time_t>(-1) == t || !localtime_r(&t, &lt)) {
    return -1;
  }

  char time_str[80];

  snprintf(time_str, sizeof time_str, "_wiq_%04d-%02d-%02d_%02d-%02d-%02d.bin",
           lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday, lt.tm_hour, lt.tm_min,
           lt.tm_sec);
  file_name = m_cp->name() + time_str;
  return 0;
}

LogFile* WcdmaIqManager::create_seg_file() {
  while (m_iq_files.size() >= m_max_iq_segs) {
    LogList<LogFile*>::iterator it = m_iq_files.begin();
    if (it == m_iq_files.end()) {
      break;
    }
    LogFile* f = *it;
    m_iq_files.erase(it);
    f->dir()->remove(f);
  }

  LogString iq_file_name;

  if (gen_iq_file_name(iq_file_name)) {
    return 0;
  }
  LogFile* f = m_cp_stor->create_file(iq_file_name, LogFile::LT_WCDMA_IQ);
  if (f) {
    m_iq_files.push_back(f);
  }

  return f;
}

int WcdmaIqManager::start_saving_thread() {
  // Create socket pair for communication
  int socks[2];
  int err;

  err = socketpair(AF_LOCAL, SOCK_STREAM, 0, socks);
  if (-1 == err) {
    return -1;
  }

  // Put the sockets to non-blocking mode
  set_nonblock(socks[0]);
  set_nonblock(socks[1]);

  // socks[0] will be used by SaveIqNotifier, socks[1] will be used
  // by the saving thread (and stored in m_stor_thread_sock).
  m_iq_stor_notifier = new SaveIqNotifier(socks[0], this);
  m_iq_stor_notifier->add_events(POLLIN);
  m_stor_thread_sock = socks[1];

  // Start the thread now.
  err = pthread_create(&m_iq_stor_thread, 0, stor_thread, this);
  if (err) {
    goto cr_thrd_fail;
  }

  return 0;

cr_thrd_fail:
  delete m_iq_stor_notifier;
  m_iq_stor_notifier = 0;
  ::close(m_stor_thread_sock);
  m_stor_thread_sock = -1;
  return -1;
}

void WcdmaIqManager::process(int /*events*/) {
  m_cur_seg_file = create_seg_file();
  if (!m_cur_seg_file) {
    ioctl(fd(), CMD_SET_IQ_RD_FINISHED);
    err_log("create WCDMA I/Q file error");
    return;
  }

  if (m_iq_stor_notifier->send_command(SaveIqNotifier::IQST_SAVE)) {
    // Send command failed. Wait for the next I/Q block.
    ll_remove(m_iq_files, m_cur_seg_file);
    m_cur_seg_file->close();
    m_cur_seg_file->dir()->remove(m_cur_seg_file);
    m_cur_seg_file = 0;
    ioctl(fd(), CMD_SET_IQ_RD_FINISHED);
  } else {
    m_state = IQS_SAVING;
    // Don't wait for I/Q data until the saving is finished.
    del_events(POLLIN);
  }
}

void WcdmaIqManager::process_stor_response(int resp) {
  switch (resp) {
    case SaveIqNotifier::IQST_SAVED:
      if (IQS_SAVING == m_state) {
        info_log("WCDMA I/Q saved");

        m_cur_seg_file->count_size(m_saved_size);
        m_cur_seg_file->close();
        m_cur_seg_file = 0;
        add_events(POLLIN);
        m_state = IQS_WAITING;
        ioctl(fd(), CMD_SET_IQ_RD_FINISHED);
      } else {
        err_log("IQST_SAVED received in state %d", m_state);
      }
      break;
    default:
      err_log("response %d received in state %d", resp, m_state);
      break;
  }
}

bool WcdmaIqManager::process_save() {
  // Get the data offset and size
  iq_buf_info iq_info;
  int err = ioctl(fd(), CMD_GET_IQ_BUF_INFO, &iq_info);

  if (-1 == err) {
    err_log("ioctl CMD_GET_IQ_BUF_INFO error");
    m_saved_size = 0;
    send_byte(m_stor_thread_sock, SaveIqNotifier::IQST_SAVED);
    return true;
  }

  // Control structure at the beginning and end of the buffer
  // is not saved.
  size_t len = iq_info.data_len;
  size_t remain_len = len;
  uint8_t* p = m_iq_buffer + iq_info.base_offs;
  int cmd = -1;

  while (remain_len) {
    size_t block_len = remain_len > SAVE_BLOCK ? SAVE_BLOCK : remain_len;
    ssize_t n = m_cur_seg_file->write_raw(p, block_len);
    if (n > 0) {
      remain_len -= n;
      p += n;
    }
    if (static_cast<size_t>(n) != block_len) {
      break;
    }
    // Check whether there is command.
    uint8_t cmd_buf;

    n = ::read(m_stor_thread_sock, &cmd_buf, 1);
    if (1 == n) {
      if (SaveIqNotifier::IQST_EXIT == cmd_buf ||
          SaveIqNotifier::IQST_PAUSE == cmd_buf) {
        cmd = cmd_buf;
        break;
      }
    }
  }

  m_saved_size = len - remain_len;

  bool ret = true;
  if (SaveIqNotifier::IQST_EXIT == cmd) {
    ret = false;
  } else if (SaveIqNotifier::IQST_PAUSE == cmd) {
    send_byte(m_stor_thread_sock, SaveIqNotifier::IQST_PAUSED);
  } else {
    send_byte(m_stor_thread_sock, SaveIqNotifier::IQST_SAVED);
  }

  return ret;
}

void* WcdmaIqManager::stor_thread(void* param) {
  WcdmaIqManager* iq = static_cast<WcdmaIqManager*>(param);
  struct pollfd iq_pol;
  bool run = true;

  iq_pol.fd = iq->m_stor_thread_sock;
  iq_pol.events = POLLIN;
  while (run) {
    int err = poll(&iq_pol, 1, -1);
    if (err > 0 && (iq_pol.revents & POLLIN)) {
      uint8_t cmd;
      ssize_t n;

      n = ::read(iq->m_stor_thread_sock, &cmd, 1);
      if (1 == n) {
        switch (cmd) {
          case SaveIqNotifier::IQST_EXIT:
            // Time to quit
            run = false;
            break;
          case SaveIqNotifier::IQST_PAUSE:
            iq->send_byte(iq->m_stor_thread_sock, SaveIqNotifier::IQST_PAUSED);
            break;
          case SaveIqNotifier::IQST_SAVE:
            run = iq->process_save();
            break;
          default:
            break;
        }
      }
    }
  }

  return 0;
}

int WcdmaIqManager::send_byte(int fd, int value) {
  uint8_t buf = static_cast<uint8_t>(value);

  ssize_t n = ::write(fd, &buf, 1);
  return 1 == n ? 0 : -1;
}

void WcdmaIqManager::clear_iq_files() {
  m_iq_stor_notifier->send_command(SaveIqNotifier::IQST_PAUSE);
  while (true) {
    int resp = m_iq_stor_notifier->wait_response();
    if (SaveIqNotifier::IQST_PAUSED == resp) {
      break;
    }
  }

  m_cur_seg_file->close();
  m_cur_seg_file = 0;
  m_iq_files.clear();

  ioctl(fd(), CMD_SET_IQ_RD_FINISHED);
  add_events(POLLIN);
  m_state = IQS_WAITING;
}

void WcdmaIqManager::notify_ext_stor_umount(void *client) {
  WcdmaIqManager* wiq = static_cast<WcdmaIqManager*>(client);

  if (IQS_SAVING == wiq->m_state) {
    wiq->clear_iq_files();
  }
}
