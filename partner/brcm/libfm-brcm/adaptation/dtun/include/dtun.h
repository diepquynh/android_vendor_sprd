/************************************************************************************
 *
 *  Copyright (C) 2009-2011 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ************************************************************************************/

#ifndef DTUN_COMMON_H
#define DTUN_COMMON_H

#include <stdint.h>
#include <pthread.h>

#ifndef __BLUETOOTH_H
/* BD Address */
typedef struct {
    uint8_t b[6];
} __attribute__((packed)) bdaddr_t;
#endif

#ifndef false
#define false (0)
#endif

#ifndef true
#define true (1)
#endif

/* this needs to be in sync with compiled btld. this is also used internaly by btld */
#ifndef DTUN_BRCM_PROP_PREFIX
#define DTUN_BRCM_PROP_PREFIX "service.brcm.bt."
#endif

/* prefix: typically above string, property: commen base property name without prefix */
#define DTUN_BRCM_PROP(property) DTUN_BRCM_PROP_PREFIX property
#define DTUN_PERSIST_BRCM_PROP(property) "persist." DTUN_BRCM_PROP_PREFIX property

#ifndef DTUN_PROPERTY_BT_ACTIVATION
#define DTUN_PROPERTY_BT_ACTIVATION DTUN_BRCM_PROP("activation")
#endif


#define DTUN_PREFIX_OFFSET (sizeof(DTUN_BRCM_PROP_PREFIX)-1)

/* DTUN SDP UUID types */
#ifndef DTUN_SDP_UUID16
#define DTUN_SDP_UUID16 0x00
#endif

#ifndef DTUN_SDP_UUID32
#define DTUN_SDP_UUID32 0x01
#endif

#ifndef DTUN_SDP_UUID128
#define DTUN_SDP_UUID128 0x02
#endif

typedef int boolean;

typedef unsigned short tDTUN_ID;
typedef unsigned short tDTUN_LEN;
typedef unsigned long  tDTUN_ERR;


typedef enum {
    DTUN_CMD_BASE = 0x1101,
    DTUN_METHOD_CALL,
    DTUN_SIGNAL_EVT,
    DTUN_CMD_END
} tDTUN_CTRL_MSG_IDS;

