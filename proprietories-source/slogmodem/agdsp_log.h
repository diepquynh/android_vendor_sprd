/*
 *  agdsp_log.h - The AG-DSP log and dump handler class declaration.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-1-13 Zhang Ziyi
 *  Initial version.
 */
#ifndef AGDSP_LOG_H_
#define AGDSP_LOG_H_

#include "agdsp_dump.h"
#include "agdsp_pcm.h"
#include "log_config.h"
#include "log_pipe_hdl.h"

class AgDspLogHandler : public LogPipeHandler {
 public:
  AgDspLogHandler(LogController* ctrl,
                  Multiplexer* multi,
                  const LogConfig::ConfigEntry* conf,
                  StorageManager& stor_mgr);

  ~AgDspLogHandler();

  /*  process - process the AG-DSP log data.
   *  @events: events on the log file descriptor.
   */
  void process(int events);
  int pause();

  /*  init_log_output - init the AG-DSP log output.
   *
   *  This function is only called when the program initializes.
   *
   *  Return Value:
   *    Return 0 on success, -1 on failure.
   */
  int init_log_output(LogConfig::AgDspLogDestination log_dest,
                      bool pcm_dump);
  int set_log_dest(LogConfig::AgDspLogDestination log_dest);
  int set_pcm_dump(bool enable);

 private:
  enum DspLogType { DLT_TP, DLT_MEMORY, DLT_DUMP, DLT_AUDIO };

  LogConfig::AgDspLogDestination m_log_dest;
  bool m_pcm_dump;
  AgDspPcmController m_pcm_controller;
  AgdspDump m_dump;
  uint32_t m_sn;

  /*  start_dump - override the virtual function to implement
   *               AG-DSP dump.
   *  Return Value:
   *    Return 0 if the dump transaction is started successfully,
   *    return -1 if the dump transaction can not be started,
   *    return 1 if the dump transaction is finished.
   */
  int start_dump(const struct tm& lt);

  /*  start - override the virtual function for starting AG-DSP log
   *
   *  Return Value:
   *    Return 0 if log starts successfully.
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

  void save_log(int fd, DspLogType type = DLT_TP);
  void add_headers(uint8_t* buf, size_t len, DspLogType type);

  int control_log_dest();

  /*  notify_xxx - PCM and dump file data notification.
   *  @fn: AgDspPcmController pointer
   *  @client: client pointer (pointer to AgDspLogHandler object)
   */
  static void notify_pcm(AgDspPcmController* fn, void* client);

  static void dump_callback(void* hdl);
  static void dump_result_notifiy(void* hdl, AgdspDump::Dump_State ds);
};

#endif  // !AGDSP_LOG_H_
