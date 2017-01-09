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
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include <ctype.h>

#include <sys/select.h>
#include <sys/poll.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include "btl_if.h"
#include "btl_ifs.h"
#include "btl_ifs_wrapper.h"

#ifndef LINUX_NATIVE
#include <cutils/properties.h>
#endif

tDTUN_METHOD *srv_tbl[DTUN_INTERFACE_MAX] = {0};
tCTRL_HANDLE dtun_ctrl[DTUN_INTERFACE_MAX] = {0};
static int dtun_initiated = 0;

#define PRINTFUNC() debug("\t\t%s()\n", __FUNCTION__);

/*******************************************************************************
**
** Function          misc functions
**
** Description
**
**
** Returns          void
**
*******************************************************************************/

/* NOTE : each method/signal area corresponds to one function table */
static const char *id_name[] =
{
    "DTUN_METHOD_BEGIN",
    /* DM methods */
    "DTUN_METHOD_DM_GET_LOCAL_INFO",
    "DTUN_METHOD_DM_START_DISCOVERY",
    "DTUN_METHOD_DM_CANCEL_DISCOVERY",
    "DTUN_METHOD_DM_GET_REMOTE_SERVICE_CHANNEL",
    "DTUN_METHOD_DM_GET_REMOTE_SERVICES",
    "DTUN_METHOD_DM_GET_ALL_REMOTE_SERVICES",
    "DTUN_METHOD_DM_CREATE_BONDING",
    "DTUN_METHOD_DM_REMOVE_BONDING",
    "DTUN_METHOD_DM_PIN_REPLY",
    "DTUN_METHOD_DM_PIN_NEG_REPLY",
    "DTUN_METHOD_DM_AUTHORIZE_RSP",
    "DTUN_METHOD_DM_SET_MODE",
    "DTUN_METHOD_DM_SET_NAME",
    "DTUN_METHOD_DM_ADD_DEV",
    "DTUN_METHOD_DM_SSP_CONFIRM_REPLY",
    "DTUN_METHOD_DM_DISC_RMT_DEV",
    "DTUN_METHOD_DM_ADD_SDP_REC",
    "DTUN_METHOD_DM_DEL_SDP_REC",
    "DTUN_METHOD_DM_SET_TESTMODE",
    "DTUN_METHOD_DM_SET_SECURITY",
    /* AV methods */
    "DTUN_METHOD_AM_AV_OPEN",
    "DTUN_METHOD_AM_AV_DISC",
    "DTUN_METHOD_AM_AV_STARTSTOP",
    /* OBEX methods */
    "DTUN_METHOD_OPC_ENABLE",
    "DTUN_METHOD_OPC_CLOSE",
    "DTUN_METHOD_OPS_CLOSE",
    "DTUN_METHOD_OPC_PUSH_OBJECT",
    "DTUN_METHOD_OPC_PULL_VCARD",
    "DTUN_METHOD_OPC_EXCH_VCARD",

    "DTUN_METHOD_OP_GRANT_ACCESS",
    "DTUN_METHOD_OP_SET_OWNER_VCARD",
    "DTUN_METHOD_OP_SET_EXCHANGE_FOLDER",
    "DTUN_METHOD_OP_CREATE_VCARD",
    "DTUN_METHOD_OP_STORE_VCARD",
    "DTUN_METHOD_END",

    "DTUN_SIG_BEGIN",
    /* DM signals */
    "DTUN_SIG_DM_LOCAL_INFO",
    "DTUN_SIG_DM_DISCOVERY_STARTED",
    "DTUN_SIG_DM_DISCOVERY_COMPLETE",
    "DTUN_SIG_DM_DEVICE_FOUND",
    "DTUN_SIG_DM_RMT_NAME",
    "DTUN_SIG_DM_RMT_SERVICE_CHANNEL",
    "DTUN_SIG_DM_RMT_SERVICES",
    "DTUN_SIG_DM_PIN_REQ",
    "DTUN_SIG_DM_AUTHORIZE_REQ",
    "DTUN_SIG_DM_AUTH_COMP",
    "DTUN_SIG_DM_LINK_DOWN",
    "DTUN_SIG_DM_SSP_CFM_REQ",
    "DTUN_SIG_DM_LINK_UP",
    "DTUN_SIG_DM_SDP_REC_HANDLE ",
    "DTUN_SIG_DM_TESTMODE_STATE",
    /* AV signals */
    "DTUN_SIG_AM_AV_EVENT",
    /* OBEX signals */
    "DTUN_SIG_OPC_ENABLE",
    "DTUN_SIG_OPC_OPEN",
    "DTUN_SIG_OPC_PROGRESS",
    "DTUN_SIG_OPC_OBJECT_RECEIVED",
    "DTUN_SIG_OPC_OBJECT_PUSHED",
    "DTUN_SIG_OPC_CLOSE",

    "DTUN_SIG_OPS_PROGRESS",
    "DTUN_SIG_OPS_OBJECT_RECEIVED",
    "DTUN_SIG_OPS_OPEN",
    "DTUN_SIG_OPS_ACCESS_REQUEST",
    "DTUN_SIG_OPS_CLOSE",
    "DTUN_SIG_OP_CREATE_VCARD",
    "DTUN_SIG_OP_OWNER_VCARD_NOT_SET",
    "DTUN_SIG_OP_STORE_VCARD",
    "DTUN_SIG_END",


    /* Common */
    "DTUN_DM_ERROR",

    "DTUN_ID_MAX"
};


