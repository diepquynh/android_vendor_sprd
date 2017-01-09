/****************************************************************************
**
**  Name:          btapp_dm.c
**
**  Description:   contains  device manager application
**
**
**  Copyright (c) 2002-2010, Broadcom Corp., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
******************************************************************************/
#define _BTAPP_DM_C_

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "gki.h"
#include "btu.h"
#include "bta_api.h"
#include "btui.h"
#include "btui_int.h"
#include "bd.h"
#include "btapp.h"
#include "btapp_dm.h"
#include "btm_api.h"
#include "bte_appl.h"
#include "btl_ifs.h"
#include "btl_cfg.h"

#if defined(BTAPP_TESTMODE_INCLUDED) && (BTAPP_TESTMODE_INCLUDED==TRUE)
#include "bt_test_mode.h"
#endif

#if (L2CAP_FCR_INCLUDED == TRUE && PORT_ENABLE_L2CAP_FCR_TEST == TRUE)
#include "port_api.h"
#endif

#include "bta_dm_ci.h"
#include <stdio.h>
#include <string.h>

#if ((defined BTA_AV_INCLUDED) && (BTA_AV_INCLUDED == TRUE))
#include <btl_av_codec.h>
#if (BTU_DUAL_STACK_INCLUDED == TRUE )
#include "btui_av_audio.h"
#include "bta_av_ci.h"
#endif
#endif

#include "dtun_api.h"

#define LOG_TAG "BTL-BTAPP_DM"
#ifndef LINUX_NATIVE
#include <cutils/log.h>
#else
#define LOGE(format, ...)  fprintf(stderr, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGI(format, ...)  fprintf(stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#endif


#ifdef APPL_SLAVE_EXCEPTION
BOOLEAN (APPL_SLAVE_EXCEPTION)(BD_ADDR bd_addr);
#endif

#ifndef BTAPP_DM_FIND_ME_TIME
#define BTAPP_DM_FIND_ME_TIME       62  /* Tgap(104) more than 1 min */
#endif

#ifndef BTAPP_DM_LIMITED_RSSI_OFFSET
#define BTAPP_DM_LIMITED_RSSI_OFFSET    30
#endif

#ifndef BTAPP_DM_DI_DB_SIZE
#define BTAPP_DM_DI_DB_SIZE      1024
#endif

/* Supported services - used decide what services to query for when doing remote getServices */
#ifndef BTAPP_DM_SUPPORTED_SERVICES
#define BTAPP_DM_SUPPORTED_SERVICES (   BTA_SPP_SERVICE_MASK | \
                                        BTA_DUN_SERVICE_MASK | \
                                        BTA_HSP_SERVICE_MASK | \
                                        BTA_HFP_SERVICE_MASK | \
                                        BTA_OPP_SERVICE_MASK | \
                                        BTA_FTP_SERVICE_MASK | \
                                        BTA_BPP_SERVICE_MASK | \
                                        BTA_BIP_SERVICE_MASK | \
                                        BTA_SAP_SERVICE_MASK | \
                                        BTA_A2DP_SERVICE_MASK | \
                                        BTA_AVRCP_SERVICE_MASK | \
                                        BTA_HID_SERVICE_MASK | \
                                        BTA_PBAP_SERVICE_MASK )
#endif


#ifndef BTAPP_DM_ALL_SUPPORTED_SERVICES
/* BTA_ALL_SERVICE_MASK: this returns really ALL services found in remote device SDP:
BTA_ALL_SERVICE_MASK & ~BTA_RES_SERVICE_MASK: this returns all BTA connectable serivces */
#define BTAPP_DM_ALL_SUPPORTED_SERVICES (BTA_ALL_SERVICE_MASK)
#endif

/* Shutdown state */
extern BOOLEAN bBtaDisabled;            /* TRUE after BTA has been disabled */

extern void btapp_am_AvOpen(tDTUN_DEVICE_METHOD *msg);
extern void btapp_am_AvDisc(tDTUN_DEVICE_METHOD *msg);
extern void btapp_am_AvStartStop(tDTUN_DEVICE_METHOD *msg);

/*****************************************************************************
**  Local Function prototypes
*****************************************************************************/
static void btui_security_cback (tBTA_DM_SEC_EVT event, tBTA_DM_SEC *p_data);
       void btui_add_devices(void);
static void btui_discover_cb(tBTA_DM_SEARCH_EVT event, tBTA_DM_SEARCH *p_data);
void btui_search_cb(tBTA_DM_SEARCH_EVT event, tBTA_DM_SEARCH *p_data);

void btapp_dm_dtun_start(void);
void btapp_dm_dtun_stop(void);
void btapp_dm_DeviceFound(BD_ADDR *p_bd, UINT16 rssi, UINT32 cod);

/* maps the index of the device in the database to the menu options */
UINT8 btui_device_db_to_menu_option_lkup_tbl[BTUI_NUM_REM_DEVICE];

/* newly found devices */
tBTUI_INQ_DB btui_inq_db;
/* device database */
tBTUI_DEV_DB btui_device_db;



/* Enumeration of current discovery in progress */
typedef enum {
    DISC_IN_PROGRESS_NONE,
    DISC_IN_PROGRESS_DEVICE_SEARCH,      /* DTUN_METHOD_DM_START_DISCOVERY */
    DISC_IN_PROGRESS_GET_SCN,               /* DTUN_METHOD_DM_GET_REMOTE_SERVICE_CHANNEL */
    DISC_IN_PROGRESS_SERVICE_DISCOVERY      /* DTUN_METHOD_DM_GET_REMOTE_SERVICES */
} tDISC_IN_PROGRESS;

boolean btapp_dm_remove_bonding_pending;
BD_ADDR btapp_dm_remove_bonding_pending_bda;
tDISC_IN_PROGRESS btapp_dm_discovery_in_progress;   /* Current type of discovery in progress */

enum {
	RESUME_NONE,
	WAS_CANCELLED,
	CANCELLED
};

enum {
	PENDING_NONE,
	PENDING_DISC,
	PENDING_GET_SCN
};

/* Struct for storing pending Get SCN or DiscDevices (waiting for current discovery to be cancelled) */
typedef struct
{
    int code; //Either Discovery or getSCN could be pending refer to enum above
    tDTUN_DEVICE_METHOD dtun_msg;
} tBTAPP_DM_PENDING;
tBTAPP_DM_PENDING btapp_dm_pending;
tBTAPP_DM_PENDING btapp_possible_cancel;

#if defined(BTAPP_TESTMODE_INCLUDED) && (BTAPP_TESTMODE_INCLUDED==TRUE)
  /* testmode cb. could go into a generic dm_cb. make sure all testmode variables are set 0 at load time */
tBTAPP_DM_TESTMODE_CB btapp_dm_testmode_cb = {
        FALSE           /* test mode off. this is simply global on off flag. Use Set/GetTestMode for
                         * for setting sub testmode states */
};
#endif


void btapp_dm_check_pending(void);
void btapp_dm_GetRemoteServiceChannel(tDTUN_DEVICE_METHOD *msg);


/* Pending SSP user confirmation info */
typedef struct {
    BD_ADDR bda;
    UINT32 num_val;
    BOOLEAN just_works;
    TIMER_LIST_ENT tle;
    BOOLEAN is_pending;
    UINT32 cod;
} tSSP_CFM;
void btapp_dm_ssp_cfm_req(TIMER_LIST_ENT *p_tle);

enum {
	BTAPP_GOT_NONE,
	BTAPP_GOT_AUTH_COMP,
	BTAPP_GOT_CONN_COMP
};

/* Local bonding request info */
typedef struct {
    BD_ADDR bda;
    BOOLEAN in_progress;
    UINT8      state;
    tDTUN_DEVICE_SIGNAL auth_comp_sig;
} tLOCAL_BOND;

/* BTAPP_DM control block */
typedef struct {
    tLOCAL_BOND local_bond;     /* TRUE if bonding has been initiated locally */
    tSSP_CFM pending_ssp_cfm;   /* Pending SSP user confirmation info */
    BD_ADDR  bda_sdp;           /* BDA of device we are currently performing sdp on */

    /* Current pairing mode (used to restore desired pairing mode after bonding) */
    BOOLEAN pairable;               /* TRUE if pairing is enabled */
    BOOLEAN connect_only_paired;    /* TRUE if only paired devices are allowed to open ACL connections */

} tBTAPP_DM_CB;
tBTAPP_DM_CB btapp_dm_cb;

/*******************************************************************************
**
** Function         btapp_startup
**
** Description      Initializes bt application and waits for device to come up
**
** Returns          void
*******************************************************************************/
void btapp_startup(void)
{
#ifdef BTUI_APP
    memset(&btui_device_db,0x00,sizeof(btui_device_db));
    memset(&btui_device_db_to_menu_option_lkup_tbl, 0,
                sizeof(btui_device_db_to_menu_option_lkup_tbl));

    while(!BTA_DmIsDeviceUp())
    {
        GKI_delay(200);
    }
#endif

    /* read all parmeters stored in nvram */
    btui_init_device_db();

    btui_platform_startup();

    btapp_dm_dtun_start();

    BTL_IF_ServerInit();        /* Initialize socket server manager */

    btapp_dm_remove_bonding_pending = FALSE;
    btapp_dm_discovery_in_progress = DISC_IN_PROGRESS_NONE;
    btapp_dm_pending.code = PENDING_NONE;
    btapp_possible_cancel.code = RESUME_NONE;
}

/*******************************************************************************
**
** Function         btapp_dm_init
**
** Description      Initializes Device manger
**
** Returns          void
*******************************************************************************/
void btapp_dm_init(void)
{
    /* Clear btapp_dm control block, register btapp_dm timer with btu */
    memset(&btapp_dm_cb, 0, sizeof(tBTAPP_DM_CB));

    /* Initial pairing mode: pairable + allow unpaired devices to initiate connection (for SDP) */
    btapp_dm_cb.pairable = TRUE;
    btapp_dm_cb.connect_only_paired = FALSE;

   // btu_register_timer (&(btapp_dm_cb.pending_ssp_cfm.tle), BTAPP_BTU_TTYPE_DM_SSP_CFM_DELAY, -1, btapp_dm_ssp_cfm_req);

    /* enable bluetooth before calling
    other BTA API */
}

/*******************************************************************************
**
** Function         btapp_dm_enable_bt
**
** Description      Enables Bluetooth.
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_enable_bt(void)
{
    BTA_EnableBluetooth(&btui_security_cback);
#if (L2CAP_FCR_INCLUDED == TRUE && PORT_ENABLE_L2CAP_FCR_TEST == TRUE)
    PORT_SetFCR((UINT8 *)&btui_cfg.l2ccfg, btui_cfg.l2c_chan_mode_opts);
#endif

    /* set auth for simple pairing */
    //    btui_cfg.sp_auth_req = BTA_AUTH_SPGB_YES;
    APPL_TRACE_DEBUG0("btapp_dm_enable_bt set btui_cfg.sp_auth_req = BTA_AUTH_SPGB_NO");
    btui_cfg.sp_auth_req = BTUI_AUTH_REQ_GEN_BOND_DD;
}

