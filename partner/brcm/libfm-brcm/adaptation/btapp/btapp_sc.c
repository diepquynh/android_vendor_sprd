/****************************************************************************
**
**  Name:          btapp_sc.c
**
**  Description:    Contains application functions for SIM Access server
**
**
**  Copyright (c) 2003, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include "bt_target.h"

#if (defined BTA_SC_INCLUDED) && (BTA_SC_INCLUDED == TRUE)

#include "bta_api.h"
#include "btui.h"
#include "btui_int.h"
#include "bta_sc_co.h"
#include "bta_sc_ci.h"
#include "btapp_sc.h"
#include <string.h>


#if (defined LOG_TAG)
#undef LOG_TAG
#endif
#define LOG_TAG "BTAPP_SC:"

#ifndef LINUX_NATIVE
#include <cutils/log.h>
#else
#include <stdio.h>
#define LOGI(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGE(format, ...)  fprintf (stderr, LOG_TAG format"\n", ## __VA_ARGS__)
#endif


/*****************************************************************************
**  Constants
*****************************************************************************/
#define BTAPP_SC_READER_ID     3    /* Card Reader identifier */
#define BTAPP_SC_READER_FLAGS  0    /* Card Reader flags 0=Not removable */
#define BTAPP_SC_MSGMIN        32   /* Min msg size for accessing SIM card */
#define BTAPP_SC_MSGMAX        300  /* Max msg size for accessing SIM card */


/*****************************************************************************
**  Local Function prototypes
*****************************************************************************/
static void btapp_sc_on_rx_ctrl(tCTRL_HANDLE fd, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params);


/* Test APDU command: REFRESH */
static const UINT8 apdu_cmd_refresh[] = {
    /* APDU command header */
    0xD0,   /* Proactive SIM command tag*/
    0x09,   /* LEN */
    0x01,   /* Command Detail Tag*/
    0x03,   /* Command Detail Len */
    0x02,   /* Command Detail Number */
    0x01,   /* Command Detail Type Refresh*/
    0x00,   /* Command Detail Command Qualifier */
    0x02,   /* Device Identify Tag */
    0x02,   /* Device Identify Len */
    0x81,   /* Source Device Identify SIM */
    0x82    /* Destination Device Identify ME */
};
#define APDU_CMD_REFRESH_LEN   (sizeof(apdu_cmd_refresh))


/* BTAPP SC Control block variable */
tBTAPP_SC_CB  btapp_sc_cb;


/*******************************************************************************
**
** Function         btapp_sc_init
**
** Description      Initializes SIM access server
**
**
** Returns          void
**
*******************************************************************************/
void btapp_sc_init(void)
{
    tBTL_IF_Result result;

    memset(&btapp_sc_cb, 0, sizeof(tBTAPP_SC_CB));

    BTL_IF_ServerInit();

    result = BTL_IF_RegisterSubSystem(&btapp_sc_cb.btl_if_handle, SUB_SAPS, NULL, btapp_sc_on_rx_ctrl);

    LOGI("%s: result = %d, btl_if_handle = %d", __FUNCTION__, result, btapp_sc_cb.btl_if_handle);
}


/*******************************************************************************
**
** Function         btapp_sc_close
**
** Description      Closes connection
**
**
** Returns          void
**
*******************************************************************************/
void btapp_sc_close(tBTA_SC_DISCONNECT_TYPE type)
{
    LOGI("%s: sap_state = %d", __FUNCTION__, btapp_sc_cb.sap_state);

    if (btapp_sc_cb.sap_state != BTAPP_SC_STATE_IDLE) {
        BTA_ScClose(type);
    }
}


/*******************************************************************************
**
** Function         btapp_sc_disable
**
** Description      Disable SIM access server
**
**
** Returns          void
**
*******************************************************************************/
void btapp_sc_disable(void)
{
    LOGI("%s: sap_state = %d", __FUNCTION__, btapp_sc_cb.sap_state);

    if (btapp_sc_cb.enabled) {
        BTA_ScDisable();
        btapp_sc_cb.enabled = FALSE;
    }
}


/*******************************************************************************
**
** Function         btapp_sc_refresh
**
** Description      Send Refresh GSM APDU to client
**
**
** Returns          void
**
*******************************************************************************/
void btapp_sc_refresh(void)
{
    LOGI("%s: sap_state = %d", __FUNCTION__, btapp_sc_cb.sap_state);

    bta_sc_ci_apdu(BTA_SC_REQUEST_APDU, (UINT8 *) apdu_cmd_refresh, APDU_CMD_REFRESH_LEN);
}


/*******************************************************************************
**
** Function         btapp_sc_card_reset
**
** Description
**
**
** Returns          void
**
*******************************************************************************/
void btapp_sc_card_reset(void)
{
    LOGI("%s: sap_state = %d", __FUNCTION__, btapp_sc_cb.sap_state);

    BTA_ScCardStatus(BTA_SC_CARD_RESET);
    btapp_sc_cb.card_status = BTAPP_SC_CARD_READY;
}


