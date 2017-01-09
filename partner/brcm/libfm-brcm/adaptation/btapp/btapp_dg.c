/****************************************************************************
**
**  Name:          btapp_dg.c
**
**  Description:   Contains application functions for dial up networking server
**
**
**  Copyright (c) 2002-2010, Broadcom Corp., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include "bt_target.h"

#if defined(BTA_DG_INCLUDED) && (BTA_DG_INCLUDED == TRUE)

#include "gki.h"
#include "gki_common.h"
#include "bta_api.h"
#include "btui.h"
#include "btui_int.h"
#include "bd.h"
#include "btapp_dm.h"
#include "btm_api.h"
#include "bte_appl.h"
#include "dtun_api.h"
#include "bta_dg_api.h"
#include "bta_dg_ci.h"
#include "bta_dg_co.h"
#include "utl.h"
#include "btapp_dg.h"
#include "btl_cfg.h"
#include "btl_ifs.h"

#include <signal.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



#include <sys/wait.h>

#define LOG_TAG "BTAPP_DG:"

#ifndef LINUX_NATIVE
#include <cutils/log.h>
#else
#include <stdio.h>
#define LOGI(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGD(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGV(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGE(format, ...)  fprintf (stderr, LOG_TAG format"\n", ## __VA_ARGS__)
#endif

#define BTAPP_DG_DEBUG TRUE

#ifndef BTAPP_DG_DEBUG
#define BTAPP_DG_DEBUG FALSE
#endif

/*****************************************************************************
**  Constants
*****************************************************************************/

#define MIN(a,b) ((a<b)?a:b)


#define DUN_PORT_INVALID (-1)
#define PPPD_THREAD_ID_INVALID (-1)
#define DUN_SERVER_HDL_INVALID (0)

#define PPPDCMD "pppd"

#define NOT_READY 0
#define START_PPP 1

#define PPP_FLAGSEQUENCE 0x7e
#define PPP_CONTROL_ESCAPE 0x7d
#define PPP_CONF_REQ 0x1
#define PPP_TERM_REQ 0x5
#define PROT_LCP 0xc021

/*****************************************************************************
**  Typedefs and macros
*****************************************************************************/

typedef struct
{
    UINT16       server_hdl;
    int          dun_port; /* back ref */
    tDATA_HANDLE datahdl;
    int          buf_cnt;
    BUFFER_Q     rx_q;

    pid_t        pppd_pid;
    int          pppd_port;
    int          pppd_mtu;
} t_DUNPORT;

typedef int t_DP_HANDLE;

#define APP_ID_TO_DUNPORT(appid) (appid-BTUI_DG_DUN_APPID_BEGIN)
#define DUNPORT_TO_APP_UD(dunport) (dunport+BTUI_DG_DUN_APPID_BEGIN)

#define GKIBUF_MUTEX_LOCK() pthread_mutex_lock(&gkibuf_mutex);
#define GKIBUF_MUTEX_UNLOCK() pthread_mutex_unlock(&gkibuf_mutex);

/*****************************************************************************
**  Local config
*****************************************************************************/

#ifndef DUN_MAX_PORTS
#define DUN_MAX_PORTS 3
#endif


/* data flow watermark levels per dun server */
#ifndef DUN_BUF_WATERMARK_FLOW_OFF
#define DUN_BUF_WATERMARK_FLOW_OFF (10)
#endif

#ifndef DUN_BUF_WATERMARK_FLOW_ON
#define DUN_BUF_WATERMARK_FLOW_ON  (9)
#endif

/* data flow watermark levels for each spp port */
#ifndef SPP_BUF_WATERMARK_FLOW_OFF
#define SPP_BUF_WATERMARK_FLOW_OFF        (10)
#endif

#ifndef SPP_BUF_WATERMARK_FLOW_ON
#define SPP_BUF_WATERMARK_FLOW_ON         (9)
#endif

#define DUN_ZERO_COPY_ENABLED
#define REMOTE_PEER_IP_ADDR_OFFSET 10

/*Default DUN security setting*/
#ifndef DG_DUN_DEFAULT_SECURITY
#define DG_DUN_DEFAULT_SECURITY BTA_SEC_AUTHORIZE
#endif

/*Default SPP security setting*/
#ifndef DG_SPP_DEFAULT_SECURITY
#define DG_SPP_DEFAULT_SECURITY BTA_SEC_NONE
#endif

/* use last spp port for redirection */
#ifndef DG_REDIRECTION_RESERVED_APP_ID
#define DG_REDIRECTION_RESERVED_APP_ID BTUI_DG_SPP_APPID_END
#endif

/* some customers require an accompanying SPP port along side the DUN port */
#ifndef DG_REDIRECTION_ACCOMPANYING_SPP_ENABLED
#define DG_REDIRECTION_ACCOMPANYING_SPP_ENABLED TRUE
#endif

/*****************************************************************************
**  Local vars
*****************************************************************************/

/* BTUI DG server main control block */

static tCTRL_HANDLE btapp_dun_ctrl = CTRL_SOCKET_INVALID;
static pthread_mutex_t gkibuf_mutex = PTHREAD_MUTEX_INITIALIZER;

static int dun_buf_total_cnt=0;
static int dun_nbr_open_ports = 0;

tBTUI_DG_CB btui_dg_cb;

static t_DUNPORT dport_cb[DUN_MAX_PORTS];

static unsigned char *pppd_options[16];

#ifndef DUN_ZERO_COPY_ENABLED
char pppd_rx_buf[1000];
#endif

/*****************************************************************************
**  Local Function prototypes
*****************************************************************************/

static tCTRL_HANDLE btapp_spp_ctrl = CTRL_SOCKET_INVALID;

static void bta_dg_dun_cback(tBTA_DG_EVT event, tBTA_DG *p_data);
static void btlif_dg_spp_on_rx_ctrl(tCTRL_HANDLE fd, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params);
static void btlif_dg_spp_on_rx_data(tDATA_HANDLE handle, char *p, int len);
static void stop_pppd(t_DP_HANDLE dp);
static BOOLEAN spp_setup_rx_buffer(int app_id);
static BOOLEAN setup_rx_buf(t_DP_HANDLE dp);
static void dg_free_rxbuf(t_DP_HANDLE dp);

/*******************************************************************************
**
** Function
**
** Description    Misc helper functions
**
** Returns          void
*******************************************************************************/

char * bd2str(BD_ADDR addr)
{
    static char msg[20];
    sprintf(msg, "%02x%02x%02x%02x%02x%02x",
            addr[0],addr[1],addr[2],
            addr[3],addr[4],addr[5]);
    return msg;
}

static void copy_to_rx_q(int dp, char *p, int len)
{
    BT_HDR *p_buf;

    APPL_TRACE_DEBUG1("enqueueing dun data (%d bytes)", len);

    if ((p_buf = GKI_getbuf(len)) != NULL)
    {
        p_buf->offset=0;
        p_buf->len = len;
        memcpy((char *)(p_buf+1), (char *)p, len);
        GKI_enqueue(&dport_cb[dp].rx_q , p_buf);
    }
    else
    {
        APPL_TRACE_ERROR0("Unable to enqueue dun buffer (no buffers)");
    }
}

static void flush_rx_q(int dp)
{
    BT_HDR *p_buf;

    if (dp>=DUN_MAX_PORTS)
    {
        error("invalid port (%d)", dp);
        return;
    }

    /* Check if anything enqueued */
    while ((p_buf = (BT_HDR *)GKI_dequeue(&dport_cb[dp].rx_q)) != NULL)
    {
        APPL_TRACE_DEBUG1("Flushing dun rx queue %i bytes", p_buf->len);
        BTL_IF_SendData(dport_cb[dp].datahdl, (char *)(p_buf+1), p_buf->len);
        GKI_freebuf(p_buf);
    }
}


/*******************************************************************************
**
** Function         spp_is_reserved_app_id
**
** Description
**
** Returns          Returns TRUE if app id is reserved
**
*******************************************************************************/

BOOLEAN spp_is_reserved_app_id(int app_id)
{
    if( (app_id > BTUI_DG_SPP_APPID_END) ||
        (BTPORT_IsEnabled() && app_id == DG_REDIRECTION_RESERVED_APP_ID))
    {
        /* invalid port */
        LOGI("%s: reserved or invalid application id (%d)", __FUNCTION__, app_id);
        return TRUE;
    }
    return FALSE;
}

/*******************************************************************************
**
** Function         dun_init_port
**
** Description
**
** Returns          void
*******************************************************************************/

void dun_init_port(t_DP_HANDLE dp)
{
    if (dp>=DUN_MAX_PORTS)
    {
        APPL_TRACE_ERROR1("dun_init_port : invalid port (%d)", dp);
        return;
    }

    dport_cb[dp].pppd_pid = PPPD_THREAD_ID_INVALID;
    dport_cb[dp].dun_port = DUN_PORT_INVALID;
    dport_cb[dp].pppd_port = DUN_PORT_INVALID;
    dport_cb[dp].datahdl = DATA_SOCKET_INVALID;
    dport_cb[dp].buf_cnt = 0;

    /* used to store incoming data before pppd datapath is setup */
    GKI_init_q(&dport_cb[dp].rx_q);
}

