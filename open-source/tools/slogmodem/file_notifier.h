/*
 *  data_notifier.h - The file data notifier class.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-2-20 Zhang Ziyi
 *  Initial version.
 */
#ifndef FILE_NOTIFIER_H_
#define FILE_NOTIFIER_H_

#include <cstddef>
#include <cstdint>
#include "fd_hdl.h"

class FileNotifier : public FdHandler {
 public:
  typedef void (*file_data_notify_t)(FileNotifier* notifier, void* client);

  FileNotifier(LogController* ctrl, Multiplexer* multiplexer,
               const char* file_path);

  void set_notify(file_data_notify_t cb, void* client) {
    m_callback = cb;
    m_client = client;
  }

  bool started() const { return fd() >= 0; }
  int start();
  void stop();

  void process(int events);

 private:
  const char* m_file_path;
  void* m_client;
  file_data_notify_t m_callback;
};

#endif  // !FILE_NOTIFIER_H_
