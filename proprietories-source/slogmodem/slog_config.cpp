/*
 *  slog_config.cpp - The slog.conf parsing class implementation.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-4-23 Zhang Ziyi
 *  Initial version.
 */
#include <climits>
#include <cstdint>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdlib>

#include "parse_utils.h"
#include "slog_config.h"

SLogConfig::SLogConfig() : m_total_enable{false} {}

SLogConfig::~SLogConfig() { clear_ptr_container(m_config); }

int SLogConfig::read_config(const char* cfile) {
  uint8_t* buf;
  FILE* pf;
  int line_num = 0;

  buf = new uint8_t[1024];

  pf = fopen(cfile, "rt");
  if (!pf) {
    err_log("can not open %s", cfile);
    goto delBuf;
  }

  // Read config file
  while (true) {
    char* p = fgets(reinterpret_cast<char*>(buf), 1024, pf);
    if (!p) {
      break;
    }
    ++line_num;
    int err = parse_line(buf);
    if (-1 == err) {
      err_log("Invalid line: %d\n", line_num);
    }
  }

  fclose(pf);
  delete[] buf;

  if (!m_total_enable) {
    ConfigIter it;

    for (it = m_config.begin(); it != m_config.end(); ++it) {
      (*it)->enable = false;
    }
  }

  return 0;

delBuf:
  delete[] buf;
  return -1;
}

int SLogConfig::parse_stream_line(const uint8_t* buf) {
  const uint8_t* t;
  size_t tlen;
  const uint8_t* pn;
  size_t nlen;

  // Get the modem name
  pn = get_token(buf, nlen);
  if (!pn) {
    return -1;
  }
  CpType cp_type = get_modem_type(pn, nlen);
  if (CT_UNKNOWN == cp_type) {
    // Ignore unknown CP
    return 0;
  }

  // Get enable state
  bool is_enable;

  buf = pn + nlen;
  t = get_token(buf, tlen);
  if (!t) {
    return -1;
  }
  if (2 == tlen && !memcmp(t, "on", 2)) {
    is_enable = true;
  } else {
    is_enable = false;
  }

  // Size
  buf = t + tlen;
  t = get_token(buf, tlen);
  if (!t) {
    return -1;
  }
  char* endp;
  unsigned long sz = strtoul(reinterpret_cast<const char*>(t), &endp, 0);
  if ((ULONG_MAX == sz && ERANGE == errno) ||
      (' ' != *endp && '\t' != *endp && '\r' != *endp && '\n' != *endp &&
       '\0' != *endp)) {
    return -1;
  }

  // Level
  buf = reinterpret_cast<const uint8_t*>(endp);
  t = get_token(buf, tlen);
  if (!t) {
    return -1;
  }
  unsigned long level = strtoul(reinterpret_cast<const char*>(t), &endp, 0);
  if ((ULONG_MAX == level && ERANGE == errno) ||
      (' ' != *endp && '\t' != *endp && '\r' != *endp && '\n' != *endp &&
       '\0' != *endp)) {
    return -1;
  }

  ConfigEntry* pe = new ConfigEntry(cp_type, is_enable);
  m_config.push_back(pe);

  return 0;
}

int SLogConfig::parse_line(const uint8_t* buf) {
  // Search for the first token
  const uint8_t* t;
  size_t tlen;
  int err = 0;

  t = get_token(buf, tlen);
  if (!t || '#' == *t) {
    return 0;
  }

  // What line?
  buf += tlen;
  switch (tlen) {
    case 6:
      if (!memcmp(t, "stream", 6)) {
        err = parse_stream_line(buf);
      } else if (!memcmp(t, "enable", 6)) {
        m_total_enable = true;
      }
      break;
    default:
      break;
  }

  return err;
}

CpType SLogConfig::get_modem_type(const uint8_t* name, size_t len) {
  CpType type = CT_UNKNOWN;

  switch (len) {
    case 6:
      if (!memcmp(name, "cp_wcn", 6)) {
        type = CT_WCN;
      }
      break;
    case 7:
      if (!memcmp(name, "cp_gnss", 7)) {
        type = CT_GNSS;
      }
      break;
    case 8:
      if (!memcmp(name, "cp_wcdma", 8)) {
        type = CT_WCDMA;
      }
      break;
    case 9:
      if (!memcmp(name, "cp_td-lte", 9)) {
        type = CT_5MODE;
      }
      break;
    case 10:
      if (!memcmp(name, "cp_tdd-lte", 10)) {
        type = CT_3MODE;
      } else if (!memcmp(name, "cp_fdd-lte", 10)) {
        type = CT_4MODE;
      }
      break;
    case 11:
      if (!memcmp(name, "cp_td-scdma", 11)) {
        type = CT_TD;
      }
      break;
    default:
      break;
  }

  return type;
}
