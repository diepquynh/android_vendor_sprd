/*
 *  ext_wcn_stat_hdl.cpp - The external WCN status handler implementation.
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

#include "ext_wcn_stat_hdl.h"
#include "multiplexer.h"
#include "log_ctrl.h"
#include "parse_utils.h"

ExtWcnStateHandler::ExtWcnStateHandler(LogController* ctrl,
                                       Multiplexer* multiplexer,
                                       const char* serv_name)
    : CpStateHandler(ctrl, multiplexer, serv_name) {}

CpStateHandler::CpEvent ExtWcnStateHandler::parse_notify(const uint8_t* buf,
                                                         size_t len,
                                                         CpType& type) {
  const uint8_t* p = find_str(
      buf, len, reinterpret_cast<const uint8_t*>("WCN-EXTERNAL-ALIVE"), 18);
  if (p) {
    type = CT_WCN;
    return CE_ALIVE;
  }

  p = find_str(buf, len, reinterpret_cast<const uint8_t*>("WCN-EXTERNAL-DUMP"),
               17);
  if (p) {
    type = CT_WCN;
    return CE_ASSERT;
  }

  return CE_NONE;
}
