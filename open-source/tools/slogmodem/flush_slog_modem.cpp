/*
 *  flush_slog_modem.cpp - process to trigger the slogmodem FLUSH command.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-8-15 Yan Zhihang
 *  Initial version.
 */

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#ifdef HOST_TEST_
  #include "sock_test.h"
#else
  #include <cutils/sockets.h>
#endif
#include <poll.h>
#include <unistd.h>

#include "cp_log_cmn.h"
#include "def_config.h"

#define MaxLenResp 10

/*  get_response - wait for the response for the specified time.
 *  @fd: socket file descriptor
 *  @timeout: number of milliseconds to wait for.
 *
 *  If the response is OK, return 1; if the response is ERROR or other,
 *  return -1; if timeout occurs, return 0.
 */
static int get_response(int fd, size_t timeout) {
  int ret = -1;
  uint8_t resp[MaxLenResp];
  struct pollfd r_pollfd;

  r_pollfd.fd = fd;
  r_pollfd.events = POLLIN;

  ret = poll(&r_pollfd, 1, timeout);

  if ((1 == ret) && (r_pollfd.revents & POLLIN)) {
    if ((read(fd, resp, MaxLenResp)) < 3 || memcmp(resp, "OK\n", 3)) {
      err_log("err response from slogmodem");
      ret = -1;
    }
  }
  return ret;
}

int main(int argc, char** argv) {
  int fd;
  fd = socket_local_client(SLOG_MODEM_SERVER_SOCK_NAME,
                           ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
  if (fd < 0) {
    err_log("can't connect to slogmodem server");
    fprintf(stderr, "ERROR: connect to %s %d\n",
            SLOG_MODEM_SERVER_SOCK_NAME, errno);
    return 1;
  }

  long flags = fcntl(fd, F_GETFL);
  flags |= O_NONBLOCK;
  int err = fcntl(fd, F_SETFL, flags);
  if (-1 == err) {
    err_log("set flush socket to O_NONBLOCK error");
    return 1;
  }

  ssize_t len = write(fd, "FLUSH\n", 6);

  if (6 != static_cast<size_t>(len)) {
    err_log("FLUSH command write error, len=%d",
            static_cast<int>(len));
    fprintf(stderr, "ERROR: write socket %d\n", errno);
    return 1;
  }

  // Wait for the response for a while before failure.
  int result = get_response(fd, 3000);
  if (1 != result) {
    fprintf(stderr, "ERROR: get response %d\n", result);
  }
  return 1 == result ? 0 : 2;
}
