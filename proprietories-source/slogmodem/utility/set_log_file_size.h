/*
 *  set_log_file_size.h - set log file's max size.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-11-4 YAN Zhihang
 *  Initial version.
 */
#ifndef SET_LOG_FILE_SIZE_H_
#define SET_LOG_FILE_SIZE_H_

#include "slogm_req.h"

class SetLogFileSize : public SlogmRequest {
 public:
  SetLogFileSize(unsigned size);

 protected:
  /*  do_request - implement the request.
   *
   *  Return 0 on success, -1 on failure.
   */
  int do_request() override;

 private:
  unsigned m_size;
};

#endif  // !SET_LOG_FILE_SIZE_H_
