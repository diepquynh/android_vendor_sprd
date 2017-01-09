/****************************************************************************
**  Name:          btapp_hh.c
**
**  Description:   Contains application functions for HID Host.
**
**  Copyright (c) 2009, Broadcom Corp., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include "bt_target.h"

#if (defined(BTA_HH_INCLUDED) && (BTA_HH_INCLUDED == TRUE))

#include "bd.h"
#include "bta_api.h"
#include "bta_hh_api.h"
#include "btui.h"
#include "btui_int.h"
#include "btl_ifs.h"
#include "btapp_hh.h"

#include <string.h>
#include <cutils/properties.h>

#ifdef TAG
#undef TAG
#endif
#define TAG "BTAPP_HH: "

#ifdef LOGE
#undef LOGE
#endif
#define LOGE(format, ...)  fprintf(stderr, TAG format "\n", ## __VA_ARGS__)

#ifdef LOGI
#undef LOGI
#endif
#define LOGI(format, ...)  fprintf(stdout, TAG format "\n", ## __VA_ARGS__)


/*****************************************************************************
**  External function declarations
*****************************************************************************/
extern int  scru_ascii_2_hex(char *p_ascii, int len, UINT8 *p_hex);
extern void bta_hh_co_send_dscp(tBTUI_HH_DEVICE *p_dev, int len, UINT8 *p_dscp);

/*****************************************************************************
**  Local Function prototypes
*****************************************************************************/
static void btapp_hh_cback(tBTA_HH_EVT event, tBTA_HH *p_data);
static void btapp_hh_on_rx_ctrl(tCTRL_HANDLE fd, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params);


tBTUI_HH_CB  btui_hh_cb;


/*******************************************************************************
**
** Function         btapp_hh_find_unused_dev_entry
**
** Description      Return the pointer to an unused entry in the device table.
**
** Returns          Unused device entry pointer in the device table
*******************************************************************************/
tBTUI_HH_DEVICE *btapp_hh_find_unused_dev_entry(void)
{
    UINT32 i;
    for (i = 0; i < BTUI_HH_MAX_HID; i++)
    {
        if (!btui_hh_cb.connected_hid[i].is_connected)
        {
            return &btui_hh_cb.connected_hid[i];
        }
    }
    return NULL;
}


/*******************************************************************************
**
** Function         btapp_hh_find_dev_by_handle
**
** Description      Return the device pointer of the specified devive handle
**
** Returns          Device entry pointer in the device table
*******************************************************************************/
tBTUI_HH_DEVICE *btapp_hh_find_dev_by_handle(UINT8 handle)
{
    UINT32 i;
    for (i = 0; i < BTUI_HH_MAX_HID; i++)
    {
        if (btui_hh_cb.connected_hid[i].is_connected &&
            btui_hh_cb.connected_hid[i].dev_handle == handle)
        {
            return &btui_hh_cb.connected_hid[i];
        }
    }
    return NULL;
}


/*******************************************************************************
**
** Function         btapp_hh_find_dev_by_bda
**
** Description      Return the device pointer of the specified BD_ADDR.
**
** Returns          Device entry pointer in the device table
*******************************************************************************/
static tBTUI_HH_DEVICE *btapp_hh_find_dev_by_bda(BD_ADDR bda)
{
    UINT32 i;
    for (i = 0; i < BTUI_HH_MAX_HID; i++)
    {
        if (btui_hh_cb.connected_hid[i].is_connected &&
            memcmp(btui_hh_cb.connected_hid[i].bd_addr, bda, BD_ADDR_LEN) == 0)
        {
            return &btui_hh_cb.connected_hid[i];
        }
    }
    return NULL;
}


/*******************************************************************************
**
** Function         btapp_hh_init
**
** Description      Initialises HID host
**
** Returns          void
*******************************************************************************/
void btapp_hh_init(void)
{
    tBTL_IF_Result result;
    char val[16];

    LOGI("%s", __FUNCTION__);

    /* Check property to see whether HH is enabled. */
    btui_cfg.hh_included = FALSE;
    if (property_get("ril.bt_hh_support", val, NULL))
    {
        if (strcmp(val, "true") == 0)
        {
            btui_cfg.hh_included = TRUE;
        }
    }

    if (btui_cfg.hh_included == FALSE)
    {
        return;
    }

    memset(&btui_hh_cb, 0, sizeof(tBTUI_HH_CB));

    /* Register BTL-IF interface. */
    result = BTL_IF_ServerInit();
    result = BTL_IF_RegisterSubSystem(&btui_hh_cb.btl_if_handle, SUB_HH, NULL, btapp_hh_on_rx_ctrl);

    LOGI("%s: BTL_IF_RegisterSubSystem result = %d, btl_if_handle = %d",
         __FUNCTION__, result, btui_hh_cb.btl_if_handle);

    /* Enable HH. */
    /* btui_cfg.hh_security = BTA_SEC_ENCRYPT | BTA_SEC_AUTHENTICAT; */
    btui_cfg.hh_security = BTA_SEC_NONE;
    strncpy(btui_cfg.hh_service_name, "BRCM HID Host", sizeof(btui_cfg.hh_service_name));

    BTA_HhEnable(btui_cfg.hh_security, btapp_hh_cback);
}


