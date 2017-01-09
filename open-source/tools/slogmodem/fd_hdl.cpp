/*
 *  fd_hdl.cpp - The file descriptor handler base class implementation.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-2-16 Zhang Ziyi
 *  Initial version.
 */
#include <unistd.h>

#include "fd_hdl.h"
#include "multiplexer.h"

FdHandler::FdHandler(int fd, LogController* ctrl, Multiplexer* multiplexer)
    : m_fd(fd), m_log_ctrl(ctrl), m_multiplexer(multiplexer) {}

FdHandler::~FdHandler() {
  if (m_fd >= 0) {
    m_multiplexer->unregister_fd(this, POLLIN);
    ::close(m_fd);
  }
}

void FdHandler::add_events(int events) {
  m_multiplexer->register_fd(this, events);
}

void FdHandler::del_events(int events) {
  m_multiplexer->unregister_fd(this, events);
}

int FdHandler::close() {
  int ret = 0;

  if (m_fd >= 0) {
    ret = ::close(m_fd);
    m_fd = -1;
  }

  return ret;
}
