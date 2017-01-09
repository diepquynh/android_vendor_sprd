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
#include <sys/select.h>
#include <poll.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/sco.h>

/* this tag needs to be defined before btl_ifc.h include */
#define LOG_TAG "BLZ_WRAPPER"

#include "btl_ifc.h"
#include "btl_ifc_wrapper.h"

tCTRL_HANDLE ctrl_hdl;
static BOOLEAN blz_wrapper_init_done = FALSE;

#define CHECK_BLZ_INIT BLZ_AUTO_INIT
//#define CHECK_BLZ_INIT() if (!blz_wrapper_init_done){ error("%s : blz wrapper not initialized\n", __FUNCTION__); exit(1);}
#define BLZ_AUTO_INIT() if (!blz_wrapper_init_done) { info("autostarting blz wrapper\n");if (blz_wrapper_init()<0) return -1;}
#define DUMP_WSOCK(msg, ws) if (ws) debug("[%s] %s: (%d:%d), disc_pending %d, asnc %d, selct %d, flags %x\n", __FUNCTION__, msg, ws->wsock, ws->listen_fd, ws->disconnect_pending, ws->async_msg_pnd, ws->select_pending, ws->flags);

/*******************************************************************************
**
**  Misc
**
**
*******************************************************************************/

void swap_array(char *dst, char *src, int n)
{
    char *d = dst;
    char *s = src;
    unsigned int i;

    for (i = 0; i < n; i++)
        d[i] = s[n - 1 - i];
}

/*******************************************************************************
**
**  Blocking syscall management
**
**
*******************************************************************************/

typedef enum {
    WAIT_EVENT_NONE,
    WAIT_EVENT_CONNECT_CFM,
} t_blz_event;

pthread_mutex_t ctrl_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cond_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cond  = PTHREAD_COND_INITIALIZER;

static t_blz_event wait_event_id;
static tBTL_PARAMS wait_params;

struct sockaddr_rc rc_addr_stored;
static tBTL_ListNode ws_async_pending;

tBTL_PARAMS* wait_response(t_blz_event event)
{
    debug("wait_response [%d]\n", event);
    pthread_mutex_lock(&cond_mutex);
    pthread_mutex_unlock(&ctrl_mutex);
    wait_event_id = event;
    pthread_cond_wait(&cond, &cond_mutex);
    pthread_mutex_unlock(&cond_mutex);
    debug("wait_response [%d] unblocked\n", event);
  return &wait_params;
}

void signal_event(t_blz_event event, tBTL_PARAMS *params)
{
    debug("signal_event [%d]\n", event);
    pthread_mutex_lock(&cond_mutex);

    /* if event matches the current wait event, unblock action */
    if (event == wait_event_id)
    {
        //wait_params = *params;
        memcpy(&wait_params, params, sizeof(tBTL_PARAMS));

        pthread_cond_signal(&cond);
    }
    pthread_mutex_unlock(&cond_mutex);
}

/*******************************************************************************
**
**  Async syscall management
**
**
*******************************************************************************/

void ws_async_add(t_wsock *ws)
{
    DUMP_WSOCK("", ws);
    list_add_node(&ws->async_node, &ws_async_pending);
}
void ws_async_del(t_wsock *ws)
{
    DUMP_WSOCK("", ws);
    list_del_node(&ws->async_node);
}

t_wsock * ws_async_fetch(void)
{
    t_wsock *ws = NULL;

    if (!list_isempty(&ws_async_pending))
    {
        /* get first in queue */
        ws = LIST_GET_NODE_OBJ(ws_async_pending.p_next, t_wsock, async_node);
        DUMP_WSOCK("", ws);
    }
    return ws;
}

t_wsock *ws_async_find_fdset(fd_set *fds)
{
    t_wsock *ws;
    struct tBTL_ListNode_tag *pos;

    LIST_FOREACH(pos, &ws_async_pending)
    {
        ws = LIST_GET_NODE_OBJ(pos, t_wsock, async_node);

        if ((ws->wsock>0) && FD_ISSET(ws->wsock, fds))
        {
            //DUMP_WSOCK("found wsock", ws);
            return ws;
        }
    }
    return NULL;
}

/*******************************************************************************
**
**  Bluez wrapper helper functions
**
**
*******************************************************************************/

