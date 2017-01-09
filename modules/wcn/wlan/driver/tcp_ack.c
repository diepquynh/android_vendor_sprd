#include <uapi/linux/if_ether.h>
#include <uapi/linux/tcp.h>
#include <uapi/linux/ip.h>
#include <uapi/linux/in.h>
#include <linux/moduleparam.h>

#include "sprdwl.h"
#include "intf_ops.h"

#define SPRDWL_TCP_ACK_NUM  4
#define SPRDWL_TCP_ACK_CONTINUE_NUM	10
#define SPRDWL_TCP_ACK_DROP_CNT		6
#define SPRDWL_TCP_ACK_DROP_TIME	10
#define SPRDWL_TCP_ACK_EXIT_VAL		0x800

#define SPRDWL_ACK_OLD_TIME	4000
#define SPRDWL_U32_BEFORE(a, b)	((__s32)((__u32)a - (__u32)b) < 0)

static unsigned int tcp_ack_drop_cnt = SPRDWL_TCP_ACK_DROP_CNT;
/* Maybe you need S_IRUGO | S_IWUSR for debug */
module_param(tcp_ack_drop_cnt, uint, 0);
MODULE_PARM_DESC(tcp_ack_drop_cnt, "valid values: [1, 13]");

struct sprdwl_tcp_ack_msg {
	u16 source;
	u16 dest;
	s32 saddr;
	s32 daddr;
	u32 seq;
};

struct sprdwl_tcp_ack_info {
	int busy;
	int drop_cnt;
	int psh_flag;
	u32 psh_seq;
	/* lock for ack info */
	spinlock_t lock;
	unsigned long last_time;
	unsigned long timeout;
	struct timer_list timer;
	struct sprdwl_msg_buf *msgbuf;
	struct sprdwl_tcp_ack_msg ack_msg;
};

struct sprdwl_tcp_ack_manage {
	/* 1 filter */
	int max_num;
	int free_index;
	unsigned long last_time;
	unsigned long timeout;
	unsigned long drop_time;
	atomic_t ref;
	/* lock for tcp ack alloc and free */
	spinlock_t lock;
	struct sprdwl_priv *priv;
	struct sprdwl_tcp_ack_info ack_info[SPRDWL_TCP_ACK_NUM];
};

static struct sprdwl_tcp_ack_manage sprdwl_ack_manage;

static void sprdwl_tcp_ack_timeout(unsigned long data)
{
	struct sprdwl_tcp_ack_info *ack_info;
	struct sprdwl_msg_buf *msg;

	ack_info = (struct sprdwl_tcp_ack_info *)data;
	spin_lock_bh(&ack_info->lock);
	msg = ack_info->msgbuf;
	if (msg) {
		ack_info->msgbuf = NULL;
		ack_info->drop_cnt = 0;
		spin_unlock_bh(&ack_info->lock);
		sprdwl_intf_tx(sprdwl_ack_manage.priv, msg);
		return;
	}
	spin_unlock_bh(&ack_info->lock);
}

void sprdwl_tcp_ack_init(struct sprdwl_priv *priv)
{
	int i;
	struct sprdwl_tcp_ack_info *ack_info;
	struct sprdwl_tcp_ack_manage *ack_m = &sprdwl_ack_manage;

	memset(ack_m, 0, sizeof(struct sprdwl_tcp_ack_manage));
	if (tcp_ack_drop_cnt == 0 || tcp_ack_drop_cnt >= 14)
		tcp_ack_drop_cnt = SPRDWL_TCP_ACK_DROP_CNT;
	ack_m->priv = priv;
	atomic_set(&ack_m->ref, 0);
	spin_lock_init(&ack_m->lock);
	ack_m->drop_time = SPRDWL_TCP_ACK_DROP_TIME * HZ / 1000;
	ack_m->last_time = jiffies;
	ack_m->timeout = msecs_to_jiffies(SPRDWL_ACK_OLD_TIME);
	for (i = 0; i < SPRDWL_TCP_ACK_NUM; i++) {
		ack_info = &ack_m->ack_info[i];
		spin_lock_init(&ack_info->lock);
		ack_info->last_time = jiffies;
		ack_info->timeout = msecs_to_jiffies(SPRDWL_ACK_OLD_TIME);
		setup_timer(&ack_info->timer, sprdwl_tcp_ack_timeout,
			    (unsigned long)ack_info);
	}
}

