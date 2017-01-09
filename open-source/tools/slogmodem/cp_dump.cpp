/*
 *  cp_dump.cpp - The CP dump class.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-6-13 Zhang Ziyi
 *  Initial version.
 */
#include <poll.h>
#include "cp_dir.h"
#include "cp_dump.h"
#include "cp_stor.h"
#include "log_file.h"

CpDumpConsumer::CpDumpConsumer(const LogString& cp_name, CpStorage& cp_stor,
                               const struct tm& lt)
    : DataConsumer(cp_name, cp_stor), m_dump_file{0} {
  m_time = lt;
}

CpDumpConsumer::~CpDumpConsumer() {
  if (m_dump_file) {
    m_dump_file->close();
  }
}

LogFile* CpDumpConsumer::open_dump_file() {
  char log_name[64];

  snprintf(log_name, sizeof log_name,
           "_memory_%04d-%02d-%02d_%02d-%02d-%02d.dmp", m_time.tm_year + 1900,
           m_time.tm_mon + 1, m_time.tm_mday, m_time.tm_hour, m_time.tm_min,
           m_time.tm_sec);
  LogString dump_file_name = cp_name() + log_name;
  m_dump_file = storage().create_file(dump_file_name, LogFile::LT_DUMP);
  if (!m_dump_file) {
    err_log("open dump file %s failed", ls2cstring(dump_file_name));
  }
  return m_dump_file;
}

LogFile* CpDumpConsumer::open_dump_mem_file() {
  char log_name[64];

  snprintf(log_name, sizeof log_name,
           "_memory_%04d-%02d-%02d_%02d-%02d-%02d.mem", m_time.tm_year + 1900,
           m_time.tm_mon + 1, m_time.tm_mday, m_time.tm_hour, m_time.tm_min,
           m_time.tm_sec);
  LogString mem_file_name = cp_name() + log_name;
  LogFile* f = storage().create_file(mem_file_name, LogFile::LT_DUMP);
  if (!f) {
    err_log("open dump mem file %s failed", ls2cstring(mem_file_name));
  }
  return f;
}

void CpDumpConsumer::remove_dump_file() {
  m_dump_file->close();
  m_dump_file->dir()->remove(m_dump_file);
  m_dump_file = 0;
}
