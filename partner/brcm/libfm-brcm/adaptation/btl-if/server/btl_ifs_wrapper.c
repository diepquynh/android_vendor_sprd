/************************************************************************************
 *
 *  Copyright (C) 2009-2010 Broadcom Corporation
 *
 *  This program is the proprietary software of Broadcom Corporation and/or its
 *  licensors, and may only be used, duplicated, modified or distributed
 *  pursuant to the terms and conditions of a separate, written license
 *  agreement executed between you and Broadcom (an "Authorized License").
 *  Except as set forth in an Authorized License, Broadcom grants no license
 *  (express or implied), right to use, or waiver of any kind with respect to
 *  the Software, and Broadcom expressly reserves all rights in and to the
 *  Software and all intellectual property rights therein.
 *  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS
 *  SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
 *  ALL USE OF THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1.     This program, including its structure, sequence and organization,
 *         constitutes the valuable trade secrets of Broadcom, and you shall
 *         use all reasonable efforts to protect the confidentiality thereof,
 *         and to use this information only in connection with your use of
 *         Broadcom integrated circuit products.
 *
 *  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 *         "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 *         REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
 *         OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 *         DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 *         NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 *         ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 *         CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT
 *         OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *         ITS LICENSORS BE LIABLE FOR
 *         (i)   CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY
 *               DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO
 *               YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *               HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR
 *         (ii)  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
 *               SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE
 *               LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF
 *               ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
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
#include <ctype.h>
#include "btl_ifs_wrapper.h"

#ifndef LINUX_NATIVE
#include <cutils/sockets.h>
#endif

#define LOG_TAG "BTL-IFS-WRAPPER"

#include "btl_ifs.h"

/* static receive buffer (used when no dedicated rx buf is setup) */
#define RX_BUFFER_SIZE 2048

static char rx_data_buf[RX_BUFFER_SIZE];

/* only subsystems that uses multiple data connections needs a dynamic port map */
typedef struct
{
    int bts_portmap[BTS_CHAN_MAX];
} tBTLIF_DYNAMIC_PORT_MAP;

static tBTLIF_DYNAMIC_PORT_MAP port_map;


//#define debug info

#define DUMP_WSOCK(msg, ws) if (ws) debug("[%s] %s: %s sub %d (%d:%d)", __FUNCTION__, msg, ws->name, ws->subsystem,  \
                                            ws->wsock, ws->listen_fd);

pthread_mutex_t activeset_mutex = PTHREAD_MUTEX_INITIALIZER;
#define ACTSET_MUTEX_LOCK() pthread_mutex_lock(&activeset_mutex);
#define ACTSET_MUTEX_UNLOCK() pthread_mutex_unlock(&activeset_mutex);

/*******************************************************************************
**
** Socket wrapper
**
**
*******************************************************************************/

typedef struct {
    t_wsock ws[SOCKET_USER_MAX];
} t_wrapper;

static t_wrapper wcb_main[BTL_IF_SUBSYSTEM_MAX];
static tBTL_ListNode ws_data_list;
static tBTL_ListNode ws_listen_list;

/*******************************************************************************
**
** Socket Rx/Tx
**
**
*******************************************************************************/

void hex_dump(char *msg, void *data, int size, int trunc)
{
    unsigned char *p = data;
    unsigned char c;
    int n;
    char bytestr[4] = {0};
    char addrstr[10] = {0};
    char hexstr[ 16*3 + 5] = {0};
    char charstr[16*1 + 5] = {0};

    verbose("%s ", msg);

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
            verbose("[%4.4s]   %-50.50s  %s", addrstr, hexstr, charstr);
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
        verbose("[%4.4s]   %-50.50s  %s", addrstr, hexstr, charstr);
    }
}


int tx_data(int fd, char *p, int len, int non_sock)
{
    int result;

    //hex_dump("[BTL IFS TX]", p, len, 0)

    if (non_sock)
        result = write(fd, p, len);
    else
        result = send(fd, p, len, 0);

    if (result != len)
    {
         error("failed : %s (%d)", strerror(errno), result);
         return -1;
    }

    return result;
}

