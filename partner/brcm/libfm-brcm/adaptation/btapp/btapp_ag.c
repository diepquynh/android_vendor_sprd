/****************************************************************************
**
**  Name:          btapp_ag.c
**
**  Description:   Contains application code for audio gateway
**
**
**  Copyright (c) 2002-2010, Broadcom Corp., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include "bt_target.h"
#include "gki.h"

#if( defined BTA_AG_INCLUDED ) && ( BTA_AG_INCLUDED == TRUE )

#include "bta_api.h"
#include "bta_ag_api.h"
#include "bta_ag_ci.h"
#include "bte_appl.h"
#include "btui.h"
#include "btui_int.h"
#include "btapp_ag.h"
#include "btapp_dm.h"
#include "bd.h"
#include "btl_ifs.h"
#include <stdio.h>
#include <string.h>

#ifndef BTUI_AG_DEBUG
#define BTUI_AG_DEBUG   TRUE
#endif

#if BTUI_AG_DEBUG == TRUE
static char *btui_ag_evt_str(tBTA_AG_EVT event);
#endif

/* BTUI AG main control block */
tBTUI_AG_CB btui_ag_cb;

/* Definitions for BTA_AgRegister, reconfigurable from btld.txt */
#ifndef BTAPP_AG_SERVICES
#define BTAPP_AG_SERVICES           (BTA_HSP_SERVICE_MASK | BTA_HFP_SERVICE_MASK)
#endif
#ifndef BTAPP_AG_HSP_SERVICE_NAME
#define BTAPP_AG_HSP_SERVICE_NAME   ("Headset Gateway")
#endif
#ifndef BTAPP_AG_HFP_SERVICE_NAME
#define BTAPP_AG_HFP_SERVICE_NAME   ("Handsfree Gateway")
#endif

#ifndef BTAPP_AG_SECMASK
#define BTAPP_AG_SECMASK  (BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT)
#endif

#ifndef BTAPP_AG_FEATURES
#define BTAPP_AG_FEATURES           (BTA_AG_FEAT_ECNR   | \
                                     BTA_AG_FEAT_REJECT | \
                                     BTA_AG_FEAT_ECS    | \
                                     BTA_AG_FEAT_ECC    | \
                                     BTA_AG_FEAT_EXTERR | \
                                     BTA_AG_FEAT_BTRH   | \
                                     BTA_AG_FEAT_VREC )
#endif

/* Lookup table to associate chdl with ag_handle */
typedef struct {
    BD_ADDR bd_addr;
    UINT16 ag_handle;
    BUFFER_Q data_q;
    UINT32 rf_chan;
    tDATA_HANDLE dhandle;   /* btl-if socket data handle */
    UINT8 app_id;
    BOOLEAN is_client_init;
    BOOLEAN is_local_disc_req;
    BOOLEAN is_sco_up;
    tBTA_SERVICE_ID service_id;
} tBTAPP_AG_CONN_INFO;
#define BTAPP_AG_HANDLE_INVALID ((UINT16)0xFFFF)
#define BTAPP_AG_CHDL_INVALID   ((tCTRL_HANDLE)0xFFFFFFFF)
#define BTAPP_AG_RF_CHAN_INVALID (0xFFFFFFFF)

tBTAPP_AG_CONN_INFO btapp_ag_conn_info_tbl[BTUI_AG_NUM_APP];      /* SocketId-to-ag_handle lookup table */

#define BTAPP_AG_MAX_CLIENT_LISTENERS   (2)
typedef struct {
    tCTRL_HANDLE chdl;
    UINT32 rfcomm_chan;
    BOOLEAN conn_up;    /* TRUE if headset connect is up for this listener */
    BD_ADDR bd_addr;
    UINT16 ag_handle;
} tBTAPP_AG_LISTENER_INFO;
tBTAPP_AG_LISTENER_INFO btapp_ag_listener_info_tbl[BTAPP_AG_MAX_CLIENT_LISTENERS];

tCTRL_HANDLE btapp_ag_sco_chdl = BTAPP_AG_CHDL_INVALID;         /* Handle to BTL-IF sco channel to JNI (TODO: move to control block) */
tCTRL_HANDLE btapp_ag_btlif_chdl = BTAPP_AG_CHDL_INVALID;       /* Handle to BTL-IF AG subsystem */

#define BTAPP_HFP_LISTENER_ID   0
#define BTAPP_HSP_LISTENER_ID   1


/* Local function prototypes */
void btapp_ag_on_rx_data(tDATA_HANDLE handle, char *p, int len);
void btapp_ag_on_rx_ctrl(tCTRL_HANDLE handle, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params);
void btapp_ag_on_open_evt(tBTA_AG_OPEN *p_open_data);
void btapp_ag_on_close_evt(tBTA_AG *p_data);

void btapp_ag_on_rx_sco_data(tDATA_HANDLE handle, char *p, int len);
void btapp_ag_on_rx_sco_ctrl(tCTRL_HANDLE handle, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params);
void btapp_ag_on_sco_open_evt(tBTA_AG *p_data);
void btapp_ag_on_sco_close_evt(tBTA_AG *p_data);
tBTAPP_AG_CONN_INFO *btapp_ag_handle_to_conn_info(UINT16 handle);
tBTAPP_AG_CONN_INFO *btapp_bda_to_conn_info(BD_ADDR bd_addr);

/* For testing: simulate headset */
// #define BTAPP_AG_SIMULATED_HEADSET (TRUE)

 /*******************************************************************************
**
** Function         btui_ag_cback
**
** Description      AG callback from BTA
**
**
** Returns          void
*******************************************************************************/
void btui_ag_cback(tBTA_AG_EVT event, tBTA_AG *p_data)
{
    tBTUI_BTA_MSG    *p_msg;
    UINT16            handle = 0;
    UINT8             app_id = 0;
    tBTA_AG_RES_DATA    data;
    tBTUI_CALL_DATA* call_data = NULL;

    if (!p_data)
    {
        return;
    }

    handle = p_data->hdr.handle;
    app_id = p_data->hdr.app_id;

#if BTUI_AG_DEBUG == TRUE
    APPL_TRACE_DEBUG2("BTUI ag callback: Event %d (%s)", event, btui_ag_evt_str(event));
#else
    APPL_TRACE_DEBUG1("BTUI ag callback: Event %d", event);
#endif

    switch (event)
    {
    case BTA_AG_ENABLE_EVT:
        btui_ag_cb.enabled = TRUE;
        btl_av_sco_status(FALSE);
        break;

    case BTA_AG_REGISTER_EVT:
        APPL_TRACE_DEBUG2("  handle:%d app_id:%d", handle, app_id);

        btui_ag_cb.app[app_id].handle = handle;

        /* Clear update conn_info table */
        btapp_ag_conn_info_tbl[app_id].ag_handle = handle;
        btapp_ag_conn_info_tbl[app_id].app_id = app_id;
        btapp_ag_conn_info_tbl[app_id].rf_chan = BTAPP_AG_RF_CHAN_INVALID;
        btapp_ag_conn_info_tbl[app_id].is_client_init = FALSE;
        btapp_ag_conn_info_tbl[app_id].is_local_disc_req = FALSE;
        btapp_ag_conn_info_tbl[app_id].is_sco_up = FALSE;
        break;


    case BTA_AG_OPEN_EVT:
        APPL_TRACE_DEBUG4("  handle:%d app_id:%d service_id:%d status:%d", handle,
                          app_id, p_data->open.service_id, p_data->open.status);

        if (p_data->open.status == BTA_AG_SUCCESS)
        {
#if (defined BTA_HS_INCLUDED &&(BTA_HS_INCLUDED == TRUE))
#ifdef BTUI_APP
            if((p_msg = (tBTUI_BTA_MSG *)GKI_getbuf(sizeof(tBTUI_BTA_MSG))) != NULL)
            {
                p_msg->open.hdr.event = BTUI_MMI_HS_AG_ACTIVE;
                GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, p_msg);
            }
#else
            /* TODO Android: notify that AG service level (rfcomm) connection us up */
#endif
#endif
            btui_ag_cb.app[app_id].is_open = TRUE;
            btui_ag_cb.app[app_id].bvra_active = FALSE; /* Voice Recognition starts off */
            btui_ag_cb.app[app_id].service = p_data->open.service_id;
            btui_ag_cb.app[app_id].p_dev = btapp_dm_db_get_device_info(p_data->open.bd_addr);
        }
        else
        {
            /* always inform about failure. BTA_AG_FAIL_SDP and BTA_AG_FAIL_RFCOMM are most likely due to page
             * time-out, especially SDP
             */
            APPL_TRACE_WARNING1( "btui_ag_cback(BTA_AG_OPEN_EVT::FAILED (page timeout or protocol)::status %d",
                                 p_data->open.status );
        }

        btui_ag_cb.call_handle = handle;
        btl_av_sco_status(FALSE);

#if (defined BTA_FM_INCLUDED &&(BTA_FM_INCLUDED == TRUE))
        btui_ag_cb.cur_handle = handle;

        if (btui_ag_cb.play_fm == FALSE)
#endif
        {
#ifdef BTUI_APP
            if((p_msg = (tBTUI_BTA_MSG *)GKI_getbuf(sizeof(tBTUI_BTA_MSG))) != NULL)
            {
                p_msg->open.hdr.event = BTUI_MMI_CONN_UP;
                bdcpy(p_msg->open.bd_addr, p_data->open.bd_addr);
                p_msg->open.service = p_data->open.service_id;
                p_msg->open.status = p_data->open.status;
                GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, p_msg);
            }
#else /* ANDROID */
            /* TODO Android: notify AG connection is up. */
            btapp_ag_on_open_evt((tBTA_AG_OPEN *)p_data);
#endif

        }
        break;

    case BTA_AG_CLOSE_EVT:
        APPL_TRACE_DEBUG2("  handle:%d app_id:%d", handle, app_id);

        btui_ag_cb.app[app_id].is_open = FALSE;
        btl_av_sco_status(FALSE);
