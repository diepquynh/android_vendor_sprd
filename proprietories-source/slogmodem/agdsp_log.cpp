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
#include <sys/ioctl.h>
#include <unistd.h>

#include "agdsp_log.h"
#include "audio_dsp_ioctl.h"
#include "client_mgr.h"
#include "cp_stor.h"
#include "log_ctrl.h"
#include "multiplexer.h"

AgDspLogHandler::AgDspLogHandler(LogController* ctrl, Multiplexer* multi,
                                 const LogConfig::ConfigEntry* conf,
                                 StorageManager& stor_mgr)
  :LogPipeHandler{ctrl, multi, conf, stor_mgr},
   m_log_dest{LogConfig::AGDSP_LOG_DEST_OFF},
   m_pcm_dump{false},
   m_pcm_controller{ctrl, multi, "/dev/audio_dsp_pcm"},
   m_dump{ctrl, multi, "/dev/audio_dsp_mem"},
   m_sn{0} {
  m_dump.set_dump_start_callback(this, dump_callback);
  m_dump.set_dump_result_notifier(this, dump_result_notifiy);
  m_pcm_controller.set_notify(notify_pcm, this);
}

AgDspLogHandler::~AgDspLogHandler() { stop(); }

int AgDspLogHandler::start_dump(const struct tm& /*lt*/) {
  // Not implemented yet.
  return -1;
}

int AgDspLogHandler::control_log_dest() {
  int dest;

  switch (m_log_dest) {
    case LogConfig::AGDSP_LOG_DEST_OFF:
      dest = 0;
      break;
    case LogConfig::AGDSP_LOG_DEST_UART:
      dest = 1;
      break;
    default:
      dest = 2;
      break;
  }

  return ioctl(fd(), DSPLOG_CMD_LOG_ENABLE, reinterpret_cast<void*>(dest));
}

int AgDspLogHandler::init_log_output(LogConfig::AgDspLogDestination log_dest,
                                     bool pcm_dump) {
  bool close_fd = false;

  if (fd() < 0) {
    if (-1 == open()) {
      err_log("open AG-DSP log device error");
      return -1;
    }
    close_fd = true;
  }

  m_log_dest = log_dest;
  m_pcm_dump = pcm_dump;

  int err1 = control_log_dest();

  if (err1) {
    err_log("set AG-DSP log output error");
  }

  int err2 = 0;

  if (LogConfig::AGDSP_LOG_DEST_OFF != log_dest && !err1) {
    err2 = m_pcm_controller.set_pcm_dump(pcm_dump);
    if (err2) {
      err_log("set AG-DSP PCM output error");
    }
  }

  if (close_fd) {
    close_devices();
  }

  return (err1 < 0 || err2 < 0) ? -1 : 0;
}

int AgDspLogHandler::set_log_dest(LogConfig::AgDspLogDestination log_dest) {
  bool close_fd = false;

  if (fd() < 0) {
    if (-1 == open()) {
      return -1;
    }
    close_fd = true;
  }

  m_log_dest = log_dest;
  int err = control_log_dest();

  if (close_fd) {
    close_devices();
  }

  return err;
}

int AgDspLogHandler::set_pcm_dump(bool enable) {
  m_pcm_dump = enable;
  return m_pcm_controller.set_pcm_dump(enable);
}

int AgDspLogHandler::start() {
  int ret = start_logging();

  if (!ret) {
    if (m_pcm_controller.start()) {
      err_log("start PCM notifier error");
    }

    // start agdsp dump
    m_dump.start();
  }

  return ret;
}

int AgDspLogHandler::pause() {
  m_dump.wait_dump();

  return 0;
}

int AgDspLogHandler::stop() {
  int ret = stop_logging();

  if (!ret) {
    m_pcm_controller.stop();
    m_dump.stop();
  }

  return ret;
}

void AgDspLogHandler::process(int /*events*/) {
  save_log(fd());
}

void AgDspLogHandler::save_log(int fd, DspLogType type) {
  if (!m_buffer) {
    m_buffer = storage()->get_buffer();
    if (!m_buffer) {  // No free buffers
      del_events(POLLIN);
      return;
    }
  }

  // Process AG-DSP log
  size_t wr_start = m_buffer->data_start + m_buffer->data_len;
  uint8_t* wr_ptr = m_buffer->buffer + wr_start;
  size_t rlen = m_buffer->buf_size - wr_start;
  ssize_t nr = read(fd, wr_ptr + 20, rlen - 20);

  if (nr > 0) {
    // Add SMP and message header
    add_headers(wr_ptr, nr, type);
    nr += 20;
    m_buffer->data_len += nr;
    rlen -= nr;

    if (m_buffer->data_len >= m_buffer->buf_size * 5 / 8 || rlen < 40) {
      int err = storage()->write(m_buffer);

      if (err < 0) {
        err_log("enqueue CP %s log error, %u bytes discarded",
                ls2cstring(name()),
                static_cast<unsigned>(m_buffer->data_len));
        m_buffer->data_start = m_buffer->data_len = 0;
      } else {
        m_buffer = nullptr;
      }
    }
  } else {
    if (-1 == nr) {
      if (EAGAIN != errno && EINTR != errno) {
        err_log("read AG-DSP log or PCM error");
      }
    } else {
      err_log("AG-DSP log or PCM device driver bug: read returns 0");
    }
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

void AgDspLogHandler::notify_pcm(AgDspPcmController* fn, void* client) {
  AgDspLogHandler* agdsp = static_cast<AgDspLogHandler*>(client);
  agdsp->save_log(fn->fd(), DLT_MEMORY);
}

void AgDspLogHandler::dump_callback(void* hdl) {
  AgDspLogHandler* agdsp_hdl = static_cast<AgDspLogHandler*>(hdl);

  if (!agdsp_hdl->storage()) {
    if (agdsp_hdl->create_storage()) {
      err_log("create AG-DSP CpStorage error");
      agdsp_hdl->m_dump.end_dump();
      return;
    }
  }

  if (agdsp_hdl->storage()->check_quota()) {
      err_log("no space for agdsp dump");
      agdsp_hdl->m_dump.end_dump();
      return;
  }

  ClientManager* cli_mgr = agdsp_hdl->controller()->cli_mgr();
  cli_mgr->notify_cp_dump(agdsp_hdl->type(), ClientHandler::CE_DUMP_START);

  agdsp_hdl->m_dump.start_dump(*(agdsp_hdl->storage()), agdsp_hdl->name());
}

void AgDspLogHandler::dump_result_notifiy(void* hdl, AgdspDump::Dump_State ds) {
  AgDspLogHandler* cp = static_cast<AgDspLogHandler*>(hdl);
  if (AgdspDump::DS_FAIL == ds) {
    err_log("agdsp dump failed.");
  }

  ClientManager* cli_mgr = cp->controller()->cli_mgr();
  cli_mgr->notify_cp_dump(cp->type(), ClientHandler::CE_DUMP_END);
}
