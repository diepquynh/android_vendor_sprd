/*
 *  agdsp_pcm.h - The AG-DSP PCM controller.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-2-20 Zhang Ziyi
 *  Initial version.
 *
 *  2016-7-6 Zhang Ziyi
 *  FileNotifier renamed to AgDspPcmController
 */
#ifndef AGDSP_PCM_H_
#define AGDSP_PCM_H_

#include <cstddef>
#include <cstdint>
#include "fd_hdl.h"

class AgDspPcmController : public FdHandler {
 public:
  typedef void (*file_data_notify_t)(AgDspPcmController* notifier,
                                     void* client);

  AgDspPcmController(LogController* ctrl, Multiplexer* multiplexer,
                     const char* file_path);

  void set_notify(file_data_notify_t cb, void* client) {
    m_callback = cb;
    m_client = client;
  }

  bool started() const { return fd() >= 0; }
  int start();
  void stop();

  int set_pcm_dump(bool enable);

  void process(int events);

 private:
  const char* m_file_path;
  void* m_client;
  file_data_notify_t m_callback;

  int control_pcm_dump(bool enable);
};

#endif  // !AGDSP_PCM_H_
