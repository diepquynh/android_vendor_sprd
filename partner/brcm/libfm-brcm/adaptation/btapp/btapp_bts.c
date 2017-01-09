/****************************************************************************
**
**  Name:          btapp_bts.c
**
**  Description:   Contains application functions for android BlueTooth Socket layer
**
**
**  Copyright (c) 2002-2010, Broadcom Corp., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include "bt_target.h"
#include "gki.h"
#include "gki_common.h"
#include "bd.h"
#include "utl.h"
#include "bta_api.h"
#include "bta_jv_api.h"
#include "btm_api.h"
#include "l2c_api.h"
#include "gap_api.h"
#include "spp_api.h"
#include "btui.h"
#include "btui_int.h"
#include "btm_api.h"
#include "bte_appl.h"
#include "sdpdefs.h"
#include "spp_int.h"
#include "btapp_dm.h"
#include "btl_ifs.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include "bta_fts_int.h"


typedef struct
{
    tBTL_ListNode bts_chan_list; /* main list holding all allocated channels */
    int bts_chan_list_count; /* tracks nbr of allocated channels */
} tBTS_MAIN;

typedef enum {
    BTS_TYPE_RFCOMM,
    BTS_TYPE_SCO,
    BTS_TYPE_L2CAP,
    BTS_TYPE_NONE
} tBTLIF_BTS_TYPE;

typedef struct
{
    tBTL_ListNode       node;
    tCTRL_HANDLE        btlif_ctrl;     /* btl if subsystem ctrl handle */
    UINT16              btlif_port;     /* btlif socket port */
    tBTLIF_BTS_TYPE     type;
    tDATA_HANDLE        dhdl;
    UINT32              bta_hdl;
    UINT16              port_handle;    /* port api handle */
    UINT16              peer_mtu;
    UINT8               sec_id;
    BOOLEAN             use_co;
    int                 scn;            /* unique btl-if id */
    int                 rc_chan;        /* blz socket server channel */
    int                 blz_listen_fd;  /* unique reference for bts client mapping to client listener fd */
    int                 blz_sock;       /* unique reference for bts client mapping to client datapath socket */
    int                 listen_cancel_pnd;

    /* buffer stats */
    int tx_q;           /* rfcomm <-- btlif client */
    int tx_q_peak;
    int rx_q;           /* rfcomm --> btlif client */
    int rx_q_peak;

} tBTS_CHAN;

#define ENABLE_TX_PATH_NO_COPY

static tBTS_MAIN bts_main;

static void jw_if_rfcomm_sr_cback(tBTA_JV_EVT event, tBTA_JV *p_data);
static void jw_if_rfcomm_cl_cback(tBTA_JV_EVT event, tBTA_JV *p_data);

#define BTA_HANDLE_PENDING 0x1234
#define INVALID_VAL (0xffffffff)
#define PORT_HDL_INVALID (0xffff)

typedef enum {
    KEY_RC_CHAN_SRV,
    KEY_RC_CHAN_CLNT,
    KEY_DATA_HDL,
    KEY_BTA_HDL,
    KEY_ANY_SOCK,
    KEY_SCN,
    KEY_BTLIF_PORT
} t_search_key;

#define DUMP_CHAN(msg, ch) if (ch) debug("%s type %d, dhdl %d, bta_hdl %d, scn %d, phdl %d, lstpnd %d, %d:%d (L:D)", \
            msg, ch->type, ch->dhdl, ch->bta_hdl, ch->scn, ch->port_handle, ch->listen_cancel_pnd, ch->blz_listen_fd, ch->blz_sock);

void bts_chan_dump_all(void)
{
    struct tBTL_ListNode_tag *pos;
    tBTS_CHAN *p;

    debug("### channel list ###");

    LIST_FOREACH(pos, &bts_main.bts_chan_list)
    {
        p = LIST_GET_NODE_OBJ(pos, tBTS_CHAN, node);
        DUMP_CHAN("", p);
    }
}

tBTS_CHAN* find_duplicates(t_search_key key, int val)
{
    struct tBTL_ListNode_tag *pos;
    tBTS_CHAN *p;
    int counter = 0;

    LIST_FOREACH(pos, &bts_main.bts_chan_list)
    {
        p = LIST_GET_NODE_OBJ(pos, tBTS_CHAN, node);

        switch(key)
        {
            case KEY_RC_CHAN_CLNT:
                if ((p->rc_chan== val) && (p->blz_listen_fd == DATA_SOCKET_INVALID)) counter++;
                break;

            case KEY_RC_CHAN_SRV:
                if ((p->rc_chan== val) && (p->blz_listen_fd != DATA_SOCKET_INVALID)) counter++;
                break;

            case KEY_DATA_HDL:
                if (p->dhdl == val) counter++;
                break;

            case KEY_BTA_HDL:
                if ((int)p->bta_hdl == val) counter++;
                break;

            case KEY_SCN:
                if ((int)p->scn == val) counter++;
                break;

            case KEY_ANY_SOCK:
                if (((int)p->blz_listen_fd == val) || ((int)p->blz_sock== val)) counter++;
                break;

            case KEY_BTLIF_PORT:
                if ((int)p->btlif_port == val) counter++;
                break;

            default:
                ASSERTC(0, "invalid key", key);
                break;

        }
    }

    if (counter>1)
    {
        debug("### WARNING : Found duplicates for key %d, val %d ! ###", key, val);
        bts_chan_dump_all();
    }

    return NULL;
}

tBTS_CHAN* bts_chan_find_by_key(t_search_key key, int val)
{
    struct tBTL_ListNode_tag *pos;
    tBTS_CHAN *p;

    //find_duplicates(key, val);

    LIST_FOREACH(pos, &bts_main.bts_chan_list)
    {
        p = LIST_GET_NODE_OBJ(pos, tBTS_CHAN, node);

        switch(key)
        {
            case KEY_RC_CHAN_CLNT:
                if ((p->rc_chan== val) && (p->blz_listen_fd == DATA_SOCKET_INVALID)) return p;
                break;

            case KEY_RC_CHAN_SRV:
                if ((p->rc_chan== val) && (p->blz_listen_fd != DATA_SOCKET_INVALID)) return p;
                break;

            case KEY_DATA_HDL:
                if (p->dhdl == val) return p;
                break;

            case KEY_BTA_HDL:
                if ((int)p->bta_hdl == val) return p;
                break;

            case KEY_SCN:
                if ((int)p->scn == val) return p;
                break;

            case KEY_ANY_SOCK:
                if (((int)p->blz_listen_fd == val) || ((int)p->blz_sock== val)) return p;
                break;

            case KEY_BTLIF_PORT:
                if ((int)p->btlif_port == val) return p;
                break;

            default:
                ASSERTC(0, "invalid key", key);
                break;

        }
    }
    return NULL;
}

static tBTS_CHAN* bts_chan_alloc_add(tCTRL_HANDLE btlif_hdl)
{
    tBTS_CHAN *p;

    p = malloc(sizeof(tBTS_CHAN));

    if (!p)
    {
        error("no mem");
        return NULL;
    }

    /* initialize members */
    INIT_LIST_NODE(&p->node);

    p->btlif_ctrl = btlif_hdl;
    p->btlif_port = 0;
    p->type = BTS_TYPE_NONE;
    p->dhdl = DATA_SOCKET_INVALID;
    p->bta_hdl = INVALID_VAL;
    p->port_handle = PORT_HDL_INVALID;
    p->peer_mtu = 0;
    p->sec_id = INVALID_VAL;
    p->use_co = INVALID_VAL;
    p->scn = INVALID_VAL;
    p->rc_chan = INVALID_VAL;
    p->blz_listen_fd = DATA_SOCKET_INVALID;
    p->blz_sock = DATA_SOCKET_INVALID;
    p->listen_cancel_pnd = 0;
    p->tx_q = 0;
    p->tx_q_peak = 0;
    p->rx_q = 0;
    p->rx_q_peak = 0;

    /* add to connection list */
    list_add_node(&p->node, &bts_main.bts_chan_list);

    bts_main.bts_chan_list_count++;

    debug("%s : success (tot %d)", __FUNCTION__, bts_main.bts_chan_list_count);

    //bts_chan_dump_all();

    return p;
}

