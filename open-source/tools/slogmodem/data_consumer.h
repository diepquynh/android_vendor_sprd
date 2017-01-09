/*
 *  data_consumer.h - The data consumer base class.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-6-13 Zhang Ziyi
 *  Initial version.
 */
#ifndef _DATA_CONSUMER_H_
#define _DATA_CONSUMER_H_

#include "dev_file_hdl.h"

class CpStorage;
class DiagDeviceHandler;

class DataConsumer {
 public:
  enum LogProcResult {
    LPR_SUCCESS,
    LPR_FAILURE,
    LPR_SLEEPLOG_NO_SUPPORTED,
    LPR_RINGBUF_NO_SUPPORTED
  };

  typedef void (*log_proc_callback_t)(void* client, LogProcResult result);

  DataConsumer(const LogString& cp_name, CpStorage& cp_stor);
  virtual ~DataConsumer() {}

  const LogString& cp_name() const { return m_cp_name; }

  void set_callback(void* client, log_proc_callback_t cb) {
    m_client = client;
    m_callback = cb;
  }

  void bind(DiagDeviceHandler* diag) { m_diag_dev = diag; }

  virtual int start() = 0;
  virtual bool process(DeviceFileHandler::DataBuffer& buffer) = 0;

 protected:
  DiagDeviceHandler* diag_dev() const { return m_diag_dev; }
  void notify_client(LogProcResult result) const {
    if (m_callback) {
      m_callback(m_client, result);
    }
  }
  CpStorage& storage() { return m_stor; }
  void* get_client() { return m_client; }

 private:
  // CP name
  const LogString& m_cp_name;
  // Diagnosis device handler
  DiagDeviceHandler* m_diag_dev;
  // CP storage
  CpStorage& m_stor;
  // Client parameter
  void* m_client;
  // Callback to notify the client
  log_proc_callback_t m_callback;
};

#endif  // !_DATA_CONSUMER_H_
