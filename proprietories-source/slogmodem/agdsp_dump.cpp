/*
 *  agdsp_dump.cpp - The AG-DSP dump class.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-6-1 Zhang Ziyi
 *  Initial version.
 */

#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "agdsp_dump.h"
#include "audio_dsp_ioctl.h"
#include "cp_dir.h"
#include "cp_stor.h"
#include "multiplexer.h"

AgdspDump::AgdspDump(LogController* ctrl, Multiplexer* multiplexer,
                     const char* dump_dev)
    :FdHandler{-1, ctrl, multiplexer},
     m_dump_device{dump_dev},
     m_notifier{nullptr},
     m_timer{nullptr},
     m_sock_fd{-1},
     m_dump_file{nullptr},
     m_state{DS_IDLE},
     m_saved_size{0},
     m_event_client{nullptr},
     m_event_callback{nullptr},
     m_result_client{nullptr},
     m_result_notifier{nullptr} {
  pthread_mutex_init(&m_lock, nullptr);
}

AgdspDump::~AgdspDump() {
  stop();
  pthread_mutex_destroy(&m_lock);
}

int AgdspDump::start() {
  if (DS_IDLE != m_state) {
    return -1;
  }

  m_fd = ::open(ls2cstring(m_dump_device), O_RDONLY);
  if (m_fd < 0) {
    err_log("open AG-DSP dump device error");
    m_timer =
        multiplexer()->timer_mgr().create_timer(3000, reopen_dump_dev, this);
  } else {
    if (start_saving_thread()) {
      err_log("start saving thread fail.");
      close();
      return -1;
    } else {
      m_state = DS_WAITING;
      multiplexer()->register_fd(this, POLLIN);
    }
  }
  return 0;
}

int AgdspDump::start_saving_thread() {
  int ret = -1;
  // create socket pair for agdsp dump thread
  int socks[2];

  ret = socketpair(AF_LOCAL, SOCK_STREAM, 0, socks);
  if (-1 != ret) {
    // Put the sockets to non-blocking mode
    if (set_nonblock(socks[0]) || set_nonblock(socks[1])) {
      err_log("agdsp dump socket set O_NONBLOCK failed.");
      ::close(socks[0]);
      ::close(socks[1]);
      ret = -1;
    } else {
      m_notifier = new DumpStoreNotifier(socks[0], this);
      multiplexer()->register_fd(m_notifier, POLLIN);
      m_sock_fd = socks[1];

      ret = pthread_create(&m_save_thread, 0, agdsp_dump, this);
      if (ret) {
        err_log("agdsp dump thread creation fail.");

        delete m_notifier;
        m_notifier = nullptr;

        ::close(m_sock_fd);
        m_sock_fd = -1;
        ret = -1;
      }
    }
  } else {
    err_log("socketpair creation for agdsp dump is failed.");
  }

  return ret;
}

void* AgdspDump::agdsp_dump(void* param) {
  AgdspDump* agdsp_dump = static_cast<AgdspDump*>(param);
  struct pollfd dump_pol;
  bool run = true;

  dump_pol.fd = agdsp_dump->m_sock_fd;
  dump_pol.events = POLLIN;

  while (run) {
    int err = poll(&dump_pol, 1, -1);
    if (err > 0 && (dump_pol.revents & POLLIN)) {
      ssize_t n;
      uint8_t cmd;

      n = ::read(agdsp_dump->m_sock_fd, &cmd, 1);

      if (1 == n) {
        int save_result;

        switch (cmd) {
          case DumpStoreNotifier::START_DUMP:
            pthread_mutex_lock(&agdsp_dump->m_lock);
            save_result = agdsp_dump->save_dump();
            pthread_mutex_unlock(&agdsp_dump->m_lock);
            if (save_result) {
              send_byte(agdsp_dump->m_sock_fd, DumpStoreNotifier::DUMP_FAILED);
            } else {
              send_byte(agdsp_dump->m_sock_fd, DumpStoreNotifier::DUMP_SAVED);
            }
            break;
          case DumpStoreNotifier::STOP_DUMP:
            run = false;
            break;
          default:
            break;
        }
      }
    }
  }

  return 0;
}

