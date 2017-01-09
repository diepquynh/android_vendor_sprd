/*
 *  log_pipe_hdl.cpp - The CP log and dump handler class.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-3-2 Zhang Ziyi
 *  Initial version.
 */
#include <cassert>
#include <cstdlib>
#include <cstring>
#ifdef HOST_TEST_
#include "sock_test.h"
#else
#include "cutils/sockets.h"
#endif
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "cp_dir.h"
#include "cp_ringbuf.h"
#include "cp_sleep_log.h"
#include "cp_stor.h"
#include "diag_dev_hdl.h"
#include "ext_wcn_dump.h"
#include "log_ctrl.h"
#include "log_pipe_hdl.h"
#include "modem_dump.h"
#include "multiplexer.h"
#include "req_err.h"
#include "stor_mgr.h"

uint8_t LogPipeHandler::log_buffer[LogPipeHandler::LOG_BUFFER_SIZE];

LogPipeHandler::LogPipeHandler(LogController* ctrl, Multiplexer* multi,
                               const LogConfig::ConfigEntry* conf,
                               StorageManager& stor_mgr)
    : FdHandler(-1, ctrl, multi),
      m_enable{false},
      m_save_wcdma_iq{false},
      m_modem_name(conf->modem_name),
      m_type{conf->type},
      m_log_diag_same{false},
      m_cp_state{CWS_WORKING},
      m_diag_handler{0},
      m_consumer{0},
      m_trans_client{0},
      m_stor_mgr(stor_mgr),
      m_storage{0},
      m_wcdma_iq_mgr(this) {
  // Reset property
  if (CT_WCN == conf->type) {
    m_reset_prop = MODEM_WCN_DEVICE_RESET;
  } else {
    m_reset_prop = MODEMRESET_PROPERTY;
  }
}

LogPipeHandler::~LogPipeHandler() {
  if (m_storage) {
    m_stor_mgr.delete_storage(m_storage);
  }

  stop_transaction(CWS_NOT_WORKING);
}

int LogPipeHandler::start_logging() {
  m_enable = true;

  if (CWS_WORKING == m_cp_state) {
    if (-1 == open()) {
      err_log("slogcp: open %s error", ls2cstring(m_modem_name));
      multiplexer()->timer_mgr().add_timer(3000, reopen_log_dev, this);
    } else {
      add_events(POLLIN);
    }
  }

  if (!m_storage) {
    if (create_storage()) {
      err_log("create_storage error");
    }
  }

  // Start WCDMA I/Q if enabled
  if (m_save_wcdma_iq && m_storage) {
    if (m_wcdma_iq_mgr.start(m_storage)) {
      err_log("start saving WCDMA I/Q error");
    }
  }

  return 0;
}

int LogPipeHandler::stop_logging() {
  // If WCDMA I/Q is started, stop it.
  if (m_wcdma_iq_mgr.is_started()) {
    if (m_wcdma_iq_mgr.stop()) {
      err_log("stop saving WCDMA I/Q error");
    }
  }

  if (m_storage) {
    m_storage->stop();
  }

  m_enable = false;
  delete m_consumer;
  m_consumer = 0;
  delete m_diag_handler;
  m_diag_handler = 0;
  if (m_fd >= 0) {
    del_events(POLLIN);
    ::close(m_fd);
    m_fd = -1;
  }

  return 0;
}

int LogPipeHandler::pause() {
  if (m_wcdma_iq_mgr.is_started()) {
    m_wcdma_iq_mgr.pause();
    m_wcdma_iq_mgr.pre_clear();
  }

  return 0;
}

int LogPipeHandler::resume() {
  if (m_wcdma_iq_mgr.is_started()) {
    m_wcdma_iq_mgr.resume();
  }

  return 0;
}

void LogPipeHandler::close_devices() {
  ::close(m_fd);
  m_fd = -1;
}

