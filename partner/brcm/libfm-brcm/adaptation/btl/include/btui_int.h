/*****************************************************************************
**
**  Name:             btui_int.h
**
**  Description:     This file contains btui internal interface
**                   definition
**
**  Copyright (c) 2000-2009, Broadcom Corp., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include "bte_ht_api.h"
#include "gki.h"

#if (defined(__CYGWIN__) || defined(__linux__))
#include <stdio.h>
#endif

#ifndef BTUI_INT_H
#define BTUI_INT_H

#if( defined BTA_SS_INCLUDED ) && (BTA_SS_INCLUDED == TRUE)
#include "bta_ss_api.h"
#endif
#if( defined BTA_AC_INCLUDED ) && (BTA_AC_INCLUDED == TRUE)
#include "bta_ac_api.h"
#endif
#if (defined BTA_FT_INCLUDED) && (BTA_FT_INCLUDED == TRUE)
#include "bta_ft_api.h"
#include "xml_flp_api.h"
#include "xml_vlist_api.h"
#endif

#if (defined BTA_PBS_INCLUDED) && (BTA_PBS_INCLUDED == TRUE)
#include "bta_pbs_api.h"
#endif

#if (defined BTA_PBC_INCLUDED) && (BTA_PBC_INCLUDED == TRUE)
#include "bta_pbc_api.h"
#include "xml_vlist_api.h"
#endif

#if (defined BTA_SC_INCLUDED) && (BTA_SC_INCLUDED == TRUE)
#include "bta_sc_api.h"
#endif

#if (defined BTA_AV_INCLUDED) && (BTA_AV_INCLUDED == TRUE)
#include "bta_av_api.h"
#endif

#if( defined BTA_AVK_INCLUDED ) && (BTA_AVK_INCLUDED == TRUE)
#include "bta_avk_api.h"
#endif

#if (defined BTA_PAN_INCLUDED) && (BTA_PAN_INCLUDED == TRUE)
#include "bta_pan_api.h"
#endif

#if( defined BTA_HD_INCLUDED ) && (BTA_HD_INCLUDED == TRUE)
#include "bta_hd_api.h"
#endif

#if( defined BTA_HH_INCLUDED ) && (BTA_HH_INCLUDED == TRUE)
#include "bta_hh_api.h"
#endif

/* UI IDs */
#define UI_DM_ID     1
#define UI_DG_ID     2
#define UI_AG_ID     3
#define UI_FTS_ID    4
#define UI_OPC_ID    5
#define UI_CT_ID     6
#define UI_TEST_ID   7
#define UI_FTC_ID    8
#define UI_SS_ID     9
#define UI_ACC_ID    10
#define UI_ACS_ID    11
#define UI_HD_ID     12
#define UI_PAN_ID    13
#define UI_SC_ID     14
#define UI_AV_ID     15
#define UI_PR_ID     16
#define UI_HH_ID     17
#define UI_PBS_ID    18
#define UI_PBC_ID    19
#define UI_PRM_ID    20
#define UI_FM_ID     21
#define UI_FMTX_ID   22
#define UI_HS_ID     23

/* macro for getting the
root state for a ID */
#define UI_STATE_START(id)         ((id) << 8)

typedef UINT32 tBTUI_STATE;

/* main states */
enum
{
     UI_INIT,
     UI_IDLE ,
     UI_CURRENT_CONNECTIONS
};


/* UI DM states */
enum
{
    UI_BT_SETTING             =     UI_STATE_START(UI_DM_ID),
    UI_VIEW_DEVICES,
    UI_VISIBILITY,
    UI_CHANGE_NAME,
    UI_DEVICE_NAME,
    UI_DISPLAY_DEVICES,
    UI_SEARCH_NEW_DEVICE,
    UI_VIEW_DISPLAYED_DEVICE,
    UI_VIEW_DISPLAY_DI,
    UI_LOCAL_DI,
    UI_SELECT_DEVICE ,
    UI_NEW_DEVICE_DISCOVER,
    UI_NEW_DEVICE_DISCOVER_CMPL,
    UI_PIN_REPLY,
    UI_PIN_REPLY_MANUAL,
    UI_CONFIRM_REPLY,
    UI_PASSKEY_REPLY,
    UI_OOB_C_REPLY,
    UI_OOB_R_REPLY,
    UI_AUTH_REPLY,
    UI_DEVICE_BOND,
    UI_DEVICE_BOND_MANUAL,
    UI_DISPLAY_SERVICES,
    UI_ENTER_BDADDR,
    UI_DEVICE_BOND_CANCEL
};



/* UI AG states */
enum
{

     UI_AUDIO_DEVICE        =      UI_STATE_START(UI_AG_ID),
     UI_VIEW_AUDIO_DEVICES,
     UI_AUDIO_DEVICE_CONNECTED,
     UI_AUDIO_DEVICE_SELECTED,

     UI_HF_DEVICES,
     UI_SELECT_AUDIO_DEVICE,
     UI_NEW_AUDIO_DEVICE,
     UI_NEW_HF_DEVICE,
     UI_SELECT_HF_BOND,
     UI_SELECT_AUDIO_DEVICE_BOND,
     UI_SELECT_NEW_AUDIO_DEVICE,
     UI_HS_CONNECTED,
     UI_SELECT_NEW_AUDIO_DEVICE_BOND,
     UI_SELECT_NEW_HF_DEVICE,
     UI_HF_CONNECTED,
     UI_SELECT_NEW_HF_BOND,
     UI_ACTIVE_HS_CONNECTION,
     UI_ACTIVE_HF_CONNECTION
};

/* UI HS states */
enum
{

     UI_HS_MAIN        =      UI_STATE_START(UI_HS_ID),
     UI_HS_TEST1,
     UI_HS_TEST2,
     UI_HS_TEST3,
     UI_HS_TEST4,
     UI_HS_TEST2_VTS

};



/* UI DG states */
enum
{
     UI_DG_CURRENT_CONNECTIONS  = UI_STATE_START(UI_DG_ID),
     UI_DG_VIEW_DEVICES,
     UI_DG_MAIN,
     UI_DG_DEVICE_SELECTED,
     UI_DG_LOOP_BACK,
     UI_SELECT_DG_DEVICE,
     UI_NEW_DG_DEVICE,
     UI_SELECT_NEW_DG_DEVICE,

};


/* UI OP states */
enum
{
     UI_OP_DEVICE  = UI_STATE_START(UI_OPC_ID),
     UI_VIEW_OP_DEVICES,
     UI_NEW_OP_DEVICE,
     UI_SELECT_OP_DEVICE,
     UI_SELECT_NEW_OP_DEVICE,
     UI_VIEW_OP_FILES,
     UI_SENDING_OP_FILE,