typedef enum {
    DTUN_METHOD_BEGIN,

    /* DM methods */
    DTUN_METHOD_DM_GET_LOCAL_INFO,
    DTUN_METHOD_DM_START_DISCOVERY,
    DTUN_METHOD_DM_CANCEL_DISCOVERY,
    DTUN_METHOD_DM_GET_REMOTE_SERVICE_CHANNEL,
    DTUN_METHOD_DM_GET_REMOTE_SERVICES,
    DTUN_METHOD_DM_GET_ALL_REMOTE_SERVICES,
    DTUN_METHOD_DM_CREATE_BONDING,
    DTUN_METHOD_DM_REMOVE_BONDING,
    DTUN_METHOD_DM_PIN_REPLY,
    DTUN_METHOD_DM_PIN_NEG_REPLY,
    DTUN_METHOD_DM_AUTHORIZE_RSP,
    DTUN_METHOD_DM_SET_MODE,
    DTUN_METHOD_DM_SET_NAME,
    DTUN_METHOD_DM_ADD_DEV,
    DTUN_METHOD_DM_SSP_CONFIRM,
    DTUN_METHOD_DM_DISC_RMT_DEV,
    DTUN_METHOD_DM_ADD_SDP_REC,
    DTUN_METHOD_DM_DEL_SDP_REC,
    DTUN_METHOD_DM_SET_TESTMODE,       /* enable/disable test mode APIs */
    DTUN_METHOD_DM_SET_SECURITY,

    /* AV methods */
    DTUN_METHOD_AM_AV_OPEN,
    DTUN_METHOD_AM_AV_DISC,
    DTUN_METHOD_AM_AV_STARTSTOP,

    /* OBEX methods */
    DTUN_METHOD_OPC_ENABLE,
    DTUN_METHOD_OPC_CLOSE,
    DTUN_METHOD_OPS_CLOSE,
    DTUN_METHOD_OPC_PUSH_OBJECT,
    DTUN_METHOD_OPC_PULL_VCARD,
    DTUN_METHOD_OPC_EXCH_VCARD,

    DTUN_METHOD_OP_GRANT_ACCESS,
    DTUN_METHOD_OP_SET_OWNER_VCARD,
    DTUN_METHOD_OP_SET_EXCHANGE_FOLDER,
    DTUN_METHOD_OP_CREATE_VCARD,
    DTUN_METHOD_OP_STORE_VCARD,
    DTUN_METHOD_END,

    DTUN_SIG_BEGIN,

    /* DM signals */
    DTUN_SIG_DM_LOCAL_INFO,
    DTUN_SIG_DM_DISCOVERY_STARTED,
    DTUN_SIG_DM_DISCOVERY_COMPLETE,
    DTUN_SIG_DM_DEVICE_FOUND,
    DTUN_SIG_DM_RMT_NAME,
    DTUN_SIG_DM_RMT_SERVICE_CHANNEL,
    DTUN_SIG_DM_RMT_SERVICES,
    DTUN_SIG_DM_PIN_REQ,
    DTUN_SIG_DM_AUTHORIZE_REQ,
    DTUN_SIG_DM_AUTH_COMP,
    DTUN_SIG_DM_LINK_DOWN,
    DTUN_SIG_DM_SSP_CFM_REQ,
    DTUN_SIG_DM_LINK_UP,
    DTUN_SIG_DM_SDP_REC_HANDLE,
    DTUN_SIG_DM_TESTMODE_STATE,

    /* AV signals */
    DTUN_SIG_AM_AV_EVENT,

    /* OPC signals */
    DTUN_SIG_OPC_ENABLE,
    DTUN_SIG_OPC_OPEN,               /* Connection to peer is open. */
    DTUN_SIG_OPC_PROGRESS,           /* Object being sent or received. */
    DTUN_SIG_OPC_OBJECT_RECEIVED,
    DTUN_SIG_OPC_OBJECT_PUSHED,
    DTUN_SIG_OPC_CLOSE,              /* Connection to peer closed. */

    /* OPS signals */
    DTUN_SIG_OPS_PROGRESS,
    DTUN_SIG_OPS_OBJECT_RECEIVED,
    DTUN_SIG_OPS_OPEN,
    DTUN_SIG_OPS_ACCESS_REQUEST,
    DTUN_SIG_OPS_CLOSE,
    DTUN_SIG_OP_CREATE_VCARD,
    DTUN_SIG_OP_OWNER_VCARD_NOT_SET,
    DTUN_SIG_OP_STORE_VCARD,
    DTUN_SIG_END,

    /* Common */
    DTUN_DM_ERROR,

    DTUN_ID_MAX
} eDTUN_ID;

typedef struct
{
    tDTUN_ID           id;
    tDTUN_LEN          len;
} tDTUN_HDR;

/* needs to match server side */
typedef enum {
   SUBSYSTEM_RES1,
   SUBSYSTEM_RES2,
   SUBSYSTEM_DTUN
} tSUBSYSTEM;

typedef enum {
   DTUN_INTERFACE,
   DTUN_INTERFACE_MAX
} tDTUN_INTERFACE;

#define DTUN_MAX_DEV_NAME_LEN 248
#define DTUN_DEFAULT_DEV_NAME "My Android Phone"

#define DTUN_PROPERTY_SERVER_ACTIVE     DTUN_BRCM_PROP("srv_active")
#define DTUN_PROPERTY_HCID_ACTIVE       DTUN_BRCM_PROP("hcid_active")
#define DTUN_PROPERTY_OBEXD_ACTIVE      DTUN_BRCM_PROP("obexd_active")
#define DTUN_PROPERTY_BT_ACTIVE         DTUN_BRCM_PROP("activation")

#define DTUN_FD_NOT_CONNECTED (-1)

#define DTUN_LOCAL_SERVER_ADDR "brcm.bt.dtun" //comment out this line disable local socket implementation

#ifdef DTUN_LOCAL_SERVER_ADDR
#define DTUN_SERVER_ADDR DTUN_LOCAL_SERVER_ADDR
//local server name format: brcm.bt.dtun.port_num
#define DTUN_MAKE_LOCAL_SERVER_NAME(name_new, name_old, port_num) \
    do { \
        name_new[0] = 0; \
        if(name_old && strlen(name_old) < UNIX_PATH_MAX - 10) \
        { \
            sprintf(name_new, "%s.%d", name_old, port_num); \
            LOGD("DTUN_MAKE_LOCAL_SERVER_NAME return name: %s\n", name_new); \
        } \
        else LOGD("DTUN_MAKE_LOCAL_SERVER_NAME failed, the name is null or too long!\n"); \
    } while(0)
