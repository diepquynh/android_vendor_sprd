/*
 *  cp_sleep_log.h - The CP sleep log class.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-6-13 Zhang Ziyi
 *  Initial version.
 */
#ifndef _CP_SLEEP_LOG_H_
#define _CP_SLEEP_LOG_H_

#include "cp_log_cmn.h"
#include "data_consumer.h"
#include "diag_stream_parser.h"
#include "timer_mgr.h"

class LogFile;

class CpSleepLogConsumer : public DataConsumer {
 public:
  CpSleepLogConsumer(const LogString& cp_name, CpStorage& cp_stor);
  ~CpSleepLogConsumer();

  bool process(DeviceFileHandler::DataBuffer& buffer);

  int start();

 private:
  DiagStreamParser parser;

  TimerManager::Timer* m_timer;

  LogFile* m_file;

  LogFile* open_file();
  void close_file();
  void close_rm_file();

  static void sleep_log_read_timeout(void* param);
};

#endif  // !_CP_SLEEP_LOG_H_