     UI_OPC_MAIN_MENU,
     UI_OPC_VIEW_OP_DEVICES,
     UI_OPC_SELECT_DEVICE,
     UI_OPC_VIEW_FILES
};

/* Cordless UI states */
enum
{
     UI_CT_MAIN_MENU  = UI_STATE_START(UI_CT_ID),


     UI_CT_VIEW_CT_DEVICES,
     UI_CT_VIEW_IC_DEVICES,
     UI_CT_ACTIVE_CONNECTION,
     UI_CT_ENTER_NUMBER,
     UI_CT_SELECT_CT_DEVICES,
     UI_CT_NAME_OR_EXTN,
     UI_CT_VIEW_IC_DEVICES_BY_NAME,
     UI_CT_VIEW_IC_DEVICES_BY_EXTN,
     UI_CT_SELECT_IC_DEVICE_BY_NAME
};

/* FT server UI states */
enum
{
     UI_FT_SERVER  = UI_STATE_START(UI_FTS_ID),
};

/* Sync server UI states */
enum
{
     UI_SS_SERVER  = UI_STATE_START(UI_SS_ID),
};

/* FT client UI states */
enum
{
     UI_FTC_MAIN_MENU  = UI_STATE_START(UI_FTC_ID),


     UI_FTC_VIEW_DEVICES,
     UI_FTC_VIEW_FILES,
     UI_FTC_VIEW_PEER_FOLDER,
     UI_FTC_PEER_FOLDER_EMPTY,
     UI_FTC_VIEW_SELECTED_FILE,
     UI_FTC_ENTER_FOLDER_NAME,
     UI_FTC_DEVICE,
     UI_VIEW_FTC_DEVICES,
     UI_NEW_FTC_DEVICE,
     UI_SELECT_FTC_DEVICE,
     UI_SELECT_NEW_FTC_DEVICE,
     UI_VIEW_FTC_FILES,
     UI_SENDING_FTC_FILE,
     UI_RECEIVING_FTC_FILE,
     UI_REMOVING_FTC_FILE
};

/* PBC client UI states */
enum
{
     UI_PBC_MAIN_MENU  = UI_STATE_START(UI_PBC_ID),


     UI_PBC_VIEW_DEVICES,
     UI_PBC_VIEW_PEER_FOLDER,
     UI_PBC_PEER_FOLDER_EMPTY,
     UI_PBC_VIEW_SELECTED_FILE,
     UI_PBC_DEVICE,
     UI_VIEW_PBC_DEVICES,
     UI_NEW_PBC_DEVICE,
     UI_SELECT_PBC_DEVICE,
     UI_SELECT_NEW_PBC_DEVICE,
     UI_PBC_VIEW_PULL_PB,
     UI_PBC_ENTER_PB_NAME,
     UI_PBC_ENTER_LIST_SEARCH_VALUE,
     UI_PBC_VIEW_PB_OPTION
};

/* AC server UI states */
enum
{
     UI_AC_SERVER  = UI_STATE_START(UI_ACS_ID),
};

/* AC client UI states */
enum
{
     UI_ACC_DEVICE  = UI_STATE_START(UI_ACC_ID),
     UI_VIEW_ACC_DEVICES,
     UI_NEW_ACC_DEVICE,
     UI_SELECT_ACC_DEVICE,
     UI_SELECT_NEW_ACC_DEVICE,
     UI_ACC_CONNECTING,
     UI_VIEW_ACC_FILES,
     UI_SENDING_ACC_FILE,
     UI_RECEIVING_ACC_FILE
};

/* AV states */
enum
{

     UI_AV_DEVICE        =      UI_STATE_START(UI_AV_ID),
     UI_AV_CURRENT_CONNECTION,
     UI_AV_VIEW_DEVICES,
     UI_AV_SELECT_DEVICE,
     UI_AV_OPEN,
     UI_AV_MAIN,
     UI_AV_ACTIVE_AUDIO,
     UI_NEW_AV_DEVICE,
     UI_SELECT_AV_DEVICE,
     UI_SELECT_NEW_AV_DEVICE
};

/* HD states */
enum
{

    UI_HD_DEVICE      =     UI_STATE_START(UI_HD_ID),
    UI_VIEW_HD_DEVICES,
    UI_NEW_HD_DEVICES,
    UI_SELECT_HD_DEVICE,
    UI_SELECT_NEW_HD_DEVICE,
    UI_NEW_HD_DEVICE
};

/* HD states */
enum
{

    UI_HH_DEVICE      =     UI_STATE_START(UI_HH_ID),
    UI_VIEW_HH_DEVICES,
    UI_NEW_HH_DEVICES,
    UI_SELECT_HH_DEVICE,
    UI_SELECT_NEW_HH_DEVICE,
    UI_NEW_HH_DEVICE
};

/* PAN states */
enum
{

    UI_PAN_MAIN      =     UI_STATE_START(UI_PAN_ID),
    UI_PAN_VIEW_DEVICES,
    UI_PAN_DEVICE_SELECTED,
    UI_PAN_SELECT_SERVICE_TO_CONNECT

};

/* SC states */
enum
{

    UI_SC_MAIN      =     UI_STATE_START(UI_SC_ID),
    UI_SC_STATUS,
    UI_SC_SERVER,
    UI_SC_SIM_CARD


};

/* PR states */
enum
{

    UI_PR_MAIN      =     UI_STATE_START(UI_PR_ID),
    UI_PR_VIEW_DEVICES,
    UI_PR_GET_CAPS,
    UI_PR_VIEW_FILES


};

/* FM states */
enum
{

    UI_FM_MAIN      =     UI_STATE_START(UI_FM_ID),
    UI_FM_TUNE_FREQ,
    UI_FM_AUDIO_MODE,
    UI_FM_ENABLE_OPT,
    UI_FM_RDS_SCAN,
    UI_FM_RDS_MODE,
    UI_FM_SCAN_MODE,
    UI_FM_FAVO_LIST,
    UI_FM_FAVO_ENTRY
};

/* UI Test states */
enum
{

     UI_TEST_MAIN = UI_STATE_START(UI_TEST_ID),
     UI_TEST_AG_MAIN,
     UI_TEST_AG_CALL,
     UI_TEST_AG_AUDIO,
     UI_TEST_AG_MISC,
     UI_TEST_AG_API,

     UI_TEST_HS_MAIN,
     UI_TEST_HS_TEST1,
     UI_TEST_HS_TEST2,
     UI_TEST_HS_TEST3,
     UI_TEST_HS_TEST4,
     UI_TEST_HS_TEST2_VTS,

