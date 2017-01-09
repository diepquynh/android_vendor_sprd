/*****************************************************************************
**
**  Name:             btapp_sc.h
**
**  Description:     This file contains btpp internal interface definition
**
**  Copyright (c) 2000-2004, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
******************************************************************************/
#ifndef BTAPP_SC_H
#define BTAPP_SC_H

#include "gki.h"

#if (defined BTA_SC_INCLUDED) && (BTA_SC_INCLUDED == TRUE)

#include "btl_ifs.h"


/* Enumeration of sap states */
enum {
    BTAPP_SC_STATE_IDLE,           /* Not connected to client */
    BTAPP_SC_STATE_OPEN,           /* Connected to client, but not active yet (waiting for SIM to be reset for client) */
    BTAPP_SC_STATE_RESET_PENDING,  /* Reset is pending (there was a on-going call on the phone..waiting for call to be released) */
    BTAPP_SC_STATE_READY           /* SIM card has been reset...client can start sending commands */
};

/* Enumeration of sim card status */
enum {
    BTAPP_SC_CARD_READY,
    BTAPP_SC_CARD_INSERTED,
    BTAPP_SC_CARD_REMOVED
};

/* BTAPP SC control block */
typedef struct
{
    tCTRL_HANDLE btl_if_handle;

    BOOLEAN      enabled;
    UINT8        sap_state;
    UINT8        card_status;       /* Status of SIM card (READY, INSERTED, or REMOVED) */
    BOOLEAN      call_in_progress;  /* TRUE to simulate a call in progress */
} tBTAPP_SC_CB;
extern tBTAPP_SC_CB  btapp_sc_cb;


void btapp_sc_init(void);
void btapp_sc_close(tBTA_SC_DISCONNECT_TYPE type);
void btapp_sc_disable(void);
void btapp_sc_refresh(void);

void btapp_sc_card_reset(void);
void btapp_sc_card_remove(void);
void btapp_sc_card_insert(void);

void btapp_sc_call_state(BOOLEAN is_active);

#if (defined SAPS_USE_RIL) && (SAPS_USE_RIL == TRUE)
void btapp_sc_ril_on_rx_ctrl(tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params);
#endif

#endif  // BTA_SC_INCLUDED
#endif  // BTAPP_SC_H