#if 0
/*******************************************************************************
**
** Function         btapp_dm_pin_code_reply
**
** Description      Process the passkey entered by user
**
** Returns          void
*******************************************************************************/
void btapp_dm_pin_code_reply(BOOLEAN accept, UINT8 pin_len,
                                   UINT8 *p_pin)

{
    BTA_DmPinReply( btui_cb.peer_bdaddr,
                  accept,
                  pin_len,
                  (UINT8*)p_pin);
}

/*******************************************************************************
**
** Function         btapp_dm_confirm_reply
**
** Description      Process the confirm/reject entered by user
**
** Returns          void
*******************************************************************************/
void btapp_dm_confirm_reply(BOOLEAN accept)
{
    BTA_DmConfirm( btui_cb.peer_bdaddr, accept);
}

/*******************************************************************************
**
** Function         btapp_dm_passkey_cancel
**
** Description      Process the passkey cancel entered by user
**
** Returns          void
*******************************************************************************/
void btapp_dm_passkey_cancel(void)
{
    BTA_DmPasskeyCancel(btui_cb.peer_bdaddr);
}
#endif

/*******************************************************************************
**
** Function         btapp_dm_proc_io_req
**
** Description
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_proc_io_req(BD_ADDR bd_addr, tBTA_IO_CAP *p_io_cap, tBTA_OOB_DATA *p_oob_data,
                                     tBTA_AUTH_REQ *p_auth_req, BOOLEAN is_orig)
{
    UINT8   dd_bit = (btui_cfg.sp_auth_req & BTUI_AUTH_REQ_DD_BIT);
    UINT8   yes_no_bit = BTA_AUTH_SP_YES & *p_auth_req;

    if(0 == bdcmp(btui_cb.oob_bdaddr, bd_addr))
        *p_oob_data = TRUE;
    APPL_TRACE_DEBUG3("btapp_dm_proc_io_req cfg:%d, auth_req:%d, oob:%d",
        btui_cfg.sp_auth_req, *p_auth_req, *p_oob_data);

    switch(btui_cfg.sp_auth_req)
    {
    case BTUI_AUTH_REQ_NO:  /* 0:not required */
    case BTUI_AUTH_REQ_YES: /* 1:required */
        *p_auth_req = btui_cfg.sp_auth_req;
        break;
    case BTUI_AUTH_REQ_GEN_BOND: /* 2:use default + general bonding DD=NO*/
    case BTUI_AUTH_REQ_GEN_BOND_DD:/* 4:use default + general bonding DD=YES*/
        /* the new cswg discussion wants us to indicate the bonding bit */
        //if(btui_get_device_record(bd_addr))        <---- Commented out to resolve double pairing issue with bluez devices
        //{
            if(btui_cb.is_dd_bond)
            {
                /* if initing/responding to a dedicated bonding, use dedicate bonding bit */
                if(dd_bit)
                    *p_auth_req = BTA_AUTH_DD_BOND | BTA_AUTH_SP_YES;
                else
                    *p_auth_req = BTA_AUTH_DD_BOND;
            }
            else
            {
                *p_auth_req = BTA_AUTH_GEN_BOND | yes_no_bit; /* set the general bonding bit for stored device */
            }
        //}
        break;
    default:/*and BTUI_AUTH_REQ_DEFAULT 3:use default */
        if(btui_cb.is_dd_bond)
        {
            /* if initing/responding to a dedicated bonding, use dedicate bonding bit */
            if(dd_bit)
                *p_auth_req = BTA_AUTH_DD_BOND | BTA_AUTH_SP_YES;
            else
                *p_auth_req = BTA_AUTH_DD_BOND;
        }
        break;
    }
    APPL_TRACE_DEBUG1("auth_req:%d",*p_auth_req);
}

/*******************************************************************************
**
** Function         btapp_dm_proc_io_rsp
**
** Description
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_proc_io_rsp(BD_ADDR bd_addr, tBTA_IO_CAP io_cap, tBTA_AUTH_REQ auth_req)
{
    tBTUI_REM_DEVICE    *p_device_rec;
    p_device_rec = btui_get_device_record(bd_addr);
    if(p_device_rec)
    {
        p_device_rec->peer_io_cap = io_cap;
    }
    if(auth_req & BTA_AUTH_BONDS)
    {
        if(auth_req & BTA_AUTH_DD_BOND)
            btui_cb.is_dd_bond = TRUE;
        /* store the next generator link key */
        bdcpy(btui_cb.sp_bond_bdaddr, bd_addr);
        btui_cb.sp_io_cap = io_cap;
        btui_cb.sp_bond_bits = (auth_req & BTA_AUTH_BONDS);
    }
    APPL_TRACE_DEBUG2("btapp_dm_proc_io_rsp auth_req:%d, is_dd_bond:%d",
        auth_req, btui_cb.is_dd_bond);
}



#if 0

/*******************************************************************************
**
** Function         btapp_dm_proc_lk_upgrade
**
** Description
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_proc_lk_upgrade(BD_ADDR bd_addr, BOOLEAN *p_upgrade)
{
    if(btui_cfg.sp_auth_req == BTUI_AUTH_REQ_NO)
    {
        /* if hard coded to use no MITM, do not upgrade the link key */
        *p_upgrade = FALSE;
    }
}

/*******************************************************************************
**
** Function         btapp_dm_rmt_oob_reply
**
** Description      Process the hash C, randomizer r/reject entered by user
**
** Returns          void
*******************************************************************************/
void btapp_dm_rmt_oob_reply(BOOLEAN accept, BT_OCTET16 c, BT_OCTET16 r)
{
    APPL_TRACE_API0("calling bta_dm_ci_rmt_oob");
    bta_dm_ci_rmt_oob(accept, btui_cb.peer_bdaddr, c, r);
}

/*******************************************************************************
**
** Function         btapp_dm_loc_oob
**
** Description      Read OOB data from local LM
**
** Returns          void
*******************************************************************************/
void  btapp_dm_loc_oob(void)
{
#if (BTM_OOB_INCLUDED == TRUE)
     BTA_DmLocalOob();
#else
    APPL_TRACE_ERROR0("BTM_OOB_INCLUDED is FALSE!!(btapp_dm_loc_oob)");
#endif
}

/*******************************************************************************
**
** Function         btapp_dm_authorize_resp
**
** Description      Action function to process auth reply
**
** Returns          void
*******************************************************************************/
void btapp_dm_authorize_resp(tBTA_AUTH_RESP response)
{

    tBTUI_REM_DEVICE * p_device_rec;
    tBTUI_REM_DEVICE  device_rec;

    btui_cb.auth_pin_menu_active = FALSE;


    if(response == BTA_DM_AUTH_PERM)
    {

        if((p_device_rec = btui_get_device_record(btui_cb.peer_bdaddr))!= NULL)
        {
            p_device_rec->is_trusted = TRUE;
            p_device_rec->trusted_mask |= (1<<btui_cb.peer_service);
            btui_store_device(p_device_rec);
        }
        else
        {
            memset(&device_rec, 0, sizeof(device_rec));
            device_rec.trusted_mask  = (1<<btui_cb.peer_service);
            strncpy(device_rec.name, btui_cb.peer_name, BTUI_DEV_NAME_LENGTH);
            bdcpy(device_rec.bd_addr, btui_cb.peer_bdaddr);
            device_rec.is_trusted = TRUE;
            btui_store_device(&device_rec);
        }

        BTA_DmAuthorizeReply(btui_cb.peer_bdaddr, btui_cb.peer_service,
                                BTA_DM_AUTH_PERM);
    }
    else if(response == BTA_DM_AUTH_TEMP)
    {
        BTA_DmAuthorizeReply(btui_cb.peer_bdaddr, btui_cb.peer_service,
                                BTA_DM_AUTH_TEMP);

    }
    else
    {
        BTA_DmAuthorizeReply(btui_cb.peer_bdaddr, btui_cb.peer_service,
                                BTA_DM_NOT_AUTH);
    }

}



void btapp_dm_AuthorizeRsp(tDTUN_DEVICE_METHOD *msg)
{
        BTA_DmAuthorizeReply(msg->authorize_rsp.info.bd_addr.b, msg->authorize_rsp.info.service,
                                msg->authorize_rsp.info.response);
}

#endif

/*******************************************************************************
**
** Function         btapp_dm_disable_bt
**
** Description      Disables Bluetooth.
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_disable_bt( void )
{
    /* NOTE: These changes are available as part of HTC-ECLAIR_220596
       * This note is to ensure smooth conflict resolution
       */
    /* make sure nobody connects during disable. */
    /* done in bte_appl.c till TODO is fixed BTA_DmSetVisibility(BTA_DM_NON_DISC, BTA_DM_NON_CONN); */

    /* Set l2c idle timeout to 0 (so BTE immediately disconnects ACL link after last channel is closed) */
  //  L2CA_SetIdleTimeoutByBdAddr((UINT8 *)BT_BD_ANY, 0);

    /* disable bta, closes all connections and unregisters all BTA modules. Page and Inquiry scan
     * are also disabled */
    BTA_DisableBluetooth();
}


