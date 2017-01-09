/*
 *  on_off_req.h - log enable/disable request class.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-7-19 Zhang Ziyi
 *  Initial version.
 */
#ifndef ON_OFF_REQ_H_
#define ON_OFF_REQ_H_

#include "slogm_req.h"

class OnOffRequest : public SlogmRequest {
 public:
  OnOffRequest(bool enable, const LogVector<CpType>& types);

 protected:
  /*  do_request - implement the request.
   *
   *  Return 0 on success, -1 on failure.
   */
  int do_request() override;

 private:
  bool m_enable;
  LogVector<CpType> m_sys_list;

  uint8_t* prepare_cmd(size_t& len);
};

#endif  // !ON_OFF_REQ_H_
