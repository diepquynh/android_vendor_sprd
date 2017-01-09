/*
 *  conf_ext_sd_a6.cpp - The external SD function on Sprdroid 6.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-10-20 Zhang Ziyi
 *  Initial version.
 */

#include <cstdlib>
#ifdef HOST_TEST_
#include "prop_test.h"
#else
#include <cutils/properties.h>
#endif

#include "cp_log_cmn.h"
#include "log_config.h"

bool LogConfig::get_sd_root() {
  // On Sprdroid 6:
  // If sys.internal.emulated is 1, the mount point is in
  // PHYSICAL_STORAGE.
  // If sys.internal.emulated is 0, the mount point is in
  // PHYSICAL1_STORAGE.

  char val[PROPERTY_VALUE_MAX];

  property_get("sys.internal.emulated", val, "");
  if (!val[0]) {
    err_log("sys.internal.type non exist");
    return false;
  }

  unsigned long type;
  char* endp;
  bool ret = false;

  type = strtoul(val, &endp, 0);
  if (1 == type) {
    char* path = getenv("PHYSICAL_STORAGE");
    if (path) {
      m_sd_top_dir = path;
      ret = true;
    } else {
      err_log("no PHYSICAL_STORAGE");
    }
  } else if (!type) {
    char* path = getenv("PHYSICAL1_STORAGE");
    if (path) {
      m_sd_top_dir = path;
      ret = true;
    } else {
      err_log("no PHYSICAL1_STORAGE");
    }
  } else {
    err_log("unexpected sys.internal.emulated %s", val);
  }

  return ret;
}
