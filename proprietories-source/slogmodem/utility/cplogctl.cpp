/*
 *  cplogctl.cpp - utility program to control slogmodem.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-02-17 Yan Zhihang
 *  Initial version.
 *
 *  2016-7-4 Zhang Ziyi
 *  More commands added.
 *  Source files moved into cplogctl directory.
 *  Use libc++ instead of libutils.
 */

/*  Usage: cplogctl <command> [<arg1> [<arg2> ...]]
 *
 *  <command>
 *    clear  (no arguments)
 *    enable <subsys1> [<subsys2> ...]
 *    disable <subsys1> [<subsys2> ...]
 *      <subsysn> may be 5mode, wcn, gnss, pmsh or agdsp.
 *    flush  (no arguments)
 *    getcapacity <storage>
 *      <storage> may be internal or external.
 *    getfilesize (no arguments)
 *    getoverwrite (no arguments)
 *    setaglog <output>
 *      <output> may be off, uart or ap.
 *    setagpcm <enable>
 *      <enable> may be on or off.
 *    setcapacity <storage> <size>
 *      <storage> may be internal or external.
 *      <size> should be positive integer in megabyte(MB). If <size> is 0,
 *      use all free space
 *    setfilesize <size>
 *      <size> should be positive integer in megabyte(MB).
 *    setoverwrite <command>
 *      <command> maybe enable, disable.
 *    state <subsys>
 *
 *  Exit code: 0 - success, 1 - fail.
 */

#include <algorithm>
#include <signal.h>
#include <stdio.h>

#include "clear_req.h"
#include "cplogctl_cmn.h"
#include "flush_req.h"
#include "get_log_file_size.h"
#include "get_storage_capacity.h"
#include "on_off_req.h"
#include "overwrite_req.h"
#include "query_state_req.h"
#include "set_ag_log_dest.h"
#include "set_ag_pcm.h"
#include "set_log_file_size.h"
#include "set_storage_capacity.h"

/*  usage() - usage for modem log control
 */
void usage() {
  fprintf(stderr,
          "Usage: cplogctl <command> [<arg1> [<arg2> ...]]\n\n"
          "<command>\n"
          "  clear  (no arguments)\n"
          "    clear all logs.\n"
          "\n"
          "  enable <subsys1> [<subsys2> ...]\n"
          "    save logs to SD for specified subsystems\n"
          "\n"
          "  disable <subsys1> [<subsys2> ...]\n"
          "    stop saving logs to SD for specified subsystems\n"
          "    <subsysn> may be 5mode, wcn, gnss, pmsh or agdsp.\n"
          "\n"
          "  flush  (no arguments)\n"
          "    flush all buffered logs.\n"
          "\n"
          "  getcapacity <storage>\n"
          "    get max size of specified storage media for log saving.\n"
          "    <storage> may be internal or external.\n"
          "    query result format:\n"
          "      <size> MB\n"
          "    If <size> is 0, all free space will be used.\n"
          "\n"
          "  getfilesize  (no arguments)\n"
          "    get max size of a single log file.\n"
          "    query result format:\n"
          "      <size> MB\n"
          "\n"
          "  getoverwrite  (no arguments)\n"
          "    query log files' overwrite state\n"
          "    the result format is:\n"
          "      overwrite:<state> (e.g., overwrite:enabled)\n"
          "\n"
          "  setaglog <output>\n"
          "    set AG-DSP log output method.\n"
          "    <output> may be off, uart or ap.\n"
          "\n"
          "  setagpcm <enable>\n"
          "    enable/disable AG-DSP PCM dump.\n"
          "    <enable> may be on or off.\n"
          "\n"
          "  setcapacity <storage> <size>\n"
          "    set max size of specified storage media for log saving.\n"
          "    <storage> may be internal or external.\n"
          "    <size> should be any positive integer in megabyte(MB).\n"
          "    If <size> is 0, use all free space.\n"
          "\n"
          "  setfilesize <size>\n"
          "    set max size of a single log file.\n"
          "    <size> should be positive integer in megabyte(MB).\n"
          "\n"
          "  setoverwrite <command>\n"
          "    enable/disable log files overwrite.\n"
          "    <command> may be enable, disable or query.\n"
          "\n"
          "  state <subsys>\n"
          "    query log state for specified subsystem.\n"
          "    query result format:\n"
          "      <subsys>:<state> (e.g. 5mode:on, wcn:off)\n"
          "\n"
          "Exit code: 0 - success, 1 - fail.\n"
          "\n");
}

