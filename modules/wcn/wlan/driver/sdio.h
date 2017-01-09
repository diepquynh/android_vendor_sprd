/*
 * Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 * Authors	:
 * Keguang Zhang <keguang.zhang@spreadtrum.com>
 * Jingxiang Li <Jingxiang.li@spreadtrum.com>
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

#ifndef __SPRDWL_SDIO_H__
#define __SPRDWL_SDIO_H__

#include <linux/types.h>
#include <linux/wait.h>
#include <linux/spinlock.h>

#define SPRDWL_RX_NO_WORKQUEUE
#define SPRDWL_TX_SELF
#ifdef SPRDWL_TX_SELF
#include <linux/wakelock.h>
struct sprdwl_tx_buf {
	unsigned char   *base;
	unsigned short  buf_len;
	unsigned short  curpos;
	int change_size;
};
#endif

struct sprdwl_priv;
struct sprdwl_sdio {
	struct platform_device *pdev;
	/* priv use void *, after MCC adn priv->flags,
	 * and change txrx intf pass priv to void later
	 */
	struct sprdwl_priv *priv;

	/* if nedd more flags which not only exit, fix it*/
	/* unsigned int exit:1; */
	int exit;

	unsigned long cmd_timeout;
	unsigned long data_timeout;
	int flag;
	int lastflag;
	/* lock for do_tx */
	spinlock_t lock;
	unsigned long do_tx;
	wait_queue_head_t waitq;
	unsigned int net_stop_cnt;
	unsigned int net_start_cnt;
	unsigned int drop_cmd_cnt;
	/* sta */
	unsigned int drop_data1_cnt;
	/* p2p */
	unsigned int drop_data2_cnt;
	unsigned int ring_cp;
	unsigned int ring_ap;
	atomic_t flow0;
	atomic_t flow1;
	atomic_t flow2;

	/* 1 for send data; 0 for not send data */
	int driver_status;

	/* list not included in sprdwl_vif */
	/* just as our driver support less port */
	/* for cmd */
	struct sprdwl_msg_list tx_list0;
	/* for STA/SOFTAP data */
	struct sprdwl_msg_list tx_list1;
	/* for P2P data */
	struct sprdwl_msg_list tx_list2;
#ifdef SPRDWL_TX_SELF
	struct sprdwl_tx_buf txbuf;
	struct wake_lock tx_wakelock;
#endif
	/* off screen tx may into deepsleep */
	struct wake_lock keep_wake;
	unsigned long wake_last_time;
	unsigned long wake_timeout;
	unsigned long wake_pre_timeout;

	struct work_struct tx_work;
	struct workqueue_struct *tx_queue;

#ifndef SPRDWL_RX_NO_WORKQUEUE
	struct sprdwl_msg_list rx_list;
	struct work_struct rx_work;
	struct workqueue_struct *rx_queue;
#endif
};

#endif
