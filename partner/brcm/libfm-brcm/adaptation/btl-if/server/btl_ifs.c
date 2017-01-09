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
#ifndef BTLIF_STANDALONE
#include "bt_target.h"
#include "data_types.h"
#endif

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

#include "btl_ifs.h"
#include "btl_ifs_wrapper.h"
#include "dtun.h"

/*******************************************************************************
**
** Typedefs
**
**
*******************************************************************************/

#if (defined LOG_TAG)
#undef LOG_TAG
#endif

#define LOG_TAG "BTL-IFS"

#define STREAM_TO_UINT8(u8, p)   {u8 = (UINT8)(*(p)); (p) += 1;}
#define STREAM_TO_UINT16(u16, p) {u16 = ((UINT16)(*(p)) + (((UINT16)(*((p) + 1))) << 8)); (p) += 2;}
#define STREAM_TO_UINT32(u32, p) {u32 = (((UINT32)(*(p))) + ((((UINT32)(*((p) + 1)))) << 8) + ((((UINT32)(*((p) + 2)))) << 16) + ((((UINT32)(*((p) + 3)))) << 24)); (p) += 4;}
#define MAX_CLIENTS BTL_IF_SUBSYSTEM_MAX

#define CHECK_SUBSYSTEM(sub)     if (sub >= BTL_IF_SUBSYSTEM_MAX) { error("%s :: invalid subsystem [%d]\n", __FUNCTION__, sub); return FALSE;}

#define BTLIF_SERVER_SHUTDOWN_TMO 50

/* fixme -- move all variable into a control block */

tBTL_IF_CB sub_cb[MAX_CLIENTS];

/* receive buffer */
//char ctrl_rx_buf[BTLIF_CTRL_RX_BUF_MAX]; // fixme -- provide from API caller, for now use static

/* runtime state of data server */
typedef enum {
    STATE_STOPPED,
    STATE_STARTING,
    STATE_RUNNING,
    STATE_SHUTTING_DOWN
} t_state;

static int signal_fds[2];

t_state volatile state = STATE_STOPPED;
pthread_t server_thread_id;
fd_set read_set;
fd_set active_set;
int max_fd;

/* Forward declarations */
void close_all_open_sockets(void);

/*******************************************************************************
**
** Local variables
**
**
*******************************************************************************/

const char *sub2str[] =
{
    "NONE",         //SUB_NONE
    "CTRL",         //SUB_CTRL
    "DTUN",         //SUB_DTUN
    "AG",           //SUB_AG
    "AV",           //SUB_AV
    "FM",           //SUB_FM
    "SCO",          //SUB_SCO
    "PR",           //SUB_PR
    "PBS",          //SUB_PBS
    "FTPS",         //SUB_FTPS
    "DUN",          //SUB_DUN
    "HH",           //SUB_HH
    "SPP",          //SUB_SPP
    "TEST",         //SUB_TEST
    "SAPS",         //SUB_SAPS
    "BTS",          //SUB_BTS
    "HCIUTILS",     //SUB_HCIUTILS
    "",             //SUB_MAX
};

/*******************************************************************************
**
** Socket signalling functions
**
**
*******************************************************************************/

static inline int btl_ifs_select_wakeup_init(fd_set* set)
{
    if(signal_fds[0] == 0 && socketpair(AF_UNIX, SOCK_STREAM, 0, signal_fds) < 0)
    {
        error("create_signal_sockets:socketpair failed: %s", strerror(errno));
        return -1;
    }
    FD_SET(signal_fds[0], set);
    return signal_fds[0];
}

static inline int btl_ifs_select_wakeup(void)
{
    char sig_on = 1;
    return send(signal_fds[1], &sig_on, sizeof(sig_on), 0);
}

static inline int btl_ifs_select_wake_reset(void)
{
    char sig_recv = 0;
    recv(signal_fds[0], &sig_recv, sizeof(sig_recv), MSG_WAITALL);
    return (int)sig_recv;
}

static inline int btl_ifs_select_wake_signaled(fd_set* set)
{
    return FD_ISSET(signal_fds[0], set);
}

/*******************************************************************************
**
** Debug
**
**
*******************************************************************************/