#if (defined BTA_FM_INCLUDED &&(BTA_FM_INCLUDED == TRUE))
        if (btui_ag_cb.play_fm )
        {
            if (!btui_ag_cb.app[app_id].is_audio_open)
                btui_ag_cb.play_fm = FALSE;
        }
        else
#endif
#if (defined BTA_HS_INCLUDED &&(BTA_HS_INCLUDED == TRUE))
        if (!btui_ag_cb.app[BTUI_AG_ID_1].is_open &&
            !btui_ag_cb.app[BTUI_AG_ID_2].is_open)
        {   /* no active AG connection, send event to HS to enable the service */
#ifdef BTUI_APP
            if((p_msg = (tBTUI_BTA_MSG *)GKI_getbuf(sizeof(tBTUI_BTA_MSG))) != NULL)
            {
                p_msg->open.hdr.event = BTUI_MMI_HS_AG_INACT;
                GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, p_msg);
            }
#endif
        }
#endif

#ifdef BTUI_APP
        if ((p_msg = (tBTUI_BTA_MSG *)GKI_getbuf(sizeof(tBTUI_BTA_MSG))) != NULL)
        {
            p_msg->close.hdr.event = BTUI_MMI_CONN_DOWN;
            p_msg->close.service = btui_ag_cb.app[app_id].service;
            GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, p_msg);
        }
#else /* ANDROID */
        btapp_ag_on_close_evt(p_data);
#endif
        break;

    case BTA_AG_CONN_EVT:
        btl_av_sco_status(FALSE);
#if (BTM_WBS_INCLUDED == TRUE )
        APPL_TRACE_DEBUG4("  handle:%d app_id:%d features:0x%x codec:0x%x",
            handle, app_id, p_data->conn.peer_feat, p_data->conn.peer_codec);
        btui_ag_cb.peer_feat = p_data->conn.peer_feat;
        btui_ag_cb.peer_codec = p_data->conn.peer_codec;

        if((btui_cfg.ag_features & BTA_AG_FEAT_CODEC) &&
            (btui_ag_cb.peer_feat & BTA_AG_PEER_FEAT_CODEC))
        {
            if(btui_cfg.ag_sco_codec & btui_ag_cb.peer_codec)
                btui_ag_cb.sco_codec = btui_cfg.ag_sco_codec;
            else
                btui_ag_cb.sco_codec = BTA_AG_CODEC_NONE;

            BTA_AgSetCodec(btui_ag_cb.call_handle, btui_ag_cb.sco_codec);
        }
        else
        {
            btui_ag_cb.sco_codec = BTA_AG_CODEC_NONE;
        }
#else
        APPL_TRACE_DEBUG3("  handle:%d app_id:%d features:0x%x", handle, app_id, p_data->conn.peer_feat);
        btui_ag_cb.peer_feat = p_data->conn.peer_feat;
#endif

#if (defined BTA_FM_INCLUDED &&(BTA_FM_INCLUDED == TRUE))
       if ( btui_ag_cb.play_fm)
       {
            btui_ag_cb.sco_owner = 2;

            APPL_TRACE_ERROR0("sending BTUI_MMI_AG_FM_OPEN");
            BTA_AgAudioOpen(btui_ag_cb.call_handle);
       }
#endif

#ifdef BTUI_APP
        /* Need to send RING to the peer HS if we having incoming call. */
        if (btui_ag_cb.call_state == BTUI_AG_CALL_INC)
        {
            /* indicate incoming call */
            call_data = btui_ag_get_call_data_in_call_setup();
            if(call_data)
            {
                sprintf(data.str, "\"%s\"", call_data->dial_num);
            }
            else
            {
                APPL_TRACE_DEBUG0("No call in call setup to Answer");
                strcpy(data.str, "\"8584538400\"");
            }

            data.num = 129;
            BTA_AgResult(BTA_AG_HANDLE_ALL, BTA_AG_IN_CALL_RES, &data);
        }
#endif
        break;

    case BTA_AG_AUDIO_OPEN_EVT:
        APPL_TRACE_DEBUG2("  handle:%d app_id:%d", handle, app_id);
        btui_ag_cb.app[app_id].is_audio_open = TRUE;

        btl_av_sco_status(TRUE);

#if (defined BTA_FM_INCLUDED &&(BTA_FM_INCLUDED == TRUE))
        if ( btui_ag_cb.play_fm && btui_ag_cb.sco_owner == 2)
        {
#ifdef BTUI_APP
            if ((p_msg = (tBTUI_BTA_MSG *)GKI_getbuf(sizeof(tBTUI_BTA_MSG))) != NULL)
            {
                p_msg->hdr.event = BTUI_MMI_FM_BT_OPEN;
                p_msg->hdr.layer_specific = handle;
                GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, p_msg);
            }
#else /* ANDROID */
            /* TODO Android: */
#endif
        }
        else
#endif

#ifdef BTUI_APP
        if ((p_msg = (tBTUI_BTA_MSG *)GKI_getbuf(sizeof(tBTUI_BTA_MSG))) != NULL)
        {
            p_msg->hdr.event = BTUI_MMI_AG_AUDIO_OPEN;
            p_msg->hdr.layer_specific = handle;
            GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, p_msg);
        }
#else /* ANDROID */
        {
            /* TODO Android: */
            btapp_ag_on_sco_open_evt(p_data);
        }
#endif
        break;

    case BTA_AG_AUDIO_CLOSE_EVT:
        /* If voice recognition is enabled send the unsolicited disable to peer
           because this could have closed because of a SCO transfer */
        btapp_ag_disable_vr(app_id);

        APPL_TRACE_DEBUG2("aud_close:  handle:%d app_id:%d", handle, app_id);
        btui_ag_cb.app[app_id].is_audio_open = FALSE;

        btl_av_sco_status(FALSE);

#ifdef BTUI_APP
        if ((p_msg = (tBTUI_BTA_MSG *)GKI_getbuf(sizeof(tBTUI_BTA_MSG))) != NULL)
        {
            p_msg->hdr.layer_specific = handle;
#if (defined BTA_FM_INCLUDED &&(BTA_FM_INCLUDED == TRUE))
            p_msg->hdr.offset           = btui_ag_cb.play_fm;
#endif
            p_msg->hdr.event = BTUI_MMI_AG_AUDIO_CLOSE;
            GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, p_msg);
        }
#else /* ANDROID */
        /* TODO Android: */
        btapp_ag_on_sco_close_evt(p_data);
#endif

#if (defined BTA_FM_INCLUDED &&(BTA_FM_INCLUDED == TRUE))
        if (!btui_ag_cb.app[app_id].is_open)
            btui_ag_cb.play_fm = FALSE;
#endif
        break;

    default:
#ifdef BTUI_APP
        btui_platform_ag_event(event, p_data);
#else /* ANDROID */
        APPL_TRACE_DEBUG1("bta_ag_cb: unhandled event %i", event);
#endif
        break;
    }
}

/*******************************************************************************
**
** Function         btapp_ag_disable
**
** Description      Disable AG service
**
** Returns          void
*******************************************************************************/
void btapp_ag_disable(void)
{
    BTA_AgDeregister(btui_ag_cb.app[BTUI_AG_ID_1].handle);
//    BTA_AgDeregister(btui_ag_cb.app[BTUI_AG_ID_2].handle);
    BTA_AgDisable();

    btui_ag_cb.enabled = FALSE;
}

