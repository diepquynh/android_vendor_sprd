/*
 *  wcdma_iq_notifier.cpp - The WCDMA I/Q data saving thread notifier.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-7-24 Zhang Ziyi
 *  Initial version.
 */

#include <poll.h>
#include "wcdma_iq_mgr.h"

WcdmaIqManager::SaveIqNotifier::SaveIqNotifier(int fd, WcdmaIqManager* wiq)
    : FdHandler(fd, wiq->controller(), wiq->multiplexer()), m_wcdma_iq{wiq} {}

void WcdmaIqManager::SaveIqNotifier::process(int /*events*/) {
  while (true) {
    uint8_t cmd;

    ssize_t ret = ::read(fd(), &cmd, 1);
    if (ret <= 0) {
      break;
    }

    m_wcdma_iq->process_stor_response(cmd);
  }
}

int WcdmaIqManager::SaveIqNotifier::send_command(int cmd) {
  uint8_t buf = static_cast<uint8_t>(cmd);
  ssize_t n = ::write(fd(), &buf, 1);
  return n <= 0 ? -1 : 0;
}

int WcdmaIqManager::SaveIqNotifier::wait_response() {
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