#else
#define DTUN_SERVER_ADDR "127.0.0.1"
#define DTUN_SERVER_ADDR_ALT "10.0.2.2"
#endif
#define DTUN_PORT 9000

#define REGISTER_SUBSYS_REQ 0x1001

/*******************************************************************************
**
**  SERVER
**
**
**
*******************************************************************************/

#pragma pack(1)

/* Method structs */

/* Data type for DTUN_METHOD_DM_START_DISCOVERY */
typedef struct
{
    tDTUN_HDR           hdr;
    /* params */
} tDTUN_START_DISCOVERY;

/* Data type for DTUN_METHOD_DM_CANCEL_DISCOVERY */
typedef struct
{
    tDTUN_HDR           hdr;
    /* params */
} tDTUN_CANCEL_DISCOVERY;

/* Data type for DTUN_METHOD_DM_GET_REMOTE_SERVICE_CHANNEL */
typedef struct {
        uint8_t data[16];
} tUINT128;

typedef struct {
        uint8_t type;
        union {
                uint16_t  uuid16;
                uint32_t  uuid32;
                tUINT128  uuid128;
        } value;
} tUUID;

typedef struct
{
    tDTUN_HDR           hdr;

    /* params */
    uint8_t            uuid_len;
    tUUID             uuid1;
    bdaddr_t           bdaddr;
    uint8_t            uuid[128];
} tDTUN_GET_SCN;

/* Data type for DTUN_METHOD_DM_CREATE_BONDING */
typedef struct
{
    tDTUN_HDR           hdr;
    bdaddr_t            bdaddr;
    /* params */
} tDTUN_BOND;

 /* Data type for    DTUN_METHOD_DM_PIN_REPLY */

typedef struct
{
    tDTUN_HDR           hdr;
    bdaddr_t            bdaddr;
    uint8_t             pin_len;
    uint8_t             pin_code[16];
    /* params */
} tDTUN_METHOD_DM_PIN_REPLY;

 /* Data type for    DTUN_METHOD_DM_PIN_NEG_REPLY */
 typedef struct
{
    tDTUN_HDR           hdr;
    bdaddr_t            bdaddr;
    /* params */
} tDTUN_METHOD_DM_PIN_NEG_REPLY;

 typedef struct
{
    tDTUN_HDR           hdr;
    bdaddr_t            bdaddr;
    /* params */
} tDTUN_METHOD_DM_DISC_RMT_DEV;


 typedef struct
{
    uint16_t           channel;
    uint32_t           exposed_handle;
    uint8_t             uuid[16];

    char                name[DTUN_MAX_DEV_NAME_LEN];
} tDTUN_METHOD_DM_ADD_SDP_REC_INFO;

 typedef struct
{
    tDTUN_HDR            hdr;
    tDTUN_METHOD_DM_ADD_SDP_REC_INFO            info;
} tDTUN_METHOD_DM_ADD_SDP_REC;

 typedef struct
{
    tDTUN_HDR            hdr;
    uint32_t                 handle;
    /* params */
} tDTUN_METHOD_DM_DEL_SDP_REC;

 typedef struct
{
    tDTUN_HDR           hdr;
    uint8_t             mode;
    /* params */
} tDTUN_METHOD_DM_SET_MODE;

 typedef struct
 {
    tDTUN_HDR           hdr;
    char                name[DTUN_MAX_DEV_NAME_LEN];
 } tDTUN_METHOD_DM_SET_NAME;

 typedef struct
 {
     bdaddr_t           bd_addr;
     uint8_t            service;
     uint8_t            response;
} tDTUN_METHOD_DM_AUTHORIZE_RSP_INFO;

 typedef struct
 {
    tDTUN_HDR            hdr;
    tDTUN_METHOD_DM_AUTHORIZE_RSP_INFO info;
 } tDTUN_METHOD_DM_AUTHORIZE_RSP;

 typedef struct
 {
     bdaddr_t           bd_addr;
     uint8_t            key[16];  /* Link key associated with peer device. */
     uint8_t            key_type;
 } tDTUN_METHOD_DM_ADD_DEV_INFO;

 typedef struct
 {
    tDTUN_HDR            hdr;
    tDTUN_METHOD_DM_ADD_DEV_INFO info;
 } tDTUN_METHOD_DM_ADD_DEV;

 typedef struct
{
    tDTUN_HDR           hdr;
    /* params */
    bdaddr_t            bd_addr;
    boolean             accepted;
}tDTUN_METHOD_SSP_CONFIRM;


