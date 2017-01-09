/*****************************************************************************
**
**  Name:           bta_sc_co.c
**
**  Description:    This file contains the SIM Access server call-out
**                  function implementation for BTAPP.
**
**  Copyright (c) 2003-2004, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include "bt_target.h"

#if (defined BTA_SC_INCLUDED) && (BTA_SC_INCLUDED == TRUE)

#include "bta_api.h"
#include "btui.h"
#include "btui_int.h"
#include "bta_sc_co.h"
#include "bta_sc_ci.h"
#include "btapp_sc.h"


#if (defined LOG_TAG)
#undef LOG_TAG
#endif
#define LOG_TAG "BTA_SC_CO:"

#ifndef LINUX_NATIVE
#include <cutils/log.h>
#else
#include <stdio.h>
#define LOGI(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGE(format, ...)  fprintf (stderr, LOG_TAG format"\n", ## __VA_ARGS__)
#endif


#if (!defined SAPS_USE_RIL) || (SAPS_USE_RIL != TRUE)
/*******************************************************************************
** Constants and Definitions
*******************************************************************************/

/* ATR string. Used for response to TRANSFER_ATR_REQ, and contains infomation  */
/* about the SIM card's operating characteristics (see GSM 11.11).             */
/* Since there is no actual SIM hardware, a minimal ATR string will be used.   */
static const UINT8 btapp_sc_atr[] = {
    0x01,   /* Card tag */
    0x03,   /* Number of ATR bytes that follow */
    0x3F,   /* TS Initial character */
    0x00,   /* T0 Format character (TA1,TB1,TC1,TD1 not included, and no historic chars) */
    0xC0    /* TCK (check byte) */
};


/* Structure definition for APDU command lookup table */
typedef struct
{
    UINT8 ins_id;           /* Instruction ID */
    char  *p_ins_name;      /* Instruction name */
} tBTAPP_SC_APDU_INS;

/* Lookup table to convert APDU instruction id to name */
const tBTAPP_SC_APDU_INS btapp_sc_apdu_ins_table[] =
{
    {0xA4, "SELECT"},
    {0xF2, "STATUS"},
    {0xB0, "READ BINARY"},
    {0xD6, "UPDATE BINARY"},
    {0xB2, "READ RECORD"},
    {0xDC, "UPDATE RECORD"},
    {0xA2, "SEEK"},
    {0x32, "INCREASE"},
    {0x20, "VERIFY CHV"},
    {0x24, "CHANGE CHV"},
    {0x26, "DISABLE CHV"},
    {0x28, "ENABLE CHV"},
    {0x2C, "UNBLOCK CHV"},
    {0x04, "INVALIDATE"},
    {0x44, "REHABILITATE"},
    {0x88, "RUN GSM ALGORITHM"},
    {0xFA, "SLEEP"},
    {0xC0, "GET RESPONSE"}
};
#define BTAPP_SC_APDU_INS_TABLE_SIZE  (sizeof(btapp_sc_apdu_ins_table)/sizeof(tBTAPP_SC_APDU_INS))


/* Definitions for decoding APDU commands */
#define BTA_APDU_CMD_OFFSET_CLA     0       /* Offset of APDU command class byte */
#define BTA_APDU_CMD_OFFSET_INS     1       /* Offset of APDU command instruction byte */

#define BTA_APDU_CMD_CLASS_GSM      0xA0    /* APDU command class for GSM */
#endif