void sprdwl_tcp_ack_deinit(void)
{
	int i;
	unsigned long timeout;
	struct sprdwl_tcp_ack_manage *ack_m = &sprdwl_ack_manage;

	atomic_add(SPRDWL_TCP_ACK_EXIT_VAL, &ack_m->ref);
	timeout = jiffies + msecs_to_jiffies(1000);
	while (atomic_read(&ack_m->ref) > SPRDWL_TCP_ACK_EXIT_VAL) {
		if (time_after(jiffies, timeout)) {
			pr_err("%s cmd lock timeout!\n", __func__);
			WARN_ON(1);
		}
		usleep_range(2000, 2500);
	}
	for (i = 0; i < SPRDWL_TCP_ACK_NUM; i++)
		del_timer_sync(&ack_m->ack_info[i].timer);
}

static int sprdwl_tcp_check_quick_ack(unsigned char *buf,
				      struct sprdwl_tcp_ack_msg *msg)
{
	int ip_hdr_len;
	unsigned char *temp;
	struct ethhdr *ethhdr;
	struct iphdr *iphdr;
	struct tcphdr *tcphdr;

	ethhdr = (struct ethhdr *)buf;
	if (ethhdr->h_proto != htons(ETH_P_IP))
		return 0;
	iphdr = (struct iphdr *)(ethhdr + 1);
	if (iphdr->version != 4 || iphdr->protocol != IPPROTO_TCP)
		return 0;
	ip_hdr_len = iphdr->ihl * 4;
	temp = (unsigned char *)(iphdr) + ip_hdr_len;
	tcphdr = (struct tcphdr *)temp;
	/* TCP_FLAG_ACK */
	if (!(temp[13] & 0x10))
		return 0;
	if (temp[13] & 0x8) {
		msg->saddr = iphdr->daddr;
		msg->daddr = iphdr->saddr;
		msg->source = tcphdr->dest;
		msg->dest = tcphdr->source;
		msg->seq = ntohl(tcphdr->seq);
		return 1;
	}

	return 0;
}

/* flag:0 for not tcp ack
 *	1 for ack which can be drope
 *	2 for other ack whith more info
 */
static int sprdwl_tcp_check_ack(unsigned char *buf,
				struct sprdwl_tcp_ack_msg *msg)
{
	int ret;
	int ip_hdr_len;
	int iptotal_len;
	unsigned char *temp;
	struct ethhdr *ethhdr;
	struct iphdr *iphdr;
	struct tcphdr *tcphdr;

	ethhdr = (struct ethhdr *)buf;
	if (ethhdr->h_proto != htons(ETH_P_IP))
		return 0;
	iphdr = (struct iphdr *)(ethhdr + 1);
	if (iphdr->version != 4 || iphdr->protocol != IPPROTO_TCP)
		return 0;
	ip_hdr_len = iphdr->ihl * 4;
	temp = (unsigned char *)(iphdr) + ip_hdr_len;
	tcphdr = (struct tcphdr *)temp;
	/* TCP_FLAG_ACK */
	if (!(temp[13] & 0x10))
		return 0;
	if (temp[13] != 0x10) {
		ret = 2;
		goto out;
	}
	iptotal_len = ntohs(iphdr->tot_len);
	ret = (iptotal_len == 40) ? 1 : 2;

out:
	msg->saddr = iphdr->saddr;
	msg->daddr = iphdr->daddr;
	msg->source = tcphdr->source;
	msg->dest = tcphdr->dest;
	msg->seq = ntohl(tcphdr->ack_seq);

