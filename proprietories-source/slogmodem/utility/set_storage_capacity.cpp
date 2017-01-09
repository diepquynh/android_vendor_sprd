/*
 *  set_storage_capacity.cpp - set storage's max size for log saving.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-11-2 YAN Zhihang
 *  Initial version.
 */

#include "set_storage_capacity.h"

SetStorageCapacity::SetStorageCapacity(MediaType mt, unsigned size)
    : m_type{mt},
      m_size{size} {}

int SetStorageCapacity::do_request() {
  char buf[64];
  size_t len;

  if (MT_INTERNAL == m_type) {
    len = snprintf(buf, sizeof buf, "SET_DATA_MAX_SIZE %u\n", m_size);
  } else {  // MT_EXTERNAL == m_type
    len = snprintf(buf, sizeof buf, "SET_SD_MAX_SIZE %u\n", m_size);
  }

  if (send_req(buf, len)) {
    report_error(RE_SEND_CMD_ERROR);
    return -1;
  }

  return wait_simple_response(DEFAULT_RSP_TIME);
}
