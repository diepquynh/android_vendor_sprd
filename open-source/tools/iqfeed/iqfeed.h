/*
 *  iqfeed.h - Common declarations for iqfeed.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-9-22 Zhang Ziyi
 *  Initial version.
 */
#ifndef _IQFEED_H_
#define _IQFEED_H_

#define LOG_TAG "WIQFEED"

#include <cutils/log.h>
#include <cutils/properties.h>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <utils/String8.h>

#define err_log(fmt, arg...) ALOGE("%s: " fmt " [%d]\n", __func__, ## arg, errno)
#define warn_log(fmt,arg...) ALOGW("%s: " fmt "\n", __func__, ## arg)
#define info_log(fmt,arg...) ALOGI("%s: " fmt "\n", __func__, ## arg)

bool get_sd_root(android::String8& dir);
bool get_sd_state(const android::String8& dir);
#endif  // !_IQFEED_H_
