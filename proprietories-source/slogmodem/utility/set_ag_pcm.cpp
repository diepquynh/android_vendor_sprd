/*
 *  set_ag_pcm.cpp - set AG-DSP PCM request class.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-7-20 Zhang Ziyi
 *  Initial version.
 */

#include "set_ag_pcm.h"

SetAgdspPcm::SetAgdspPcm(bool pcm_on)
    :m_pcm_on{pcm_on} {}

int SetAgdspPcm::do_request() {
  uint8_t buf[32];
  size_t len;

  memcpy(buf, "SET_AGDSP_PCM_OUTPUT ", 21);
  if (m_pcm_on) {
    memcpy(buf + 21, "ON\n", 3);
    len = 24;
  } else {
    memcpy(buf + 21, "OFF\n", 4);
    len = 25;
  }
  if (send_req(buf, len)) {
    report_error(RE_SEND_CMD_ERROR);
    return -1;
  }

  return wait_simple_response(DEFAULT_RSP_TIME);
}
