/*
 *  ctrl_modem_log.cpp - process to trigger the slogmodem FLUSH command.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-02-17 Yan Zhihang
 *  Initial version.
 */
#include <cutils/sockets.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "cp_log_cmn.h"
#include "def_config.h"

#define MaxLenResp 10

/*  usage() - usage for modem log control
 */
void usage() {
  fprintf(stderr,
          "\n"
          "cplogctl flush - flush modem log to log file.\n"
          "cplogctl clear - delete modem log.\n");
}

/*  get_response - wait for the response for the specified time.
 *  @fd: socket file descriptor
 *  @timeout: number of milliseconds to wait for.
 *
 *  If the response is OK, return 1; if the response is ERROR or other,
 *  return -1; if timeout occurs, return 0.
 */
static void get_response(int fd, size_t timeout) {
  int ret= -1;
  uint8_t resp[MaxLenResp];
  struct pollfd r_pollfd;

  r_pollfd.fd = fd;
  r_pollfd.events = POLLIN;

  ret = poll(&r_pollfd, 1, timeout);

  if ((1 == ret) && (r_pollfd.revents & POLLIN)) {
    ssize_t read_num = read(fd, resp, MaxLenResp - 1);

    if (-1 == read_num) {
      err_log("socket read error from slogmodem client manager.");
      fprintf(stderr,
              "socket read error from slogmodem client manager, errno = %d.\n",
              errno);
    } else if (!read_num) {
      // Connection closed by the peer.
      err_log("connection closed by the peer.");
      fprintf(stderr, "connection closed by the peer.\n");
    } else {
      // Response received.
      resp[read_num] = '\0';
      if ((3 != read_num) || memcmp(resp, "OK\n", 3)) {
        // Command unsucessful
        err_log("err response: %s from slogmodem.",
                reinterpret_cast<char*>(&resp[0]));
        fprintf(stderr, "err response: %s from slogmodem.\n",
                reinterpret_cast<char*>(&resp[0]));
      }
    }
  } else if (0 == ret ) {
    err_log("Wait for response time out.");
    fprintf(stderr, "Wait for response time out.\n");
  } else if (ret < 0) {
    err_log("fail to poll the waiting socket.");
    fprintf(stderr, "fail to poll the waiting socket. errno = %d.\n", errno);
  }
}

static void ctrl_modem_log(int fd, const char* cmd) {
  size_t cmd_len = strlen(cmd);
  if (cmd_len <= 0) {
    return;
  }

  ssize_t len = write(fd, cmd, cmd_len);

  if (cmd_len != static_cast<size_t>(len)) {
    err_log("command write error, len=%d", static_cast<int>(len));
    fprintf(stderr, "command write error, len=%d, errno = %d.\n",
            static_cast<int>(len), errno);
    return;
  }

  // Wait for the response for a while before failure.
  get_response(fd, 3000);
}

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "Missing argument.\n");
    usage();
    return 1;
  } else {
    int fd;
    fd = socket_local_client(SLOG_MODEM_SERVER_SOCK_NAME,
                             ANDROID_SOCKET_NAMESPACE_ABSTRACT,
                             SOCK_STREAM);
    if (fd < 0) {
      err_log("can't connect to slogmodem server");
      fprintf(stderr, "Connect to slogmodem server refused, errno = %d.\n", errno);
      return 1;
    }

    long flags = fcntl(fd, F_GETFL);
    flags |= O_NONBLOCK;
    int err = fcntl(fd, F_SETFL, flags);
    if (-1 == err) {
      close(fd);
      err_log("set socket to O_NONBLOCK error");
      fprintf(stderr, "Fail to set socket O_NONBLOCK, errno = %d.\n", errno);
      return 1;
    }
    // Ignore SIGPIPE to avoid to be killed by the kernel
    // when writing to a socket which is closed by the peer.
    struct sigaction siga;

    memset(&siga, 0, sizeof siga);
    siga.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &siga, 0);

    if (0 == strcmp(argv[1], "flush")) {
        ctrl_modem_log(fd, "FLUSH\n");
    } else if (0 == strcmp(argv[1], "clear")) {
        ctrl_modem_log(fd, "slogctl clear\n");
    } else {
        usage();
    }
    close(fd);
    return 0;
  }
}
