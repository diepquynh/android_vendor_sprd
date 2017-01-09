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
#ifndef BLZ_WRAPPER_H
#define BLZ_WRAPPER_H

#undef socket
#define socket btl_if_socket

#undef connect
#define connect btl_if_connect

#undef bind
#define bind btl_if_bind

#undef listen
#define listen btl_if_listen

#undef accept
#define accept btl_if_accept

#undef close
#define close btl_if_close

#undef setsockopt
#define setsockopt btl_if_setsockopt

#undef getsockopt
#define getsockopt btl_if_getsockopt

#undef fcntl
#define fcntl btl_if_fcntl

#undef poll
#define poll btl_if_poll

//#undef read
//#define read btl_if_read

#undef select
#define select btl_if_select


/*******************************************************************************/
/*
  * NOTE : before any of these wrappers are used blz wrapper needs
  *            to be initialized with below function call
  */


/* called once per application process */
extern int blz_wrapper_init(void);

/********************************************************************************/


extern int btl_if_socket(int socket_family, int socket_type, int protocol);
extern int btl_if_bind(int s, const struct sockaddr *my_addr, socklen_t addrlen);
extern int btl_if_listen(int s, int backlog);
extern int btl_if_accept(int s, struct sockaddr *addr, socklen_t *addrlen);
extern int btl_if_connect(int s, const struct sockaddr *serv_addr, socklen_t addrlen);
extern int btl_if_close(int s);
extern int btl_if_setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen);
extern int btl_if_getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen);
extern int btl_if_fcntl(int s, int cmd, long arg);
extern int btl_if_read(int fd, void *buf, size_t count);
extern int btl_if_select(int nfds, fd_set *readfds, fd_set *writefds,
                   fd_set *exceptfds, struct timeval *timeout);
extern int btl_if_poll(struct pollfd *fds, nfds_t nfds, int timeout);


#endif

