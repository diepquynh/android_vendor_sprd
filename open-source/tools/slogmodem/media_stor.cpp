/*
 *  media_stor.cpp - storage manager for one media.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-5-14 Zhang Ziyi
 *  Initial version.
 */

#include <cstdlib>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "cp_dir.h"
#include "cp_set_dir.h"
#include "def_config.h"
#include "file_watcher.h"
#include "media_stor.h"
#include "stor_mgr.h"

MediaStorage::MediaStorage(StorageManager* stor_mgr)
    : m_stor_mgr{stor_mgr},
      m_inited{false},
      m_limit{0},
      m_file_limit{DEFAULT_EXT_LOG_SIZE_LIMIT},
      m_overwrite{true},
      m_cur_set{0},
      m_size{0},
      m_file_watcher{0} {}

MediaStorage::~MediaStorage() { clear_ptr_container(m_log_dirs); }

int MediaStorage::init(FileWatcher* fw) {
  m_file_watcher = fw;

  if ((m_stor_mgr->is_modem_log_in_slog()) &&
      access(ls2cstring(m_parent_dir), F_OK)) {
    int err =
        mkdir(ls2cstring(m_parent_dir), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (err && EEXIST != errno) {
      return -1;
    }
  }

  int err = mkdir(ls2cstring(m_log_dir), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

  if (err && EEXIST != errno) {
    return -1;
  }

  DIR* pd = opendir(ls2cstring(m_log_dir));
  if (!pd) {
    return -1;
  }

  while (true) {
    struct dirent* dent = readdir(pd);
    if (!dent) {
      break;
    }
    if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, "..")) {
      continue;
    }
    struct stat file_stat;
    LogString path = m_log_dir + "/" + dent->d_name;
    if (!stat(ls2cstring(path), &file_stat) && S_ISDIR(file_stat.st_mode)) {
      CpSetDirectory* cp_set = new CpSetDirectory(this, path);
      if (!cp_set->stat()) {
        insert_ascending(m_log_dirs, cp_set);
        m_size += cp_set->size();
      } else {
        delete cp_set;
      }
    }
  }

  closedir(pd);

  m_inited = true;

  return 0;
}

void MediaStorage::stor_media_vanished() {
  clear_ptr_container(m_log_dirs);
  m_cur_set = 0;
  m_size = 0;
  m_inited = false;
}

void MediaStorage::insert_ascending(LogList<CpSetDirectory*>& lst,
                                    CpSetDirectory* cp_set) {
  LogList<CpSetDirectory*>::iterator it;

  for (it = lst.begin(); it != lst.end(); ++it) {
    CpSetDirectory* p = *it;
    if (*cp_set < *p) {
      break;
    }
  }

  lst.insert(it, cp_set);
}

CpSetDirectory* MediaStorage::create_cp_set() {
  time_t t = time(0);
  struct tm lt;

  if (static_cast<time_t>(-1) == t || !localtime_r(&t, &lt)) {
    return 0;
  }
  char ts[32];
  snprintf(ts, sizeof ts, "/%04d-%02d-%02d-%02d-%02d-%02d", lt.tm_year + 1900,
           lt.tm_mon + 1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);
  LogString spath = m_log_dir + ts;
  CpSetDirectory* cp_set = new CpSetDirectory(this, spath);
  if (cp_set->create()) {
    delete cp_set;
    cp_set = 0;
    err_log("create CP set %s failed", ls2cstring(spath));
  } else {
    m_log_dirs.push_back(cp_set);
    m_cur_set = cp_set;
  }

  return cp_set;
}

void MediaStorage::add_size(size_t len) { m_size += len; }

void MediaStorage::dec_size(size_t len) { m_size -= len; }

void MediaStorage::stop() {
  if (m_cur_set) {
    m_cur_set->stop();
    m_cur_set = 0;
  }
}

int MediaStorage::check_quota() {
  if (!m_limit || m_size <= m_limit) {
    return 0;
  }

  if (!m_overwrite) {
    return -1;
  }

  info_log("data large: %u/%uM", static_cast<unsigned>(m_size),
           static_cast<unsigned>(m_limit >> 20));

  trim(m_size - m_limit);

  return m_size > m_limit ? -1 : 0;
}

