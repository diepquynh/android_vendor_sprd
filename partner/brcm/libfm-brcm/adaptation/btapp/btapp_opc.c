/*****************************************************************************
**
**  Name:           btapp_opc.c
**
**  Description:     Contains application functions for OP Client
**
**  Copyright (c) 2003-2005, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bt_target.h"

#if( defined BTA_OP_INCLUDED ) && (BTA_OP_INCLUDED == TRUE)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "gki.h"
#include "bta_api.h"
#include "bta_op_api.h"
#include "bta_fs_api.h"
#include "bta_fs_co.h"
#include "bd.h"
#include "btui.h"
#include "btui_int.h"
#include "btapp.h"
#include "btapp_dm.h"
#include "dtun_api.h"
#include "btapp_opc.h"
#include "btapp_ops.h"
#include "btapp_vcard.h"

#define LOG_TAG "BTL-BTAPP_OPC:"

#ifndef LINUX_NATIVE
#include <cutils/log.h>
#include "utils/Log.h"
#else
#include <stdio.h>
#define LOGV(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGE(format, ...)  fprintf (stderr, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGI(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#endif

#define info(format, ...) LOGI (format, ## __VA_ARGS__)
#define debug(format, ...) LOGD (format, ## __VA_ARGS__)
#define error(format, ...) LOGE (format, ## __VA_ARGS__)

/*****************************************************************************
**  Local Function prototypes
*****************************************************************************/

void btui_opc_cback(tBTA_OPC_EVT event, tBTA_OPC *p_data);
void btapp_opc_dtun_register(void);

/* BTUI OPC main control block */
tBTUI_OPC_CB btui_opc_cb;

/*******************************************************************************
**
** Function         btapp_opc_init
**
** Description     Initializes OPC DTUN Interface
**
**
** Returns          void
*******************************************************************************/
void btapp_opc_init (void)
{
    info("**** Starting DTUN [OPC] Interface ****");
    btapp_opc_dtun_register();
}

/*******************************************************************************
**
** Function         btapp_opc_enable
**
** Description     Initializes & Enables BTUI & BTA OPC
**
**
** Returns          void
*******************************************************************************/
void btapp_opc_enable (tDTUN_DEVICE_METHOD *p_data)
{
    memset(&btui_opc_cb, 0, sizeof(tBTUI_OPC_CB));

    btui_opc_cb.single_op = btui_cfg.opc_single_op;
    BTA_OpcEnable(btui_cfg.opc_security, btui_opc_cback, btui_opc_cb.single_op,FALSE, 0);
}

/*******************************************************************************
**
** Function         btapp_opc_close
**
** Description      Closes connection
**
**
** Returns          void
*******************************************************************************/
void btapp_opc_close (tDTUN_DEVICE_METHOD *p_data)
{
    BTA_OpcClose();
}

/*******************************************************************************
**
** Function         btapp_ops_close
**
** Description      Closes connection
**
**
** Returns          void
*******************************************************************************/
void btapp_ops_close (tDTUN_DEVICE_METHOD *p_data)
{
    BTA_OpsClose();
}

