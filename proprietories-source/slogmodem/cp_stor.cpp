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
      m_cur_file{0},
      m_shall_stop{false},
      m_log_chan{nullptr} {
  m_log_scheduler.set_file_written_callback(this, file_wr_callback);
}

CpStorage::~CpStorage() {
  if (m_log_chan) {
    m_log_scheduler.close();
    m_log_chan->stop();
    delete m_log_chan;
  }

  if (m_cur_file) {
    m_cur_file->close();
  }
}

void CpStorage::set_log_buffer_size(size_t max_buf, size_t max_num) {
  m_log_scheduler.set_buffer_size(max_buf, max_num);
}

void CpStorage::set_log_commit_threshold(size_t size) {
  m_log_scheduler.set_commit_threshold(size);
}

DataBuffer* CpStorage::get_buffer() {
  return m_log_scheduler.get_free_buffer();
}

void CpStorage::free_buffer(DataBuffer* buf) {
  m_log_scheduler.free_buffer(buf);
}

void CpStorage::set_buf_avail_callback(void* client,
                                       IoScheduler::buffer_avail_callback_t cb) {
  m_log_scheduler.set_buf_avail_callback(client, cb);
}

int CpStorage::init() {
  int ret = 0;

  m_log_chan = new IoChannel{m_cp.controller(), m_cp.multiplexer()};
  if (m_log_chan->init()) {
    delete m_log_chan;
    m_log_chan = nullptr;
    ret = -1;
  } else {
    m_log_scheduler.init_buffer();
    m_log_scheduler.bind(m_log_chan);
  }

  return ret;
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

bool CpStorage::amend_current_file(DataBuffer* buf) {
  bool ret = false;

  if (m_cur_file) {
    m_log_scheduler.enqueue(buf);
    ret = true;
  }

  return ret;
}

int CpStorage::write(DataBuffer* data) {
  MediaStorage* ms = m_stor_mgr.get_media_stor();

  if (ms) {
    if (m_shall_stop) {  // No quota for log
      if (!ms->check_quota()) {
        m_shall_stop = false;
      }
    }
  } else {  // Not saving log currently
    if (!m_stor_mgr.check_media_change()) {  // Can not find a media to use
      err_log("no storage media available");
      return LogFile::FIO_ERROR;
    }

    ms = m_stor_mgr.get_media_stor();
    if (ms->check_quota()) {  // No quota for log
      m_shall_stop = true;
    } else {
      m_shall_stop = false;
    }
  }

  if (m_shall_stop) {  // Still no quota for log
    return LogFile::FIO_ERROR;
  }

  bool cp_dir_created{false};

  if (!m_cur_file) {  // It's not saving log now
    // First check whether the media shall be changed
    m_stor_mgr.check_media_change();
    ms = m_stor_mgr.get_media_stor();

    CpDirectory* cp_dir = ms->current_cp_dir(m_cp.name());
    if (cp_dir) {
      cp_dir->rotate();
    }

    m_cur_file = ms->create_file(m_cp.name(), LogFile::LT_LOG,
                                 cp_dir_created);
    if (!m_cur_file) {
      err_log("create log file failed");
      return LogFile::FIO_ERROR;
    }
    if (m_new_log_cb) {
      m_new_log_cb(&m_cp, m_cur_file);
    }

    m_log_scheduler.open(m_cur_file);
  }

  int ret = 0;

  if (m_log_scheduler.enqueue(data)) {
    ret = LogFile::FIO_ERROR;
  }

  return ret;
}

int CpStorage::flush() {
  int ret = 0;

  if (m_cur_file) {
    ret = m_log_scheduler.flush();
  }

  return ret;
}

void CpStorage::stop() {
  if (m_cur_file) {
    m_log_scheduler.close();
    m_cur_file->close();
    m_cur_file = nullptr;
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
    f = ms->create_file(m_cp.name(), t, created, 0, fname);
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

void CpStorage::file_wr_callback(void* client, int err) {
  CpStorage* stor = static_cast<CpStorage*>(client);

  stor->on_file_size_update(err);
}

void CpStorage::on_file_size_update(int err) {
  // 1. If the storage media is changed, switch to the new media.
  if (m_stor_mgr.check_media_change()) {
    // Media changed and current log file closed
    MediaStorage* m = m_stor_mgr.get_media_stor();

    if (m->check_quota()) {  // No quota for log
      m_shall_stop = true;
      // Discard data
      m_log_scheduler.discard_queue();
    } else {
      m_shall_stop = false;

      bool cp_dir_created{false};

      m_cur_file = m->create_file(m_cp.name(), LogFile::LT_LOG,
                                  cp_dir_created);
      if (m_cur_file) {
        m_log_scheduler.open(m_cur_file);
        if (m_new_log_cb) {
          m_new_log_cb(&m_cp, m_cur_file);
        }
      }
    }

    return;
  }

  // 2. If the storage media is not changed and the current log file is
  //    removed, recreate the log file.
  // 3. If the media is unchanged and the current file exists, then
  //   3.1 check the current file size and open a new file if necessary;
  //   3.2 check the total size on the media and remove oldest file
  //       if necessary.
  CpDirectory* cp_dir = m_cur_file->dir();
  MediaStorage* m = cp_dir->cp_set_dir()->get_media();
  bool cp_dir_created{false};

  if (m_cur_file->exists()) {
    // Check current log file size
    if (m_cur_file->size() >= m->file_size_limit()) {
      m_log_scheduler.close();
      cp_dir->close_log_file();
      m_cur_file = nullptr;

      cp_dir->rotate();
    }
    // Check total log size
    if (m->check_quota()) {
      m_shall_stop = true;
    }
  } else {  // Current log file removed
    m_log_scheduler.close();
    m_cur_file->close();
    cp_dir->file_removed(m_cur_file);
    m_cur_file = nullptr;
  }

  if (m_shall_stop) {  // No quota for log
    // Discard buffered log
    m_log_scheduler.discard_queue();
  } else if (!m_cur_file) {
    m_cur_file = m->create_file(m_cp.name(), LogFile::LT_LOG,
                                cp_dir_created);

    if (!m_cur_file) {
      err_log("recreate current log file failed");
      return;
    }
    if (m_new_log_cb) {
      m_new_log_cb(&m_cp, m_cur_file);
    }

    m_log_scheduler.open(m_cur_file);
  }
}
