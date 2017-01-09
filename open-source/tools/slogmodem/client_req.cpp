/*
 *  client_req.cpp - Client request parsing functions.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-2-16 Zhang Ziyi
 *  Initial version.
 */
#include <cstddef>
#include <cstring>
#include <unistd.h>

#include "client_req.h"
#include "parse_utils.h"

int send_response(int conn, ResponseErrorCode err) {
  char resp[32];
  size_t len;
  int ret = -1;

  if (REC_SUCCESS == err) {
    memcpy(resp, "OK\n", 3);
    len = 3;
  } else {
    len = snprintf(resp, sizeof resp, "ERROR %d\n", err);
  }

  ssize_t n = write(conn, resp, len);
  if (static_cast<size_t>(n) == len) {
    ret = 0;
  }

  return ret;
}

int send_log_state_response(int conn, bool enabled) {
  char resp[32];
  int len = snprintf(resp, 32, "OK %s\n", enabled ? "ON" : "OFF");
  ssize_t n = write(conn, resp, len);
  int ret = -1;

  if (len == n) {
    ret = 0;
  }

  return ret;
}

CpType get_cp_type(const uint8_t* cp, size_t len) {
  CpType t = CT_UNKNOWN;

  switch (len) {
    case 2:
      if (!memcmp(cp, "TD", 2)) {
        t = CT_TD;
      }
      break;
    case 3:
      if (!memcmp(cp, "WCN", 3)) {
        t = CT_WCN;
      }
      break;
    case 4:
      if (!memcmp(cp, "GNSS", 4)) {
        t = CT_GNSS;
      }
      break;
    case 5:
      if (!memcmp(cp, "WCDMA", 5)) {
        t = CT_WCDMA;
      } else if (!memcmp(cp, "5MODE", 5)) {
        t = CT_5MODE;
      }
      break;
    case 6:
      if (!memcmp(cp, "AG-DSP", 6)) {
        t = CT_AGDSP;
      }
      break;
    case 7:
      if (!memcmp(cp, "TDD-LTE", 7)) {
        t = CT_3MODE;
      } else if (!memcmp(cp, "FDD-LTE", 7)) {
        t = CT_4MODE;
      }
      break;
    default:
      break;
  }

  return t;
}

int parse_modem_set(const uint8_t* req, size_t len, ModemSet& ms) {
  size_t tok_len;
  const uint8_t* endp = req + len;
  const uint8_t* token;
  int err = 0;
  int n = 0;

  while (req < endp) {
    token = get_token(req, len, tok_len);
    if (!token) {
      break;
    }
    if (n >= MAX_MODEM_NUM) {
      err = -1;
      break;
    }
    CpType t = get_cp_type(token, tok_len);
    if (CT_UNKNOWN == t) {
      err = -1;
      break;
    }
    // Duplicate CP?
    int i;
    for (i = 0; i < n; ++i) {
      if (ms.modems[i] == t) {
        break;
      }
    }
    if (i < n) {
      err = -1;
      break;
    }
    ms.modems[n] = t;
    ++n;
    req = token + tok_len;
  }
  ms.num = n;

  return err;
}

int put_cp_type(uint8_t* buf, size_t len, CpType t, size_t& tlen) {
  int ret = -1;

  switch (t) {
    case CT_WCDMA:
      if (len >= 5) {
        memcpy(buf, "WCDMA", 5);
        tlen = 5;
        ret = 0;
      }
      break;
    case CT_TD:
      if (len >= 2) {
        memcpy(buf, "TD", 2);
        tlen = 2;
        ret = 0;
      }
      break;
    case CT_3MODE:
      if (len >= 7) {
        memcpy(buf, "TDD-LTE", 7);
        tlen = 7;
        ret = 0;
      }
      break;
    case CT_4MODE:
      if (len >= 7) {
        memcpy(buf, "FDD-LTE", 7);
        tlen = 7;
        ret = 0;
      }
      break;
    case CT_5MODE:
      if (len >= 5) {
        memcpy(buf, "5MODE", 5);
        tlen = 5;
        ret = 0;
      }
      break;
    case CT_WCN:
      if (len >= 3) {
        memcpy(buf, "WCN", 3);
        tlen = 3;
        ret = 0;
      }
      break;
    case CT_GNSS:
      if (len >= 4) {
        memcpy(buf, "GNSS", 4);
        tlen = 4;
        ret = 0;
      }
      break;
    case CT_AGDSP:
      if (len >= 6) {
        memcpy(buf, "AG-DSP", 6);
        tlen = 6;
        ret = 0;
      }
      break;
    default:
      break;
  }

  return ret;
}