/*******************************************************************************
**
** Function         btapp_opc_push_object
**
** Description      Sends the file object
**
**
** Returns          void
*******************************************************************************/
void btapp_opc_push_object (tDTUN_DEVICE_METHOD *p_data)
{
    char *p_name = p_data->opc_push_object.file_path_name;
    tBTA_OP_FMT format;
    DTUN_PATH lname;
    UINT16 i;

    if (btui_opc_cb.is_enabled && p_name)
    {
        for (i=0; p_name[i] != '\0'; i++) {
            lname[i] = (char)tolower(p_name[i]);
        }
        lname[i] = '\0';

        if(strstr(lname, ".vcf"))
        {
            format = BTA_OP_VCARD21_FMT;
        }
        else if(strstr(lname, ".vcd"))
        {
            format = BTA_OP_VCARD30_FMT;
        }
        else if(strstr(lname, ".vcs"))
        {
            format = BTA_OP_VCAL_FMT;
        }
        else if(strstr(lname, ".ics"))
        {
            format = BTA_OP_ICAL_FMT;
        }
        else if(strstr(lname, ".vnt"))
        {
            format = BTA_OP_VNOTE_FMT;
        }
        else if(strstr(lname, ".vmg"))
        {
            format = BTA_OP_VMSG_FMT;
        }
        else
        {
            format = BTA_OP_OTHER_FMT;
        }

        info("%s(%02X:%02X:%02X:%02X:%02X:%02X, %d, %s)", __FUNCTION__,
            p_data->opc_push_object.bdaddr.b[0], p_data->opc_push_object.bdaddr.b[1],
            p_data->opc_push_object.bdaddr.b[2], p_data->opc_push_object.bdaddr.b[3],
            p_data->opc_push_object.bdaddr.b[4], p_data->opc_push_object.bdaddr.b[5],
            format, p_name);

        BTA_OpcPush(p_data->opc_push_object.bdaddr.b, format, p_name);
    }
}

/*******************************************************************************
**
** Function         btapp_opc_pull_vcard
**
** Description      Pulls default card from peer device
**
**
** Returns          void
*******************************************************************************/
void btapp_opc_pull_vcard (tDTUN_DEVICE_METHOD *p_data)
{
    if (btui_opc_cb.is_enabled)
    {
        info("%s(%02X:%02X:%02X:%02X:%02X:%02X, %s)", __FUNCTION__,
            p_data->opc_pull_vcard.bdaddr.b[0], p_data->opc_pull_vcard.bdaddr.b[1],
            p_data->opc_pull_vcard.bdaddr.b[2], p_data->opc_pull_vcard.bdaddr.b[3],
            p_data->opc_pull_vcard.bdaddr.b[4], p_data->opc_pull_vcard.bdaddr.b[5],
            btui_cfg.root_path);

        BTA_OpcPullCard(p_data->opc_pull_vcard.bdaddr.b, btui_cfg.root_path);
    }
}

/*******************************************************************************
**
** Function         btapp_opc_exch_vcard
**
** Description      Initiates card exchange
**
**
** Returns          void
*******************************************************************************/
void btapp_opc_exch_vcard (tDTUN_DEVICE_METHOD *p_data)
{
    tDTUN_DEVICE_SIGNAL sig;
    char file_path[BTUI_MAX_PATH_LENGTH+1];
    BD_ADDR bdaddr;
#ifndef LINUX_NATIVE

    if (btui_opc_cb.is_enabled)
      { 
	memcpy(bdaddr,p_data->opc_exch_vcard.bdaddr.b,6);

	if (*(btui_cfg.op_owner_vcard) == '/') {
	  snprintf(file_path, BTUI_MAX_PATH_LENGTH+1,"%s", btui_cfg.op_owner_vcard);
	} else {
	  snprintf(file_path, BTUI_MAX_PATH_LENGTH+1,"%s%s", btui_cfg.root_path, btui_cfg.op_owner_vcard);
	}
	file_path[BTUI_MAX_PATH_LENGTH]='\0';

        if (access(file_path, F_OK) != 0)
        {
            info("Owner vCard not set");
            sig.hdr.id = DTUN_SIG_OP_OWNER_VCARD_NOT_SET;
            sig.hdr.len = DTUN_PATH_LEN;
            memcpy(sig.op_owner_vcard_not_set.name, file_path, DTUN_PATH_LEN);
            dtun_server_send_signal(&sig);
            return;
        }

        info("%s(%02X:%02X:%02X:%02X:%02X:%02X, %s, %s)", __FUNCTION__,
            p_data->opc_exch_vcard.bdaddr.b[0], p_data->opc_exch_vcard.bdaddr.b[1],
            p_data->opc_exch_vcard.bdaddr.b[2], p_data->opc_exch_vcard.bdaddr.b[3],
            p_data->opc_exch_vcard.bdaddr.b[4], p_data->opc_exch_vcard.bdaddr.b[5],
            file_path, btui_cfg.root_path);

        BTA_OpcExchCard(bdaddr, file_path, btui_cfg.root_path);
    }
#endif
}