     UI_TEST_DM_MAIN,
     UI_TEST_DM1,
     UI_TEST_DM1_LP_ADDR,
     UI_TEST_DM1_LP_CS,
     UI_TEST_DM2,
     UI_TEST_DM3,
     UI_TEST_DM4,
     UI_TEST_DM4_SET_ADDR,
     UI_TEST_DM4_AUTH_REQ,
     UI_TEST_DG,
     UI_TEST_OPS,
     UI_TEST_OPC,
     UI_TEST_OP_ERTM,
     UI_TEST_CT_MAIN,
     UI_TEST_CT1,
     UI_TEST_CT2,
     UI_TEST_CT3,
     UI_TEST_CT4,
     UI_TEST_CT5,
     UI_TEST_FT,
     UI_TEST_PR,
     UI_TEST_AC,
     UI_TEST_PAN,
     UI_TEST_HD,
     UI_TEST_HH,
     UI_TEST_HH_DATA,
    UI_TEST_CG,
    UI_TEST_AV_MAIN,
    UI_TEST_AV_C,
    UI_TEST_AV_A,
    UI_TEST_AV_V,
    UI_TEST_PBC,
    UI_TEST_PBC_ENTER_VALUE,
    UI_TEST_PBS,
    UI_TEST_AV_META,
    UI_TEST_AV_ADV_CTRL,
    UI_TEST_TRACE
};

#define  BTUI_DEFAULT_INQ_DURATION     10 /* in 1.28 secs */

#define BTUI_NUM_REM_DEVICE 25


#define MENU_ITEM_0 '0'
#define MENU_ITEM_1 '1'
#define MENU_ITEM_2 '2'
#define MENU_ITEM_3 '3'
#define MENU_ITEM_4 '4'
#define MENU_ITEM_5 '5'
#define MENU_ITEM_6 '6'
#define MENU_ITEM_7 '7'
#define MENU_ITEM_8 '8'
#define MENU_ITEM_9 '9'
#define MENU_ITEM_A 'A'
#define MENU_ITEM_B 'B'
#define MENU_ITEM_C 'C'
#define MENU_ITEM_D 'D'
#define MENU_ITEM_E 'E'
#define MENU_ITEM_F 'F'
#define MENU_ITEM_H 'H'
#define MENU_ITEM_I 'I'
#define MENU_ITEM_J 'J'
#define MENU_ITEM_K 'K'

typedef void (*tBTUI_ACT_NO_PARAM)(void);
typedef void (*tBTUI_ACT_ONE_PARAM)(UINT32);


extern tHT_MENU_ITEM menu_buffer[BTUI_NUM_REM_DEVICE + 1];


/* remote device */
typedef struct
{
     BOOLEAN    in_use;
     BD_ADDR    bd_addr;
     char        name[BTUI_DEV_NAME_LENGTH+1];
     UINT8      pin_code[PIN_CODE_LEN];
     DEV_CLASS dev_class;
     char        short_name[BTUI_DEV_NAME_LENGTH+1];      /* short name which user can assign to a device */
     LINK_KEY  link_key;
     UINT8      key_type;
     tBTA_IO_CAP peer_io_cap;
     BOOLEAN    link_key_present;
     BOOLEAN    is_trusted;
     tBTA_SERVICE_MASK trusted_mask;
     BOOLEAN    is_default;
     tBTA_SERVICE_MASK services;
     INT8       rssi;
     INT8       rssi_offset;

} tBTUI_REM_DEVICE;

/* connected device */
typedef struct
{
     BOOLEAN    in_use;
     BD_ADDR    bd_addr;

} tBTUI_CONN_DEVICE;
#define BTUI_MAX_CONN_DEVICE    7

/* typedef for all data that application needs to
store in nvram */
typedef struct
{
     BOOLEAN                    bt_enabled;  /* bluetooth enabled or not */
     UINT8                      count;
     char                       local_device_name[BTUI_DEV_NAME_LENGTH+1];     /* local bluetooth name */
     BOOLEAN                    visibility;
     tBTUI_REM_DEVICE           device[BTUI_NUM_REM_DEVICE];
     TIMER_LIST_ENT             dev_tle;

}
tBTUI_DEV_DB;

extern tBTUI_DEV_DB btui_device_db;

#if( defined BTA_FM_INCLUDED ) && (BTA_FM_INCLUDED == TRUE)

#define BTUI_FM_MAX_FAVORITE        BTUI_NUM_REM_DEVICE
typedef struct
{
    UINT16              freq[BTUI_FM_MAX_FAVORITE];       /* frequency */
    UINT8               name[BTUI_FM_MAX_FAVORITE][10];   /* station name */
}
tBTUI_FM_DB;
extern tBTUI_FM_DB btui_fm_db;
#endif

#if( defined BTA_HS_INCLUDED ) && (BTA_HS_INCLUDED == TRUE)
typedef struct
{
    UINT8               spk_vol;
    UINT8               mic_vol;
    BD_ADDR             las_cnt_bda;
}
tBTUI_HS_DB;
extern tBTUI_HS_DB btui_hs_db;
#endif


/* Inquiry results database */
typedef struct
{
     tBTUI_REM_DEVICE     remote_device[BTUI_NUM_REM_DEVICE];
     UINT8                    rem_index;
} tBTUI_INQ_DB;

extern tBTUI_INQ_DB btui_inq_db;

#define BTUI_MAX_MENU_CHARACTERS_STORED 600
#define BTUI_MAX_FMT_TYPES    10

#define BTUI_SCREEN_LENGTH    16

typedef void (tBTUI_MENU)( void );

#define BTUI_MAX_DEFAULT_PEERS      (2)         /* Maximum number of default peer bd address */