/*******************************************************************************
**
** Function         btapp_ag_enable
**
** Description      Enable AG service
**
**
** Returns          void
*******************************************************************************/
tBTA_SERVICE_MASK btapp_ag_enable(void)
{
    int i;
    char * p_service_names[BTUI_AG_NUM_APP];
    tBTA_AG_FEAT features;

    /* Register handler for subsystem AG */
    BTL_IF_RegisterSubSystem(&btapp_ag_btlif_chdl, SUB_AG, btapp_ag_on_rx_data, btapp_ag_on_rx_ctrl);
    APPL_TRACE_DEBUG2("%s: BTL_IF_RegisterSubSystem called for SUB_AG. Returned handle=0x%x", __FUNCTION__, btapp_ag_btlif_chdl);

    /* Clear socketId-to-ag_handle lookup table */
    memset (btapp_ag_conn_info_tbl, sizeof(btapp_ag_conn_info_tbl), 0);

    /* Clear listener info table */
    memset (btapp_ag_listener_info_tbl, sizeof(btapp_ag_listener_info_tbl), 0);
    for (i=0; i<BTAPP_AG_MAX_CLIENT_LISTENERS; i++)
    {
        btapp_ag_listener_info_tbl[i].rfcomm_chan = BTAPP_AG_RF_CHAN_INVALID;
    }

    /* Register handler for SCO subystem */
    BTL_IF_RegisterSubSystem(&btapp_ag_sco_chdl, SUB_SCO, btapp_ag_on_rx_sco_data, btapp_ag_on_rx_sco_ctrl);
    APPL_TRACE_DEBUG2("%s: BTL_IF_RegisterSubSystem called for SUB_SCO. Returned handle=0x%x", __FUNCTION__, btapp_ag_sco_chdl);


    /* Enable Audio gateway service. PASS_THROUGH mode: application will parse at commands */
    BTA_AgEnable(BTA_AG_PASS_THROUGH, btui_ag_cback);

    /* Register HFP services */
    p_service_names[0] = BTAPP_AG_HSP_SERVICE_NAME;
    p_service_names[1] = BTAPP_AG_HFP_SERVICE_NAME;

    features = BTAPP_AG_FEATURES;
    if (bte_appl_cfg.ag_enable_3way_conf)
    {
        features |= BTA_AG_FEAT_3WAY;
    }

    APPL_TRACE_DEBUG2("%s: features=0x%x", __FUNCTION__, features);
    BTA_AgRegister(BTAPP_AG_SERVICES, BTAPP_AG_SECMASK, features, p_service_names, BTUI_AG_ID_1);

    return (BTAPP_AG_SERVICES);
}


/*******************************************************************************
**
** Function         btapp_ag_exit_sniff_and_close
**
** Description      Close the AG service connection specified by the app_id
**                       Any sniff mode will be exited before close.
**
** Returns          void
*******************************************************************************/

void btapp_ag_exit_sniff_and_close(UINT16 ag_handle)
{
    tBTAPP_AG_CONN_INFO *p = NULL;
    BD_ADDR *p_bd;

    APPL_TRACE_EVENT1("exit sniff and close for ag hdl %d", ag_handle);

    p = btapp_ag_handle_to_conn_info(ag_handle);

    if (!p)
    {
        APPL_TRACE_ERROR1("conn info not found for ag hdl %d", ag_handle);
        return;
    }

    /* make sure we exit any sniff mode before disconnecting as some remote devices
             might end up in a non responsive state otherwise */
    bta_dm_pm_active(p->bd_addr);

    BTA_AgClose(ag_handle);
}


/*******************************************************************************
**
** Function         btapp_ag_change_spk_volume
**
** Description      Change speaker volume on the connection specified by the app_id
**
**
** Returns          void
*******************************************************************************/
void btapp_ag_change_spk_volume(UINT8 app_id, BOOLEAN increase)
{
    tBTA_AG_RES_DATA    data;

    if(increase)
    {
        /* the upper limit for volume is 15 */
        if(btui_ag_cb.app[app_id].spk_vol < 15)
            btui_ag_cb.app[app_id].spk_vol++;

        data.num = btui_ag_cb.app[app_id].spk_vol;

    }
    else
    {
        /* the lower limit for volume is 0 */
        if(btui_ag_cb.app[app_id].spk_vol > 0)
            btui_ag_cb.app[app_id].spk_vol--;

        data.num = btui_ag_cb.app[app_id].spk_vol;
        data.ok_flag = BTA_AG_OK_DONE;
    }

    BTA_AgResult(btui_ag_cb.app[app_id].handle, BTA_AG_SPK_RES, &data);

}

/*******************************************************************************
**
** Function         btapp_ag_change_spk_volume
**
** Description      Change MIC volume on the connection specified by the app_id
**
**
** Returns          void
*******************************************************************************/
void btapp_ag_change_mic_volume(UINT8 app_id,BOOLEAN increase)
{
    tBTA_AG_RES_DATA    data;

    if(increase)
    {
        /* the upper limit for volume is 15 */
        if(btui_ag_cb.app[app_id].mic_vol < 15)
            btui_ag_cb.app[app_id].mic_vol++;

        data.num = btui_ag_cb.app[app_id].mic_vol;
        data.ok_flag = BTA_AG_OK_DONE;
    }
    else
    {
        /* the upper limit for volume is 15 */
        if(btui_ag_cb.app[app_id].mic_vol > 0)
            btui_ag_cb.app[app_id].mic_vol--;

        data.num = btui_ag_cb.app[app_id].mic_vol;


    }

    BTA_AgResult(btui_ag_cb.app[app_id].handle, BTA_AG_MIC_RES, &data);


}


/*******************************************************************************
**
** Function         btapp_ag_set_audio_device_authorized
**
** Description      Bond with the device
**
**
** Returns          void
*******************************************************************************/
void btapp_ag_set_audio_device_authorized (tBTUI_REM_DEVICE * p_device_rec)
{


    /* update BTA with this information.If a device is set as trusted, BTA will
    not ask application for authorization, if authorization is required for
    AG connections */

    p_device_rec->is_trusted = TRUE;
    p_device_rec->trusted_mask |= BTA_HSP_SERVICE_MASK |BTA_HFP_SERVICE_MASK;
    btui_store_device(p_device_rec);
    btapp_dm_sec_add_device(p_device_rec);


}

/*******************************************************************************
**
** Function         btapp_ag_set_device_default_headset
**
** Description      Sets the device as default headset
**
**
** Returns          void
*******************************************************************************/
void btapp_ag_set_device_default_headset (tBTUI_REM_DEVICE * p_device_rec )
{

    int i;

    /* if any other device was set as
    default change that.*/
    for(i=0; i<BTUI_NUM_REM_DEVICE; i++)
    {
        btui_device_db.device[i].is_default = FALSE;

    }

    /* phone can try a audio connection to default
    audio device during a incoming call if no audio
    connections are active at that time */
    p_device_rec->is_default = TRUE;

    btui_store_device(p_device_rec);



}

/*******************************************************************************
**
** Function         btapp_ag_connect_device
**
** Description      Makes an HSP/HFP connection to the specified device
**
**
** Returns          void
*******************************************************************************/
BOOLEAN btapp_ag_connect_device(tBTUI_REM_DEVICE * p_device_rec, BOOLEAN is_fm)
{

    UINT16 handle;

    if (btui_ag_cb.app[BTUI_AG_ID_1].is_open == FALSE)
    {
        handle = btui_ag_cb.app[BTUI_AG_ID_1].handle;
    }
    else if (btui_ag_cb.app[BTUI_AG_ID_2].is_open == FALSE)
    {
        handle = btui_ag_cb.app[BTUI_AG_ID_2].handle;
    }
    else
    {
        /* both handles are busy, cannot make a new connection now */
        return FALSE;
    }
#if( defined BTA_FM_INCLUDED ) && ( BTA_FM_INCLUDED == TRUE )
    btui_ag_cb.play_fm = is_fm;
#endif

//    btapp_dm_DisableScanMode(); //Remove page scan management code to that was put in place to avoid coonection attempt collision since stack can now recover.

    BTA_AgOpen(handle, p_device_rec->bd_addr, btui_cfg.ag_security,
        btui_cfg.supported_services & (BTA_HSP_SERVICE_MASK | BTA_HFP_SERVICE_MASK));

    return TRUE;



}
/*******************************************************************************
**
** Function         btapp_ag_find_active_vr
**
** Description      Searches for the handle with Voice Recognition active.
**                  BTUI_AG_NUM_APP is returned if none active..
**                  Note: Assumes only one active VR at a time.
**
** Returns          void
*******************************************************************************/
UINT8 btapp_ag_find_active_vr(void)
{
    UINT8 xx;

    /* Find an active VR if any */
    for (xx = 0; xx < BTUI_AG_NUM_APP; xx++)
    {
        /* Should only be one active SCO */
        if (btui_ag_cb.app[xx].bvra_active)
        {
            break;
        }
    }

    return xx;
}

