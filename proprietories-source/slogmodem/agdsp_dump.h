/*
 *  agdsp_dump.h - The AG-DSP dump class.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-6-1 Zhang Ziyi
 *  Initial version.
 */
#ifndef AGDSP_DUMP_H_
#define AGDSP_DUMP_H_

#include <cstddef>
#include <cstdint>
#include "fd_hdl.h"
#include "log_file.h"
#include "timer_mgr.h"

class AgDspLogHandler;
class CpStorage;

class AgdspDump : public FdHandler {
 public:
  enum Dump_State { DS_IDLE, DS_WAITING, DS_DUMPING, DS_SUCCESS, DS_FAIL };

  AgdspDump(LogController* ctrl, Multiplexer* multiplexer,
            const char* dump_dev);
  ~AgdspDump();

  typedef void (*dump_event_callback)(void* client);
  typedef void (*dump_result_notifier)(void* client, Dump_State ds);

  void set_dump_start_callback(void* client, dump_event_callback callback);
  void set_dump_result_notifier(void* client, dump_result_notifier callback);

  int start();
  int stop();
  int wait_dump();

  void process(int events);
  void end_dump();
  void start_dump(CpStorage& stor, const LogString& name);

 private:
  static const size_t AGDSP_DUMP_BUF_SIZE = 1024 * 32;

  class DumpStoreNotifier : public FdHandler {
   public:
    DumpStoreNotifier(int fd, AgdspDump* dump);

    int send_dump_cmd(int cmd);
    int wait_response();
    void process(int events);

    // command to the saving thread
    static const int START_DUMP = 0;
    static const int STOP_DUMP = 1;

    // response from the saving thread
    static const int DUMP_SAVED = 10;
    static const int DUMP_STOPPED = 11;
    static const int DUMP_FAILED = 21;

   private:
    AgdspDump* m_dump;
  };

  LogString m_dump_device;
  DumpStoreNotifier* m_notifier;
  pthread_t m_save_thread;
  // Mutex for sync between main thread and agdsp_dump thread
  pthread_mutex_t m_lock;
  TimerManager::Timer* m_timer;
  // The socket descriptor used by the dump saving thread (agdsp_dump)
  int m_sock_fd;
  LogFile* m_dump_file;
  Dump_State m_state;
  size_t m_saved_size;
  void* m_event_client;
  dump_event_callback m_event_callback;
  void* m_result_client;
  dump_result_notifier m_result_notifier;

  int start_saving_thread();
  void process_stor_response(int resp);
  int save_dump();

  static void reopen_dump_dev(void* param);
  static void* agdsp_dump(void* param);
  static int send_byte(int fd, int value);
};

#endif  // !AGDSP_DUMP_H_
