/*
 *  ext_wcn_dump.h - The external WCN dump class.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-6-13 Zhang Ziyi
 *  Initial version.
 */
#ifndef _EXT_WCN_DUMP_H_
#define _EXT_WCN_DUMP_H_

#include "cp_dump.h"

class LogFile;

class ExtWcnDumpConsumer : public CpDumpConsumer {
 public:
  ExtWcnDumpConsumer(const LogString& cp_name, CpStorage& cp_stor,
                     const struct tm& lt);

  ~ExtWcnDumpConsumer();

  int start();

  bool process(DeviceFileHandler::DataBuffer& buffer);

 private:
  /*  notify_ext_stor_umount - callback when external storage umounted.
  */
  static void notify_ext_stor_umount(void* client);

};

#endif  // !_EXT_WCN_DUMP_H_