int rx_data(int fd, char *p, int len)
{
    int ret;

    ret = recv(fd, p, len, MSG_WAITALL);

    //hex_dump("[BTL IFS RX]", p, len, 0);

    if (ret == 0)
    {
        info("socket disconnected.");
    }
    else if (ret < 0)
    {
        error("failed (%s)", strerror(errno));
    }
    return ret;
}


/*******************************************************************************
**
**
**
**
*******************************************************************************/


void wrp_init_dynamicportmapper(void)
{
    memset(&port_map, 0, sizeof(port_map));
}

/* both listeners and client connections needs to allocate a unique port to be used
   for the btlif data connection */
int wrp_alloc_dynamicport(tSUB sub, int sock)
{
    int port_max;
    int port_base;
    int *p_port;
    int i = 0;

    switch (sub)
    {
        case SUB_BTS:
            p_port = port_map.bts_portmap;
            port_base = BTLIF_PORT_BASE_BTS;
            port_max = BTS_CHAN_MAX;
            break;

        default:
            error("subsystem [%s] does not support dynamic port mapping", sub2str[sub]);
            return -1;
    }

    /* search available ports in portmap */
    while (p_port[i] && i<port_max) i++;

    if (i < port_max)
    {
        /* we found a free port, set as allocated */
        p_port[i] = sock;
        debug("allocated port %d (s:%d) (sub %d)", port_base+i, sock, i);
        return port_base+i;
    }

    return -1;
}

void wrp_free_dynamicport(tSUB sub, int port)
{
    int port_max;
    int port_base;
    int *p_port;

    switch (sub)
    {
        case SUB_BTS:
            p_port = port_map.bts_portmap;
            port_base = BTLIF_PORT_BASE_BTS;
            port_max = BTS_CHAN_MAX;
            break;

        default:
            error("subsystem does not support dynamic port mapping");
            return;
    }

    debug("free dyn port %d (s:%d), sub %d", port, p_port[port-port_base], port-port_base);

    p_port[port - port_base] = 0;
}



int wrp_getport(tSUB sub, int sub_port)
{
    switch (sub)
    {
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
            error("unsupported subsystem");
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
            error("unsupported subsystem");
            return BTLIF_PORT_INVALID;
    }


}


///////////////////////////////////////////////////////////////////////////////


void ws_listener_add(t_wsock *ws, int add_active_set)
{
    verbose("ws_listener_add (%d), active set %x", ws->listen_fd, add_active_set);
    list_add_node(&ws->lstnode, &ws_listen_list);

    if (add_active_set)
    {
        FD_SET(ws->listen_fd, &active_set);
        max_fd = MAX(max_fd,ws->listen_fd);
    }
}

void ws_listener_del(t_wsock *ws)
{
    verbose("%s [%s]", __FUNCTION__, sub2str[ws->sub]);

    if (ws->listen_fd == DATA_SOCKET_INVALID)
    {
        error("trying to delete non existing listener");
        return;
    }

    list_del_node(&ws->lstnode);
    FD_CLR(ws->listen_fd, &active_set);

}

static BOOLEAN ws_in_listener_list(t_wsock *ws)
{
    struct tBTL_ListNode_tag *pos;
    t_wsock *tmp;

    LIST_FOREACH(pos, &ws_listen_list)
    {
        tmp = LIST_GET_NODE_OBJ(pos, t_wsock, lstnode);
        if (ws == tmp)
            return TRUE;
    }
    return FALSE;
}


void ws_del_sub_listeners(tSUB sub)
{
    struct tBTL_ListNode_tag *pos;
    t_wsock *ws_tmp;

    verbose("%s [%s]", __FUNCTION__, sub2str[sub]);

    LIST_FOREACH(pos, &ws_listen_list)
    {
        ws_tmp = LIST_GET_NODE_OBJ(pos, t_wsock, lstnode);

        if (ws_tmp->sub == sub)
        {
            ws_listener_del(ws_tmp);
            if (ws_tmp->listen_fd != DATA_SOCKET_INVALID) {
                close(ws_tmp->listen_fd);
            }
            ws_tmp->listen_fd = DATA_SOCKET_INVALID;
        }
    }
}

