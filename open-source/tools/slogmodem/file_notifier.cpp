/*
 *  data_notifier.cpp - The file data notifier class.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-2-20 Zhang Ziyi
 *  Initial version.
 */

#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "file_notifier.h"

FileNotifier::FileNotifier(LogController* ctrl, Multiplexer* multiplexer,
                           const char* file_path)
  :FdHandler{-1, ctrl, multiplexer},
   m_file_path{file_path},
   m_client{0},
   m_callback{0} {}

int FileNotifier::start() {
  int ret = 0;

  if (-1 == fd()) {
    m_fd = ::open(m_file_path, O_RDONLY);
    if (-1 == m_fd) {
      ret = -1;
    } else {
      add_events(POLLIN);
    }
  }

  return ret;
}

void FileNotifier::stop() {
  if (fd() >= 0) {
    del_events(POLLIN);
    close();
  }
}

void FileNotifier::process(int events) {
  if (m_callback) {
    m_callback(this, m_client);
  }
}
