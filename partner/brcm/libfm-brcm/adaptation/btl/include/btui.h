/*****************************************************************************
**                                                                           *
**  Name:          btui.h                                                    *
**                                                                           *
**  Description:    This is the interface file for the btui                  *
**                                                                           *
**                                                                           *
**  Copyright (c) 2000-2009, Broadcom Corp., All Rights Reserved.            *
**  Widcomm Bluetooth Core. Proprietary and confidential.                    *
******************************************************************************/
#ifndef BTUI_H
#define BTUI_H

#include "bta_api.h"
#include "bte_appl.h"

#if( defined BTA_FM_INCLUDED ) && (BTA_FM_INCLUDED == TRUE)
#include "bta_fm_api.h"
#endif
#if( defined BTA_FMTX_INCLUDED ) && (BTA_FMTX_INCLUDED == TRUE)
#include "bta_fmtx_api.h"
#endif
#if( defined BTA_AC_INCLUDED ) && (BTA_AC_INCLUDED == TRUE)
#include "bta_ac_api.h"
#endif
#if( defined BTA_HS_INCLUDED ) && (BTA_HS_INCLUDED == TRUE)
#include "bta_hs_api.h"
#endif
#if( defined BTA_AV_INCLUDED ) && (BTA_AV_INCLUDED == TRUE)
#include "bta_av_api.h"
#endif

/* event masks for events to the task */
#define BTUI_MMI_EVT_MASK       BTAPP_APPL_MAIL_EVENT(0x01)  /* events from mmi to the BTUI application */
#define BTUI_BTA_EVT_MASK       BTAPP_APPL_MAIL_EVENT(0x02)  /* events from BTA to the BTUI application */
#define BTUI_MMI_AVK_EVT_MASK   BTAPP_APPL_MAIL_EVENT(0x04)  /* events from btui_avsnk MMI to the BTUI application */
#define BTAPP_TESTMODE_EVT_MASK BTAPP_APPL_MAIL_EVENT(0x08)  /* events ipsocket or bta to btui/application (bte_appl) */

/* defines the events (.event in BT_HDR) as per BTA or other sub systems (see BTA ids in bta_sys.h) */
#define BTUI_MMI_EVENT(x)       (BTUI_MMI_EVT_MASK|(x & 0xff))  /* events from mmi to the BTUI application */
#define BTUI_BTA_EVENT(x)       (BTUI_BTA_EVT_MASK|(x & 0xff))  /* events from BTA to the BTUI application */
#define BTUI_MMI_AVK_EVENT(x)   (BTUI_MMI_AVK_EVT_MASK|(x & 0xff))  /* events from btui_avsnk MMI to the BTUI application */
#define BTAPP_TESTMODE_EVENT(x) (BTAPP_TESTMODE_EVT_MASK|(x & 0xff))  /* events ipsocket or bta to btui/application (bte_appl) */

enum {
    BTUI_UNKNOWN_DEVICE    ,
    BTUI_DATA_BASE_FULL    ,
    BTUI_NOT_INITIALISED,
    BTUI_FAILED_TO_STORE_EVENT    ,
    BTUI_INVALID_DATA,
    BTUI_INVALID_EVENT,
    BTUI_INVALID_ORIG,
    BTUI_UNABLE_TO_CREATE_EVT_BUF,
    BTUI_FAIL,
    BTUI_SUCCESS
 };

typedef UINT8 tBTUI_STATUS;


/* events from MMI */
enum
{
    BTUI_MMI_CMD      =     BTUI_MMI_EVT_MASK
};

/* events from BTA */
enum
{

    BTUI_MMI_CONN_UP      =     BTUI_BTA_EVT_MASK,
    BTUI_MMI_CONN_DOWN,
    BTUI_MMI_PIN_REQ,
    BTUI_MMI_CFM_REQ,
    BTUI_MMI_KEY_NOTIF,
    BTUI_MMI_KEY_PRESS,
    BTUI_MMI_RMT_OOB,
    BTUI_MMI_AUTH_REQ,
    BTUI_MMI_AUTH_CMPL,
    BTUI_MMI_DISCV_CMP,
    BTUI_MMI_INQ_CMP,
    BTUI_MMI_INQ_RES,
    BTUI_MMI_SEARCH_DISCV_RES,
    BTUI_MMI_DEVICE_DISCV_RES,
    BTUI_MMI_LINK_DOWN,
    BTUI_MMI_LINK_UP,
    BTUI_MMI_ENABLE,
    BTUI_MMI_DISABLE,
    BTUI_MMI_INQ_RES_EVT,
    BTUI_MMI_INQ_CMPL_EVT,
    BTUI_MMI_DISC_RES_EVT,
    BTUI_MMI_DISC_CMPL_EVT,
    BTUI_MMI_SEARCH_CANCEL_CMPL_EVT,
    BTUI_MMI_BOND_CANCEL_CMPL_EVT,
    BTUI_MMI_SCO_DATA,
    BTUI_MMI_SCO_DATA_DONE,
#if ((BTU_DUAL_STACK_INCLUDED == TRUE) || (BTU_STACK_LITE_ENABLED == TRUE))
    BTUI_MMI_SWITCH_STACK_CMPL_EVT,
#endif

    BTUI_MMI_AG_AUDIO_OPEN,
    BTUI_MMI_AG_HS_ACTIVE,
    BTUI_MMI_AG_HS_INACT,
    BTUI_MMI_AG_AUDIO_CLOSE,

