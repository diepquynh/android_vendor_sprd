/*
 *  modem_stat_hdl.cpp - The MODEM status handler implementation.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-2-16 Zhang Ziyi
 *  Initial version.
 */
#include <sys/types.h>
#include <sys/socket.h>
#ifdef HOST_TEST_
#include "sock_test.h"
#else
#include "cutils/sockets.h"
#endif

#include "modem_stat_hdl.h"
#include "multiplexer.h"
#include "log_ctrl.h"
#include "parse_utils.h"

ModemStateHandler::ModemStateHandler(LogController* ctrl,
                                     Multiplexer* multiplexer,
                                     const char* serv_name)
    : CpStateHandler(ctrl, multiplexer, serv_name) {}

CpStateHandler::CpEvent ModemStateHandler::parse_notify(const uint8_t* buf,
                                                        size_t len,
                                                        CpType& type) {
  type = CT_WANMODEM;
  const uint8_t* p = find_str(
      buf, len, reinterpret_cast<const uint8_t*>("TD Modem Assert"), 15);
  if (p) {
    return CE_ASSERT;
  }

  p = find_str(buf, len, reinterpret_cast<const uint8_t*>("Modem Assert"), 12);
  if (p) {
    return CE_ASSERT;
  }

  p = find_str(buf, len, reinterpret_cast<const uint8_t*>("Modem Alive"), 11);
  if (p) {
    return CE_ALIVE;
  }

  p = find_str(buf, len, reinterpret_cast<const uint8_t*>("Modem Blocked"), 13);
  if (p) {
    return CE_BLOCKED;
  }

  return CE_NONE;
}