void AgdspDump::start_dump(CpStorage& stor, const LogString& name) {
  time_t t_now = time(0);
  struct tm lt;

  if (static_cast<time_t>(-1) == t_now || !localtime_r(&t_now, &lt)) {
    end_dump();
    m_state = DS_FAIL;

    // notify result
    m_result_notifier(m_result_client, DS_FAIL);
    err_log("get local time for dump file name error.");
    return;
  }

  pthread_mutex_lock(&m_lock);

  char td[32];
  snprintf(td, sizeof td,
           "%04d-%02d-%02d_%02d-%02d-%02d.mem",
           lt.tm_year + 1900,
           lt.tm_mon + 1,
           lt.tm_mday,
           lt.tm_hour,
           lt.tm_min,
           lt.tm_sec);

  LogString file_name = name + "_memory_" + td;
  m_dump_file = stor.create_file(file_name, LogFile::LT_DUMP);

  pthread_mutex_unlock(&m_lock);

  if (!m_dump_file) {
    end_dump();
    m_state = DS_FAIL;

    // notify result
    m_result_notifier(m_result_client, DS_FAIL);
    err_log("Agdsp dump file creation fail.");
    return;
  }

  if (m_notifier->send_dump_cmd(DumpStoreNotifier::START_DUMP)) {
    end_dump();
    m_state = DS_FAIL;
    m_dump_file->close();
    m_dump_file->dir()->remove(m_dump_file);
    m_dump_file = nullptr;
    // notify result
    m_result_notifier(m_result_client, DS_FAIL);

    err_log("AGDSP dump start command fail.");
  } else {
    del_events(POLLIN);
    m_state = DS_DUMPING;
  }
}

void AgdspDump::process_stor_response(int resp) {
  if (DS_DUMPING == m_state) {
    pthread_mutex_lock(&m_lock);

    if (DumpStoreNotifier::DUMP_SAVED == resp) {
      m_state = DS_SUCCESS;
      m_dump_file->count_size(m_saved_size);
      m_dump_file->close();
    } else if (DumpStoreNotifier::DUMP_FAILED == resp) {
      m_state = DS_FAIL;
      m_dump_file->close();
      m_dump_file->dir()->remove(m_dump_file);
    }

    pthread_mutex_unlock(&m_lock);

    m_saved_size = 0;
    m_dump_file = nullptr;
    end_dump();
    // notify result
    m_result_notifier(m_result_client, m_state);

    add_events(POLLIN);
  } else {
    err_log("response %d received in state %d", resp, m_state);
  }
}

int AgdspDump::wait_dump() {
  if (m_state == DS_DUMPING) {
    process_stor_response(m_notifier->wait_response());
  }

  return 0;
}

int AgdspDump::stop() {
  if (DS_IDLE == m_state) {
    if (m_timer) {
      multiplexer()->timer_mgr().del_timer(m_timer);
      m_timer = nullptr;
    }
  } else {
    multiplexer()->unregister_fd(this);

    if (m_notifier->send_dump_cmd(DumpStoreNotifier::STOP_DUMP)) {
      err_log("AGDSP dump stop command fail.");
    }
    pthread_join(m_save_thread, 0);

    delete m_notifier;
    m_notifier = nullptr;

    ::close(m_sock_fd);
    m_sock_fd = -1;

    if (m_dump_file) {
      m_dump_file->close();
      m_dump_file = nullptr;
    }

    close();
    m_state = DS_IDLE;
  }

  return 0;
}

void AgdspDump::reopen_dump_dev(void* param) {
  AgdspDump* dump = static_cast<AgdspDump*>(param);

  dump->m_timer = nullptr;
  dump->start();
}

void AgdspDump::set_dump_start_callback(void* client,
                                        dump_event_callback callback) {
  m_event_client = client;
  m_event_callback = callback;
}

void AgdspDump::set_dump_result_notifier(void* client,
                                         dump_result_notifier callback) {
  m_result_client = client;
  m_result_notifier = callback;
}

void AgdspDump::process(int events) {
  // When the AG-DSP dump device is readable, it has asserted.
  if ((DS_IDLE == m_state) || (DS_DUMPING == m_state)) {
    if (DS_IDLE == m_state) {
      end_dump();
    }

    err_log("Unexpected agdsp assert event received under state %d.", m_state);
    return;
  }

  // Notify the agdsp assert state
  m_event_callback(m_event_client);
}

int AgdspDump::save_dump() {
  int ret = -1;
  uint8_t* buf;

  // Read the dump device file
  buf = new uint8_t[AGDSP_DUMP_BUF_SIZE];

  while (true) {
    ssize_t nr = read(fd(), buf, AGDSP_DUMP_BUF_SIZE);
    if (-1 == nr) {
      if (EINTR == errno) {
        continue;
      }
      // Other errors: stop polling the file
      err_log("read AG-DSP dump device error");
      break;
    }

    if (!nr) {
      // Dump finished
      ret = 0;
      break;
    }

    ssize_t nwr = m_dump_file->write_raw(buf, nr);
    if (nwr != nr) {
      err_log("write AG-DSP dump error");
      break;
    }

    m_saved_size += nwr;
  }

  delete [] buf;

  return ret;
}

void AgdspDump::end_dump() {
  ioctl(fd(), DSPLOG_CMD_DSPASSERT, 0);
}

int AgdspDump::send_byte(int fd, int value) {
  uint8_t buf = static_cast<uint8_t>(value);

  ssize_t n = ::write(fd, &buf, 1);
  return 1 == n ? 0 : -1;
}