typedef enum {
    DM_DISABLE_TESTMODE,
    DM_ENABLE_TESTMODE,
    DM_SET_TRACE_LEVEL = 0xffff0000,   /* MSB: layer_id, LSB: type/level (0-5, ff)  */
    DM_GET_TESTMODE_STATE = 0xffffffff     /* keep this last */
} tDTUN_DM_TESTMODE_STATE;

/* enable/disable test mode data type */
typedef struct {
    tDTUN_HDR           hdr;
    uint32_t            mode;
} tDTUN_METHOD_DM_SET_TESTMODE;

/* Set security mode */
typedef struct
{
    tDTUN_HDR           hdr;
    uint32_t             mode;
} tDTUN_METHOD_DM_SET_SECURITY;

/* Data type for DTUN_METHOD_AM_AV_OPEN */
typedef struct
{
    tDTUN_HDR           hdr;
    bdaddr_t            bdaddr;
    /* params */
} tDTUN_METHOD_AM_AV_OPEN;


typedef struct
{
    tDTUN_HDR           hdr;
    bdaddr_t            bdaddr;
}tDTUN_METHOD_AM_AV_DISC ;

 typedef struct
{
    tDTUN_HDR           hdr;
    uint32_t            op;
}tDTUN_METHOD_AM_AV_START_STOP;

#ifndef DTUN_PATH_LEN
#define DTUN_PATH_LEN 513 //BTUI_MAX_PATH_LENGTH+1
#endif
typedef char DTUN_PATH[DTUN_PATH_LEN];

/* Generic Data type for methods that only have bdaddr as a parameter */
typedef struct
{
    tDTUN_HDR           hdr;
    bdaddr_t            bdaddr;
    /* params */
} tDTUN_METHOD_RMT_DEV;

/* Method for SPP */

/* Serial Port Profile Section, Question: Should it be defined in another header file??? */
typedef enum
{
    SPP_CREATE,
    SPP_DESTORY,
    SPP_ENABLE,
    SPP_DISABLE,
    SPP_CONNECT,
    SPP_DISCONNECT,
}eDTUN_SPP_ACTION;

typedef enum

{

    SERIAL_PORT_AVAILABLE,
    SERIAL_PORT_CREATED,

    SERIAL_PORT_ENABLED,

    SERIAL_PORT_CONNECTING,

    SERIAL_PORT_CONNECTED,

    SERIAL_PORT_DISCONNECTED,
    SERIAL_PORT_DISABLED,
    SERIAL_PORT_NOT_AVAILABLE,   // add this if data gateway if the particular port pf data gateway server has been claimed by other profile

}ePORT_STATE;

typedef enum
{
     /* BLUETOOTH SPP Event */

     SPP_EVT_CONNECTED ,
     SPP_EVT_DISCONNECTED ,
     SPP_EVT_CONNECTING_TO_HOST,
     SPP_EVT_CONNECTED_TO_HOST,
     SPP_EVT_CONNECTING_TO_HOST_FAILED,
     SPP_EVT_DATA_ARRIVED,
     SPP_EVT_DATA_SENT,
     /* The following event Java does not know, used for dtun, btld & jni */
     SPP_EVT_PORT_CREATED,
     SPP_EVT_PORT_ENABLED,
     SPP_EVT_PORT_DISABLED,
     SPP_EVT_PORT_DESTROIED,
     SPP_EVT_CONNECT_RW_SOCKET,
} eSPP_EVT;

#define SPP_SERVER		1
#define SPP_CLIENT		2

/*******************************************************************************
**
**  CLIENT
**
**
**
*******************************************************************************/

/* Signals structs */

