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
#include "cp_set_dir.h"
#include "cp_sleep_log.h"
#include "cp_stor.h"
#include "diag_dev_hdl.h"
#include "ext_wcn_dump.h"
#include "log_ctrl.h"
#include "log_pipe_hdl.h"
#include "multiplexer.h"
#include "parse_utils.h"
#include "stor_mgr.h"

uint8_t LogPipeHandler::log_buffer[LogPipeHandler::LOG_BUFFER_SIZE];

LogPipeHandler::LogPipeHandler(LogController* ctrl, Multiplexer* multi,
                               const LogConfig::ConfigEntry* conf,
                               StorageManager& stor_mgr)
    : FdHandler(-1, ctrl, multi),
      m_max_buf{0},
      m_max_buf_num{0},
      m_log_commit_threshold{0},
      m_buffer{nullptr},
      m_enable{false},
      m_modem_name(conf->modem_name),
      m_type{conf->type},
      m_log_diag_same{false},
      m_reset_prop{nullptr},
      m_cp_state{CWS_WORKING},
      m_cp_state_hdl{nullptr},
      m_diag_handler{nullptr},
      m_consumer{nullptr},
      m_trans_client{nullptr},
      m_stor_mgr(stor_mgr),
      m_storage{nullptr} {
  // Reset property
  switch (conf->type) {
    case CT_WCDMA:
    case CT_TD:
    case CT_3MODE:
    case CT_4MODE:
    case CT_5MODE:
      m_max_buf = 1024 * 256;
      m_max_buf_num = 4;
      m_log_commit_threshold = 1024 * 128;

      m_reset_prop = MODEMRESET_PROPERTY;
      break;
    case CT_WCN:
      // There is a bug in WCN log device driver (Bug 615457) so that the driver
      // read function didn't check the read buffer length.
      // The bug won't be fixed in Marlin/Marlin2.
      // To work around that, 12288 bytes' buffer is setup for WCN log reading,
      // so before the buffer is ready to write to file (when the WCN log size in
      // the buffer is less than 12288 * (5 / 8) bytes), there will always be at
      // least 12288 * ( 3 / 8) = 4608 bytes' free space, which is larger than
      // WCN log device driver's max read return size 4096 bytes.
      m_max_buf = 1024 * 160;
      m_max_buf_num = 3;
      m_log_commit_threshold = 1024 * 128;

      m_reset_prop = MODEM_WCN_DEVICE_RESET;
      break;
    case CT_GNSS:
      m_max_buf = 4096;
      m_max_buf_num = 4;
      m_log_commit_threshold = 4096;
      break;
    case CT_AGDSP:
      m_max_buf = 1024 * 160;
      m_max_buf_num = 3;
      m_log_commit_threshold = 1024 * 128;
      break;
    case CT_PM_SH:
      m_max_buf = 2048;
      m_max_buf_num = 2;
      m_log_commit_threshold = 1024;

      m_reset_prop = MODEMRESET_PROPERTY;
      break;
    default:
      break;
  }
}

LogPipeHandler::~LogPipeHandler() {
  if (m_storage) {
    if (m_buffer) {
      m_storage->free_buffer(m_buffer);
    }
    m_stor_mgr.delete_storage(m_storage);
  }

  stop_transaction(CWS_NOT_WORKING);
}

int LogPipeHandler::start_logging() {
  m_enable = true;

  if (CWS_WORKING == m_cp_state) {
    if (-1 == open()) {
      err_log("open %s error", ls2cstring(m_modem_name));
      multiplexer()->timer_mgr().add_timer(3000, reopen_log_dev, this);
    } else {
      multiplexer()->register_fd(this, POLLIN);
    }
  }

  if (!m_storage) {
    if (create_storage()) {
      err_log("create_storage error");
    }
  }

  if (m_storage) {
    if (!m_buffer) {
      m_buffer = m_storage->get_buffer();
    }
    m_storage->set_buf_avail_callback(this, buf_avail_callback);
  }

  return 0;
}