/* BTUI main control block */
typedef struct
{
     tBTUI_STATE ui_state;                                     /* current UI state */
     tBTUI_STATE ui_prev_state;                              /* previous UI state. Used to store the state when same menu is reached through different paths*/
     tBTUI_STATE ui_next_state;                              /* next state for the UI This is used to store the
                                                                          state when a event from BTA prempts the current state */

     UINT8   ui_input_remaining;
     tBTUI_MENU*  pin_cback_menu;
     tBTUI_MENU*  search_cback_menu;

     BD_ADDR    local_bd_addr;                                         /* local bdaddr */
     char    bd_addr_str[BD_ADDR_LEN*3];                                         /* local bdaddr */

     tBTA_SERVICE_MASK ui_current_active_connection;    /* active connection mask */
     tBTUI_REM_DEVICE * p_selected_rem_device;          /* pointer to device selected by UI */
     char   *pr_doc_fmt_types;
     BD_ADDR peer_bdaddr;                                      /* peer bdaddr stored for pin_reply etc*/
     BT_OCTET16 sp_c;
     BT_OCTET16 sp_r;
     BD_ADDR  oob_bdaddr;                                      /* peer bdaddr stored for SP OOB process etc*/
     UINT32     pass_key;
     UINT8      notif_index;
     UINT8      sp_bond_bits;
     tBTA_IO_CAP    sp_io_cap;
     BD_ADDR    sp_bond_bdaddr;                                   /* peer bdaddr stored for SP IO process etc*/
     BOOLEAN    is_dd_bond;
     char     peer_name[BTUI_DEV_NAME_LENGTH+1];          /* bluetooth name of peer device for pin reply etc */
     BOOLEAN auth_pin_menu_active;                            /* flag to indicate whether authorization
                                                                         or pin request menu is active */
     UINT32  auth_menu_handle;                                 /* handle of the menu por auth req/ pin req */


     tBTA_SERVICE_ID peer_service;                          /* service for authorization */
     UINT8 num_devices;                                         /* num_devices in db */
     UINT8 num_audio_devices;                                 /* num audio devices in db */
     UINT8 num_op_devices;                                     /* num op devices in db */
     UINT8 num_pr_devices;                                     /* num printer devices in db */
     UINT8 num_ct_devices;                                     /* num CT devices in db */
     UINT8 num_icom_devices;                                  /* num ICOM devices in db */
     UINT8 num_pan_devices;                                    /* num PAN devices in db */
     UINT8 num_hh_devices;                                      /* num HID devices in db */

     tBTUI_CONN_DEVICE     conn_dev[BTUI_MAX_CONN_DEVICE];
     UINT8 num_conn_devices;                                    /* num connected devices in db */
     UINT8 sel_conn_devices;                                    /* selected connected devices in db */

     tBTA_SERVICE_MASK search_services;                    /* services to search for */
     char menu_store[BTUI_MAX_MENU_CHARACTERS_STORED];/* current menu stored for redisplay */
     UINT16 menu_indx;                                          /* index to the menu store to current position */
     BOOLEAN record_screen;                                    /* store the strings displayed to the menu store */
     BOOLEAN get_string;                                        /* for next user input wait for a string input as opposed to a character input */
     UINT32  current_search_menu_handle;
     UINT32  current_menu_handle;
     UINT32  current_bonding_handle;
     BOOLEAN is_bonding;

     TIMER_LIST_Q  timer_queue;                              /* timer queue */
     tHT_KEY_PRESSED_CBACK  * p_search_cback;
     BOOLEAN                        loop_back_test_status;
     UINT8  menu_action;                                        /* track the currently selected menu option */

     tBTA_DISCOVERY_DB  *p_di_db;
     UINT32             di_handle;
#if ((BTU_DUAL_STACK_INCLUDED == TRUE ) || (BTU_DUAL_STACK_BTC_INCLUDED))
     BOOLEAN is_switched;
     UINT8   switch_to;
     BOOLEAN is_starting_stream;
     BOOLEAN manual_switch;
#endif

     BOOLEAN        sco_hci;

     /* Default peer bda */
     BD_ADDR peer_bda[BTUI_MAX_DEFAULT_PEERS];
} tBTUI_CB;

extern tBTUI_CB btui_cb;


extern char * btui_addr_str(UINT8 *p_addr);
extern void btui_fclose(FILE **pp_f);
extern UINT8 * btui_get_selected_dev_name(void);

extern UINT8 btui_device_db_to_menu_option_lkup_tbl[BTUI_NUM_REM_DEVICE];

extern void btui_app_start_timer (TIMER_LIST_ENT *p_tle, UINT16 type, UINT32 timeout);
extern void btui_app_stop_timer (TIMER_LIST_ENT *p_tle);

extern void btui_initial_msg (void);
extern void btui_menu_idle (void);
extern void btui_menu_setting (void);
extern void btui_menu_view_devices (void);
extern void btui_act_show_new_devices (void);
extern void btui_show_search_results (void);
extern void btui_display_stored_devices();
extern void btui_delete_device(BD_ADDR bd_addr);
extern void btui_search(tBTA_DM_INQ_MODE mode, tBTA_SERVICE_MASK services, tHT_KEY_PRESSED_CBACK  p_cback,tBTA_DM_COD_COND * inq_filt_cond);

extern void BTUI_SetInputMode(BOOLEAN on);

/* functions for displaying the menus */
extern void BTUI_putc(UINT8 ch);
extern void BTUI_puts(const char *str);
extern void BTUI_cls(void);

typedef void (tBTUI_INIT_FN)(void);
typedef void (tBTUI_EVENT_HDLR)(tBTUI_MMI_MSG *p_msg);


#define BTUI_NAME_LENGTH 32
#define BTUI_MAX_AUTH_KEY_LENGTH    16  /* must be <= BTA_FTS_MAX_AUTH_KEY_SIZE */
#define BTUI_MAX_AUTH_REALM_LENGTH 16  /* must be <= OBX_MAX_AUTH_REALM_SIZE */
#define BTUI_MAX_PATH_LENGTH         512
#define BTUI_MAX_FILENAME_LENGTH     255 //Linux length limitation, excluding \0
#define BTUI_MAX_LUID_LENGTH         100
#define BTUI_MIN_LUID_LENGTH         8
#define BTUI_MAX_DID_LENGTH          30
#define BTUI_SS_ROOT_FOLDER          "test_files\\ss"