/*******************************************************************************
**
** Function         btapp_ag_disable_vr
**
** Description      Disables the voice recognition if enabled by sending an
**                  unsolicited BVRA:0 to the peer HS.
**                  BTUI_AG_NUM_APP is used to turn off any active VR peer.
**                  Note: Assumes only one active VR at a time.
**
** Returns          void
*******************************************************************************/
void btapp_ag_disable_vr(UINT8 app_id)
{
    tBTA_AG_RES_DATA  data;
    BOOLEAN           is_active = FALSE;

    if (app_id < BTUI_AG_NUM_APP && btui_ag_cb.app[app_id].bvra_active)
    {
        is_active = TRUE;   /* instance is specified and active */
    }
    /* See if disabling unspecified peer */
    else if (app_id == BTUI_AG_NUM_APP)
    {
        if ((app_id = btapp_ag_find_active_vr()) < BTUI_AG_NUM_APP)
        {
            is_active = TRUE;   /* instance is specified and active */
        }
    }

    if (is_active)
    {
        APPL_TRACE_DEBUG1("btapp_ag_disable_vr: Sending BVRA:0 for handle %d",
                          btui_ag_cb.app[app_id].handle);
        btui_ag_cb.app[app_id].bvra_active = FALSE;
        data.state = FALSE;
        BTA_AgResult(btui_ag_cb.app[app_id].handle, BTA_AG_BVRA_RES, &data);
    }
}


#if BTUI_AG_DEBUG == TRUE
static char *btui_ag_evt_str(tBTA_AG_EVT event)
{
    switch (event)
    {
    case BTA_AG_ENABLE_EVT:
        return "Enable Evt";
    case BTA_AG_REGISTER_EVT:
        return "Register Evt";
    case BTA_AG_OPEN_EVT:
        return "Open Evt";
    case BTA_AG_CLOSE_EVT:
        return "Close Evt";
    case BTA_AG_CONN_EVT:
        return "Connect Evt";
    case BTA_AG_AUDIO_OPEN_EVT:
        return "Audio Opened";
    case BTA_AG_AUDIO_CLOSE_EVT:
        return "Audio Closed";
    default:
        return "Other";
    }
}
#endif


/****************************************************************************
**
**  BTA_AG Socket Interface
**
******************************************************************************/

/*******************************************************************************
**
** Function         btapp_ag_bind_skt_to_handle
**
** Description      Find available ag_handle to associate with this socket conn id
**
** Returns          p_conn_info that is binded to chdl
*******************************************************************************/
tBTAPP_AG_CONN_INFO *btapp_ag_bind_skt_to_handle(UINT32 rfcomm_chan, BD_ADDR bd_addr)
{
    UINT8 i;
    UINT16 ag_handle = BTAPP_AG_HANDLE_INVALID;
    tBTAPP_AG_CONN_INFO *p_conn_info = NULL;

    APPL_TRACE_EVENT1("btapp_ag_bind_skt_to_handle rfcomm_chan %d\n", rfcomm_chan);

    /* Look for an available ag_handle to bind chdl */
    for (i=0; i<BTUI_AG_NUM_APP; i++)
    {
        if (btapp_ag_conn_info_tbl[i].rf_chan == BTAPP_AG_RF_CHAN_INVALID)
        {
            APPL_TRACE_EVENT2("btapp_ag_bind_skt_to_handle rfcomm_chan %d at index %d\n", rfcomm_chan, i);
            btapp_ag_conn_info_tbl[i].rf_chan = rfcomm_chan;
            btapp_ag_conn_info_tbl[i].is_local_disc_req = FALSE;
            btapp_ag_conn_info_tbl[i].is_sco_up = FALSE;
            bdcpy(btapp_ag_conn_info_tbl[i].bd_addr, bd_addr);
            ag_handle = btapp_ag_conn_info_tbl[i].ag_handle;
            p_conn_info = &btapp_ag_conn_info_tbl[i];

            GKI_init_q(&btapp_ag_conn_info_tbl[i].data_q);
            btapp_ag_conn_info_tbl[i].dhandle = BTAPP_AG_HANDLE_INVALID;
            break;
        }
    }

    return (p_conn_info);
}

/*******************************************************************************
**
** Function         btapp_ag_unbind_skt_to_handle
**
** Description      remove binding of chdl from ag_handle
**
** Returns          void
*******************************************************************************/
void btapp_ag_unbind_skt_to_handle(UINT32 rf_chan)
{
    UINT8 i;
    UINT16 ag_handle = BTAPP_AG_HANDLE_INVALID;
    BT_HDR *p_buf;

    for (i=0; i<BTUI_AG_NUM_APP; i++)
    {
        if (btapp_ag_conn_info_tbl[i].rf_chan == rf_chan)
        {
            APPL_TRACE_DEBUG2("btapp_ag: unbinding rf_chan 0x%x from ag_handle %i", rf_chan, btapp_ag_conn_info_tbl[i].ag_handle);
            btapp_ag_conn_info_tbl[i].rf_chan = BTAPP_AG_RF_CHAN_INVALID;

            /* Free any buffers not sent */
            while ((p_buf = (BT_HDR *)GKI_dequeue(&btapp_ag_conn_info_tbl[i].data_q)) != NULL)
            {
                GKI_freebuf(p_buf);
            }
        }
    }
}

/*******************************************************************************
**
** Function         btapp_ag_skt_to_handle
**
** Description      Return ag_handle assoicated with given socket conn id
**
** Returns          AG handle
*******************************************************************************/
UINT16 btapp_ag_skt_to_handle(UINT32 rf_chan)
{
    UINT8 i;
    UINT16 ag_handle = BTAPP_AG_HANDLE_INVALID;

    for (i=0; i<BTUI_AG_NUM_APP; i++)
    {
        if (btapp_ag_conn_info_tbl[i].rf_chan == rf_chan)
        {
            ag_handle = btapp_ag_conn_info_tbl[i].ag_handle;
        }
    }

    return (ag_handle);
}


/*******************************************************************************
**
** Function         btapp_ag_data_handle_to_ag_handle
**
** Description      Return ag_handle assoicated with given data handle
**
** Returns          AG handle
*******************************************************************************/
UINT16 btapp_ag_data_handle_to_ag_handle(tDATA_HANDLE data_handle)
{
    UINT8 i;
    UINT16 ag_handle = BTAPP_AG_HANDLE_INVALID;

    for (i=0; i<BTUI_AG_NUM_APP; i++)
    {
        if (btapp_ag_conn_info_tbl[i].dhandle == data_handle)
        {
            ag_handle = btapp_ag_conn_info_tbl[i].ag_handle;
        }
    }

    return (ag_handle);
}



/*******************************************************************************
**
** Function         btapp_ag_handle_to_rfcomm_chan
**
** Description      Return rfcomm_chan assoicated with given ag_handle
**
** Returns          socket conn id
*******************************************************************************/
UINT32 btapp_ag_handle_to_rf_chan(UINT16 handle)
{
    UINT8 i;
    UINT32 rf_chan = BTAPP_AG_RF_CHAN_INVALID;

    for (i=0; i<BTUI_AG_NUM_APP; i++)
    {
        if (btapp_ag_conn_info_tbl[i].ag_handle == handle)
        {
            rf_chan = btapp_ag_conn_info_tbl[i].rf_chan;
        }
    }

    return (rf_chan);
}

/*******************************************************************************
**
** Function         btapp_ag_handle_to_conn_info
**
** Description      Return tBTAPP_AG_CONN_INFO struct assoicated with given ag_handle
**
** Returns          p_conn_info or NULL
*******************************************************************************/
tBTAPP_AG_CONN_INFO *btapp_ag_handle_to_conn_info(UINT16 handle)
{
    UINT8 i;
    tBTAPP_AG_CONN_INFO *p_conn_info = NULL;

    for (i=0; i<BTUI_AG_NUM_APP; i++)
    {
        if (btapp_ag_conn_info_tbl[i].ag_handle == handle)
        {
            p_conn_info = &btapp_ag_conn_info_tbl[i];
        }
    }

    return (p_conn_info);
}

/*******************************************************************************
**
** Function         btapp_bda_to_conn_info
**
** Description      Return tBTAPP_AG_CONN_INFO struct assoicated with given bda
**
** Returns          p_conn_info or NULL
*******************************************************************************/
tBTAPP_AG_CONN_INFO *btapp_bda_to_conn_info(BD_ADDR bd_addr)
{
    UINT8 i;
    tBTAPP_AG_CONN_INFO *p_conn_info = NULL;

    for (i=0; i<BTUI_AG_NUM_APP; i++)
    {
        if ((btapp_ag_conn_info_tbl[i].rf_chan != BTAPP_AG_RF_CHAN_INVALID) && (bdcmp(btapp_ag_conn_info_tbl[i].bd_addr, bd_addr) == 0))
        {
            p_conn_info = &btapp_ag_conn_info_tbl[i];
            break;
        }
    }

    return (p_conn_info);
}

