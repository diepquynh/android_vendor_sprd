/*
 *  cp_set_dir.cpp - directory object for one run.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-5-14 Zhang Ziyi
 *  Initial version.
 */

#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include "cp_set_dir.h"
#include "media_stor.h"
#include "cp_dir.h"

CpSetDirectory::CpSetDirectory(MediaStorage* media, const LogString& dir)
    : m_media{media}, m_path(dir), m_size{0} {}

CpSetDirectory::~CpSetDirectory() { clear_ptr_container(m_cp_dirs); }

int CpSetDirectory::stat() {
  DIR* pd = opendir(ls2cstring(m_path));
  if (!pd) {
    return -1;
  }

  LogString d_path;
  while (true) {
    struct dirent* dent = readdir(pd);
    if (!dent) {
      break;
    }
    if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, "..")) {
      continue;
    }
    d_path = m_path + "/" + dent->d_name;
    struct stat file_stat;

    if (!::stat(ls2cstring(d_path), &file_stat) && S_ISDIR(file_stat.st_mode)) {
      CpDirectory* cp_dir = new CpDirectory(this, LogString(dent->d_name));
      if (!cp_dir->stat()) {
        m_cp_dirs.push_back(cp_dir);
        m_size += cp_dir->size();
      } else {
        delete cp_dir;
      }
    }
  }

  closedir(pd);

  return 0;
}

int CpSetDirectory::create() {
  int ret = mkdir(ls2cstring(m_path), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  if (-1 == ret && EEXIST == errno) {
    ret = 0;
  }
  return ret;
}

int CpSetDirectory::remove() {
  LogString cmd("rm -fr ");

  cmd += m_path;

  return system(ls2cstring(cmd));
}

CpDirectory* CpSetDirectory::prepare_cp_dir(const LogString& cp_name,
    bool& new_cp_dir) {
  CpDirectory* cp_dir = get_cp_dir(cp_name);

  if (!cp_dir) {
    cp_dir = new CpDirectory(this, cp_name);
    if (cp_dir->create()) {
      delete cp_dir;
      cp_dir = 0;
    } else {
      new_cp_dir = true;
      m_cp_dirs.push_back(cp_dir);
    }
  } else if (access(ls2cstring(m_path + "/" + cp_name), R_OK | W_OK | X_OK)) {
    // Remove the CpDirectory
    ll_remove(m_cp_dirs, cp_dir);
    m_size -= cp_dir->size();
    delete cp_dir;

    cp_dir = new CpDirectory(this, cp_name);
    if (cp_dir->create()) {
      delete cp_dir;
      cp_dir = 0;
    } else {
      new_cp_dir = true;
      m_cp_dirs.push_back(cp_dir);
    }
  }

  return cp_dir;
}

void CpSetDirectory::add_size(size_t len) {
  m_size += len;
  m_media->add_size(len);
}

void CpSetDirectory::dec_size(size_t len) {
  m_size -= len;
  m_media->dec_size(len);
}

void CpSetDirectory::stop() {
  LogList<CpDirectory*>::iterator it;

  for (it = m_cp_dirs.begin(); it != m_cp_dirs.end(); ++it) {
    CpDirectory* p = *it;
    p->stop();
  }
}

uint64_t CpSetDirectory::trim(uint64_t sz) {
  LogList<CpDirectory*>::iterator it = m_cp_dirs.begin();
  uint64_t total_dec = 0;

  while (it != m_cp_dirs.end()) {
    CpDirectory* cp_dir = *it;
    uint64_t dec_size = cp_dir->trim(sz);
    total_dec += dec_size;
    if (cp_dir->empty()) {
      it = m_cp_dirs.erase(it);
      cp_dir->remove();
      delete cp_dir;
    } else {
      ++it;
    }
    if (dec_size >= sz) {
      break;
    }
    sz -= dec_size;
  }

  m_size -= total_dec;
  return total_dec;
}

uint64_t CpSetDirectory::trim_working_dir(uint64_t sz) {
  LogList<CpDirectory*>::iterator it = m_cp_dirs.begin();
  uint64_t total_dec = 0;

  while (it != m_cp_dirs.end()) {
    CpDirectory* cp_dir = *it;
    uint64_t dec_size = cp_dir->trim_working_dir(sz);
    total_dec += dec_size;
    if (cp_dir->empty()) {
      it = m_cp_dirs.erase(it);
      cp_dir->remove();
      delete cp_dir;
    } else {
      ++it;
    }
    if (dec_size >= sz) {
      break;
    }
    sz -= dec_size;
  }

  m_size -= total_dec;
  return total_dec;
}

LogFile* CpSetDirectory::create_file(const LogString& cp_name,
                                     const LogString& fname,
                                     LogFile::LogType t) {
  bool created = false;

  // Get the CP directory
  CpDirectory* cp_dir = prepare_cp_dir(cp_name, created);
  LogFile* f = 0;

  if (cp_dir) {
    f = cp_dir->create_file(fname, t);
  }

  return f;
}

CpDirectory* CpSetDirectory::get_cp_dir(const LogString& cp_name) {
  LogList<CpDirectory*>::iterator it;
  CpDirectory* cp_dir = 0;

  for (it = m_cp_dirs.begin(); it != m_cp_dirs.end(); ++it) {
    CpDirectory* cp = *it;
    if (cp->name() == cp_name) {
      cp_dir = cp;
      break;
    }
  }

  return cp_dir;
}

LogFile* CpSetDirectory::recreate_log_file(const LogString& cp_name,
    bool& new_cp_dir) {
  LogFile* log_file = 0;
  bool created = false;
  CpDirectory* cp_dir = prepare_cp_dir(cp_name, created);

  if (cp_dir) {
    log_file = cp_dir->create_log_file();
    if (log_file) {
      new_cp_dir = created;
    }
  }

  return log_file;
}