void blz_ctrl_msg(tCTRL_HANDLE handle, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params)
{
    t_wsock *ws;

    debug("[blz ctrl] received message [%s]\n", dump_msg_id(id));

    pthread_mutex_lock(&ctrl_mutex);

    switch(id)
    {
        case BTLIF_CONNECT_REQ:
            /* managed by upper layers */
            break;

        case BTLIF_CONNECT_IND:
            {
                /* search for listener bound to this rfcomm channel */
                ws = wrp_find_wsock_by_rfhdl(params->ag_conreq.rf_chan, TRUE);

                /* While Bluetooth is turning off and a stereo headset is trying
                 * to connect, if we don't have headset's bd_address and rf_chan
                 * information, we could not connect it again. Hence
                 * "rc_addr_stored.rc_bdaddr.b" and "rc_addr_stored.rc_channel"
                 * need to be stored for next connection.
                 */

                /* save incoming bd address for later notification through syscalls */
                swap_array((char*)rc_addr_stored.rc_bdaddr.b, (char *)&params->ag_conreq, 6);
                rc_addr_stored.rc_channel = params->ag_conreq.rf_chan;

                if (!ws)
                {
                    error("BTLIF_CONNECT_IND : wsock not found for rf chan %d\n", params->ag_conreq.rf_chan);
                    error("Disconnect RFCOMM Channel");
                    BTLIF_AG_Disconnect(ctrl_hdl, params->ag_conreq.rf_chan);
                    pthread_mutex_unlock(&ctrl_mutex);

                    return;
                }

                BTLIF_AG_ConnectIndAck(ctrl_hdl, (BD_ADDR *)&params->ag_conreq, params->ag_conreq.rf_chan);
            }
            break;

        case BTLIF_CONNECT_RSP:
            {
                ws = ws_async_fetch();

                DUMP_WSOCK("async fetch", ws);

                if (ws && ws->async_msg_pnd)
                {
                    debug("blz_ctrl_msg : set selectpending 0\n");

                    memcpy(&ws->async_params, params, sizeof(tBTLIF_CONNECT_RSP_PARAM));
                    //ws->async_params = *params;

                    ws->select_pending = 0;

                    // fixme -- break current select call instead of waiting out the full timeout
                }
                else
                {
                    /* blocking call */
                    signal_event(WAIT_EVENT_CONNECT_CFM, params);
                }
            }
            break;

        case BTLIF_DISCONNECT_RSP:
               /* managed by upper layers in blz wrapper */
            break;

        case BTLIF_DISCONNECT_IND:
           {
                t_wsock *ws;

                /* data channel disconnected by server */
                debug("Channel disconnected remotely, rf_ch %d\n", params->ag_disc.rf_chan);

                /* search for active data socket bound to this rfcomm channel */
                ws = wrp_find_wsock_by_rfhdl(params->ag_disc.rf_chan, FALSE);

                if (ws)
                {
                    /* make sure we notify user next time select or poll is called */
                    ws->disconnect_pending= 1;
                    DUMP_WSOCK("Set disconnect pending\n", ws);
                }
           }
           break;

        default:
            break;
    }

    pthread_mutex_unlock(&ctrl_mutex);
}


/* called once per application process */
int blz_wrapper_init(void)
{
    if (blz_wrapper_init_done) return 0;

    info("btl_if_blz_wrapper_init\n");

    /* initialize async list */
    INIT_LIST_NODE(&ws_async_pending);

    /* initialize wrapper */
    wrp_init();

    /* make sure client is initialized */
    BTL_IFC_ClientInit();

    if (btl_ifc_main_running())
    {
       debug("blz wrapper init done !\n");
       blz_wrapper_init_done = TRUE;
       return 0;
    }

    debug("blz wrapper init failed\n");
    return -1;
}

/* called once per subsystem init */
int blz_init_subsystem(tBTL_IF_SUBSYSTEM sub)
{
    /* register with datapath server, no datacallback used as upper layers will manage raw sockets */
    return BTL_IFC_RegisterSubSystem(&ctrl_hdl, sub, NULL, blz_ctrl_msg);
}

/*******************************************************************************
**
** Wrapper APIx
**
**
*******************************************************************************/

