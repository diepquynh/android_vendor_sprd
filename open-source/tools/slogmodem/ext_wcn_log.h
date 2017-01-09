/*
 *  ext_wcn_log.h - The external WCN log and dump handler class declaration.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-7-13 Zhang Ziyi
 *  Initial version.
 */
#ifndef EXT_WCN_LOG_H_
#define EXT_WCN_LOG_H_

#include "log_pipe_hdl.h"

class ExtWcnLogHandler : public LogPipeHandler {
 public:
  ExtWcnLogHandler(LogController* ctrl, Multiplexer* multi,
                   const LogConfig::ConfigEntry* conf,
                   StorageManager& stor_mgr);

 private:
  /*  start_dump - override the virtual function to implement
   *               external WCN dump.
   *  Return Value:
   *    Return 0 if the dump transaction is started successfully,
   *    return -1 if the dump transaction can not be started,
   *    return 1 if the dump transaction is finished.
   */
  int start_dump(const struct tm& lt);

  /*  diag_transaction_notify - Diagnosis port transaction result
   *                            notification function.
   *  @client: client parameter. It's the WanModemLogHandler* pointer.
   *  @result: the transaction result.
   *
   *  This function is called by current DataConsumer object.
   */
  static void diag_transaction_notify(void* client,
                                      DataConsumer::LogProcResult res);

  /*  start - override the virtual function for starting ext WCN log
   *
   *  Return Value:
   *    Return 0 if ext WCN log start successfully.
   */
  int start();

  /*
   *    stop - Stop logging.
   *
   *    This function put the LogPipeHandler in an adminitrative
   *    disable state and close the log device.
   *
   *    Return Value:
   *      0
   */
  int stop();
};

#endif  // !EXT_WCN_LOG_H_