static int bts_chan_free(tBTS_CHAN *p)
{
    UINT32 bta_hdl;

    debug("p = 0x%x", p);

    if (!p)
    {
        error("bts_free NULL");
        return -1;
    }

    bta_hdl = p->bta_hdl;

    /* remove from list */
    list_del_node(&p->node);

    wrp_free_dynamicport(SUB_BTS, p->btlif_port);

    /* free memory */
    free(p);

    p = NULL;

    bts_main.bts_chan_list_count--;

    debug("%s success, bta hdl %d (tot %d)", __FUNCTION__, bta_hdl, bts_main.bts_chan_list_count);

    bts_chan_dump_all();

    return 0;
}

static void bts_chan_free_all(tCTRL_HANDLE hdl)
{
    tBTL_ListNode *pos;
    tBTS_CHAN *p;

    LIST_FOREACH(pos, &bts_main.bts_chan_list)
    {
        p = LIST_GET_NODE_OBJ(pos, tBTS_CHAN, node);

        /* only process channels attached to this btlif ctrl handle */
        if (p->btlif_ctrl != hdl)
            continue;

        info("free bts chan resources (bta hdl %d, dhdl %d)", p->bta_hdl, p->dhdl);

        /* connect any active links */
        if (p->bta_hdl != INVALID_VAL)
            BTA_JvRfcommClose(p->bta_hdl);

        /* stop any active servers */
        BTA_JvRfcommStopServer(p->scn);

        /* free SCN */
        BTM_FreeSCN(p->scn);

        /* make sure all datapaths are disconnected */
        BTL_IF_DisconnectDatapath(p->dhdl);
    }

    /* free matching list elements */
    while (!list_isempty(&bts_main.bts_chan_list))
    {
        pos = bts_main.bts_chan_list.Next;

        p = LIST_GET_NODE_OBJ(pos, tBTS_CHAN, node);

        /* only process channels attached to this btlif ctrl handle */
        if (p->btlif_ctrl != hdl)
            continue;

        /* now free element */
        bts_chan_free(p);
    }

    return NULL;
}

void bts_log_tstamps_us(char *comment, tBTS_CHAN *p, BOOLEAN dumponly)
{
#define USEC_PER_SEC 1000000L

    static struct timespec prev = {0, 0};
    struct timespec now, diff;
    unsigned int diff_us, now_us;

    if (!dumponly)
    {
        clock_gettime(CLOCK_MONOTONIC, &now);
        now_us = now.tv_sec*USEC_PER_SEC + now.tv_nsec/1000;
        diff_us = (now.tv_sec - prev.tv_sec) * USEC_PER_SEC + (now.tv_nsec - prev.tv_nsec)/1000;
    }

        debug("[%s] ts %10d, bta hdl %d, diff %08d, tx_q %d (%d), rx_q %d (%d)",
                    comment, now_us, p->bta_hdl, diff_us, p->tx_q, p->tx_q_peak, p->rx_q, p->rx_q_peak);
    if (!dumponly)
        prev = now;
    }


void bts_retrieve_port_handle(tBTS_CHAN *p_chan)
{
    p_chan->port_handle = BTA_JvRfcommGetPortHdl(p_chan->bta_hdl);

    if (p_chan->port_handle == PORT_HDL_INVALID)
    {
        error("failed to get port handle, bta hdl %d", p_chan->bta_hdl);
        return;
    }

    info("port handle %d", p_chan->port_handle);
}

/* this function assumes port handle has been set */
void bts_retrieve_peer_mtu(tBTS_CHAN *p_chan)
{
    tPORT_STATUS port_stat;

    ASSERTC(p_chan->port_handle != PORT_HDL_INVALID, "port handle not set", p_chan->port_handle);

    PORT_GetQueueStatus(p_chan->port_handle, &port_stat);

    p_chan->peer_mtu = port_stat.mtu_size;

    info("retrieved peer mtu = %d", p_chan->peer_mtu);
}


void bts_update_stats(tBTS_CHAN *p)
{
    tPORT_STATUS stat;

    PORT_GetQueueStatus(p->port_handle, &stat);

    /* rx path */
    p->rx_q = stat.in_queue_size;
    p->rx_q_peak = MAX(p->rx_q, p->rx_q_peak);

    /* tx path */
    p->tx_q_peak = MAX(p->tx_q, p->tx_q_peak);

    bts_log_tstamps_us("update stats", p, TRUE);
}

static UINT32 sdp_register_ftp_server (int scn, char *p_service_name)
{
    tSDP_PROTOCOL_ELEM  protoList [3];
    UINT16              ftp_service = UUID_SERVCLASS_OBEX_FILE_TRANSFER;
    UINT16              browse = UUID_SERVCLASS_PUBLIC_BROWSE_GROUP;
    BOOLEAN             status = FALSE;
    UINT32 sdp_handle;

    info("scn %d, service name %s", scn, p_service_name);

    if ((sdp_handle = SDP_CreateRecord()) == 0)
    {
        error("FTS SDP: Unable to register FTP Service");
        return sdp_handle;
    }

    /* add service class */
    if (SDP_AddServiceClassIdList(sdp_handle, 1, &ftp_service))
    {
        /* add protocol list, including RFCOMM scn */
        protoList[0].protocol_uuid = UUID_PROTOCOL_L2CAP;
        protoList[0].num_params = 0;
        protoList[1].protocol_uuid = UUID_PROTOCOL_RFCOMM;
        protoList[1].num_params = 1;
        protoList[1].params[0] = scn;
        protoList[2].protocol_uuid = UUID_PROTOCOL_OBEX;
        protoList[2].num_params = 0;

        if (SDP_AddProtocolList(sdp_handle, 3, protoList))
        {
            status = TRUE;  /* All mandatory fields were successful */

            /* optional:  if name is not "", add a name entry */
            if (*p_service_name != '\0')
                SDP_AddAttribute(sdp_handle,
                                 (UINT16)ATTR_ID_SERVICE_NAME,
                                 (UINT8)TEXT_STR_DESC_TYPE,
                                 (UINT32)(strlen(p_service_name) + 1),
                                 (UINT8 *)p_service_name);

            /* Add in the Bluetooth Profile Descriptor List */
            SDP_AddProfileDescriptorList(sdp_handle,
                                             UUID_SERVCLASS_OBEX_FILE_TRANSFER,
                                             BTA_FTS_DEFAULT_VERSION);

        } /* end of setting mandatory protocol list */
    } /* end of setting mandatory service class */

    /* Make the service browseable */
    SDP_AddUuidSequence (sdp_handle, ATTR_ID_BROWSE_GROUP_LIST, 1, &browse);

    if (!status)
    {
        SDP_DeleteRecord(sdp_handle);
        error("bta_fts_sdp_register FAILED");
    }
    else
    {
        bta_sys_add_uuid(ftp_service); /* UUID_SERVCLASS_OBEX_FILE_TRANSFER */
        error("FTS:  SDP Registered (handle 0x%08x)", sdp_handle);
    }

    return sdp_handle;
}

#include "bta_op_api.h"


/* object format lookup table */
static const tBTA_OP_FMT bta_ops_obj_fmt[] =
{
    BTA_OP_VCARD21_FMT,
    BTA_OP_VCARD30_FMT,
    BTA_OP_VCAL_FMT,
    BTA_OP_ICAL_FMT,
    BTA_OP_VNOTE_FMT,
    BTA_OP_VMSG_FMT,
    BTA_OP_OTHER_FMT
};

#define BTA_OPS_NUM_FMTS        7
#define BTA_OPS_PROTOCOL_COUNT  3