const char* dump_msg_id(tBTLIF_CTRL_MSG_ID id)
{
    char id_str[64];

    switch(id)
    {
        /* COMMON */
        case BTLIF_REGISTER_SUBSYS_REQ: return "BTLIF_REGISTER_SUBSYS_REQ";
        case BTLIF_REGISTER_SUBSYS_RSP: return "BTLIF_REGISTER_SUBSYS_RSP";
        case BTLIF_UNREGISTER_SUBSYS_REQ: return "BTLIF_UNREGISTER_SUBSYS_REQ";
        case BTLIF_UNREGISTER_SUBSYS_RSP: return "BTLIF_UNREGISTER_SUBSYS_RSP";
        case BTLIF_CONNECT_REQ: return "BTLIF_CONNECT_REQ";
        case BTLIF_CONNECT_RSP: return "BTLIF_CONNECT_RSP";
        case BTLIF_CONNECT_IND: return "BTLIF_CONNECT_IND";
        case BTLIF_DISCONNECT_REQ: return "BTLIF_DISCONNECT_REQ";
        case BTLIF_DISCONNECT_RSP: return "BTLIF_DISCONNECT_RSP";
        case BTLIF_DISCONNECT_IND: return "BTLIF_DISCONNECT_IND";
        case BTLIF_LISTEN_REQ: return "BTLIF_LISTEN_REQ";
        case BTLIF_SET_CONFIG: return "BTLIF_SET_CONFIG";
        case BTLIF_GET_CONFIG: return "BTLIF_GET_CONFIG";
        case BTLIF_DATA_CHAN_IND : return "BTLIF_DATA_CHAN_IND";
        case BTLIF_DATA_CHAN_DISC_IND : return "BTLIF_DATA_CH_DISC_IND";
        case BTLIF_SUBSYSTEM_ATTACHED : return "BTLIF_SUBSYSTEM_ATTACHED";
        case BTLIF_SUBSYSTEM_DETACHED : return "BTLIF_SUBSYSTEM_DETACHED";
        case BTLIF_CONNECT_IND_ACK : return "BTLIF_CONNECT_IND_ACK";
        case BTLIF_DISC_RX_BUF_PENDING : return "BTLIF_DISC_RX_BUF_PENDING";

        /* FM */
        case BTLIF_FM_ENABLE: return "BTLIF_FM_ENABLE";
        case BTLIF_FM_DISABLE: return "BTLIF_FM_DISABLE";
        case BTLIF_FM_TUNE: return "BTLIF_FM_TUNE";
        case BTLIF_FM_MUTE: return "BTLIF_FM_MUTE";
        case BTLIF_FM_SEARCH: return "BTLIF_FM_SEARCH";
        case BTLIF_FM_COMBO_SEARCH: return "BTLIF_FM_COMBO_SEARCH";
        case BTLIF_FM_SEARCH_ABORT: return "BTLIF_FM_SEARCH_ABORT";
        case BTLIF_FM_SET_RDS_MODE: return "BTLIF_FM_SET_RDS_MODE";
        case BTLIF_FM_SET_RDS_RBDS: return "BTLIF_FM_SET_RDS_RBDS";
        case BTLIF_FM_RDS_UPDATE: return "BTLIF_FM_RDS_UPDATE";
        case BTLIF_FM_AUDIO_MODE: return "BTLIF_FM_AUDIO_MODE";
        case BTLIF_FM_AUDIO_PATH: return "BTLIF_FM_AUDIO_PATH";
        case BTLIF_FM_SCAN_STEP: return "BTLIF_FM_SCAN_STEP";
        case BTLIF_FM_SET_REGION: return "BTLIF_FM_SET_REGION";
        case BTLIF_FM_CONFIGURE_DEEMPHASIS: return "BTLIF_FM_CONFIGURE_DEEMPHASIS";
        case BTLIF_FM_ESTIMATE_NFL: return "BTLIF_FM_ESTIMATE_NFL";
        case BTLIF_FM_GET_AUDIO_QUALITY: return "BTLIF_FM_GET_AUDIO_QUALITY";
        case BTLIF_FM_CONFIGURE_SIGNAL_NOTIFICATION: return "BTLIF_FM_CONFIGURE_SIGNAL_NOTIFICATION";
        case BTLIF_FM_AF_JMP_EVT: return "BTLIF_FM_AF_JMP_EVT";
        case BTLIF_FM_SET_VOLUME: return "BTLIF_FM_SET_VOLUME";
        case BTLIF_FM_SET_VOLUME_EVT: return "BTLIF_FM_SET_VOLUME_EVT";

        /* DTUN */
        case BTLIF_DTUN_METHOD_CALL: return "BTLIF_DTUN_METHOD_CALL";
        case BTLIF_DTUN_SIGNAL_EVT: return "BTLIF_DTUN_SIGNAL_EVT";

        /* PBAP */
        case BTLIF_PBS_ENABLE:  return "BTLIF_PBS_ENABLE";
        case BTLIF_PBS_DISABLE: return "BTLIF_PBS_DISABLE";
        case BTLIF_PBS_AUTH_RSP: return "BTLIF_PBS_AUTH_RSP";
        case BTLIF_PBS_ACCESS_RSP: return "BTLIF_PBS_ACCESS_RSP";

        case BTLIF_PBS_ENABLE_EVT: return "BTLIF_PBS_ENABLE_EVT";
        case BTLIF_PBS_OPEN_EVT: return "BTLIF_PBS_OPEN_EVT";
        case BTLIF_PBS_CLOSE_EVT: return "BTLIF_PBS_CLOSE_EVT";
        case BTLIF_PBS_AUTH_EVT: return "BTLIF_PBS_AUTH_EVT";
        case BTLIF_PBS_ACCESS_EVT: return "BTLIF_PBS_ACCESS_EVT";
        case BTLIF_PBS_OPER_CMPL_EVT: return "BTLIF_PBS_OPER_CMPL_EVT";

        /* FTP Server */
        case BTLIF_FTPS_ENABLE: return "BTLIF_FTPS_ENABLE";
        case BTLIF_FTPS_DISABLE: return "BTLIF_FTPS_DISABLE";
        case BTLIF_FTPS_CLOSE: return "BTLIF_FTPS_CLOSE";
        case BTLIF_FTPS_AUTH_RSP: return "BTLIF_FTPS_AUTH_RSP";
        case BTLIF_FTPS_ACCESS_RSP: return "BTLIF_FTPS_ACCESS_RSP";
        case BTLIF_FTPS_ENABLE_EVT: return "BTLIF_FTPS_ENABLE_EVT";
        case BTLIF_FTPS_DISABLE_EVT: return "BTLIF_FTPS_DISABLE_EVT";
        case BTLIF_FTPS_OPEN_EVT: return "BTLIF_FTPS_OPEN_EVT";
        case BTLIF_FTPS_CLOSE_EVT: return "BTLIF_FTPS_CLOSE_EVT";
        case BTLIF_FTPS_AUTH_EVT: return "BTLIF_FTPS_AUTH_EVT";
        case BTLIF_FTPS_ACCESS_EVT: return "BTLIF_FTPS_ACCESS_EVT";
        case BTLIF_FTPS_FILE_TRANSFER_IN_PRGS_CMPL_EVT: return "BTLIF_FTPS_FILE_TRANSFER_IN_PRGS_CMPL_EVT";
        case BTLIF_FTPS_OPER_PUT_CMPL_EVT: return "BTLIF_FTPS_OPER_PUT_CMPL_EVT";
        case BTLIF_FTPS_OPER_GET_CMPL_EVT: return "BTLIF_FTPS_OPER_GET_CMPL_EVT";
        case BTLIF_FTPS_OPER_DEL_CMPL_EVT: return "BTLIF_FTPS_OPER_DEL_CMPL_EVT";

        /* DG Server */
        case BTLIF_DG_ENABLE: return "BTLIF_DG_ENABLE";
        case BTLIF_DG_DISABLE: return "BTLIF_DG_DISABLE";
        case BTLIF_DG_LISTEN: return "BTLIF_DG_LISTEN";
        case BTLIF_DG_OPEN: return "BTLIF_DG_OPEN";
        case BTLIF_DG_CLOSE: return "BTLIF_DG_CLOSE";
        case BTLIF_DG_SHUTDOWN: return "BTLIF_DG_SHUTDOWN";
        case BTLIF_DG_SPP_CREATE: return "BTLIF_DG_SPP_CREATE";
        case BTLIF_DG_SPP_DESTORY: return "BTLIF_DG_SPP_DESTORY";
        case BTLIF_DG_CONNECT_DATA_PATH: return "BTLIF_DG_CONNECT_DATA_PATH";
        case BTLIF_DG_ENABLE_EVT: return "BTLIF_DG_ENABLE_EVT";
        case BTLIF_DG_DISABLE_EVT: return "BTLIF_DG_DISABLE_EVT";
        case BTLIF_DG_LISTEN_EVT: return "BTLIF_DG_LISTEN_EVT";
        case BTLIF_DG_OPENING_EVT: return "BTLIF_DG_OPENING_EVT";
        case BTLIF_DG_OPEN_EVT: return "BTLIF_DG_OPEN_EVT";
        case BTLIF_DG_CLOSE_EVT: return "BTLIF_DG_CLOSE_EVT";
        case BTLIF_DG_SHUTDOWN_EVT: return "BTLIF_DG_SHUTDOWN_EVT";
        case BTLIF_DG_RIL_FLOW_CTRL_REQ: return "BTLIF_DG_RIL_FLOW_CTRL_REQ";
        case BTLIF_DG_RIL_FLOW_CTRL_EVT: return "BTLIF_DG_RIL_FLOW_CTRL_EVT";


        /* HH Server */
        case BTLIF_HH_OPEN: return "BTLIF_HH_OPEN";
        case BTLIF_HH_CLOSE: return "BTLIF_HH_CLOSE";
        case BTLIF_HH_OPEN_EVT: return "BTLIF_HH_OPEN_EVT";
        case BTLIF_HH_CLOSE_EVT: return "BTLIF_HH_CLOSE_EVT";

        /* testmode */
        case BTLIF_TST_SET_TESTMODE: return "BTLIF_TST_SET_TESTMODE";
        case BTLIF_TST_GET_TST_STATE: return "BTLIF_TST_GET_TST_STATE";
        case BTLIF_TST_TX_RX_TEST: return "BTLIF_TST_TX_RX_TEST";
        case BTLIF_TST_SEND_TST_CMD: return "BTLIF_TST_SEND_TST_CMD";

        /* BTS */
        case BTLIF_BTS_RFC_BIND_REQ: return "BTLIF_BTS_RFC_BIND_REQ";
        case BTLIF_BTS_RFC_BIND_RSP: return "BTLIF_BTS_RFC_BIND_RSP";
        case BTLIF_BTS_RFC_LISTEN_REQ: return "BTLIF_BTS_RFC_LISTEN_REQ";
        case BTLIF_BTS_RFC_LISTEN_RSP: return "BTLIF_BTS_RFC_LISTEN_RSP";
        case BTLIF_BTS_RFC_LISTEN_CANCEL: return "BTLIF_BTS_RFC_LISTEN_CANCEL";
        case BTLIF_BTS_RFC_CON_IND: return "BTLIF_BTS_RFC_CON_IND";
        case BTLIF_BTS_RFC_CON_IND_ACK: return "BTLIF_BTS_RFC_CON_IND_ACK";
        case BTLIF_BTS_RFC_CON_REQ: return "BTLIF_BTS_RFC_CON_REQ";
        case BTLIF_BTS_RFC_CON_RSP: return "BTLIF_BTS_RFC_CON_RSP";
        case BTLIF_BTS_RFC_CLOSE: return "BTLIF_BTS_RFC_CLOSE";
        case BTLIF_BTS_RFC_CLOSE_CFM: return "BTLIF_BTS_RFC_CLOSE_CFM";
        case BTLIF_BTS_RFC_DISC_IND: return "BTLIF_BTS_RFC_DISC_IND";
        case BTLIF_BTS_RFC_DISC_IND_ACK: return "BTLIF_BTS_RFC_DISC_IND_ACK";
        case BTLIF_BTS_RFC_LST_NOTIFY_DSOCK: return "BTLIF_BTS_RFC_LST_NOTIFY_DSOCK";
        case BTLIF_BTS_L2C_LISTEN_REQ: return "BTLIF_BTS_L2C_LISTEN_REQ";
        case BTLIF_BTS_L2C_LISTEN_RSP: return "BTLIF_BTS_L2C_LISTEN_RSP";
        case BTLIF_BTS_L2C_CONNECT_REQ: return "BTLIF_BTS_L2C_CONNECT_REQ";
        case BTLIF_BTS_L2C_CONNECT_RSP: return "BTLIF_BTS_L2C_CONNECT_RSP";
        case BTLIF_BTS_L2C_DISC_REQ: return "BTLIF_BTS_L2C_DISC_REQ";
        case BTLIF_BTS_L2C_DISC_RSP: return "BTLIF_BTS_L2C_DISC_RSP";
        case BTLIF_BTS_L2C_DISC_IND: return "BTLIF_BTS_L2C_DISC_IND";
        case BTLIF_BTS_SCO_CON_REQ: return "BTLIF_BTS_SCO_CON_REQ";
        case BTLIF_BTS_SCO_CON_RSP: return "BTLIF_BTS_SCO_CON_RSP";
        case BTLIF_BTS_SCO_DISC_REQ: return "BTLIF_BTS_SCO_DISC_REQ";
        case BTLIF_BTS_SCO_DISC_RSP: return "BTLIF_BTS_SCO_DISC_RSP";
        case BTLIF_BTS_SCO_DISC_IND: return "BTLIF_BTS_SCO_DISC_IND";
        case BTLIF_BTS_EVT_ABORT: return "BTLIF_BTS_EVT_ABORT";

        /* HCIUTILS */
        case BTLIF_HCIUTILS_NOTIFY_EVT:    return "BTLIF_HCIUTILS_NOTIFY_EVT";


        /* Printer */
        case BTLIF_PR_ENABLE:            return "BTLIF_PR_ENABLE";
        case BTLIF_PR_DISABLE:           return "BTLIF_PR_DISABLE";
        case BTLIF_PR_GET_CAPS:          return "BTLIF_PR_GET_CAPS";
        case BTLIF_PR_AUTH_RSP:          return "BTLIF_PR_AUTH_RSP";
        case BTLIF_PR_PRINT:             return "BTLIF_PR_PRINT";
        case BTLIF_PR_PARTIAL_IMAGE_RSP: return "BTLIF_PR_PARTIAL_IMAGE_RSP";
        case BTLIF_PR_REF_OBJ_RSP:       return "BTLIF_PR_REF_OBJ_RSP";
        case BTLIF_PR_ABORT:             return "BTLIF_PR_ABORT";
        case BTLIF_PR_CANCEL_BP_STATUS:  return "BTLIF_PR_CANCEL_BP_STATUS";
        case BTLIF_PR_CARD_TO_XHTML:     return "BTLIF_PR_CARD_TO_XHTML";
        case BTLIF_PR_CAL_TO_XHTML:      return "BTLIF_PR_CAL_TO_XHTML";
        case BTLIF_PR_ENABLE_EVT:        return "BTLIF_PR_ENABLE_EVT";
        case BTLIF_PR_OPEN_EVT:          return "BTLIF_PR_OPEN_EVT";
        case BTLIF_PR_PROGRESS_EVT:      return "BTLIF_PR_PROGRESS_EVT";
        case BTLIF_PR_CLOSE_EVT:         return "BTLIF_PR_CLOSE_EVT";
        case BTLIF_PR_GETCAPS_EVT:       return "BTLIF_PR_GETCAPS_EVT";
        case BTLIF_PR_AUTH_EVT:          return "BTLIF_PR_AUTH_EVT";
        case BTLIF_PR_THUMBNAIL_EVT:     return "BTLIF_PR_THUMBNAIL_EVT";
        case BTLIF_PR_PARTIAL_IMAGE_EVT: return "BTLIF_PR_PARTIAL_IMAGE_EVT";
        case BTLIF_PR_GET_OBJ_EVT:       return "BTLIF_PR_GET_OBJ_EVT";
        case BTLIF_PR_BP_DOC_CMPL_EVT:   return "BTLIF_PR_BP_DOC_CMPL_EVT";
        case BTLIF_PR_JOB_STATUS_EVT:    return "BTLIF_PR_JOB_STATUS_EVT";
        case BTLIF_PR_NOTE_TO_XHTML:     return "BTLIF_PR_NOTE_TO_XHTML";

        /* SAP Server */
        case BTLIF_SAPS_ENABLE:            return "BTLIF_SAPS_ENABLE";
        case BTLIF_SAPS_DISABLE:           return "BTLIF_SAPS_DISABLE";
        case BTLIF_SAPS_DISCONNECT:        return "BTLIF_SAPS_DISCONNECT";
        case BTLIF_SAPS_STATUS:            return "BTLIF_SAPS_STATUS";
        case BTLIF_SAPS_CARD_STATUS:       return "BTLIF_SAPS_CARD_STATUS";
        case BTLIF_SAPS_READER_STATUS:     return "BTLIF_SAPS_READER_STATUS";
        case BTLIF_SAPS_ENABLE_EVT:        return "BTLIF_SAPS_ENABLE_EVT";
        case BTLIF_SAPS_DISABLE_EVT:       return "BTLIF_SAPS_DISABLE_EVT";
        case BTLIF_SAPS_OPEN_EVT:          return "BTLIF_SAPS_OPEN_EVT";
        case BTLIF_SAPS_CLOSE_EVT:         return "BTLIF_SAPS_CLOSE_EVT";

        case BTLIF_SAPS_RIL_SIM_ON:        return "BTLIF_SAPS_RIL_SIM_ON";
        case BTLIF_SAPS_RIL_SIM_OFF:       return "BTLIF_SAPS_RIL_SIM_OFF";
        case BTLIF_SAPS_RIL_SIM_RESET:     return "BTLIF_SAPS_RIL_SIM_RESET";
        case BTLIF_SAPS_RIL_SIM_ATR:       return "BTLIF_SAPS_RIL_SIM_ATR";
        case BTLIF_SAPS_RIL_SIM_APDU:      return "BTLIF_SAPS_RIL_SIM_APDU";
        case BTLIF_SAPS_RIL_SIM_ON_EVT:    return "BTLIF_SAPS_RIL_SIM_ON_EVT";
        case BTLIF_SAPS_RIL_SIM_OFF_EVT:   return "BTLIF_SAPS_RIL_SIM_OFF_EVT";
        case BTLIF_SAPS_RIL_SIM_RESET_EVT: return "BTLIF_SAPS_RIL_SIM_RESET_EVT";
        case BTLIF_SAPS_RIL_SIM_ATR_EVT:   return "BTLIF_SAPS_RIL_SIM_ATR_EVT";
        case BTLIF_SAPS_RIL_SIM_APDU_EVT:  return "BTLIF_SAPS_RIL_SIM_APDU_EVT";


        default :
            sprintf(id_str, "Unknown message id %d", id);
        return id_str;
    }
}