/*******************************************************************************
**
** Function         btapp_sc_card_remove
**
** Description
**
**
** Returns          void
**
*******************************************************************************/
void btapp_sc_card_remove(void)
{
    LOGI("%s: sap_state = %d", __FUNCTION__, btapp_sc_cb.sap_state);

    BTA_ScCardStatus(BTA_SC_CARD_REMOVED);
    btapp_sc_cb.card_status = BTAPP_SC_CARD_REMOVED;
}


/*******************************************************************************
**
** Function         btapp_sc_card_insert
**
** Description
**
**
** Returns          void
**
*******************************************************************************/
void btapp_sc_card_insert(void)
{
    LOGI("%s: sap_state = %d", __FUNCTION__, btapp_sc_cb.sap_state);

    BTA_ScCardStatus(BTA_SC_CARD_INSERTED);
    btapp_sc_cb.card_status = BTAPP_SC_CARD_INSERTED;
}


/*******************************************************************************
**
** Function         btapp_sc_call_state
**
** Description      Notification of call state change
**
** Returns          void
**
*******************************************************************************/
void btapp_sc_call_state(BOOLEAN is_active)
{
    LOGI("%s: is_active = %s", __FUNCTION__, (is_active ? "true" : "false"));

    /* Store call state */
    btapp_sc_cb.call_in_progress = is_active;

    /* If call is terminated, and there is a SIM reset pending,
     * i.e. SAP client is waiting to take control.
     */
    if ((!is_active) && (btapp_sc_cb.sap_state = BTAPP_SC_STATE_RESET_PENDING)) {
        /* Platfrom should reset SIM now */

        /* Send new SIM status to client (SIM card successfully reset) */
        btapp_sc_card_reset();
        btapp_sc_cb.sap_state = BTAPP_SC_STATE_READY;
    }
}


/*******************************************************************************
**
** Function         btapp_sc_cback
**
** Description      The callback function for the BTA stack.
**
**
** Returns          void
**
*******************************************************************************/
static void btapp_sc_cback(tBTA_SC_EVT event, tBTA_SC *p_data)
{
    tBTL_PARAMS params;

    switch (event) {
    case BTA_SC_ENABLE_EVT:
        LOGI("%s: event = BTA_SC_ENABLE_EVT", __FUNCTION__);
        BTL_IF_CtrlSend(btapp_sc_cb.btl_if_handle, SUB_SAPS,  BTLIF_SAPS_ENABLE_EVT, NULL, 0);
        break;

    case BTA_SC_DISABLE_EVT:
        LOGI("%s: event = BTA_SC_DISABLE_EVT", __FUNCTION__);
        BTL_IF_CtrlSend(btapp_sc_cb.btl_if_handle, SUB_SAPS,  BTLIF_SAPS_DISABLE_EVT,
                        NULL, 0);
        break;

    case BTA_SC_OPEN_EVT:
        LOGI("%s: event = BTA_SC_OPEN_EVT, bd_addr = [%02X:%02X:%02X:%02X:%02X:%02X]",
             __FUNCTION__,
             p_data->open.bd_addr[0], p_data->open.bd_addr[1], p_data->open.bd_addr[2],
             p_data->open.bd_addr[3], p_data->open.bd_addr[4], p_data->open.bd_addr[5]);
        btapp_sc_cb.sap_state = BTAPP_SC_STATE_OPEN;
        memcpy(params.saps_open_evt_param.bd_addr, p_data->open.bd_addr, BD_ADDR_LEN);
        BTL_IF_CtrlSend(btapp_sc_cb.btl_if_handle, SUB_SAPS,  BTLIF_SAPS_OPEN_EVT,
                        &params, sizeof(tBTLIF_SAPS_OPEN_EVT_PARAM));
        break;

    case BTA_SC_CLOSE_EVT:
        LOGI("%s: event = BTA_SC_CLOSE_EVT", __FUNCTION__);
        btapp_sc_cb.sap_state = BTAPP_SC_STATE_IDLE;
        BTL_IF_CtrlSend(btapp_sc_cb.btl_if_handle, SUB_SAPS,  BTLIF_SAPS_CLOSE_EVT, NULL, 0 );
        break;

    default:
        LOGI("%s: Oops, unknown event %d", __FUNCTION__, event);
        break;
    }
}