/* Data Type for DTUN_SIG_DM_LOCAL_INFO */
typedef struct
{
    tDTUN_HDR           hdr;
    bdaddr_t            bdaddr;
} tDTUN_SIG_DM_LOCAL_INFO;

/* Data type for DTUN_SIG_DISCOVERY_STARTED*/
typedef struct
{
    tDTUN_HDR           hdr;
} tDTUN_SIG_DISCOVERY_STARTED;

/* Data type for DTUN_SIG_DISCOVERY_COMPLETE */
typedef struct
{
    tDTUN_HDR           hdr;
} tDTUN_SIG_DISCOVERY_COMPLETE;

/* Data type for DTUN_SIG_DEVICE_FOUND */
typedef struct
{
    bdaddr_t            bd;
    uint16_t            rssi;
    uint32_t            cod;
} tDTUN_SIG_DEVICE_FOUND_INFO;

typedef struct
{
    tDTUN_HDR           hdr;
    tDTUN_SIG_DEVICE_FOUND_INFO info;
} tDTUN_SIG_DEVICE_FOUND;

/* Data Type for DTUN_SIG_DM_PIN_REQ */
typedef struct
{
    bdaddr_t           bdaddr;
    uint32_t            cod;
} tDTUN_SIG_DM_PIN_REQ_INFO;

typedef struct
{
    tDTUN_HDR           hdr;
    tDTUN_SIG_DM_PIN_REQ_INFO info;

} tDTUN_SIG_DM_PIN_REQ;


typedef struct
{
    bdaddr_t          bd_addr;            /* BD address peer device. */
    uint8_t             bd_name[DTUN_MAX_DEV_NAME_LEN];        /* Name of peer device. */
    uint8_t	             service;            /* Service ID to authorize. */
    uint32_t            cod;
} tDTUN_SIG_DM_AUTHORIZE_REQ_INFO;

typedef struct
{
    tDTUN_HDR           hdr;
    tDTUN_SIG_DM_AUTHORIZE_REQ_INFO info;            /* Service ID to authorize. */
} tDTUN_SIG_DM_AUTHORIZE_REQ;

typedef struct
{
    uint8_t             success;            /* TRUE of authentication succeeded, FALSE if failed. */
    uint8_t             key_present;        /* Valid link key value in key element */
    uint8_t             key_type;           /* The type of Link Key */
    bdaddr_t            bd_addr;            /* BD address peer device. */

    uint8_t   key[16];  /* Link key associated with peer device. */
} tDTUN_AUTH_COMP_INFO;


/* Data Type for DTUN_SIG_DM_AUTH_COMP */
typedef struct
{
    tDTUN_HDR           hdr;
    tDTUN_AUTH_COMP_INFO info;

} tDTUN_SIG_DM_AUTH_COMP;



typedef struct
{
    bdaddr_t            bd_addr;        /* BD address peer device. */
    uint8_t             bd_name[DTUN_MAX_DEV_NAME_LEN];        /* Name of peer device. */
} tDTUN_SIG_DM_RMT_NAME_INFO;

typedef struct
{
    tDTUN_HDR           hdr;
    tDTUN_SIG_DM_RMT_NAME_INFO info;
} tDTUN_SIG_DM_RMT_NAME;

typedef struct
{
    tDTUN_HDR           hdr;
    uint8_t             success;
    uint32_t            services;
} tDTUN_SIG_DM_RMT_SERVICE_CHANNEL;

/* Data type for DTUN_SIG_DM_RMT_SERVICES */
typedef struct
{
    tDTUN_HDR           hdr;
    uint8_t             success;
    uint8_t             ignore_err;
    uint32_t            services;
} tDTUN_SIG_DM_RMT_SERVICES;

typedef struct
{
    bdaddr_t             bd_addr;        /* BD address peer device. */
} tDTUN_SIG_DM_LINK_UP_INFO;

typedef struct
{
    tDTUN_HDR           hdr;
    tDTUN_SIG_DM_LINK_UP_INFO info;
}tDTUN_SIG_DM_LINK_UP;

typedef struct
{
    tDTUN_HDR           hdr;
    uint32_t  exposed_handle;
    uint32_t  handle;
}tDTUN_SIG_DM_SDP_REC_HANDLE;

