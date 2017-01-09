/*
 *  multiplexer.cpp - The multiplexer implementation.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-2-16 Zhang Ziyi
 *  Initial version.
 */
#include <unistd.h>

#include "cp_log_cmn.h"
#include "multiplexer.h"

Multiplexer::Multiplexer()
    : m_run{true},
      m_dirty{true},
      m_poll_arr_size{0},
      m_current_num{0},
      m_current_fds{0},
      m_current_handlers{0} {}

Multiplexer::~Multiplexer() {
  delete [] m_current_fds;
  delete [] m_current_handlers;
}

size_t Multiplexer::find_handler(FdHandler* handler) {
  size_t i;

  for (i = 0; i < m_polling_hdl.size(); ++i) {
    if (m_polling_hdl[i].handler == handler) {
      break;
    }
  }

  return i;
}

int Multiplexer::register_fd(FdHandler* handler, int events) {
  // First find the entry
  size_t i = find_handler(handler);
  int ret = -1;

  if (i == m_polling_hdl.size()) {
    PollingEntry e;

    e.handler = handler;
    e.events = events;
    m_polling_hdl.push_back(e);
    m_dirty = true;
    ret = 0;
  }

  return ret;
}

void Multiplexer::unregister_fd(FdHandler* handler) {
  // First find the entry
  size_t i = find_handler(handler);

  if (i < m_polling_hdl.size()) {
    remove_cur_handler(handler);
    remove_at< LogVector<PollingEntry> >(m_polling_hdl, i);
    m_dirty = true;
  }
}

void Multiplexer::remove_cur_handler(FdHandler* handler) {
  // Don't change m_current_num, just reset the entry
  for (unsigned i = 0; i < m_current_num; ++i) {
    if (m_current_handlers[i] == handler) {
      m_current_handlers[i] = nullptr;
      m_current_fds[i].revents = 0;
      break;
    }
  }
}

int Multiplexer::add_events(FdHandler* handler, int events) {
  // First find the entry
  size_t i = find_handler(handler);
  int ret = -1;

  if (i < m_polling_hdl.size()) {
    PollingEntry& e = item(m_polling_hdl, i);
    if ((e.events & events) != events) {
      e.events |= events;
      m_dirty = true;
    }
    ret = 0;
  }

  return ret;
}

int Multiplexer::del_events(FdHandler* handler, int events) {
  // First find the entry
  size_t i = find_handler(handler);
  int ret = -1;

  if (i < m_polling_hdl.size()) {
    PollingEntry& e = item(m_polling_hdl, i);
    if (e.events & events) {
      clear_cur_events(handler, events);
      e.events &= ~events;
      m_dirty = true;
    }
    ret = 0;
  }

  return ret;
}

void Multiplexer::clear_cur_events(FdHandler* handler, int events) {
  for (unsigned i = 0; i < m_current_num; ++i) {
    if (m_current_handlers[i] == handler) {
      m_current_fds[i].revents &= ~events;
      break;
    }
  }
}

void Multiplexer::prepare_polling_array() {
  if (m_polling_hdl.size() > m_poll_arr_size) {
    size_t step_times = (m_polling_hdl.size() + POLL_ARRAY_INC_STEP - 1)
                          / POLL_ARRAY_INC_STEP;
    size_t new_size = step_times * POLL_ARRAY_INC_STEP;
    pollfd* new_poll = new pollfd[new_size];

    delete [] m_current_fds;
    m_current_fds = new_poll;

    FdHandler** new_hdl = new FdHandler*[new_size];
    delete [] m_current_handlers;
    m_current_handlers = new_hdl;

    m_poll_arr_size = new_size;
  }

  nfds_t i;
  size_t j = 0;

  for (i = 0; i < m_polling_hdl.size(); ++i) {
    int events = m_polling_hdl[i].events;

    if (events) {
      m_current_fds[j].fd = m_polling_hdl[i].handler->fd();
      m_current_fds[j].events = events;
      m_current_fds[j].revents = 0;
      m_current_handlers[j] = m_polling_hdl[i].handler;
      ++j;
    }
  }

  m_current_num = j;
}

int Multiplexer::run() {
  while (m_run) {
    if (m_dirty) {
      // Fill m_current_fds, m_current_handlers according to
      // m_polling_hdl
      prepare_polling_array();

      m_dirty = false;
    }

    int to;

    if (m_timer_mgr.next_time(to) < 0) {
      to = -1;
    }

    int err = poll(m_current_fds, m_current_num, to);
    m_timer_mgr.run();
    // Here process the events
    if (err > 0) {
      for (unsigned i = 0; i < m_current_num; ++i) {
        short revents = m_current_fds[i].revents;
        if (revents) {
          m_current_handlers[i]->process(revents);
          if (!m_run) {
            break;
          }
        }
      }
    }
  }

  return 0;
}
