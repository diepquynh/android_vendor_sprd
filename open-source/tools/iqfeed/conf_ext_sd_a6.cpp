/*
 *  conf_ext_sd_a6.cpp - The external SD function on Sprdroid 6.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-10-20 Zhang Ziyi
 *  Initial version.
 */
#include "iqfeed.h"

bool get_sd_root(android::String8& dir)
{
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
			dir = path;
			ret = true;
		} else {
			err_log("no PHYSICAL_STORAGE");
		}
	} else if (!type) {
		char* path = getenv("PHYSICAL1_STORAGE");
		if (path) {
			dir = path;
			ret = true;
		} else {
			err_log("no PHYSICAL1_STORAGE");
		}
	} else {
		err_log("unexpected sys.internal.emulated %s", val);
	}

	return ret;
}