#define BTUI_OPS_FORMATS            (BTA_OP_VCARD21_MASK | BTA_OP_VCARD30_MASK | \
                                         BTA_OP_VCAL_MASK | BTA_OP_ICAL_MASK | \
                                         BTA_OP_VNOTE_MASK | BTA_OP_VMSG_MASK | \
                                         BTA_OP_ANY_MASK )


UINT32 bts_register_sdp_ops(int scn, char *p_service_name)
{


    tSDP_PROTOCOL_ELEM  protoList [BTA_OPS_PROTOCOL_COUNT];
    tOBX_StartParams    start_params;
    UINT16      servclass = UUID_SERVCLASS_OBEX_OBJECT_PUSH;
    int         i, j;
    tBTA_UTL_COD cod;
    tOBX_STATUS status;
    UINT8       desc_type[BTA_OPS_NUM_FMTS];
    UINT8       type_len[BTA_OPS_NUM_FMTS];
    UINT8       *type_value[BTA_OPS_NUM_FMTS];
    UINT16      browse;
    UINT32 sdp_handle;
    tBTA_OP_FMT_MASK    formats = BTUI_OPS_FORMATS;

    info("scn %d, service name %s", scn, p_service_name);

    sdp_handle = SDP_CreateRecord();

    /* add service class */
    if (SDP_AddServiceClassIdList(sdp_handle, 1, &servclass))
    {
        /* add protocol list, including RFCOMM scn */
        protoList[0].protocol_uuid = UUID_PROTOCOL_L2CAP;
        protoList[0].num_params = 0;
        protoList[1].protocol_uuid = UUID_PROTOCOL_RFCOMM;
        protoList[1].num_params = 1;
        protoList[1].params[0] = scn;
        protoList[2].protocol_uuid = UUID_PROTOCOL_OBEX;
        protoList[2].num_params = 0;

        if (SDP_AddProtocolList(sdp_handle, BTA_OPS_PROTOCOL_COUNT, protoList))
        {
            SDP_AddAttribute(sdp_handle,
                (UINT16)ATTR_ID_SERVICE_NAME,
                (UINT8)TEXT_STR_DESC_TYPE,
                (UINT32)(strlen(p_service_name) + 1),
                (UINT8 *)p_service_name);

            SDP_AddProfileDescriptorList(sdp_handle,
                UUID_SERVCLASS_OBEX_OBJECT_PUSH,
                0x0100);
        }
    }

    /* Make the service browseable */
    browse = UUID_SERVCLASS_PUBLIC_BROWSE_GROUP;
    SDP_AddUuidSequence (sdp_handle, ATTR_ID_BROWSE_GROUP_LIST, 1, &browse);

    /* add sequence for supported types */
    for (i = 0, j = 0; i < BTA_OPS_NUM_FMTS; i++)
    {
        if ((formats >> i) & 1)
        {
            type_value[j] = (UINT8 *) &bta_ops_obj_fmt[i];
            desc_type[j] = UINT_DESC_TYPE;
            type_len[j++] = 1;
        }
    }

    SDP_AddSequence(sdp_handle, (UINT16) ATTR_ID_SUPPORTED_FORMATS_LIST,
        (UINT8) j, desc_type, type_len, type_value);

    /* set class of device */
    cod.service = COD_SERVICE_OBJ_TRANSFER;
    utl_set_device_class(&cod, BTA_UTL_SET_COD_SERVICE_CLASS);

    bta_sys_add_uuid(servclass); /* UUID_SERVCLASS_OBEX_OBJECT_PUSH */

    return sdp_handle;
}





UINT32 bts_register_sdp_ftp(int scn, char *service_name)
{
    tBTA_UTL_COD         cod;

    info("scn %d, service name %s", scn, service_name);

    /* also set cod service bit for now */
    cod.service = COD_SERVICE_OBJ_TRANSFER;
    utl_set_device_class(&cod, BTA_UTL_SET_COD_SERVICE_CLASS);
    return sdp_register_ftp_server(scn, service_name);
}



tSPP_STATUS bts_register_sdp_spp(UINT8 scn, char *service_name)
{
    tSPP_STATUS         status = SPP_SUCCESS;
    UINT16              serviceclassid = UUID_SERVCLASS_SERIAL_PORT;
    tSDP_PROTOCOL_ELEM  proto_elem_list[SPP_NUM_PROTO_ELEMS];
    UINT32              sdp_handle;

    info("scn %d, service name %s", scn, service_name);

    /* register the service */
    if ((sdp_handle = SDP_CreateRecord()) != FALSE)
    {
        /*** Fill out the protocol element sequence for SDP ***/
        proto_elem_list[0].protocol_uuid = UUID_PROTOCOL_L2CAP;
        proto_elem_list[0].num_params = 0;
        proto_elem_list[1].protocol_uuid = UUID_PROTOCOL_RFCOMM;
        proto_elem_list[1].num_params = 1;

        proto_elem_list[1].params[0] = scn;

        if (SDP_AddProtocolList(sdp_handle, SPP_NUM_PROTO_ELEMS,
            proto_elem_list))
        {
            if (SDP_AddServiceClassIdList(sdp_handle, 1, &serviceclassid))
            {
                if ((SDP_AddAttribute(sdp_handle, ATTR_ID_SERVICE_NAME,
                    TEXT_STR_DESC_TYPE, (UINT32)(strlen(service_name)+1),
                    (UINT8 *)service_name)) == FALSE)

                    status = SPP_ERR_SDP_ATTR;
                else
                {
                    UINT16  list[1];

                    /* Make the service browseable */
                    list[0] = UUID_SERVCLASS_PUBLIC_BROWSE_GROUP;
                    if ((SDP_AddUuidSequence (sdp_handle,  ATTR_ID_BROWSE_GROUP_LIST,
                        1, list)) == FALSE)

                        status = SPP_ERR_SDP_CLASSID;
                }
            }
            else
                status = SPP_ERR_SDP_CLASSID;
        }
        else
            status = SPP_ERR_SDP_PROTO;
    }
    else
        status = SPP_ERR_SDP_REG;

    return (status);
}



const char * jv_evt[] = {
    "BTA_JV_ENABLE_EVT",
    "BTA_JV_SET_DISCOVER_EVT",
    "BTA_JV_LOCAL_ADDR_EVT",
    "BTA_JV_LOCAL_NAME_EVT",
    "BTA_JV_REMOTE_NAME_EVT",
    "BTA_JV_SET_ENCRYPTION_EVT",
    "BTA_JV_GET_SCN_EVT",
    "BTA_JV_GET_PSM_EVT",
    "BTA_JV_DISCOVERY_COMP_EVT",
    "BTA_JV_SERVICES_LEN_EVT",
    "BTA_JV_SERVICE_SEL_EVT",
    "BTA_JV_CREATE_RECORD_EVT",
    "BTA_JV_UPDATE_RECORD_EVT",
    "BTA_JV_ADD_ATTR_EVT",
    "BTA_JV_DELETE_ATTR_EVT",
    "BTA_JV_L2CAP_OPEN_EVT",
    "BTA_JV_L2CAP_CLOSE_EVT",
    "BTA_JV_L2CAP_START_EVT",
    "BTA_JV_L2CAP_CL_INIT_EVT",
    "BTA_JV_L2CAP_DATA_IND_EVT",
    "BTA_JV_L2CAP_CONG_EVT",
    "BTA_JV_L2CAP_READ_EVT",
    "BTA_JV_L2CAP_WRITE_EVT",
    "BTA_JV_RFCOMM_OPEN_EVT",
    "BTA_JV_RFCOMM_CLOSE_EVT",
    "BTA_JV_RFCOMM_START_EVT",
    "BTA_JV_RFCOMM_CL_INIT_EVT",
    "BTA_JV_RFCOMM_DATA_IND_EVT",
    "BTA_JV_RFCOMM_CONG_EVT",
    "BTA_JV_RFCOMM_READ_EVT",
    "BTA_JV_RFCOMM_WRITE_EVT",
    "BTA_JV_MAX_EVT"
};