#if ((BTU_DUAL_STACK_INCLUDED == TRUE ) ||  (BTU_DUAL_STACK_BTC_INCLUDED == TRUE ))
/*******************************************************************************
**
** Function         btapp_dm_switch_stack_cback
**
** Description      Callback indicating result of stack switch
**
**
** Returns          void
*******************************************************************************/
void btapp_switch_stack_cback(tBTA_DM_SWITCH_EVT evt, tBTA_STATUS status)
{
    tBTUI_BTA_MSG       *p_event_msg;

    APPL_TRACE_DEBUG1("SWITCH STACK evt %d",evt);
    APPL_TRACE_DEBUG1("SWITCH STACK status %d",status);
    if (status == BTA_SUCCESS)
    {
        if(btui_cb.is_switched && (btui_cb.switch_to == BTA_DM_SW_MM_TO_BB))
        {
            btui_cb.is_switched = FALSE;
            APPL_TRACE_DEBUG0("SWITCHED to BB");

#if (BTA_AV_INCLUDED == TRUE)
            if(btui_av_cb.audio_open_count > 0)
            {
                btui_av_send_sync_request(BTUI_AV_AUDIO_SYNC_TO_FULL);
            }
#endif
        }
        else if(btui_cb.is_switched && (btui_cb.switch_to == BTA_DM_SW_BTC_TO_BB))
        {
            btui_cb.is_switched = FALSE;
            APPL_TRACE_DEBUG0("SWITCHED to BB");
        }
        else
        {
            if (!btui_cb.is_switched && (btui_cb.switch_to == BTA_DM_SW_BB_TO_MM))
            {
                btui_cb.is_switched = TRUE;
                APPL_TRACE_DEBUG0("SWITCHED to MM");

#if (BTA_AV_INCLUDED == TRUE)
                if(btui_av_cb.audio_open_count > 0)
                {
                    btui_av_send_sync_request(BTUI_AV_AUDIO_SYNC_TO_LITE);
                }

                if(btui_cb.is_starting_stream)
                {
                    btui_cb.is_starting_stream = FALSE;
                    btapp_av_start_stream();
                }

                if(btui_av_cb.is_prestart)
                {
                    btui_av_cb.is_prestart = FALSE;
                    bta_av_ci_startok(btui_av_cb.prestart_hndl);
                }
#endif
            }
            else if (!btui_cb.is_switched && (btui_cb.switch_to == BTA_DM_SW_BB_TO_BTC))
            {
                btui_cb.is_switched = TRUE;
                if(btui_cb.is_starting_stream)
                {
                    btui_cb.is_starting_stream = FALSE;
                    btapp_av_start_stream();
                }
                APPL_TRACE_DEBUG0("SWITCHED to BTC");
            }
        }
    }
    else
    {
        APPL_TRACE_DEBUG0("STACK SWITCH FAILED!");
    }

    if ((p_event_msg = (tBTUI_BTA_MSG *)GKI_getbuf(sizeof(tBTUI_BTA_MSG))) != NULL)
    {
        p_event_msg->switch_stack_cmpl.hdr.event = BTUI_MMI_SWITCH_STACK_CMPL_EVT;
        p_event_msg->switch_stack_cmpl.result = (BTA_SUCCESS == status)? BTUI_SUCCESS : BTUI_FAIL;

        GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, p_event_msg);
    }

}

/*******************************************************************************
**
** Function         btapp_dm_switch_bb2mm
**
** Description      Switches from Baseband to Multimedia
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_switch_bb2mm(void)
{
    btui_cb.switch_to = BTA_DM_SW_BB_TO_MM;
    BTA_DmSwitchStack(BTA_DM_SW_BB_TO_MM, btapp_switch_stack_cback);
}
/*******************************************************************************
**
** Function         btapp_dm_switch_mm2bb
**
** Description      Switches from Multimedia to Baseband
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_switch_mm2bb(void)
{
    btui_cb.switch_to = BTA_DM_SW_MM_TO_BB;
    BTA_DmSwitchStack(BTA_DM_SW_MM_TO_BB, btapp_switch_stack_cback);
}
/*******************************************************************************
**
** Function         btapp_dm_switch_bb2btc
**
** Description      Switches from Baseband to BTC
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_switch_bb2btc(void)
{
    btui_cb.switch_to = BTA_DM_SW_BB_TO_BTC;
    BTA_DmSwitchStack(BTA_DM_SW_BB_TO_BTC, btapp_switch_stack_cback);
}
/*******************************************************************************
**
** Function         btapp_dm_switch_btc2bb
**
** Description      Switches from BTC to Baseband
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_switch_btc2bb(void)
{
    btui_cb.switch_to = BTA_DM_SW_BTC_TO_BB;
    BTA_DmSwitchStack(BTA_DM_SW_BTC_TO_BB, btapp_switch_stack_cback);
}
#endif /*(BTU_DUAL_STACK_INCLUDED == TRUE ) || (BTU_DUAL_STACK_BTC_INCLUDED == TRUE) */


#if 0
/*******************************************************************************
**
** Function         btapp_dm_set_visibility
**
** Description      Sets visibilty
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_set_visibility( BOOLEAN is_visible, BOOLEAN is_temp)
{

    BTA_DmSetVisibility( (tBTA_DM_DISC) ((is_visible) ? BTA_DM_GENERAL_DISC : BTA_DM_NON_DISC),
                             BTA_DM_CONN);

    btui_device_db.visibility = is_visible;


    if(!is_temp)
    {
        /* update to nvram */
        btui_store_visibility_setting(btui_device_db.visibility);
    }

}

#ifdef BTAPP_FINDME_INCLUDED
/*******************************************************************************
**
** Function         btapp_dm_timer_cback
**
** Description      Timer used to end find-me disconverable mode
**
**
** Returns          void
**
*******************************************************************************/
void btapp_dm_timer_cback(void *p_tle)
{
    APPL_TRACE_EVENT1(" Find me mode ends. vis: %d", btui_device_db.visibility);
    /* go back to the previous DISC mode */
    btapp_dm_set_visibility(btui_device_db.visibility, TRUE);
}

/*******************************************************************************
**
** Function         btapp_dm_set_find_me
**
** Description      Sets visibilty
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_set_find_me( void)
{
    btui_device_db.dev_tle.p_cback = btapp_dm_timer_cback;
    btui_app_start_timer(&btui_device_db.dev_tle, 0, BTAPP_DM_FIND_ME_TIME);

    BTA_DmSetVisibility( BTA_DM_LIMITED_DISC, BTA_DM_CONN);
}
#endif

/*******************************************************************************
**
** Function         btapp_dm_set_local_name
**
** Description      Sets local name
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_set_local_name(char *p_name)
{
    BTA_DmSetDeviceName(p_name);
    strncpy(btui_device_db.local_device_name, p_name, BTUI_DEV_NAME_LENGTH);
    /* update to nv memory */
    btui_store_local_name(btui_device_db.local_device_name);
}


/*******************************************************************************
**
** Function         btapp_dm_sec_add_device
**
** Description      Sets device as trusted
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_sec_add_device(tBTUI_REM_DEVICE * p_device_rec)
{
    DEV_CLASS dev_class = {0, 0, 0};

    /* update BTA with new settings */
    if(p_device_rec->link_key_present)
        BTA_DmAddDevice(p_device_rec->bd_addr, dev_class, p_device_rec->link_key,
            p_device_rec->trusted_mask, p_device_rec->is_trusted,
            p_device_rec->key_type, p_device_rec->peer_io_cap);
    else
        BTA_DmAddDevice(p_device_rec->bd_addr, dev_class, NULL, p_device_rec->trusted_mask,
            p_device_rec->is_trusted, p_device_rec->key_type, p_device_rec->peer_io_cap);

}


/*******************************************************************************
**
** Function         btapp_dm_set_trusted
**
** Description      Sets device as trusted
**
**
** Returns          void
*******************************************************************************/
BOOLEAN btapp_dm_set_trusted(tBTA_SERVICE_MASK trusted_mask, tBTUI_REM_DEVICE * p_device_rec)
{

    p_device_rec->trusted_mask |= trusted_mask;
    p_device_rec->is_trusted = TRUE;
    btui_store_device(p_device_rec);
    /* update BTA with new settings */
    btapp_dm_sec_add_device(p_device_rec);

    return TRUE;


}

/*******************************************************************************
**
** Function         btapp_dm_set_not_trusted
**
** Description      Sets device as not trusted
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_set_not_trusted(tBTUI_REM_DEVICE * p_device_rec)
{

    p_device_rec->is_trusted = FALSE;
    btui_store_device(p_device_rec);
    btapp_dm_sec_add_device(p_device_rec);


}
/*******************************************************************************
**
** Function         btapp_dm_delete_device
**
** Description      Deletes a device from data base
**
**
** Returns          void
*******************************************************************************/
BOOLEAN btapp_dm_delete_device(void)
{
    if ((BTA_DmRemoveDevice (btui_cb.p_selected_rem_device->bd_addr)) == BTA_SUCCESS)
    {
        btui_delete_device(btui_cb.p_selected_rem_device->bd_addr);
        return TRUE;
    }
    else
        return FALSE;

}

/*******************************************************************************
**
** Function         btapp_dm_discover_device
**
** Description      Searches for services on designated device.
**
**
** Returns          void
*******************************************************************************/

void btapp_dm_discover_device(BD_ADDR bd_addr, BOOLEAN is_new)
{
    tBTA_SERVICE_MASK client_services;
    BOOLEAN sdp_search;

    /* we need to find only services for which we can be in client role */
    client_services = (btui_cfg.supported_services & ~(BTA_SPP_SERVICE_MASK | BTA_DUN_SERVICE_MASK | BTA_FAX_SERVICE_MASK | BTA_LAP_SERVICE_MASK));
    APPL_TRACE_EVENT2(" btapp_dm_discover_device  client_services x%x %c",client_services, btui_cfg.dg_client_service_id[0]);

    if(!btui_cfg.ftc_included)
        client_services &= ~ BTA_FTP_SERVICE_MASK;

    if(!btui_cfg.opc_included)
        client_services &= ~ BTA_OPP_SERVICE_MASK;

    client_services |= 1 << (btui_cfg.dg_client_service_id[0] - '0');

    if( is_new )
        sdp_search = FALSE;
    else
        sdp_search = TRUE;

    BTA_DmDiscover(bd_addr, client_services, btui_discover_cb, sdp_search);
}




/*******************************************************************************
**
** Function         btapp_dm_stored_device_unbond
**
** Description      Unbond selected device
**
** Returns          void
*******************************************************************************/
BOOLEAN btapp_dm_stored_device_unbond ()
{

    if (!btui_cb.p_selected_rem_device->link_key_present||
        (BTA_DmRemoveDevice(btui_cb.p_selected_rem_device->bd_addr) != BTA_SUCCESS))
    {
        btui_cb.p_selected_rem_device->link_key_present = FALSE;
        btui_store_device(btui_cb.p_selected_rem_device);
        return FALSE;
    }
    else
    {
        btui_cb.p_selected_rem_device->link_key_present = FALSE;
        btui_store_device(btui_cb.p_selected_rem_device);
        return TRUE;
    }

}