	return ret;
}

/* return val: -1 for not match, others for match */
static int sprdwl_tcp_ack_match(struct sprdwl_tcp_ack_manage *ack_m,
				struct sprdwl_tcp_ack_msg *ack_msg)
{
	int i;
	struct sprdwl_tcp_ack_info *ack_info;
	struct sprdwl_tcp_ack_msg *ack;

	for (i = 0; i < SPRDWL_TCP_ACK_NUM; i++) {
		ack_info = &ack_m->ack_info[i];
		ack = &ack_info->ack_msg;
		if (ack_info->busy) {
			if (ack->dest == ack_msg->dest &&
			    ack->source == ack_msg->source &&
			    ack->saddr == ack_msg->saddr &&
			    ack->daddr == ack_msg->daddr)
				return i;
		}
	}

	return -1;
}

static void sprdwl_tcp_ack_updata(struct sprdwl_tcp_ack_manage *ack_m)
{
	int i;
	struct sprdwl_tcp_ack_info *ack_info;

	if (time_after(jiffies, ack_m->last_time + ack_m->timeout)) {
		spin_lock_bh(&ack_m->lock);
		ack_m->last_time = jiffies;
		for (i = SPRDWL_TCP_ACK_NUM - 1; i >= 0; i--) {
			ack_info = &ack_m->ack_info[i];
			if (ack_info->busy &&
			    time_after(jiffies, ack_info->last_time +
				       ack_info->timeout)) {
				ack_m->free_index = i;
				ack_m->max_num--;
				ack_info->busy = 0;
			}
		}
		spin_unlock_bh(&ack_m->lock);
	}
}

/* return val: -1 for no index, others for index */
static int sprdwl_tcp_ack_alloc_index(struct sprdwl_tcp_ack_manage *ack_m)
{
	int i;
	struct sprdwl_tcp_ack_info *ack_info;

	if (ack_m->max_num == SPRDWL_TCP_ACK_NUM)
		return -1;
	spin_lock_bh(&ack_m->lock);
	if (ack_m->free_index >= 0) {
		i = ack_m->free_index;
		ack_m->free_index = -1;
		ack_m->max_num++;
		ack_m->ack_info[i].busy = 1;
		ack_m->ack_info[i].psh_flag = 0;
		ack_m->ack_info[i].last_time = jiffies;
		spin_unlock_bh(&ack_m->lock);
		return i;
	}
	for (i = 0; i < SPRDWL_TCP_ACK_NUM; i++) {
		ack_info = &ack_m->ack_info[i];
		if (ack_info->busy) {
			continue;
		} else {
			ack_m->free_index = -1;
			ack_m->max_num++;
			ack_info->busy = 1;
			ack_info->psh_flag = 0;
			ack_info->last_time = jiffies;
			spin_unlock_bh(&ack_m->lock);
			return i;
		}
	}

	spin_unlock_bh(&ack_m->lock);
	return -1;
}

