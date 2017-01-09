#ifndef __SPRDWL_QOS_H__
#define __SPRDWL_QOS_H__

#define SPRDWL_QOS_NUM 5

/* the pkt send order */
enum sprdwl_qos_index {
	/* other pkts queue (such as arp etc which is not ip packets */
	QOS_OTHER = 0,
	/* AC_VI_Q queue */
	QOS_AC_VO = 1,
	/* AC_VO_Q queue */
	QOS_AC_VI = 2,
	/* AC_BE_Q queue */
	QOS_AC_BE = 3,
	/* AC_BK_Q queue */
	QOS_AC_BK = 4
};

struct sprdwl_msg_list;
struct sprdwl_msg_buf;

struct sprdwl_qos_list {
	struct sprdwl_msg_buf *head;
	struct sprdwl_msg_buf *tail;
};

struct sprdwl_qos_t {
	struct list_head *head;
	struct list_head *last_node;
	struct sprdwl_msg_list *txlist;
	int enable;
	int change;
	int num_wrap;
	int txnum;
	/* 0 for not qos data, [1, 4] for qos data */
	int cur_weight[SPRDWL_QOS_NUM];
	int num[SPRDWL_QOS_NUM];
	struct sprdwl_qos_list list[SPRDWL_QOS_NUM];
};

void sprdwl_qos_init(struct sprdwl_qos_t *qos,
		     struct sprdwl_msg_list *list);
int sprdwl_qos_map(unsigned char *frame);
void sprdwl_qos_reorder(struct sprdwl_qos_t *qos);
struct sprdwl_msg_buf *sprdwl_qos_peek_msg(struct sprdwl_qos_t *qos,
					   int *switch_buf);
void sprdwl_qos_update(struct sprdwl_qos_t *qos,
		       struct sprdwl_msg_buf *msg_buf,
		       struct list_head *node);
void sprdwl_need_resch(struct sprdwl_qos_t *qos);
#endif
