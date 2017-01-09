/*
 *  cp_ringbuf.h - The CP RingBuf class.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-6-13 Zhang Ziyi
 *  Initial version.
 */
#ifndef _CP_RINGBUF_H_
#define _CP_RINGBUF_H_

#include "cp_log_cmn.h"
#include "data_consumer.h"
#include "diag_stream_parser.h"
#include "timer_mgr.h"

class LogFile;

class CpRingBufConsumer : public DataConsumer {
 public:
  CpRingBufConsumer(const LogString& cp_name, CpStorage& cp_stor);
  ~CpRingBufConsumer();

  bool process(DeviceFileHandler::DataBuffer& buffer);

  int start();

 private:
  DiagStreamParser parser;

  TimerManager::Timer* m_timer;

  LogFile* m_file;

  LogFile* open_file();
  void close_file();
  void close_rm_file();

  static void ring_buffer_read_timeout(void* param);
};

#endif  // !_CP_RINGBUF_H_