int parse_subsys(char** argv, int argc, LogVector<CpType>& types) {
  // <subsysn> may be 5mode, wcn, gnss, pmsh or agdsp.
  bool valid = true;

  for (int i = 0; i < argc; ++i) {
    CpType type;

    if (!strcmp(argv[i], "5mode")) {
      type = CT_5MODE;
    } else if (!strcmp(argv[i], "wcn")) {
      type = CT_WCN;
    } else if (!strcmp(argv[i], "gnss")) {
      type = CT_GNSS;
    } else if (!strcmp(argv[i], "pmsh")) {
      type = CT_PM_SH;
    } else if (!strcmp(argv[i], "agdsp")) {
      type = CT_AGDSP;
    } else {
      valid = false;
      types.clear();
      fprintf(stderr, "Unknown subsystem: %s\n", argv[i]);
      break;
    }

    if (types.end() == std::find(types.begin(), types.end(), type)) {
      types.push_back(type);
    }
  }

  return valid ? 0 : -1;
}

SlogmRequest* proc_set_storage_max_size(char** argv, int argc) {
  if (2 != argc) {
    fprintf(stderr, "invalid parameters\n");
    usage();
    return nullptr;
  }

  MediaType mt;
  if (!strcmp(argv[0], "internal")) {
    mt = MT_INTERNAL;
  } else if (!strcmp(argv[0], "external")) {
    mt = MT_EXTERNAL;
  } else {
    fprintf(stderr, "invalid media storage type %s\n", argv[0]);
    return nullptr;
  }

  SetStorageCapacity* set_cap{nullptr};
  unsigned long size;
  const char* endp;

  if (non_negative_number(argv[1], size, endp)) {
    if (spaces_only(endp)) {
      set_cap = new SetStorageCapacity{mt, static_cast<unsigned>(size)};
    }
  }

  if (!set_cap) {
    fprintf(stderr, "invalid size %s\n", argv[1]);
    usage();
  }

  return set_cap;
}

SlogmRequest* proc_set_file_size(char** argv, int argc) {
  if (1 != argc) {
    fprintf(stderr, "invalid parameters\n");
    usage();
    return nullptr;
  }

  SetLogFileSize* set_size{nullptr};
  unsigned long size;
  const char* endp;

  if (non_negative_number(argv[0], size, endp)) {
    if (spaces_only(endp)) {
      set_size = new SetLogFileSize{static_cast<unsigned>(size)};
    }
  }

  if (!set_size) {
    fprintf(stderr, "invalid parameter %s\n", argv[1]);
    usage();
  }

  return set_size;
}

SlogmRequest* proc_get_storage_max_size(char** argv, int argc) {
  if (1 != argc) {
    fprintf(stderr, "invalid parameters\n");
    usage();
    return nullptr;
  }

  MediaType mt;
  if (!strcmp(argv[0], "internal")) {
    mt = MT_INTERNAL;
  } else if (!strcmp(argv[0], "external")) {
    mt = MT_EXTERNAL;
  } else {
    fprintf(stderr, "invalid media storage type %s\n", argv[0]);
    return nullptr;
  }

  return new GetStorageCapacity{mt};
}

SlogmRequest* proc_enable_disable(char** argv, int argc, bool enable) {
  LogVector<CpType> types;

  if (parse_subsys(argv, argc, types)) {
    return nullptr;
  }

  if (!argc) {
    fprintf(stderr, "No subsystem defined\n");
    return nullptr;
  }

  return new OnOffRequest{enable, types};
}

SlogmRequest* proc_overwrite(char** argv, int argc) {
  if (1 != argc) {
    fprintf(stderr, "invalid parameters\n");
    usage();
    return nullptr;
  }

  if (!strcmp(argv[0], "enable")) {
    return new OverwriteRequest{OverwriteRequest::OCT_ENABLE};
  } else if (!strcmp(argv[0], "disable")) {
    return new OverwriteRequest{OverwriteRequest::OCT_DISABLE};
  } else {
    fprintf(stderr, "invalid parameters\n");
    usage();
    return nullptr;
  }
}

SlogmRequest* proc_query_state(char** argv, int argc) {
  if (1 != argc) {
    fprintf(stderr, "invalid parameters\n");
    usage();
    return nullptr;
  }

  LogVector<CpType> types;

  if (parse_subsys(argv, argc, types)) {
    return nullptr;
  }

  return new QueryStateRequest{argv[0], types[0]};
}

