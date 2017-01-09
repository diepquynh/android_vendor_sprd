/*****************************************************************************
**
**  Name:           bta_dg_co.c
**
**  Description:    This file contains the data gateway callout function
**                  implementation for Insight.
**
**  Copyright (c) 2010, Broadcom Corp., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <termios.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

#include "bta_api.h"
#include "bta_dg_api.h"
#include "bta_dg_ci.h"
#include "bta_dg_co.h"
#include "port_api.h"
#include "userial.h"
#include "gki.h"

#include "btui.h"
#include "btui_int.h"
#include "btapp_dm.h"
#include "btapp_dg.h"
#include "dtun_api.h"
#include "btl_ifs.h"

#if( defined DUN_USE_SMD_INTERFACE ) && (DUN_USE_SMD_INTERFACE == TRUE)
#include <sys/ioctl.h>
#include <semaphore.h>  /* added for sem_wait, sem_init, and sem_port  */
#endif


#if (defined LOG_TAG)
#undef LOG_TAG
#endif
#define LOG_TAG "BTA_DG_CO:"

#ifndef LINUX_NATIVE
#include <cutils/log.h>
#else
#define LOGI(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGE(format, ...)  fprintf (stderr, LOG_TAG format"\n", ## __VA_ARGS__)
#endif


#define F_SETSIG 10

#define BTLINUX_PORT_OPENED         0
#define BTLINUX_PORT_CLOSED         1
#define BTLINUX_PORT_ENABLE_MS      2
#define BTLINUX_PORT_SET_BREAK_ON   3
#define BTLINUX_PORT_SET_BREAK_OFF  4
#define BTLINUX_PORT_MODEM_CONTROL  5
#define BTLINUX_PORT_MODEM_STATUS   6
#define BTLINUX_PORT_TX_EMPTY       7

#define MODEM_CNTRL_DTRDSR_MASK        0x0001
#define MODEM_CNTRL_CTSRTS_MASK        0x0002
#define MODEM_CNTRL_RNG_MASK           0x0004
#define MODEM_CNTRL_CAR_MASK           0x0008

/* ioctl defs */
#define IOCTL_BTPORT_GET_EVENT            0x1001
#define IOCTL_BTPORT_SET_EVENT_RESULT     0x1002
#define IOCTL_BTPORT_HANDLE_MCTRL         0x1003

/* data flow watermark levels per dun server */
#define SMD_DUN_BUF_WATERMARK_FLOW_ON   10
#define SMD_DUN_BUF_WATERMARK_FLOW_OFF  6
#define FLOW_ON   1
#define FLOW_OFF  2

/* assume only one DUN connection will be active when redirection is enabled */
#define DG_DUN_HANDLE_INVALID (0xffff)


#if( defined DUN_USE_SMD_INTERFACE ) && (DUN_USE_SMD_INTERFACE == TRUE)

static pthread_t p_th;
static int       smdport_fd = -1;
static char      smd_read_buf[L2CAP_MTU_SIZE];
static int       flow_state = FLOW_OFF;
static sem_t     dun_high_water_mark_sem;

/* To parse CONNECT result code in dialing phase */
static UINT8 bt_dun_parse_connect = FALSE;
static int prev_smd_status = 0;

#endif

typedef struct _BTLINUXPORTEvent
{
    int eventCode;
    union {
       short modemControlReg;
    } u;
} BTLINUXPORTEvent, *pBTLINUXPORTEvent;

int btport_redirection_enable = 0;

UINT16 dg_dun_handle    = DG_DUN_HANDLE_INVALID;
UINT16 dg_spp_handle    = DG_DUN_HANDLE_INVALID;
int    dg_dun_dialup_fd = DG_DUN_HANDLE_INVALID;
int    dg_spp_dialup_fd = DG_DUN_HANDLE_INVALID;
int    dg_dun_app_id    = DG_DUN_HANDLE_INVALID;
int    dg_spp_app_id    = DG_DUN_HANDLE_INVALID;


UINT16 get_dg_handle(int btport_fd)
{
    if (btport_fd == dg_dun_dialup_fd)
        return dg_dun_handle;
    else if (btport_fd == dg_spp_dialup_fd)
        return dg_spp_handle;
    else
        return DG_DUN_HANDLE_INVALID;
}


UINT8 get_dg_app_id(int btport_fd)
{
    if (btport_fd == dg_dun_dialup_fd)
        return (UINT8) dg_dun_app_id;
    else if (btport_fd == dg_spp_dialup_fd)
        return (UINT8) dg_spp_app_id;
    else
        return (UINT8) DG_DUN_HANDLE_INVALID;
}


UINT16 get_btport_fd_by_app_id(int app_id)
{
    if (app_id == dg_dun_app_id)
        return dg_dun_dialup_fd;
    else if (app_id == dg_spp_app_id)
        return dg_spp_dialup_fd;
    else
        return DG_DUN_HANDLE_INVALID;
}


