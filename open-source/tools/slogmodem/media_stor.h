/*
 *  media_stor.h - storage manager for one media.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-5-14 Zhang Ziyi
 *  Initial version.
 */
#ifndef _MEDIA_STOR_H_
#define _MEDIA_STOR_H_

#include "cp_log_cmn.h"
#include "log_file.h"

class StorageManager;
class CpDirectory;
class CpSetDirectory;
class FileWatcher;

class MediaStorage {
 public:
  MediaStorage(StorageManager* stor_mgr, const LogString& top_dir);
  explicit MediaStorage(StorageManager* stor_mgr);
  ~MediaStorage();

  void set_parent_dir_path(const LogString& parent_dir) {
    m_parent_dir = parent_dir;
    m_log_dir = parent_dir + "/modem_log";
  }

  void set_root_dir_path(const LogString& root_dir_path) {
    m_root_dir = root_dir_path;
  }

  void set_total_limit(uint64_t lim) { m_limit = lim; }
  uint64_t total_limit() const { return m_limit; }
  void set_file_size_limit(size_t sz) { m_file_limit = sz; }
  size_t file_size_limit() const { return m_file_limit; }
  void set_overwrite(bool ow) { m_overwrite = ow; }
  bool overwrite() const { return m_overwrite; }

  /*  init - initialize the MediaStorage object when the media is
   *         present.
   *  @fw: the FileWatcher object.
   *
   *  Return 0 on success, -1 on failure.
   */
  int init(FileWatcher* fw);
  bool inited() const { return m_inited; }

  /*  stor_media_vanished - reset all the parameters when one of the
   *                        media file system is removed in the system.
   */
  void stor_media_vanished();

  StorageManager* stor_mgr() { return m_stor_mgr; }

  FileWatcher* file_watcher() { return m_file_watcher; }

  CpSetDirectory* current_cp_set() { return m_cur_set; }

  CpDirectory* current_cp_dir(const LogString& name);

  /*  create_cp_set - create a new CP set directory and make it the
   *                  current CP set directory.
   *
   *  This function will create new CP set directory and new CP
   *  directory.
   *
   *  Return CpDirectory pointer on success, 0 on failure.
   */
  CpSetDirectory* create_cp_set();

  /*  recreate_log_file - recreate the log file after it's deleted.
   *  @cp_name: the CP name
   *  @new_cp_dir: whether the CP directory is newly created.
   */
  LogFile* recreate_log_file(const LogString& cp_name, bool& new_cp_dir);

  // Non-log file
  LogFile* create_file(const LogString& cp_name, LogFile::LogType t,
                       bool& new_cp_dir, const LogString& fname = LogString(""));

  uint64_t size() const { return m_size; }
  void add_size(size_t len);
  void dec_size(size_t len);

  /*  check_quota - check quota in terms of the specified CP.
   *
   *  Return 0 if there is space for CP, -1 if there is no
   *  space for more log.
   */
  int check_quota();

  /*  process_disk_full - process the disk full problem.
   *  @cp_dir: the CP directory
   *
   *  Return 0 if there is space for CP, -1 if there is no
   *  space for more log.
   */
  int process_disk_full(CpDirectory* cp_dir);

  /*  stop - stop the media storage.
   *
   *  This function is called when we stop using the media for logging.
   *  The caller shall be sure all CpStorages' have stopped using the
   *  storage media.
   */
  void stop();

  void clear();

  const LogString& get_root_dir_path() const { return m_root_dir; }

  const LogString& get_top_dir() const { return m_log_dir; }

 private:
  StorageManager* m_stor_mgr;
  bool m_inited;
  // Log size limit. 0 indicates all free space.
  uint64_t m_limit;
  // Log file size limit
  size_t m_file_limit;
  // Old log overwrite
  bool m_overwrite;
  // Modem log Storage path
  LogString m_parent_dir;
  // External Storage root path
  LogString m_root_dir;
  // Top dir on the media (the modem_log directory)
  LogString m_log_dir;
  LogList<CpSetDirectory*> m_log_dirs;
  CpSetDirectory* m_cur_set;
  uint64_t m_size;
  // File watcher
  FileWatcher* m_file_watcher;

  uint64_t trim(uint64_t sz);
  /*  prepare_cp_set - create new CP set directory for log/dump/minidump
   *                   if CP set is not available
   *
   *  Return CpDirectory pointer on success, 0 on failure.
   */
  CpSetDirectory* prepare_cur_set();

  static void insert_ascending(LogList<CpSetDirectory*>& lst,
                               CpSetDirectory* cp_set);
};

#endif  // !_MEDIA_STOR_H_
