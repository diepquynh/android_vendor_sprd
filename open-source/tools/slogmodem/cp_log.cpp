/*
 *  cp_log.cpp - The main function for the CP log and dump program.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-2-16 Zhang Ziyi
 *  Initial version.
 */
#include <cstring>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "def_config.h"
#include "log_ctrl.h"
#include "log_config.h"

int main(int argc, char** argv) {
  umask(0);
  LogController log_controller;
  LogConfig log_config(CP_LOG_TMP_CONFIG_FILE);

  info_log("slogmodem start ID=%u/%u, GID=%u/%u",
           static_cast<unsigned>(getuid()), static_cast<unsigned>(geteuid()),
           static_cast<unsigned>(getgid()), static_cast<unsigned>(getegid()));

  int err = log_config.read_config(argc, argv);
  if (err < 0) {
    return err;
  }

  // Ignore SIGPIPE to avoid to be killed by the kernel
  // when writing to a socket which is closed by the peer.
  struct sigaction siga;

  memset(&siga, 0, sizeof siga);
  siga.sa_handler = SIG_IGN;
  sigaction(SIGPIPE, &siga, 0);

  if (log_controller.init(&log_config) < 0) {
    return 2;
  }
  return log_controller.run();
}
