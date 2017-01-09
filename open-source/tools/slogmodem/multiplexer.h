/*
 *  multiplexer.h - The multiplexer declaration.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-2-16 Zhang Ziyi
 *  Initial version.
 */
#ifndef _MULTIPLEXER_H_
#define _MULTIPLEXER_H_

#include <poll.h>
#include "cp_log_cmn.h"
#include "fd_hdl.h"
#include "timer_mgr.h"

class Multiplexer {
 public:
  Multiplexer();

  /*
   * register_fd - Register a new handler, or modify an existing
   *               handler events.
   *
   * Return Value:
   *   Return 0 if successful, return -1 if the handler array is
   *   full.
   */
  int register_fd(FdHandler* handler, int events);
  void unregister_fd(FdHandler* handler, int events);

  typedef void (*check_callback_t)(void* param);

  TimerManager& timer_mgr() { return m_timer_mgr; }

  int run();

 private:
#define MAX_MULTIPLEXER_NUM 20

  struct PollingEntry {
    FdHandler* handler;
    int events;
  };

  LogVector<PollingEntry> m_polling_hdl;
  bool m_dirty;
  nfds_t m_current_num;
  pollfd m_current_fds[MAX_MULTIPLEXER_NUM];
  FdHandler* m_current_handlers[MAX_MULTIPLEXER_NUM];

  // Timer manager
  TimerManager m_timer_mgr;

  // Find handler
  size_t find_handler(FdHandler* handler);
  void prepare_polling_array();
  void call_check_callback();
};

#endif  // !_MULTIPLEXER_H_