void ws_close_all_listeners(void)
{
    struct tBTL_ListNode_tag *pos;
    t_wsock *ws_tmp;

    if (list_isempty(&ws_listen_list))
    {
        verbose("%s : no listeners", __FUNCTION__);
        return;
    }

    LIST_FOREACH(pos, &ws_listen_list)
    {
        ws_tmp = LIST_GET_NODE_OBJ(pos, t_wsock, lstnode);

        verbose("Closed listener [%s] (%d)", sub2str[ws_tmp->sub], ws_tmp->listen_fd);
        ws_listener_del(ws_tmp);
        if (ws_tmp->listen_fd != DATA_SOCKET_INVALID) {
            close(ws_tmp->listen_fd);
        }
        ws_tmp->listen_fd = DATA_SOCKET_INVALID;
    }
}


static void wsactive_init(void)
{
    verbose("init active list");
    INIT_LIST_NODE(&ws_data_list);
    INIT_LIST_NODE(&ws_listen_list);

    wrp_init_dynamicportmapper();
}

static void wsactive_add(t_wsock *ws, int fd)
{
    debug("add wsock %d to active list [%x]", fd, (int)ws);

    list_add_node(&ws->node, &ws_data_list);
    FD_SET(fd, &active_set);
    max_fd = MAX(max_fd,fd);
}

void wsactive_del(t_wsock *ws, int fd)
{
    debug("delete wsock %d from active list [%x]", fd, (int)ws);

    FD_CLR(fd, &active_set);
    list_del_node(&ws->node);
}

static BOOLEAN ws_in_active_list(t_wsock *ws)
{
    struct tBTL_ListNode_tag *pos;
    t_wsock *ws_tmp;

    LIST_FOREACH(pos, &ws_data_list)
    {
        ws_tmp = LIST_GET_NODE_OBJ(pos, t_wsock, node);
        if (ws == ws_tmp)
            return TRUE;
    }
    return FALSE;
}

static t_wsock* wrp_alloc_new_sock(int fdc, int sub)
{
    int i;
    t_wsock *ws;

    verbose("wrp_alloc_new_sock hdl %d, sub [%s]", fdc, sub2str[sub]);

    for (i=0; i<SOCKET_USER_MAX;i++)
    {
        ws = &wcb_main[sub].ws[i];

        if (ws->busy == 0)
        {
            ws->ctrl_fd = fdc;
            ws->busy = 1;
            ws->sub = sub; // set backreference for convenience
            INIT_LIST_NODE(&ws->node);
            INIT_LIST_NODE(&ws->lstnode);
            return ws;
       }
    }
    return NULL;
}

/* fixme -- use linked lists instead  */
t_wsock* wrp_find_wsock(int s)
{
    int i;
    int j;
    t_wsock *ws;

    if (s == DATA_SOCKET_INVALID)
    {
        error("%s invalid hdl %d", __FUNCTION__, s);
        return NULL;
    }

    verbose("wrp_find_wsock s:%d", s);

    for (i=0; i<BTL_IF_SUBSYSTEM_MAX; i++)
    {
        for (j=0; j<SOCKET_USER_MAX; j++)
        {
            ws = &wcb_main[i].ws[j];
            if (ws->wsock == s)
            {
               verbose("wrp_find_wsock : found entry !");
               return ws;
            }
        }
    }
    verbose("wrp_find_wsock : no entry found");
    return NULL;
}

/* fixme -- use generic search by key function */
t_wsock* wrp_find_wsock_by_port(int sub, int sub_port)
{
    int i;
    int j;
    t_wsock *ws;
    int port = wrp_getport(sub, sub_port);

    verbose("wrp_find_wsock_by_port sub s:%d, subport %d", sub, sub_port);

    for (j=0; j<SOCKET_USER_MAX; j++)
    {
        ws = &wcb_main[sub].ws[j];
        if (ws->port == port)
        {
           verbose("wrp_find_wsock_by_port : found entry !");
           return ws;
        }
    }

    verbose("wrp_find_wsock_by_port : no entry found");
    return NULL;
}

