/*
 *  wan_modem_log.cpp - The WAN MODEM log and dump handler class.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-7-13 Zhang Ziyi
 *  Initial version.
 */

#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <unistd.h>

#include "client_mgr.h"
#include "cp_stor.h"
#include "diag_stream_parser.h"
#include "diag_dev_hdl.h"
#include "log_config.h"
#include "log_ctrl.h"
#include "pm_modem_dump.h"
#include "trans_modem_ver.h"
#include "wan_modem_log.h"

WanModemLogHandler::WanModemLogHandler(LogController* ctrl, Multiplexer* multi,
                                       const LogConfig::ConfigEntry* conf,
                                       StorageManager& stor_mgr,
                                       const char* dump_path)
    : LogPipeHandler{ctrl, multi, conf, stor_mgr},
      m_dump_path{dump_path},
      m_wcdma_iq_mgr{this},
      m_save_wcdma_iq{false},
      m_time_sync_notifier{nullptr},
      m_timestamp_miss{false},
      m_ver_state{MVS_NOT_BEGIN},
      m_ver_stor_state{MVSS_NOT_SAVED},
      m_reserved_version_len{0} {}

WanModemLogHandler::~WanModemLogHandler() {
  if (m_time_sync_notifier) {
    m_time_sync_notifier->unsubscribe_cp_time_update_evt(this);
    delete m_time_sync_notifier;
    m_time_sync_notifier = nullptr;
  }
}

int WanModemLogHandler::start_dump(const struct tm& lt) {
  // Create the data consumer first
  int err = 0;
  PmModemDumpConsumer* dump;
  DiagDeviceHandler* diag;

  dump = new PmModemDumpConsumer(name(), *storage(), lt, m_dump_path,
                                 "\nmodem_memdump_finish", 0x33);

  dump->set_callback(this, diag_transaction_notify);

  diag = create_diag_device(dump);
  if (diag) {
    multiplexer()->register_fd(diag, POLLIN);
    dump->bind(diag);
    if (dump->start()) {
      err = -1;
    }
  } else {
    err = -1;
  }

  if (err) {
    delete diag;
    delete dump;
  } else {
    start_transaction(diag, dump, CWS_DUMP);
  }

  return err;
}

void WanModemLogHandler::diag_transaction_notify(
    void* client, DataConsumer::LogProcResult res) {
  WanModemLogHandler* cp = static_cast<WanModemLogHandler*>(client);

  if (CWS_DUMP == cp->cp_state()) {
    cp->end_dump();
    ClientManager* cli_mgr = cp->controller()->cli_mgr();
    cli_mgr->notify_cp_dump(cp->type(), ClientHandler::CE_DUMP_END);
  } else {
    err_log("Receive diag notify %d under state %d, ignore", res,
            cp->cp_state());
  }
}

int WanModemLogHandler::create_storage() {
  int ret = LogPipeHandler::create_storage();

  if (!ret) {
    storage()->set_new_log_callback(new_log_callback);
  }

  return ret;
}

int WanModemLogHandler::start_ver_query() {
  WanModemVerQuery* query = new WanModemVerQuery{name(), *storage()};
  query->set_callback(this, transaction_modem_ver_notify);
  DiagDeviceHandler* diag = create_diag_device(query);
  int err = 0;

  if (diag) {
    multiplexer()->register_fd(diag, POLLIN);
    query->bind(diag);
    if (query->start()) {
      err = -1;
    }
  } else {
    err = -1;
  }

  if (err) {
    delete diag;
    delete query;
  } else {
    start_transaction(diag, query, CWS_QUERY_VER);
  }

  return err;
}

void WanModemLogHandler::fill_smp_header(uint8_t* buf, size_t len,
                                         unsigned lcn, unsigned type) {
  memset(buf, 0x7e, 4);
  buf[4] = static_cast<uint8_t>(len);
  buf[5] = static_cast<uint8_t>(len >> 8);
  buf[6] = static_cast<uint8_t>(lcn);
  buf[7] = static_cast<uint8_t>(type);
  buf[8] = 0x5a;
  buf[9] = 0x5a;

  uint32_t cs = ((type << 8) | lcn);

  cs += (len + 0x5a5a);
  cs = (cs & 0xffff) + (cs >> 16);
  cs = ~cs;
  buf[10] = static_cast<uint8_t>(cs);
  buf[11] = static_cast<uint8_t>(cs >> 8);
}

