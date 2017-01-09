/*
 *  stor_mgr_sd_a6.cpp - external SD mount state for Sprdroid 6.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-10-27 Zhang Ziyi
 *  Initial version.
 */
#include <unistd.h>
#include "iqfeed.h"

bool get_sd_state(const android::String8& dir) 
{
	return !access(dir.string(), R_OK | W_OK);
}
