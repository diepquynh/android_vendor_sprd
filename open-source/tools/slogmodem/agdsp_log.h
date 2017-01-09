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

#include "file_notifier.h"
#include "log_pipe_hdl.h"

class AgDspLogHandler : public LogPipeHandler {
 public:
  AgDspLogHandler(LogController* ctrl,
                  Multiplexer* multi,
                  const LogConfig::ConfigEntry* conf,
                  StorageManager& stor_mgr);

  /*  process - process the AG-DSP log data.
   *  @events: events on the log file descriptor.
   */
  void process(int events);

 private:
  enum DspLogType { DLT_TP, DLT_DUMP = 2, DLT_PCM };

  FileNotifier m_audio_notifier;
  FileNotifier m_dump_notifier;
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

  void save_audio_log(FileNotifier* fn, DspLogType type);
  void save_dump(FileNotifier* fn);
  void add_headers(uint8_t* buf, size_t len, DspLogType type = DLT_TP);

  /*  notify_xxx - PCM and dump file data notification.
   *  @fn: FileNotifier pointer
   *  @client: client pointer (pointer to AgDspLogHandler object)
   */
  static void notify_pcm(FileNotifier* fn, void* client);
  static void notify_dump(FileNotifier* fn, void* client);
};

#endif  // !AGDSP_LOG_H_
