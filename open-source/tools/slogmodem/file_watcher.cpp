/*
 *  file_watcher.cpp - The file watcher class implementation.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-6-8 Zhang Ziyi
 *  Initial version.
 */

#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include "file_watcher.h"

FileWatcher::FileWatcher(LogController* ctrl, Multiplexer* multiplexer)
    : FdHandler(-1, ctrl, multiplexer) {}

FileWatcher::~FileWatcher() {
  size_t i;

  for (i = 0; i < m_watch_set.size(); ++i) {
    inotify_rm_watch(m_fd, m_watch_set[i]->wd);
  }

  clear_ptr_container(m_watch_set);
}

int FileWatcher::init() {
  m_fd = inotify_init();
  if (m_fd >= 0) {
    long flags = fcntl(m_fd, F_GETFL);
    int ret = fcntl(m_fd, F_SETFL, flags | O_NONBLOCK);
    if (ret < 0) {
      err_log("fcntl inotify instance error");
      ::close(m_fd);
      m_fd = -1;
    } else {
      add_events(POLLIN);
    }
  }
  return m_fd;
}

int FileWatcher::add(void* client, FileWatchCallback_t cb,
                     const LogString& path, FileWatch*& fw) {
  int i = get(path);
  if (i >= 0) {
    return -1;
  }
  int wd = inotify_add_watch(m_fd, ls2cstring(path), IN_DELETE_SELF);
  if (-1 == wd) {
    return -1;
  }
  FileWatch* w = new FileWatch(path, client, cb, wd);
  m_watch_set.push_back(w);
  fw = w;
  return 0;
}

int FileWatcher::del(FileWatch* fw) {
  LogVector<FileWatch*>::iterator it;
  int ret = -1;

  for (it = m_watch_set.begin(); it != m_watch_set.end(); ++it) {
    if ((*it) == fw) {
      m_watch_set.erase(it);
      ret = 0;
      break;
    }
  }
  if (!ret) {
    inotify_rm_watch(m_fd, fw->wd);
    delete fw;
  }

  return ret;
}

int FileWatcher::get(const LogString& path) {
  size_t i;
  int ret = -1;

  for (i = 0; i < m_watch_set.size(); ++i) {
    if (m_watch_set[i]->path == path) {
      ret = static_cast<int>(i);
      break;
    }
  }

  return ret;
}

int FileWatcher::get(int wd) {
  size_t i;
  int ret = -1;

  for (i = 0; i < m_watch_set.size(); ++i) {
    if (m_watch_set[i]->wd == wd) {
      ret = static_cast<int>(i);
      break;
    }
  }

  return ret;
}

void FileWatcher::process(int events) {
  if (!(events & POLLIN)) {
    return;
  }

  while (true) {
    inotify_event evt;
    ssize_t n = read(m_fd, &evt, sizeof evt);
    if (-1 == n) {
      if (EAGAIN != errno) {
        err_log("read inotify error");
      }
      break;
    }
    if (evt.mask & IN_DELETE_SELF) {
      int i = get(evt.wd);
      if (i >= 0) {
        FileWatch* fw = m_watch_set[i];
        fw->cb(fw->client, evt.mask);
      }
    }
  }
}
