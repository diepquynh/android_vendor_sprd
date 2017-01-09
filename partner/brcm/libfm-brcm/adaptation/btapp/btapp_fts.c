/****************************************************************************
**
**  Name:          btapp_fts.c
**
**  Description:   Contains  functions for file transfer server
**
**
**  Copyright (c) 2003-2006, Broadcom Corp., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include "bt_target.h"

#if( defined BTA_FT_INCLUDED ) && (BTA_FT_INCLUDED == TRUE)


#include "gki.h"
#include "bta_api.h"
#include "btui.h"
#include "bta_fs_api.h"
#include "bta_ft_api.h"
#include "bta_fs_co.h"
#include "btui_int.h"
#include "btapp_fts.h"
#include "btl_ifs.h"

#define LOG_TAG "BTAPP_FTPS"

#ifndef LINUX_NATIVE
#include <cutils/properties.h>
#include <cutils/log.h>
#else
#include <stdio.h>
#define LOGI(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGD(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGV(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGE(format, ...)  fprintf (stderr, LOG_TAG format"\n", ## __VA_ARGS__)
#endif


/* Root name off of the working directory */
#define FTS_ROOT_FOLDER         "/sdcard"
#define FTS_ROOT_FOLDER_PROPERTY "service.brcm.bt.FTS_ROOT_FOLDER"

/* Maximum path length supported by MMI */
#ifndef BTUI_PATH_LEN
#define BTUI_PATH_LEN           260
#endif

/* Maximum file name size*/
#ifndef BTAPP_FTS_ACC_FNAME_MAX
#define BTAPP_FTS_ACC_FNAME_MAX 550
#endif

/* Notification interval for object transfer progress (in milliseconds) */
#ifndef BTAPP_FTS_FRAMEWORK_NOTIFICATION_INTERVAL
#define BTAPP_FTS_FRAMEWORK_NOTIFICATION_INTERVAL   (1000)
#endif

static tCTRL_HANDLE btl_if_fts_handle;

static void btapp_fts_on_rx_ctrl(tCTRL_HANDLE fd, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params);


/* BTUI FT server main control block */
tBTUI_FTS_CB btui_fts_cb;


/*****************************************************************************
**  Local Function prototypes
*****************************************************************************/
static void btui_fts_cback(tBTA_FTS_EVT event, tBTA_FTS *p_data);



/*******************************************************************************
**
** Function         btapp_fts_init
**
** Description      FTS initialization function.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_fts_init(void)
{
    tBTL_IF_Result result;

    result = BTL_IF_ServerInit();
    APPL_TRACE_DEBUG2("%s IFS init result = %d", __FUNCTION__, result);
    result = BTL_IF_RegisterSubSystem(&btl_if_fts_handle, SUB_FTPS, NULL, btapp_fts_on_rx_ctrl);
    APPL_TRACE_DEBUG2("BTL_IF_RegisterSubSystem result = %d btl_if_fts_handle = %d", result, btl_if_fts_handle);

}

static void btapp_fts_on_rx_enable(void)
{
    char   *p_root_path;
    BOOLEAN start_ok = TRUE;
    BOOLEAN is_dir;

    /* No menus for FT server
    Keep the previous code as such.
    It can be used later if needed */

    btui_cfg.p_fts_event_hdlr = NULL;
    btui_cfg.p_fts_menu = NULL;

    /* Set Authentication security bit so remote is prompted for pincode if not already paired */
    btui_cfg.fts_security = BTA_SEC_AUTHORIZE | BTA_SEC_AUTHENTICATE;
    strcpy(btui_cfg.fts_service_name, BTUI_FTS_SERVICE_NAME);

    /* Sample app always allows PUT file operations unless changed in test menu */
    btui_fts_cb.access_flag = BTA_FT_ACCESS_ALLOW;

    if ((p_root_path = (char *)GKI_getbuf((UINT16)(BTUI_PATH_LEN + 1))) != NULL)
    {
        memset(p_root_path, 0, BTUI_PATH_LEN + 1);

        /* Use the BTA environment if it exists, otherwise create a default root path */
	if(! property_get(FTS_ROOT_FOLDER_PROPERTY, p_root_path, ""))
	{
            sprintf(p_root_path, "%s", FTS_ROOT_FOLDER);
	}
        if ((bta_fs_co_access (p_root_path, BTA_FS_ACC_EXIST, &is_dir, UI_FTS_ID)) != BTA_FS_CO_OK)
        {
            APPL_TRACE_WARNING1("FTP SERVER cannot open Root Path [%s]", p_root_path);
        }

        BTA_FtsEnable(btui_cfg.fts_security, btui_cfg.fts_service_name,
                      p_root_path, btui_cfg.fts_obex_auth,
                      (UINT8)strlen(btui_cfg.fts_realm),
                      btui_cfg.fts_realm, btui_fts_cback, UI_FTS_ID);
        APPL_TRACE_EVENT1("FTP SERVER enabled with Root Path [%s]", p_root_path);
        GKI_freebuf(p_root_path);
    }
}

