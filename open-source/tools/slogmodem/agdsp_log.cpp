/*
 *  agdsp_log.cpp - The AG-DSP log and dump handler class implementation.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-1-13 Zhang Ziyi
 *  Initial version.
 */
#include <poll.h>

#include "agdsp_log.h"
#include "audio_dsp_ioctl.h"
#include "cp_stor.h"
#include "multiplexer.h"

AgDspLogHandler::AgDspLogHandler(LogController* ctrl, Multiplexer* multi,
                                 const LogConfig::ConfigEntry* conf,
                                 StorageManager& stor_mgr)
  :LogPipeHandler{ctrl, multi, conf, stor_mgr},
   m_audio_notifier{ctrl, multi, "/dev/audio_dsp_pcm"},
   m_dump_notifier{ctrl, multi, "/dev/audio_dsp_mem"},
   m_sn{0} {
  m_audio_notifier.set_notify(notify_pcm, this);
  m_dump_notifier.set_notify(notify_dump, this);
}

int AgDspLogHandler::start_dump(const struct tm& /*lt*/) {
  // Not implemented yet.
  return -1;
}

int AgDspLogHandler::start() {
  int ret = start_logging();

  if (!ret) {
    if (m_audio_notifier.start()) {
      err_log("start PCM notifier error");
    }
    if (m_dump_notifier.start()) {
      err_log("start dump notifier error");
    }
  }

  return ret;
}

int AgDspLogHandler::stop() {
  int ret = stop_logging();

  if (!ret) {
    m_audio_notifier.stop();
    m_dump_notifier.stop();
  }

  return ret;
}

void AgDspLogHandler::process(int /*events*/) {
  // Process AG-DSP log
  ssize_t nr = read(fd(), log_buffer + 20, LOG_BUFFER_SIZE - 20);
  if (-1 == nr) {
    if (EAGAIN == errno || EINTR == errno) {
      return;
    }
    // Other errors: try to reopen the device
    del_events(POLLIN);
    close_devices();
    if (open() >= 0) {  // Success
      add_events(POLLIN);
    } else {  // Failure: arrange a check callback
      multiplexer()->timer_mgr().add_timer(3000, reopen_log_dev, this);
    }
    return;
  }

  // The driver shall not return 0 when there is no data under nonblocking
  // mode.
  if (!nr) {
    err_log("AG-DSP log device driver bug: read returns 0");
    return;
  }

  if (!storage()) {
    if (create_storage()) {
      return;
    }
  }

  // Add SMP and message header
  add_headers(log_buffer, nr);

  int err = storage()->write(log_buffer, nr + 20);
  if (err < 0) {
    err_log("%s save log error", ls2cstring(name()));
  }
}

void AgDspLogHandler::add_headers(uint8_t* buf, size_t len, DspLogType type) {
  // SMP header first

  // FLAGS
  buf[0] = 0x7e;
  buf[1] = 0x7e;
  buf[2] = 0x7e;
  buf[3] = 0x7e;
  // LEN (length excluding FLAGS in little Endian)
  size_t pkt_len = len + 8 + 8;
  buf[4] = static_cast<uint8_t>(pkt_len);
  buf[5] = static_cast<uint8_t>(pkt_len >> 8);
  // CHANNEL
  buf[6] = 0;
  // TYPE
  buf[7] = 0;
  // RESERVED
  buf[8] = 0x5a;
  buf[9] = 0x5a;
  // Checksum (only cover LEN, CHANNEL, TYPE and RESERVED)
  uint32_t n = static_cast<uint32_t>(pkt_len) + 0x5a5a;
  n = (n & 0xffff) + (n >> 16);
  n = ~n;
  buf[10] = static_cast<uint8_t>(n);
  buf[11] = static_cast<uint8_t>(n >> 8);

  // MSG_HEAD_T

  // SN
  buf[12] = static_cast<uint8_t>(m_sn);
  buf[13] = static_cast<uint8_t>(m_sn >> 8);
  buf[14] = static_cast<uint8_t>(m_sn >> 16);
  buf[15] = static_cast<uint8_t>(m_sn >> 24);
  ++m_sn;

  // length
  pkt_len = len + 8;
  buf[16] = static_cast<uint8_t>(pkt_len);
  buf[17] = static_cast<uint8_t>(pkt_len >> 8);
  // type
  buf[18] = 0x9d;
  // subtype: AG-DSP
  buf[19] = static_cast<uint8_t>(0x40 | type);
}

void AgDspLogHandler::notify_pcm(FileNotifier* fn, void* client) {
  AgDspLogHandler* agdsp = static_cast<AgDspLogHandler*>(client);
  agdsp->save_audio_log(fn, DLT_PCM);
}

void AgDspLogHandler::notify_dump(FileNotifier* fn, void* client) {
  AgDspLogHandler* agdsp = static_cast<AgDspLogHandler*>(client);
  info_log("AG-DSP %s assert", ls2cstring(agdsp->name()));
  agdsp->save_dump(fn);
}

void AgDspLogHandler::save_audio_log(FileNotifier* fn, DspLogType type) {
  // Read the file
  ssize_t nr = read(fn->fd(), log_buffer + 20, LOG_BUFFER_SIZE - 20);
  if (-1 == nr) {
    if (EAGAIN == errno || EINTR == errno) {
      return;
    }
    // Other errors: stop polling the file
    err_log("read PCM device error");
    fn->stop();
    return;
  }

  if (!storage()) {
    if (create_storage()) {
      err_log("create CpStorage error");
      return;
    }
  }

  // Add SMP and message header
  add_headers(log_buffer, nr, type);

  int err = storage()->write(log_buffer, nr + 20);
  if (err < 0) {
    err_log("%s save log error", ls2cstring(name()));
  }
}

void AgDspLogHandler::save_dump(FileNotifier* fn) {
  if (!storage()) {
    if (create_storage()) {
      ioctl(fn->fd(), DSPLOG_CMD_DSPASSERT, 0);
      err_log("create AG-DSP CpStorage error");
      return;
    }
  }

  // Read the dump device file
  while (true) {
    ssize_t nr = read(fn->fd(), log_buffer + 20, LOG_BUFFER_SIZE - 20);
    if (-1 == nr) {
      if (EINTR == errno) {
        continue;
      }
      // Other errors: stop polling the file
      err_log("read AG-DSP dump device error");
      break;
    }

    if (!nr) {
      // Dump finished
      break;
    }

    // Add SMP and message header
    add_headers(log_buffer, nr, DLT_DUMP);

    nr += 20;
    int err = storage()->write(log_buffer, nr);
    if (err != nr) {
      err_log("%s save AG-DSP dump error", ls2cstring(name()));
      break;
    }
  }

  ioctl(fn->fd(), DSPLOG_CMD_DSPASSERT, 0);
  storage()->flush();
}
