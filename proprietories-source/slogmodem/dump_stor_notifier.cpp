/*
 *  dump_stor_notifier.cpp - The AG-DSP dump notifier.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-6-15 Yan Zhihang
 *  Initial version.
 */

#include <poll.h>
#include <unistd.h>

#include "agdsp_dump.h"

AgdspDump::DumpStoreNotifier::DumpStoreNotifier(int fd, AgdspDump* dump)
    : FdHandler{fd, dump->controller(), dump->multiplexer()},
      m_dump{dump} {}

void AgdspDump::DumpStoreNotifier::process(int events) {
  while (true) {
    uint8_t cmd;

    ssize_t ret = ::read(fd(), &cmd, 1);
    if (ret <= 0) {
      break;
    }

    m_dump->process_stor_response(cmd);
  }
}

int AgdspDump::DumpStoreNotifier::send_dump_cmd(int cmd) {
  int ret = 0;

  ssize_t n = ::write(fd(), &cmd, 1);
  if (1 != n) {
    ret = -1;
    err_log("write to agdsp dump socket fail.");
  }

  return ret;
}

int AgdspDump::DumpStoreNotifier::wait_response() {
  struct pollfd rsp_pol;
  int err;
  int cmd;

  while (true) {
    rsp_pol.fd = fd();
    rsp_pol.events = POLLIN;
    err = poll(&rsp_pol, 1, -1);
    if (err > 0 && (rsp_pol.revents & POLLIN)) {
      uint8_t cmd_buf;
      ssize_t n = ::read(fd(), &cmd_buf, 1);
      if (1 == n) {
        cmd = cmd_buf;
        break;
      }
    }
  }

  return cmd;
}