/* type for all configuartion parameters
for the BTUI APP. */
typedef struct
{

     /* Mask for services supported by application */
     tBTA_SERVICE_MASK    supported_services;

     /* event handler for BTUI device manager */
     tBTUI_EVENT_HDLR * p_dm_event_hdlr;
     tBTA_DM_INQ_FILT     dm_inq_filt_type;
     tBTA_DM_INQ_COND     dm_inq_filt_cond;     /* Filter condition data. */

     tBTA_AUTH_REQ    sp_auth_req;
     BOOLEAN            sp_auto_reply;

     tBTUI_INIT_FN         * p_ag_init;
     tBTUI_INIT_FN         * p_ag_menu;
     tBTUI_EVENT_HDLR     * p_ag_event_hdlr;/* event handler for BTUI Audio Gateway */

     /* AG configuartion parameters */
     tBTA_SEC                ag_security;
     char                     hsag_service_name[BTUI_NAME_LENGTH + 1];
     char                     hfag_service_name[BTUI_NAME_LENGTH + 1];
     int                      ag_features;
#if (BTM_WBS_INCLUDED == TRUE)
     int                      ag_sco_codec;
#endif
     int                      ag_instances;
     BOOLEAN                  ag_bldn_enable;
     BOOLEAN                  ag_sco_over_hci;

     /* HS configuartion parameters */
     tBTA_SEC                 hs_security;
     char                     hshs_service_name[BTUI_NAME_LENGTH + 1];
     char                     hfhs_service_name[BTUI_NAME_LENGTH + 1];
     int                      hs_features;
     BOOLEAN                  hs_sco_over_hci;
     BOOLEAN                  hs_slc_auto_answer;

     tBTUI_INIT_FN          * p_hs_init;
     tBTUI_INIT_FN          * p_hs_menu;
     tBTUI_EVENT_HDLR       * p_hs_event_hdlr;/* event handler for BTUI Audio Gateway */

     BOOLEAN                  sco_use_mic;

     /* DG configuartion parameters */
     BOOLEAN                 dg_included;
     BOOLEAN                 dg_enabled;
     char                     dg_port_name[BTUI_NAME_LENGTH + 1];
     char                     dg_port_baud[BTUI_NAME_LENGTH + 1];
     tBTA_SEC                sppdg_security;
     char                     sppdg_service_name[BTUI_NAME_LENGTH + 1];
     tBTA_SEC                dundg_security;
     char                     dundg_service_name[BTUI_NAME_LENGTH + 1];
     tBTA_SEC                faxdg_security;
     char                     faxdg_service_name[BTUI_NAME_LENGTH + 1];

     BOOLEAN                 spp_loopback_mode;
     BOOLEAN                 spp_senddata_mode;

     /* DG client configuartion parameters */
     char                     dg_client_port_name[BTUI_NAME_LENGTH + 1];
     char                     dg_client_port_baud[BTUI_NAME_LENGTH + 1];
     char                     dg_client_service_id[2];
     tBTA_SEC                 dg_client_security;
     char                     dg_client_peer_name[BTUI_NAME_LENGTH + 1];

     tBTUI_INIT_FN         * p_dg_init;
     tBTUI_INIT_FN         * p_dg_menu;
     tBTUI_EVENT_HDLR     * p_dg_event_hdlr;



     /* OPS configuration parameters */
     BOOLEAN                 ops_included;
     tBTUI_INIT_FN         * p_ops_init;
     tBTA_SEC                ops_security;
     char                    ops_service_name[BTUI_NAME_LENGTH + 1];
     char                    op_owner_vcard[BTUI_MAX_PATH_LENGTH + 1];

     /* OPC configuration parameters */
     BOOLEAN                 opc_included;
     tBTUI_INIT_FN         * p_opc_init;
     tBTUI_INIT_FN         * p_opc_menu;
     tBTUI_EVENT_HDLR     * p_opc_event_hdlr;
     tBTA_SEC                opc_security;
     BOOLEAN                opc_single_op;
     char                     opc_service_name[BTUI_NAME_LENGTH + 1];

     /* Print configuration parameters */
     BOOLEAN                 pr_included;
     tBTUI_INIT_FN         * p_pr_init;
     tBTUI_INIT_FN         * p_pr_menu;
     tBTUI_EVENT_HDLR     * p_pr_event_hdlr;
     tBTA_SEC                pr_security;
     char                     pr_service_name[BTUI_NAME_LENGTH + 1];
     char                     pr_password[BTUI_MAX_AUTH_KEY_LENGTH + 1];
     char                     pr_userid[BTUI_MAX_AUTH_KEY_LENGTH + 1];

     /* FTS configuration parameters */
     BOOLEAN                 fts_included;
     tBTUI_EVENT_HDLR     * p_fts_event_hdlr;
     tBTUI_INIT_FN         * p_fts_init;
     tBTUI_INIT_FN         * p_fts_menu;
     char                     fts_service_name[BTUI_NAME_LENGTH + 1];
     char                     fts_key [BTUI_MAX_AUTH_KEY_LENGTH + 1];
     char                     fts_realm [BTUI_MAX_AUTH_REALM_LENGTH + 1];
     BOOLEAN                 fts_obex_auth;
     tBTA_SEC                fts_security;

     /* PBS configuration parameters */
     BOOLEAN                 pbs_included;
     tBTUI_EVENT_HDLR     * p_pbs_event_hdlr;
     tBTUI_INIT_FN         * p_pbs_init;
     tBTUI_INIT_FN         * p_pbs_menu;
     char                     pbs_service_name[BTUI_NAME_LENGTH + 1];
     char                     pbs_key [BTUI_MAX_AUTH_KEY_LENGTH + 1];
     char                     pbs_realm [BTUI_MAX_AUTH_REALM_LENGTH + 1];
     BOOLEAN                 pbs_obex_auth;
     tBTA_SEC                pbs_security;


     /* FTC configuration parameters */
     BOOLEAN                 ftc_included;
     tBTUI_EVENT_HDLR     * p_ftc_event_hdlr;
     tBTUI_INIT_FN         * p_ftc_init;
     tBTUI_INIT_FN         * p_ftc_menu;
     char                     ftc_password[BTUI_MAX_AUTH_KEY_LENGTH + 1];
     char                     ftc_userid[BTUI_MAX_AUTH_KEY_LENGTH + 1];
     tBTA_SEC                ftc_security;
     tBTA_SERVICE_MASK    ftc_services;

/* PBC configuration parameters */
     BOOLEAN                 pbc_included;
     tBTUI_EVENT_HDLR     * p_pbc_event_hdlr;
     tBTUI_INIT_FN         * p_pbc_init;
     tBTUI_INIT_FN         * p_pbc_menu;
     char                     pbc_password[BTUI_MAX_AUTH_KEY_LENGTH + 1];
     char                     pbc_userid[BTUI_MAX_AUTH_KEY_LENGTH + 1];
     tBTA_SEC                pbc_security;
     UINT32                 pbc_filter;
     UINT8                  pbc_format;
     UINT8                  pbc_order;
     UINT8                  pbc_attr;
     UINT16                 pbc_max_list_count;
     UINT16                 pbc_offset;

     /* ACS configuration parameters */
     BOOLEAN                 acs_included;
     tBTUI_EVENT_HDLR     * p_acs_event_hdlr;
     tBTUI_INIT_FN         * p_acs_init;
     tBTUI_INIT_FN         * p_acs_menu;
     char                     acs_service_name[BTUI_NAME_LENGTH + 1];
     tBTA_SEC                acs_security;
     BOOLEAN                 acs_req_thumb; /* request for the thumbnail version on PutImage */
#if( defined BTA_AC_INCLUDED ) && (BTA_AC_INCLUDED == TRUE)
     tBIP_FEATURE_FLAGS  acs_features;  /* supported features */
     tBIP_UINT64            acs_capacity;  /* total image capacity(in bytes) */
     char               acs_path[BTUI_MAX_PATH_LENGTH + 1];
    tBIP_IMG_HDL_STR    acs_del_img_hdl;    /* the image handle */
#endif
    UINT8               acs_del_bi_hdl;

    /* ACC configuration parameters */
    BOOLEAN             acc_included;
    tBTUI_EVENT_HDLR    * p_acc_event_hdlr;
    tBTUI_INIT_FN       * p_acc_init;
    tBTUI_INIT_FN       * p_acc_menu;
    tBTA_SEC            acc_security;
    BOOLEAN             acc_cam_get_image;
    char                acc_path[BTUI_MAX_PATH_LENGTH + 1];
    char                acc_password[BTUI_MAX_AUTH_KEY_LENGTH + 1];
    char                acc_userid[BTUI_MAX_AUTH_KEY_LENGTH + 1];

    /* SS configuration parameters */
    BOOLEAN             ss_included;
    tBTUI_EVENT_HDLR    * p_ss_event_hdlr;
    tBTUI_INIT_FN       * p_ss_init;
    tBTUI_INIT_FN       * p_ss_menu;
    char                ss_service_name[BTUI_NAME_LENGTH + 1];
    char                ss_key [BTUI_MAX_AUTH_KEY_LENGTH + 1];
    char                ss_realm [BTUI_MAX_AUTH_REALM_LENGTH + 1];
    BOOLEAN             ss_obex_auth;
    tBTA_SEC            ss_security;
#if( defined BTA_SS_INCLUDED ) && (BTA_SS_INCLUDED == TRUE)
    tBTA_SS_DATA_TYPE_MASK ss_data_types;
#endif
    int                 ss_luid_size;
    int                 ss_cl_size;
    int                 ss_did_size;

    /* SC configuration parameters */
    BOOLEAN             sc_included;
    tBTUI_EVENT_HDLR    * p_sc_event_hdlr;
    tBTUI_INIT_FN       * p_sc_init;
    tBTUI_INIT_FN       * p_sc_menu;
    char                sc_service_name[BTUI_NAME_LENGTH + 1];
    tBTA_SEC            sc_security;

    /* CT configuration parameters */
    tBTUI_INIT_FN       * p_ct_init;
    tBTUI_INIT_FN       * p_ct_menu;
    tBTUI_EVENT_HDLR    * p_ct_event_hdlr;
    tBTA_SEC            ct_security;

    /* PAN configuration parameters */
    BOOLEAN             pan_included;
    BOOLEAN             panu_supported;
    BOOLEAN             pangn_supported;
    BOOLEAN             pannap_supported;
    tBTUI_INIT_FN       * p_pan_init;
    tBTUI_INIT_FN       * p_pan_menu;
    tBTUI_EVENT_HDLR    * p_pan_event_hdlr;
    tBTA_SEC            pan_security;
    char                panu_service_name[BTUI_NAME_LENGTH + 1];
    char                pangn_service_name[BTUI_NAME_LENGTH + 1];
    char                pannap_service_name[BTUI_NAME_LENGTH + 1];
    char                pan_port_name[BTUI_NAME_LENGTH + 1];
    char                pan_port_baud[BTUI_NAME_LENGTH + 1];

    /* AV configuration parameters */
    BOOLEAN             av_included;
    tBTUI_EVENT_HDLR    * p_av_event_hdlr;
    tBTUI_INIT_FN       * p_av_init;
    tBTUI_INIT_FN       * p_av_menu;
    tBTA_SEC            av_security;
    BOOLEAN             use_avrc;
#if( defined BTA_AV_INCLUDED ) && (BTA_AV_INCLUDED == TRUE)
    tBTA_AV_FEAT        av_features;
#endif
    UINT16              av_line_speed_kbps;
    UINT16              av_line_speed_busy;
    UINT16              av_line_speed_swampd;
    UINT16              av_line_speed_count;
    UINT8               av_sampling_freq;
    UINT8               av_channel_mode;
    UINT8               av_num_blocks;
    UINT8               av_num_subbands;
    UINT8               av_allocation_mode;
    UINT8               av_max_bitpool;
    BOOLEAN             av_repeat_list;
    char                av_audio_file_name[BTUI_NAME_LENGTH + 1];
    char                av_audio_md_name[BTUI_NAME_LENGTH + 1];
    char                av_audio_file_name2[BTUI_NAME_LENGTH + 1];
    char                av_audio_md_name2[BTUI_NAME_LENGTH + 1];
    char                av_audio_service_name[BTUI_NAME_LENGTH + 1];
    char                av_video_service_name[BTUI_NAME_LENGTH + 1];
    tBTA_SERVICE_MASK   av_services_used;                      /* Mask of services to use when printing */
    BOOLEAN             av_vdp_support;
    UINT8               av_num_audio;
    BOOLEAN             av_use_player;
#if (BTU_DUAL_STACK_INCLUDED == TRUE )
    BOOLEAN             av_auto_switch;
#if (BTU_DUAL_STACK_BTC_INCLUDED == TRUE) && (BTU_DUAL_STACK_NETWORK_MUSIC_INCLUDED == TRUE)
    tBTA_DM_AUX_PATH    av_aux_path;
#endif
#endif
    UINT8               av_min_trace;
    BOOLEAN             av_suspd_rcfg;
    BOOLEAN             av_reconn_rej; /* TRUE to reconnect to rejected connection */

#if( defined BTA_AVK_INCLUDED ) && (BTA_AVK_INCLUDED == TRUE)
    /* AVK (Audio-Video Sink) configuration parameters */
    BOOLEAN             avk_included;
    tBTA_SEC            avk_security;
    tBTA_AVK_FEAT       avk_features;
    UINT8               avk_channel_mode;
    char                avk_service_name[BTUI_NAME_LENGTH + 1];
#endif

#if (defined(GPS_INCLUDED) && (GPS_INCLUDED == TRUE))
    /* GPS configuration parameters */
    BOOLEAN             gps_included;
    char                gps_nvs_name[BTUI_MAX_PATH_LENGTH + 1];
    char                gps_lto_name[BTUI_MAX_PATH_LENGTH + 1];
    UINT16              gps_timeout_sec;
    UINT16              gps_accuracy_m;
    UINT16              gps_max_loc_age_sec;
    UINT16              gps_period_ms;
    INT16               gps_max_valid_fix_cnt;
    INT16               gps_max_total_fix_cnt;
    INT16               gps_max_duration_sec;
    UINT32              gps_rf;                 /* GPS Radio */
    UINT32              gps_freq_plan;          /* GPS Frequency Plan */
    UINT32              gps_rf_wildbase;
    UINT32              gps_rf_clk_ref;
    UINT32              gps_rf_clk_div;
    tBTUI_EVENT_HDLR    * p_gps_event_hdlr;
    tBTUI_INIT_FN       * p_gps_init;
    tBTUI_INIT_FN       * p_gps_menu;
#endif

    /* HD configuration parameters */
    BOOLEAN             hd_included;
    tBTUI_INIT_FN       * p_hd_init;
    tBTUI_INIT_FN       * p_hd_menu;
    tBTUI_EVENT_HDLR    * p_hd_event_hdlr;
    char                hd_service_name[BTUI_NAME_LENGTH + 1];
    tBTA_SEC            hd_security;

    /* HH configuration parameters */
    BOOLEAN             hh_included;
    tBTUI_INIT_FN       * p_hh_init;
    tBTUI_INIT_FN       * p_hh_menu;
    tBTUI_EVENT_HDLR    * p_hh_event_hdlr;
    char                hh_service_name[BTUI_NAME_LENGTH + 1];
    tBTA_SEC            hh_security;

    /* number of devices to be found during inquiry */
    UINT32              num_inq_devices;


    /* TEST configuration parameters */
    BOOLEAN             test_included;
    tBTUI_EVENT_HDLR    * p_test_event_hdlr;
    tBTUI_INIT_FN       * p_test_init;
    tBTUI_INIT_FN       * p_test_menu;
    char                test_bd_addr1[BD_ADDR_LEN + 1];
    char                test_bd_addr2[BD_ADDR_LEN + 1];

    char                root_path[BTUI_MAX_PATH_LENGTH];

    /* type of UI, txt based or hset gui */
    BOOLEAN             hset_ui_mode;

    /* FM run time configuration */
    BOOLEAN             fm_included;
    tBTUI_EVENT_HDLR    * p_fm_event_hdlr;
    tBTUI_INIT_FN       * p_fm_init;
    tBTUI_INIT_FN       * p_fm_menu;
    UINT8               fm_func_mask;
    UINT8               fm_rssi_mono;
    UINT8               fm_rssi_stereo;
    BOOLEAN             addi_thresh;
    UINT8               fm_snr_thresh;
    UINT8               fm_cos_thresh;
    BOOLEAN             fm_live_update;
    BOOLEAN             fm_embedded_lite;

    /* FMTX run time configuration */
    BOOLEAN             fmtx_included;
    BOOLEAN             fmtx_use_player;
    UINT8               fmtx_country;

    tBTUI_EVENT_HDLR    * p_fmtx_event_hdlr;
    tBTUI_INIT_FN       * p_fmtx_init;
    tBTUI_INIT_FN       * p_fmtx_menu;

    /* reconfig baud rate */
    UINT8               reconfig_baudrate;

    /* uart clock rate */
    UINT32              uart_clock_rate;

#if (BTU_DUAL_STACK_INCLUDED == TRUE )
    /* mm baud rate */
    UINT8               mm_baudrate;
#endif
    /* patch ram */
    BOOLEAN             patchram_enable;
    UINT32              patchram_address;
    char                patchram_path[BTUI_MAX_PATH_LENGTH];

    /* sleep mode */
    UINT8               lpm_sleep_mode; /* set it by HCISU config */
    UINT8               lpm_host_stack_idle_threshold;
    UINT8               lpm_host_controller_idle_threshold;
    UINT8               lpm_bt_wake_polarity;
    UINT8               lpm_host_wake_polarity;
    UINT8               lpm_allow_host_sleep_during_sco;
    UINT8               lpm_combine_sleep_mode_and_lpm;
    UINT8               lpm_enable_uart_txd_tri_state;
    UINT8               lpm_active_connection_handling;
    UINT8               lpm_resume_timeout;

#if L2CAP_FCR_INCLUDED == TRUE
#if PORT_ENABLE_L2CAP_FCR_TEST == TRUE
    UINT8               l2c_chan_mode_opts;     /* BASIC, ERTM, and/or STM */
    tL2CAP_CFG_INFO     l2ccfg;
#endif
#if L2CAP_CORRUPT_ERTM_PKTS == TRUE
    UINT16              l2c_corrupt_test_count;
#endif
#endif  /* L2CAP_FCR_INCLUDED == TRUE */


    /* bte stack run-time configurable items */
    UINT16              l2c_hi_pri_chan_quota;  /* acl buffs for hi-pri l2c channel (0=use bte default) */

    /* Stack configurable items */
} tBTUI_CFG;