#if (defined SAPS_USE_RIL) && (SAPS_USE_RIL == TRUE)
/*******************************************************************************
**
** Function         btapp_sc_ril_on_rx_ctrl
**
** Description      BTL-IF SAPS/RIL event handler.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_sc_ril_on_rx_ctrl(tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params)
{
    tBTA_SC_RESULT result;

    switch (id) {
    case BTLIF_SAPS_RIL_SIM_ON:
        LOGI("%s: id = BTLIF_SAPS_RIL_SIM_ON, result_code = %d",
             __FUNCTION__, params->result.result);
        if (params->result.result == BTLIF_SAPS_RESULT_OK) {
            result = BTA_SC_RESULT_OK;
        }
        else {
            result = BTA_SC_RESULT_ERROR;
        }
        bta_sc_ci_sim_on(result);
        break;

    case BTLIF_SAPS_RIL_SIM_OFF:
        LOGI("%s: id = BTLIF_SAPS_RIL_SIM_OFF, result_code = %d",
             __FUNCTION__, params->result.result);
        if (params->result.result == BTLIF_SAPS_RESULT_OK) {
            result = BTA_SC_RESULT_OK;
        }
        else {
            result = BTA_SC_RESULT_ERROR;
        }
        bta_sc_ci_sim_off(result);
        break;

    case BTLIF_SAPS_RIL_SIM_RESET:
        LOGI("%s: id = BTLIF_SAPS_RIL_SIM_RESET, result_code = %d",
             __FUNCTION__, params->result.result);
        if (params->result.result == BTLIF_SAPS_RESULT_OK) {
            result = BTA_SC_RESULT_OK;
        }
        else {
            result = BTA_SC_RESULT_ERROR;
        }
        bta_sc_ci_sim_reset(result);
        break;

    case BTLIF_SAPS_RIL_SIM_ATR:
        LOGI("%s: id = BTLIF_SAPS_RIL_SIM_ATR", __FUNCTION__);
        bta_sc_ci_atr(params->saps_ril_atr_param.result,
                      params->saps_ril_atr_param.atr,
                      params->saps_ril_atr_param.atrlen);
        break;

    case BTLIF_SAPS_RIL_SIM_APDU:
        LOGI("%s: id = BTLIF_SAPS_RIL_SIM_APDU", __FUNCTION__);
        bta_sc_ci_apdu(params->saps_ril_apdu_param.result,
                       params->saps_ril_apdu_param.apdu,
                       params->saps_ril_apdu_param.apdulen);
        break;

    default:
        LOGI("%s: Oops, unknown RIL id %d", __FUNCTION__, id);
        break;
    }
}
#endif


/*******************************************************************************
**
** Function         bta_sc_co_sim_open
**
** Description      Open the SIM card
**
** Parameters       none
**
** Returns          void
**
*******************************************************************************/
void bta_sc_co_sim_open()
{
    LOGI("%s:", __FUNCTION__);

#if (defined SAPS_USE_RIL) && (SAPS_USE_RIL == TRUE)
    BTL_IF_SendMsgNoParams(btapp_sc_cb.btl_if_handle, SUB_SAPS, BTLIF_SAPS_RIL_SIM_OPEN_EVT);
#endif
}


/*******************************************************************************
**
** Function         bta_sc_co_sim_close
**
** Description      Close the SIM card
**
** Parameters       none
**
** Returns          void
**
*******************************************************************************/
void bta_sc_co_sim_close()
{
    LOGI("%s:", __FUNCTION__);

#if (defined SAPS_USE_RIL) && (SAPS_USE_RIL == TRUE)
    BTL_IF_SendMsgNoParams(btapp_sc_cb.btl_if_handle, SUB_SAPS, BTLIF_SAPS_RIL_SIM_CLOSE_EVT);
#endif
}


/*******************************************************************************
**
** Function         bta_sc_co_sim_reset
**
** Description      Reset the SIM card.
**                  Since there is no actual SIM hardware, simply call the
**                  BTA_SC call-in with  result=BTA_SC_RESULT_OK.
**
** Parameters       none
**
** Returns          void
**
*******************************************************************************/
void bta_sc_co_sim_reset(void)
{
    LOGI("%s:", __FUNCTION__);

#if (defined SAPS_USE_RIL) && (SAPS_USE_RIL == TRUE)
    BTL_IF_SendMsgNoParams(btapp_sc_cb.btl_if_handle, SUB_SAPS, BTLIF_SAPS_RIL_SIM_RESET_EVT);
#else
    /* If SIM card has been removed, then return error */
    if (btapp_sc_cb.card_status == BTAPP_SC_CARD_REMOVED) {
        bta_sc_ci_sim_reset(BTA_SC_RESET_RESULT_ERROR);
    }
    else {
        btapp_sc_cb.card_status = BTAPP_SC_CARD_READY;

        /* If SAP client connection is not active yet, but there is already an ongoing call... */
        if ((btapp_sc_cb.sap_state == BTAPP_SC_STATE_OPEN) && (btapp_sc_cb.call_in_progress)) {
            /* Notify client that call is in progress, and will perform RESET
             * after call is released (and send STATUS_IND).
             */
            btapp_sc_cb.sap_state = BTAPP_SC_STATE_RESET_PENDING;
            bta_sc_ci_sim_reset(BTA_SC_RESET_RESULT_OK_ONGOING_CALL);
        }
        else {
            /* Card is reset, and ready for client to send commands */
            btapp_sc_cb.sap_state = BTAPP_SC_STATE_READY;
            bta_sc_ci_sim_reset(BTA_SC_RESET_RESULT_OK);
        }
    }
#endif
}


/*******************************************************************************
**
** Function         bta_sc_co_sim_on
**
** Description      Power on the SIM card.
**                  Since there is no actual SIM hardware, simply call the
**                  BTA_SC call-in with  result=BTA_SC_RESULT_OK.
**
** Parameters       none
**
** Returns          void
**
*******************************************************************************/
void bta_sc_co_sim_on(void)
{
    LOGI("%s:", __FUNCTION__);

#if (defined SAPS_USE_RIL) && (SAPS_USE_RIL == TRUE)
    BTL_IF_SendMsgNoParams(btapp_sc_cb.btl_if_handle, SUB_SAPS, BTLIF_SAPS_RIL_SIM_ON_EVT);
#else
    btapp_sc_cb.card_status = BTAPP_SC_CARD_READY;
    bta_sc_ci_sim_on(BTA_SC_RESULT_OK);
#endif
}


