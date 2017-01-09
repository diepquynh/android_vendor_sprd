/****************************************************************************
**
**  Name:          btapp_pbs.c
**
**  Description:   Contains  functions for phone book access server
**
**
**  Copyright (c) 2003, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include "bt_target.h"

#if( defined BTA_PBS_INCLUDED ) && (BTA_PBS_INCLUDED == TRUE)

#include "gki.h"
#include "bta_api.h"
#include "btui.h"
#include "bta_pbs_api.h"
#include "btui_int.h"
#include "btapp_pbs.h"

#include "btl_ifs.h"
#include "dtun.h"

#define LOG_TAG "BTAPP_PBS:"

#ifndef LINUX_NATIVE
#include <cutils/log.h>
#else
#include <stdio.h>
#define LOGI(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGD(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGV(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGE(format, ...)  fprintf (stderr, LOG_TAG format"\n", ## __VA_ARGS__)
#endif


/* Root name off of the working directory */
#define PBS_ROOT_FOLDER        ""

/* Maximum path length supported by MMI */
#ifndef BTUI_PATH_LEN
#define BTUI_PATH_LEN           260
#endif

static tCTRL_HANDLE btl_if_pbs_handle;

static void btapp_pbs_on_rx_ctrl(tCTRL_HANDLE fd, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params);



/* BTUI PBS server main control block */
tBTUI_PBS_CB btui_pbs_cb;


/*****************************************************************************
**  Local Function prototypes
*****************************************************************************/
static void btui_pbs_cback(tBTA_PBS_EVT event, tBTA_PBS *p_data);



/*******************************************************************************
**
** Function         btapp_pbs_init
**
** Description      PBS initialization function.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_pbs_init(void)
{
    tBTL_IF_Result result;

    result = BTL_IF_ServerInit();
    APPL_TRACE_DEBUG2("%s IFS init result = %d", __FUNCTION__, result);
    LOGD("%s IFS init result = %d", __FUNCTION__, result);
    result = BTL_IF_RegisterSubSystem(&btl_if_pbs_handle, SUB_PBS, NULL, btapp_pbs_on_rx_ctrl);
    APPL_TRACE_DEBUG2("BTL_IF_RegisterSubSystem result = %d btl_if_pbs_handle = %d", result, btl_if_pbs_handle);
    LOGD("BTL_IF_RegisterSubSystem result = %d btl_if_pbs_handle = %d", result, btl_if_pbs_handle);



}


void btapp_pbs_on_rx_enable(void)
{

    char   *p_root_path;
    BOOLEAN start_ok = TRUE;

    APPL_TRACE_DEBUG1("%s", __FUNCTION__);
    /* Sample app always allows PUT file operations unless changed in test menu */
    btui_pbs_cb.access_flag = BTA_PBS_ACCESS_TYPE_ALLOW;

    if ((p_root_path = (char *)GKI_getbuf((UINT16)(BTUI_PATH_LEN + 1))) != NULL)
    {
        memset(p_root_path, 0, BTUI_PATH_LEN + 1);

        btui_cfg.pbs_security = BTA_SEC_NONE;
        /* Use the BTA environment if it exists, otherwise create a default root path */
        //sprintf(p_root_path, "%s\\%s", btui_cfg.root_path, PBS_ROOT_FOLDER);
        sprintf(p_root_path, "");
        btui_cfg.pbs_security = BTA_SEC_NONE;
        //btui_cfg.pbs_security = BTUI_PBS_SECURITY;
        btui_cfg.pbs_obex_auth = FALSE;
        strcpy(btui_cfg.pbs_service_name, BTUI_PBS_SERVICE_NAME);
        strcpy(btui_cfg.pbs_key, BTUI_PBS_KEY);
        strcpy(btui_cfg.pbs_realm, BTUI_PBS_REALM);

        BTA_PbsEnable(BTA_SEC_AUTHORIZE, btui_cfg.pbs_service_name,
                p_root_path, btui_cfg.pbs_obex_auth,
                (UINT8)strlen(btui_cfg.pbs_realm),
                btui_cfg.pbs_realm, btui_pbs_cback, UI_PBS_ID);
            APPL_TRACE_EVENT1("PBS SERVER enabled with Root Path [%s]", p_root_path);

        GKI_freebuf(p_root_path);
    }
}

