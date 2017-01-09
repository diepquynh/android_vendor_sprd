/*****************************************************************************
**
**  Name:             btapp_dm.h
**
**  Description:     This file contains btui internal interface
**				     definition
**
**  Copyright (c) 2000-2004, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include "gki.h"

#ifndef BTAPP_DM_H
#define BTAPP_DM_H


#define BTUI_AUTH_REQ_NO            0   /* always not required */
#define BTUI_AUTH_REQ_YES           1   /* always required */
#define BTUI_AUTH_REQ_GEN_BOND      2   /* security database + general bonding, dedicated bonding as not required */
#define BTUI_AUTH_REQ_DEFAULT       3   /* security database, dedicated bonding as not required */
#define BTUI_AUTH_REQ_GEN_BOND_DD   4   /* security database + general bonding, dedicated bonding as required */
#define BTUI_AUTH_REQ_DEFAULT_DD    5   /* security database, dedicated bonding as required */

#define BTUI_AUTH_REQ_DD_BIT        4   /* dedicated bonding bit */

/* AG/HS will invoke SCO callout, need to identify different profile by APP ID */
/* AG use app ID 1, and 2, assign app ID 3 to HS */
#define BTUI_DM_SCO_4_HS_APP_ID	    3	/* SCO over HCI app ID for HS */

extern void btapp_startup(void);
extern void btapp_dm_init(void);
extern void btapp_dm_pin_code_reply(BOOLEAN accept, UINT8 pin_len, UINT8 *p_pin);
extern void btapp_dm_confirm_reply(BOOLEAN accept);
extern void btapp_dm_passkey_cancel(void);
extern void btapp_dm_rmt_oob_reply(BOOLEAN accept, BT_OCTET16 c, BT_OCTET16 r);
extern void  btapp_dm_loc_oob(void);
extern void btapp_dm_authorize_resp(tBTA_AUTH_RESP response);
extern void btapp_dm_disable_bt(void);
extern void btapp_dm_set_visibility( BOOLEAN is_visible, BOOLEAN is_temp);
extern void btapp_dm_set_find_me( void);
extern void btapp_dm_make_non_discoverable(void);
extern void btapp_dm_set_local_name(char *p_name);
extern BOOLEAN btapp_dm_set_trusted(tBTA_SERVICE_MASK trusted_mask, tBTUI_REM_DEVICE * p_device_rec);
extern void btapp_dm_set_not_trusted(tBTUI_REM_DEVICE * p_device_rec);
extern BOOLEAN btapp_dm_delete_device(void);
extern void btapp_dm_discover_device(BD_ADDR bd_addr, BOOLEAN is_new);
extern BOOLEAN btapp_dm_stored_device_unbond (void);
extern void btapp_dm_cancel_search(void);
extern void btapp_dm_search(tBTA_SERVICE_MASK services,tBTA_DM_INQ *p_data);
extern BOOLEAN btapp_dm_add_device(void);
extern void btapp_dm_sec_add_device(tBTUI_REM_DEVICE * p_device_rec);
extern void btapp_dm_bond(tBTUI_REM_DEVICE * p_device_rec);
extern void btapp_dm_bond_cancel(tBTUI_REM_DEVICE * p_device_rec);
extern void btapp_dm_rename_device(tBTUI_REM_DEVICE * p_device_rec, UINT8 * p_text);
extern void btui_add_devices(void);
extern tBTUI_REM_DEVICE * btapp_dm_db_get_device_info(BD_ADDR bd_addr);
extern BOOLEAN btapp_dm_db_get_device_list(	tBTA_SERVICE_MASK services,	tBTUI_REM_DEVICE * p_device,UINT8*	number_of_devices, BOOLEAN new_only);
extern void btapp_dm_switch_bb2mm(void);
extern void btapp_dm_switch_mm2bb(void);
extern void btapp_dm_switch_bb2btc(void);
extern void btapp_dm_switch_btc2bb(void);

extern void btapp_dm_proc_io_req(BD_ADDR bd_addr, tBTA_IO_CAP *p_io_cap, tBTA_OOB_DATA *p_oob_data,
                                 tBTA_AUTH_REQ *p_auth_req, BOOLEAN is_orig);
extern void btapp_dm_proc_io_rsp(BD_ADDR bd_addr, tBTA_IO_CAP io_cap, tBTA_AUTH_REQ auth_req);
extern void btapp_dm_proc_lk_upgrade(BD_ADDR bd_addr, BOOLEAN *p_upgrade);

#if BTA_DI_INCLUDED == TRUE
extern void btapp_dm_di_discover(BD_ADDR bd_addr);
extern UINT16 btapp_dm_add_di_record(void);
extern tBTA_STATUS btapp_dm_get_di_remote_record(tBTA_DI_GET_RECORD *p_record, UINT8 index);
extern tBTA_STATUS btapp_dm_get_di_local_record(tBTA_DI_GET_RECORD *p_di_record, UINT32 handle);
#endif

#endif
