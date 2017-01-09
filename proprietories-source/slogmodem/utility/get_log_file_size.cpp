/*
 *  get_log_file_size.cpp - get log file's size.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-11-4 YAN Zhihang
 *  Initial version.
 */
#include "cplogctl_cmn.h"
#include "get_log_file_size.h"

int GetLogFileSize::do_request() {
  if (send_req("GET_LOG_FILE_SIZE\n", 18)) {
    report_error(RE_SEND_CMD_ERROR);
    return -1;
  }

  return parse_query_result();
}

int GetLogFileSize::parse_query_result() {
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

      if ((' ' == *res) && (res_len > 1)) {
        ++res;
        --res_len;
        LogString str_size;
        str_assign(str_size, res, res_len);

        unsigned long number;
        const char* endp;
        if (non_negative_number(ls2cstring(str_size), number, endp) &&
            spaces_only(endp)) {
          fprintf(stdout, "%s MB\n", ls2cstring(str_size));
        } else {
          ret = -1;
          fprintf(stderr, "%s is not a valid file size", ls2cstring(str_size));
          err_log("%s is not a valid file size", ls2cstring(str_size));
        }
      } else {
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