/*******************************************************************************
**
** Function         dun_init
**
** Description
**
** Returns          void
*******************************************************************************/

void dun_init(void)
{
    int i;

    for (i=0; i< DUN_MAX_PORTS; i++)
    {
        dun_init_port(i);
    }

    dun_buf_total_cnt = 0;

    /* init redirection mode if enabled */
    BTPORT_Init();
}


/*******************************************************************************
**
** Function         dun_close_port
**
** Description
**
** Returns          void
*******************************************************************************/

void dun_close_port(t_DP_HANDLE dp)
{
    if (dp >= DUN_MAX_PORTS)
    {
        APPL_TRACE_ERROR1("dun_close_port : invalid subport (%d)", dp);
        return;
    }

    APPL_TRACE_EVENT1("#### dun_close_port %d ####", dp);

    stop_pppd(dp);
    dun_init_port(dp);
}

/*******************************************************************************
**
** Function         dun_get_port_by_dhdl
**
** Description
**
** Returns          void
*******************************************************************************/

int dun_get_port_by_dhdl(tDATA_HANDLE handle)
{
    int i;

    for (i=0; i< DUN_MAX_PORTS; i++)
    {
        if (dport_cb[i].datahdl == handle)
            return dport_cb[i].dun_port;
    }
    return DUN_PORT_INVALID;
}



/*******************************************************************************
**
** Function         spp_close_connection
**
** Description
**
** Returns          void
*******************************************************************************/

void spp_close_connection(tBTUI_DG_APP_CB *p_cb, int app_id)
{
    tBTL_PARAMS params;

    /* check for reserved app id in case redirection mode is enabled */
    if (spp_is_reserved_app_id(app_id))
    {
        /* stop redirection port */
        BTPORT_StopRedirection(BTA_SPP_SERVICE_ID, p_cb->data_handle);
        params.dg_close_param.notify_only = 1;
    }
    else
    {
        if(p_cb->data_handle == DATA_SOCKET_INVALID)
        {
            LOGD("%s CLOSE EVT Cancel Listener on Port %d",
                 __FUNCTION__,
                 app_id);

            /* make sure we cancel any pending listener for this port */
            BTL_IF_CancelListener(btapp_spp_ctrl, SUB_SPP, app_id);
        }
        else
        {
            // let the frameworks disconnect the data path
            // BTL_IF_DisconnectDatapath(p_cb->data_handle);
            // p_cb->data_handle=DATA_SOCKET_INVALID;
        }
        params.dg_close_param.notify_only = 0;
    }

    /* Forward to BTL_IF client and the java application will be informed the event */
    params.dg_close_param.app_id = app_id;

    BTL_IF_CtrlSend(btapp_spp_ctrl,
                    SUB_SPP,
                    BTLIF_DG_CLOSE_EVT,
                    &params,
                    sizeof(tBTLIF_DG_CLOSE_PARAM));

}

/*******************************************************************************
**
** Function         dun_close_connection
**
** Description
**
** Returns          void
*******************************************************************************/

void dun_close_connection(int dp)
{
    /* check for reserved app id in case redirection mode is enabled */
    if(BTPORT_IsEnabled())
    {
        /* start redirection port */
        BTPORT_StopRedirection(BTA_DUN_SERVICE_ID, dport_cb[dp].datahdl);
    }
    else
    {
        /* only cancel listener if no data connection was completed */
        if (dport_cb[dp].datahdl == DATA_SOCKET_INVALID)
        {
            /* make sure we cancel any pending listener for this port */
            BTL_IF_CancelListener(btapp_dun_ctrl, SUB_DUN, dp);
        }
    }

    dun_close_port(dp);
}

/*******************************************************************************
**
** Function         buf_flow_inc
**
** Description
**
** Returns          void
*******************************************************************************/

/* increase buffer usage, flow off if we filled our max usage */
static void buf_flow_inc(t_DP_HANDLE dp)
{
    GKIBUF_MUTEX_LOCK();
    dun_buf_total_cnt++;
    dport_cb[dp].buf_cnt++;
    GKIBUF_MUTEX_UNLOCK();

    APPL_TRACE_DEBUG2("      ++ dun port buf usage %d (%d)", dport_cb[dp].buf_cnt, dun_buf_total_cnt);

    /* flow off if we reached high watermark */
    if ((dport_cb[dp].datahdl != DATA_SOCKET_INVALID) && dport_cb[dp].buf_cnt >= DUN_BUF_WATERMARK_FLOW_OFF)
        BTL_IF_SetupRxFlow(dport_cb[dp].datahdl, 0);
}

/*******************************************************************************
**
** Function         buf_flow_dec
**
** Description
**
** Returns          void
*******************************************************************************/

/* decrease buffer usage, flow on if reached low watermark */
static void buf_flow_dec(t_DP_HANDLE dp)
{
    GKIBUF_MUTEX_LOCK();
    dun_buf_total_cnt--;
    dport_cb[dp].buf_cnt--;
    GKIBUF_MUTEX_UNLOCK();

    APPL_TRACE_DEBUG2("      -- dun port buf usage %d (%d)", dport_cb[dp].buf_cnt, dun_buf_total_cnt);

    /* flow on if we reached low watermark */
    if ((dport_cb[dp].datahdl != DATA_SOCKET_INVALID) && dport_cb[dp].buf_cnt <= DUN_BUF_WATERMARK_FLOW_ON)
        BTL_IF_SetupRxFlow(dport_cb[dp].datahdl, 1);
}

/*******************************************************************************
**
** Function         setup_rx_buf
**
** Description     zero copy buffering
**
** Returns          void
*******************************************************************************/

static BOOLEAN setup_rx_buf(t_DP_HANDLE dp)
{
    BT_HDR          *p_buf;
    int buf_sz;
    char *p_tmp;
    t_DUNPORT *dun;

    dun = &dport_cb[dp];

    APPL_TRACE_DEBUG1("setup_rx_buf on dunport %d", dp);

    /* set up new receive buffer */
    if ((p_buf = (BT_HDR *) GKI_getpoolbuf(RFCOMM_DATA_POOL_ID)) != NULL)
    {
        buf_sz = RFCOMM_DATA_POOL_BUF_SIZE; /* the same but more efficient then GKI_get_buf_size(p_buf); */
        p_buf->len = 0;
        p_buf->offset = L2CAP_MIN_OFFSET + RFCOMM_MIN_OFFSET;
        p_tmp = (UINT8 *)(p_buf + 1) + p_buf->offset;

        APPL_TRACE_DEBUG3("setup_rx_buf : max %d, offset %d, len %d ", buf_sz, p_buf->offset, p_buf->len);

        buf_sz -= (BT_HDR_SIZE + p_buf->offset);

        if (buf_sz < dun->pppd_mtu)
        {
            APPL_TRACE_WARNING0("warning : gki buffer too small !");
            GKI_freebuf( p_buf );
            return FALSE;
        }

        /* setup receive buffer to match the rfcomm links mtu size */
        if (BTL_IF_SetupRxBuf(dun->datahdl, p_tmp, dun->pppd_mtu) != BTL_IF_SUCCESS)
        {
            APPL_TRACE_ERROR1("%s: setup rx buffer failed", __FUNCTION__);
            GKI_freebuf(p_buf);
            return FALSE;
        }
        return TRUE;
    }
    else
    {
        APPL_TRACE_ERROR1("%s: Out of buffers", __FUNCTION__);
        return FALSE;
    }

    return TRUE;
}

/*******************************************************************************
**
** Function         tx_data_copy
**
** Description
**
** Returns          void
*******************************************************************************/

static BOOLEAN tx_data_copy(t_DP_HANDLE dp, char *p, int len)
{
    tBTUI_DG_APP_CB *p_cb;
    BT_HDR          *p_buf;
    int buf_sz;
    char *p_tmp;
    int app_id = DUNPORT_TO_APP_UD(dp);

    if (app_id >= BTUI_DG_NUM_SERVICES)
    {
        APPL_TRACE_ERROR1("tx_data_copy :  invalid app_id (%d)", app_id);
        return FALSE;
    }

    p_cb = &btui_dg_cb.app_cb[app_id];

    APPL_TRACE_DEBUG2("bt <-- dun %d bytes on app id %d [copy]", len, app_id);

    hex_dump("bt <-- dun", p, len, 0);

    /* allocate buffer */
    if ((p_buf = (BT_HDR *) GKI_getpoolbuf(RFCOMM_DATA_POOL_ID)) != NULL)
    {
        buf_sz = RFCOMM_DATA_POOL_BUF_SIZE; /* the same but more efficient then GKI_get_buf_size(p_buf); */

        p_buf->offset = L2CAP_MIN_OFFSET + RFCOMM_MIN_OFFSET;
        buf_sz -= (sizeof(BT_HDR) + p_buf->offset);
        p_tmp = (UINT8 *)(p_buf + 1) + p_buf->offset;

        p_buf->len = len;
        memcpy(p_tmp, p, len);

        GKI_enqueue(&p_cb->data_q, p_buf);

        buf_flow_inc(dp);

        bta_dg_ci_rx_ready(p_cb->port_handle);

        return TRUE;
    }
    else
    {
        APPL_TRACE_DEBUG1("%s: Out of buffers", __FUNCTION__);
        return FALSE;
    }
}

