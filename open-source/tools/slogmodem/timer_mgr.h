/*
 *  timer_mgr.h - Timer manager class declaration.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-3-2 Zhang Ziyi
 *  Initial version.
 */
#ifndef _TIMER_MGR_H_
#define _TIMER_MGR_H_

#include <time.h>

#include "cp_log_cmn.h"

class TimerManager {
 public:
  typedef void (*timer_callback)(void* param);

  struct Timer {
    struct timespec due_time;
    timer_callback cb;
    void* param;
  };

  ~TimerManager();

  /*  add_timer - create an one-shot timer and add it to the timer
   *              list.
   *  @interval: timer interval in millisecond.
   *  @cb: timer callback function.
   *  @param: parameter to pass to the callback function.
   *
   *  Return 0 on success, -1 on error.
   */
  int add_timer(unsigned interval, timer_callback cb, void* param);

  /*  add_timer - create an one-shot timer and add it to the timer
   *              list.
   *  @interval: timer interval in millisecond.
   *  @cb: timer callback function.
   *  @param: parameter to pass to the callback function.
   *
   *  Return the timer object pointer on success, NULL on error.
   */
  Timer* create_timer(unsigned interval, timer_callback cb, void* param);

  /*  del_timer - delete a timer.
   *  @cb: the timer callback function.
   */
  void del_timer(timer_callback cb);

  void del_timer(Timer* t);

  /*  set_new_due_time - update the due time of the timer.
   *  @t: the Timer pointer
   *  @interval: the new due time shall be set to the current
   *             time plus interval.
   *
   *  Return 0 on success, -1 on error.
   */
  int set_new_due_time(Timer* t, unsigned interval);

  /*  run - process the timers.
   *
   *  To save computing power, the function will not query system time
   *  (by means of system call) if there is no timer in the timer
   *  list.
   */
  void run();

  /*  next_time - get the time (in millisecond) between now and the
   *              next due time.
   *  @time_span: the function will put the time span (in millisecond)
   *              between now and the next due time in the variable.
   *
   *  Return 0 on success, -1 on error.
   */
  int next_time(int& time_span) const;

 private:
  LogList<Timer*> m_timers;

  LogList<Timer*>::iterator find(Timer* t);
  void insert_timer(Timer* pt);
  void clear();
};

timespec operator+(const timespec&, int);
int operator-(const timespec&, const timespec&);
bool operator<(const timespec&, const timespec&);

#endif  // !_TIMER_MGR_H_
