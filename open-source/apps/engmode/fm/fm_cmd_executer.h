/*
 * Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 * Authors:<george.wang@spreadtrum.com>
 *
 * Function:FM lava auto test tool
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <fmr.h>

#ifndef MODULE_A_H
#define MODULE_A_H
extern "C" int fm_runcommand(int client_fd, int argc, char **argv);
#endif

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "FMENG"

#define FMENG_DEBUG
#define FMENG_LOGD(x...) ALOGD( x )
#define FMENG_LOGE(x...) ALOGE( x )

#define OK_STR "OK"
#define FAIL_STR "FAIL"
#define CMD_RESULT_BUFFER_LEN (128)
#define FMR_MAX_IDX 1
extern struct fmr_ds *pfmr_data[FMR_MAX_IDX];

/*  errno */
typedef enum
{
    FM_ENG_NONE_ERROR,
    FM_ENG_CMD_INVALID,
    FM_ENG_INIT_ERROR,
    FM_ENG_ENABLE_ERROR,
    FM_ENG_DISABLE_ERROR,
    FM_ENG_TUNE_ERROR,
    FM_ENG_GETRSSI_ERROR
}FM_ENG_ERROR_E;