/*******************************************************************************
**
** Function         btlif_dun_ppp_data_cb
**
** Description
**
** Returns          void
*******************************************************************************/
void btlif_dun_ppp_data_cb(tDATA_HANDLE dhdl, char *p, int len)
{
    tBTUI_DG_APP_CB *p_cb;
    BT_HDR          *p_buf;
    int dun_port;
    int app_id;

    /* find dun port from data handle */
    dun_port = dun_get_port_by_dhdl(dhdl);

    if (dun_port == DUN_PORT_INVALID)
    {
        APPL_TRACE_ERROR1("dun_rx_ppp_data : ERROR, no dun port found for this handle (%d)", dhdl);
        return;
    }

    app_id = DUNPORT_TO_APP_UD(dun_port);

    p_cb = &btui_dg_cb.app_cb[app_id];

    //APPL_TRACE_DEBUG2("dun_rx_ppp_data :  len %d (port %d)", len, dun_port);

#ifdef DUN_ZERO_COPY_ENABLED
    p_buf = (BT_HDR *)((UINT8 *) p - BT_HDR_SIZE - L2CAP_MIN_OFFSET - RFCOMM_MIN_OFFSET);
    p_buf->len = len;
    p_buf->event = 0;
    p_buf->layer_specific = 0;

    /* notify new data */
    if (p_buf->len > 0)
    {
        APPL_TRACE_DEBUG2("bt <-- dun %d bytes on app id %d [ppp]", len, app_id);

        hex_dump("bt <-- dun", p, len, 0);

        GKI_enqueue(&p_cb->data_q, p_buf);

        buf_flow_inc(dun_port);

        bta_dg_ci_rx_ready(p_cb->port_handle);
    }
    else
    {
        //buf_flow_dec(dun_port);
        GKI_freebuf(p_buf);
        return;
    }

    /* now setup another buffer for next receive */
    setup_rx_buf(dun_port);
#else
    tx_data_copy(dun_port, p, len);
#endif
}

/******************************************************
 ** Function         bta_dg_dun_enable_ports
 **
 ** Description      Enable the DUN listening ports
 **
 ** Returns          void
 ******************************************************/
static void bta_dg_dun_enable_ports(void)
{
    int i;
    for (i=0; i<DUN_MAX_PORTS; i++)
        dport_cb[i].server_hdl=DUN_SERVER_HDL_INVALID;

    for (i=0; i<DUN_MAX_PORTS; i++)
    {
        /* Initialize server hdl for this port to be invalid to ensure it is set properly */
        BTA_DgListen(BTA_DUN_SERVICE_ID, DG_DUN_DEFAULT_SECURITY, BTUI_DUNDG_SERVICE_NAME, BTUI_DG_DUN_APPID_BEGIN+i);
    }

#if DG_REDIRECTION_ACCOMPANYING_SPP_ENABLED == TRUE
    /* if redirection and accompanying spp port is enabled, set it up */
    if(BTPORT_IsEnabled())
    {
        LOGI("bta_dg_dun_enable_ports(): starting accompanying spp port %d", DG_REDIRECTION_RESERVED_APP_ID);
        BTA_DgListen(BTA_SPP_SERVICE_ID,
                     DG_SPP_DEFAULT_SECURITY,
                     BTUI_SPPDG_SERVICE_NAME,
                     DG_REDIRECTION_RESERVED_APP_ID);
    }
#endif
}

/******************************************************
 ** Function         bta_dg_dun_disable_ports
 **
 ** Description      Disable the DUN listening ports
 **
 ** Returns          void
 ******************************************************/

static void bta_dg_dun_disable_ports(void)
{
    int i;

    for (i=0; i<DUN_MAX_PORTS; i++)
    {
        UINT16 dun_server_hdl = dport_cb[i].server_hdl;

        /* cancel listener if noone connected */
        if (dport_cb[i].datahdl == DATA_SOCKET_INVALID)
        {
            BTL_IF_CancelListener(btapp_dun_ctrl, SUB_DUN, i);
        }

        dun_close_connection(i);

        if (dun_server_hdl != DUN_SERVER_HDL_INVALID)
        {
            LOGI("bta_dg_dun_disable_ports(): shutting down port %d", dun_server_hdl);
            BTA_DgShutdown(dun_server_hdl);
            dport_cb[i].server_hdl=DUN_SERVER_HDL_INVALID;
        }
    }

#if DG_REDIRECTION_ACCOMPANYING_SPP_ENABLED == TRUE
        /* if redirection and accompanying spp port is enabled, set it up */
        if(BTPORT_IsEnabled())
        {
            UINT16 port_handle = btui_dg_cb.app_cb[DG_REDIRECTION_RESERVED_APP_ID].listen_handle;
            LOGI("bta_dg_dun_disable_ports(): shutting down accompanying spp port %d", port_handle);
            BTA_DgShutdown(port_handle);
        }
#endif

    BTL_IF_CtrlSend(btapp_dun_ctrl, SUB_DUN, BTLIF_DG_DISABLE_EVT, NULL, 0);

    dun_nbr_open_ports = 0;
}

/******************************************************
 ** Function         bta_dg_dun_ports_have_state
 **
 ** Description      Checks if all the dun ports have the same state. 1 = enabled 0 = disabled
 **
 ** Returns          TRUE if all ports have same state, FALSE otherwise
 ******************************************************/

static BOOLEAN bta_dg_dun_ports_have_state(int state)
{
    int i;

    if (state == 0)
    {
        for (i =0; i < DUN_MAX_PORTS;i++)
        {
            LOGI("bta_dg_dun_ports_have_state(disable): port %d hdl = %d", i, dport_cb[i].server_hdl);
            if (dport_cb[i].server_hdl != DUN_SERVER_HDL_INVALID)
            {
                return FALSE;
            }
        }
    }
    else
    {
        for (i =0; i < DUN_MAX_PORTS;i++)
        {
            LOGI("bta_dg_dun_ports_have_state(enable): port %d hdl = %d", i, dport_cb[i].server_hdl);
            if (dport_cb[i].server_hdl == DUN_SERVER_HDL_INVALID)
            {
                return FALSE;
            }
        }
    }
    LOGI("bta_dg_dun_ports_have_state() returning TRUE");
    return TRUE;
}


#if( defined DUN_USE_BTPORT ) && (DUN_USE_BTPORT == TRUE)
/*********************************************************************************************
 **
 ** Function        process_modem_ctrl_evt
 **
 ** Description     btlif modem signal are sent to remote. the assumption is that modem signal
 **                 bit fields are the same as BTA!
 **
 ** Returns         void
 **
 ** WARNING:        depending on platform, the signal may be set from at different interface, e.g.
 **                 kernel UART driver. in this case application should not set the signals!
 **********************************************************************************************/
static void process_modem_ctrl_evt( UINT8 app_id, UINT8 signals, UINT8 values )
{
    tBTUI_DG_APP_CB *p_cb       = &btui_dg_cb.app_cb[app_id];
    unsigned int current_modem  = p_cb->modem_ctrl;
    unsigned int sig            = signals;
    unsigned int val            = values;

#if (BTAPP_DG_DEBUG==TRUE)
    APPL_TRACE_EVENT4( "process_modem_ctrl_evt( app_id: %d, signals: x%x, values: x%x ): current: x%x",
                       app_id, signals, values, current_modem );
#endif
    /* paranoia test */
    if ( sig )
    {
        /* updated current modem state with changed signal */
        current_modem = ( current_modem & (~signals) ) | values;
        p_cb->modem_ctrl = current_modem;
        /* send changed values to remote side. to avoid pre-emption, do it last */
        bta_dg_ci_control( p_cb->port_handle, signals , values );
#if (BTAPP_DG_DEBUG==TRUE)
    APPL_TRACE_EVENT1( "process_modem_ctrl_evt(): NEW current: x%x",
                       current_modem );
#endif
    }
} /* process_modem_ctrl_evt() */
#endif


#if( defined DUN_USE_SMD_INTERFACE ) && (DUN_USE_SMD_INTERFACE == TRUE)
/*********************************************************************************************
**
** Function         process_flow_ctrl_evt
**
** Description     btl if flow ctrl request from Phone RIL interface (Shared Memory only)
**
** Returns          void
**********************************************************************************************/
void process_flow_ctrl_evt(int signals, int mask)
{
    LOGI("process_flow_ctrl_evt: signals = 0x%02x, mask = 0x%02x", signals, mask);
    smd_handle_ril_flow_ctrl_req(BTA_DUN_SERVICE_ID, signals, mask);
}

