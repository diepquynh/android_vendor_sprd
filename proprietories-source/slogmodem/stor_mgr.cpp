/*
 *  stor_mgr.cpp - storage manager.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-5-14 Zhang Ziyi
 *  Initial version.
 */

#include <climits>
#include <cstdlib>
#ifdef HOST_TEST_
#include "prop_test.h"
#else
#include <cutils/properties.h>
#endif
#include <unistd.h>

#include "cp_log_cmn.h"
#include "cp_stor.h"
#include "cp_set_dir.h"
#include "cp_dir.h"
#include "ext_storage_monitor.h"
#include "file_watcher.h"
#include "log_pipe_hdl.h"
#include "parse_utils.h"
#include "stor_mgr.h"

StorageManager::StorageManager()
    : m_cur_type{MT_NONE},
      m_data_part(this),
      m_sd_card(this),
      m_overwrite{true},
      m_file_watcher{0},
      m_set_in_ap_log_dir{0},
      m_save_in_ap_log{false},
      m_ext_stor_detect_method{SDM_AUTO},
      m_use_ext_stor_fuse{true},
      m_ext_stor_mount_state{MMS_NOT_MOUNTED},
      m_ext_stor_monitor{nullptr},
      m_major_num {UINT_MAX},
      m_minor_num {UINT_MAX} {}

StorageManager::~StorageManager() {
  // delete m_file_watcher;
  if (m_ext_stor_monitor) {
    m_ext_stor_monitor->unsubscribe_ext_storage_evt(this);
    delete m_ext_stor_monitor;
    m_ext_stor_monitor = 0;
  }
  clear_ptr_container(m_event_clients);
  clear_ptr_container(m_umt_event_clients);
}

int StorageManager::init(const LogString& extern_stor_pos, LogController* ctrl,
                         Multiplexer* multiplexer) {
  if (!str_empty(extern_stor_pos)) {
    m_mount_point = extern_stor_pos;
    m_ext_stor_detect_method = SDM_MANUAL;
  }
  // TODO: file watcher shall be implemented

  if (m_set_in_ap_log_dir) {
    m_data_part.set_parent_dir(LogString("/data") + m_set_in_ap_log_dir);
  } else {
    m_data_part.set_parent_dir(LogString("/data"));
  }

  if (m_data_part.init(m_file_watcher)) {
    goto init_fail;
  }

  m_ext_stor_monitor = new ExtStorageMonitor(this, ctrl, multiplexer);
  if (m_ext_stor_monitor->init()) {
    delete m_ext_stor_monitor;
    m_ext_stor_monitor = nullptr;
    err_log("ExtStorageMonitor init failed");
  } else {
    m_ext_stor_monitor->subscribe_ext_storage_evt(this,
                                                  notify_ext_storage_event);
  }

  // Check external storage mount state
  if (SDM_AUTO == m_ext_stor_detect_method) {
    if (check_vfat_mount(m_mount_point)) {  // SD card is present
      init_ext_stor();
      // External storage initialization failure is not fatal.
      if (m_sd_card.init(m_file_watcher)) {
        m_mount_point.clear();
        err_log("init external storage failure");
      } else {
        m_ext_stor_mount_state = MMS_MOUNTED;
      }
    }
  } else {  // External storage mount point is specified manually
    init_ext_stor();
    // External storage initialization failure is not fatal.
    if (m_sd_card.init(m_file_watcher)) {
      err_log("init external storage failure");
    } else {
      m_ext_stor_mount_state = MMS_MOUNTED;
    }
  }

  return 0;

init_fail:
  // delete m_file_watcher;
  // m_file_watcher = 0;
  return -1;
}

MediaStorage* StorageManager::get_media_stor(MediaType t) {
  MediaStorage* m;

  switch (t) {
    case MT_INTERNAL:
      m = &m_data_part;
      break;
    case MT_SD_CARD:
      m = &m_sd_card;
      break;
    default:
      m = 0;
      break;
  }

  return m;
}

