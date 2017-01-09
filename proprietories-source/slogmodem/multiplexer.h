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
  ~Multiplexer();

  /*
   * register_fd - Register a new handler
   *
   * Return Value:
   *   Return 0 if successful, return -1 if the handler array is
   *   full.
   */
  int register_fd(FdHandler* handler, int events);
  void unregister_fd(FdHandler* handler);

  /*  add_events - add events for a registered handler.
   *  @handler: the FdHandler object pointer
   *  @events: events to add
   *
   *  Return 0 on success, -1 on error.
   */
  int add_events(FdHandler* handler, int events);
  /*  del_events - delete events for a registered handler.
   *  @handler: the FdHandler object pointer
   *  @events: events to delete
   *
   *  Return 0 on success, -1 on error.
   */
  int del_events(FdHandler* handler, int events);

  TimerManager& timer_mgr() { return m_timer_mgr; }

  int run();

  void shall_quit() { m_run = false; }

 private:
  static const unsigned POLL_ARRAY_INC_STEP = 32;

  struct PollingEntry {
    FdHandler* handler;
    int events;
  };

  bool m_run;
  LogVector<PollingEntry> m_polling_hdl;
  bool m_dirty;
  size_t m_poll_arr_size;
  nfds_t m_current_num;
  pollfd* m_current_fds;
  FdHandler** m_current_handlers;

  // Timer manager
  TimerManager m_timer_mgr;

  // Find handler
  size_t find_handler(FdHandler* handler);
  void prepare_polling_array();
  void call_check_callback();
  void remove_cur_handler(FdHandler* handler);
  void clear_cur_events(FdHandler* handler, int events);
};

#endif  // !_MULTIPLEXER_H_