    BTUI_MMI_HS_OPEN,
    BTUI_MMI_HS_ENABLE,
    BTUI_MMI_HS_DISABLE,
    BTUI_MMI_HS_CLOSE,
    BTUI_MMI_HS_CONN,
    BTUI_MMI_HS_CONN_LOST,
    BTUI_MMI_HS_AUDIO_OPEN,
    BTUI_MMI_HS_AUDIO_CLOSE,
    BTUI_MMI_HS_OK,
    BTUI_MMI_HS_RING,
    BTUI_MMI_HS_VGM,
    BTUI_MMI_HS_VGS,
    BTUI_MMI_HS_CIEV,
    BTUI_MMI_HS_CCWA,
    BTUI_MMI_HS_CIND,
    BTUI_MMI_HS_BSIR,
    BTUI_MMI_HS_BVRA,
    BTUI_MMI_HS_CHLD,
    BTUI_MMI_HS_BINP,
    BTUI_MMI_HS_CNUM,
    BTUI_MMI_HS_BTRH,
    BTUI_MMI_HS_UNAT,
    BTUI_MMI_HS_COPS,
    BTUI_MMI_HS_CMEE,
    BTUI_MMI_HS_CLCC,
    BTUI_MMI_HS_CLIP,
    BTUI_MMI_HS_BCS,
    BTUI_MMI_HS_AG_ACTIVE,
    BTUI_MMI_HS_AG_INACT,
    BTUI_MMI_HS_ERR,



    BTUI_MMI_CT_CALL_INCOMING,
    BTUI_MMI_CT_CALL_CONN,
    BTUI_MMI_CT_CALL_DISCONN,


    BTUI_MMI_OPC_OBJECT,
    BTUI_MMI_OPC_CLOSE,
    BTUI_MMI_OPC_PSHD,

    BTUI_MMI_OPS_OBJECT,

    BTUI_MMI_SC_OPEN,
    BTUI_MMI_SC_CLOSE,

    BTUI_MMI_FTC_ENABLE,
    BTUI_MMI_FTC_OPEN,
    BTUI_MMI_FTC_BI_CAPS,
    BTUI_MMI_FTC_THUMBNAIL,
    BTUI_MMI_FTC_CLOSE,
    BTUI_MMI_FTC_LIST,
    BTUI_MMI_FTC_REMOVE,
    BTUI_MMI_FTC_MKDIR,
    BTUI_MMI_FTC_CHDIR,
    BTUI_MMI_FTC_PUT,
    BTUI_MMI_FTC_GET,

    BTUI_MMI_PBC_ENABLE,
    BTUI_MMI_PBC_OPEN,
    BTUI_MMI_PBC_CLOSE,
    BTUI_MMI_PBC_LIST,
    BTUI_MMI_PBC_CHDIR,
    BTUI_MMI_PBC_GET,

    BTUI_MMI_HD_OPEN,
    BTUI_MMI_HD_CLOSE,

    BTUI_MMI_PR_GETCAPS,
    BTUI_MMI_PR_PROGRESS,
    BTUI_MMI_PR_THUMBNAIL,
    BTUI_MMI_PR_CLOSE,

    BTUI_MMI_AV_AUDIO_EOF_EVT,
    BTUI_MMI_AV_AUDIO_FERR_EVT,
    BTUI_MMI_AV_AUDIO_ERR_EVT,
#if (defined BTA_FM_INCLUDED &&(BTA_FM_INCLUDED == TRUE))
    BTUI_MMI_AV_AUDIO_FM_CFGED,
#endif
    BTUI_MMI_AV_VIDEO_EOF_EVT,
    BTUI_MMI_AV_VIDEO_FERR_EVT,

    BTUI_MMI_ACC_ENABLE,
    BTUI_MMI_ACC_OPEN,
    BTUI_MMI_ACC_CLOSE,
    BTUI_MMI_ACC_TEST_STATUS,
    BTUI_MMI_ACC_GET_MONITOR,
    BTUI_MMI_ACC_THUMBNAIL,
    BTUI_MMI_ACC_GET_IMAGE,
    BTUI_MMI_ACC_PROPERTIES,
    BTUI_MMI_ACC_CLOSE_MENU,
    BTUI_MMI_ACC_ARCHIVE_CLOSE,

    BTUI_MMI_ACS_OPEN,
    BTUI_MMI_ACS_PUT_FINAL,

    BTUI_MMI_SS_ENABLED,
    BTUI_MMI_SS_DISABLED,
    BTUI_MMI_SS_SYNC_CMD,

    BTUI_MMI_HH_ENABLE,
    BTUI_MMI_HH_OPEN,
    BTUI_MMI_HH_CLOSE,
    BTUI_MMI_HH_GETDSCP,
    BTUI_MMI_HH_SETRPT,
    BTUI_MMI_HH_VCUNPLUG,

    BTUI_MMI_FM_ENABLE,
    BTUI_MMI_FM_BT_OPEN,
    BTUI_MMI_FM_TUNED,
    BTUI_MMI_FM_AF_JUMP,
    BTUI_MMI_FM_SEARCHED,
    BTUI_MMI_FM_SCH_CMPL,
    BTUI_MMI_FM_AUDIO_DATA,
    BTUI_MMI_FM_AUDIO_MODE,
    BTUI_MMI_FM_AUD_MUTE,
    BTUI_MMI_FM_SCAN_STEP,
    BTUI_MMI_FM_DEEMPHA,
    BTUI_MMI_FM_BT_DROP,
    BTUI_MMI_RDS_PS,
    BTUI_MMI_RDS_PTY,
    BTUI_MMI_RDS_PTYN,
    BTUI_MMI_RDS_RT,
    BTUI_MMI_RDS_RTP,
    BTUI_MMI_RDS_AF,
    BTUI_MMI_RDS_INDICATOR,
    BTUI_MMI_RDS_MODE,
    BTUI_MMI_FM_DISABLE,

