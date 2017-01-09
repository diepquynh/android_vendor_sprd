/*
 *  data_proc_hdl.h - The socket data handler class implementation.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-2-16 Zhang Ziyi
 *  Initial version.
 */
#ifndef DATA_PROC_HDL_H_
#define DATA_PROC_HDL_H_

#include <cstddef>
#include <cstdint>
#include "fd_hdl.h"

class DataProcessHandler : public FdHandler {
 public:
  DataProcessHandler(int sock, LogController* ctrl, Multiplexer* multiplexer,
                     size_t buf_len);
  ~DataProcessHandler();

  void process(int events);

 protected:
  struct ConnectionBuffer {
    size_t buf_size;
    uint8_t* buffer;
    size_t data_start;
    size_t data_len;
  };

  ConnectionBuffer m_buffer;

 private:
  /*
   *    process_data - Process data in m_buffer.
   *
   *    Currently this function will always return 0.
   */
  virtual int process_data() = 0;
  virtual void process_conn_closed() = 0;
  virtual void process_conn_error(int err) = 0;
};

#endif  // !DATA_PROC_HDL_H_
