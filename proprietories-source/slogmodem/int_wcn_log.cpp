/*
 *  int_wcn_log.cpp - The internal WCN log and dump handler class
 *                    implementation declaration.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-8-7 Zhang Ziyi
 *  Initial version.
 */

#include "cp_dir.h"
#include "int_wcn_log.h"
#include "log_file.h"

IntWcnLogHandler::IntWcnLogHandler(LogController* ctrl, Multiplexer* multi,
                                   const LogConfig::ConfigEntry* conf,
                                   StorageManager& stor_mgr,
                                   const char* dump_path)
    : LogPipeHandler(ctrl, multi, conf, stor_mgr),
      m_dump_path(dump_path) {}

int IntWcnLogHandler::start_dump(const struct tm& lt) {
  // Save to dump file directly.
  int err;

  LogFile* mem_file = open_dump_mem_file(lt);
  if (mem_file) {
    err = mem_file->copy(m_dump_path);
    mem_file->close();
    if (err) {
      mem_file->dir()->remove(mem_file);
      err_log("save dump error");
    } else {
      // Dump has been saved.
      err = 1;
    }
  } else {
    err_log("create dump mem file failed");
    err = -1;
  }

  return err;
}

int IntWcnLogHandler::start() { return start_logging(); }

int IntWcnLogHandler::stop() { return stop_logging(); }