void WanModemLogHandler::fill_diag_header(uint8_t* buf, uint32_t sn,
                                          unsigned len, unsigned cmd,
                                          unsigned subcmd) {
  buf[0] = static_cast<uint8_t>(sn);
  buf[1] = static_cast<uint8_t>(sn >> 8);
  buf[2] = static_cast<uint8_t>(sn >> 16);
  buf[3] = static_cast<uint8_t>(sn >> 24);

  buf[4] = static_cast<uint8_t>(len);
  buf[5] = static_cast<uint8_t>(len >> 8);
  buf[6] = static_cast<uint8_t>(cmd);
  buf[7] = static_cast<uint8_t>(subcmd);
}

uint8_t* WanModemLogHandler::frame_noversion_and_reserve(size_t& length) {
  const char* unavailable = "(unavailable)";
  uint8_t* ret = nullptr;

  if (log_diag_dev_same()) {
    // Keep the place holder for diag frame
    // modem version length * 2 (escaped length) +
    // 8 * 2 (length for escaped header) + 2 (two flags)
    length = (PRE_MODEM_VERSION_LEN << 1) + 18;
    ret = new uint8_t[length];

    size_t framed_len;
    uint8_t* diag_frame =
        DiagStreamParser::frame(0xffffffff, 0, 0,
                                reinterpret_cast<uint8_t*>(
                                    const_cast<char*>(unavailable)),
                                strlen(unavailable), framed_len);
    memcpy(ret, diag_frame, framed_len);
    delete [] diag_frame;
    memset(ret + framed_len, 0x7e, length - framed_len);
  } else {
    // Keep the place holder for smp frame
    // modem version length + 12(smp) + 8(diag)
    length = PRE_MODEM_VERSION_LEN + 20;
    ret = new uint8_t[length];
    memset(ret, ' ', length);

    fill_smp_header(ret, length - 4, 1, 0x9d);
    fill_diag_header(ret + 12, 0xffffffff, length - 12, 0, 0);
    memcpy(ret + 20, unavailable, strlen(unavailable));
  }

  return ret;
}

uint8_t* WanModemLogHandler::frame_version_info(const uint8_t* payload,
                                                size_t pl_len,
                                                size_t len_reserved,
                                                size_t& frame_len) {
  uint8_t* ret = nullptr;

  if (log_diag_dev_same()) {
    ret = DiagStreamParser::frame(0xffffffff, 0, 0, payload,
                                  pl_len, frame_len);
    if (len_reserved && (frame_len > len_reserved)) {
      // if framed info is larger than preserved, the next diag frame will
      // be destroyed.
      delete [] ret;
      ret = nullptr;
      err_log("reserved length %u is less than needed %u",
              static_cast<unsigned>(len_reserved),
              static_cast<unsigned>(frame_len));
    }
  } else {
    if (len_reserved) {
      if (len_reserved < pl_len + 20) {
        err_log("reserved length %u is less than needed %u",
                static_cast<unsigned>(len_reserved),
                static_cast<unsigned>(pl_len + 20));
        return nullptr;
      } else {
        frame_len = len_reserved;
      }
    } else {
      frame_len = 20 + pl_len;
    }

    ret = new uint8_t[frame_len];
    memset(ret, ' ', frame_len);

    fill_smp_header(ret, frame_len - 4, 1, 0x9d);
    fill_diag_header(ret + 12, 0xffffffff, frame_len - 12, 0, 0);
    memcpy(ret + 20, payload, pl_len);
  }

  return ret;
}

void WanModemLogHandler::correct_ver_info() {
  CpStorage* stor = storage();
  DataBuffer* buf = stor->get_buffer();

  if (!buf) {
    err_log("No buffer for correcting MODEM version info");
    return;
  }

  size_t size_to_frame = m_modem_ver.length();
  if (size_to_frame > PRE_MODEM_VERSION_LEN) {
    size_to_frame = PRE_MODEM_VERSION_LEN;
  }

  size_t framed_len;
  uint8_t* ver_framed =
      frame_version_info(reinterpret_cast<uint8_t*>(const_cast<char*>(
                             ls2cstring(m_modem_ver))),
                         size_to_frame, m_reserved_version_len, framed_len);
  if (!ver_framed) {
    err_log("fail to correct modem version information");
    stor->free_buffer(buf);
    return;
  }

  memcpy(buf->buffer, ver_framed, framed_len);
  delete [] ver_framed;

  buf->dst_offset = sizeof(modem_timestamp);
  buf->data_len = framed_len;

  if (!stor->amend_current_file(buf)) {
    stor->free_buffer(buf);
    err_log("amend_current_file MODEM version error");
  }
}