/*******************************************************************************
**
** Function         btapp_fts_disable
**
** Description      Disables FTS
**
**
** Returns          void
**
*******************************************************************************/
void btapp_fts_disable(void)
{

    BTA_FtsDisable();

}



/*******************************************************************************
**
** Function         btui_fts_cback
**
** Description      FTS UI Callback function.  Handles all FTS events for the UI
**
**
** Returns          void
**
*******************************************************************************/
static void btui_fts_cback(tBTA_FTS_EVT event, tBTA_FTS *p_data)
{
    tBTL_PARAMS* p_btl_if_params;
    int param_data_length = 0;

    switch (event)
    {
    case BTA_FTS_ENABLE_EVT:
        APPL_TRACE_EVENT0("File Transfer: Server Enabled...");
        BTL_IF_CtrlSend(btl_if_fts_handle, SUB_FTPS,  BTLIF_FTPS_ENABLE_EVT, NULL, 0 );
        break;

    case BTA_FTS_AUTH_EVT:
        if (p_data->auth.p_userid)
        {
            APPL_TRACE_EVENT1("File Transfer: Authentication Processed for User ID [%s]...",
                              p_data->auth.p_userid);
        }
        else  {
            APPL_TRACE_EVENT0("File Transfer: Authentication Processed without client user id...");
        }


        if (p_data->auth.userid_required)
        {
            APPL_TRACE_EVENT0("               *** Client Requests User ID ***");
        }

        BTA_FtsAuthRsp(btui_cfg.fts_key, "Guest");
        break;

    case BTA_FTS_ACCESS_EVT:
        LOGI("%s: BTA_FTS_ACCESS_EVT, File [%s], Size [%d], Device [%s][%02x%02x%02x%02x%02x%02x]",
             __FUNCTION__,
             p_data->access.p_name, p_data->access.size, p_data->access.dev_name,
             p_data->access.bd_addr[0], p_data->access.bd_addr[1], p_data->access.bd_addr[2],
             p_data->access.bd_addr[3], p_data->access.bd_addr[4], p_data->access.bd_addr[5]);

        btui_fts_cb.bytes_transferred = 0;

        p_btl_if_params = (tBTL_PARAMS*)calloc(1, sizeof(tBTLIF_FTPS_ACCESS_RSP_PARAM));
        if(p_btl_if_params == NULL)
        {
            LOGE("Can not allocate memory for p_btl_if_params\n");
            return;
        }

        p_btl_if_params->ftps_access_req_param.oper_code = p_data->access.oper;
        p_btl_if_params->ftps_access_req_param.size_of_file = p_data->access.size;
        memcpy(p_btl_if_params->ftps_access_req_param.bd_addr, p_data->access.bd_addr, BD_ADDR_LEN);
        memcpy(p_btl_if_params->ftps_access_req_param.bd_name, p_data->access.dev_name, sizeof(p_btl_if_params->ftps_access_req_param.bd_name));
        memcpy(p_btl_if_params->ftps_access_req_param.filename, p_data->access.p_name, strlen(p_data->access.p_name));

        // calculate real_length that need to send over
        param_data_length = sizeof(tBTLIF_FTPS_ACCESS_RSP_PARAM) - BTAPP_FTS_ACC_FNAME_MAX + strlen(p_data->access.p_name) + 1;

        BTL_IF_CtrlSend(btl_if_fts_handle, SUB_FTPS, BTLIF_FTPS_ACCESS_EVT, p_btl_if_params, param_data_length );
        free(p_btl_if_params);

        //BTA_FtsAccessRsp(p_data->access.oper, btui_fts_cb.access_flag, p_data->access.p_name);
        break;

    case BTA_FTS_OPEN_EVT:
        LOGI("%s: BTA_FTS_OPEN_EVT, bd_addr = [%02x%02x%02x%02x%02x%02x]", __FUNCTION__,
             p_data->bd_addr[0], p_data->bd_addr[1], p_data->bd_addr[2],
             p_data->bd_addr[3], p_data->bd_addr[4], p_data->bd_addr[5]);

        p_btl_if_params = (tBTL_PARAMS *) calloc(1, sizeof(tBTLIF_FTPS_OPEN_PARAM));
        if (p_btl_if_params == NULL)
        {
            LOGE("%s: Can not allocate memory for p_btl_if_params\n", __FUNCTION__);
            return;
        }
        memcpy(p_btl_if_params->ftps_open_param.bd_addr, p_data->bd_addr, BD_ADDR_LEN);

		BTL_IF_CtrlSend(btl_if_fts_handle, SUB_FTPS, BTLIF_FTPS_OPEN_EVT,
                        p_btl_if_params, sizeof(tBTLIF_FTPS_OPEN_PARAM));
        free(p_btl_if_params);
        break;

    case BTA_FTS_CLOSE_EVT:
        APPL_TRACE_EVENT0("File Transfer: Connection Closed...");
        BTL_IF_CtrlSend(btl_if_fts_handle, SUB_FTPS,  BTLIF_FTPS_CLOSE_EVT, NULL, 0 );
        break;

    case BTA_FTS_PROGRESS_EVT:
        btui_fts_cb.bytes_transferred += p_data->prog.bytes;
        if (p_data->prog.file_size != BTA_FS_LEN_UNKNOWN)
        {
            APPL_TRACE_EVENT2("File Transfer Server PROGRESS (%d of %d total)...",
                              btui_fts_cb.bytes_transferred, p_data->prog.file_size);
        }
        else
        {
            APPL_TRACE_EVENT1("File Transfer Server PROGRESS (%d bytes total)...",
                              btui_fts_cb.bytes_transferred);
        }

        /* Notify framework of progress every BTAPP_FTS_FRAMEWORK_NOTIFICATION_INTERVAL */
        {
            UINT32 tick_count_current = GKI_get_os_tick_count();
            static UINT32 tick_count_last_notification = 0;

            if (GKI_TICKS_TO_MS(tick_count_current - tick_count_last_notification) > BTAPP_FTS_FRAMEWORK_NOTIFICATION_INTERVAL)
            {
				p_btl_if_params = (tBTL_PARAMS*)calloc(1, sizeof(tBTLIF_FTPS_PROGRESS_PARAM));  // calloc suppose to set memory to 0
                if(p_btl_if_params == NULL)
                {
                    LOGE("Can not allocate memory for p_btl_if_params\n");
                    return;
                }

                p_btl_if_params->ftps_progress_param.file_size = p_data->prog.file_size ;
                p_btl_if_params->ftps_progress_param.bytes_transferred = btui_fts_cb.bytes_transferred;

                // calculate real_length that need to send over
                param_data_length = sizeof(tBTLIF_FTPS_PROGRESS_PARAM);

                BTL_IF_CtrlSend(btl_if_fts_handle, SUB_FTPS, BTLIF_FTPS_FILE_TRANSFER_IN_PRGS_CMPL_EVT,
			        p_btl_if_params, param_data_length );
                free(p_btl_if_params);
            }
        }
        break;

    case BTA_FTS_DEL_CMPL_EVT:
        APPL_TRACE_EVENT2("File Transfer: Delete Complete, status %d, [%s]",
                          p_data->obj.status, p_data->obj.p_name);
        break;

    case BTA_FTS_GET_CMPL_EVT:
        APPL_TRACE_EVENT2("File Transfer: Get File Complete, status %d, [%s]",
                          p_data->obj.status, p_data->obj.p_name);
        p_btl_if_params = (tBTL_PARAMS*)calloc(1, sizeof(tBTLIF_FTPS_OPER_CMPL_PARAM));  // calloc suppose to set memory to 0
        if(p_btl_if_params == NULL)
        {
            LOGE("Can not allocate memory for p_btl_if_params\n");
            return;
        }

        p_btl_if_params->ftps_oper_cmpl_param.oper_code = p_data->obj.status;
        memcpy(p_btl_if_params->ftps_oper_cmpl_param.file_name, p_data->obj.p_name, strlen(p_data->obj.p_name));

        // calculate real_length that need to send over
        param_data_length = sizeof(tBTLIF_FTPS_OPER_CMPL_PARAM) +1;

        BTL_IF_CtrlSend(btl_if_fts_handle, SUB_FTPS, BTLIF_FTPS_OPER_GET_CMPL_EVT,
			p_btl_if_params, param_data_length );
        free(p_btl_if_params);

        break;

    case BTA_FTS_PUT_CMPL_EVT:
        APPL_TRACE_EVENT2("File Transfer: Put File Complete, status %d, [%s]",
                          p_data->obj.status, p_data->obj.p_name);
        p_btl_if_params = (tBTL_PARAMS*)calloc(1, sizeof(tBTLIF_FTPS_OPER_CMPL_PARAM));  // calloc suppose to set memory to 0
        if(p_btl_if_params == NULL)
        {
            LOGE("Can not allocate memory for p_btl_if_params\n");
            return;
        }

        p_btl_if_params->ftps_oper_cmpl_param.oper_code = p_data->obj.status;
        memcpy(p_btl_if_params->ftps_oper_cmpl_param.file_name, p_data->obj.p_name, strlen(p_data->obj.p_name));

        // calculate real_length that need to send over
        param_data_length = sizeof(tBTLIF_FTPS_OPER_CMPL_PARAM) +1;

        BTL_IF_CtrlSend(btl_if_fts_handle, SUB_FTPS, BTLIF_FTPS_OPER_PUT_CMPL_EVT,
			p_btl_if_params, param_data_length );
        free(p_btl_if_params);
	  break;

    case BTA_FTS_DISABLE_EVT:
        APPL_TRACE_EVENT0("File Transfer: Server Disabled...");
        BTL_IF_CtrlSend(btl_if_fts_handle, SUB_FTPS,  BTLIF_FTPS_DISABLE_EVT, NULL, 0 );
        break;

    default:
        APPL_TRACE_EVENT1("File Transfer: Unknown Event (0x%02x)...", event);
    }
}