/* return val: 0 for not handle tx, 1 for handle tx */
int sprdwl_tcp_ack_handle(struct sprdwl_msg_buf *new_msgbuf,
			  struct sprdwl_tcp_ack_manage *ack_m,
			  struct sprdwl_tcp_ack_info *ack_info,
			  struct sprdwl_tcp_ack_msg *ack_msg,
			  int type)
{
	int quick_ack = 0;
	struct sprdwl_tcp_ack_msg *ack;

	ack = &ack_info->ack_msg;
	if (type == 2) {
		if (SPRDWL_U32_BEFORE(ack->seq, ack_msg->seq)) {
			spin_lock_bh(&ack_info->lock);
			ack->seq = ack_msg->seq;
			if (unlikely(!ack_info->msgbuf)) {
				spin_unlock_bh(&ack_info->lock);
				return 0;
			}
			sprdwl_intf_tcp_drop_msg(ack_m->priv, ack_info->msgbuf);
			ack_info->msgbuf = NULL;
			ack_info->drop_cnt = 0;
			spin_unlock_bh(&ack_info->lock);
			if (timer_pending(&ack_info->timer))
				del_timer_sync(&ack_info->timer);
		}
		return 0;
	}
	if (SPRDWL_U32_BEFORE(ack->seq, ack_msg->seq)) {
		spin_lock_bh(&ack_info->lock);
		if (ack_info->msgbuf) {
			sprdwl_intf_tcp_drop_msg(ack_m->priv, ack_info->msgbuf);
			ack_info->msgbuf = NULL;
		}
		if (ack_info->psh_flag &&
		    !SPRDWL_U32_BEFORE(ack_msg->seq, ack_info->psh_seq)) {
			ack_info->drop_cnt = 0;
			ack_info->psh_flag = 0;
			quick_ack = 1;
		} else {
			ack_info->drop_cnt++;
		}
		ack->seq = ack_msg->seq;
		spin_unlock_bh(&ack_info->lock);
		if (quick_ack || ack_info->drop_cnt > tcp_ack_drop_cnt) {
			if (timer_pending(&ack_info->timer))
				del_timer_sync(&ack_info->timer);
			ack_info->drop_cnt = 0;
			return 0;
		}
		spin_lock_bh(&ack_info->lock);
		ack_info->msgbuf = new_msgbuf;
		spin_unlock_bh(&ack_info->lock);
		if (!timer_pending(&ack_info->timer))
			mod_timer(&ack_info->timer, jiffies + ack_m->drop_time);
		return 1;
	}

	return 0;
}

void sprdwl_fileter_rx_tcp_ack(unsigned char *buf)
{
	int index;
	struct sprdwl_tcp_ack_msg ack_msg;
	struct sprdwl_tcp_ack_manage *ack_m;
	struct sprdwl_tcp_ack_info *ack_info;

	ack_m = &sprdwl_ack_manage;

	if (!sprdwl_tcp_check_quick_ack(buf, &ack_msg))
		return;

	index = sprdwl_tcp_ack_match(ack_m, &ack_msg);
	if (index >= 0) {
		ack_info = ack_m->ack_info + index;
		spin_lock_bh(&ack_info->lock);
		ack_info->psh_flag = 1;
		ack_info->psh_seq = ack_msg.seq;
		spin_unlock_bh(&ack_info->lock);
	}
}

/* return val: 0 for not fileter, 1 for fileter */
int sprdwl_fileter_send_tcp_ack(struct sprdwl_msg_buf *msgbuf,
				unsigned char *buf)
{
	int ret = 0;
	int index, drop;
	struct sprdwl_tcp_ack_msg ack_msg;
	struct sprdwl_tcp_ack_msg *ack;
	struct sprdwl_tcp_ack_info *ack_info;
	struct sprdwl_tcp_ack_manage *ack_m;

	ack_m = &sprdwl_ack_manage;
	sprdwl_tcp_ack_updata(ack_m);
	drop = sprdwl_tcp_check_ack(buf, &ack_msg);
	if (!drop)
		return 0;
	if (unlikely(atomic_inc_return(&ack_m->ref) >=
	    SPRDWL_TCP_ACK_EXIT_VAL))
		goto out;

	index = sprdwl_tcp_ack_match(ack_m, &ack_msg);
	if (index >= 0) {
		ack_info = ack_m->ack_info + index;
		ack_info->last_time = jiffies;
		ret = sprdwl_tcp_ack_handle(msgbuf, ack_m, ack_info,
					    &ack_msg, drop);
		goto out;
	}

	index = sprdwl_tcp_ack_alloc_index(ack_m);
	if (index >= 0) {
		ack = &ack_m->ack_info[index].ack_msg;
		ack->dest = ack_msg.dest;
		ack->source = ack_msg.source;
		ack->saddr = ack_msg.saddr;
		ack->daddr = ack_msg.daddr;
		ack->seq = ack_msg.seq;
	}

out:
	atomic_dec(&ack_m->ref);

	return ret;
}