/*******************************************************************************
**
** Function         btapp_dm_cancel_search
**
** Description      Cancels search
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_cancel_search(void)
{

    BTA_DmSearchCancel();

}
#if BTA_DI_INCLUDED == TRUE
/*******************************************************************************
**
** Function         btapp_dm_di_discover
**
** Description      Start DI discover
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_di_discover(BD_ADDR bd_addr)
{
    btui_cb.p_di_db = (tBTA_DISCOVERY_DB *)GKI_getbuf(BTAPP_DM_DI_DB_SIZE);

    BTA_DmDiDiscover(bd_addr, btui_cb.p_di_db, BTAPP_DM_DI_DB_SIZE, btui_discover_cb);

}
/*******************************************************************************
**
** Function         btapp_dm_add_di_record
**
** Description      Set local DI record
**
**
** Returns          void
*******************************************************************************/
UINT16 btapp_dm_add_di_record(void)
{
    tBTA_DI_RECORD      device_info;

    memset(&device_info, 0, sizeof(tBTA_DI_RECORD));

    device_info.vendor = LMP_COMPID_WIDCOMM ;           /* 17 */
    device_info.vendor_id_source = DI_VENDOR_ID_SOURCE_BTSIG;  /* from Bluetooth SIG */
    device_info.product = 0x1234;
    device_info.version = 0x0312;       /* version 3.1.2 */
    device_info.primary_record = FALSE;

    return BTA_DmSetLocalDiRecord(&device_info, &btui_cb.di_handle);

}
/*******************************************************************************
**
** Function         btapp_dm_get_di_local_record
**
** Description      Get local DI record
**
**
** Returns          void
*******************************************************************************/
tBTA_STATUS btapp_dm_get_di_local_record(tBTA_DI_GET_RECORD *p_di_record, UINT32 handle)
{
    UINT32 di_handle = handle;

    return BTA_DmGetLocalDiRecord(p_di_record, &di_handle);
}
/*******************************************************************************
**
** Function         btapp_dm_get_di_remote_record
**
** Description      Get remote DI record by index.
**
**
** Returns          void
*******************************************************************************/
tBTA_STATUS btapp_dm_get_di_remote_record(tBTA_DI_GET_RECORD *p_record, UINT8 index)
{
    tBTA_STATUS status = BTA_FAILURE;

    memset(p_record, 0 , sizeof(tBTA_DI_GET_RECORD));

    if (btui_cb.p_di_db != NULL)
    {
        status = BTA_DmGetDiRecord(index, p_record, btui_cb.p_di_db);
    }

    return status;
}
#endif
/*******************************************************************************
**
** Function         btapp_dm_search
**
** Description      Searches for devices supporting the services specified
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_search(tBTA_SERVICE_MASK services,tBTA_DM_INQ *p_data)
{
    tBTA_DM_INQ inq_params;

    if(!p_data)
    {
        inq_params.mode = 0;
        inq_params.duration = BTUI_DEFAULT_INQ_DURATION;
        inq_params.max_resps = btui_cfg.num_inq_devices;
        inq_params.filter_type = btui_cfg.dm_inq_filt_type;
        inq_params.report_dup = TRUE;
        memcpy(&inq_params.filter_cond, &btui_cfg.dm_inq_filt_cond, sizeof(tBTA_DM_INQ_COND));
    }
    else
    {
        memcpy(&inq_params, p_data, sizeof(tBTA_DM_INQ));

    }
    btui_inq_db.rem_index = 0;
    memset(&btui_inq_db, 0, sizeof(btui_inq_db));
    btui_cb.search_services = services;
    /* find nearby devices */
    BTA_DmSearch(&inq_params, services, btui_search_cb);


}

/*******************************************************************************
**
** Function         btapp_dm_add_device
**
** Description      Adds a new device to database
**
**
** Returns          void
*******************************************************************************/
BOOLEAN btapp_dm_add_device(void)
{

    if(btui_store_device(btui_cb.p_selected_rem_device))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }

}


/*******************************************************************************
**
** Function         btapp_dm_CreateBonding
**
** Description      Handler for "CreateBonding" dbus message
**
** Returns          void
*******************************************************************************/
void btapp_dm_CreateBonding(tDTUN_DEVICE_METHOD *msg)
{
    /* Store local bonding request */
    btapp_dm_cb.local_bond.in_progress = TRUE;
    btapp_dm_cb.local_bond.state = BTAPP_GOT_NONE;
    bdcpy(btapp_dm_cb.local_bond.bda, *(BD_ADDR*)msg->bond.bdaddr.b);

    /* Disable unpaired devices from connecting while we are bonding */
    BTM_SetPairableMode(TRUE, TRUE);

    BTA_DmBond (*(BD_ADDR*)msg->bond.bdaddr.b);
}


/*******************************************************************************
**
** Function         btapp_dm_RemoveBonding
**
** Description      Handler for "RemoveBonding" dbus message
**
** Returns          void
*******************************************************************************/
void btapp_dm_RemoveBonding(tDTUN_DEVICE_METHOD *msg)
{
    APPL_TRACE_DEBUG6("RemoveBonding: %02X:%02X:%02X:%02X:%02X:%02X",
            msg->bond.bdaddr.b[0], msg->bond.bdaddr.b[1], msg->bond.bdaddr.b[2],
            msg->bond.bdaddr.b[3], msg->bond.bdaddr.b[4], msg->bond.bdaddr.b[5]);


    if (BTA_DmRemoveDevice(*(BD_ADDR*)msg->bond.bdaddr.b) == BTA_SUCCESS)
    {
        APPL_TRACE_DEBUG0("Removing bonding...");

        /* TODO: any response? */
    }
    else
    {
        APPL_TRACE_DEBUG0("RemoveBonding: connection still active. Closing connection first.");

        /* Connection still active. Force close */
        btm_remove_acl(*(BD_ADDR*)msg->bond.bdaddr.b);
        btapp_dm_remove_bonding_pending = TRUE;
        bdcpy(btapp_dm_remove_bonding_pending_bda, *(BD_ADDR*)msg->bond.bdaddr.b);
    }
}

/*******************************************************************************
**
** Function         btapp_dm_check_remove_bonding_pending
**
** Description      Called on link close. If link down is due to removebonding,
**                  then remove bonding now.
**
** Returns          void
*******************************************************************************/
void btapp_dm_check_remove_bonding_pending(BD_ADDR bd_addr)
{
    APPL_TRACE_DEBUG6("Check RemoveBonding Pending: %02X:%02X:%02X:%02X:%02X:%02X",
            bd_addr[0], bd_addr[1], bd_addr[2],
            bd_addr[3], bd_addr[4], bd_addr[5]);

    if ((btapp_dm_remove_bonding_pending) && (bdcmp(bd_addr, btapp_dm_remove_bonding_pending_bda) == 0))
    {
        APPL_TRACE_DEBUG0("Removing bonding...");

        btapp_dm_remove_bonding_pending = FALSE;

        BTA_DmRemoveDevice(btapp_dm_remove_bonding_pending_bda);

        /* TODO: any response? */
    }

}


/*******************************************************************************
**
** Function         btapp_dm_PinReply
**
** Description      Handler for "PinReply" dbus message
**
** Returns          void
*******************************************************************************/
void btapp_dm_PinReply(tDTUN_DEVICE_METHOD *msg)
{
            BTA_DmPinReply( msg->pin_reply.bdaddr.b,
                    TRUE,
                    msg->pin_reply.pin_len,
                    msg->pin_reply.pin_code);
}

/*******************************************************************************
**
** Function         btapp_dm_PinNegReply
**
** Description      Handler for "PinNegReply" dbus message
**
** Returns          void
*******************************************************************************/
void btapp_dm_PinNegReply(tDTUN_DEVICE_METHOD *msg)
{
        tDTUN_DEVICE_SIGNAL sig;

            BTA_DmPinReply( msg->pin_reply.bdaddr.b,
                  FALSE,
                  0,
                  NULL);

	sleep(1);

        sig.hdr.id = DTUN_SIG_DM_LINK_DOWN;
        sig.hdr.len = sizeof( tDTUN_SIG_DM_LINK_DOWN_INFO );
        memcpy(sig.link_down.info.bd_addr.b, msg->pin_reply.bdaddr.b, 6);
        sig.link_down.info.reason = 0x5;

        dtun_server_send_signal(&sig);
}

/*******************************************************************************
**
** Function         btapp_dm_DiscRmtDev
**
** Description      Handler for "DisconnectRemoteDevice" dbus message
**
** Returns          void
*******************************************************************************/
void btapp_dm_DiscRmtDev(tDTUN_DEVICE_METHOD *msg)
{
   btm_remove_acl(*(BD_ADDR*)msg->disc_rmt_dev.bdaddr.b);
}

void btapp_dm_AddSDPRecord(tDTUN_DEVICE_METHOD *msg)
{

}

void btapp_dm_DelSDPRecord(tDTUN_DEVICE_METHOD *msg)
{
    APPL_TRACE_EVENT2("%s : delete sdp handle %d", __FUNCTION__, msg->del_sdp_rec.handle);
   BTM_DelSDPRecord( msg->del_sdp_rec.handle);
}


/*******************************************************************************
**
** Function         btapp_dm_SspConfirmReply
**
** Description      Handler for "SSP Confirm" dbus message
**
** Returns          void
*******************************************************************************/
void btapp_dm_SspConfirmReply(tDTUN_DEVICE_METHOD *msg)
{
    BTA_DmConfirm( msg->pin_reply.bdaddr.b, msg->ssp_confirm.accepted);
}


/*******************************************************************************
**
** Function         btapp_dm_bond
**
** Description      Initiates bonding with selected device
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_bond(tBTUI_REM_DEVICE * p_device_rec)
{

    if (btui_cb.is_dd_bond == FALSE)
    {
        btui_cb.is_dd_bond = TRUE;
        bdcpy(btui_cb.sp_bond_bdaddr, p_device_rec->bd_addr);
        BTA_DmBond (p_device_rec->bd_addr);

        btui_store_device(p_device_rec);
    }

}



/*******************************************************************************
**
** Function         btapp_dm_bond_cancel
**
** Description      Cancels bonding with selected device
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_bond_cancel(tBTUI_REM_DEVICE * p_device_rec)
{


    btui_cb.is_dd_bond = FALSE;
    BTA_DmBondCancel (p_device_rec->bd_addr);


}


/*******************************************************************************
**
** Function         btapp_dm_rename_device
**
** Description      sets user friendly name for remote device
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_rename_device(tBTUI_REM_DEVICE * p_device_rec, UINT8 * p_text)
{


    strncpy(p_device_rec->short_name, p_text, BTUI_DEV_NAME_LENGTH);
    /* update to nv memory */
    btui_store_device(p_device_rec);


}
#endif

/*******************************************************************************
**
** Function         btui_security_cback
**
** Description      Security callback from bta
**
**
** Returns          void
*******************************************************************************/
static void btui_security_cback (tBTA_DM_SEC_EVT event, tBTA_DM_SEC *p_data)
{

    tBTUI_BTA_MSG * p_event_msg;
    tBTUI_REM_DEVICE * p_device_record;
    BOOLEAN alloc = FALSE;
    tDTUN_DEVICE_SIGNAL sig;

    char msg_str[64];

    APPL_TRACE_EVENT1(" btui_security_cback  event %d ",event);

    switch(event)
    {
    case BTA_DM_ENABLE_EVT:
        bdcpy(btui_cb.local_bd_addr, p_data->enable.bd_addr);
        btui_device_db.bt_enabled = TRUE;
        btui_store_bt_enable_setting(TRUE);
        sprintf (msg_str, "local bdaddr %02x:%02x:%02x:%02x:%02x:%02x\n",
                 p_data->enable.bd_addr[0], p_data->enable.bd_addr[1],
                 p_data->enable.bd_addr[2], p_data->enable.bd_addr[3],
                 p_data->enable.bd_addr[4], p_data->enable.bd_addr[5]);
        APPL_TRACE_EVENT0(msg_str);
        APPL_TRACE_EVENT1("auth_req:%d", btui_cfg.sp_auth_req);

#ifdef BTUI_APP
        if ((p_event_msg = (tBTUI_BTA_MSG *)GKI_getbuf(sizeof(tBTUI_BTA_MSG))) != NULL)
        {
            p_event_msg->hdr.event = BTUI_MMI_ENABLE;
            GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, p_event_msg);
        }
#else
        /* Notify application task */
        GKI_send_event(BTE_APPL_TASK, BTE_APPL_BTA_ENABLE_EVT);
#endif
        return;

    case BTA_DM_DISABLE_EVT:
        btui_device_db.bt_enabled = FALSE;
        btui_store_bt_enable_setting(FALSE);
        // Add for GKI / Thread shutdown
        bBtaDisabled = TRUE;

#ifdef BTUI_APP
        if ((p_event_msg = (tBTUI_BTA_MSG *)GKI_getbuf(sizeof(tBTUI_BTA_MSG))) != NULL)
        {
            p_event_msg->hdr.event = BTUI_MMI_DISABLE;
            GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, p_event_msg);
        }
