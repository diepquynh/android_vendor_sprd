/*
 *  log_pipe_dev.cpp - The open function for the CP log device.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-3-2 Zhang Ziyi
 *  Initial version.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "log_pipe_hdl.h"

int LogPipeHandler::open() {
  m_fd = ::open(ls2cstring(m_log_dev_path), O_RDWR | O_NONBLOCK);
  return m_fd;
}
