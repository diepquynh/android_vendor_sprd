/*
 *  int_wcn_stat_hdl.h - The internal WCN status handler declaration.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-4-8 Zhang Ziyi
 *  Initial version.
 */
#ifndef _INT_WCN_STAT_HDL_H_
#define _INT_WCN_STAT_HDL_H_

#include "cp_stat_hdl.h"

class IntWcnStateHandler : public CpStateHandler {
 public:
  IntWcnStateHandler(LogController* ctrl, Multiplexer* multiplexer,
                     const char* serv_name);

 private:
  CpEvent parse_notify(const uint8_t* buf, size_t len, CpType& type);
};

#endif  // !_INT_WCN_STAT_HDL_H_
