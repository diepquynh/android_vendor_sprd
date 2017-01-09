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
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#include <ctype.h>

#include <sys/select.h>
#include <sys/poll.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cutils/sockets.h>

#include "btl_ifc.h"
#include "btl_ifc_wrapper.h"

#define LOG_TAG "BTL_IFC_WRP"

#define DUMP_WSOCK(msg, ws) //if (ws) //debug(" %s: (%d:%d), pend : disc %d, asnc %d, selct %d, flags %x",  msg, ws->wsock, ws->listen_fd, ws->disconnect_pending, ws->async_msg_pnd, ws->select_pending, ws->flags);

typedef struct {
    char *name;
    t_wsock ws[SOCKET_USER_MAX];
} t_wrapper;

static t_wrapper wcb[BTL_IF_SUBSYSTEM_MAX];

#define DATAPATH_MAX_RX_SIZE 2048
static char rx_data_buf[DATAPATH_MAX_RX_SIZE];

static int active_max_fd;
static fd_set active_set;
static tBTL_ListNode ws_active_list;
static pthread_mutex_t activeset_mutex = PTHREAD_MUTEX_INITIALIZER;
#define ACTSET_MUTEX_LOCK() pthread_mutex_lock(&activeset_mutex);
#define ACTSET_MUTEX_UNLOCK() pthread_mutex_unlock(&activeset_mutex);

void hex_dump(char *msg, void *data, int size, int trunc)
{
    unsigned char *p = data;
    unsigned char c;
    int n;
    char bytestr[4] = {0};
    char addrstr[10] = {0};
    char hexstr[ 16*3 + 5] = {0};
    char charstr[16*1 + 5] = {0};

    //verbose("\t%s  ", msg);

    /* truncate */
    if(trunc && (size>16))
        size = 16;

    for(n=1;n<=size;n++) {
        if (n%16 == 1) {
            /* store address for this line */
            snprintf(addrstr, sizeof(addrstr), "%.4x",
               ((unsigned int)p-(unsigned int)data) );
        }

        c = *p;
        if (isalnum(c) == 0) {
            c = '.';
        }

        /* store hex str (for left side) */
        snprintf(bytestr, sizeof(bytestr), "%02X ", *p);
        strncat(hexstr, bytestr, sizeof(hexstr)-strlen(hexstr)-1);

        /* store char str (for right side) */
        snprintf(bytestr, sizeof(bytestr), "%c", c);
        strncat(charstr, bytestr, sizeof(charstr)-strlen(charstr)-1);

        if(n%16 == 0) {
            /* line completed */
            //verbose("[%4.4s]   %-50.50s  %s", addrstr, hexstr, charstr);
            hexstr[0] = 0;
            charstr[0] = 0;
        } else if(n%8 == 0) {
            /* half line: add whitespaces */
            strncat(hexstr, "  ", sizeof(hexstr)-strlen(hexstr)-1);
            strncat(charstr, " ", sizeof(charstr)-strlen(charstr)-1);
        }
        p++; /* next byte */
    }

    if (strlen(hexstr) > 0) {
        /* print rest of buffer if not empty */
        //verbose("[%4.4s]   %-50.50s  %s", addrstr, hexstr, charstr);
    }
}

/*******************************************************************************
**
** Description      active fd list helper functions
**
**
*******************************************************************************/

void __wsactive_init(void)
{
    //debug("init active list");
    INIT_LIST_NODE(&ws_active_list);
}

void __wsactive_add(t_wsock *ws,  int s)
{
    //debug("add fd %d to active list", s);

    ACTSET_MUTEX_LOCK();

    /* null ws is allowed to add custom fd:s */
    if (ws)
        list_add_node(&ws->node, &ws_active_list);

    FD_SET(s, &active_set);
    active_max_fd = MAX(s, active_max_fd);

    ACTSET_MUTEX_UNLOCK();
}

void __wsactive_del(t_wsock *ws, int fd)
{
    //debug("delete fd %d from active list [%x]", fd, (int)ws);

    ACTSET_MUTEX_LOCK();

    FD_CLR(fd, &active_set);
    list_del_node(&ws->node);

    ACTSET_MUTEX_UNLOCK();
}

fd_set __wsactive_get(int *p_max_fd)
{
    fd_set set;

    ACTSET_MUTEX_LOCK();
    set = active_set;
    *p_max_fd = active_max_fd;
    ACTSET_MUTEX_UNLOCK();

    return set;
}

