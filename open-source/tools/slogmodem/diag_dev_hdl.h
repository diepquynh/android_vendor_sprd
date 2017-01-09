/*
 *  diag_dev_hdl.h - The diagnosis device handler.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-6-13 Zhang Ziyi
 *  Initial version.
 */
#ifndef _DIAG_DEV_HDL_H_
#define _DIAG_DEV_HDL_H_

#include "dev_file_hdl.h"

class DataConsumer;

class DiagDeviceHandler : public DeviceFileHandler {
 public:
  DiagDeviceHandler(const LogString& file_path, DataConsumer* consumer,
                    LogController* ctrl, Multiplexer* multiplexer);
  DiagDeviceHandler(int fd, DataConsumer* consumer, LogController* ctrl,
                    Multiplexer* multiplexer);

 private:
  DataConsumer* m_consumer;

  bool process_data(DataBuffer& buffer);
};

#endif  // !_DIAG_DEV_HDL_H_
