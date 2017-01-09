/*
 *  query_state_req.h - query the cp log's state
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-8-25 YAN Zhihang
 *  Initial version.
 */

#ifndef QUERY_STATE_REQ_H_
#define QUERY_STATE_REQ_H_

#include "slogm_req.h"

class QueryStateRequest : public SlogmRequest {
 public:
  QueryStateRequest(const char* cp_name, CpType type);

 protected:
  int do_request() override;

 private:
  CpType m_type;
  const char* m_cp_name;

  int parse_query_result();
};

#endif  //!QUERY_STATE_REQ_H_