void __wsactive_reset(void)
{
    ACTSET_MUTEX_LOCK();
    FD_ZERO(&active_set);
    ACTSET_MUTEX_UNLOCK();
}

BOOLEAN ws_in_active_list(t_wsock *ws)
{
    struct tBTL_ListNode_tag *pos;
    t_wsock *ws_tmp;

    LIST_FOREACH(pos, &ws_active_list)
    {
        ws_tmp = LIST_GET_NODE_OBJ(pos, t_wsock, node);
        if (ws == ws_tmp)
            return TRUE;
    }
    return FALSE;
}

int wrp_add_to_active_set(int s)
{
    t_wsock *ws = wrp_find_wsock(s);

    //debug("fd %d", s);

    if (!ws)
    {
        //error("wrp_add_to_active_set : wsock fd %d not found", s);
        return -1;
    }

    __wsactive_add(ws, s);

    return 0;
}

/* mainly used to add wake up fds to signal set */
int wrp_add_to_active_set_custom_fd(int s)
{
    //debug("fd %d", s);

    __wsactive_add(NULL, s);

    return 0;
}

int wrp_remove_active_set(int s)
{
    t_wsock *ws = wrp_find_wsock(s);

    if (!ws)
        return -1;

    if (ws_in_active_list(ws))
    {
        __wsactive_del(ws, s);
    }
    else
    {
        //error("fd %d not in list", s);
        return -1;
    }
    return 0;
}

fd_set wrp_get_active_set(int *p_max_fd)
{
    return __wsactive_get(p_max_fd);
}

void wrp_reset_active_set(void)
{
    return __wsactive_reset();
}

/*******************************************************************************
**
** Description      active fd list helper functions
**
**
*******************************************************************************/


int wrp_getport(tSUB sub, int sub_port)
{
    switch (sub) {
        case SUB_AG: return (BTLIF_PORT_BASE_AG + sub_port);
        case SUB_SCO: return (BTLIF_PORT_BASE_SCO + sub_port);
        case SUB_AV: return (BTLIF_PORT_BASE_AV + sub_port);
        case SUB_FM: return (BTLIF_PORT_BASE_FM + sub_port);
        case SUB_DTUN: return (BTLIF_PORT_BASE_DTUN + sub_port);
        case SUB_PBS: return (BTLIF_PORT_BASE_PBS + sub_port);
        case SUB_FTPS: return (BTLIF_PORT_BASE_FTPS + sub_port);
        case SUB_DUN: return (BTLIF_PORT_BASE_DUN + sub_port);
        case SUB_HH: return (BTLIF_PORT_BASE_HH + sub_port);
        case SUB_SPP: return (BTLIF_PORT_BASE_SPP + sub_port);
        case SUB_TEST: return (BTLIF_PORT_BASE_TEST + sub_port);
        case SUB_SAPS: return (BTLIF_PORT_BASE_SAPS + sub_port);
        case SUB_BTS: return (BTLIF_PORT_BASE_BTS + sub_port);

        default:
            //error("unsupported subsystem %d", sub);
            return BTLIF_PORT_INVALID;
    }
}

int wrp_getsubport(t_wsock *ws)
{
    if (!ws)
        return BTLIF_PORT_INVALID;

    switch(ws->sub)
    {
        case SUB_AG: return (ws->port - BTLIF_PORT_BASE_AG);
        case SUB_SCO: return (ws->port - BTLIF_PORT_BASE_SCO);
        case SUB_AV: return (ws->port - BTLIF_PORT_BASE_AV);
        case SUB_FM: return (ws->port - BTLIF_PORT_BASE_FM);
        case SUB_DTUN: return (ws->port - BTLIF_PORT_BASE_DTUN);
        case SUB_PBS: return (ws->port - BTLIF_PORT_BASE_PBS);
        case SUB_FTPS: return (ws->port - BTLIF_PORT_BASE_FTPS);
        case SUB_DUN: return (ws->port - BTLIF_PORT_BASE_DUN);
        case SUB_HH: return (ws->port - BTLIF_PORT_BASE_HH);
        case SUB_SPP: return (ws->port - BTLIF_PORT_BASE_SPP);
        case SUB_TEST: return (ws->port - BTLIF_PORT_BASE_TEST);
        case SUB_SAPS: return (ws->port - BTLIF_PORT_BASE_SAPS);
        case SUB_BTS: return (ws->port - BTLIF_PORT_BASE_BTS);

        default:
            //error("unsupported subsystem");
            return BTLIF_PORT_INVALID;
    }
}