typedef struct
{
    bdaddr_t            bd_addr;        /* BD address peer device. */
    uint8_t             reason;
} tDTUN_SIG_DM_LINK_DOWN_INFO;

typedef struct
{
    tDTUN_HDR           hdr;
    tDTUN_SIG_DM_LINK_DOWN_INFO info;
}tDTUN_SIG_DM_LINK_DOWN;

/* Data type for DTUN_SIG_SSP_CFM_REQ*/
typedef struct
{
    /* parameters */
    bdaddr_t            bd_addr;          /* peer device BT address */
    uint32_t                cod;   /* peer CoD  */
    uint32_t            num_value;   /* Numeric value for comparision. If just works, do not show this number to UI */
    boolean             just_work;   /* TRUE, if "Just Works association model */
    /* parameters ? */
} tDTUN_SIG_SSP_CFM_REQ_INFO;

typedef struct
{
    tDTUN_HDR           hdr;

    tDTUN_SIG_SSP_CFM_REQ_INFO info;
    /* parameters ? */
} tDTUN_SIG_SSP_CFM_REQ;

/* data type for DTUN_SIG_DM_TESTMODE_STATE */
typedef struct
{
    tDTUN_HDR           hdr;
    uint32_t            state;
} tDTUN_SIG_DM_TESTMODE_STATE;


typedef struct
{
    bdaddr_t            peer_addr;
    uint8_t             event;
    uint8_t             status;
    uint8_t             path[32];
} tDTUN_SIG_AM_AV_INFO;

typedef struct
{
    tDTUN_HDR           hdr;
    tDTUN_SIG_AM_AV_INFO info;
} tDTUN_SIG_AM_AV_EVENT;


typedef struct
{
    tDTUN_HDR              hdr;
    uint8_t		   port;
    uint8_t	           event;
}tDTUN_SIG_SPP_PORT_EVENT;

/* =============================================== */
/* OBEX - signals                                  */
/* =============================================== */

typedef struct
{
    tDTUN_HDR hdr;
    bdaddr_t  bdaddr;
    DTUN_PATH file_path_name;
} tDTUN_METHOD_OPC_PUSH_OBJECT;

typedef struct
{
    tDTUN_HDR hdr;
    bdaddr_t  bdaddr;
} tDTUN_METHOD_OPC_PULL_VCARD;

 typedef struct
{
    tDTUN_HDR hdr;
    bdaddr_t  bdaddr;
} tDTUN_METHOD_OPC_EXCH_VCARD;

typedef struct
{
    tDTUN_HDR hdr;
    uint8_t   operation;
    uint8_t   access;
    DTUN_PATH file_path_name;
} tDTUN_METHOD_OP_GRANT_ACCESS;

typedef struct
{
    tDTUN_HDR hdr;
    DTUN_PATH file_name;
} tDTUN_METHOD_OP_SET_OWNER_VCARD;

typedef struct
{
    tDTUN_HDR hdr;
    DTUN_PATH path_name;
} tDTUN_METHOD_OP_SET_EXCHANGE_FOLDER;

typedef struct
{
    tDTUN_HDR hdr;
    char vcard_uri[50];
    DTUN_PATH file_path_name;
} tDTUN_METHOD_OP_CREATE_VCARD;

typedef struct
{
    tDTUN_HDR hdr;
    DTUN_PATH file_path_name;
    uint16_t  dup_action;
} tDTUN_METHOD_OP_STORE_VCARD;

/* Data type for DTUN_SIG_OPC_PROGRESS */
typedef struct
{
    tDTUN_HDR hdr;
    uint32_t  obj_size;   /* Total size of object (BTA_FS_LEN_UNKNOWN if unknown) */
    uint32_t  bytes;      /* Number of bytes read or written since last progress event */
} tDTUN_SIG_OPC_PROGRESS;

typedef enum {
	SIG_OPC_OBJECT_OK,		/* Object push succeeded. */
	SIG_OPC_OBJECT_FAIL,           /* Object push failed. */
	SIG_OPC_OBJECT_NOT_FOUND,      /* Object not found. */
	SIG_OPC_OBJECT_NO_PERMISSION,  /* Operation not authorized. */
	SIG_OPC_OBJECT_SRV_UNAVAIL     /* Service unavaliable */
} eOPC_OBJECT_STATUS;