/*******************************************************************************
**
** Select helper functions
**
**
*******************************************************************************/

static void ctrl_remove_active_set(int fd)
{
    verbose("ctrl_remove_active_set %d", fd);
    FD_CLR(fd, &active_set);

}
static void ctrl_add_active_set(int fd)
{
    verbose("ctrl_add_active_set %d", fd);
    FD_SET(fd, &active_set);
    max_fd = MAX(max_fd,fd);
}


/*******************************************************************************
**
** Server registration
**
**
*******************************************************************************/

static BOOLEAN register_subsystem(tBTL_IF_SUBSYSTEM sub, tBTL_IF_DATA_CALLBACK data_cb, tBTL_IF_CTRL_CALLBACK ctrl_cb)
{
    if (sub_cb[sub].sub == SUB_NONE)
    {
        sub_cb[sub].sub = sub;
        sub_cb[sub].data_cb = data_cb;
        sub_cb[sub].ctrl_cb = ctrl_cb;
        sub_cb[sub].ctrl_fd = DATA_SOCKET_INVALID;
        sub_cb[sub].mclnt_enabled = 0;
        return TRUE;
    }
    else
        return FALSE;
}

/* temp name until mclient support verified */
static BOOLEAN register_subsystem_mclient(tBTL_IF_SUBSYSTEM sub, tBTL_IF_DATA_CALLBACK data_cb, tBTL_IF_CTRL_CALLBACK ctrl_cb)
{
    int i;
    if (sub_cb[sub].sub == SUB_NONE)
    {
        sub_cb[sub].sub = sub;
        sub_cb[sub].data_cb = data_cb;
        sub_cb[sub].ctrl_cb = ctrl_cb;
        sub_cb[sub].ctrl_fd = DATA_SOCKET_INVALID;

        /* mclient support */
        sub_cb[sub].mclnt_enabled = 1;
        for (i=0; i<SUBSYS_MAX_CLIENTS; i++)
            sub_cb[sub].mctrl_fd[i] = DATA_SOCKET_INVALID;

        return TRUE;
    }
    else
        return FALSE;
}

static BOOLEAN unregister_subsystem(tBTL_IF_SUBSYSTEM sub)
{
    if (sub_cb[sub].data_cb || sub_cb[sub].ctrl_cb)
    {
        sub_cb[sub].data_cb = NULL;
        sub_cb[sub].ctrl_cb = NULL;
        sub_cb[sub].sub = SUB_NONE;
        sub_cb[sub].ctrl_fd = DATA_SOCKET_INVALID;
        return TRUE;
    }
    else
        return FALSE;
}

static BOOLEAN subsystem_registered(tBTL_IF_SUBSYSTEM sub)
{
    if (sub_cb[sub].sub != SUB_NONE)
        return TRUE;
    else
        return FALSE;
}

static BOOLEAN is_client_attached(tBTL_IF_SUBSYSTEM sub)
{
    if (sub_cb[sub].sub != SUB_NONE)
        return TRUE;
    else
        return TRUE;
}

static int mclient_get_free_handle(tBTL_IF_SUBSYSTEM sub)
{
    int i;
    for (i=0; i<SUBSYS_MAX_CLIENTS; i++)
    {
        if (sub_cb[sub].mctrl_fd[i] == DATA_SOCKET_INVALID)
            return i;
    }
    error("no available handles");
    return -1;
}

static int mclient_find_handle(tBTL_IF_SUBSYSTEM sub, int ctrl_fd)
{
    int i;
    for (i=0; i<SUBSYS_MAX_CLIENTS; i++)
    {
        if (sub_cb[sub].mctrl_fd[i] == ctrl_fd)
            return i;
    }
    error("find failed (%d)", ctrl_fd);
    return -1;
}


static BOOLEAN attach_client(int fd, tBTL_IF_SUBSYSTEM sub)
{
    tBTL_PARAMS params;

    if (sub_cb[sub].mclnt_enabled)
    {
        /* multi client support, find available entry */
        int index;

        index = mclient_get_free_handle(sub);

        info("multiclient index %d", index);

        if (index >= 0)
            sub_cb[sub].mctrl_fd[index] = fd;
        else
            return FALSE; // all entries busy
    }
    else
    {
        /* single client, check if busy */
        if (sub_cb[sub].ctrl_fd != DATA_SOCKET_INVALID)
        {
            /* already registered */
            return FALSE;
        }
        sub_cb[sub].ctrl_fd = fd;
    }

    info("######## Attached client subsystem %s (%d) ######## ", sub2str[sub], fd);

    if (sub_cb[sub].mclnt_enabled)
    {
        params.attached.handle = fd;

        /* notify user that the ctrl channel for this subsystem is down */
        btl_if_notify_local_event(fd, sub, BTLIF_SUBSYSTEM_ATTACHED, &params);
    }

    return TRUE;
}

static BOOLEAN detach_client(tBTL_IF_SUBSYSTEM sub, int ctrl_fd)
{
    tBTL_PARAMS params;

    info("########  Detached client subsystem (%s) fd %d ######## ", sub2str[sub], ctrl_fd);

    if (sub_cb[sub].mclnt_enabled)
    {
        int index;

        /* multi client support */
        index = mclient_find_handle(sub, ctrl_fd);

        if (index < 0)
        {
            error("invalid ctrl fd %d (sub %d)", ctrl_fd, sub);
            return FALSE;
        }

        /* save handle that detached */
        params.detached.handle = ctrl_fd;

        ctrl_remove_active_set(ctrl_fd);
        close(ctrl_fd);

        sub_cb[sub].mctrl_fd[index] = DATA_SOCKET_INVALID;
    }
    else
    {
        /* single client */
        ASSERTC(sub_cb[sub].ctrl_fd == ctrl_fd, "invalid ctrl fd", ctrl_fd);

        /* save handle that detached */
        params.detached.handle = ctrl_fd;

        ctrl_remove_active_set(ctrl_fd);
        close(ctrl_fd);

        sub_cb[sub].ctrl_fd = DATA_SOCKET_INVALID;
    }

    /* also make sure we close any remaining listeners for this subsystem */
    ws_del_sub_listeners(sub);

    /* notify user that the ctrl channel for this subsystem is down */
    btl_if_notify_local_event(ctrl_fd, sub, BTLIF_SUBSYSTEM_DETACHED, &params);

    return TRUE;
}

/*******************************************************************************
**
** Control channel
**
**
*******************************************************************************/

/* fixme -- need to exit context and setup event notification through main select */

void btl_if_notify_local_event(int fdc, tSUB sub, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params)
{
    info("Notify local event %s", dump_msg_id(id));

    btl_ifs_select_wakeup();

    /* notify user of new data connection */
    if (sub_cb[sub].ctrl_cb)
    {
        /* for single client we use sub system as handle, for multi clnt we use actual handle */
        /* once mclnt is verified single clnt will be obsoleted */
        if (sub_cb[sub].mclnt_enabled)
            sub_cb[sub].ctrl_cb(fdc, id, params);
        else
            sub_cb[sub].ctrl_cb(sub, id, params);
    }

    /* to make sure we don't get a data callback before user has processed
       the data chan ind we add to active set after the notification */
    if ((id == BTLIF_DATA_CHAN_IND) && sub_cb[sub].data_cb)
    {
        verbose("Datacallback registered, add to active list");
        wrp_add_to_active_set(params->chan_ind.handle);
    }
}

void btl_if_ctrl_rx(int fd, tSUB  sub, char *p)
{
    tBTLIF_CTRL_MSG_PKT *pkt;

    pkt = (tBTLIF_CTRL_MSG_PKT *)p;

    //info("[BTL_IFS CTRL] recv %s (%s) %d pbytes (hdl %d)", dump_msg_id(pkt->hdr.id), sub2str[sub], pkt->hdr.len, fd);

    if (sub_cb[sub].ctrl_cb)
    {
        /* for single client we use sub system as handle, for multi clnt we use actual handle */
        /* once mclnt is verified single clnt will be obsoleted */
        if (sub_cb[sub].mclnt_enabled)
            sub_cb[sub].ctrl_cb(fd, pkt->hdr.id, &pkt->params);
        else
            sub_cb[sub].ctrl_cb(sub, pkt->hdr.id, &pkt->params);
    }
}


int btl_if_ctrl_tx(int s, tBTLIF_CTRL_MSG_PKT *pkt, int len)
{
    return tx_data(s, (char *)pkt, len, 0); /* never non_sock */
}

static int send_ctrl_msg(int fd, tSUB sub, tBTLIF_CTRL_MSG_ID type, tBTL_PARAMS *params, int param_len)
{
    tBTLIF_CTRL_MSG_PKT pkt;

    pkt.hdr.len = sizeof(tSUB) + param_len;
    pkt.hdr.id = type;
    pkt.sub = sub;

    info("[BTL_IFS CTRL] send %s (%s) %d pbytes (hdl %d)", dump_msg_id(type), sub2str[sub], param_len, fd);

    if (param_len)
        memcpy(&pkt.params, params, param_len);

    return btl_if_ctrl_tx(fd, &pkt, sizeof(tBTL_IF_HDR) + sizeof(tSUB) + param_len);
}

