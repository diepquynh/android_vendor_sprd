/*
 *  timer_mgr.cpp - Timer manager class.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-3-2 Zhang Ziyi
 *  Initial version.
 */
#include "timer_mgr.h"

TimerManager::~TimerManager() { clear(); }

void TimerManager::clear() {
  LogList<Timer*>::iterator it;

  for (it = m_timers.begin(); it != m_timers.end(); ++it) {
    delete *it;
  }

  m_timers.clear();
}

int TimerManager::add_timer(unsigned interval, timer_callback cb, void* param) {
  struct timespec tnow;

  if (-1 == clock_gettime(CLOCK_BOOTTIME, &tnow)) {
    err_log("clock_gettime CLOCK_BOOTTIME error");
    return -1;
  }
  Timer* p = new Timer;

  p->due_time = tnow + interval;
  p->cb = cb;
  p->param = param;

  insert_timer(p);
  return 0;
}

TimerManager::Timer* TimerManager::create_timer(unsigned interval,
                                                timer_callback cb,
                                                void* param) {
  struct timespec tnow;

  if (-1 == clock_gettime(CLOCK_BOOTTIME, &tnow)) {
    return 0;
  }
  Timer* p = new Timer;

  p->due_time = tnow + interval;
  p->cb = cb;
  p->param = param;

  insert_timer(p);
  return p;
}

void TimerManager::insert_timer(Timer* pt) {
  LogList<Timer*>::iterator it;

  for (it = m_timers.begin(); it != m_timers.end(); ++it) {
    if (pt->due_time < (*it)->due_time) {
      break;
    }
  }

  m_timers.insert(it, pt);
}

void TimerManager::del_timer(timer_callback cb) {
  LogList<Timer*>::iterator it = m_timers.begin();

  while (it != m_timers.end()) {
    Timer* pt = *it;

    if (pt->cb == cb) {
      it = m_timers.erase(it);
      delete pt;
    } else {
      ++it;
    }
  }
}

LogList<TimerManager::Timer*>::iterator TimerManager::find(
    TimerManager::Timer* t) {
  LogList<Timer*>::iterator it;

  for (it = m_timers.begin(); it != m_timers.end(); ++it) {
    if (t == (*it)) {
      break;
    }
  }

  return it;
}

int TimerManager::set_new_due_time(TimerManager::Timer* t, unsigned interval) {
  LogList<Timer*>::iterator it = find(t);
  if (it == m_timers.end()) {
    return -1;
  }

  timespec t_now;

  if (-1 == clock_gettime(CLOCK_BOOTTIME, &t_now)) {
    return -1;
  }

  t->due_time = t_now + interval;
  it = m_timers.erase(it);
  while (it != m_timers.end()) {
    if (t->due_time < (*it)->due_time) {
      break;
    }
    ++it;
  }
  m_timers.insert(it, t);
  return 0;
}

void TimerManager::del_timer(TimerManager::Timer* t) {
  LogList<Timer*>::iterator it = find(t);
  if (it != m_timers.end()) {
    m_timers.erase(it);
    delete t;
  }
}

int TimerManager::next_time(int& time_span) const {
  if (m_timers.empty()) {
    time_span = -1;
    return 0;
  }

  struct timespec tnow;

  if (-1 == clock_gettime(CLOCK_BOOTTIME, &tnow)) {
    return -1;
  }

  LogList<Timer*>::const_iterator it = m_timers.begin();
  const Timer* pt = *it;
  time_span = pt->due_time - tnow;
  if (time_span < 0) {
    time_span = 0;
  }
  return 0;
}

void TimerManager::run() {
  if (m_timers.empty()) {
    return;
  }

  struct timespec tnow;

  if (-1 == clock_gettime(CLOCK_BOOTTIME, &tnow)) {
    return;
  }

  LogList<Timer*>::iterator it = m_timers.begin();
  while (it != m_timers.end()) {
    Timer* p = *it;

    if (tnow < p->due_time) {
      break;
    }
    it = m_timers.erase(it);
    p->cb(p->param);
    delete p;
  }
}

timespec operator+(const timespec& t1, int t2) {
  timespec t = t1;
  int sec_num = t2 / 1000;

  // ms -> ns
  t2 = (t2 % 1000) * 1000000;

  t.tv_sec += sec_num;
  t.tv_nsec += t2;
  if (t.tv_nsec >= 1000000000) {
    ++t.tv_sec;
    t.tv_nsec -= 1000000000;
  }

  return t;
}

int operator-(const timespec& t1, const timespec& t2) {
  int sec = static_cast<int>(t1.tv_sec - t2.tv_sec);
  long ms = (t1.tv_nsec - t2.tv_nsec) / 1000000;

  return (ms + (sec * 1000));
}

bool operator<(const timespec& t1, const timespec& t2) {
  return t1.tv_sec < t2.tv_sec ||
         (t1.tv_sec == t2.tv_sec && t1.tv_nsec < t2.tv_nsec);
}