int btl_if_socket(int socket_family, int socket_type, int protocol)
{
    CHECK_BLZ_INIT();

    if (socket_family != PF_BLUETOOTH)
        return -1;

    info("btl_if_socket :: fam %d, type %d, proto %d\n", socket_family, socket_type, protocol);

    switch (protocol)
    {
        case BTPROTO_RFCOMM :
            /* make sure this subsystem is initialized */
            blz_init_subsystem(SUB_AG);
            return wrp_sock_create(SUB_AG);
            break;

        case BTPROTO_SCO :
            /* make sure this subsystem is initialized */
            blz_init_subsystem(SUB_SCO);
            return wrp_sock_create(SUB_SCO);

        default:
            return -1;
    }
    return -1;
}


int btl_if_read(int fd, void *buf, size_t count)
{
    t_wsock* ws;

    CHECK_BLZ_INIT();

    ws = wrp_find_wsock(fd);

    info("%s: fd %d, buf %x\n", __FUNCTION__, fd, count);

    /* while we have an async operation pending, return 0 */
    if (ws->async_msg_pnd)
        return 0;

    return read(fd, buf, count);
}

/* returns nbr of fd:s that has info on requested events */
int btl_if_poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
    int i;
    t_wsock* ws;
    int disconnected = 0;
    int result;

    CHECK_BLZ_INIT();

    /* check if this socket is disconnected */
    for (i=0; i < (int)nfds; i++)
    {
        //info("%s: poll fd %d, ev %d, timeout %d\n", __FUNCTION__, fds[i].fd, fds[i].events, timeout);

        ws = wrp_find_wsock(fds[i].fd);

        // remove for now to reduce logging
        //DUMP_WSOCK("", ws);

        if (ws==NULL)
        {
            /* notify poller that this socket is not up anymore */
            debug("wsock down, return POLLHUP poll fd %d, ev %x\n", fds[i].fd, fds[i].events);
            errno = EIO; // signal io error
            fds[i].revents = POLLHUP;
            disconnected = 1;
        }

        /* return error if link disconnection was detected (this should trigger a close) */
        else if (ws->disconnect_pending)
        {
            /* only notify data sockets */
            if (ws->wsock == fds[i].fd)
            {
                error("detected pending data socket disconnect, notify caller\n");
                errno = EIO; // signal io error
                fds[i].revents =  POLLHUP;
                disconnected = 1;
            }
        }
    }

    /* return -1 will trigger a close of this socket */
    if (disconnected) {
        /* wait full poll timeout before returning */
        usleep(timeout*1000);
        return -1;
    }

    /* transparent poll */
    result = poll(fds, nfds, timeout);

    if (result == 0)
    {
       //info("transp poll : no events on fds\n");
    }
    else
    {
        for (i=0; i < result; i++)
            info("transp poll : (fd %d) returned r_ev %d\n", fds[i].fd, fds[i].revents);
    }

    return result;
}


int btl_if_select(int nfds, fd_set *readfds, fd_set *writefds,
                   fd_set *exceptfds, struct timeval *timeout)
{
    t_wsock* ws;
    int result;

    CHECK_BLZ_INIT();

    //info("%s, timeout %d,%d\n", __FUNCTION__, (int)timeout->tv_sec, (int)timeout->tv_usec);

    /* check if we need to do any special handling of this select call for pending btl if operations */
    ws = ws_async_find_fdset(readfds);

    //DUMP_WSOCK("", ws);

    if (ws)
    {

        DUMP_WSOCK("", ws);

        /* return error if link disconnection was detected (this should trigger a close) */
        if (ws->disconnect_pending || !ws->busy)
        {
             error("disconnect pending, return -1\n");
             errno = EIO; // signal io error
             return -1;
        }

        if (ws->select_pending)
        {
            debug("btl_if_select : async pending, sleep [%d s, %d us]\n", (int)timeout->tv_sec, (int)timeout->tv_usec);

            /* wait full timeout for now... */
            sleep(timeout->tv_sec);
            usleep(timeout->tv_usec);
            return 0;
        }
        else
        {
           int result;
           debug("btl_if_select : async connection completed, check result\n");

           /* server accepted our connect request, now setup actual datachannel.
                        we will only have 1 datachannel for this subsystem so lets use subport 0 */

           ws = wrp_find_wsock(ws->wsock);

           if(ws->async_msg_pnd == BTLIF_CONNECT_RSP)
           {
              /* check result of connection attempt */
              if (ws->async_params.conrsp.result !=  BTL_IF_SUCCESS)
              {
                  info("async connection completed with result %d\n", ws->async_params.conrsp.result);

                  /* clear async list */
                  ws_async_del(ws);
                  ws->async_msg_pnd = 0;
                  errno = EHOSTDOWN;
                  return -1;
              }
           }
           else
           {
               debug("async operation %d completed but was unhandled\n", ws->async_msg_pnd);
           }

           /* establish connection */
           result = wrp_sock_connect(ws, ws->wsock, btl_ifc_get_srvaddr(), ws->port);

           /* clear async list */
           ws_async_del(ws);
           ws->async_msg_pnd = 0;

           /* now set the flags on new socket fd */
           fcntl(ws->wsock, F_SETFL, ws->flags);

        }
    }

    /* ok, we are done with any special handling, let user call select transparantly */
    result = select(nfds, readfds, writefds, exceptfds, timeout);

    debug("btl_if_select : transparant mode, result %d\n", result);

    return result;
}