void WanModemLogHandler::transaction_modem_ver_notify(void* client,
    DataConsumer::LogProcResult res) {
  WanModemLogHandler* cp = static_cast<WanModemLogHandler*>(client);

  if (CWS_QUERY_VER == cp->cp_state()) {
    if (DataConsumer::LPR_SUCCESS == res) {
      WanModemVerQuery* query = static_cast<WanModemVerQuery*>(cp->consumer());
      size_t len;
      const char* ver = query->version(len);
      str_assign(cp->m_modem_ver, ver, len);
      cp->m_ver_state = MVS_GOT;

      info_log("WAN MODEM ver: %s", ls2cstring(cp->m_modem_ver));

      LogFile* cur_file = cp->storage()->current_file();

      if (cur_file && MVSS_NOT_SAVED == cp->m_ver_stor_state) {
        // Correct the version info
        cp->correct_ver_info();
        cp->m_ver_stor_state = MVSS_SAVED;
      }
    } else {  // Failed to get version
      cp->m_ver_state = MVS_FAILED;

      err_log("query MODEM software version failed");
    }

    cp->stop_transaction(CWS_WORKING);
  } else {
    err_log("receive MODEM version query result %d under state %d, ignore",
            res, cp->cp_state());
  }
}

bool WanModemLogHandler::save_timestamp(LogFile* f) {
  time_sync ts;
  bool ret = false;
  modem_timestamp modem_ts = {0, 0, 0, 0};

  if (m_time_sync_notifier->active_get_time_sync_info(ts)) {
    m_timestamp_miss = false;
    calculate_modem_timestamp(ts, modem_ts);
  } else {
    m_timestamp_miss = true;
    err_log("Wan modem timestamp is not available.");
  }

  // if timestamp is not available, 0 will be written to the first 16 bytes.
  // otherwise, save the timestamp.
  ssize_t n = f->write_raw(&modem_ts, sizeof(uint32_t) * 4);

  if (static_cast<size_t>(n) == (sizeof(uint32_t) * 4)) {
    ret = true;
  } else {
    err_log("write timestamp fail.");
  }

  if (n > 0) {
    f->add_size(n);
  }

  return ret;
}

void WanModemLogHandler::on_new_log_file(LogFile* f) {
  save_timestamp(f);

  // MODEM version info
  uint8_t* buf = nullptr;
  // resulted length to be written
  size_t ver_len = 0;

  if (MVS_GOT == m_ver_state) {
    buf = frame_version_info(reinterpret_cast<uint8_t*>(const_cast<char*>(
                                 ls2cstring(m_modem_ver))),
                             m_modem_ver.length(), 0, ver_len);
    m_ver_stor_state = MVSS_SAVED;
  } else {
    info_log("no MODEM version when log file is created");

    buf = frame_noversion_and_reserve(ver_len);
    m_reserved_version_len = ver_len;
    m_ver_stor_state = MVSS_NOT_SAVED;
  }

  ssize_t nwr = f->write_raw(buf, ver_len);
  delete [] buf;

  if (nwr > 0) {
    f->add_size(nwr);
  }
}

void WanModemLogHandler::notify_modem_time_update(void* client,
    const time_sync& ts) {
  WanModemLogHandler* wan = static_cast<WanModemLogHandler*>(client);

  if (!wan->enabled()) {
    return;
  }

  CpStorage* stor = wan->storage();

  if (stor->current_file()) {
    if (wan->m_timestamp_miss) {
      modem_timestamp modem_ts;

      wan->calculate_modem_timestamp(ts, modem_ts);
      // Amend the current log file with the correct time stamp.
      DataBuffer* buf = stor->get_buffer();

      if (buf) {
        memcpy(buf->buffer, &modem_ts, sizeof modem_ts);
        buf->data_len = sizeof modem_ts;
        buf->dst_offset = 0;
        if (stor->amend_current_file(buf)) {
          wan->m_timestamp_miss = false;
          info_log("WAN MODEM time stamp is corrected.");
        } else {
          stor->free_buffer(buf);
          err_log("amend_current_file error");
        }
      } else {
        err_log("no buffer to correct WAN MODEM time alignment info");
      }
    } else {
      err_log("New Time sync notification but current log file have timestamp.");
    }
  } else {
    info_log("No modem log file when modem time sync info is received.");
  }
}

