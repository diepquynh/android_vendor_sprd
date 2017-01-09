/*
 *  flush_req.h - flush request class.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-7-4 Zhang Ziyi
 *  Initial version.
 */
#ifndef FLUSH_REQ_H_
#define FLUSH_REQ_H_

#include "slogm_req.h"

class FlushRequest : public SlogmRequest {
 protected:
  /*  do_request - implement the request.
   *
   *  Return 0 on success, -1 on failure.
   */
  int do_request() override;
};

#endif  // !FLUSH_REQ_H_