/* Data type for DTUN_SIG_OPC_OBJECT_RECEIVED */
typedef struct
{
    tDTUN_HDR hdr;
    uint8_t   status;
    DTUN_PATH name;
} tDTUN_SIG_OPC_OBJECT_RECEIVED;


/* Data type for DTUN_SIG_OPC_OBJECT_PUSHED */
typedef struct
{
    tDTUN_HDR hdr;
    uint8_t   status;
    DTUN_PATH name;
} tDTUN_SIG_OPC_OBJECT_PUSHED;


/* Data type for DTUN_SIG_OPC_CLOSE */
typedef struct
{
    tDTUN_HDR hdr;
    uint8_t   status;
} tDTUN_SIG_OPC_CLOSE;

/* Data type for DTUN_SIG_OPS_PROGRESS */
typedef struct
{
    tDTUN_HDR hdr;
    uint32_t  obj_size;
    uint32_t  bytes;
} tDTUN_SIG_OPS_PROGRESS;

typedef enum {
    SIG_OPS_FORMAT_OTHER,    /* Other */
    SIG_OPS_FORMAT_VCARD2_1, /* vCard 2.1 */
    SIG_OPS_FORMAT_VCARD3_0, /* vCard 3.0 */
    SIG_OPS_FORMAT_VCAL1_0,  /* vCal 1.0 */
    SIG_OPS_FORMAT_VCAL2_0,  /* vCal 2.0 */
    SIG_OPS_FORMAT_VNOTE,    /* vNote */
    SIG_OPS_FORMAT_VMESSAGE  /* vMessage */
} eOPS_OBJECT_FORMAT;

/* Data type for DTUN_SIG_OPS_OBJECT_RECEIVED */
typedef struct
{
    tDTUN_HDR hdr;
    uint8_t   format;
    DTUN_PATH name;
} tDTUN_SIG_OPS_OBJECT_RECEIVED;

/* Data type for DTUN_SIG_OPS_ACCESS_REQUEST */
 typedef struct
{
    tDTUN_HDR hdr;
    char      bdname[DTUN_MAX_DEV_NAME_LEN];
    char      bdaddr[18];
    uint8_t   oper;
    uint8_t   format;
    DTUN_PATH name;
    uint32_t  obj_size;
} tDTUN_SIG_OPS_ACCESS_REQUEST;

/* Data type for DTUN_SIG_OP_CREATE_VCARD */
typedef struct
{
    tDTUN_HDR hdr;
    uint8_t   status;
    DTUN_PATH name;
} tDTUN_SIG_OP_CREATE_VCARD;

/* Data type for DTUN_SIG_OP_OWNER_VCARD_NOT_SET */
typedef struct
{
    tDTUN_HDR hdr;
    DTUN_PATH name;
} tDTUN_SIG_OP_OWNER_VCARD_NOT_SET;

/* Data type for DTUN_SIG_OP_STORE_VCARD */
typedef struct
{
    tDTUN_HDR hdr;
    uint16_t  status;
    DTUN_PATH name;
    char      contact_name[30];
    uint16_t  store_id;
} tDTUN_SIG_OP_STORE_VCARD;

/*******************************************************************************
**
**  COMMON
**
**
**
*******************************************************************************/

/* Data type for DTUN_DM_ERROR */
typedef struct
{
    tDTUN_HDR           hdr;
    tDTUN_ERR           err;
} tDTUN_ERROR;

/**************************************************************/
/* Unions */