void WanModemLogHandler::calculate_modem_timestamp(const time_sync& ts,
    modem_timestamp& mp) {
  struct timeval time_now;
  struct sysinfo sinfo;

  gettimeofday(&time_now, 0);
  sysinfo(&sinfo);

  mp.magic_number = 0x12345678;
  mp.sys_cnt = ts.sys_cnt + (sinfo.uptime - ts.uptime) * 1000;
  mp.tv_sec = time_now.tv_sec + get_timezone_diff();
  mp.tv_usec = time_now.tv_usec;
}

void WanModemLogHandler::process_assert(bool save_md) {
  if (m_time_sync_notifier) {
    m_time_sync_notifier->clear_time_sync_info();
  }

  LogPipeHandler::process_assert(save_md);
}

int WanModemLogHandler::start() {
  if (!start_logging()) {
    storage()->subscribe_media_change_event(notify_media_change, this);
    // Start WCDMA I/Q if enabled
    if (m_save_wcdma_iq) {
      if (m_wcdma_iq_mgr.start(storage())) {
        err_log("start saving WCDMA I/Q error");
      }
    }

    if (!m_time_sync_notifier) {
      m_time_sync_notifier = new WanModemTimeSync(controller(), multiplexer());
      m_time_sync_notifier->subscribe_cp_time_update_evt(this,
          notify_modem_time_update);
      m_time_sync_notifier->init();
    }

    // Start version query
    if (MVS_NOT_BEGIN == m_ver_state || MVS_FAILED == m_ver_state) {
      if (!start_ver_query()) {
        m_ver_state = MVS_QUERY;
      } else {
        err_log("start MODEM version query error");

        m_ver_state = MVS_FAILED;
      }
    }

    StorageManager& smgr = stor_mgr();
    MediaStorage* mstor = smgr.get_media_stor();
    if (mstor) {
      save_log_parsing_lib(*mstor);
    }
    return 0;
  } else {
    return -1;
  }
}

int WanModemLogHandler::stop() {
  storage()->unsubscribe_media_change_event(this);
  // If WCDMA I/Q is started, stop it.
  if (m_wcdma_iq_mgr.is_started()) {
    if (m_wcdma_iq_mgr.stop()) {
      err_log("stop saving WCDMA I/Q error");
    }
  }
  // Stop MODEM version query
  if (MVS_QUERY == m_ver_state) {
    WanModemVerQuery* query = static_cast<WanModemVerQuery*>(consumer());

    query->stop();
    stop_transaction(CWS_WORKING);
  }

  return stop_logging();
}

int WanModemLogHandler::enable_wcdma_iq() {
  // We need to reboot to turn on the /dev/iq_mem device, so we
  // only set the m_save_wcdma_iq member here.
  m_save_wcdma_iq = true;
  return LCR_SUCCESS;
}

int WanModemLogHandler::disable_wcdma_iq() {
  if (!m_save_wcdma_iq) {
    return LCR_SUCCESS;
  }

  int ret = LCR_SUCCESS;

  if (enabled()) {
    if (!m_wcdma_iq_mgr.stop()) {
      m_save_wcdma_iq = false;
    } else {
      ret = LCR_ERROR;
    }
  } else {
    m_save_wcdma_iq = false;
  }
  return ret;
}

int WanModemLogHandler::pause() {
  if (m_wcdma_iq_mgr.is_started()) {
    m_wcdma_iq_mgr.pause();
    m_wcdma_iq_mgr.pre_clear();
  }

  return 0;
}

int WanModemLogHandler::resume() {
  if (m_wcdma_iq_mgr.is_started()) {
    m_wcdma_iq_mgr.resume();
  }

  return 0;
}

void WanModemLogHandler::process_alive() {
  // Call base class first
  LogPipeHandler::process_alive();

  if (enabled() &&
      (MVS_NOT_BEGIN == m_ver_state || MVS_FAILED == m_ver_state)) {
    if (!start_ver_query()) {
      m_ver_state = MVS_QUERY;
    } else {
      err_log("start MODEM version query error");

      m_ver_state = MVS_FAILED;
    }
  }
}

