/*
 *  pm_modem_dump.h - The 3G/4G MODEM and PM Sensorhub dump class.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-6-15 Zhang Ziyi
 *  Initial version.
 *  2016-7-13 Yan Zhihang
 *  Dump consumer to support both modem and pm sensorhub.
 */
#ifndef _PM_MODEM_DUMP_H_
#define _PM_MODEM_DUMP_H_

#include "cp_dump.h"
#include "diag_stream_parser.h"
#include "timer_mgr.h"

class LogFile;

class PmModemDumpConsumer : public CpDumpConsumer {
 public:
  PmModemDumpConsumer(const LogString& cp_name, CpStorage& cp_stor,
                      const struct tm& lt, const char* dump_path,
                      const char* end_of_content, uint8_t cmd);
  ~PmModemDumpConsumer();

  int start();

  bool process(DataBuffer& buffer);

 private:
  const char* m_dump_path;
  TimerManager::Timer* m_timer;
  DiagStreamParser m_parser;
  const char* m_end_of_content;
  size_t m_end_content_size;
  const uint8_t m_cmd;

  static void dump_read_timeout(void* param);

  /*  notify_ext_stor_umount - callback when external storage umounted.
  */
  static void notify_ext_stor_umount(void* client);

  bool check_ending(DataBuffer& buffer);

  /* save_dump_file - if Read dump from spipe failed, save /proc/cpxxx/mem
   */
  bool save_dump_file();
};

#endif  // !_PM_MODEM_DUMP_H_
