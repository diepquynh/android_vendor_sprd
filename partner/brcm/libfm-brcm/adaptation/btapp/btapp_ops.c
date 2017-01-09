/****************************************************************************
**
**  Name:          btapp_ops.c
**
**  Description:   Contains application functions for object push server
**
**
**  Copyright (c) 2002-2006, Broadcom Corp., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include "bt_target.h"

#if( defined BTA_OP_INCLUDED ) && (BTA_OP_INCLUDED == TRUE)

#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#include "gki.h"
#include "bta_api.h"
#include "bta_op_api.h"
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

#define LOG_TAG "BTL-BTAPP_OPS:"

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

#define OPP_DEBUG 1

/* BTUI OP server main control block */
tBTUI_OPS_CB btui_ops_cb;

/*****************************************************************************
**  Constants
*****************************************************************************/
static const char * const bta_ops_fmt_2_string[] =
{
    "Other",
    "vCard 2.1",
    "vCard 3.0",
    "vCal 1.0",
    "vCal 2.0",
    "vNote",
    "vMessage"
};

/*****************************************************************************
**  Local Function prototypes
*****************************************************************************/
void bta_ops_cback(tBTA_OPS_EVT event, tBTA_OPS *p_data);

#ifndef BTUI_OPS_FORMATS
#define BTUI_OPS_FORMATS            (BTA_OP_VCARD21_MASK | BTA_OP_VCARD30_MASK | \
                                     BTA_OP_VCAL_MASK | BTA_OP_ICAL_MASK | \
                                     BTA_OP_VNOTE_MASK | BTA_OP_VMSG_MASK | \
                                     BTA_OP_ANY_MASK )
#endif
#define OP_FILE_PATH         "/data/data/com.broadcom.bt.app.opp/"
#define DEFAULT_OWNER_VCARD  "op_default.vcf"

/*******************************************************************************
**
** Function         btapp_ops_init
**
** Description      Initializes OP server
**
**
** Returns          void
*******************************************************************************/
void btapp_ops_init(void)
{
    btui_cfg.ops_included = TRUE;
    btui_cfg.ops_security = BTA_SEC_AUTHORIZE;
    strncpy(btui_cfg.root_path, OP_FILE_PATH, sizeof(btui_cfg.root_path));
    strncpy(btui_cfg.op_owner_vcard, DEFAULT_OWNER_VCARD,
        sizeof(btui_cfg.op_owner_vcard));

    /* make sure OPS Service name is added for SDP record */
    strncpy(btui_cfg.ops_service_name, BTUI_OPS_SERVICE_NAME,
        sizeof(btui_cfg.ops_service_name));

    info("OPS Supported Formats = 0x%x", BTUI_OPS_FORMATS);
    if (btui_cfg.ops_included)
        BTA_OpsEnable(btui_cfg.ops_security, BTUI_OPS_FORMATS,
                      btui_cfg.ops_service_name, bta_ops_cback, FALSE, 0);

    /* Sample app always allows object push operations unless changed in test menu */
    btui_ops_cb.access_flag = BTA_OP_ACCESS_ALLOW;
}

/*******************************************************************************
**
** Function         btapp_op_grant_access
**
** Description      Access granted
**
**
** Returns          void
*******************************************************************************/
void btapp_op_grant_access (tDTUN_DEVICE_METHOD *p_data)
{
#ifndef LINUX_NATIVE
    tDTUN_DEVICE_SIGNAL sig;

    btui_ops_cb.access_flag = p_data->op_grant_access.access;

    if ((p_data->op_grant_access.operation == BTA_OP_OPER_PULL) &&
        (p_data->op_grant_access.access == BTA_OP_ACCESS_ALLOW)) {
        if (access(p_data->op_grant_access.file_path_name, F_OK) != 0) {
            info("Owner vCard not set");
            sig.hdr.id = DTUN_SIG_OP_OWNER_VCARD_NOT_SET;
            sig.hdr.len = DTUN_PATH_LEN;
            memcpy(sig.op_owner_vcard_not_set.name,
                p_data->op_grant_access.file_path_name, DTUN_PATH_LEN);
            dtun_server_send_signal(&sig);

            btui_ops_cb.access_flag = BTA_OP_ACCESS_NONSUP;
        }
    }

    info("%s(%d, %d, %s)", __FUNCTION__, p_data->op_grant_access.operation,
        btui_ops_cb.access_flag, p_data->op_grant_access.file_path_name);

    BTA_OpsAccessRsp(p_data->op_grant_access.operation,
        btui_ops_cb.access_flag, p_data->op_grant_access.file_path_name);
#endif
}