static BOOLEAN bts_alloc_buf(tBTS_CHAN *p_chan, BT_HDR **pp_buf)
{
    BT_HDR          *p_buf;
    int buf_sz;

    *pp_buf = NULL;

    if ((p_buf = (BT_HDR *) GKI_getpoolbuf(RFCOMM_DATA_POOL_ID)) != NULL)
    {
        buf_sz = RFCOMM_DATA_POOL_BUF_SIZE; /* the result it the same but more efficent then GKI_get_buf_size(p_buf); */
        p_buf->len = 0;
        p_buf->offset = L2CAP_MIN_OFFSET + RFCOMM_MIN_OFFSET;

        debug("setup_rx_buf : max %d, offset %d, len %d ", buf_sz, p_buf->offset, p_buf->len);

        debug("peer mtu %d", p_chan->peer_mtu);

        buf_sz -= (BT_HDR_SIZE + p_buf->offset);

        if (buf_sz < p_chan->peer_mtu)
        {
            /* if we end up here, the RFCOMM buffer pool is not tuned properly */
            ASSERTC(0, "rfcomm pool not tuned", 0);
            GKI_freebuf(p_buf);
            return FALSE;
        }
        *pp_buf = p_buf;
        return TRUE;
    }

    return FALSE;
}


static BOOLEAN rfc_setup_rx_buf(tBTS_CHAN *p_chan)
{
    BT_HDR          *p_buf;
    char *p_tmp;

    APPL_TRACE_DEBUG2("%s on btahdl %d", __FUNCTION__, p_chan->bta_hdl);

    /* set up new receive buffer */
    if (bts_alloc_buf(p_chan, &p_buf) == TRUE)
    {
        /* buf len is setup in btl if datapath callback */

        /* save stats */
        p_chan->tx_q++;
        bts_update_stats(p_chan);

        p_tmp = (UINT8 *)(p_buf + 1) + p_buf->offset;

        /* setup receive buffer to match the rfcomm links mtu size */
        if (BTL_IF_SetupRxBuf(p_chan->dhdl, p_tmp, p_chan->peer_mtu) != BTL_IF_SUCCESS)
        {
            error("%s: setup rx buffer failed", __FUNCTION__);
            GKI_freebuf(p_buf);
            return FALSE;
        }
        return TRUE;
    }
    else
    {
        error("%s: Out of buffers", __FUNCTION__);
        return FALSE;
    }

    return TRUE;
}

static BOOLEAN rfc_tx_data_copy(tBTS_CHAN *p_chan, char *p, int len)
{
    BT_HDR          *p_buf;
    int buf_sz;
    char *p_tmp;

    debug("<-- bts-tx %d bytes on bta hdl %d [copy]", len, p_chan->bta_hdl);

    /* set up new receive buffer */
    if (bts_alloc_buf(p_chan, &p_buf) == TRUE)
    {
        p_tmp = (UINT8 *)(p_buf + 1) + p_buf->offset;

        memcpy(p_tmp, p, len);
        p_buf->len = len;

        /* save stats */
        p_chan->tx_q++;
        bts_update_stats(p_chan);

        //bts_log_tstamps_us("rfc_write", p_chan, FALSE);

        /* forward to rfcomm */
        BTA_JvRfcommWrite(p_chan->bta_hdl, p_buf, p, len);
        return TRUE;
    }
    else
    {
        error("%s: Out of buffers", __FUNCTION__);
        return FALSE;
    }
}


void jv_rfc_write(tBTA_JV_RFCOMM_WRITE *p)
{
    tBTS_CHAN *p_chan;

    APPL_TRACE_DEBUG1("buffer transmitted (%x)", p);

    p_chan = bts_chan_find_by_key(KEY_BTA_HDL, p->handle);

    ASSERTC(p_chan, "channel not found", 0);

    /* buffer was transmitted */
    GKI_freebuf(p->req_id);

    p_chan->tx_q--;
    bts_update_stats(p_chan);
}

void btlif_bts_api_data_cb(tDATA_HANDLE dhdl, char *p, int len)
{
    tBTS_CHAN *p_chan;
    BT_HDR *p_buf;

    debug("btlif_bts_api_data_cb : hdl %d, len %d", dhdl, len);

    p_chan = bts_chan_find_by_key(KEY_DATA_HDL, dhdl);

    ASSERTC(p_chan, "channel not found", 0);

#ifdef ENABLE_TX_PATH_NO_COPY
    p_buf = (BT_HDR *)((UINT8 *) p - BT_HDR_SIZE - L2CAP_MIN_OFFSET - RFCOMM_MIN_OFFSET);
    p_buf->len = len;
    p_buf->event = 0;
    p_buf->layer_specific = 0;

    /* notify new data */
    if (p_buf->len > 0)
    {
        APPL_TRACE_DEBUG2("<-- bts-tx %d bytes on bta hdl %d [nocopy]", len, p_chan->bta_hdl);

        //bts_log_tstamps_us("rfc_write", p_chan, FALSE);

        /* forward to rfcomm */
        BTA_JvRfcommWrite(p_chan->bta_hdl, p_buf, p, len);
    }
    else
    {
       //buf_flow_dec(dun_port);
       GKI_freebuf(p_buf);
       return;
    }

    /* now setup another buffer for next receive */
    rfc_setup_rx_buf(p_chan);
#else
    rfc_tx_data_copy(p_chan, p, len);
#endif
}



/*******************************************************************************
**
**
**     JV CALLBACK HANDLERS
**
**
**
*******************************************************************************/

void jv_rfc_srv_connect_ind(tBTA_JV_RFCOMM_OPEN *p_open)
{
    tBTLIF_BTS_RFC_CON_IND_PARAM ind;
    tBTS_CHAN *p_chan;

    int scn;
    int res;

    info("jvapi_rfcomm_connect_ind, handle %d, status %d", p_open->handle, p_open->status);

    p_chan = bts_chan_find_by_key(KEY_BTA_HDL, p_open->handle);

    if (p_chan == NULL)
    {
        info("No listener channel found, disconnect connection");
        /* no listener available, close this channel */
        BTA_JvRfcommClose(p_open->handle);
        return;
    }

    //ASSERTC(p_chan, "chan not found", 0);

    ind.status = p_open->status;
    ind.handle = p_open->handle;
    ind.listen_fd = p_chan->blz_listen_fd;

    /* store port api handle */
    bts_retrieve_port_handle(p_chan);

    /* get peer mtu for this connection */
    bts_retrieve_peer_mtu(p_chan);

    /* reverse bd addr */
    ind.rem_bda[5] = p_open->rem_bda[0];
    ind.rem_bda[4] = p_open->rem_bda[1];
    ind.rem_bda[3] = p_open->rem_bda[2];
    ind.rem_bda[2] = p_open->rem_bda[3];
    ind.rem_bda[1] = p_open->rem_bda[4];
    ind.rem_bda[0] = p_open->rem_bda[5];

    BTL_IF_CtrlSend(p_chan->btlif_ctrl, SUB_BTS, BTLIF_BTS_RFC_CON_IND, (tBTL_PARAMS*)&ind, sizeof(tBTLIF_BTS_RFC_CON_IND_PARAM));
}

