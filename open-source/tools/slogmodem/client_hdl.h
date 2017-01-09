/*
 *  client_hdl.h - The base class declaration for file descriptor
 *                 handler.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-2-16 Zhang Ziyi
 *  Initial version.
 *
 *  2015-6-5 Zhang Ziyi
 *  CP dump notification added.
 */
#ifndef CLIENT_HDL_H_
#define CLIENT_HDL_H_

#include "client_req.h"
#include "cp_log_cmn.h"
#include "data_proc_hdl.h"

class ClientManager;
class LogPipeHandler;

class ClientHandler : public DataProcessHandler {
 public:
  enum CpEvent { CE_DUMP_START, CE_DUMP_END };

  enum ClientTransType { CTT_UNKNOWN, CTT_SAVE_SLEEP_LOG, CTT_SAVE_RINGBUF };

  enum ClientTransState { CTS_IDLE, CTS_EXECUTING };

  ClientHandler(int sock, LogController* ctrl, Multiplexer* multiplexer,
                ClientManager* mgr);
  ~ClientHandler();

  void notify_cp_dump(CpType cpt, CpEvent evt);

  /*  notify_trans_result - transaction result notification.
   *  @result: transaction result. Possible values are LCR_XXX
   *           macros defined in req_err.h.
   */
  void notify_trans_result(int result);

 private:
#define CLIENT_BUF_SIZE 256

  ClientManager* m_mgr;
  // Client transactin type
  ClientTransType m_trans_type;
  // Client transaction state
  ClientTransState m_state;
  // LogPipeHandler object for current transaction
  LogPipeHandler* m_cp;
  bool m_cp_dump_notify[CT_NUMBER];

  int process_data();
  void process_conn_closed();
  void process_conn_error(int err);

  void process_req(const uint8_t* req, size_t len);
  void proc_slogctl(const uint8_t* req, size_t len);
  void proc_enable_log(const uint8_t* req, size_t len);
  void proc_disable_log(const uint8_t* req, size_t len);
  void proc_get_log_state(const uint8_t* req, size_t len);
  void proc_enable_md(const uint8_t* req, size_t len);
  void proc_disable_md(const uint8_t* req, size_t len);
  void proc_mini_dump(const uint8_t* req, size_t len);
  void proc_get_log_file_size(const uint8_t* req, size_t len);
  void proc_set_log_file_size(const uint8_t* req, size_t len);
  void proc_enable_overwrite(const uint8_t* req, size_t len);
  void proc_disable_overwrite(const uint8_t* req, size_t len);
  void proc_get_data_part_size(const uint8_t* req, size_t len);
  void proc_set_data_part_size(const uint8_t* req, size_t len);
  void proc_get_md_pos(const uint8_t* req, size_t len);
  void proc_set_md_pos(const uint8_t* req, size_t len);
  void proc_get_sd_size(const uint8_t* req, size_t len);
  void proc_set_sd_size(const uint8_t* req, size_t len);
  void proc_get_log_overwrite(const uint8_t* req, size_t len);
  void proc_subscribe(const uint8_t* req, size_t len);
  void proc_unsubscribe(const uint8_t* req, size_t len);
  void proc_sleep_log(const uint8_t* req, size_t len);
  void proc_ringbuf(const uint8_t* req, size_t len);
  void proc_enable_iq(const uint8_t* req, size_t len);
  void proc_disable_iq(const uint8_t* req, size_t len);
  void proc_flush(const uint8_t* req, size_t len);

  void proc_sleep_log_result(int result);
  void proc_ringbuf_result(int result);

  static const uint8_t* search_end(const uint8_t* req, size_t len);
  static int send_dump_notify(int fd, CpType cpt, CpEvent evt);
  static ResponseErrorCode trans_result_to_req_result(int result);
};

#endif  // !CLIENT_HDL_H_