/*******************************************************************************
**
** Socket connections
**
**
*******************************************************************************/

int btl_if_ctrl_listen(void)
{
    int result;
    t_wsock *ws;

    verbose("Starting main ctrl listener");

    /* handle is don't care for SUB_CTRL */
    ws = wrp_sock_create(0, SUB_CTRL, BTLIF_IP_ADDR, BTLIF_PORT_BASE_CTRL, 1);

    result = wrp_sock_bind(ws);

    if (result < 0)
        goto failed;

    /* return fd if successful */
    result = wrp_sock_listen(ws);

    failed:
    if (result < 0)
    {
        /* free wrapper socket */
        wrp_wsock_free(ws);
        return result;
    }

    /* added to active set in main loop */

    return result;
}

void btl_if_rxdata(tBTL_IF_SUBSYSTEM sub, int fd, char *p, int len)
{
    verbose("btl_if_data_rx : %d bytes on %d", len, fd);

    if (sub_cb[sub].data_cb)
        sub_cb[sub].data_cb(fd, p, len);
}

void send_registration_rsp(int fd, tBTL_IF_Result result)
{
    tBTL_PARAMS rsp;

    debug("send_registration_rsp [%d]", result);

    rsp.result.result = result;
    send_ctrl_msg(fd, SUB_CTRL, BTLIF_REGISTER_SUBSYS_RSP, &rsp, sizeof(tBTLIF_RESULT_PARAM));
}

void send_unregistration_rsp(int fd, tBTL_IF_Result result)
{
    tBTL_PARAMS rsp;

    debug("send_unregistration_rsp [%d]", result);

    rsp.result.result = result;
    send_ctrl_msg(fd, SUB_CTRL, BTLIF_UNREGISTER_SUBSYS_RSP, &rsp, sizeof(tBTLIF_RESULT_PARAM));
}


static BOOLEAN process_new_conn(int fd)
{
    char ctrl_rx_buf[BTLIF_CTRL_RX_BUF_MAX];
    tBTLIF_CTRL_MSG_PKT *ctrl;
    tBTL_IF_HDR *hdr = (tBTL_IF_HDR*)ctrl_rx_buf;

    verbose("process_new_conn %d", fd);

    /* wait for header */
    if (rx_data(fd, (char *)hdr, sizeof(tBTL_IF_HDR)) <= 0)
    {
        return FALSE;
    }

    /* wait for payload */
    if (rx_data(fd, ctrl_rx_buf+sizeof(tBTL_IF_HDR), hdr->len) <=0)
    {
        return FALSE;
    }

    ctrl = (tBTLIF_CTRL_MSG_PKT *)ctrl_rx_buf;

    /* first control packet on a new ctrl socket needs to be a sub system registration */
    if (ctrl->hdr.id != BTLIF_REGISTER_SUBSYS_REQ)
    {
        error("error : protocol error [%x]", ctrl->hdr.id);
        send_registration_rsp(fd, BTL_IF_PROTOCOL_ERROR);
        return FALSE;
    }

    if (ctrl->sub >= BTL_IF_SUBSYSTEM_MAX)
    {
        error("error : invalid sub system");
        send_registration_rsp(fd, BTL_IF_SUBSYSTEM_INVALID);
        return FALSE;
    }

    /* first make sure this sub system is registered locally */
    if (subsystem_registered(ctrl->sub))
    {
        /* bind client to btl-if */
        if (attach_client(fd, ctrl->sub) == FALSE)
        {
            error("error : subsystem busy");
            send_registration_rsp(fd, BTL_IF_SUBSYSTEM_BUSY);
            return FALSE;
        }

        send_registration_rsp(fd, BTL_IF_SUCCESS);
    }
    else
    {
        error("error : subsystem not registered");
        send_registration_rsp(fd, BTL_IF_SUBSYSTEM_NOT_REGISTERED);
        return FALSE;
    }

    return TRUE;
}

static void btl_ifs_check_fd(int fdc, tSUB i)
{
    char ctrl_rx_buf[BTLIF_CTRL_RX_BUF_MAX];

    if ((fdc != DATA_SOCKET_INVALID) && (FD_ISSET(fdc, &read_set)))
    {
        int rx_discard = 0;
        tBTL_IF_HDR *hdr = (tBTL_IF_HDR*)ctrl_rx_buf;

        /* wait for header */
        if (rx_data(fdc, (char *)hdr, sizeof(tBTL_IF_HDR)) <= 0)
        {
            detach_client(i, fdc);
            return;
        }

        if (hdr->len > BTLIF_CTRL_RX_BUF_MAX-sizeof(tBTL_IF_HDR))
        {
            rx_discard = hdr->len - BTLIF_CTRL_RX_BUF_MAX-sizeof(tBTL_IF_HDR);
            error("[BTL_IFS] ERROR rx pkt size too big, truncate data (%d out of %d)", rx_discard, hdr->len);
            hdr->len=BTLIF_CTRL_RX_BUF_MAX-sizeof(tBTL_IF_HDR);
        }

        /* wait for payload */
        if (rx_data(fdc, ctrl_rx_buf+sizeof(tBTL_IF_HDR), hdr->len) <=0)
        {
            detach_client(i, fdc);
            return;
        }

        verbose("[CTRL] Received %d bytes on fd %d", hdr->len, fdc);

        if (hdr->id == BTLIF_UNREGISTER_SUBSYS_REQ)
        {
            tBTLIF_CTRL_MSG_PKT *ctrl;
            ctrl = (tBTLIF_CTRL_MSG_PKT *)ctrl_rx_buf;

            /* make sure subsystem is attached */
            if (sub_cb[ctrl->sub].ctrl_fd == DATA_SOCKET_INVALID)
            {
                send_unregistration_rsp(fdc, BTL_IF_NOT_CONNECTED);
                return;
            }
            send_unregistration_rsp(fdc, BTL_IF_SUCCESS);
            detach_client(ctrl->sub, fdc);
            return;
        }

        btl_if_ctrl_rx(fdc, i, ctrl_rx_buf);

        /* now discard any excess */
        if (rx_discard)
           rx_data(fdc, ctrl_rx_buf, rx_discard);

    }
}

// fixme -- use linked lists instead for all searches to avoid searching non active objects

void wrp_monitor_ctrl_fds(void)
{
    int i;

    /* manage incoming data */
    for (i=0; i < MAX_CLIENTS; i++)
    {
        int fdc;

        if (sub_cb[i].mclnt_enabled)
        {
            int j;
            for (j=0; j<SUBSYS_MAX_CLIENTS; j++)
            {
                fdc = sub_cb[i].mctrl_fd[j];
                btl_ifs_check_fd(fdc, i);
            }
        }
        else
        {
            fdc = sub_cb[i].ctrl_fd;
            btl_ifs_check_fd(fdc, i);
        }
   }
}


static void *main_server_thread(void *p)
{
    int result;
    int main_ctrl_listen_fd;

    if (state == STATE_SHUTTING_DOWN)
    {
        error("BTL-IF server start cancelled");
        return NULL;
    }

    state = STATE_RUNNING;

    verbose("BTL-IF server thread starting...");

    main_ctrl_listen_fd = btl_if_ctrl_listen();

    if (main_ctrl_listen_fd < 0)
    {
        error("critical error :: main ctrl channel listen failed: %s", strerror(errno));
        close_all_open_sockets();
        state = STATE_STOPPED;
        exit(1);
    }

    FD_SET(main_ctrl_listen_fd, &active_set);

    max_fd = MAX(max_fd, main_ctrl_listen_fd);
    property_set(DTUN_PROPERTY_FM_BTLIF_SERVER_ACTIVE, "1");

    while (state == STATE_RUNNING)
    {
        read_set = active_set;

        /* wait for a connection or socket data, blocking call */
        verbose("main_server_thread: waiting for select...");

        result = select(max_fd+1, &read_set, NULL, NULL, NULL);

        if (result == 0)
        {
            //info("select timeout");
            continue;
        }
        else if (result < 0 && errno != EINTR)
        {
            /* Error can happen because of the data-path disconnect, so just continue */
            error("select err %s", strerror(errno));
            continue;
        }

        if (btl_ifs_select_wake_signaled(&read_set))
        {
            verbose("main_server_thread: signal socket signaled");

            btl_ifs_select_wake_reset();

            /* Exit if state had changed to no longer RUNNING while we were waiting for 'select' */
            if (state != STATE_RUNNING)
            {
                info("main_server_thread: bt shutdown while waiting for select");
                break;
            }
        }

        verbose("main_server_thread: woke up");

        /* now check any pending fd:s */
        if (FD_ISSET(main_ctrl_listen_fd, &read_set))
        {
            struct sockaddr_un client_address;
            socklen_t clen;
            int new_fd;

            clen = sizeof(client_address);
            new_fd = accept(main_ctrl_listen_fd,
                        (struct sockaddr*)&client_address,
                        &clen);

            /* incoming connection */
            info("[CTRL] Client connected (%d)", new_fd);

            if (process_new_conn(new_fd) == TRUE)
            {
                /* add this fd to select set */
                ctrl_add_active_set(new_fd);
            }
            else
            {
                error("Client registration failed, disconnecting socket... (%d)", new_fd);
                close(new_fd);
            }
        }
        else
        {
            /* check for incoming data packet */
            wrp_monitor_data_fds();

            /* check for incoming ctrl msg */
            wrp_monitor_ctrl_fds();
        }
    }

    close_all_open_sockets();

    wrp_close_data(main_ctrl_listen_fd);
    close(main_ctrl_listen_fd);
    state = STATE_STOPPED;

    info("BTL-IF server thread terminating...");

    return NULL;
}


void close_all_open_sockets(void)
{
    int sub;

    info("Closing all open sockets");

    /* close all data sockets */
    ws_close_all_active();

    /* close all listeners */
    ws_close_all_listeners();

    /* close all ctrl sockets */
    for (sub = 0; sub<BTL_IF_SUBSYSTEM_MAX; sub++)
    {
        if (sub_cb[sub].mclnt_enabled)
        {
            int index;
            for (index=0; index<SUBSYS_MAX_CLIENTS; index++)
            {
                if (sub_cb[sub].mctrl_fd[index] != DATA_SOCKET_INVALID)
                    detach_client(sub, sub_cb[sub].mctrl_fd[index]);
            }
        }
        else if (sub_cb[sub].ctrl_fd != DATA_SOCKET_INVALID)
        {
            detach_client(sub, sub_cb[sub].ctrl_fd);
        }
    }
}

