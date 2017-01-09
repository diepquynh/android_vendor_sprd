/*
 *  stor_mgr_sd_a6.cpp - external SD mount state for Sprdroid 6.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-10-27 Zhang Ziyi
 *  Initial version.
 */

#ifdef HOST_TEST_
#include "prop_test.h"
#else
#include <cutils/properties.h>
#endif
#include <unistd.h>

#include "cp_log_cmn.h"
#include "stor_mgr.h"

bool StorageManager::get_sd_state() {
  return !access(ls2cstring(m_sd_card.get_root_dir_path()), R_OK | W_OK);
}
