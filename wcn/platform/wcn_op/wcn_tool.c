/*
 * WCN debug program - By wcn_tool, to read/write wcn register from ap side 
 * Author: sam.sun
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "wcn_op.h"


int main(int argc,char *argv[])
{
	int ret = 0;
	unsigned long addr = 0;
	int length = 0;
	unsigned long tmp_val;
	unsigned long tmp_reg = 0;
	char *string;
	int reg_cnt = 0,i=0;
	struct wcn_op_attr_t wcn_op_attr;
	
	
	if(argc < 4) {
		printf("Usage:	wcn_tool <read> <addr> <length> \n");
		printf("		<write> <addr> <length> <value> <value1> ... \n");
		printf("Note:	length means the number of bytes\n");
		return -1;	
	}
	
	string = argv[2];
	addr = strtoul(string, NULL, 16);
	string = argv[3];
	length = strtoul(string, NULL, 0);
	
	if(length%4 != 0) {
		printf("length is invalid,and it means the num of bytes\n");
		return -1;
	}
	reg_cnt = length/4;
	
	wcn_op_attr.addr = addr;
	wcn_op_attr.val = tmp_val;
	wcn_op_attr.length = 4;
 	if(strcmp(argv[1],"write") == 0)
    {
        if(argc < 5) {
            printf("wcn_tool <write> <addr> <length> <value> \n\n");
			
            return -1;
        }
		
        if(argc != (reg_cnt + 4)) {
            printf("length is not match with value number\n");
			
            return -1;
        }

		for(i=0; i<reg_cnt; i++) {
			string = argv[4+i];
			tmp_val = strtoul(string, NULL, 0);

			printf("addr:0x%x, value:0x%x\n",wcn_op_attr.addr,tmp_val);
			
			wcn_op_attr.val = tmp_val;
			ret = wcn_op_write(wcn_op_attr);
			if(ret != 0)
				return -1;
			(wcn_op_attr.addr) += sizeof(unsigned long);
		}
		
    } else if (strcmp(argv[1],"read") == 0) {

		if(argc > 4) {
            printf("wcn_tool <read> <addr> <length> \n\n");
			
            return -1;
        }
		
		for(; reg_cnt!=0; reg_cnt--) {
			ret = wcn_op_read(wcn_op_attr, &(wcn_op_attr.val));
			if(ret != 0)
				return -1;
			printf("addr:0x%x, value:0x%x\n",wcn_op_attr.addr,wcn_op_attr.val);	
			(wcn_op_attr.addr) += sizeof(unsigned long);
		}
		
    }
	
	return 0;
}