void sig_port_status_handler (int sig, siginfo_t *info, void *context)
{
    BTLINUXPORTEvent getevent;
    int result[2];
    int dialup_fd = info->si_fd;
    int dg_handle;
    int res;
    int app_id;
    tBTUI_DG_APP_CB *p_cb;
    UINT8 signal;

    /* search for dg handle for this */
    dg_handle = get_dg_handle(dialup_fd);

    APPL_TRACE_DEBUG5("sig_port_status_handler -- signo %d errno %d code %d fd %d band %d\n",
            info->si_signo, info->si_errno, info->si_code, info->si_fd, info->si_band);

    /* fetch kernel driver event */
    ioctl(dialup_fd, IOCTL_BTPORT_GET_EVENT, &getevent);

    switch (getevent.eventCode)
    {
        case BTLINUX_PORT_MODEM_CONTROL:
            APPL_TRACE_DEBUG1("[KERNEL [DCE] --> USER[DTE] [BTLINUX_PORT_MODEM_CONTROL] control %x\n",
                           getevent.u.modemControlReg);

            /* use BTA_DG_DTRDSR_ON macro instead of rfcomm directly as this is the API being
             * used! btport should also use BTA value instead */
            if (getevent.u.modemControlReg & BTA_DG_DTRDSR_ON) {
                APPL_TRACE_EVENT0("[DCE] DSR SET");
            } else {
                APPL_TRACE_EVENT0("[DCE] DSR     CLR");
            }

            if (getevent.u.modemControlReg & BTA_DG_RTSCTS_ON) {
                APPL_TRACE_EVENT0("[DCE] CTS SET");
            } else {
                APPL_TRACE_EVENT0("[DCE] CTS     CLR");
            }

            if (getevent.u.modemControlReg & BTA_DG_RI_ON) {
                APPL_TRACE_EVENT0("[DCE] RING SET");
            } else {
                APPL_TRACE_EVENT0("[DCE] RING    CLR");
            }

            if (getevent.u.modemControlReg & BTA_DG_CD_ON) {
                APPL_TRACE_EVENT0("[DCE] DCD SET");
            } else {
                APPL_TRACE_EVENT0("[DCE] DCD     CLR");
            }

#if( defined DUN_USE_BTPORT ) && (DUN_USE_BTPORT == TRUE)
            /* is a bit dangerous as get_dg_app_id might return invalid handle! TODO: fix get_dg_app_id() */
            app_id = get_dg_app_id(dialup_fd);
            p_cb = &btui_dg_cb.app_cb[app_id];
            /* TODO: calculate CHANGED signals only. update internal modem signal state */
            p_cb->modem_ctrl = ( p_cb->modem_ctrl & (~(BTA_DG_DTRDSR|BTA_DG_RTSCTS|BTA_DG_RI|BTA_DG_CD)) ) | getevent.u.modemControlReg;
            btapp_dg_btlif_send_modem_ctrl( app_id,
                                            (BTA_DG_DTRDSR|BTA_DG_RTSCTS|BTA_DG_RI|BTA_DG_CD),
                                            (const UINT8)getevent.u.modemControlReg);
#endif

            /* ci_control can handle multiple signal changes in a single call! comment out
             * BTA_DG_CD in case of emulator! as pppd does de-assert CD on open! */
            bta_dg_ci_control( dg_handle, (BTA_DG_DTRDSR|BTA_DG_RTSCTS|BTA_DG_RI|BTA_DG_CD),
                               getevent.u.modemControlReg );

#if( defined DUN_USE_BTPORT ) && (DUN_USE_BTPORT == TRUE)
            /* if CTS is on, kick in again tx_path. execute it after flow control change */
            if (getevent.u.modemControlReg & BTA_DG_RTSCTS_ON)
            {
                if ( ! p_cb->tx_data_enable )
                {
                    p_cb->tx_data_enable = TRUE;
                    bta_dg_ci_tx_ready( dg_handle );
                }
            }
#endif

            /* acknowledge ok */
            result[0] = 1;
            result[1] = 0;

            res = ioctl(dialup_fd, IOCTL_BTPORT_SET_EVENT_RESULT, result);

            APPL_TRACE_DEBUG1("Received kernel status ok (ioctl result %d)", res);
            break;

        case BTLINUX_PORT_MODEM_STATUS:
            /* TODO: use a local copy of signals as provide by BTA. BTA provide
             * signal changes and not a complete signal status
             */
            if (PORT_GetModemStatus (dg_handle, &signal) != PORT_SUCCESS)
            {
                APPL_TRACE_WARNING0( "sig_port_status_handler()::PORT_GetModemStatus() failed!" );
                result[0] = 0;
            }
            else
            {
                APPL_TRACE_DEBUG1("KERNEL [DCE] <-- USER[DTE] [BTLINUX_PORT_MODEM_STATUS] fetched rfcomm status, notify driver [%x]\n", signal);

                result[0] = 1;
                result[1] = signal;
            }
            res = ioctl(dialup_fd, IOCTL_BTPORT_SET_EVENT_RESULT, result);

            if (res < 0) {
                LOGE("%s: ioctl failed : result = %d", __FUNCTION__, res);
            }
            break;
    }
}


/* Parse /proc/devices file to find BtPort Major # */
static int get_major_number(void)
{
    FILE *file;
    int major = -1, found_major = 0;
    char line[80], major_name[32];

    file = fopen("/proc/devices", "r");
    if (!file) return -1;

    do {
        if (!fgets(line, (sizeof line)-1, file)) {
            fclose(file);
            return -1;
        }
            line[(sizeof line)-1] = 0;
    } while(strncmp(line, "Character", 9) != 0);

    while(fgets(line, (sizeof line)-1, file)) {
        if (sscanf(line, "%d %31s\n", &major, major_name) != 2)
            continue;
        if (strcmp(major_name, "BtPort") != 0)
            continue;
        found_major = 1;
        break;
    }
    if(!found_major) {
        major = -1;
    }

    fclose(file);
    return major;
}


static int btport_create_node(const char *path, int minor)
{
    int result, major = -1;
    mode_t mode;
    dev_t dev;

    //mode = get_device_perm(path, &uid, &gid) |  S_IFCHR;
    mode = 0600 |  S_IFCHR;
    //mode = 0664 |  S_IFCHR;

    /* try to find dynamically created Major # for BtPort  */
    major = get_major_number();

    if(major == -1)
    {
        APPL_TRACE_EVENT1("btport_create_node: invalid btport major %d", major);
        return -1;
    }
    APPL_TRACE_EVENT1("btport_create_node: found btport major %d", major);

    dev = (major << 8) | minor;
    result = mknod(path, mode, dev);

    if (result == 0)
    {
        APPL_TRACE_EVENT1("Created device node %s", path);
        chown(path, 0, 0);
    }
    return result;
}

void create_dev_nodes(void)
{
    dev_t dev;
    APPL_TRACE_EVENT0("Creating device nodes...");
    btport_create_node(BTPORT_NAME_DUN, 0);
    btport_create_node(BTPORT_NAME_SPP, 1);
}


int BTPORT_Init(void)
{
    /* read property for redirection enable (we assume this won't change in btld runtime) */
    btport_redirection_enable = btl_cfg_get_btport_redirection_enable();

    APPL_TRACE_EVENT1("Port redirection enable %d", btport_redirection_enable);

    if(btport_redirection_enable)
    {
#ifndef LINUX_NATIVE
        create_dev_nodes();
#endif
    }
    return 0;
}

int BTPORT_IsEnabled(void)
{
    return btport_redirection_enable;
}

