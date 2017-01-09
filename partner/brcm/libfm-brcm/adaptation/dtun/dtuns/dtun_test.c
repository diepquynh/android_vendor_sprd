/************************************************************************************
 *
 *  Copyright (C) 2009-2010 Broadcom Corporation
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
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>

#include "dtun_api.h"

/*******************************************************************************
**
** Function          DTUN DM SERVER TEST (BTLD SIDE)
**
*/

/* method callbacks */

static void dm_start_discovery(tDTUN_DEVICE_METHOD *msg)
{
    /* send start discovery request to btld */
    PRINTFUNC();

    // fake discovery process
    {
       tDTUN_DEVICE_SIGNAL sig;

       sig.discv_started.hdr.id  = DTUN_SIG_DM_DISCOVERY_STARTED;
       sig.discv_started.hdr.len = 0;
       dtun_server_send_signal(&sig);

       sleep(2);

       sig.device_found.hdr.id  = DTUN_SIG_DM_DEVICE_FOUND;
       sig.device_found.hdr.len = 6;
       memset(sig.device_found.bd, 1,6);
       dtun_server_send_signal(&sig);

       sleep(2);

       sig.discv_started.hdr.id  = DTUN_SIG_DM_DEVICE_FOUND;
       sig.device_found.hdr.len = 6;
       memset(sig.device_found.bd, 2,6);

       dtun_server_send_signal(&sig);


       sleep(2);

       sig.discv_started.hdr.id  = DTUN_SIG_DM_DEVICE_FOUND;
       sig.device_found.hdr.len = 6;
       memset(sig.device_found.bd, 3,6);

       dtun_server_send_signal(&sig);


       sleep(2);

       sig.discv_started.hdr.id  = DTUN_SIG_DM_DEVICE_FOUND;
       sig.device_found.hdr.len = 6;
       memset(sig.device_found.bd, 4,6);

       dtun_server_send_signal(&sig);

       sleep(3);

       sig.discv_started.hdr.id  = DTUN_SIG_DM_DISCOVERY_COMPLETE;
       sig.discv_started.hdr.len = 0;
       dtun_server_send_signal(&sig);

    }
}

static void dm_cancel_discovery(tDTUN_DEVICE_METHOD *msg)
{
    /* send cancel discovery request to btld */
    PRINTFUNC();

    /* fixme -- hookup to BTLD */

    // fake signal response
    {
       tDTUN_DEVICE_SIGNAL sig;
       sig.discv_started.hdr.id  = DTUN_SIG_DM_DISCOVERY_COMPLETE;
       sig.discv_started.hdr.len = 0;
       dtun_server_send_signal(&sig);
    }
}

/* method callbacks */
const tDTUN_METHOD method_tbl[] =
{
    /* dm */
    dm_start_discovery,            /* DTUN_METHOD_DM_START_DISCOVERY */
    dm_cancel_discovery,           /* DTUN_METHOD_DM_CANCEL_DISCOVERY */
};

void dtun_server_test(void)
{
    dtun_server_register_interface(BTA_INTERFACE_DM, (tDTUN_METHOD*)&method_tbl);
    dtun_server_start();
    sleep(120);
    dtun_server_stop();
}



/*******************************************************************************
**
** Function        TEST  MAIN
**
**
*******************************************************************************/

int main(int argc, char** argv)
{
    char *cmd, *arg;

    printf("starting dtun test app...\n");

       dtun_server_test();

    return 0;
}


