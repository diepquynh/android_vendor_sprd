/*
 *  stor_mgr.cpp - storage manager.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-5-14 Zhang Ziyi
 *  Initial version.
 */

#include <cstdlib>
#ifdef HOST_TEST_
#include "prop_test.h"
#else
#include <cutils/properties.h>
#endif

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
      m_save_in_slog{false},
      m_ext_stor_event(EVT_NONE),
      m_ext_stor_monitor{0},
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

int StorageManager::init(LogString extern_stor_pos, LogController* ctrl,
                         Multiplexer* multiplexer) {
  m_mount_point = extern_stor_pos;
  // TODO: file watcher shall be implemented

  if (is_modem_log_in_slog()) {
    m_data_part.set_parent_dir_path(LogString("/data/slog"));
  } else {
    m_data_part.set_parent_dir_path(LogString("/data"));
  }

  if (m_data_part.init(m_file_watcher)) {
    goto init_fail;
  }

  if (get_sd_state()) {  // SD card is present
    init_ext_stor();
    // External storage initialization failure is not fatal.
    if (m_sd_card.init(m_file_watcher)) {
      err_log("init external storage failure");
    }
  }

  m_ext_stor_monitor = new ExtStorageMonitor(this, ctrl, multiplexer);

  if (m_ext_stor_monitor->init()) {
    delete m_ext_stor_monitor;
    m_ext_stor_monitor = 0;
    err_log("Netlink socket init fail.");
    return -1;
  } else {
    m_ext_stor_monitor->subscribe_ext_storage_evt(this,
                                                  notify_ext_storage_change);
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

CpStorage* StorageManager::create_storage(LogPipeHandler& cp_log) {
  CpStorage* cp_stor = new CpStorage(*this, cp_log);

  m_cp_handles.push_back(cp_stor);
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
  LogList<CpStorage*>::iterator it;

  for (it = m_cp_handles.begin(); it != m_cp_handles.end(); ++it) {
    CpStorage* stor = *it;
    stor->stop();
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

void StorageManager::notify_ext_storage_change(void* client,
    ExtStorEvent event, unsigned major_num, unsigned minor_num) {
  StorageManager* stor_mgr = static_cast<StorageManager*>(client);

  if ((static_cast<int>(stor_mgr->m_major_num) == major_num)
      && (static_cast<int>(stor_mgr->m_minor_num) == minor_num)
      && (stor_mgr->m_ext_stor_event != event)) {
    stor_mgr->m_ext_stor_event = event;
    stor_mgr->stop_all_cps();
  }
}

bool StorageManager::check_media_change() {
  bool changed = false;

  if (get_sd_state()) {// SD is present
    if (MT_SD_CARD != m_cur_type) {
      info_log("Storage changing to external SD");

      if (!m_sd_card.inited()) {
        init_ext_stor();
        if (m_sd_card.init(m_file_watcher)) {
          err_log("init SD card failed");

          // Init external storage failed.
          // Use internal storage.
          if (MT_INTERNAL != m_cur_type) {
            if (m_data_part.create_cp_set()) {
              m_cur_type = MT_INTERNAL;
              changed = true;

              notify_media_change();
            }
          }
          return changed;
        }
      }

      // Create CP set directory
      if (!m_sd_card.create_cp_set()) {
        // Create external storage directory failed.
        // Use internal storage.
        if (MT_INTERNAL != m_cur_type) {
          if (m_data_part.create_cp_set()) {
            m_cur_type = MT_INTERNAL;
            changed = true;

            notify_media_change();
          }
        }
        return changed;
      }

      if (MT_INTERNAL == m_cur_type) {
        stop_all_cps();
      }

      changed = true;
      m_cur_type = MT_SD_CARD;
    }
  } else {  // No SD card
    if (MT_INTERNAL != m_cur_type) {
      if (MT_SD_CARD == m_cur_type) {
        info_log("sd card is umounted!");
        notify_ext_stor_umount();
        stop_all_cps();
        m_sd_card.stor_media_vanished();
      }

      // Create CP set directory
      m_data_part.create_cp_set();

      changed = true;
      m_cur_type = MT_INTERNAL;
    }
  }

  if (changed) {
    notify_media_change();
  }

  return changed;
}

void StorageManager::clear() {
  stop_all_cps();
  m_data_part.clear();
  m_sd_card.clear();
  // Reset current media to MT_NONE so that the next request will
  // recreate the CP set directory on the appropriate media.
  m_cur_type = MT_NONE;
}

void StorageManager::proc_working_dir_removed(MediaStorage* ms) {
  if (get_media_stor() == ms) {
    stop_all_cps();
  }
}

void StorageManager::init_ext_stor() {
  // Set SD card mount point and parent dir for modem_log
  m_sd_card.set_root_dir_path(m_mount_point);
  if (is_modem_log_in_slog()) {
    m_sd_card.set_parent_dir_path(m_mount_point + "/slog");
    info_log("CP log saved to slog/modem_log");
  } else {
    m_sd_card.set_parent_dir_path(m_mount_point);
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

    if (tok && 4 == tlen && !memcmp(tok, "vfat", 4)) {
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
  fclose(pf);

  return sd_present;
}

bool StorageManager::get_sd_state() {
  bool sd_present = false;

  if (ExtStorEvent::EVT_RMD == m_ext_stor_event) {
    return sd_present;
  }

  if (!str_empty(m_mount_point)) {
    if (!access(m_mount_point, R_OK | W_OK | X_OK)) {
      sd_present = true;
    }
  } else {
    sd_present = check_vfat_mount(m_mount_point);
  }

  return sd_present;
}
