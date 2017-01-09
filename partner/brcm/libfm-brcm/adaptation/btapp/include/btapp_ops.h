/*****************************************************************************
**
**  Name:             btapp_ops.h
**
**  Description:     This file contains btui internal interface
**				     definition
**
**  Copyright (c) 2000-2004, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#ifndef BTAPP_OPS_H
#define BTAPP_OPS_H

#include "bta_op_api.h"
#include "dtun.h"

typedef struct
{

     UINT32  bytes_transferred;
  UINT32         tick_count_last_notification;   /* tick count of last progress*/
     tBTA_OP_ACCESS  access_flag;
} tBTUI_OPS_CB;

extern tBTUI_OPS_CB btui_ops_cb;

void btapp_ops_init (void);
void btapp_op_grant_access (tDTUN_DEVICE_METHOD *p_data);
void btapp_op_set_owner_vcard (tDTUN_DEVICE_METHOD *p_data);
void btapp_op_set_exchange_folder (tDTUN_DEVICE_METHOD *p_data);
void btapp_op_create_vcard (tDTUN_DEVICE_METHOD *p_data);
void btapp_op_store_vcard (tDTUN_DEVICE_METHOD *p_data);

#endif
