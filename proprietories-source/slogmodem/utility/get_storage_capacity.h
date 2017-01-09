/*
 *  get_storage_capacity.h - get storage's max size for log saving.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-11-2 YAN Zhihang
 *  Initial version.
 */
#ifndef GET_STORAGE_CAPACITY_H_
#define GET_STORAGE_CAPACITY_H_

#include "cplogctl_cmn.h"
#include "slogm_req.h"

class GetStorageCapacity : public SlogmRequest {
 public:
  GetStorageCapacity(MediaType type);

 protected:
  /*  do_request - implement the request.
   *
   *  Return 0 on success, -1 on failure.
   */
  int do_request() override;

 private:
  MediaType m_type;

  int parse_query_result();
};

#endif  // !GET_STORAGE_CAPACITY_H_