int BTPORT_StartRedirection(tCTRL_HANDLE btlif_ctrl, tBTA_SERVICE_ID service, int app_id, UINT16 port_handle)
{
    struct sigaction act;
    int flags;
    int fd=(-1);

    /* store dg handle */
    switch(service)
    {
        case BTA_DUN_SERVICE_ID:

            APPL_TRACE_EVENT3("Opening DUN redirection port %s... app_id %d, dg handle %d", BTPORT_NAME_DUN, app_id, port_handle);

            if ((fd = open(BTPORT_NAME_DUN, O_RDWR | O_NOCTTY)) == -1)
            {
                APPL_TRACE_ERROR0("Failed to open DUN port\n");
                return fd;
            }
            dg_dun_dialup_fd = fd;
            dg_dun_handle = port_handle;
            dg_dun_app_id = app_id;

            /* attach btport fd to btl if server */
            BTL_IF_AttachExtFd(btlif_ctrl, fd, SUB_DUN, 0);
            break;

        case BTA_SPP_SERVICE_ID:

            APPL_TRACE_EVENT3("Opening SPP redirection port %s... app_id %d, dg handle %d", BTPORT_NAME_SPP, app_id, port_handle);

            if ((fd = open(BTPORT_NAME_SPP, O_RDWR | O_NOCTTY)) == -1)
            {
                APPL_TRACE_ERROR0("Failed to open SPP port\n");
                return fd;
            }
            dg_spp_dialup_fd = fd;
            dg_spp_handle = port_handle;
            dg_spp_app_id = app_id;

            /* attach btport fd to btl if server  */
            BTL_IF_AttachExtFd(btlif_ctrl, fd, SUB_SPP, app_id);
            break;

        default:
            APPL_TRACE_ERROR2("%s invalid service id", __FUNCTION__, service);
            break;
    }

    /* register signal handler to manage tty flow control signalling from kernel driver */
    sigemptyset (&act.sa_mask);
    act.sa_sigaction = sig_port_status_handler;
    act.sa_flags = SA_SIGINFO | SA_RESTART;
    sigaddset(&act.sa_mask, SIGUSR1);

    if (sigaction(SIGUSR1, &act, NULL) < 0)
    {
        APPL_TRACE_EVENT0("sigaction error");
        close(fd);
        return 0;
    }

    fcntl(fd, F_SETOWN, getpid());
    flags = fcntl(fd, F_GETFL);
    fcntl (fd, F_SETFL, flags | FASYNC);
    fcntl (fd, F_SETSIG, SIGUSR1);

    return 0;
}


int BTPORT_StopRedirection(tBTA_SERVICE_ID service, INT32 fd)
{
    APPL_TRACE_EVENT1("BTPORT_StopRedirection fd %d", fd);

    /* disconnect btport */
    BTL_IF_DisconnectDatapath(fd);

    /* reset port redirection dg handles */

    /* store dg handle */
    switch(service)
    {
        case BTA_DUN_SERVICE_ID:
            dg_dun_dialup_fd = DG_DUN_HANDLE_INVALID;
            dg_dun_handle = DG_DUN_HANDLE_INVALID;
            dg_dun_app_id = DG_DUN_HANDLE_INVALID;
            break;
        case BTA_SPP_SERVICE_ID:
            dg_spp_dialup_fd = DG_DUN_HANDLE_INVALID;
            dg_spp_handle = DG_DUN_HANDLE_INVALID;
            dg_spp_app_id = DG_DUN_HANDLE_INVALID;
            break;
        default:
            APPL_TRACE_ERROR2("%s invalid service id", __FUNCTION__, service);
            break;
    }
    return 0;
}

#if( defined DUN_USE_SMD_INTERFACE ) && (DUN_USE_SMD_INTERFACE == TRUE)
void smd_handle_ril_flow_ctrl_req(int app_id, int signals, int mask)
{
    UINT8 val = 0;
    tBTUI_DG_APP_CB *p_cb = &btui_dg_cb.app_cb[app_id];

    LOGI("%s: signals = 0x%02x, mask = 0x%02x", __FUNCTION__, signals, mask);

    /* Now set corresponding bits in physical interface if needed */
    if (mask == prev_smd_status)
    {
        LOGI("%s: No change on RS232 control signals", __FUNCTION__);
        return;
    }

    if (signals & DG_RIL_FLOW_CTRL_SIGNAL_DTRDSR)
    {
        LOGI("%s: DG_RIL_FLOW_CTRL_SIGNAL_DTRDSR", __FUNCTION__);
        if ((mask & DG_RIL_FLOW_CTRL_DTRDSR_ON) != (prev_smd_status & DG_RIL_FLOW_CTRL_DTRDSR_ON))
        {
            val |= (mask & DG_RIL_FLOW_CTRL_DTRDSR_ON) ? BTA_DG_DTRDSR_ON : BTA_DG_DTRDSR_OFF ;
            bta_dg_ci_control(p_cb->port_handle, BTA_DG_DTRDSR, val);
        }
    }

    if (signals & DG_RIL_FLOW_CTRL_SIGNAL_RTSCTS)
    {
        LOGI("%s: DG_RIL_FLOW_CTRL_SIGNAL_RTSCTS", __FUNCTION__);
        if ((mask & DG_RIL_FLOW_CTRL_RTSCTS_ON) != (prev_smd_status & DG_RIL_FLOW_CTRL_RTSCTS_ON))
        {
            val |= (mask & DG_RIL_FLOW_CTRL_RTSCTS_ON) ? BTA_DG_RTSCTS_ON : BTA_DG_RTSCTS_OFF ;
            bta_dg_ci_control(p_cb->port_handle, BTA_DG_RTSCTS, val);
        }
    }

    if (signals & DG_RIL_FLOW_CTRL_SIGNAL_RI)
    {
        LOGI("%s: DG_RIL_FLOW_CTRL_SIGNAL_RI", __FUNCTION__);
        if ((mask & DG_RIL_FLOW_CTRL_RI_ON) != (prev_smd_status & DG_RIL_FLOW_CTRL_RI_ON))
        {
            val |= (mask & DG_RIL_FLOW_CTRL_RI_ON) ? BTA_DG_RI_ON : BTA_DG_RI_OFF ;
            bta_dg_ci_control(p_cb->port_handle, BTA_DG_RI, val);
        }
    }

    if (signals & DG_RIL_FLOW_CTRL_SIGNAL_CD)
    {
        LOGI("%s: DG_RIL_FLOW_CTRL_SIGNAL_CD", __FUNCTION__);
        if ((mask & DG_RIL_FLOW_CTRL_CD_ON) != (prev_smd_status & DG_RIL_FLOW_CTRL_CD_ON))
        {
            val |= (mask & DG_RIL_FLOW_CTRL_CD_ON) ? BTA_DG_CD_ON : BTA_DG_CD_OFF ;
            bta_dg_ci_control(p_cb->port_handle, BTA_DG_CD, val);
        }
    }

    prev_smd_status = mask;
}

static void smd_monitor_exit_handler(int sig)
{
   pthread_exit(0);
}

static void flow_inc(int count)
{
    //APPL_TRACE_ERROR2("flow_inc: q count = %d %s", count, (flow_state == 1?"Flow ON":"Flow OFF"));

    /* flow off if we reached high watermark */
    if ((count >= SMD_DUN_BUF_WATERMARK_FLOW_ON) && (flow_state == FLOW_OFF))
    {
        APPL_TRACE_ERROR0("****************************************************");
        APPL_TRACE_ERROR1("Above the High Water Mark - stop reading, q count = %d", count);
        APPL_TRACE_ERROR0("****************************************************");

        flow_state = FLOW_ON;
        sem_wait(&dun_high_water_mark_sem);
    }
}