/* notify we are disconnecting and rx buffer is pending */
void btl_if_notify_rx_buf_pending(tDATA_HANDLE handle)
{
    tBTL_PARAMS params;
    t_wsock *ws;

    ws = wrp_find_wsock(handle);

    if (ws && ws->rx_buf && ws->rx_buf_pending)
    {
        info("rx buffer pending, notify user (0x%x %d)", (int)ws->rx_buf, ws->rx_buf_size);

        params.rx_buf_pnd.handle = ws->wsock;
        params.rx_buf_pnd.rx_buf = ws->rx_buf;
        params.rx_buf_pnd.rx_buf_size = ws->rx_buf_size;

        /* user should have feed this buffer now and we can unset pending */
        ws->rx_buf_pending = 0;

        btl_if_notify_local_event(sub_cb[ws->sub].ctrl_fd, ws->sub, BTLIF_DISC_RX_BUF_PENDING, &params);
    }
}

/*******************************************************************************
**                                   BTL IF  SERVER API
**
*******************************************************************************/



/*******************************************************************************
**
** Function          BTL_IF_RegisterSubSystem
**
** Description      Register sub system datapath handler in local server.
**
**
** Returns
**
*******************************************************************************/

tBTL_IF_Result BTL_IF_RegisterSubSystem(tCTRL_HANDLE *handle, tBTL_IF_SUBSYSTEM sub, tBTL_IF_DATA_CALLBACK data_cb, tBTL_IF_CTRL_CALLBACK ctrl_cb)
{
    info("Registered subsystem [%s]", sub2str[sub]);

    CHECK_SUBSYSTEM(sub);

    /* register subsystem locally */
    if (register_subsystem(sub, data_cb, ctrl_cb) == FALSE)
    {
        error("register failed");
        return BTL_IF_GENERIC_ERR;
    }

    /* 1:1 mapping between handle and subsys */
    *handle = sub;

    return BTL_IF_SUCCESS;
}


/*******************************************************************************
**
** Function          BTL_IF_RegisterSubSystemMultiClnt
**
** Description      Register sub system datapath handler in local server.
**
**
** Returns
**
*******************************************************************************/

/* fixme -- use register multiclient for all subsystems, currently only BTS uses multiclient support */

tBTL_IF_Result BTL_IF_RegisterSubSystemMultiClnt(tBTL_IF_SUBSYSTEM sub, tBTL_IF_DATA_CALLBACK data_cb, tBTL_IF_CTRL_CALLBACK ctrl_cb)
{
    info("Registered subsystem [%s]", sub2str[sub]);

    CHECK_SUBSYSTEM(sub);

    /* register subsystem locally */
    if (register_subsystem_mclient(sub, data_cb, ctrl_cb) == FALSE)
    {
        error("register failed");
        return BTL_IF_GENERIC_ERR;
    }

    /* handle is returned with BTLIF_SUBSYSTEM_ATTACHED upon client attach */

    return BTL_IF_SUCCESS;
}


/*******************************************************************************
**
** Function          BTL_IF_UnregisterSubSystem
**
** Description      Unregister sub system datapath handler in local server.
**
**
** Returns
**
*******************************************************************************/

tBTL_IF_Result BTL_IF_UnregisterSubSystem(tBTL_IF_SUBSYSTEM sub)
{
    info("Unregistered subsystem [%s]", sub2str[sub]);

    CHECK_SUBSYSTEM(sub);

    /* unregister subsystem locally */
    if (unregister_subsystem(sub) == FALSE)
    {
        error("unregister failed");
        return BTL_IF_GENERIC_ERR;
    }

    return BTL_IF_SUCCESS;
}

/*******************************************************************************
**
** Function          BTL_IF_ConnectDatapath
**
** Description
**
**
** Returns          tBTL_IF_Result
**
*******************************************************************************/

tBTL_IF_Result BTL_IF_ConnectDatapath(tCTRL_HANDLE ctrl_hdl, tDATA_HANDLE* handle, tBTL_IF_SUBSYSTEM sub, int sub_port)
{
    int s;

    info("[BTL_IFS] connect datapath [%s] on sub port %d", sub2str[sub], sub_port);

    CHECK_SUBSYSTEM(sub);

    s = wrp_setup_data_connect(ctrl_hdl, sub, sub_port);

    if (s<0)
        return BTL_IF_GENERIC_ERR;

    /* add to active set if we have a registered data callback */
    if (sub_cb[sub].data_cb)
        wrp_add_to_active_set(s);

    btl_ifs_select_wakeup();

    *handle = s;

    info("[BTL_IFS] got datapath handle %d", s);

    return BTL_IF_SUCCESS;
}


/*******************************************************************************
**
** Function          BTL_IF_AttachExtFd
**
** Description      Add external filedescriptor to monitor in main server thread
**
**
** Returns          tBTL_IF_Result
**
*******************************************************************************/

tBTL_IF_Result BTL_IF_AttachExtFd(tCTRL_HANDLE ctrl_hdl, int ext_fd, tSUB sub, int sub_port)
{
    t_wsock *ws;
    tBTL_PARAMS param;

    info("[BTL_IFS] attach ext fd [%s] on sub port %d (ctrl fd %d)", sub2str[sub], sub_port, ctrl_hdl);

    CHECK_SUBSYSTEM(sub);

    if (ext_fd<0)
        return BTL_IF_GENERIC_ERR;

    /* create fake entry */
    ws = wrp_sock_create_ext(ctrl_hdl, sub, (char *)sub2str[sub], wrp_getport(sub, sub_port), ext_fd);

    if (!ws)
        return BTL_IF_GENERIC_ERR;

    /* notify user via ctrl callback */
    param.chan_ind.handle = ws->wsock;
    param.chan_ind.subport = sub_port;

    /* automatically added to active set */
    btl_if_notify_local_event(ctrl_hdl, ws->subsystem, BTLIF_DATA_CHAN_IND, &param);

    info("[BTL_IFS] attached ext fd %d", ext_fd);

    return BTL_IF_SUCCESS;
}

/*******************************************************************************
**
** Function           BTL_IF_WaitForDataChan
**
** Description      Blocking wait
**
**
** Returns
**
*******************************************************************************/

tBTL_IF_Result BTL_IF_WaitForDataChan(tDATA_HANDLE *handle, tCTRL_HANDLE ctrl_fd, tBTL_IF_SUBSYSTEM sub, int sub_port)
{
    t_wsock *ws;
    tDATA_HANDLE hdl;

    info("[BTL_IFS] waiting for datachannel [%s]", sub2str[sub]);

    CHECK_SUBSYSTEM(sub);

    ws = wrp_setup_data_listener(ctrl_fd, sub, sub_port);

    /* don't add listen fd to active set as this is a blocking call */
    ws_listener_add(ws, 0);

    hdl = wrp_data_accept(ws);

    if (hdl<0)
        return BTL_IF_GENERIC_ERR;

    ws_del_sub_listeners(sub);

    *handle = hdl;

    return BTL_IF_SUCCESS;
}

/*******************************************************************************
**
** Function          BTL_IF_SetupRxBuf
**
** Description
**
**
** Returns          tBTL_IF_Result
**
*******************************************************************************/

tBTL_IF_Result BTL_IF_SetupRxBuf(tDATA_HANDLE handle, char *p_rx, int size)
{
    int s;
    t_wsock *ws = wrp_find_wsock(handle);

    if (!ws)
    {
        error("BTL_IF_SetupRxBuf, could not find wsock");
        return BTL_IF_GENERIC_ERR;
    }

    verbose("Set receive buffer 0x%x, %d bytes", p_rx, size);

    wrp_setup_rxbuf(ws, p_rx, size);

    return BTL_IF_SUCCESS;
}

char* BTL_IF_GetRxBuf(tDATA_HANDLE handle)
{
    t_wsock *ws = wrp_find_wsock(handle);

    if (!ws)
    {
        error("Could not find wsock");
        return BTL_IF_GENERIC_ERR;
    }
    return ws->rx_buf;

}
/*******************************************************************************
**
** Function          BTL_IF_SetupRxFlow
**
** Description
**
**
** Returns          tBTL_IF_Result
**
*******************************************************************************/

tBTL_IF_Result BTL_IF_SetupRxFlow(tDATA_HANDLE handle, BOOLEAN flow_on)
{
    int s;
    t_wsock *ws = wrp_find_wsock(handle);

    if (!ws)
    {
        error("BTL_IF_SetupRxFlow, could not find wsock");
        return BTL_IF_GENERIC_ERR;
    }

    verbose("BTL_IF_SetupRxFlow : handle %d, flow_on %d", handle, flow_on);

    wrp_setup_rxflow(ws, flow_on);

    btl_ifs_select_wakeup();

    return BTL_IF_SUCCESS;
}


/*******************************************************************************
**
** Function           BTL_IF_SetupListener
**
** Description      Non blocking wait.
**                       Data connect ind will be notified through ctrl callback
**
** Returns
**
*******************************************************************************/

tBTL_IF_Result BTL_IF_SetupListener(tCTRL_HANDLE ctrl_hdl, tBTL_IF_SUBSYSTEM sub, int sub_port)
{
    t_wsock *ws;

    info("[BTL_IFS] setup datachannel listener [%s], sub_port %d", sub2str[sub], sub_port);

    CHECK_SUBSYSTEM(sub);

    ws = wrp_setup_data_listener(ctrl_hdl, sub, sub_port);

    if (!ws)
        return BTL_IF_GENERIC_ERR;

    /* add to active set */
    ws_listener_add(ws, 1);

    /* make sure select loop updates read set */
    btl_ifs_select_wakeup();

    return BTL_IF_SUCCESS;
}

/*******************************************************************************
**
** Function           BTL_IF_CancelListener
**
** Description
**
**
** Returns
**
*******************************************************************************/

tBTL_IF_Result BTL_IF_CancelListener(tCTRL_HANDLE ctrl_fd, tBTL_IF_SUBSYSTEM sub, int sub_port)
{
    t_wsock *ws;

    info("[BTL_IFS] cancel datachannel listener [%s], sub_port %d", sub2str[sub], sub_port);

    CHECK_SUBSYSTEM(sub);

    ws = wrp_find_wsock_by_port(sub, sub_port);

    if (ws)
    {
        /* remove from active set */
        ws_listener_del(ws);

        wrp_close_all(ws);
    }
    else
    {
        info("[BTL_IFS] subport not found");
    }

    return BTL_IF_SUCCESS;
}


/*******************************************************************************
**
** Function          BTL_IF_SendData
**
** Description       Sends data to BTL
**
**
** Returns          tBTL_IF_Result
**
*******************************************************************************/

tBTL_IF_Result BTL_IF_SendData(tDATA_HANDLE handle, char *p, int len)
{
    t_wsock *ws;

    ws = wrp_find_wsock(handle);

    if (!ws)
        return BTL_IF_GENERIC_ERR;

    if (tx_data(handle, p, len, ws->non_sock) < 0)
    {
        return BTL_IF_GENERIC_ERR;
    }
    return BTL_IF_SUCCESS;
}

