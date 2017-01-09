/*
 *  io_chan.h - I/O channel.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-7-11 Zhang Ziyi
 *  Initial version.
 */
#ifndef _IO_CHAN_H_
#define _IO_CHAN_H_

#include <pthread.h>
#include <vector>

#include "data_buf.h"
#include "fd_hdl.h"

class LogFile;

class IoChannel : public FdHandler {
 public:
  // I/O request
  enum IoRequestType {
    IRT_WRITE,
    IRT_ABORT
  };

  struct IoRequest;

  typedef void (*io_result_callback_t)(void* client, IoRequest* req);

  struct IoRequest {
    io_result_callback_t callback;
    void* client;
    IoRequestType type;
    LogFile* file;
    std::vector<DataBuffer*>* data_list;
    size_t written;
    int err_code;
  };

  IoChannel(LogController* ctrl, Multiplexer* multiplexer);
  ~IoChannel();

  int init();
  void stop();

  /*  set_block_num_hint - allocate iovec array of specified length.
   *  @num: number of iovec objects
   */
  void set_block_num_hint(int num);

  int request(IoRequest* req);
  /*  wiat_io - wait for the background I/O to finish.
   *
   *  When the I/O is finished, this function will reset the I/O state
   *  to idle and will not call the result callback.
   */
  void wait_io();

  void process(int events) override;

 private:
  enum IoState {
    IS_IDLE,
    IS_EXECUTING
  };

  // Commands and responses to the I/O thread
  enum IoThreadMessageType {
    // Requests
    ITMT_IO,
    ITMT_QUIT,
    // Responses
    ITMT_IO_RESULT = 100,
    ITMT_UNKNOWN_REQ
  };

  IoState m_state;
  IoRequest* m_cur_req;
  // iovec array for I/O
  struct iovec* m_block_vec;
  int m_vec_num;
  // Whether the thread is created
  bool m_inited;
  // Mutex to guarantee memory visibility to the client thread and the
  // I/O thread
  pthread_mutex_t m_mutex;
  pthread_t m_thread;
  // Socket used by the I/O thread
  int m_thread_sock;

  void do_io();
  void on_io_done();
  int send_simple_req(IoThreadMessageType req);
  int send_response(IoThreadMessageType resp);

  static void* io_thread_func(void* param);
};

#endif  //!_IO_CHAN_H_