    BTUI_MMI_FMTX_ENABLE,
    BTUI_MMI_FMTX_TUNE,
    BTUI_MMI_FMTX_MUTE,
    BTUI_MMI_FMTX_RDS_CMPL,
    BTUI_MMI_FMTX_BEST_CHNL,
    BTUI_MMI_FMTX_DISABLE

#if (defined(GPS_INCLUDED) && (GPS_INCLUDED == TRUE))
    ,BTUI_MMI_GPS_ENABLE
    ,BTUI_MMI_GPS_PERIODIC
    ,BTUI_MMI_GPS_SINGLE
    ,BTUI_MMI_GPS_DISABLE
#endif
};


#define BTUI_MMI_DM_EVT_FIRST BTUI_MMI_CONN_UP
#if ((BTU_DUAL_STACK_INCLUDED == TRUE) || (BTU_STACK_LITE_ENABLED == TRUE))
#define BTUI_MMI_DM_EVT_LAST  BTUI_MMI_SWITCH_STACK_CMPL_EVT
#else
#define BTUI_MMI_DM_EVT_LAST  BTUI_MMI_BOND_CANCEL_CMPL_EVT
#endif

#define BTUI_MMI_AG_EVT_FIRST BTUI_MMI_AG_AUDIO_OPEN
#define BTUI_MMI_AG_EVT_LAST  BTUI_MMI_AG_AUDIO_CLOSE

#define BTUI_MMI_HS_EVT_FIRST BTUI_MMI_HS_OPEN
#define BTUI_MMI_HS_EVT_LAST  BTUI_MMI_HS_ERR

#define BTUI_MMI_CT_EVT_FIRST BTUI_MMI_CT_CALL_INCOMING
#define BTUI_MMI_CT_EVT_LAST  BTUI_MMI_CT_CALL_DISCONN

#define BTUI_MMI_OPC_EVT_FIRST BTUI_MMI_OPC_OBJECT
#define BTUI_MMI_OPC_EVT_LAST  BTUI_MMI_OPC_PSHD

#define BTUI_MMI_OPS_EVT_FIRST BTUI_MMI_OPS_OBJECT
#define BTUI_MMI_OPS_EVT_LAST  BTUI_MMI_OPS_OBJECT

#define BTUI_MMI_SC_EVT_FIRST BTUI_MMI_SC_OPEN
#define BTUI_MMI_SC_EVT_LAST  BTUI_MMI_SC_CLOSE

#define BTUI_MMI_FTC_EVT_FIRST BTUI_MMI_FTC_ENABLE
#define BTUI_MMI_FTC_EVT_LAST  BTUI_MMI_FTC_GET

#define BTUI_MMI_PBC_EVT_FIRST BTUI_MMI_PBC_ENABLE
#define BTUI_MMI_PBC_EVT_LAST  BTUI_MMI_PBC_GET

#define BTUI_MMI_HD_EVT_FIRST BTUI_MMI_HD_OPEN
#define BTUI_MMI_HD_EVT_LAST  BTUI_MMI_HD_CLOSE

#define BTUI_MMI_HH_EVT_FIRST BTUI_MMI_HH_ENABLE
#define BTUI_MMI_HH_EVT_LAST  BTUI_MMI_HH_VCUNPLUG

#define BTUI_MMI_PR_EVT_FIRST BTUI_MMI_PR_GETCAPS
#define BTUI_MMI_PR_EVT_LAST  BTUI_MMI_PR_CLOSE

#define BTUI_MMI_AV_EVT_FIRST BTUI_MMI_AV_AUDIO_EOF_EVT
#define BTUI_MMI_AV_EVT_LAST  BTUI_MMI_AV_VIDEO_FERR_EVT

#define BTUI_MMI_ACC_EVT_FIRST BTUI_MMI_ACC_ENABLE
#define BTUI_MMI_ACC_EVT_LAST  BTUI_MMI_ACC_ARCHIVE_CLOSE

#define BTUI_MMI_ACS_EVT_FIRST BTUI_MMI_ACS_OPEN
#define BTUI_MMI_ACS_EVT_LAST  BTUI_MMI_ACS_PUT_FINAL

#define BTUI_MMI_SS_EVT_FIRST BTUI_MMI_SS_ENABLED
#define BTUI_MMI_SS_EVT_LAST  BTUI_MMI_SS_SYNC_CMD

#define BTUI_MMI_FM_EVT_FIRST BTUI_MMI_FM_ENABLE
#define BTUI_MMI_FM_EVT_LAST  BTUI_MMI_FM_DISABLE

#define BTUI_MMI_FMTX_EVT_FIRST BTUI_MMI_FMTX_ENABLE
#define BTUI_MMI_FMTX_EVT_LAST  BTUI_MMI_FMTX_DISABLE

#if (defined(GPS_INCLUDED) && (GPS_INCLUDED == TRUE))
#define BTUI_MMI_GPS_EVT_FIRST BTUI_MMI_GPS_ENABLE
#define BTUI_MMI_GPS_EVT_LAST  BTUI_MMI_GPS_DISABLE
#endif

#define BTUI_DATA_LEN          56
#define BTUI_DEV_NAME_LENGTH   148

/* data type for MMI message */
typedef struct
{
    BT_HDR    hdr;
    UINT8     data[BTUI_DATA_LEN+1];
} tBTUI_MMI_MSG;

/* data type for connection up event */
typedef struct
{
    BT_HDR    hdr;
    BD_ADDR   bd_addr;
    tBTA_STATUS status;
    tBTA_SERVICE_ID service;

} tBTUI_CONN_UP_MSG;

/* data type for connection down event */
typedef struct
{
    BT_HDR    hdr;
    tBTA_SERVICE_ID service;
    BD_ADDR   bd_addr;
    UINT8     status;

} tBTUI_CONN_DOWN_MSG;

