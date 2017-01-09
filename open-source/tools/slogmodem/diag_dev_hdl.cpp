/*
 *  diag_dev_hdl.cpp - The diagnosis device handler.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-6-13 Zhang Ziyi
 *  Initial version.
 */
#include "data_consumer.h"
#include "diag_dev_hdl.h"

DiagDeviceHandler::DiagDeviceHandler(const LogString& file_path,
                                     DataConsumer* consumer,
                                     LogController* ctrl,
                                     Multiplexer* multiplexer)
    : DeviceFileHandler(file_path, 1024 * 32, ctrl, multiplexer),
      m_consumer{consumer} {}

DiagDeviceHandler::DiagDeviceHandler(int fd, DataConsumer* consumer,
                                     LogController* ctrl,
                                     Multiplexer* multiplexer)
    : DeviceFileHandler(fd, 1024 * 32, ctrl, multiplexer),
      m_consumer{consumer} {}

bool DiagDeviceHandler::process_data(DeviceFileHandler::DataBuffer& buffer) {
  bool ret = true;
  if (m_consumer) {
    ret = m_consumer->process(buffer);
  } else {
    err_log("No data consumer for %s", ls2cstring(path()));
  }
  return ret;
}
