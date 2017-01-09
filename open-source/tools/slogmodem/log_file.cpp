/*
 *  log_file.cpp - log file class.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-5-14 Zhang Ziyi
 *  Initial version.
 */

#include <cstring>
#include <cctype>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include "log_file.h"
#include "cp_dir.h"
#include "cp_set_dir.h"
#include "parse_utils.h"

LogFile::LogFile(const LogString& base_name, CpDirectory* dir,
                 LogType type /* = LT_UNKNOWN */, size_t sz /* = 0 */)
    : m_dir{dir},
      m_base_name(base_name),
      m_type{type},
      m_time{0, 0, 0, 0, 0, 0},
      m_size{sz},
      m_fd{-1},
      m_buffer{0},
      m_buf_len{0},
      m_data_len{0} {}

LogFile::LogFile(const LogString& base_name, CpDirectory* dir,
                 const struct tm& file_time)
    : m_dir{dir},
      m_base_name(base_name),
      m_type{LogFile::LT_LOG},
      m_time{file_time.tm_year + 1900, file_time.tm_mon + 1, file_time.tm_mday,
             file_time.tm_hour,        file_time.tm_min,     file_time.tm_sec},
      m_size{0},
      m_fd{-1},
      m_buffer{0},
      m_buf_len{0},
      m_data_len{0} {}

LogFile::~LogFile() { close(); }

int LogFile::get_size() {
  const CpSetDirectory* cp_set = m_dir->cp_set_dir();

  LogString file_path =
      cp_set->path() + "/" + m_dir->name() + "/" + m_base_name;
  struct stat file_stat;
  int ret = ::stat(ls2cstring(file_path), &file_stat);
  if (!ret) {
    m_size = static_cast<size_t>(file_stat.st_size);
  }

  return ret;
}