MediaStorage* StorageManager::get_media_stor(const LogString& mnt_point) {
  MediaStorage* ms = nullptr;

  if (MMS_MOUNTED == m_ext_stor_mount_state &&
      m_mount_point == mnt_point) {
    ms = &m_sd_card;
  }

  return ms;
}

void StorageManager::set_overwrite(bool ow) {
  m_overwrite = ow;
  m_data_part.set_overwrite(ow);
  m_sd_card.set_overwrite(ow);
}

void StorageManager::set_file_size_limit(size_t sz) {
  m_data_part.set_file_size_limit(sz);
  m_sd_card.set_file_size_limit(sz);
}

void StorageManager::set_data_part_limit(uint64_t sz) {
  m_data_part.set_total_limit(sz);
}

void StorageManager::set_sd_limit(uint64_t sz) {
  m_sd_card.set_total_limit(sz);
}

CpStorage* StorageManager::create_storage(LogPipeHandler& cp_log,
                                          size_t max_buf,
                                          size_t max_num,
                                          size_t cmt_size) {
  CpStorage* cp_stor = new CpStorage(*this, cp_log);

  cp_stor->set_log_buffer_size(max_buf, max_num);
  cp_stor->set_log_commit_threshold(cmt_size);
  if (!cp_stor->init()) {
    m_cp_handles.push_back(cp_stor);
  } else {
    delete cp_stor;
    cp_stor = nullptr;
  }
  return cp_stor;
}

void StorageManager::delete_storage(CpStorage* stor) {
  LogList<CpStorage*>::iterator it;

  for (it = m_cp_handles.begin(); it != m_cp_handles.end(); ++it) {
    CpStorage* cp = *it;
    if (stor == cp) {
      m_cp_handles.erase(it);
      delete stor;
      break;
    }
  }
}

void StorageManager::stop_all_cps() {
  for (auto it = m_cp_handles.begin(); it != m_cp_handles.end(); ++it) {
    (*it)->stop();
  }
}

void StorageManager::subscribe_media_change_evt(void* client_ptr,
                                                stor_event_callback_t cb) {
  EventClient* c = new EventClient;
  c->client_ptr = client_ptr;
  c->cb = cb;
  m_event_clients.push_back(c);
}

void StorageManager::unsubscribe_media_change_evt(void* client_ptr) {
  for (auto it = m_event_clients.begin(); it != m_event_clients.end(); ++it) {
    EventClient* client = *it;
    if (client_ptr == client->client_ptr) {
      m_event_clients.erase(it);
      delete client;
      break;
    }
  }
}

void StorageManager::subscribe_ext_stor_umount_evt(
    void* client_ptr, ext_stor_umount_callback_t cb) {
  EventClient* c = new EventClient;
  c->client_ptr = client_ptr;
  c->cb_umnt = cb;
  m_umt_event_clients.push_back(c);
}

void StorageManager::unsubscribe_ext_stor_umount_evt(void* client_ptr) {
  for (auto it = m_umt_event_clients.begin(); it != m_umt_event_clients.end();
       ++it) {
    EventClient* client = *it;
    if (client_ptr == client->client_ptr) {
      m_umt_event_clients.erase(it);
      delete client;
      break;
    }
  }
}

void StorageManager::notify_ext_stor_umount() {
  for (auto it = m_umt_event_clients.begin(); it != m_umt_event_clients.end();
       ++it) {
    EventClient* client = *it;
    client->cb_umnt(client->client_ptr);
  }
}

void StorageManager::notify_media_change() {
  for (auto it = m_event_clients.begin(); it != m_event_clients.end(); ++it) {
    EventClient* client = *it;
    client->cb(client->client_ptr, m_cur_type);
  }
}