void ws_close_all_active(void)
{
    struct tBTL_ListNode_tag *pos;
    t_wsock *ws;

    LIST_FOREACH(pos, &ws_data_list)
    {
        ws = LIST_GET_NODE_OBJ(pos, t_wsock, node);
        wrp_close_data(ws->wsock);
    }
}


int wrp_add_to_active_set(int s)
{
    t_wsock *ws = wrp_find_wsock(s);

    info("wrp_add_to_active_set %d", s);

    if (!ws)
    {
        error("wrp_add_to_active_set : wsock fd %d not found", s);
        return -1;
    }

    wsactive_add(ws, s);

    return 0;
}

int wrp_remove_active_set(int s)
{
    t_wsock *ws = wrp_find_wsock(s);

    if (!ws)
    {
        info("wrp_remove_active_set : wsock fd %d not found", s);
        return -1;
    }

    if (ws_in_active_list(ws))
    {
        wsactive_del(ws, s);
    }
    else
    {
        error("wrp_remove_active_set : %d not in list", s);
        return -1;
    }
    return 0;
}


void wrp_wsock_free(t_wsock *ws)
{
    ACTSET_MUTEX_LOCK();

    ws->ctrl_fd = DATA_SOCKET_INVALID;
    ws->busy = 0;
    ws->wsock = DATA_SOCKET_INVALID;
    ws->listen_fd = DATA_SOCKET_INVALID;
    ws->sub = BTL_IF_SUBSYSTEM_MAX;
    ws->name = "";
    ws->port = 0;
    ws->rx_buf = NULL;
    ws->rx_buf_size = 0;
    ws->rx_flow = 1;
    ws->rx_buf_pending = 0;
    ws->non_sock = 0;

    ACTSET_MUTEX_UNLOCK();
}

void wrp_sock_init(void)
{
    int i;
    int j;

    verbose("wrp_sock_init");

    wsactive_init();

    for (i=0; i<BTL_IF_SUBSYSTEM_MAX; i++)
    {
        for (j=0; j<SOCKET_USER_MAX; j++)
        {
            wrp_wsock_free(&wcb_main[i].ws[j]);
        }
    }
}

t_wsock *wrp_sock_create_ext(tCTRL_HANDLE handle, tSUB sub, char *name, int port, int ext_fd)
{
    int fd;
    t_wsock *ws;

    ws = wrp_alloc_new_sock(handle, sub);

    if (ws==NULL)
    {
        error("wrp_sock_create_ext : out of wsock blocks");
        return NULL;
    }

    ws->wsock = ext_fd;
    ws->non_sock = 1; // this is not a socket
    ws->subsystem = sub;
    ws->name = name;
    ws->port = port;

    info("wrp_sock_create_ext : setup ext fd %d", ext_fd);

    return ws;
}


t_wsock *wrp_sock_create(tCTRL_HANDLE handle, tSUB sub, char *name, int port, int is_listen)
{
    int fd;
    t_wsock *ws;

    ws = wrp_alloc_new_sock(handle, sub);

    if (ws==NULL)
    {
        error("wrp_sock_create : out of wsock blocks");
        return NULL;
    }

#ifdef DTUN_LOCAL_SERVER_ADDR
    fd = socket(AF_LOCAL, SOCK_STREAM, 0);
#else
    fd = socket(AF_INET, SOCK_STREAM, 0);
#endif

    if (fd<0)
    {
        error("socket create failed %s", strerror(errno));
        wrp_wsock_free(ws);
        return NULL;
    }

    if (is_listen)
        ws->listen_fd = fd;
    else
        ws->wsock = fd;

    ws->subsystem = sub;
    ws->name = name;
    ws->port = port;

    info("wrp_sock_create : created socket (fd %d)", fd);

    return ws;
}

