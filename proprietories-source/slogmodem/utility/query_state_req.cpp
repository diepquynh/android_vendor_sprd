/*
 *  query_state_req.cpp - query the cp log's state
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-8-25 YAN Zhihang
 *  Initial version.
 */
#include "query_state_req.h"

QueryStateRequest::QueryStateRequest(const char* cp_name, CpType type)
    : m_type{type},
      m_cp_name{cp_name} {}

int QueryStateRequest::do_request() {
  //"GET_LOG_STATE <cp_name>\n"
  uint8_t buf[32];

  memcpy(buf, "GET_LOG_STATE ", 14);

  uint8_t* p = buf + 14;
  size_t rlen = sizeof buf - 14;
  size_t tlen = 0;
  int err = put_cp_type(p, rlen, m_type, tlen);

  if (err || (rlen == tlen)) {
    fprintf(stderr,
            "Can not make the subsystem parameter of GET_LOG_STATE\n");
    return -1;
  }

  p += tlen;
  *p = '\n';
  err = send_req(buf, tlen + 15);

  if (err) {
    report_error(RE_SEND_CMD_ERROR);
    return -1;
  }

  return parse_query_result();
}

int QueryStateRequest::parse_query_result() {
  char resp[SLOGM_RSP_LENGTH];
  size_t rlen = SLOGM_RSP_LENGTH;

  int ret = wait_response(resp, rlen, DEFAULT_RSP_TIME);

  if (ret) {
    report_error(RE_WAIT_RSP_ERROR);
    return -1;
  }

  ResponseErrorCode err_code;
  const void* stop_ptr;

  ret = parse_result(resp, rlen, err_code, stop_ptr);
  if (ret) {
    LogString sd;

    str_assign(sd, resp, rlen);
    fprintf(stderr, "Invalid response: %s\n", ls2cstring(sd));
    err_log("invalid response: %s", ls2cstring(sd));
  } else {
    if (REC_SUCCESS != err_code) {
      const char* err_str = resp_code_to_string(err_code);
      fprintf(stderr, "Error: %d(%s)\n", static_cast<int>(err_code),
              err_str);
      err_log("error: %d(%s)", static_cast<int>(err_code),
              err_str);
      ret = -1;
    } else {
      char* res = static_cast<char*>(const_cast<void*>(stop_ptr));
      size_t res_len = static_cast<size_t>(rlen - (res - &resp[0]));

      if ((' ' == *res) && (res_len > 2)) {
        ++res;
        --res_len;

        if ((res_len == 2) && !memcmp(res, "ON", res_len)) {
          printf("%s:%s\n", m_cp_name, "on");
        } else if ((res_len == 3) && !memcmp(res, "OFF", res_len)) {
          printf("%s:%s\n", m_cp_name, "off");
        } else {
          ret = -1;
          LogString res_err;

          str_assign(res_err, res, res_len);
          fprintf(stderr, "state query return: %s error\n", ls2cstring(res_err));
          err_log("state query return: %s error", ls2cstring(res_err));
        }
      } else {
        ret = -1;
        LogString invalid_res_err;

        str_assign(invalid_res_err, resp, rlen);
        fprintf(stderr, "Invalid state response: %s\n",
                ls2cstring(invalid_res_err));
        err_log("invalid state response: %s", ls2cstring(invalid_res_err));
      }
    }
  }

  return ret;
}