void StorageManager::notify_ext_storage_event(void* client,
    ExtStorageMonitor::StorageEvent event,
    unsigned major_num, unsigned minor_num) {
  StorageManager* stor_mgr = static_cast<StorageManager*>(client);

  // If the current media is unmounted, stop writing files to it.
  switch (event) {
    case ExtStorageMonitor::EVT_ADD:
      // The major and minor number is unknown now, just change the state
      if (MMS_MOUNTED != stor_mgr->m_ext_stor_mount_state) {
        stor_mgr->m_ext_stor_mount_state = MMS_MEDIA_INSERTED;
      }
      break;
    case ExtStorageMonitor::EVT_REMOVE:
      if (stor_mgr->m_major_num == major_num &&
          stor_mgr->m_minor_num == minor_num) {
        stor_mgr->m_ext_stor_mount_state = MMS_MEDIA_REMOVED;
        if (&stor_mgr->m_sd_card == stor_mgr->get_media_stor()) {
            stor_mgr->stop_all_cps();
        }
      }
      break;
    default:
      break;
  }
}

int StorageManager::try_use_sd_card() {
  if (!m_sd_card.inited()) {
    init_ext_stor();
    if (m_sd_card.init(m_file_watcher)) {
      err_log("init SD error");
      return -1;
    }
  }

  return m_sd_card.create_cp_set() ? 0 : -1;
}

bool StorageManager::check_media_change() {
  bool changed = false;
  bool sd_present = get_sd_state();

  switch (m_cur_type) {
    case MT_NONE:
      if (sd_present) {  // Try external media
        if (!try_use_sd_card()) {  // Use SD card
          info_log("switch to SD");
          m_cur_type = MT_SD_CARD;
          changed = true;
        } else if (m_data_part.create_cp_set()) {
          m_cur_type = MT_INTERNAL;
          changed = true;
        }
      } else {  // Use internal media
        if (m_data_part.create_cp_set()) {
          m_cur_type = MT_INTERNAL;
          changed = true;
        }
      }
      break;
    case MT_INTERNAL:
      if (sd_present) {  // Try external media
        if (!try_use_sd_card()) {  // Try to use SD card
          info_log("switch to SD");
          m_cur_type = MT_SD_CARD;
          changed = true;
          stop_all_cps();
        }
      }
      break;
    default:  // Current media: external
      if (!sd_present) {
        info_log("SD card is unmounted!");
        notify_ext_stor_umount();
        stop_all_cps();
        m_sd_card.stor_media_vanished();

        // Create CP set directory
        m_data_part.create_cp_set();

        changed = true;
        m_cur_type = MT_INTERNAL;
      }
      break;
  }

  if (changed) {
    notify_media_change();
  }

  return changed;
}

void StorageManager::clear() {
  stop_all_cps();
  m_data_part.clear();

  if (MT_SD_CARD == m_cur_type) {
    m_sd_card.clear();
  } else {  // Current media is not external SD
    clear_ext_sd();
  }
  // Reset current media to MT_NONE so that the next request will
  // recreate the CP set directory on the appropriate media.
  m_cur_type = MT_NONE;
}

void StorageManager::clear_ext_sd() {
  LogString mnt_point;

  if (check_vfat_mount(mnt_point)) {
    if (m_set_in_ap_log_dir) {
      mnt_point += m_set_in_ap_log_dir;
    }
    mnt_point += "/modem_log/[0-9]*";

    LogString cmd{"rm -fr "};

    cmd += mnt_point;
    system(ls2cstring(cmd));
  }
}

void StorageManager::proc_working_dir_removed(MediaStorage* ms) {
  if (get_media_stor() == ms) {
    stop_all_cps();
  }
}

void StorageManager::init_ext_stor() {
  // Set SD card mount point and parent dir for modem_log
  m_sd_card.set_root_dir(m_mount_point);
  if (m_set_in_ap_log_dir) {
    m_sd_card.set_parent_dir(m_mount_point + m_set_in_ap_log_dir);
  } else {
    m_sd_card.set_parent_dir(m_mount_point);
  }
}