int wrp_sock_bind(t_wsock *ws)
{
    int ret;
    int len;
    int n = 1;

    if (ws->listen_fd == DATA_SOCKET_INVALID)
    {
        error("wrp_sock_bind : no listening socket available");
        return -1;
    }

    verbose("wrp_sock_bind : port %d (%d)", ws->port, ws->listen_fd);

    bzero(&ws->address, sizeof(SA_TYPE));
#ifdef DTUN_LOCAL_SERVER_ADDR
    char local_server_name[UNIX_PATH_MAX];
    DTUN_MAKE_LOCAL_SERVER_NAME(local_server_name, DTUN_LOCAL_SERVER_ADDR, ws->port);
    ws->address.sin_family      = AF_LOCAL;
    ws->address.sin_addr.s_addr = htonl(INADDR_ANY);
    ws->address.sin_port        = htons(ws->port);
    ret = socket_local_server_bind(ws->listen_fd, local_server_name, ANDROID_SOCKET_NAMESPACE_ABSTRACT) >= 0 ? 0 : -1;
    debug("btl_ifs:wrp_sock_bind:socket_local_server_bind result:%d server_name:%s\n", ret, local_server_name);
    debug("socket_local_server_bind on port %d", ws->port);
#else
    ws->address.sin_family      = AF_INET;
    ws->address.sin_addr.s_addr = htonl(INADDR_ANY);
    ws->address.sin_port        = htons(ws->port);
    len = sizeof(SA_TYPE);

    /* allow reuse of sock addr upon bind */
    setsockopt(ws->listen_fd, SOL_SOCKET, SO_REUSEADDR, &n, sizeof(n));

    ret = bind(ws->listen_fd, (struct sockaddr*)&ws->address, len);
    debug("socket_local_server_bind on port %d", ws->port);
#endif
    if (ret < 0)
        error("bind failed (%s)", strerror(errno));

    return ret;
}

int wrp_sock_listen(t_wsock *ws)
{
    int ret;

    info("wrp_sock_listen : (%s) listening on %s:%d (%d)", sub2str[ws->subsystem], ws->name, ws->port, ws->listen_fd);

    ret = listen(ws->listen_fd, 5);

    if (ret < 0)
    {
        error("listen failed (%s)", strerror(errno));
        return -1;
    }

    return ws->listen_fd;
}

int wrp_sock_connect(t_wsock *ws)
{
    SA_TYPE address;
    int         len;
    int         result;

    info("wrp_sock_connect : connecting to (%s:%d) (%d)", ws->name, ws->port, ws->wsock);

    if (ws->wsock == DATA_SOCKET_INVALID)
    {
        error("wrp_sock_connect : no socket available");
        return -1;
    }

    bzero(&address, sizeof(address));
#ifdef DTUN_LOCAL_SERVER_ADDR
    address.sin_family = AF_LOCAL;
    //address.sin_addr.s_addr = inet_addr(ws->name);
    address.sin_port = htons(ws->port);
    len = sizeof(address);
    char local_server_name[UNIX_PATH_MAX];
    DTUN_MAKE_LOCAL_SERVER_NAME(local_server_name, ws->name, ws->port);
    result = socket_local_client_connect(ws->wsock, local_server_name, ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM) >= 0 ? 0 : -1;
    printf("btl_ifs:wrp_sock_connect ret:%d server name:%s\n", result, local_server_name);
#else
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ws->name);
    address.sin_port = htons(ws->port);
    len = sizeof(address);

    result = connect(ws->wsock, (struct sockaddr*)&address, len);
#endif
    if(result == -1)
    {
        error("connect failed (%s)", strerror(errno));
        return -1;
    }
    else
        info("Connected. (%d)", ws->wsock);

    return ws->wsock;
}