/*********************************************************************************************
**
** Function         btapp_dg_set_ril_flow_ctrl
**
** Description     Send flow control command.
**
** Returns          void
**********************************************************************************************/
void btapp_dg_set_ril_flow_ctrl(int signals, int mask)
{
    tBTL_PARAMS params;

    params.dg_ril_flow_ctrl_param.mask = mask;
    params.dg_ril_flow_ctrl_param.signals = signals;
    BTL_IF_CtrlSend(btapp_dun_ctrl,
                    SUB_DUN,
                    BTLIF_DG_RIL_FLOW_CTRL_EVT,
                    &params,
                    sizeof(tBTLIF_DG_RIL_FLOW_CTRL_PARAM));
}

#endif /* #if( defined DUN_USE_SMD_INTERFACE ) && (DUN_USE_SMD_INTERFACE == TRUE) */


#if( defined DUN_USE_BTPORT ) && (DUN_USE_BTPORT == TRUE)
/*********************************************************************************************
 **
 ** Function        btapp_dg_btlif_send_modem_ctrl
 **
 ** Description     sends modem control (flow, state etc) to btlif. currently this works only for
 **                 DUN but there should be NO difference for DUN or SPP
 **
 ** Returns          void
 **
 **********************************************************************************************/
void btapp_dg_btlif_send_modem_ctrl( const UINT8 app_id,
                                     const UINT8 signals,
                                     const UINT8 values )
{
    tBTLIF_DG_MODEM_CTRL_PARAM modem_params;

#if (BTAPP_DG_DEBUG==TRUE)
    APPL_TRACE_EVENT3( "btapp_dg_btlif_send_modem_ctrl( app_id: %d, signals: x%x, values: x%x ) for SUB_DUN",
                       app_id, signals, values );
#endif
    modem_params.app_id     = app_id;
    modem_params.signals    = signals;
    modem_params.values     = values;
    BTL_IF_CtrlSend( btapp_dun_ctrl,
                     SUB_DUN,
                     BTLIF_DG_MODEM_CTRL_EVT,
                     (tBTL_PARAMS *)&modem_params,
                     sizeof(tBTLIF_DG_MODEM_CTRL_PARAM));
} /* bta_dg_btlif_send_modem_ctrl() */
#endif


/*******************************************************************************
**
** Function         btlif_dun_ctrl_cb
**
** Description     btl if control callback for DUN subsystem
**
** Returns          void
*******************************************************************************/

void btlif_dun_ctrl_cb(tCTRL_HANDLE ctrl_hdl, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params)
{
    t_DP_HANDLE dp;
    t_DUNPORT *dun;
    UINT16 port_handle;

    APPL_TRACE_EVENT1("btlif_dun_ctrl_cb : id %s", dump_msg_id(id));

    switch (id)
    {
        case BTLIF_DG_ENABLE:
          bta_dg_dun_enable_ports();
          break;

        case BTLIF_DG_DISABLE:
          bta_dg_dun_disable_ports();
          break;

        case BTLIF_DATA_CHAN_IND:
            dp = params->chan_ind.subport;

            if (dp >= DUN_MAX_PORTS)
            {
                APPL_TRACE_EVENT1("btapp_dun_receive_ctrl : BTLIF_DATA_CHAN_IND, invalid subport (%d)", dp);
                return;
            }

            dun = &dport_cb[dp];
            dun->datahdl = params->chan_ind.handle;

            APPL_TRACE_EVENT1("Got data channel fd %d", dun->datahdl);

#ifndef DUN_ZERO_COPY_ENABLED
            BTL_IF_SetupRxBuf(dun->datahdl, pppd_rx_buf, dun->pppd_mtu);
#else
            /* setup first rx buffer */
            setup_rx_buf(dp);
#endif
            break;

        case BTLIF_DATA_CHAN_DISC_IND:
            dp = params->chan_ind.subport;

            if (dp >= DUN_MAX_PORTS)
            {
                APPL_TRACE_EVENT1("btapp_dun_receive_ctrl : BTLIF_DATA_CHAN_DISC_IND, invalid subport (%d)", dp);
                return;
            }

            dun = &dport_cb[dp];

            stop_pppd(dp);

            dun_init_port(dp);
            break;

        case BTLIF_DISC_RX_BUF_PENDING:
            {
                /* make sure we free the rx buffer not yet delivered back */
                BT_HDR *p_buf;

                info("free rx buf (0x%x)", params->rx_buf_pnd.rx_buf);

                p_buf = (BT_HDR *)((UINT8 *) params->rx_buf_pnd.rx_buf - BT_HDR_SIZE - L2CAP_MIN_OFFSET - RFCOMM_MIN_OFFSET);
                GKI_freebuf(p_buf);
            }
            break;

#if( defined DUN_USE_BTPORT ) && (DUN_USE_BTPORT == TRUE)
        case BTLIF_DG_MODEM_CTRL_REQ:
            process_modem_ctrl_evt( params->dg_modem_ctrl_param.app_id,
                                    params->dg_modem_ctrl_param.signals,
                                    params->dg_modem_ctrl_param.values );
            break;
#endif

#if( defined DUN_USE_SMD_INTERFACE ) && (DUN_USE_SMD_INTERFACE == TRUE)
        case BTLIF_DG_RIL_FLOW_CTRL_REQ:
            process_flow_ctrl_evt(params->dg_ril_flow_ctrl_param.signals,
                                  params->dg_ril_flow_ctrl_param.mask);
			break;
#endif /* #if( defined DUN_USE_SMD_INTERFACE ) && (DUN_USE_SMD_INTERFACE == TRUE) */

        case BTLIF_DG_CLOSE:
            /* check if port id is valid, skip reserved port or if out of bounds */
            if (spp_is_reserved_app_id(params->dg_close_param.app_id))
                return;

            /* We never expose the handle to Java,
             * so we must find handle from btui_cb
             * through app_id, which are the same
             * for java and bta
             */
            port_handle = btui_dg_cb.app_cb[params->dg_close_param.app_id].port_handle;

            APPL_TRACE_DEBUG3("%s Rx BTLIF_DG_CLOSE app_id = %d port_handle = %d",
                              __FUNCTION__,
                              params->dg_close_param.app_id,
                              port_handle);

            BTA_DgClose(port_handle);
            break;

        default:
            debug("signal not handled");
            break;
    }
}

/*******************************************************************************
**
** Function         fake_modem
**
** Description     Sets up the modem communication towards remote host to enable PPP startup
**
** Returns          void
*******************************************************************************/

int fake_modem(t_DP_HANDLE dp, char *p, int len)
{
    tBTUI_DG_APP_CB *p_cb;
    int app_id = DUNPORT_TO_APP_UD(dp);

    char *connect = "\r\nCONNECT\r\n";
    char *client_server = "CLIENTSERVER\r";
    char *ok = "\r\nOK\r\n";
    char *busy = "\r\nBUSY\r\n";
    int done = 0, i;

    p_cb = &btui_dg_cb.app_cb[app_id];

    hex_dump("modem data", p, len, 0);

    if (len <= 0)
        return NOT_READY;

    /* Check if p is a ppp frame, if so start pppd right away */

    for (i = 0; i < len; i++)
    {
        if ((p[i] == PPP_FLAGSEQUENCE))
        {
            if ((len - i) >= 7)
            {
                unsigned short prot;
                unsigned short type;

                /* check for escaped chars */
                if (p[i+2] == PPP_CONTROL_ESCAPE)
                {
                    APPL_TRACE_EVENT0("Found ctrl esc");
                    prot = ((p[i+4]<<8) | (p[i+5]));
                }
                else
                {
                    prot = ((p[i+3]<<8) | (p[i+4]));
                }

                APPL_TRACE_EVENT1("prot : %x", prot);

                APPL_TRACE_EVENT2("ppp packet : ", p, len);

                if (prot == PROT_LCP)
                {
                    type = (p[i+7]^0x20);

                    if (type == PPP_TERM_REQ)
                    {
                        APPL_TRACE_EVENT0("Found PPP Terminate, restart modem emulator");
                        return NOT_READY;
                    }
                    else if (type == PPP_CONF_REQ)
                    {
                        APPL_TRACE_EVENT0("Found PPP Connect, start pppd");
                        return START_PPP;
                    }
                    else
                    {
                        APPL_TRACE_EVENT1("Unknown PPP LCP type [%d]", type);
                        return START_PPP; /* start pppd anyway */
                    }
                }
                else
                {
                    APPL_TRACE_EVENT1("Unknown PPP prot type [%d]", prot);
                    return NOT_READY;
                }
            }
        }
    }

    /* AT commands */

    if (!strncasecmp(p, "ATD", 3)) /* windows standard modem */
    {
        APPL_TRACE_EVENT0("Got ATD");

        /* check if call is active - if so return BUSY to DUN Client */
        if(btl_cfg_get_call_active_status())
        {
            APPL_TRACE_EVENT0("Call currently active - do not allow DUN connection");

            tx_data_copy(dp, busy, strlen(busy));
            return NOT_READY;
        }

#if (TRUE == BTAPP_DG_EMULATOR)
        p_cb->at_cmd_mode = FALSE;  /* CONNECT means binary pdu mode! Apply flow control! */
#endif
        /* indicate DCD [DCE] to remote (DTE) before sending connect */
        bta_dg_ci_control( p_cb->port_handle, BTA_DG_CD, BTA_DG_CD_ON );
        tx_data_copy(dp, connect, strlen(connect));

        APPL_TRACE_EVENT0("Modem connected!");
        return START_PPP;
    }
    else if (!strncasecmp(p, "CLIENT", 6)) /* windows null modem */
    {
        APPL_TRACE_EVENT0("Got CLIENT");
        tx_data_copy(dp, client_server, strlen(client_server));
        APPL_TRACE_EVENT0("Nullmodem connected!");
        return START_PPP;
    }
    else
    {
        APPL_TRACE_EVENT0("Modem emulator replies OK");
        tx_data_copy(dp, ok, strlen(ok));
    }

    return NOT_READY;
}