bool StorageManager::check_vfat_mount(LogString& sd_root) {
  FILE* pf;

  pf = fopen("/proc/mounts", "r");
  if (!pf) {
    err_log("can't open /proc/mounts");
    return false;
  }

  bool sd_present = false;
  char mnt_line[1024];

  while (fgets(mnt_line, 1024, pf)) {
    // Skip the first field
    size_t tlen;
    const uint8_t* tok =
        get_token(reinterpret_cast<const uint8_t*>(mnt_line), tlen);
    if (!tok) {
      continue;
    }

    if (get_major_minor_in_mount(tok, tlen)) {
      continue;
    }

    size_t path_len;
    const char* mnt_point = reinterpret_cast<const char*>(
        get_token(reinterpret_cast<const uint8_t*>(tok + tlen), path_len));
    if (!mnt_point) {
      continue;
    }

    tok =
        get_token(reinterpret_cast<const uint8_t*>(mnt_point + path_len), tlen);

    if (tok && ((4 == tlen && !memcmp(tok, "vfat", 4)) ||
                (5 == tlen && !memcmp(tok, "exfat", 5)))) {
      LogString vfat_path;

      str_assign(vfat_path, mnt_point, path_len);

      sd_present = !access(ls2cstring(vfat_path), R_OK | W_OK | X_OK);
      if (sd_present) {
        sd_root = vfat_path;
        info_log("sd card is mounted at path: %s.", ls2cstring(sd_root));
      }

      break;
    }
  }

  if (m_use_ext_stor_fuse) {  // Use FUSE mounts
    if (sd_present) {  // VFAT/exFAT file system got
      sd_present = false;
      // Search for the FUSE mount point under /storage
      while (fgets(mnt_line, 1024, pf)) {
        // Skip the first field
        size_t tlen;
        const uint8_t* tok =
            get_token(reinterpret_cast<const uint8_t*>(mnt_line), tlen);
        if (!tok) {
          continue;
        }

        // The mount point
        tok += tlen;
        tok = get_token(reinterpret_cast<const uint8_t*>(tok), tlen);
        if (!tok) {
          continue;
        }
        if (tlen <= 8 || memcmp(tok, "/storage/", 9) ||
            memchr(tok + 9, '/', tlen - 9)) {
          continue;
        }

        // File system type
        const uint8_t* fs_type;
        size_t fs_len;

        fs_type = get_token(reinterpret_cast<const uint8_t*>(tok + tlen),
                            fs_len);
        if (!fs_type || fs_len != 4 || memcmp(fs_type, "fuse", 4)) {
          continue;
        }

        str_assign(sd_root, reinterpret_cast<const char*>(tok), tlen);
        sd_present = true;
        info_log("ext SD fuse path %s", ls2cstring(sd_root));
        break;
      }
    }
  }

  fclose(pf);

  return sd_present;
}

bool StorageManager::get_sd_state() {
  bool sd_present;

  if (SDM_AUTO == m_ext_stor_detect_method) {
    switch (m_ext_stor_mount_state) {
      case MMS_NOT_MOUNTED:
      case MMS_MEDIA_INSERTED:
      case MMS_FS_MOUNTED:
        // Check whether the media is accessible
        if (check_vfat_mount(m_mount_point) &&
            !access(ls2cstring(m_mount_point), R_OK | W_OK | X_OK)) {
          m_ext_stor_mount_state = MMS_MOUNTED;
        }
        break;
      case MMS_MOUNTED:
        if (access(ls2cstring(m_mount_point), R_OK | W_OK | X_OK)) {
          m_ext_stor_mount_state = MMS_NOT_MOUNTED;
        }
        break;
      //case MMS_MEDIA_REMOVED:
      //case MMS_START_UNMOUNT:
      default:
        break;
    }

    sd_present = (MMS_MOUNTED == m_ext_stor_mount_state);
  } else {  // Mount point specified manually
    sd_present = !access(ls2cstring(m_mount_point), R_OK | W_OK | X_OK);
  }

  return sd_present;
}

void StorageManager::process_unmount(MediaStorage* ms) {
  if (ms == &m_sd_card) {
    m_ext_stor_mount_state = MMS_START_UNMOUNT;
  }
  if (ms == get_media_stor()) {
    stop_all_cps();
  }
}
