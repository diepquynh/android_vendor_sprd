/*
 *  get_log_file_size.h - get log file's size.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-11-4 YAN Zhihang
 *  Initial version.
 */
#ifndef GET_LOG_FILE_SIZE_H_
#define GET_LOG_FILE_SIZE_H_

#include "slogm_req.h"

class GetLogFileSize : public SlogmRequest {
 protected:
  /*  do_request - implement the request.
   *
   *  Return 0 on success, -1 on failure.
   */
  int do_request() override;

 private:
  int parse_query_result();
};

#endif  // !GET_LOG_FILE_SIZE_H_