#endif  /* BTUI_APP */
        break;

    case BTA_DM_SIG_STRENGTH_EVT:

        if(p_data->sig_strength.mask & BTA_SIG_STRENGTH_RSSI_MASK)
        {

            APPL_TRACE_EVENT1("rssi value %d", p_data->sig_strength.rssi_value);

        }

        if(p_data->sig_strength.mask & BTA_SIG_STRENGTH_LINK_QUALITY_MASK)
        {

            APPL_TRACE_EVENT1("link quality value %d", p_data->sig_strength.link_quality_value);

        }
        return;

    case BTA_DM_BUSY_LEVEL_EVT:
#if ((defined BTA_AV_INCLUDED) && (BTA_AV_INCLUDED == TRUE))
//        btui_codec_update_busy_level(p_data->busy_level.level);
#endif
        break;

    }


}

#if 0
/*******************************************************************************
**
** Function         btapp_dm_ssp_cfm_req
**
** Description      Notify Android of SSP user confirmation request
**                  Called after delay, on receiving BTA_DM_SP_CFM_REQ_EVT
**
** Returns          void
*******************************************************************************/
void btapp_dm_ssp_cfm_req(TIMER_LIST_ENT *p_tle)
{
    tDTUN_DEVICE_SIGNAL dtun_signal;

    if (btapp_dm_cb.pending_ssp_cfm.is_pending)
    {
        dtun_signal.ssp_cfm_req.hdr.id = DTUN_SIG_DM_SSP_CFM_REQ;
        dtun_signal.ssp_cfm_req.hdr.len = sizeof(tDTUN_SIG_SSP_CFM_REQ_INFO);

        bdcpy(*(BD_ADDR*)dtun_signal.ssp_cfm_req.info.bd_addr.b, btapp_dm_cb.pending_ssp_cfm.bda);
        dtun_signal.ssp_cfm_req.info.num_value = btapp_dm_cb.pending_ssp_cfm.num_val;
        dtun_signal.ssp_cfm_req.info.just_work = btapp_dm_cb.pending_ssp_cfm.just_works;
        dtun_signal.ssp_cfm_req.info.cod = btapp_dm_cb.pending_ssp_cfm.cod;

        LOGI("%s: #### Sending ssp_cfm_req DTUN signal ####", __FUNCTION__);
        dtun_server_send_signal(&dtun_signal);

        btapp_dm_cb.pending_ssp_cfm.is_pending = FALSE;
    }
}

/*******************************************************************************
**
** Function         btui_add_devices
**
** Description      called during startup to add the devices which are
**                  stored in NVRAM
**
** Returns          void
*******************************************************************************/
void btui_add_devices(void)
{
    UINT8 i;
    DEV_CLASS dev_class = {0, 0, 0};

    /* Update BTA with peer device information
    stored in NVRAM  */
    for(i=0; i<BTUI_NUM_REM_DEVICE; i++)
    {
        if(!btui_device_db.device[i].in_use)
            continue;

        if(btui_device_db.device[i].link_key_present)
            BTA_DmAddDevice(btui_device_db.device[i].bd_addr, dev_class, btui_device_db.device[i].link_key,
                btui_device_db.device[i].trusted_mask, btui_device_db.device[i].is_trusted,
                btui_device_db.device[i].key_type, btui_device_db.device[i].peer_io_cap);
        else if (btui_device_db.device[i].is_trusted)
            BTA_DmAddDevice(btui_device_db.device[i].bd_addr, dev_class, NULL, btui_device_db.device[i].trusted_mask,
                btui_device_db.device[i].is_trusted, btui_device_db.device[i].key_type,
                btui_device_db.device[i].peer_io_cap);

    }
}

/*******************************************************************************
**
** Function         btapp_dm_sort_inq_db
**
** Description      checks if the given inq_res is in the inq_db
**
**
** Returns          void
*******************************************************************************/
void btapp_dm_sort_inq_db(UINT8 index)
{
    tBTUI_REM_DEVICE    inq_rec;
    tBTUI_REM_DEVICE    *p_inq_rec;
    UINT8               i;
    BOOLEAN             copy = FALSE;
    int                 rssi_tgt;

    APPL_TRACE_EVENT2("btapp_dm_sort_inq_db:%d, rssi:%d",
        index, btui_inq_db.remote_device[index].rssi);

    if(index == 0 ||
        btui_inq_db.remote_device[index].rssi == BTA_DM_INQ_RES_IGNORE_RSSI)
        return;

    memcpy(&inq_rec, &btui_inq_db.remote_device[index], sizeof(tBTUI_REM_DEVICE));
    rssi_tgt = inq_rec.rssi + inq_rec.rssi_offset;
    i=index-1;
    while(i>=0)
    {
        p_inq_rec = &btui_inq_db.remote_device[i];
        if(p_inq_rec->rssi == BTA_DM_INQ_RES_IGNORE_RSSI ||
            (p_inq_rec->rssi + p_inq_rec->rssi_offset) < rssi_tgt)
        {
            APPL_TRACE_EVENT2("moving:%d to :%d", i, i+1);
            memcpy(&btui_inq_db.remote_device[i+1], p_inq_rec, sizeof(tBTUI_REM_DEVICE));
            copy = TRUE;
        }
        else
        {
            i++;
            break;
        }
        if(i==0)
            break;
        i--;
    }
    if(copy)
    {
        APPL_TRACE_EVENT2("moving:%d to :%d", index, i);
        memcpy(&btui_inq_db.remote_device[i], &inq_rec, sizeof(tBTUI_REM_DEVICE));
    }
}

/*******************************************************************************
**
** Function         btapp_dm_chk_inq_db
**
** Description      checks if the given inq_res is in the inq_db
**
**
** Returns          TRUE, is already in the inq_db
*******************************************************************************/
BOOLEAN btapp_dm_chk_inq_db(tBTA_DM_INQ_RES *p_res)
{
    tBTUI_REM_DEVICE    *p_inq_rec;
    UINT8               i;
    BOOLEAN             exist = FALSE;

    APPL_TRACE_EVENT2("btapp_dm_chk_inq_db:%d, rssi:%d",
        btui_inq_db.rem_index, p_res->rssi);
    if(btui_inq_db.rem_index)
    {
        for(i=0; i<btui_inq_db.rem_index; i++)
        {
            p_inq_rec = &btui_inq_db.remote_device[i];
            if(memcmp(p_inq_rec->bd_addr, p_res->bd_addr, BD_ADDR_LEN) == 0)
            {
                p_inq_rec->rssi = p_res->rssi;
                btapp_dm_sort_inq_db(i);
                exist = TRUE;
                break;
            }
        }
    }
    return exist;
}