extern tBTUI_CFG btui_cfg;

#define MAX_COMMAND_LEN                 512

/* config file name & location */
typedef struct
{
    char pathname[MAX_COMMAND_LEN];
    char filename[MAX_COMMAND_LEN];
} tBTUI_CFG_FILE;
extern tBTUI_CFG_FILE btui_cfg_file;


#if( defined BTA_HH_INCLUDED ) && (BTA_HH_INCLUDED == TRUE)
/* test data for HH SetReport */
#define BTUI_MAX_HH_TEST_DATA_LEN   200
#define BTUI_MAX_HH_TEST_DATA_NUM   30

typedef struct
{
    char    data_payload[BTUI_MAX_HH_TEST_DATA_LEN];
    tBTA_HH_RPT_TYPE   rpt_type;
    UINT8               rpt_id;
} tBTUI_HH_TEST_DATA;

typedef struct
{
    UINT8   test_option_num;
    tBTUI_HH_TEST_DATA  test_data[BTUI_MAX_HH_TEST_DATA_NUM];
} tBTUI_HH_CFG;

extern tBTUI_HH_CFG btui_hh_cfg;
#endif

extern void btui_dg_init(void);
extern void btui_dg_platform_init(void);
extern void btui_menu_dg_connections(void);

extern void btui_ag_init(void);
extern void btui_ag_handle_call_event(UINT16 handle, UINT8 event, UINT8 app_id);