typedef union
{
    tDTUN_HDR                           hdr; /* wo/ body */
    tDTUN_START_DISCOVERY               start_discovery;
    tDTUN_CANCEL_DISCOVERY              cancel_discovery;
    tDTUN_GET_SCN                       get_scn;
    tDTUN_BOND                          bond;
    tDTUN_METHOD_RMT_DEV                rmt_dev;
    tDTUN_METHOD_DM_PIN_REPLY           pin_reply;
    tDTUN_METHOD_DM_PIN_NEG_REPLY       pin_neg_reply;
    tDTUN_METHOD_DM_AUTHORIZE_RSP       authorize_rsp;
    tDTUN_METHOD_DM_SET_MODE            set_mode;
    tDTUN_METHOD_DM_SET_NAME            set_name;
    tDTUN_METHOD_DM_ADD_DEV             add_dev;
    tDTUN_METHOD_SSP_CONFIRM            ssp_confirm;
    tDTUN_METHOD_DM_DISC_RMT_DEV        disc_rmt_dev;
    tDTUN_METHOD_DM_ADD_SDP_REC         add_sdp_rec;
    tDTUN_METHOD_DM_DEL_SDP_REC         del_sdp_rec;
    tDTUN_METHOD_DM_SET_TESTMODE        set_testmode;
    tDTUN_METHOD_DM_SET_SECURITY        set_security;
    tDTUN_METHOD_AM_AV_OPEN             av_open;
    tDTUN_METHOD_AM_AV_DISC             av_disc;
    tDTUN_METHOD_AM_AV_START_STOP       av_startstop;

    tDTUN_METHOD_OPC_PUSH_OBJECT        opc_push_object;
    tDTUN_METHOD_OPC_PULL_VCARD         opc_pull_vcard;
    tDTUN_METHOD_OPC_EXCH_VCARD         opc_exch_vcard;
    tDTUN_METHOD_OP_GRANT_ACCESS        op_grant_access;
    tDTUN_METHOD_OP_SET_OWNER_VCARD     op_set_owner_vcard;
    tDTUN_METHOD_OP_SET_EXCHANGE_FOLDER op_set_exchange_folder;
    tDTUN_METHOD_OP_CREATE_VCARD        op_create_vcard;
    tDTUN_METHOD_OP_STORE_VCARD         op_store_vcard;
} tDTUN_DEVICE_METHOD;

/* union of all signals */
typedef union
{
    tDTUN_HDR                       hdr; /* wo/ body */
    tDTUN_SIG_DM_LOCAL_INFO         local_info;
    tDTUN_SIG_DISCOVERY_STARTED     discv_started;
    tDTUN_SIG_DISCOVERY_COMPLETE    discovery_complete;
    tDTUN_SIG_DEVICE_FOUND          device_found;
    tDTUN_SIG_DM_RMT_NAME           rmt_name;
    tDTUN_SIG_DM_RMT_SERVICE_CHANNEL rmt_scn;
    tDTUN_SIG_DM_RMT_SERVICES       rmt_services;
    tDTUN_SIG_DM_PIN_REQ            pin_req;
    tDTUN_SIG_DM_AUTHORIZE_REQ authorize_req;
    tDTUN_SIG_DM_AUTH_COMP          auth_comp;
    tDTUN_SIG_DM_LINK_DOWN          link_down;
    tDTUN_SIG_SSP_CFM_REQ           ssp_cfm_req;
    tDTUN_SIG_DM_TESTMODE_STATE     testmode_state;
    tDTUN_SIG_AM_AV_EVENT           av_event;
    tDTUN_SIG_DM_LINK_UP            link_up;
    tDTUN_SIG_DM_SDP_REC_HANDLE            sdp_handle;

    tDTUN_SIG_OPC_PROGRESS           opc_progress;
    tDTUN_SIG_OPC_OBJECT_RECEIVED    opc_object_received;
    tDTUN_SIG_OPC_OBJECT_PUSHED      opc_object_pushed;
    tDTUN_SIG_OPC_CLOSE              opc_close;

    tDTUN_SIG_OPS_PROGRESS           ops_progress;
    tDTUN_SIG_OPS_OBJECT_RECEIVED    ops_object_received;
    tDTUN_SIG_OPS_ACCESS_REQUEST     ops_access_request;
    tDTUN_SIG_OP_CREATE_VCARD        op_create_vcard;
    tDTUN_SIG_OP_OWNER_VCARD_NOT_SET op_owner_vcard_not_set;
    tDTUN_SIG_OP_STORE_VCARD         op_store_vcard;
} tDTUN_DEVICE_SIGNAL;

/* type for tunnel method functions */
typedef void (*tDTUN_METHOD)(tDTUN_DEVICE_METHOD *p_data);
typedef void (*tDTUN_SIGNAL)(tDTUN_DEVICE_SIGNAL *p_data);
typedef void (*tDTUN_PROCESS_HANDLER)(void *);
typedef void (*tDTUN_PROCESS_INIT)(void);


#pragma pack(0)

#endif
