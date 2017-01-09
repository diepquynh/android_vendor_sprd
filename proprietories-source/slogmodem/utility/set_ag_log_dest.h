/*
 *  set_ag_log_dest.h - set AG-DSP log destination request class.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-7-20 Zhang Ziyi
 *  Initial version.
 */
#ifndef SET_AG_LOG_DEST_H_
#define SET_AG_LOG_DEST_H_

#include "slogm_req.h"

class SetAgdspLogDest : public SlogmRequest {
 public:
  enum AgDspLogDest {
    AGDSP_LOG_DEST_OFF,
    AGDSP_LOG_DEST_UART,
    AGDSP_LOG_DEST_AP
  };

  SetAgdspLogDest(AgDspLogDest dest);

 protected:
  /*  do_request - implement the request.
   *
   *  Return 0 on success, -1 on failure.
   */
  int do_request() override;

 private:
  AgDspLogDest m_dest;
};

#endif  // !SET_AG_LOG_DEST_H_