/*******************************************************************************
**
** Function          BTL_IF_DisconnectDatapath
**
** Description      Disconnect datapath channel
**
**
** Returns          tBTL_IF_Result
**
*******************************************************************************/

tBTL_IF_Result BTL_IF_DisconnectDatapath(tDATA_HANDLE handle)
{
    t_wsock *ws;

    info("Disconnect datapath on handle %d", handle);

    ws = wrp_find_wsock(handle);

    if (!ws)
    {
        debug("wsock not found, already disconnected ?");
        return BTL_IF_GENERIC_ERR;
    }

    /* make sure flow is off */
    wrp_setup_rxflow(ws, FALSE);

    /* check if we have any pending rx buffers */
    btl_if_notify_rx_buf_pending(handle);

    /* make sure any blocking r/w are cleared before calling close */
    btl_ifs_select_wakeup();

    /* now close the data connection */
    wrp_close_data(handle);

    return BTL_IF_SUCCESS;
}

/*******************************************************************************
**
** Function          BTL_IF_CtrlSend
**
** Description     Send control msg
**
**
** Returns          tBTL_IF_DataPath
**
*******************************************************************************/

tBTL_IF_Result BTL_IF_CtrlSend(tCTRL_HANDLE handle, tSUB sub, tBTLIF_CTRL_MSG_ID msg_id, tBTL_PARAMS *params, int param_len)
{
    int result;

    CHECK_SUBSYSTEM(sub);

    if (!is_client_attached(sub))
    {
        error("No client is registered for sub system %s", sub2str[sub]);
        return BTL_IF_NOT_READY;
    }

    if (sub_cb[sub].mclnt_enabled)
        result = send_ctrl_msg(handle, sub, msg_id, params, param_len);
    else
        result = send_ctrl_msg(sub_cb[handle].ctrl_fd, sub, msg_id, params, param_len);

    if (result < 0)
        return BTL_IF_GENERIC_ERR;

    return BTL_IF_SUCCESS;
}


/*******************************************************************************
**
** Function          BTL_IF_SendMsgNoParams
**
** Description     Send control msg without any parameters
**
**
** Returns          tBTL_IF_Result
**
**
*******************************************************************************/

tBTL_IF_Result BTL_IF_SendMsgNoParams(tCTRL_HANDLE handle, tSUB sub, tBTLIF_CTRL_MSG_ID msg_id)
{
    int result;

    CHECK_SUBSYSTEM(sub);

    result = BTL_IF_CtrlSend(handle, sub, msg_id, NULL, 0);

    if (result<0)
        return BTL_IF_GENERIC_ERR;

    return BTL_IF_SUCCESS;
}


/*******************************************************************************
**
** Function          BTL_IF_ServerInit
**
** Description      Init BTL interface server
**
**
** Returns           tBTL_IF_Result
**
*******************************************************************************/

tBTL_IF_Result BTL_IF_ServerInit(void)
{
    int i;

    verbose("BTL-IF Server initiating");

    if (state != STATE_STOPPED)
    {
        return BTL_IF_ALREADY_INITIALIZED;
    }

    wrp_sock_init();

    FD_ZERO(&read_set);
    FD_ZERO(&active_set);

    max_fd = btl_ifs_select_wakeup_init(&active_set);

    state = STATE_STARTING;

    /* create main server thread using inherited settings (joinable!) */
    if ( pthread_create(&server_thread_id, NULL, (void*)main_server_thread, NULL)!=0 )
        error("pthread_create : %s", strerror(errno));

    /* initialize client database */
    for (i=0; i<MAX_CLIENTS; i++)
    {
        sub_cb[i].ctrl_fd = DATA_SOCKET_INVALID;
        sub_cb[i].data_cb = NULL;
        sub_cb[i].ctrl_cb = NULL;
        sub_cb[i].sub = SUB_NONE;
    }

#ifdef BTL_IF_TARGET_CONSOLE
    btl_ifs_console_server_start();
#endif

    return BTL_IF_SUCCESS;
}

/*******************************************************************************
**
** Function          BTL_IF_ServerShutdown
**
** Description      Shutdown BTL interface server
**
**
** Returns           tBTL_IF_Result
**
*******************************************************************************/

tBTL_IF_Result BTL_IF_ServerShutdown(void)
{
    if (state != STATE_RUNNING)
    {
        error("BTL-IF server never started, ignore shutdown request");
        return BTL_IF_GENERIC_ERR;
    }

    state = STATE_SHUTTING_DOWN;

    btl_ifs_select_wakeup();

    info("BTL-IF Server shutting down...");

    /* wait for thread clean up sockets and exit */
    if ( 0 != pthread_join( server_thread_id, NULL ) )
    {
        error( "Failed to join main_server_thread" );
    }

    info("BTL-IF Server shutdown complete.");

    /* Comment out DTUN server stop method for BTLZ as it is never enabled */
    /*dtun_server_stop();*/

    return BTL_IF_SUCCESS;
}

/* FIXME -- deprecate these custom APIs and use standard btlif api */

/*******************************************************************************
**
** Some common API usage
**
**
*******************************************************************************/

int BTLIF_ConnectRsp(tCTRL_HANDLE handle, tSUB sub, tBTL_IF_Result result, tSTATUS status)
{
    tBTL_PARAMS params;

    params.conrsp.result = result;
    params.conrsp.status = status;

    return BTL_IF_CtrlSend(handle, sub, BTLIF_CONNECT_RSP, &params, sizeof(tBTLIF_CONNECT_RSP_PARAM));
}


/* AG */

int BTLIF_AG_ConnectReq(tCTRL_HANDLE handle, BD_ADDR *p_bd, unsigned short rf_chan)
{
    tBTL_PARAMS params;

    memcpy(params.ag_conreq.bd, p_bd, 6);
    params.ag_conreq.rf_chan = rf_chan;

    return BTL_IF_CtrlSend(handle, SUB_AG, BTLIF_CONNECT_REQ, &params, sizeof(tBTLIF_AG_CONNECT_REQ_PARAM));
}

int BTLIF_AG_ConnectRsp(tCTRL_HANDLE handle, tBTL_IF_Result result, tSTATUS status)
{
    tBTL_PARAMS params;

    params.conrsp.result = result;
    params.conrsp.status = status;

    return BTL_IF_CtrlSend(handle, SUB_AG, BTLIF_CONNECT_RSP, &params, sizeof(tBTLIF_CONNECT_RSP_PARAM));
}

int BTLIF_AG_ConnectInd(tCTRL_HANDLE handle, BD_ADDR *p_bd, unsigned short rf_chan)
{
    tBTL_PARAMS params;

    memcpy(params.ag_conreq.bd, p_bd, 6);
    params.ag_conreq.rf_chan = rf_chan;

    return BTL_IF_CtrlSend(handle, SUB_AG, BTLIF_CONNECT_IND, &params, sizeof(tBTLIF_AG_CONNECT_REQ_PARAM));
}

int BTLIF_AG_DisconnectInd(tCTRL_HANDLE handle, unsigned short rf_chan)
{
    tBTL_PARAMS params;

    params.ag_disc.rf_chan = rf_chan;

    return BTL_IF_CtrlSend(handle, SUB_AG, BTLIF_DISCONNECT_IND, &params, sizeof(tBTLIF_AG_DISCONNECT_PARAM));
}

int BTLIF_AG_DisconnectRsp(tCTRL_HANDLE handle, unsigned short rf_chan)
{
    tBTL_PARAMS params;

    params.ag_disc.rf_chan = rf_chan;

    return BTL_IF_CtrlSend(handle, SUB_AG, BTLIF_DISCONNECT_RSP, &params, sizeof(tBTLIF_AG_DISCONNECT_PARAM));
}


/* SCO */
int BTLIF_SCO_ConnectRsp(tCTRL_HANDLE handle, tBTL_IF_Result result, tSTATUS status)
{
    tBTL_PARAMS params;

    params.conrsp.result = result;
    params.conrsp.status = status;

    return BTL_IF_CtrlSend(handle, SUB_SCO, BTLIF_CONNECT_RSP, &params, sizeof(tBTLIF_CONNECT_RSP_PARAM));
}


















#if defined(BTL_IF_TEST_ENABLE) || defined(BTL_IF_TARGET_CONSOLE)

/* FIXME -- MOVE INTO SEPARATE FILE */

/*******************************************************************************
**
** Function        TEST  MAIN
**
**
*******************************************************************************/

tDATA_HANDLE ag_dhandle;
tCTRL_HANDLE ag_chandle;
unsigned short rf_chan_stored1 = -1;
unsigned short rf_chan_stored2 = -1;

#define BTLDLOG(format, ...) fprintf (stdout, "\t[BTLD SIM] " format, ## __VA_ARGS__)

static void rw_thread(void *p)
{
    int fd = (int)p;
    char buf[1024];
    int i = 5;
    int slen;
    int n;

    info("jni_read_write_thread starting... (fd %d)", fd);

    while (i--)
    {
        slen = sprintf(buf, "%d - rw thread server test data blablablablablablabla", i);

        n = write(fd, buf, slen);

        if (n<0)
            perror("write");

        printf("<--- %d bytes [%s]", n, buf);

#if 0
        n = read(fd, buf, 50);

        if (n<0)
            perror("read");

        printf("---> %d bytes [%s]", n, buf);
#endif
    }

    sleep(3);

    info("now request disconnect of datapath\n");

    BTLIF_AG_DisconnectInd(ag_chandle, rf_chan_stored1);

    //BTL_IF_DisconnectDatapath(fd);
}


void start_rw_thread(tDATA_HANDLE hdl)
{
    pthread_t rw_thread_id;

    /* create AT parser test thread */
    if (pthread_create(&rw_thread_id, NULL,
                  (void*)rw_thread, (void*)hdl)!=0)
       perror("pthread_create");
}


static void BTLD_AG_ReceiveData(tDATA_HANDLE handle, char *p, int len)
{
    BTLDLOG("[AG DATA] RX %d bytes, echo back !", len);

    BTL_IF_SendData(handle, p, len);
}