static void flow_dec(int count)
{
    //APPL_TRACE_ERROR2("flow_dec: q count = %d %s", count, (flow_state == 1?"Flow ON":"Flow OFF"));

    /* flow on if we reached low watermark */
    if ((count <= SMD_DUN_BUF_WATERMARK_FLOW_OFF) && (flow_state == FLOW_ON))
    {
        APPL_TRACE_ERROR0("****************************************************");
        APPL_TRACE_ERROR1("Under the High Water Mark - start reading, q count = %d", count);
        APPL_TRACE_ERROR0("****************************************************");

        flow_state = FLOW_OFF;
        sem_post(&dun_high_water_mark_sem);
    }
}

static void start_read_delay (UINT32 timeout)
{
    struct timespec delay;
    int err;

    delay.tv_sec = timeout / 1000;
    delay.tv_nsec = 1000 * 1000 * (timeout%1000);

    /* [u]sleep can't be used because it uses SIGALRM */

    do {
        err = nanosleep(&delay, &delay);
    } while (err < 0 && errno ==EINTR);

    return;
}

static void *smd_monitor(void *arg)
{
    BT_HDR *p_buf;
    int num_read;
    void *p_data = NULL;
    int smd_status = 0;
    UINT8 val = 0;
    UINT16 buf_size=0;
    struct sigaction actions;
    tBTUI_DG_APP_CB *p_cb = (tBTUI_DG_APP_CB *)arg;

    memset(&actions, 0, sizeof(actions));
    sigemptyset(&actions.sa_mask);
    actions.sa_flags = 0;
    actions.sa_handler = smd_monitor_exit_handler;

    if (sigaction(SIGUSR1,&actions,NULL) < 0) {
        APPL_TRACE_ERROR0("Error in sigaction");
    }

    while(1)
    {
        if (smdport_fd < 0)
        {
            APPL_TRACE_ERROR0("smdport_fd wasn't opened successfully");
            continue;
        }
        //need to be revised later.... congestion case.

        if(p_cb->data_q.count >= SMD_DUN_BUF_WATERMARK_FLOW_ON)
        {
              APPL_TRACE_ERROR1("%s : Let's give some time to the stack to handle previous Rx data", __FUNCTION__);

              start_read_delay(100);
        }

        buf_size = ((p_cb->peer_mtu) < (BTA_DUN_MTU)) ? (p_cb->peer_mtu) : (BTA_DUN_MTU);
        p_buf = GKI_getbuf(buf_size+L2CAP_MIN_OFFSET+RFCOMM_DATA_OVERHEAD+sizeof(BT_HDR));
        if(!p_buf)
        {
        APPL_TRACE_ERROR1("%s : FAIL TO GET GKI BUFFER", __FUNCTION__);

        /*
         * seems we can get into a situation were we can lock up BTLD if we have no
         * buffers available...  if this is the case then put this thread to sleep
         * for 500 ms to give the rest of the threads a chance to run.
         */
        start_read_delay(500);
            continue;
        }

        if ((num_read = read(smdport_fd, smd_read_buf, (size_t)buf_size)) < 0)
        {
            APPL_TRACE_ERROR0("FAIL TO READ FROM SMD PORT");
            GKI_freebuf(p_buf);
            continue;
        }

        if(num_read == 0)
        {
            APPL_TRACE_WARNING0("NO VALIND DATA READ FROM SMD PORT");
            GKI_freebuf(p_buf);
            continue;
        }

        // APPL_TRACE_ERROR3("%s: Read from smdport %d bytes successfully, (q count = %d)", __FUNCTION__, num_read, p_cb->data_q.count);

        /* AT result - CONNECT */
        if (!bt_dun_parse_connect && !strncasecmp(smd_read_buf, "\r\nCONNECT", 9))
        {
            char *revised_result = "\r\nCONNECT 115200\r\n";
            bt_dun_parse_connect= TRUE;
            APPL_TRACE_ERROR2("Got AT Result : %s, revised AT Result: %s", smd_read_buf, revised_result);

            num_read = strlen(revised_result);
            memcpy(smd_read_buf, revised_result, strlen(revised_result));
        }

        p_buf->offset = L2CAP_MIN_OFFSET+RFCOMM_DATA_OVERHEAD;
        p_data = (UINT8 *)(p_buf+1)+ p_buf->offset;
        p_buf->len = num_read;

        memcpy(p_data, smd_read_buf, num_read);

        GKI_enqueue(&p_cb->data_q, p_buf);

        bta_dg_ci_rx_ready(p_cb->port_handle);

        flow_inc(p_cb->data_q.count);

#if 0 /* flow control is being handled via phone RIL interface - refer to function smd_handle_ril_flow_ctrl_req */

        /* Get modem status then tell if needed */
        if( ioctl (smdport_fd, TIOCMGET, &smd_status) <0 )
        {
            APPL_TRACE_ERROR0("TIOCMGET for smd port failed ");
            continue;
        }

        /* Now set corresponding bits in physical interface if needed */
        if (smd_status == prev_smd_status)
        {
            APPL_TRACE_DEBUG0("No change on RS232 control signals");
            continue;
        }

        if((smd_status & TIOCM_DSR) != (prev_smd_status & TIOCM_DSR))
        {
            val |= (smd_status & TIOCM_DSR) ? BTA_DG_DTRDSR_ON : BTA_DG_DTRDSR_OFF ;
            bta_dg_ci_control(p_cb->port_handle, BTA_DG_DTRDSR, val);
        }
        if((smd_status & TIOCM_RTS) != (prev_smd_status & TIOCM_RTS))
        {
            val |= (smd_status & TIOCM_RTS) ? BTA_DG_RTSCTS_ON : BTA_DG_RTSCTS_OFF ;
            bta_dg_ci_control(p_cb->port_handle, BTA_DG_DTRDSR, val);
        }
        if((smd_status & TIOCM_RI) != (prev_smd_status & TIOCM_RI))
        {
            val |= (smd_status & TIOCM_RI) ? BTA_DG_RI_ON : BTA_DG_RI_OFF ;
            bta_dg_ci_control(p_cb->port_handle, BTA_DG_RI, val);
        }
        if((smd_status & TIOCM_CD) != (prev_smd_status & TIOCM_CD))
        {
            val |= (smd_status & TIOCM_CD) ? BTA_DG_CD_ON : BTA_DG_CD_OFF ;
            bta_dg_ci_control(p_cb->port_handle, BTA_DG_CD, val);
        }
        prev_smd_status = smd_status;
#endif
    } /* while */

    return NULL;
}

static void start_smd_monitor_thread(tBTUI_DG_APP_CB *p_cb)
{
    pthread_attr_t thread_attr;
    struct sched_param param;
    int policy;

    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);

    if (pthread_create( &p_th, &thread_attr, smd_monitor, (void *)p_cb) != 0 )
    {
        APPL_TRACE_ERROR0("!!! FAILED pthread_create !!!");
        return;
    }
    if(pthread_getschedparam(p_th, &policy, &param)==0)
    {
        policy = SCHED_FIFO;

    /*
     * the priority of the smd_monitor thread should
     * not be higher than the threads created using
     * GKI_create_task - this can cause some issues
     * if we get stuck in this thread doing lots of
     * additional processing.
     *
     * Changing to priority = 20 for now
     */
        param.sched_priority = 90;//need to be revised.....
        pthread_setschedparam(p_th, policy, &param);
    }
}

