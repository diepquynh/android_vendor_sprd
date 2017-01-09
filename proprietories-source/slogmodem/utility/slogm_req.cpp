/*
 *  slogm_req.cpp - slogmodem request base class.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-7-4 Zhang Ziyi
 *  Initial version.
 */

#ifdef HOST_TEST_
  #include "sock_test.h"
  #include <sys/socket.h>
  #include <sys/types.h>
#else
  #include <cutils/sockets.h>
#endif
#include <poll.h>
#include <unistd.h>

#include "cp_log_cmn.h"
#include "def_config.h"
#include "parse_utils.h"
#include "slogm_req.h"

SlogmRequest::SlogmRequest():m_fd{-1} {}

SlogmRequest::~SlogmRequest() {
  if (m_fd >= 0) {
    close(m_fd);
  }
}

int SlogmRequest::connect() {
  int ret = 0;

  m_fd = socket_local_client(SLOG_MODEM_SERVER_SOCK_NAME,
                             ANDROID_SOCKET_NAMESPACE_ABSTRACT,
                             SOCK_STREAM);
  if (m_fd < 0) {
    err_log("can't connect to slogmodem server");
    fprintf(stderr, "Connect to slogmodem server refused, errno = %d.\n", errno);
    ret = -1;
  }

  return ret;
}

int SlogmRequest::prepare_conn() {
  return connect();
}

int SlogmRequest::send_req(const void* cmd, size_t len) {
  ssize_t nwr = write(m_fd, cmd, len);

  return static_cast<size_t>(nwr) == len ? 0 : -1;
}

int SlogmRequest::wait_response(void* buf, size_t& len, int wait_time) {
  struct pollfd pol;

  pol.fd = m_fd;
  pol.events = POLLIN;
  pol.revents = 0;

  int err = poll(&pol, 1, wait_time);

  if (err <= 0) {  // Error or timeout
    err = -1;
  } else {
    ssize_t nr = read(m_fd, buf, len);

    if (0 == nr) {  // slogmodem closed the connection
      err = -1;
    } else if (-1 == nr) {  // Error
      err = -1;
    } else {
      char* p = static_cast<char*>(buf);
      if ('\n' == p[nr - 1]) {
        len = nr - 1;
      } else {
        len = nr;
      }
      err = 0;
    }
  }

  return err;
}

void SlogmRequest::report_error(RequestError err) {
  switch (err) {
    case RE_CONN_FAILURE:
      fprintf(stderr, "Connect to slogmodem error: %d\n", errno);
      err_log("connect to slogmodem error");
      break;
    default:
      fprintf(stderr, "Request error: %d\n", errno);
      err_log("request error");
      break;
  }
}

int SlogmRequest::exec() {
  if (prepare_conn()) {
    report_error(RE_CONN_FAILURE);
    return -1;
  }

  return do_request();
}

int SlogmRequest::parse_result(const void* resp, size_t len,
                               ResponseErrorCode& result,
                               const void*& stop_ptr) {
  const uint8_t* beginp = static_cast<const uint8_t*>(resp);
  int ret = 0;

  if (len >= 2 && !memcmp(resp, "OK", 2)) {
    result = REC_SUCCESS;
    stop_ptr = beginp + 2;
  } else if (len >= 6 && !memcmp(resp, "ERROR ", 6)) {
    beginp += 6;
    len -= 6;

    unsigned n;
    size_t parsed;

    if (parse_number(beginp, len, n, parsed) || n >= REC_MAX_ERROR) {
      ret = -1;
    } else {
      result = static_cast<ResponseErrorCode>(n);
      stop_ptr = beginp + parsed;
    }
  } else {
    ret = -1;
  }

  return ret;
}

const char* SlogmRequest::resp_code_to_string(ResponseErrorCode err_code) {
  const char* err_str = "Unknown error";

  switch (err_code) {
    case REC_UNKNOWN_REQ:
      err_str = "unknown request";
      break;
    case REC_INVAL_PARAM:
      err_str = "invalid parameter";
      break;
    case REC_FAILURE:
      err_str = "execution failure";
      break;
    case REC_IN_TRANSACTION:
      err_str = "transaction going on";
      break;
    case REC_LOG_DISABLED:
      err_str = "log is not saved to SD";
      break;
    case REC_SLEEP_LOG_NOT_SUPPORTED:
      err_str = "sleep log not supported";
      break;
    case REC_RINGBUF_NOT_SUPPORTED:
      err_str = "RingBuf log not supported";
      break;
    case REC_CP_ASSERTED:
      err_str = "the CP has asserted";
      break;
    case REC_UNKNOWN_CP_TYPE:
      err_str = "unknown CP or subsystem";
      break;
    case REC_NO_AGDSP:
      err_str = "no AG-DSP in the system";
      break;
    default:
      break;
  }

  return err_str;
}

int SlogmRequest::wait_simple_response(int wait_time) {
  char resp[SLOGM_RSP_LENGTH];
  size_t rlen = SLOGM_RSP_LENGTH;

  int ret = wait_response(resp, rlen, wait_time);

  if (ret) {
    report_error(RE_WAIT_RSP_ERROR);
    return -1;
  }

  ResponseErrorCode err_code;
  const void* stop_ptr;

  ret = parse_result(resp, rlen, err_code, stop_ptr);
  if (ret) {
    LogString sd;

    str_assign(sd, resp, rlen);
    fprintf(stderr, "Invalid response: %s\n", ls2cstring(sd));
    err_log("invalid response: %s", ls2cstring(sd));
  } else {
    if (REC_SUCCESS != err_code) {
      const char* err_str = resp_code_to_string(err_code);
      fprintf(stderr, "Error: %d(%s)\n", static_cast<int>(err_code),
              err_str);
      err_log("error: %d(%s)", static_cast<int>(err_code),
              err_str);
      ret = -1;
    }
  }

  return ret;
}