int wrp_data_accept(t_wsock *ws)
{
#ifdef DTUN_LOCAL_SERVER_ADDR
    union {
        struct sockaddr address;
        struct sockaddr_un un_address;
    } sa;
    socklen_t len;
    tBTL_PARAMS param;

    printf("btl_ifs: calling accept() listen socket:%d\n", ws->listen_fd);
    do {
        len = sizeof(sa);
        ws->wsock = accept(ws->listen_fd, &(sa.address), &len);
    } while (ws->wsock < 0 && errno == EINTR);
    printf("btl_ifs:wrp_data_accept() accept local socket:%d\n", ws->wsock);
#else

    socklen_t clen = sizeof(SA_TYPE);
    SA_TYPE claddr;
    tBTL_PARAMS param;

    info("accept on listen fd %d", ws->listen_fd);

    clen = sizeof(SA_TYPE);

    ws->wsock = accept(ws->listen_fd,
               (struct sockaddr*)&claddr,
               &clen);
#endif
    if (ws->wsock < 0)
    {
        error("accept failed (%s)", strerror(errno));
        return -1;
    }

    /* incoming connection */
    info("[DATA] Connected (%d) [%s]",  ws->wsock, ws->name);

    /* notify user via ctrl callback */
    param.chan_ind.handle = ws->wsock;
    param.chan_ind.subport = wrp_getsubport(ws);

    btl_if_notify_local_event(ws->ctrl_fd, ws->subsystem, BTLIF_DATA_CHAN_IND, &param);

    return ws->wsock;
}


void wrp_close_data(int sock_fd)
{
    t_wsock *ws;

    /* find sub system for this handle */
    ws = wrp_find_wsock(sock_fd);

    if (ws==NULL)
    {
        error("wrapper ctrl block not found");
        return;
    }

    info("wrp_close %d [%s]", ws->wsock, ws->name);

    if (ws->wsock == DATA_SOCKET_INVALID)
    {
        error("wrp_close_data : already closed");
        return;
    }

    wsactive_del(ws, ws->wsock);
    close(ws->wsock);
    wrp_wsock_free(ws);
}

void wrp_close_all(t_wsock *ws)
{
    if (ws==NULL)
    {
        error("wrapper ctrl block not found");
        return;
    }

    if (ws->wsock != DATA_SOCKET_INVALID)
    {
        wsactive_del(ws, ws->wsock);
        close(ws->wsock);
    }

    if (ws->listen_fd != DATA_SOCKET_INVALID)
    {
        ws_listener_del(ws);
        close(ws->listen_fd);
        ws->listen_fd = DATA_SOCKET_INVALID;
    }

    wrp_wsock_free(ws);
}


void wrp_setup_rxbuf(t_wsock *ws, char *p, int size)
{
    ws->rx_buf = p;
    ws->rx_buf_size = size;
    ws->rx_buf_pending = 1;
}

/* called from multiple thread contexts */
void wrp_setup_rxflow(t_wsock *ws, BOOLEAN flow_on)
{
    /* only process state changes */
    if (flow_on == ws->rx_flow)
        return;

    debug("wrp_setup_rxflow : %d (fd %d)", flow_on, ws->wsock);

    ACTSET_MUTEX_LOCK();

    ws->rx_flow = flow_on;

    if (flow_on == 0)
        wrp_remove_active_set(ws->wsock);
    else
        wrp_add_to_active_set(ws->wsock);

    ACTSET_MUTEX_UNLOCK();

}