static void stop_smd_monitor_thread(tBTUI_DG_APP_CB *p_cb)
{
    int status;

    /* kill thread which reads external port */
    if((status = pthread_kill(p_th, SIGUSR1)) != 0)
    {
       APPL_TRACE_ERROR2("Error cancelling thread %d, error = %d", p_th, status);
    }

    pthread_join(p_th, NULL);
    APPL_TRACE_DEBUG0("Joined smd monitor thread ");

}
#endif  /* #if( defined DUN_USE_SMD_INTERFACE ) && (DUN_USE_SMD_INTERFACE == TRUE) */

/*******************************************************************************
**
** Function         bta_dg_co_init
**
** Description      This callout function is executed by DG when a server is
**                  started by calling BTA_DgListen().  This function can be
**                  used by the phone to initialize data paths or for other
**                  initialization purposes.  The function must return the
**                  data flow mask as described below.
**
**
** Returns          Data flow mask.
**
*******************************************************************************/

UINT8 bta_dg_co_init(UINT16 port_handle, UINT8 app_id)
{
    APPL_TRACE_DEBUG2("bta_dg_co_init port_handle:%d app_id:%d", port_handle, app_id);

    tBTUI_DG_APP_CB *p_cb = &btui_dg_cb.app_cb[app_id];
    p_cb->port_handle = port_handle;

    /* only support PULL interface for all connections  */
    return (BTA_DG_RX_PULL | BTA_DG_TX_PULL);
}

/*******************************************************************************
**
** Function         bta_dg_co_open
**
** Description      This function is executed by DG when a connection to a
**                  server is opened.  The phone can use this function to set
**                  up data paths or perform any required initialization or
**                  set up particular to the connected service.
**
**
** Returns          void
**
*******************************************************************************/
void bta_dg_co_open(UINT16 port_handle, UINT8 app_id, tBTA_SERVICE_ID service, UINT16 mtu)
{
    tBTUI_DG_APP_CB *p_cb = &btui_dg_cb.app_cb[app_id];

    APPL_TRACE_DEBUG4("bta_dg_co_open port_handle:%d, app_id:%d, p_cb->port_handle: %d mtu is %d", port_handle, app_id, p_cb->port_handle, mtu);

#if( defined DUN_USE_SMD_INTERFACE ) && (DUN_USE_SMD_INTERFACE == TRUE)
    bt_dun_parse_connect= FALSE;
#endif

    switch (service)
    {
    case BTA_SPP_SERVICE_ID:
        p_cb->port_handle = port_handle;
        p_cb->service_id = service;  /* No need.  */
        p_cb->peer_mtu = mtu;
        GKI_init_q(&p_cb->data_q);

#if( defined DUN_USE_BTPORT ) && (DUN_USE_BTPORT == TRUE)
        p_cb->tx_data_enable = TRUE;    /* allow tx_co_path data to pass */
#if (TRUE == BTAPP_DG_EMULATOR)
        p_cb->at_cmd_mode = FALSE;       /* at open, for SPP no command mode */
#endif
#endif
        break;

    case BTA_DUN_SERVICE_ID:

        APPL_TRACE_DEBUG2("bta_dg_co_open : [DUN] app id %d, port handle %d", app_id, port_handle);
        p_cb->service_id = service;
        p_cb->port_handle = port_handle;
        p_cb->peer_mtu = mtu;
        GKI_init_q(&p_cb->data_q);

#if( defined DUN_USE_BTPORT ) && (DUN_USE_BTPORT == TRUE)
        p_cb->tx_data_enable = TRUE;    /* allow tx_co_path data to pass */
        p_cb->modem_ctrl = 0;           /* all modem signals should be off */
#if (TRUE == BTAPP_DG_EMULATOR)
        p_cb->at_cmd_mode = TRUE;       /* at open, assume at command line mode, no flow control! */
#endif
#endif

#if( defined DUN_USE_SMD_INTERFACE ) && (DUN_USE_SMD_INTERFACE == TRUE)
        /* open the SMD port */
        if ((smdport_fd = open(MODEM_SMD_PORT, O_RDWR)) < 0)
        {
            LOGE("%s[DUN]: Unable to open SMD port %s", __FUNCTION__, MODEM_SMD_PORT);
            return;
        }
        LOGI("%s[DUN]: Opened SMD port, smdport_fd: %x", __FUNCTION__, smdport_fd);

        /*
         * initialize the high water mark semaphore
         * 1st param: ptr to sem_t variable
         * 2nd param: not supported in linux - set = 0
         * 3rd param: looks like this needs to be a non-zero value - set = 1
         */
        sem_init(&dun_high_water_mark_sem, 0, 1);

#if (0) //need to be revised later copied from dun_service.c for USB DUN implementations.
        /* Put external port into RAW mode.  Note that this is not
         * necessary for SMD port as a raw mode is the only one available
         */
        if (tcgetattr (pportparms->extport_fd, &term_params) < 0) {
            LOGE ("tcgetattr() call fails : %s\n", strerror(errno));
            return -1;
        }

        orig_term_params = term_params;
        term_params.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
        term_params.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
        term_params.c_cflag &= ~(CSIZE | PARENB);
        term_params.c_cflag |= CS8;
        term_params.c_oflag &= ~(OPOST);
        term_params.c_cc[VMIN] = 1;
        term_params.c_cc[VTIME] = 0;

        if (tcsetattr (pportparms->extport_fd, TCSAFLUSH, &term_params) < 0) {
            LOGE ("tcsetattr() call fails : %s\n", strerror(errno));
            return -1;
        }
#endif

        start_smd_monitor_thread((void *)p_cb);
#endif  /* #if( defined DUN_USE_SMD_INTERFACE ) && (DUN_USE_SMD_INTERFACE == TRUE)  */
        break;

    case BTA_FAX_SERVICE_ID:
        LOGI("%s[FAX]: port_handle = %d, app_id = %d, p_cb->port_handle = %d, mtu = %d",
             __FUNCTION__, port_handle, app_id, p_cb->port_handle, mtu);
        break;

    default:
        LOGI("%s: Unknown service ID %d, port_handle = %d, app_id = %d, mtu = %d",
             __FUNCTION__, service, port_handle, app_id, mtu);
        break;
    }

#if( defined DUN_USE_BTPORT ) && (DUN_USE_BTPORT == TRUE)
    /* on open, force DCE signals CTS and DSR on. this should make BTW/DUN happy */
    bta_dg_ci_control( port_handle, BTA_DG_DTRDSR|BTA_DG_RTSCTS, BTA_DG_DTRDSR_ON|BTA_DG_RTSCTS_ON );

    /* reflect the state in the modem signals. actually updated happens only in btapp_dg_cback().
     * this should make sure the modem signal change get to the android application AFTER open event */
    p_cb->modem_ctrl = BTA_DG_DTRDSR_ON|BTA_DG_RTSCTS_ON;
#endif
}

