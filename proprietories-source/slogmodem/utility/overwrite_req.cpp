/*
 *  overwrite_req.cpp - request to enable/disable/query overwrite
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-11-2 YAN Zhihang
 *  Initial version.
 */

#include "cplogctl_cmn.h"
#include "overwrite_req.h"
#include "parse_utils.h"

OverwriteRequest::OverwriteRequest(OverwriteCmdType cmd_type)
    : m_cmd_type{cmd_type} {}

int OverwriteRequest::do_request() {
  uint8_t buf[32];
  size_t len;

  if (OCT_ENABLE == m_cmd_type) {
    memcpy(buf, "ENABLE_LOG_OVERWRITE\n", 21);
    len = 21;
  } else if (OCT_DISABLE == m_cmd_type) {
    memcpy(buf, "DISABLE_LOG_OVERWRITE\n", 22);
    len = 22;
  } else {
  // OCT_QUERY == cmd_type
    memcpy(buf, "GET_LOG_OVERWRITE\n", 18);
    len = 18;
  }

  int err = send_req(buf, len);

  if (err) {
    report_error(RE_SEND_CMD_ERROR);
    return -1;
  }

  return parse_overwrite_result();
}

int OverwriteRequest::parse_overwrite_result() {
  if (OCT_QUERY != m_cmd_type) {
    return wait_simple_response(DEFAULT_RSP_TIME);
  } else {
    // query command: GET_LOG_OVERWRITE
    // Response:
    //   OK ENABLE | DISABLE
    //   ERROR <err_code>
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
      if (REC_SUCCESS != err_code) {  // ERROR
        const char* err_str = resp_code_to_string(err_code);
        fprintf(stderr, "Error: %d(%s)\n", static_cast<int>(err_code),
                err_str);
        err_log("error: %d(%s)", static_cast<int>(err_code),
                err_str);
        ret = -1;
      } else {  // OK
        char* res = static_cast<char*>(const_cast<void*>(stop_ptr));
        size_t res_len = static_cast<size_t>(rlen - (res - &resp[0]));
        const uint8_t* tok;
        size_t tlen;

        // SlogmRequest::parse_result only knows the response begins with
        // "OK".
        if ((' ' == *res) &&
            (tok = get_token(reinterpret_cast<uint8_t*>(res + 1),
                             res_len - 1, tlen))) {
          const uint8_t* remain = tok + tlen;
          size_t trailing_len =
              res_len - (tok + tlen - reinterpret_cast<uint8_t*>(res));

          if (6 == tlen &&
              !memcmp(tok, "ENABLE", 6) &&
              spaces_only(remain, trailing_len)) {
            fprintf(stdout, "overwrite:%s\n", "enabled");
          } else if (7 == tlen &&
                     !memcmp(tok, "DISABLE", tlen) &&
                     spaces_only(remain, trailing_len)) {
            fprintf(stdout, "overwrite:%s\n", "disabled");
          } else {  // Invalid OK response
            ret = -1;
            LogString res_err;

            str_assign(res_err, res, res_len);
            fprintf(stderr, "overwrite query return: %s error\n",
                    ls2cstring(res_err));
            err_log("overwrite query return: %s error", ls2cstring(res_err));
          }
        } else {  // Invalid OK response
          ret = -1;
          LogString invalid_res_err;

          str_assign(invalid_res_err, resp, rlen);
          fprintf(stderr, "Invalid response: %s\n", ls2cstring(invalid_res_err));
          err_log("invalid response: %s", ls2cstring(invalid_res_err));
        }
      }
    }

    return ret;
  }
}