void LogPipeHandler::buf_avail_callback(void* client) {
  LogPipeHandler* log_pipe = static_cast<LogPipeHandler*>(client);

  if (log_pipe->m_enable && log_pipe->fd() >= 0) {
    bool addevt = false;

    switch (log_pipe->m_cp_state) {
      case CWS_DUMP:
      case CWS_SAVE_SLEEP_LOG:
      case CWS_SAVE_RINGBUF:
      case CWS_QUERY_VER:
        if (!log_pipe->m_log_diag_same) {
          addevt = true;
        }
        break;
      default:  // No transaction on diag device
        addevt = true;
        break;
    }
    if (addevt) {
      log_pipe->add_events(POLLIN);
    }
  }
}

int LogPipeHandler::stop_logging() {
  if (m_storage) {
    m_storage->stop();
    if (m_buffer) {
      m_storage->free_buffer(m_buffer);
      m_buffer = nullptr;
    }
  }

  m_enable = false;
  delete m_consumer;
  m_consumer = 0;
  delete m_diag_handler;
  m_diag_handler = 0;
  if (m_fd >= 0) {
    multiplexer()->unregister_fd(this);
    ::close(m_fd);
    m_fd = -1;
  }

  return 0;
}

void LogPipeHandler::close_devices() {
  ::close(m_fd);
  m_fd = -1;
}

void LogPipeHandler::process(int /*events*/) {
  if (!m_buffer) {
    m_buffer = m_storage->get_buffer();
    if (!m_buffer) {  // No free buffers
      del_events(POLLIN);
      return;
    }
  }

  size_t wr_start = m_buffer->data_start + m_buffer->data_len;
  uint8_t* wr_ptr = m_buffer->buffer + wr_start;
  size_t rlen = m_buffer->buf_size - wr_start;
  ssize_t nr = read(fd(), wr_ptr, rlen);

  if (nr > 0) {
    m_buffer->data_len += nr;

    if (m_buffer->data_len >= m_buffer->buf_size * 4 / 5) {
      int err = m_storage->write(m_buffer);

      if (err < 0) {
        err_log("enqueue CP %s log error, %u bytes discarded",
                ls2cstring(m_modem_name),
                static_cast<unsigned>(m_buffer->data_len));
        m_buffer->data_start = m_buffer->data_len = 0;
      } else {
        m_buffer = nullptr;
      }
    }
  } else {
    if (-1 == nr) {
      if (EAGAIN == errno || EINTR == errno || ENODATA == errno) {
        return;
      }
      // Other errors: try to reopen the device
      multiplexer()->unregister_fd(this);
      close_devices();
      if (open() >= 0) {  // Success
        multiplexer()->register_fd(this, POLLIN);
      } else {  // Failure: arrange a check callback
        multiplexer()->timer_mgr().add_timer(3000, reopen_log_dev, this);
      }
    } else {
      err_log("read %s returns 0", ls2cstring(m_modem_name));
    }
  }
}

void LogPipeHandler::reopen_log_dev(void* param) {
  LogPipeHandler* log_pipe = static_cast<LogPipeHandler*>(param);

  if (log_pipe->m_enable && CWS_WORKING == log_pipe->m_cp_state) {
    if (log_pipe->open() >= 0) {
      log_pipe->multiplexer()->register_fd(log_pipe, POLLIN);
    } else {
      err_log("reopen %s error", ls2cstring(log_pipe->m_modem_name));
      TimerManager& tmgr = log_pipe->multiplexer()->timer_mgr();
      tmgr.add_timer(3000, reopen_log_dev, param);
    }
  }
}

void LogPipeHandler::process_reset() {
  if (m_storage) {
    m_storage->stop();
  }
}

void LogPipeHandler::process_alive() {
  if (m_enable && (-1 == m_fd)) {
    multiplexer()->timer_mgr().del_timer(reopen_log_dev);
    if (open() >= 0) {
      multiplexer()->register_fd(this, POLLIN);
    } else {
      multiplexer()->timer_mgr().add_timer(3000, reopen_log_dev, this);
    }
  }
  m_cp_state = CWS_WORKING;
}