/*******************************************************************************
**
** Function         btui_search_cb
**
** Description      callback to notify the completion of device search
**
**
** Returns          void
*******************************************************************************/
void btui_search_cb(tBTA_DM_SEARCH_EVT event, tBTA_DM_SEARCH *p_data)
{

    tBTUI_REM_DEVICE    *p_device_rec, *p_inquiry_rec;
    char msg_str[128];
    tBTUI_BTA_MSG       *p_event_msg;
    UINT8               *p_eir_remote_name;
    UINT8               remote_name_len;
    UINT8               *p_cached_name = NULL;
    tBTA_SERVICE_MASK   services = 0;


    APPL_TRACE_DEBUG1("search callback %d",event);
    if(event == BTA_DM_DISC_RES_EVT)
    {
        if(!btui_cb.search_services ||  (btui_cb.search_services & p_data->disc_res.services))
        {
            p_inquiry_rec = btui_get_inquiry_record(p_data->disc_res.bd_addr);

            if (p_inquiry_rec)
            {
                p_inquiry_rec->in_use = TRUE;
                p_inquiry_rec->services |= p_data->disc_res.services;
                memcpy ((void *)&p_inquiry_rec->bd_addr,
                         (const void *)p_data->disc_res.bd_addr, BD_ADDR_LEN);
                if (strlen((const char *)p_data->disc_res.bd_name))
                {
                    strncpy ((char *)p_inquiry_rec->name,
                         (char *)p_data->disc_res.bd_name, BTUI_DEV_NAME_LENGTH);
        		btapp_dm_RmtNameUpdate(p_inquiry_rec);
                }
            } else {
                return;
            }

            p_device_rec = btui_get_device_record(p_data->disc_res.bd_addr);
            if(p_device_rec)
            {
                p_device_rec->services |= p_data->disc_res.services;

                if(strlen(p_data->disc_res.bd_name))
                {
                    strncpy ((char *)p_device_rec->name,
                        (char *)p_data->disc_res.bd_name, BTUI_DEV_NAME_LENGTH);
                }
                memcpy(p_inquiry_rec, p_device_rec, sizeof(tBTUI_REM_DEVICE));

                btui_store_device(p_device_rec);
            }
        }


        APPL_TRACE_DEBUG0("search callback BTA_DM_DISC_RES_EVT: RemoteNameUpdated");

    }
    else if(event == BTA_DM_DISC_CMPL_EVT)
    {
        APPL_TRACE_DEBUG0("search callback: DiscoveryCompleted");
        btapp_dm_discovery_in_progress = DISC_IN_PROGRESS_NONE;
        btapp_dm_check_pending();

	if( btapp_possible_cancel.code == WAS_CANCELLED )
	{
	     	 btapp_possible_cancel.code = RESUME_NONE;
	}
       dtun_server_send_signal_id(DTUN_SIG_DM_DISCOVERY_COMPLETE);

    }
    else if (event == BTA_DM_INQ_RES_EVT)
    {
        UINT32 dev_class;
        sprintf (msg_str, "%02x:%02x:%02x:%02x:%02x:%02x\0",
                 p_data->inq_res.bd_addr[0], p_data->inq_res.bd_addr[1],
                 p_data->inq_res.bd_addr[2], p_data->inq_res.bd_addr[3],
                 p_data->inq_res.bd_addr[4], p_data->inq_res.bd_addr[5]);
        APPL_TRACE_EVENT0(msg_str);

        dev_class = (p_data->inq_res.dev_class[2]) |
                    (p_data->inq_res.dev_class[1] << 8) |
                    (p_data->inq_res.dev_class[0] << 16);

        //btapp_dm_DeviceFound(p_data->inq_res.bd_addr, p_data->inq_res.rssi, dev_class );

        /* If device name is already cached, then request BTA to bypass RNR */
        if (BTA_DmGetCachedRemoteName(p_data->inq_res.bd_addr, &p_cached_name) == BTA_SUCCESS)
        {
            p_data->inq_res.remt_name_not_required = TRUE;
        }

        if(btapp_dm_chk_inq_db(&p_data->inq_res))
        {
            /* already in the inq_db */
            return;
        }
        p_inquiry_rec = &btui_inq_db.remote_device[btui_inq_db.rem_index];
        p_inquiry_rec->in_use = TRUE;

        memcpy((void *)&p_inquiry_rec->bd_addr,
                    (const void *)p_data->inq_res.bd_addr, BD_ADDR_LEN);

        memcpy( p_inquiry_rec->dev_class,
                p_data->inq_res.dev_class, sizeof(DEV_CLASS));

        p_inquiry_rec->name[0] = 0;
        p_inquiry_rec->services = 0;
        p_inquiry_rec->rssi_offset = 0;
        if(p_data->inq_res.is_limited)
            p_inquiry_rec->rssi_offset = BTAPP_DM_LIMITED_RSSI_OFFSET;
        p_inquiry_rec->rssi = p_data->inq_res.rssi;

        if( p_data->inq_res.p_eir )
        {
            p_eir_remote_name = BTA_CheckEirData( p_data->inq_res.p_eir,
                                                  BTM_EIR_COMPLETE_LOCAL_NAME_TYPE,
                                                  &remote_name_len );
            if( !p_eir_remote_name )
            {
                p_eir_remote_name = BTA_CheckEirData( p_data->inq_res.p_eir,
                                                      BTM_EIR_SHORTENED_LOCAL_NAME_TYPE,
                                                      &remote_name_len );
            }

            if( p_eir_remote_name )
            {
                if( remote_name_len > BTUI_DEV_NAME_LENGTH )
                    remote_name_len = BTUI_DEV_NAME_LENGTH;

                memcpy( p_inquiry_rec->name,
                        p_eir_remote_name, remote_name_len );
                p_inquiry_rec->name[remote_name_len] = 0;
		  btapp_dm_RmtNameUpdate(p_inquiry_rec);
            }

            BTA_GetEirService( p_data->inq_res.p_eir, &services);
            APPL_TRACE_EVENT1("EIR BTA services = %08X", services);
        }
        else if (p_cached_name)
        {
            /* If cached name is available, then pass it to the app */
            if ((remote_name_len = strlen((char *)p_cached_name)) > BTUI_DEV_NAME_LENGTH)
                remote_name_len = BTUI_DEV_NAME_LENGTH;

            memcpy( p_inquiry_rec->name, p_cached_name, remote_name_len);
            p_inquiry_rec->name[remote_name_len] = 0;
            btapp_dm_RmtNameUpdate(p_inquiry_rec);
        }

        btapp_dm_DeviceFound(p_data->inq_res.bd_addr, p_data->inq_res.rssi, dev_class );

        p_device_rec = btui_get_device_record(p_data->inq_res.bd_addr);
        if(p_device_rec)
        {
            if( p_inquiry_rec->name[0] )
            {
                APPL_TRACE_EVENT1("EIR remote name = %s", p_inquiry_rec->name);
                strncpy( p_device_rec->name,
                         p_inquiry_rec->name, BTUI_DEV_NAME_LENGTH );

                p_data->inq_res.remt_name_not_required = TRUE;
            }
            else if(p_device_rec->name[0])
            {
                APPL_TRACE_EVENT1("stored remote name = %s",p_device_rec->name);
                strncpy( p_inquiry_rec->name,
                         p_device_rec->name, BTUI_DEV_NAME_LENGTH );
                /*if we know the name of the device, tell BTA not to get it */
                p_data->inq_res.remt_name_not_required = TRUE;
            }
            memcpy(p_inquiry_rec, p_device_rec, sizeof(tBTUI_REM_DEVICE));
        }
        else if( p_inquiry_rec->name[0] )
        {
            APPL_TRACE_EVENT1("EIR remote name = %s", p_inquiry_rec->name);
            p_data->inq_res.remt_name_not_required = TRUE;
        }
        else
        {
            strcpy(p_inquiry_rec->name, msg_str);
        }
        p_inquiry_rec->services |= services;

        btui_inq_db.rem_index++;
        btapp_dm_sort_inq_db((UINT8)(btui_inq_db.rem_index-1));


    }
    else if(event == BTA_DM_INQ_CMPL_EVT)
    {
    }
    else if(event == BTA_DM_DI_DISC_CMPL_EVT)
    {
        APPL_TRACE_DEBUG0("search callback: DiscoveryCompleted");

        btapp_dm_discovery_in_progress = DISC_IN_PROGRESS_NONE;
        btapp_dm_check_pending();

	if( btapp_possible_cancel.code == WAS_CANCELLED )
	{
	     	 btapp_possible_cancel.code = RESUME_NONE;
	}
       dtun_server_send_signal_id(DTUN_SIG_DM_DISCOVERY_COMPLETE);

    }
    else if(event == BTA_DM_SEARCH_CANCEL_CMPL_EVT)
    {
        APPL_TRACE_DEBUG0("search callback: DiscoveryCompleted");

        btapp_dm_discovery_in_progress = DISC_IN_PROGRESS_NONE;
        btapp_dm_check_pending();

	if( btapp_possible_cancel.code == WAS_CANCELLED )
	{
	     	 btapp_possible_cancel.code = CANCELLED;
	}
        else dtun_server_send_signal_id(DTUN_SIG_DM_DISCOVERY_COMPLETE);
    }
}

/*******************************************************************************
**
** Function         btui_discover_cb
**
** Description      callback to notify the completion of device service discovery
**
**
** Returns          void
*******************************************************************************/
static void btui_discover_cb(tBTA_DM_SEARCH_EVT event, tBTA_DM_SEARCH *p_data)
{

    tBTUI_REM_DEVICE * p_device_rec;
    tBTUI_BTA_MSG * p_event_msg;
    tDTUN_DEVICE_SIGNAL sig;

    APPL_TRACE_DEBUG1("discover callback %d",event);

    if(event == BTA_DM_DISC_RES_EVT)
    {

        APPL_TRACE_DEBUG2("discover result=0x%x, services 0x%x", p_data->disc_res.result, p_data->disc_res.services);

        /* Notify result */
        if (btapp_dm_discovery_in_progress == DISC_IN_PROGRESS_GET_SCN)
        {
           sig.hdr.id = DTUN_SIG_DM_RMT_SERVICE_CHANNEL;
           sig.hdr.len = sizeof( tDTUN_SIG_DM_RMT_SERVICE_CHANNEL);
           sig.rmt_scn.success = p_data->disc_res.result;
           sig.rmt_scn.services = p_data->disc_res.services;

           dtun_server_send_signal(&sig);
        }
        else if (btapp_dm_discovery_in_progress == DISC_IN_PROGRESS_SERVICE_DISCOVERY)
        {
            sig.hdr.id = DTUN_SIG_DM_RMT_SERVICES;
            sig.hdr.len = sizeof(tDTUN_SIG_DM_RMT_SERVICES);
            sig.rmt_services.success = (p_data->disc_res.result == BTA_SUCCESS) ? TRUE : FALSE;
            sig.rmt_services.services = p_data->disc_res.services;

            dtun_server_send_signal(&sig);
        }
        else
        {
            APPL_TRACE_DEBUG0("discover callback, no discovery in progress");
        }
    }
#if BTA_DI_INCLUDED == TRUE
    else if(event == BTA_DM_DI_DISC_CMPL_EVT)
    {

        APPL_TRACE_DEBUG1("!!!!!discover servcies %x", p_data->disc_res.services);

        if ((p_event_msg = (tBTUI_BTA_MSG *)GKI_getbuf(sizeof(tBTUI_BTA_MSG))) != NULL)
        {
            p_event_msg->hdr.event = BTUI_MMI_DISCV_CMP;
            p_event_msg->hdr.layer_specific = event;
            p_event_msg->hdr.offset         = p_data->di_disc.num_record;

            GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, p_event_msg);
        }
    }
#endif
   else if( event == BTA_DM_DISC_CMPL_EVT )
   	{

            btapp_dm_discovery_in_progress = DISC_IN_PROGRESS_NONE;
            btapp_dm_check_pending();
   	}
}


/*******************************************************************************

 $Function:        btapp_dm_db_get_device_info

 $Description:        gets the device record of a stored device.

 $Returns:        NULL if device not found. Pointer to a device structure if found. This data should
                 be copied if wanted to be used somewhere else.
                 If the device is not stored in Flash, the is_new flag is set => this means it is
                 a newly found device ( from an inquiry or discovery result ).

 $Arguments:        DB_ADDR of the device wanted.


*******************************************************************************/
tBTUI_REM_DEVICE * btapp_dm_db_get_device_info(BD_ADDR bd_addr)
{
    UINT8 i;

    for(i=0; i<BTUI_NUM_REM_DEVICE; i++)
    {
        if(    btui_device_db.device[i].in_use
            && !memcmp(btui_device_db.device[i].bd_addr, bd_addr, BD_ADDR_LEN))
        {
            return &btui_device_db.device[i];
        }
    }

    /* we didn't find our device, look into the inquiry db */
    for(i=0; i<BTUI_NUM_REM_DEVICE; i++)
    {
        if(    btui_inq_db.remote_device[i].in_use
            && !memcmp(btui_inq_db.remote_device[i].bd_addr, bd_addr, BD_ADDR_LEN))
        {
            return &btui_inq_db.remote_device[i];
        }
    }

    return NULL;

}