/*******************************************************************************
 **
 ** Function         btapp_hh_disable
 **
 ** Description      Disables HID host at BTA level. (BTL_IF
 **
 ** Returns          void
 *******************************************************************************/
void btapp_hh_disable(void)
{
    BTA_HhDisable();
} /* btapp_hh_disable() */


/*******************************************************************************
**
** Function         btapp_hh_cback
**
** Description      process the events from HH
**
** Returns          void
*******************************************************************************/
static void btapp_hh_cback(tBTA_HH_EVT event, tBTA_HH *p_data)
{
    BD_ADDR          bda;
    tBTLIF_HH_PARAM  params;
    tBTUI_HH_DEVICE  *p_dev_cb = NULL;

    switch(event) {
    case BTA_HH_ENABLE_EVT:
        LOGI("BTA_HH_ENABLE_EVT: status = %d", p_data->status);
        break;

    case BTA_HH_DISABLE_EVT:
        LOGI("BTA_HH_DISABLE_EVT:");
        break;

    case BTA_HH_OPEN_EVT:
        LOGI("BTA_HH_OPEN_EVT: status = %d, handle = %d, bda = %02x:%02x:%02x:%02x:%02x:%02x",
             p_data->conn.status, p_data->conn.handle,
             p_data->conn.bda[0], p_data->conn.bda[1], p_data->conn.bda[2],
             p_data->conn.bda[3], p_data->conn.bda[4], p_data->conn.bda[5]);

        if (p_data->conn.status == BTA_HH_OK)
        {
            p_dev_cb = btapp_hh_find_dev_by_handle(p_data->conn.handle);

            memcpy(p_dev_cb->bd_addr, p_data->conn.bda, BD_ADDR_LEN);

            BTA_HhSetIdle(p_data->conn.handle, 0);

            params.status = BTA_HH_OK;
        }
        else if (p_data->conn.status != BTA_HH_OK)
        {
            params.status = p_data->conn.status;
        }
        else
        {
            params.status = BTA_HH_ERR_DB_FULL;
        }

        memcpy(params.bd_addr, p_data->conn.bda, BD_ADDR_LEN);

        BTL_IF_CtrlSend(btui_hh_cb.btl_if_handle, SUB_HH, BTLIF_HH_OPEN_EVT,
                        (tBTL_PARAMS *) &params, sizeof(tBTLIF_HH_PARAM));
        break;

    case BTA_HH_CLOSE_EVT:
        LOGI("BTA_HH_CLOSE_EVT: dev_handle = %d", p_data->dev_status.handle);

        p_dev_cb = btapp_hh_find_dev_by_handle(p_data->dev_status.handle);
        if (p_dev_cb != NULL)
        {
            btui_hh_cb.connected_dev_num--;
            p_dev_cb->is_connected = FALSE;

            memcpy(params.bd_addr, p_dev_cb->bd_addr, BD_ADDR_LEN);
            params.status = p_data->dev_status.status;

            BTL_IF_CtrlSend(btui_hh_cb.btl_if_handle, SUB_HH, BTLIF_HH_CLOSE_EVT,
                            (tBTL_PARAMS *) &params, sizeof(tBTLIF_HH_PARAM));
        }
        else
        {
            LOGE("Oops: Can not find device with handle %d", p_data->dev_status.handle);
        }
        break;

    case BTA_HH_GET_RPT_EVT:
        LOGI("BTA_HH_GET_RPT_EVT:");
        break;

    case BTA_HH_SET_RPT_EVT:
        LOGI("BTA_HH_SET_RPT_EVT:");
        break;

    case BTA_HH_GET_PROTO_EVT:
        LOGI("BTA_HH_GET_PROTO_EVT: status = %d, handle = %d, proto = [%d], %s",
             p_data->hs_data.status, p_data->hs_data.handle,
             p_data->hs_data.rsp_data.proto_mode,
             (p_data->hs_data.rsp_data.proto_mode == BTA_HH_PROTO_RPT_MODE) ? "Report Mode" :
             (p_data->hs_data.rsp_data.proto_mode == BTA_HH_PROTO_BOOT_MODE) ? "Boot Mode" : "Unsupported");
        break;

    case BTA_HH_SET_PROTO_EVT:
        LOGI("BTA_HH_SET_PROTO_EVT: status = %d", p_data->dev_status.status);
        break;

    case BTA_HH_GET_IDLE_EVT:
        LOGI("BTA_HH_GET_IDLE_EVT: handle = %d, status =%d, rate = %d",
             p_data->hs_data.handle, p_data->hs_data.status,
             p_data->hs_data.rsp_data.idle_rate);
        break;

    case BTA_HH_SET_IDLE_EVT:
        LOGI("BTA_HH_SET_IDLE_EVT: handle = %d, status =%d",
             p_data->dev_status.handle, p_data->dev_status.status);

        btui_hh_cb.p_curr_dev = btapp_hh_find_dev_by_handle(p_data->dev_status.handle);
        BTA_HhGetDscpInfo(p_data->dev_status.handle);

        break;

    case BTA_HH_GET_DSCP_EVT:
        LOGI("BTA_HH_GET_DSCP_EVT: len = %d", p_data->dscp_info.dl_len);
        bta_hh_co_send_dscp(btui_hh_cb.p_curr_dev,
                            p_data->dscp_info.dl_len, p_data->dscp_info.dsc_list);
        break;

    case BTA_HH_ADD_DEV_EVT:
        LOGI("BTA_HH_ADD_DEV_EVT: status = %d, handle = %d",
             p_data->dev_info.status, p_data->dev_info.handle);
        break;

    case BTA_HH_RMV_DEV_EVT:
        LOGI("BTA_HH_RMV_DEV_EVT:");
        break;

    case BTA_HH_VC_UNPLUG_EVT:
        LOGI("BTA_HH_VC_UNPLUG_EVT:");
        break;

    case BTA_HH_API_ERR_EVT  :
        LOGI("BTA_HH API_ERR");
        break;

    default:
        LOGE("%s: Unknown event", __FUNCTION__);
    }
}