void  jv_rfc_srv_listen_started(tBTA_JV_RFCOMM_START *p_start)
{
    tBTLIF_BTS_RFC_LISTEN_RSP_PARAM rsp;
    tBTS_CHAN *p_chan;
    int scn;
    int res;

    info("%s", __FUNCTION__);

    /* fixme -- need to use use_co ? */
    /* fixme -- need to use sec id ? */

    /* assumes callback is called immediately after jv api call */
    p_chan = bts_chan_find_by_key(KEY_BTA_HDL, BTA_HANDLE_PENDING);

    ASSERTC(p_chan, "chan not found", 0);

    /* unique blz20 reference */
    rsp.listen_fd = p_chan->blz_listen_fd;

    if (p_start->status != BTA_JV_SUCCESS)
    {
        error("failed with status %d", p_start->status);
        rsp.status = BTL_IF_GENERIC_ERR;
        rsp.scn = INVALID_VAL;
        rsp.bta_handle = INVALID_VAL;
        p_chan->bta_hdl = INVALID_VAL;
    }
    else
    {
        rsp.bta_handle = p_start->handle;
        rsp.rc_chan = p_chan->rc_chan;
        rsp.scn = p_chan->scn;
        rsp.status = BTL_IF_SUCCESS;
        p_chan->bta_hdl = p_start->handle;
    }
    BTL_IF_CtrlSend(p_chan->btlif_ctrl, SUB_BTS, BTLIF_BTS_RFC_LISTEN_RSP, (tBTL_PARAMS*)&rsp, sizeof(tBTLIF_BTS_RFC_LISTEN_RSP_PARAM));
}


void jv_rfc_close(tBTA_JV_RFCOMM_CLOSE *p)
{
    tBTS_CHAN *p_chan;

    info("%s hdl %d, status %d, remotely initiated %d, result %d, port status %d",
                    __FUNCTION__, p->handle, p->status, p->async, p->port_status);

    p_chan = bts_chan_find_by_key(KEY_BTA_HDL, p->handle);

    if (!p_chan)
    {
        debug("channel already disconnected");
        return;
    }

    if (p->async == 1)
    {
        /* remotely initiated */
        tBTLIF_BTS_RFC_DISC_IND_PARAM ind;

        /* send scn as that is a unique reference for jni, if we get close event right after connect req failure */
        ind.scn_remote = p_chan->scn;
        ind.status = p->port_status;

        /* send any valid socket as reference */
        if (p_chan->blz_listen_fd != DATA_SOCKET_INVALID)
        {
            ind.sock = p_chan->blz_listen_fd;
        }
        else if (p_chan->blz_sock != DATA_SOCKET_INVALID)
        {
            ind.sock = p_chan->blz_sock;
        }
        else {
            ASSERTC(0, "no valid sockets", 0);
            ind.sock = DATA_SOCKET_INVALID;
        }

        info("scn  %d, sock %d", ind.scn_remote, ind.sock);

        /* notify JNI that this link is down */
        BTL_IF_CtrlSend(p_chan->btlif_ctrl, SUB_BTS, BTLIF_BTS_RFC_DISC_IND, (tBTL_PARAMS*)&ind, sizeof(tBTLIF_BTS_RFC_DISC_IND_PARAM));

        /* wait for disc ind ack */
    }
    else
    {
        /* locally initiated */
        tBTL_IF_Result result;
        tBTLIF_BTS_RFC_CLOSE_CFM_PARAM cfm;

        cfm.handle = p->handle;
        cfm.status = p->port_status;

        /* disconnection requests is completed */
        result = BTL_IF_CtrlSend(p_chan->btlif_ctrl, SUB_BTS, BTLIF_BTS_RFC_CLOSE_CFM, (tBTL_PARAMS*)&cfm, sizeof(tBTLIF_BTS_RFC_CLOSE_CFM_PARAM));

        /* channel should already be deleted */
    }
}

void jv_forward_data_to_jni(UINT32 bta_handle)
{
    BT_HDR    *p_buf;
    tBTS_CHAN *p_chan;
    UINT16          port_errors;
    tPORT_STATUS    port_status;
    int             status;

    char *p;

    debug("BTA_JV_RFCOMM_DATA_IND_EVT bta hdl %d", bta_handle);

    p_chan = bts_chan_find_by_key(KEY_BTA_HDL, bta_handle);

    if (!p_chan)
    {
        /* resource not available, did ctrl channel detach ? */
        error("chan not found");
        return;
    }

    //ASSERTC(p_chan, "chan not found", 0);

    /* bypass jv layer and fetch gki buffer directly without copying data */
    if ((status = PORT_Read(p_chan->port_handle, &p_buf)) != PORT_SUCCESS)
    {
        GKI_freebuf(p_buf);

        /* check for and clear line error */
        if (status == PORT_LINE_ERR)
        {
            PORT_ClearError(p_chan->port_handle, &port_errors, &port_status);
        }

        error("PORT_ReadData error status:%d", status);
        return;
    }

    p = (char *)(p_buf+1);
    p+=p_buf->offset;

    APPL_TRACE_DEBUG2("jv_forward_data_to_jni %d bytes on hdl %d", p_buf->len, p_chan->dhdl);

    bts_update_stats(p_chan);
    //bts_log_tstamps_us("rfc_recv", p_chan, FALSE);

    BTL_IF_SendData(p_chan->dhdl, p, p_buf->len);
    GKI_freebuf(p_buf);
}


void jv_rfc_cong(tBTA_JV_RFCOMM_CONG *p_data)
{
    tBTS_CHAN *p_chan;

    APPL_TRACE_DEBUG3("jv_rfc_cong hdl %d, cong %d, status %d", p_data->handle, p_data->cong,  p_data->status);

    p_chan = bts_chan_find_by_key(KEY_BTA_HDL, p_data->handle);

    if (!p_chan)
    {
        /* resource not available, did ctrl channel detach ? */
        error("chan not found");
        return;
    }

    if (p_data->cong == 0)
    {
        /* flow on */
        BTL_IF_SetupRxFlow(p_chan->dhdl, 1);
    }
    else
    {
        /* flow off */
        BTL_IF_SetupRxFlow(p_chan->dhdl, 0);
    }
}


static void jw_if_rfcomm_srv_cback(tBTA_JV_EVT event, tBTA_JV *p_data)
{
    int rc;

    APPL_TRACE_DEBUG1("event=%s", jv_evt[event]);

    switch (event)
    {
    case BTA_JV_RFCOMM_START_EVT:
        info("[BTA_JV_RFCOMM_START_EVT] start status: %d, handle:%d, security id: %d",
            p_data->rfc_start.status, p_data->rfc_start.handle, p_data->rfc_start.sec_id);
         jv_rfc_srv_listen_started(&p_data->rfc_start);
        break;

    case BTA_JV_RFCOMM_OPEN_EVT:
         jv_rfc_srv_connect_ind(&p_data->rfc_open);
        break;

    case BTA_JV_RFCOMM_CLOSE_EVT:
         jv_rfc_close(&p_data->rfc_close);
        break;

    case BTA_JV_RFCOMM_READ_EVT:
        break;

    case BTA_JV_RFCOMM_WRITE_EVT:
        /* data write ack, free this buffer */
        jv_rfc_write(&p_data->rfc_write);
        break;

    case BTA_JV_RFCOMM_DATA_IND_EVT:
        jv_forward_data_to_jni(p_data->handle);
        break;

    case BTA_JV_RFCOMM_CONG_EVT:
        jv_rfc_cong(&p_data->rfc_cong);
        break;
    }
}


void jv_rfc_cl_open_evt(tBTA_JV_RFCOMM_OPEN *p_data)
{
    tBTLIF_BTS_RFC_CON_RSP_PARAM rsp;
    tBTS_CHAN *p_chan;

    info("handle %d, status %d, bd %s", p_data->handle, p_data->status, bd2str(p_data->rem_bda));

    p_chan = bts_chan_find_by_key(KEY_BTA_HDL, p_data->handle);

    ASSERTC(p_chan, "chan not found", 0);

    p_chan->bta_hdl = p_data->handle;

    /* store port api handle */
    bts_retrieve_port_handle(p_chan);

    /* get peer mtu for this connection */
    bts_retrieve_peer_mtu(p_chan);

    /* setup listener for incoming connection */
    BTL_IF_SetupListener(p_chan->btlif_ctrl, SUB_BTS, p_chan->btlif_port-BTLIF_PORT_BASE_BTS);

    rsp.handle = p_data->handle;
    rsp.scn_remote = p_chan->rc_chan;
    rsp.sock = p_chan->blz_sock;
    rsp.btlif_port = p_chan->btlif_port; /* notify client which port to connect to */

    memcpy(rsp.rem_bda, p_data->rem_bda, sizeof(BD_ADDR));

    if (p_data->status == BTA_JV_SUCCESS)
        rsp.status = BTL_IF_SUCCESS;
    else
        rsp.status = BTL_IF_GENERIC_ERR;

    BTL_IF_CtrlSend(p_chan->btlif_ctrl, SUB_BTS, BTLIF_BTS_RFC_CON_RSP, &rsp, sizeof(tBTLIF_BTS_RFC_CON_RSP_PARAM));
}


