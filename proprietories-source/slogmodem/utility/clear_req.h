/*
 *  clear_req.h - clear request class.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-7-4 Zhang Ziyi
 *  Initial version.
 */
#ifndef CLEAR_REQ_H_
#define CLEAR_REQ_H_

#include "slogm_req.h"

class ClearRequest : public SlogmRequest {
 protected:
  /*  do_request - implement the request.
   *
   *  Return 0 on success, -1 on failure.
   */
  int do_request() override;
};

#endif  // !CLEAR_REQ_H_
