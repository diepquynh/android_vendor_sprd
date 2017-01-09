/************************************************************************************
 *
 *  Copyright (C) 2009-2011 Broadcom Corporation
 *
 *  This program is the proprietary software of Broadcom Corporation and/or its
 *  licensors, and may only be used, duplicated, modified or distributed
 *  pursuant to the terms and conditions of a separate, written license
 *  agreement executed between you and Broadcom (an "Authorized License").
 *  Except as set forth in an Authorized License, Broadcom grants no license
 *  (express or implied), right to use, or waiver of any kind with respect to
 *  the Software, and Broadcom expressly reserves all rights in and to the
 *  Software and all intellectual property rights therein.
 *  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS
 *  SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
 *  ALL USE OF THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1.     This program, including its structure, sequence and organization,
 *         constitutes the valuable trade secrets of Broadcom, and you shall
 *         use all reasonable efforts to protect the confidentiality thereof,
 *         and to use this information only in connection with your use of
 *         Broadcom integrated circuit products.
 *
 *  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 *         "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 *         REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
 *         OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 *         DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 *         NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 *         ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 *         CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT
 *         OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *         ITS LICENSORS BE LIABLE FOR
 *         (i)   CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY
 *               DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO
 *               YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *               HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR
 *         (ii)  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
 *               SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE
 *               LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF
 *               ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 ************************************************************************************/
#ifndef BTAPP_H
#define BTAPP_H

/* btapp btu timer type IDs */
#define     BTAPP_BTU_TTYPE_FIRST   (200)       /* Timer types below 200 are reserved */
enum {
    BTAPP_BTU_TTYPE_DM_SSP_CFM_DELAY=BTAPP_BTU_TTYPE_FIRST,     /* Timer for delaying sending SSP User Cfm req to UI */

    BTAPP_BTU_TTYPE_LAST
};

/* Notification interval for object transfer progress (in milliseconds) */
#ifndef BTAPP_OBX_FRAMEWORK_PROGRESS_NOTIFICATION_INTERVAL
#define BTAPP_OBX_FRAMEWORK_PROGRESS_NOTIFICATION_INTERVAL  (1000)
#endif

/* progress statistics used by all all obex progress events */
typedef struct {
    UINT32  bytes_transferred;
    UINT32  tick_count_last_notification;   /* tick count of last progress notification to framework */
    UINT32  bytes_last_notification;        /* number of bytes on last notification for statistic
                                               purposes */
} tBTAPP_OBX_PROGRESS_STATS;

/* initialise progress stat values, pass POINTER to stat structure! */
#define INIT_OBX_PROG_STATS(p_cb) \
{\
    (p_cb)->bytes_transferred = 0;\
    (p_cb)->tick_count_last_notification = GKI_get_os_tick_count();\
    (p_cb)->bytes_last_notification = 0;\
}

/* returns time elapsed in ms handling wrap arround */
static __inline UINT32 btapp_obx_time_elapsed(tBTAPP_OBX_PROGRESS_STATS *p_stats, const UINT32 tick_count_current)
{
     if (tick_count_current<p_stats->tick_count_last_notification)
     {
         return GKI_TICKS_TO_MS(UINT_MAX-p_stats->tick_count_last_notification+tick_count_current);
     }
     return GKI_TICKS_TO_MS(tick_count_current - p_stats->tick_count_last_notification);
}

/* this updates the progress stats structure. Make sure that tick_count_current is bigger then
 * tick_count_last_notification (wrapp around handling) */
static __inline UINT32 btapp_update_obx_kb_per_s(tBTAPP_OBX_PROGRESS_STATS *p_stats, const UINT32 tick_count_current)
{
    UINT32 kb_per_s;    /* KB/s, 1000bytes = 1KB */

    kb_per_s = (p_stats->bytes_transferred-p_stats->bytes_last_notification)/btapp_obx_time_elapsed(p_stats,tick_count_current);

    /* update structure only after calculation of kb_per_s */
    p_stats->tick_count_last_notification = tick_count_current;
    p_stats->bytes_last_notification = p_stats->bytes_transferred;

    return kb_per_s;
}

#endif /* BTAPP_H */