void LogPipeHandler::process(int /*events*/) {
  // The log device file readable, read it

  ssize_t nr = read(m_fd, log_buffer, LOG_BUFFER_SIZE);
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

  // There is a bug in CP2 log device driver: poll reports the device
  // is readable, but read returns 0.
  if (!nr) {
    err_log("log device driver bug: read returns 0");
    return;
  }

  if (!m_storage) {
    if (create_storage()) {
      return;
    }
  }

  int err = m_storage->write(log_buffer, nr);
  if (err < 0) {
    err_log("CP %s save log error", ls2cstring(m_modem_name));
  }
}

void LogPipeHandler::reopen_log_dev(void* param) {
  LogPipeHandler* log_pipe = static_cast<LogPipeHandler*>(param);

  if (log_pipe->m_enable && CWS_WORKING == log_pipe->m_cp_state) {
    if (log_pipe->open() >= 0) {
      log_pipe->add_events(POLLIN);
    } else {
      err_log("slogcp: reopen %s error", ls2cstring(log_pipe->m_modem_name));
      TimerManager& tmgr = log_pipe->multiplexer()->timer_mgr();
      tmgr.add_timer(3000, reopen_log_dev, param);
    }
  }
}

bool LogPipeHandler::save_version() {
  char version[PROPERTY_VALUE_MAX];

  property_get(MODEM_VERSION, version, "");
  if (!version[0]) {
    return false;
  }

  bool ret = false;
  LogString ver_file = m_modem_name + ".version";
  LogFile* logf = m_storage->create_file(ver_file, LogFile::LT_VERSION);

  if (logf) {
    size_t len = strlen(version);
    ssize_t n = logf->write(version, len);
    logf->close();
    if (static_cast<size_t>(n) == len) {
      ret = true;
    }
  }

  return ret;
}

void LogPipeHandler::open_on_alive() {
  if (m_enable) {
    if (open() >= 0) {
      add_events(POLLIN);
    }
  }
  m_cp_state = CWS_WORKING;
}

void LogPipeHandler::close_on_assert() {
  if (m_fd >= 0) {
    del_events(POLLIN);
  }
  close_devices();

  // Flush current log file
  if (m_storage) {
    m_storage->flush();
  }

  m_cp_state = CWS_NOT_WORKING;
}

void LogPipeHandler::process_assert(bool save_md /*= true*/) {
  if (CWS_NOT_WORKING == m_cp_state || CWS_DUMP == m_cp_state) {
    return;
  }

  // If it's saving sleep log/RingBuf, stop them
  if (CWS_WORKING != m_cp_state) {
    stop_transaction(CWS_WORKING);
  }

  bool will_reset_cp = will_be_reset();
  time_t t;
  struct tm lt;

  // If the CP will be reset, save mini dump if requested by the
  // caller.
  if (will_reset_cp) {
    if (save_md) {
      if (!m_storage) {
        m_storage = m_stor_mgr.create_storage(*this);
        if (!m_storage) {
          return;
        }
        m_storage->set_new_log_callback(new_log_callback);
      }

      t = time(0);
      if (static_cast<time_t>(-1) == t || !localtime_r(&t, &lt)) {
        return;
      }
      controller()->save_mini_dump(m_storage, lt, m_type);
    }
    m_cp_state = CWS_NOT_WORKING;
    return;
  }

  // CP will not be reset: if log is turned on, save all dumps
  // as requested; otherwise only save mini dump as requested.
  if (!m_storage) {
    m_storage = m_stor_mgr.create_storage(*this);
    if (!m_storage) {
      return;
    }
    m_storage->set_new_log_callback(new_log_callback);
  }

  t = time(0);
  if (static_cast<time_t>(-1) == t || !localtime_r(&t, &lt)) {
    return;
  }

  LogController::save_sipc_dump(m_storage, lt);

  if (save_md) {  // Mini dump shall be saved
    controller()->save_mini_dump(m_storage, lt, m_type);
  }

  // Save MODEM dump if log is turned on
  if (m_enable) {
    ClientManager* cli_mgr = controller()->cli_mgr();
    cli_mgr->notify_cp_dump(m_type, ClientHandler::CE_DUMP_START);

    if (!start_dump(lt)) {
      if (m_log_diag_same) {
        del_events(POLLIN);
      }
      m_cp_state = CWS_DUMP;
    } else {
      cli_mgr->notify_cp_dump(m_type, ClientHandler::CE_DUMP_END);
      err_log("Start dump transaction error");
      close_on_assert();
    }
  } else {
    close_on_assert();
  }
}

