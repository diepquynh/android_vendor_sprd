/*
 *  client_req.h - Client request parsing functions declarations.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-2-16 Zhang Ziyi
 *  Initial version.
 */
#ifndef _CLIENT_REQ_H_
#define _CLIENT_REQ_H_

#include "cp_log_cmn.h"

// Error codes
#define CLI_REQ_SUCCESS 0
#define CLI_REQ_INCOMPLETE (-1)
#define CLI_REQ_INVAL_REQ (-2)

enum RequestType {
  RT_ENABLE_LOG,
  RT_DISABLE_LOG,
  RT_ENABLE_MINI_DUMP,
  RT_DISABLE_MINI_DUMP,
  RT_MINI_DUMP
};

// Response error code
enum ResponseErrorCode {
  REC_SUCCESS,
  REC_UNKNOWN_REQ,
  REC_INVAL_PARAM,
  REC_FAILURE,
  REC_IN_TRANSACTION,
  REC_LOG_DISABLED,
  REC_SLEEP_LOG_NOT_SUPPORTED,
  REC_RINGBUF_NOT_SUPPORTED,
  REC_CP_ASSERTED,
  REC_UNKNOWN_CP_TYPE
};

#define MAX_MODEM_NUM 5

struct ModemSet {
  int num;
  enum CpType modems[MAX_MODEM_NUM];
};

struct ClientRequest {
  enum RequestType type;
  union {
    struct ModemSet modems;
  } param;
};

int send_response(int conn, ResponseErrorCode err);
int send_log_state_response(int conn, bool enabled);
int parse_modem_set(const uint8_t* req, size_t len, ModemSet& ms);
CpType get_cp_type(const uint8_t* cp, size_t len);
int put_cp_type(uint8_t* buf, size_t len, CpType t, size_t& tlen);

#endif  // !_CLIENT_REQ_H_