/* data type for pin request message */
typedef struct
{
    BT_HDR    hdr;
    BD_ADDR   bd_addr;
    char      dev_name[BTUI_DEV_NAME_LENGTH+1];
    DEV_CLASS dev_class;
} tBTUI_PIN_REQ_MSG;

/* data type for user confirm request message */
typedef struct
{
    BT_HDR    hdr;
    BD_ADDR   bd_addr;
    char      dev_name[BTUI_DEV_NAME_LENGTH+1];
    DEV_CLASS dev_class;
    UINT32    num_val;        /* the numeric value for comparison. If just_works, do not show this number to UI */
    BOOLEAN   just_works;     /* TRUE, if "Just Works" association model */
} tBTUI_CFM_REQ_MSG;

/* data type for user confirm request message */
typedef struct
{
    BT_HDR    hdr;
    BD_ADDR   bd_addr;
    char      dev_name[BTUI_DEV_NAME_LENGTH+1];
    DEV_CLASS dev_class;
    UINT32    passkey;        /* the numeric value for comparison. If just_works, do not show this number to UI */
} tBTUI_KEY_NOT_MSG;

typedef struct
{
    BT_HDR              hdr;
    BD_ADDR             bd_addr;
    tBTA_SP_KEY_TYPE    notif_type;
} tBTUI_KEY_PRE_MSG;

/* data type for remote oob request message */
typedef struct
{
    BT_HDR    hdr;
    BD_ADDR   bd_addr;
    char      dev_name[BTUI_DEV_NAME_LENGTH+1];
    DEV_CLASS dev_class;
} tBTUI_RMT_OOB_MSG;

typedef struct
{
    BT_HDR    hdr;
    BD_ADDR   bd_addr;
    char      dev_name[BTUI_DEV_NAME_LENGTH+1];
    tBTA_SERVICE_ID service;

} tBTUI_AUTH_REQ_MSG;


typedef struct
{
    BT_HDR      hdr;
    BD_ADDR    bd_addr;
    UINT8        dev_name[BTUI_DEV_NAME_LENGTH+1];
    BOOLEAN        is_success;
}
tBTUI_DM_AUTH_CMPL;

typedef struct
{
    BT_HDR    hdr;
    BD_ADDR    bd_addr;
    UINT8        dev_name[BTUI_DEV_NAME_LENGTH+1];
    tBTA_SERVICE_MASK services;
    tBTA_STATUS       result;
} tBTUI_DISC_MSG;

typedef struct
{
       BT_HDR      hdr;
    BD_ADDR    bd_addr;
}
tBTUI_DM_LINK_CHANGE;

typedef struct
{
    BT_HDR      hdr;
    UINT16       data;
    /* length should be AG_AT_STR_MAX_LENGTH. Don't know where it is defined! */
    char            str[50];
} tBTUI_BTA_AG_AT_MSG;

/* data type for MMI message */
typedef struct
{
    BT_HDR    hdr;
    BT_HDR     *data;
    UINT8       app_id;
} tBTUI_DATA_MSG;

/* data type for MMI message */
typedef struct
{
    BT_HDR    hdr;
    UINT8     status;
} tBTUI_OPC_CLOSE;

/* data type for MMI message */
typedef struct
{
    BT_HDR    hdr;
    UINT8     format;
    char      name[100];/* move */
} tBTUI_OPS_OBJECT;

typedef struct
{
    BT_HDR    hdr;
    UINT8     status;

} tBTUI_FTC_STATUS;

typedef struct
{
    BT_HDR    hdr;
    UINT8     status;

} tBTUI_PBC_STATUS;

typedef struct
{
    BT_HDR    hdr;
    UINT8     status;

} tBTUI_SS_STATUS;

typedef struct
{
    BT_HDR    hdr;
    UINT8     status;
    BOOLEAN   final;
    BOOLEAN   parse_status;
} tBTUI_FTC_LIST;

typedef struct
{
    BT_HDR    hdr;
    UINT8     status;
    BOOLEAN   final;
    BOOLEAN   parse_status;
} tBTUI_PBC_LIST;

typedef struct
{
    BT_HDR    hdr;
    tBTA_SERVICE_MASK   services;

} tBTUI_PR_GETCAPS;

typedef struct
{
    BT_HDR    hdr;
    UINT8     bta_event;

} tBTUI_ACC_ARCHIVE_CBACK;

typedef struct
{
    BT_HDR    hdr;
    UINT8     obx_rsp;

} tBTUI_ACC_TEST_STATUS;

typedef struct
{
    BT_HDR    hdr;
    UINT8     img_hdl[/*change*/ 20];

} tBTUI_ACC_GET_MONITOR;

typedef struct
{
    BT_HDR    hdr;
    UINT8     img_hdl[/*change*/ 20];

} tBTUI_ACC_GET_THUMBNAIL;

typedef struct
{
    BT_HDR    hdr;
    UINT32    menu;
} tBTUI_ACC_CLOSE_MENU;

#if( defined BTA_FM_INCLUDED ) && (BTA_FM_INCLUDED == TRUE)

typedef struct
{
    BT_HDR        hdr;
} tBTUI_FM_RDS_DATA;


typedef struct
{
    BT_HDR      hdr;
    UINT16      num_af;
    UINT16      af_list[BTA_FM_AF_MAX_NUM];
    UINT8       af_type[BTA_FM_AF_MAX_NUM];
} tBTUI_FM_AF_DATA;

typedef struct
{
    BT_HDR      hdr;
    UINT8       indicator;
    UINT8       enable;
} tBTUI_FM_RDS_IND;

typedef struct
{
    BT_HDR          hdr;
    tBTA_FM_STATUS  status;
    UINT8           rssi;
    UINT16          freq;
} tBTUI_FM_CHNL_DATA;
#endif
#if( defined BTA_FMTX_INCLUDED ) && (BTA_FMTX_INCLUDED == TRUE)
typedef struct
{
    BT_HDR          hdr;
    tBTA_FMTX_STATUS  status;
    UINT16          freq;
} tBTUI_FMTX_CHNL_DATA;

