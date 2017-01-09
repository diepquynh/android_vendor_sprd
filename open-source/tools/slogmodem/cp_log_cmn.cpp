/*
 *  cp_log_cmn.cpp - Common functions for the CP log and dump program.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-2-16 Zhang Ziyi
 *  Initial version.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include "cp_log_cmn.h"

int get_timezone() {
  time_t t1;
  struct tm* p;
  struct tm local_tm;
  struct tm gm_tm;

  t1 = time(0);
  if (static_cast<time_t>(-1) == t1) {
    return 0;
  }
  p = gmtime_r(&t1, &gm_tm);
  if (!p) {
    return 0;
  }
  p = localtime_r(&t1, &local_tm);
  if (!p) {
    return 0;
  }

  int tz = local_tm.tm_hour - gm_tm.tm_hour;
  if (tz > 12) {
    tz -= 24;
  } else if (tz < -12) {
    tz += 24;
  }

  return tz;
}

int copy_file_seg(int src_fd, int dest_fd, size_t m) {
  int err = 0;
  static const size_t FILE_COPY_BUF_SIZE = (1024 * 32);
  static uint8_t s_copy_buf[FILE_COPY_BUF_SIZE];
  size_t cum = 0;
  size_t left_len = m;

  while (cum < m) {
    size_t to_wr;
    left_len = m - cum;

    if (left_len > FILE_COPY_BUF_SIZE) {
      to_wr = FILE_COPY_BUF_SIZE;
    } else {
      to_wr = left_len;
    }

    ssize_t n = read(src_fd, s_copy_buf, to_wr);

    if (-1 == n) {
      err = -1;
      break;
    }
    if (!n) {  // End of file
      break;
    }
    cum += to_wr;

    n = write(dest_fd, s_copy_buf, to_wr);
    if (-1 == n || static_cast<size_t>(n) != to_wr) {
      err = -1;
      break;
    }
  }

  return err;
}

int copy_file(int src_fd, int dest_fd) {
  int err = 0;
  static const size_t FILE_COPY_BUF_SIZE = (1024 * 32);
  static uint8_t s_copy_buf[FILE_COPY_BUF_SIZE];

  while (true) {
    ssize_t n = read(src_fd, s_copy_buf, FILE_COPY_BUF_SIZE);

    if (-1 == n) {
      err = -1;
      break;
    }
    if (!n) {  // End of file
      break;
    }
    size_t to_wr = n;
    n = write(dest_fd, s_copy_buf, to_wr);
    if (-1 == n || static_cast<size_t>(n) != to_wr) {
      err = -1;
      break;
    }
  }

  return err;
}

int copy_file(const char* src, const char* dest) {
  // Open the source and the destination file
  int src_fd;
  int dest_fd;

  src_fd = open(src, O_RDONLY);
  if (-1 == src_fd) {
    err_log("open source file %s failed", src);
    return -1;
  }

  dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC,
                 S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (-1 == dest_fd) {
    close(src_fd);
    err_log("open dest file %s failed", dest);
    return -1;
  }

  int err = copy_file(src_fd, dest_fd);

  close(dest_fd);
  close(src_fd);

  return err;
}

int set_nonblock(int fd) {
  long flag = fcntl(fd, F_GETFL);
  int ret = -1;

  if (flag != -1) {
    flag |= O_NONBLOCK;
    ret = fcntl(fd, F_SETFL, flag);
    if (ret < 0) {
      return -1;
    }
    ret = 0;
  }

  return ret;
}

void data2HexString(uint8_t* out_buf, const uint8_t* data, size_t len) {
  static uint8_t s_hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                              '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
  int n = 0;
  for (size_t i = 0; i < len; ++i, n += 2) {
    out_buf[n] = s_hex[data[i] >> 4];
    out_buf[n + 1] = s_hex[data[i] & 0xf];
  }
}
