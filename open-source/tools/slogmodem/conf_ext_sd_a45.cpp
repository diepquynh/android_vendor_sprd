/*
 *  conf_ext_sd_a45.cpp - The external SD function on Sprdroid 4/5.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-2-16 Zhang Ziyi
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
  // On Sprdroid 4/5:
  // If persist.storage.sdcard0 is 1, the mount point is in
  // EXTERNAL_STORAGE.
  // If persist.storage.sdcard0 is 2, the mount point is in
  // SECONDARY_STORAGE.

  char val[PROPERTY_VALUE_MAX];

  property_get("persist.storage.type", val, "");
  if (!val[0]) {
    err_log("persist.storage.type non exist");
    return false;
  }

  unsigned long type;
  char* endp;
  bool ret = false;

  type = strtoul(val, &endp, 0);
  if (1 == type) {
    char* path = getenv("EXTERNAL_STORAGE");
    if (path) {
      m_sd_top_dir = path;
      ret = true;
    } else {
      err_log("no EXTERNAL_STORAGE");
    }
  } else if (2 == type) {
    char* path = getenv("SECONDARY_STORAGE");
    if (path) {
      m_sd_top_dir = path;
      ret = true;
    } else {
      err_log("no SECONDARY_STORAGE");
    }
  } else {
    err_log("unexpected persist.storage.type %s", val);
  }

  return ret;
}