/*******************************************************************************
**
** Function         btapp_sc_enable
**
** Description      Enable SAP.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_sc_enable(void)
{
    LOGI("%s: sap_state = %d", __FUNCTION__, btapp_sc_cb.sap_state);

    /* Sample app always allows PUT file operations unless changed in test menu */
    if (!btapp_sc_cb.enabled) {
        tCTRL_HANDLE handle;

        handle = btapp_sc_cb.btl_if_handle;
        memset(&btapp_sc_cb, 0, sizeof(tBTAPP_SC_CB));
        btapp_sc_cb.btl_if_handle = handle;

        btapp_sc_cb.card_status = BTAPP_SC_CARD_READY;
        btapp_sc_cb.call_in_progress = FALSE;
        btapp_sc_cb.sap_state = BTAPP_SC_STATE_IDLE;

        //btui_cfg.sc_security = BTA_SEC_NONE;
        btui_cfg.sc_security = BTUI_SC_SECURITY;
        strncpy(btui_cfg.sc_service_name, BTUI_SC_SERVICE_NAME, sizeof(btui_cfg.sc_service_name) - 1);
        btui_cfg.sc_service_name[sizeof(btui_cfg.sc_service_name) - 1] = 0;
        btui_cfg.p_sc_menu = NULL;

        BTA_ScEnable(btui_cfg.sc_security,
                     btui_cfg.sc_service_name,
                     BTAPP_SC_READER_ID, BTAPP_SC_READER_FLAGS,
                     BTAPP_SC_MSGMIN, BTAPP_SC_MSGMAX,
                     btapp_sc_cback);

        btapp_sc_cb.enabled = TRUE;

        LOGI("SIM ACCESS SERVER enabled, service name:%s", btui_cfg.sc_service_name);
    }
}


/*******************************************************************************
**
** Function         btapp_sc_on_rx_ctrl
**
** Description      BTL-IF event handler.
**
**
** Returns          void
**
*******************************************************************************/
static void btapp_sc_on_rx_ctrl(tCTRL_HANDLE fd, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params)
{
    switch (id) {
    case BTLIF_SAPS_ENABLE:
        /* Make it simple now, later on we should get the parameters from java application,
         * then call BTA_PbsEnable(). Or it may never need to do that.
         */
        LOGI("%s: id = BTLIF_SAPS_ENABLE", __FUNCTION__);
        btapp_sc_enable();
        break;

    case BTLIF_SAPS_DISABLE:
        LOGI("%s: id = BTLIF_SAPS_DISABLE", __FUNCTION__);
        btapp_sc_disable();
        break;

    case BTLIF_SAPS_DISCONNECT:
        LOGI("%s: id = BTLIF_SAPS_DISCONNECT, type = %d",
             __FUNCTION__, params->result.result);
        btapp_sc_close((params->result.result == BTLIF_SAPS_DISCONNECT_TYPE_GRACEFUL) ?
                       (BTA_SC_DISC_GRACEFUL) : (BTA_SC_DISC_IMMEDIATE));
        break;

    case BTLIF_SAPS_STATUS:
        switch (params->result.result) {
        case BTLIF_SAPS_STATUS_UNKNOWN:
            LOGI("%s: SAP status UNKNOWN. Do nothing.", __FUNCTION__);
            break;
        case BTLIF_SAPS_STATUS_NO_SIM:
            LOGI("%s: SAP status NO_SIM. Disconnect gracefully.", __FUNCTION__);
            btapp_sc_close(BTA_SC_DISC_GRACEFUL);
            break;
        case BTLIF_SAPS_STATUS_NOT_READY:
            LOGI("%s: SAP status NOT_READY. Disconnect gracefully.", __FUNCTION__);
            btapp_sc_close(BTA_SC_DISC_GRACEFUL);
            break;
        case BTLIF_SAPS_STATUS_READY:
            LOGI("%s: SAP status READY. Nothing to do.", __FUNCTION__);
            break;
        case BTLIF_SAPS_STATUS_CONNECTED:
            LOGI("%s: SAP status CONNECTED. Nothing to do.", __FUNCTION__);
            break;
        }
        break;

    case BTLIF_SAPS_CARD_STATUS:
        switch (params->result.result) {
        case BTLIF_SAPS_CARD_STATUS_UNKNOWN:
            LOGI("%s: SAP card status UNKNOWN. Do nothing.", __FUNCTION__);
            break;
        case BTLIF_SAPS_CARD_STATUS_RESET:
            LOGI("%s: SAP card status RESET. TBD", __FUNCTION__);
            btapp_sc_card_reset();
            break;
        case BTLIF_SAPS_CARD_STATUS_NOT_ACCESSIBLE:
            LOGI("%s: SAP card status NOT_ACCESSIBLE. Treat as card removed.", __FUNCTION__);
            btapp_sc_card_remove();
            break;
        case BTLIF_SAPS_CARD_STATUS_REMOVED:
            LOGI("%s: SAP card status REMOVED.", __FUNCTION__);
            btapp_sc_card_remove();
            break;
        case BTLIF_SAPS_CARD_STATUS_INSERTED:
            LOGI("%s: SAP card status INSERTED.", __FUNCTION__);
            btapp_sc_card_insert();
            break;
        case BTLIF_SAPS_CARD_STATUS_RECOVERED:
            LOGI("%s: SAP card status RECOVERED. Treat as card inserted.", __FUNCTION__);
            btapp_sc_card_insert();
            break;
        }
        break;

    default:
#if (defined SAPS_USE_RIL) && (SAPS_USE_RIL == TRUE)
        btapp_sc_ril_on_rx_ctrl(id, params);
#else
        LOGI("%s: Oops, unknown id %d", __FUNCTION__, id);
#endif
        break;
    }
}

#endif
