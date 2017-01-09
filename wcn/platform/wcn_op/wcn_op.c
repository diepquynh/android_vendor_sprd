#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <errno.h>  
#include <sys/ioctl.h>
#include <net/if.h>	
#include <sys/time.h>
#include "wcn_op.h"

#define  IOCTL_WCN_OP_READ		0xFF01
#define  IOCTL_WCN_OP_WRITE		0xFF02

static int wcn_op_fd = -1;

int get_wcn_op_fd(void)
{
	if(wcn_op_fd < 0) {

		wcn_op_fd = open("/dev/wcn_op",O_RDWR);
		if(wcn_op_fd<0) {
			printf("open /dev/wcn_op failed!\r\n");
		}
	}
	return wcn_op_fd;
}

int wcn_op_read(struct wcn_op_attr_t wcn_op_attr, unsigned int *pval)
{
	int ret = -1;
	
	if(get_wcn_op_fd() < 0) {
		return ret;
	}
	
	ret = ioctl(wcn_op_fd, IOCTL_WCN_OP_READ, &wcn_op_attr);
	if(ret != 0){
		printf("wcn_op ioctl read err!\r\n");
	}
	*pval = wcn_op_attr.val;
	
	return ret;
}

int wcn_op_write(struct wcn_op_attr_t wcn_op_attr)
{
	int ret = -1;
	
	if(get_wcn_op_fd() < 0) {
		return ret;
	}
	
	ret = ioctl(wcn_op_fd,IOCTL_WCN_OP_WRITE,&wcn_op_attr);
	if(ret != 0){
		printf("wcn_op ioctl write err!\r\n");
	}	
	
	return ret;
}