static void BTLD_AG_ReceiveCtrlMsg(tCTRL_HANDLE ctrl_hdl, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params)
{
    BTLDLOG("[AG CTRL] RX [%s] (%d)", dump_msg_id(id), ctrl_hdl);

    switch(id)
    {
        case BTLIF_CONNECT_REQ:
           {
                /* *** EXAMPLE CODE *** */

                /* TODO -- init btld ag connectvion and send connect rsp when complete. For now respond directly */

                BTLDLOG("Incoming AG connection on rfchan %d", params->ag_conreq.rf_chan);

                /* data channel notification will come through ctrl callback */
                BTL_IF_SetupListener(ctrl_hdl, SUB_AG, params->ag_conreq.rf_chan);

                rf_chan_stored1 = params->ag_conreq.rf_chan;


                BTLIF_AG_ConnectRsp(ag_chandle, BTL_IF_SUCCESS, 0);
                //BTLIF_AG_ConnectRsp(ag_chandle, BTL_IF_GENERIC_ERR, 0);
           }
           break;

        case BTLIF_LISTEN_REQ:
            {
                /* *** EXAMPLE CODE *** */

                BD_ADDR remote_bd = {0x1,0xaa,0xbb,0xaa,0xdd,0x1};

                BTLDLOG("sleep 3 secs and then test incoming data connection");
                sleep(3);

                /* Now notify client about an incoming call */
                BTLDLOG("INCOMING CONNECTION on sub port %d !", params->ag_listen.rf_chan);

                rf_chan_stored1 = params->ag_listen.rf_chan;

                /* Notify client that there is a new AG link */
                BTLIF_AG_ConnectInd(ag_chandle, &remote_bd, params->ag_listen.rf_chan);

                /* Connect data path */
                BTL_IF_ConnectDatapath(ag_chandle, &ag_dhandle, SUB_AG, params->ag_listen.rf_chan);
            }
            break;

        case BTLIF_DISCONNECT_REQ:
            {
                /* Remote side wants to initiate a disconnect */
                BTLDLOG("Client requested disconnect of rf chan %d", params->ag_disc.rf_chan);

                /* Emulate disconnection delay */
                sleep(1);
                BTLDLOG("BTLD disconnected rf chan %d !\n", params->ag_disc.rf_chan);

                /* Confirm disconnection */
                BTLIF_AG_DisconnectRsp(ag_chandle, params->ag_disc.rf_chan);

                /* Disconnect initiator is responsible of disconnecting data channel */
            }
            break;

        case BTLIF_DATA_CHAN_IND:
            BTLDLOG("Server got new data channel (%d)", params->chan_ind.handle);

            ag_dhandle = params->chan_ind.handle;

            start_rw_thread(ag_dhandle);
            break;

        default:
            //BTLDLOG("Unknown ctrl message received (%d)", id);
            break;
    }
}

void ag_test_incoming_disconnection(unsigned short rf_chan)
{
    BTLDLOG("Link disconnected remotly, send disconnect ind");

    /* Send disconnect ind notification */
    BTLIF_AG_DisconnectInd(ag_chandle, rf_chan);

    BTLDLOG("Now disconnect datapath");

    /* Disconnect data channel */
    BTL_IF_DisconnectDatapath(ag_dhandle);
}

void server_test_waitconnection(void)
{
    BTLDLOG("server_test_waitconnection");

    /* Initialize datapath server */
    BTL_IF_ServerInit();

    /* Register data callback for subsystem AV */
    BTL_IF_RegisterSubSystem(&ag_chandle, SUB_AG, BTLD_AG_ReceiveData, BTLD_AG_ReceiveCtrlMsg);

    // temp
    while(1)
        sleep(1);

    sleep(30);

    ag_test_incoming_disconnection(rf_chan_stored1);
    ag_test_incoming_disconnection(rf_chan_stored2);

    sleep(10);
}

void server_test_initiate_conn(void)
{
    BD_ADDR bd = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    char *at_cmd = "AT+VGS=15";

    BTLDLOG("server_test_initiate_conn\n");

    /* Initialize datapath server */
    BTL_IF_ServerInit();

    /* Register handler for subsystem AG */
    BTL_IF_RegisterSubSystem(&ag_chandle, SUB_AG, BTLD_AG_ReceiveData, BTLD_AG_ReceiveCtrlMsg);

    sleep(10);

    rf_chan_stored1 = 3;
    BTLIF_AG_ConnectReq(ag_chandle, &bd, rf_chan_stored1);

    sleep(3);

    BTLDLOG("Send AT command\n");

    /* send some data */
    BTL_IF_SendData(ag_dhandle, at_cmd, strlen(at_cmd));
    BTL_IF_SendData(ag_dhandle, at_cmd, strlen(at_cmd));
    BTL_IF_SendData(ag_dhandle, at_cmd, strlen(at_cmd));

    sleep(1);

    /* no need for confirmation from server side, just notify with an indication. Server will close any open data channels */
    BTLIF_AG_DisconnectInd(ag_chandle, rf_chan_stored1);

    BTL_IF_ServerShutdown();
}


/////////////////////////////////////////////////////////////////////////////////////////////////

tDATA_HANDLE sco_data;
tCTRL_HANDLE sco_ctrl;

static void BTLD_SCO_ReceiveData(tDATA_HANDLE handle, char *p, int len)
{
    BTLDLOG("[SCO DATA] Received %d bytes, echo back !", len);

    BTL_IF_SendData(handle, p, len);
}

static void BTLD_SCO_ReceiveCtrlMsg(tCTRL_HANDLE ctrl_hdl, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params)
{
    BTLDLOG("[SCO CTRL] Received message [%s]", dump_msg_id(id));

    switch(id)
    {
        case BTLIF_CONNECT_REQ:
           {
                /* *** EXAMPLE CODE *** */

                /* TODO -- init btld ag connectvion and send connect rsp when complete. For now respond directly */

                BTLDLOG("Incoming SCO connection %d", sco_ctrl);

                /* data channel notification will come through ctrl callback */
                BTL_IF_SetupListener(sco_ctrl, SUB_SCO, 0);

                BTLIF_SCO_ConnectRsp(sco_ctrl, BTL_IF_SUCCESS, 0);
           }
           break;

        case BTLIF_LISTEN_REQ:
            {
                /* *** EXAMPLE CODE *** */

                BTLDLOG("sleep 2 secs and then test incoming data connection");
                sleep(2);

                /* Now notify client about an incoming call */
                BTLDLOG("Emulate INCOMING connection on SCO...");

                /* Notify client that there is a new SCO link */
                BTL_IF_SendMsgNoParams(sco_ctrl, SUB_SCO, BTLIF_CONNECT_IND);

                /* Connect data path on subport 0 */
                BTL_IF_ConnectDatapath(sco_ctrl, &sco_data, SUB_SCO, 0);

                BTLDLOG("SCO datapath setup (%d) !", sco_data);
            }
            break;

        case BTLIF_DISCONNECT_REQ:
            {
                /* Remote side wants to initiate a disconnect */
                BTLDLOG("Client requested disconnect SCO chan");

                /* Emulate disconnection delay */
                sleep(1);
                BTLDLOG("BTLD disconnected sco link");

                /* Confirm disconnection */
                BTL_IF_SendMsgNoParams(sco_ctrl, SUB_SCO, BTLIF_DISCONNECT_RSP);

                /* Disconnect initiator is responsible of disconnecting data channel */
            }
            break;

        case BTLIF_DATA_CHAN_IND:
            BTLDLOG("Server got new data channel (%d)", params->chan_ind.handle);
            sco_data = params->chan_ind.handle;
            break;

        default:
            BTLDLOG("Unknown ctrl message received (%d)", id);
            break;
    }
}


void sco_wait(void)
{
    BTLDLOG("sco_wait");

    /* Initialize datapath server */
    BTL_IF_ServerInit();

    /* Register data callback for subsystem AV */
    BTL_IF_RegisterSubSystem(&sco_ctrl, SUB_SCO, BTLD_SCO_ReceiveData, BTLD_SCO_ReceiveCtrlMsg);

    while(1)
        sleep(1);
}

void all_wait(void)
{
    /* Initialize datapath server */
    BTL_IF_ServerInit();

    dtun_server_start();

    /* Register data callback for subsystem AV */
    BTL_IF_RegisterSubSystem(&ag_chandle, SUB_AG, BTLD_AG_ReceiveData, BTLD_AG_ReceiveCtrlMsg);

    /* Register data callback for subsystem AV */
    BTL_IF_RegisterSubSystem(&sco_ctrl, SUB_SCO, BTLD_SCO_ReceiveData, BTLD_SCO_ReceiveCtrlMsg);

    while (1)
        sleep(1);
}

static void BTLD_AV_ReceiveCtrlMsg(tCTRL_HANDLE ctrl_hdl, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params)
{
    printf("AV ctrl %d", id);
}

void av_data_wait(void)
{
    tDATA_HANDLE av_hdl;
    int fd;

    /* Initialize datapath server */
    BTL_IF_ServerInit();

    sleep(2);

    /* Register data callback for subsystem AV */
    BTL_IF_RegisterSubSystem(&av_hdl, SUB_AV, NULL, BTLD_AV_ReceiveCtrlMsg);

    while (1)
    {
        char buf[20];
        int n = 1;
        BTL_IF_WaitForDataChan(&fd, av_hdl, SUB_AV, 0);

        while (n>0)
        {
            n = read(fd, buf, 7);
            printf("read %d", n);
        }

        printf("Link disconnected");

        BTL_IF_DisconnectDatapath(fd);

        sleep(1);
    }
}


