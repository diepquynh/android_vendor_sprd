/*****************************************************************************
**
**  Name:             btapp_opc.h
**
**  Description:     This file contains btapp interface
**				     definition
**
**  Copyright (c) 2000-2005, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#ifndef BTAPP_OPC_H
#define BTAPP_OPC_H

#include "bta_op_api.h"
#include "btui_int.h"
#include "dtun.h"

/* BTUI opc control block */
typedef struct
{
     UINT32         bytes_transferred;
  UINT32         tick_count_last_notification;   /* tick count of last progress*/
#if (defined BTA_OP_INCLUDED) && (BTA_OP_INCLUDED == TRUE)
     tBTA_OP_OPER operation;
#endif

     BOOLEAN        is_connected;
     BOOLEAN        single_op;
     UINT8          num_files;
     BOOLEAN        is_enabled;
     tBTUI_REM_DEVICE * p_connected_rem_device;
} tBTUI_OPC_CB;

extern tBTUI_OPC_CB btui_opc_cb;

void btapp_opc_init (void);
void btapp_opc_enable (tDTUN_DEVICE_METHOD *p_data);
void btapp_opc_close (tDTUN_DEVICE_METHOD *p_data);
void btapp_ops_close (tDTUN_DEVICE_METHOD *p_data);
void btapp_opc_push_object (tDTUN_DEVICE_METHOD *p_data);
void btapp_opc_pull_vcard (tDTUN_DEVICE_METHOD *p_data);
void btapp_opc_exch_vcard (tDTUN_DEVICE_METHOD *p_data);

#endif
