/*
 *  flush_req.cpp - flush request class.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-7-4 Zhang Ziyi
 *  Initial version.
 */

#include "flush_req.h"

int FlushRequest::do_request() {
  if (send_req("FLUSH\n", 6)) {
    report_error(RE_SEND_CMD_ERROR);
    return -1;
  }

  return wait_simple_response(DEFAULT_RSP_TIME);

}