#endif
#if( defined BTA_HS_INCLUDED ) && (BTA_HS_INCLUDED == TRUE)
typedef struct
{
    BT_HDR          hdr;
    tBTA_HS         data;
}tBTUI_HS_DATA;
#endif
typedef struct
{
    BT_HDR          hdr;
    void         *p_data;
} tBTUI_AG_SCO_DATA;
typedef struct
{
    BT_HDR      hdr;
    tBTUI_STATUS    result;
} tBTUI_BOND_CANCEL_CMPL;

#if (BTU_DUAL_STACK_INCLUDED == TRUE)
typedef struct
{
    BT_HDR      hdr;
    tBTUI_STATUS    result;
} tBTUI_SWITCH_STACK_CMPL;
#endif

#if (defined(GPS_INCLUDED) && (GPS_INCLUDED == TRUE))
typedef struct
{
    BT_HDR      hdr;
    tBTUI_STATUS    result;
} tBTUI_GPS_EVT;
#endif

/* union of all message type */
typedef union
{
    BT_HDR    hdr;
    tBTUI_CONN_UP_MSG   open;
    tBTUI_CONN_DOWN_MSG close;
    tBTUI_PIN_REQ_MSG   pin_req;
    tBTUI_CFM_REQ_MSG   cfm_req;
    tBTUI_KEY_NOT_MSG   key_notif;
    tBTUI_KEY_PRE_MSG   key_press;
    tBTUI_RMT_OOB_MSG   rmt_oob;
    tBTUI_AUTH_REQ_MSG  auth_req;
    tBTUI_DISC_MSG disc_result;
    tBTUI_DM_AUTH_CMPL auth_cmpl;
    tBTUI_DM_LINK_CHANGE link_change;
    tBTUI_BTA_AG_AT_MSG ag_msg;
    tBTUI_DATA_MSG data_msg;
    tBTUI_OPC_CLOSE opc_close;
    tBTUI_OPS_OBJECT ops_object;
    tBTUI_FTC_STATUS ftc_status;
    tBTUI_PBC_STATUS pbc_status;
    tBTUI_SS_STATUS  ss_status;
    tBTUI_FTC_LIST   ftc_list;
    tBTUI_PBC_LIST   pbc_list;
    tBTUI_PR_GETCAPS get_caps;
    tBTUI_ACC_ARCHIVE_CBACK acc_archive_cback;
    tBTUI_ACC_TEST_STATUS  acc_test_status;
    tBTUI_ACC_GET_MONITOR  acc_get_monitor;
    tBTUI_ACC_GET_THUMBNAIL acc_get_thumbmail;
    tBTUI_ACC_CLOSE_MENU    acc_close_menu;
#if( defined BTA_FM_INCLUDED ) && (BTA_FM_INCLUDED == TRUE)
    tBTUI_FM_RDS_DATA rds_data;
    tBTUI_FM_AF_DATA af_data;
    tBTUI_FM_RDS_IND rds_ind;
    tBTUI_FM_CHNL_DATA  chnl_data;
#endif
#if( defined BTA_FMTX_INCLUDED ) && (BTA_FMTX_INCLUDED == TRUE)
    tBTUI_FMTX_CHNL_DATA    set_freq;
#endif
    tBTUI_BOND_CANCEL_CMPL bond_cancel_cmpl;
#if (BTU_DUAL_STACK_INCLUDED == TRUE)
    tBTUI_SWITCH_STACK_CMPL switch_stack_cmpl;
#endif
    tBTUI_AG_SCO_DATA   sco_data;
#if( defined BTA_HS_INCLUDED ) && (BTA_HS_INCLUDED == TRUE)
    tBTUI_HS_DATA             hs;
#endif
#if( defined GPS_INCLUDED ) && (GPS_INCLUDED == TRUE)
    tBTUI_GPS_EVT           gps;
#endif
} tBTUI_BTA_MSG;


extern UINT8 appl_trace_level;

extern void btui_test_event_hdlr(tBTUI_MMI_MSG *p_msg);

/* ************************************************ */
#define BTUI_FM_EVENT BTUI_BTA_EVENT(BTA_ID_FM)

/* Enumeration for FM/PBS task */
enum
{
        BTUI_FM_TASK_EVT,
        BTUI_PBS_TASK_EVT
};

/* Enumeration for FM EVT */
enum
{
        BTUI_FM_TUNE_CMD,
        BTUI_FM_MUTE_CMD,
        BTUI_FM_SEARCH_CMD,
        BTUI_FM_COMBO_SEARCH_CMD,
        BTUI_FM_SET_RDS_MODE_CMD,
        BTUI_FM_SET_RDS_RBDS_CMD,
        BTUI_FM_SET_AUDIO_MODE_CMD,
        BTUI_FM_SET_AUDIO_PATH_CMD,
        BTUI_FM_SET_SCAN_STEP_CMD,
        BTUI_FM_SET_REGION_CMD,
        BTUI_FM_SET_DEEMPH_CMD,
        BTUI_FM_SET_NFL_CMD,
        BTUI_FM_RD_AUDIO_QUALITY_CMD,
        BTUI_FM_SET_SIGNAL_NOTIFICATION_CMD,
        BTUI_FM_VOLUME_CMD,
        BTUI_FM_SET_SNR_THRES_CMD,

