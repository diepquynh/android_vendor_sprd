/*****************************************************************************
**
**  Name:             btapp_ag.h
**
**  Description:     This file contains btui internal interface
**				     definition
**
**  Copyright (c) 2000-2005, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include "gki.h"

#ifndef BTAPP_AG_H
#define BTAPP_AG_H
#include "bta_ag_api.h"

/* phone call-related events */
enum
{
    BTUI_AG_HF_DIAL_EVT,
    BTUI_AG_HF_HANGUP_EVT,
    BTUI_AG_HF_ANS_EVT,
    BTUI_AG_HF_ANS_HELD_EVT,
    BTUI_AG_HF_CANCEL_EVT,
    BTUI_AG_IN_CALL_EVT,
    BTUI_AG_IN_CALL_CONN_PH_EVT,
    BTUI_AG_IN_CALL_CONN_EVT,
    BTUI_AG_OUT_CALL_ORIG_EVT,
    BTUI_AG_OUT_CALL_ORIG_PH_EVT,
    BTUI_AG_OUT_CALL_CONN_EVT,
    BTUI_AG_OUT_CALL_CONN_PH_EVT,
    BTUI_AG_END_CALL_EVT,
    BTUI_AG_CALL_WAIT_EVT,
    BTUI_AG_CALL_CANCEL_EVT,
    BTUI_AG_HS_CKPD_EVT
};

#define BTUI_AG_IND_CALL    0
#define BTUI_AG_IND_SETUP   2
#define BTUI_AG_IND_SVC     4
#define BTUI_AG_IND_SIG     6
#define BTUI_AG_IND_ROAM    8
#define BTUI_AG_IND_BATT    10
#define BTUI_AG_IND_HELD    12
#define BTUI_AG_IND_LEN     14

#define BTUI_AG_ID_1        0
#define BTUI_AG_ID_2        1

#define BTUI_AG_NUM_APP     2

/* phone call state */
enum
{
    BTUI_AG_CALL_NONE,
    BTUI_AG_CALL_INC,
    BTUI_AG_CALL_OUT,
    BTUI_AG_CALL_ACTIVE,
    BTUI_AG_CALL_HELD
};

typedef struct
{
    tBTA_SERVICE_ID service;
    UINT16          handle;
    BOOLEAN         is_open;
    BOOLEAN         is_audio_open;
    UINT8           spk_vol;
    UINT8           mic_vol;
    BOOLEAN         bvra_active;
    tBTUI_REM_DEVICE    *p_dev;     /* BT device */
} tBTUI_AG_APP;

typedef struct
{
    BOOLEAN         enabled;
    tBTUI_AG_APP    app[BTUI_AG_NUM_APP];
    char            ind[BTUI_AG_IND_LEN];
    UINT16          call_handle;
    UINT8           call_state;
    BOOLEAN         inband_ring;
    UINT8           btrh_state;
    BOOLEAN         btrh_active;
    BOOLEAN         cnum_disabled;
    UINT8           btrh_cmd;
    BOOLEAN         btrh_no_sco;
    UINT8           atd_err_code;
    UINT16          atd_err_timer;
    TIMER_LIST_ENT  atd_err_tle;
    tBTA_AG_PEER_FEAT   peer_feat;
#if (BTM_WBS_INCLUDED == TRUE )
    tBTA_AG_PEER_CODEC  peer_codec;
    tBTA_AG_PEER_CODEC  sco_codec;
#endif

#if (defined BTA_FM_INCLUDED && BTA_FM_INCLUDED == TRUE)
/* add for FM */
    BOOLEAN         play_fm;
    BOOLEAN         fm_disconnect;
    BOOLEAN         fm_connect;
    UINT16          cur_handle;
    UINT8           sco_owner; /* 0, none; 1: AG, 2: FM */
#endif

} tBTUI_AG_CB;

extern tBTUI_AG_CB btui_ag_cb;

extern tBTA_SERVICE_MASK btapp_ag_enable(void);
extern void btapp_ag_disable(void);
extern void btapp_ag_close_conn(UINT8 app_id);
extern void btapp_ag_change_spk_volume(UINT8 app_id, BOOLEAN increase);
extern void btapp_ag_change_mic_volume(UINT8 app_id,BOOLEAN increase);
extern void btapp_ag_set_audio_device_authorized (tBTUI_REM_DEVICE * p_device_rec);
extern void btapp_ag_set_device_default_headset (tBTUI_REM_DEVICE * p_device_rec );
extern BOOLEAN btapp_ag_connect_device(tBTUI_REM_DEVICE * p_device_rec, BOOLEAN is_fm );
extern void btui_platform_ag_event(tBTA_AG_EVT event, tBTA_AG *p_data);
extern void btui_platform_ag_init(void);
extern void btapp_ag_disable_vr(UINT8 app_id);
extern UINT8 btapp_ag_find_active_vr(void);

#if (defined BTA_FM_INCLUDED && BTA_FM_INCLUDED == TRUE)
extern void btui_view_audio_devices (void);
#endif

#endif