static void notify_data_ch_disc_ind(t_wsock *ws)
{
    tSUB sub;
    tBTL_PARAMS param;
    tSUBPORT subport;

    DUMP_WSOCK("notify_data_ch_disc_ind", ws);

    if (!ws)
    {
        error("%s, error : null wsock !", __FUNCTION__);
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

    btl_if_notify_rx_buf_pending(ws->wsock);

    btl_if_notify_local_event(ws->ctrl_fd, ws->sub, BTLIF_DATA_CHAN_DISC_IND, &param);
}


static void check_listener_sock(t_wsock *ws)
{
    info("check_listener %x (%d, listen %d)", ws, ws->wsock, ws->listen_fd);

    if (FD_ISSET(ws->listen_fd, &read_set))
    {
        tDATA_HANDLE hdl;

        info("[DATA] Incoming connection on [%s] (%d)", ws->name, ws->listen_fd);

        hdl = wrp_data_accept(ws);

        /* close listen fd */
        ws_del_sub_listeners(ws->sub);
    }

}

static void check_data_sock(t_wsock *ws)
{
    if ((ws->wsock !=DATA_SOCKET_INVALID) && (FD_ISSET(ws->wsock, &read_set)))
    {
        int len;
        char* p_rx;

        verbose("[DATA] Received [%s] on fd %d (flow %d)", ws->name, ws->wsock, ws->rx_flow);

        /* sanity check */
        if (ws->rx_flow==0)
        {
            info("warning : received data although flow is off");
        }

        if (ws->rx_buf && !ws->rx_buf_size)
        {
            error("warning : dedicated rx buf is set to 0 bytes");
        }

        /* check if this fd is not a socket */
        if (!ws->non_sock)
        {
            /* check for dedicated rx buffer, if none use static */
            if (ws->rx_buf)
                len = recv(ws->wsock, ws->rx_buf, ws->rx_buf_size, MSG_DONTWAIT);
            else
                len = recv(ws->wsock, rx_data_buf, RX_BUFFER_SIZE, MSG_DONTWAIT);
        }
        else
        {
            /* check for dedicated rx buffer, if none use static */
            if (ws->rx_buf)
                len = read(ws->wsock, ws->rx_buf, ws->rx_buf_size);
            else
                len = read(ws->wsock, rx_data_buf, RX_BUFFER_SIZE);
        }

        if (len == 0)
        {
            info("[DATA] Channel disconnected [%s]", ws->name);
            notify_data_ch_disc_ind(ws);
            wrp_close_data(ws->wsock);
        }
        else if (len < 0)
        {
            error("[DATA] Read error (%s)", strerror(errno));
            notify_data_ch_disc_ind(ws);
            wrp_close_data(ws->wsock);
        }
        else
        {
            /* now forward the data to the subsystem client */
            if (ws->rx_buf)
            {
                /* reset rx buf pending to indicate buffer has been delivered back to user */
                ws->rx_buf_pending = 0;

                btl_if_rxdata(ws->subsystem, ws->wsock, ws->rx_buf, len);
            }
            else
            {
                btl_if_rxdata(ws->subsystem, ws->wsock, rx_data_buf, len);
            }
        }
    }
}

void wrp_monitor_data_fds(void)
{
    t_wsock *ws;
    struct tBTL_ListNode_tag *pos;

    /* first check for incoming data */
    LIST_FOREACH(pos, &ws_data_list)
    {
        ws = LIST_GET_NODE_OBJ(pos, t_wsock, node);
        check_data_sock(ws);
    }

    /* now check all listeners */
    LIST_FOREACH(pos, &ws_listen_list)
    {
        ws = LIST_GET_NODE_OBJ(pos, t_wsock, lstnode);
        check_listener_sock(ws);
    }
}

t_wsock *wrp_setup_data_listener(tCTRL_HANDLE handle, tSUB sub, int sub_port)
{
    t_wsock *ws;

    verbose("wrp_setup_data_listener [%s] on sub port %d (ctrl_fd %d)", sub2str[sub], sub_port, handle);

    ws = wrp_sock_create(handle, sub, BTLIF_IP_ADDR, wrp_getport(sub, sub_port), 1);

    if (!ws)
    {
        error("wrp_setup_data_listener failed due to out of resources");
        return NULL;
    }

    wrp_sock_bind(ws);

    wrp_sock_listen(ws);

    return ws;
}

int wrp_setup_data_connect(tCTRL_HANDLE handle, tSUB sub, int sub_port)
{
    t_wsock *ws;
    int s;

    verbose("wrp_setup_data_connect [%s] on sub port %d (ctrl_fd %d)", sub2str[sub], sub_port, handle);

    ws = wrp_sock_create(handle, sub, BTLIF_IP_ADDR, wrp_getport(sub, sub_port), 0);

    if (!ws)
    {
        error("wrp_setup_data_connect failed due to out of resources");
        return -1;
    }

    s = wrp_sock_connect(ws);

    return s;
}


