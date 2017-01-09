/*
 *  log_ctrl.cpp - The log controller class implementation.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-2-16 Zhang Ziyi
 *  Initial version.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#ifdef HOST_TEST_
#include "prop_test.h"
#else
#include <cutils/properties.h>
#endif

#include "agdsp_log.h"
#include "pm_sensorhub_log.h"
#include "cp_dir.h"
#include "cp_log_cmn.h"
#include "cp_stor.h"
#include "def_config.h"
#include "ext_gnss_log.h"
#ifdef EXTERNAL_WCN
#include "ext_wcn_log.h"
#else
#include "int_wcn_log.h"
#endif
#include "log_ctrl.h"
#include "req_err.h"
#include "slog_config.h"
#include "wan_modem_log.h"

LogController::LogController()
    : m_config{0},
      m_save_md{false},
      m_cli_mgr{0},
      m_modem_state{0},
      m_wcn_state{0} {}

LogController::~LogController() {
  delete m_wcn_state;
  delete m_modem_state;
  clear_ptr_container(m_log_pipes);
  delete m_cli_mgr;
}

LogPipeHandler* LogController::create_handler(const LogConfig::ConfigEntry* e) {
  // First get log device files
  bool same;
  LogString log_path;
  LogString diag_path;
  int err = get_dev_paths(e->type, same, log_path, diag_path);

  if (err) {
    err_log("Can not get log/diag path for %s", ls2cstring(e->modem_name));
    return nullptr;
  }

  LogPipeHandler* log_pipe = nullptr;
  CpType cp_type = e->type;
  const char* proc_mem;

  switch (cp_type) {
#ifdef SUPPORT_WCN
    case CT_WCN:
  #ifdef EXTERNAL_WCN
      log_pipe = new ExtWcnLogHandler(this, &m_multiplexer, e, m_stor_mgr);
  #else
      log_pipe = new IntWcnLogHandler(this, &m_multiplexer, e, m_stor_mgr,
                                      "/proc/cpwcn/mem");
  #endif
      break;
#endif
    case CT_GNSS:
      log_pipe = new ExtGnssLogHandler(this, &m_multiplexer, e, m_stor_mgr);
      break;
    case CT_AGDSP:
      log_pipe = new AgDspLogHandler(this, &m_multiplexer, e, m_stor_mgr);
      break;
    case CT_PM_SH:
      log_pipe = new PmSensorhubLogHandler(this, &m_multiplexer, e, m_stor_mgr);
      break;
    case CT_TD:
      cp_type = CT_WANMODEM;
      proc_mem = "/proc/cpt/mem";
      break;
    case CT_WCDMA:
      cp_type = CT_WANMODEM;
      proc_mem = "/proc/cpw/mem";
      break;
    case CT_3MODE:
    case CT_4MODE:
    case CT_5MODE:
      cp_type = CT_WANMODEM;
      proc_mem = "/proc/cptl/mem";
      break;
    default:
      break;
  }

  if (cp_type == CT_WANMODEM) {
#ifdef HOST_TEST_
    proc_mem = "/data/local/tmp/mem";
#endif
    log_pipe = new WanModemLogHandler(this, &m_multiplexer, e, m_stor_mgr,
                                      proc_mem);

    WanModemLogHandler* wan_cp = static_cast<WanModemLogHandler*>(log_pipe);
    // If WCDMA I/Q is enabled, set it.
    if (m_config->wcdma_iq_enabled(e->type)) {
      wan_cp->enable_wcdma_iq();
    }
  }

  if (log_pipe) {
    if (same) {
      info_log("CP %s dev %s", ls2cstring(e->modem_name),
               ls2cstring(log_path));
      log_pipe->set_log_diag_dev_path(log_path);
    } else {
      info_log("CP %s dev %s %s", ls2cstring(e->modem_name),
               ls2cstring(log_path), ls2cstring(diag_path));
      log_pipe->set_log_diag_dev_path(log_path, diag_path);
    }
    if (e->enable) {
      err = log_pipe->start();
      if (err < 0) {
        err_log("start %s log failed", ls2cstring(e->modem_name));
      }
    }
  }

  return log_pipe;
}

int LogController::init(LogConfig* config) {
  // Init the /data statistics
  uint64_t log_sz = config->max_data_part_size();
  log_sz <<= 20;
  m_stor_mgr.set_data_part_limit(log_sz);
  log_sz = config->max_sd_size();
  log_sz <<= 20;
  m_stor_mgr.set_sd_limit(log_sz);
  m_stor_mgr.set_file_size_limit(config->max_log_file() << 20);
  m_stor_mgr.set_overwrite(config->overwrite_old_log());

  m_config = config;
  m_save_md = config->md_enabled();

  if (LogConfig::SLOG_DIR == config->cp_log_dir_type()) {
    m_stor_mgr.set_cp_log_in_ap_log_dir("/slog");
  } else if (LogConfig::YLOG_DIR == config->cp_log_dir_type()) {
    m_stor_mgr.set_cp_log_in_ap_log_dir("/ylog");
  }

  // Initialize the storage manager
  int err = m_stor_mgr.init(m_config->get_mount_point(), this, &m_multiplexer);
  if (err < 0) {
    err_log("init storage manager error");
    return -1;
  }

  // Create LogPipeHandlers according to settings in config
  // And add them to m_log_pipes
  const LogConfig::ConfigList& conf_list = config->get_conf();
  for (auto it = conf_list.begin(); it != conf_list.end(); ++it) {
    LogConfig::ConfigEntry* pentry = *it;
    LogPipeHandler* handler = create_handler(pentry);
    if (handler) {
      m_log_pipes.push_back(handler);
      // Initialize AG-DSP log
      if (CT_AGDSP == handler->type() &&
          !str_empty(handler->log_dev_path())) {
        info_log("init AG-DSP log output: %d, %s",
                 static_cast<int>(config->agdsp_log_dest()),
                 config->agdsp_pcm_dump_enabled() ? "ON" : "OFF");
        AgDspLogHandler* agdsp = static_cast<AgDspLogHandler*>(handler);
        if (agdsp->init_log_output(config->agdsp_log_dest(),
                                   config->agdsp_pcm_dump_enabled())) {
          err_log("AG-DSP log init error");
        }
      }
    }
  }

  // Server socket for clients (Engineering mode)
  m_cli_mgr = new ClientManager(this, &m_multiplexer, 3);
  if (m_cli_mgr->init(SLOG_MODEM_SERVER_SOCK_NAME) < 0) {
    delete m_cli_mgr;
    m_cli_mgr = 0;
  }

  // Connection to modemd/wcnd
  init_state_handler(m_modem_state, MODEM_SOCKET_NAME);

#ifdef SUPPORT_WCN
  init_wcn_state_handler(m_wcn_state, WCN_STAT_SOCK_NAME, WCN_ASSERT_MESSAGE);
#endif

  return 0;
}

void LogController::init_state_handler(ModemStateHandler*& handler,
                                       const char* serv_name) {
  ModemStateHandler* p = new ModemStateHandler(this, &m_multiplexer, serv_name);

  int err = p->init();
  if (err < 0) {
    delete p;
    err_log("connect to %s error", serv_name);
  } else {
    handler = p;
  }
}

void LogController::init_wcn_state_handler(WcndStateHandler*& handler,
                                           const char* serv_name,
                                           const char* assert_msg) {
  WcndStateHandler* p =
      new WcndStateHandler(this, &m_multiplexer, serv_name, assert_msg);

  int err = p->init();
  if (err < 0) {
    delete p;
    err_log("connect to %s error", serv_name);
  } else {
    handler = p;
  }
}

int LogController::run() { return m_multiplexer.run(); }

void LogController::process_cp_evt(CpType type, CpStateHandler::CpEvent evt) {
  LogList<LogPipeHandler*>::iterator it;

  if (CT_WANMODEM == type) {
    it = find_log_handler(m_log_pipes, m_config->get_wan_modem_type());
  } else {
    it = find_log_handler(m_log_pipes, type);
  }

  if (it == m_log_pipes.end()) {
    err_log("unknown MODEM type %d asserts", type);
    return;
  }

  LogPipeHandler* p = *it;

  if (evt == CpStateHandler::CE_ALIVE) {
    p->process_alive();
  } else if (evt == CpStateHandler::CE_RESET) {
    p->process_reset();
  } else if ((evt == CpStateHandler::CE_ASSERT) ||
             (evt == CpStateHandler::CE_BLOCKED)) {
    p->process_assert(m_save_md);
  } else {
    err_log("unknown event %d", static_cast<int>(evt));
  }
}

int LogController::save_mini_dump(CpStorage* stor, const struct tm& lt,
                                  CpType type) {
  char md_name[80];
  LogFile* f;

  snprintf(md_name, sizeof md_name,
           "mini_dump_%04d-%02d-%02d_%02d-%02d-%02d.bin", lt.tm_year + 1900,
           lt.tm_mon + 1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);

  if (m_config->md_save_to_int()) {
    // First check storage quota
    if (stor->check_quota(StorageManager::MT_INTERNAL) < 0) {
      err_log("no space for mini dump");
      return -1;
    }
    f = stor->create_file(LogString(md_name), LogFile::LT_MINI_DUMP,
                          StorageManager::MT_INTERNAL);
  } else {
    // First check storage quota
    if (stor->check_quota() < 0) {
      err_log("no space for mini dump");
      return -1;
    }
    f = stor->create_file(LogString(md_name), LogFile::LT_MINI_DUMP);
  }

  int ret = -1;
  if (f) {
    if (CT_WCDMA == type) {
      ret = f->copy(MINI_DUMP_WCDMA_SRC_FILE);
    } else if (CT_TD == type) {
      ret = f->copy(MINI_DUMP_TD_SRC_FILE);
    } else {
      ret = f->copy(MINI_DUMP_LTE_SRC_FILE);
    }
    f->close();
    if (ret) {
      err_log("save mini dump error");
      f->dir()->remove(f);
    }
  } else {
    err_log("create mini dump file %s failed", md_name);
  }

  return ret;
}

int LogController::save_sipc_dump(CpStorage* stor, const struct tm& lt) {
  char fn[64];
  int year = lt.tm_year + 1900;
  int mon = lt.tm_mon + 1;
  int ret = -1;

  // /d/sipc/smsg
  snprintf(fn, sizeof fn, "smsg_%04d-%02d-%02d_%02d-%02d-%02d.log", year, mon,
           lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);
  int err = 0;

  LogFile* f = stor->create_file(LogString(fn), LogFile::LT_SIPC);
  if (f) {
    ret = f->copy(DEBUG_SMSG_PATH);
    f->close();
    if (ret) {
      err_log("save SIPC smsg info failed");
      f->dir()->remove(f);
      err = -1;
    }
  }

  // /d/sipc/sbuf
  snprintf(fn, sizeof fn, "sbuf_%04d-%02d-%02d_%02d-%02d-%02d.log", year, mon,
           lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);
  f = stor->create_file(LogString(fn), LogFile::LT_SIPC);
  if (f) {
    ret = f->copy(DEBUG_SBUF_PATH);
    f->close();
    if (ret) {
      err_log("save SIPC sbuf info failed");
      f->dir()->remove(f);
      err = -1;
    }
  }

  // /d/sipc/sblock
  snprintf(fn, sizeof fn, "sblock_%04d-%02d-%02d_%02d-%02d-%02d.log", year, mon,
           lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);
  f = stor->create_file(LogString(fn), LogFile::LT_SIPC);
  if (f) {
    ret = f->copy(DEBUG_SBLOCK_PATH);
    f->close();
    if (ret) {
      err_log("save SIPC sblock info failed");
      f->dir()->remove(f);
      err = -1;
    }
  }

  return err;
}

LogList<LogPipeHandler*>::iterator LogController::find_log_handler(
    LogList<LogPipeHandler*>& handlers, CpType t) {
  LogList<LogPipeHandler*>::iterator it;

  for (it = handlers.begin(); it != handlers.end(); ++it) {
    if (t == (*it)->type()) {
      break;
    }
  }

  return it;
}

LogList<LogPipeHandler*>::const_iterator LogController::find_log_handler(
    const LogList<LogPipeHandler*>& handlers, CpType t) {
  LogList<LogPipeHandler*>::const_iterator it;

  for (it = handlers.begin(); it != handlers.end(); ++it) {
    if (t == (*it)->type()) {
      break;
    }
  }

  return it;
}

int LogController::enable_log(const CpType* cps, size_t num) {
  int mod_num = 0;
  size_t i;

  for (i = 0; i < num; ++i) {
    LogList<LogPipeHandler*>::iterator it =
        find_log_handler(m_log_pipes, cps[i]);
    if (it != m_log_pipes.end()) {
      LogPipeHandler* p = *it;
      if (!str_empty(p->log_dev_path())) {
        if (!p->enabled()) {
          p->start();
          m_config->enable_log(p->type());
          ++mod_num;
        }
      }
    }
  }

  if (mod_num) {
    int err = m_config->save();
    if (err) {
      err_log("save config file failed");
    }
  }

  return 0;
}

int LogController::disable_log(const CpType* cps, size_t num) {
  int mod_num = 0;
  size_t i;

  for (i = 0; i < num; ++i) {
    LogList<LogPipeHandler*>::iterator it =
        find_log_handler(m_log_pipes, cps[i]);
    if (it != m_log_pipes.end()) {
      LogPipeHandler* p = *it;
      if (p->enabled()) {
        p->stop();
        m_config->enable_log(p->type(), false);
        ++mod_num;
      }
    }
  }

  if (mod_num) {
    int err = m_config->save();
    if (err) {
      err_log("save config file failed");
    }
  }

  return 0;
}

bool LogController::get_log_state(CpType ct) const {
  bool enabled = false;
  auto it = find_log_handler(m_log_pipes, ct);

  if (it != m_log_pipes.end()) {
    enabled = (*it)->enabled();
  }

  return enabled;
}

int LogController::enable_md() {
  if (!m_save_md) {
    m_save_md = true;
    m_config->enable_md();
    if (m_config->save()) {
      err_log("save config file failed");
    }
  }

  return 0;
}

int LogController::disable_md() {
  if (m_save_md) {
    m_save_md = false;
    m_config->enable_md(false);
    if (m_config->save()) {
      err_log("save config file failed");
    }
  }

  return 0;
}

int LogController::mini_dump() {
  time_t t = time(0);
  struct tm lt;

  if (static_cast<time_t>(-1) == t || !localtime_r(&t, &lt)) {
    return -1;
  }
  // TODO: use m_stor_mgr to save file
  return -1;
}

int LogController::reload_slog_conf() {
  // Reload the /data/local/tmp/slog/slog.conf, and only update
  // the CP enable states.
  SLogConfig slogc;

  if (slogc.read_config(TMP_SLOG_CONFIG)) {
    err_log("load slog.conf failed");
    return -1;
  }

  const SLogConfig::ConfigList& clist = slogc.get_conf();
  SLogConfig::ConstConfigIter it;

  for (it = clist.begin(); it != clist.end(); ++it) {
    const SLogConfig::ConfigEntry* p = *it;
    LogList<LogPipeHandler*>::iterator it_cp =
        find_log_handler(m_log_pipes, p->type);
    if (it_cp != m_log_pipes.end()) {
      LogPipeHandler* pipe_hdl = *it_cp;
      if (pipe_hdl->enabled() != p->enable) {
        info_log("%s CP %s", p->enable ? "enable" : "disable",
                 ls2cstring(pipe_hdl->name()));
        if (p->enable) {
          bool same;
          LogString log_path;
          LogString diag_path;
          int err = get_dev_paths(p->type, same, log_path, diag_path);
          if (!err) {
            if (same) {
              pipe_hdl->set_log_diag_dev_path(log_path);
            } else {
              pipe_hdl->set_log_diag_dev_path(log_path, diag_path);
            }
            pipe_hdl->start();
          } else {
            err_log("Can not get log/diag path for %s",
                    ls2cstring(pipe_hdl->name()));
          }
        } else {
          pipe_hdl->flush();
          pipe_hdl->stop();
        }
        m_config->enable_log(p->type, p->enable);
      }
    }
  }

  if (m_config->dirty()) {
    if (m_config->save()) {
      err_log("save config failed");
    }
  }

  return 0;
}

size_t LogController::get_log_file_size() const {
  return m_config->max_log_file();
}

int LogController::set_log_file_size(size_t len) {
  m_stor_mgr.set_file_size_limit(len << 20);

  m_config->set_log_file_size(len);
  if (m_config->dirty()) {
    if (m_config->save()) {
      err_log("save config file failed");
    }
  }

  return 0;
}

int LogController::set_log_overwrite(bool en) {
  m_stor_mgr.set_overwrite(en);

  m_config->set_overwrite(en);
  if (m_config->dirty()) {
    if (m_config->save()) {
      err_log("save config file failed");
    }
  }

  return 0;
}

size_t LogController::get_data_part_size() const {
  return m_config->max_data_part_size();
}

int LogController::set_data_part_size(size_t sz) {
  uint64_t byte_sz = sz;

  byte_sz <<= 20;
  m_stor_mgr.set_data_part_limit(byte_sz);

  m_config->set_data_part_size(sz);
  if (m_config->dirty()) {
    if (m_config->save()) {
      err_log("save config file failed");
    }
  }

  return 0;
}

bool LogController::get_md_int_stor() const {
  return m_config->md_save_to_int();
}

int LogController::set_md_int_stor(bool md_int_stor) {
  m_config->set_md_int_stor(md_int_stor);

  if (m_config->dirty()) {
    if (m_config->save()) {
      err_log("save config file failed");
    }
  }

  return 0;
}

size_t LogController::get_sd_size() const { return m_config->max_sd_size(); }

int LogController::set_sd_size(size_t sz) {
  uint64_t byte_sz = sz;

  byte_sz <<= 20;
  m_stor_mgr.set_sd_limit(byte_sz);

  m_config->set_sd_size(sz);
  if (m_config->dirty()) {
    if (m_config->save()) {
      err_log("save config file failed");
    }
  }

  return 0;
}

bool LogController::get_log_overwrite() const {
  return m_config->overwrite_old_log();
}

int LogController::clear_log() {
  LogList<LogPipeHandler*>::iterator it;

  for (it = m_log_pipes.begin(); it != m_log_pipes.end(); ++it) {
    if ((*it)->in_transaction()) {
      return LCR_IN_TRANSACTION;
    }
  }

  // Now we can pause log
  for (it = m_log_pipes.begin(); it != m_log_pipes.end(); ++it) {
    LogPipeHandler* cp = *it;

    if (cp->enabled()) {
      cp->pause();
    }
  }

  m_stor_mgr.clear();

  // Now we can resume log
  for (it = m_log_pipes.begin(); it != m_log_pipes.end(); ++it) {
    LogPipeHandler* cp = *it;

    if (cp->enabled()) {
      cp->resume();
    }
  }

  return LCR_SUCCESS;
}

LogPipeHandler* LogController::get_cp(CpType type) {
  LogList<LogPipeHandler*>::iterator it = find_log_handler(m_log_pipes, type);
  LogPipeHandler* p = 0;
  if (it != m_log_pipes.end()) {
    p = (*it);
  }
  return p;
}

int LogController::enable_wcdma_iq(CpType cpt) {
  if ((cpt == CT_WCN) || (cpt == CT_PM_SH) ||
      (cpt == CT_GNSS) || (cpt == CT_AGDSP)) {
    err_log("enable WCDMA I/Q on wrong CP %d", cpt);
    return LCR_ERROR;
  }

  WanModemLogHandler* cp = static_cast<WanModemLogHandler*>(get_cp(cpt));
  if (!cp) {
    return LCR_CP_NONEXISTENT;
  }

  int err = cp->enable_wcdma_iq();
  if (LCR_SUCCESS == err) {
    m_config->enable_iq(cpt, IT_WCDMA);
    m_config->save();
  }

  return err;
}

int LogController::disable_wcdma_iq(CpType cpt) {
   if ((cpt == CT_WCN) || (cpt == CT_PM_SH) ||
      (cpt == CT_GNSS) || (cpt == CT_AGDSP)) {
    err_log("disable WCDMA I/Q on wrong CP %d", cpt);
    return LCR_ERROR;
  }

  WanModemLogHandler* cp = static_cast<WanModemLogHandler*>(get_cp(cpt));
  if (!cp) {
    return LCR_CP_NONEXISTENT;
  }

  int err = cp->disable_wcdma_iq();
  if (LCR_SUCCESS == err) {
    m_config->disable_iq(cpt, IT_WCDMA);
    m_config->save();
  }

  return err;
}

int LogController::copy_cp_files(CpType cpt, LogFile::LogType file_type,
                                 const LogString& src_file_path) {
  int ret = LCR_CP_NONEXISTENT;
  LogPipeHandler* cp = get_cp(cpt);

  if (cp) {
    ret = cp->copy_files(file_type, src_file_path);
  }

  return ret;
}

int LogController::flush() {
  LogList<LogPipeHandler*>::iterator it;

  for (it = m_log_pipes.begin(); it != m_log_pipes.end(); ++it) {
    LogPipeHandler* cp = *it;
    if (cp->enabled()) {
      cp->flush();
    }
  }

  return LCR_SUCCESS;
}

LogConfig::AgDspLogDestination LogController::get_agdsp_log_dest() const {
  return m_config->agdsp_log_dest();
}

int LogController::set_agdsp_log_dest(LogConfig::AgDspLogDestination dest) {
  info_log("set AG-DSP log output %d", static_cast<int>(dest));

  LogList<LogPipeHandler*>::iterator it =
      find_log_handler(m_log_pipes, CT_AGDSP);
  int ret = LCR_NO_AGDSP;

  if (it != m_log_pipes.end()) {
    AgDspLogHandler* p = static_cast<AgDspLogHandler*>(*it);
    if (!str_empty(p->log_dev_path())) {
      p->set_log_dest(dest);
      m_config->set_agdsp_log_dest(dest);
      if (m_config->dirty()) {
        m_config->save();
      }

      ret = LCR_SUCCESS;
    }
  }

  return ret;
}

bool LogController::get_agdsp_pcm_dump() const {
  return m_config->agdsp_pcm_dump_enabled();
}

int LogController::set_agdsp_pcm_dump(bool pcm) {
  info_log("set AG-DSP PCM dump %s", pcm ? "ON" : "OFF");

  LogList<LogPipeHandler*>::iterator it =
      find_log_handler(m_log_pipes, CT_AGDSP);
  int ret = LCR_NO_AGDSP;

  if (it != m_log_pipes.end()) {
    AgDspLogHandler* p = static_cast<AgDspLogHandler*>(*it);
    if (!str_empty(p->log_dev_path())) {
      p->set_pcm_dump(pcm);
      m_config->enable_agdsp_pcm_dump(pcm);
      if (m_config->dirty()) {
        m_config->save();
      }

      ret = LCR_SUCCESS;
    }
  }

  return ret;
}
