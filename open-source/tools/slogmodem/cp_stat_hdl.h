/*
 *  cp_stat_hdl.h - The general CP state handler.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-4-8 Zhang Ziyi
 *  Initial version.
 */
#ifndef _CP_STAT_HDL_H_
#define _CP_STAT_HDL_H_

#include "cp_log_cmn.h"
#include "data_proc_hdl.h"

class CpStateHandler : public DataProcessHandler {
 public:
  CpStateHandler(LogController* ctrl, Multiplexer* multiplexer,
                 const char* serv_name);

  int init();

  enum CpEvent { CE_NONE, CE_ASSERT, CE_BLOCKED, CE_ALIVE };

 protected:
#define MODEM_STATE_BUF_SIZE 256

 private:
  LogString m_serv_name;

  int process_data();
  void process_conn_closed();
  void process_conn_error(int err);

  virtual CpEvent parse_notify(const uint8_t* buf, size_t len,
                               CpType& type) = 0;

  int mini_dump_response();

  static void connect_server(void* param);
};

#endif  // !_CP_STAT_HDL_H_
