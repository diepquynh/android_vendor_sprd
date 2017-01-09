
#ifndef __WCN_OP_H__
#define __WCN_OP_H__

struct wcn_op_attr_t {	
	unsigned long addr ;
	unsigned long val;
	int length;
};

int wcn_op_read(struct wcn_op_attr_t wcn_op_attr, unsigned int *pval);
int wcn_op_write(struct wcn_op_attr_t wcn_op_attr);
#endif