int btl_if_connect(int s, const struct sockaddr *serv_addr, socklen_t addrlen)
{
    t_wsock* ws;

    CHECK_BLZ_INIT();

    ws = wrp_find_wsock(s);

    DUMP_WSOCK("", ws);

    if (!ws)
    {
        error("btl_if_connect : no wsock found\n");
        return -1;
    }

    //info("btl_if_connect : fd %d, async %d [%s]\n", s, ws->flags&O_NONBLOCK, ws->name);

    switch(ws->sub)
    {
        case SUB_AG:
            {
                struct sockaddr_rc *addr = (struct sockaddr_rc*)serv_addr;

                info("btl_if_connect : rf_chan %d, bd[%x:%x]\n", addr->rc_channel, (unsigned char)addr->rc_bdaddr.b[4], (unsigned char)addr->rc_bdaddr.b[5]);

                /* check if socket is setup in non blocking mode */
                if (ws->flags & O_NONBLOCK)
                {
                    /* make select wait until we are done with this operation */
                    ws->select_pending = 1;

                    if (!ws->async_msg_pnd)
                    {
                        debug("set async pending, return EINPROGRESS and send connect req...\n");

                        ws->async_msg_pnd = BTLIF_CONNECT_RSP;
                        ws->port = wrp_getport(SUB_AG, addr->rc_channel); // save this for later
                        ws->rf_chan = addr->rc_channel;

                        /* store this wsock for later retrieval when server responds */
                        ws_async_add(ws);

                        BTLIF_AG_ConnectReq(ctrl_hdl, (BD_ADDR *)&addr->rc_bdaddr, addr->rc_channel);

                        errno = EINPROGRESS;
                        return -1;
                    }
                    else
                        error("error : async already pending !\n");
                }
                else
                {
                    tBTL_PARAMS *params;

                    /* make sure we block this call until a response is received */
                    pthread_mutex_lock(&ctrl_mutex);

                    BTLIF_AG_ConnectReq(ctrl_hdl, (BD_ADDR *)&addr->rc_bdaddr, addr->rc_channel);

                    info("wait for con rsp\n");

                    /* wait here until we have a response */
                    params = wait_response(WAIT_EVENT_CONNECT_CFM);

                    info("connect response %d\n", params->conrsp.result);

                    if (params->conrsp.result == BTL_IF_SUCCESS)
                    {
                        /* server accepted our connect request, now setup actual datachannel.
                                            we will only have 1 datachannel for this subsystem so lets use subport 0 */

                        ws = wrp_find_wsock(s);
                        ws->rf_chan = addr->rc_channel;

                        /* establish connection */
                        wrp_sock_connect(ws, s, btl_ifc_get_srvaddr(), wrp_getport(SUB_AG, addr->rc_channel));

                        /* add handle to active list */
                        //wsactive_add(ws, s);
                    }
                    else
                    {
                        /* connection failed, clear async list */
                        ws_async_del(ws);
                        ws->async_msg_pnd = 0;

                        /* connection failed, return error to JNI */
                        return -1;
                    }
                }
            }
            break;

        case SUB_SCO:
         {
                //struct sockaddr_rc *addr = (struct sockaddr_rc*)serv_addr;

                info("btl_if_connect : [sco]\n");

                /* check if socket is setup in non blocking mode */
                if (ws->flags & O_NONBLOCK)
                {
                    /* make select wait until we are done with this operation */
                    ws->select_pending = 1;

                    if (!ws->async_msg_pnd)
                    {
                        debug("set async pending, return EINPROGRESS\n");
                        ws->async_msg_pnd = BTLIF_CONNECT_RSP;
                        ws->port = wrp_getport(SUB_SCO, 0); // save this for later

                        /* store this wsock for later retrieval when server responds */
                        ws_async_add(ws);

                        BTL_IFC_CtrlSend(ctrl_hdl, SUB_SCO, BTLIF_CONNECT_REQ, NULL, 0);
                        errno = EINPROGRESS;
                        return -1;
                    }
                }
                else
                {
                    tBTL_PARAMS *params;

                    /* make sure we block this call until a response is received */
                    pthread_mutex_lock(&ctrl_mutex);

                    BTL_IFC_CtrlSend(ctrl_hdl, SUB_SCO, BTLIF_CONNECT_REQ, NULL, 0);

                    info("wait for con rsp\n");

                    /* wait here until we have a response */
                    params = wait_response(WAIT_EVENT_CONNECT_CFM);

                    info("connect status %d !\n", params->conrsp.result);

                    if (params->conrsp.result == BTL_IF_SUCCESS)
                    {
                        ws = wrp_find_wsock(s);

                        /* establish connection ono subport 0 */
                        if (wrp_sock_connect(ws, s, btl_ifc_get_srvaddr(), wrp_getport(SUB_SCO, 0))<0)
                           return -1;

                        /* add handle to active list */
                        //wsactive_add(ws, s);
                    }
                    else
                    {
                        /* connection failed, give other status than -1 ? */
                        return -1;
                    }
                }
            }

            break;
        default :
            error("%s: invalid subsystem %d\n", __FUNCTION__, ws->sub);
            return -1;

    }


    return ws->wsock;//wrp_sock_connect(ws, s);
}