/*******************************************************************************
**
** Function         btapp_hh_on_rx_ctrl
**
** Description      BTL-IF control command callback function.
**
** Returns          void
*******************************************************************************/
static void btapp_hh_on_rx_ctrl(tCTRL_HANDLE fd, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params)
{
    tBTLIF_HH_PARAM  out_params;
    tBTUI_HH_DEVICE  *p_dev_cb;

    LOGI("%s: fd = %d, event = %d, address = %02X:%02X:%02X:%02X:%02X:%02X",
         __FUNCTION__, fd, id,
         params->hh_param.bd_addr[0], params->hh_param.bd_addr[1],
         params->hh_param.bd_addr[2], params->hh_param.bd_addr[3],
         params->hh_param.bd_addr[4], params->hh_param.bd_addr[5]);

    if (btui_hh_cb.btl_if_handle != fd)
    {
        LOGE("%s: btl_if_handle changed, old = %d, new = %d",
             __FUNCTION__, btui_hh_cb.btl_if_handle, fd);
        btui_hh_cb.btl_if_handle = fd;
    }

    switch (id) {
    case BTLIF_HH_OPEN:
        LOGI("BTLIF_HH_OPEN");
        if (btui_cfg.hh_included == FALSE)
        {
            LOGE("Oops: HH not enabled.");

            memcpy(out_params.bd_addr, params->hh_param.bd_addr, BD_ADDR_LEN);
            out_params.status = BTA_HH_ERR;

            BTL_IF_CtrlSend(btui_hh_cb.btl_if_handle, SUB_HH, BTLIF_HH_OPEN_EVT,
                            (tBTL_PARAMS *) &out_params, sizeof(tBTLIF_HH_PARAM));
        }
        else if (btui_hh_cb.connected_dev_num < BTUI_HH_MAX_HID) {
            BTA_HhOpen(params->hh_param.bd_addr, BTA_HH_PROTO_BOOT_MODE, BTA_SEC_NONE);
        }
        else {
            /* No space for more HID device now. */
            LOGE("Oops: Exceeded the maximum supported HID device number.");

            memcpy(out_params.bd_addr, params->hh_param.bd_addr, BD_ADDR_LEN);
            out_params.status = BTA_HH_ERR_DB_FULL;

            BTL_IF_CtrlSend(btui_hh_cb.btl_if_handle, SUB_HH, BTLIF_HH_OPEN_EVT,
                            (tBTL_PARAMS *) &out_params, sizeof(tBTLIF_HH_PARAM));
        }
        break;

    case BTLIF_HH_CLOSE:
        LOGI("BTLIF_HH_CLOSE");
        p_dev_cb = btapp_hh_find_dev_by_bda(params->hh_param.bd_addr);
        if (p_dev_cb != NULL)
        {
            BTA_HhClose(p_dev_cb->dev_handle);
        }
        else
        {
            LOGE("Oops: Device %02X:%02X:%02X:%02X:%02X:%02X not opened.",
                 params->hh_param.bd_addr[0], params->hh_param.bd_addr[1],
                 params->hh_param.bd_addr[2], params->hh_param.bd_addr[3],
                 params->hh_param.bd_addr[4], params->hh_param.bd_addr[5]);

            memcpy(out_params.bd_addr, params->hh_param.bd_addr, BD_ADDR_LEN);
            out_params.status = BTA_HH_ERR;

            BTL_IF_CtrlSend(btui_hh_cb.btl_if_handle, SUB_HH, BTLIF_HH_CLOSE_EVT,
                            (tBTL_PARAMS *) &out_params, sizeof(tBTLIF_HH_PARAM));
        }

        break;

    default:
        APPL_TRACE_DEBUG2("%s: Unknow command %d", __FUNCTION__, id);
        break;
    }
}

#endif