/*******************************************************************************
**
** Function         btapp_ag_get_active_conn_info
**
** Description      Return active conn_info struct
**
** Returns          p_conn_info or NULL
*******************************************************************************/
tBTAPP_AG_CONN_INFO *btapp_ag_get_active_conn_info(void)
{
    UINT8 i;
    tBTAPP_AG_CONN_INFO *p_conn_info = NULL;

    for (i=0; i<BTUI_AG_NUM_APP; i++)
    {
        if (btapp_ag_conn_info_tbl[i].rf_chan != BTAPP_AG_RF_CHAN_INVALID)
        {
            p_conn_info = &btapp_ag_conn_info_tbl[i];
            break;
        }
    }

    return (p_conn_info);
}

/*******************************************************************************
**
** Function         btapp_ag_get_conn_info_sco
**
** Description      Get AG connection info that has active SCO
**
** Returns          p_conn_info or NULL
*******************************************************************************/
tBTAPP_AG_CONN_INFO *btapp_ag_get_conn_info_sco(void)
{
    UINT8 i;
    tBTAPP_AG_CONN_INFO *p_conn_info = NULL;

    for (i=0; i<BTUI_AG_NUM_APP; i++)
    {
        if (btapp_ag_conn_info_tbl[i].is_sco_up)
        {
            p_conn_info = &btapp_ag_conn_info_tbl[i];
            break;
        }
    }

    return (p_conn_info);
}

/*******************************************************************************
**
** Function         btapp_ag_store_client_listening_hdl
**
** Description
**
** Returns          void
*******************************************************************************/
BOOLEAN btapp_ag_store_client_listening_hdl(tCTRL_HANDLE chdl, UINT16 rfcomm_chan)
{
    int i;
    BOOLEAN retval = FALSE;

    for (i=0; i<BTAPP_AG_MAX_CLIENT_LISTENERS; i++)
    {
        if (btapp_ag_listener_info_tbl[i].rfcomm_chan == BTAPP_AG_RF_CHAN_INVALID)
        {
            APPL_TRACE_DEBUG3("Stored listener handle 0x%x rfcomm_chan %i (listener idx=%i)", chdl, rfcomm_chan, i);

            btapp_ag_listener_info_tbl[i].chdl = chdl;
            btapp_ag_listener_info_tbl[i].rfcomm_chan = rfcomm_chan;
            btapp_ag_listener_info_tbl[i].conn_up = FALSE;
            retval = TRUE;
            break;
        }
    }

    if (retval == FALSE)
    {
        APPL_TRACE_ERROR2("Error: unable to store listener handle 0x%x rfcomm_chan %i", chdl, rfcomm_chan);
    }


    return (retval);
}

/*******************************************************************************
**
** Function         btapp_ag_handle_to_listener_info
**
** Description      Get pointer to listener info given bta_ag handle
**
** Returns          tBTAPP_AG_LISTENER_INFO *
*******************************************************************************/
tBTAPP_AG_LISTENER_INFO *btapp_ag_handle_to_listener_info(UINT16 ag_handle)
{
    int i;
    tBTAPP_AG_LISTENER_INFO *p_listener = NULL;

    for (i=0; i<BTAPP_AG_MAX_CLIENT_LISTENERS; i++)
    {
        if (btapp_ag_listener_info_tbl[i].ag_handle == ag_handle)
        {
            p_listener = &btapp_ag_listener_info_tbl[i];
            break;
        }
    }

    return (p_listener);
}

/*******************************************************************************
**
** Function         btapp_ag_handle_outgoing_connection_failure
**
** Description      Send connection_failed notification to JNI,
**                  perform clean up.
**
** Returns          void
*******************************************************************************/
void btapp_ag_handle_outgoing_connection_failure(UINT16 ag_handle)
{
    UINT32 rf_chan;
    tBTAPP_AG_CONN_INFO *p_conn_info;

    APPL_TRACE_DEBUG1("Unable to open connection to headset (ag_handle=%i)", ag_handle);

    /* Get conn info associated with this handle */
    if ((p_conn_info = btapp_ag_handle_to_conn_info(ag_handle))!= NULL)
    {
        BTLIF_AG_ConnectRsp(btapp_ag_btlif_chdl, BTL_IF_CONNECTION_FAILED, 0);
    }

    /* Clean up btl_if (remove chdl associated with this ag_handle) */
    rf_chan = btapp_ag_handle_to_rf_chan(ag_handle);
    btapp_ag_unbind_skt_to_handle(rf_chan);
}


/*******************************************************************************
**
** Function         btapp_ag_service_already_connected
**
** Description      Returns true if connected to the service passed in
**
** Returns          void
*******************************************************************************/
BOOLEAN btapp_ag_service_already_connected(BD_ADDR bd_addr, tBTA_SERVICE_ID ser_id)
{
    tBTAPP_AG_CONN_INFO *p_conn_info;
    if ((p_conn_info = btapp_bda_to_conn_info(bd_addr))== NULL)
        return FALSE;
    return (p_conn_info->service_id == ser_id)? TRUE:FALSE;
}


/*******************************************************************************
**
** Function         btapp_ag_on_open_evt
**
** Description      Handle hsp/hfp open event
**
** Returns          void
*******************************************************************************/
void btapp_ag_on_open_evt(tBTA_AG_OPEN *p_open_evt)
{
    UINT16 ag_handle;
    tBTAPP_AG_CONN_INFO *p_conn_info;
    tBTAPP_AG_LISTENER_INFO *p_listener;
    tBTL_PARAMS params;
    BOOLEAN is_incoming = TRUE;

    APPL_TRACE_DEBUG2("%s: status=%i", __FUNCTION__, p_open_evt->status);

    ag_handle = p_open_evt->hdr.handle;
//    btapp_dm_RecoverScanMode(); //Remove page scan management code to that was put in place to avoid coonection attempt collision since stack can now recover.

    if (p_open_evt->status == BTA_AG_SUCCESS)
    {
        /* Get conn info associated with this handle */
        if (((p_conn_info = btapp_ag_handle_to_conn_info(ag_handle))!= NULL) &&
            (p_conn_info->rf_chan != BTAPP_AG_RF_CHAN_INVALID))
        {
            /* Check if this is the connection that we initiated */
            /* Note: Low-order byte of UUID is used as rf_chann during GetRemoteSCN - since BTA does not expose rfcomm chan to the application */
            if ((bdcmp(p_open_evt->bd_addr, p_conn_info->bd_addr) !=0) ||
                ((p_open_evt->service_id == BTA_HFP_SERVICE_ID) && (p_conn_info->rf_chan != (UUID_SERVCLASS_HF_HANDSFREE & 0xFF))) ||
                ((p_open_evt->service_id == BTA_HSP_SERVICE_ID) && (p_conn_info->rf_chan != (UUID_SERVCLASS_HEADSET & 0xFF))))
            {
                /* Race condition detected: connection initiated + accepted on the same ag_handle   */
                /*                          (BDA or service mismatch)                               */

                /* Cancel outgoing connection, and allow incoming connection to proceed */

                APPL_TRACE_DEBUG1("Cancelling outgoing connection on handle %i", ag_handle);
                btapp_ag_handle_outgoing_connection_failure(ag_handle);

                is_incoming = TRUE;
            }
            else
            {
                /* This is the connection that we initiated */
                is_incoming = FALSE;
                p_conn_info->service_id= p_open_evt->service_id;

                /* Notify BTL-IF that ag connection is up */
                APPL_TRACE_DEBUG1("Calling BTL_IF_SetupListener. rf_chan=%i", p_conn_info->rf_chan);

                /* data channel notification will come through ctrl callback */
                BTL_IF_SetupListener(btapp_ag_btlif_chdl, SUB_AG, p_conn_info->rf_chan);

                /* Notify BTL-IF that ag connection is up */
                APPL_TRACE_DEBUG2("Notifying btl-f that ag connection is up. ag_handle=%i, chdl=0x%x", ag_handle, btapp_ag_btlif_chdl);

                BTLIF_AG_ConnectRsp(btapp_ag_btlif_chdl, BTL_IF_SUCCESS, 0);
            }
        }


        if (is_incoming)
        {
            APPL_TRACE_DEBUG1("Incoming connection for ag_handle %i", ag_handle);

            /* Connection is headset initiated. Find available listener socket */
            if ((p_open_evt->service_id == BTA_HFP_SERVICE_ID) && (!btapp_ag_listener_info_tbl[BTAPP_HFP_LISTENER_ID].conn_up))
            {
                p_listener = &btapp_ag_listener_info_tbl[BTAPP_HFP_LISTENER_ID];
                APPL_TRACE_DEBUG3("Using HFP listener handle for incoming hs connection (chdl=0x%x, rfchan=%i, ag_handle=%i)",
                    p_listener->chdl, p_listener->rfcomm_chan, ag_handle);
            }
            else if ((p_open_evt->service_id == BTA_HSP_SERVICE_ID) && (!btapp_ag_listener_info_tbl[BTAPP_HSP_LISTENER_ID].conn_up))
            {
                p_listener = &btapp_ag_listener_info_tbl[BTAPP_HSP_LISTENER_ID];
                APPL_TRACE_DEBUG3("Using HSP listener handle for incoming hs connection (chdl=0x%x, rfchan=%i, ag_handle=%i)",
                    p_listener->chdl, p_listener->rfcomm_chan, ag_handle);
            }
            else
            {
                APPL_TRACE_ERROR0("Error: unable to find available listener for incoming hs connection. Closing connection...");
                btapp_ag_exit_sniff_and_close(ag_handle);
                return;
            }

            /* Save info into listener control block */
            p_listener->conn_up = TRUE;
            p_listener->ag_handle = ag_handle;
            bdcpy(p_listener->bd_addr, p_open_evt->bd_addr);

            /* bind this chdl to p_conn_info */
            if ((p_conn_info = btapp_ag_bind_skt_to_handle(p_listener->rfcomm_chan, p_open_evt->bd_addr)) != NULL)
            {
                p_conn_info->is_client_init = FALSE;
                p_conn_info->rf_chan = p_listener->rfcomm_chan;
                p_conn_info->service_id= p_open_evt->service_id;
                bdcpy(p_conn_info->bd_addr, p_open_evt->bd_addr);

                /* Notify BTL-IF that ag connection is up */
                APPL_TRACE_DEBUG1("Calling BTLIF_AG_ConnectInd. rf_chan=%i", p_listener->rfcomm_chan);
                APPL_TRACE_DEBUG6("    BD_ADDR=%02x:%02x:%02x:%02x:%02x:%02x",
                    p_open_evt->bd_addr[0], p_open_evt->bd_addr[1],
                    p_open_evt->bd_addr[2], p_open_evt->bd_addr[3],
                    p_open_evt->bd_addr[4], p_open_evt->bd_addr[5]);

                BTLIF_AG_ConnectInd(p_listener->chdl, (BD_ADDR*)p_open_evt->bd_addr, p_listener->rfcomm_chan);

            }
        }
    }
    else
    {
        btapp_ag_handle_outgoing_connection_failure(ag_handle);
    }
}

