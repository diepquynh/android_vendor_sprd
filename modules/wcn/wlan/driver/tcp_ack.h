/*
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 * Authors:
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __SPRDWL_TCP_ACK_H__
#define __SPRDWL_TCP_ACK_H__

#include "msg.h"

void sprdwl_tcp_ack_init(struct sprdwl_priv *priv);
void sprdwl_tcp_ack_deinit(void);
void sprdwl_fileter_rx_tcp_ack(unsigned char *buf);
/* return val: 0 for not fileter, 1 for fileter */
int sprdwl_fileter_send_tcp_ack(struct sprdwl_msg_buf *msgbuf,
				unsigned char *buf);

#endif
