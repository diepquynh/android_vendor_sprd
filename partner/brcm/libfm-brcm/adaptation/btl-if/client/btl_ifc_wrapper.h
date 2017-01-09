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
#ifndef BTL_IFC_WRAPPER_H
#define BTL_IFC_WRAPPER_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include "btl_if.h"

// fixme - allocate dynamically

/* max simultaneous users of one socket type */
#define SOCKET_USER_MAX 10


typedef struct {
    tBTL_ListNode node; // datapath active handle list
    tBTL_ListNode async_node; // async syscall list (wrapper)
    tBTL_IF_SUBSYSTEM sub; // backref
    int listen_fd;
    int wsock;
    int busy;
    int flags;
    int port;
    char *name;   // backref  tcp addr or local socket name
    SA_TYPE address;
    char *rx_buf; // dedicated receive buffer
    int rx_buf_size;
    int rx_buf_pending; // if set, buffer has not been received yet
    int rx_flow;

    void *priv;

    /* ag */
    unsigned short rf_chan;

    /* fixme -- use bit flags instead */
    int select_pending;
    int disconnect_pending;

    int async_msg_pnd;
    tBTL_PARAMS async_params;
} t_wsock;


/* wrapper socket management */
void wrp_init(void);
void wrp_wsock_init(t_wsock *ws);
t_wsock* wrp_find_wsock(int s);
t_wsock* wrp_find_fdset(fd_set *fds);
t_wsock* wrp_find_wsock_by_rfhdl(unsigned short rf_chan, BOOLEAN is_listener);

BOOLEAN ws_in_active_list(t_wsock *ws);
int wrp_remove_active_set(int s);
int wrp_add_to_active_set(int s);
int wrp_add_to_active_set_custom_fd(int s);
fd_set wrp_get_active_set(int *max_fd);
void wrp_reset_active_set(void);


void wrp_setup_rxbuf(t_wsock *ws, char *p, int size);
void wrp_setup_rxflow(t_wsock *ws, BOOLEAN flow_on);

t_wsock* wrp_wsock_create(int sub);
int wrp_sock_create(int sub);
int wrp_sock_bind(t_wsock* ws, int s, char *name, int port);
int wrp_sock_accept(t_wsock* ws, int s);
int wrp_sock_listen(t_wsock* ws, int s);
int wrp_sock_listen_bl(t_wsock* ws, int s, int backlog);

int wrp_sock_connect(t_wsock* ws, int s, char *name, int port);
int wrp_check_active_handles(fd_set *p_read_set);
void wrp_close_sub_all(int sub);
void wrp_close_full(t_wsock *ws);
BOOLEAN wrp_close_s_only(t_wsock *ws, int s);

int wrp_getport(tSUB sub, int sub_port);

void hex_dump(char *msg, void *data, int size, int trunc);
int rx_data(int fd, char *p, int len);

#endif