/*******************************************************************************
**
**  Socket mapping helper functions
**
*******************************************************************************/

// fixme -- add dynamically allocated wrapper control blocks + linked list

t_wsock* wrp_alloc_new_sock(int sub)
{
    int i;
    t_wsock *ws;

    //debug("wrp_alloc_new_sock sub %d", sub);

    for (i=0; i<SOCKET_USER_MAX;i++)
    {
        ws = &wcb[sub].ws[i];

        if (ws->busy == 0)
        {
            ws->busy = 1;
            ws->sub = sub; // set backreference for convenience
            ws->async_msg_pnd = 0;
            INIT_LIST_NODE(&ws->node);
            INIT_LIST_NODE(&ws->async_node);
            return ws;
       }
    }
    return NULL;
}



t_wsock* wrp_find_wsock(int s)
{
    int i;
    int j;
    t_wsock *ws;

    if (s == DATA_SOCKET_INVALID)
    {
        //error("invalid hdl %d",  s);
        return NULL;
    }

    //verbose("s:%d",  s);

    for (i=0; i<BTL_IF_SUBSYSTEM_MAX; i++)
    {
        for (j=0; j<SOCKET_USER_MAX; j++)
        {
            ws = &wcb[i].ws[j];
            if ((ws->wsock == s) || (ws->listen_fd == s))
            {
               //verbose("found entry !");
               return ws;
            }
        }
    }
    //info("no entry found");
    return NULL;
}

t_wsock* wrp_find_wsock_by_rfhdl(unsigned short rf_chan, BOOLEAN is_listener)
{
    int i;
    int j;
    t_wsock *ws;

    for (i=0; i<BTL_IF_SUBSYSTEM_MAX; i++)
    {
        for (j=0; j<SOCKET_USER_MAX; j++)
        {
            ws = &wcb[i].ws[j];

            if (ws->rf_chan == rf_chan)
            {
                if (is_listener && (ws->listen_fd != DATA_SOCKET_INVALID))
                {
                    //debug("wrp_find_wsock_by_rfhdl sub : found entry (listener)");
                    return ws;
                }
                else if (!is_listener &&  (ws->wsock != DATA_SOCKET_INVALID))
                {
                    //debug("wrp_find_wsock_by_rfhdl sub : found entry (data channel)");
                    return ws;
                }
            }
        }
    }
    //info("wrp_find_wsock_by_rfhdl : no entry found, lst %d, rfchan %d", is_listener, rf_chan);
    return NULL;
}


void wrp_wsock_init(t_wsock *ws)
{
    ws->busy = 0;
    ws->wsock = DATA_SOCKET_INVALID;
    ws->listen_fd = DATA_SOCKET_INVALID;
    ws->sub = BTL_IF_SUBSYSTEM_MAX;
    ws->flags = 0;
    ws->rf_chan = 0;
    ws->name = "";
    ws->disconnect_pending = 0;
    ws->rx_buf = NULL;
    ws->rx_buf_size = 0;
    ws->rx_buf_pending = 0;
    ws->rx_flow = 1;
}

void wrp_init(void)
{
    int i;
    int j;

    __wsactive_init();

    for (i=0; i<BTL_IF_SUBSYSTEM_MAX; i++)
    {
        for (j=0; j<SOCKET_USER_MAX; j++)
        {
            wrp_wsock_init(&wcb[i].ws[j]);
        }
    }
}

/*******************************************************************************
**
**  Local unix socket helper functions
**
**  Maps custom sockets with unix sockets
**
**
*******************************************************************************/

t_wsock* wrp_wsock_create(int sub)
{
    int fd;
    t_wsock *ws;

    //debug("%s", sub2str[sub]);

    ws = wrp_alloc_new_sock(sub);

    if (ws==NULL)
    {
        //error("wrp_sock_create : out of wsock blocks");
        return NULL;
    }
#ifdef DTUN_LOCAL_SERVER_ADDR
    fd = socket(AF_LOCAL, SOCK_STREAM, 0);
#else
    fd = socket(AF_INET, SOCK_STREAM, 0);
#endif

    if (fd < 0)
    {
        //error("socket create failed (%s)", str//error(errno));
        return NULL;
    }

    ws->wsock = fd;

    //debug(" ws->listen_fd %d, ws->wsock: %d", ws->listen_fd, ws->wsock);

    return ws;
}