/*******************************************************************************
**
**  Console cmd table
**
*******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#if defined(HAVE_READLINE) && HAVE_READLINE==1
#include <readline/readline.h>
#include <readline/history.h>
#else
#define readline(p) local_getline(p,stdin)
#define add_history(X)
#define read_history(X)
#define write_history(X)
#endif

#define HISTORY_FILE ".history"

#if !defined(HAVE_READLINE)

static char *local_getline(char *zPrompt, FILE *in){
  char *zLine;
  int nLine;
  int n;
  int eol;

  if( zPrompt && *zPrompt ){
    printf("%s",zPrompt);
    fflush(stdout);
  }
  nLine = 100;
  zLine = malloc( nLine );
  if( zLine==0 ) return 0;
  n = 0;
  eol = 0;
  while( !eol ){
    if( n+100>nLine ){
      nLine = nLine*2 + 100;
      zLine = realloc(zLine, nLine);
      if( zLine==0 ) return 0;
    }
    if( fgets(&zLine[n], nLine - n, in)==0 ){
      if( n==0 ){
        free(zLine);
        return 0;
      }
      zLine[n] = 0;
      eol = 1;
      break;
    }
    while( zLine[n] ){ n++; }
    if( n>0 && zLine[n-1]=='\n' ){
      n--;
      zLine[n] = 0;
      eol = 1;
    }
  }
  zLine = realloc( zLine, n+1 );
  return zLine;
}

#endif


void skip_blanks(char **p)
{
  while (**p == ' ')
    (*p)++;
}


UINT32 get_int(char **p, int DefaultValue)
{
  UINT32 Value = 0;
  BOOLEAN   UseDefault;

  UseDefault = TRUE;
  skip_blanks(p);

  while ( ((**p)<= '9' && (**p)>= '0') )
    {
      Value = Value * 10 + (**p) - '0';
      UseDefault = FALSE;
      (*p)++;
    }

  if (UseDefault)
    return DefaultValue;
  else
    return Value;
}

int get_signed_int(char **p, int DefaultValue)
{
  int    Value = 0;
  BOOLEAN   UseDefault;
  BOOLEAN   NegativeNum = FALSE;

  UseDefault = TRUE;
  skip_blanks(p);

  if ( (**p) == '-')
    {
      NegativeNum = TRUE;
      (*p)++;
    }
  while ( ((**p)<= '9' && (**p)>= '0') )
    {
      Value = Value * 10 + (**p) - '0';
      UseDefault = FALSE;
      (*p)++;
    }

  if (UseDefault)
    return DefaultValue;
  else
    return ((NegativeNum == FALSE)? Value : -Value);
}

void get_str(char **p, char *Buffer)
{
  skip_blanks(p);

  while (**p != 0 && **p != ' ')
    {
      *Buffer = **p;
      (*p)++;
      Buffer++;
    }

  *Buffer = 0;
}

UINT32 get_hex(char **p, int DefaultValue)
{
  UINT32 Value = 0;
  BOOLEAN   UseDefault;

  UseDefault = TRUE;
  skip_blanks(p);

  while ( ((**p)<= '9' && (**p)>= '0') ||
          ((**p)<= 'f' && (**p)>= 'a') ||
          ((**p)<= 'F' && (**p)>= 'A') )
    {
      if (**p >= 'a')
        Value = Value * 16 + (**p) - 'a' + 10;
      else if (**p >= 'A')
        Value = Value * 16 + (**p) - 'A' + 10;
      else
        Value = Value * 16 + (**p) - '0';
      UseDefault = FALSE;
      (*p)++;
    }

  if (UseDefault)
    return DefaultValue;
  else
    return Value;
}

void get_bdaddr(const char *str, bdaddr_t *ba) {
    char *d = ((char *)ba) + 5, *endp;
    int i;
    for(i = 0; i < 6; i++) {
        *d-- = strtol(str, &endp, 16);
        if (*endp != ':' && i != 5) {
            memset(ba, 0, sizeof(bdaddr_t));
            return;
        }
        str = endp + 1;
    }
}


tCTRL_HANDLE console_hdl = CTRL_SOCKET_INVALID;


void
RemoteLogMsg(const char *fmt_str, ...)
{
    va_list ap;
    tBTL_PARAMS param;
    int len;

    memset(param.dbg_console_param.buffer, 0, 256);

    va_start(ap, fmt_str);
    vsprintf(param.dbg_console_param.buffer, fmt_str, ap);

    len = strlen(param.dbg_console_param.buffer);

    ASSERTC(len < 256, "overflow", len);

    BTL_IF_CtrlSend(console_hdl, SUB_DBG, BTLIF_DBG_EVT_STRING_MSG, &param, len);

    va_end(ap);
}



#define is_cmd(str) ((strlen(str) == strlen(cmd)) && strncmp((const char *)&cmd, str, strlen(str)) == 0)
#define if_cmd(str)  if (is_cmd(str))

typedef void (t_console_cmd_handler) (char *p);

typedef struct {
    const char *name;
    t_console_cmd_handler *handler;
    const char *help;
    BOOLEAN is_job;
} t_cmd;


const t_cmd console_cmd_list[];

void process_cmd(char *p, BOOLEAN is_job);


static void job_handler(void *param)
{
    char *job_cmd = (char*)param;

    info("job starting (%s)", job_cmd);

    process_cmd(job_cmd, TRUE);

    info("job terminating");

    free(job_cmd);
}

static int create_job(char *cmd)
{
    pthread_t thread_id;
    char *job_cmd;

    job_cmd = malloc(strlen(cmd)+1); /* freed in job handler */
    strcpy(job_cmd, cmd);

    if (pthread_create(&thread_id, NULL,
                       (void*)job_handler, (void*)job_cmd)!=0)
      perror("pthread_create");

    return 0;
}


void do_help(char *p)
{
    int i = 0;

    while (console_cmd_list[i].name != NULL)
    {
        RemoteLogMsg("\"%s\" - %s\n", (char*)console_cmd_list[i].name, (char*)console_cmd_list[i].help);
        i++;
    }
}


/************************************************************
 **
 **  MAIN CONSOLE COMMAND TABLE
 **
 **
 **/

extern const tBTTRC_FUNC_MAP bttrc_set_level_map[];

void do_set_trace(char *p)
{
    char id_str[32];
    int trace_level;
    int dump_levels;
    int index = 0;

    get_str(&p, id_str);
    trace_level = get_int(&p, -1);
    dump_levels = get_int(&p, -1);

    if (strncmp(id_str, "get", 3)==0)
        dump_levels = 2;

    RemoteLogMsg("\n");

    while (bttrc_set_level_map[index].layer_id_start)
    {
        if (bttrc_set_level_map[index].p_f == NULL)
        {
            index++;
            continue;
        }

        if (dump_levels == 2)
        {
            RemoteLogMsg("\tid [%s] level %d\n", bttrc_set_level_map[index].trc_name, bttrc_set_level_map[index].p_f(0xff));
        }
        else if (strncmp(id_str, bttrc_set_level_map[index].trc_name, strlen(bttrc_set_level_map[index].trc_name)) == 0)
        {
            if (dump_levels == 1)
                trace_level = bttrc_set_level_map[index].p_f(0xff);
            else
            trace_level = bttrc_set_level_map[index].p_f(trace_level);

            RemoteLogMsg("current trace level : id [%s] level %d\n", id_str, trace_level);
            return;
        }
        index++;
    }

    if (dump_levels == -1)
        RemoteLogMsg("unknown trace id %s\n", id_str);
}


void do_bts_dump(char *p)
{
    void bts_set_dump_interval(int interval_ms);

    int dump_interval_ms;

    dump_interval_ms = get_int(&p, -1);

    bts_set_dump_interval(dump_interval_ms);
}



void do_bts_tune_rfc_tx_mtu(char *p)
{
    extern void rfc_tune_txsize(int size);
    //int handle;
    int tx_mtu;
    //handle = get_int(&p, 0);
    tx_mtu = get_int(&p, 0);

    if (tx_mtu)
    {
        rfc_tune_txsize(tx_mtu);
    }
}


void do_example(char *p)
{
    bdaddr_t bd;
    int scn;
    char bd_str[20];

    scn = get_int(&p, -1);
    get_str(&p, bd_str);
    get_bdaddr(bd_str, &bd);

    RemoteLogMsg("example : scn %d bd %s\n", scn, bd_str);
}

const t_cmd console_cmd_list[] =
{
    { "help", do_help, "lists all available console commands", FALSE },
    { "settrc", do_set_trace, "<id str or getall> <level> <get (optional)>", FALSE },
    { "btsdump", do_bts_dump, "<interval ms>", FALSE },
    { "rfcsettxmtu", do_bts_tune_rfc_tx_mtu, "<data handle> <size>", FALSE },
    { "example", do_example, "<scn> <bd xx:yy:... >", FALSE },
    {NULL, NULL, ""},
};



void process_cmd(char *p, BOOLEAN is_job)
{
    char cmd[64];
    int i = 0;
    char *p_saved = p;

    get_str(&p, cmd);

    debug("cmd %s", cmd);

    /* table commands */
    while (console_cmd_list[i].name != NULL)
    {
        if (is_cmd(console_cmd_list[i].name))
        {
            if (!is_job && console_cmd_list[i].is_job)
                create_job(p_saved);
            else
                console_cmd_list[i].handler(p);
            return;
        }
        i++;
    }
}

#if !defined(HAVE_READLINE)

static char *local_getline(char *zPrompt, FILE *in){
  char *zLine;
  int nLine;
  int n;
  int eol;

  if( zPrompt && *zPrompt ){
    printf("%s",zPrompt);
    fflush(stdout);
  }
  nLine = 100;
  zLine = malloc( nLine );
  if( zLine==0 ) return 0;
  n = 0;
  eol = 0;
  while( !eol ){
    if( n+100>nLine ){
      nLine = nLine*2 + 100;
      zLine = realloc(zLine, nLine);
      if( zLine==0 ) return 0;
    }
    if( fgets(&zLine[n], nLine - n, in)==0 ){
      if( n==0 ){
        free(zLine);
        return 0;
      }
      zLine[n] = 0;
      eol = 1;
      break;
    }
    while( zLine[n] ){ n++; }
    if( n>0 && zLine[n-1]=='\n' ){
      n--;
      zLine[n] = 0;
      eol = 1;
    }
  }
  zLine = realloc( zLine, n+1 );
  return zLine;
}
#endif

#if defined(HAVE_READLINE) && HAVE_READLINE==1
#include <readline/readline.h>
#include <readline/history.h>
#else
#define readline(p) local_getline(p,stdin)
#define add_history(X)
#define read_history(X)
#define write_history(X)
#endif

#define HISTORY_FILE ".history"

void btlifs_parsecmd(char *cmd)
{
    read_history(HISTORY_FILE);

    printf("\ntype 'help' for list of commands\n\n");

    while (1)
    {
        char *line = (char *)readline("> ");

        add_history(line);

        /* write for each new command */
        write_history(HISTORY_FILE);

        if (line[0]==0)
           continue;

        process_cmd(line, FALSE);
  }
}



/*************************************************************/

tCTRL_HANDLE console_fd = CTRL_SOCKET_INVALID;

void console_server_cb(tCTRL_HANDLE handle, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params)
{
    info("console_server_cb %d\n", id);
    switch(id)
    {
        case BTLIF_DBG_CONSOLE_CMD:
            /* pass into dbg console */
            process_cmd(params->dbg_console_param.buffer, FALSE);
            break;
    }
}

/* remote control */
void btl_ifs_console_server_start(void)
{
    printf("Starting debug console server\n");

    BTL_IF_ServerInit();
    BTL_IF_RegisterSubSystem(&console_hdl, SUB_DBG, NULL, console_server_cb);

#ifndef BTL_IF_TARGET_CONSOLE
    /* tmp */
    while (1)
        sleep(1);
#endif
}

#ifndef BTL_IF_TARGET_CONSOLE

int main(int argc, char** argv)
{
    char *cmd, *arg;

    if (argc<2)
    {
        info("No option provided <wait|init");
        exit(0);
    }

    cmd = argv[1];
    arg = argv[2];

    if (!strcmp(cmd, "wait"))
    {
       server_test_waitconnection();
    }
//    else if (!strcmp(cmd, "init"))
//    {
//        server_test_initiate_conn();
//    }
    else if (!strcmp(cmd, "scowait"))
    {
        sco_wait();
    }

    else if (!strcmp(cmd, "waitall"))
    {
        all_wait();
    }
    else if (!strcmp(cmd, "waitdata"))
    {
        av_data_wait();
    }
    else if (!strcmp(cmd, "console"))
    {
        btl_ifs_console_server_start();
    }
    return 0;
}

#endif

#endif