static void btapp_fts_on_rx_ctrl(tCTRL_HANDLE fd, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params)
{
    btl_if_fts_handle = fd;


    switch(id)
    {
    case BTLIF_FTPS_ENABLE:
         /*
          * make it simple now, later on we should get the parameters from java application, then call BTA_PbsEnable()
          * Or it may never need to do that
          */
         btapp_fts_on_rx_enable();
         break;
    case BTLIF_FTPS_DISABLE:
        BTA_FtsDisable();
        break;
    case BTLIF_FTPS_CLOSE:
        BTA_FtsClose();
        break;
    case BTLIF_FTPS_AUTH_RSP:
         // This is not implemented now
         break;
    case BTLIF_FTPS_ACCESS_RSP:
         // According to our BTA_API document, 0 means allowed and 1 means forbidden.
         // We don't want to have customer confused. From java, 1 means allowd and 0 means forbidden,

         APPL_TRACE_EVENT4("#### %s op_code = %d access = %d file name %s ####\n", __FUNCTION__, params->ftps_access_req_param.oper_code,
              params->ftps_access_req_param.access, params->ftps_access_req_param.filename);
         BTA_FtsAccessRsp(params->ftps_access_req_param.oper_code, params->ftps_access_req_param.access ? 0 : 1, params->ftps_access_req_param.filename);

    default:
         APPL_TRACE_DEBUG2("%s unknow command %d", __FUNCTION__, id);
         break;
    }
}


#endif

