/*
 *  wcdma_iq_mgr.h - The WCDMA I/Q data manager.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-7-23 Zhang Ziyi
 *  Initial version.
 */
#ifndef WCDMA_IQ_MGR_H_
#define WCDMA_IQ_MGR_H_

#include "cp_log_cmn.h"
#include "fd_hdl.h"
#include "log_file.h"

class CpStorage;
class LogPipeHandler;

class WcdmaIqManager : public FdHandler {
 public:
  explicit WcdmaIqManager(LogPipeHandler* cp);
  ~WcdmaIqManager();

  void set_max_segs(unsigned n) { m_max_iq_segs = n; }

  bool is_started() const { return IQS_IDLE != m_state; }

  /*  start - start I/Q data saving.
   *  @cp_stor: CpStorage to use.
   *
   *  Return 0 on success, -1 on error.
   */
  int start(CpStorage* cp_stor);
  /*  stop - stop I/Q data saving and exit the thread.
   *
   *  Return 0 on success, -1 on error.
   */
  int stop();
  /*  resume - resume I/Q data saving.
   *
   *  Return 0 on success, -1 on error.
   */
  int resume();
  /*  pause - stop current I/Q data saving.
   *
   *  Return 0 on success, -1 on error.
   */
  int pause();

  /*  pre_clear - get preparation for log clearing.
   *
   *  This function MUST be called after pause().
   */
  void pre_clear();

  void process(int events);

  void process_stor_response(int resp);

 private:
  enum IqState { IQS_IDLE, IQS_WAITING, IQS_SAVING, IQS_PAUSED };

  static const unsigned SAVE_BLOCK = (1024 * 1024 * 10);

  // Saving thread notification class
  class SaveIqNotifier : public FdHandler {
   public:
    SaveIqNotifier(int fd, WcdmaIqManager* wiq);

    void process(int events);

    int send_command(int cmd);
    /*  wait_response - wait for response from the saving thread.
     *
     *  Return the response.
     */
    int wait_response();

    // Command to the I/Q storage thread
    static const int IQST_EXIT = 0;   // Exit the thread. No response.
    static const int IQST_PAUSE = 1;  // Pause saving.
    static const int IQST_SAVE = 2;   // Save I/Q buffer to file.
    // Response from the I/Q storage thread
    static const int IQST_PAUSED = 101;  // Response for IQST_PAUSE
    static const int IQST_SAVED = 102;   // Response for IQST_SAVE

   private:
    WcdmaIqManager* m_wcdma_iq;
  };

  // Data members
  LogPipeHandler* m_cp;
  CpStorage* m_cp_stor;
  IqState m_state;
  unsigned m_max_iq_segs;
  LogList<LogFile*> m_iq_files;
  // I/Q buffer mapped
  uint8_t* m_iq_buffer;
  // File for current I/Q segment
  LogFile* m_cur_seg_file;
  // Notification receiver for I/Q storage thread
  SaveIqNotifier* m_iq_stor_notifier;
  // I/Q storage thread
  pthread_t m_iq_stor_thread;
  // Pipe descriptor for the I/Q storage thread
  int m_stor_thread_sock;
  // Number of bytes saved.
  size_t m_saved_size;

  int prepare_iq_mem_map();
  int gen_iq_file_name(LogString& file_name) const;
  LogFile* create_seg_file();
  int start_saving_thread();
  /*  process_save - process the SaveIqNotifier::IQST_SAVE command.
   *
   *  This function is called from the stor_thread thread to save
   *  I/Q data to file.
   *
   *  Return true if the thread shall continue, false if the thread
   *  shall exit.
   */
  bool process_save();

  void clear_iq_files();

  static void* stor_thread(void* param);
  static int send_byte(int fd, int value);

  /*  notify_ext_stor_umount - callback when external storage umounted.
   */
  static void notify_ext_stor_umount(void* client);
};

#endif  //! WCDMA_IQ_MGR_H_