/* remove later */
int wrp_sock_create(int sub)
{
    int fd;
    t_wsock *ws;

    //debug("%s", sub2str[sub]);

    ws = wrp_alloc_new_sock(sub);

    if (ws==NULL)
    {
        //error("wrp_sock_create : out of wsock blocks");
        return -1;
    }

#ifdef DTUN_LOCAL_SERVER_ADDR
    fd = socket(AF_LOCAL, SOCK_STREAM, 0);
#else
    fd = socket(AF_INET, SOCK_STREAM, 0);
#endif

    if (fd<0)
    {
        //error("socket create failed (%s)", str//error(errno));
    }

    ws->wsock = fd;

    //debug("%d", fd);

    return fd;
}

#define LOGD
int wrp_sock_bind(t_wsock* ws, int s, char *name, int port)
{
    socklen_t len;
    int result;
    int n = 1;

    //debug("socket %d [%s], bts port %d", s, ws->name, port);

    ws->name = name;
    ws->port = port;

    bzero(&ws->address, sizeof(ws->address));
#ifdef DTUN_LOCAL_SERVER_ADDR
    char local_server_name[UNIX_PATH_MAX];
    DTUN_MAKE_LOCAL_SERVER_NAME(local_server_name, name, port);
    result = socket_local_server_bind(s, local_server_name, ANDROID_SOCKET_NAMESPACE_ABSTRACT) >= 0 ? 0 : -1;

    //debug("result:%d server_name:%s\n", result, local_server_name);

    ws->address.sin_family      = AF_LOCAL;
    ws->address.sin_addr.s_addr = htonl(INADDR_ANY);
    ws->address.sin_port        = htons(ws->port);
    len = sizeof(ws->address);

    //debug("socket_local_server_bind on port %d", ws->port);
#else
    ws->address.sin_family      = AF_INET;
    ws->address.sin_addr.s_addr = htonl(INADDR_ANY);
    ws->address.sin_port        = htons(ws->port);
    len = sizeof(ws->address);

    //debug("wrp_sock_bind on port %d", ws->port);

    /* allow reuse of sock addr upon bind */
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &n, sizeof(n));

    result = bind(s, (struct sockaddr*)&ws->address, len);
#endif

    //if (result < 0)
       //error("bind failed (%s)", str//error(errno));

    return result;
}

int wrp_sock_accept(t_wsock* ws, int s)
{
    int sock_new;
    socklen_t len = sizeof(SA_TYPE);
    tBTL_PARAMS param;

#ifdef DTUN_LOCAL_SERVER_ADDR
    //debug("local server name:%s, port:%d, fd:%d", ws->name, (int)ws->port, s);

    do {
        len = sizeof(ws->address);
        sock_new = accept(s, (struct sockaddr *)&(ws->address), &len);
    } while (sock_new < 0 && errno == EINTR);

    //debug("wrp_sock_accept accept local socket:%d, errno:%d\n", sock_new, errno);
#else
    //debug("%s (%d)", ws->name, s);
    sock_new = accept(s, (struct sockaddr *)&ws->address, &len);
#endif

    if (sock_new < 0)
    {
        //error("failed (%s)", str//error(errno));
        return DATA_SOCKET_INVALID;
    }

    //debug("new fd %d", sock_new);

    /* remember listener */
    ws->listen_fd = s;

    /* assign data socket */
    ws->wsock = sock_new;

    /* notify user via ctrl callback */
    param.chan_ind.handle = sock_new;
    param.chan_ind.subport = wrp_getsubport(ws);

    btl_ifc_notify_local_event(ws->sub, BTLIF_DATA_CHAN_IND, &param);

    return sock_new;
}

int wrp_sock_listen(t_wsock* ws, int s)
{
    int ret;

    //debug("wrp_sock_listen : fd %d %s:%d", s, ws->name, ws->port);

    /* remember listen fd */
    ws->listen_fd = s;

    ret = listen(s, BACKLOG_DEFAULT);

    //if (ret < 0)
        //error("listen failed (%s)", str//error(errno));

    return ret;
}

