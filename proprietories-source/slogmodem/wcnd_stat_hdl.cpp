/*
 *  wcnd_stat_hdl.cpp - The internal WCN status handler implementation.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-4-8 Zhang Ziyi
 *  Initial version.
 */
#include <sys/types.h>
#include <sys/socket.h>
#ifdef HOST_TEST_
#include "sock_test.h"
#else
#include "cutils/sockets.h"
#endif

#include "wcnd_stat_hdl.h"
#include "multiplexer.h"
#include "log_ctrl.h"
#include "parse_utils.h"

WcndStateHandler::WcndStateHandler(LogController* ctrl,
                                   Multiplexer* multiplexer,
                                   const char* serv_name,
                                   const char* assert_msg)
    : CpStateHandler(ctrl, multiplexer, serv_name),
      m_assert_msg{assert_msg} {}

CpStateHandler::CpEvent WcndStateHandler::parse_notify(const uint8_t* buf,
                                                       size_t len,
                                                       CpType& type) {

  const uint8_t* p =
      find_str(buf, len, reinterpret_cast<const uint8_t*>("WCN-CP2-ALIVE"), 13);
  if (p) {
    type = CT_WCN;
    return CE_ALIVE;
  }

  if (!str_empty(m_assert_msg)) {
    p = find_str(buf, len,
                 reinterpret_cast<const uint8_t*>(
                     const_cast<char*>(ls2cstring(m_assert_msg))),
                 m_assert_msg.length());
  }
  if (p) {
    type = CT_WCN;
    return CE_ASSERT;
  }

  return CE_NONE;
}