/*******************************************************************************
**
** Function         nat_setup
**
** Description     Configures iptables for virtual network for remote peer
**
** Returns          void
*******************************************************************************/

void nat_setup(char *netname)
{
    char cmdline[255];

    LOGI("Setting up private network for remote peer net if [%s]", netname);

    // fixme -- setup more rules using separate script

    /* setup iptables for NAT */
    sprintf(cmdline, "iptables -t nat -A POSTROUTING -o %s -j MASQUERADE", netname);
    APPL_TRACE_EVENT1("%s", cmdline);
    system(cmdline);

    /* make sure ip forwarding is enabled */
    sprintf(cmdline, "echo 1 > /proc/sys/net/ipv4/ip_forward");
    APPL_TRACE_EVENT1("%s", cmdline);
    system(cmdline);
}

/*******************************************************************************
**
** Function         find_active_net_interface
**
** Description     Automatically discover active network interface
**
** Returns          void
*******************************************************************************/

int ifc_ctl_sock;

BOOLEAN net_iface_active(const char *name)
{
    unsigned int addr, mask, flags;
    struct ifreq ifr;

    memset((void*)&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, name, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ - 1] = 0;

    if (ioctl(ifc_ctl_sock, SIOCGIFADDR, &ifr) < 0)
    {
        addr = 0;
    }
    else
    {
        addr = ((struct sockaddr_in*) &ifr.ifr_addr)->sin_addr.s_addr;
    }

    if (ioctl(ifc_ctl_sock, SIOCGIFFLAGS, &ifr) < 0)
    {
        flags = 0;
    }
    else
    {
        flags = ifr.ifr_flags;
    }

    //APPL_TRACE_EVENT2("interface %s [%s]", name, flags&1 ? "UP":"DOWN");

    if (flags & 1)
        return TRUE;

    return FALSE;
}


BOOLEAN get_active_interface(char *netname)
{
    DIR *d;
    struct dirent *de;
    BOOLEAN retval = FALSE;

    ifc_ctl_sock = socket(AF_INET, SOCK_DGRAM, 0);

    d = opendir("/sys/class/net");
    if (d == 0) return -1;

    while ((de = readdir(d)))
    {

        if (de->d_name[0] == '.') continue;

        /* skip local interface */
        if (strncmp(de->d_name, "lo", 2)==0) continue;

        /* assume only one active interface */

        /* use property to specify name of desired network interface ? */

        if (net_iface_active(de->d_name) == TRUE)
        {
            sprintf(netname, "%s", de->d_name);
            APPL_TRACE_EVENT1("Found active network interface [%s]", netname);
            retval = TRUE;
            break;
        }
    }

    closedir(d);
    if (ifc_ctl_sock > 0) {
        close(ifc_ctl_sock);
    }
    return retval;
}


/*******************************************************************************
**
** Function         pppd_main
**
** Description     Configures one pppd server
**
** Returns          void
*******************************************************************************/

void pppd_main(signed long dp)
{
    char btlif_socket[32];
    char netname[100];
    char ip_addresses[100];
    char ms_dns[40];
    char gw[40];
    int result;
    t_DUNPORT *myport;
    int i=0;

    if (dp >= DUN_MAX_PORTS)
    {
        APPL_TRACE_ERROR1("invalid dun port %d", dp);
        return;
    }

    myport = &dport_cb[dp];

    if(!BTPORT_IsEnabled())
    {
        /* btl if socket for DUN */
    #ifdef DTUN_LOCAL_SERVER_ADDR
        sprintf(btlif_socket, DTUN_LOCAL_SERVER_ADDR ".%d", myport->pppd_port);
    #else
        sprintf(btlif_socket, "127.0.0.1:%d", myport->pppd_port);
    #endif
    }

    /* create remote ip address from 172.16.0.xx subnet */
    sprintf(ip_addresses, "172.16.0.1:172.16.0.%d", myport->dun_port+REMOTE_PEER_IP_ADDR_OFFSET);

#ifndef LINUX_NATIVE

    if (get_active_interface(netname) == TRUE)
    {
        /* setup iptables for nat */
        nat_setup(netname);
    }
    else
    {
        /* no active network interface found, skip iptables */
        LOGI("No active network interface found, skip nat setup");
    }

    property_get("net.dns1", ms_dns, "0.0.0.0");

#else
    sprintf(netname, "ppp0");
    sprintf(ms_dns, "130.244.127.161");
#endif

    pppd_options[i++] = PPPDCMD;

    if(!BTPORT_IsEnabled())
    {
        pppd_options[i++] = "socket";
        pppd_options[i++] = btlif_socket;
    }
    else
    {
        pppd_options[i++] = TTY_NAME_DUN;
        pppd_options[i++] = "115200"; /* dummy value */
        pppd_options[i++] = "crtscts";
        pppd_options[i++] = "local";
    }
    pppd_options[i++] = ip_addresses;
    pppd_options[i++] = "debug";
    pppd_options[i++] = "noauth";
    pppd_options[i++] = "nopersist";
    pppd_options[i++] = "nodetach";    /* needed to be able to kill pppd process upon DG exit    */
    //pppd_options[i++] = "silent";    /* pts requires server to initiate LCP negotiation */
    pppd_options[i++] = "passive";
    pppd_options[i++] = "proxyarp";    /* 'send all packets to me' */
    pppd_options[i++] = "ktune";       /* enables ip_forwarding */
    pppd_options[i++] = "ms-dns";
    pppd_options[i++] = ms_dns;
    pppd_options[i++] = NULL;

    LOGI("Starting pppd on dun port %d (%s)", myport->dun_port, ip_addresses);

#if 1
    {
        /* print pppd_options */
        i = 0;
        while (pppd_options[i])
        {
            APPL_TRACE_DEBUG1("%s", pppd_options[i]);
            i++;
        }
    }
#endif

    result =  execvp(PPPDCMD, pppd_options);

    if (result < 0)
    {
        error("exiting pppd (%s)", strerror(errno));
    }
}

/*******************************************************************************
**
** Function         start_pppd
**
** Description     Starts PPP server
**
** Returns          void
*******************************************************************************/

int start_pppd(t_DP_HANDLE dp)
{
    pid_t pid;

    LOGI("### start_pppd : dun port %d ###", dp);

    if (dport_cb[dp].pppd_pid != PPPD_THREAD_ID_INVALID)
    {
        LOGI("warning : pppd already started (pid %d)", dport_cb[dp].pppd_pid);
        return -1;
    }

    /* now fork off a pppd process */
    pid = fork();

    if (pid < 0)
    {
        perror("from fork()");
        return -1;
    }
    else if (pid == 0)
    {
        /* now start actual pppd */
        pppd_main(dp);
        _exit(0);
    }
    else
    {
        /* parent side, store pid and continue */
        dport_cb[dp].pppd_pid = pid;
        APPL_TRACE_EVENT1("pppd pid %d", pid);
    }

    return pid;
}

/*******************************************************************************
**
** Function         stop_pppd
**
** Description     Stop PPP server
**
** Returns          void
*******************************************************************************/

void stop_pppd(t_DP_HANDLE dp)
{
    if (dport_cb[dp].pppd_pid>0)
    {
        int result;
        int exit_status;

        LOGI("### stop_pppd : dun port %d (pid %d) ###", dp, dport_cb[dp].pppd_pid);

        /* make sure pppd process is terminated */
        result = kill(dport_cb[dp].pppd_pid, SIGTERM);

        if (result < 0)
            LOGI("kill failed with reason %s", strerror(errno));

        if (waitpid(dport_cb[dp].pppd_pid, &exit_status, WNOHANG) < 0)
            error("waitpid pppd %s", strerror(errno));

        dport_cb[dp].pppd_pid = PPPD_THREAD_ID_INVALID;
        LOGI("terminated pppd on dun port %d... exit status %d", dp, exit_status);
    }
}