/*******************************************************************************
**
** Function         btapp_pbs_disable
**
** Description      Disables PBS
**
**
** Returns          void
**
*******************************************************************************/
void btapp_pbs_disable(void)
{

    BTA_PbsDisable();

    btui_pbs_cb.state = BTUI_PBS_DISABLE_ST;

}



/*******************************************************************************
**
** Function         btui_pbs_cback
**
** Description      PBS UI Callback function.  Handles all PBS events for the UI
**
**
** Returns          void
**
*******************************************************************************/
static void btui_pbs_cback(tBTA_PBS_EVT event, tBTA_PBS *p_data)
{
    char buf[400];
    int param_data_length = 0;
    tBTL_PARAMS*  p_btl_if_params;  // avoid stack overflow here
    char bd_addr[20];

    APPL_TRACE_EVENT1("btui_pbs_cback : events %d received ...", event);

    switch (event)
    {
    case BTA_PBS_ENABLE_EVT:

        APPL_TRACE_EVENT0("PBS : Server Enabled...");
        btui_pbs_cb.state = BTUI_PBS_CLOSE_ST;
	  BTL_IF_CtrlSend(btl_if_pbs_handle, SUB_PBS, BTLIF_PBS_ENABLE_EVT, NULL, 0);
        break;

    case BTA_PBS_OPEN_EVT:
        btui_pbs_cb.state = BTUI_PBS_CONNECTED_ST;
        BTL_IF_CtrlSend(btl_if_pbs_handle, SUB_PBS, BTLIF_PBS_OPEN_EVT, NULL, 0);
        APPL_TRACE_EVENT1("PBS: Connection Opened, peer BD %s", p_data->open.bd_addr);
        break;

    case BTA_PBS_CLOSE_EVT:
        btui_pbs_cb.state = BTUI_PBS_CLOSE_ST;
        BTL_IF_CtrlSend(btl_if_pbs_handle, SUB_PBS, BTLIF_PBS_CLOSE_EVT, NULL, 0);
        APPL_TRACE_EVENT0("PBS: Connection Closed...");
        break;

    case BTA_PBS_AUTH_EVT:
        if (p_data->auth.p_userid)
        {
            APPL_TRACE_EVENT1("PBS: Authentication Processed for User ID [%s]...",
                              p_data->auth.p_userid);
        }
        else
            APPL_TRACE_EVENT0("PBS: Authentication Processed without client user id...");

        if (p_data->auth.userid_required)
        {
            APPL_TRACE_EVENT0("               *** Client Requests User ID ***");
        }

        BTA_PbsAuthRsp(btui_cfg.pbs_key, "Guest");
        break;

    case BTA_PBS_ACCESS_EVT:

        APPL_TRACE_EVENT2("  Name [%s], Device [%s]",
           p_data->access.p_name, p_data->access.dev_name);
       //    p_data->access.bd_addr);
       // btui_addr_str(p_data->access.bd_addr));

        p_btl_if_params = (tBTL_PARAMS*)calloc(1, sizeof(tBTLIF_PBS_ACCESS_RSP_PARAM));  // calloc suppose to set memory to 0

        if(p_btl_if_params == NULL)
        {
            LOGE("Can not allocate memory for btl_params\n");
            return;
        }
        APPL_TRACE_DEBUG2(" %s Send ACCESS_RSP  %d \n",__FUNCTION__, (int)btui_pbs_cb.access_flag);

        p_btl_if_params->pbs_access_req_param.oper_code = p_data->access.oper;


            // Send it over to JNI so that it can be reported to Java
            //  need to convert bd_addr to string
        memset(bd_addr, 0, 20);
        sprintf(bd_addr, "%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X",
                p_data->access.bd_addr[0], p_data->access.bd_addr[1], p_data->access.bd_addr[2],
                p_data->access.bd_addr[3], p_data->access.bd_addr[4], p_data->access.bd_addr[5]);
        if(p_data->access.p_name != NULL)
             memcpy(p_btl_if_params->pbs_access_req_param.filename, p_data->access.p_name, strlen(p_data->access.p_name));

        memcpy(p_btl_if_params->pbs_access_req_param.remote_bd_addr, bd_addr, 20);
        memcpy(p_btl_if_params->pbs_access_req_param.remote_bd_name, p_data->access.dev_name, strlen(p_data->access.dev_name) > 32 ? 31 : strlen(p_data->access.dev_name));
        param_data_length = sizeof(tBTLIF_PBS_ACCESS_RSP_PARAM);

        LOGD("#### PBS Access %s %s %s %d ####\n", p_btl_if_params->pbs_access_req_param.filename, p_btl_if_params->pbs_access_req_param.remote_bd_addr,
              p_btl_if_params->pbs_access_req_param.remote_bd_name, p_btl_if_params->pbs_access_req_param.oper_code);


        BTL_IF_CtrlSend(btl_if_pbs_handle, SUB_PBS, BTLIF_PBS_ACCESS_EVT, p_btl_if_params, param_data_length);
        free(p_btl_if_params);
        //BTA_PbsAccessRsp(p_data->access.oper, btui_pbs_cb.access_flag, p_data->access.p_name);
        break;

    case BTA_PBS_OPER_CMPL_EVT:
        switch (p_data->obj.operation)
        {
        case BTA_PBS_OPER_PULL_PB:
            strcpy(buf, "PBS: Pull PB ");
            break;
        case BTA_PBS_OPER_SET_PB:
            strcpy(buf, "PBS: Set Path ");
            break;
        case BTA_PBS_OPER_PULL_VCARD_LIST:
            strcpy(buf, "PBS: Pull Vcard List ");
            break;
        case BTA_PBS_OPER_PULL_VCARD_ENTRY:
            strcpy(buf, "PBS: Pull Vcard Entry ");
            break;
        default:
            //strcpy(buf, "PBS: unknown Access request ");
            break;
        }
        if (p_data->obj.status == BTA_PBS_OK)
            strcat(buf, "Successed for:");
        else
            strcat(buf, "Failed for:");
        strncat(buf, p_data->obj.p_name, sizeof(buf));
        APPL_TRACE_EVENT0(buf);
        break;

    default:
        APPL_TRACE_EVENT1("PBS: Unknown Event (0x%02x)...", event);
        break;
    }
}