/*******************************************************************************
**
** Function         btapp_dm_db_get_device_list
**
** Description      gets the devices which mmets the input conditions
**
**
** Returns          void
*******************************************************************************/
BOOLEAN btapp_dm_db_get_device_list(    tBTA_SERVICE_MASK services,
                                                tBTUI_REM_DEVICE * p_device,
                                                UINT8*    number_of_devices,
                                                   BOOLEAN new_only)
{
    UINT8 i;

    *number_of_devices = 0;
    if( services == 0 )
        services = BTA_ALL_SERVICE_MASK;

    APPL_TRACE_DEBUG0("btui_get_device_list");
    APPL_TRACE_DEBUG1("btui_get_device_list - searched services = %x", services );

    for(i=0; i<BTUI_NUM_REM_DEVICE; i++)
    {
        if(new_only == FALSE )
        {
            /* first, get the stored devices - only if not new_only asked */
            if(    btui_device_db.device[i].in_use &&
                (( btui_device_db.device[i].services & services)||(services==BTA_ALL_SERVICE_MASK))  )
            {
                memcpy(&p_device[*number_of_devices], &btui_device_db.device[i], sizeof(tBTUI_REM_DEVICE));
                (*number_of_devices)++;
            }
        }


        /* then, get the new devices */
        APPL_TRACE_DEBUG1("btui_get_device_list - device services = %x",btui_inq_db.remote_device[i].services);

        if( (btui_inq_db.remote_device[i].in_use) && ( (btui_inq_db.remote_device[i].services & services)||(services==BTA_ALL_SERVICE_MASK)) )
        {
            if ( &p_device[*number_of_devices] == NULL  )
            {
                APPL_TRACE_DEBUG0("pp_device[*number_of_devices] is NULL!");
            }
            else if ( &(btui_inq_db.remote_device[i]) == NULL  )
            {
                APPL_TRACE_DEBUG0("&(btui_inq_db[i]) is NULL!");
            }
            else
                memcpy(&p_device[*number_of_devices], &(btui_inq_db.remote_device[i]), sizeof(tBTUI_REM_DEVICE));

            (*number_of_devices) ++;
        }

    }



    APPL_TRACE_DEBUG1("%i devices into inq db",(*number_of_devices));

    if(*number_of_devices == 0 )
        return BTUI_FAIL;

    return BTUI_SUCCESS;
}


/*******************************************************************************
**
** Function         btapp_dm_check_pending_get_scn
**
** Description      Called when discovery is completed/cancelled.
**                  If a get scn is pending, then start it now.
**
** Returns          void
*******************************************************************************/
void btapp_dm_check_pending(void)
{
    APPL_TRACE_DEBUG0(__FUNCTION__);
    if (btapp_dm_pending.code == PENDING_GET_SCN)
    {
        APPL_TRACE_DEBUG0("Resuming GetRemoteServiceChannel...");
        btapp_dm_pending.code = PENDING_NONE;
        btapp_dm_GetRemoteServiceChannel(&btapp_dm_pending.dtun_msg);
    }
    else if (btapp_dm_pending.code == PENDING_DISC)
    {
        APPL_TRACE_DEBUG0("Resuming DiscoverDevices...");
        btapp_dm_pending.code = PENDING_NONE;
        btapp_dm_DiscoverDevices(&btapp_dm_pending.dtun_msg);
    }
    else if( btapp_possible_cancel.code == CANCELLED )
    {
        APPL_TRACE_DEBUG0("Resuming DiscoverDevices after cancellation...");
        btapp_possible_cancel.code = RESUME_NONE;
        btapp_dm_DiscoverDevices(&btapp_possible_cancel.dtun_msg);
    }

}


/*******************************************************************************
**
**  DBUS/DTUN Interface
**
*******************************************************************************/


/*******************************************************************************/
/* Signals */



/*******************************************************************************
**
** Function         btapp_dm_DeviceFound
**
** Description      Handler for "DiscoverDevices" dbus message
**
** Returns          void
*******************************************************************************/


void btapp_dm_DeviceFound(BD_ADDR *p_bd, UINT16 rssi, UINT32 cod)
{
    tDTUN_DEVICE_SIGNAL sig;

    sig.hdr.id = DTUN_SIG_DM_DEVICE_FOUND;
    sig.hdr.len = sizeof( tDTUN_SIG_DEVICE_FOUND_INFO );
    memcpy(&sig.device_found.info.bd, p_bd, 6);
    sig.device_found.info.rssi = rssi;
    sig.device_found.info.cod = cod;

    dtun_server_send_signal(&sig);
}

/*******************************************************************************
**
** Function         btapp_dm_RmtNameUpdate
**
** Description      Handler for "DiscoverDevices" dbus message
**
** Returns          void
*******************************************************************************/


void btapp_dm_RmtNameUpdate(tBTUI_REM_DEVICE *p_device_rec)
{
    tDTUN_DEVICE_SIGNAL sig;

    if( !p_device_rec )
		return;

    if(!p_device_rec->name[0])
        return;

    sig.hdr.id = DTUN_SIG_DM_RMT_NAME;
    sig.hdr.len = sizeof( tDTUN_SIG_DM_RMT_NAME_INFO ) ;
    memcpy(&sig.rmt_name.info.bd_addr, p_device_rec->bd_addr, 6);
    strncpy ((char *)sig.rmt_name.info.bd_name,
	    (char *)p_device_rec->name, DTUN_MAX_DEV_NAME_LEN);

    dtun_server_send_signal(&sig);
}

/*******************************************************************************/
/* Methods */

/*******************************************************************************
**
** Function         btapp_dm_DiscoverDevices
**
** Description      Handler for "DiscoverDevices" dbus message
**
** Returns          void
*******************************************************************************/
void btapp_dm_DiscoverDevices(tDTUN_DEVICE_METHOD *msg)
{
    tBTA_DM_INQ inq_params;
    tBTA_SERVICE_MASK services;

    PRINTFUNC();

    if( btapp_dm_discovery_in_progress == DISC_IN_PROGRESS_GET_SCN)
    {
        APPL_TRACE_DEBUG1("%s: postponing until current get SCN  is completed/cancelled.", __FUNCTION__);
        btapp_dm_pending.code = PENDING_DISC;
        memcpy(&btapp_dm_pending.dtun_msg, msg, sizeof(tDTUN_DEVICE_METHOD));
//        BTA_DmSearchCancel();
        return;
    }

    memcpy(&btapp_possible_cancel.dtun_msg, msg, sizeof(tDTUN_DEVICE_METHOD));
    btapp_dm_discovery_in_progress = DISC_IN_PROGRESS_DEVICE_SEARCH;


    inq_params.mode = BTA_DM_GENERAL_INQUIRY;
    inq_params.max_resps = BTUI_NUM_REM_DEVICE;
    inq_params.filter_type = BTA_DM_INQ_CLR;

    if ( btl_cfg_getBDAFilterCond(inq_params.filter_cond.bd_addr) != 0 )
    {
        inq_params.filter_type = BTA_DM_INQ_BD_ADDR;
    }

    inq_params.report_dup = TRUE;
    inq_params.duration = 10;
    services = 0;

    /* Start the search */
    btui_inq_db.rem_index = 0;
    memset(&btui_inq_db, 0, sizeof(btui_inq_db));
    btui_cb.search_services = services;

    /* find nearby devices */
    BTA_DmSearch(&inq_params, services, btui_search_cb);

    /* now send discovery started notification */
    dtun_server_send_signal_id(DTUN_SIG_DM_DISCOVERY_STARTED);
}


/*******************************************************************************
**
** Function         btapp_dm_CancelDiscovery
**
** Description      Handler for "BTA_DmSearchCancel" dbus message
**
** Returns          void
*******************************************************************************/
void btapp_dm_CancelDiscovery(tDTUN_DEVICE_METHOD *msg)
{
    PRINTFUNC();

#ifdef BTLA_REL_2_X

    if( btapp_dm_discovery_in_progress == DISC_IN_PROGRESS_DEVICE_SEARCH )
    {
        LOGI("%s: Still searching[btapp_dm_discovery_in_progress =%d] so cancel", __FUNCTION__, btapp_dm_discovery_in_progress);
        BTA_DmSearchCancel();
//        dtun_server_send_signal_id(DTUN_SIG_DM_DISCOVERY_COMPLETE);
    }
    btapp_dm_discovery_in_progress = DISC_IN_PROGRESS_NONE;

#else

    BTA_DmSearchCancel();

    btapp_dm_discovery_in_progress = DISC_IN_PROGRESS_NONE;
    dtun_server_send_signal_id(DTUN_SIG_DM_DISCOVERY_COMPLETE);

#endif

}

/*******************************************************************************
**
** Function         btapp_dm_GetRemoteServiceChannel
**
** Description      Handler for "GetRemoteServiceChannel" dbus message
**                  (called prior to openning hsp/hfp or other rfcomm connection)
**
** Returns          void
*******************************************************************************/
void btapp_dm_GetRemoteServiceChannel(tDTUN_DEVICE_METHOD *msg)
{
    tBTA_SERVICE_MASK services;
    BD_ADDR bd_addr;
    tBTA_DM_SEARCH search_rsp;
    tSDP_UUID uuid;
    UINT16 uuid_16bit;
    UINT32 uuid_32bit;

    PRINTFUNC();

    /* If discovery is already in progress, then cancel it before starting SCN */
    /* bta only supports one active service discovery at a time.               */
    if (btapp_dm_discovery_in_progress != DISC_IN_PROGRESS_NONE)    /* GetSCN has priority - cancel current discovery */
    {
        APPL_TRACE_DEBUG1("%s: postponing until current service discovery is completed/cancelled.", __FUNCTION__);
        btapp_dm_pending.code = PENDING_GET_SCN;
        memcpy(&btapp_dm_pending.dtun_msg, msg, sizeof(tDTUN_DEVICE_METHOD));
	 btapp_possible_cancel.code = WAS_CANCELLED;
	 BTA_DmSearchCancel();

        return;
    }

    /* Start the discovery */
    bdcpy(bd_addr, *(BD_ADDR*)msg->get_scn.bdaddr.b);

    btapp_dm_discovery_in_progress = DISC_IN_PROGRESS_GET_SCN;

    /* find nearby devices */
    if(msg->get_scn.uuid1.type == DTUN_SDP_UUID16)
    {
        uuid.len = 2;

        uuid.uu.uuid16 = msg->get_scn.uuid1.value.uuid16;

        APPL_TRACE_DEBUG2("%s: UUID16=0x%02X", __FUNCTION__, uuid.uu.uuid16);
    }
    else if(msg->get_scn.uuid1.type == DTUN_SDP_UUID32)
    {
        uuid.len = 4;

        uuid.uu.uuid32 = msg->get_scn.uuid1.value.uuid32;

        APPL_TRACE_DEBUG1("    UUID32 = 0x%04x", uuid.uu.uuid32);

    }
    else  /* if not 16 or 32 then assume it is 128 */
    {
        uuid.len = 16;
        memcpy( &uuid.uu.uuid128, &msg->get_scn.uuid1.value.uuid128.data, 16);
        APPL_TRACE_EVENT4("using 128 bit uuid %x,%x,%x,%x, ...",
               uuid.uu.uuid128[0], uuid.uu.uuid128[1], uuid.uu.uuid128[2], uuid.uu.uuid128[3]);
    }
    APPL_TRACE_DEBUG6("Starting discovery on %02X:%02X:%02X:%02X:%02X:%02X",
        bd_addr[0],bd_addr[1],bd_addr[2],bd_addr[3],bd_addr[4],bd_addr[5]);

    BTA_DmDiscoverUUID(bd_addr, &uuid, btui_discover_cb, TRUE);
}