/*******************************************************************************
**
** Function         btapp_op_set_owner_vcard
**
** Description      Set Owner vCard
**
**
** Returns          void
*******************************************************************************/
void btapp_op_set_owner_vcard (tDTUN_DEVICE_METHOD *p_data)
{
    info("%s(%s)", __FUNCTION__, p_data->op_set_owner_vcard.file_name);
    int len = strlen(p_data->op_set_owner_vcard.file_name);

    if (len <=0) {
        info("ERROR setting owner vcard. No file path specified.");
         return;
    }

    //Check path length
    int path_length=0;
    if (*p_data->op_set_owner_vcard.file_name == '/') {
        //Full path specified
        path_length=len;
    } else {
        //Relative path specified
        path_length = len + strlen(btui_cfg.root_path);
    }
    if (path_length > BTUI_MAX_PATH_LENGTH) {
        info("ERROR setting owner vcard. Path length exceeded. path_length=%d ", path_length);
        return;
    }

    //Check the filename length
    int filename_length = len;
    char* last_dir_token= strrchr(p_data->op_set_owner_vcard.file_name,'/');
    if (last_dir_token != NULL) {
        filename_length = path_length- (last_dir_token-p_data->op_set_owner_vcard.file_name) +1;
    }
    if (filename_length > BTUI_MAX_FILENAME_LENGTH) {
        info("ERROR setting owner vcard. Filename length exceeded. filename_length=%d ", filename_length);
        return;
    }

    strncpy(btui_cfg.op_owner_vcard, p_data->op_set_owner_vcard.file_name,
        sizeof(btui_cfg.op_owner_vcard));
}

/*******************************************************************************
**
** Function         btapp_op_set_exchange_folder
**
** Description      Set Exchange Folder
**
**
** Returns          void
*******************************************************************************/
void btapp_op_set_exchange_folder (tDTUN_DEVICE_METHOD *p_data)
{
    UINT16 len;

    len = strlen(p_data->op_set_exchange_folder.path_name);
    if (p_data->op_set_exchange_folder.path_name[len-1] != '/') {
        strcat(p_data->op_set_exchange_folder.path_name, "/");
    }

    info("%s(%s)", __FUNCTION__, p_data->op_set_exchange_folder.path_name);

    strncpy(btui_cfg.root_path, p_data->op_set_exchange_folder.path_name,
        sizeof(btui_cfg.root_path));
}

/*******************************************************************************
**
** Function         btapp_op_create_vcard
**
** Description      Create vCard
**
**
** Returns          void
*******************************************************************************/
void btapp_op_create_vcard (tDTUN_DEVICE_METHOD *p_data)
{
    tDTUN_DEVICE_SIGNAL sig;

    info("%s(%s, %s)", __FUNCTION__, p_data->op_create_vcard.vcard_uri,
        p_data->op_create_vcard.file_path_name);

    sig.hdr.id = DTUN_SIG_OP_CREATE_VCARD;
    sig.hdr.len = sizeof(uint8_t) + DTUN_PATH_LEN;
    sig.op_create_vcard.status =
        btapp_vcard_build_vcard(p_data->op_create_vcard.vcard_uri,
            p_data->op_create_vcard.file_path_name);
    memcpy(sig.op_create_vcard.name, p_data->op_create_vcard.file_path_name,
        DTUN_PATH_LEN);
    dtun_server_send_signal(&sig);
}