        BTUI_FM_ENABLE_EVT,
        BTUI_FM_DISABLE_EVT,
        BTUI_FM_AF_JMP_EVT,
        BTUI_FM_TUNE_EVT,
        BTUI_FM_SEARCH_CMPL_EVT,
        BTUI_FM_SEARCH_EVT,
        BTUI_FM_AUDIO_MODE_EVT,
        BTUI_FM_RDS_UPD_EVT,
        BTUI_FM_AUDIO_DATA_EVT,
        BTUI_FM_AUDIO_PATH_EVT,
        BTUI_FM_RDS_MODE_EVT,
        BTUI_FM_SET_DEEMPH_EVT,
        BTUI_FM_MUTE_AUDIO_EVT,
        BTUI_FM_SCAN_STEP_EVT,
        BTUI_FM_SET_REGION_EVT,
        BTUI_FM_SET_NFL_EVT,
        BTUI_FM_SET_RDS_TYPE_EVT,
        BTUI_FM_RESET_PARSER_TASK_EVT,
        BTUI_FM_PARSE_RDS_TASK_EVT,
        BTUI_FM_VOLUME_EVT,
        BTUI_FM_SET_SNR_THRES_EVT
};

#if( defined BTA_FM_INCLUDED ) && (BTA_FM_INCLUDED == TRUE)
/* ************************************** */
/* FM commands structures */

typedef struct {
    BT_HDR hdr;
    UINT16 freq;
} tFM_RDS_TUNE_CMD;

typedef struct {
    BT_HDR hdr;
    BOOLEAN mute;
} tFM_RDS_MUTE_AUDIO_CMD;

typedef struct {
    BT_HDR hdr;
    tBTA_FM_SCAN_MODE pending_search_scan_mode;
    UINT8 pending_search_rssi;
    tBTA_FM_SCH_RDS_TYPE pending_search_cond_type;
    UINT8 pending_search_cond_val;
} tFM_RDS_SEARCH_CMD;

typedef struct {
    BT_HDR hdr;
    UINT16 pending_combo_search_start_freq;
    UINT16 pending_combo_search_end_freq;
    UINT8 pending_combo_search_rssi;
    UINT8 pending_combo_search_direction;
    tBTA_FM_SCAN_METHOD pending_combo_search_scan_method;
    BOOLEAN pending_combo_search_multi_channel;
    tBTA_FM_SCH_RDS_TYPE pending_combo_search_cond_type;
    UINT8 pending_combo_search_cond_val;
} tFM_RDS_COMBO_SEARCH_CMD;

typedef struct {
    BT_HDR hdr;
    BOOLEAN rds_on;
    BOOLEAN af_on;
} tFM_RDS_SET_RDS_MODE_CMD;

typedef struct {
    BT_HDR hdr;
    UINT8 rds_type;
} tFM_RDS_SET_RDS_RBDS_CMD;

typedef struct {
    BT_HDR hdr;
    UINT8 audio_mode;
} tFM_RDS_SET_AUDIO_MODE_CMD;

typedef struct {
    BT_HDR hdr;
    UINT8 audio_path;
} tFM_RDS_SET_AUDIO_PATH_CMD;

typedef struct {
    BT_HDR hdr;
    UINT8 scan_step;
} tFM_RDS_SET_SCAN_STEP_CMD;

typedef struct {
    BT_HDR hdr;
    UINT8 region;
} tFM_RDS_SET_REGION_CMD;

typedef struct {
    BT_HDR hdr;
    UINT8 deemphasis;
} tFM_RDS_SET_DEEMPH_CMD;

typedef struct {
    BT_HDR hdr;
    UINT8 nfloor;
} tFM_RDS_SET_NFL_CMD;

typedef struct {
    BT_HDR hdr;
    BOOLEAN turn_on;
} tFM_RDS_RD_AUDIO_QUALITY_CMD;

typedef struct {
    BT_HDR hdr;
    UINT32 notif;
} tFM_RDS_SET_SIGNAL_NOTIF_CMD;

typedef struct {
    BT_HDR  hdr;
    UINT16  level;
} tFM_VOLUME_CMD;

typedef struct {
    BT_HDR  hdr;
    INT8  snr;
} tFM_SET_SNR_THRES_CMD;

/* ************************************** */
/* FM events structures */
typedef struct {
    BT_HDR hdr;
    UINT16 len;
    UINT8 data[255];
} tFM_RDS_PARSER_EVT;

typedef struct {
    BT_HDR hdr;
    UINT16 status;
} tFM_RDS_STATUS_EVT;

typedef struct {
    BT_HDR hdr;
    UINT16 status;
    UINT16 freq;
    UINT16 rssi;
    INT8   snr;
} tFM_RDS_FREQ_EVT;

typedef struct {
    BT_HDR hdr;
    UINT16 status;
    UINT16 audio_mode;
} tFM_RDS_AUDIO_MODE_EVT;

typedef struct {
    BT_HDR hdr;
    UINT16 status;
    UINT16 data;
    UINT16 index;
    UINT8 s1[65];
} tFM_RDS_RDS_UPD_EVT;

typedef struct {
    BT_HDR hdr;
    UINT16 status;
    UINT16 rssi;
    INT8   snr;
    UINT16 audio_mode;
} tFM_RDS_AUDIO_DATA_EVT;


typedef struct {
    BT_HDR hdr;
    UINT16 status;
    UINT16 audio_path;
} tFM_RDS_AUDIO_PATH_EVT;

typedef struct {
    BT_HDR hdr;
    UINT16 status;
    BOOLEAN rds_on;
    BOOLEAN af_on;
} tFM_RDS_RDS_AF_MODE_EVT;

typedef struct {
    BT_HDR hdr;
    UINT16 status;
    UINT16 time_const;
} tFM_RDS_RDS_SET_DEEMPH_EVT;

typedef struct {
    BT_HDR hdr;
    UINT16 status;
    BOOLEAN is_mute;
} tFM_RDS_MUTE_AUDIO_EVT;

