/*****************************************************************************
**
**  Name:             btapp_dg.h
**
**  Description:     This file contains btui internal interface
**				     definition
**
**  Copyright (c) 2000-2004, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#ifndef BTAPP_DG_H
#define BTAPP_DG_H

#include "btl_if.h"

#define BTPORT_NAME_DUN "/dev/BtPort0"
#define TTY_NAME_DUN "/dev/ttySA0"

#define BTPORT_NAME_SPP "/dev/BtPort1"
#define TTY_NAME_SPP "/dev/ttySA1"

#define BTUI_DG_ID_SPP          0
#define BTUI_DG_ID_DUN          1
#define BTUI_DG_ID_FAX          2
#define BTUI_DG_ID_CLIENT       3

/* DG ID allocations */

/* DG SPP */
#define BTUI_DG_SPP_NUM_SERVICES (5)
#define BTUI_DG_SPP_APPID_BEGIN  (0)
#define BTUI_DG_SPP_APPID_END    (BTUI_DG_SPP_APPID_BEGIN+BTUI_DG_SPP_NUM_SERVICES-1)

/* DG DUN */
#define BTUI_DG_DUN_NUM_SERVICES (3)
#define BTUI_DG_DUN_APPID_BEGIN  (BTUI_DG_SPP_APPID_END+1)

#define BTUI_DG_DUN_APPID_END    (BTUI_DG_DUN_APPID_BEGIN+BTUI_DG_DUN_NUM_SERVICES-1)

/* DG FAX */
#define BTUI_DG_FAX_NUM_SERVICES (0)
#define BTUI_DG_FAX_APPID_BEGIN  (BTUI_DG_DUN_APPID_END+1)

#define BTUI_DG_FAX_APPID_END    (BTUI_DG_FAX_APPID_BEGIN+BTUI_DG_FAX_NUM_SERVICES-1)

/* DG CLIENT */
#define BTUI_DG_CLNT_NUM_SERVICES (0)
#define BTUI_DG_CLNT_APPID_BEGIN  (BTUI_DG_FAX_APPID_END+1)

#define BTUI_DG_CLNT_APPID_END    (BTUI_DG_CLNT_APPID_BEGIN+BTUI_DG_CLNT_NUM_SERVICES-1)

#define BTUI_DG_NUM_SERVICES      (BTUI_DG_SPP_NUM_SERVICES+BTUI_DG_DUN_NUM_SERVICES + \
                                   BTUI_DG_FAX_NUM_SERVICES + BTUI_DG_CLNT_NUM_SERVICES )

#define BTUI_DG_DATA_TEST_TOUT  2

/* required to work arround some emulator limitations. should not be needed for real platform */
#ifndef BTAPP_DG_EMULATOR
#define BTAPP_DG_EMULATOR FALSE
#endif

typedef struct
{
    BUFFER_Q        data_q;
#if( defined DUN_USE_BTPORT ) && (DUN_USE_BTPORT == TRUE)
    int             tx_data_enable; /* flow control variable for bta_dg_co_tx_path() */
#if (TRUE == BTAPP_DG_EMULATOR)
    int             at_cmd_mode;
#endif
#endif
    UINT16          port_handle;
    UINT16          listen_handle;
    UINT16          rx_buf_len;
    UINT16          peer_mtu;
#if( defined DUN_USE_BTPORT ) && (DUN_USE_BTPORT == TRUE)
    UINT8           modem_ctrl;     /* current state of modem control signals */
#endif
    INT32           data_handle;
    tBTA_SERVICE_ID service_id;
} tBTUI_DG_APP_CB;

/* typedef for BTUI DG control block */
typedef struct
{
    tBTUI_DG_APP_CB app_cb[BTUI_DG_NUM_SERVICES];
} tBTUI_DG_CB;

extern tBTUI_DG_CB btui_dg_cb;

extern void btapp_dg_close_connection(UINT8 conn_index);
extern void btapp_dg_connect(void);
extern void btapp_dg_init(void);
extern void btapp_dg_disable(void);
extern void btapp_dg_set_device_authorized (tBTUI_REM_DEVICE * p_device_rec);
extern void btapp_dg_dun_rx_data(int appid, BT_HDR *p_buf);
extern void btapp_dg_dun_tx_dequeued(int appid, BT_HDR *p_buf);
extern void btapp_dg_spp_tx_dequeued(int appid, BT_HDR *p_buf, int qcount);
extern void btapp_dg_spp_tx_data(UINT8 appid, BT_HDR *p_buf);

int BTPORT_Init(void);
int BTPORT_IsEnabled(void);
int BTPORT_StartRedirection(tCTRL_HANDLE btlif_ctrl, tBTA_SERVICE_ID service, int sub_port, UINT16 port_handle);
int BTPORT_StopRedirection(tBTA_SERVICE_ID service, INT32 fd);

#if( defined DUN_USE_BTPORT ) && (DUN_USE_BTPORT == TRUE)
extern void btapp_dg_btlif_send_modem_ctrl( const UINT8 app_id, const UINT8 signals, const UINT8 values );
#endif

#if( defined DUN_USE_SMD_INTERFACE ) && (DUN_USE_SMD_INTERFACE == TRUE)
void btapp_dg_set_ril_flow_ctrl(int signals, int mask);
void smd_handle_ril_flow_ctrl_req(int app_id, int signals, int mask);
#endif /* #if( defined DUN_USE_SMD_INTERFACE ) && (DUN_USE_SMD_INTERFACE == TRUE) */

#endif