int btl_if_bind(int s, const struct sockaddr *my_addr, socklen_t addrlen)
{
    t_wsock* ws;
    int result = -1;

    CHECK_BLZ_INIT();

    //info("btl_if_bind : fd %d\n", s);

    ws = wrp_find_wsock(s);

    DUMP_WSOCK("", ws);

    if (!ws)
    {
        error("btl_if_bind : no wsock found\n");
        return -1;
    }

    switch(ws->sub)
    {
        case SUB_AG:
            {
                struct sockaddr_rc *addr = (struct sockaddr_rc*)my_addr;

                memcpy((char*)&rc_addr_stored, (char*)addr, sizeof(struct sockaddr_rc));

                if (addr->rc_channel >= AG_CHAN_MAX)
                {
                    error("RFC channel too big\n");
                    return -1;
                }

                info("btl_if_bind : [rfc] rc chan %d, bd[%x:%x]\n", addr->rc_channel, (unsigned char)addr->rc_bdaddr.b[4], (unsigned char)addr->rc_bdaddr.b[5]);

                result = wrp_sock_bind(ws, s, btl_ifc_get_srvaddr(), wrp_getport(SUB_AG, addr->rc_channel));

                if (result<0)
                    return result;

                /* store rf chan for this wsock so we can disconnet it later */
                ws->rf_chan = addr->rc_channel;

                /* bind here as instead of in listen call as we we have the addr params available */
                BTLIF_AG_ListenReq(ctrl_hdl, (BD_ADDR *)&addr->rc_bdaddr, addr->rc_channel);
            }
            break;

        case SUB_SCO:
            {
                //struct sockaddr_sco *addr = (struct sockaddr_sco*)my_addr;

                info("btl_if_bind : [sco]\n");

                /* assume max 1 SCO link active, use subport 0 */
                result = wrp_sock_bind(ws, s, btl_ifc_get_srvaddr(), wrp_getport(SUB_SCO, 0));

                if (result<0)
                    return result;

                BTL_IFC_CtrlSend(ctrl_hdl, SUB_SCO, BTLIF_LISTEN_REQ, NULL, 0);
            }
            break;
        default :
            error("%s: invalid subsystem %d\n", __FUNCTION__, ws->sub);
            result = -1;
            break;

    }
    return result;
}



int btl_if_listen(int s, int backlog)
{
    t_wsock* ws;
    int result;

    CHECK_BLZ_INIT();

    //debug("%s (%d)\n", __FUNCTION__, s);

    ws = wrp_find_wsock(s);

    DUMP_WSOCK("", ws);

    if (!ws)
    {
        error("btl_if_listen : no wsock found\n");
        return -1;
    }

    debug("btl_if_listen : fd %d [%s]\n", s, ws->name);

    /* already sent listen request in bind call */

    /* no translation needed */
    result = wrp_sock_listen(ws, s);

    //wsactive_add(ws, s);

    return result;
}