extern void btui_hs_init(void);

extern void btui_fts_init(void);
extern void btapp_pbs_init(void);

extern void btui_ftc_init(void);
extern void btui_pbc_init(void);

extern UINT32 btui_ftc_get_file_size(char *p_name);

extern void btui_acs_init(void);

extern void btui_acc_init(void);

extern void btui_acc_disable(void);

extern void btui_act_init(tBTUI_MMI_MSG *p_msg);

extern void btui_ss_init(void);

extern void btui_cfg_init(void);

extern void btui_startup(void);

extern void btui_dm_init(void);

extern void btui_ops_init(void);

extern void btui_opc_init(void);

extern void btui_pr_init(void);

extern void btui_sc_init(void);            /* SIM card server */

extern void btui_ct_init(void);

extern void btui_test_init(void);

extern char *btui_opc_platform_get_path(UINT8 id, char **p_name);
extern int btui_platform_opc_list_files(void);

int btui_platform_filemenu(char menu_name[], const char filepath[], tHT_KEY_PRESSED_CBACK menu_cback);
char *btui_platform_pr_filemenu_get_sel(UINT32 h_menu);

extern const char BTUI_PR_FILEPATH[];

enum
{
     BTUI_IMG_ERR_NONE,    /* OK */
     BTUI_IMG_ERR_EOF,     /* EOF */
     BTUI_IMG_ERR_BAD      /* bad format */
};
extern UINT8 btui_jpg_scan_header (char *p_name, UINT16 *p_height, UINT16 *p_width);

