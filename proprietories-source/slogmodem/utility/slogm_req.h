/*
 *  slogm_req.h - slogmodem request base class.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-7-4 Zhang Ziyi
 *  Initial version.
 */
#ifndef SLOGM_REQ_H_
#define SLOGM_REQ_H_

#include "client_req.h"

class SlogmRequest {
 public:
  SlogmRequest();
  virtual ~SlogmRequest();

  // Execute the reqest
  int exec();

 protected:
  enum RequestError {
    RE_ERROR,
    RE_CONN_FAILURE,
    RE_SEND_CMD_ERROR,
    RE_WAIT_RSP_ERROR
  };

  static const int SLOGM_RSP_LENGTH = 32;
  static const int DEFAULT_RSP_TIME = 3000;

  /*  prepare - prepare for the request connection.
   *
   *  This function is called before sending request.
   *
   *  Return 0 on success, -1 on error.
   */
  int prepare_conn();

  int wait_simple_response(int wait_time);

  /*  wait_response - wait for the request response.
   *  @buf: the buffer to put response in
   *  @len: the length of the buffer. If the response is got, return the
   *        response length excluding the trailing '\n'
   *  @wait_time: the maximum time to wait for response, in millisecond
   *
   *  Return 0 on success, -1 on error.
   */
  int wait_response(void* buf, size_t& len, int wait_time);

  /*  do_request - implement the request.
   *
   *  Return 0 on success, -1 on failure.
   */
  virtual int do_request() = 0;

  int connect();
  int send_req(const void* cmd, size_t len);

  /*  parse_result - parse the result.
   *  @resp:  the result
   *  @len:   length of the result
   *  @result: the parsed result
   *  @stop_ptr: where the parsing stops
   *
   *  Return 0 on success, -1 on error.
   */
  int parse_result(const void* resp, size_t len,
                   ResponseErrorCode& result,
                   const void*& stop_ptr);

  static void report_error(RequestError err);

  static const char* resp_code_to_string(ResponseErrorCode err_code);

 private:
  int m_fd;
};

#endif  // !SLOGM_REQ_H_