int LogFile::create() {
  if (m_fd >= 0) {
    return -1;
  }

  const CpSetDirectory* cp_set = m_dir->cp_set_dir();

  LogString file_path =
      cp_set->path() + "/" + m_dir->name() + "/" + m_base_name;
  m_fd = ::open(ls2cstring(file_path), O_CREAT | O_RDWR,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  int ret = -1;
  if (m_fd >= 0) {
    ret = 0;
    m_buffer = new uint8_t[FILE_IO_BUF_SIZE];
    m_buf_len = FILE_IO_BUF_SIZE;
  }
  return ret;
}

int LogFile::close() {
  if (m_fd >= 0) {
    if (m_data_len) {
      write_data(m_buffer, m_data_len);
    }
    ::close(m_fd);
    m_fd = -1;

    delete[] m_buffer;
    m_buffer = 0;
    m_buf_len = 0;
    m_data_len = 0;
  }

  return 0;
}

ssize_t LogFile::write(const void* data, size_t len) {
  if (m_fd < 0) {
    return -1;
  }

  ssize_t n;

  if (len + m_data_len < m_buf_len) {
    memcpy(m_buffer + m_data_len, data, len);
    m_data_len += len;
    m_size += len;
    m_dir->add_size(len);
    n = len;
  } else {
    // Write the data into the file
    if (m_data_len) {
      struct iovec v[2];
      int vnum;

      v[0].iov_base = m_buffer;
      v[0].iov_len = m_data_len;
      v[1].iov_base = const_cast<void*>(data);
      v[1].iov_len = len;
      vnum = 2;
      n = writev(m_fd, v, vnum);
      if (n >= static_cast<ssize_t>(m_data_len)) {
        n -= m_data_len;
        m_data_len = 0;
        m_size += n;
        m_dir->add_size(n);
      } else if (n > 0) {
        // m_buffer is not flushed
        // Move data in m_buffer
        m_data_len -= n;
        memmove(m_buffer, m_buffer + n, m_data_len);
        n = 0;
      }
    } else {
      n = write_data(data, len);
      if (n > 0) {
        m_size += n;
        m_dir->add_size(n);
      }
    }
    if (n < 0) {
      err_log("write file failed");
      if (ENOSPC == errno) {
        n = -2;
      }
    }
  }
  return n;
}

ssize_t LogFile::write_raw(const void* data, size_t len) {
  ssize_t n = write_data(data, len);

  if (n < 0) {
    if (ENOSPC == errno) {
      n = -2;
    }
  }

  return n;
}

void LogFile::count_size(size_t size) {
  m_size = size;
  m_dir->add_size(m_size);
}

LogFile::LogType LogFile::get_type() {
  LogType t = LT_UNKNOWN;
  unsigned num;
  size_t len;
  const char* s = ls2cstring(m_base_name);

  if (!parse_number(reinterpret_cast<const uint8_t*>(s), m_base_name.length(),
                    num, len)) {
    // The name start with a number
    if (len < m_base_name.length() && '-' == s[len]) {
      ++len;
      if (!extract_file_time(s + len, m_base_name.length() - len, m_time)) {
        t = LT_LOG;
      }
    }
  } else if (str_ends_with(m_base_name, ".version", 8)) {
    t = LT_VERSION;
  } else if (str_ends_with(m_base_name, ".dmp", 4)) {
    if (!parse_dump_file_time(s, m_base_name.length(), m_time)) {
      t = LT_DUMP;
    }
  } else if (str_starts_with(m_base_name, "mini_dump_", 10)) {
    if (!parse_time_string(s + 10, m_base_name.length() - 10, m_time)) {
      t = LT_MINI_DUMP;
    }
  } else if (str_starts_with(m_base_name, "sblock_", 7)) {
    if (!parse_time_string(s + 7, m_base_name.length() - 7, m_time)) {
      t = LT_SIPC;
    }
  } else if (str_starts_with(m_base_name, "sbuf_", 5)) {
    if (!parse_time_string(s + 5, m_base_name.length() - 5, m_time)) {
      t = LT_SIPC;
    }
  } else if (str_starts_with(m_base_name, "smsg_", 5)) {
    if (!parse_time_string(s + 5, m_base_name.length() - 5, m_time)) {
      t = LT_SIPC;
    }
  }

  m_type = t;
  return t;
}

int LogFile::parse_time_string(const char* name, size_t len,
                               LogFile::FileTime& ft) {
  if (len < 19) {
    return -1;
  }

  unsigned num;
  FileTime ft_tmp;

  if (parse_number(reinterpret_cast<const uint8_t*>(name), 4, num)) {
    return -1;
  }
  ft_tmp.year = static_cast<int>(num);

  if (parse_number(reinterpret_cast<const uint8_t*>(name + 5), 2, num)) {
    return -1;
  }
  ft_tmp.month = static_cast<int>(num);

  if (parse_number(reinterpret_cast<const uint8_t*>(name + 8), 2, num)) {
    return -1;
  }
  ft_tmp.mday = static_cast<int>(num);

  if (parse_number(reinterpret_cast<const uint8_t*>(name + 11), 2, num)) {
    return -1;
  }
  ft_tmp.hour = static_cast<int>(num);

  if (parse_number(reinterpret_cast<const uint8_t*>(name + 14), 2, num)) {
    return -1;
  }
  ft_tmp.min = static_cast<int>(num);

  if (parse_number(reinterpret_cast<const uint8_t*>(name + 17), 2, num)) {
    return -1;
  }
  ft_tmp.sec = static_cast<int>(num);

  ft = ft_tmp;

  return 0;
}

int LogFile::parse_dump_file_time(const char* name, size_t len,
                                  LogFile::FileTime& ft) {
  const char* endp = name + len;
  const char* p;
  unsigned num;
  bool is_year = false;

  while (name < endp) {
    p = reinterpret_cast<const char*>(memchr(name, '_', len));
    if (!p) {
      return -1;
    }
    ++p;
    len = endp - p;
    if (len >= 4 &&
        !parse_number(reinterpret_cast<const uint8_t*>(p), 4, num)) {
      is_year = true;
      break;
    }
    name = p;
  }

  if (!is_year || len < 19) {
    return -1;
  }

  return parse_time_string(p, 19, ft);
}

bool LogFile::FileTime::operator<(const LogFile::FileTime& t) const {
  if (year < t.year) {
    return true;
  } else if (year > t.year) {
    return false;
  }

  if (month < t.month) {
    return true;
  } else if (month > t.month) {
    return false;
  }

  if (mday < t.mday) {
    return true;
  } else if (mday > t.mday) {
    return false;
  }

  if (hour < t.hour) {
    return true;
  } else if (hour > t.hour) {
    return false;
  }

  if (min < t.min) {
    return true;
  } else if (min > t.min) {
    return false;
  }

  return sec < t.sec;
}

int LogFile::flush() {
  if (m_fd < 0) {
    return -1;
  }

  ssize_t n = write_data(m_buffer, m_data_len);
  m_data_len = 0;
  return static_cast<size_t>(n) == m_data_len ? 0 : -1;
}

ssize_t LogFile::write_data(const void* data, size_t len) {
  if (m_fd < 0) {
    err_log("write file not opened");
    return -1;
  }

  const uint8_t* p = reinterpret_cast<const uint8_t*>(data);
  const uint8_t* start = p;
  const uint8_t* endp = p + len;
  ssize_t n = 0;

  while (p < endp) {
    n = ::write(m_fd, p, len);
    if (n < 0) {
      break;
    }
    p += n;
    len -= n;
  }

  if (n >= 0) {
    n = static_cast<ssize_t>(p - start);
  }
  return n;
}

int LogFile::rotate(const LogString& par_path) {
  unsigned num;
  size_t len;

  int ret = parse_log_file_num(ls2cstring(m_base_name), m_base_name.length(),
                               num, len);
  if (ret) {
    err_log("invalid log file name %s", ls2cstring(m_base_name));
    return -1;
  }

  LogString new_name;
  char s[32];

  snprintf(s, sizeof s, "%u", num + 1);
  new_name = s;
  new_name += (ls2cstring(m_base_name) + len);

  LogString old_path = par_path + "/" + m_base_name;
  LogString new_path = par_path + "/" + new_name;

  ret = rename(ls2cstring(old_path), ls2cstring(new_path));
  if (ret) {
    err_log("rename %s failed", ls2cstring(m_base_name));
  } else {
    m_base_name = new_name;
  }

  return ret;
}

int LogFile::parse_log_file_num(const char* name, size_t len, unsigned& num,
                                size_t& nlen) {
  int ret =
      parse_number(reinterpret_cast<const uint8_t*>(name), len, num, nlen);
  if (ret) {
    return -1;
  }

  if ('-' != name[nlen]) {
    return -1;
  }
  return 0;
}

int LogFile::change_name_num(unsigned num) {
  unsigned cur_num;
  size_t len;

  if (parse_log_file_num(ls2cstring(m_base_name), m_base_name.length(), cur_num,
                         len)) {
    return -1;
  }

  if (num == cur_num) {  // No need to change
    return 0;
  }

  char s[32];
  int num_len = snprintf(s, sizeof s, "%u", num);
  LogString new_name;

  str_assign(new_name, s, num_len);
  new_name += (ls2cstring(m_base_name) + len);

  LogString par_path = m_dir->cp_set_dir()->path() + "/" + m_dir->name() + "/";
  LogString old_path = par_path + m_base_name;
  LogString new_path = par_path + new_name;

  int ret = rename(ls2cstring(old_path), ls2cstring(new_path));
  if (ret) {
    err_log("rename number in %s failed", ls2cstring(m_base_name));
  } else {
    m_base_name = new_name;
  }

  return ret;
}

int LogFile::remove(const LogString& par_dir) {
  LogString spath = par_dir + "/" + m_base_name;

  int ret = unlink(ls2cstring(spath));
  if (-1 == ret && ENOENT == errno) {
    ret = 0;
  } else if (-1 == ret) {
    err_log("delete %s error", ls2cstring(spath));
  }

  return ret;
}

int LogFile::copy(const char* src) {
  if (m_fd < 0) {
    return -1;
  }

  // Open the source and the destination file
  int src_fd;

  src_fd = open(src, O_RDONLY);
  if (-1 == src_fd) {
    err_log("open %s error", src);
    return -1;
  }

  int ret = -1;
  size_t total_save = 0;
  while (true) {
    ssize_t n = read(src_fd, m_buffer, FILE_IO_BUF_SIZE);
    if (n < 0) {
      break;
    }
    if (!n) {  // End of file
      ret = 0;
      break;
    }
    ssize_t nwr = ::write(m_fd, m_buffer, n);
    if (nwr != n) {
      if (nwr > 0) {
        total_save += nwr;
      }
      break;
    }
    total_save += n;
  }

  m_size += total_save;
  m_dir->add_size(total_save);

  ::close(src_fd);

  return ret;
}

int LogFile::discard() {
  int ret = 0;

  if (m_fd < 0) {
    return -1;
  }

  ret = ftruncate(m_fd, 0);
  if (ret < 0) {
    return -1;
  }
  ret = lseek(m_fd, 0, SEEK_SET);
  if (ret < 0) {
    return -1;
  }
  m_dir->dec_size(m_size);
  m_size = 0;

  m_data_len = 0;
  return 0;
}

const char* LogFile::get_4digits(const char* s, size_t len) {
  const char* endp = s + len;

  // Get four continuous digits
  while (s < endp) {
    if (isdigit(*s)) {
      size_t n = endp - s;
      if (n >= 4 && isdigit(s[1]) && isdigit(s[2]) && isdigit(s[3])) {
        break;
      }
    }
    ++s;
  }
  if (s == endp) {
    s = 0;
  }
  return s;
}

int LogFile::extract_file_time(const char* s, size_t len, FileTime& file_time) {
  const char* endp = s + len;
  int ret = -1;

  while (s < endp) {
    const char* p = get_4digits(s, len);
    if (!p) {
      break;
    }
    len = endp - p;
    if (len < 19) {
      break;
    }
    if (!parse_time_string(p, len, file_time)) {
      ret = 0;
      break;
    }
    s = p + 1;
    --len;
  }

  return ret;
}

bool LogFile::exists() const {
  const CpSetDirectory* cp_set = m_dir->cp_set_dir();
  LogString file_path =
      cp_set->path() + "/" + m_dir->name() + "/" + m_base_name;
  return 0 == access(ls2cstring(file_path), R_OK | W_OK);
}