static const char *id2name(tDTUN_ID id)
{
    if (id && id < DTUN_ID_MAX)
        return id_name[id];

    return "invalid id";
}

tDTUN_INTERFACE get_iface_by_id(eDTUN_ID id)
{
    if ((id>DTUN_METHOD_BEGIN) && (id<DTUN_SIG_END))
        return DTUN_INTERFACE;
    else
        error("[get_iface_by_id] error : invalid id %d\n", id);

    return -1;
}

int get_index_by_id(eDTUN_ID id)
{
    if ((id > DTUN_METHOD_BEGIN) && (id < DTUN_METHOD_END))
        return id - DTUN_METHOD_BEGIN - 1;
    else if ((id > DTUN_SIG_BEGIN) && (id < DTUN_SIG_END))
        return id - DTUN_SIG_BEGIN - 1;
    else
        error("[get_index_by_id] error : invalid id %d\n", id);

    return -1;
}


/*******************************************************************************
**
** Function         dtun_srv_send_signal
**
** Description     Sends DTUN signal
**
**
** Returns          void
**
*******************************************************************************/

void dtun_server_send_signal(tDTUN_DEVICE_SIGNAL *signal)
{
    int result;
    tDTUN_INTERFACE iface = get_iface_by_id(signal->hdr.id);
    BTL_IF_CtrlSend(dtun_ctrl[iface], SUB_CTRL, BTLIF_DTUN_SIGNAL_EVT, (tBTL_PARAMS*)signal, signal->hdr.len+sizeof(tDTUN_HDR));
}

/*******************************************************************************
**
** Function         dtun_srv_send_signal_id
**
** Description     Sends signal without any arguments
**
**
** Returns          void
**
*******************************************************************************/

void dtun_server_send_signal_id(tDTUN_ID id)
{
    tDTUN_DEVICE_SIGNAL msg;
    msg.hdr.id = id;
    msg.hdr.len = 0;
    dtun_server_send_signal(&msg);
}

/*******************************************************************************
**
** Function          dtun_server_register_interface
**
** Description
**
**
** Returns          void
**
*******************************************************************************/

void dtun_server_register_interface(tDTUN_INTERFACE iface, tDTUN_METHOD *tbl)
{
    info("Register DTUN interface [%d]\n", iface);

    if (srv_tbl[iface] == NULL)
       srv_tbl[iface] = tbl;
    else
       error("WARNING : dtun interface busy\n");
}
/*******************************************************************************
**
** Function          dtun_server_start
**
** Description
**
**
** Returns          void
**
*******************************************************************************/

static boolean handle_method_call(tDTUN_DEVICE_METHOD *method)
{
    tDTUN_INTERFACE iface = get_iface_by_id(method->hdr.id);
    int index = get_index_by_id(method->hdr.id);

    info("handle_method_call :: received %s (id %d), len %d\n", id2name(method->hdr.id),
            method->hdr.id, method->hdr.len);

    if ((index<0) || (iface<0))
    {
        error("WARNING : no index or interface found\n");
        return false;
    }

    /* check sanity */
    if (srv_tbl[iface][index])
        (*srv_tbl[iface][index])(method);
    else
    {
        error("WARNING : no dtun method handler defined for id %s (index %d)\n", id2name(method->hdr.id), index);
    }

    return true;
}

void DTUN_ReceiveCtrlMsg(tCTRL_HANDLE handle, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params)
{
    info("[DTUN] Received message [%s] %d\n", dump_msg_id(id), id);

    switch(id)
    {
        case DTUN_METHOD_CALL:
                handle_method_call(&params->dtun_method);
            break;

        case BTLIF_SUBSYSTEM_DETACHED:
            break;

        default:
            error("Message not handled (%d)\n", id);
            break;
    }
}

void dtun_server_start(void)
{
    tBTL_IF_Result result;

    PRINTFUNC();

    if (dtun_initiated)
    {
        info("dtun server already initiated\n");
        return;
    }

    dtun_initiated = 1;

    /* Initialize datapath server */
    BTL_IF_ServerInit();

    /* Register all DTUN subsystems (use same ctrl callback for all) */
    /* Having separate subsystems for each interfaces enables using multiple separate client processes */
    BTL_IF_RegisterSubSystem(&dtun_ctrl[DTUN_INTERFACE], SUB_DTUN, NULL, DTUN_ReceiveCtrlMsg);

    /* Set dtun server property flag */
    property_set(DTUN_PROPERTY_SERVER_ACTIVE, "1");
}

/*******************************************************************************
**
** Function          dtun_server_stop
**
** Description
**
**
** Returns          void
**
*******************************************************************************/

void dtun_server_stop(void)
{
    PRINTFUNC();

    BTL_IF_UnregisterSubSystem(SUB_DTUN);
    /* Set dtun server property flag */
    /* moved to bte_main.c to avoid btld being restarted before all threads are down. property_set(DTUN_PROPERTY_SERVER_ACTIVE, "0"); */

    dtun_initiated = 0;
}