/*******************************************************************************
**
** Function         btapp_op_store_vcard
**
** Description      Store vCard
**
**
** Returns          void
*******************************************************************************/
void btapp_op_store_vcard (tDTUN_DEVICE_METHOD *p_data)
{
    const char bta_op_vcard_prs_begin[] = "BEGIN:VCARD\r\n";
    const char bta_op_vcard_end[] = "END:VCARD\r\n";
    tDUP_ENTRY_INFO *dup_entry_info;
    char op_mem[128], *p, *p_name;
    tDTUN_DEVICE_SIGNAL sig;
    int file_num=0;
    FILE *file, *outfile;

    p_name = p_data->op_store_vcard.file_path_name;
    info("%s(%s, %d)", __FUNCTION__, p_name, p_data->op_store_vcard.dup_action);

    if ((file = fopen(p_name, "r")) == NULL) {
        error("Error opening %s", p_name);
        return;
    }

    while (fgets(op_mem, sizeof(op_mem), file) != NULL) {
        if (strstr(op_mem, bta_op_vcard_prs_begin)) {
            file_num++;
        }
    }
    fclose(file);

    if (file_num == 0) {
        error("No vCard(s) found in %s", p_name);

        strncpy(sig.op_store_vcard.name, p_name, sizeof(sig.op_store_vcard.name));
        sig.hdr.id = DTUN_SIG_OP_STORE_VCARD;
        sig.hdr.len = sizeof(uint16_t) + DTUN_PATH_LEN +
            sizeof(sig.op_store_vcard.contact_name) + sizeof(uint16_t);
        sig.op_store_vcard.status = VCARD_STORE_STATUS_FAIL;
        memcpy(sig.op_store_vcard.contact_name, "",
            sizeof(sig.op_store_vcard.contact_name));
        sig.op_store_vcard.store_id = 0;
        dtun_server_send_signal(&sig);
        return;
    }

    if (file_num == 1) {
        strncpy(sig.op_store_vcard.name, p_name, sizeof(sig.op_store_vcard.name));
        dup_entry_info = btapp_vcard_store_vcard(sig.op_store_vcard.name,
                            p_data->op_store_vcard.dup_action);

        sig.hdr.id = DTUN_SIG_OP_STORE_VCARD;
        sig.hdr.len = sizeof(uint16_t) + DTUN_PATH_LEN +
            sizeof(sig.op_store_vcard.contact_name) + sizeof(uint16_t);
        sig.op_store_vcard.status = dup_entry_info->status;
        memcpy(sig.op_store_vcard.contact_name, dup_entry_info->name,
            sizeof(sig.op_store_vcard.contact_name));
        sig.op_store_vcard.store_id = dup_entry_info->person_id;
        dtun_server_send_signal(&sig);
        return;
    }

    info("Splitting %d vCards found in %s", file_num, p_name);

    if ((file = fopen(p_name, "r")) == NULL) {
        error("Error opening %s", p_name);
        return;
    }

    file_num = 0;
    if ((p = strrchr(p_name, '.'))) {
        *p = '\0';
        sprintf(sig.op_store_vcard.name, "%s-%d.%s", p_name, file_num+1, (p+1));
        *p = '.';
    }
    if ((outfile = fopen(sig.op_store_vcard.name, "w")) == NULL) {
        error("Error opening %s", sig.op_store_vcard.name);
        fclose(file);
        return;
    }

    while (fgets(op_mem, sizeof(op_mem), file) != NULL) {
        if (strstr(op_mem, bta_op_vcard_end)) {
            fputs(op_mem, outfile);
            fclose(outfile);

            dup_entry_info = btapp_vcard_store_vcard(sig.op_store_vcard.name,
                                p_data->op_store_vcard.dup_action);

            sig.hdr.id = DTUN_SIG_OP_STORE_VCARD;
            sig.hdr.len = sizeof(uint16_t) + DTUN_PATH_LEN +
                sizeof(sig.op_store_vcard.contact_name) + sizeof(uint16_t);
            sig.op_store_vcard.status = dup_entry_info->status;
            memcpy(sig.op_store_vcard.contact_name, dup_entry_info->name,
                sizeof(sig.op_store_vcard.contact_name));
            sig.op_store_vcard.store_id = dup_entry_info->person_id;
            dtun_server_send_signal(&sig);

            file_num++;
            if ((p = strrchr(p_name, '.'))) {
                *p = '\0';
                sprintf(sig.op_store_vcard.name, "%s-%d.%s", p_name, file_num+1, (p+1));
                *p = '.';
            }
            if ((outfile = fopen(sig.op_store_vcard.name, "w")) == NULL) {
                error("Error opening %s", sig.op_store_vcard.name);
                fclose(file);
                return;
            }
        } else {
            fputs(op_mem, outfile);
        }
    }
    fclose(file);
    fclose(outfile);

    if (file_num == 0) {
        remove(sig.op_store_vcard.name);
    }
    info("Processed %d vCards found in %s", file_num, p_name);
    return;
}

