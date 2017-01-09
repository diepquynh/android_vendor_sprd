/*
 *  stor_mgr_sd_a45.cpp - external SD mount state for Sprdroid 4/5.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-10-27 Zhang Ziyi
 *  Initial version.
 */
#include "iqfeed.h"

bool get_sd_state(const android::String8& dir)
{
	char val[PROPERTY_VALUE_MAX];

	property_get("init.svc.fuse_sdcard0", val, "");

	return !strcmp(val, "running");
}

