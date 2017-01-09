/*
 *  agdsp_pcm.cpp - The AG-DSP PCM controller.
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

#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "agdsp_pcm.h"
#include "audio_dsp_ioctl.h"
#include "multiplexer.h"

AgDspPcmController::AgDspPcmController(LogController* ctrl,
                                       Multiplexer* multiplexer,
                                       const char* file_path)
  :FdHandler{-1, ctrl, multiplexer},
   m_file_path{file_path},
   m_client{0},
   m_callback{0} {}

int AgDspPcmController::start() {
  int ret = 0;

  if (-1 == fd()) {
    m_fd = ::open(m_file_path, O_RDONLY);
    if (-1 == m_fd) {
      ret = -1;
    } else {
      multiplexer()->register_fd(this, POLLIN);
    }
  }

  return ret;
}

void AgDspPcmController::stop() {
  if (fd() >= 0) {
    multiplexer()->unregister_fd(this);
    close();
  }
}

void AgDspPcmController::process(int events) {
  if (m_callback) {
    m_callback(this, m_client);
  }
}

int AgDspPcmController::set_pcm_dump(bool enable) {
  bool close_fd = false;

  if (fd() < 0) {
    m_fd = open(m_file_path, O_RDWR);
    if (-1 == m_fd) {
      err_log("open AG-DSP PCM device error");
      return -1;
    }
    close_fd = true;
  }

  int err = control_pcm_dump(enable);

  if (err) {
    err_log("set AG-DSP PCM output error");
  }

  if (close_fd) {
    ::close(m_fd);
    m_fd = -1;
  }

  return err;
}

int AgDspPcmController::control_pcm_dump(bool enable) {
  int en = enable ? 1 : 0;
  return ioctl(fd(), DSPLOG_CMD_PCM_ENABLE, reinterpret_cast<void*>(en));
}
