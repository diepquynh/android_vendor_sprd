/*
 *  dev_file_hdl.h - The device file handler.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-6-13 Zhang Ziyi
 *  Initial version.
 */
#ifndef _DEV_FILE_HDL_H_
#define _DEV_FILE_HDL_H_

#include "cp_log_cmn.h"
#include "fd_hdl.h"

class DeviceFileHandler : public FdHandler {
 public:
  struct DataBuffer {
    uint8_t* buffer;
    size_t buf_size;
    size_t data_start;
    size_t data_len;
  };

  DeviceFileHandler(const LogString& dev_path, size_t buf_len,
                    LogController* ctrl, Multiplexer* multiplexer);
  DeviceFileHandler(int fd, size_t buf_len, LogController* ctrl,
                    Multiplexer* multiplexer);
  ~DeviceFileHandler();

  const LogString& path() const { return m_file_path; }

  int open();
  int close();

 private:
  // Close the file descriptor on destruction
  bool m_close_fd;
  // Full path of the device file
  LogString m_file_path;
  // Data buffer
  DataBuffer m_buffer;

  // Events handler
  void process(int events);
  virtual bool process_data(DataBuffer& buffer) = 0;
};

#endif  // !_DEV_FILE_HDL_H_
