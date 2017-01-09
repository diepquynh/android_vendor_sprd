/****************************************************************************
**
**  Name:          btapp_hciutils.c
**
**  Description:   Contains  functions for file transfer server
**
**
**  Copyright (c) 2003-2006, Broadcom Corp., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include "bt_target.h"

#include "gki.h"
#include "bta_api.h"
#include "btui.h"
#include "bta_fs_api.h"
#include "bta_ft_api.h"
#include "bta_fs_co.h"
#include "btui_int.h"
#include "btapp_hciutils.h"
#include "btl_ifs.h"

#define LOG_TAG "BTAPP_HCIUTILS"

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



tBTAPP_HCIUTILS_CB      g_hciutils_cb = {0};

static tCTRL_HANDLE     btl_if_hciutils_handle = 0;
pthread_mutex_t         hciutils_mutex = PTHREAD_MUTEX_INITIALIZER;
#define BTAPP_HCIUTILS_MUTEX_LOCK() pthread_mutex_lock(&hciutils_mutex);

#define BTAPP_HCIUTILS_MUTEX_UNLOCK() pthread_mutex_unlock(&hciutils_mutex);


static void btapp_hciutils_on_rx_ctrl(tCTRL_HANDLE fd, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params)
{
    APPL_TRACE_DEBUG2("%s 0x%x", __FUNCTION__, id);

    switch ( id )
    {
        case BTLIF_HCIUTILS_CMD_ENABLE:
            btapp_hciutils_enable();
            break;
        case BTLIF_HCIUTILS_CMD_DISABLE:
            btapp_hciutils_disable();
            break;
        case BTLIF_HCIUTILS_CMD_SET_AFH_CHANNELS:
            btapp_hciutils_set_afh_channels(params->hciutils_set_afh_channels.n_first,params->hciutils_set_afh_channels.n_last);
            break;
        case BTLIF_HCIUTILS_CMD_SET_AFH_CHANNEL_ASSESSMENT:
            btapp_hciutils_set_afh_channel_assessment(params->hciutils_set_afh_channel_assessment.enable_or_disable);
            break;
        case BTLIF_HCIUTILS_CMD_ADD_FILTER:
            btapp_hciutils_add_filter(params->hciutils_add_remove_filter_params.t_type, params->hciutils_add_remove_filter_params.n_opcode);
            break;
        case BTLIF_HCIUTILS_CMD_REMOVE_FILTER:
            btapp_hciutils_remove_filter(params->hciutils_add_remove_filter_params.t_type, params->hciutils_add_remove_filter_params.n_opcode);
            break;
        default:
            APPL_TRACE_DEBUG2("%s -- Got unhandled message 0x%x", __FUNCTION__, id);
            break;
    }
}

void btapp_hciutils_init(void)
{
    APPL_TRACE_DEBUG0(__FUNCTION__);

    g_hciutils_cb.fenabled = 0;
    //Register with BTL_IF subsystem
    tBTL_IF_Result result;

    result = BTL_IF_ServerInit();
    APPL_TRACE_DEBUG2("%s IFS init result = %d", __FUNCTION__, result);
    result = BTL_IF_RegisterSubSystem(&btl_if_hciutils_handle, SUB_HCIUTILS, NULL, btapp_hciutils_on_rx_ctrl);
    APPL_TRACE_DEBUG2("BTL_IF_RegisterSubSystem result = %d btl_if_fts_handle = %d", result, btl_if_hciutils_handle);

}

tBTAPP_HCIUTILS_NOTIFICATION_INFO * btapp_is_filter_registered(UINT32 t_type, UINT16 nOpCode )
{

    tBTAPP_HCIUTILS_NOTIFICATION_INFO * p_ret = 0;
    int i;

    APPL_TRACE_DEBUG0(__FUNCTION__);

    BTAPP_HCIUTILS_MUTEX_LOCK();

    for ( i = 0; i < g_hciutils_cb.n_items; i++ )
    {
        if ( ( (g_hciutils_cb.p_notification_info+i)->t_type & t_type) &&
             ( (g_hciutils_cb.p_notification_info+i)->nOpCode == nOpCode ||
                (g_hciutils_cb.p_notification_info+i)->nOpCode == BTLIF_HCIUTILS_OPCODE_ALL
             )
           )
        {
            p_ret = (g_hciutils_cb.p_notification_info+i);
            APPL_TRACE_DEBUG3("%s Found filter for type = 0x%x, opcode = 0x%x ",__FUNCTION__, t_type, nOpCode);
            break;
        }
    }
    BTAPP_HCIUTILS_MUTEX_UNLOCK();
    return p_ret;
}

void btapp_hciutils_set_afh_channels(UINT8 first, UINT8 last)
{
    APPL_TRACE_DEBUG0(__FUNCTION__);
    BTA_SetAfhChannels(first, last);
}

void btapp_hciutils_set_afh_channel_assessment(BOOLEAN enable_or_disable)
{
    APPL_TRACE_DEBUG0(__FUNCTION__);
    BTA_SetAfhChannelAssessment(enable_or_disable);
}
void btapp_hciutils_add_filter(UINT32 t_type, UINT16 n_opcode)
{
    APPL_TRACE_DEBUG3("%s type = 0x%x opcode = 0x%x", __FUNCTION__, t_type, n_opcode);
    //
    // if the filter is not present in our list add it
    //
    if ( btapp_is_filter_registered(t_type, n_opcode ) )
    {
        APPL_TRACE_DEBUG3("%s Filter already registered type = 0x%x, opcode = 0x%x ",__FUNCTION__, t_type, n_opcode);
    }
    else
    {
        BTAPP_HCIUTILS_MUTEX_LOCK();
        g_hciutils_cb.p_notification_info = realloc(g_hciutils_cb.p_notification_info, (g_hciutils_cb.n_items +1) * sizeof(tBTAPP_HCIUTILS_NOTIFICATION_INFO));
        if ( NULL == g_hciutils_cb.p_notification_info  )
        {
            APPL_TRACE_DEBUG0("Error allocating memory");
        }
        else
        {
            (g_hciutils_cb.p_notification_info + g_hciutils_cb.n_items)->t_type = t_type;
            (g_hciutils_cb.p_notification_info + g_hciutils_cb.n_items)->nOpCode = n_opcode;
            g_hciutils_cb.n_items ++;
        }
        BTAPP_HCIUTILS_MUTEX_UNLOCK();
    }

}
void btapp_hciutils_remove_filter(UINT32 t_type, UINT16 n_opcode)
{
    APPL_TRACE_DEBUG3("%s type = 0x%x opcode = 0x%x", __FUNCTION__, t_type, n_opcode);
    tBTAPP_HCIUTILS_NOTIFICATION_INFO  * pInfo = NULL, * pTemp = NULL;
    if ( (pInfo = btapp_is_filter_registered(t_type, n_opcode ) ) )
    {
        pTemp = realloc( pTemp, (g_hciutils_cb.n_items-1) * sizeof(tBTAPP_HCIUTILS_NOTIFICATION_INFO));
        if ( NULL != pTemp )
        {

            BTAPP_HCIUTILS_MUTEX_LOCK();
            int j = 0;
            int i;
            for ( i = 0; i < g_hciutils_cb.n_items; i ++ )
            {
                if ( (g_hciutils_cb.p_notification_info+i) != pInfo )
                {
                    memcpy(pTemp+j, (g_hciutils_cb.p_notification_info+i), sizeof(tBTAPP_HCIUTILS_NOTIFICATION_INFO));
                    j++;
                }
            }
            free(g_hciutils_cb.p_notification_info);
            g_hciutils_cb.p_notification_info = pTemp;
            g_hciutils_cb.n_items--;
            BTAPP_HCIUTILS_MUTEX_UNLOCK();
        }
    }

}

void btapp_hciutils_enable(void)
{
    APPL_TRACE_DEBUG0(__FUNCTION__);

    g_hciutils_cb.fenabled = 1;
}
void btapp_hciutils_disable(void)
{
    APPL_TRACE_DEBUG0(__FUNCTION__);

    g_hciutils_cb.fenabled = 0;
}

void btapp_hciutils_create_btl_params( tBTL_PARAMS	* p_params ,UINT16 n_op_code, void * p_buf, UINT8 len )
{
    p_params->hciutils_notify_params.n_opcode = n_op_code;
    p_params->hciutils_notify_params.n_len = len;
    memcpy( p_params->hciutils_notify_params.data,p_buf, len);
}

void btapp_hciutils_sendhcicommand(UINT16 n_op_code, void * p_buf, UINT8 len )
{
    tBTL_PARAMS     params = {0};

    if ( g_hciutils_cb.fenabled != 0 )
    {
        if ( btapp_is_filter_registered(BTLIF_HCIUTILS_COMMAND,n_op_code ) )
        {

            APPL_TRACE_DEBUG3("%s n_op_code = 0x%x, len=0x%x", __FUNCTION__,n_op_code, len );
            params.hciutils_notify_params.t_type = BTLIF_HCIUTILS_COMMAND;
            btapp_hciutils_create_btl_params( &params ,n_op_code, p_buf, len );
            BTL_IF_CtrlSend(btl_if_hciutils_handle,SUB_HCIUTILS,BTLIF_HCIUTILS_NOTIFY_EVT,&params,sizeof(params.hciutils_notify_params));
        }
    }
}

void btapp_hciutils_sendhcievent(UINT16 n_op_code, void * p_buf, UINT8 len )
{
    tBTL_PARAMS     params = {0};

    if ( g_hciutils_cb.fenabled != 0 )
    {
        if ( btapp_is_filter_registered(BTLIF_HCIUTILS_EVENT,n_op_code ) )
        {

            APPL_TRACE_DEBUG3("%s n_op_code = 0x%x, len=0x%x", __FUNCTION__,n_op_code, len );
            params.hciutils_notify_params.t_type = BTLIF_HCIUTILS_EVENT;
            btapp_hciutils_create_btl_params( &params ,n_op_code, p_buf, len );
            BTL_IF_CtrlSend(btl_if_hciutils_handle,SUB_HCIUTILS,BTLIF_HCIUTILS_NOTIFY_EVT,&params,sizeof(params.hciutils_notify_params));
        }
    }


}

