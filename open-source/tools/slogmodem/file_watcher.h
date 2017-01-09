/*
 *  file_watcher.h - The file watcher class declaration.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-5-21 Zhang Ziyi
 *  Initial version.
 */
#ifndef _FILE_WATCHER_H_
#define _FILE_WATCHER_H_

#include <sys/inotify.h>
#include "cp_log_cmn.h"
#include "fd_hdl.h"

class LogController;
class Multiplexer;

class FileWatcher : public FdHandler {
 public:
  typedef void (*FileWatchCallback_t)(void* client, uint32_t evt);

  struct FileWatch {
    LogString path;
    void* client;
    FileWatchCallback_t cb;
    int wd;

    FileWatch(const LogString& p, void* c, FileWatchCallback_t callback,
              int watch_fd)
        : path(p), client(c), cb(callback), wd(watch_fd) {}
  };

  FileWatcher(LogController* ctrl, Multiplexer* multiplexer);
  ~FileWatcher();

  int init();

  int add(void* client, FileWatchCallback_t cb, const LogString& path,
          FileWatch*& fw);
  int del(FileWatch* fw);

  // Override the events handler
  void process(int events);

 private:
  LogVector<FileWatch*> m_watch_set;

  int get(const LogString& path);
  int get(int wd);
};

#endif  // !_FILE_WATCHER_H_