/*******************************************************************************
**
** Function         btapp_ag_on_close_evt
**
** Description      Handle hsp/hfp close event
**
** Returns          void
*******************************************************************************/
void btapp_ag_on_close_evt(tBTA_AG *p_data)
{
    UINT16 ag_handle;
    tCTRL_HANDLE chdl;
    tBTAPP_AG_CONN_INFO *p_conn_info;
    tBTAPP_AG_LISTENER_INFO *p_listener;

    ag_handle = p_data->hdr.handle;

    /* Get conn info associated with this handle */
    if ((p_conn_info = btapp_ag_handle_to_conn_info(ag_handle))!= NULL)
    {
        APPL_TRACE_DEBUG2("%s: exit sniff mode before closing ag_handle %i", __FUNCTION__, ag_handle);
        bta_dm_pm_active(p_conn_info->bd_addr);

        APPL_TRACE_DEBUG4("%s: ag_handle %i closed. Clean up chnd=0x%x dhnd=0x%x", __FUNCTION__, ag_handle, btapp_ag_btlif_chdl, p_conn_info->dhandle);

        /* If remote inititated, then free up the clients listening socket */
        if (!p_conn_info->is_client_init)
        {
            if ((p_listener = btapp_ag_handle_to_listener_info(ag_handle)) != NULL)
            {
                p_listener->conn_up = FALSE;
            }
            else
            {
                APPL_TRACE_ERROR2("%s: error: could not find listener_info for ag_handle %i", __FUNCTION__, ag_handle);
            }
        }

        if (p_conn_info->is_local_disc_req)
        {
            /* Notify client that connection is down */
            BTLIF_AG_DisconnectRsp(btapp_ag_btlif_chdl, p_conn_info->rf_chan);
            p_conn_info->is_local_disc_req = FALSE;
        }
        else
        {
            /* Notify client that connection is down */
            BTLIF_AG_DisconnectInd(btapp_ag_btlif_chdl, p_conn_info->rf_chan);

            /* Close data path */
            BTL_IF_DisconnectDatapath(p_conn_info->dhandle);
        }

        /* Unbind chdl from conn_info */
        btapp_ag_unbind_skt_to_handle(p_conn_info->rf_chan);
    }
    else
    {
        APPL_TRACE_ERROR2("%s: error: unable to find chdl for ag_handle %i.", __FUNCTION__, ag_handle);
    }
}

/*******************************************************************************
**
** Function         btapp_ag_on_skt_disconnect
**
** Description      Called when JNI closes socket for BTPROTO_RFCOMM
**
** Returns          void
*******************************************************************************/
void btapp_ag_on_skt_disconnect(void *p_data)
{
    UINT16 ag_handle;
    tCTRL_HANDLE rfcomm_chan = 0;

    /* Get ag handle for this socket ID */
    //TODO: chdl = EXTRACT_SKT_ID(p_data);
    ag_handle = btapp_ag_skt_to_handle(rfcomm_chan);

    /* Remove socket ID mapping for this AG handle */
    btapp_ag_unbind_skt_to_handle(rfcomm_chan);

    /* Close connection */
    btapp_ag_exit_sniff_and_close(ag_handle);
}


/*******************************************************************************
**
** Function         btapp_ag_on_rx_data
**
** Description      Called when data is received over socket
**
** Returns          void
*******************************************************************************/
void btapp_ag_on_rx_data(tDATA_HANDLE dhdl, char *p, int len)
{
    UINT16 ag_handle;

    /* Get ag handle for this socket ID */
    ag_handle = btapp_ag_data_handle_to_ag_handle(dhdl);

    APPL_TRACE_DEBUG3("btapp_ag_on_rx_data: (dhdl=0x%x) writing %i bytes to ag_handle %i", dhdl, len, ag_handle);

    /* Send the data */
    bta_ag_ci_rx_write(ag_handle, p, len);
}



/*******************************************************************************
**
** Function         btapp_ag_on_tx_data
**
** Description      Called when data is received from remote headset
**
** Returns          void
*******************************************************************************/
void btapp_ag_on_tx_data(UINT16 ag_handle, UINT8 * p_data, UINT16 len)
{
    tBTAPP_AG_CONN_INFO *p_conn_info;
    BT_HDR *p_buf;
    const char at_bvra[] = "AT+BVRA";
    const char at_error_rsp[] = "\r\nERROR\r\n";

    /* Send data over socket to the Phone application */
    APPL_TRACE_DEBUG1("btapp_ag_on_tx_data ag handle:%d", ag_handle);

    /* Android does not support VR; return ERROR result code here */
    if (!(BTAPP_AG_FEATURES & BTA_AG_FEAT_VREC) && strncmp((char *)p_data, at_bvra, strlen(at_bvra)) == 0)
    {
        APPL_TRACE_DEBUG0("AT+BVRA not supported.");
        bta_ag_ci_rx_write(ag_handle, (char *)at_error_rsp, strlen(at_error_rsp));
        return;
    }


    /* Find control block for this socket handle */
    if ((p_conn_info = btapp_ag_handle_to_conn_info(ag_handle)) != NULL)
    {
        /* Check if data socket is up yet */
        if (p_conn_info->dhandle ==  BTAPP_AG_HANDLE_INVALID)
        {
            APPL_TRACE_DEBUG0("Data channel not up yet. Enqueuing...");
            if ((p_buf = GKI_getbuf(len)) != NULL)
            {
                p_buf->offset=0;
                p_buf->len = len;
                memcpy((char *)(p_buf+1), (char *)p_data, len);
                GKI_enqueue(&p_conn_info->data_q, p_buf);
            }
            else
            {
                APPL_TRACE_ERROR0("Unable to enqueue tx_data (no buffers)");
            }
        }
        else
        {
            APPL_TRACE_DEBUG2("%s calling BTL_IF_SendData dhandle=0x%x", __FUNCTION__, p_conn_info->dhandle);
            BTL_IF_SendData(p_conn_info->dhandle, (char *)p_data, len);
        }
    }
    else
    {
        APPL_TRACE_ERROR1("Unable to find conn info for ag_handle %x", ag_handle);
    }
}