int wrp_sock_listen_bl(t_wsock* ws, int s, int backlog)
{
    int ret;

    //debug("wrp_sock_listen_bl : fd %d %s:%d backlog %d", s, ws->name, ws->port, backlog);

    /* remember listen fd */
    ws->listen_fd = s;

    ret = listen(s, backlog);

    //if (ret<0)
        //error("listen failed (%s)", str//error(errno));

    return ret;
}


void set_blocking(int s)
{
    int opts;
    opts = fcntl(s, F_GETFL);
    if (opts<0) error("set blocking F_GETFL fail(%s)", strerror(errno));
    opts &= ~O_NONBLOCK;
    opts = fcntl(s, F_SETFL, opts);
    if (opts<0) error("set blocking F_SETFL fail(%s)", strerror(errno));
}

int wrp_sock_connect(t_wsock* ws, int s, char *name, int port)
{
    SA_TYPE address;
    int         len;
    int         result;

    //verbose("s %d", s);

    ws->name = name;
    ws->port = port;

    //debug("wrp_sock_connect %s:%d (%d)", ws->name, ws->port, s);

    bzero(&address, sizeof(address));
#ifdef DTUN_LOCAL_SERVER_ADDR
    address.sin_family = AF_LOCAL;
    address.sin_port = htons(ws->port);
    len = sizeof(address);

    set_blocking(s);
    char local_server_name[UNIX_PATH_MAX];
    DTUN_MAKE_LOCAL_SERVER_NAME(local_server_name, name, port);
    result = socket_local_client_connect(s, local_server_name, ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM) >= 0 ? 0 : -1;

    //debug("result %d, server name:%s\n", result, local_server_name);
#else
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ws->name);
    address.sin_port = htons(ws->port);
    len = sizeof(address);

    /* we need to temporarily set the socket to blocking to guarantee operation */
    set_blocking(s);

    result = connect(s, (struct sockaddr*)&address, len);
#endif

    if(result == -1)
    {
        //error("connect failed (%s)", str//error(errno));
        return -1;
    }
    else
        //info("Connected. (%d)", s);

    return s;
}


/* close either data or listener socket, remove wsock when both are closed
     Return TRUE means full socket was closed */
BOOLEAN wrp_close_s_only(t_wsock *ws, int s)
{
    //debug("wrp_close_s_only [%d] (%d:%d) [%s]", s, ws->wsock, ws->listen_fd, ws->name);

    if (s == ws->wsock)
    {
        //debug("data socket closed");
        __wsactive_del(ws, s);
        close(ws->wsock);
        ws->wsock = DATA_SOCKET_INVALID;
    }

    if (s == ws->listen_fd)
    {
        //debug("listen socket closed");
        __wsactive_del(ws, ws->listen_fd);
        close(ws->listen_fd);
        ws->listen_fd = DATA_SOCKET_INVALID;
    }

    if ((ws->listen_fd == DATA_SOCKET_INVALID) && (ws->wsock == DATA_SOCKET_INVALID))
    {
        //debug("wsock fully closed, return to pool");
        wrp_wsock_init(ws);
        return TRUE;
    }

    return FALSE;
}


void wrp_close_full(t_wsock *ws)
{
    //debug("wrp_close (%d:%d) [%s]", ws->wsock, ws->listen_fd, ws->name);

    __wsactive_del(ws, ws->wsock);

    close(ws->wsock);

    if (ws->listen_fd != DATA_SOCKET_INVALID)
    {
       __wsactive_del(ws, ws->listen_fd);
       close(ws->listen_fd);
    }

    wrp_wsock_init(ws);
}


void wrp_close_sub_all(int sub)
{
    int i;
    t_wsock *ws;

    //debug("wrp_close_sub_all sub %d", sub);

    for (i=0; i<SOCKET_USER_MAX;i++)
    {
        ws = &wcb[sub].ws[i];

        if (ws->busy == 1)
        {
             wrp_close_full (ws);
        }
    }
}

void wrp_setup_rxbuf(t_wsock *ws, char *p, int size)
{
    ws->rx_buf = p;
    ws->rx_buf_size = size;
    ws->rx_buf_pending = 1;
}


