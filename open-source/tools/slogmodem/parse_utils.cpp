/*
 *  parse_utils.cpp - The parsing utility functions.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-2-16 Zhang Ziyi
 *  Initial version.
 */
#include <cstring>
#include <climits>
#include <cctype>
#include "parse_utils.h"

const uint8_t* get_token(const uint8_t* buf, size_t& tlen) {
  // Search for the first non-blank
  while (*buf) {
    uint8_t c = *buf;
    if (' ' != c && '\t' != c && '\r' != c && '\n' != c) {
      break;
    }
    ++buf;
  }
  if (!*buf) {
    return 0;
  }

  // Search for the first blank
  const uint8_t* p = buf + 1;
  while (*p) {
    uint8_t c = *p;
    if (' ' == c || '\t' == c || '\r' == c || '\n' == c) {
      break;
    }
    ++p;
  }

  tlen = p - buf;
  return buf;
}

const uint8_t* get_token(const uint8_t* data, size_t len, size_t& tlen) {
  // Search for the first non-blank
  const uint8_t* endp = data + len;

  while (data < endp) {
    uint8_t c = *data;
    if (' ' != c && '\t' != c && '\r' != c && '\n' != c) {
      break;
    }
    ++data;
  }

  if (data == endp) {
    return 0;
  }

  const uint8_t* p1 = data + 1;
  while (p1 < endp) {
    uint8_t c = *p1;
    if (' ' == c || '\t' == c || '\r' == c || '\n' == c || '\0' == c) {
      break;
    }
    ++p1;
  }

  tlen = p1 - data;

  return data;
}

const uint8_t* find_str(const uint8_t* data, size_t len, const uint8_t* pat,
                        size_t pat_len) {
  const uint8_t* d_end = data + len;
  const uint8_t* p_end = data + pat_len;
  const uint8_t* p = 0;

  while (p_end <= d_end) {
    if (!memcmp(data, pat, pat_len)) {
      p = data;
      break;
    }
    ++data;
    ++p_end;
  }

  return p;
}

int parse_number(const uint8_t* data, size_t len, unsigned& num) {
  const uint8_t* endp = data + len;
  num = 0;

  while (data < endp) {
    int n = *data;
    if (n < '0' || n > '9') {
      return -1;
    }
    if (num >= UINT_MAX) {
      return -1;
    }
    n -= '0';
    num = num * 10 + n;
    ++data;
  }

  return 0;
}

int parse_number(const uint8_t* data, size_t len, unsigned& num,
                 size_t& parsed) {
  if (!len || !isdigit(data[0])) {
    return -1;
  }

  const uint8_t* endp = data + len;
  const uint8_t* p = data;

  num = 0;
  while (p < endp) {
    int n = *p;
    if (n < '0' || n > '9') {
      break;
    }
    if (num >= UINT_MAX) {
      return -1;
    }
    n -= '0';
    num = num * 10 + n;
    ++p;
  }

  parsed = p - data;
  return 0;
}