void LogPipeHandler::close_on_assert() {
  if (m_fd >= 0) {
    multiplexer()->unregister_fd(this);
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
    inform_cp_controller(CpStateHandler::CE_DUMP_END);
    return;
  }

  // If it's saving sleep log/RingBuf, stop them
  if (CWS_WORKING != m_cp_state) {
    stop_transaction(CWS_WORKING);
  }

  bool will_reset_cp = will_be_reset();
  time_t t;
  struct tm lt;

  info_log("%s %s, cp state is %d, cp log is %s.", ls2cstring(m_modem_name),
           (will_reset_cp ? "will be rest" : "is asserted"),
           m_cp_state,
           (m_enable ? "enabled" : "disabled"));

  // If the CP will be reset, save mini dump if requested by the
  // caller.
  if (will_reset_cp) {
    if (!m_storage) {
      if (create_storage()) {
        inform_cp_controller(CpStateHandler::CE_DUMP_END);
        return;
      }
    }

    t = time(0);
    if (static_cast<time_t>(-1) == t || !localtime_r(&t, &lt)) {
      inform_cp_controller(CpStateHandler::CE_DUMP_END);
      return;
    }
    LogController::save_sipc_dump(m_storage, lt);

    if (m_type >= CT_WCDMA && m_type <= CT_5MODE && save_md) {
      controller()->save_mini_dump(m_storage, lt, m_type);
    }
    m_cp_state = CWS_NOT_WORKING;
    inform_cp_controller(CpStateHandler::CE_DUMP_END);
    return;
  }

  if (!m_storage) {
    if (create_storage()) {
      inform_cp_controller(CpStateHandler::CE_DUMP_END);
      return;
    }
  }

  t = time(0);
  if (static_cast<time_t>(-1) == t || !localtime_r(&t, &lt)) {
    inform_cp_controller(CpStateHandler::CE_DUMP_END);
    return;
  }

  // CP will not be reset: if log is turned on, save all dumps
  // as requested; otherwise only save mini dump as requested.
  LogController::save_sipc_dump(m_storage, lt);

  if (m_type >= CT_WCDMA && m_type <= CT_5MODE && save_md) {
    controller()->save_mini_dump(m_storage, lt, m_type);
  }

  // Save MODEM dump if log is turned on
  if (m_enable) {
    ClientManager* cli_mgr = controller()->cli_mgr();
    cli_mgr->notify_cp_dump(m_type, ClientHandler::CE_DUMP_START);

    if (!start_dump(lt)) {
      inform_cp_controller(CpStateHandler::CE_DUMP_START);
      if (m_log_diag_same) {
        del_events(POLLIN);
      }
      m_cp_state = CWS_DUMP;
    } else {
      // wcnd requires the property to be set
      if (CT_WCN == m_type) {
        set_wcn_dump_prop();
      }
      cli_mgr->notify_cp_dump(m_type, ClientHandler::CE_DUMP_END);
      err_log("Start dump transaction error");
      close_on_assert();
      inform_cp_controller(CpStateHandler::CE_DUMP_END);
    }
  } else {
    close_on_assert();
    inform_cp_controller(CpStateHandler::CE_DUMP_END);
  }
}

bool LogPipeHandler::will_be_reset() const {
  bool do_reset = false;

  if (m_reset_prop) {
    char reset[PROPERTY_VALUE_MAX];
    unsigned long n;
    char* endp;

    property_get(m_reset_prop, reset, "");
    n = strtoul(reset, &endp, 0);
    do_reset = (1 == n);
  }

  return do_reset;
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
  inform_cp_controller(CpStateHandler::CE_DUMP_END);
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
    multiplexer()->register_fd(m_diag_handler, POLLIN);
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
    multiplexer()->register_fd(m_diag_handler, POLLIN);
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
  int ret = -1;
  m_storage = m_stor_mgr.create_storage(*this,
                                        m_max_buf,
                                        m_max_buf_num,
                                        m_log_commit_threshold);
  if (m_storage) {
    ret = 0;
  } else {
    err_log("create %s CpStorage failed", ls2cstring(m_modem_name));
  }

  return ret;
}