/*******************************************************************************
**
** Function         btui_opc_cback
**
** Description      Call back from BTA opc
**
**
** Returns          void
*******************************************************************************/
void btui_opc_cback (tBTA_OPC_EVT event, tBTA_OPC *p_data)
{
    char file_path[BTUI_MAX_PATH_LENGTH];
    tDTUN_DEVICE_SIGNAL sig;
    UINT32 tick_count_current;

    switch (event)
    {
    case BTA_OPC_ENABLE_EVT:
        btui_opc_cb.is_enabled = TRUE;
        btui_opc_cb.p_connected_rem_device = NULL;
        info("BTUI Object Transfer Client ENABLED");
        dtun_server_send_signal_id(DTUN_SIG_OPC_ENABLE);
        break;

    case BTA_OPC_OPEN_EVT:
        info("BTUI Object Transfer Client CONNECTED");
        btui_opc_cb.bytes_transferred = 0;
        btui_opc_cb.tick_count_last_notification = 0;
        btui_opc_cb.operation = 0;  /* No operation active yet */
        btui_opc_cb.is_connected = TRUE;
        btui_opc_cb.p_connected_rem_device = btui_cb.p_selected_rem_device;
        dtun_server_send_signal_id(DTUN_SIG_OPC_OPEN);
        break;

    case BTA_OPC_PROGRESS_EVT:
        if (btui_opc_cb.operation != p_data->prog.operation)
        {
            btui_opc_cb.operation = p_data->prog.operation;
            btui_opc_cb.bytes_transferred = 0;
        }

        btui_opc_cb.bytes_transferred += p_data->prog.bytes;
        tick_count_current = GKI_get_os_tick_count();        
        
        if (p_data->prog.obj_size != BTA_FS_LEN_UNKNOWN)
        {
            debug("BTUI Object Transfer Client PROGRESS (%d of %d total)...",
                (int)btui_opc_cb.bytes_transferred, (int)p_data->prog.obj_size);
        }
        else
        {
            debug("BTUI Object Transfer Client PROGRESS (%d bytes total)...",
                (int)btui_opc_cb.bytes_transferred);
        }
        
        /* Notify framework of progress every BTAPP_OBX_FRAMEWORK_PROGRESS_NOTIFICATION_INTERVAL */
        if ((GKI_TICKS_TO_MS(tick_count_current - btui_opc_cb.tick_count_last_notification)) > BTAPP_OBX_FRAMEWORK_PROGRESS_NOTIFICATION_INTERVAL)
        {
            btui_opc_cb.tick_count_last_notification = tick_count_current;

            sig.hdr.id = DTUN_SIG_OPC_PROGRESS;
            sig.hdr.len = sizeof(UINT32) + sizeof(UINT32);
            sig.opc_progress.obj_size = (p_data->prog.obj_size != BTA_FS_LEN_UNKNOWN) ? p_data->prog.obj_size : 0;
            sig.opc_progress.bytes = btui_opc_cb.bytes_transferred;
            dtun_server_send_signal(&sig);
        }
        break;

    case BTA_OPC_OBJECT_EVT:
        btui_opc_cb.bytes_transferred = 0;
        btui_opc_cb.operation = 0;
        sprintf (file_path, "%s%s", btui_cfg.root_path, p_data->object.p_name);

        switch (p_data->object.status)
        {
        case BTA_OPC_OK:
            info("BTUI Object Transfer Client: vCard Received [name %s]", file_path);
            break;

        case BTA_OPC_NOT_FOUND:
            info("BTUI Object Transfer Client: vCard Not Found [name %s]", file_path);
            break;

        case BTA_OPC_NO_PERMISSION:
            info("BTUI Object Transfer Client: vCard [No Permission]");
            break;

        default:
            info("BTUI Object Transfer Client: vCard [Failed]");
            break;
        }

        sig.hdr.id = DTUN_SIG_OPC_OBJECT_RECEIVED;
        sig.hdr.len = sizeof(UINT8) + DTUN_PATH_LEN;
        sig.opc_object_received.status = p_data->object.status;
        memcpy(sig.opc_object_received.name, file_path, DTUN_PATH_LEN);
        dtun_server_send_signal(&sig);
        break;

    case BTA_OPC_OBJECT_PSHD_EVT:
        btui_opc_cb.bytes_transferred = 0;
        btui_opc_cb.operation = 0;

        switch (p_data->object.status)
        {
        case BTA_OPC_OK:
            info("BTUI Object Transfer Client: Object Sent [name %s]", p_data->object.p_name);
            break;

        case BTA_OPC_NO_PERMISSION:
            info("BTUI Object Transfer Client: Object Not Sent [No Permission]");
            break;

        default:
            info("BTUI Object Transfer Client: Object Not Sent [Failed]");
            break;
        }

        sig.hdr.id = DTUN_SIG_OPC_OBJECT_PUSHED;
        sig.hdr.len = sizeof(UINT8) + DTUN_PATH_LEN;
        sig.opc_object_pushed.status = p_data->object.status;
        memcpy(sig.opc_object_pushed.name, p_data->object.p_name, DTUN_PATH_LEN);
        dtun_server_send_signal(&sig);
        break;

    case BTA_OPC_CLOSE_EVT:
        btui_opc_cb.is_connected = FALSE;
        btui_opc_cb.p_connected_rem_device = NULL;
        info("BTUI Object Transfer Client DISCONNECTED (status %d)", p_data->status);
        sig.hdr.id = DTUN_SIG_OPC_CLOSE;
        sig.hdr.len = sizeof(UINT8);
        sig.opc_close.status = p_data->status;
        dtun_server_send_signal(&sig);
        break;
    }
}