void jv_rfc_cl_init_evt(tBTA_JV_RFCOMM_CL_INIT *p_data)
{
    tBTS_CHAN *p_chan;

    info( "CL init status: %d, handle:%d, security id: %d", p_data->status,
        p_data->handle, p_data->sec_id);

    p_chan = bts_chan_find_by_key(KEY_BTA_HDL, BTA_HANDLE_PENDING);

    ASSERTC(p_chan, "chan not found", 0);

    /* store event params */
    p_chan->bta_hdl = p_data->handle;
    p_chan->sec_id = p_data->sec_id;
    p_chan->use_co = p_data->use_co;

}

static void jw_if_rfcomm_cl_cback(tBTA_JV_EVT event, tBTA_JV *p_data)
{
    int rc;

    debug( "jw_if_rfcomm_cl_cback event=%s", jv_evt[event]);

    switch (event)
    {
        case BTA_JV_RFCOMM_CL_INIT_EVT:
            jv_rfc_cl_init_evt(&p_data->rfc_cl_init);
            break;

        case BTA_JV_RFCOMM_OPEN_EVT:
            jv_rfc_cl_open_evt(&p_data->rfc_open);
            break;

        case BTA_JV_RFCOMM_CLOSE_EVT:
             jv_rfc_close(&p_data->rfc_close);
            break;

        case BTA_JV_RFCOMM_READ_EVT:
            info("BTA_JV_RFCOMM_READ_EVT   FIXME ");
            break;

        case BTA_JV_RFCOMM_WRITE_EVT:
            /* data write ack, free this buffer */
            jv_rfc_write(&p_data->rfc_write);
            break;

        case BTA_JV_RFCOMM_DATA_IND_EVT:

            jv_forward_data_to_jni(p_data->handle);
            break;

        case BTA_JV_RFCOMM_CONG_EVT:
            jv_rfc_cong(&p_data->rfc_cong);
            break;
    }
}



/*******************************************************************************
**
**
**      BTLIF CALLBACK HANDLERS
**
**
**
*******************************************************************************/


/*******************************************************************************
**
**      RFCOMM
**
*******************************************************************************/

/* this function should only be called after a listener is created */
int btapp_bts_convert_rc_chan_to_scn(int rc_channel)
{
    tBTS_CHAN *p_chan;

    /* search existing listeners for this rc channel */
    p_chan = bts_chan_find_by_key(KEY_RC_CHAN_SRV, rc_channel);

    if (p_chan == NULL)
    {
        error("rc chan %d not found", rc_channel);
        return -1;
    }

    info("found scn %d", p_chan->scn);
    return p_chan->scn;
}

static void btlif_rfc_bind_req(tCTRL_HANDLE btlif_hdl, tBTL_PARAMS *params)
{
    tBTS_CHAN *p_chan;

    tBTLIF_BTS_RFC_BIND_RSP_PARAM rsp;

    rsp.sock = params->bts_rfc_bind_req_param.sock;

    rsp.btlif_port = wrp_alloc_dynamicport(SUB_BTS, rsp.sock);

    if (rsp.btlif_port > 0)
    {
        rsp.status = BTL_IF_SUCCESS;

        /* allocate ctrl block and store params */
        p_chan = bts_chan_alloc_add(btlif_hdl);

        ASSERTC(p_chan, "bts chan alloc failed", 0);

        p_chan->type = BTS_TYPE_RFCOMM;
        p_chan->rc_chan = params->bts_rfc_bind_req_param.rc_chan; /* store jni reference */
        p_chan->btlif_port = rsp.btlif_port;
    }
    else
    {
        error("no ports available");
        rsp.status = BTL_IF_GENERIC_ERR;
    }

    BTL_IF_CtrlSend(btlif_hdl, SUB_BTS, BTLIF_BTS_RFC_BIND_RSP, (tBTL_PARAMS*)&rsp, sizeof(tBTLIF_BTS_RFC_BIND_RSP_PARAM));
}


static void btlif_rfc_listen_req(tCTRL_HANDLE btlif_hdl, tBTL_PARAMS *params)
{
    tBTLIF_BTS_RFC_LISTEN_RSP_PARAM rsp;
    int scn;
    int res;
    tBTA_JV_ROLE role;
    tBTA_SEC sec_mask;
    tBTS_CHAN *p_chan;
    char sdp_name[128];
    UINT8 max_session = 1;

    /* translate params */
    sec_mask = params->bts_rfc_listen_req_param.auth ? BTM_SEC_OUT_AUTHENTICATE : 0;
    sec_mask |= params->bts_rfc_listen_req_param.encrypt ? BTM_SEC_OUT_ENCRYPT : 0;
    role = (params->bts_rfc_listen_req_param.master)?BTA_JV_ROLE_MASTER:BTA_JV_ROLE_SLAVE;

    /* existing listeners ports are already checked up in blz20 wrapper */

    /* first try to use actual blz server channel */
    if (BTM_TryAllocateSCN((UINT8)params->bts_rfc_listen_req_param.rc_chan) == TRUE)
    {
        /* channel was available */
        scn = params->bts_rfc_listen_req_param.rc_chan;
        info("Allocated supplied server channel %d", scn);
    }
    else
    {
        /* allocate a dynamically assigned rfcomm server channel */
        scn = BTM_AllocateSCN();
        info("Allocated dynamic server channel %d (supplied %d)", scn, params->bts_rfc_listen_req_param.rc_chan);
    }

    /* sdp records are set through DBUS/DTUN */
#ifdef LINUX_NATIVE
    sprintf(sdp_name, "Serial Port (SCN=%d)", scn);
    res = bts_register_sdp_spp(scn, sdp_name);
    ASSERTC(res==SPP_SUCCESS, "sdp register failed", res);
#endif

    p_chan = bts_chan_find_by_key(KEY_BTLIF_PORT, params->bts_rfc_listen_req_param.btlif_port);

    ASSERTC(p_chan, "bts chan not found", params->bts_rfc_listen_req_param.btlif_port);

    p_chan->scn = scn;
    p_chan->blz_listen_fd = params->bts_rfc_listen_req_param.listen_fd;

    /* mark this chan so that we can find it when response comes back */
    p_chan->bta_hdl= BTA_HANDLE_PENDING;

    bts_chan_dump_all();

    info("start rfcomm server on scn %d, role %d, sec_mask 0x%x, btlifport %d", scn, role, sec_mask, p_chan->btlif_port);

    /* now start the server */
    res = BTA_JvRfcommStartServer(sec_mask, role, scn, max_session, jw_if_rfcomm_srv_cback);

    if (res != BTA_JV_SUCCESS)
    {
        error("start server failed due to %d", res);
        rsp.status = BTL_IF_GENERIC_ERR;
        rsp.bta_handle = INVALID_VAL;
        bts_chan_free(p_chan);

        BTL_IF_CtrlSend(p_chan->btlif_ctrl, SUB_BTS, BTLIF_BTS_RFC_LISTEN_RSP, (tBTL_PARAMS*)&rsp, sizeof(tBTLIF_BTS_RFC_CON_RSP_PARAM));
        return;
    }

    /* wait for listen start callback */
}

