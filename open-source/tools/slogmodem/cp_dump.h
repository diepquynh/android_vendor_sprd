/*
 *  cp_dump.h - The CP dump class.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-6-13 Zhang Ziyi
 *  Initial version.
 */
#ifndef _CP_DUMP_H_
#define _CP_DUMP_H_

#include "data_consumer.h"
#include "log_file.h"

class LogFile;

class CpDumpConsumer : public DataConsumer {
 public:
  CpDumpConsumer(const LogString& cp_name, CpStorage& cp_stor,
                 const struct tm& lt);
  ~CpDumpConsumer();

  const struct tm& time() const { return m_time; }

 protected:
  LogFile* open_dump_file();
  LogFile* open_dump_mem_file();
  LogFile* dump_file() { return m_dump_file; }
  void close_dump_file() {
    m_dump_file->close();
    m_dump_file = 0;
  }
  void remove_dump_file();

 private:
  struct tm m_time;
  LogFile* m_dump_file;
};

#endif  // !_CP_DUMP_H_
