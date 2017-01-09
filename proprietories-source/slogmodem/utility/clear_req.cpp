/*
 *  clear_req.cpp - clear request class.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-7-4 Zhang Ziyi
 *  Initial version.
 */

#include "clear_req.h"

int ClearRequest::do_request() {
  if (send_req("slogctl clear\n", 14)) {
    report_error(RE_SEND_CMD_ERROR);
    return -1;
  }

  return wait_simple_response(DEFAULT_RSP_TIME);
}