static void btapp_pbs_on_rx_ctrl(tCTRL_HANDLE fd, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params)
{
    btl_if_pbs_handle = fd;

    tBTLIF_PBS_ACCESS_RSP_PARAM * p_param = (tBTLIF_PBS_ACCESS_RSP_PARAM*) params;

    switch(id)
    {
    case BTLIF_PBS_ENABLE:
         /*
          * make it simple now, later on we should get the parameters from java application, then call BTA_PbsEnable()
          * Or it may never need to do that
          */
         btapp_pbs_on_rx_enable();
         break;
    case BTLIF_PBS_DISABLE:
         // This is goog enough
         APPL_TRACE_DEBUG1("%s BTLIF_PBS_DISABLE", __FUNCTION__);
         btapp_pbs_disable();
         APPL_TRACE_DEBUG0("#### Finish btapp_pbs_disable() ####");
         break;
    case BTLIF_PBS_AUTH_RSP:
         // This is not implemented now
         break;
    case BTLIF_PBS_ACCESS_RSP:
         // According to our BTA_API document, 0 means allowed and 1 means forbidden.
         // We don't want to have customer confused. From java, 1 means allowd and 0 means forbidden,

         LOGD("#### %s op_code = %d access = %d file name %s ####\n", __FUNCTION__, p_param->oper_code,p_param->access, p_param->filename);
         BTA_PbsAccessRsp(p_param->oper_code, p_param->access ? 0 : 1, p_param->filename);

    default:
         APPL_TRACE_DEBUG2("%s unknow command %d", __FUNCTION__, id);
         break;
    }
}


#endif

