/************************************************************************************
 *
 *  Copyright (C) 2009-2011 Broadcom Corporation
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
#ifndef BTL_IFS_WRAPPER_H
#define BTL_IFS_WRAPPER_H

#include "btl_if.h"

extern fd_set read_set;
extern fd_set active_set;
extern int max_fd;

// fixme - allocate dynamically

/* max simultaneous users of one socket type */
#define SOCKET_USER_MAX 40


typedef struct {
    tBTL_ListNode node; // datapath active handle list
    tBTL_ListNode lstnode;
    tCTRL_HANDLE ctrl_fd; /* back ref */
    int busy;
    int wsock;
    int listen_fd;
    int subsystem;
    char *name;
    SA_TYPE address;
    int port;
    tBTL_IF_SUBSYSTEM sub; // backref
    char *rx_buf; // dedicated receive buffer
    int rx_buf_size;
    int rx_flow;
    int rx_buf_pending;
    int non_sock;
} t_wsock;

void hex_dump(char *msg, void *data, int size, int trunc);

void wrp_init(void);

void wrp_sock_init(void);
t_wsock *wrp_sock_create(tCTRL_HANDLE handle, tSUB sub, char *name, int port, int is_listen);
t_wsock *wrp_sock_create_ext(tCTRL_HANDLE handle, tSUB sub, char *name, int port, int ext_fd);
void wrp_wsock_free(t_wsock *ws);

int wrp_sock_bind(t_wsock *ws);
int wrp_sock_listen(t_wsock *ws);
int wrp_sock_connect(t_wsock *ws);
int wrp_data_accept(t_wsock *ws);
void ws_close_all_active(void);

void ws_listener_add(t_wsock *ws, int add_active_set);
void ws_listener_del(t_wsock *ws);

void ws_del_sub_listeners(tSUB sub);
void ws_close_all_listeners(void);

void wrp_close(int sock_fd);
void wrp_close_all(t_wsock *ws);
void wrp_setup_rxbuf(t_wsock *ws, char *p, int size);
t_wsock* wrp_find_wsock(int s);
t_wsock* wrp_find_wsock_by_port(int sub, int sub_port);

int wrp_add_to_active_set(int s);
int wrp_remove_active_set(int fd);

void wrp_monitor_data_fds(void);
void wrp_monitor_ctrl_fds(void);

t_wsock *wrp_setup_data_listener(tCTRL_HANDLE handle, tSUB sub, int sub_port);
int wrp_setup_data_connect(tCTRL_HANDLE handle, tSUB sub, int sub_port);

int wrp_getport(tSUB sub, int sub_port);
int wrp_getsubport(t_wsock *ws);

void wrp_setup_rxflow(t_wsock *ws, BOOLEAN flow_on);


int wrp_alloc_dynamicport(tSUB sub, int sock);
void wrp_free_dynamicport(tSUB sub, int port);

int tx_data(int fd, char *p, int len, int non_sock);
int rx_data(int fd, char *p, int len);
void btl_if_rxdata(tBTL_IF_SUBSYSTEM sub, int fd, char *p, int len);

#endif

