/*
 *  fd_hdl.h - The file descriptor handler base class declaration.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-2-16 Zhang Ziyi
 *  Initial version.
 */
#ifndef FD_HDL_H_
#define FD_HDL_H_

class LogController;
class Multiplexer;

class FdHandler {
 public:
  FdHandler(int fd, LogController* ctrl, Multiplexer* multiplexer);

  virtual ~FdHandler();

  int fd() const { return m_fd; }
  LogController* controller() const { return m_log_ctrl; }
  Multiplexer* multiplexer() const { return m_multiplexer; }

  void add_events(int events);
  void del_events(int events);

  int close();

  // Events handler
  virtual void process(int events) = 0;

 protected:
  int m_fd;

 private:
  LogController* m_log_ctrl;
  Multiplexer* m_multiplexer;
};

#endif  // !FD_HDL_H_
