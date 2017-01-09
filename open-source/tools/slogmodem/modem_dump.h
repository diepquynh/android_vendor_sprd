/*
 *  modem_dump.h - The 3G/4G MODEM dump class.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-6-15 Zhang Ziyi
 *  Initial version.
 */
#ifndef _MODEM_DUMP_H_
#define _MODEM_DUMP_H_

#include "cp_dump.h"
#include "diag_stream_parser.h"
#include "timer_mgr.h"

class LogFile;

class ModemDumpConsumer : public CpDumpConsumer {
 public:
  ModemDumpConsumer(const LogString& cp_name, CpStorage& cp_stor,
                    const struct tm& lt, const char* dump_path);
  ~ModemDumpConsumer();

  int start();

  bool process(DeviceFileHandler::DataBuffer& buffer);

 private:
  const char* m_dump_path;
  TimerManager::Timer* m_timer;
  DiagStreamParser m_parser;

  static void dump_read_timeout(void* param);

  /*  notify_ext_stor_umount - callback when external storage umounted.
  */
  static void notify_ext_stor_umount(void* client);

  bool check_ending(DeviceFileHandler::DataBuffer& buffer);

  /* save_dump_file - if Read dump from spipe failed, save /proc/cpxxx/mem
   */
  bool save_dump_file();
};

#endif  // !_MODEM_DUMP_H_
