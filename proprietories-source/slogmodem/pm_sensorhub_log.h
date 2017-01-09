/*
 *  pm_sensorhub_log.h - The power management, sensorhub log
 *                       and dump handler class declaration.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-3-2 YAN Zhihang
 *  Initial version.
 */
#ifndef PM_SENSORHUB_LOG_H_
#define PM_SENSORHUB_LOG_H_

#include "log_pipe_hdl.h"

class PmSensorhubLogHandler : public LogPipeHandler {
 public:
  PmSensorhubLogHandler(LogController* ctrl, Multiplexer* multi,
                       const LogConfig::ConfigEntry* conf,
                       StorageManager& stor_mgr);

 private:
  /*  start_dump - override the virtual function to implement
   *               power management and sensorhub module dump.
   *  Return Value:
   *    Return 0 if the dump transaction is started successfully,
   *    return -1 if the dump transaction can not be started,
   *    return 1 if the dump transaction is finished.
   */
  int start_dump(const struct tm& lt);

  /*  diag_transaction_notify - Diagnosis port transaction result
   *                            notification function.
   *  @client: client parameter. It's the PmSensorhubLogHandler* pointer.
   *  @result: the transaction result.
   *
   *  This function is called by current DataConsumer object.
   */
  static void diag_transaction_notify(void* client,
                                      DataConsumer::LogProcResult res);

  /*  start - override the virtual function for starting power
   *          management and sensorhub log.
   *
   *  Return Value:
   *    Return 0 if pm and sensorhub log start successfully,
   */
  int start();

  /*
   *  stop - Stop logging.
   *
   *  This function put the LogPipeHandler in an adminitrative
   *  disable state and close the log device.
   *
   *  Return Value:
   *    0
   */
  int stop();
};

#endif  // !PM_SENSORHUB_LOG_H_
