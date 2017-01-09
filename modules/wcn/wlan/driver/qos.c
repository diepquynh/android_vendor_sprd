#include <uapi/linux/if_ether.h>
#include <uapi/linux/ip.h>
#include <linux/moduleparam.h>
#include <linux/spinlock.h>

#include "sprdwl.h"
#include "qos.h"

/* IPV6 field decodes */
#define IPV6_TRAFFIC_CLASS(ipv6_body) \
		(((((unsigned char *)(ipv6_body))[0] & 0x0f) << 4) | \
		((((unsigned char *)(ipv6_body))[1] & 0xf0) >> 4))

void sprdwl_qos_init(struct sprdwl_qos_t *qos, struct sprdwl_msg_list *list)
{
	memset(qos, 0, sizeof(struct sprdwl_qos_t));
	qos->head = &list->busylist;
	qos->last_node = &list->busylist;
	qos->txlist = list;
	qos->enable = 1;
}

int sprdwl_qos_map(unsigned char *frame)
{
	struct ethhdr *eh;
	struct iphdr *iph;
	unsigned char tos_tc;
	int index;

	eh = (struct ethhdr *)frame;
	if (eh->h_proto == htons(ETH_P_IP)) {
		iph = (struct iphdr *)(eh + 1);
		tos_tc = iph->tos;
	} else if (eh->h_proto == htons(ETH_P_IPV6)) {
		tos_tc = IPV6_TRAFFIC_CLASS(eh + 1);
	} else {
		return QOS_OTHER;
	}

	switch (tos_tc & 0xE0) {
	case 0x0:
	case 0x60:
		index = QOS_AC_BE;
		break;
	case 0x20:
	case 0x40:
		index = QOS_AC_BK;
		break;
	case 0x80:
	case 0xA0:
		index = QOS_AC_VI;
		break;
	default:
		index = QOS_AC_VO;
		break;
	}

	return index;
}

void sprdwl_qos_reorder(struct sprdwl_qos_t *qos)
{
	struct sprdwl_qos_list *list;
	struct sprdwl_msg_buf *msg_buf;
	struct list_head *end;
	struct list_head *pos;

	spin_lock_bh(&qos->txlist->busylock);
	end = qos->head->prev;
	spin_unlock_bh(&qos->txlist->busylock);
	for (pos = qos->last_node->next; pos != qos->head; pos = pos->next) {
		msg_buf = list_entry(pos, struct sprdwl_msg_buf, list);
		qos->num[msg_buf->index]++;
		qos->txnum++;
		qos->last_node = &msg_buf->list;
		msg_buf->next = NULL;
		list = &qos->list[msg_buf->index];
		if (list->head == NULL) {
			list->head = msg_buf;
			list->tail = msg_buf;
		} else {
			list->tail->next = msg_buf;
			list->tail = msg_buf;
		}
		if (pos == end)
			break;
	}
}

/* Qos weight should adjust at the time.
 * Not use timer, here just count for simple.
 * Maybe you can make it better here.
 */
#define SPRDWL_QOS_WRAP_COUNT 200
/* the queue send weight */
int sprdwl_weight[2][SPRDWL_QOS_NUM] = {
	{1, 1, 1, 1, 1},
	{1, 1, 3, 12, 48}
};

/* switch_buf: If switch send queue.
 *             We do our best to send the same qos msg in one sdio trans.
 *             if -1, a new sdio trans start.
 */
struct sprdwl_msg_buf *sprdwl_qos_peek_msg(struct sprdwl_qos_t *qos,
					   int *switch_buf)
{
	int i, j, min = 0;
	int first;

	if (*switch_buf != -1) {
		if (qos->num[*switch_buf]) {
			j = *switch_buf;
			goto same_index;
		}
	}
	for (i = 0, first = 1, j = -1; i < SPRDWL_QOS_NUM; i++) {
		if (qos->num[i]) {
			if (first) {
				first = 0;
				min = qos->cur_weight[i];
				j = i;
				*switch_buf = i;
			} else if (min > qos->cur_weight[i]) {
				min = qos->cur_weight[i];
				j = i;
				*switch_buf = i;
			}
		}
	}
same_index:
	if (j != -1) {
		qos->cur_weight[j] += sprdwl_weight[qos->enable][j];
		if (qos->num_wrap++ >= SPRDWL_QOS_WRAP_COUNT || qos->change) {
			memset(qos->cur_weight, 0, sizeof(qos->cur_weight));
			qos->num_wrap = 0;
			qos->change = 0;
		}
		return qos->list[j].head;
	}

	return NULL;
}

void sprdwl_qos_update(struct sprdwl_qos_t *qos,
		       struct sprdwl_msg_buf *msg_buf,
		       struct list_head *node)
{
	struct sprdwl_qos_list *list;

	list = &qos->list[msg_buf->index];
	list->head = msg_buf->next;
	qos->num[msg_buf->index]--;
	qos->txnum--;
	if (node == qos->last_node)
		qos->last_node = node->prev;
}

/* call it after sprdwl_qos_update */
void sprdwl_need_resch(struct sprdwl_qos_t *qos)
{
	if (!qos->txnum)
		sprdwl_qos_reorder(qos);
}
