/************************************************************************************
 *
 *  Copyright (C) 2009-2011 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ************************************************************************************/
#ifndef BTL_IFC_H
#define BTL_IFC_H

#include "btl_if.h"

extern const char *sub2str[];

/*******************************************************************************
**
** Function          BTL_IFC_RegisterSubSystem
**
** Description      Register subsystem
**
**
** Returns          tBTL_SockFd
**
*******************************************************************************/

tBTL_IF_Result BTL_IFC_RegisterSubSystem(tCTRL_HANDLE *handle, tBTL_IF_SUBSYSTEM sub,
                                                 tBTL_IF_DATA_CALLBACK data_cb, tBTL_IF_CTRL_CALLBACK ctrl_cb);

/*******************************************************************************
**
** Function          BTL_IFC_UnregisterSubSystem
**
** Description      Unregister subsystem
**
**
** Returns          tBTL_SockFd
**
*******************************************************************************/

tBTL_IF_Result BTL_IFC_UnregisterSubSystem(tCTRL_HANDLE *handle,
                                                tBTL_IF_SUBSYSTEM sub);

/*******************************************************************************
**
** Function          BTL_IFC_ConnectDatapath
**
** Description
**
**
** Returns          tBTL_SockFd
**
*******************************************************************************/

tBTL_IF_Result BTL_IFC_ConnectDatapath(tDATA_HANDLE* handle, tBTL_IF_SUBSYSTEM sub, int sub_port);

/*******************************************************************************
**
** Function           BTL_IFC_SetupListener
**
** Description      Notify remote side that a listener is available for incoming data connections.
**                       Data connect ind will be notified through ctrl callback
**                       Non blocking wait.
**
** Returns
**
*******************************************************************************/

tBTL_IF_Result BTL_IFC_SetupDatapathListener(tCTRL_HANDLE ctrl, tBTL_IF_SUBSYSTEM sub, int sub_port);

/*******************************************************************************
**
** Function          BTL_IFC_SetupRxBuf
**
** Description     Setup dedicate receive buffer
**
**
** Returns          tBTL_IF_Result
**
*******************************************************************************/

tBTL_IF_Result BTL_IFC_SetupRxBuf(tDATA_HANDLE handle, char *p_rx, int size);


/*******************************************************************************
**
** Function          BTL_IFC_RegisterDatapath
**
** Description      Sends data frame to BTL
**
**
** Returns          tBTL_IF_Result
**
*******************************************************************************/

tBTL_IF_Result BTL_IFC_SendData(tDATA_HANDLE handle, char *p, int len);

/*******************************************************************************
**
** Function          BTL_IFC_UnregisterDatapath
**
** Description      Disconnect datapath
**
**
** Returns          tBTL_IF_Result
**
*******************************************************************************/

tBTL_IF_Result BTL_IFC_DisconnectDatapath(tDATA_HANDLE handle);

/*******************************************************************************
**
** Function          BTL_IFC_CtrlSend
**
** Description     Send control msg
**
**
** Returns          tBTL_IF_DataPath
**
*******************************************************************************/

tBTL_IF_Result BTL_IFC_CtrlSend(int ctrl_fd, tSUB sub, tBTLIF_CTRL_MSG_ID msg_id, tBTL_PARAMS *params, int param_len);

/*******************************************************************************
**
** Function          BTL_IFC_SendMsgNoParams
**
** Description     Send control msg without any parameters
**
**
** Returns          tBTL_IF_DataPath
**
**
*******************************************************************************/

tBTL_IF_Result BTL_IFC_SendMsgNoParams(tCTRL_HANDLE handle, tSUB sub, tBTLIF_CTRL_MSG_ID msg_id);


/*******************************************************************************
**
** Function          BTL_IFC_ClientInit
**
** Description      Init client
**
**
** Returns          tBTL_IF_Result
**
*******************************************************************************/

tBTL_IF_Result BTL_IFC_ClientInit(void);

/*******************************************************************************
**
** Function          BTL_IFC_ClientShutdown
**
** Description      Shutdown client
**
**
** Returns          tBTL_IF_Result
**
*******************************************************************************/

tBTL_IF_Result BTL_IFC_ClientShutdown(void);


/*******************************************************************************
**
** Function          BTL_IFC_SetRemoteSrv
**
** Description      Setup alternate remote server
**
**
** Returns          tBTL_IF_Result
**
*******************************************************************************/

tBTL_IF_Result BTL_IFC_SetRemoteSrv(const char *ip_addr);

/*******************************************************************************
**
** Function          BTLIF_AG_ConnectIndAck
**
** Description      Setup alternate remote server
**
**
** Returns          tBTL_IF_Result
**
*******************************************************************************/

int BTLIF_AG_ConnectIndAck(tCTRL_HANDLE handle, BD_ADDR *p_bd, unsigned short rf_chan);


/*******************************************************************************
**     AG API
**/

int BTLIF_AG_ConnectReq(tCTRL_HANDLE handle, BD_ADDR *p_bd, unsigned short rf_chan);
int BTLIF_AG_ConnectRsp(tCTRL_HANDLE handle, tBTL_IF_Result result, tSTATUS status);
int BTLIF_AG_ListenReq(tCTRL_HANDLE handle, BD_ADDR *p_bd, unsigned short rf_chan);
int BTLIF_AG_Disconnect(tCTRL_HANDLE handle, unsigned short rf_chan);

BOOLEAN btl_ifc_main_running(void);
void btl_ifc_rxdata(tBTL_IF_SUBSYSTEM sub, tDATA_HANDLE handle, char *p, int len);
char* btl_ifc_get_srvaddr(void);
void btl_ifc_notify_local_event(tSUB sub, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params);
void btl_ifc_notify_rx_buf_pending(tDATA_HANDLE handle);
const char* dump_msg_id(tBTLIF_CTRL_MSG_ID id);

#endif


