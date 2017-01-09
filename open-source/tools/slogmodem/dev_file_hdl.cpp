/*
 *  dev_file_hdl.cpp - The device file handler.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-6-13 Zhang Ziyi
 *  Initial version.
 */

#include <fcntl.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "dev_file_hdl.h"

DeviceFileHandler::DeviceFileHandler(const LogString& dev_path, size_t buf_len,
                                     LogController* ctrl,
                                     Multiplexer* multiplexer)
    : FdHandler(-1, ctrl, multiplexer),
      m_close_fd{true},
      m_file_path(dev_path) {
  m_buffer.buffer = new uint8_t[buf_len];
  m_buffer.buf_size = buf_len;
  m_buffer.data_start = 0;
  m_buffer.data_len = 0;
}

DeviceFileHandler::DeviceFileHandler(int fd, size_t buf_len,
                                     LogController* ctrl,
                                     Multiplexer* multiplexer)
    : FdHandler(fd, ctrl, multiplexer), m_close_fd{false} {
  m_buffer.buffer = new uint8_t[buf_len];
  m_buffer.buf_size = buf_len;
  m_buffer.data_start = 0;
  m_buffer.data_len = 0;
}

DeviceFileHandler::~DeviceFileHandler() {
  delete[] m_buffer.buffer;
  if (!m_close_fd) {
    if (m_fd >= 0) {
      del_events(POLLIN | POLLOUT);
    }
    m_fd = -1;
  }
}

int DeviceFileHandler::close() {
  ::close(m_fd);
  m_fd = -1;
  return 0;
}

void DeviceFileHandler::process(int events) {
  if (!(events & POLLIN)) {
    err_log("file %s unexpected event %#x", ls2cstring(m_file_path), events);
    return;
  }

  // Drain data from the file
  size_t free_size;
  size_t free_start;
  while (true) {
    free_size = m_buffer.buf_size - m_buffer.data_start - m_buffer.data_len;
    free_start = m_buffer.data_start + m_buffer.data_len;
    ssize_t n = ::read(m_fd, m_buffer.buffer + free_start, free_size);
    if (-1 == n) {
      // TODO: there is a bug in the spipe driver: it will
      // return ENODATA instead of EAGAIN when there is no
      // data to read.
      if (EAGAIN != errno && ENODATA != errno) {
        err_log("read %s error", ls2cstring(m_file_path));
      }
      break;
    }
    if (!n) {
      err_log("read %s return 0", ls2cstring(m_file_path));
      break;
    }
    m_buffer.data_len += n;
    if (process_data(m_buffer)) {
      break;
    }
  }
}
