/*
 *  stor_mgr_sd_a45.cpp - external SD mount state for Sprdroid 4/5.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-10-27 Zhang Ziyi
 *  Initial version.
 */

#include <cstring>
#ifdef HOST_TEST_
#include "prop_test.h"
#else
#include <cutils/properties.h>
#endif
#include "stor_mgr.h"

bool StorageManager::get_sd_state() {
  char val[PROPERTY_VALUE_MAX];

  property_get("init.svc.fuse_sdcard0", val, "");

  return !strcmp(val, "running");
}