/* may be called from multiple thread contexts */
void wrp_setup_rxflow(t_wsock *ws, BOOLEAN flow_on)
{
    /* only process state changes */
    if (flow_on == ws->rx_flow)
        return;

    //debug("wrp_setup_rxflow : %d (fd %d)", flow_on, ws->wsock);

    ws->rx_flow = flow_on;

    if (flow_on == 0)
        wrp_remove_active_set(ws->wsock);
    else
        wrp_add_to_active_set(ws->wsock);
}

void notify_data_ch_disc_ind(t_wsock *ws)
{
    tBTL_PARAMS param;
    tSUBPORT subport;

    DUMP_WSOCK("notify_data_ch_disc_ind", ws);

    if (!ws)
    {
        //error("//error : null wsock !");
        return;
    }

    subport = wrp_getsubport(ws);

    if (subport == BTLIF_PORT_INVALID)
    {
       DUMP_WSOCK("notify_data_ch_disc : failed to find subport", ws);
       return;
    }

    param.chan_ind.handle = ws->wsock;
    param.chan_ind.subport = subport;

    btl_ifc_notify_local_event(ws->sub, BTLIF_DATA_CHAN_DISC_IND, (tBTL_PARAMS*)&param);
}

static int check_data_sock(t_wsock *ws, fd_set *p_read_set)
{
    if ((ws->wsock !=DATA_SOCKET_INVALID) && (FD_ISSET(ws->wsock, p_read_set)))
    {
        int len;

        //verbose("[DATA] Received [%s] on fd %d (rx len %d)", ws->name, ws->wsock, ws->rx_buf_size);

        /* sanity check */
        if (ws->rx_flow==0)
        {
            //info("warning : received data although flow is off");
        }

        if (ws->rx_buf && !ws->rx_buf_size)
        {
            //error("warning : dedicated rx buf is set to 0 bytes");
        }

        /* check for dedicated rx buffer, if none use static */
        if (ws->rx_buf)
            len = recv(ws->wsock, ws->rx_buf, ws->rx_buf_size, MSG_DONTWAIT);
        else
            len = recv(ws->wsock, rx_data_buf, DATAPATH_MAX_RX_SIZE, MSG_DONTWAIT);

        if (len == 0)
        {
            //info("[DATA] Channel disconnected [%s]", ws->name);
            notify_data_ch_disc_ind(ws);
            wrp_close_s_only(ws, ws->wsock);
        }
        else if (len < 0)
        {
            //error("[DATA] Read //error (%s)", str//error(errno));
            notify_data_ch_disc_ind(ws);
            wrp_close_s_only(ws, ws->wsock);
        }
        else
        {
            if (ws->rx_buf)
            {
                /* reset rx buf pending to indicate buffer has been delivered back to user */
                ws->rx_buf_pending = 0;

                btl_ifc_rxdata(ws->sub, ws->wsock, ws->rx_buf, len);
            }
            else
            {
                btl_ifc_rxdata(ws->sub, ws->wsock, rx_data_buf, len);
            }
        }
        return 1;
    }
    else
    {
        return 0;
    }
}


/* returns 1 if fd was processed */
int wrp_check_fdset(t_wsock *ws, fd_set *p_read_set)
{
    if ((ws->listen_fd !=DATA_SOCKET_INVALID) && (FD_ISSET(ws->listen_fd, p_read_set)))
    {
        tDATA_HANDLE hdl;

        //info("[DATA] Incoming connection on [%s] (%d)", ws->name, ws->listen_fd);

        hdl = wrp_sock_accept(ws, ws->listen_fd);

        /* when using BTL IFC with data and ctrl callbacks  ifc wrapper will automatically close the listener upon accept */
        //debug("auto closing listener socket fd (%d)", ws->listen_fd);

        /* remove from active list */
        __wsactive_del(ws, ws->listen_fd);

        /* close listener */
        wrp_close_s_only(ws, ws->listen_fd);
        return 1;
    }
    else
    {
        return check_data_sock(ws, p_read_set);
    }
}


/* returns nbr of processed fds */
int wrp_check_active_handles(fd_set *p_read_set)
{
    t_wsock *ws;
    struct tBTL_ListNode_tag *pos;
    int fds_processed = 0;

    LIST_FOREACH(pos, &ws_active_list)
    {
        ws = LIST_GET_NODE_OBJ(pos, t_wsock, node);

        fds_processed += wrp_check_fdset(ws, p_read_set);
    }
    return fds_processed;
}



