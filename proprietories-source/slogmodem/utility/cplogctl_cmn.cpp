/* cplogctl_cmn.h - common definitions for cplogctl
 *
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 * History:
 * 2016-11-15 YAN Zhihang
 * initial version
 */

#include <cctype>
#include <climits>
#include <cstdlib>
#include <errno.h>

#include "cplogctl_cmn.h"

bool non_negative_number(const char* result, unsigned long& number,
                         const char*& stop) {
  char* endp{nullptr};
  bool ret{true};

  // Forward to the first non-space
  const char* p = result;
  while (*p) {
    if (!isspace(*p)) {
      break;
    }
    ++p;
  }
  if (!*p) {
    return false;
  }

  number = strtoul(p, &endp, 0);

  if ((ULONG_MAX == number && ERANGE == errno) ||
      (!number && p == endp)) {
    ret = false;
  } else {
    stop = endp;
  }

  return ret;
}

bool spaces_only(const char* s) {
  bool ret{true};

  while (*s) {
    if (!isspace(*s)) {
      ret = false;
      break;
    }
    ++s;
  }

  return ret;
}

bool spaces_only(const uint8_t* remain, size_t len) {
  bool ret{true};

  for (size_t i = 0; i < len; ++i) {
    if (!isspace(remain[i])) {
      ret = false;
      break;
    }
  }

  return ret;
}
