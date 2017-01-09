/*
 *  ext_wcn_dump.cpp - The external WCN dump class.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-6-15 Zhang Ziyi
 *  Initial version.
 */
#include "cp_stor.h"
#include "diag_dev_hdl.h"
#include "ext_wcn_dump.h"
#include "multiplexer.h"
#include "parse_utils.h"

ExtWcnDumpConsumer::ExtWcnDumpConsumer(const LogString& cp_name,
                                       CpStorage& cp_stor, const struct tm& lt)
    : CpDumpConsumer(cp_name, cp_stor, lt) {}

ExtWcnDumpConsumer::~ExtWcnDumpConsumer() {
  storage().unsubscribe_ext_stor_umount_evt(this);
}

int ExtWcnDumpConsumer::start() {
  if (!open_dump_file()) {
    storage().subscribe_ext_stor_umount_evt(this, notify_ext_stor_umount);
    err_log("open dump file failed!");
    return -1;
  }

  return 0;
}

bool ExtWcnDumpConsumer::process(DeviceFileHandler::DataBuffer& buffer) {
  bool ret = false;
  const uint8_t* p = buffer.buffer + buffer.data_start;
  if (buffer.data_len < 4096 &&
      find_str(p, buffer.data_len,
               reinterpret_cast<const uint8_t*>("marlin_memdump_finish"), 21)) {
    diag_dev()->del_events(POLLIN);
    buffer.data_start = 0;
    buffer.data_len = 0;
    close_dump_file();
    notify_client(LPR_SUCCESS);
    ret = true;
    return ret;
  }

  LogFile* f = dump_file();
  size_t len = buffer.data_len;
  ssize_t n = f->write(buffer.buffer + buffer.data_start, len);
  buffer.data_start = 0;
  buffer.data_len = 0;
  if (static_cast<size_t>(n) != len) {
    diag_dev()->del_events(POLLIN);
    remove_dump_file();
    notify_client(LPR_FAILURE);
    ret = true;
  }

  return ret;
}

void ExtWcnDumpConsumer::notify_ext_stor_umount(void *client) {
  ExtWcnDumpConsumer* consumer = static_cast<ExtWcnDumpConsumer*>(client);
  consumer->notify_client(LPR_FAILURE);
}