int LogPipeHandler::copy_files(LogFile::LogType file_type,
                               const LogString& src_file_path) {
  time_t t = time(0);
  struct tm lt;
  int err_code = LCR_ERROR;

  // generate time to append to copied files' name.
  if (static_cast<time_t>(-1) == t || !localtime_r(&t, &lt)) {
    return err_code;
  }

  char ts[32];
  snprintf(ts, sizeof ts, "_%04d-%02d-%02d_%02d-%02d-%02d", lt.tm_year + 1900,
           lt.tm_mon + 1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);

  if (!m_storage) {
    if (create_storage()) {
      err_log("create storage error.");
      return err_code;
    }
  }

  size_t tlen;
  const uint8_t* paths =
      reinterpret_cast<const uint8_t*>(ls2cstring(src_file_path));
  const char* src_path;
  const char* file_name;

  // copy files listed in the paths one by one.
  while (0 !=
         (src_path = reinterpret_cast<const char*>(get_token(paths, tlen)))) {

    if (m_storage->check_quota() < 0) {
      err_log("No space left for copying files.");
      err_code = LCR_ERROR;
      break;
    }

    paths = reinterpret_cast<const uint8_t*>(src_path) + tlen;
    LogString file_path;
    str_assign(file_path, src_path, tlen);

    // Get the file's name in the path/to/the/file_name.
    file_name = static_cast<char*>(const_cast<void*>(memrchr(src_path, '/', tlen)));

    if (file_name) {
      if (tlen == static_cast<size_t>(file_name - src_path + 1)) {
        err_code = LCR_PARAM_INVALID;
        break;
      }

      LogString file_name_dest;
      const char* file_extension =
          static_cast<char*>(const_cast<void*>(memrchr(file_name + 1, '.',
                                     tlen - (file_name - src_path + 1))));

      if (file_extension) {
        if (tlen == static_cast<size_t>(file_extension - src_path + 1)) {
          err_log("File's full path %s is not correct.", ls2cstring(file_path));
          err_code = LCR_PARAM_INVALID;
          break;
        }

        str_assign(file_name_dest, file_name + 1,
                   file_extension - file_name - 1);
        file_name_dest += ts;

        LogString file_extension_name;
        str_assign(file_extension_name, file_extension,
                   tlen - (file_extension - src_path));

        file_name_dest += file_extension_name;
      } else {
        str_assign(file_name_dest, file_name + 1,
                   tlen - (file_name - src_path + 1));
        file_name_dest += ts;
      }

      LogFile* copy_dest_file =
          m_storage->create_file(file_name_dest, file_type);

      if (!copy_dest_file) {
        err_log("Destination file: %s creation is failed.",
                ls2cstring(file_name_dest));
        err_code = LCR_ERROR;
        break;
      } else if (copy_dest_file->copy(ls2cstring(file_path))) {
        err_log("Copy %s to %s failed.", ls2cstring(file_path),
                ls2cstring(file_name_dest));
        copy_dest_file->dir()->remove(copy_dest_file);
        err_code = LCR_ERROR;
        break;
      } else {
        err_code = LCR_SUCCESS;
      }
    } else {
      // the original file path should be the full path.
      err_log("File: %s path is not valid.", ls2cstring(file_path));
      err_code = LCR_PARAM_INVALID;
      break;
    }
  }

  return err_code;
}

void LogPipeHandler::flush() {
  if (m_storage) {
    m_storage->flush();
  }
}

void LogPipeHandler::inform_cp_controller(CpStateHandler::CpNotify evt) {
  if (m_cp_state_hdl) {
    m_cp_state_hdl->send_dump_response(evt);
  }
}

void LogPipeHandler::set_wcn_dump_prop() {
  property_set(MODEM_WCN_DUMP_LOG_COMPLETE, "1");
}