/*******************************************************************************
**
** Function         btapp_ag_on_rx_ctrl
**
** Description
**
** Returns          void
*******************************************************************************/
void btapp_ag_on_rx_ctrl(tCTRL_HANDLE fd, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params)
{
    UINT16 ag_handle;
    tBTAPP_AG_CONN_INFO *p_conn_info;
    BT_HDR *p_buf;
    BD_ADDR bd_addr;
    char * p_service_names[BTUI_AG_NUM_APP];

    APPL_TRACE_DEBUG2("ag_rx_ctrl hdl=%i msg id=%i", fd, id);

    switch (id)
    {
    case BTLIF_CONNECT_REQ:
        APPL_TRACE_DEBUG1("\tReceived AG config :: rf_chan %d", params->ag_conreq.rf_chan);

        /* Reverse bdaddr */
        bd_addr[5] = params->ag_conreq.bd[0];
        bd_addr[4] = params->ag_conreq.bd[1];
        bd_addr[3] = params->ag_conreq.bd[2];
        bd_addr[2] = params->ag_conreq.bd[3];
        bd_addr[1] = params->ag_conreq.bd[4];
        bd_addr[0] = params->ag_conreq.bd[5];

        APPL_TRACE_DEBUG6("\t   bd [%02x:%02x:%02x:%02x:%02x:%02x]", bd_addr[0],  bd_addr[1], bd_addr[2],  bd_addr[3], bd_addr[4],  bd_addr[5]);

        /* Bind socket fd/bda to a bta_ag connection handle */
        if ((p_conn_info = btapp_ag_bind_skt_to_handle((UINT32)params->ag_conreq.rf_chan, bd_addr)) != NULL)
        {
            p_conn_info->is_client_init = TRUE;
            ag_handle = p_conn_info->ag_handle;
            p_conn_info->rf_chan = params->ag_conreq.rf_chan;

            APPL_TRACE_DEBUG2("btapp_ag: binding skt_fd 0x%x to ag_handle %i", fd, ag_handle);
        }
        else
        {
            /* Could not find an available conn handle for this socket */
            APPL_TRACE_ERROR1("btapp_ag: no available ag_handle to bind skt_fd 0x%x", fd);
            BTLIF_AG_ConnectRsp(btapp_ag_btlif_chdl, BTL_IF_CONNECTION_FAILED, 0);
            return;
        }

        /* Initiate the connection */
        BTA_AgOpen( ag_handle,
                    bd_addr,
                    BTM_SEC_NONE,
                    BTA_HSP_SERVICE_MASK | BTA_HFP_SERVICE_MASK);
        break;

    case BTLIF_DISCONNECT_REQ:
        APPL_TRACE_DEBUG1("Received BTLIF_DISCONNECT_REQ :: chdl=0x%x", fd);

        if ((p_conn_info = btapp_ag_get_active_conn_info()) != NULL)
        {
            p_conn_info->is_local_disc_req = TRUE;

            btapp_ag_exit_sniff_and_close(p_conn_info->ag_handle);
        }
        else
        {
            APPL_TRACE_ERROR1("\tCould not find conn_info for chdl=0x%x", fd);
        }
        break;



    case BTLIF_DATA_CHAN_IND:
        APPL_TRACE_DEBUG2("\tReceived BTLIF_DATA_CHAN_IND :: chdl=0x%x dhdl 0x%x", fd, params->chan_ind.handle);

        /* Find control block for this socket handle */
        if ((p_conn_info = btapp_ag_get_active_conn_info()) != NULL)
        {
            p_conn_info->dhandle = params->chan_ind.handle;

            /* Check if anything enqueued */
            while ((p_buf = (BT_HDR *)GKI_dequeue(&p_conn_info->data_q)) != NULL)
            {
                APPL_TRACE_DEBUG1("Dequeuing and sending %i bytes", p_buf->len);
                BTL_IF_SendData(p_conn_info->dhandle, (char *)(p_buf+1), p_buf->len);
                GKI_freebuf(p_buf);
            }
        }
        else
        {
            APPL_TRACE_ERROR1("Unable to find conn info for btl-if chdl %x", fd);
        }
        break;



    case BTLIF_LISTEN_REQ:
        APPL_TRACE_DEBUG2("\tReceived listen req :: chdl=%x rf_chan %d", fd, params->ag_listen.rf_chan);

        /* Save chdl for incoming headset connectons */
        btapp_ag_store_client_listening_hdl(fd, params->ag_listen.rf_chan);

        /* set visibility and connectability */
        BTA_DmSetVisibility(BTA_DM_NON_DISC, BTA_DM_CONN);

        break;

    case BTLIF_CONNECT_IND_ACK:
        /* now connect data channel */

        if ((p_conn_info = btapp_bda_to_conn_info(params->ag_conreq.bd)) != NULL)
        {
            BTL_IF_ConnectDatapath(btapp_ag_btlif_chdl, &p_conn_info->dhandle, SUB_AG, params->ag_conreq.rf_chan);
            APPL_TRACE_DEBUG1("BTL_IF_ConnectDatapath returned with dhdl=0x%x", p_conn_info->dhandle );

            /* Check if anything enqueued from the time when we sent connect ind to when we got the ack */
            while ((p_buf = (BT_HDR *)GKI_dequeue(&p_conn_info->data_q)) != NULL)
            {
                APPL_TRACE_DEBUG1("Dequeuing and sending %i bytes", p_buf->len);
                BTL_IF_SendData(p_conn_info->dhandle, (char *)(p_buf+1), p_buf->len);
                GKI_freebuf(p_buf);
            }
        }
        else
        {
            APPL_TRACE_DEBUG0("BTLIF_CONNECT_IND_ACK : Couldn't find conn info" );
        }
        break;

    default:
        break;
    }
}

/* if already connecting but not yet authenticated, disconnect previous connection (S9 pairing issue) */
void btapp_remove_ag_conn_attempt( BD_ADDR bda )
{
    tBTAPP_AG_CONN_INFO *p_conn_info;

    APPL_TRACE_DEBUG0( "Check for Authentication while connecting the headse" );
    if ((p_conn_info = btapp_bda_to_conn_info(bda)) != NULL)
    {
        APPL_TRACE_DEBUG1( "CLOSE the AG conn: ag_handle: %d", p_conn_info->ag_handle );
        btapp_ag_exit_sniff_and_close(p_conn_info->ag_handle);
    }
}


/*******************************************************************************
** SCO
*******************************************************************************/

/*******************************************************************************
**
** Function         btapp_ag_on_sco_open_evt
**
** Description      Handle sco open event
**
** Returns          void
*******************************************************************************/
void btapp_ag_on_sco_open_evt(tBTA_AG *p_data)
{
    UINT16 ag_handle;
    tBTAPP_AG_CONN_INFO *p_conn_info;
    tBTL_PARAMS params;

    APPL_TRACE_DEBUG1("%s", __FUNCTION__);

    ag_handle = p_data->hdr.handle;

    /* Get conn info associated with this handle */
    if (((p_conn_info = btapp_ag_handle_to_conn_info(ag_handle))!= NULL) &&
        (p_conn_info->rf_chan != BTAPP_AG_RF_CHAN_INVALID))
    {
        /* Set SCO_UP flag for this connection */
        p_conn_info->is_sco_up = TRUE;

        /* Notify BTL-IF that ag connection is up */
        APPL_TRACE_DEBUG3("%s: Notifying btl-f that sco connection is up. ag_handle=%i, sco shdl=0x%x", __FUNCTION__, ag_handle, btapp_ag_sco_chdl);
        BTL_IF_SendMsgNoParams(btapp_ag_sco_chdl, SUB_SCO, BTLIF_CONNECT_RSP);
    }
    else
    {
        APPL_TRACE_DEBUG2("%s: No conn_info found for ag_handle %i", __FUNCTION__, ag_handle);
    }
}

/*******************************************************************************
**
** Function         btapp_ag_on_sco_close_evt
**
** Description      Handle hsp/hfp close event
**
** Returns          void
*******************************************************************************/
void btapp_ag_on_sco_close_evt(tBTA_AG *p_data)
{
    UINT16 ag_handle;
    tCTRL_HANDLE chdl;
    tBTAPP_AG_CONN_INFO *p_conn_info;
    tBTAPP_AG_LISTENER_INFO *p_listener;

    ag_handle = p_data->hdr.handle;

    /* Get conn info associated with this handle */
    if ((p_conn_info = btapp_ag_handle_to_conn_info(ag_handle))!= NULL)
    {
        APPL_TRACE_DEBUG3("%s: ag_handle %i closed. Clean up sco chnd=0x%x", __FUNCTION__, ag_handle, btapp_ag_sco_chdl);

        /* Clear SCO_UP flag for this connection */
        p_conn_info->is_sco_up = FALSE;

        /* Notify client that connection is down */
        BTL_IF_SendMsgNoParams(btapp_ag_sco_chdl, SUB_SCO, BTLIF_DISCONNECT_IND);
    }
    else
    {
        APPL_TRACE_ERROR2("%s: error: unable to find conn info for ag_handle %i.", __FUNCTION__, ag_handle);
    }
}