/*******************************************************************************
**
** Function         bta_sc_co_sim_off
**
** Description      Power off the SIM card.
**                  Since there is no actual SIM hardware, simply call the
**                  BTA_SC call-in with  result=BTA_SC_RESULT_OK.
**
** Parameters       none
**
** Returns          void
**
*******************************************************************************/
void bta_sc_co_sim_off(void)
{
    LOGI("%s:", __FUNCTION__);

#if (defined SAPS_USE_RIL) && (SAPS_USE_RIL == TRUE)
    BTL_IF_SendMsgNoParams(btapp_sc_cb.btl_if_handle, SUB_SAPS, BTLIF_SAPS_RIL_SIM_OFF_EVT);
#else
    btapp_sc_cb.card_status = BTAPP_SC_CARD_INSERTED;
    bta_sc_ci_sim_off(BTA_SC_RESULT_OK);
#endif
}


/*******************************************************************************
**
** Function         bta_sc_co_atr
**
** Description      Return ATR information.
**
** Parameters       none
**
** Returns          void
**
*******************************************************************************/
void bta_sc_co_atr(void)
{
    LOGI("%s:", __FUNCTION__);

#if (defined SAPS_USE_RIL) && (SAPS_USE_RIL == TRUE)
    BTL_IF_SendMsgNoParams(btapp_sc_cb.btl_if_handle, SUB_SAPS, BTLIF_SAPS_RIL_SIM_ATR_EVT);
#else
    bta_sc_ci_atr(BTA_SC_RESULT_OK, (UINT8 *) btapp_sc_atr, sizeof(btapp_sc_atr));
#endif
}


/*******************************************************************************
**
** Function         bta_sc_co_apdu
**
** Description      Process APDU request.
**                  Since there is no actual SIM hardware, simply parse the
**                  APDU request to display the command; and then call the
**                  BTA_SC call-in with result=BTA_SC_RESULT_DATA_NOT_AVAILABLE
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
void bta_sc_co_apdu(UINT8 *p_apdu_req, UINT16 req_len, UINT16 rsp_maxlen)
{
#if (defined SAPS_USE_RIL) && (SAPS_USE_RIL == TRUE)
    int params_len;
    tBTL_PARAMS* p_btl_if_params;

    LOGI("%s: req_len = %d", __FUNCTION__, req_len);

    params_len = req_len + sizeof(tBTLIF_SAPS_RIL_SIM_APDU_EVT_PARAM) - 4;
    p_btl_if_params = (tBTL_PARAMS*) calloc(1, params_len);
    if (p_btl_if_params == NULL) {
        LOGE("%s: Oops, Failed to allocate APDU parameter memory %d bytes", __FUNCTION__, params_len);
        return;
    }

    p_btl_if_params->saps_ril_apdu_evt_param.req_len    = req_len;
    p_btl_if_params->saps_ril_apdu_evt_param.rsp_maxlen = rsp_maxlen;
    memcpy(p_btl_if_params->saps_ril_apdu_evt_param.apdu_req, p_apdu_req, req_len);

    BTL_IF_CtrlSend(btapp_sc_cb.btl_if_handle, SUB_SAPS, BTLIF_SAPS_RIL_SIM_APDU_EVT, p_btl_if_params, params_len);
    free(p_btl_if_params);
#else
    UINT8 i = 0;
    tBTA_SC_RESULT result;

    LOGI("%s: req_len = %d", __FUNCTION__, req_len);

    /* Verify that command's class is GSM */
    if (p_apdu_req[BTA_APDU_CMD_OFFSET_CLA] != BTA_APDU_CMD_CLASS_GSM) {
        LOGE("%s: Received non-GSM APDU command class (0x%x)", __FUNCTION__, p_apdu_req[BTA_APDU_CMD_OFFSET_CLA]);
        result = BTA_SC_RESULT_ERROR;
    }
    else {
        /* Lookup the command's instruction ID */
        while (i < BTAPP_SC_APDU_INS_TABLE_SIZE) {
            if (btapp_sc_apdu_ins_table[i].ins_id == p_apdu_req[BTA_APDU_CMD_OFFSET_INS])
                break;
            i++;
        }

        /* Display name of command */
        if (i < BTAPP_SC_APDU_INS_TABLE_SIZE) {
            LOGI("%s: Received APDU command '%s'", __FUNCTION__, btapp_sc_apdu_ins_table[i].p_ins_name);
            result = BTA_SC_RESULT_DATA_NOT_AVAILABLE;      /* Respond with error result (since there is no actual SIM) */
        }
        else {
            LOGE("%s: Received unrecognized APDU command '0x%x'", __FUNCTION__, p_apdu_req[BTA_APDU_CMD_OFFSET_INS]);
            result = BTA_SC_RESULT_ERROR;
        }
    }

    bta_sc_ci_apdu(result, NULL, 0);
#endif
}

#endif