#if 0
/* Method callbacks */ 
const tDTUN_METHOD opc_method_tbl[] =
{
    /* OPC */
    btapp_opc_enable,             /* DTUN_METHOD_OPC_ENABLE */
    btapp_opc_close,              /* DTUN_METHOD_OPC_CLOSE */
    btapp_ops_close,              /* DTUN_METHOD_OPS_CLOSE */
    btapp_opc_push_object,        /* DTUN_METHOD_OPC_PUSH_OBJECT */
    btapp_opc_pull_vcard,         /* DTUN_METHOD_OPC_PULL_VCARD */
    btapp_opc_exch_vcard,         /* DTUN_METHOD_OPC_EXCH_VCARD */

    btapp_op_grant_access,        /* DTUN_METHOD_OP_GRANT_ACCESS */
    btapp_op_set_owner_vcard,     /* DTUN_METHOD_OP_SET_OWNER_VCARD */
    btapp_op_set_exchange_folder, /* DTUN_METHOD_OP_SET_EXCHANGE_FOLDER */
    btapp_op_create_vcard,        /* DTUN_METHOD_OP_CREATE_VCARD */
    btapp_op_store_vcard,         /* DTUN_METHOD_OP_STORE_VCARD */
};
#endif

/*******************************************************************************
**
** Function         btapp_opc_dtun_register
**
** Description     Registers dtun methods
**
**
** Returns          void
*******************************************************************************/
void btapp_opc_dtun_register(void)
{
//    dtun_server_register_interface(DTUN_INTERFACE_OBEX, (tDTUN_METHOD *)&opc_method_tbl);
//    dtun_server_start();
}

#endif