/*******************************************************************************
**
** Function         btapp_dm_GetRemoteServices
**
** Description      Handler for "GetRemoteServiceIdentifers" dbus message
**                  Get remote device's services
**
** Returns          void
*******************************************************************************/
void btapp_dm_GetRemoteServices(tDTUN_DEVICE_METHOD *msg)
{
    tDTUN_DEVICE_SIGNAL sig;
//    tBTA_SERVICE_MASK services = (BTA_ALL_SERVICE_MASK & ~BTA_RES_SERVICE_MASK);    /* Search for all devices supported by BTA */
    tBTA_SERVICE_MASK services = BTAPP_DM_SUPPORTED_SERVICES;

    PRINTFUNC();

    if( btapp_dm_discovery_in_progress != DISC_IN_PROGRESS_NONE)
    {
        /* Only one discovery allowed at a time */
        sig.hdr.id = DTUN_SIG_DM_RMT_SERVICES;
        sig.hdr.len = sizeof(tDTUN_SIG_DM_RMT_SERVICES);
        sig.rmt_services.success = FALSE;
	sig.rmt_services.ignore_err= TRUE;
        dtun_server_send_signal(&sig);
        return;
    }

    btapp_dm_discovery_in_progress = DISC_IN_PROGRESS_SERVICE_DISCOVERY;
    bdcpy(btapp_dm_cb.bda_sdp, *(BD_ADDR*)msg->rmt_dev.bdaddr.b);

    /* Start the discovery */
    BTA_DmDiscover(btapp_dm_cb.bda_sdp, services, btui_discover_cb, TRUE);
}


/*******************************************************************************
**
** Function         btapp_dm_GetRemoteServices
**
** Description      Handler for "GetRemoteServiceIdentifers" dbus message
**                  Get remote device's services
**
** Returns          void
*******************************************************************************/
void btapp_dm_GetAllRemoteServices(tDTUN_DEVICE_METHOD *msg)
{
    tDTUN_DEVICE_SIGNAL sig;
    //tBTA_SERVICE_MASK services = (BTA_ALL_SERVICE_MASK & ~BTA_RES_SERVICE_MASK);    /* Search for all devices supported by BTA */
    tBTA_SERVICE_MASK services = BTAPP_DM_ALL_SUPPORTED_SERVICES;

    LOGI("%s: Querying for supported servies %d", __FUNCTION__, services);

    if( btapp_dm_discovery_in_progress != DISC_IN_PROGRESS_NONE)
    {
        /* Only one discovery allowed at a time */
        sig.hdr.id = DTUN_SIG_DM_RMT_SERVICES;
        sig.hdr.len = sizeof(tDTUN_SIG_DM_RMT_SERVICES);
        sig.rmt_services.success = FALSE;
	sig.rmt_services.ignore_err= TRUE;
        dtun_server_send_signal(&sig);
        return;
    }

    btapp_dm_discovery_in_progress = DISC_IN_PROGRESS_SERVICE_DISCOVERY;
    bdcpy(btapp_dm_cb.bda_sdp, *(BD_ADDR*)msg->rmt_dev.bdaddr.b);

    /* Start the discovery */
    BTA_DmDiscover(btapp_dm_cb.bda_sdp, services, btui_discover_cb, TRUE);
}


void btapp_send_local_bd_addr( BD_ADDR p_bd )
{
    tDTUN_DEVICE_SIGNAL sig;

    memcpy(&sig.local_info.bdaddr, p_bd, 6);
    sig.hdr.id = DTUN_SIG_DM_LOCAL_INFO;
    sig.hdr.len = 6;

    dtun_server_send_signal(&sig);
}
/*******************************************************************************
**
** Function         btapp_dm_GetLocalInfo
**
** Description
**
** Returns          void
*******************************************************************************/
void btapp_dm_GetLocalInfo(tDTUN_DEVICE_METHOD *msg)
{

    PRINTFUNC();

    BTM_ReadLocalDeviceAddr( btapp_send_local_bd_addr );

}

#define MODE_OFF		0x00
#define MODE_CONNECTABLE	0x01
#define MODE_DISCOVERABLE	0x02
#define MODE_LIMITED		0x03

//Phone is connectable when it starts
uint8_t btapp_curr_set_mode = MODE_CONNECTABLE;

void btapp_dm_RecoverScanMode( void )
{
    switch( btapp_curr_set_mode )
    {
    case MODE_OFF:
	BTA_DmSetVisibility(BTA_DM_NON_DISC, BTA_DM_NON_CONN);
	break;

    case MODE_CONNECTABLE:
        BTA_DmSetVisibility(BTA_DM_NON_DISC, BTA_DM_CONN);
	break;

    case MODE_DISCOVERABLE:
        BTA_DmSetVisibility(BTA_DM_GENERAL_DISC, BTA_DM_CONN);
	break;

    case MODE_LIMITED:
        BTA_DmSetVisibility(BTA_DM_LIMITED_DISC, BTA_DM_CONN);
	break;
    }
}

void btapp_dm_DisableScanMode( void )
{
	BTA_DmSetVisibility(BTA_DM_NON_DISC, BTA_DM_NON_CONN);
}

void btapp_dm_SetMode(tDTUN_DEVICE_METHOD *msg)
{

    PRINTFUNC();

    switch( msg->set_mode.mode)
    {
    case MODE_OFF:
	BTA_DmSetVisibility(BTA_DM_NON_DISC, BTA_DM_NON_CONN);
	break;

    case MODE_CONNECTABLE:
        BTA_DmSetVisibility(BTA_DM_NON_DISC, BTA_DM_CONN);
	break;

    case MODE_DISCOVERABLE:
        BTA_DmSetVisibility(BTA_DM_GENERAL_DISC, BTA_DM_CONN);
	break;

    case MODE_LIMITED:
        BTA_DmSetVisibility(BTA_DM_LIMITED_DISC, BTA_DM_CONN);
	break;
    }

    btapp_curr_set_mode = msg->set_mode.mode;

}

void btapp_dm_SetName(tDTUN_DEVICE_METHOD *msg)
{

    PRINTFUNC();

    BTA_DmSetDeviceName( msg->set_name.name );

}

/* Mode values defined below shall match the ones in BluetoothAdapter.java. */
#define SEC_MODE_UNDEFINED   0
#define SEC_MODE_NONE        1
#define SEC_MODE_SERVICE     2
#define SEC_MODE_LINK        3
#define SEC_MODE_SP          4
#define SEC_MODE_SP_DEBUG    5

void btapp_dm_SetSecurity(tDTUN_DEVICE_METHOD *msg)
{
    UINT8 mode;

    switch (msg->set_security.mode) {
    case SEC_MODE_UNDEFINED:
        mode = BTM_SEC_MODE_UNDEFINED;
        break;
    case SEC_MODE_NONE:
        mode = BTM_SEC_MODE_NONE;
        break;
    case SEC_MODE_SERVICE:
        mode = BTM_SEC_MODE_SERVICE;
        break;
    case SEC_MODE_LINK:
        mode = BTM_SEC_MODE_LINK;
        break;
    case SEC_MODE_SP:
        mode = BTM_SEC_MODE_SP;
        break;
    case SEC_MODE_SP_DEBUG:
        mode = BTM_SEC_MODE_SP_DEBUG;
        break;
    default:
        mode = BTM_SEC_MODE_UNDEFINED;
        break;
    }
    LOGI("%s: mode = %d", __FUNCTION__, mode);

    BTM_SetSecurityMode(mode);
}


void btapp_dm_AddDev(tDTUN_DEVICE_METHOD *msg)
{
    DEV_CLASS dev_class = {0, 0, 0};

    PRINTFUNC();

        BTA_DmAddDevice(msg->add_dev.info.bd_addr.b, dev_class, msg->add_dev.info.key,
            0, 0, msg->add_dev.info.key_type, 0);
}
#endif

/*******************************************************************************
 **
 ** Function         btapp_dm_SetTestMode
 **
 ** Description      Handler for enabling/disabling the test mode interface. depreciated interface!
 **  dbus message
 **
 ** Returns          void
 *******************************************************************************/
void btapp_dm_SetTestMode( tDTUN_DEVICE_METHOD *msg )
{
    if ((DM_GET_TESTMODE_STATE == msg->set_testmode.mode)
            || (DM_SET_TRACE_LEVEL > msg->set_testmode.mode))
    {
        UINT8       testmode_res;
        tDTUN_DM_TESTMODE_STATE state = DM_DISABLE_TESTMODE;
#if defined(BTAPP_TESTMODE_INCLUDED) && (BTAPP_TESTMODE_INCLUDED==TRUE)
        tBTL_TST_SET_TESTMODE_EVT *p_msg;
        if ( NULL!= (p_msg=(tBTL_TST_SET_TESTMODE_EVT *)GKI_getbuf(sizeof(tBTL_TST_SET_TESTMODE_EVT))))
        {
            p_msg->hdr.event          = BTL_TST_BTLIF_EVENT;
            p_msg->hdr.layer_specific = BTLIF_TST_SET_TESTMODE;
            p_msg->mode               = (msg->set_testmode.mode==DM_ENABLE_TESTMODE)?
                           TST_ENABLE_TESTMODE:TST_DISABLE_TESTMODE;
            GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *)p_msg);
        } else
#endif
        {
            /* no testmode library available. return disable state */
            tDTUN_DEVICE_SIGNAL sig;

            sig.testmode_state.hdr.id = DTUN_SIG_DM_TESTMODE_STATE;
            sig.testmode_state.hdr.len = sizeof(sig.testmode_state.state);
            sig.testmode_state.state = (uint32_t) state;

            dtun_server_send_signal(&sig);
            APPL_TRACE_WARNING2( "btapp_dm_SetTestMode( mode: %d ) resulting state: %d",
                                  msg->set_testmode.mode, state );
        }
    } else
    {
        /* set trace level accordingly to lower 16 bits in the mode field */
        tBTTRC_LEVEL levels[2];

        levels[0].layer_id = (msg->set_testmode.mode & ~DM_SET_TRACE_LEVEL) >> 8;
        levels[0].type = (msg->set_testmode.mode & ~DM_SET_TRACE_LEVEL) & 0xff;

        /* list end marker */
        levels[1].layer_id = 0;
        levels[1].type = 0;
        APPL_TRACE_ERROR2( "SET_TRACE_LEVEL layer_id: x%02x, x%02x", levels[0].layer_id, levels[0].type );
        /* ignoring returned trace level settings */
        BTA_SysSetTraceLevel(&levels[0]);
    }
} /* btapp_dm_SetTestMode() */


/*******************************************************************************/
/* Method callbacks and init/shutdown code  */


void btapp_dm_dtun_start(void)
{
    APPL_TRACE_EVENT0("Starting DTUN [DM] Interface");
   // dtun_server_register_interface(DTUN_INTERFACE, (tDTUN_METHOD*)&dtun_method_tbl);
    dtun_server_start();
}

void btapp_dm_dtun_stop(void)
{
    APPL_TRACE_EVENT0("Stopping DTUN [DM] Interface");
    dtun_server_stop();
}

#undef _BTAPP_DM_C_
