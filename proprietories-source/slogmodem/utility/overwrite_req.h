/*
 *  overwrite_req.h - request to enable/disable/query overwrite
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-11-2 YAN Zhihang
 *  Initial version.
 */
#ifndef OVERWRITE_REQ_H_
#define OVERWRITE_REQ_H_

#include "slogm_req.h"

class OverwriteRequest : public SlogmRequest {
 public:
  enum OverwriteCmdType { OCT_ENABLE, OCT_DISABLE, OCT_QUERY };
  OverwriteRequest(OverwriteCmdType oct);

 protected:
  /*  do_request - implement the request.
   *
   *  Return 0 on success, -1 on failure.
   */
  int do_request() override;

 private:
  OverwriteCmdType m_cmd_type;

  int parse_overwrite_result();
};

#endif  // !OVERWRITE_REQ_H_