int btl_if_accept(int s, struct sockaddr *my_addr, socklen_t *addrlen)
{
    int sock_new;
    t_wsock* ws;

    CHECK_BLZ_INIT();

    //info("%s (%d)\n", __FUNCTION__, s);

    ws = wrp_find_wsock(s);

    DUMP_WSOCK("", ws);

    if (!ws)
    {
        error("btl_if_accept : no wsock found\n");
        return -1;
    }

    switch(ws->sub)
    {
      case SUB_AG:
          {
              info("btl_if_accept : [rfc], copy params\n");
              memcpy((void*)my_addr, (void*)&rc_addr_stored, sizeof(struct sockaddr_rc));
          }
          break;

      case SUB_SCO:
          {
              info("btl_if_accept : [sco]\n");
          }
          break;
      default :
          error("%s: invalid subsystem %d\n", __FUNCTION__, ws->sub);
          return -1;

    }

    sock_new = wrp_sock_accept(ws, s);

    if (sock_new < 0)
        return sock_new;

    /* remember listen fd  (done in wrp functions....0 */

    //ws->listen_fd = ws->wsock;
    //ws->wsock = sock_new;

#if 0
    /* clone new socket */
    ws_new = wrp_clone(ws);

    /* map this to the new socket fd */
    ws_new->wsock = sock_new;

    /* add to active list */
    wsactive_add(ws_new);
#endif

    return sock_new;
}



int btl_if_getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen)
{
    CHECK_BLZ_INIT();
    info("btl_if_getsockopt -- not implemented\n");
    return 0;
}

int btl_if_setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen)
{
    CHECK_BLZ_INIT();

    debug("btl_if_setsockopt level:%d, optname:%d\n", level, optname);

    if (((level != SOL_RFCOMM) && (level != SOL_SOCKET)) )
        return -1;

    if (optname == RFCOMM_LM)
    {
        info("configure rfcomm lm mode\n");

        // fixme -- send across params over ctrl channel

    }
    return 0;
}


int btl_if_fcntl(int s, int cmd, long arg)
{
    t_wsock* ws;

    CHECK_BLZ_INIT();

    //info("%s (%d) cmd 0x%x, arg 0x%x\n", __FUNCTION__, s, cmd, (int)arg);

    ws = wrp_find_wsock(s);

    //DUMP_WSOCK("", ws);

    if (!ws) {
        error("btl_if_fcntl : no wsock found\n");
        return -1;
    }

    /* make sure we catch a non blocking configuration to emulate the blz sockets */
    if (cmd == F_SETFL)
    {
        ws->flags = arg;
    }

    //return 0;
    /* also let this pass onto sysfunc */
    return fcntl(s, cmd, arg);
}


int btl_if_close (int s)
{
    t_wsock* ws;

    CHECK_BLZ_INIT();

    info("%s (%d)\n", __FUNCTION__, s);

    ws = wrp_find_wsock(s);

    DUMP_WSOCK("", ws);

    if (!ws)
    {
        error("btl_if_close : no wsock found\n");
        return -1;
    }

    /* send disconnect req if socket matches active wsock data handle s*/
    if (ws->wsock == s)
    {
        switch(ws->sub)
        {
            case SUB_AG:
                /* notify server that this subsystem is closed */
                BTLIF_AG_Disconnect(ctrl_hdl, ws->rf_chan);
                break;

            case SUB_SCO:
                /* notify server that this subsystem is closed */
                BTL_IFC_CtrlSend(ctrl_hdl, SUB_SCO, BTLIF_DISCONNECT_REQ, NULL, 0);
                break;

            default:
                error("%s: invalid subsystem %d\n", __FUNCTION__, ws->sub);
                return -1;
        }
    }

    /* clear any disconnect pending flag */
    if(ws->disconnect_pending)
        debug("clear disconnect pending\n");

    ws->disconnect_pending = 0;

    /* only close requested socket, wsock will terminate when both listener and data socket is closed */
    if (wrp_close_s_only(ws, s) == TRUE)
        ws_async_del( ws );

    //wrp_close(ws);

    /* wsocket re-initialized when fully closed */

    return 0;
}