bool LogPipeHandler::will_be_reset() const {
  char reset[PROPERTY_VALUE_MAX];
  long n;
  char* endp;

  property_get(m_reset_prop, reset, "");
  n = strtoul(reset, &endp, 0);
  return 1 == n ? true : false;
}

void LogPipeHandler::new_log_callback(LogPipeHandler* cp, LogFile* f) {
  cp->save_timestamp(f);
}

void LogPipeHandler::new_dir_callback(LogPipeHandler* cp) {
  cp->save_version();
}

bool LogPipeHandler::save_timestamp(LogFile* f) {
  modem_timestamp ts;
  int ts_file;
  int sleep_time = 0;
  int ret = 0;

  // Open the CP time sync server
  while (sleep_time < 1000000) {
    ts_file =
        socket_local_client(CP_TIME_SYNC_SERVER_SOCKET,
                            ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    if (ts_file >= 0) {
      break;
    }
    usleep(100000);
    sleep_time += 100000;
  }
  if (-1 == ts_file) {
    err_log("connect CP time sync server error");
    return false;
  }

  long flags = fcntl(ts_file, F_GETFL);
  if (flags < 0) {
    err_log("time sync conn F_GETFL failed");
    ::close(ts_file);
    return false;
  }
  flags |= O_NONBLOCK;
  ret = fcntl(ts_file, F_SETFL, flags);
  if (ret < 0) {
    err_log("set time sync conn O_NONBLOCK failed");
    ::close(ts_file);
    return false;
  }

  struct pollfd pol_fifo;

  pol_fifo.fd = ts_file;
  pol_fifo.events = POLLIN;
  pol_fifo.revents = 0;
  ret = poll(&pol_fifo, 1, 1000 - sleep_time / 1000);
  ssize_t n = 0;

  if (ret > 0 && (pol_fifo.revents & POLLIN)) {
    n = read(ts_file,
             reinterpret_cast<uint8_t*>(&ts) +
                 offsetof(modem_timestamp, tv_sec),
             sizeof(uint32_t) * 3);
  }
  ::close(ts_file);
  if (n < 12) {
    err_log("can not read time info");
    return false;
  }

  ts.magic_number = 0x12345678;
  int tz = get_timezone();
  ts.tv_sec += (3600 * tz);
  n = f->write(&ts, sizeof ts);
  return (static_cast<size_t>(n) == sizeof ts);
}

DiagDeviceHandler* LogPipeHandler::create_diag_device(DataConsumer* consumer) {
  DiagDeviceHandler* diag;

  if (m_log_diag_same) {
    diag = new DiagDeviceHandler(fd(), consumer, controller(), multiplexer());
  } else {
    diag = new DiagDeviceHandler(diag_dev_path(), consumer, controller(),
                                 multiplexer());
    int fd = diag->open();
    if (fd < 0) {
      delete diag;
      diag = 0;
    }
  }

  return diag;
}

LogFile* LogPipeHandler::open_dump_mem_file(const struct tm& lt) {
  char log_name[64];

  snprintf(log_name, sizeof log_name,
           "_memory_%04d-%02d-%02d_%02d-%02d-%02d.mem", lt.tm_year + 1900,
           lt.tm_mon + 1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);
  LogString mem_file_name = m_modem_name + log_name;
  LogFile* f = m_storage->create_file(mem_file_name, LogFile::LT_DUMP);
  if (!f) {
    err_log("create dump mem file %s failed", ls2cstring(mem_file_name));
  }

  return f;
}

void LogPipeHandler::end_dump() {
  stop_transaction(CWS_NOT_WORKING);
  close_on_assert();
}

void LogPipeHandler::start_transaction(DiagDeviceHandler* dev,
                                       DataConsumer* consumer,
                                       CpWorkState state) {
  m_diag_handler = dev;
  m_consumer = consumer;
  m_cp_state = state;
  if (m_log_diag_same) {
    del_events(POLLIN);
  }
}

void LogPipeHandler::stop_transaction(CpWorkState state) {
  delete m_consumer;
  m_consumer = 0;
  if (m_diag_handler) {
    delete m_diag_handler;
    m_diag_handler = 0;
  }
  m_cp_state = state;
  if (m_log_diag_same && fd() >= 0) {
    add_events(POLLIN);
  }
}

void LogPipeHandler::diag_transaction_notify(void* client,
                                             DataConsumer::LogProcResult res) {
  LogPipeHandler* cp = static_cast<LogPipeHandler*>(client);
  int result = LCR_SUCCESS;

  if (DataConsumer::LPR_SUCCESS == res) {
    result = LCR_SUCCESS;
  } else if (DataConsumer::LPR_SLEEPLOG_NO_SUPPORTED == res) {
    result = LCR_SLEEP_LOG_NOT_SUPPORTED;
  } else if (DataConsumer::LPR_RINGBUF_NO_SUPPORTED == res) {
    result = LCR_RINGBUF_NOT_SUPPORTED;
  } else {
    result = LCR_ERROR;
  }
  switch (cp->m_cp_state) {
    case CWS_SAVE_SLEEP_LOG:
      cp->stop_transaction(CWS_WORKING);
      if (cp->m_trans_client) {
        cp->m_trans_client->notify_trans_result(result);
        cp->m_trans_client = 0;
      }
      break;
    case CWS_SAVE_RINGBUF:
      cp->stop_transaction(CWS_WORKING);
      if (cp->m_trans_client) {
        cp->m_trans_client->notify_trans_result(result);
        cp->m_trans_client = 0;
      }
      break;
    default:
      err_log("Receive diag notify %d under state %d, ignore", res,
              cp->m_cp_state);
      break;
  }
}

int LogPipeHandler::save_sleep_log(ClientHandler* client) {
  if (!m_enable) {
    return LCR_LOG_DISABLED;
  }

  if (CWS_NOT_WORKING == m_cp_state) {
    return LCR_CP_ASSERTED;
  }

  if (CWS_WORKING != m_cp_state) {
    return LCR_IN_TRANSACTION;
  }

  // When ClientHandler request to save sleep log, m_storage
  // may not be created.
  if (!m_storage) {
    if (create_storage()) {
      return LCR_ERROR;
    }
  }

  int err = start_sleep_log();

  if (!err) {
    m_trans_client = client;
    m_cp_state = CWS_SAVE_SLEEP_LOG;
    err = LCR_IN_PROGRESS;
  } else {
    err = LCR_ERROR;
  }
  return err;
}

int LogPipeHandler::start_sleep_log() {
  assert(0 == m_diag_handler && 0 == m_consumer);

  // Create the data consumer first
  int err = 0;
  CpSleepLogConsumer* sleep_log =
      new CpSleepLogConsumer(m_modem_name, *m_storage);

  sleep_log->set_callback(this, diag_transaction_notify);

  if (m_log_diag_same) {
    m_diag_handler =
        new DiagDeviceHandler(m_fd, sleep_log, controller(), multiplexer());
  } else {
    m_diag_handler = new DiagDeviceHandler(m_diag_dev_path, sleep_log,
                                           controller(), multiplexer());
    int fd = m_diag_handler->open();
    if (fd < 0) {
      err = -1;
    }
  }

  if (!err) {
    m_diag_handler->add_events(POLLIN);
    sleep_log->bind(m_diag_handler);
    if (!sleep_log->start()) {
      start_transaction(m_diag_handler, sleep_log, CWS_SAVE_SLEEP_LOG);
    } else {
      err = -1;
    }
  }

  if (err) {
    delete m_diag_handler;
    m_diag_handler = 0;
    delete sleep_log;
  }

  return err;
}

int LogPipeHandler::save_ringbuf(ClientHandler* client) {
  if (!m_enable) {
    return LCR_LOG_DISABLED;
  }

  if (CWS_NOT_WORKING == m_cp_state) {
    return LCR_CP_ASSERTED;
  }

  if (CWS_WORKING != m_cp_state) {
    return LCR_IN_TRANSACTION;
  }

  // When ClientHandler request to save ringbuf, m_storage
  // may not be created.
  if (!m_storage) {
    if (create_storage()) {
      return LCR_ERROR;
    }
  }

  int err = start_ringbuf();

  if (!err) {
    m_trans_client = client;
    m_cp_state = CWS_SAVE_RINGBUF;
    err = LCR_IN_PROGRESS;
  } else {
    err = LCR_ERROR;
  }
  return err;
}

int LogPipeHandler::start_ringbuf() {
  assert(0 == m_diag_handler && 0 == m_consumer);

  // Create the data consumer first
  int err = 0;
  CpRingBufConsumer* ringbuf = new CpRingBufConsumer(m_modem_name, *m_storage);

  ringbuf->set_callback(this, diag_transaction_notify);

  if (m_log_diag_same) {
    m_diag_handler =
        new DiagDeviceHandler(m_fd, ringbuf, controller(), multiplexer());
  } else {
    m_diag_handler = new DiagDeviceHandler(m_diag_dev_path, ringbuf,
                                           controller(), multiplexer());
    int fd = m_diag_handler->open();
    if (fd < 0) {
      err = -1;
    }
  }

  if (!err) {
    m_diag_handler->add_events(POLLIN);
    ringbuf->bind(m_diag_handler);
    if (!ringbuf->start()) {
      start_transaction(m_diag_handler, ringbuf, CWS_SAVE_RINGBUF);
    } else {
      err = -1;
    }
  }

  if (err) {
    delete m_diag_handler;
    m_diag_handler = 0;
    delete ringbuf;
  }

  return err;
}

void LogPipeHandler::cancel_trans_result_notify(ClientHandler* client) {
  if (m_trans_client == client) {
    m_trans_client = 0;
  }
}

int LogPipeHandler::create_storage() {
  m_storage = m_stor_mgr.create_storage(*this);
  if (!m_storage) {
    err_log("create %s CpStorage failed", ls2cstring(m_modem_name));
    return -1;
  }
  if (m_type >= CT_WCDMA && m_type <= CT_5MODE) {
    m_storage->set_new_log_callback(new_log_callback);
    m_storage->set_new_dir_callback(new_dir_callback);
  }
  return 0;
}

int LogPipeHandler::enable_wcdma_iq() {
  // We need to reboot to turn on the /dev/iq_mem device, so we
  // only set the m_save_wcdma_iq member here.
  m_save_wcdma_iq = true;
  return LCR_SUCCESS;
}

int LogPipeHandler::disable_wcdma_iq() {
  if (!m_save_wcdma_iq) {
    return LCR_SUCCESS;
  }

  int ret = LCR_ERROR;

  if (m_enable) {
    if (!m_wcdma_iq_mgr.stop()) {
      m_save_wcdma_iq = false;
      ret = LCR_SUCCESS;
    }
  } else {
    m_save_wcdma_iq = false;
  }
  return ret;
}

void LogPipeHandler::flush() {
  if (m_storage) {
    m_storage->flush();
  }
}
