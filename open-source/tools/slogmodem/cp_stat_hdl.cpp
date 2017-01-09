/*
 *  cp_stat_hdl.cpp - The general CP status handler implementation.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-4-8 Zhang Ziyi
 *  Initial version.
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#ifdef HOST_TEST_
#include "sock_test.h"
#else
#include "cutils/sockets.h"
#endif

#include "cp_stat_hdl.h"
#include "multiplexer.h"
#include "log_ctrl.h"
#include "parse_utils.h"

CpStateHandler::CpStateHandler(LogController* ctrl, Multiplexer* multiplexer,
                               const char* serv_name)
    : DataProcessHandler(-1, ctrl, multiplexer, MODEM_STATE_BUF_SIZE),
      m_serv_name(serv_name) {}

int CpStateHandler::init() {
  m_fd = socket_local_client(ls2cstring(m_serv_name),
                             ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
  if (-1 == m_fd) {
    multiplexer()->timer_mgr().add_timer(3000, connect_server, this);
  } else {
    add_events(POLLIN);
  }

  return 0;
}

void CpStateHandler::connect_server(void* param) {
  CpStateHandler* p = static_cast<CpStateHandler*>(param);

  p->init();
}

int CpStateHandler::process_data() {
  LogString s;

  str_assign(s, reinterpret_cast<const char*>(m_buffer.buffer),
             m_buffer.data_len);
  info_log("%u bytes: %s", static_cast<unsigned>(m_buffer.data_len),
           ls2cstring(s));

  CpType type;
  CpEvent evt = parse_notify(m_buffer.buffer, m_buffer.data_len, type);

  if (evt != CE_NONE) {
    controller()->process_cp_evt(type, evt);
  }

  m_buffer.data_len = 0;

  return 0;
}

void CpStateHandler::process_conn_closed() {
  // Parse the request and inform the LogController to execute it.
  del_events(POLLIN);
  m_fd = -1;

  init();
}

void CpStateHandler::process_conn_error(int /*err*/) {
  CpStateHandler::process_conn_closed();
}

int CpStateHandler::mini_dump_response() {
  ssize_t n = write(m_fd, "MINIDUMP COMPLETE", 17);

  return 17 == n ? 0 : -1;
}