/*******************************************************************************
**
** Function         bta_dg_co_close
**
** Description      This function is called by DG when a connection to a
**                  server is closed.
**
**
** Returns          void
**
*******************************************************************************/
void bta_dg_co_close(UINT16 port_handle, UINT8 app_id)
{
    tBTUI_DG_APP_CB *p_cb = &btui_dg_cb.app_cb[app_id];
#if( defined DUN_USE_BTPORT ) && (DUN_USE_BTPORT == TRUE)
    int fd;
    int res;
    UINT8 result[2];
#endif
#if( defined DUN_USE_SMD_INTERFACE ) && (DUN_USE_SMD_INTERFACE == TRUE)
    BT_HDR * p_buf ;
#endif

    APPL_TRACE_DEBUG2("bta_dg_co_close port_handle:%d app_id:%d", port_handle, app_id);

#if( defined DUN_USE_SMD_INTERFACE ) && (DUN_USE_SMD_INTERFACE == TRUE)
    switch (p_cb->service_id)
    {
        case BTA_SPP_SERVICE_ID:
            break;

        case BTA_DUN_SERVICE_ID:
            if(smdport_fd >= 0)
            {
                stop_smd_monitor_thread((void *)p_cb);

                /* close the SMD port */
                if ((close(smdport_fd)) < 0)
                {
                    APPL_TRACE_ERROR1("FAIL TO CLOSE SMD port", smdport_fd);
                }
                smdport_fd = (-1);
                APPL_TRACE_DEBUG1("SMD port closed successfully, smdport_fd: %x", smdport_fd);
            }

            /* clean out queue */
            while((p_buf = (BT_HDR*)GKI_dequeue (&p_cb->data_q)) != NULL)
                GKI_freebuf(p_buf);
            break;

        case BTA_FAX_SERVICE_ID:
            break;

        default:
            break;
    }
#endif

#if( defined DUN_USE_BTPORT ) && (DUN_USE_BTPORT == TRUE)
    /* remap app id to btport fd */
    fd = get_btport_fd_by_app_id(app_id);

    /* make sure app id is for a redirection port */
    if (fd != DG_DUN_HANDLE_INVALID)
    {
        APPL_TRACE_EVENT0( "bta_dg_co_close(): DSR&CTS CLEARED [DTE->DCE]" );
        /* indicate closure of BT DUN also via DTE->DCE signals, DSR & CTS */
        result[0] = BTA_DG_DTRDSR|BTA_DG_RTSCTS;
        result[1] = BTA_DG_DTRDSR_OFF|BTA_DG_RTSCTS_OFF;

        res = ioctl(fd, IOCTL_BTPORT_HANDLE_MCTRL, result);

         if (res < 0)
         {
             APPL_TRACE_ERROR2("bta_dg_co_close()::ioctl failed with reason %s (%d)", strerror(errno), res);
         }
         /* TODO: use for all dg apps independent of service. all set signals are set to 0 if not yet
          * done. e.g. in link loss case this might happen! */
         if ( p_cb->modem_ctrl )
             btapp_dg_btlif_send_modem_ctrl( app_id, p_cb->modem_ctrl, 0 );
    }
    else
    {
        APPL_TRACE_WARNING2( "bta_dg_co_close(port_handle: %d, app_id %d) NO redirection handle for this app id",
                             port_handle, app_id );
    }

    p_cb->modem_ctrl = 0;
    /* 0 is invalid port handle. using this handle should result in correct error messages from RFCOMM */
    p_cb->port_handle = 0;
#endif
}

/*******************************************************************************
**
** Function         bta_dg_co_tx_path
**
** Description      This function is called by DG to transfer data on the
**                  TX path; that is, data being sent from BTA to the phone.
**                  This function is used when the TX data path is configured
**                  to use the pull interface.  The implementation of this
**                  function will typically call Bluetooth stack functions
**                  PORT_Read() or PORT_ReadData() to read data from RFCOMM
**                  and then a platform-specific function to send data that
**                  data to the phone.
**
**
** Returns          void
**
*******************************************************************************/
void bta_dg_co_tx_path(UINT16 port_handle, UINT8 app_id)
{
    BT_HDR          *p_buf;
    int             status;
    UINT16          port_errors;
    tPORT_STATUS    port_status;
    tBTUI_DG_APP_CB *p_cb = &btui_dg_cb.app_cb[app_id];
#if( defined DUN_USE_SMD_INTERFACE ) && (DUN_USE_SMD_INTERFACE == TRUE)
    void *p_data = NULL;
    UINT16 read_len = 0;
#endif

    APPL_TRACE_DEBUG2("bta_dg_co_tx_path port_handle: %d, service id %d",  port_handle, p_cb->service_id);

    /* useless code. PORT_Read() returns NULL if empty queue */
#if 0
    /* check queue status */
    if (PORT_GetQueueStatus(port_handle, &port_status) != PORT_SUCCESS)
    {
       APPL_TRACE_ERROR0("PORT_GetQueueStatus failed");
       return;
    }

    /* make sure there is any data available */
    if (port_status.in_queue_size == 0)
    {
       APPL_TRACE_ERROR0("bta_dg_co_tx_path : no data in queue");
       return;
    }
#endif

#if( defined DUN_USE_BTPORT ) && (DUN_USE_BTPORT == TRUE)
    /* if tx path data flow is disallowed, ignore call-out. rfcomm flow control will kick in if
     * queues are full
     */
    if ( FALSE == p_cb->tx_data_enable )
    {
#if (TRUE == BTAPP_DG_EMULATOR)
        if (! p_cb->at_cmd_mode )
        {
#endif
        return;
#if (TRUE == BTAPP_DG_EMULATOR)
        }
#endif
    }
#endif

    /* read data from port */
    if ((status = PORT_Read(port_handle, &p_buf)) != PORT_SUCCESS)
    {
        /* GKI_freebuf(p_buf); in case of an error no buffer is allocated! */

        /* check for and clear line error */
        if (status == PORT_LINE_ERR)
        {
            PORT_ClearError(port_handle, &port_errors, &port_status);
        }

        APPL_TRACE_WARNING1("PORT_ReadData error status:%d", status);
        return;
    }

    /* TODO: fundamentally only the app id/port handle should matter, not the service_id!!!*/
    switch (p_cb->service_id)
    {
        case BTA_SPP_SERVICE_ID:
            btapp_dg_spp_tx_data(app_id, p_buf);
            break;

        case BTA_DUN_SERVICE_ID:
#if( defined DUN_USE_SMD_INTERFACE ) && (DUN_USE_SMD_INTERFACE == TRUE)
            if (smdport_fd < 0)
            {
                APPL_TRACE_ERROR1("bta_dg_co_tx_path,  read_len: %d", read_len);
                break;
            }

            p_data = (UINT8 *)(p_buf + 1) + p_buf->offset;
            read_len = p_buf->len;

#if defined (PTS_DUN_TEST_ONLY) && (PTS_DUN_TEST_ONLY == TRUE)
            /* AT commands */
            if (!strncasecmp(p_data, "ATD", 3)) /* windows standard modem */
            {
                APPL_TRACE_EVENT0("Got ATD");

                /* check if call is active - if so return BUSY to DUN Client */
                if(btl_cfg_get_call_active_status())
                {
                    char *busy = "\r\nBUSY\r\n";

                    APPL_TRACE_EVENT0("Call currently active - do not allow DUN connection");

                    p_buf->offset = L2CAP_MIN_OFFSET+RFCOMM_DATA_OVERHEAD;
                    p_data = (UINT8 *)(p_buf + 1) + p_buf->offset;
                    p_buf->len = strlen(busy);
                    memcpy(p_data, busy, p_buf->len);

                    APPL_TRACE_EVENT2("(%d) bytes sent : %s ", p_buf->len, p_data);

                    GKI_enqueue(&p_cb->data_q, p_buf);

                    bta_dg_ci_rx_ready(p_cb->port_handle);
                    return;
                }
            }
#endif
            APPL_TRACE_DEBUG1("bta_dg_co_tx_path,  read_len: %d", read_len);

            if (smdport_fd && read_len > 0)
            {
                if (write (smdport_fd, (void *) p_data, read_len) < 0)
                {
                    APPL_TRACE_ERROR1("bta_dg_co_tx_path,  read_len: %d", read_len);
                }
            }

#else
            btapp_dg_dun_rx_data(app_id, p_buf);
#endif  /* #if( defined DUN_USE_SMD_INTERFACE ) && (DUN_USE_SMD_INTERFACE == TRUE) */
            break;

        case BTA_FAX_SERVICE_ID:
            break;

        default:
           break;
    }

    /* now free buffer */
    GKI_freebuf(p_buf);
}

