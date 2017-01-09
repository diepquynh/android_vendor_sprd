/*
 *  cp_stor.h - CP storage handle.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-5-15 Zhang Ziyi
 *  Initial version.
 */
#ifndef _CP_STOR_H_
#define _CP_STOR_H_

#include "cp_log_cmn.h"
#include "io_sched.h"
#include "log_file.h"
#include "stor_mgr.h"

struct DataBuffer;
class LogPipeHandler;

class CpStorage {
 public:
  typedef void (*new_log_callback_t)(LogPipeHandler*, LogFile* f);

  CpStorage(StorageManager& stor_mgr, LogPipeHandler& cp);
  ~CpStorage();

  LogFile* current_file() {
    return m_cur_file;
  }
  void set_log_buffer_size(size_t max_buf, size_t max_num);
  void set_log_commit_threshold(size_t size);

  /* init - initialize the I/O channel.
   *
   * Return: 0 on success, -1 on failure.
   */
  int init();

  DataBuffer* get_buffer();
  void free_buffer(DataBuffer* buf);
  void set_buf_avail_callback(void* client,
                              IoScheduler::buffer_avail_callback_t cb);

  /* amend_current_file - amend the data to the offset posiotion.
   * @buf - data to be witten.
   *
   * Return: true if succeed.
   */
  bool amend_current_file(DataBuffer* buf);

  /*  write - enqueue the data block.
   *  @data: data to write
   *
   *  This function is called to save log. It will:
   *    1. if there is no log file currently, create the log file.
   *    2. enqueue the data to IoScheduler (m_log_scheduler).
   *
   *  Return Value:
   *    0  the data is queued for writing
   *    LogFile::FIO_ERROR  unknown error
   *    LogFile::FIO_DISK_FULL  disk full
   */
  int write(DataBuffer* data);
  /*  flush - flush current log file.
   *
   *  Return 0 on success, -1 on error.
   */
  int flush();

  void stop();

  void set_new_log_callback(new_log_callback_t cb) { m_new_log_cb = cb; }

  // Non-log files
  LogFile* create_file(const LogString& name, LogFile::LogType t,
                       StorageManager::MediaType mt = StorageManager::MT_NONE);

  void subscribe_media_change_event(StorageManager::stor_event_callback_t cb,
                                    void* client);

  void unsubscribe_media_change_event(void* client);

  /*  subscribe_ext_stor_umount_evt - subscribe the service of external storage
   *                               umount event.
   *
   *  @client_ptr - non log file clients:
   *                sleep log/ringbuffer/minidump/memory dump
   *  @cb - callback how the client handle the event of umount
   */
  void subscribe_ext_stor_umount_evt(void* client,
                                     StorageManager::ext_stor_umount_callback_t cb);

  /*  unsubscribe_ext_stor_umount_evt - unsubscribe the service of external
   *                                    storage umount.
   *  @client_ptr - non log file clients:
   *                sleep log/ringbuffer/minidump/memory dump
   */
  void unsubscribe_ext_stor_umount_evt(void* client);

  /*  check_quota - check quota.
   *
   *  Return Value:
   *    Return 0 if there is more storage capacity, -1 otherwise.
   */
  int check_quota(StorageManager::MediaType mt = StorageManager::MT_NONE);

 private:
  StorageManager& m_stor_mgr;
  LogPipeHandler& m_cp;
  new_log_callback_t m_new_log_cb;
  // Current log file
  LogFile* m_cur_file;
  // Log capacity state
  bool m_shall_stop;
  // I/O scheduler for current log
  IoScheduler m_log_scheduler;
  // I/O thread for current log
  IoChannel* m_log_chan;

  /*  on_file_size_update - check quota.
   *  @err: the error code of the last write. May be one of LogFile::FIO_xxx.
   *
   *  This function is called after a batch of data blocks has been written
   *  into the file, and the next batch of data blocks has not been committed.
   *  This function will do:
   *    1. If the storage media is changed, switch to the new media.
   *    2. If the storage media is not changed and the current log file is
   *       removed, recreate the log file.
   *    3. If the media is unchanged and the current file exists, then
   *       3.1 check the current file size and open a new file if necessary;
   *       3.2 check the total size on the media and remove oldest file
   *           if necessary.
   */
  void on_file_size_update(int err);

  static void file_wr_callback(void* client, int err);
};

#endif  // !_CP_STOR_H_
