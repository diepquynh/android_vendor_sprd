/*
 *  set_ag_log_dest.cpp - set AG-DSP log destination request class.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-7-20 Zhang Ziyi
 *  Initial version.
 */

#include "set_ag_log_dest.h"

SetAgdspLogDest::SetAgdspLogDest(AgDspLogDest dest)
    :m_dest{dest} {}

int SetAgdspLogDest::do_request() {
  uint8_t buf[32];
  size_t len;

  memcpy(buf, "SET_AGDSP_LOG_OUTPUT ", 21);
  switch (m_dest) {
    case AGDSP_LOG_DEST_OFF:
      memcpy(buf + 21, "OFF\n", 4);
      len = 25;
      break;
    case AGDSP_LOG_DEST_UART:
      memcpy(buf + 21, "UART\n", 5);
      len = 26;
      break;
    default:
    //case AGDSP_LOG_DEST_AP:
      memcpy(buf + 21, "USB\n", 4);
      len = 25;
      break;
  }

  if (send_req(buf, len)) {
    report_error(RE_SEND_CMD_ERROR);
    return -1;
  }

  return wait_simple_response(DEFAULT_RSP_TIME);
}