/*******************************************************************************
**
** Function         btapp_dg_dun_rx_data
**
** Description
**
** Returns          void
*******************************************************************************/

void btapp_dg_dun_rx_data(int appid, BT_HDR *p_buf)
{
    int dp = APP_ID_TO_DUNPORT(appid);
    char *p;
    int len;

    p = (UINT8 *)(p_buf + 1) + p_buf->offset;
    len = p_buf->len;

    APPL_TRACE_DEBUG2("bt --> dun : len %d, appid %d [bt]", len, appid);

    /* this code should only be enabled when testing without RIL modem */
#if (FALSE == BTAPP_DG_EMULATOR)
    if(!BTPORT_IsEnabled())
#endif
    {
        /* only check for AT commands if pppd thread is not yet started */
        if (dport_cb[dp].pppd_pid == PPPD_THREAD_ID_INVALID)
        {
            if (fake_modem(dp, p, len) == START_PPP)
            {
                start_pppd(dp);

                /* give some time for pppd to start up */
                //sleep(1);
            }
            return;
        }
    }

    if (dport_cb[dp].datahdl == DATA_SOCKET_INVALID)
    {
        /* datasocket not yet up, copy & enqueue data, this buffer is freed by caller */
        // not verified yet copy_to_rx_q(dp, p, len);
        return;
    }

    hex_dump("bt --> dun", p, len, 0);

    /* forward data to ppp server */
    BTL_IF_SendData(dport_cb[dp].datahdl, p, len);
}

/*******************************************************************************
**
** Function         btapp_dg_dun_tx_dequeued
**
** Description
**
** Returns          void
*******************************************************************************/

void btapp_dg_dun_tx_dequeued(int appid, BT_HDR *p_buf)
{
    int dp = APP_ID_TO_DUNPORT(appid);
    buf_flow_dec(dp);
}


/*******************************************************************************
**
** Function         btapp_dg_init
**
** Description     Initialises DG server
**
** Returns          void
*******************************************************************************/

static void btapp_dg_dun_init(void)
{
    APPL_TRACE_DEBUG0("btapp_dg_dun_init");
    btui_cfg.dg_included = TRUE;
    int i = 0;

    //btapp_dg_dun_init called after enable, so setting default
    //security setting here won't work.
    //btui_cfg.dundg_security = BTA_SEC_AUTHORIZE;

    //BTA_DgEnable(bta_dg_dun_cback);

    dun_init();

    for ( i = BTUI_DG_DUN_APPID_BEGIN; i < (BTUI_DG_DUN_APPID_END + 1); i++)
    {
        btui_dg_cb.app_cb[i].service_id = BTA_DUN_SERVICE_ID;
    }
}

/*******************************************************************************
**
** Function         btapp_dg_disable
**
** Description      Disable DG server
**
** Returns          void
*******************************************************************************/
void btapp_dg_disable(void)
{
    BTA_DgDisable();
}


/*******************************************************************************
**
** Function         process_dg_enable_evt
**
** Description
**
** Returns          void
*******************************************************************************/


static void process_dg_enable_evt(tBTA_DG *p_data)
{
    tBTUI_DG_APP_CB *p_cb = NULL;
    tBTA_UTL_COD cod;

    btui_cfg.dg_enabled = TRUE;

    APPL_TRACE_EVENT0("DG-DUN [BTA_DG_ENABLE_EVT]");

    /* Register in BTL IF for DUN datapath */
    BTL_IF_RegisterSubSystem(&btapp_dun_ctrl, SUB_DUN, btlif_dun_ppp_data_cb, btlif_dun_ctrl_cb);

    /* Register BTL IF for SPP datapath */
    BTL_IF_RegisterSubSystem(&btapp_spp_ctrl, SUB_SPP, btlif_dg_spp_on_rx_data, btlif_dg_spp_on_rx_ctrl);

    /* set class of device */
    cod.service = COD_SERVICE_TELEPHONY|COD_SERVICE_NETWORKING;
    utl_set_device_class(&cod, BTA_UTL_SET_COD_SERVICE_CLASS);

#ifdef LINUX_NATIVE
    bta_dg_dun_enable_ports();
#endif

}

/*******************************************************************************
**
** Function         process_dg_listen_evt
**
** Description
**
** Returns          void
*******************************************************************************/

static void process_dg_listen_evt(tBTA_DG *p_data)
{
    tBTUI_DG_APP_CB *p_cb = NULL;
    tBTL_PARAMS params;
    t_DP_HANDLE dp;

    p_cb = &btui_dg_cb.app_cb[p_data->listen.app_id];
    switch (p_cb->service_id)
    {
        case BTA_SPP_SERVICE_ID:
            APPL_TRACE_EVENT2("DG-SPP [BTA_DG_LISTEN_EVT] server handle %d on port %d", p_data->listen.handle, p_data->listen.app_id);

            /* store server handle */
            p_cb->listen_handle = p_data->listen.handle;

            /* if redirection enabled, don't notify JNI if port is the reserved id */
            if (spp_is_reserved_app_id(p_data->listen.app_id))
                return;

            /* Forward to btl-if client so that the port
             * status will be changed to enabled for server port
             */
            params.dg_listening_param.app_id = p_data->listen.app_id;
            BTL_IF_CtrlSend(btapp_spp_ctrl,
                           SUB_SPP,
                           BTLIF_DG_LISTEN_EVT,
                           &params,
                           sizeof(tBTLIF_DG_LISTENING_PARAM));
         break;

       case BTA_DUN_SERVICE_ID:

           dp  = APP_ID_TO_DUNPORT(p_data->listen.app_id);
           dport_cb[dp].server_hdl = p_data->listen.handle;

           APPL_TRACE_EVENT3("DG-DUN [BTA_DG_LISTEN_EVT] dun_port %d, app_id %d, server handle %d", dp, p_data->listen.app_id, p_data->listen.handle);

           /* don't send btl if listen events if btport redirection is enabled */
           if (!BTPORT_IsEnabled())
           {
               params.dg_listening_param.app_id = p_data->listen.app_id;
               BTL_IF_CtrlSend(btapp_dun_ctrl, SUB_DUN, BTLIF_DG_LISTEN_EVT, &params, sizeof(tBTLIF_DG_LISTENING_PARAM));
           }

           /* always send ENABLE evt to notify advanced settings UI */
           if (bta_dg_dun_ports_have_state(1))
           {
                BTL_IF_CtrlSend(btapp_dun_ctrl, SUB_DUN, BTLIF_DG_ENABLE_EVT, NULL, 0);
           }
           break;

       case BTA_FAX_SERVICE_ID:
           break;

       default:
           break;
    }
}

/*******************************************************************************
**
** Function         process_dg_opening_evt
**
** Description
**
** Returns          void
*******************************************************************************/

static void process_dg_opening_evt(tBTA_DG *p_data)
{
    tBTUI_DG_APP_CB *p_cb = NULL;
    tBTL_PARAMS params;

    p_cb = &btui_dg_cb.app_cb[p_data->opening.app_id];

    APPL_TRACE_EVENT3("DG [BTA_DG_OPENING_EVT] service %d, hdl %d, app_id %d",
                      p_cb->service_id,
                      p_data->opening.handle,
                      p_data->opening.app_id);

    switch (p_cb->service_id)
    {
        case BTA_SPP_SERVICE_ID:


            params.dg_opening_param.app_id = p_data->opening.app_id;
            BTL_IF_CtrlSend(btapp_spp_ctrl,
                            SUB_SPP,
                            BTLIF_DG_OPENING_EVT,
                            &params,
                            sizeof(tBTLIF_DG_OPENING_PARAM));

            break;

        case BTA_DUN_SERVICE_ID:
             break;

        case BTA_FAX_SERVICE_ID:
            break;

        default:
            break;
    }
}
/*******************************************************************************
**
** Function         process_dg_open_evt
**
** Description
**
** Returns          void
*******************************************************************************/

