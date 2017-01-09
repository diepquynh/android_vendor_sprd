/*
 *  set_ag_pcm.h - set AG-DSP PCM request class.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-7-20 Zhang Ziyi
 *  Initial version.
 */
#ifndef SET_AG_PCM_H_
#define SET_AG_PCM_H_

#include "slogm_req.h"

class SetAgdspPcm : public SlogmRequest {
 public:
  SetAgdspPcm(bool pcm_on);

 protected:
  /*  do_request - implement the request.
   *
   *  Return 0 on success, -1 on failure.
   */
  int do_request() override;

 private:
  bool m_pcm_on;
};

#endif  // !SET_AG_PCM_H_
