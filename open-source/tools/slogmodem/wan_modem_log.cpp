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
#include <sys/types.h>

#include "client_mgr.h"
#include "cp_stor.h"
#include "diag_dev_hdl.h"
#include "log_config.h"
#include "log_ctrl.h"
#include "modem_dump.h"
#include "wan_modem_log.h"

WanModemLogHandler::WanModemLogHandler(LogController* ctrl, Multiplexer* multi,
                                       const LogConfig::ConfigEntry* conf,
                                       StorageManager& stor_mgr,
                                       const char* dump_path)
    : LogPipeHandler(ctrl, multi, conf, stor_mgr),
      m_dump_path(dump_path) {}

int WanModemLogHandler::start_dump(const struct tm& lt) {
  // Create the data consumer first
  int err = 0;
  ModemDumpConsumer* dump;
  DiagDeviceHandler* diag;

  dump = new ModemDumpConsumer(name(), *storage(), lt, m_dump_path);

  dump->set_callback(this, diag_transaction_notify);

  diag = create_diag_device(dump);
  if (diag) {
    diag->add_events(POLLIN);
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

int WanModemLogHandler::start() {
  if (!start_logging()) {
    storage()->subscribe_media_change_event(notify_media_change, this);

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

  return stop_logging();
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