typedef struct {
    BT_HDR hdr;
    UINT16 scan_step;
} tFM_RDS_SCAN_STEP_EVT;

typedef struct {
    BT_HDR hdr;
    UINT16 status;
    UINT16 region;
} tFM_RDS_SET_REGION_EVT;

typedef struct {
    BT_HDR hdr;
    UINT16 rssi;
} tFM_RDS_SET_NFL_EVT;

typedef struct {
    BT_HDR hdr;
    UINT16 status;
    UINT16 type;
} tFM_RDS_SET_RDS_TYPE_EVT;

typedef struct {
    BT_HDR          hdr;
    tBTA_FM_VOLUME  vol;
} tFM_VOLUME_EVT;

typedef struct {
    BT_HDR hdr;
    UINT16 status;
    INT8 snr;
} tFM_SET_SNR_THRES_EVT;

typedef union
{
    BT_HDR hdr;

    tFM_RDS_TUNE_CMD tune_cmd;
    tFM_RDS_MUTE_AUDIO_CMD mute_cmd;
    tFM_RDS_SEARCH_CMD search_cmd;
    tFM_RDS_COMBO_SEARCH_CMD combo_search_cmd;
    tFM_RDS_SET_RDS_MODE_CMD rds_mode_cmd;
    tFM_RDS_SET_RDS_RBDS_CMD rds_rbds_cmd;
    tFM_RDS_SET_AUDIO_MODE_CMD audio_mode_cmd;
    tFM_RDS_SET_AUDIO_PATH_CMD audio_path_cmd;
    tFM_RDS_SET_SCAN_STEP_CMD scan_step_cmd;
    tFM_RDS_SET_REGION_CMD region_cmd;
    tFM_RDS_SET_DEEMPH_CMD deemph_cmd;
    tFM_RDS_SET_NFL_CMD nfloor_cmd;
    tFM_RDS_RD_AUDIO_QUALITY_CMD audio_quality_cmd;
    tFM_RDS_SET_SIGNAL_NOTIF_CMD signal_notif_cmd;
    tFM_VOLUME_CMD volume_cmd;
    tFM_SET_SNR_THRES_CMD set_snr_cmd;

    tFM_RDS_PARSER_EVT parser;
    tFM_RDS_STATUS_EVT st;
    tFM_RDS_FREQ_EVT af_jmp;
    tFM_RDS_FREQ_EVT tune;
    tFM_RDS_FREQ_EVT search_cmpl;
    tFM_RDS_FREQ_EVT search;
    tFM_RDS_AUDIO_MODE_EVT audio_mode;
    tFM_RDS_RDS_UPD_EVT rds_upd;
    tFM_RDS_AUDIO_DATA_EVT audio_data;
    tFM_RDS_AUDIO_PATH_EVT audio_path;
    tFM_RDS_RDS_AF_MODE_EVT rds_af_mode;
    tFM_RDS_RDS_SET_DEEMPH_EVT deemphasis;
    tFM_RDS_MUTE_AUDIO_EVT mute_stat;
    tFM_RDS_SCAN_STEP_EVT scan_step;
    tFM_RDS_SET_REGION_EVT region_info;
    tFM_RDS_SET_NFL_EVT nfloor;
    tFM_RDS_SET_RDS_TYPE_EVT rds_type;
    tFM_VOLUME_EVT volume_evt;
}  tBTUI_FM_RDS_EVT;

/* ************************************************ */
#endif


extern void btui_mmi_evt_hdlr(tBTUI_MMI_MSG *p_msg);
extern void btui_bta_evt_hdlr(tBTUI_BTA_MSG *p_msg);
extern void btui_main_init (void);
extern void btui_bta_dm_evt_hdlr(tBTUI_BTA_MSG *p_msg);
extern void btui_bta_ag_evt_hdlr(tBTUI_BTA_MSG *p_msg);
extern void btui_bta_hs_evt_hdlr(tBTUI_BTA_MSG *p_msg);
extern void btui_bta_ct_evt_hdlr(tBTUI_BTA_MSG *p_msg);
extern void btui_bta_ftc_evt_hdlr(tBTUI_BTA_MSG *p_msg);
extern void btui_bta_pbc_evt_hdlr(tBTUI_BTA_MSG *p_msg);
extern void btui_bta_opc_evt_hdlr(tBTUI_BTA_MSG *p_msg);
extern void btui_bta_ops_evt_hdlr(tBTUI_BTA_MSG *p_msg);
extern void btui_bta_sc_evt_hdlr(tBTUI_BTA_MSG *p_msg);
extern void btui_bta_hd_evt_hdlr(tBTUI_BTA_MSG *p_msg);
extern void btui_bta_acc_evt_hdlr(tBTUI_BTA_MSG *p_msg);
extern void btui_bta_acs_evt_hdlr(tBTUI_BTA_MSG *p_msg);
extern void btui_bta_pr_evt_hdlr(tBTUI_BTA_MSG *p_msg);
extern void btui_bta_ss_evt_hdlr(tBTUI_BTA_MSG *p_msg);
extern void btui_bta_av_evt_hdlr(tBTUI_BTA_MSG *p_msg);
extern void btui_bta_hh_evt_hdlr(tBTUI_BTA_MSG *p_msg);
extern void btui_bta_fm_evt_hdlr(tBTUI_BTA_MSG *p_msg);
extern void btui_bta_fmtx_evt_hdlr(tBTUI_BTA_MSG *p_msg);
#if (defined(GPS_INCLUDED) && (GPS_INCLUDED == TRUE))
extern void btui_bta_gps_evt_hdlr(tBTUI_BTA_MSG *p_msg);
#endif

