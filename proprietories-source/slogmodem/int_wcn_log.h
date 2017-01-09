/*
 *  int_wcn_log.h - The internal WCN log and dump handler class declaration.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-8-7 Zhang Ziyi
 *  Initial version.
 */
#ifndef INT_WCN_LOG_H_
#define INT_WCN_LOG_H_

#include "log_pipe_hdl.h"

class IntWcnLogHandler : public LogPipeHandler {
 public:
  IntWcnLogHandler(LogController* ctrl, Multiplexer* multi,
                   const LogConfig::ConfigEntry* conf,
                   StorageManager& stor_mgr, const char* dump_path);

 private:
  const char* m_dump_path;

  /*  start_dump - override the virtual function to implement
   *               internal WCN dump.
   *  Return Value:
   *    return -1 if the dump can not be saved,
   *    return 1 if the dump is saved.
   */
  int start_dump(const struct tm& lt);

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

#endif  // !INT_WCN_LOG_H_