static void btlif_rfc_listen_cancel(tBTL_PARAMS *params)
{
    tBTS_CHAN *p_chan;

    p_chan = bts_chan_find_by_key(KEY_ANY_SOCK, params->bts_rfc_listen_cancel_param.listen_fd);

    ASSERTC(p_chan, "chan not found", 0);

    DUMP_CHAN("", p_chan);

    /* check if we have any active connections on this server channel */
    if (p_chan->port_handle == PORT_HDL_INVALID)
    {
        /* no active connections yet, stop server and free this channel */
        BTA_JvRfcommStopServer(p_chan->scn);

        /* free SCN */
        BTM_FreeSCN(p_chan->scn);

        bts_chan_free(p_chan);
    }
    else
    {
        /* current jv api doesn't support removing server listener without dropping and active connection on this server port
                  to work around this issue we will simply mark the listen fd as invalid and remove server once the active
                  connection is down */
        p_chan->listen_cancel_pnd = 1;
        p_chan->blz_listen_fd = -1;
        info("active connection for this port, mark channel and remove server once connection is closed");
    }

    /* if we have active connections free will be done upon btlif close instead */
}

static void btlif_rfc_connect_req(tCTRL_HANDLE btlif_hdl, tBTL_PARAMS *params)
{
    tBTLIF_BTS_RFC_CON_RSP_PARAM rsp;
    tBTA_JV_ROLE role;
    tBTA_SEC sec_mask;
    int res;
    BD_ADDR bd_addr;
    tBTS_CHAN *p_chan;

    /* allocate ctrl block and store params */
    p_chan = bts_chan_alloc_add(btlif_hdl);

    ASSERTC(p_chan, "bts chan alloc failed", 0);

    p_chan->type = BTS_TYPE_RFCOMM;
    p_chan->rc_chan = params->bts_rfc_con_req_param.scn_remote; /* store jni reference */
    p_chan->scn = params->bts_rfc_con_req_param.scn_remote;
    p_chan->blz_sock = params->bts_rfc_con_req_param.sock;

    p_chan->btlif_port = wrp_alloc_dynamicport(SUB_BTS, params->bts_rfc_con_req_param.sock);

    if (p_chan->btlif_port < 0)
    {
        rsp.status = BTL_IF_NO_PORT_AVAILABLE;
        rsp.handle = INVALID_VAL;
        rsp.scn_remote= params->bts_rfc_con_req_param.scn_remote;
        memset(rsp.rem_bda, 0, sizeof(BD_ADDR));
        BTL_IF_CtrlSend(p_chan->btlif_ctrl, SUB_BTS, BTLIF_BTS_RFC_CON_RSP, (tBTL_PARAMS*)&rsp, sizeof(tBTLIF_BTS_RFC_CON_RSP_PARAM));
        bts_chan_free(p_chan);
        return;
    }

    /* mark this chan so that we can find it when response comes back */
    p_chan->bta_hdl= BTA_HANDLE_PENDING;

    bts_chan_dump_all();

    /* translate params */
    sec_mask = params->bts_rfc_con_req_param.auth ? BTM_SEC_OUT_AUTHENTICATE : 0;
    sec_mask |= params->bts_rfc_con_req_param.encrypt ? BTM_SEC_OUT_ENCRYPT : 0;
    role = (params->bts_rfc_con_req_param.master)?BTA_JV_ROLE_MASTER:BTA_JV_ROLE_SLAVE;

    /* Reverse bdaddr */
    bd_addr[5] = params->bts_rfc_con_req_param.bd[0];
    bd_addr[4] = params->bts_rfc_con_req_param.bd[1];
    bd_addr[3] = params->bts_rfc_con_req_param.bd[2];
    bd_addr[2] = params->bts_rfc_con_req_param.bd[3];
    bd_addr[1] = params->bts_rfc_con_req_param.bd[4];
    bd_addr[0] = params->bts_rfc_con_req_param.bd[5];

    info("bts_rfcomm_connect_req : bd %s, sec_mask %x, role %d, scn %d, btlifport %d",
                    bd2str(bd_addr), sec_mask, role, params->bts_rfc_con_req_param.scn_remote, p_chan->btlif_port);

    res = BTA_JvRfcommConnect(sec_mask, role, params->bts_rfc_con_req_param.scn_remote,
                              bd_addr, jw_if_rfcomm_cl_cback);

    if (res == BTA_JV_FAILURE)
    {
        rsp.status = BTL_IF_GENERIC_ERR;
        rsp.handle = INVALID_VAL;
        rsp.scn_remote= params->bts_rfc_con_req_param.scn_remote; // unique id
        memset(rsp.rem_bda, 0, sizeof(BD_ADDR));

        BTL_IF_CtrlSend(p_chan->btlif_ctrl, SUB_BTS, BTLIF_BTS_RFC_CON_RSP, (tBTL_PARAMS*)&rsp, sizeof(tBTLIF_BTS_RFC_CON_RSP_PARAM));
        bts_chan_free(p_chan);
        return;
    }

    /* wait for connect response callback */

}


static void btlif_rfc_connect_ind_ack(tBTL_PARAMS *params)
{
    tBTS_CHAN *p_chan;
    tBTL_IF_Result ret;

    p_chan = bts_chan_find_by_key(KEY_BTA_HDL, params->bts_rfc_con_ind_ack_param.handle);

    ASSERTC(p_chan, "chan not found", 0);

    /* jni side acknowledged being ready for incoming connection */
    ret = BTL_IF_ConnectDatapath(p_chan->btlif_ctrl, &p_chan->dhdl, SUB_BTS, p_chan->btlif_port-BTLIF_PORT_BASE_BTS);

    ASSERTC(ret == BTL_IF_SUCCESS, "no acceptor setup", p_chan->dhdl);

    rfc_setup_rx_buf(p_chan);
}

static void btlif_rfc_close(tBTL_PARAMS *params)
{
    tBTS_CHAN *p_chan;

    debug("scn %d, sock %d", params->bts_rfc_close_param.scn_remote, params->bts_rfc_close_param.sock);

    p_chan = bts_chan_find_by_key(KEY_ANY_SOCK, params->bts_rfc_close_param.sock);

    if (!p_chan)
    {
        /* possibly chan was already closed remotely or btl-if client didn't yet bind this socket yet */
        info("channel not found scn %d, sock %d", params->bts_rfc_close_param.scn_remote, params->bts_rfc_close_param.sock);
        return;
    }

    DUMP_CHAN("", p_chan);

    if (p_chan->bta_hdl != INVALID_VAL)
    {
        info("close rfcomm connection");

        /* close active connection */
        BTA_JvRfcommClose(p_chan->bta_hdl);

        /* active connection is down, check if we have any pending listen cancel */
        if (p_chan->listen_cancel_pnd)
        {
            info("connection down, pending listener cancel found");
            BTA_JvRfcommStopServer(p_chan->scn);

            /* free SCN */
            BTM_FreeSCN(p_chan->scn);

            p_chan->listen_cancel_pnd = 0;
        }

        /* only free channel in case there is no listener */
        if (p_chan->blz_listen_fd == DATA_SOCKET_INVALID)
        {
            /* free channel as there is no listener waiting on this scn */
            bts_chan_free(p_chan);
        }
        else
        {
            /* invalidate data socket */
            p_chan->blz_sock = DATA_SOCKET_INVALID;

        }

        /* wait for close event where we will send disc response back */
    }
    else
    {
        info("rfcomm connection already closed");
        // only free when listener is cancelled bts_chan_free(p_chan);
    }
}

static void btlif_rfc_disc_ind_ack(tBTL_PARAMS *params)
{
    tBTS_CHAN *p_chan;

    info("scn remote %d", params->bts_rfc_disc_ind_ack_param.scn_remote);

    p_chan = bts_chan_find_by_key(KEY_ANY_SOCK, params->bts_rfc_disc_ind_ack_param.sock);

    if (!p_chan)
    {
        info("bts channel already closed");

        /* wait for client to close data chan */
        return;
    }

    if (p_chan->dhdl != DATA_SOCKET_INVALID)
    {
        /* close socket on this end */
        BTL_IF_DisconnectDatapath(p_chan->dhdl);
    }

    /* this server port was cancelled earlier but was deferred until active connection is down */
    if (p_chan->listen_cancel_pnd)
    {
        info("connection down, pending listener cancel found");
        BTA_JvRfcommStopServer(p_chan->scn);

        /* free SCN */
        BTM_FreeSCN(p_chan->scn);

        bts_chan_free(p_chan);
        return;
    }

    /* free channel if no listener available */
    if (p_chan->blz_listen_fd == DATA_SOCKET_INVALID)
    {
       bts_chan_free(p_chan);
    }
    else
    {
        /* we still have a listener, make sure port handle is invalidated */
        p_chan->port_handle = PORT_HDL_INVALID;
    }
}