/* Local Constants */
#ifndef BTUI_AG_SECURITY
#define BTUI_AG_SECURITY (BTA_SEC_AUTHORIZE | BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT)
#endif
#ifndef BTUI_HSAG_SERVICE_NAME
#define BTUI_HSAG_SERVICE_NAME "Headset Gateway"
#endif
#ifndef BTUI_HFAG_SERVICE_NAME
#define BTUI_HFAG_SERVICE_NAME "Handsfree Gateway"
#endif

#ifndef BTUI_HS_SECURITY
#define BTUI_HS_SECURITY (BTA_SEC_AUTHORIZE | BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT)
#endif

#ifndef BTUI_HSHS_SERVICE_NAME
#define BTUI_HSHS_SERVICE_NAME "BRCM Headset"
#endif
#ifndef BTUI_HFHS_SERVICE_NAME
#define BTUI_HFHS_SERVICE_NAME "BRCM Handsfree"
#endif

#ifndef BTUI_SPPDG_SECURITY
#define BTUI_SPPDG_SECURITY (BTA_SEC_AUTHORIZE | BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT)
#endif
#ifndef BTUI_SPPDG_SERVICE_NAME
#define BTUI_SPPDG_SERVICE_NAME "Serial Port"
#endif

#ifndef BTUI_DUNDG_SECURITY
#define BTUI_DUNDG_SECURITY (BTA_SEC_AUTHORIZE | BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT)
#endif

#ifndef BTUI_DUNDG_SERVICE_NAME
#define BTUI_DUNDG_SERVICE_NAME "Dial-up Networking"
#endif

#ifndef BTUI_FAXDG_SECURITY
#define BTUI_FAXDG_SECURITY (BTA_SEC_AUTHORIZE | BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT)
#endif

#ifndef BTUI_FAXDG_SERVICE_NAME
#define BTUI_FAXDG_SERVICE_NAME "Fax"
#endif


/* SIM Card server defaults */
#ifndef BTUI_SC_SECURITY
#define BTUI_SC_SECURITY (BTA_SEC_AUTHORIZE | BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT)
#endif
#ifndef BTUI_SC_SERVICE_NAME
#define BTUI_SC_SERVICE_NAME "SIM Access"
#endif


#ifndef BTUI_OPS_SECURITY
#define BTUI_OPS_SECURITY (BTA_SEC_AUTHORIZE | BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT)
#endif

#ifndef BTUI_OPS_SERVICE_NAME
#define BTUI_OPS_SERVICE_NAME "Object Push"
#endif

#ifndef BTUI_OPC_SECURITY
#define BTUI_OPC_SECURITY (BTA_SEC_AUTHORIZE | BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT)
#endif

#ifndef BTUI_OPC_SERVICE_NAME
#define BTUI_OPC_SERVICE_NAME "Object Push"
#endif

#ifndef BTUI_FTS_SECURITY
#define BTUI_FTS_SECURITY (BTA_SEC_AUTHORIZE | BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT)
#endif

#ifndef BTUI_FTS_SERVICE_NAME
#define BTUI_FTS_SERVICE_NAME "OBEX File Transfer"
#endif

#ifndef BTUI_FTS_KEY
#define BTUI_FTS_KEY "0000"
#endif

#ifndef BTUI_FTS_REALM
#define BTUI_FTS_REALM "guest"
#endif

#ifndef BTUI_PBS_SECURITY
#define BTUI_PBS_SECURITY (BTA_SEC_AUTHORIZE | BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT)
#endif

#ifndef BTUI_PBS_SERVICE_NAME
#define BTUI_PBS_SERVICE_NAME "Phonebook Access PSE"
#endif

#ifndef BTUI_PBS_KEY
#define BTUI_PBS_KEY "0000"
#endif

#ifndef BTUI_PBS_REALM
#define BTUI_PBS_REALM "guest"
#endif

#ifndef BTUI_ACC_SECURITY
#define BTUI_ACC_SECURITY (BTA_SEC_AUTHORIZE | BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT)
#endif

#ifndef BTUI_ACC_CAM_GET_IMAGE
#define BTUI_ACC_CAM_GET_IMAGE  TRUE
#endif

#ifndef BTUI_ACS_SECURITY
#define BTUI_ACS_SECURITY (BTA_SEC_AUTHORIZE | BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT)
#endif

#ifndef BTUI_ACS_SERVICE_NAME
#define BTUI_ACS_SERVICE_NAME "Advanced Camera"
#endif

#ifndef BTUI_ACS_REQUEST_THUMB
#define BTUI_ACS_REQUEST_THUMB  TRUE
#endif

#if( defined BTA_AC_INCLUDED ) && (BTA_AC_INCLUDED == TRUE)
#ifndef BTUI_ACS_FEATURES
#define BTUI_ACS_FEATURES   (BIP_FT_IMG_PUSH|BIP_FT_IMG_PULL|BIP_FT_REMOTE_CAMERA)
#endif
#endif

#ifndef BTUI_ACS_CAPACITY
#define BTUI_ACS_CAPACITY   "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
#endif

#define BTUI_SS_SECURITY (BTA_SEC_AUTHORIZE | BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT)

#define BTUI_SS_SERVICE_NAME "OBEX Synchronization"

#define BTUI_SS_KEY "0000"

#define BTUI_SS_REALM "guest"

#define BTUI_SS_DATA_TYPES (BTA_SS_API_VCARD | BTA_SS_API_VCAL | BTA_SS_API_VNOTE | BTA_SS_API_VMSG)

#define SS_MAX_LUID_SIZE        50
#define SS_MAX_CL_ENTRIES       20
#define SS_MAX_PIM_ENTRIES      40
#define SS_MAX_DID_SIZE         10

#define BTUI_MENU_ACT_BOND          0x1
#define BTUI_MENU_ACT_BOND_N_DISC   0x2

#endif
