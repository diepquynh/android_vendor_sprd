/*
 *  cp_stor.cpp - CP storage handle.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-5-15 Zhang Ziyi
 *  Initial version.
 */

#include "cp_dir.h"
#include "cp_set_dir.h"
#include "cp_stor.h"
#include "log_file.h"
#include "log_pipe_hdl.h"
#include "stor_mgr.h"

CpStorage::CpStorage(StorageManager& stor_mgr, LogPipeHandler& cp)
    : m_stor_mgr(stor_mgr),
      m_cp(cp),
      m_new_log_cb{0},
      m_new_dir_cb{0},
      m_cur_file{0},
      m_shall_stop{false} {}

CpStorage::~CpStorage() {
  if (m_cur_file) {
    m_cur_file->close();
  }
}

void CpStorage::subscribe_media_change_event(
    StorageManager::stor_event_callback_t cb, void* client) {
  m_stor_mgr.subscribe_media_change_evt(client, cb);
}

void CpStorage::unsubscribe_media_change_event(void* client) {
  m_stor_mgr.unsubscribe_media_change_evt(client);
}

void CpStorage::subscribe_ext_stor_umount_evt(
    void* client, StorageManager::ext_stor_umount_callback_t cb) {
  m_stor_mgr.subscribe_ext_stor_umount_evt(client, cb);
}

void CpStorage::unsubscribe_ext_stor_umount_evt(void* client) {
  m_stor_mgr.unsubscribe_ext_stor_umount_evt(client);
}

ssize_t CpStorage::write(const void* data, size_t len) {
  if (m_stor_mgr.check_media_change()) {
    if (m_cur_file) {
      m_cur_file->close();
      m_cur_file = 0;
    }
  }

  bool cp_dir_created = false;

  if (!m_cur_file) {  // Log file not created

    MediaStorage* m = m_stor_mgr.get_media_stor();
    CpDirectory* cp_dir = m->current_cp_dir(m_cp.name());
    if (cp_dir) {
      cp_dir->rotate();
    }

    m_cur_file = m->create_file(m_cp.name(), LogFile::LT_LOG, cp_dir_created);
    if (!m_cur_file) {
      return -1;
    }
    if (cp_dir_created && m_new_dir_cb) {
      m_new_dir_cb(&m_cp);
    }
    if (m_new_log_cb) {
      m_new_log_cb(&m_cp, m_cur_file);
    }

  } else {  // Log file opened
    // Check whether the file exists
    CpDirectory* cp_dir = m_cur_file->dir();
    MediaStorage* m = m_cur_file->dir()->cp_set_dir()->get_media();

    if (!m_cur_file->exists()) {
      m_cur_file->close();
      cp_dir->file_removed(m_cur_file);
      m_cur_file = m->create_file(m_cp.name(), LogFile::LT_LOG, cp_dir_created);

      if (!m_cur_file) {
        return -1;
      }
      if (cp_dir_created && m_new_dir_cb) {
        m_new_dir_cb(&m_cp);
      }
      if (m_new_log_cb) {
        m_new_log_cb(&m_cp, m_cur_file);
      }
    }
  }

  MediaStorage* ms = m_stor_mgr.get_media_stor();
  // It's full on last time write. Try to make more room.
  if (m_shall_stop) {
    if (ms->check_quota()) {
      return -1;
    }
    m_shall_stop = false;
  }

  ssize_t n = m_cur_file->write(data, len);

  if (n > 0) {
    if (m_cur_file->size() >= ms->file_size_limit()) {
      CpDirectory* cp_dir = m_cur_file->dir();
      cp_dir->close_log_file();
      m_cur_file = 0;
    }

    if (ms->check_quota()) {
      m_shall_stop = true;
      if (m_cur_file) {
        m_cur_file->flush();
      }
    }
  } else if (-2 == n) {  // Disk space full
    if (ms->process_disk_full(m_cur_file->dir())) {
      m_shall_stop = true;
      m_cur_file->flush();
    }
  }

  return n;
}

int CpStorage::flush() {
  int ret = 0;

  if (m_cur_file) {
    ret = m_cur_file->flush();
  }

  return ret;
}

void CpStorage::stop() {
  if (m_cur_file) {
    m_cur_file->close();
    m_cur_file = 0;
  }
}

LogFile* CpStorage::create_file(const LogString& fname, LogFile::LogType t,
                                StorageManager::MediaType mt) {
  LogFile* f = 0;
  MediaStorage* ms = 0;
  bool created = false;

  if (StorageManager::MT_NONE == mt) {
    m_stor_mgr.check_media_change();
    ms = m_stor_mgr.get_media_stor();
  } else {
    ms = m_stor_mgr.get_media_stor(mt);
  }

  if (ms) {
    f = ms->create_file(m_cp.name(), t, created, fname);
  }

  return f;
}

int CpStorage::check_quota(StorageManager::MediaType mt) {
  MediaStorage* ms;

  if (StorageManager::MT_NONE == mt) {
    ms = m_stor_mgr.get_media_stor();
  } else {
    ms = m_stor_mgr.get_media_stor(mt);
  }
  int ret = 0;

  if (ms) {
    ret = ms->check_quota();
  }

  return ret;
}