/*******************************************************************************
**
** Function         btapp_ag_on_rx_sco_data
**
** Description      Called when data is received over socket
**
** Returns          void
*******************************************************************************/
void btapp_ag_on_rx_sco_data(tDATA_HANDLE dhdl, char *p, int len)
{
    APPL_TRACE_DEBUG3("%s: (dhdl=0x%x) received %i sco bytes from client", __FUNCTION__, dhdl, len);
}


/*******************************************************************************
**
**
Function         btapp_ag_on_btm_role_switch_complete

**

** Description

**
** Returns          void

*******************************************************************************/

void btapp_ag_on_btm_role_switch_complete(tBTM_ROLE_SWITCH_CMPL * p)

{

    tBTAPP_AG_CONN_INFO *p_conn_info = NULL;

    APPL_TRACE_DEBUG1("%s", __FUNCTION__);

    /* Lookup ag_conn_info for this bd_addr */

    if ((p_conn_info = btapp_ag_get_active_conn_info()) != NULL)

    {
        APPL_TRACE_DEBUG6(" Remote BDA [%02x:%02x:%02x:%02x:%02x:%02x]", p->remote_bd_addr[0], p->remote_bd_addr[1], p->remote_bd_addr[2], p->remote_bd_addr[3], p->remote_bd_addr[4], p->remote_bd_addr[5]);
        APPL_TRACE_DEBUG6("Remote BDA from conn [%02x:%02x:%02x:%02x:%02x:%02x]", p_conn_info->bd_addr[0], p_conn_info->bd_addr[1], p_conn_info->bd_addr[2], p_conn_info->bd_addr[3], p_conn_info->bd_addr[4], p_conn_info->bd_addr[5]);

        if ( p->remote_bd_addr[0] == p_conn_info->bd_addr[0] &&
             p->remote_bd_addr[1] == p_conn_info->bd_addr[1] &&
             p->remote_bd_addr[2] == p_conn_info->bd_addr[2] &&
             p->remote_bd_addr[3] == p_conn_info->bd_addr[3] &&
             p->remote_bd_addr[4] == p_conn_info->bd_addr[4] &&
             p->remote_bd_addr[5] == p_conn_info->bd_addr[5]
           )

        {
            BTA_AgAudioOpen(p_conn_info->ag_handle);
        }
        else
        {
          APPL_TRACE_DEBUG0("Active connection does not match role switch bda");
        }
    }
    else
    {
        APPL_TRACE_DEBUG0("No active connection");
    }
}



/*******************************************************************************
**
** Function         btapp_ag_on_rx_sco_ctrl
**
** Description
**
** Returns          void
*******************************************************************************/
void btapp_ag_on_rx_sco_ctrl(tCTRL_HANDLE chdl, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params)
{
    UINT16 ag_handle;
    tBTAPP_AG_CONN_INFO *p_conn_info;
    BT_HDR *p_buf;
    BD_ADDR bd_addr;
    char * p_service_names[BTUI_AG_NUM_APP];

    APPL_TRACE_DEBUG3("%s: chdl=%i msg id=%i", __FUNCTION__, chdl, id);

    switch (id)
    {
    case BTLIF_CONNECT_REQ:
        APPL_TRACE_DEBUG2("%s: Received AG config :: rf_chan %d", __FUNCTION__, params->ag_conreq.rf_chan);

        /* Reverse bdaddr */
        bd_addr[5] = params->ag_conreq.bd[0];
        bd_addr[4] = params->ag_conreq.bd[1];
        bd_addr[3] = params->ag_conreq.bd[2];
        bd_addr[2] = params->ag_conreq.bd[3];
        bd_addr[1] = params->ag_conreq.bd[4];
        bd_addr[0] = params->ag_conreq.bd[5];

        APPL_TRACE_DEBUG6("SCO Open request:bd [%02x:%02x:%02x:%02x:%02x:%02x]", bd_addr[0],  bd_addr[1], bd_addr[2],  bd_addr[3], bd_addr[4],  bd_addr[5]);

        /* Lookup ag_conn_info for this bd_addr */
        if ((p_conn_info = btapp_ag_get_active_conn_info()) != NULL)
        {
            APPL_TRACE_DEBUG1("p_conn_info->is_sco_up = %i", p_conn_info->is_sco_up);

            /* Check if SCO is already up */
            if (p_conn_info->is_sco_up)
            {
                /* Notify BTL-IF that ag connection is up */
                APPL_TRACE_DEBUG2("%s: Sco is already up. Notifying btl-f. ag_handle=%i, sco shdl=0x%x", __FUNCTION__, btapp_ag_sco_chdl);
                BTL_IF_SendMsgNoParams(btapp_ag_sco_chdl, SUB_SCO, BTLIF_CONNECT_RSP);
            }
            else
            {
                /* Blacklist for certain devices - only first 3 bytes are of interest */
                BD_ADDR bda_blackList[] = { {0x00,0x07,0x62,0x00,0x00,0x00} /* iMT525 */
                                            ,{0x00,0x24,0x1c,0x00,0x00,0x00} /* Moto H790 */

                                          };
                int nListItems = 0;         //List is bypassed for now since this fix is not needed by HTC anymore.
                int fbdaInBlackList = 0;
                int i;
                tBTM_STATUS ret = BTM_SUCCESS;
                UINT8 nRole = BTM_ROLE_MASTER;

                for ( i =0; i < nListItems ; i++)
                {
                    APPL_TRACE_DEBUG6("Checking for blacklist [%02x:%02x:%02x] [%02x:%02x:%02x]", bda_blackList[i][0],
                                        bda_blackList[i][1], bda_blackList[i][2], bd_addr[0], bd_addr[1], bd_addr[2]);
                    if ( bd_addr[0] == bda_blackList[i][0] &&
                         bd_addr[1] == bda_blackList[i][1] &&
                         bd_addr[2] == bda_blackList[i][2]
                       )
                    {
                        //Come out of sniff mode before issuing SCO connect as we may be a slave and in sniff mode will get delayed for the SCO LMP's
                        HCILP_WakeupBTDevice(NULL);         /* Make sure BT_WAKE is asserted */

                        bta_dm_pm_active(bd_addr);

                        fbdaInBlackList = 1;
                        APPL_TRACE_DEBUG0("Found device in blacklist");
                        /* Remove the sniff mode */
                        /* If the role is already not master then switch it to master */
                        ret = BTM_GetRole(bd_addr, &nRole);
                        if ( ret == BTM_SUCCESS && nRole != BTM_ROLE_MASTER )
                        {
                            /* Try and call role switch */
                            ret = BTM_SwitchRole(bd_addr, BTM_ROLE_MASTER, btapp_ag_on_btm_role_switch_complete );
                        }
                        break;
                    }
                }
                if ( !fbdaInBlackList || (ret != BTM_SUCCESS && ret != BTM_CMD_STARTED) || nRole == BTM_ROLE_MASTER )
                {
                    if ( ret != BTM_SUCCESS && ret != BTM_CMD_STARTED )
                    {
                        APPL_TRACE_DEBUG1("GetRole or SwitchRole Failed 0x%x", ret);
                    }
                    if ( nRole == BTM_ROLE_MASTER )
                    {
                        APPL_TRACE_DEBUG0("Role already master");
                    }
                    BTA_AgAudioOpen(p_conn_info->ag_handle);
                }

            }
        }
        else
        {
            /* Could not find an available conn handle for this socket */
            APPL_TRACE_ERROR1("%s: unable to find active connection for this bdaddr", __FUNCTION__);
        }
        break;

    case BTLIF_DISCONNECT_REQ:
        APPL_TRACE_DEBUG1("\tReceived BTLIF_DISCONNECT_REQ :: sco chdl=0x%x", chdl);

        /* Get ag_handle of connection w/ active sco */
        if ((p_conn_info = btapp_ag_get_conn_info_sco()) != NULL)
        {
            tBTAPP_AG_CONN_INFO *p = NULL;

            APPL_TRACE_DEBUG0("Exit sniff mode and close Ag Audio cnx");
            p = btapp_ag_handle_to_conn_info(ag_handle);
            if (p)
            {
                /* Make sure we exit any sniff mode before disconnecting as some remote
                 * devices might end up in a non responsive state otherwise.
                 */
                bta_dm_pm_active(p->bd_addr);
            }

            BTA_AgAudioClose(p_conn_info->ag_handle);
        }
        else
        {
            APPL_TRACE_ERROR0("\tCould not find conn_info with active SCO");
        }
        break;

    case BTLIF_DATA_CHAN_IND:
        APPL_TRACE_DEBUG2("\tReceived BTLIF_DATA_CHAN_IND :: chdl=0x%x dhdl 0x%x", chdl, params->chan_ind.handle);

        /* No data channel needed for sco */
        break;

    case BTLIF_LISTEN_REQ:
        APPL_TRACE_DEBUG1("\tReceived listen req :: chdl=%x", chdl);

        /* TODO: handle listener for remote-initiated SCO connections */
        break;

    default:
        break;
    }
}
#endif
