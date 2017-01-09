/*
 *  dev_file_open.cpp - open function of DeviceFileHandler.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-6-17 Zhang Ziyi
 *  Initial version.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "dev_file_hdl.h"

int DeviceFileHandler::open() {
  m_fd = ::open(ls2cstring(m_file_path), O_RDWR | O_NONBLOCK);
  return m_fd;
}