static void process_dg_open_evt(tBTA_DG *p_data)
{
    tBTUI_DG_APP_CB *p_cb = NULL;
    tBTL_PARAMS params;
    t_DP_HANDLE dp;

    p_cb = &btui_dg_cb.app_cb[p_data->open.app_id];

    params.dg_open_param.app_id = p_data->open.app_id;
    memcpy(params.dg_open_param.bd_addr, p_data->open.bd_addr, BD_ADDR_LEN);

    switch (p_cb->service_id)
    {
       case BTA_SPP_SERVICE_ID:

           LOGD("DG-SPP [BTA_DG_OPEN_EVT] :  port %d, handle %d", p_data->open.app_id, p_data->open.handle);

            /* check for reserved app id in case redirection mode is enabled */
            if (spp_is_reserved_app_id(p_data->open.app_id))
            {
                /* start redirection port, also notify OPEN event for redirection mode */
                BTPORT_StartRedirection(btapp_spp_ctrl, BTA_SPP_SERVICE_ID, p_data->open.app_id, p_cb->port_handle);

                /* make sure JNI doesn't try to setup btl if data connection */
                params.dg_open_param.notify_only = 1;
            }
            else
            {
                /* setup data listener  */
                BTL_IF_SetupListener(btapp_spp_ctrl, SUB_SPP, p_data->open.app_id);

                params.dg_open_param.notify_only = 0;
            }

            /* Now forward event to btl-if client so that java application will be informed */
            BTL_IF_CtrlSend(btapp_spp_ctrl,
                            SUB_SPP,
                            BTLIF_DG_OPEN_EVT,
                            &params,
                            sizeof(tBTLIF_DG_OPEN_PARAM));
            break;

        case BTA_DUN_SERVICE_ID:
            p_cb = &btui_dg_cb.app_cb[p_data->open.app_id];

            dp = APP_ID_TO_DUNPORT(p_data->open.app_id);

            dun_nbr_open_ports++;

            /* log for when btld traces are off */
            LOGI("DUN client connected [%s] (servers in use %d out of %d)", bd2str(p_data->open.bd_addr),
                 dun_nbr_open_ports, DUN_MAX_PORTS);

            APPL_TRACE_EVENT6("DG-DUN [BTA_DG_OPEN_EVT] : remote bd %s, hdl %d, svc %d, app_id %d, port handle %d, nbr open ports %d",
                              bd2str(p_data->open.bd_addr), p_data->open.handle,
                              p_data->open.service, p_data->open.app_id, p_cb->port_handle, dun_nbr_open_ports);

            /* setup max mtu size for data socket */
            dport_cb[dp].pppd_mtu = p_cb->peer_mtu;

            /* set up mapping between pppd and dun port */
            dport_cb[dp].pppd_port = wrp_getport(SUB_DUN, dp);
            dport_cb[dp].dun_port = dp;

            if(BTPORT_IsEnabled())
            {
                /* start redirection port instead of data listener, also notify jni */
                BTPORT_StartRedirection(btapp_spp_ctrl, BTA_DUN_SERVICE_ID, p_data->open.app_id, p_cb->port_handle);

                params.dg_open_param.notify_only = 1;
            }
            else
            {
                /* setup data listener  */
                BTL_IF_SetupListener(btapp_dun_ctrl, SUB_DUN, dp);

                params.dg_open_param.notify_only = 0;
            }

            BTL_IF_CtrlSend(btapp_dun_ctrl,
                            SUB_DUN,
                            BTLIF_DG_OPEN_EVT,
                            &params,
                            sizeof(tBTLIF_DG_OPEN_PARAM));

#if( defined DUN_USE_BTPORT ) && (DUN_USE_BTPORT == TRUE)
            /* as it is the first time, send modem signals if not off! */
            if ( p_cb->modem_ctrl )
            {
                btapp_dg_btlif_send_modem_ctrl( p_data->open.app_id, p_cb->modem_ctrl, p_cb->modem_ctrl );
            }
#endif
            break;

        case BTA_FAX_SERVICE_ID:
            break;

        default:
            break;
    }


}

/*******************************************************************************
**
** Function         process_dg_close_evt
**
** Description
**
** Returns          void
*******************************************************************************/

static void process_dg_close_evt(tBTA_DG *p_data)
{
    tBTUI_DG_APP_CB *p_cb = NULL;
    tBTL_PARAMS params;
    t_DP_HANDLE dp;

    p_cb = &btui_dg_cb.app_cb[p_data->close.app_id];

    switch (p_cb->service_id)
    {
        case BTA_SPP_SERVICE_ID:

            LOGD("DG-SPP [BTA_DG_CLOSE_EVT] : app id %d", p_data->close.app_id);

            spp_close_connection(p_cb, p_data->close.app_id);
            break;

        case BTA_DUN_SERVICE_ID:
            dp  = APP_ID_TO_DUNPORT(p_data->close.app_id);

            APPL_TRACE_EVENT3("DG-DUN [BTA_DG_CLOSE_EVT] : hdl %d, app_id %d, dp %d",
                               p_data->close.handle,
                               p_data->close.app_id, dp);

            dun_close_connection(dp);

            dun_nbr_open_ports--;

            /* Forward to BTL_IF client and the java application will be informed the event */
            params.dg_close_param.app_id = p_data->close.app_id;
            params.dg_close_param.notify_only = BTPORT_IsEnabled();

            BTL_IF_CtrlSend(btapp_dun_ctrl, SUB_DUN, BTLIF_DG_CLOSE_EVT, &params, sizeof(tBTLIF_DG_CLOSE_PARAM));
            break;

        case BTA_FAX_SERVICE_ID:
            break;

        default:
            APPL_TRACE_ERROR1("Invalid service id %d", p_cb->service_id);
            break;
    }
}

/*******************************************************************************
**
** Function         bta_dg_cback
**
** Description      Callback from BTA DG
**
** Returns          void
*******************************************************************************/

static void bta_dg_cback(tBTA_DG_EVT event, tBTA_DG *p_data)
{
    switch (event)
    {
        case BTA_DG_ENABLE_EVT:
            process_dg_enable_evt(p_data);
            break;

        case BTA_DG_LISTEN_EVT:
            process_dg_listen_evt(p_data);
            break;

        case BTA_DG_OPENING_EVT:
            process_dg_opening_evt(p_data);
            break;

        case BTA_DG_OPEN_EVT:
            process_dg_open_evt(p_data);
            break;

        case BTA_DG_CLOSE_EVT:
            process_dg_close_evt(p_data);
            break;

        default:
            APPL_TRACE_ERROR1("### DG unknown event:%d ###", event);
            break;
    }
}

/*******************************************************************************
**
** Function         btapp_dg_spp_tx_dequeued
**
** Description
**
** Returns          void
*******************************************************************************/

void btapp_dg_spp_tx_dequeued(int app_id, BT_HDR *p_buf, int qcount)
{
    tBTUI_DG_APP_CB *p_cb = &btui_dg_cb.app_cb[app_id];

    if(qcount <= SPP_BUF_WATERMARK_FLOW_ON)
    {
        APPL_TRACE_DEBUG2("%s  turn on the receiving socket %d",__FUNCTION__, p_cb->data_handle);
        BTL_IF_SetupRxFlow(p_cb->data_handle, 1);
    }
}


/*******************************************************************************
**
** Function         spp_setup_rx_buffer
**
** Description     The function set up the rx buffer for SPP
**
** Returns         none
**
*******************************************************************************/

// FIXME -- align with DUN function

static BOOLEAN spp_setup_rx_buffer(int app_id)
{
    BT_HDR          *p_buf;
    int buf_sz;
    char *p_tmp;
    tBTUI_DG_APP_CB *p_cb = &btui_dg_cb.app_cb[app_id];

    if ((p_buf = (BT_HDR *) GKI_getpoolbuf(RFCOMM_DATA_POOL_ID)) != NULL)
    {
        buf_sz = RFCOMM_DATA_POOL_BUF_SIZE; /* the same but more efficient then GKI_get_buf_size(p_buf); */
        p_buf->len = 0;
        p_buf->offset = L2CAP_MIN_OFFSET + RFCOMM_MIN_OFFSET;
        p_tmp = (UINT8 *)(p_buf + 1) + p_buf->offset;

        APPL_TRACE_DEBUG4("spp_setup_rx_buffer : max %d, offset %d, len %d, mtu %d",
                           buf_sz, p_buf->offset, p_buf->len, p_cb->peer_mtu);

        buf_sz -= (BT_HDR_SIZE + p_buf->offset);

        /* make sure we have room for mtu size */
        if ( buf_sz < p_cb->peer_mtu)
        {
            /* if we end up here there is a configuration issue which should be tuned, for now discard data and log an error */
            APPL_TRACE_ERROR0("error : gki buffer smaller than peer mtu");

            GKI_freebuf(p_buf);
            return FALSE;
        }

        /* setup receive buffer to match the rfcomm links mtu size */
        if (BTL_IF_SetupRxBuf(p_cb->data_handle, p_tmp, p_cb->peer_mtu) != BTL_IF_SUCCESS)
        {
            APPL_TRACE_DEBUG1("%s: setup rx buffer failed", __FUNCTION__);
            GKI_freebuf(p_buf);
            return FALSE;
        }

        return TRUE;

    }
    else
    {
        APPL_TRACE_ERROR1("%s: Out of buffers", __FUNCTION__);
        return FALSE;
    }
}


/*******************************************************************************
**
** Function         btapp_dg_spp_on_rx_ctrl
**
** Description      The function handle control message received on SUB_SPP
**
** Returns         none
**
*******************************************************************************/

