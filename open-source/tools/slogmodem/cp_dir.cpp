/*
 *  cp_dir.cpp - directory object for one CP in one run.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-5-14 Zhang Ziyi
 *  Initial version.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include "cp_dir.h"
#include "cp_set_dir.h"
#include "log_file.h"
#include "media_stor.h"

CpDirectory::CpDirectory(CpSetDirectory* par_dir, const LogString& dir)
    : m_cp_set_dir{par_dir},
      m_name(dir),
      m_size{0},
      m_start_log{0},
      m_cur_log{0},
      m_log_watch{0} {}

CpDirectory::~CpDirectory() {
  // TODO: cancel file watch
  // cancel_watch();
  clear_ptr_container(m_log_files);
}

int CpDirectory::stat() {
  LogString path = m_cp_set_dir->path() + "/" + m_name;
  DIR* pd = opendir(ls2cstring(path));

  if (!pd) {
    return -1;
  }

  struct dirent* dent;

  path += "/";
  while (true) {
    dent = readdir(pd);
    if (!dent) {
      break;
    }
    if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, "..")) {
      continue;
    }
    LogString file_path = path + dent->d_name;
    struct stat file_stat;
    if (!::stat(ls2cstring(file_path), &file_stat) &&
        S_ISREG(file_stat.st_mode)) {
      LogFile* log = new LogFile(LogString(dent->d_name), this,
                                 LogFile::LT_UNKNOWN, file_stat.st_size);
      log->get_type();
      insert_ascending(m_log_files, log);
      m_size += file_stat.st_size;
    }
  }

  closedir(pd);

  return 0;
}

void CpDirectory::insert_ascending(LogList<LogFile*>& lst, LogFile* f) {
  switch (f->type()) {
    case LogFile::LT_VERSION:
      lst.push_back(f);
      break;
    case LogFile::LT_UNKNOWN:
      lst.push_front(f);
      break;
    default:
      LogList<LogFile*>::iterator it;
      for (it = lst.begin(); it != lst.end(); ++it) {
        LogFile* p = *it;
        if (LogFile::LT_VERSION == p->type()) {
          break;
        }
        if (LogFile::LT_UNKNOWN != p->type()) {
          if (*f < *p) {
            break;
          }
        }
      }
      lst.insert(it, f);
      break;
  }
}

int CpDirectory::create() {
  LogString s = m_cp_set_dir->path() + "/" + m_name;
  int ret = mkdir(ls2cstring(s), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  if (-1 == ret && EEXIST == errno) {
    ret = 0;
  }

  return ret;
}

LogFile* CpDirectory::create_log_file() {
  time_t t = time(0);
  struct tm lt;

  if (static_cast<time_t>(-1) == t || !localtime_r(&t, &lt)) {
    return 0;
  }

  char s[64];
  snprintf(s, sizeof s, "0-%s-%04d-%02d-%02d_%02d-%02d-%02d.log",
           ls2cstring(m_name), lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday,
           lt.tm_hour, lt.tm_min, lt.tm_sec);
  LogFile* log_file = new LogFile(LogString(s), this, lt);
  if (log_file->create()) {
    delete log_file;
    log_file = 0;
  } else {
    if (!get_file_type_num(m_log_files, LogFile::LT_LOG)) {
      m_start_log = log_file;
    }
    m_log_files.push_back(log_file);
    m_cur_log = log_file;
    // TODO: Watch the current log
    // FileWatcher* fw = cp_set_dir()->get_media()->file_watcher();
    // fw->add(this, log_delete_notify, s, m_log_watch);
  }

  return log_file;
}

int CpDirectory::close_log_file() {
  m_cur_log->close();
  m_cur_log = 0;
  // TODO: Cancel file watch
  // cancel_watch();
  return 0;
}

void CpDirectory::add_size(size_t len) {
  m_size += len;
  m_cp_set_dir->add_size(len);
}

void CpDirectory::dec_size(size_t len) {
  m_size -= len;
  m_cp_set_dir->dec_size(len);
}

void CpDirectory::stop() {
  if (m_cur_log) {
    m_cur_log->close();
    m_cur_log = 0;
  }
}

void CpDirectory::rotate() {
  LogList<LogFile*>::iterator it;
  LogString spath;

  spath = m_cp_set_dir->path() + "/" + m_name;
  for (it = m_log_files.begin(); it != m_log_files.end(); ++it) {
    LogFile* f = *it;
    if (LogFile::LT_LOG == f->type()) {
      f->rotate(spath);
    }
  }
}

uint64_t CpDirectory::trim(uint64_t sz) {
  LogList<LogFile*>::iterator it = m_log_files.begin();
  uint64_t total_dec = 0;
  LogString par_dir = m_cp_set_dir->path() + "/" + m_name;

  while (it != m_log_files.end()) {
    LogFile* f = *it;
    if (f->remove(par_dir)) {
      err_log("delete %s error", ls2cstring(f->base_name()));
      break;
    }
    it = m_log_files.erase(it);
    size_t dec_size = f->size();
    delete f;
    total_dec += dec_size;
    if (dec_size >= sz) {
      break;
    }
    sz -= dec_size;
  }

  m_size -= total_dec;

  return total_dec;
}

int CpDirectory::remove() {
  LogString spath;

  str_assign(spath, "rm -fr ", 7);
  spath += (m_cp_set_dir->path() + "/" + m_name);
  return system(ls2cstring(spath));
}

int CpDirectory::remove(LogFile* rmf) {
  LogList<LogFile*>::iterator it;
  int ret = -1;

  for (it = m_log_files.begin(); it != m_log_files.end(); ++it) {
    LogFile* f = *it;
    if (rmf == f) {
      m_log_files.erase(it);
      LogString spath = m_cp_set_dir->path() + "/" + m_name;
      f->remove(spath);
      dec_size(f->size());
      delete f;
      ret = 0;
      break;
    }
  }

  return ret;
}

uint64_t CpDirectory::trim_working_dir(uint64_t sz) {
  LogList<LogFile*>::iterator it = m_log_files.begin();
  uint64_t total_dec = 0;
  LogString par_dir = m_cp_set_dir->path() + "/" + m_name;

  while (it != m_log_files.end()) {
    LogFile* f = *it;
    if (f == m_start_log || f == m_cur_log ||
        LogFile::LT_VERSION == f->type() || LogFile::LT_WCDMA_IQ == f->type()) {
      ++it;
      continue;
    }
    if (f->remove(par_dir)) {
      err_log("delete %s error", ls2cstring(f->base_name()));
      break;
    }
    it = m_log_files.erase(it);
    size_t dec_size = f->size();
    delete f;
    total_dec += dec_size;
    if (dec_size >= sz) {
      break;
    }
    sz -= dec_size;
  }

  m_size -= total_dec;

  // Rename the starting log
  rename_start_log();

  return total_dec;
}

int CpDirectory::get_file_type_num(const LogList<LogFile*>& lst,
                                   LogFile::LogType type) {
  LogList<LogFile*>::const_iterator it;
  int num = 0;

  for (it = lst.begin(); it != lst.end(); ++it) {
    const LogFile* f = *it;
    if (type == f->type()) {
      ++num;
    }
  }

  return num;
}

int CpDirectory::rename_start_log() {
  if (!m_start_log) {
    return 0;
  }

  LogList<LogFile*>::iterator it;

  for (it = m_log_files.begin(); it != m_log_files.end(); ++it) {
    LogFile* f = *it;
    if (f == m_start_log) {
      continue;
    }
    if (LogFile::LT_LOG == f->type()) {
      break;
    }
  }

  int ret = -1;

  if (it != m_log_files.end()) {
    LogFile* f = *it;
    const LogString& name = f->base_name();
    size_t len;
    unsigned num;

    if (!LogFile::parse_log_file_num(ls2cstring(name), name.length(), num,
                                     len)) {
      ++num;
      ret = m_start_log->change_name_num(num);
    }
  }

  return ret;
}

LogFile* CpDirectory::create_file(const LogString& fname, LogFile::LogType t) {
  LogFile* log_file = new LogFile(fname, this, t);
  if (log_file->create()) {
    delete log_file;
    log_file = 0;
  } else {
    m_log_files.push_back(log_file);
  }

  return log_file;
}

void CpDirectory::cancel_watch() {
  if (m_log_watch) {
    FileWatcher* fw = cp_set_dir()->get_media()->file_watcher();
    if (fw) {
      fw->del(m_log_watch);
    }
    m_log_watch = 0;
  }
}

void CpDirectory::log_delete_notify(void* client, uint32_t evt) {
  CpDirectory* cp_dir = static_cast<CpDirectory*>(client);

  if ((evt & IN_DELETE_SELF) && cp_dir->m_log_watch) {
    // TODO: inform the StorageManager
    // cp_dir->cp_set_dir()->get_media()->stor_mgr()->
    cp_dir->close_log_file();
  }
}

void CpDirectory::file_removed(LogFile* f) {
  LogList<LogFile*>::iterator it;
  bool found = false;

  for (it = m_log_files.begin(); it != m_log_files.end(); ++it) {
    LogFile* pf = *it;
    if (f == pf) {
      m_log_files.erase(it);
      found = true;
      break;
    }
  }

  if (found) {
    m_size -= f->size();
    if (f == m_start_log) {
      m_start_log = 0;
    }
    delete f;
  } else {
    err_log("log %s does not exist in dir %s", ls2cstring(f->base_name()),
            ls2cstring(m_name));
  }
}
