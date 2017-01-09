/*
 *  cp_dir.h - directory object for one CP in one run.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-5-14 Zhang Ziyi
 *  Initial version.
 */
#ifndef _CP_DIR_H_
#define _CP_DIR_H_

#include "cp_log_cmn.h"
#include "log_file.h"
#include "file_watcher.h"

class CpSetDirectory;

class CpDirectory {
 public:
  CpDirectory(CpSetDirectory* par_dir, const LogString& dir);
  ~CpDirectory();

  const CpSetDirectory* cp_set_dir() const { return m_cp_set_dir; }
  CpSetDirectory* cp_set_dir() { return m_cp_set_dir; }

  const LogString& name() const { return m_name; }

  uint64_t size() const { return m_size; }

  bool empty() const { return m_log_files.empty(); }

  /*  stat - collect file statistics of the directory.
   *
   *  Return 0 on success, -1 on failure.
   */
  int stat();

  /*  create - create the directory.
   *
   *  Return 0 on success, -1 on failure.
   */
  int create();

  /*  remove - remove the directory.
   *
   *  Return 0 on success, -1 on failure.
   */
  int remove();

  /*  remove - remove file if it's in the file list. Update the size.
   *
   *  Return 0 on success, -1 on failure.
   */
  int remove(LogFile* f);

  /*  create_file - create new log file.
   *
   *  Return LogFile pointer on success, 0 on failure.
   */
  LogFile* create_log_file();
  /*  close_log_file - close current log file.
   *
   *  Return 0 on success, -1 on failure.
   */
  int close_log_file();

  // Non-log file
  LogFile* create_file(const LogString& fname, LogFile::LogType t);

  /*  file_removed - inform the CpDirectory object of the file removal.
   *  @f: the LogFile that has been removed.
   *
   *  This function shall be called after the f is closed.
   *  This function is to be cleaned up.
   */
  void file_removed(LogFile* f);

  void add_size(size_t len);
  void dec_size(size_t len);

  uint64_t trim(uint64_t sz);
  /*  trim_working_dir - trim the working directory.
   *  @sz: the size to trim.
   *
   *  This function won't delete the starting log and the current log.
   *
   *  Return 0 on success, -1 on failure.
   */
  uint64_t trim_working_dir(uint64_t sz);

  void stop();

  void rotate();

 private:
  CpSetDirectory* m_cp_set_dir;
  LogString m_name;
  // Log size of the directory
  uint64_t m_size;
  // Starting log in this run. For a history directory, this is 0.
  LogFile* m_start_log;
  // Log list, including the starting log file (the first one)
  LogList<LogFile*> m_log_files;
  // Current log file
  LogFile* m_cur_log;
  // File watch on current log
  FileWatcher::FileWatch* m_log_watch;

  int rename_start_log();
  void cancel_watch();

  static void insert_ascending(LogList<LogFile*>& lst, LogFile* f);
  static int get_file_type_num(const LogList<LogFile*>& lst,
                               LogFile::LogType type);
  /*  log_delete_notify - current log file deletion notification
   *                      function.
   *  @client: pointer to the CpDirectory object
   *  @evt: the file events, represented in IN_xxx macros
   *
   */
  static void log_delete_notify(void* client, uint32_t evt);
};

#endif  // !_CP_DIR_H_
