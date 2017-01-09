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
#ifndef BTL_IFS_H
#define BTL_IFS_H

#include "btl_if.h"

extern const char *sub2str[];

/* misc */
void btl_if_data_rx(tBTL_IF_SUBSYSTEM sub, int fd, char *p, int len);
void btl_if_notify_local_event(int fdc, tSUB sub, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params);
void btl_if_notify_rx_buf_pending(tDATA_HANDLE handle);
const char* dump_msg_id(tBTLIF_CTRL_MSG_ID id);

void dtun_server_start(void);
void dtun_server_stop(void);


/*******************************************************************************
**
** Function          BTL_IF_RegisterSubSystem
**
** Description      Register sub system datapath handler in local server.
**
**
** Returns           tBTL_SockFd
**
*******************************************************************************/

tBTL_IF_Result BTL_IF_RegisterSubSystem(tCTRL_HANDLE *handle, tBTL_IF_SUBSYSTEM sub,
                                               tBTL_IF_DATA_CALLBACK data_cb, tBTL_IF_CTRL_CALLBACK ctrl_cb);


/*******************************************************************************
**
** Function          BTL_IF_RegisterSubSystemMultiClnt
**
** Description      Register sub system datapath handler in local server.
**
**
** Returns           tBTL_IF_Result
**
*******************************************************************************/

tBTL_IF_Result BTL_IF_RegisterSubSystemMultiClnt(tBTL_IF_SUBSYSTEM sub, tBTL_IF_DATA_CALLBACK data_cb, tBTL_IF_CTRL_CALLBACK ctrl_cb);

/*******************************************************************************
**
** Function          BTL_IF_UnregisterSubSystem
**
** Description       Register sub system datapath handler in local server.
**
**
** Returns           tBTL_IF_Result
**
*******************************************************************************/

tBTL_IF_Result BTL_IF_UnregisterSubSystem(tBTL_IF_SUBSYSTEM sub);

/*******************************************************************************
**
** Function          BTL_IF_ConnectDatapath
**
** Description
**
**
** Returns           tBTL_IF_Result
**
*******************************************************************************/

tBTL_IF_Result BTL_IF_ConnectDatapath(tCTRL_HANDLE ctrl_hdl, tDATA_HANDLE* handle, tBTL_IF_SUBSYSTEM sub, int sub_port);

/*******************************************************************************
**
** Function          BTL_IF_AttachExtFd
**
** Description
**
**
** Returns           tBTL_IF_Result
**
*******************************************************************************/

tBTL_IF_Result BTL_IF_AttachExtFd(tCTRL_HANDLE ctrl_hdl, tDATA_HANDLE ext_fd, tSUB sub, int sub_port);



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

tBTL_IF_Result BTL_IF_WaitForDataChan(tDATA_HANDLE *handle, tCTRL_HANDLE ctrl_fd, tBTL_IF_SUBSYSTEM sub, int sub_port);


/*******************************************************************************
**
** Function          BTL_IF_SetupRxBuf
**
** Description     Setup dedicate receive buffer
**
**
** Returns          tBTL_IF_Result
**
*******************************************************************************/

tBTL_IF_Result BTL_IF_SetupRxBuf(tDATA_HANDLE handle, char *p_rx, int size);

/*******************************************************************************
**
** Function          BTL_IF_GetRxBuf
**
** Description     Get dedicate receive buffer
**
**
** Returns          dedicate receive buffer
**
*******************************************************************************/

char* BTL_IF_GetRxBuf(tDATA_HANDLE handle);

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

tBTL_IF_Result BTL_IF_SetupRxFlow(tDATA_HANDLE handle, BOOLEAN flow_on);


/*******************************************************************************
**
** Function           BTL_IF_SetupListener
**
** Description      Non blocking wait.
**                       Data connect ind will be notified through ctrl callback
**
** Returns            tBTL_IF_Result
**
*******************************************************************************/

tBTL_IF_Result BTL_IF_SetupListener(tCTRL_HANDLE ctrl_hdl, tBTL_IF_SUBSYSTEM sub, int sub_port);

/*******************************************************************************
**
** Function           BTL_IF_CancelListener
**
** Description
**
** Returns            tBTL_IF_Result
**
*******************************************************************************/

tBTL_IF_Result BTL_IF_CancelListener(tCTRL_HANDLE ctrl_hdl, tBTL_IF_SUBSYSTEM sub, int sub_port);

/*******************************************************************************
**
** Function          BTL_IF_SendData
**
** Description      Sends data to BTL
**
**
** Returns          tBTL_IF_Result
**
*******************************************************************************/

tBTL_IF_Result BTL_IF_SendData(tDATA_HANDLE handle, char *p, int len);

/*******************************************************************************
**
** Function          BTL_IF_CtrlSend
**
** Description     Send control msg
**
**
** Returns          tBTL_IF_Result
**
*******************************************************************************/

tBTL_IF_Result BTL_IF_CtrlSend(tCTRL_HANDLE handle, tSUB sub,
                                   tBTLIF_CTRL_MSG_ID msg_id, tBTL_PARAMS *params, int param_len);


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

tBTL_IF_Result BTL_IF_SendMsgNoParams(tCTRL_HANDLE handle, tSUB sub, tBTLIF_CTRL_MSG_ID msg_id);


/*******************************************************************************
**
** Function          BTL_IF_DisconnectDatapath
**
** Description
**
**
** Returns           tBTL_IF_Result
**
*******************************************************************************/

tBTL_IF_Result BTL_IF_DisconnectDatapath(tDATA_HANDLE handle);

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

tBTL_IF_Result BTL_IF_ServerInit(void);

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

tBTL_IF_Result BTL_IF_ServerShutdown(void);


/*******************************************************************************
**     Common CTRL API
**/

int BTLIF_ConnectRsp(tCTRL_HANDLE handle, tSUB sub, tBTL_IF_Result result, tSTATUS status);


/*******************************************************************************
**     AG API
**/

int BTLIF_AG_ConnectReq(tCTRL_HANDLE handle, BD_ADDR *p_bd, unsigned short rf_chan);
int BTLIF_AG_ConnectRsp(tCTRL_HANDLE handle, tBTL_IF_Result result, tSTATUS status);
int BTLIF_AG_ConnectInd(tCTRL_HANDLE handle, BD_ADDR *p_bd, unsigned short rf_chan);
int BTLIF_AG_DisconnectInd(tCTRL_HANDLE handle, unsigned short rf_chan);
int BTLIF_AG_DisconnectRsp(tCTRL_HANDLE handle, unsigned short rf_chan);


#endif

