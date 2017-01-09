/*
 *  set_storage_capacity.h - set storage's max size for log saving.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-11-2 YAN Zhihang
 *  Initial version.
 */
#ifndef SET_STORAGE_CAPACITY_H_
#define SET_STORAGE_CAPACITY_H_

#include "cplogctl_cmn.h"
#include "slogm_req.h"

class SetStorageCapacity : public SlogmRequest {
 public:
  SetStorageCapacity(MediaType mt, unsigned size);

 protected:
  /*  do_request - implement the request.
   *
   *  Return 0 on success, -1 on failure.
   */
  int do_request() override;

 private:
  MediaType m_type;
  unsigned m_size;
};

#endif  // !SET_STORAGE_CAPACITY_H_
