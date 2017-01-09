/*
 *  wcnd_stat_hdl.h - The internal WCN status handler declaration.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-4-8 Zhang Ziyi
 *  Initial version.
 */
#ifndef _WCND_STAT_HDL_H_
#define _WCND_STAT_HDL_H_

#include "cp_stat_hdl.h"

class WcndStateHandler : public CpStateHandler {
 public:
  WcndStateHandler(LogController* ctrl, Multiplexer* multiplexer,
                   const char* serv_name, const char* assert_notification);

 private:
  // the notification string of wcn assertion
  LogString m_assert_msg;

  CpEvent parse_notify(const uint8_t* buf, size_t len, CpType& type);
};

#endif  // !_INT_WCN_STAT_HDL_H_
