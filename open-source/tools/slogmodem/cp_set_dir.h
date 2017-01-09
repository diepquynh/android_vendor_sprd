/*
 *  cp_set_dir.h - directory object for one run.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-5-14 Zhang Ziyi
 *  Initial version.
 */
#ifndef _CP_SET_DIR_H_
#define _CP_SET_DIR_H_

#include "cp_log_cmn.h"
#include "log_file.h"

class MediaStorage;
class CpDirectory;

class CpSetDirectory {
 public:
  /*  CpSetDirectory - constructor
   *  @media: MediaStorage pointer
   *  @dir: full path of the CP set directory
   *
   */
  CpSetDirectory(MediaStorage* media, const LogString& dir);
  ~CpSetDirectory();

  const LogString& path() const { return m_path; }

  MediaStorage* get_media() { return m_media; }

  bool empty() const { return m_cp_dirs.empty(); }

  uint64_t size() const { return m_size; }

  /*  stat - collect file statistics of the directory.
   *
   *  Return 0 on success, -1 on failure.
   */
  int stat();

  bool operator<(const CpSetDirectory& d) const { return m_path < d.m_path; }

  /*  create - create the CP set directory.
   *
   *  Return 0 on success, -1 on failure.
   */
  int create();

  /*  remove - remove the CP set directory.
   *
   *  Return 0 on success, -1 on failure.
   */
  int remove();

  CpDirectory* get_cp_dir(const LogString& name);

  LogFile* create_file(const LogString& cp_name, const LogString& fname,
                       LogFile::LogType t);

  /*  recreate_log_file - recreate the log file after it's deleted.
   *  @cp_name: the CP name.
   *
   */
  LogFile* recreate_log_file(const LogString& cp_name, bool& new_cp_dir);

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

 private:
  MediaStorage* m_media;
  // Full path of the directory
  LogString m_path;
  LogList<CpDirectory*> m_cp_dirs;
  uint64_t m_size;

  /* prepare_cp_dir - prepare cp directory for log/minidump/ememdump
   * @cp_name - CP name (wcn/wan modem... ...)
   * @new_cp_dir - if a new cp directory is created.
   *
   * Return cp directory.
   */
  CpDirectory* prepare_cp_dir(const LogString& cp_name, bool& new_cp_dir);

};

#endif  // !_CP_SET_DIR_H_
