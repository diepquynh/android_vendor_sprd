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
    : m_dirty(true),
      m_current_num(0),
      m_current_fds{{0, 0, 0}},
      m_current_handlers{0} {}

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
  int ret = 0;

  if (i == m_polling_hdl.size()) {
    if (m_polling_hdl.size() < MAX_MULTIPLEXER_NUM) {
      PollingEntry e;

      e.handler = handler;
      e.events = events;
      m_polling_hdl.push_back(e);
      m_dirty = true;
    } else {
      ret = -1;
    }
  } else {
    if ((m_polling_hdl[i].events & events) != events) {
#ifdef HOST_TEST_
      m_polling_hdl[i].events |= events;
#else
      m_polling_hdl.editItemAt(i).events |= events;
#endif
      m_dirty = true;
    }
  }

  return ret;
}

void Multiplexer::unregister_fd(FdHandler* handler, int events) {
  // First find the entry
  size_t i = find_handler(handler);

  if (i < m_polling_hdl.size()) {
    if (m_polling_hdl[i].events & events) {
#ifdef HOST_TEST_
      m_polling_hdl[i].events &= ~events;
#else
      m_polling_hdl.editItemAt(i).events &= ~events;
#endif
      if (!m_polling_hdl[i].events) {
        remove_at<LogVector<PollingEntry> >(m_polling_hdl, i);
      }
      m_dirty = true;
    }
  }
}

void Multiplexer::prepare_polling_array() {
  nfds_t i;

  for (i = 0; i < m_polling_hdl.size(); ++i) {
    m_current_fds[i].fd = m_polling_hdl[i].handler->fd();
    m_current_fds[i].events = m_polling_hdl[i].events;
    m_current_fds[i].revents = 0;
    m_current_handlers[i] = m_polling_hdl[i].handler;
  }

  m_current_num = i;
}

int Multiplexer::run() {
  while (true) {
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
        }
      }
    }
  }

  return 0;
}