void WanModemLogHandler::stop_transaction(CpWorkState state) {
  if (MVS_QUERY == m_ver_state) {
    WanModemVerQuery* query = static_cast<WanModemVerQuery*>(consumer());
    query->stop();

    m_ver_state = MVS_FAILED;
  }

  LogPipeHandler::stop_transaction(state);
}

void WanModemLogHandler::notify_media_change(void* client,
                                             StorageManager::MediaType type) {
  WanModemLogHandler* wan = static_cast<WanModemLogHandler*>(client);
  StorageManager& sm = wan->stor_mgr();
  MediaStorage* ms = sm.get_media_stor();

  if (ms) {
    wan->save_log_parsing_lib(*ms);
  }
}

int WanModemLogHandler::check_cur_parsing_lib(const LogString& lib_name,
                                              const LogString& sha1_name,
                                              uint8_t* old_sha1,
                                              size_t& old_len) {
  struct stat file_stat;

  if (lstat(ls2cstring(lib_name), &file_stat)) {
    err_log("lstat(%s) error", ls2cstring(lib_name));
    return -1;
  }

  int sha1_fd = ::open(ls2cstring(sha1_name), O_RDONLY);

  if (-1 == sha1_fd) {
    err_log("open(%s) error", ls2cstring(sha1_name));
    return -1;
  }

  ssize_t read_len = read(sha1_fd, old_sha1, 40);
  ::close(sha1_fd);

  int ret;

  if (40 == read_len) {
    old_len = static_cast<size_t>(file_stat.st_size);
    ret = 0;
  } else {
    err_log("read SHA-1 file error: %d", static_cast<int>(read_len));
    ret = -1;
  }

  return ret;
}

int WanModemLogHandler::get_image_path(CpType ct, char* img_path, size_t len) {
  char path_prefix[PROPERTY_VALUE_MAX];

  property_get("ro.product.partitionpath", path_prefix, "");
  if (!path_prefix[0]) {
    err_log("no ro.product.partitionpath property");
    return -1;
  }

  char prop[32];
  const char* prop_suffix;

  switch (ct) {
    case CT_WCDMA:
      prop_suffix = "w.nvp";
      break;
    case CT_TD:
      prop_suffix = "t.nvp";
      break;
    case CT_3MODE:
      prop_suffix = "tl.nvp";
      break;
    case CT_4MODE:
      prop_suffix = "lf.nvp";
      break;
    default:  // CT_5MODE
      prop_suffix = "l.nvp";
      break;
  }
  snprintf(prop, sizeof prop, "%s%s", PERSIST_MODEM_CHAR, prop_suffix);

  char path_mid[PROPERTY_VALUE_MAX];

  property_get(prop, path_mid, "");
  if (!path_mid[0]) {
    err_log("no %s property", prop);
    return -1;
  }

  snprintf(img_path, len, "%s%smodem", path_prefix, path_mid);

  return 0;
}

int WanModemLogHandler::open_modem_img() {
  CpType ct = LogConfig::get_wan_modem_type();

  if (CT_UNKNOWN == ct) {
    err_log("can not get MODEM type");
    return -1;
  }

  char img_path[128];

  if (get_image_path(ct, img_path, sizeof img_path)) {
    err_log("can not get MODEM image file path");
    return -1;
  }

  return ::open(img_path, O_RDONLY);
}