/*******************************************************************************
**
** Function         bta_dg_co_rx_path
**
** Description      This function is called by DG to transfer data on the
**                  RX path; that is, data being sent from the phone to BTA.
**                  This function is used when the RX data path is configured
**                  to use the pull interface.  The implementation of this
**                  function will typically call a platform-specific function
**                  to read data from the phone and then call Bluetooth stack
**                  functions PORT_Write() or PORT_WriteData() to send data
**                  to RFCOMM.
**
**
** Returns          void
**
*******************************************************************************/
void bta_dg_co_rx_path(UINT16 port_handle, UINT8 app_id, UINT16 mtu)
{
    BT_HDR          *p_buf;
    tBTUI_DG_APP_CB *p_cb = &btui_dg_cb.app_cb[app_id];
    int             status;
    UINT16          port_errors;
    tPORT_STATUS    port_status;

    switch (p_cb->service_id)
    {
        case BTA_SPP_SERVICE_ID:

            APPL_TRACE_DEBUG1("bta_dg_co_rx_path port_handle:%d", port_handle);

            if((p_buf = (BT_HDR *)GKI_dequeue(&p_cb->data_q)) != NULL)
            {
                btapp_dg_spp_tx_dequeued(app_id, p_buf, p_cb->data_q.count);

                status = PORT_Write(port_handle, p_buf);

                if (status == PORT_LINE_ERR)
                {
                    PORT_ClearError(port_handle, &port_errors, &port_status);
                    GKI_freebuf(p_buf);
                }
                else
                {
                    APPL_TRACE_DEBUG2("bta_dg_co_rx_path mtu : %d buf len %d",  mtu, p_buf->len);
                }
            }
            break;

        case BTA_DUN_SERVICE_ID:
            if((p_buf = (BT_HDR *)GKI_dequeue(&p_cb->data_q)) != NULL)
            {
                //APPL_TRACE_EVENT2("bta_dg_co_rx_path, dequeue %d bytes (mtu %d)", p_buf->len, mtu);

#if( defined DUN_USE_BTPORT ) && (DUN_USE_BTPORT == TRUE)
                btapp_dg_dun_tx_dequeued(app_id, p_buf);
#endif
                status = PORT_Write(port_handle, p_buf);

                if (status!=PORT_SUCCESS)
                    APPL_TRACE_ERROR1("DUN error : PORT_Write status %d\n", status);

                if (status == PORT_LINE_ERR)
                {
                    PORT_ClearError(port_handle, &port_errors, &port_status);
                    GKI_freebuf(p_buf);
                }

#if( defined DUN_USE_SMD_INTERFACE ) && (DUN_USE_SMD_INTERFACE == TRUE)
                /* If the q.count is below the watermark then start
                 * reading data again from the SMD interface.
                 */
                flow_dec(p_cb->data_q.count);

                /* If there is data in Q then call the bta_dg_ci_rx_ready function
                 * because there is a chance that if there is no more data to read
                 * then the remaining data in Q will never be sent over BT.
                 */
                if (p_cb->data_q.count)
                    bta_dg_ci_rx_ready(p_cb->port_handle);
#endif
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
** Function         bta_dg_co_tx_write
**
** Description      This function is called by DG to send data to the phone
**                  when the TX path is configured to use a push interface.
**                  The implementation of this function must copy the data to
**                  the phone's memory.
**
**
** Returns          void
**
*******************************************************************************/
void bta_dg_co_tx_write(UINT16 port_handle, UINT8 app_id, UINT8 *p_data, UINT16 len)
{
    APPL_TRACE_DEBUG1("bta_dg_co_tx_write port_handle:%d", port_handle);
}

/*******************************************************************************
**
** Function         bta_dg_co_tx_writebuf
**
** Description      This function is called by DG to send data to the phone
**                  when the TX path is configured to use a push interface with
**                  zero copy.  The phone must free the buffer using function
**                  GKI_freebuf() when it is through processing the buffer.
**
**
** Returns          void
**
*******************************************************************************/
void bta_dg_co_tx_writebuf(UINT16 port_handle, UINT8 app_id, BT_HDR *p_buf)
{
    APPL_TRACE_DEBUG2("bta_dg_co_tx_writebuf port_handle:%d port %d", port_handle, app_id);
}

/*******************************************************************************
**
** Function         bta_dg_co_rx_flow
**
** Description      This function is called by DG to enable or disable
**                  data flow on the RX path when it is configured to use
**                  a push interface.  If data flow is disabled the phone must
**                  not call bta_dg_ci_rx_write() or bta_dg_ci_rx_writebuf()
**                  until data flow is enabled again.
**
**
** Returns          void
**
*******************************************************************************/
void bta_dg_co_rx_flow(UINT16 port_handle, UINT8 app_id, BOOLEAN enable)
{
    APPL_TRACE_DEBUG2("bta_dg_co_rx_flow port_handle:%d enable:%d", port_handle, enable);
}

/*******************************************************************************
**
** Function         bta_dg_co_control
**
** Description      This function is called by DG to send RS-232 signal
**                  information to the phone.  This function allows these
**                  signals to be propagated from the RFCOMM channel to the
**                  phone.  If the phone does not use these signals the
**                  implementation of this function can do nothing.
**
**
** Returns          void
**
*******************************************************************************/
void bta_dg_co_control(UINT16 port_handle, UINT8 app_id, UINT8 signals, UINT8 values)
{
    UINT8 result[2];
    int res;
    int fd;
    int cmd_bits = 0;
    tBTUI_DG_APP_CB *p_cb = &btui_dg_cb.app_cb[app_id];

#if( defined DUN_USE_SMD_INTERFACE ) && (DUN_USE_SMD_INTERFACE == TRUE)
    APPL_TRACE_API3("bta_dg_co_control handle:%d signals:0x%x values:0x%x",
                      port_handle, signals, values);

    if(signals & BTA_DG_DTRDSR)
    {
        if (values & BTA_DG_DTRDSR_ON)
        {
            APPL_TRACE_ERROR0("bta_dg_co_control: DTRDSR ON ");

            cmd_bits |= DG_RIL_FLOW_CTRL_DTRDSR_ON;
        }
        else
        {
            APPL_TRACE_ERROR0("bta_dg_co_control: DTRDSR OFF ");

            cmd_bits |= DG_RIL_FLOW_CTRL_DTRDSR_OFF;
        }
    }

    if(signals & BTA_DG_RTSCTS)
    {
        if (values & BTA_DG_RTSCTS_ON)
        {
            APPL_TRACE_ERROR0("bta_dg_co_control: RTSCTS ON ");

            cmd_bits |= DG_RIL_FLOW_CTRL_RTSCTS_ON;
        }
        else
        {
            APPL_TRACE_ERROR0("bta_dg_co_control: RTSCTS OFF ");

            cmd_bits |= DG_RIL_FLOW_CTRL_RTSCTS_OFF;
        }
    }

    if(signals & BTA_DG_RI)
    {
        if (values & BTA_DG_RI_ON)
        {
            APPL_TRACE_ERROR0("bta_dg_co_control: RI ON ");

            cmd_bits |= DG_RIL_FLOW_CTRL_RI_ON;
        }
        else
        {
            APPL_TRACE_ERROR0("bta_dg_co_control: RI OFF ");

            cmd_bits |= DG_RIL_FLOW_CTRL_RI_OFF;
        }
    }

    if(signals & BTA_DG_CD)
    {
        if (values & BTA_DG_CD_ON)
        {
            APPL_TRACE_ERROR0("bta_dg_co_control: CD ON ");

            cmd_bits |= DG_RIL_FLOW_CTRL_CD_ON;
        }
        else
        {
        APPL_TRACE_ERROR0("bta_dg_co_control: CD OFF ");

            cmd_bits |= DG_RIL_FLOW_CTRL_CD_OFF;
        }
    }
    btapp_dg_set_ril_flow_ctrl(signals, cmd_bits);
#else
    if (btport_redirection_enable)
    {
        /* remap app id to btport fd */
        fd = get_btport_fd_by_app_id(app_id);

        /* make sure app id is for a redirection port */
        if (fd == DG_DUN_HANDLE_INVALID)
        {
            APPL_TRACE_WARNING2( "bta_dg_co_control(port_handle: %d, app_id %d) NO handle for this app id",
                    port_handle, app_id );
            return;
        }

        APPL_TRACE_DEBUG4("[KERNEL[DCE] <-- USER[DTE]] bta_dg_co_control handle:%d signals:0x%x values:0x%x (fd %d)",
                port_handle, signals, values, fd);

        if ( signals & BTA_DG_DTRDSR)
        {
            if (values & BTA_DG_DTRDSR_ON) {
                APPL_TRACE_EVENT0("[DTE] DTR SET");
            } else {
                APPL_TRACE_EVENT0("[DTE] DTR     CLR");
            }
        }

        if ( signals & BTA_DG_RTSCTS )
        {
            if (values & BTA_DG_RTSCTS_ON) {
                APPL_TRACE_EVENT0("[DTE] RTS SET");
            } else {
                APPL_TRACE_EVENT0("[DTE] RTS     CLR");
            }
        }
        /* btld is a DCE device. ring and DCD go from btld to remote BT device. they should not change */
        if ( signals & BTA_DG_RI )
        {
            if (values & BTA_DG_RI_ON) {
                APPL_TRACE_EVENT0("RING SET");
            } else {
                APPL_TRACE_EVENT0("RING    CLR");
            }
        }

        if ( signals & BTA_DG_CD )
        {
            if (values & BTA_DG_CD_ON) {
                APPL_TRACE_EVENT0("DCD SET");
            } else {
                APPL_TRACE_EVENT0("DCD     CLR");
            }
        }

        result[0] = signals;
        result[1] = values;

        res = ioctl(fd, IOCTL_BTPORT_HANDLE_MCTRL, result);

        if (res < 0)
        {
            APPL_TRACE_ERROR2("ioctl failed with reason %s (%d)", strerror(errno), res);

            if (spp_is_reserved_app_id(app_id))
            {
                return;
            }
            /* this means most likely that modem application has not yet opened the port. to keep
             * remote application happy, flow control remote! to unblock this, it important that btport
             * open from application side sets the flow control signals again!! */
            p_cb->tx_data_enable = FALSE;
            bta_dg_ci_control( port_handle, BTA_DG_DTRDSR|BTA_DG_RTSCTS, BTA_DG_DTRDSR_ON|BTA_DG_RTSCTS_OFF );
            signals |= (BTA_DG_DTRDSR|BTA_DG_RTSCTS);
            values = (values & ~(BTA_DG_DTRDSR|BTA_DG_RTSCTS)) | (BTA_DG_DTRDSR_ON|BTA_DG_RTSCTS_OFF);
        }

        /* update application modem control state */
        p_cb->modem_ctrl = (p_cb->modem_ctrl & (~signals)) | values;
        /* TODO: is generic, should be used for ALL services! */
        btapp_dg_btlif_send_modem_ctrl( app_id, signals, values );
    }
#endif  /* #if( defined DUN_USE_SMD_INTERFACE ) && (DUN_USE_SMD_INTERFACE == TRUE) */

}