int MediaStorage::process_disk_full(CpDirectory* cp_dir) {
  if (!m_overwrite) {
    return -1;
  }
  uint64_t dec_size = trim(m_file_limit);
  return dec_size >= (m_file_limit >> 1) ? 0 : -1;
}

uint64_t MediaStorage::trim(uint64_t sz) {
  // Log too large, make some room.
  LogList<CpSetDirectory*>::iterator it = m_log_dirs.begin();
  uint64_t total_dec = 0;

  while (it != m_log_dirs.end()) {
    CpSetDirectory* cp_set = *it;
    if (cp_set == m_cur_set) {
      ++it;
      continue;
    }
    uint64_t dec_size = cp_set->trim(sz);
    total_dec += dec_size;
    if (cp_set->empty()) {
      it = m_log_dirs.erase(it);
      cp_set->remove();
      delete cp_set;
    } else {
      ++it;
    }
    if (dec_size >= sz) {
      sz = 0;
      break;
    }
    sz -= dec_size;
  }

  m_size -= total_dec;
  if (!sz) {
    return total_dec;
  }

  // Trim the current CP set directory
  if (m_cur_set) {
    uint64_t dec_size = m_cur_set->trim_working_dir(sz);
    total_dec += dec_size;
    m_size -= dec_size;
  }

  return total_dec;
}

LogFile* MediaStorage::create_file(const LogString& cp_name,
                                   LogFile::LogType t,
                                   bool& new_cp_dir,
                                   const LogString& fname) {
  LogFile* f = 0;

  if (prepare_cur_set()) {
    if (LogFile::LT_LOG == t) {
      f = m_cur_set->recreate_log_file(cp_name, new_cp_dir);
    } else {
      f = m_cur_set->create_file(cp_name, fname, t);
    }
  }

  return f;
}

void MediaStorage::clear() {
  clear_ptr_container(m_log_dirs);
  m_cur_set = 0;
  m_size = 0;

  LogString cmd("rm -fr ");
  cmd += m_log_dir;
  cmd += "/[0-9]*";
  system(ls2cstring(cmd));
}

LogFile* MediaStorage::recreate_log_file(const LogString& cp_name,
    bool& new_cp_dir) {
  LogFile* f = 0;
  if (prepare_cur_set()) {
    f = m_cur_set->recreate_log_file(cp_name, new_cp_dir);
  }

  return f;
}

CpDirectory* MediaStorage::current_cp_dir(const LogString& name) {
  CpDirectory* cp_dir = 0;

  if (m_cur_set) {
    cp_dir = m_cur_set->get_cp_dir(name);
  }

  return cp_dir;
}

CpSetDirectory* MediaStorage::prepare_cur_set() {
  // If modem log is located in slog, check if slog accessible
  if ((m_stor_mgr->is_modem_log_in_slog())
      && access(ls2cstring(m_parent_dir), F_OK)) {

    int err = mkdir(ls2cstring(m_parent_dir),
                    S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    if (err && EEXIST != errno) {
      err_log("create parent dir error");
      return 0;
    }
  }

  // Check the top directory (modem_log)
  if (access(ls2cstring(m_log_dir), R_OK | W_OK | X_OK)) {
    // The modem_log does not exist
    m_stor_mgr->proc_working_dir_removed(this);
    clear_ptr_container(m_log_dirs);
    m_cur_set = 0;
    m_size = 0;
    if (mkdir(ls2cstring(m_log_dir), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
      err_log("create top dir error");
      return 0;
    }
  }

  CpSetDirectory* cp_set = 0;

  // Check the working directory
  if (!m_cur_set) {
    cp_set = create_cp_set();
  } else if (access(ls2cstring(m_cur_set->path()), R_OK | W_OK | X_OK)) {
    m_stor_mgr->proc_working_dir_removed(this);
    ll_remove(m_log_dirs, m_cur_set);
    m_size -= m_cur_set->size();
    delete m_cur_set;
    m_cur_set = 0;

    cp_set = create_cp_set();
  } else {
    cp_set = m_cur_set;
  }

  return cp_set;
}