int WanModemLogHandler::get_modem_img(int img_fd, uint8_t* new_sha1,
                                      size_t& lib_offset, size_t& lib_len) {

#ifdef SECURE_BOOT_ENABLE
  if (SECURE_BOOT_SIZE != lseek(img_fd, SECURE_BOOT_SIZE, SEEK_SET)) {
    err_log("lseek secure boot fail.");
    return -1;
  }
#endif

  // There are two headers at least
  uint8_t buf[24];
  ssize_t n = read(img_fd, buf, 24);

  if (24 != n) {
    err_log("MODEM image length less than two headers");
    return -1;
  }

  if (memcmp(buf, "SCI1", 4)) {
    err_log("MODEM image not SCI1 format");
    return -1;
  }

  bool found = false;

  while (true) {
    if (2 == buf[12]) {
      if (buf[13] & 4) {
        found = true;
      } else {
        err_log("MODEM image has no SHA-1 checksum");
      }
      break;
    }
    if (buf[13] & 1) {
      break;
    }
    n = read(img_fd, buf + 12, 12);
    if (12 != n) {
      err_log("can not read more headers %d", static_cast<int>(n));
      break;
    }
  }

  if (found) {
    // Offset
    uint32_t offset = buf[16];

    offset += (static_cast<uint32_t>(buf[17]) << 8);
    offset += (static_cast<uint32_t>(buf[18]) << 16);
    offset += (static_cast<uint32_t>(buf[19]) << 24);
    lib_offset = offset + 20;
#ifdef SECURE_BOOT_ENABLE
    lib_offset += SECURE_BOOT_SIZE;
#endif

    // Length
    uint32_t val = buf[20];
    val += (static_cast<uint32_t>(buf[21]) << 8);
    val += (static_cast<uint32_t>(buf[22]) << 16);
    val += (static_cast<uint32_t>(buf[23]) << 24);
    lib_len = val - 20;

    // Get SHA-1 checksum
    if (static_cast<off_t>(offset) == lseek(img_fd, offset, SEEK_SET)) {
      uint8_t sha1[20];

      n = read(img_fd, sha1, 20);
      if (20 != n) {
        found = false;
        err_log("read SHA-1 error %d", static_cast<int>(n));
      } else {
        data2HexString(new_sha1, sha1, 20);
      }
    } else {
      err_log("lseek MODEM image error");
    }
  }

  return found ? 0 : -1;
}

int WanModemLogHandler::save_log_lib(const LogString& lib_name,
                                     const LogString& sha1_name, int modem_part,
                                     size_t offset, size_t len,
                                     const uint8_t* sha1) {
  if (static_cast<off_t>(offset) != lseek(modem_part, offset, SEEK_SET)) {
    err_log("lseek MODEM image file %u error", static_cast<unsigned>(offset));
    return -1;
  }

  int log_lib_file = ::open(ls2cstring(lib_name), O_CREAT | O_WRONLY | O_TRUNC,
                            S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

  if (-1 == log_lib_file) {
    err_log("open %s failed", ls2cstring(lib_name));
    return -1;
  }

  int ret = copy_file_seg(modem_part, log_lib_file, len);

  ::close(log_lib_file);
  if (ret < 0) {
    err_log("save parsing lib error");
    unlink(ls2cstring(lib_name));
    return ret;
  }

  int sha1_file = ::open(ls2cstring(sha1_name), O_CREAT | O_WRONLY | O_TRUNC,
                         S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

  if (-1 == sha1_file) {
    err_log("create SHA-1 file error");
    unlink(ls2cstring(lib_name));
    return -1;
  }

  ssize_t wr_len = write(sha1_file, sha1, 40);

  ::close(sha1_file);
  if (40 != wr_len) {
    err_log("write SHA-1 file error %d", static_cast<int>(wr_len));
    unlink(ls2cstring(sha1_name));
    unlink(ls2cstring(lib_name));
    ret = -1;
  } else {
    ret = 0;
  }

  return ret;
}

int WanModemLogHandler::save_log_parsing_lib(MediaStorage& mstor) {
  uint8_t old_sha1[40];
  size_t old_len;
  const LogString& log_dir = mstor.get_top_dir();
  const LogString lib_name = log_dir + "/modem_db.gz";
  const LogString sha1_name = log_dir + "/sha1.txt";
  int ret = check_cur_parsing_lib(lib_name, sha1_name, old_sha1, old_len);
  int modem_part = open_modem_img();

  if (-1 == modem_part) {
    err_log("open MODEM image failed");
    return -1;
  }

  uint8_t new_sha1[40];
  size_t lib_offset;
  size_t lib_len;
  int modem_img_ret = get_modem_img(modem_part, new_sha1, lib_offset, lib_len);

  if (modem_img_ret) {  // Get parsing lib offset and length failed
    ::close(modem_part);
    err_log("get MODEM image info error");
    return -1;
  }

  bool is_same = false;

  if (!ret && (old_len == lib_len) && !memcmp(old_sha1, new_sha1, 40)) {
    is_same = true;
  }

  if (!is_same) {  // Different parsing lib
    ret = save_log_lib(lib_name, sha1_name, modem_part, lib_offset, lib_len,
                       new_sha1);
  } else {
    ret = 0;
  }

  ::close(modem_part);

  return ret;
}