SlogmRequest* proc_agdsp_log_dest(char** argv, int argc) {
  if (1 != argc) {
    fprintf(stderr, "invalid parameters\n");
    usage();
    return nullptr;
  }

  // <output> may be off, uart or ap.
  SetAgdspLogDest* req{nullptr};
  SetAgdspLogDest::AgDspLogDest dest;
  bool valid = true;

  if (!strcmp(argv[0], "off")) {
    dest = SetAgdspLogDest::AGDSP_LOG_DEST_OFF;
  } else if (!strcmp(argv[0], "uart")) {
    dest = SetAgdspLogDest::AGDSP_LOG_DEST_UART;
  } else if (!strcmp(argv[0], "ap")) {
    dest = SetAgdspLogDest::AGDSP_LOG_DEST_AP;
  } else {
    fprintf(stderr, "Invalid AG-DSP log destination %s\n", argv[0]);
    valid = false;
  }

  if (valid) {
    req = new SetAgdspLogDest{dest};
  }

  return req;
}

SlogmRequest* proc_agdsp_pcm(char** argv, int argc) {
  if (1 != argc) {
    fprintf(stderr, "invalid parameters\n");
    usage();
    return nullptr;
  }

  // <output> may be off, uart or ap.
  SetAgdspPcm* req{nullptr};
  bool pcm_on;
  bool valid = true;

  if (!strcmp(argv[0], "off")) {
    pcm_on = false;
  } else if (!strcmp(argv[0], "on")) {
    pcm_on = true;
  } else {
    fprintf(stderr, "Invalid AG-DSP PCM setting %s\n", argv[0]);
    valid = false;
  }

  if (valid) {
    req = new SetAgdspPcm{pcm_on};
  }

  return req;
}

int main(int argc, char** argv) {
  int ret = 1;

  if (argc < 2) {
    fprintf(stderr, "Missing argument.\n");
    usage();
    return ret;
  }

  // Ignore SIGPIPE to avoid to be killed by the kernel
  // when writing to a socket which is closed by the peer.
  struct sigaction siga;

  memset(&siga, 0, sizeof siga);
  siga.sa_handler = SIG_IGN;
  sigaction(SIGPIPE, &siga, 0);

  SlogmRequest* req{nullptr};

  if (!strcmp(argv[1], "clear")) {
    if (2 != argc) {
      fprintf(stderr, "clear command does not have any arguments\n");
    } else {
      req = new ClearRequest;
    }
  } else if (!strcmp(argv[1], "disable")) {
    req = proc_enable_disable(argv + 2, argc - 2, false);
  } else if (!strcmp(argv[1], "enable")) {
    req = proc_enable_disable(argv + 2, argc - 2, true);
  } else if (!strcmp(argv[1], "flush")) {
    if (2 != argc) {
      fprintf(stderr, "flush command does not have any arguments\n");
    } else {
      req = new FlushRequest;
    }
  } else if (!strcmp(argv[1], "getcapacity")) {
    req = proc_get_storage_max_size(argv + 2, argc - 2);
  } else if (!strcmp(argv[1], "getfilesize")) {
    if (2 != argc) {
      fprintf(stderr, "getfilesize command does not have any arguments\n");
    } else {
      req = new GetLogFileSize;
    }
  } else if (!strcmp(argv[1], "getoverwrite")) {
    if (2 != argc) {
      fprintf(stderr, "getoverwrite command does not have any arguments\n");
    } else {
      req = new OverwriteRequest{OverwriteRequest::OCT_QUERY};
    }
  } else if (!strcmp(argv[1], "setaglog")) {
    req = proc_agdsp_log_dest(argv + 2, argc - 2);
  } else if (!strcmp(argv[1], "setagpcm")) {
    req = proc_agdsp_pcm(argv + 2, argc - 2);
  } else if (!strcmp(argv[1], "setcapacity")) {
    req = proc_set_storage_max_size(argv + 2, argc - 2);
  } else if (!strcmp(argv[1], "setfilesize")) {
    req = proc_set_file_size(argv + 2, argc - 2);
  } else if (!strcmp(argv[1], "setoverwrite")) {
    req = proc_overwrite(argv + 2, argc - 2);
  } else if (!strcmp(argv[1], "state")) {
    req = proc_query_state(argv + 2, argc - 2);
  } else {
    fprintf(stderr, "Invalid command: %s\n", argv[1]);
  }

  if (req) {
    if (0 == req->exec()) {
      ret = 0;
    }
    delete req;
  }

  return ret;
}