extern UINT8 btui_bmp_scan_header (char *p_name, UINT16 *p_height, UINT16 *p_width);
extern UINT8 btui_gif_scan_header (char * p_name, UINT16 *p_height, UINT16 *p_width);

#if (defined BTA_FT_INCLUDED) && (BTA_FT_INCLUDED == TRUE)
extern void btui_ftc_platform_get_thumb(char *path, char *p_name);
extern BOOLEAN btui_ftc_platform_get_path(UINT8 id, char *p_path, tBTA_FTC_PARAM *p_param);
extern void btui_ftc_platform_get_root(char *p_path);
extern int  btui_platform_ftc_list_files(UINT8 type);
#endif

#if (defined BTA_PBC_INCLUDED) && (BTA_PBC_INCLUDED == TRUE)
extern void btui_pbc_platform_get_root(char *p_path);
#endif

enum
{
     BTUI_FILE_ANY,  /* any kind of file */
     BTUI_FILE_IMAGE /* image files */
};


extern void btui_pan_init(void);

extern void btui_av_init(void);

extern void btui_gps_init(void);

extern void btui_hd_init(void);

extern void btui_hh_init(void);

extern void btui_fm_init(void);

extern void btui_fmtx_init(void);

/* functions related to device data base */
extern void btui_act_delete_all_stored_device(void);
extern void btui_init_device_db (void);
extern void btui_nv_init_device_db(void);
extern void btui_nv_store_device_db(void);
extern void btui_store_visibility_setting(BOOLEAN visibility);
extern void btui_store_bt_enable_setting(BOOLEAN enabled);
extern void btui_store_local_name(char * p_name);
extern void btui_nv_init_wug_db(void);
extern void btui_nv_store_wug_db(void);
extern void btui_nv_store_fm_db(void);
extern void btui_nv_init_fm_db(void);
extern void btui_nv_store_hs_db(void);
extern void btui_nv_init_hs_db(void);


extern void btui_platform_startup(void);
extern tBTUI_REM_DEVICE * btui_get_inquiry_record(BD_ADDR bd_addr);
extern tBTUI_REM_DEVICE * btui_get_device_record(BD_ADDR bd_addr);
extern tBTUI_REM_DEVICE * btui_alloc_device_record(BD_ADDR bd_addr);
extern char * btui_get_dev_name(BD_ADDR bd_addr);
extern BOOLEAN btui_store_device( tBTUI_REM_DEVICE * p_rem_device);
extern UINT8 btui_display_devices(tBTA_SERVICE_MASK services,  tHT_KEY_PRESSED_CBACK  p_menu_callback, UINT8* sft_key2_msg);

extern void btui_proc_link_change(BD_ADDR bd_addr, BOOLEAN is_up);

extern void btui_platform_load_configuration(void);
extern void btui_platform_set_root_path(void);
extern void btui_platform_load_hh_test_data(void);
extern void btui_platform_configure_stack(void);

extern void btui_menu_cback(UINT32 handle, UINT32 key);
extern void btui_show_new_devices (tHT_KEY_PRESSED_CBACK  p_menu_callback);

#define BTUI_TIMEOUT_MSG_PERIOD      3000


extern void ascii_2_bdaddr (char *p_ascii, UINT8 *p_bd);
extern void btui_bdaddr_2_ascii (UINT8 *p_bd, char *p_ascii);


/* BTUI functions for displaying menus and message boxes */
extern UINT32 btui_edit_box(UINT32 flags, tHT_KEY_PRESSED_CBACK *p_cback,
                                UINT8 *p_prompt_text);

extern UINT32 btui_message_box(UINT8 *p_text, tHT_KEY_PRESSED_CBACK *p_cback);

extern UINT32 btui_create_menu(tHT_MENU_ITEM *p_menu_items, UINT32 menu_item_count,
                                tHT_KEY_PRESSED_CBACK *p_cback,  UINT8 *p_prompt_text);

extern UINT32 btui_create_top_menu(tHT_MENU_ITEM *p_menu_items, UINT32 menu_item_count,
                                tHT_KEY_PRESSED_CBACK *p_cback,  UINT8 *p_prompt_text);

extern void btui_close_object(UINT32 handle);


/* Call Table related changes */
enum
{
    BTUI_CALL_STATE_NONE,       /* Call doesn't exist */
    BTUI_CALL_STATE_INCOMING,   /* Call is in incoming state */
    BTUI_CALL_STATE_OUTGOING,   /* Call is in outgoing state */
    BTUI_CALL_STATE_ACTIVE,     /* Call is active */
    BTUI_CALL_STATE_ACTIVE_HELD /* Call is active and held */
};

enum
{
    BTUI_CALL_DIR_MO,           /* Mobile Originated Call */
    BTUI_CALL_DIR_MT            /* Mobile Terminated Call */
};

#define BTUI_MAX_DIAL_DIGITS            15
#define BTUI_MAX_CALLS                  3

typedef struct
{
    BOOLEAN         is_in_use;
    char            dial_num[BTUI_MAX_DIAL_DIGITS+1]; /* For dial cmd, copy the number HF sends,
                                                       * for other cases use dial_num_to_use */
    UINT8           call_index;
    UINT8           call_state;
    UINT8           call_dir;
    char            dial_num_to_use[BTUI_MAX_DIAL_DIGITS+1];
                    /* This is used to generate an outgoing call or an incoming call */
} tBTUI_CALL_DATA;

extern tBTUI_CALL_DATA     btui_call_table[];
extern tBTUI_REM_DEVICE stored_devices_list[BTUI_NUM_REM_DEVICE];

extern void btui_ag_get_call_stats(UINT8 *num_active, UINT8 *num_held);
extern void btui_ag_release_calls_in_state(UINT8 call_state);
extern tBTUI_CALL_DATA* btui_ag_get_call_table_entry(void);
extern tBTUI_CALL_DATA* btui_ag_get_call_data_by_index(UINT8 index);
extern tBTUI_CALL_DATA* btui_ag_get_call_data_in_call_setup(void);
extern BOOLEAN btui_ag_is_in_call_setup(void);
extern BOOLEAN btui_ag_is_in_incall_setup(void);
extern void btui_ag_reset_call_table(void);
extern UINT8 btui_ag_calc_held_ind(void);
extern void btui_ag_release_call_by_index(UINT8 index);

#endif /*BTUI_INT_H*/
