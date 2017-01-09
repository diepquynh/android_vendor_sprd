/*
 *  on_off_req.cpp - log enable/disable request class.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-7-19 Zhang Ziyi
 *  Initial version.
 */

#include "client_req.h"
#include "on_off_req.h"

OnOffRequest::OnOffRequest(bool enable, const LogVector<CpType>& types)
    :m_enable{enable},
     m_sys_list(types) {}

int OnOffRequest::do_request() {
  uint8_t* cmd;
  size_t len;

  cmd = prepare_cmd(len);
  if (!cmd) {
    return -1;
  }

  int err = send_req(cmd, len);

  delete [] cmd;

  if (err) {
    report_error(RE_SEND_CMD_ERROR);
    return -1;
  }

  return wait_simple_response(DEFAULT_RSP_TIME);
}

uint8_t* OnOffRequest::prepare_cmd(size_t& len) {
  size_t n = 13 + (m_sys_list.size() << 3);
  uint8_t* buf = new uint8_t[n];
  uint8_t* p;
  size_t mlen;
  size_t rlen;

  if (m_enable) {
    memcpy(buf, "ENABLE_LOG", 10);
    mlen = 10;
    p = buf + 10;
    rlen = n - 10;
  } else {
    memcpy(buf, "DISABLE_LOG", 11);
    mlen = 11;
    p = buf + 11;
    rlen = n - 11;
  }

  bool ok = true;

  for (unsigned i = 0; i < m_sys_list.size(); ++i) {
    if (!rlen) {
      ok = false;
      break;
    }

    *p = ' ';
    ++p;
    --rlen;
    ++mlen;

    size_t wr_len;
    int err = put_cp_type(p, rlen, m_sys_list[i], wr_len);

    if (err < 0) {
      ok = false;
      break;
    }

    p += wr_len;
    rlen -= wr_len;
    mlen += wr_len;
  }

  if (ok && rlen) {
    *p = '\n';
    len = mlen + 1;
  } else {
    delete [] buf;
    buf = nullptr;
  }

  return buf;
}
