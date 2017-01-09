/*
 *  trans_modem_ver.h - The MODEM software version query transaction.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-6-8 Zhang Ziyi
 *  Initial version.
 */
#ifndef _TRANS_MODEM_VER_H_
#define _TRANS_MODEM_VER_H_

#include "cp_log_cmn.h"
#include "data_consumer.h"
#include "diag_stream_parser.h"
#include "timer_mgr.h"

class DiagDeviceHandler;

class WanModemVerQuery : public DataConsumer {
 public:
  enum QueryState {
    QS_NOT_BEGIN,
    QS_WAIT_RESULT,
    QS_FINISHED
  };

  WanModemVerQuery(const LogString& cp_name,
                   CpStorage& cp_stor);
  ~WanModemVerQuery();

  bool process(DataBuffer& buffer);

  int start();
  void stop();

  const char* version(size_t& len) const {
    len = m_ver_len;
    return m_modem_ver;
  }

 private:
  DiagStreamParser m_parser;
  // Max version query times
  int m_max_times;
  // Failed times
  int m_failed_times;
  // Query state
  QueryState m_state;
  // Version query timeout
  TimerManager::Timer* m_timer;
  // MODEM version
  size_t m_ver_len;
  char* m_modem_ver;

  int start_query();
  int send_query();

  static void ver_query_timeout(void* param);
};

#endif  // !_TRANS_MODEM_VER_H_