static void btlif_dg_spp_on_rx_ctrl(tCTRL_HANDLE fd, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params)
{
    UINT16 port_handle;

    LOGI("btlif_dg_spp_on_rx_ctrl called with event %s", dump_msg_id(id));

    switch (id)
    {
        case BTLIF_DG_ENABLE:
            //APPL_TRACE_DEBUG1("%s No Need to call BTA_DgEnable()", __FUNCTION__);
            break;

        case BTLIF_DG_LISTEN:

            /* check if port id is valid, skip reserved port or if out of bounds */
            if (spp_is_reserved_app_id(params->dg_listen_param.app_id))
                return;

            APPL_TRACE_DEBUG3("%s BTLIF_DG_LISTEN %s %d",
                               __FUNCTION__,
                               params->dg_listen_param.service_name,
                               params->dg_listen_param.app_id);

            /*Since we are rx from SPP socket descriptor, it means the service is SPP */
            BTA_DgListen(BTA_SPP_SERVICE_ID,
                         DG_SPP_DEFAULT_SECURITY,
                         params->dg_listen_param.service_name,
                         params->dg_listen_param.app_id);
            break;

        case BTLIF_DG_OPEN:

            /* check if port id is valid, skip reserved port or if out of bounds */
            if (spp_is_reserved_app_id(params->dg_open_param.app_id))
                return;

            APPL_TRACE_DEBUG1("%s Rx BTLIF_DG_OPEN", __FUNCTION__);

            /*Since we are rx from SPP socket descriptor, it means the service is SPP */
            BTA_DgOpen(params->dg_open_param.bd_addr,
                       BTA_SPP_SERVICE_ID,
                       params->dg_open_param.sec_mask,
                       params->dg_open_param.service_name,
                       params->dg_open_param.app_id);
            break;

        case BTLIF_DG_CLOSE:

            /* check if port id is valid, skip reserved port or if out of bounds */
            if (spp_is_reserved_app_id(params->dg_close_param.app_id))
                return;

            /* We never expose the handle to Java,
             * so we must find handle from btui_cb
             * through app_id, which are the same
             * for java and bta
             */

            port_handle = btui_dg_cb.app_cb[params->dg_close_param.app_id].port_handle;

            APPL_TRACE_DEBUG3("%s Rx BTLIF_DG_CLOSE app_id = %d port_handle = %d",
                              __FUNCTION__,
                              params->dg_close_param.app_id,
                              port_handle);

            BTA_DgClose(port_handle);
            break;

        case BTLIF_DG_SHUTDOWN:

            /* check if port id is valid, skip reserved port or if out of bounds */
            if (spp_is_reserved_app_id(params->dg_shutdown_param.app_id))
                return;

            /* Fix me, there could be two handles, one for
             * listen and the other for connection use the
             * handle for listen to shutdown
             */
            port_handle = btui_dg_cb.app_cb[params->dg_shutdown_param.app_id].listen_handle;

            APPL_TRACE_DEBUG3("%s Rx BTLIF_DG_SHUTDOWN app_id = %d port_handle = %d",
                              __FUNCTION__,
                              params->dg_close_param.app_id,
                              port_handle);

            BTA_DgShutdown(port_handle);
            break;

        case BTLIF_DATA_CHAN_IND:

            APPL_TRACE_DEBUG2("Received Data Channel Connected on Port %d Handle is %d", params->chan_ind.subport, params->chan_ind.handle);

            /* store btl if data handle for this control block*/
            btui_dg_cb.app_cb[params->chan_ind.subport].data_handle = params->chan_ind.handle;

            /* Set up the receive buffer so that data copy can be avoided  (subport = app_id)  */
            spp_setup_rx_buffer(params->chan_ind.subport);
            break;

        case BTLIF_DATA_CHAN_DISC_IND:

            APPL_TRACE_DEBUG2("Received Data Channel Disconnected on Port %d Handle is %d", params->chan_ind.subport, params->chan_ind.handle);

            /* Java application close the connection on data path, so the btld should do some clean up */
            BTL_IF_DisconnectDatapath(params->chan_ind.handle);

            /* invalidate data handle to avoid accidental match if a app_id is used with the same
             * socket handle */
            btui_dg_cb.app_cb[params->chan_ind.subport].data_handle = DATA_SOCKET_INVALID;
            break;

        case BTLIF_DISC_RX_BUF_PENDING:
            {
                /* make sure we free the rx buffer not yet delivered back */
                BT_HDR *p_buf;

                info("free rx buf (0x%x)", params->rx_buf_pnd.rx_buf);

                p_buf = (BT_HDR *)((UINT8 *) params->rx_buf_pnd.rx_buf - BT_HDR_SIZE - L2CAP_MIN_OFFSET - RFCOMM_MIN_OFFSET);
                GKI_freebuf(p_buf);
            }
            break;

        default:
            break;
    }

}

/*******************************************************************************
**
** Function         btapp_dg_spp_tx_data
**
** Description
**
**
** Returns          void
*******************************************************************************/

void btapp_dg_spp_tx_data(UINT8 appid, BT_HDR *p_buf)
{
    char *p;
    int len;

    static int spp_tx_ed = 0;

    p = (UINT8 *)(p_buf + 1) + p_buf->offset;
    len = p_buf->len;

    APPL_TRACE_DEBUG3(" bt --> spp : len %d, appid %d %d [bt]", len, appid, btui_dg_cb.app_cb[appid].data_handle);

    if(btui_dg_cb.app_cb[appid].data_handle == DATA_SOCKET_INVALID)
    {
        APPL_TRACE_DEBUG1("%s data socket invalid", __FUNCTION__);
        /* discard data, buffer freed by caller */
        return;
    }

    //hex_dump("bt --> spp", p, len, 0);
    BTL_IF_SendData(btui_dg_cb.app_cb[appid].data_handle, p, len);

    /* p_buf will be freed in bta_dg_co_tx_path where this function is called */
}

/*******************************************************************************
**
** Function         btapp_dg_spp_on_rx_data
**
** Description     The function handle data received on SUB_SPP
**
** Parameter
**
** Returns         none
**
***************************** **************************************************/

static void btlif_dg_spp_on_rx_data(tDATA_HANDLE handle, char *p, int len)
{
    tBTUI_DG_APP_CB *p_cb;
    BT_HDR          *p_buf;
    int             status, buffer_size = 0;
    int app_id;

    /* find application id for this handle */
    for (app_id = 0; app_id < BTUI_DG_SPP_NUM_SERVICES; app_id++) {
        p_cb = &btui_dg_cb.app_cb[app_id];
        if (p_cb->data_handle == handle ) {
            APPL_TRACE_DEBUG3(" %s %d bytes data received on port %d", __FUNCTION__, len, app_id);
            break;
        }
    }


    p_buf = (BT_HDR *)((UINT8 *) p - BT_HDR_SIZE - L2CAP_MIN_OFFSET - RFCOMM_MIN_OFFSET);
    p_buf->len = len;
    p_buf->event = 0;
    p_buf->layer_specific = 0;

    /* notify new data */
    if (p_buf->len > 0)
    {
        APPL_TRACE_DEBUG3("bt <-- spp %d bytes on app id %d [spp] port handle is %d ", len, app_id, p_cb->port_handle);

        //hex_dump("bt <-- dun", p, len, 0);

        GKI_enqueue(&p_cb->data_q, p_buf);

        if(p_cb->data_q.count >= SPP_BUF_WATERMARK_FLOW_OFF)
        {
            APPL_TRACE_DEBUG0("Turn the receiving socket off");
            BTL_IF_SetupRxFlow(handle, 0);
        }

        bta_dg_ci_rx_ready(p_cb->port_handle);
    }
    else
    {
        GKI_freebuf(p_buf);
        return;
    }

    spp_setup_rx_buffer(app_id);

}

/*******************************************************************************
**
** Function         btapp_dg_spp_init
**
** Description      The function register SUB_SPP with control if server
**
** Returns         none
**
*******************************************************************************/

static void btapp_dg_spp_init(void)
{
    int i = 0;
    for ( i = BTUI_DG_SPP_APPID_BEGIN; i < (BTUI_DG_SPP_APPID_END + 1); i++)
    {
        btui_dg_cb.app_cb[i].service_id = BTA_SPP_SERVICE_ID;
        btui_dg_cb.app_cb[i].data_handle = DATA_SOCKET_INVALID;
    }
}

/*******************************************************************************
**
** Function        btapp_dg_init
**
** Description     The function register SUB_DUN and SUN_SPP with control if server
**
** Returns         none
**
** Note            Should we add a parameter so that customer can configure it?
**                 Request comment
**
*******************************************************************************/

void btapp_dg_init(void)
{
    memset((void*)&btui_dg_cb, 0, sizeof(tBTUI_DG_CB));

    /*
     * Changed initialization order so that we check
     * redirection flag before bring up DG. Based on
     * timing we can have DG enabled before we check
     * redirection flag - this causes redirection SPP
     * port not to be created.
     */
    btapp_dg_spp_init();
    btapp_dg_dun_init();

    BTA_DgEnable(bta_dg_cback);
}

#endif //#if( defined DUN_INCLUDED ) && (DUN_INCLUDED == TRUE)