/* when listener accepts a socket in btl if client this notification is sent */
static void btlif_rfc_rfc_lst_notify_dsock(tBTL_PARAMS *params)
{
    tBTS_CHAN *p_chan;

    info("listen_fd %d, dsock %d", params->bts_rfc_notify_lst_dsock_param.listen_fd, params->bts_rfc_notify_lst_dsock_param.data_sock_fd);

    p_chan = bts_chan_find_by_key(KEY_ANY_SOCK, params->bts_rfc_notify_lst_dsock_param.listen_fd);

    ASSERTC(p_chan, "ch not found", params->bts_rfc_notify_lst_dsock_param.listen_fd);

    p_chan->blz_sock = params->bts_rfc_notify_lst_dsock_param.data_sock_fd;
}

static void btlif_rfc_data_chan_ind(tBTL_PARAMS *params)
{
    tBTS_CHAN *p_chan;

    /* btlif port is a unique id across both server and client connections */
    p_chan = bts_chan_find_by_key(KEY_BTLIF_PORT, wrp_getport(SUB_BTS, params->chan_ind.subport));

    if (!p_chan)
    {
        info("bts channel already closed");

        /* wait for client to close data chan */
        return;
    }

    p_chan->dhdl = params->chan_ind.handle;


    info("connected, set dhdl %d", p_chan->dhdl);

    /* setup rx buffer */
    rfc_setup_rx_buf(p_chan);
}


static void btlif_rfc_data_chan_disc_ind(tBTL_PARAMS *params)
{
    tBTS_CHAN *p_chan;

    p_chan = bts_chan_find_by_key(KEY_DATA_HDL, params->chan_ind.handle);

    //ASSERTC(p_chan, "chan not found", params->chan_ind.handle);

    info("disconnected hdl %d", params->chan_ind.handle);

    if (!p_chan)
    {
        info("bts channel already closed");

        /* wait for client to close data chan */
        return;
    }

    /* invalidate data socket */
    p_chan->dhdl = DATA_SOCKET_INVALID;
}

static void btlif_rfc_rx_buf_pending(tBTL_PARAMS *params)
{
    BT_HDR *p_buf;

    /* make sure we free the rx buffer not yet delivered back */
    info("free rx buf %x", params->rx_buf_pnd.rx_buf);

    p_buf = (BT_HDR *)((UINT8 *) params->rx_buf_pnd.rx_buf - BT_HDR_SIZE - L2CAP_MIN_OFFSET - RFCOMM_MIN_OFFSET);

    GKI_freebuf(p_buf);
}

static void btlif_rfc_ctrl_chan_attached(tBTLIF_SUBSYSTEM_ATTACHED *params)
{
    info("client attached ctrl handle %d", params->handle);
}

static void btlif_rfc_ctrl_chan_detached(tCTRL_HANDLE hdl, tBTLIF_SUBSYSTEM_DETACHED *params)
{
    info("client detached ctrl handle %d", params->handle);
    bts_chan_free_all(hdl);
}


/*******************************************************************************
**
**      L2CAP
**
*******************************************************************************/


/*******************************************************************************
**
**      SCO
**
*******************************************************************************/


/*******************************************************************************
**
**    BTLIF MAIN CTRL CALLBACK
**
*******************************************************************************/

void btlif_bts_api_ctrl_cb(tCTRL_HANDLE ctrl_hdl, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params)
{
    debug("btlif_bts_api_ctrl_cb : id %s (%d) on hdl %d", dump_msg_id(id), id, ctrl_hdl);

    switch (id)
    {
        case BTLIF_BTS_RFC_BIND_REQ:
            btlif_rfc_bind_req(ctrl_hdl, params);
            break;
        case BTLIF_BTS_RFC_LISTEN_REQ:
            btlif_rfc_listen_req(ctrl_hdl, params);
            break;
        case BTLIF_BTS_RFC_LISTEN_CANCEL:
            btlif_rfc_listen_cancel(params);
            break;
        case BTLIF_BTS_RFC_CON_REQ:
            btlif_rfc_connect_req(ctrl_hdl, params);
            break;
        case BTLIF_BTS_RFC_CON_IND_ACK:
            btlif_rfc_connect_ind_ack(params);
            break;
        case BTLIF_BTS_RFC_CLOSE:
            btlif_rfc_close(params);
            break;
        case BTLIF_DATA_CHAN_IND :
            btlif_rfc_data_chan_ind(params);
            break;
        case BTLIF_DATA_CHAN_DISC_IND :
            btlif_rfc_data_chan_disc_ind(params);
            break;
        case BTLIF_BTS_RFC_DISC_IND_ACK:
            btlif_rfc_disc_ind_ack(params);
            break;
        case BTLIF_BTS_RFC_LST_NOTIFY_DSOCK:
            btlif_rfc_rfc_lst_notify_dsock(params);
            break;
        case BTLIF_DISC_RX_BUF_PENDING:
            btlif_rfc_rx_buf_pending(params);
            break;
        case BTLIF_SUBSYSTEM_ATTACHED:
            btlif_rfc_ctrl_chan_attached((tBTLIF_SUBSYSTEM_ATTACHED *)params);
            break;
        case BTLIF_SUBSYSTEM_DETACHED:
            btlif_rfc_ctrl_chan_detached(ctrl_hdl, (tBTLIF_SUBSYSTEM_DETACHED *)params);
            break;
        default :
            info("not implemented");
            break;
    }
}


static void jvapi_if_dm_cback(tBTA_JV_EVT event, tBTA_JV *p_data)
{
    debug("#### jw_if_dm_cback ev %s ####", jv_evt[event]);

    switch(event)
    {
    case BTA_JV_ENABLE_EVT:
        break;
    case BTA_JV_SET_DISCOVER_EVT:
        break;
    case BTA_JV_LOCAL_ADDR_EVT:
        break;
    case BTA_JV_LOCAL_NAME_EVT:
        break;
    case BTA_JV_REMOTE_NAME_EVT:
        break;
    case BTA_JV_SET_ENCRYPTION_EVT:
        break;
    case BTA_JV_GET_SCN_EVT:
        break;
    case BTA_JV_DISCOVERY_COMP_EVT:
        break;
    case BTA_JV_SERVICES_LEN_EVT:
        break;
    case BTA_JV_SERVICE_SEL_EVT:
        break;
    case BTA_JV_CREATE_RECORD_EVT:
        break;
    case BTA_JV_UPDATE_RECORD_EVT:
        break;
    case BTA_JV_ADD_ATTR_EVT:
        break;
    case BTA_JV_DELETE_ATTR_EVT:
        break;
    }
}


/*******************************************************************************
**
** Function         btapp_bts_init
**
** Description
**
** Returns
*******************************************************************************/

void btapp_bts_init(void)
{
    UINT32              sdp_handle;
    UINT8 scn;
    int res;

    info("%s", __FUNCTION__);

    /* init local variables */
    INIT_LIST_NODE(&bts_main.bts_chan_list);

    bts_main.bts_chan_list_count = 0;

    /* Register multiclient support in BTL IF for BTS */
    BTL_IF_RegisterSubSystemMultiClnt(SUB_BTS, btlif_bts_api_data_cb, btlif_bts_api_ctrl_cb);

    BTA_JvEnable(jvapi_if_dm_cback);
}