/*******************************************************************************
**
** Function         bta_ops_cback
**
** Description      Callback from BTA OPS
**
**
** Returns          void
*******************************************************************************/
void bta_ops_cback(tBTA_OPS_EVT event, tBTA_OPS *p_data)
{
    UINT8 acc_index;
    char  msg_str[132];
    tBTUI_BTA_MSG * p_msg;
    tDTUN_DEVICE_SIGNAL sig;
    UINT32 tick_count_current;
    char file_path[BTUI_MAX_PATH_LENGTH+1]; //If file_path is fully set
    int path_length;
    int filename_length;
    BOOLEAN access_denied;

    switch (event)
    {
    case BTA_OPS_PROGRESS_EVT:
        btui_ops_cb.bytes_transferred += p_data->prog.bytes;
        tick_count_current = GKI_get_os_tick_count();

        if (p_data->prog.obj_size != BTA_FS_LEN_UNKNOWN)
        {
            debug("Object Transfer Server PROGRESS (%d of %d total)...",
                (int)btui_ops_cb.bytes_transferred, (int)p_data->prog.obj_size);
        }
        else
        {
            debug("Object Transfer Server PROGRESS (%d bytes total)...", (int)btui_ops_cb.bytes_transferred);
        }


        /* Notify framework of progress every BTAPP_OBX_FRAMEWORK_PROGRESS_NOTIFICATION_INTERVAL */
        if ((GKI_TICKS_TO_MS(tick_count_current - btui_ops_cb.tick_count_last_notification)) > BTAPP_OBX_FRAMEWORK_PROGRESS_NOTIFICATION_INTERVAL)
        {
            btui_ops_cb.tick_count_last_notification = tick_count_current;

        	sig.hdr.id = DTUN_SIG_OPS_PROGRESS;
        	sig.hdr.len = sizeof(UINT32) + sizeof(UINT32);
        	sig.ops_progress.obj_size = (p_data->prog.obj_size != BTA_FS_LEN_UNKNOWN) ? p_data->prog.obj_size : 0;
        	sig.ops_progress.bytes = btui_ops_cb.bytes_transferred;
        	dtun_server_send_signal(&sig);
            }
        break;

    case BTA_OPS_OBJECT_EVT:
        acc_index = (p_data->object.format != BTA_OP_OTHER_FMT) ? p_data->object.format : 0;
        sprintf(file_path, "%s%s", btui_cfg.root_path, p_data->object.p_name);
        info("Object Received: Name [%s], Type [%s]", file_path, bta_ops_fmt_2_string[acc_index]);

        sig.hdr.id = DTUN_SIG_OPS_OBJECT_RECEIVED;
        sig.hdr.len = sizeof(UINT8) + DTUN_PATH_LEN;
        sig.ops_object_received.format = p_data->object.format;
        memcpy(sig.ops_object_received.name, file_path, DTUN_PATH_LEN);
        dtun_server_send_signal(&sig);
        break;

    case BTA_OPS_OPEN_EVT:
        info("OPS Connection Opened with %02X:%02X:%02X:%02X:%02X:%02X",
            p_data->bd_addr[0], p_data->bd_addr[1], p_data->bd_addr[2],
            p_data->bd_addr[3], p_data->bd_addr[4], p_data->bd_addr[5]);
        btui_ops_cb.access_flag = BTA_OP_ACCESS_ALLOW;
        dtun_server_send_signal_id(DTUN_SIG_OPS_OPEN);
        break;

    case BTA_OPS_CLOSE_EVT:
        info("OPS Connection Closed...");
        dtun_server_send_signal_id(DTUN_SIG_OPS_CLOSE);
        break;

    case BTA_OPS_ACCESS_EVT:
        btui_ops_cb.bytes_transferred = 0;
        btui_ops_cb.tick_count_last_notification = 0;
	access_denied=FALSE;

        if (btui_ops_cb.access_flag == BTA_OP_ACCESS_ALLOW) {
	    if (p_data->access.oper == BTA_OP_OPER_PUSH) {
	        //Check file path and path length for push.
		filename_length = strlen(p_data->access.p_name);
	        path_length = strlen(btui_cfg.root_path) + filename_length;

	        if (filename_length<=BTUI_MAX_FILENAME_LENGTH && path_length <=BTUI_MAX_PATH_LENGTH) {
  		    snprintf(file_path,BTUI_MAX_PATH_LENGTH+1,"%s%s", btui_cfg.root_path, p_data->access.p_name);
		    file_path[BTUI_MAX_PATH_LENGTH]='\0';
		} else {
 		    access_denied=TRUE;
		}
            } else {
	        //Dont checkfile path and path length for pull, since it was
                //already checked in the set owner vcard call
                if (*btui_cfg.op_owner_vcard == '/') {
		    snprintf(file_path,BTUI_MAX_PATH_LENGTH+1, "%s", btui_cfg.op_owner_vcard);
                } else {
		    snprintf(file_path,BTUI_MAX_PATH_LENGTH+1, "%s%s", btui_cfg.root_path, btui_cfg.op_owner_vcard);
		}
		file_path[BTUI_MAX_PATH_LENGTH]='\0';
            }
        } else {
 	    access_denied=TRUE;
	}

	//-------DEBUGGING LOG
	if (OPP_DEBUG) {
	  if (p_data->access.oper == BTA_OP_OPER_PUSH) {
            strncpy(msg_str, "Object Transfer: PUSH Access request: ", sizeof(msg_str));
	  } else if (p_data->access.oper == BTA_OP_OPER_PULL) {
            strncpy(msg_str, "Object Transfer: PULL Access request: ", sizeof(msg_str));
	  }

	  if (access_denied ==FALSE) {
	    strcat(msg_str, "GRANTED for:");
	  } else {
	    strcat(msg_str, "DENIED for:");
	  }

	  info("%s", msg_str);
	  acc_index = (p_data->access.format != BTA_OP_OTHER_FMT) ? p_data->access.format : 0;
	  if (p_data->access.oper == BTA_OP_OPER_PUSH) {
            if (p_data->access.p_type) {
	      info("   Obj [%s], Type [%s (%s)], Size [%d]",
		   p_data->access.p_name, p_data->access.p_type,
                    bta_ops_fmt_2_string[acc_index], (int)p_data->access.size);
            } else {
	      info("   Obj [%s], Type [%s], Size [%d]", p_data->access.p_name,
		   bta_ops_fmt_2_string[acc_index], (int)p_data->access.size);
            }
	  } else {
            info("   Type [%s]", bta_ops_fmt_2_string[acc_index]);
	  }

	  sprintf(msg_str, "%02X:%02X:%02X:%02X:%02X:%02X",
		  p_data->access.bd_addr[0], p_data->access.bd_addr[1],
		  p_data->access.bd_addr[2], p_data->access.bd_addr[3],
		  p_data->access.bd_addr[4], p_data->access.bd_addr[5]);
	  info("   Device [%s] %s", p_data->access.dev_name, msg_str);
	  if (path_length > BTUI_MAX_PATH_LENGTH || filename_length> BTUI_MAX_FILENAME_LENGTH ) {
	      info ("    ERROR: invalid filepath!!! path_length= %d, filename_length= %d",path_length,filename_length );
	  }

	}
	//-------END DEBUGGING LOG


	if (access_denied) {
	  BTA_OpsAccessRsp(p_data->access.oper,
			   BTA_OP_ACCESS_FORBID, p_data->access.p_name);
	  return;
	}

	//Forward grant access request up to app layer
        sig.hdr.id = DTUN_SIG_OPS_ACCESS_REQUEST;
        sig.hdr.len = sizeof(sig.ops_access_request.bdname) +
            sizeof(sig.ops_access_request.bdaddr) + sizeof(UINT8) + sizeof(UINT8) +
            sizeof(sig.ops_access_request.name) + sizeof(UINT32);
        memcpy(sig.ops_access_request.bdname, p_data->access.dev_name,
            sizeof(sig.ops_access_request.bdname));
        memcpy(sig.ops_access_request.bdaddr, msg_str,
            sizeof(sig.ops_access_request.bdaddr));
        sig.ops_access_request.oper = p_data->access.oper;
        sig.ops_access_request.format = p_data->access.format;
        memcpy(sig.ops_access_request.name, file_path, sizeof(sig.ops_access_request.name));
        sig.ops_access_request.obj_size = p_data->access.size;
        dtun_server_send_signal(&sig);
        break;

    default:
            info("OPS event:%d", event);
    }
}

#endif
