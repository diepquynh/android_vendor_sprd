/*
 *  diag_cmd_def.h - diag stream parser definitions
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-8-15 Cao Xiaoyin
 *  Initial version.
 */

#ifndef DIAG_CMD_DEF_H__
#define DIAG_CMD_DEF_H__

#include "cp_log_cmn.h"

#define FLAG_BYTE 0x7e
#define ESCAPE_BYTE 0x7d
#define COMPLEMENT_BYTE 0x20

typedef enum { PPP_DOWN = 0, PPP_READY, PPP_HALT, PPP_UP } DiagParseState;

#define DIAG_READ_RING_BUFFER 58
#define DIAG_READ_SLEEP_LOG 59

#define DIAG_REQ_RING_BUFFER 0
#define DIAG_RSP_RING_BUFFER_OK 1
#define DIAG_RSP_RING_BUFFER_FAIL 2
#define DIAG_RSP_RING_BUFFER_ERR 3
#define DIAG_DATA_RING_BUFFER 100

#define DIAG_REQ_SLEEP_LOG 0
#define DIAG_RSP_SLEEP_LOG_OK 1
#define DIAG_RSP_SLEEP_LOG_FAIL 2
#define DIAG_RSP_SLEEP_LOG_ERR 3
#define DIAG_DATA_SLEEP_LOG 100

struct diag_cmd_head {
  uint32_t seq_num;
  uint16_t data_len;
  uint8_t type;
  uint8_t subtype;
};

#endif
