/*
 *  io_chan.cpp - I/O channel.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-7-11 Zhang Ziyi
 *  Initial version.
 */

#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "cp_log_cmn.h"
#include "io_chan.h"
#include "log_file.h"
#include "multiplexer.h"

IoChannel::IoChannel(LogController* ctrl, Multiplexer* multiplexer)
    :FdHandler{-1, ctrl, multiplexer},
     m_state{IS_IDLE},
     m_cur_req{nullptr},
     m_block_vec{nullptr},
     m_vec_num{0},
     m_inited{false},
     m_thread_sock{-1} {}

IoChannel::~IoChannel() {
  if (m_inited) {
    stop();

    ::close(m_thread_sock);
    pthread_mutex_destroy(&m_mutex);
  }

  delete [] m_block_vec;
}

int IoChannel::init() {
  if (m_inited) {
    return -1;
  }

  // Create the sockets for communication with I/O thread
  int sock_fds[2];
  int err = socketpair(AF_LOCAL, SOCK_STREAM, 0, sock_fds);

  if (-1 == err) {
    err_log("socketpair error");
    return -1;
  }

  m_fd = sock_fds[0];
  m_thread_sock = sock_fds[1];

  err = pthread_mutex_init(&m_mutex, nullptr);
  if (err) {
    err_log("pthread_mutex_init error %d", err);
    goto clSock;
  }

  err = pthread_create(&m_thread, nullptr, io_thread_func, this);
  if (err) {
    err_log("pthread_create error %d", err);
    goto destroyMutex;
  }

  multiplexer()->register_fd(this, POLLIN);
  m_inited = true;

  return 0;

destroyMutex:
  pthread_mutex_destroy(&m_mutex);
clSock:
  ::close(sock_fds[0]);
  ::close(sock_fds[1]);
  m_fd = -1;
  m_thread_sock = -1;
  return -1;
}

void IoChannel::stop() {
  if (!m_inited) {
    return;
  }

  if (IS_EXECUTING == m_state) {
    wait_io();
  }

  send_simple_req(ITMT_QUIT);
  pthread_join(m_thread, nullptr);
}

void IoChannel::set_block_num_hint(int num) {
  if (m_vec_num < num) {
    delete [] m_block_vec;
    m_block_vec = new struct iovec[num];
    m_vec_num = num;
  }
}

int IoChannel::send_simple_req(IoThreadMessageType req) {
  uint8_t msg = static_cast<uint8_t>(req);
  ssize_t nwr = write(fd(), &msg, sizeof msg);

  return sizeof msg == static_cast<size_t>(nwr) ? 0 : -1;
}

int IoChannel::request(IoRequest* req) {
  if (IS_EXECUTING == m_state) {
    err_log("request when busy");
    return -1;
  }

  int err = pthread_mutex_lock(&m_mutex);

  if (err) {
    err_log("pthread_mutex_lock error %d", err);
    return -1;
  }

  m_cur_req = req;
  pthread_mutex_unlock(&m_mutex);

  size_t nwr;
  uint8_t msg = ITMT_IO;

  nwr = write(fd(), &msg, sizeof msg);
  if (sizeof msg != static_cast<size_t>(nwr)) {
    m_cur_req = nullptr;
    err_log("send request to I/O thread error");
    return -1;
  }
  m_state = IS_EXECUTING;
  return 0;
}

void IoChannel::process(int events) {
  uint8_t msg;
  ssize_t nr;

  nr = read(fd(), &msg, sizeof msg);
  if (!nr) {
    err_log("I/O thread close the message socket");
  } else if (-1 == nr) {
    err_log("read I/O thread message error");
  } else {
    if (ITMT_IO_RESULT == msg) {
      on_io_done();
    } else {
      err_log("unknown response %d", msg);
    }
  }
}

void* IoChannel::io_thread_func(void* param) {
  IoChannel* chan = static_cast<IoChannel*>(param);
  uint8_t msg;
  ssize_t nr;
  bool run = true;

  while (run) {
    nr = read(chan->m_thread_sock, &msg, sizeof msg);
    if (!nr) {
      err_log("socket closed");
      break;
    }
    if (-1 == nr) {
      err_log("read error");
      continue;
    }

    switch (msg) {
      case ITMT_IO:  // I/O request
        chan->do_io();
        break;
      case ITMT_QUIT:
        run = false;
        break;
      default:
        chan->send_response(ITMT_UNKNOWN_REQ);
        break;
    }
  }

  return nullptr;
}

int IoChannel::send_response(IoThreadMessageType resp) {
  uint8_t msg = static_cast<uint8_t>(resp);
  ssize_t nwr = write(m_thread_sock, &msg, sizeof msg);

  return sizeof msg == static_cast<size_t>(nwr) ? 0 : -1;
}

void IoChannel::do_io() {
  pthread_mutex_lock(&m_mutex);

  std::vector<DataBuffer*>& data_list = *m_cur_req->data_list;

  // Make sure m_block_vec is long enough
  set_block_num_hint(data_list.size());

  unsigned i;

  for (i = 0; i < data_list.size(); ++i) {
    DataBuffer* buf = data_list[i];
    m_block_vec[i].iov_base = buf->buffer + buf->data_start;
    m_block_vec[i].iov_len = buf->data_len;
  }
  ssize_t nwr = m_cur_req->file->write_raw(m_block_vec, static_cast<int>(i));

  // Make sure m_cur_req is visible to the client thread
  if (nwr >= 0) {
    m_cur_req->err_code = 0;
    m_cur_req->written = nwr;
  } else {
    m_cur_req->err_code = static_cast<int>(nwr);
    m_cur_req->written = 0;
  }
  pthread_mutex_unlock(&m_mutex);

  // Send response to the client thread
  if (send_response(ITMT_IO_RESULT)) {
    err_log("send I/O response error");
  }
}

void IoChannel::wait_io() {
  struct pollfd pol;
  int err;

  pol.fd = fd();
  pol.events = POLLIN;
  pol.revents = 0;
  while (true) {
    err = poll(&pol, 1, -1);
    if (err > 0) {
      uint8_t msg;
      ssize_t nr = read(fd(), &msg, sizeof msg);

      if (1 == nr) {
        if (ITMT_IO_RESULT == msg) {
          m_state = IS_IDLE;
          m_cur_req = nullptr;
          break;
        }
      } else if (!nr) {
        err_log("message socket closed unexpectedly");
        break;
      } else {
        err_log("read response error");
      }
    }
  }
}

void IoChannel::on_io_done() {
  // Make sure the m_cur_req object is visible to this thread
  pthread_mutex_lock(&m_mutex);

  IoRequest* req = m_cur_req;

  pthread_mutex_unlock(&m_mutex);

  m_state = IS_IDLE;
  m_cur_req = nullptr;
  if (req->callback) {
    req->callback(req->client, req);
  }
}
